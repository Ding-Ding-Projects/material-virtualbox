#requires -Version 5.1
<#
.SYNOPSIS
    Runtime screenshot capture harness for MD3 VirtualBox surfaces.

.DESCRIPTION
    Launches a target Windows GUI executable on a NAMED, OFF-SCREEN Win32
    desktop (created with CreateDesktopW) so the operator's visible desktop,
    cursor, keyboard focus, and foreground window are never touched. It then
    resolves the application's real top-level windows DYNAMICALLY at runtime
    by enumerating every window that exists on that off-screen desktop (never
    a hard-coded handle -- handles are different every launch), filters out
    the junk windows that a Qt/Win32 process routinely spawns alongside its
    real UI (input-method windows, Cicero/TSF windows, UAC input-indicator
    windows, tooltip windows, and the screen-sized Qt "ScreenChangeObserver"
    helper window), and captures each surviving window with Win32
    PrintWindow(..., PW_RENDERFULLCONTENT) -- which works even though the
    window is unfocused and living on a desktop nobody is looking at.

    Every capture is validated before it is trusted: a freshly allocated GDI
    bitmap is uninitialised memory that reads as solid black, and
    PrintWindow returns success whether or not the window actually painted
    anything into it. So each PNG is read back afterward and sampled on a
    grid; a capture whose sampled pixels are all one colour is reported as
    INVALID, never silently accepted as a legible screenshot.

    Results are written as a JSON manifest (array, appended across runs) next
    to the images, recording per capture: the surface label, the resolved
    window class and title, pixel dimensions, the output path, the target
    app path, and a UTC timestamp -- plus whatever build/environment metadata
    the caller supplies (commit, Qt version, display scale, language mode,
    profile state) so the capture can be traced back to what produced it.

    The desktop and the launched process are always cleaned up, including on
    failure: the process tree is terminated and the off-screen desktop
    handle is closed in a `finally` block.

.PARAMETER TargetPath
    Path to the GUI executable to launch and capture.

.PARAMETER OutputDir
    Directory to write PNG captures and the JSON manifest into. Created if
    it does not exist.

.PARAMETER SurfaceLabel
    Human label used as a filename/manifest prefix for this invocation
    (e.g. "manager-shell-cold-launch"). Defaults to the target's base name.
    This labels the *invocation*; each captured window additionally records
    its own real title and class, since one invocation can legitimately
    surface more than one top-level window (e.g. a main window plus a
    modal error dialog).

.PARAMETER Arguments
    Optional array of command-line arguments to pass to the target.

.PARAMETER DesktopName
    Name of the off-screen desktop to create. Defaults to
    "VbxCaptureHarness-<pid>-<random>" so concurrent runs never collide.

.PARAMETER WindowTimeoutSeconds
    How long to poll for the first qualifying top-level window before giving
    up and reporting honestly that nothing appeared. Default 30.

.PARAMETER SettleSeconds
    After the first qualifying window is found, wait this long before the
    final capture pass, so a second window (e.g. an async error dialog) has
    a chance to appear and finish rendering too. Default 3.0.

.PARAMETER PollIntervalMs
    Delay between window-enumeration polls while waiting for the first
    window. Default 250.

.PARAMETER MinWidth / MinHeight
    Minimum window size (in pixels) to be considered a real surface rather
    than a zero-sized helper window. Default 2 for both.

.PARAMETER Commit / QtVersion / DisplayScale / LanguageMode / ProfileState / Notes
    Optional provenance metadata recorded verbatim on every manifest entry
    from this run. None are guessed: an omitted value is recorded as
    $null / "not supplied" rather than invented, per the project's evidence
    rules.

.PARAMETER ManifestPath
    Path to the JSON manifest file. Defaults to
    "<OutputDir>/capture-manifest.json". If the file already exists its
    entries are preserved and the new captures are appended.

.OUTPUTS
    Writes a summary object to the pipeline and sets $LASTEXITCODE:
      0 = at least one window was captured and validated non-uniform
      2 = no qualifying window ever appeared (timeout)
      3 = window(s) were captured but every capture was rejected as uniform
      4 = the target process failed to launch
      5 = unexpected harness error

.EXAMPLE
    pwsh -File Invoke-CaptureHarness.ps1 `
        -TargetPath 'C:\Users\me\AppData\Local\VirtualBox\app-7.2.97\VirtualBox.exe' `
        -OutputDir 'doc\md3\captures' `
        -SurfaceLabel 'manager-com-failure' `
        -Commit 'cb9f573030e' -QtVersion '6.8.3' -DisplayScale '100%' -LanguageMode 'English'
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$TargetPath,

    [Parameter(Mandatory = $true)]
    [string]$OutputDir,

    [string]$SurfaceLabel,

    [string[]]$Arguments = @(),

    [string]$DesktopName = ("VbxCaptureHarness-{0}-{1}" -f $PID, (Get-Random -Maximum 999999)),

    [int]$WindowTimeoutSeconds = 30,

    [double]$SettleSeconds = 3.0,

    [int]$PollIntervalMs = 250,

    [int]$MinWidth = 2,

    [int]$MinHeight = 2,

    # Some build/install pipelines briefly delete-and-replace the target
    # executable while packaging or reinstalling it (observed directly
    # against a Squirrel-managed install directory that another lane was
    # actively cycling during development of this harness: the file existed,
    # then didn't, then existed again as a different size, within seconds).
    # If TargetPath does not exist yet, wait up to this long -- polling and
    # requiring the file size to be unchanged across two checks before
    # trusting it -- rather than failing immediately on a transient gap.
    # Default 0 preserves the simple "fail fast if missing" behavior.
    [int]$TargetPathWaitSeconds = 0,

    [string]$Commit,
    [string]$QtVersion,
    [string]$DisplayScale,
    [string]$LanguageMode,
    [string]$ProfileState,
    [string]$Notes,

    [string]$ManifestPath,

    [int]$UniformGridSize = 12,

    [string[]]$ExtraJunkClassPattern = @()
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

# ---------------------------------------------------------------------------
# Native Win32 interop: off-screen desktop, process launch onto it, window
# enumeration scoped to that desktop, PrintWindow capture, and a pixel-grid
# uniformity check over the resulting PNG.
#
# The C# source lives in the sibling file VbxCaptureNative.cs rather than an
# inline here-string, specifically so Test-UniformnessCheck.ps1 (the
# companion self-test that proves the black-frame detector actually fires)
# can load the exact same compiled IsImageUniform implementation instead of
# a copy that could silently drift from what the harness really runs. It is
# compiled through Import-CaptureNativeType.ps1's strategy cascade -- see
# that file for why a single Add-Type call is not portable across PowerShell
# hosts here.
# ---------------------------------------------------------------------------
. (Join-Path $PSScriptRoot 'Import-CaptureNativeType.ps1')
Import-VbxCaptureNativeType


# ---------------------------------------------------------------------------
# Window classification: what counts as a real captureable surface versus a
# Qt/Win32 process's own junk windows.
#
# A Qt GUI process on Windows routinely owns several top-level windows that
# have nothing to do with the application's actual UI:
#   - "Default IME" / class "IME"                       (input-method window)
#   - "MSCTFIME UI" / class "MSCTFIME UI"                (TSF input window)
#   - class "CicLoaderWndClass"                          (Cicero loader)
#   - class "CiceroUIWndFrame"                           (Cicero UI frame)
#   - "Touch Tooltip Window"                             (touch-tooltip helper)
#   - UAC input-indicator windows                        (secure-desktop hint)
#   - class "Qt<ver>ScreenChangeObserverWindow"          (Qt's own screen-size
#                                                          observer window,
#                                                          sized to the whole
#                                                          screen -- easy to
#                                                          mistake for the
#                                                          real app window if
#                                                          you only filter by
#                                                          size)
# Naively enumerating and grabbing "the first window" reliably lands on one
# of these -- most often the 0x0 IME window -- and produces a capture
# failure that looks like the *application's* fault rather than the harness
# filtering badly. So real surfaces are recognised by elimination: visible,
# non-empty title, real size, AND not matching any of the known-junk class
# or title patterns below (never by allow-listing a Qt version-specific
# class name, since the Qt version -- "Qt683..." today -- changes with every
# Qt upgrade).
# ---------------------------------------------------------------------------
$script:JunkPatterns = @(
    '^IME$',
    'MSCTFIME UI',
    'CicLoaderWndClass',
    'CiceroUIWndFrame',
    'Touch Tooltip Window',
    'ScreenChangeObserverWindow$',
    'UACInputIndicator',
    'Windows\.UI\.Core\.CoreWindow',
    '^tooltips_class',
    '^Shell_TrayWnd$'
) + $ExtraJunkClassPattern

function Test-JunkWindow {
    param([string]$ClassName, [string]$Title)
    foreach ($pattern in $script:JunkPatterns) {
        if ($ClassName -match $pattern -or $Title -match $pattern) { return $true }
    }
    return $false
}

function Get-QualifyingWindows {
    param(
        [IntPtr]$DesktopHandle,
        [int]$TargetPid,
        [int]$MinWidth,
        [int]$MinHeight
    )
    $all = [VbxCaptureHarness.Native]::ListDesktopWindows($DesktopHandle)
    $qualifying = @()
    foreach ($w in $all) {
        if ($w.Pid -ne $TargetPid) { continue }
        if (-not $w.Visible) { continue }
        if ([string]::IsNullOrWhiteSpace($w.Title)) { continue }
        if ($w.Width -lt $MinWidth -or $w.Height -lt $MinHeight) { continue }
        if (Test-JunkWindow -ClassName $w.ClassName -Title $w.Title) { continue }
        $qualifying += $w
    }
    return ,$qualifying
}

function ConvertTo-SafeFileNamePart {
    param([string]$Text)
    $safe = $Text -replace '[^A-Za-z0-9._-]+', '-'
    $safe = $safe.Trim('-')
    if ([string]::IsNullOrWhiteSpace($safe)) { $safe = 'window' }
    if ($safe.Length -gt 80) { $safe = $safe.Substring(0, 80) }
    return $safe
}

# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------
$exitCode = 5
$desktopHandle = [IntPtr]::Zero
$desktopCreated = $false
$launchedPid = $null
$manifestEntries = @()

try {
    # Defaulted before anything that can throw: if TargetPath/OutputDir
    # validation fails below, the `finally` block still needs a real,
    # non-empty ManifestPath to write the failure entry to. Leaving this
    # default until after those checks meant an early throw left
    # $ManifestPath as an empty string, and `finally`'s own
    # `Test-Path -LiteralPath $ManifestPath` then failed with a confusing
    # parameter-binding error that masked the real one -- caught while
    # proving this harness against a target whose install directory another
    # lane was actively rewriting underneath it.
    if (-not $ManifestPath) {
        $ManifestPath = Join-Path $OutputDir 'capture-manifest.json'
    }

    if ($TargetPathWaitSeconds -gt 0 -and -not (Test-Path -LiteralPath $TargetPath)) {
        Write-Host "[capture-harness] TargetPath not present yet; waiting up to ${TargetPathWaitSeconds}s (another process may be writing it) ..."
        $waitDeadline = (Get-Date).AddSeconds($TargetPathWaitSeconds)
        $lastSize = -1
        while ((Get-Date) -lt $waitDeadline) {
            if (Test-Path -LiteralPath $TargetPath) {
                $size = (Get-Item -LiteralPath $TargetPath -ErrorAction SilentlyContinue).Length
                if ($null -ne $size -and $size -eq $lastSize -and $size -gt 0) {
                    Write-Host "[capture-harness] TargetPath present and size-stable ($size bytes). Proceeding."
                    break
                }
                $lastSize = $size
            } else {
                $lastSize = -1
            }
            Start-Sleep -Milliseconds 500
        }
    }
    if (-not (Test-Path -LiteralPath $TargetPath)) {
        throw "TargetPath does not exist: $TargetPath"
    }
    $TargetPath = (Resolve-Path -LiteralPath $TargetPath).ProviderPath

    if (-not (Test-Path -LiteralPath $OutputDir)) {
        New-Item -ItemType Directory -Path $OutputDir -Force | Out-Null
    }
    $OutputDir = (Resolve-Path -LiteralPath $OutputDir).ProviderPath

    if (-not $SurfaceLabel) {
        $SurfaceLabel = [System.IO.Path]::GetFileNameWithoutExtension($TargetPath)
    }
    if (-not $ManifestPath) {
        $ManifestPath = Join-Path $OutputDir 'capture-manifest.json'
    }

    Write-Host "[capture-harness] Creating off-screen desktop '$DesktopName' ..."
    $desktopHandle = [VbxCaptureHarness.Native]::CreateHeadlessDesktop($DesktopName)
    $desktopCreated = $true
    $desktopFullName = "WinSta0\$DesktopName"
    Write-Host "[capture-harness] Desktop created: $desktopFullName (handle 0x$($desktopHandle.ToString('X')))"

    $cmdLine = '"' + $TargetPath + '"'
    foreach ($a in $Arguments) { $cmdLine += ' ' + $a }

    Write-Host "[capture-harness] Launching on hidden desktop: $cmdLine"
    try {
        $launchedPid = [VbxCaptureHarness.Native]::LaunchOnDesktop($desktopFullName, $cmdLine)
    } catch {
        Write-Host "[capture-harness] LAUNCH FAILED: $($_.Exception.Message)"
        $exitCode = 4
        throw
    }
    Write-Host "[capture-harness] Launched PID $launchedPid. Never touches the visible desktop -- it exists only on '$desktopFullName'."

    # Window discovery arrives late: the process exists before its main
    # window does. Poll with a bounded timeout instead of assuming the
    # window is there the instant CreateProcess returns.
    $deadline = (Get-Date).AddSeconds($WindowTimeoutSeconds)
    $firstMatch = @()
    Write-Host "[capture-harness] Polling for a qualifying window (timeout ${WindowTimeoutSeconds}s) ..."
    while ((Get-Date) -lt $deadline) {
        $firstMatch = Get-QualifyingWindows -DesktopHandle $desktopHandle -TargetPid $launchedPid -MinWidth $MinWidth -MinHeight $MinHeight
        if ($firstMatch.Count -gt 0) { break }
        Start-Sleep -Milliseconds $PollIntervalMs
    }

    if ($firstMatch.Count -eq 0) {
        Write-Host "[capture-harness] TIMEOUT: no qualifying window appeared for PID $launchedPid within ${WindowTimeoutSeconds}s."
        $manifestEntries += [ordered]@{
            surface        = $SurfaceLabel
            status         = 'not_found'
            error          = "no qualifying top-level window appeared within ${WindowTimeoutSeconds}s"
            appPath        = $TargetPath
            pid            = $launchedPid
            desktopName    = $desktopFullName
            timestampUtc   = (Get-Date).ToUniversalTime().ToString('o')
            commit         = $(if ($Commit) { $Commit } else { $null })
            qtVersion      = $(if ($QtVersion) { $QtVersion } else { $null })
            displayScale   = $(if ($DisplayScale) { $DisplayScale } else { $null })
            languageMode   = $(if ($LanguageMode) { $LanguageMode } else { $null })
            profileState   = $(if ($ProfileState) { $ProfileState } else { $null })
            notes          = $(if ($Notes) { $Notes } else { $null })
        }
        $exitCode = 2
    } else {
        Write-Host "[capture-harness] First qualifying window found: class='$($firstMatch[0].ClassName)' title='$($firstMatch[0].Title)' size=$($firstMatch[0].Width)x$($firstMatch[0].Height)."
        Write-Host "[capture-harness] Settling ${SettleSeconds}s for any further windows (e.g. an async dialog) to appear and finish rendering ..."
        Start-Sleep -Seconds $SettleSeconds

        $finalMatches = Get-QualifyingWindows -DesktopHandle $desktopHandle -TargetPid $launchedPid -MinWidth $MinWidth -MinHeight $MinHeight
        Write-Host "[capture-harness] Final pass: $($finalMatches.Count) qualifying window(s) after settle."

        $capturedCount = 0
        $index = 0
        foreach ($w in $finalMatches) {
            $index++
            $classSafe = ConvertTo-SafeFileNamePart -Text $w.ClassName
            $titleSafe = ConvertTo-SafeFileNamePart -Text $w.Title
            $fileName = "{0}--{1:D2}--{2}--{3}.png" -f (ConvertTo-SafeFileNamePart -Text $SurfaceLabel), $index, $classSafe, $titleSafe
            $outPath = Join-Path $OutputDir $fileName

            [int]$capW = 0; [int]$capH = 0; [string]$capErr = $null
            $printed = [VbxCaptureHarness.Native]::CaptureWindow($w.Handle, $outPath, [ref]$capW, [ref]$capH, [ref]$capErr)

            $isUniform = $true
            $distinctColors = 0
            $totalSamples = 0
            $validationError = $null
            if ($printed -and (Test-Path -LiteralPath $outPath)) {
                try {
                    $isUniform = [VbxCaptureHarness.Native]::IsImageUniform($outPath, $UniformGridSize, [ref]$distinctColors, [ref]$totalSamples)
                } catch {
                    $validationError = "failed to read back PNG for validation: $($_.Exception.Message)"
                    $isUniform = $true
                }
            } else {
                $validationError = if ($capErr) { $capErr } else { 'PrintWindow/save did not produce a file' }
            }

            $status = if ($printed -and (Test-Path -LiteralPath $outPath) -and -not $isUniform) { 'captured' } else { 'invalid' }

            if ($status -eq 'captured') {
                $capturedCount++
                Write-Host "[capture-harness] CAPTURED #$index -> $fileName ($($capW)x$($capH)px, $distinctColors/$totalSamples distinct sampled colors) class='$($w.ClassName)' title='$($w.Title)'"
            } else {
                Write-Host "[capture-harness] REJECTED #$index ($fileName): status=$status printWindow=$printed distinctColors=$distinctColors error='$validationError'"
                # Keep the rejected file on disk for forensic inspection, but
                # never let it be mistaken for accepted evidence: the
                # manifest entry below records status=invalid explicitly.
            }

            $relOutPath = $outPath
            try {
                if ($outPath.StartsWith((Get-Location).ProviderPath, [System.StringComparison]::OrdinalIgnoreCase)) {
                    $relOutPath = [System.IO.Path]::GetRelativePath((Get-Location).ProviderPath, $outPath)
                }
            } catch { }

            $manifestEntries += [ordered]@{
                surface             = $SurfaceLabel
                status              = $status
                windowClass         = $w.ClassName
                windowTitle         = $w.Title
                width               = $capW
                height              = $capH
                outputPath          = $relOutPath
                appPath             = $TargetPath
                pid                 = $launchedPid
                desktopName         = $desktopFullName
                timestampUtc        = (Get-Date).ToUniversalTime().ToString('o')
                distinctSampleColors= $distinctColors
                totalSamples        = $totalSamples
                printWindowSucceeded= $printed
                error               = $validationError
                commit              = $(if ($Commit) { $Commit } else { $null })
                qtVersion           = $(if ($QtVersion) { $QtVersion } else { $null })
                displayScale        = $(if ($DisplayScale) { $DisplayScale } else { $null })
                languageMode        = $(if ($LanguageMode) { $LanguageMode } else { $null })
                profileState        = $(if ($ProfileState) { $ProfileState } else { $null })
                notes               = $(if ($Notes) { $Notes } else { $null })
            }
        }

        if ($capturedCount -gt 0) {
            $exitCode = 0
        } else {
            $exitCode = 3
        }
    }
} catch {
    Write-Host "[capture-harness] ERROR: $($_.Exception.Message)"
    if ($exitCode -eq 5 -or $exitCode -eq 0) {
        # Preserve a more specific code (2/3/4) if one was already set above;
        # otherwise this is an unexpected harness-level failure.
        $exitCode = 5
    }
    $manifestEntries += [ordered]@{
        surface      = $SurfaceLabel
        status       = 'harness_error'
        error        = $_.Exception.Message
        appPath      = $TargetPath
        pid          = $launchedPid
        desktopName  = $(if ($desktopCreated) { "WinSta0\$DesktopName" } else { $null })
        timestampUtc = (Get-Date).ToUniversalTime().ToString('o')
    }
} finally {
    if ($launchedPid) {
        Write-Host "[capture-harness] Cleaning up: terminating PID $launchedPid (and its tree) ..."
        try {
            & taskkill.exe /PID $launchedPid /T /F 2>&1 | Out-Null
        } catch { }
    }
    if ($desktopCreated -and $desktopHandle -ne [IntPtr]::Zero) {
        Write-Host "[capture-harness] Closing off-screen desktop handle ..."
        try {
            [VbxCaptureHarness.Native]::CloseDesktop($desktopHandle) | Out-Null
        } catch { }
    }

    # Merge into the persistent manifest rather than overwriting it, so
    # repeated invocations (different surfaces, different runs) accumulate
    # into one auditable record instead of clobbering each other.
    if (-not $ManifestPath) {
        # Belt-and-braces: should be unreachable given the early default
        # above, but a bare fallback here is cheap insurance against ever
        # again handing Test-Path an empty -LiteralPath in this block.
        $ManifestPath = Join-Path ([System.IO.Path]::GetTempPath()) 'vbx-capture-manifest-fallback.json'
        Write-Host "[capture-harness] WARNING: ManifestPath was unset when cleanup ran; writing to fallback location: $ManifestPath"
    }
    if ($manifestEntries.Count -gt 0) {
        $existing = @()
        if (Test-Path -LiteralPath $ManifestPath) {
            try {
                $raw = Get-Content -LiteralPath $ManifestPath -Raw
                if ($raw) {
                    $parsed = $raw | ConvertFrom-Json
                    if ($parsed -is [System.Array]) { $existing = @($parsed) }
                    elseif ($null -ne $parsed) { $existing = @($parsed) }
                }
            } catch {
                Write-Host "[capture-harness] WARNING: existing manifest at '$ManifestPath' could not be parsed and will be preserved unmodified; new entries written alongside it will NOT include prior history. Error: $($_.Exception.Message)"
                $existing = @()
            }
        }
        $combined = @($existing) + @($manifestEntries)
        $combined | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath $ManifestPath -Encoding utf8
        Write-Host "[capture-harness] Manifest written: $ManifestPath ($($combined.Count) total entries, $($manifestEntries.Count) added this run)."
    }
}

Write-Host "[capture-harness] Exit code: $exitCode"
[PSCustomObject]@{
    ExitCode   = $exitCode
    Entries    = $manifestEntries
    ManifestPath = $ManifestPath
} | Write-Output

exit $exitCode
