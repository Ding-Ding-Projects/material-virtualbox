#requires -Version 5.1
<#
.SYNOPSIS
    Ends a driven session started by Start-DrivenSession.ps1, honestly:
    terminates the recorded process tree and lets that script's own
    `finally` block (still running in its own process, polling for exactly
    this) close the off-screen desktop handle -- rather than force-killing
    the session-holder script itself, which would skip its `finally` and
    leak the desktop handle.
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$StateFile
)

$ErrorActionPreference = 'Stop'

if (-not (Test-Path -LiteralPath $StateFile)) {
    Write-Host "[stop-driven-session] No state file at $StateFile; nothing to stop."
    exit 0
}
$state = Get-Content -LiteralPath $StateFile -Raw | ConvertFrom-Json
if ($state.pid) {
    Write-Host "[stop-driven-session] Terminating PID $($state.pid) (and its tree) ..."
    try { & taskkill.exe /PID $state.pid /T /F 2>&1 | Out-Null } catch { }
} else {
    Write-Host "[stop-driven-session] State file has no pid recorded; nothing to terminate."
}
Write-Host "[stop-driven-session] Sent. Start-DrivenSession.ps1's own loop will detect the process exited within ~2s and close the off-screen desktop handle itself."
