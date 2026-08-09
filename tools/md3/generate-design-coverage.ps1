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
$sha256 = [System.Security.Cryptography.SHA256]::Create()
$i = 0
foreach ($file in $files) {
    $i++
    $relative = $file.FullName.Substring($root.Length + 1).Replace('\','/')
    $bytes = [System.IO.File]::ReadAllBytes($file.FullName)
    # Git may materialize text files with CRLF on Deen No while the archive
    # hashes are defined over LF bytes. Normalize only textual entries; binary
    # icons and preview metadata must remain byte-for-byte exact.
    if ($file.Extension -in @('.cpp','.h','.html','.js','.kmk','.md') -or $file.Name -eq 'HANDOFF.md') {
        $text = [System.Text.Encoding]::UTF8.GetString($bytes).Replace("`r`n", "`n")
        $bytes = [System.Text.Encoding]::UTF8.GetBytes($text)
    }
    $hash = ([System.BitConverter]::ToString($sha256.ComputeHash($bytes))).Replace('-', '').ToLowerInvariant()
    $role = if ($relative -match '^cpp/md3/') { 'C++ reference' } elseif ($relative -match '^icons/') { 'icon' } elseif ($relative -eq 'support.js') { 'shared prototype behavior' } elseif ($relative -eq 'HANDOFF.md') { 'handoff' } elseif ($relative -eq '.thumbnail') { 'preview metadata' } else { 'prototype' }
    $disposition = if ($role -eq 'icon') { 'map to qrc and production call site' } elseif ($role -eq 'preview metadata') { 'consume as archive evidence' } elseif ($role -eq 'handoff') { 'consume as implementation authority' } else { 'integrate or adapt into native Qt' }
    $path = if ($relative -match '^cpp/md3/') { 'src/VBox/Frontends/VirtualBox/src/md3/' } elseif ($role -eq 'icon') { 'src/VBox/Frontends/VirtualBox/src/md3/icons/ and qrc' } else { 'doc/md3/DesignCoverage.md' }
    $rows.Add(('| ARCH-{0:D3} | `{1}` | `{2}` | {3} | {4} | `{5}` | build/test or screenshot coverage recorded per integration lane | In progress | Generated from the checked-in design archive. |' -f $i,$relative,$hash,$role,$disposition,$path))
    $manifestLines.Add(('{0}  {1}' -f $hash,$relative))
}
Set-Content -LiteralPath $Output -Value (($rows -join [Environment]::NewLine) + [Environment]::NewLine) -Encoding utf8
Set-Content -LiteralPath $Manifest -Value (($manifestLines -join [Environment]::NewLine) + [Environment]::NewLine) -Encoding ascii
Write-Output ("Generated {0} archive rows." -f $files.Count)
