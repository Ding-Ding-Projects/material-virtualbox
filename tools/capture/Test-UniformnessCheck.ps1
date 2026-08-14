#requires -Version 5.1
<#
.SYNOPSIS
    Self-test proving the capture harness's black-frame detector actually
    detects a black frame.

.DESCRIPTION
    A freshly allocated GDI bitmap is uninitialised memory and reads back as
    solid black; Win32 PrintWindow returns success whether or not the target
    window painted anything into it. So Invoke-CaptureHarness.ps1 never
    trusts a "capture succeeded" flag -- it reads every PNG back afterward
    and calls VbxCaptureHarness.Native.IsImageUniform to sample it on a
    grid, rejecting a capture whose sampled pixels are all one colour.

    This script does not re-implement that check or a copy of it: it loads
    the exact same VbxCaptureNative.cs that the harness itself compiles, so
    a change to the real detector is exercised by this test automatically
    instead of a second, driftable copy quietly going stale.

    It proves three things, each independently assertable and printed:
      1. A synthetic solid-black image IS reported uniform (i.e. the
         detector would reject a black frame instead of shipping it).
      2. A synthetic solid-white image IS ALSO reported uniform -- a
         uniformly *legible-looking* colour is exactly as suspect as black;
         the detector does not special-case black.
      3. A synthetic image with real, varied content (a gridded pattern of
         many different colours, the same shape a real UI screenshot has)
         is reported NON-uniform -- proving the check does not also reject
         everything, which would make it useless.

.EXAMPLE
    pwsh -File Test-UniformnessCheck.ps1
    # Exit code 0 on pass, 1 on any assertion failure.
#>
[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

# Loads the exact same compiled VbxCaptureNative.cs the real harness uses,
# through the same portability cascade -- see Import-CaptureNativeType.ps1.
. (Join-Path $PSScriptRoot 'Import-CaptureNativeType.ps1')
Import-VbxCaptureNativeType

$workDir = Join-Path ([System.IO.Path]::GetTempPath()) ("vbx-capture-selftest-" + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $workDir -Force | Out-Null

$failures = @()

function Assert-Case {
    param(
        [string]$Name,
        [string]$ImagePath,
        [bool]$ExpectUniform
    )
    [int]$distinct = 0
    [int]$total = 0
    $actualUniform = [VbxCaptureHarness.Native]::IsImageUniform($ImagePath, 12, [ref]$distinct, [ref]$total)
    $pass = ($actualUniform -eq $ExpectUniform)
    $verdict = if ($pass) { 'PASS' } else { 'FAIL' }
    Write-Host ("[{0}] {1}: expected uniform={2}, got uniform={3} ({4}/{5} distinct sampled colors)" -f `
        $verdict, $Name, $ExpectUniform, $actualUniform, $distinct, $total)
    if (-not $pass) {
        $script:failures += $Name
    }
}

try {
    # Case 1: solid black -- the exact failure mode PrintWindow produces
    # when it reports success but paints nothing into an uninitialised
    # bitmap. This MUST be flagged uniform (invalid).
    $blackPath = Join-Path $workDir 'solid-black.png'
    $blackBmp = New-Object System.Drawing.Bitmap(220, 160)
    $gfx = [System.Drawing.Graphics]::FromImage($blackBmp)
    $gfx.Clear([System.Drawing.Color]::Black)
    $gfx.Dispose()
    $blackBmp.Save($blackPath, [System.Drawing.Imaging.ImageFormat]::Png)
    $blackBmp.Dispose()
    Assert-Case -Name 'solid black frame (the PrintWindow-failed-silently case)' -ImagePath $blackPath -ExpectUniform $true

    # Case 2: solid white -- proves the detector isn't merely "== black",
    # it is "== uniform", which is the actually correct property to check.
    $whitePath = Join-Path $workDir 'solid-white.png'
    $whiteBmp = New-Object System.Drawing.Bitmap(220, 160)
    $gfx2 = [System.Drawing.Graphics]::FromImage($whiteBmp)
    $gfx2.Clear([System.Drawing.Color]::White)
    $gfx2.Dispose()
    $whiteBmp.Save($whitePath, [System.Drawing.Imaging.ImageFormat]::Png)
    $whiteBmp.Dispose()
    Assert-Case -Name 'solid white frame (uniform-but-not-black control case)' -ImagePath $whitePath -ExpectUniform $true

    # Case 3: real, varied content -- a checkerboard of many distinct
    # colours, standing in for a legible UI screenshot. This MUST NOT be
    # flagged uniform, or the check would reject every real capture too.
    $variedPath = Join-Path $workDir 'varied-content.png'
    $variedBmp = New-Object System.Drawing.Bitmap(220, 160)
    $gfx3 = [System.Drawing.Graphics]::FromImage($variedBmp)
    $rand = New-Object System.Random(42)
    $cellSize = 14
    for ($x = 0; $x -lt 220; $x += $cellSize) {
        for ($y = 0; $y -lt 160; $y += $cellSize) {
            $color = [System.Drawing.Color]::FromArgb(
                $rand.Next(0, 256), $rand.Next(0, 256), $rand.Next(0, 256))
            $brush = New-Object System.Drawing.SolidBrush($color)
            $gfx3.FillRectangle($brush, $x, $y, $cellSize, $cellSize)
            $brush.Dispose()
        }
    }
    $gfx3.Dispose()
    $variedBmp.Save($variedPath, [System.Drawing.Imaging.ImageFormat]::Png)
    $variedBmp.Dispose()
    Assert-Case -Name 'checkerboard of distinct colors (a legible-capture stand-in)' -ImagePath $variedPath -ExpectUniform $false
} finally {
    Remove-Item -LiteralPath $workDir -Recurse -Force -ErrorAction SilentlyContinue
}

if ($failures.Count -gt 0) {
    Write-Host "SELF-TEST FAILED: $($failures.Count) case(s) did not match their expected verdict: $($failures -join '; ')"
    exit 1
}

Write-Host "SELF-TEST PASSED: the uniform-image detector correctly flags solid-color frames (black AND white) as invalid, and correctly accepts varied content as valid."
exit 0
