[CmdletBinding()]
param(
    [string]$RepositoryRoot,
    [string]$Destination
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

if (-not $RepositoryRoot) { $RepositoryRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path }
if (-not $Destination) { $Destination = Join-Path $RepositoryRoot 'artifacts\native-build-evidence' }
New-Item -ItemType Directory -Force -Path $Destination | Out-Null

$metadata = [ordered]@{
    schemaVersion = 1
    collectedUtc = [DateTimeOffset]::UtcNow.ToString('o')
    runId = if ($env:GITHUB_RUN_ID) { $env:GITHUB_RUN_ID } else { 'local' }
    runAttempt = if ($env:GITHUB_RUN_ATTEMPT) { $env:GITHUB_RUN_ATTEMPT } else { 'local' }
    commitSha = if ($env:GITHUB_SHA) { $env:GITHUB_SHA } else { (& git -C $RepositoryRoot rev-parse HEAD).Trim() }
    job = if ($env:GITHUB_JOB) { $env:GITHUB_JOB } else { 'local' }
    runner = [ordered]@{
        os = if ($env:RUNNER_OS) { $env:RUNNER_OS } else { [Runtime.InteropServices.RuntimeInformation]::OSDescription }
        architecture = if ($env:RUNNER_ARCH) { $env:RUNNER_ARCH } else { [Runtime.InteropServices.RuntimeInformation]::OSArchitecture.ToString() }
        name = if ($env:RUNNER_NAME) { $env:RUNNER_NAME } else { 'local' }
    }
}
$metadata | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $Destination 'run-context.json') -Encoding utf8

$safeFiles = [ordered]@{
    'AutoConfig.kmk' = 'AutoConfig.kmk'
    'env.bat' = 'env.bat'
    'latest-native-build-manifest.json' = 'out\build-logs\latest-native-build-manifest.json'
}
foreach ($entry in $safeFiles.GetEnumerator()) {
    $source = Join-Path $RepositoryRoot $entry.Value
    if (Test-Path -LiteralPath $source -PathType Leaf) {
        Copy-Item -LiteralPath $source -Destination (Join-Path $Destination $entry.Key) -Force
    }
}

$logParent = Join-Path $RepositoryRoot 'out\build-logs'
if (Test-Path -LiteralPath $logParent -PathType Container) {
    $latest = Get-ChildItem -LiteralPath $logParent -Directory |
        Where-Object { $_.Name -match '^\d{8}T\d{9}Z$' } |
        Sort-Object Name -Descending |
        Select-Object -First 1
    if ($latest) {
        $logDestination = Join-Path $Destination 'logs'
        New-Item -ItemType Directory -Force -Path $logDestination | Out-Null
        $allowlist = @(
            'configure.log', 'configure-console.log', 'kbuild-submodule.log',
            'python-install.log', 'aqt-install.log', 'qt-install.log',
            'build-UICommon.log', 'build-VirtualBox.log', 'build-VirtualBoxVM.log',
            'build-start-utc.txt', 'native-build-manifest.json',
            'native-build-manifest.jsonl', 'native-build-manifest.md',
            'install-Microsoft.WindowsSDK.10.0.22621.log',
            'install-Microsoft.WindowsWDK.10.0.22621.log'
        )
        foreach ($name in $allowlist) {
            $source = Join-Path $latest.FullName $name
            if (Test-Path -LiteralPath $source -PathType Leaf) {
                $item = Get-Item -LiteralPath $source
                if ($item.Length -le 20MB) {
                    Copy-Item -LiteralPath $source -Destination (Join-Path $logDestination $name) -Force
                }
            }
        }
    }
}

@(
    '# Native build artifact collection', '',
    '- This directory contains only allowlisted configuration, logs, manifests, and run context.',
    '- Source trees, dependency directories, caches, profiles, environment dumps, credentials, and signing material are excluded.',
    '- Missing files are allowed so failed bootstrap phases still upload the evidence that exists.'
) | Set-Content -LiteralPath (Join-Path $Destination 'README.md') -Encoding utf8

Write-Host "Collected allowlisted native build evidence at '$Destination'."
