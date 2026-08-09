param(
    [string]$DesignRoot = (Join-Path $PSScriptRoot '..\..\design'),
    [string]$Output = (Join-Path $PSScriptRoot '..\..\doc\md3\DesignCoverage.md'),
    [string]$Manifest = (Join-Path $PSScriptRoot '..\..\doc\md3\ArchiveManifest.sha256')
)

$ErrorActionPreference = 'Stop'
$root = (Resolve-Path $DesignRoot).Path
$outDir = Split-Path -Parent $Output
New-Item -ItemType Directory -Force $outDir | Out-Null
$files = Get-ChildItem -LiteralPath $root -Recurse -File | Sort-Object FullName
$rows = [System.Collections.Generic.List[string]]::new()
$rows.Add('| Design ID | Archive path | SHA-256 | Source role | Required production disposition | Production path(s) | Behavior/test evidence | Status | Notes |')
$rows.Add('|---|---|---|---|---|---|---|---|---|')
$manifestLines = [System.Collections.Generic.List[string]]::new()
$i = 0
foreach ($file in $files) {
    $i++
    $relative = $file.FullName.Substring($root.Length + 1).Replace('\','/')
    $hash = (Get-FileHash -LiteralPath $file.FullName -Algorithm SHA256).Hash.ToLowerInvariant()
    $role = if ($relative -match '^cpp/md3/') { 'C++ reference' } elseif ($relative -match '^icons/') { 'icon' } elseif ($relative -eq 'support.js') { 'shared prototype behavior' } elseif ($relative -eq 'HANDOFF.md') { 'handoff' } elseif ($relative -eq '.thumbnail') { 'preview metadata' } else { 'prototype' }
    $disposition = if ($role -eq 'icon') { 'map to qrc and production call site' } elseif ($role -eq 'preview metadata') { 'consume as archive evidence' } elseif ($role -eq 'handoff') { 'consume as implementation authority' } else { 'integrate or adapt into native Qt' }
    $path = if ($relative -match '^cpp/md3/') { 'src/VBox/Frontends/VirtualBox/src/md3/' } elseif ($role -eq 'icon') { 'src/VBox/Frontends/VirtualBox/src/md3/icons/ and qrc' } else { 'doc/md3/DesignCoverage.md' }
    $rows.Add(('| ARCH-{0:D3} | `{1}` | `{2}` | {3} | {4} | `{5}` | build/test or screenshot coverage recorded per integration lane | In progress | Generated from the checked-in design archive. |' -f $i,$relative,$hash,$role,$disposition,$path))
    $manifestLines.Add(('{0}  {1}' -f $hash,$relative))
}
Set-Content -LiteralPath $Output -Value (($rows -join [Environment]::NewLine) + [Environment]::NewLine) -Encoding utf8
Set-Content -LiteralPath $Manifest -Value (($manifestLines -join [Environment]::NewLine) + [Environment]::NewLine) -Encoding ascii
Write-Output ("Generated {0} archive rows." -f $files.Count)
