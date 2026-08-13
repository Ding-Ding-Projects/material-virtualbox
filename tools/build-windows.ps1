# $Id$
# Windows build and unsigned Squirrel.Windows packaging helper.
#
# Copyright (C) 2026 Oracle and/or its affiliates.
# SPDX-License-Identifier: GPL-3.0-only

[CmdletBinding()]
param(
    [ValidateSet('Build', 'Installer')]
    [string] $Mode = 'Build',
    [switch] $Silent
)

$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot
Set-Location -LiteralPath $repoRoot

if ($env:SILENT -eq '1') {
    $Silent = $true
}

$toolRoot = Join-Path $env:LOCALAPPDATA 'VirtualBox\build-tools'
$dependencyRoot = Join-Path $toolRoot 'dependencies'
$downloadRoot = Join-Path $toolRoot 'downloads'
New-Item -ItemType Directory -Force $toolRoot, $dependencyRoot, $downloadRoot | Out-Null

function Invoke-Checked {
    param(
        [Parameter(Mandatory = $true)] [string] $Name,
        [Parameter(Mandatory = $true)] [scriptblock] $Action
    )
    $timer = [Diagnostics.Stopwatch]::StartNew()
    Write-Host "==> $Name"
    # Send the action's own output straight to the host.  Anything left on the
    # success stream would otherwise be returned by whichever Ensure-* function
    # invoked this, turning a single path string into an array.
    & $Action | Out-Host
    if ($LASTEXITCODE -ne 0) {
        throw "$Name failed with exit code $LASTEXITCODE."
    }
    $timer.Stop()
    Write-Host ("    completed in {0:n1}s" -f $timer.Elapsed.TotalSeconds)
}

function Refresh-ProcessPath {
    $machinePath = [Environment]::GetEnvironmentVariable('Path', 'Machine')
    $userPath = [Environment]::GetEnvironmentVariable('Path', 'User')
    if ($machinePath -and $userPath) {
        $env:Path = "$machinePath;$userPath"
    }
}

function Get-Winget {
    $command = Get-Command winget.exe -ErrorAction SilentlyContinue
    if (-not $command) {
        throw 'winget.exe is unavailable. Install the App Installer package from the Microsoft Store, then rerun this script; no manual toolchain changes were made.'
    }
    return $command.Path
}

function Ensure-WingetPackage {
    param([Parameter(Mandatory = $true)] [string] $Id)
    $winget = Get-Winget
    Write-Host "Installing missing dependency $Id through the canonical package source."
    & $winget install --id $Id --scope user --accept-package-agreements --accept-source-agreements --silent | Out-Host
    if ($LASTEXITCODE -ne 0) {
        throw "winget could not install $Id (exit code $LASTEXITCODE)."
    }
    Refresh-ProcessPath
}

function Ensure-SevenZip {
    $command = Get-Command 7z.exe -ErrorAction SilentlyContinue
    if ($command) { return $command.Path }
    $candidates = @(
        (Join-Path ${env:ProgramFiles} '7-Zip\7z.exe'),
        (Join-Path ${env:ProgramFiles(x86)} '7-Zip\7z.exe')
    ) | Where-Object { $_ -and (Test-Path -LiteralPath $_) }
    $candidate = $candidates | Select-Object -First 1
    if ($candidate) { return $candidate }
    Ensure-WingetPackage '7zip.7zip'
    $command = Get-Command 7z.exe -ErrorAction SilentlyContinue
    if ($command) { return $command.Path }
    $candidate = @(
        (Join-Path ${env:ProgramFiles} '7-Zip\7z.exe'),
        (Join-Path ${env:ProgramFiles(x86)} '7-Zip\7z.exe')
    ) | Where-Object { $_ -and (Test-Path -LiteralPath $_) } | Select-Object -First 1
    if ($candidate) { return $candidate }
    throw '7-Zip is still unavailable after the canonical user-scoped install attempt.'
}

function Get-RequiredPython {
    $python = Get-Command python.exe -ErrorAction SilentlyContinue
    if ($python) {
        $version = (& $python.Path --version 2>&1 | Out-String).Trim()
        if ($version -match '^Python 3\.') {
            return $python.Path
        }
    }
    $launcher = Get-Command py.exe -ErrorAction SilentlyContinue
    if ($launcher) {
        $candidate = (& $launcher.Path -3 -c "import sys; print(sys.executable)" 2>$null | Select-Object -First 1).Trim()
        if ($candidate -and (Test-Path -LiteralPath $candidate)) {
            return $candidate
        }
    }
    Ensure-WingetPackage 'Python.Python.3.12'
    $python = Get-Command python.exe -ErrorAction SilentlyContinue
    if (-not $python) {
        throw 'Python 3.12 is still unavailable after the canonical user-scoped install attempt.'
    }
    $version = (& $python.Path --version 2>&1 | Out-String).Trim()
    if ($version -notmatch '^Python 3\.') {
        throw "Python 3 is required; the discovered interpreter reported: $version"
    }
    return $python.Path
}

function Get-ShortPath {
    param([Parameter(Mandatory = $true)] [string] $Path)
    if (-not ('VirtualBoxShortPath' -as [type])) {
        Add-Type -TypeDefinition @'
using System;
using System.Text;
using System.Runtime.InteropServices;
public static class VirtualBoxShortPath
{
    [DllImport("kernel32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
    private static extern uint GetShortPathName(string longPath, StringBuilder shortPath, int shortPathLength);
    public static string Convert(string path)
    {
        var buffer = new StringBuilder(32768);
        var length = GetShortPathName(path, buffer, buffer.Capacity);
        return length == 0 || length >= buffer.Capacity ? path : buffer.ToString();
    }
}
'@
    }
    return ([VirtualBoxShortPath]::Convert($Path)).Replace('\', '/')
}

function Get-DownloadedFile {
    param(
        [Parameter(Mandatory = $true)] [string] $Name,
        [Parameter(Mandatory = $true)] [string] $Uri,
        [Parameter(Mandatory = $true)] [string] $Sha256
    )
    $path = Join-Path $downloadRoot $Name
    if (-not (Test-Path -LiteralPath $path)) {
        Invoke-WebRequest -UseBasicParsing -Uri $Uri -OutFile $path
    }
    $actual = (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash
    if ($actual -ne $Sha256) {
        throw "SHA-256 mismatch for ${Name}: expected $Sha256, got $actual."
    }
    return $path
}

function Ensure-WindowsKits {
    $kitsRoot = 'C:\Program Files (x86)\Windows Kits\10'
    $sdkInclude = Join-Path $kitsRoot 'Include'
    if (-not (Test-Path -LiteralPath $sdkInclude)) {
        $sdk = Get-DownloadedFile `
            -Name 'winsdksetup.exe' `
            -Uri 'https://download.microsoft.com/download/3/b/d/3bd97f81-3f5b-4922-b86d-dc5145cd6bfe/windowssdk/winsdksetup.exe' `
            -Sha256 '3F73F59566B0CF3EDDDDAF61AD72BB0C6E4588A5D9E004ABF68115B752EBBBD8'
        $process = Start-Process -FilePath $sdk -ArgumentList '/quiet', '/norestart' -Wait -PassThru -WindowStyle Hidden
        if ($process.ExitCode -ne 0) { throw "Windows SDK installation failed with exit code $($process.ExitCode)." }
    }
    if (-not (Test-Path -LiteralPath $sdkInclude)) {
        throw "Windows SDK installation did not create $sdkInclude."
    }
    $wdkInclude = Join-Path $kitsRoot 'Include'
    $wdkInstalled = Get-ChildItem -LiteralPath $wdkInclude -Directory -ErrorAction SilentlyContinue |
        Where-Object { Test-Path -LiteralPath (Join-Path $_.FullName 'km') } |
        Select-Object -First 1
    if (-not $wdkInstalled) {
        $wdk = Get-DownloadedFile `
            -Name 'wdksetup.exe' `
            -Uri 'https://download.microsoft.com/download/7/b/f/7bfc8dbe-00cb-47de-b856-70e696ef4f46/wdk/wdksetup.exe' `
            -Sha256 'A891543C0EAF610757EE7852F515C5CE89D0202F22A15DFA31C4D49F2BAFDF27'
        $process = Start-Process -FilePath $wdk -ArgumentList '/quiet', '/norestart' -Wait -PassThru -WindowStyle Hidden
        if ($process.ExitCode -ne 0) { throw "Windows Driver Kit installation failed with exit code $($process.ExitCode)." }
    }
    $wdk71CacheRoot = Join-Path $dependencyRoot 'winddk71'
    $wdk71Marker = Get-ChildItem $wdk71CacheRoot -Recurse -Filter 'rxce.lib' -File -ErrorAction SilentlyContinue |
        Where-Object { $_.FullName -match '\\lib\\wlh\\amd64\\rxce\.lib$' } | Select-Object -First 1
    if (-not $wdk71Marker) {
        $wdk71Iso = Get-DownloadedFile `
            -Name 'GRMWDK_EN_7600_1.ISO' `
            -Uri 'https://download.microsoft.com/download/4/a/2/4a25c7d5-efbe-4182-b6a9-ae6850409a78/GRMWDK_EN_7600_1.ISO' `
            -Sha256 '5EDC723B50EA28A070CAD361DD0927DF402B7A861A036BBCF11D27EBBA77657D'
        # An /a administrative install is carried out by the out-of-process Windows
        # Installer service, so the service itself has to be able to open the package.
        # Stage the extracted packages under the OS temporary directory, which the
        # service can always read, rather than inside the tool cache, which may carry
        # restrictive inherited permissions.  Only the extraction output is cached.
        $wdk71Packages = Join-Path ([IO.Path]::GetTempPath()) ('vbox-wdk71-' + [guid]::NewGuid().ToString('N'))
        New-Item -ItemType Directory -Force $wdk71Packages | Out-Null
        try {
            $sevenZip = Ensure-SevenZip
            & $sevenZip x $wdk71Iso 'WDK\headers.msi' 'WDK\headers_cab001.cab' 'WDK\vistalibs_x64fre.msi' 'WDK\vistalibs_x64fre_cab001.cab' 'WDK\wnetlibs_x64fre.msi' 'WDK\wnetlibs_x64fre_cab001.cab' "-o$wdk71Packages" '-y' | Out-Host
            if ($LASTEXITCODE -ne 0) { throw "WDK 7.1 package extraction failed with exit code $LASTEXITCODE." }
            New-Item -ItemType Directory -Force $wdk71CacheRoot | Out-Null
            foreach ($packageName in @('headers.msi', 'vistalibs_x64fre.msi', 'wnetlibs_x64fre.msi')) {
                $packagePath = Join-Path $wdk71Packages "WDK\$packageName"
                if (-not (Test-Path -LiteralPath $packagePath)) { throw "WDK 7.1 package $packageName was not extracted to $packagePath." }
                $install = Start-Process -FilePath 'msiexec.exe' -ArgumentList @('/a', $packagePath, "TARGETDIR=$wdk71CacheRoot", '/qn', '/norestart') -Wait -PassThru -WindowStyle Hidden
                if ($install.ExitCode -ne 0) { throw "WDK 7.1 package $packageName extraction from $packagePath failed with exit code $($install.ExitCode)." }
            }
        } finally {
            Remove-Item -LiteralPath $wdk71Packages -Recurse -Force -ErrorAction SilentlyContinue
        }
        $wdk71Marker = Get-ChildItem $wdk71CacheRoot -Recurse -Filter 'rxce.lib' -File -ErrorAction SilentlyContinue |
            Where-Object { $_.FullName -match '\\lib\\wlh\\amd64\\rxce\.lib$' } | Select-Object -First 1
    }
    if (-not $wdk71Marker) { throw 'WDK 7.1 extraction did not provide lib\wlh\amd64\rxce.lib.' }
    $script:Wdk71Root = $wdk71Marker.Directory.Parent.Parent.Parent.FullName
    $shortKits = Get-ShortPath $kitsRoot
    return $shortKits
}

function Ensure-Vcpkg {
    $command = Get-Command vcpkg.exe -ErrorAction SilentlyContinue
    if ($command) {
        return (Split-Path -Parent $command.Path)
    }
    $vcpkgRoot = Join-Path $toolRoot 'vcpkg'
    $vcpkgExe = Join-Path $vcpkgRoot 'vcpkg.exe'
    if (-not (Test-Path -LiteralPath $vcpkgExe)) {
        $git = Get-Command git.exe -ErrorAction SilentlyContinue
        if (-not $git) { throw 'git.exe is required to obtain vcpkg from its canonical upstream.' }
        if (-not (Test-Path -LiteralPath $vcpkgRoot)) {
            & $git.Path clone --depth 1 https://github.com/microsoft/vcpkg.git $vcpkgRoot | Out-Host
            if ($LASTEXITCODE -ne 0) { throw "vcpkg clone failed with exit code $LASTEXITCODE." }
        }
        Push-Location $vcpkgRoot
        try {
            & .\bootstrap-vcpkg.bat -disableMetrics | Out-Host
            if ($LASTEXITCODE -ne 0) { throw "vcpkg bootstrap failed with exit code $LASTEXITCODE." }
        } finally {
            Pop-Location
        }
    }
    if (-not (Test-Path -LiteralPath $vcpkgExe)) { throw 'vcpkg bootstrap did not produce vcpkg.exe.' }
    return $vcpkgRoot
}

function Ensure-Nasm {
    $nasm = Get-Command nasm.exe -ErrorAction SilentlyContinue
    if ($nasm) { return (Split-Path -Parent $nasm.Path) }
    $version = '2.16.01'
    $zip = Join-Path $downloadRoot "nasm-$version-win64.zip"
    $root = Join-Path $toolRoot "nasm-$version"
    if (-not (Test-Path -LiteralPath $root)) {
        Invoke-WebRequest -UseBasicParsing -Uri "https://www.nasm.us/pub/nasm/releasebuilds/$version/win64/nasm-$version-win64.zip" -OutFile $zip
        Expand-Archive -LiteralPath $zip -DestinationPath $root -Force
    }
    $nasm = Get-ChildItem -LiteralPath $root -Recurse -Filter nasm.exe -File | Select-Object -First 1
    if (-not $nasm) { throw 'NASM bootstrap did not provide nasm.exe.' }
    return (Get-ShortPath $nasm.DirectoryName)
}

function Ensure-Zip {
    $zip = Get-Command zip.exe -ErrorAction SilentlyContinue
    if ($zip) { return (Split-Path -Parent $zip.Path) }
    $candidates = @(
        (Join-Path ${env:ProgramFiles} 'Git\usr\bin\zip.exe'),
        (Join-Path ${env:ProgramFiles(x86)} 'Git\usr\bin\zip.exe')
    ) | Where-Object { $_ -and (Test-Path -LiteralPath $_) }
    $candidate = $candidates | Select-Object -First 1
    if ($candidate) { return (Get-ShortPath (Split-Path -Parent $candidate)) }
    $sevenZip = Ensure-SevenZip
    $archive = Join-Path $downloadRoot 'miktex-zip-bin-x64.tar.lzma'
    if (-not (Test-Path -LiteralPath $archive)) {
        Invoke-WebRequest -UseBasicParsing `
            -Uri 'https://ftp.fau.de/ctan/systems/win32/miktex/tm/packages/miktex-zip-bin-x64.tar.lzma' `
            -OutFile $archive
    }
    $archiveInfo = Get-Item -LiteralPath $archive
    if ($archiveInfo.Length -lt 100000) { throw "Info-ZIP bootstrap archive is unexpectedly small: $($archiveInfo.Length) bytes." }
    $archiveHash = (Get-FileHash -LiteralPath $archive -Algorithm SHA256).Hash.ToLowerInvariant()
    Write-Host "Info-ZIP bootstrap archive SHA-256=$archiveHash ($($archiveInfo.Length) bytes)."
    $installRoot = Join-Path $toolRoot 'miktex-zip-bin-x64'
    $zip = Get-ChildItem -LiteralPath $installRoot -Recurse -Filter zip.exe -File -ErrorAction SilentlyContinue | Select-Object -First 1
    if (-not $zip) {
        New-Item -ItemType Directory -Force $installRoot | Out-Null
        $zipOutput = '-o' + $installRoot
        & $sevenZip x $archive $zipOutput '-y' | Out-Host
        if ($LASTEXITCODE -ne 0) { throw "Info-ZIP LZMA extraction failed with exit code $LASTEXITCODE." }
        $tarArchive = Get-ChildItem -LiteralPath $installRoot -Filter '*.tar' -File | Select-Object -First 1
        if (-not $tarArchive) { throw 'Info-ZIP bootstrap did not produce its tar payload.' }
        & $sevenZip t $tarArchive.FullName | Out-Host
        if ($LASTEXITCODE -ne 0) { throw "Info-ZIP tar payload validation failed with exit code $LASTEXITCODE." }
        & $sevenZip x $tarArchive.FullName $zipOutput '-y' | Out-Host
        if ($LASTEXITCODE -ne 0) { throw "Info-ZIP tar payload extraction failed with exit code $LASTEXITCODE." }
        $zipBinary = Get-ChildItem -LiteralPath $installRoot -Recurse -Filter miktex-zip.exe -File -ErrorAction SilentlyContinue | Select-Object -First 1
        if ($zipBinary) {
            $zipHash = (Get-FileHash -LiteralPath $zipBinary.FullName -Algorithm SHA256).Hash.ToLowerInvariant()
            if ($zipHash -ne 'f52e7f6a0a01a0e70443dfecbbe9e3d42d0bd80dd295ec6acc234b0971e0a3fd') {
                throw "Info-ZIP payload SHA-256 mismatch: got $zipHash."
            }
            $zipPath = Join-Path $installRoot 'zip.exe'
            Copy-Item -LiteralPath $zipBinary.FullName -Destination $zipPath -Force
            $zip = Get-Item -LiteralPath $zipPath
        }
    }
    if ($zip) { return (Get-ShortPath $zip.DirectoryName) }
    throw 'zip.exe is required for the Validation Kit package and the verified Info-ZIP package did not provide it.'
}

function Ensure-WinFlexBison {
    $version = '2.5.24'
    $target = Join-Path $repoRoot 'tools\win.x86\win_flex_bison\v3.7.4'
    $bison = Join-Path $target 'win_bison.exe'
    $flex = Join-Path $target 'win_flex.exe'
    if ((Test-Path -LiteralPath $bison) -and (Test-Path -LiteralPath $flex)) {
        return $target
    }
    $zip = Get-DownloadedFile `
        -Name "win_flex_bison-$version.zip" `
        -Uri "https://github.com/lexxmark/winflexbison/releases/download/v$version/win_flex_bison-$version.zip" `
        -Sha256 '39C6086CE211D5415500ACC5ED2D8939861CA1696AEE48909C7F6DAF5122B505'
    $root = Join-Path $toolRoot "win_flex_bison-$version"
    New-Item -ItemType Directory -Force $root | Out-Null
    Expand-Archive -LiteralPath $zip -DestinationPath $root -Force
    $bisonFile = Get-ChildItem -LiteralPath $root -Recurse -Filter win_bison.exe -File | Select-Object -First 1
    if (-not $bisonFile -or -not (Test-Path (Join-Path $bisonFile.DirectoryName 'win_flex.exe'))) {
        throw 'WinFlexBison bootstrap did not provide win_bison.exe and win_flex.exe.'
    }
    New-Item -ItemType Directory -Force $target | Out-Null
    Copy-Item (Join-Path $bisonFile.DirectoryName '*') $target -Recurse -Force
    if (-not (Test-Path -LiteralPath $bison) -or -not (Test-Path -LiteralPath $flex)) {
        throw 'WinFlexBison was not materialized under tools\win.x86\win_flex_bison\v3.7.4.'
    }
    return $target
}

function Ensure-Nsis {
    $target = Join-Path $repoRoot 'tools\win.x86\nsis\v3.10-log-r1'
    $required = @(
        (Join-Path $target 'makensis.exe'),
        (Join-Path $target 'Include\nsProcess.nsh'),
        (Join-Path $target 'Plugins\x86-unicode\nsProcess.dll'),
        (Join-Path $target 'Plugins\x86-unicode\AccessControl.dll'),
        (Join-Path $target 'VirtualBox-NSIS_CONFIG_LOG.txt')
    )
    if (@($required | Where-Object { -not (Test-Path -LiteralPath $_) }).Count -eq 0) {
        return $target
    }
    $environmentFile = Join-Path $repoRoot 'env.bat'
    if (-not (Test-Path -LiteralPath $environmentFile)) {
        throw 'The VirtualBox environment must be configured before the custom NSIS tool can be built.'
    }
    $prepare = Join-Path $repoRoot 'tools\prepare-nsis.ps1'
    $powerShell = Get-Command pwsh.exe -ErrorAction SilentlyContinue
    if (-not $powerShell) { $powerShell = Get-Command powershell.exe -ErrorAction Stop }
    Invoke-Checked 'Build custom NSIS 3.10 with log support' {
        & $powerShell.Source -NoLogo -NoProfile -ExecutionPolicy Bypass -File $prepare -EnvironmentFile $environmentFile
    }
    if (@($required | Where-Object { -not (Test-Path -LiteralPath $_) }).Count -gt 0) {
        throw 'The custom NSIS source build did not materialize the complete log-enabled package.'
    }
    return $target
}

function Get-PreferredVisualCppRoot {
    # The tree is built and released with the Visual Studio 2022 toolset.  MSVC
    # 14.5x (Visual Studio 2026) diverges from it in ways this source does not
    # accommodate yet: it rejects IPRT's no-CRT definitions of intrinsic
    # functions (C2169) and raises new warnings that -Wall -WX turns into
    # errors.  Prefer a complete 17.x installation so a local build uses the
    # same compiler as the hosted runners, and fall back to configure.py's own
    # newest-installation probe when no such toolset is present.
    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
    if (-not (Test-Path -LiteralPath $vswhere)) { return $null }
    $installs = @(& $vswhere -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 `
            -version '[17.0,18.0)' -property installationPath 2>$null |
        ForEach-Object { $_.Trim() } | Where-Object { $_ })
    foreach ($install in $installs) {
        $toolsets = @(Get-ChildItem -LiteralPath (Join-Path $install 'VC\Tools\MSVC') -Directory -ErrorAction SilentlyContinue |
            Sort-Object Name -Descending)
        foreach ($toolset in $toolsets) {
            $compiler = Join-Path $toolset.FullName 'bin\Hostx64\x64\cl.exe'
            $runtime = Join-Path $toolset.FullName 'lib\x64\libvcruntime.lib'
            if ((Test-Path -LiteralPath $compiler) -and (Test-Path -LiteralPath $runtime)) {
                Write-Host "Using the Visual Studio 2022 toolset at $install for the VirtualBox build."
                return $install
            }
        }
    }
    return $null
}

function Ensure-Qt {
    param([Parameter(Mandatory = $true)] [string] $Python)
    $qtRoot = Join-Path $dependencyRoot 'virtualbox-qt'
    $qmake = Get-ChildItem -LiteralPath $qtRoot -Recurse -Filter qmake.exe -File -ErrorAction SilentlyContinue | Select-Object -First 1
    if (-not $qmake) {
        Invoke-Checked 'Install aqtinstall' { & $Python -m pip install --disable-pip-version-check --user aqtinstall }
        Invoke-Checked 'Install Qt 6.8.3 MSVC 2022' { & $Python -m aqt install-qt windows desktop 6.8.3 win64_msvc2022_64 --outputdir $qtRoot }
        $qmake = Get-ChildItem -LiteralPath $qtRoot -Recurse -Filter qmake.exe -File | Select-Object -First 1
    }
    if (-not $qmake) { throw 'Qt installation did not provide qmake.exe.' }
    return (Split-Path -Parent (Split-Path -Parent $qmake.FullName))
}

function Ensure-MesaPython {
    param([Parameter(Mandatory = $true)] [string] $Python)
    Invoke-Checked 'Install Mesa Mako build dependency' {
        & $Python -m pip install --disable-pip-version-check --user 'mako==1.3.10'
    }
    Invoke-Checked 'Verify Mesa Mako build dependency' {
        & $Python -c "import mako; assert tuple(int(x) for x in mako.__version__.split('.')[:2]) >= (0, 8); print('Mako ' + mako.__version__)"
    }
}

function Ensure-Squirrel {
    $nuget = Join-Path $toolRoot 'nuget.exe'
    if (-not (Test-Path -LiteralPath $nuget)) {
        Invoke-WebRequest -UseBasicParsing -Uri 'https://dist.nuget.org/win-x86-commandline/v6.11.1/nuget.exe' -OutFile $nuget
    }
    $squirrelRoot = Join-Path $toolRoot 'squirrel'
    # The NuGet package is "squirrel.windows"; a package named "Squirrel" does not
    # exist.  -ExcludeVersion drops the version from the directory but keeps the
    # package id, so the tool lands in <root>\squirrel.windows\tools.
    $squirrel = Join-Path $squirrelRoot 'squirrel.windows\tools\Squirrel.exe'
    if (-not (Test-Path -LiteralPath $squirrel)) {
        & $nuget install squirrel.windows -Version 1.9.1 -OutputDirectory $squirrelRoot -ExcludeVersion -NonInteractive | Out-Host
        if ($LASTEXITCODE -ne 0) { throw "NuGet Squirrel installation failed with exit code $LASTEXITCODE." }
    }
    if (-not (Test-Path -LiteralPath $squirrel)) { throw 'NuGet did not provide Squirrel.exe.' }
    return @{ NuGet = $nuget; Squirrel = $squirrel }
}

function Invoke-VirtualBoxBuild {
    param(
        [Parameter(Mandatory = $true)] [string] $Python,
        [Parameter(Mandatory = $true)] [string] $QtRoot,
        [Parameter(Mandatory = $true)] [string] $SdkRoot,
        [Parameter(Mandatory = $true)] [string] $Wdk71Root,
        [Parameter(Mandatory = $true)] [string] $VcpkgRoot,
        [Parameter(Mandatory = $true)] [string] $NasmRoot,
        [Parameter(Mandatory = $true)] [string] $ZipRoot
    )
    $env:VBOX_SIGNING_MODE = ''
    $env:VBOX_WITHOUT_HARDENING = '1'
    $env:VBOX_OSE = '1'
    $env:VBOX_CI_QT_ROOT = $QtRoot
    $env:VBOX_CI_WINDOWS_SDK_ROOT = $SdkRoot
    $env:PATH_SDK_WINDDK71 = $Wdk71Root
    $env:VBOX_CI_VCPKG_ROOT = $VcpkgRoot
    $env:VCPKG_ROOT = $VcpkgRoot
    $env:Path = "$ZipRoot;$NasmRoot;$env:Path"
    $pythonRoot = (Split-Path -Parent $Python).Replace('\', '/')
    $env:Path = "$pythonRoot;$env:Path"
    $arguments = @(
        '--disable-hardening', '--disable-python_c_api', '--disable-win-ddk',
        '--disable-win-msi', '--disable-win-wix',
        "--with-kbuild-path=$($repoRoot.Replace('\', '/'))/kBuild/kBuild",
        "--with-qt-path=$QtRoot", "--with-sdk10=$SdkRoot",
        "--with-win-vcpkg-root=$VcpkgRoot", "--with-python-path=$pythonRoot"
    )
    $visualCppRoot = Get-PreferredVisualCppRoot
    if ($visualCppRoot) { $arguments += "--with-vc=$visualCppRoot" }
    # configure.ps1 resolves "python3" ahead of "python", and on a default Windows
    # install python3.exe is the Microsoft Store app-execution alias rather than an
    # interpreter: it writes an advertisement to stderr and exits non-zero.  This
    # script already discovered and version-checked a real Python 3, so drive
    # configure.py with that interpreter instead of re-running the wrapper's search.
    Invoke-Checked 'Configure the unsigned Windows build' { & $Python configure.py @arguments }
    if (-not (Test-Path -LiteralPath .\env.bat)) { throw 'configure.py did not generate env.bat.' }
    $null = Ensure-Nsis
    $revisionMatch = Select-String configure.py -Pattern '\$Id: configure.py (\d+)'
    if (-not $revisionMatch) { throw 'configure.py does not expose a numeric source revision for the Git mirror build.' }
    $revision = $revisionMatch.Matches[0].Groups[1].Value
    $revisionFile = Join-Path $repoRoot 'out\win.amd64\release\revision.kmk'
    New-Item -ItemType Directory -Force -Path (Split-Path -Parent $revisionFile) | Out-Null
    Set-Content -LiteralPath $revisionFile -Value "export VBOX_SVN_REV=$revision" -Encoding ascii
    Invoke-Checked 'Stage OpenSSL headers' {
        & cmd.exe /d /c "call `"$repoRoot\env.bat`" && kmk crypto-headers"
    }
    Invoke-Checked 'Build the Windows host binaries' {
        # A plain "kmk" pass (no explicit target) must run before the "packing"
        # pass below.  "packing" only runs VirtualBox's packing/staging rules; it
        # does not itself compile the host binaries (VirtualBox.exe, VBoxSVC.exe,
        # VBoxManage.exe, the Qt frontend, and the rest of
        # out\win.amd64\release\bin).  Running "kmk ... packing" alone against a
        # cold checkout leaves those binaries unbuilt and the payload directory
        # missing VirtualBox.exe.
        & cmd.exe /d /c "call `"$repoRoot\env.bat`" && kmk VBOX_SVN_REV=$revision SDK_WINSDK10_MAX_VERSION=10.0.22621.0 VBOX_WINDDK_GST_W7=WINSDK10-KM VBOX_WINDDK_GST_W8=WINSDK10-KM VBOX_WINDDK_GST_WLH=WINDDK71WLH VBOX_WINDDK_GST_W2K3=WINSDK10-KM VBOX_WINDDK_GST_WXP=WINSDK10-KM VBOX_WINDDK_GST_W2K=WINSDK10-KM VBOX_WINDDK_GST_NT4=WINSDK10-KM VBOX_USE_RTISOMAKER=1 VBOX_WITHOUT_WIN_HOST_INSTALLER=1"
    }
    Invoke-Checked 'Build the Windows package payload' {
        # VBOX_WITHOUT_WIN_HOST_INSTALLER skips the WiX/MSI host installer during
        # "packing".  This package ships the unsigned Squirrel installer built from
        # out\win.amd64\release\bin further down, not the traditional VirtualBox MSI,
        # so the MSI would need the WiX toolset nothing installs and would produce an
        # artifact this pipeline never publishes.  The host binaries themselves were
        # already built by the full pass above; this pass only packs them.
        & cmd.exe /d /c "call `"$repoRoot\env.bat`" && kmk VBOX_SVN_REV=$revision SDK_WINSDK10_MAX_VERSION=10.0.22621.0 VBOX_WINDDK_GST_W7=WINSDK10-KM VBOX_WINDDK_GST_W8=WINSDK10-KM VBOX_WINDDK_GST_WLH=WINDDK71WLH VBOX_WINDDK_GST_W2K3=WINSDK10-KM VBOX_WINDDK_GST_WXP=WINSDK10-KM VBOX_WINDDK_GST_W2K=WINSDK10-KM VBOX_WINDDK_GST_NT4=WINSDK10-KM VBOX_USE_RTISOMAKER=1 VBOX_WITHOUT_WIN_HOST_INSTALLER=1 packing"
    }
    $payload = Join-Path $repoRoot 'out\win.amd64\release\bin'
    if (-not (Test-Path -LiteralPath (Join-Path $payload 'VirtualBox.exe'))) {
        throw "The build completed without $payload\VirtualBox.exe."
    }
    return $payload
}

function New-SquirrelInstaller {
    param(
        [Parameter(Mandatory = $true)] [string] $Payload,
        [Parameter(Mandatory = $true)] [hashtable] $Tools
    )
    $major = (Select-String Version.kmk -Pattern '^VBOX_VERSION_MAJOR\s*=\s*(\d+)' | ForEach-Object { $_.Matches[0].Groups[1].Value })
    $minor = (Select-String Version.kmk -Pattern '^VBOX_VERSION_MINOR\s*=\s*(\d+)' | ForEach-Object { $_.Matches[0].Groups[1].Value })
    $build = (Select-String Version.kmk -Pattern '^VBOX_VERSION_BUILD\s*=\s*(\d+)' | ForEach-Object { $_.Matches[0].Groups[1].Value })
    if (-not $major -or -not $minor -or -not $build) { throw 'Version.kmk did not provide a numeric VirtualBox version.' }
    $version = "$major.$minor.$build"
    $workRoot = Join-Path ([IO.Path]::GetTempPath()) ("virtualbox-squirrel-" + [guid]::NewGuid().ToString('N'))
    $stage = Join-Path $workRoot 'stage'
    $release = Join-Path $workRoot 'release'
    New-Item -ItemType Directory -Force (Join-Path $stage 'lib\net45'), $release | Out-Null
    Copy-Item (Join-Path $Payload '*') (Join-Path $stage 'lib\net45') -Recurse -Force
    $nuspec = @"
<?xml version="1.0"?>
<package>
  <metadata>
    <id>VirtualBox</id>
    <version>$version</version>
    <authors>Oracle</authors>
    <description>Unsigned VirtualBox Windows package.</description>
  </metadata>
  <files><file src="lib\net45\**\*" target="lib\net45" /></files>
</package>
"@
    $nuspecPath = Join-Path $stage 'VirtualBox.nuspec'
    Set-Content -LiteralPath $nuspecPath -Value $nuspec -Encoding UTF8
    Push-Location $stage
    try {
        & $Tools.NuGet pack $nuspecPath -NoPackageAnalysis -NonInteractive | Out-Host
        if ($LASTEXITCODE -ne 0) { throw "NuGet package creation failed with exit code $LASTEXITCODE." }
    } finally {
        Pop-Location
    }
    $package = Get-ChildItem -LiteralPath $stage -Filter '*.nupkg' -File | Select-Object -First 1
    if (-not $package) { throw 'NuGet did not produce the Squirrel input package.' }
    & $Tools.Squirrel --releasify $package.FullName --releaseDir $release --no-msi | Out-Host
    if ($LASTEXITCODE -ne 0) { throw "Squirrel releasify failed with exit code $LASTEXITCODE." }
    $setup = Join-Path $release 'Setup.exe'
    $releases = Join-Path $release 'RELEASES'
    $fullPackage = Get-ChildItem -LiteralPath $release -Filter '*-full.nupkg' -File | Select-Object -First 1
    if (-not (Test-Path -LiteralPath $setup) -or -not (Test-Path -LiteralPath $releases) -or -not $fullPackage) {
        throw 'Squirrel did not produce Setup.exe, RELEASES, and a full .nupkg.'
    }
    foreach ($file in Get-ChildItem -LiteralPath $release -File) {
        if ($file.Extension -eq '.exe') {
            $signature = Get-AuthenticodeSignature -LiteralPath $file.FullName
            if ($signature.Status -ne 'NotSigned') { throw "Expected unsigned installer asset: $($file.Name) ($($signature.Status))." }
        }
    }
    $hashes = Get-ChildItem -LiteralPath $release -File | Get-FileHash -Algorithm SHA256
    $hashes | ForEach-Object { "{0}  {1}" -f $_.Hash.ToLowerInvariant(), $_.Path.Substring($release.Length + 1) } | Set-Content -LiteralPath (Join-Path $release 'SHA256SUMS.txt') -Encoding UTF8
    Write-Host "Unsigned Squirrel installer: $setup"
    Write-Host "RELEASES index: $releases"
    Write-Host "Full package: $($fullPackage.FullName)"
    $hashes | Where-Object { $_.Path -eq $setup } | ForEach-Object { Write-Host "Setup.exe SHA-256: $($_.Hash)" }
    return $release
}

$python = Get-RequiredPython
Invoke-Checked 'Bootstrap Windows SDK and WDK' { $script:SdkRoot = Ensure-WindowsKits }
$wdk71Root = $script:Wdk71Root
if (-not $wdk71Root -or -not (Test-Path -LiteralPath (Join-Path $wdk71Root 'lib\wlh\amd64\rxce.lib'))) {
    throw 'WDK 7.1 compatibility libraries were not materialized.'
}
$vcpkgRoot = Ensure-Vcpkg
$nasmRoot = Ensure-Nasm
$zipRoot = Ensure-Zip
$flexBisonRoot = Ensure-WinFlexBison
Ensure-MesaPython $python
$qtRoot = Ensure-Qt $python
$payload = Invoke-VirtualBoxBuild -Python $python -QtRoot $qtRoot -SdkRoot $SdkRoot -Wdk71Root $wdk71Root -VcpkgRoot $vcpkgRoot -NasmRoot $nasmRoot -ZipRoot $zipRoot

if ($Mode -eq 'Installer') {
    $squirrelTools = Ensure-Squirrel
    $releaseDir = New-SquirrelInstaller -Payload $payload -Tools $squirrelTools
    Write-Host "Installer artifacts remain at $releaseDir for inspection."
} elseif (-not $Silent) {
    $answer = Read-Host 'Build succeeded. Launch VirtualBox.exe now? [y/N]'
    if ($answer -match '^(y|yes)$') {
        Start-Process -FilePath (Join-Path $payload 'VirtualBox.exe')
    }
}

Write-Host "Completed $Mode mode without code signing."
