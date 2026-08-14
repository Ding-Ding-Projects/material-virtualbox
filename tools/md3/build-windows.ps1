[CmdletBinding()]
param(
    [switch]$Silent
)

$ErrorActionPreference = 'Stop'
$ProgressPreference = 'SilentlyContinue'
Set-StrictMode -Version Latest

$script:RunStarted = [DateTimeOffset]::UtcNow
$script:PhaseStarted = $script:RunStarted
$script:RunId = $script:RunStarted.ToString('yyyyMMddTHHmmssfffZ')
$script:RepoRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..'))
$script:LogRoot = Join-Path $script:RepoRoot "out\build-logs\$script:RunId"
$script:ToolchainRoot = Join-Path ([Environment]::GetFolderPath('LocalApplicationData')) 'material-virtualbox-toolchain'
$script:OutputRoot = Join-Path $script:RepoRoot 'out\win.amd64\release'
$script:SdkVersion = '10.0.22621.0'
$script:PythonVersion = '3.13.15'
$script:NasmVersion = '2.16.03'
$script:QtVersion = '6.8.3'
$script:AqtVersion = '3.3.0'
$script:PythonInstallerSha256 = 'edec09c4853aeae9ac36efb8c9f95b6b8e2fee65eee56d9767a8b7c69c574403'
$script:NasmArchiveSha256 = '3ee4782247bcb874378d02f7eab4e294a84d3d15f3f6ee2de2f47a46aa7226e6'
$script:ExpectedKBuildSha = $null
$script:KBuildRoot = $null
$script:PythonExe = $null
$script:NasmRoot = $null
$script:QtRoot = $null
$script:VsRoot = $null
$script:VccRoot = $null
$script:SdkRoot = $null
$script:SdkViewRoot = $null

function Write-Phase {
    param([Parameter(Mandatory)][string]$Name)
    $now = [DateTimeOffset]::UtcNow
    $elapsed = $now - $script:PhaseStarted
    Write-Host ('[PHASE] {0} (previous phase {1:hh\:mm\:ss\.fff})' -f $Name, $elapsed)
    $script:PhaseStarted = $now
}

function Fail-Build {
    param([Parameter(Mandatory)][string]$Message, [int]$ExitCode = 1)
    Write-Error $Message
    exit $ExitCode
}

function Invoke-Native {
    param(
        [Parameter(Mandatory)][string]$FilePath,
        [Parameter(Mandatory)][string[]]$ArgumentList,
        [string]$LogPath,
        [hashtable]$Environment = @{},
        [switch]$CaptureOutput
    )

    $psi = [Diagnostics.ProcessStartInfo]::new()
    $psi.FileName = $FilePath
    $psi.UseShellExecute = $false
    $psi.CreateNoWindow = $true
    $psi.RedirectStandardOutput = $true
    $psi.RedirectStandardError = $true
    if ($psi.PSObject.Properties.Name -contains 'ArgumentList') {
        foreach ($argument in $ArgumentList) {
            [void]$psi.ArgumentList.Add($argument)
        }
    }
    else {
        $psi.Arguments = ($ArgumentList | ForEach-Object {
            if ($_ -notmatch '[\s"]') { $_ }
            else { '"' + ($_ -replace '(\\*)"', '$1$1\"' -replace '(\\+)$', '$1$1') + '"' }
        }) -join ' '
    }
    foreach ($entry in $Environment.GetEnumerator()) {
        if ($null -eq $entry.Value) {
            [void]$psi.Environment.Remove([string]$entry.Key)
        }
        else {
            $psi.Environment[[string]$entry.Key] = [string]$entry.Value
        }
    }

    $process = [Diagnostics.Process]::new()
    $process.StartInfo = $psi
    if (-not $process.Start()) {
        Fail-Build "Failed to start '$FilePath'."
    }
    $stdout = $process.StandardOutput.ReadToEnd()
    $stderr = $process.StandardError.ReadToEnd()
    $process.WaitForExit()

    if ($LogPath) {
        $parent = Split-Path -Parent $LogPath
        if ($parent) { New-Item -ItemType Directory -Force -Path $parent | Out-Null }
        @($stdout, $stderr) | Set-Content -LiteralPath $LogPath -Encoding utf8
    }
    elseif (-not $Silent) {
        if ($stdout) { Write-Host $stdout.TrimEnd() }
        if ($stderr) { Write-Host $stderr.TrimEnd() }
    }

    if ($process.ExitCode -ne 0) {
        $where = if ($LogPath) { " See '$LogPath'." } else { '' }
        Fail-Build "'$FilePath' exited with code $($process.ExitCode).$where" $process.ExitCode
    }
    if ($CaptureOutput) {
        return [pscustomobject]@{ StdOut = $stdout; StdErr = $stderr; ExitCode = $process.ExitCode }
    }
}

function Assert-File {
    param([Parameter(Mandatory)][string]$Path, [Parameter(Mandatory)][string]$Description)
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        Fail-Build "$Description is missing: $Path"
    }
    return (Get-Item -LiteralPath $Path)
}

function Assert-Hash {
    param([Parameter(Mandatory)][string]$Path, [Parameter(Mandatory)][string]$Expected)
    $actual = (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToLowerInvariant()
    if ($actual -ne $Expected.ToLowerInvariant()) {
        Fail-Build "SHA-256 mismatch for '$Path'. Expected $Expected; found $actual. Delete the invalid cache entry and retry."
    }
}

function Get-VerifiedDownload {
    param(
        [Parameter(Mandatory)][uri]$Uri,
        [Parameter(Mandatory)][string]$Destination,
        [Parameter(Mandatory)][string]$Sha256
    )
    if (Test-Path -LiteralPath $Destination -PathType Leaf) {
        try {
            Assert-Hash $Destination $Sha256
            Write-Host "[CACHE] Reusing verified $Destination"
            return
        }
        catch {
            Remove-Item -LiteralPath $Destination -Force
        }
    }
    New-Item -ItemType Directory -Force -Path (Split-Path -Parent $Destination) | Out-Null
    $temporary = "$Destination.partial-$PID"
    Remove-Item -LiteralPath $temporary -Force -ErrorAction SilentlyContinue
    try {
        Invoke-WebRequest -Uri $Uri -OutFile $temporary -MaximumRedirection 0 -UseBasicParsing
        Assert-Hash $temporary $Sha256
        Move-Item -LiteralPath $temporary -Destination $Destination -Force
    }
    finally {
        Remove-Item -LiteralPath $temporary -Force -ErrorAction SilentlyContinue
    }
}

function Test-IsAdministrator {
    $identity = [Security.Principal.WindowsIdentity]::GetCurrent()
    $principal = [Security.Principal.WindowsPrincipal]::new($identity)
    return $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
}

function Get-VsWherePath {
    $candidate = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
    if (Test-Path -LiteralPath $candidate -PathType Leaf) { return $candidate }
    $command = Get-Command vswhere.exe -ErrorAction SilentlyContinue
    if ($command) { return $command.Source }
    return $null
}

function Find-VisualStudio {
    $vswhere = Get-VsWherePath
    if (-not $vswhere) { return $null }
    $result = Invoke-Native -FilePath $vswhere -ArgumentList @(
        '-latest', '-products', '*',
        '-version', '[17.0,18.0)',
        '-requires', 'Microsoft.VisualStudio.Component.VC.Tools.x86.x64',
        '-property', 'installationPath'
    ) -CaptureOutput
    $root = $result.StdOut.Trim()
    if (-not $root) { return $null }
    $vctoolsFile = Join-Path $root 'VC\Auxiliary\Build\Microsoft.VCToolsVersion.default.txt'
    Assert-File $vctoolsFile 'Visual Studio VCC143 version marker' | Out-Null
    $vctoolsVersion = (Get-Content -LiteralPath $vctoolsFile -Raw).Trim()
    if ($vctoolsVersion -notmatch '^14\.3') {
        Fail-Build "Visual Studio 2022 is present, but VCC143 was not selected (found $vctoolsVersion). Install Microsoft.VisualStudio.Component.VC.Tools.x86.x64."
    }
    $vccRoot = Join-Path $root "VC\Tools\MSVC\$vctoolsVersion"
    foreach ($relative in @('bin\Hostx64\x64\cl.exe', 'bin\Hostx64\x64\link.exe', 'include\vcruntime.h', 'lib\x64\libcmt.lib')) {
        Assert-File (Join-Path $vccRoot $relative) "VCC143 component '$relative'" | Out-Null
    }
    return [pscustomobject]@{ Root = $root; VccRoot = $vccRoot; Version = $vctoolsVersion; VsWhere = $vswhere }
}

function Find-SdkRoot {
    $candidates = @()
    try {
        $root = (Get-ItemProperty -LiteralPath 'Registry::HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows Kits\Installed Roots' -Name KitsRoot10 -ErrorAction Stop).KitsRoot10
        if ($root) { $candidates += $root }
    }
    catch { }
    if (${env:ProgramFiles(x86)}) { $candidates += (Join-Path ${env:ProgramFiles(x86)} 'Windows Kits\10') }
    foreach ($candidate in ($candidates | Select-Object -Unique)) {
        if (Test-Path -LiteralPath (Join-Path $candidate "Include\$script:SdkVersion") -PathType Container) {
            return [IO.Path]::GetFullPath($candidate)
        }
    }
    return $null
}

function Test-SdkAndWdk {
    param([Parameter(Mandatory)][string]$Root)
    $sdk = @(
        "Include\$script:SdkVersion\um\Windows.h",
        "Include\$script:SdkVersion\shared\sdkddkver.h",
        "Include\$script:SdkVersion\ucrt\stdio.h",
        "Lib\$script:SdkVersion\um\x64\kernel32.lib",
        "Lib\$script:SdkVersion\ucrt\x64\ucrt.lib",
        "Bin\$script:SdkVersion\x64\rc.exe",
        "Bin\$script:SdkVersion\x64\midl.exe",
        "Bin\$script:SdkVersion\x64\signtool.exe"
    )
    $wdk = @(
        "Include\$script:SdkVersion\km\ntddk.h",
        "Include\$script:SdkVersion\km\wdm.h",
        "Lib\$script:SdkVersion\km\x64\ntoskrnl.lib"
    )
    $missingSdk = @($sdk | Where-Object { -not (Test-Path -LiteralPath (Join-Path $Root $_) -PathType Leaf) })
    $missingWdk = @($wdk | Where-Object { -not (Test-Path -LiteralPath (Join-Path $Root $_) -PathType Leaf) })
    return [pscustomobject]@{ MissingSdk = $missingSdk; MissingWdk = $missingWdk; Complete = ($missingSdk.Count -eq 0 -and $missingWdk.Count -eq 0) }
}

function Install-CiSdkAndWdk {
    if ($env:GITHUB_ACTIONS -ne 'true') { return $false }
    if (-not (Test-IsAdministrator)) {
        Fail-Build 'Windows SDK/WDK 10.0.22621.0 is incomplete. GitHub Actions may install Microsoft.WindowsSDK.10.0.22621 and Microsoft.WindowsWDK.10.0.22621 only from an elevated windows-2022 job; this process is not elevated.'
    }
    $winget = Get-Command winget.exe -ErrorAction SilentlyContinue
    if (-not $winget) {
        Fail-Build 'Windows SDK/WDK 10.0.22621.0 is incomplete and canonical winget package installation is unavailable. The windows-2022 job requires winget packages Microsoft.WindowsSDK.10.0.22621 and Microsoft.WindowsWDK.10.0.22621.'
    }
    foreach ($package in @('Microsoft.WindowsSDK.10.0.22621', 'Microsoft.WindowsWDK.10.0.22621')) {
        Invoke-Native -FilePath $winget.Source -ArgumentList @(
            'install', '--id', $package, '--exact', '--silent',
            '--accept-package-agreements', '--accept-source-agreements', '--disable-interactivity'
        ) -LogPath (Join-Path $script:LogRoot "install-$package.log")
    }
    return $true
}

function Assert-HostPrerequisites {
    Write-Phase 'Preflight native build host'
    if ($env:OS -ne 'Windows_NT' -or -not [Environment]::Is64BitOperatingSystem) {
        Fail-Build 'This lane requires a 64-bit native Windows host.'
    }
    if ([Runtime.InteropServices.RuntimeInformation]::OSArchitecture -ne [Runtime.InteropServices.Architecture]::X64) {
        Fail-Build 'This lane requires a native x64 Windows process; ARM64 and emulated processes are unsupported.'
    }

    $vs = Find-VisualStudio
    $sdkRoot = Find-SdkRoot
    $sdkState = if ($sdkRoot) { Test-SdkAndWdk $sdkRoot } else { [pscustomobject]@{ Complete = $false; MissingSdk = @('SDK root'); MissingWdk = @('WDK root') } }
    if (-not $vs -or -not $sdkState.Complete) {
        if (-not (Test-IsAdministrator) -and $env:GITHUB_ACTIONS -ne 'true') {
            $missing = @()
            if (-not $vs) { $missing += 'Visual Studio 2022 VCC143 (Microsoft.VisualStudio.Component.VC.Tools.x86.x64)' }
            if ($sdkState.MissingSdk.Count) { $missing += 'Windows SDK exactly 10.0.22621.0' }
            if ($sdkState.MissingWdk.Count) { $missing += 'Windows Driver Kit exactly 10.0.22621.0' }
            Fail-Build ("Native build prerequisites are missing: {0}. No files or packages were changed. Install Visual Studio 2022 Build Tools with VCC143 plus Windows SDK/WDK 10.0.22621.0 from Microsoft in an elevated installer session, then rerun this script from a normal non-elevated shell. The script never installs drivers." -f ($missing -join '; '))
        }
        if (-not $vs) {
            Fail-Build 'Visual Studio 2022 with VCC143 is missing. Install Microsoft.VisualStudio.Component.VC.Tools.x86.x64 from the canonical Visual Studio installer; this script will not mutate Visual Studio.'
        }
        if (-not $sdkState.Complete) {
            [void](Install-CiSdkAndWdk)
            $sdkRoot = Find-SdkRoot
            $sdkState = if ($sdkRoot) { Test-SdkAndWdk $sdkRoot } else { $null }
            if (-not $sdkState -or -not $sdkState.Complete) {
                Fail-Build 'Canonical SDK/WDK installation completed without yielding the required 10.0.22621.0 headers, libraries, and tools.'
            }
        }
    }
    $script:VsRoot = $vs.Root
    $script:VccRoot = $vs.VccRoot
    $script:SdkRoot = $sdkRoot
    Write-Host "[FOUND] Visual Studio 2022 VCC143 $($vs.Version): $($vs.VccRoot)"
    Write-Host "[FOUND] Windows SDK/WDK $script:SdkVersion: $sdkRoot"
}

function New-ExactSdkView {
    Write-Phase "Create exact SDK/WDK $script:SdkVersion view"
    $viewRoot = Join-Path $script:ToolchainRoot "winsdk-wdk-$script:SdkVersion"
    New-Item -ItemType Directory -Force -Path $viewRoot | Out-Null
    foreach ($directory in @('Include', 'Lib', 'Bin')) {
        $viewParent = Join-Path $viewRoot $directory
        New-Item -ItemType Directory -Force -Path $viewParent | Out-Null
        $source = Join-Path $script:SdkRoot "$directory\$script:SdkVersion"
        $target = Join-Path $viewParent $script:SdkVersion
        if (Test-Path -LiteralPath $target) {
            $item = Get-Item -LiteralPath $target -Force
            if (-not ($item.Attributes -band [IO.FileAttributes]::ReparsePoint)) {
                Fail-Build "Exact SDK view path is not an owned junction: $target"
            }
        }
        else {
            $output = & cmd.exe /d /s /c "mklink /J `"$target`" `"$source`"" 2>&1
            if ($LASTEXITCODE -ne 0) { Fail-Build "Failed to create the exact SDK view junction '$target' -> '$source': $output" }
        }
    }
    foreach ($directory in @('References', 'UnionMetadata')) {
        $source = Join-Path $script:SdkRoot $directory
        $target = Join-Path $viewRoot $directory
        if ((Test-Path -LiteralPath $source -PathType Container) -and -not (Test-Path -LiteralPath $target)) {
            $output = & cmd.exe /d /s /c "mklink /J `"$target`" `"$source`"" 2>&1
            if ($LASTEXITCODE -ne 0) { Fail-Build "Failed to create SDK metadata junction '$target' -> '$source': $output" }
        }
    }
    $script:SdkViewRoot = $viewRoot
}

function Initialize-KBuild {
    Write-Phase 'Initialize pinned kBuild gitlink'
    $git = (Get-Command git.exe -ErrorAction Stop).Source
    $expected = (Invoke-Native -FilePath $git -ArgumentList @('-C', $script:RepoRoot, 'ls-files', '-s', '--', 'kBuild') -CaptureOutput).StdOut.Trim().Split()[1]
    if ($expected -notmatch '^[0-9a-f]{40}$') { Fail-Build 'Unable to read the exact kBuild gitlink SHA.' }
    $script:ExpectedKBuildSha = $expected
    Invoke-Native -FilePath $git -ArgumentList @('-C', $script:RepoRoot, 'submodule', 'update', '--init', '--checkout', '--', 'kBuild') -LogPath (Join-Path $script:LogRoot 'kbuild-submodule.log')
    $actual = (Invoke-Native -FilePath $git -ArgumentList @('-C', (Join-Path $script:RepoRoot 'kBuild'), 'rev-parse', 'HEAD') -CaptureOutput).StdOut.Trim()
    if ($actual -ne $expected) { Fail-Build "kBuild checkout mismatch. Gitlink is $expected; checkout is $actual." }
    foreach ($candidate in @((Join-Path $script:RepoRoot 'kBuild'), (Join-Path $script:RepoRoot 'kBuild\kBuild'))) {
        if ((Test-Path (Join-Path $candidate 'header.kmk') -PathType Leaf) -and
            (Test-Path (Join-Path $candidate 'footer.kmk') -PathType Leaf) -and
            (Test-Path (Join-Path $candidate 'rules.kmk') -PathType Leaf) -and
            (Test-Path (Join-Path $candidate 'bin\win.amd64\kmk.exe') -PathType Leaf) -and
            (Test-Path (Join-Path $candidate 'bin\win.amd64\kmk_ash.exe') -PathType Leaf)) {
            $script:KBuildRoot = [IO.Path]::GetFullPath($candidate)
            break
        }
    }
    if (-not $script:KBuildRoot) { Fail-Build 'The pinned submodule does not contain a usable kBuild root.' }
    Write-Host "[FOUND] kBuild ${actual}: $script:KBuildRoot"
}

function Ensure-Python {
    Write-Phase "Verify Python $script:PythonVersion x64"
    $root = Join-Path $script:ToolchainRoot "python-$script:PythonVersion-amd64"
    $exe = Join-Path $root 'python.exe'
    $valid = $false
    if (Test-Path -LiteralPath $exe -PathType Leaf) {
        try {
            $probe = Invoke-Native -FilePath $exe -ArgumentList @('-c', 'import platform,sys;print(platform.python_version());print(platform.machine());print(8*__import__("struct").calcsize("P"))') -CaptureOutput
            $lines = @($probe.StdOut.Trim() -split "`r?`n")
            $valid = ($lines.Count -ge 3 -and $lines[0] -eq $script:PythonVersion -and $lines[2] -eq '64')
        }
        catch { $valid = $false }
    }
    if (-not $valid) {
        Remove-Item -LiteralPath $root -Recurse -Force -ErrorAction SilentlyContinue
        $cache = Join-Path $script:ToolchainRoot "downloads\python-$script:PythonVersion-amd64.exe"
        Get-VerifiedDownload -Uri "https://www.python.org/ftp/python/$script:PythonVersion/python-$script:PythonVersion-amd64.exe" -Destination $cache -Sha256 $script:PythonInstallerSha256
        Invoke-Native -FilePath $cache -ArgumentList @('/quiet', 'InstallAllUsers=0', 'PrependPath=0', 'Include_launcher=0', 'Include_test=0', "TargetDir=$root") -LogPath (Join-Path $script:LogRoot 'python-install.log')
    }
    Assert-File $exe "Python $script:PythonVersion executable" | Out-Null
    $version = (Invoke-Native -FilePath $exe -ArgumentList @('-c', 'import platform;print(platform.python_version())') -CaptureOutput).StdOut.Trim()
    if ($version -ne $script:PythonVersion) { Fail-Build "Python cache validation failed: expected $script:PythonVersion; found $version." }
    $script:PythonExe = $exe
}

function Ensure-Nasm {
    Write-Phase "Verify NASM $script:NasmVersion"
    $root = Join-Path $script:ToolchainRoot "nasm-$script:NasmVersion"
    $exe = Join-Path $root 'nasm.exe'
    $valid = $false
    if (Test-Path -LiteralPath $exe -PathType Leaf) {
        try { $valid = ((Invoke-Native -FilePath $exe -ArgumentList @('-v') -CaptureOutput).StdOut -match [regex]::Escape("NASM version $script:NasmVersion")) } catch { }
    }
    if (-not $valid) {
        Remove-Item -LiteralPath $root -Recurse -Force -ErrorAction SilentlyContinue
        $archive = Join-Path $script:ToolchainRoot "downloads\nasm-$script:NasmVersion-win64.zip"
        Get-VerifiedDownload -Uri "https://www.nasm.us/pub/nasm/releasebuilds/$script:NasmVersion/win64/nasm-$script:NasmVersion-win64.zip" -Destination $archive -Sha256 $script:NasmArchiveSha256
        $extract = Join-Path $script:ToolchainRoot "extract-nasm-$PID"
        Remove-Item -LiteralPath $extract -Recurse -Force -ErrorAction SilentlyContinue
        Expand-Archive -LiteralPath $archive -DestinationPath $extract -Force
        $extracted = Get-ChildItem -LiteralPath $extract -Recurse -Filter nasm.exe -File | Select-Object -First 1
        if (-not $extracted) { Fail-Build 'The verified NASM archive does not contain nasm.exe.' }
        New-Item -ItemType Directory -Force -Path $root | Out-Null
        Copy-Item -LiteralPath (Split-Path -Parent $extracted.FullName)\* -Destination $root -Recurse -Force
        Remove-Item -LiteralPath $extract -Recurse -Force
    }
    $probe = (Invoke-Native -FilePath $exe -ArgumentList @('-v') -CaptureOutput).StdOut
    if ($probe -notmatch [regex]::Escape("NASM version $script:NasmVersion")) { Fail-Build "NASM cache is not version $script:NasmVersion." }
    $script:NasmRoot = $root
}

function Ensure-Qt {
    Write-Phase "Verify Qt $script:QtVersion with qtscxml"
    $aqtRoot = Join-Path $script:ToolchainRoot "aqt-$script:AqtVersion-py$script:PythonVersion"
    $requirements = Join-Path $script:RepoRoot 'tools\ci\aqt-requirements.txt'
    Assert-File $requirements 'Hashed aqt requirements file' | Out-Null
    $aqtProbe = Join-Path $aqtRoot 'aqt\__init__.py'
    if (-not (Test-Path -LiteralPath $aqtProbe -PathType Leaf)) {
        Remove-Item -LiteralPath $aqtRoot -Recurse -Force -ErrorAction SilentlyContinue
        Invoke-Native -FilePath $script:PythonExe -ArgumentList @('-m', 'pip', 'install', '--disable-pip-version-check', '--only-binary=:all:', '--require-hashes', '--no-deps', '--target', $aqtRoot, '-r', $requirements) -LogPath (Join-Path $script:LogRoot 'aqt-install.log')
    }
    $qtBase = Join-Path $script:ToolchainRoot "qt-$script:QtVersion"
    $qtRoot = Join-Path $qtBase "$script:QtVersion\msvc2022_64"
    $required = @(
        'bin\qmake.exe', 'bin\moc.exe', 'bin\rcc.exe', 'bin\uic.exe', 'bin\lrelease.exe',
        'bin\Qt6Core.dll', 'include\QtCore\QtGlobal', 'lib\Qt6Core.lib',
        'bin\Qt6Scxml.dll', 'include\QtScxml\QtScxml', 'lib\Qt6Scxml.lib', 'modules\Scxml.json'
    )
    $missing = @($required | Where-Object { -not (Test-Path -LiteralPath (Join-Path $qtRoot $_) -PathType Leaf) })
    $versionOkay = $false
    if ($missing.Count -eq 0) {
        try {
            $query = Invoke-Native -FilePath (Join-Path $qtRoot 'bin\qmake.exe') -ArgumentList @('-query', 'QT_VERSION') -CaptureOutput
            $versionOkay = ($query.StdOut.Trim() -eq $script:QtVersion)
        }
        catch { }
    }
    if (-not $versionOkay) {
        Remove-Item -LiteralPath $qtBase -Recurse -Force -ErrorAction SilentlyContinue
        $aqtEnv = @{ PYTHONPATH = $aqtRoot }
        Invoke-Native -FilePath $script:PythonExe -ArgumentList @(
            '-m', 'aqt', 'install-qt', 'windows', 'desktop', $script:QtVersion, 'win64_msvc2022_64',
            '--outputdir', $qtBase, '--modules', 'qtscxml'
        ) -Environment $aqtEnv -LogPath (Join-Path $script:LogRoot 'qt-install.log')
    }
    foreach ($relative in $required) { Assert-File (Join-Path $qtRoot $relative) "Qt component '$relative'" | Out-Null }
    $query = (Invoke-Native -FilePath (Join-Path $qtRoot 'bin\qmake.exe') -ArgumentList @('-query', 'QT_VERSION') -CaptureOutput).StdOut.Trim()
    if ($query -ne $script:QtVersion) { Fail-Build "Qt cache validation failed: expected $script:QtVersion; found $query." }
    $script:QtRoot = $qtRoot
}

function Get-SigningEnvironment {
    $environment = @{}
    foreach ($entry in [Environment]::GetEnvironmentVariables().GetEnumerator()) {
        $name = [string]$entry.Key
        if ($name -match '(?i)(VBOX_SIGNING_MODE|SIGN.*(CERT|FINGERPRINT|TIMESTAMP|ATTEST)|CERT.*(SIGN|FINGERPRINT)|FINGERPRINT|TIMESTAMP|ATTESTATION|CORPORATE.*SIGN)') {
            $environment[$name] = $null
        }
    }
    foreach ($name in @('VBOX_SIGNING_MODE', 'VBOX_SIGNING_CERTIFICATE', 'VBOX_SIGNING_CERTIFICATE_FINGERPRINT', 'VBOX_SIGNING_TIMESTAMP_URL', 'VBOX_ATTESTATION_CREDENTIAL', 'VBOX_CORPORATE_SIGNING')) {
        $environment[$name] = $null
    }
    return $environment
}

function Configure-Build {
    Write-Phase 'Configure Release OSE amd64 Windows build'
    $autoConfig = Join-Path $script:RepoRoot 'AutoConfig.kmk'
    $envBat = Join-Path $script:RepoRoot 'env.bat'
    $configureLog = Join-Path $script:LogRoot 'configure.log'
    $args = @(
        (Join-Path $script:RepoRoot 'configure.py'),
        '--ose', '--build-target', 'win', '--build-arch', 'amd64', '--build-type', 'release',
        '--disable-hardening', '--disable-docs', '--disable-additions', '--disable-validationkit', '--disable-extpack',
        '--disable-sdl', '--disable-udptunnel', '--disable-dtrace', '--disable-pylint',
        '--disable-java', '--disable-gsoap', '--disable-openwatcom', '--disable-yasm',
        '--disable-win-ddk', '--disable-win-nsis', '--disable-win-msi', '--disable-win-wix',
        '--with-kbuild-path', $script:KBuildRoot,
        '--with-python-path', (Split-Path -Parent $script:PythonExe),
        '--with-nasm-path', $script:NasmRoot,
        '--with-qt-path', $script:QtRoot,
        '--with-win-visualcpp-path', $script:VsRoot,
        '--with-win-sdk10-path', $script:SdkViewRoot,
        '--output-file-autoconfig', $autoConfig,
        '--output-file-env', $envBat,
        '--output-file-log', $configureLog,
        '--output-dir', $script:RepoRoot,
        '--output-build-dir', (Join-Path $script:RepoRoot 'out')
    )
    Invoke-Native -FilePath $script:PythonExe -ArgumentList $args -Environment (Get-SigningEnvironment) -LogPath (Join-Path $script:LogRoot 'configure-console.log')
    Assert-File $autoConfig 'AutoConfig.kmk' | Out-Null
    Assert-File $envBat 'env.bat' | Out-Null
    Assert-File $configureLog 'configure.log' | Out-Null

    @(
        '', '# Native Windows Qt desktop lane: explicit out-of-scope feature assignments.',
        'VBOX_WITH_QT_PAYLOAD = 1',
        'VBOX_WITHOUT_HARDENING = 1',
        'VBOX_WITHOUT_WINDOWS_KERNEL_CODE_SIGNING_CERT = 1',
        'VBOX_WITHOUT_ADDITIONS = 1',
        'VBOX_WITH_DOCS =',
        'VBOX_WITH_DOCS_PACKING =',
        'VBOX_WITH_VALIDATIONKIT =',
        'VBOX_WITH_EXTPACK_PUEL_BUILD =',
        'VBOX_WITH_EXTPACK_VNC =',
        'VBOX_WITH_VBOXSDL =',
        'VBOX_WITH_SDL =',
        'VBOX_WITH_UDPTUNNEL =',
        'VBOX_WITH_DTRACE =',
        'VBOX_WITH_PYLINT =',
        'VBOX_WITH_JWS =',
        'VBOX_WITH_JMSCOM =',
        'VBOX_WITH_JXPCOM =',
        'VBOX_WITH_GSOAP =',
        'VBOX_WITH_WEBSERVICES =',
        'VBOX_WITH_OPEN_WATCOM =',
        'DONT_USE_YASM = 1',
        'VBOX_WITH_NSIS =',
        'VBOX_WITH_MSI =',
        'VBOX_WITH_WIX =',
        "SDK_WINSDK10_VERSION = $script:SdkVersion",
        "SDK_WINSDK10_MAX_VERSION = $script:SdkVersion",
        "PATH_SDK_WINSDK10 = $($script:SdkViewRoot -replace '\\','/')"
    ) | Add-Content -LiteralPath $autoConfig -Encoding utf8
}

function Build-Targets {
    Write-Phase 'Build UICommon, VirtualBox, and VirtualBoxVM serially'
    $targets = @('UICommon', 'VirtualBox', 'VirtualBoxVM')
    $owned = @(
        (Join-Path $script:OutputRoot 'bin\UICommon.dll'),
        (Join-Path $script:OutputRoot 'lib\UICommon.lib'),
        (Join-Path $script:OutputRoot 'bin\VirtualBox.exe'),
        (Join-Path $script:OutputRoot 'bin\VirtualBoxVM.exe')
    )
    foreach ($path in $owned) { Remove-Item -LiteralPath $path -Force -ErrorAction SilentlyContinue }
    $buildStart = [DateTimeOffset]::UtcNow
    $buildStartMarker = Join-Path $script:LogRoot 'build-start-utc.txt'
    $buildStart.ToString('o') | Set-Content -LiteralPath $buildStartMarker -Encoding ascii

    $assignments = @(
        'KBUILD_TYPE=release', 'KBUILD_TARGET=win', 'KBUILD_TARGET_ARCH=amd64',
        'VBOX_WITH_QT_PAYLOAD=1', 'VBOX_WITHOUT_HARDENING=1', 'VBOX_WITHOUT_WINDOWS_KERNEL_CODE_SIGNING_CERT=1',
        'VBOX_WITHOUT_ADDITIONS=1', 'VBOX_WITH_DOCS=', 'VBOX_WITH_DOCS_PACKING=', 'VBOX_WITH_VALIDATIONKIT=',
        'VBOX_WITH_EXTPACK_PUEL_BUILD=', 'VBOX_WITH_EXTPACK_VNC=', 'VBOX_WITH_VBOXSDL=', 'VBOX_WITH_SDL=',
        'VBOX_WITH_UDPTUNNEL=', 'VBOX_WITH_DTRACE=', 'VBOX_WITH_PYLINT=', 'VBOX_WITH_JWS=', 'VBOX_WITH_JMSCOM=',
        'VBOX_WITH_JXPCOM=', 'VBOX_WITH_GSOAP=', 'VBOX_WITH_WEBSERVICES=', 'VBOX_WITH_OPEN_WATCOM=', 'DONT_USE_YASM=1',
        'VBOX_WITH_NSIS=', 'VBOX_WITH_MSI=', 'VBOX_WITH_WIX=',
        "SDK_WINSDK10_VERSION=$script:SdkVersion", "SDK_WINSDK10_MAX_VERSION=$script:SdkVersion",
        ('PATH_SDK_WINSDK10=' + ($script:SdkViewRoot -replace '\\','/')),
        ('PATH_SDK_QT6=' + ($script:QtRoot -replace '\\','/')),
        ('VBOX_PATH_QT=' + ($script:QtRoot -replace '\\','/')),
        ('PATH_TOOL_NASM=' + ($script:NasmRoot -replace '\\','/'))
    )
    $cmd = Join-Path $env:SystemRoot 'System32\cmd.exe'
    $childEnvironment = Get-SigningEnvironment
    foreach ($target in $targets) {
        $kmkArgs = @('-j1') + $assignments + @($target)
        $quoted = $kmkArgs | ForEach-Object { '"' + ($_ -replace '"', '\"') + '"' }
        $command = 'call "{0}" && "{1}" {2}' -f (Join-Path $script:RepoRoot 'env.bat'), (Join-Path $script:KBuildRoot 'bin\win.amd64\kmk.exe'), ($quoted -join ' ')
        Invoke-Native -FilePath $cmd -ArgumentList @('/d', '/s', '/c', $command) -Environment $childEnvironment -LogPath (Join-Path $script:LogRoot "build-$target.log")
    }

    return [pscustomobject]@{ Start = $buildStart; Marker = $buildStartMarker; Owned = $owned }
}

function Get-SignatureStatus {
    param([Parameter(Mandatory)][string]$Path)
    $signature = Get-AuthenticodeSignature -LiteralPath $Path
    if ($signature.Status -ne [Management.Automation.SignatureStatus]::NotSigned) {
        Fail-Build "Unsigned-output contract failed for '$Path': signature status is $($signature.Status)."
    }
    return $signature.Status.ToString()
}

function Write-Manifest {
    param([Parameter(Mandatory)]$BuildState)
    Write-Phase 'Verify fresh unsigned outputs and write safe manifests'
    $git = (Get-Command git.exe -ErrorAction Stop).Source
    $sourceSha = (Invoke-Native -FilePath $git -ArgumentList @('-C', $script:RepoRoot, 'rev-parse', 'HEAD') -CaptureOutput).StdOut.Trim()
    $files = @()
    foreach ($path in $BuildState.Owned) {
        $item = Assert-File $path 'Required native build output'
        if ($item.Length -le 0) { Fail-Build "Required native build output is empty: $path" }
        if ([DateTimeOffset]$item.LastWriteTimeUtc -le $BuildState.Start) {
            Fail-Build "Required native build output is stale: $path (last write $($item.LastWriteTimeUtc.ToString('o')); build start $($BuildState.Start.ToString('o')))."
        }
        $signature = if ($item.Extension -in @('.exe', '.dll')) { Get-SignatureStatus $path } else { 'NotApplicable' }
        $files += [ordered]@{
            path = $item.FullName.Substring($script:RepoRoot.TrimEnd('\').Length).TrimStart('\').Replace('\', '/')
            size = $item.Length
            sha256 = (Get-FileHash -LiteralPath $item.FullName -Algorithm SHA256).Hash.ToLowerInvariant()
            lastWriteUtc = $item.LastWriteTimeUtc.ToString('o')
            signatureStatus = $signature
        }
    }
    $manifest = [ordered]@{
        schemaVersion = 1
        runId = $script:RunId
        startedUtc = $script:RunStarted.ToString('o')
        completedUtc = [DateTimeOffset]::UtcNow.ToString('o')
        sourceSha = $sourceSha
        kBuildSha = $script:ExpectedKBuildSha
        configuration = [ordered]@{ target = 'win'; architecture = 'amd64'; type = 'release'; edition = 'OSE'; hardening = $false; signing = 'prohibited'; qtPayload = 'stock' }
        versions = [ordered]@{ python = $script:PythonVersion; nasm = $script:NasmVersion; qt = $script:QtVersion; aqtinstall = $script:AqtVersion; visualCpp = 'VCC143'; windowsSdk = $script:SdkVersion; windowsWdk = $script:SdkVersion }
        roots = [ordered]@{ toolchain = $script:ToolchainRoot; buildOutput = $script:OutputRoot; logs = $script:LogRoot }
        files = $files
    }
    $jsonPath = Join-Path $script:LogRoot 'native-build-manifest.json'
    $jsonlPath = Join-Path $script:LogRoot 'native-build-manifest.jsonl'
    $markdownPath = Join-Path $script:LogRoot 'native-build-manifest.md'
    $manifest | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $jsonPath -Encoding utf8
    @($files | ForEach-Object { $_ | ConvertTo-Json -Compress }) | Set-Content -LiteralPath $jsonlPath -Encoding utf8
    $table = @(
        '# Native Windows Qt desktop build evidence', '',
        "- Run: ``$script:RunId``",
        "- Source SHA: ``$sourceSha``",
        "- kBuild SHA: ``$script:ExpectedKBuildSha``",
        "- SDK/WDK: ``$script:SdkVersion``",
        '- Signing: prohibited; PE outputs verified `NotSigned`.', '',
        '| Path | Bytes | SHA-256 | Signature |',
        '|---|---:|---|---|'
    )
    foreach ($file in $files) { $table += "| ``$($file.path)`` | $($file.size) | ``$($file.sha256)`` | $($file.signatureStatus) |" }
    $table | Set-Content -LiteralPath $markdownPath -Encoding utf8
    Copy-Item -LiteralPath $jsonPath -Destination (Join-Path (Split-Path -Parent $script:LogRoot) 'latest-native-build-manifest.json') -Force
    Write-Host "[READY] Unsigned native Qt desktop build verified. Evidence: $script:LogRoot"
}

try {
    Assert-HostPrerequisites
    New-Item -ItemType Directory -Force -Path $script:LogRoot, $script:ToolchainRoot | Out-Null
    Initialize-KBuild
    Ensure-Python
    Ensure-Nasm
    Ensure-Qt
    New-ExactSdkView
    Configure-Build
    $state = Build-Targets
    Write-Manifest $state
    $duration = [DateTimeOffset]::UtcNow - $script:RunStarted
    Write-Host ('[DONE] Native build completed in {0:hh\:mm\:ss\.fff}.' -f $duration)
    exit 0
}
catch {
    $duration = [DateTimeOffset]::UtcNow - $script:RunStarted
    Write-Error ('[FAILED] Native build stopped after {0:hh\:mm\:ss\.fff}: {1}' -f $duration, $_.Exception.Message)
    exit 1
}
