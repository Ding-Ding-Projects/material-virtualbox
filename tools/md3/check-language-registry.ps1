<#
.SYNOPSIS
    Localization-completeness gate for the UIMd3Language MD3 text registry.
.DESCRIPTION
    Parses every compile-time UIMd3Language::registerText(key, english, cantonese)
    call site in the Qt frontend -- including the REGISTER_*_TEXT convenience
    macros, which are discovered from their own #define bodies rather than
    hardcoded -- and fails when a key has an empty English string, an empty
    Cantonese string, an empty key, or is registered twice with conflicting text.

    Honest limits, stated so the script's own documentation matches its behaviour:

      * The parser is deliberately literal-only. Anything it cannot resolve to a
        compile-time string (runtime-computed keys, values arriving through
        lambda or helper-function parameters, any non-literal expression) is
        reported as "unparseable, NOT CHECKED" with an explicit count, an exact
        file:line, and the offending snippet. Unparseable sites are never
        silently treated as passing. Use -MaxUnparseable to freeze the known
        count so a new invisible call shape cannot be introduced unnoticed.
      * This is a static text gate. It proves nothing about compilation, about
        which registration actually executes at run time (QHash::insert
        overwrites, so registration order decides the winner), about keys that
        are read but never registered, about %1/%2 placeholder arity agreement
        between the two languages, or about translation quality.
.PARAMETER Root
    Repository root. Defaults to the parent of this script's parent directory.
.PARAMETER StrictIdentical
    Treat "English identical to Cantonese" as a failure instead of a warning.
.PARAMETER MaxUnparseable
    Fail if the number of unparseable call sites exceeds this budget.
    Default -1 means "report but do not fail".
#>
[CmdletBinding()]
param(
    [string]$Root,
    [switch]$StrictIdentical,
    [int]$MaxUnparseable = -1
)

$ErrorActionPreference = 'Stop'
# Registry data is UTF-8 Cantonese; make sure it survives the console.
try { [Console]::OutputEncoding = [System.Text.UTF8Encoding]::new($false) } catch { }

if (-not $Root) { $Root = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path }
$scanRoot = Join-Path $Root 'src/VBox/Frontends/VirtualBox'
if (-not (Test-Path $scanRoot)) { Write-Error "Frontend source root not found: $scanRoot"; exit 2 }

# Keys whose English and Cantonese are legitimately identical (proper nouns,
# regex flag letters, and other non-translatable tokens). Every entry needs a
# reason; this list is deliberately short and hand-maintained.
$IdenticalAllowList = @{
    'md3.application'             = 'Product name; not translated.'
    'md3.regex.flags-placeholder' = 'PCRE2 flag letters i m s x; not translatable.'
}

# ---------------------------------------------------------------- helpers ----

function Find-MatchingParen {
    param([string]$Text, [int]$Open)
    $depth = 0; $i = $Open; $n = $Text.Length
    while ($i -lt $n) {
        $c = $Text[$i]
        if ($c -eq '"') {
            $i++
            while ($i -lt $n) {
                if ($Text[$i] -eq '\') { $i += 2; continue }
                if ($Text[$i] -eq '"') { break }
                $i++
            }
        } elseif ($c -eq "'") {
            $i++
            while ($i -lt $n) {
                if ($Text[$i] -eq '\') { $i += 2; continue }
                if ($Text[$i] -eq "'") { break }
                $i++
            }
        } elseif ($c -eq '(') { $depth++ }
        elseif ($c -eq ')') { $depth--; if ($depth -eq 0) { return $i } }
        $i++
    }
    return -1
}

function Split-TopLevel {
    # Depth-aware and string-aware. A naive split would corrupt registrations
    # whose data legitimately contains a comma (", complete") or a plus sign
    # ("One or more +").
    param([string]$Text, [char]$Sep = ',')
    $out = [System.Collections.Generic.List[string]]::new()
    $sb = [System.Text.StringBuilder]::new()
    $depth = 0; $i = 0; $n = $Text.Length
    while ($i -lt $n) {
        $c = $Text[$i]
        if ($c -eq '"' -or $c -eq "'") {
            $q = $c; $j = $i + 1
            while ($j -lt $n) {
                if ($Text[$j] -eq '\') { $j += 2; continue }
                if ($Text[$j] -eq $q) { break }
                $j++
            }
            [void]$sb.Append($Text.Substring($i, $j - $i + 1)); $i = $j + 1; continue
        }
        if ('([{'.Contains($c)) { $depth++ }
        elseif (')]}'.Contains($c)) { $depth-- }
        elseif ($c -eq $Sep -and $depth -eq 0) {
            [void]$out.Add($sb.ToString()); $sb.Clear() | Out-Null; $i++; continue
        }
        [void]$sb.Append($c); $i++
    }
    [void]$out.Add($sb.ToString())
    return @($out | ForEach-Object { $_.Trim() })
}

$Wrappers = @('QStringLiteral', 'QLatin1String', 'QLatin1StringView',
              'QString::fromUtf8', 'QString::fromLatin1', 'QStringView')

function Remove-Wrappers {
    param([string]$Expr)
    $e = $Expr.Trim()
    $changed = $true
    while ($changed) {
        $changed = $false
        foreach ($w in $Wrappers) {
            if ($e.StartsWith("$w(")) {
                $close = Find-MatchingParen -Text $e -Open $w.Length
                if ($close -eq $e.Length - 1) { $e = $e.Substring($w.Length + 1, $close - $w.Length - 1).Trim(); $changed = $true; break }
            }
        }
    }
    return $e
}

function ConvertFrom-CppEscapes {
    param([string]$S)
    $sb = [System.Text.StringBuilder]::new(); $i = 0
    while ($i -lt $S.Length) {
        if ($S[$i] -eq '\' -and $i + 1 -lt $S.Length) {
            switch ($S[$i + 1]) {
                'n' { [void]$sb.Append("`n") } 't' { [void]$sb.Append("`t") }
                'r' { [void]$sb.Append("`r") } '0' { [void]$sb.Append([char]0) }
                default { [void]$sb.Append($S[$i + 1]) }
            }
            $i += 2
        } else { [void]$sb.Append($S[$i]); $i++ }
    }
    return $sb.ToString()
}

$StrLit = [regex]'"((?:[^"\\]|\\.)*)"'

function Resolve-Literal {
    # Returns @{ Ok = $bool; Value = <string> }. Joins adjacent string literals
    # so multi-line concatenated copy resolves to its complete text, and refuses
    # anything with non-whitespace residue outside the literals.
    param([string]$Expr)
    $e = Remove-Wrappers ($Expr -replace '\s+', ' ')
    $parts = Split-TopLevel -Text $e -Sep '+'
    $sb = [System.Text.StringBuilder]::new()
    foreach ($raw in $parts) {
        $p = Remove-Wrappers $raw
        $ms = $StrLit.Matches($p)
        if ($ms.Count -eq 0) { return @{ Ok = $false } }
        $residue = ''; $last = 0
        foreach ($m in $ms) { $residue += $p.Substring($last, $m.Index - $last); $last = $m.Index + $m.Length }
        $residue += $p.Substring($last)
        if ($residue.Trim()) { return @{ Ok = $false } }
        foreach ($m in $ms) { [void]$sb.Append((ConvertFrom-CppEscapes $m.Groups[1].Value)) }
    }
    return @{ Ok = $true; Value = $sb.ToString() }
}

function Get-LineNumber { param([string]$Text, [int]$Index)
    return ($Text.Substring(0, $Index).Split("`n").Count) }

# ------------------------------------------------------------------ scan ----

$records     = [System.Collections.Generic.List[object]]::new()
$unparseable = [System.Collections.Generic.List[object]]::new()

$files = @(Get-ChildItem -Path $scanRoot -Recurse -Include '*.cpp', '*.h' -File |
           Where-Object { (Get-Content -Raw -Encoding UTF8 $_.FullName) -match 'registerText' })

foreach ($file in $files) {
    $text = Get-Content -Raw -Encoding UTF8 $file.FullName
    $rel  = [System.IO.Path]::GetRelativePath($Root, $file.FullName).Replace('\', '/')

    # ---- discover REGISTER_*(k, en, zh) macros that forward to registerText
    $macros = @{}
    foreach ($d in [regex]::Matches($text, '(?m)^[ \t]*#[ \t]*define[ \t]+(\w+)\(([^)]*)\)')) {
        $name = $d.Groups[1].Value
        $params = @($d.Groups[2].Value -split ',' | ForEach-Object { $_.Trim() })
        # collect the full define including backslash continuations
        $body = ''; $pos = $d.Index
        while ($true) {
            $nl = $text.IndexOf("`n", $pos)
            $line = if ($nl -lt 0) { $text.Substring($pos) } else { $text.Substring($pos, $nl - $pos) }
            $body += $line + "`n"
            if (-not $line.TrimEnd().EndsWith('\') -or $nl -lt 0) { break }
            $pos = $nl + 1
        }
        if ($body -notmatch 'registerText') { continue }
        $rt = $body.IndexOf('registerText'); $op = $body.IndexOf('(', $rt)
        $cl = Find-MatchingParen -Text $body -Open $op
        if ($cl -lt 0) { continue }
        $margs = Split-TopLevel -Text $body.Substring($op + 1, $cl - $op - 1)
        if ($margs.Count -ne 3) { continue }
        $order = @()
        foreach ($a in $margs) { $order += $params.IndexOf((Remove-Wrappers $a)) }
        if ($order -contains -1) { continue }
        $macros[$name] = $order
    }

    # ---- direct registerText(...) call sites
    foreach ($m in [regex]::Matches($text, '\bregisterText\s*\(')) {
        $lineStart = $text.LastIndexOf("`n", $m.Index) + 1
        $lineText  = $text.Substring($lineStart, $m.Index - $lineStart)
        # skip the declaration in UIMd3Language.h and the definition in the .cpp
        if ($lineText -match '\bvoid\s*$' -or $lineText -match 'UIMd3Language::\s*$') { continue }
        # skip occurrences inside a #define body (handled positionally above)
        $probe = $lineStart; $inDefine = $false
        while ($true) {
            $nl = $text.IndexOf("`n", $probe)
            $ln = if ($nl -lt 0) { $text.Substring($probe) } else { $text.Substring($probe, $nl - $probe) }
            if ($ln -match '^[ \t]*#[ \t]*define\b') { $inDefine = $true; break }
            if ($probe -le 0) { break }
            $pe = $text.LastIndexOf("`n", $probe - 2) + 1
            if ($pe -lt 0 -or $pe -ge $probe) { break }
            if (-not $text.Substring($pe, $probe - 1 - $pe).TrimEnd().EndsWith('\')) { break }
            $probe = $pe
        }
        if ($inDefine) { continue }

        $op = $text.IndexOf('(', $m.Index)
        $cl = Find-MatchingParen -Text $text -Open $op
        if ($cl -lt 0) { continue }
        $line = Get-LineNumber -Text $text -Index $m.Index
        $callArgs = Split-TopLevel -Text $text.Substring($op + 1, $cl - $op - 1)
        $snippet = ($text.Substring($m.Index, [Math]::Min(160, $cl - $m.Index + 1)) -replace '\s+', ' ')
        if ($callArgs.Count -ne 3) {
            $unparseable.Add([pscustomobject]@{ File = $rel; Line = $line; Reason = "$($callArgs.Count) arguments"; Snippet = $snippet }); continue
        }
        $vals = @($callArgs | ForEach-Object { Resolve-Literal $_ })
        if ($vals | Where-Object { -not $_.Ok }) {
            $unparseable.Add([pscustomobject]@{ File = $rel; Line = $line; Reason = 'non-literal argument'; Snippet = $snippet }); continue
        }
        $records.Add([pscustomobject]@{ File = $rel; Line = $line; Key = $vals[0].Value
                                        En = $vals[1].Value; Zh = $vals[2].Value; Shape = 'direct' })
    }

    # ---- REGISTER_*_TEXT macro invocations
    foreach ($name in $macros.Keys) {
        $order = $macros[$name]
        foreach ($m in [regex]::Matches($text, "(?m)^[ \t]*$([regex]::Escape($name))\s*\(")) {
            $op = $text.IndexOf('(', $m.Index)
            $cl = Find-MatchingParen -Text $text -Open $op
            if ($cl -lt 0) { continue }
            $line = Get-LineNumber -Text $text -Index $m.Index
            $callArgs = Split-TopLevel -Text $text.Substring($op + 1, $cl - $op - 1)
            $snippet = ($text.Substring($m.Index, [Math]::Min(160, $cl - $m.Index + 1)) -replace '\s+', ' ')
            if ($callArgs.Count -ne 3) {
                $unparseable.Add([pscustomobject]@{ File = $rel; Line = $line; Reason = "$name : $($callArgs.Count) arguments"; Snippet = $snippet }); continue
            }
            $vals = @($order | ForEach-Object { Resolve-Literal $callArgs[$_] })
            if ($vals | Where-Object { -not $_.Ok }) {
                $unparseable.Add([pscustomobject]@{ File = $rel; Line = $line; Reason = "$name : non-literal argument"; Snippet = $snippet }); continue
            }
            $records.Add([pscustomobject]@{ File = $rel; Line = $line; Key = $vals[0].Value
                                            En = $vals[1].Value; Zh = $vals[2].Value; Shape = "macro:$name" })
        }
    }
}

# --------------------------------------------------------------- verdict ----

$violations = [System.Collections.Generic.List[string]]::new()
$warnings   = [System.Collections.Generic.List[string]]::new()

foreach ($r in $records) {
    if (-not $r.Key.Trim())  { $violations.Add("EMPTY KEY        (no key)  $($r.File):$($r.Line)") }
    if (-not $r.En.Trim())   { $violations.Add("EMPTY ENGLISH    '$($r.Key)'  $($r.File):$($r.Line)") }
    if (-not $r.Zh.Trim())   { $violations.Add("EMPTY CANTONESE  '$($r.Key)'  $($r.File):$($r.Line)") }
}

# Registering one key twice with the SAME text is deliberate co-registration by
# two owners and must pass. Only genuinely conflicting text is a violation,
# because QHash::insert overwrites and the loser is silently wrong.
foreach ($g in $records | Group-Object Key | Where-Object Count -gt 1) {
    $variants = @($g.Group | Group-Object { "$($_.En)`u{1}$($_.Zh)" })
    if ($variants.Count -le 1) { continue }
    $sites = ($g.Group | ForEach-Object { "$($_.File):$($_.Line) EN='$($_.En)' ZH='$($_.Zh)'" }) -join "`n                     "
    $violations.Add("CONFLICTING KEY  '$($g.Name)' registered $($g.Count)x with $($variants.Count) different texts:`n                     $sites")
}

# Every allow-list hit is recorded so an exemption can never be silent: the
# summary below prints the count and names each exempted key with its reason.
$allowListed = [System.Collections.Generic.List[string]]::new()
foreach ($r in $records | Where-Object { $_.En -eq $_.Zh -and $_.En.Trim() }) {
    if ($IdenticalAllowList.ContainsKey($r.Key)) {
        $allowListed.Add("'$($r.Key)' = '$($r.En)'  $($r.File):$($r.Line)  -- $($IdenticalAllowList[$r.Key])")
        continue
    }
    $msg = "IDENTICAL EN/ZH  '$($r.Key)' = '$($r.En)'  $($r.File):$($r.Line)"
    if ($StrictIdentical) { $violations.Add($msg) } else { $warnings.Add($msg) }
}

Write-Host '== UIMd3Language registry completeness =='
Write-Host ("  parsed registrations : {0}" -f $records.Count)
Write-Host ("  distinct keys        : {0}" -f ($records | Group-Object Key).Count)
Write-Host ("  files scanned        : {0}" -f $files.Count)
Write-Host ("  unparseable, NOT CHECKED : {0}" -f $unparseable.Count)
foreach ($u in $unparseable) {
    Write-Host ("    ? $($u.File):$($u.Line)  $($u.Reason)")
    Write-Host ("        $($u.Snippet)")
}
Write-Host ("  identical-allow-listed: {0}" -f $allowListed.Count)
foreach ($a in $allowListed) { Write-Host "    = $a" }
foreach ($w in $warnings)   { Write-Host "  WARN  $w" }
foreach ($v in $violations) { Write-Host "  FAIL  $v" }
Write-Host ("  violations: {0}   warnings: {1}" -f $violations.Count, $warnings.Count)

if ($MaxUnparseable -ge 0 -and $unparseable.Count -gt $MaxUnparseable) {
    Write-Host "  FAIL  unparseable call sites ($($unparseable.Count)) exceed the budget of $MaxUnparseable"
    exit 1
}
if ($violations.Count -gt 0) { exit 1 }
Write-Host '  RESULT: clean'
exit 0
