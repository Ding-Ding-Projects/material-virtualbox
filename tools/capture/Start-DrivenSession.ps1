#requires -Version 5.1
<#
.SYNOPSIS
    Launches a target GUI executable on an off-screen desktop and keeps it
    alive so a separate driver script (Invoke-DrivenAction.ps1) can send it
    background clicks/keystrokes and capture it across several surfaces in
    one running session, then Stop-DrivenSession.ps1 tears it down.

.DESCRIPTION
    This is the multi-step counterpart to Invoke-CaptureHarness.ps1, which
    launches, captures once, and immediately kills the target -- correct for
    a single static surface, but unusable for driving a live application
    through several screens, since each screen needs the PREVIOUS one's
    state (window, focus, navigation) still standing.

    Desktop creation, process launch, and window discovery reuse the exact
    same native calls (VbxCaptureNative.cs via Import-CaptureNativeType.ps1)
    as Invoke-CaptureHarness.ps1 -- never a second, driftable copy. Once the
    first qualifying window is found, this script writes its resolved
    handle/pid/desktop name to -StateFile and then blocks
    (Start-Sleep -Seconds -SessionSeconds) so the process and its off-screen
    desktop stay alive for the caller's remaining shell invocations. A
    window handle discovered this way remains valid for OTHER processes
    (e.g. the separate powershell.exe invocations Invoke-DrivenAction.ps1
    runs) to call PostMessage/SendMessage/PrintWindow/GetWindowRect against,
    as long as this session-holder process (and the target process it
    launched) are still alive keeping the desktop object referenced --
    HWNDs are validated by the window station, not by whether the CALLING
    process personally holds an open desktop handle.

    Stop-DrivenSession.ps1 (or a direct taskkill of the PID recorded in
    -StateFile) ends this script's wait early and lets its own `finally`
    block terminate the launched process tree and close the desktop handle
    -- the exact same guaranteed cleanup contract as
    Invoke-CaptureHarness.ps1.

.PARAMETER StateFile
    Where to write the JSON session-state object (desktopName, pid,
    windowHandle, windowClass, windowTitle, width, height, targetPath).
    Must NOT be inside the repository -- this is ephemeral driving state,
    not evidence.
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$TargetPath,

    [Parameter(Mandatory = $true)]
    [string]$StateFile,

    [string[]]$Arguments = @(),

    [string]$DesktopName = ("VbxDrivenSession-{0}-{1}" -f $PID, (Get-Random -Maximum 999999)),

    [int]$WindowTimeoutSeconds = 40,

    [double]$SettleSeconds = 4.0,

    [int]$PollIntervalMs = 250,

    [int]$MinWidth = 2,
    [int]$MinHeight = 2,

    [int]$SessionSeconds = 1800,

    [string[]]$ExtraJunkClassPattern = @()
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

. (Join-Path $PSScriptRoot 'Import-CaptureNativeType.ps1')
Import-VbxCaptureNativeType

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
    param([IntPtr]$DesktopHandle, [int]$TargetPid, [int]$MinWidth, [int]$MinHeight)
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

$desktopHandle = [IntPtr]::Zero
$desktopCreated = $false
$launchedPid = $null

function Write-State {
    param([string]$Status, [object]$Extra = $null)
    $state = [ordered]@{
        status       = $Status
        desktopName  = $(if ($desktopCreated) { "WinSta0\$DesktopName" } else { $null })
        pid          = $launchedPid
        targetPath   = $TargetPath
        timestampUtc = (Get-Date).ToUniversalTime().ToString('o')
    }
    if ($Extra) { foreach ($k in $Extra.Keys) { $state[$k] = $Extra[$k] } }
    $state | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath $StateFile -Encoding utf8
}

try {
    if (-not (Test-Path -LiteralPath $TargetPath)) {
        throw "TargetPath does not exist: $TargetPath"
    }
    $TargetPath = (Resolve-Path -LiteralPath $TargetPath).ProviderPath

    Write-Host "[driven-session] Creating off-screen desktop '$DesktopName' ..."
    $desktopHandle = [VbxCaptureHarness.Native]::CreateHeadlessDesktop($DesktopName)
    $desktopCreated = $true
    $desktopFullName = "WinSta0\$DesktopName"
    Write-Host "[driven-session] Desktop created: $desktopFullName"

    $cmdLine = '"' + $TargetPath + '"'
    foreach ($a in $Arguments) { $cmdLine += ' ' + $a }
    Write-Host "[driven-session] Launching on hidden desktop: $cmdLine"
    $launchedPid = [VbxCaptureHarness.Native]::LaunchOnDesktop($desktopFullName, $cmdLine)
    Write-Host "[driven-session] Launched PID $launchedPid on $desktopFullName (visible desktop untouched)."
    Write-State -Status 'launching'

    $deadline = (Get-Date).AddSeconds($WindowTimeoutSeconds)
    $match = @()
    while ((Get-Date) -lt $deadline) {
        $match = Get-QualifyingWindows -DesktopHandle $desktopHandle -TargetPid $launchedPid -MinWidth $MinWidth -MinHeight $MinHeight
        if ($match.Count -gt 0) { break }
        Start-Sleep -Milliseconds $PollIntervalMs
    }

    if ($match.Count -eq 0) {
        Write-Host "[driven-session] TIMEOUT: no qualifying window within ${WindowTimeoutSeconds}s."
        Write-State -Status 'not_found'
        exit 2
    }

    Write-Host "[driven-session] Settling ${SettleSeconds}s ..."
    Start-Sleep -Seconds $SettleSeconds
    $match = Get-QualifyingWindows -DesktopHandle $desktopHandle -TargetPid $launchedPid -MinWidth $MinWidth -MinHeight $MinHeight
    $w = $match[0]

    Write-Host "[driven-session] READY: handle=$($w.Handle) class='$($w.ClassName)' title='$($w.Title)' size=$($w.Width)x$($w.Height)"
    Write-State -Status 'ready' -Extra ([ordered]@{
        windowHandle = $w.Handle
        windowClass  = $w.ClassName
        windowTitle  = $w.Title
        width        = $w.Width
        height       = $w.Height
    })

    Write-Host "[driven-session] Holding session open for up to ${SessionSeconds}s. Stop-DrivenSession.ps1 (or taskkill of PID $launchedPid) ends it early."
    $end = (Get-Date).AddSeconds($SessionSeconds)
    while ((Get-Date) -lt $end) {
        Start-Sleep -Seconds 2
        # If the target process has exited on its own (crash, user-triggered
        # close via a driven action), stop waiting rather than holding the
        # desktop open pointlessly for the rest of SessionSeconds.
        $stillAlive = Get-Process -Id $launchedPid -ErrorAction SilentlyContinue
        if (-not $stillAlive) {
            Write-Host "[driven-session] Target process $launchedPid is no longer running; ending session."
            Write-State -Status 'process_exited'
            break
        }
    }
} catch {
    Write-Host "[driven-session] ERROR: $($_.Exception.Message)"
    Write-State -Status 'error' -Extra ([ordered]@{ error = $_.Exception.Message })
} finally {
    if ($launchedPid) {
        Write-Host "[driven-session] Cleaning up: terminating PID $launchedPid (and its tree) ..."
        try { & taskkill.exe /PID $launchedPid /T /F 2>&1 | Out-Null } catch { }
    }
    if ($desktopCreated -and $desktopHandle -ne [IntPtr]::Zero) {
        Write-Host "[driven-session] Closing off-screen desktop handle ..."
        try { [VbxCaptureHarness.Native]::CloseDesktop($desktopHandle) | Out-Null } catch { }
    }
    Write-Host "[driven-session] Session ended and cleaned up."
}
