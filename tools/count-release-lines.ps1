#Requires -Version 7.0
<#
.SYNOPSIS
    Prints the repository line-count table that every release publishes.

.DESCRIPTION
    Emits exactly one GitHub-flavoured Markdown table on standard output, followed by
    caption lines that state every rule the table depends on: which files were counted,
    which subtrees are excluded from the project total, how binary files are handled,
    and how agent-versus-human attribution was derived.

    Only git-tracked files are counted. Diagnostics and the full skipped-file list are
    written to standard error so that standard output stays a clean Markdown fragment
    which the release workflow can append to its notes verbatim.

    Reproduce locally with:
        pwsh -NoProfile -File tools/count-release-lines.ps1

.PARAMETER AttributionBudgetSeconds
    Wall-clock budget for the git blame phase. When the budget is exhausted the
    remaining candidate files are skipped deterministically (candidates are processed
    in ordinal path order) and both the count and the paths are reported.

.PARAMETER MaxSkippedPathsInCaption
    How many skipped paths to name inline in the caption. Any remainder is still
    written in full to standard error.
#>
[CmdletBinding()]
param(
    [ValidateRange(1, 86400)]
    [int] $AttributionBudgetSeconds = 480,

    [ValidateRange(0, 10000)]
    [int] $MaxSkippedPathsInCaption = 25
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

try { [Console]::OutputEncoding = [System.Text.Encoding]::UTF8 } catch { }

$script:StartedAt = [System.Diagnostics.Stopwatch]::StartNew()

function Write-Diagnostic {
    param([string] $Message)
    [Console]::Error.WriteLine("[count-release-lines] $Message")
}

function Invoke-Git {
    <#
        Runs git and returns its exit code plus output. Native commands routinely write
        progress to stderr; with $ErrorActionPreference = 'Stop' that can surface as a
        terminating RemoteException even when git succeeded, so the preference is
        relaxed around the call and success is judged from the exit code alone.
    #>
    param(
        [Parameter(Mandatory)][string[]] $Arguments,
        [switch] $AllowFailure,
        [string] $StandardInput
    )

    $previous = $ErrorActionPreference
    $ErrorActionPreference = 'Continue'
    try {
        if ($PSBoundParameters.ContainsKey('StandardInput')) {
            $output = $StandardInput | & git @Arguments 2>$null
        } else {
            $output = & git @Arguments 2>$null
        }
        $code = $LASTEXITCODE
    } finally {
        $ErrorActionPreference = $previous
    }

    if ($code -ne 0 -and -not $AllowFailure) {
        throw "git $($Arguments -join ' ') failed with exit code $code."
    }

    return [pscustomobject]@{
        ExitCode = $code
        Output   = @($output)
    }
}

# ---------------------------------------------------------------------------
# Scanner. Line counting and classification run in compiled code because the
# tree carries tens of thousands of tracked files and a PowerShell byte loop
# over that much content would dominate the runtime budget.
# ---------------------------------------------------------------------------

Add-Type -TypeDefinition @'
using System;
using System.Collections.Generic;
using System.IO;

public static class VBoxReleaseLineScanner
{
    public const int CategoryProductSource = 0;
    public const int CategoryTests         = 1;
    public const int CategoryDocumentation = 2;
    public const int CategoryBuildSystem   = 3;
    public const int CategoryTooling       = 4;
    public const int CategoryOther         = 5;
    public const int CategoryVendored      = 6;
    public const int CategoryCount         = 7;

    private const int NullSniffWindow = 8192;

    private static readonly HashSet<string> BinaryExtensions = new HashSet<string>(StringComparer.OrdinalIgnoreCase)
    {
        ".png", ".jpg", ".jpeg", ".gif", ".bmp", ".ico", ".icns", ".webp", ".tif", ".tiff", ".svgz",
        ".zip", ".7z", ".gz", ".bz2", ".xz", ".lzma", ".tar", ".tgz", ".rar", ".cab", ".msi",
        ".exe", ".dll", ".sys", ".pdb", ".lib", ".obj", ".o", ".so", ".dylib", ".a", ".ko",
        ".pdf", ".ttf", ".otf", ".woff", ".woff2", ".eot",
        ".wav", ".mp3", ".ogg", ".mp4", ".avi", ".mkv", ".mov", ".flac",
        ".iso", ".vdi", ".vmdk", ".vhd", ".img", ".rom", ".fd", ".efi", ".bin", ".dat",
        ".class", ".jar", ".pyc", ".pyo", ".pyd", ".nupkg", ".jnilib"
    };

    /// <summary>Classifies a repository-relative, forward-slash path into one category.</summary>
    /// <remarks>
    /// Precedence is fixed and deliberate: vendored beats tests beats documentation beats
    /// build system beats tooling beats product source, with an explicit catch-all last.
    /// Vendored wins first so that a third-party makefile is never counted as this
    /// project's build system.
    /// </remarks>
    public static int Classify(string path)
    {
        if (IsVendored(path)) return CategoryVendored;
        if (IsTests(path)) return CategoryTests;
        if (path.StartsWith("doc/", StringComparison.Ordinal)) return CategoryDocumentation;
        if (IsBuildSystem(path)) return CategoryBuildSystem;
        if (IsTooling(path)) return CategoryTooling;
        if (path.StartsWith("src/VBox/", StringComparison.Ordinal) ||
            path.StartsWith("include/", StringComparison.Ordinal)) return CategoryProductSource;
        return CategoryOther;
    }

    private static bool IsVendored(string path)
    {
        return path.StartsWith("src/libs/", StringComparison.Ordinal)
            || path.StartsWith("src/VBox/Additions/3D/mesa", StringComparison.Ordinal)
            || path.StartsWith("src/VBox/Devices/EFI/Firmware/", StringComparison.Ordinal);
    }

    private static bool IsTests(string path)
    {
        return path.StartsWith("src/VBox/ValidationKit/", StringComparison.Ordinal)
            || path.StartsWith("testcase/", StringComparison.Ordinal)
            || path.IndexOf("/testcase/", StringComparison.Ordinal) >= 0;
    }

    private static bool IsBuildSystem(string path)
    {
        if (path.StartsWith("kBuild/", StringComparison.Ordinal)) return true;
        if (path.EndsWith(".kmk", StringComparison.Ordinal)) return true;
        // "configure*" means the generated-configuration entry points at the repository
        // root only; a file called "configure" deep inside a subtree belongs to whatever
        // component owns it.
        return path.IndexOf('/') < 0 && path.StartsWith("configure", StringComparison.OrdinalIgnoreCase);
    }

    private static bool IsTooling(string path)
    {
        return path.StartsWith("tools/", StringComparison.Ordinal)
            || path.StartsWith("webtools/", StringComparison.Ordinal)
            || path.StartsWith("debian/", StringComparison.Ordinal);
    }

    /// <summary>
    /// Scans one file. Returns { isBinary, totalLines, nonBlankLines }.
    /// A trailing line without a terminating newline still counts as a line, which is
    /// what git blame does, so the two totals can be compared directly.
    /// </summary>
    public static long[] ScanFile(string fullPath)
    {
        string extension = Path.GetExtension(fullPath);
        if (extension.Length > 0 && BinaryExtensions.Contains(extension))
        {
            return new long[] { 1, 0, 0 };
        }

        long total = 0;
        long nonBlank = 0;
        long consumed = 0;
        bool binary = false;
        bool lineHasContent = false;
        bool linePending = false;

        byte[] buffer = new byte[65536];
        using (FileStream stream = new FileStream(fullPath, FileMode.Open, FileAccess.Read, FileShare.ReadWrite, buffer.Length, FileOptions.SequentialScan))
        {
            int read;
            while ((read = stream.Read(buffer, 0, buffer.Length)) > 0)
            {
                for (int i = 0; i < read; i++)
                {
                    byte value = buffer[i];
                    if (value == 0 && (consumed + i) < NullSniffWindow)
                    {
                        binary = true;
                        break;
                    }

                    if (value == 10)
                    {
                        total++;
                        if (lineHasContent) nonBlank++;
                        lineHasContent = false;
                        linePending = false;
                    }
                    else
                    {
                        linePending = true;
                        if (value != 32 && value != 9 && value != 13 && value != 11 && value != 12)
                        {
                            lineHasContent = true;
                        }
                    }
                }

                if (binary) break;
                consumed += read;
            }
        }

        if (binary) return new long[] { 1, 0, 0 };

        if (linePending)
        {
            total++;
            if (lineHasContent) nonBlank++;
        }

        return new long[] { 0, total, nonBlank };
    }

    /// <summary>
    /// Scans every supplied repository-relative path and returns per-category totals as a
    /// flat array of CategoryCount * 4 entries: files, binaryFiles, totalLines, nonBlankLines.
    /// Unreadable files are reported through the <paramref name="failures"/> list rather
    /// than aborting the scan.
    /// </summary>
    public static long[] ScanAll(string root, string[] paths, List<string> failures)
    {
        long[] totals = new long[CategoryCount * 4];
        object gate = new object();

        // Reading tens of thousands of small files is dominated by per-file open
        // latency rather than by decoding, so the work is spread across threads and
        // merged once per partition. Integer sums are order-independent, so the
        // result stays identical to the sequential scan.
        System.Threading.Tasks.Parallel.ForEach(
            System.Collections.Concurrent.Partitioner.Create(0, paths.Length),
            () => new ScanState(CategoryCount * 4),
            (range, state, local) =>
            {
                for (int index = range.Item1; index < range.Item2; index++)
                {
                    string relative = paths[index];
                    int slot = Classify(relative) * 4;
                    local.Totals[slot]++;

                    string full = Path.Combine(root, relative.Replace('/', Path.DirectorySeparatorChar));
                    long[] scan;
                    try
                    {
                        scan = ScanFile(full);
                    }
                    catch (Exception error)
                    {
                        local.Failures.Add(relative + ": " + error.Message);
                        local.Totals[slot + 1]++;
                        continue;
                    }

                    if (scan[0] == 1)
                    {
                        local.Totals[slot + 1]++;
                    }
                    else
                    {
                        local.Totals[slot + 2] += scan[1];
                        local.Totals[slot + 3] += scan[2];
                    }
                }
                return local;
            },
            local =>
            {
                lock (gate)
                {
                    for (int i = 0; i < totals.Length; i++) totals[i] += local.Totals[i];
                    failures.AddRange(local.Failures);
                }
            });

        return totals;
    }

    private sealed class ScanState
    {
        public readonly long[] Totals;
        public readonly List<string> Failures = new List<string>();
        public ScanState(int size) { Totals = new long[size]; }
    }
}
'@

# ---------------------------------------------------------------------------
# Repository inventory.
# ---------------------------------------------------------------------------

$repositoryRoot = (Invoke-Git -Arguments @('rev-parse', '--show-toplevel')).Output |
    Select-Object -First 1
if ([string]::IsNullOrWhiteSpace($repositoryRoot)) {
    throw 'Unable to resolve the repository root; run this script from inside the checkout.'
}
$repositoryRoot = $repositoryRoot.Trim()

# Every git invocation below assumes the whole tree is in scope. Run from a
# subdirectory and "git ls-files" would quietly list that subdirectory alone, so the
# table would be a confident count of the wrong thing.
Set-Location -LiteralPath $repositoryRoot

$commit = ((Invoke-Git -Arguments @('rev-parse', 'HEAD')).Output | Select-Object -First 1).Trim()

Write-Diagnostic "repository root: $repositoryRoot"
Write-Diagnostic "commit: $commit"

$indexRaw = (Invoke-Git -Arguments @('-c', 'core.quotePath=off', 'ls-files', '-s', '-z')).Output -join ''
$indexEntries = $indexRaw -split "`0" | Where-Object { $_ -ne '' }

$trackedPaths = [System.Collections.Generic.List[string]]::new()
$gitlinkPaths = [System.Collections.Generic.List[string]]::new()

foreach ($entry in $indexEntries) {
    $tab = $entry.IndexOf("`t")
    if ($tab -lt 0) { continue }
    $mode = $entry.Substring(0, 6)
    $path = $entry.Substring($tab + 1)
    if ($mode -eq '160000') {
        # A submodule gitlink is a commit pointer, not a file with lines.
        $gitlinkPaths.Add($path)
        continue
    }
    $trackedPaths.Add($path)
}

if ($trackedPaths.Count -eq 0) {
    throw 'git ls-files reported no tracked files.'
}

Write-Diagnostic "tracked files: $($trackedPaths.Count) (skipped gitlinks: $($gitlinkPaths.Count))"

$scanFailures = [System.Collections.Generic.List[string]]::new()
$categoryTotals = [VBoxReleaseLineScanner]::ScanAll($repositoryRoot, $trackedPaths.ToArray(), $scanFailures)

foreach ($failure in $scanFailures) {
    Write-Diagnostic "unreadable, counted as a binary file: $failure"
}

Write-Diagnostic ("scan finished after {0:0.0}s" -f $script:StartedAt.Elapsed.TotalSeconds)

$categoryOrder = @(
    @{ Index = 0; Label = 'Product source (`src/VBox` and `include`, vendored subtrees removed)'; Vendored = $false },
    @{ Index = 1; Label = 'Tests (`src/VBox/ValidationKit` and `testcase` directories)';          Vendored = $false },
    @{ Index = 2; Label = 'Documentation (`doc`)';                                                Vendored = $false },
    @{ Index = 3; Label = 'Build system (`kBuild`, `*.kmk`, root `configure*`)';                  Vendored = $false },
    @{ Index = 4; Label = 'Tooling (`tools`, `webtools`, `debian`)';                              Vendored = $false },
    @{ Index = 5; Label = 'Other (tracked, matched by no rule above)';                            Vendored = $false },
    @{ Index = 6; Label = 'Vendored / third-party (excluded from the project total)';             Vendored = $true }
)

$rows = foreach ($category in $categoryOrder) {
    $slot = $category.Index * 4
    [pscustomobject]@{
        Label       = $category.Label
        Vendored    = $category.Vendored
        Files       = $categoryTotals[$slot]
        BinaryFiles = $categoryTotals[$slot + 1]
        TotalLines  = $categoryTotals[$slot + 2]
        NonBlank    = $categoryTotals[$slot + 3]
    }
}

$grandFiles = ($rows | Measure-Object -Property Files -Sum).Sum
$grandBinary = ($rows | Measure-Object -Property BinaryFiles -Sum).Sum
$grandTotal = ($rows | Measure-Object -Property TotalLines -Sum).Sum
$grandNonBlank = ($rows | Measure-Object -Property NonBlank -Sum).Sum

$projectRows = $rows | Where-Object { -not $_.Vendored }
$projectFiles = ($projectRows | Measure-Object -Property Files -Sum).Sum
$projectBinary = ($projectRows | Measure-Object -Property BinaryFiles -Sum).Sum
$projectTotal = ($projectRows | Measure-Object -Property TotalLines -Sum).Sum
$projectNonBlank = ($projectRows | Measure-Object -Property NonBlank -Sum).Sum

# ---------------------------------------------------------------------------
# Attribution.
#
# A line is agent-written when the commit git blame attributes it to was authored by
# "Claude Fable 5" or carries a Co-Authored-By trailer naming a Claude agent. Blaming
# every tracked file on a tree this size is not affordable, so blame runs only over the
# files that at least one such commit touched. Every other tracked file has no agent
# commit anywhere in its history, so its surviving lines cannot be agent-written and are
# counted as human-written by construction.
# ---------------------------------------------------------------------------

$attribution = [ordered]@{
    Available          = $false
    Reason             = ''
    AgentCommits       = 0
    CandidateFiles     = 0
    BlamedFiles        = 0
    BinaryCandidates   = 0
    SkippedFiles       = [System.Collections.Generic.List[string]]::new()
    FailedFiles        = [System.Collections.Generic.List[string]]::new()
    AgentTotal         = [long]0
    AgentNonBlank      = [long]0
    HumanTotal         = [long]0
    HumanNonBlank      = [long]0
    UnattributedTotal  = [long]0
    UnattributedNonBlank = [long]0
    Shallow            = $false
    HistoryDepth       = 0
    Capped             = $false
}

try {
    $shallowProbe = Invoke-Git -Arguments @('rev-parse', '--is-shallow-repository') -AllowFailure
    $attribution.Shallow = (($shallowProbe.Output | Select-Object -First 1) -eq 'true')
    $depthProbe = Invoke-Git -Arguments @('rev-list', '--count', 'HEAD') -AllowFailure
    if ($depthProbe.ExitCode -eq 0) {
        $attribution.HistoryDepth = [int](($depthProbe.Output | Select-Object -First 1).Trim())
    }

    $agentShas = [System.Collections.Generic.HashSet[string]]::new([StringComparer]::Ordinal)

    $byAuthor = Invoke-Git -Arguments @('log', '--format=%H', '--author=Claude Fable 5') -AllowFailure
    if ($byAuthor.ExitCode -eq 0) {
        foreach ($sha in $byAuthor.Output) { if ($sha) { [void]$agentShas.Add($sha.Trim()) } }
    }

    $byTrailer = Invoke-Git -Arguments @(
        'log', '--format=%H', '--extended-regexp', '--regexp-ignore-case',
        '--grep=^Co-Authored-By:.*Claude'
    ) -AllowFailure
    if ($byTrailer.ExitCode -eq 0) {
        foreach ($sha in $byTrailer.Output) { if ($sha) { [void]$agentShas.Add($sha.Trim()) } }
    }

    $attribution.AgentCommits = $agentShas.Count
    Write-Diagnostic "agent commits discovered: $($agentShas.Count)"

    if ($agentShas.Count -eq 0) {
        $attribution.Available = $true
        $attribution.HumanTotal = $grandTotal
        $attribution.HumanNonBlank = $grandNonBlank
    } else {
        $touched = Invoke-Git `
            -Arguments @('diff-tree', '--stdin', '--no-commit-id', '--name-only', '-r', '-m', '--first-parent') `
            -StandardInput (($agentShas | Sort-Object) -join "`n") `
            -AllowFailure
        if ($touched.ExitCode -ne 0) {
            throw "git diff-tree failed with exit code $($touched.ExitCode)."
        }

        $trackedLookup = [System.Collections.Generic.HashSet[string]]::new([StringComparer]::Ordinal)
        foreach ($path in $trackedPaths) { [void]$trackedLookup.Add($path) }

        $candidates = [System.Collections.Generic.SortedSet[string]]::new([StringComparer]::Ordinal)
        foreach ($path in $touched.Output) {
            if (-not $path) { continue }
            $trimmed = $path.Trim()
            if ($trimmed -and $trackedLookup.Contains($trimmed)) { [void]$candidates.Add($trimmed) }
        }

        $attribution.CandidateFiles = $candidates.Count
        Write-Diagnostic "attribution candidates: $($candidates.Count)"

        $headerPattern = [regex]::new('^([0-9a-f]{40})\s+\d+\s+\d+', 'Compiled')
        $blameClock = [System.Diagnostics.Stopwatch]::StartNew()

        foreach ($relative in $candidates) {
            if ($blameClock.Elapsed.TotalSeconds -ge $AttributionBudgetSeconds) {
                $attribution.Capped = $true
                $attribution.SkippedFiles.Add($relative)
                continue
            }

            $full = Join-Path $repositoryRoot $relative.Replace('/', [System.IO.Path]::DirectorySeparatorChar)
            $ourScan = $null
            try { $ourScan = [VBoxReleaseLineScanner]::ScanFile($full) } catch { $ourScan = $null }
            if ($null -eq $ourScan -or $ourScan[0] -eq 1) {
                # Binary or unreadable: it contributes no lines to any total, so there is
                # nothing to attribute. Counted so the caption can account for every
                # candidate rather than leaving a gap between candidates and blames.
                $attribution.BinaryCandidates++
                continue
            }

            $blame = Invoke-Git -Arguments @('blame', '--porcelain', '--no-progress', '--', $relative) -AllowFailure
            if ($blame.ExitCode -ne 0) {
                $attribution.FailedFiles.Add($relative)
                $attribution.UnattributedTotal += [long]$ourScan[1]
                $attribution.UnattributedNonBlank += [long]$ourScan[2]
                continue
            }

            $agentLines = [long]0
            $agentNonBlank = [long]0
            $blameLines = [long]0
            $blameNonBlank = [long]0
            $currentSha = ''

            foreach ($line in $blame.Output) {
                if ($null -eq $line) { continue }
                if ($line.StartsWith("`t")) {
                    $blameLines++
                    $content = $line.Substring(1)
                    $isNonBlank = -not [string]::IsNullOrWhiteSpace($content)
                    if ($isNonBlank) { $blameNonBlank++ }
                    if ($currentSha -and $agentShas.Contains($currentSha)) {
                        $agentLines++
                        if ($isNonBlank) { $agentNonBlank++ }
                    }
                    continue
                }
                $match = $headerPattern.Match($line)
                if ($match.Success) { $currentSha = $match.Groups[1].Value }
            }

            $attribution.BlamedFiles++
            $attribution.AgentTotal += $agentLines
            $attribution.AgentNonBlank += $agentNonBlank
            $attribution.HumanTotal += ($blameLines - $agentLines)
            $attribution.HumanNonBlank += ($blameNonBlank - $agentNonBlank)

            # Any disagreement between our scan and git blame (there should be none) is
            # parked in the unattributed bucket so the reported figures still add up.
            $residualTotal = [long]$ourScan[1] - $blameLines
            $residualNonBlank = [long]$ourScan[2] - $blameNonBlank
            if ($residualTotal -ne 0 -or $residualNonBlank -ne 0) {
                Write-Diagnostic "blame/scan disagreement in ${relative}: scan=$($ourScan[1])/$($ourScan[2]) blame=$blameLines/$blameNonBlank"
                $attribution.UnattributedTotal += $residualTotal
                $attribution.UnattributedNonBlank += $residualNonBlank
            }
        }

        foreach ($relative in $attribution.SkippedFiles) {
            $full = Join-Path $repositoryRoot $relative.Replace('/', [System.IO.Path]::DirectorySeparatorChar)
            try {
                $scan = [VBoxReleaseLineScanner]::ScanFile($full)
                if ($scan[0] -eq 0) {
                    $attribution.UnattributedTotal += [long]$scan[1]
                    $attribution.UnattributedNonBlank += [long]$scan[2]
                }
            } catch { }
        }

        # Everything the blame phase never looked at has no agent commit in its history.
        $coveredTotal = [long]0
        $coveredNonBlank = [long]0
        foreach ($relative in $candidates) {
            $full = Join-Path $repositoryRoot $relative.Replace('/', [System.IO.Path]::DirectorySeparatorChar)
            try {
                $scan = [VBoxReleaseLineScanner]::ScanFile($full)
                if ($scan[0] -eq 0) {
                    $coveredTotal += [long]$scan[1]
                    $coveredNonBlank += [long]$scan[2]
                }
            } catch { }
        }
        $attribution.HumanTotal += ($grandTotal - $coveredTotal)
        $attribution.HumanNonBlank += ($grandNonBlank - $coveredNonBlank)
        $attribution.Available = $true
    }
} catch {
    $attribution.Available = $false
    $attribution.Reason = $_.Exception.Message
    Write-Diagnostic "attribution unavailable: $($attribution.Reason)"
}

if ($attribution.Capped) {
    Write-Diagnostic "attribution budget of ${AttributionBudgetSeconds}s exhausted; skipped $($attribution.SkippedFiles.Count) file(s):"
    foreach ($skipped in $attribution.SkippedFiles) { Write-Diagnostic "  skipped: $skipped" }
}
foreach ($failed in $attribution.FailedFiles) { Write-Diagnostic "  blame failed: $failed" }

# ---------------------------------------------------------------------------
# Self-checks. A table whose rows do not add up to its own totals is worse than
# no table at all, so a mismatch is a hard failure.
# ---------------------------------------------------------------------------

$problems = [System.Collections.Generic.List[string]]::new()

if ($grandFiles -ne $trackedPaths.Count) {
    $problems.Add("category file counts sum to $grandFiles but git ls-files reported $($trackedPaths.Count) countable files.")
}
if (($projectFiles + ($rows | Where-Object { $_.Vendored } | Measure-Object -Property Files -Sum).Sum) -ne $grandFiles) {
    $problems.Add('project and vendored file counts do not reconstitute the grand total.')
}
if (($projectTotal + ($rows | Where-Object { $_.Vendored } | Measure-Object -Property TotalLines -Sum).Sum) -ne $grandTotal) {
    $problems.Add('project and vendored line totals do not reconstitute the grand total.')
}
if (($projectNonBlank + ($rows | Where-Object { $_.Vendored } | Measure-Object -Property NonBlank -Sum).Sum) -ne $grandNonBlank) {
    $problems.Add('project and vendored non-blank totals do not reconstitute the grand total.')
}
if ($attribution.Available) {
    $attributedTotal = $attribution.AgentTotal + $attribution.HumanTotal + $attribution.UnattributedTotal
    if ($attributedTotal -ne $grandTotal) {
        $problems.Add("attribution rows sum to $attributedTotal but the grand total is $grandTotal.")
    }
    $attributedNonBlank = $attribution.AgentNonBlank + $attribution.HumanNonBlank + $attribution.UnattributedNonBlank
    if ($attributedNonBlank -ne $grandNonBlank) {
        $problems.Add("attribution non-blank rows sum to $attributedNonBlank but the grand total is $grandNonBlank.")
    }
}

# ---------------------------------------------------------------------------
# Output.
# ---------------------------------------------------------------------------

function Format-Number {
    param([Parameter(Mandatory)] $Value)
    return ('{0:N0}' -f [long]$Value)
}

function Add-TableRow {
    <#
        Builds one Markdown row from already-formatted cells.

        This exists because of a parse that is easy to write and hard to see. Inside
        the parentheses of a .NET method call, a comma separates method arguments, so
        $builder.AppendLine('{0}-{1}' -f 'a', 'b') hands AppendLine two arguments and
        gives the format operator only 'a' — every placeholder past {0} then fails with
        an argument-list index error. It is the method-call parentheses that do it, not
        the line break: the same expression assigned to a variable first works fine on
        one line or five. Building the row here, away from any method call, removes the
        trap rather than tiptoeing around it.
    #>
    param(
        [Parameter(Mandatory)][System.Text.StringBuilder] $Builder,
        [Parameter(Mandatory)][string] $Label,
        [Parameter(Mandatory)][AllowEmptyCollection()][string[]] $Cells,
        [switch] $Strong
    )

    $renderedCells = if ($Strong) { @($Cells | ForEach-Object { "**$_**" }) } else { @($Cells) }
    $renderedLabel = if ($Strong) { "**$Label**" } else { $Label }
    $line = (@($renderedLabel) + $renderedCells) -join ' | '
    [void]$Builder.AppendLine('| ' + $line + ' |')
}

$builder = [System.Text.StringBuilder]::new()
[void]$builder.AppendLine('| Category | Files | Binary files | Total lines | Non-blank lines |')
[void]$builder.AppendLine('| --- | ---: | ---: | ---: | ---: |')

foreach ($row in $rows) {
    Add-TableRow -Builder $builder -Label $row.Label -Cells @(
        (Format-Number $row.Files),
        (Format-Number $row.BinaryFiles),
        (Format-Number $row.TotalLines),
        (Format-Number $row.NonBlank)
    )
}

Add-TableRow -Builder $builder -Label 'Project total (vendored excluded)' -Strong -Cells @(
    (Format-Number $projectFiles),
    (Format-Number $projectBinary),
    (Format-Number $projectTotal),
    (Format-Number $projectNonBlank)
)
Add-TableRow -Builder $builder -Label 'Grand total (everything tracked)' -Strong -Cells @(
    (Format-Number $grandFiles),
    (Format-Number $grandBinary),
    (Format-Number $grandTotal),
    (Format-Number $grandNonBlank)
)

if ($attribution.Available) {
    Add-TableRow -Builder $builder -Label 'Agent-written (surviving lines)' -Cells @(
        '–', '–', (Format-Number $attribution.AgentTotal), (Format-Number $attribution.AgentNonBlank)
    )
    Add-TableRow -Builder $builder -Label 'Human-written (surviving lines)' -Cells @(
        '–', '–', (Format-Number $attribution.HumanTotal), (Format-Number $attribution.HumanNonBlank)
    )
    Add-TableRow -Builder $builder -Label 'Unattributed (not blamed)' -Cells @(
        '–', '–', (Format-Number $attribution.UnattributedTotal), (Format-Number $attribution.UnattributedNonBlank)
    )
} else {
    [void]$builder.AppendLine('| Agent-written (surviving lines) | – | – | not measured | not measured |')
    [void]$builder.AppendLine('| Human-written (surviving lines) | – | – | not measured | not measured |')
    [void]$builder.AppendLine('| Unattributed (not blamed) | – | – | not measured | not measured |')
}

$captions = [System.Collections.Generic.List[string]]::new()
$captions.Add("Measured at commit ``$commit``. Reproduce with ``pwsh -NoProfile -File tools/count-release-lines.ps1`` from the repository root.")
$captions.Add("Counted files come from ``git ls-files`` only. $($gitlinkPaths.Count) submodule gitlink(s) are commit pointers rather than files and are not counted.")
$captions.Add('Vendored and third-party subtrees are `src/libs`, `src/VBox/Additions/3D/mesa*` and `src/VBox/Devices/EFI/Firmware`. They stay visible in their own row and in the grand total, and are excluded from the project total.')
$captions.Add('Category precedence is vendored, then tests, then documentation, then build system, then tooling, then product source, with an explicit catch-all row so no tracked file is silently dropped. A `*.kmk` makefile therefore counts as build system wherever it lives, unless it sits inside a vendored subtree.')
$captions.Add('Binary files are detected by a known binary extension or by a NUL byte in the first 8192 bytes. They are counted as files and excluded from every line total.')

if ($attribution.Available -and $attribution.AgentCommits -gt 0) {
    $captions.Add("Attribution rule: a surviving line is agent-written when the commit ``git blame`` attributes it to has author ``Claude Fable 5`` or carries a ``Co-Authored-By`` trailer naming a Claude agent. $($attribution.AgentCommits) such commit(s) were found.")
    $captions.Add("To keep the run bounded on this tree, ``git blame`` was restricted to the $($attribution.CandidateFiles) tracked file(s) that at least one of those commits touched: $($attribution.BlamedFiles) carry lines and were blamed, and $($attribution.BinaryCandidates) are binary and carry none. Every other tracked file has no such commit in its history, so its surviving lines are reported as human-written.")
} elseif ($attribution.Available) {
    $captions.Add('Attribution rule: a surviving line is agent-written when the commit `git blame` attributes it to has author `Claude Fable 5` or carries a `Co-Authored-By` trailer naming a Claude agent. No such commit exists in the available history, so every surviving line is reported as human-written.')
} else {
    $captions.Add("Attribution was not measured: $($attribution.Reason)")
}

if ($attribution.Shallow) {
    $captions.Add("The checkout is a shallow clone holding $($attribution.HistoryDepth) commit(s). Lines older than the shallow boundary are attributed to the boundary commit, which is not an agent commit, so they are reported as human-written.")
}
if ($attribution.FailedFiles.Count -gt 0) {
    $captions.Add("``git blame`` failed for $($attribution.FailedFiles.Count) file(s); their lines are reported as unattributed. The paths are listed in the job log.")
}
if ($attribution.Capped) {
    $sample = @($attribution.SkippedFiles | Select-Object -First $MaxSkippedPathsInCaption)
    $remainder = $attribution.SkippedFiles.Count - $sample.Count
    $text = "The ${AttributionBudgetSeconds}s attribution budget was exhausted, so $($attribution.SkippedFiles.Count) candidate file(s) were not blamed and their lines are reported as unattributed: " + (($sample | ForEach-Object { "``$_``" }) -join ', ')
    if ($remainder -gt 0) {
        $text += ", and $remainder further path(s) named in full in the job log"
    }
    $captions.Add($text + '.')
}

foreach ($caption in $captions) {
    [void]$builder.AppendLine()
    [void]$builder.Append('> ')
    [void]$builder.AppendLine($caption)
}

[Console]::Out.Write($builder.ToString())

Write-Diagnostic ("total runtime {0:0.0}s" -f $script:StartedAt.Elapsed.TotalSeconds)

if ($problems.Count -gt 0) {
    foreach ($problem in $problems) { Write-Diagnostic "self-check failed: $problem" }
    exit 2
}

exit 0
