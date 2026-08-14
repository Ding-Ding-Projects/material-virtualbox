[CmdletBinding()]
param(
    [string]$RepositoryRoot,
    [string]$WorkflowPath,
    [string]$InventoryPath
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

if (-not $RepositoryRoot) { $RepositoryRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path }
if (-not $WorkflowPath) { $WorkflowPath = Join-Path $RepositoryRoot '.github\workflows\md3-validation.yml' }
if (-not $InventoryPath) { $InventoryPath = Join-Path $RepositoryRoot 'tools\ci\windows-dependencies.json' }

function Fail-Completeness([string]$Message) {
    throw "Dependency completeness check failed: $Message"
}

if (-not (Test-Path -LiteralPath $WorkflowPath -PathType Leaf)) { Fail-Completeness "workflow is missing: $WorkflowPath" }
if (-not (Test-Path -LiteralPath $InventoryPath -PathType Leaf)) { Fail-Completeness "inventory is missing: $InventoryPath" }

try { $inventory = Get-Content -LiteralPath $InventoryPath -Raw | ConvertFrom-Json }
catch { Fail-Completeness "inventory is not valid JSON: $($_.Exception.Message)" }
if ($inventory.schemaVersion -ne 1) { Fail-Completeness "unsupported schemaVersion '$($inventory.schemaVersion)'" }

$workflow = Get-Content -LiteralPath $WorkflowPath -Raw
$jobsMatch = [regex]::Match($workflow, '(?ms)^jobs:\s*\r?\n(?<body>.*)$')
if (-not $jobsMatch.Success) { Fail-Completeness 'workflow has no jobs mapping' }
$workflowJobs = @([regex]::Matches($jobsMatch.Groups['body'].Value, '(?m)^  (?<name>[A-Za-z0-9_-]+):\s*$') | ForEach-Object { $_.Groups['name'].Value })
$inventoryJobs = @($inventory.jobs.PSObject.Properties.Name)
foreach ($job in $workflowJobs) {
    if ($inventoryJobs -notcontains $job) { Fail-Completeness "workflow job '$job' is absent from the handwritten inventory" }
}
foreach ($job in $inventoryJobs) {
    if ($workflowJobs -notcontains $job) { Fail-Completeness "inventory job '$job' no longer exists in the workflow" }
    $entry = $inventory.jobs.$job
    if (-not $entry.bootstrap) { Fail-Completeness "job '$job' has no bootstrap description" }
    if (@($entry.executables).Count -eq 0) { Fail-Completeness "job '$job' lists no invoked executables" }
    foreach ($executable in @($entry.executables)) {
        foreach ($field in @('name', 'source', 'version', 'invokedBy')) {
            if (-not $executable.$field) { Fail-Completeness "job '$job' executable is missing '$field'" }
        }
    }
}

$native = $inventory.jobs.'native-build'
$requiredExecutables = @(
    'cmd.exe', 'powershell.exe', 'pwsh.exe', 'git.exe', 'winget.exe', 'python.exe', 'vswhere.exe', 'cl.exe', 'link.exe',
    'rc.exe', 'midl.exe', 'signtool.exe', 'nasm.exe', 'aqtinstall', 'qmake.exe', 'moc.exe', 'rcc.exe', 'uic.exe',
    'lrelease.exe', 'kmk.exe', 'kmk_ash.exe'
)
$listedExecutables = @($native.executables.name)
foreach ($name in $requiredExecutables) {
    if ($listedExecutables -notcontains $name) { Fail-Completeness "native-build does not inventory '$name'" }
}

$requiredPackages = @('Microsoft.WindowsSDK.10.0.22621', 'Microsoft.WindowsWDK.10.0.22621', 'Qt')
foreach ($package in $requiredPackages) {
    if (@($native.packages.id) -notcontains $package) { Fail-Completeness "native-build does not inventory package '$package'" }
}

$buildScript = Join-Path $RepositoryRoot 'tools\md3\build-windows.ps1'
$batchScript = Join-Path $RepositoryRoot 'build.bat'
$requirements = Join-Path $RepositoryRoot 'tools\ci\aqt-requirements.txt'
foreach ($path in @($buildScript, $batchScript, $requirements)) {
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) { Fail-Completeness "required bootstrap input is missing: $path" }
}
$buildText = Get-Content -LiteralPath $buildScript -Raw
$batchText = Get-Content -LiteralPath $batchScript -Raw
foreach ($literal in @('3.13.15', '2.16.03', '6.8.3', '3.3.0', '10.0.22621.0', 'VCC143', 'qtscxml', 'Microsoft.WindowsSDK.10.0.22621', 'Microsoft.WindowsWDK.10.0.22621')) {
    if ($buildText -notmatch [regex]::Escape($literal)) { Fail-Completeness "build bootstrap does not pin '$literal'" }
}
foreach ($literal in @('/s', '--silent', 'SILENT', 'tools\md3\build-windows.ps1', 'UICommon.dll', 'VirtualBox.exe', 'VirtualBoxVM.exe', 'UICommon.lib')) {
    if ($batchText -notmatch [regex]::Escape($literal)) { Fail-Completeness "build.bat contract is missing '$literal'" }
}
foreach ($literal in @('UICommon', 'VirtualBox', 'VirtualBoxVM', "'-j1'", 'Get-AuthenticodeSignature', 'NotSigned', 'submodule', '--checkout', 'build-logs', 'material-virtualbox-toolchain')) {
    if ($buildText -notmatch [regex]::Escape($literal)) { Fail-Completeness "build bootstrap contract is missing '$literal'" }
}

$requirementsLines = @(Get-Content -LiteralPath $requirements | Where-Object { $_ -and -not $_.TrimStart().StartsWith('#') })
if ($requirementsLines.Count -eq 0) { Fail-Completeness 'aqt requirements are empty' }
foreach ($line in $requirementsLines) {
    if ($line -notmatch '^[-A-Za-z0-9_.]+==[^ ]+ --hash=sha256:[0-9a-f]{64}$') {
        Fail-Completeness "aqt requirement is not exactly pinned and hashed: $line"
    }
}
if (-not ($requirementsLines | Where-Object { $_ -match '^aqtinstall==3\.3\.0 ' })) { Fail-Completeness 'aqtinstall 3.3.0 is not pinned' }

$checkoutSha = '11d5960a326750d5838078e36cf38b85af677262'
$uploadSha = 'ea165f8d65b6e75b540449e92b4886f43607fa02'
foreach ($sha in @($checkoutSha, $uploadSha)) {
    if ($workflow -notmatch [regex]::Escape($sha)) { Fail-Completeness "workflow does not pin action SHA '$sha'" }
}
foreach ($literal in @('native-build:', 'fetch-depth: 0', 'submodules: recursive', './tools/ci/test-dependency-completeness.ps1', '.\build.bat /s', './tools/ci/collect-safe-artifacts.ps1', 'if-no-files-found: warn', 'retention-days: 7')) {
    if ($workflow -notmatch [regex]::Escape($literal)) { Fail-Completeness "workflow native build contract is missing '$literal'" }
}

Write-Host "Dependency completeness verified for $($workflowJobs.Count) workflow jobs and $($listedExecutables.Count) native-build executables."
