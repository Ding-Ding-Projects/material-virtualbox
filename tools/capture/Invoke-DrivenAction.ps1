#requires -Version 5.1
<#
.SYNOPSIS
    Sends one background input action (click / key chord / key press / text)
    to the window recorded by Start-DrivenSession.ps1's -StateFile, and
    optionally captures + validates the result -- all via PostMessage
    against a specific HWND, never SendInput, so the operator's real
    cursor/keyboard/focus/foreground window are never touched.

.DESCRIPTION
    Each invocation of this script is a fresh, stateless process: it re-
    reads -StateFile for the window handle discovered by
    Start-DrivenSession.ps1, and re-compiles VbxCaptureNative.cs (cheap,
    <1s, and Import-VbxCaptureNativeType short-circuits if already loaded
    in-process). The HWND value itself remains valid across process
    boundaries as long as the session-holder process and the target it
    launched are still alive keeping the off-screen desktop referenced --
    see Start-DrivenSession.ps1's header for why this is safe.

    -Action values:
      click     : -X/-Y are pixel coordinates in the SAME space as a
                  PrintWindow capture of this window (i.e. read them
                  straight off a previously saved PNG). Internally
                  translated to client coordinates via GetClientOffset.
      keychord  : -Modifiers is a comma-separated list of VK names
                  (Control, Shift, Menu) pressed down (in order) before
                  -MainKey, then released in reverse order. -MainKey may be
                  a single character (e.g. 'F') or a VK name (Escape,
                  Return, Tab, F1).
      keypress  : -MainKey alone, no modifiers.
      text      : -Text is typed one WM_CHAR at a time.
      capture   : take a PrintWindow capture of the CURRENT window (re-
                  resolved by pid+desktop at call time, in case a new
                  top-level window -- e.g. a dialog -- has appeared) and
                  validate it non-uniform, appending to -ManifestPath.
                  -SurfaceLabel and the provenance parameters are recorded
                  exactly like Invoke-CaptureHarness.ps1's manifest entries.
      resize    : -Width/-Height resize the window in place via SetWindowPos
                  (kept top-left corner), used to reach narrow-width
                  responsive layouts headlessly since there is no drag-edge
                  gesture to simulate that Qt would recognise any more
                  reliably than this normal window-management API.
      none      : just wait -SettleMs and re-resolve the window list, useful
                  after an action that itself needed no input (e.g. waiting
                  out an animation) before the next capture.

    A 'click'/'keychord'/'keypress'/'text' action does NOT itself capture;
    call this script again with -Action capture afterward once you know
    (from a prior capture) that the UI actually changed, so every capture
    is a deliberate, labeled step rather than one screenshot per keystroke.
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$StateFile,

    [Parameter(Mandatory = $true)]
    [ValidateSet('click', 'keychord', 'keypress', 'text', 'capture', 'resize', 'none')]
    [string]$Action,

    [int]$X,
    [int]$Y,
    [int]$Width,
    [int]$Height,

    # Accepted as a single comma-separated string (e.g. "Control,Shift")
    # rather than a native [string[]], because this script is invoked via
    # `powershell.exe -File ... -Modifiers Control,Shift` from a non-
    # PowerShell caller (a plain shell): PowerShell's automatic comma-splits-
    # into-an-array binding only happens when ITS OWN parser sees the comma
    # syntax, not when a single argv string arrives through -File. Split
    # manually below instead.
    [string]$Modifiers = '',
    [string]$MainKey,

    [string]$Text,

    [int]$SettleMs = 120,

    # capture-only parameters, mirroring Invoke-CaptureHarness.ps1
    [string]$OutputDir,
    [string]$SurfaceLabel,
    [string]$ManifestPath,
    [int]$UniformGridSize = 12,
    [string]$Commit,
    [string]$QtVersion,
    [string]$DisplayScale,
    [string]$LanguageMode,
    [string]$ProfileState,
    [string]$Notes,
    [int]$PostActionWaitMs = 0
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

. (Join-Path $PSScriptRoot 'Import-CaptureNativeType.ps1')
Import-VbxCaptureNativeType

if (-not (Test-Path -LiteralPath $StateFile)) {
    throw "StateFile not found: $StateFile (has Start-DrivenSession.ps1 reached 'ready' yet?)"
}
$state = Get-Content -LiteralPath $StateFile -Raw | ConvertFrom-Json
if ($state.status -ne 'ready' -and $state.status -ne 'process_exited') {
    throw "Session state is '$($state.status)', not 'ready'. Wait for Start-DrivenSession.ps1 to report ready."
}

# This script runs as a BRAND NEW process every invocation (see header). A
# window's HWND only resolves correctly for a thread that is itself
# attached to that window's desktop -- observed directly: without this, a
# freshly-deserialized HWND value made GetWindowRect/GetClassName fail with
# no useful GetLastError() in a fresh process, even though the identical
# value worked fine in the process that originally discovered it. Strip the
# "WinSta0\" window-station prefix Start-DrivenSession.ps1 records, since
# OpenDesktop resolves a desktop name within the CALLING process's own
# current window station.
$desktopShortName = ([string]$state.desktopName) -replace '^WinSta0\\', ''
[bool]$threadAttached = $false
[string]$attachWarning = $null
$attachedDesktop = [VbxCaptureHarness.Native]::AttachToDesktop($desktopShortName, [ref]$threadAttached, [ref]$attachWarning)
if ($attachWarning) {
    Write-Host "[driven-action] NOTE: $attachWarning"
}

$vkMap = @{
    'Control' = [VbxCaptureHarness.Native]::VK_CONTROL
    'Shift'   = [VbxCaptureHarness.Native]::VK_SHIFT
    'Menu'    = [VbxCaptureHarness.Native]::VK_MENU
    'Alt'     = [VbxCaptureHarness.Native]::VK_MENU
    'Escape'  = [VbxCaptureHarness.Native]::VK_ESCAPE
    'Return'  = [VbxCaptureHarness.Native]::VK_RETURN
    'Enter'   = [VbxCaptureHarness.Native]::VK_RETURN
    'Tab'     = [VbxCaptureHarness.Native]::VK_TAB
    'F1'      = [VbxCaptureHarness.Native]::VK_F1
}

function Resolve-Vk {
    param([string]$Name)
    if ($vkMap.ContainsKey($Name)) { return $vkMap[$Name] }
    if ($Name.Length -eq 1) { return [int]([char]$Name.ToUpperInvariant()[0]) }
    throw "Unrecognized key name: $Name"
}

$handle = [long]$state.windowHandle

switch ($Action) {
    'click' {
        Write-Host "[driven-action] click at image-space ($X,$Y) on handle $handle"
        [VbxCaptureHarness.Native]::ClickWindowPoint($handle, $X, $Y, $SettleMs) | Out-Null
    }
    'keychord' {
        $modifierList = @($Modifiers -split ',' | Where-Object { $_ -ne '' })
        $modVks = @($modifierList | ForEach-Object { Resolve-Vk $_ })
        $mainVk = Resolve-Vk $MainKey
        Write-Host "[driven-action] keychord [$($modifierList -join '+')]+$MainKey on handle $handle"
        [VbxCaptureHarness.Native]::SendKeyChord($handle, $modVks, $mainVk, $SettleMs) | Out-Null
    }
    'keypress' {
        $mainVk = Resolve-Vk $MainKey
        Write-Host "[driven-action] keypress $MainKey on handle $handle"
        [VbxCaptureHarness.Native]::SendKeyPress($handle, $mainVk, $SettleMs) | Out-Null
    }
    'text' {
        Write-Host "[driven-action] text '$Text' on handle $handle"
        [VbxCaptureHarness.Native]::SendText($handle, $Text, $SettleMs) | Out-Null
    }
    'resize' {
        Write-Host "[driven-action] resize to ${Width}x${Height} on handle $handle"
        $ok = [VbxCaptureHarness.Native]::ResizeWindow($handle, $Width, $Height)
        if (-not $ok) { Write-Host "[driven-action] WARNING: SetWindowPos reported failure" }
    }
    'none' {
        Write-Host "[driven-action] no input; waiting only"
    }
    'capture' {
        if (-not $OutputDir -or -not $SurfaceLabel) {
            throw "-OutputDir and -SurfaceLabel are required for -Action capture"
        }
        if (-not (Test-Path -LiteralPath $OutputDir)) { New-Item -ItemType Directory -Path $OutputDir -Force | Out-Null }
        $OutputDir = (Resolve-Path -LiteralPath $OutputDir).ProviderPath
        if (-not $ManifestPath) { $ManifestPath = Join-Path $OutputDir 'capture-manifest.json' }

        function ConvertTo-SafeFileNamePart {
            param([string]$Text)
            $safe = $Text -replace '[^A-Za-z0-9._-]+', '-'
            $safe = $safe.Trim('-')
            if ([string]::IsNullOrWhiteSpace($safe)) { $safe = 'window' }
            if ($safe.Length -gt 80) { $safe = $safe.Substring(0, 80) }
            return $safe
        }

        # Re-resolve the CURRENT window rect/class/title for this handle --
        # a driven action may have changed the title (navigation) or the
        # window may have been resized/moved, and this must reflect the
        # window as it stands right now, not the stale value cached at
        # session start.
        $classSb = New-Object System.Text.StringBuilder(256)
        [void][VbxCaptureHarness.Native]::GetClassName([IntPtr]$handle, $classSb, $classSb.Capacity)
        $len = [VbxCaptureHarness.Native]::GetWindowTextLength([IntPtr]$handle)
        $titleSb = New-Object System.Text.StringBuilder([Math]::Max($len, 0) + 1)
        if ($len -gt 0) { [void][VbxCaptureHarness.Native]::GetWindowText([IntPtr]$handle, $titleSb, $titleSb.Capacity) }
        $currentClass = $classSb.ToString()
        $currentTitle = $titleSb.ToString()

        $index = 1
        $classSafe = ConvertTo-SafeFileNamePart -Text $currentClass
        $titleSafe = ConvertTo-SafeFileNamePart -Text $currentTitle
        $fileName = "{0}--{1:D2}--{2}--{3}.png" -f (ConvertTo-SafeFileNamePart -Text $SurfaceLabel), $index, $classSafe, $titleSafe
        $outPath = Join-Path $OutputDir $fileName

        [int]$capW = 0; [int]$capH = 0; [string]$capErr = $null
        $printed = [VbxCaptureHarness.Native]::CaptureWindow($handle, $outPath, [ref]$capW, [ref]$capH, [ref]$capErr)

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
            Write-Host "[driven-action] CAPTURED -> $fileName ($($capW)x$($capH)px, $distinctColors/$totalSamples distinct sampled colors) class='$currentClass' title='$currentTitle'"
        } else {
            Write-Host "[driven-action] REJECTED ($fileName): status=$status printWindow=$printed distinctColors=$distinctColors error='$validationError'"
        }

        $relOutPath = $outPath
        try {
            if ($outPath.StartsWith((Get-Location).ProviderPath, [System.StringComparison]::OrdinalIgnoreCase)) {
                $relOutPath = [System.IO.Path]::GetRelativePath((Get-Location).ProviderPath, $outPath)
            }
        } catch { }

        $entry = [ordered]@{
            surface              = $SurfaceLabel
            status                = $status
            windowClass           = $currentClass
            windowTitle           = $currentTitle
            width                 = $capW
            height                = $capH
            outputPath            = $relOutPath
            appPath               = [string]$state.targetPath
            pid                   = [int]$state.pid
            desktopName           = [string]$state.desktopName
            timestampUtc          = (Get-Date).ToUniversalTime().ToString('o')
            distinctSampleColors  = $distinctColors
            totalSamples          = $totalSamples
            printWindowSucceeded  = $printed
            error                 = $validationError
            commit                = $(if ($Commit) { $Commit } else { $null })
            qtVersion             = $(if ($QtVersion) { $QtVersion } else { $null })
            displayScale          = $(if ($DisplayScale) { $DisplayScale } else { $null })
            languageMode          = $(if ($LanguageMode) { $LanguageMode } else { $null })
            profileState          = $(if ($ProfileState) { $ProfileState } else { $null })
            notes                 = $(if ($Notes) { $Notes } else { $null })
        }

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
                Write-Host "[driven-action] WARNING: existing manifest could not be parsed and will be preserved unmodified: $($_.Exception.Message)"
                $existing = @()
            }
        }
        $combined = @($existing) + @($entry)
        $combined | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath $ManifestPath -Encoding utf8
        Write-Host "[driven-action] Manifest written: $ManifestPath ($($combined.Count) total entries)."

        [PSCustomObject]@{
            Status = $status
            OutputPath = $outPath
            Width = $capW
            Height = $capH
            WindowClass = $currentClass
            WindowTitle = $currentTitle
            DistinctSampleColors = $distinctColors
        } | Write-Output
    }
}

if ($PostActionWaitMs -gt 0) {
    Start-Sleep -Milliseconds $PostActionWaitMs
}

if ($attachedDesktop -and $attachedDesktop -ne [IntPtr]::Zero) {
    try { [VbxCaptureHarness.Native]::CloseDesktop($attachedDesktop) | Out-Null } catch { }
}
