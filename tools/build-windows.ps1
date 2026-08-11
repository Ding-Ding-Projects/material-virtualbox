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
    & $Action
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
    & $winget install --id $Id --scope user --accept-package-agreements --accept-source-agreements --silent
    if ($LASTEXITCODE -ne 0) {
        throw "winget could not install $Id (exit code $LASTEXITCODE)."
    }
    Refresh-ProcessPath
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
            & $git.Path clone --depth 1 https://github.com/microsoft/vcpkg.git $vcpkgRoot
            if ($LASTEXITCODE -ne 0) { throw "vcpkg clone failed with exit code $LASTEXITCODE." }
        }
        Push-Location $vcpkgRoot
        try {
            & .\bootstrap-vcpkg.bat -disableMetrics
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
    $tar = Get-Command tar.exe -ErrorAction SilentlyContinue
    if (-not $tar) { throw 'tar.exe is required to unpack the verified Info-ZIP package.' }
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
        & $tar.Path -tf $archive | Out-Null
        if ($LASTEXITCODE -ne 0) { throw "Info-ZIP package archive validation failed with exit code $LASTEXITCODE." }
        & $tar.Path -xf $archive -C $installRoot
        if ($LASTEXITCODE -ne 0) { throw "Info-ZIP package extraction failed with exit code $LASTEXITCODE." }
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
    if (Test-Path -LiteralPath $bison -and Test-Path -LiteralPath $flex) {
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
    $squirrel = Join-Path $squirrelRoot 'Squirrel\tools\Squirrel.exe'
    if (-not (Test-Path -LiteralPath $squirrel)) {
        & $nuget install Squirrel -Version 1.9.1 -OutputDirectory $squirrelRoot -ExcludeVersion -NonInteractive
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
        [Parameter(Mandatory = $true)] [string] $VcpkgRoot,
        [Parameter(Mandatory = $true)] [string] $NasmRoot,
        [Parameter(Mandatory = $true)] [string] $ZipRoot
    )
    $env:VBOX_SIGNING_MODE = ''
    $env:VBOX_WITHOUT_HARDENING = '1'
    $env:VBOX_OSE = '1'
    $env:VBOX_CI_QT_ROOT = $QtRoot
    $env:VBOX_CI_WINDOWS_SDK_ROOT = $SdkRoot
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
    Invoke-Checked 'Configure the unsigned Windows build' { & .\configure.ps1 @arguments }
    if (-not (Test-Path -LiteralPath .\env.bat)) { throw 'configure.py did not generate env.bat.' }
    $revisionMatch = Select-String configure.py -Pattern '\$Id: configure.py (\d+)'
    if (-not $revisionMatch) { throw 'configure.py does not expose a numeric source revision for the Git mirror build.' }
    $revision = $revisionMatch.Matches[0].Groups[1].Value
    $revisionFile = Join-Path $repoRoot 'out\win.amd64\release\revision.kmk'
    New-Item -ItemType Directory -Force -Path (Split-Path -Parent $revisionFile) | Out-Null
    Set-Content -LiteralPath $revisionFile -Value "export VBOX_SVN_REV=$revision" -Encoding ascii
    Invoke-Checked 'Stage OpenSSL headers' {
        & cmd.exe /d /c "call `"$repoRoot\env.bat`" && kmk crypto-headers"
    }
    Invoke-Checked 'Build the Windows package payload' {
        & cmd.exe /d /c "call `"$repoRoot\env.bat`" && kmk VBOX_SVN_REV=$revision SDK_WINSDK10_MAX_VERSION=10.0.26100.0 VBOX_WINDDK_GST_W7=WINSDK10-KM VBOX_WINDDK_GST_W8=WINSDK10-KM VBOX_WINDDK_GST_WLH=WINSDK10-KM VBOX_WINDDK_GST_W2K3=WINSDK10-KM VBOX_WINDDK_GST_WXP=WINSDK10-KM VBOX_WINDDK_GST_W2K=WINSDK10-KM VBOX_WINDDK_GST_NT4=WINSDK10-KM packing"
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
  <files><file src="lib\\net45\\**\\*" target="lib\\net45" /></files>
</package>
"@
    $nuspecPath = Join-Path $stage 'VirtualBox.nuspec'
    Set-Content -LiteralPath $nuspecPath -Value $nuspec -Encoding UTF8
    Push-Location $stage
    try {
        & $Tools.NuGet pack $nuspecPath -NoPackageAnalysis -NonInteractive
        if ($LASTEXITCODE -ne 0) { throw "NuGet package creation failed with exit code $LASTEXITCODE." }
    } finally {
        Pop-Location
    }
    $package = Get-ChildItem -LiteralPath $stage -Filter '*.nupkg' -File | Select-Object -First 1
    if (-not $package) { throw 'NuGet did not produce the Squirrel input package.' }
    & $Tools.Squirrel --releasify $package.FullName --releaseDir $release --no-msi
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
$vcpkgRoot = Ensure-Vcpkg
$nasmRoot = Ensure-Nasm
$zipRoot = Ensure-Zip
$flexBisonRoot = Ensure-WinFlexBison
Ensure-MesaPython $python
$qtRoot = Ensure-Qt $python
$payload = Invoke-VirtualBoxBuild -Python $python -QtRoot $qtRoot -SdkRoot $SdkRoot -VcpkgRoot $vcpkgRoot -NasmRoot $nasmRoot -ZipRoot $zipRoot

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
