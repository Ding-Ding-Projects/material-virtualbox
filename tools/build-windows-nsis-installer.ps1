# $Id$
# Build the unsigned NSIS installer for the VirtualBox Windows HOST product
# (src\VBox\Installer\win\NSIS\VBoxHostInstaller.nsi) from an already staged
# release payload. This is the one and only Windows installer this project
# ships (see doc\installer\WindowsHostInstallerNSIS.md); an unsigned
# Squirrel.Windows package was retired outright in favour of it.
#
# This is a companion to tools\build-windows.ps1, not a replacement for it:
# that script still builds the Windows host binaries (VirtualBox.exe,
# VBoxSVC.exe, VBoxSDS.exe, ...) into out\win.amd64\release\bin. This script
# compiles the NSIS installer from that same payload; `tools\build-windows.ps1
# -Mode Installer` (and the release workflow) call it directly, so it does
# not usually need to be run standalone. Run `tools\build-windows.ps1 -Mode
# Build` (or an equivalent full build) first if invoking it directly.
#
# Copyright (C) 2026 Oracle and/or its affiliates.
# SPDX-License-Identifier: GPL-3.0-only

[CmdletBinding()]
param(
    # Staged release payload directory. Defaults to the same directory
    # tools\build-windows.ps1 produces and packages: out\win.amd64\release\bin.
    [string] $PayloadDir,
    # Directory the finished installer .exe and its SHA256SUMS.txt are written to.
    [string] $OutputDir
)

$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot
Set-Location -LiteralPath $repoRoot

function Invoke-Checked {
    param(
        [Parameter(Mandatory = $true)] [string] $Name,
        [Parameter(Mandatory = $true)] [scriptblock] $Action
    )
    $timer = [Diagnostics.Stopwatch]::StartNew()
    Write-Host "==> $Name"
    & $Action | Out-Host
    if ($LASTEXITCODE -ne 0) {
        throw "$Name failed with exit code $LASTEXITCODE."
    }
    $timer.Stop()
    Write-Host ("    completed in {0:n1}s" -f $timer.Elapsed.TotalSeconds)
}

# ---------------------------------------------------------------------------
# Locate (or build) the custom log-enabled NSIS 3.10 package this repository
# already carries the source and plugin bootstrap for. Mirrors the
# Ensure-Nsis() logic in tools\build-windows.ps1 without editing that file.
# ---------------------------------------------------------------------------
$nsisRoot = Join-Path $repoRoot 'tools\win.x86\nsis\v3.10-log-r1'
$makensis = Join-Path $nsisRoot 'makensis.exe'
$nsisRequired = @(
    $makensis,
    (Join-Path $nsisRoot 'Include\nsProcess.nsh'),
    (Join-Path $nsisRoot 'Plugins\x86-unicode\nsProcess.dll'),
    (Join-Path $nsisRoot 'Plugins\x86-unicode\AccessControl.dll'),
    (Join-Path $nsisRoot 'VirtualBox-NSIS_CONFIG_LOG.txt')
)
if (@($nsisRequired | Where-Object { -not (Test-Path -LiteralPath $_) }).Count -gt 0) {
    $environmentFile = Join-Path $repoRoot 'env.bat'
    if (-not (Test-Path -LiteralPath $environmentFile)) {
        throw 'The custom log-enabled NSIS 3.10 package is missing and env.bat does not exist to build it. Run configure.py (see tools\build-windows.ps1) first, then rerun this script.'
    }
    $prepare = Join-Path $repoRoot 'tools\prepare-nsis.ps1'
    $powerShell = Get-Command pwsh.exe -ErrorAction SilentlyContinue
    if (-not $powerShell) { $powerShell = Get-Command powershell.exe -ErrorAction Stop }
    Invoke-Checked 'Build custom NSIS 3.10 with log support' {
        & $powerShell.Source -NoLogo -NoProfile -ExecutionPolicy Bypass -File $prepare -EnvironmentFile $environmentFile
    }
    if (@($nsisRequired | Where-Object { -not (Test-Path -LiteralPath $_) }).Count -gt 0) {
        throw 'The custom NSIS source build did not materialize the complete log-enabled package.'
    }
}
Write-Host "Using NSIS at $makensis"

# ---------------------------------------------------------------------------
# Validate the staged release payload the installer will be built from.
# ---------------------------------------------------------------------------
if (-not $PayloadDir) {
    $PayloadDir = Join-Path $repoRoot 'out\win.amd64\release\bin'
}
$PayloadDir = [IO.Path]::GetFullPath($PayloadDir)
if (-not (Test-Path -LiteralPath $PayloadDir -PathType Container)) {
    throw "Payload directory does not exist: $PayloadDir. Build the Windows host binaries first (tools\build-windows.ps1 -Mode Build)."
}
$payloadRequired = @(
    'VirtualBox.exe', 'VBoxSVC.exe', 'VBoxSDS.exe',
    'VBoxProxyStub.dll', 'VBoxC.dll', 'VBoxDrvInst.exe'
)
$missingPayload = @($payloadRequired | Where-Object { -not (Test-Path -LiteralPath (Join-Path $PayloadDir $_)) })
if ($missingPayload.Count -gt 0) {
    throw "The staged release payload at $PayloadDir is missing required file(s): $($missingPayload -join ', '). Build the Windows host binaries first (tools\build-windows.ps1 -Mode Build)."
}
Write-Host "Using payload at $PayloadDir"

# ---------------------------------------------------------------------------
# Product version and branding, read from the same source file
# tools\build-windows.ps1 itself reads for its own packaging metadata
# (Version.kmk), plus Config.kmk for the vendor/product strings Version.kmk
# does not carry. Neither file is modified here.
# ---------------------------------------------------------------------------
function Get-KmkValue {
    param(
        [Parameter(Mandatory = $true)] [string] $File,
        [Parameter(Mandatory = $true)] [string] $Pattern,
        [Parameter(Mandatory = $true)] [string] $Description
    )
    $match = Select-String -LiteralPath $File -Pattern $Pattern | Select-Object -First 1
    if (-not $match) { throw "$File did not provide $Description (pattern: $Pattern)." }
    return $match.Matches[0].Groups[1].Value.Trim()
}

$major = Get-KmkValue -File 'Version.kmk' -Pattern '^VBOX_VERSION_MAJOR\s*=\s*(\d+)' -Description 'VBOX_VERSION_MAJOR'
$minor = Get-KmkValue -File 'Version.kmk' -Pattern '^VBOX_VERSION_MINOR\s*=\s*(\d+)' -Description 'VBOX_VERSION_MINOR'
$build = Get-KmkValue -File 'Version.kmk' -Pattern '^VBOX_VERSION_BUILD\s*=\s*(\d+)' -Description 'VBOX_VERSION_BUILD'
$version = "$major.$minor.$build"

$vendor = Get-KmkValue -File 'Config.kmk' -Pattern '^VBOX_VENDOR\s*=\s*(.+)$' -Description 'VBOX_VENDOR'
$vendorShort = Get-KmkValue -File 'Config.kmk' -Pattern '^VBOX_VENDOR_SHORT\s*=\s*(.+)$' -Description 'VBOX_VENDOR_SHORT'
$product = "$vendorShort VirtualBox"
Write-Host "Building installer for $product $version (vendor: $vendor)"

# ---------------------------------------------------------------------------
# Compile.
# ---------------------------------------------------------------------------
if (-not $OutputDir) {
    $OutputDir = Join-Path $repoRoot 'out\win.amd64\release\nsis-installer'
}
New-Item -ItemType Directory -Force $OutputDir | Out-Null
$outFile = Join-Path $OutputDir "VirtualBox-$version-Setup.exe"
Remove-Item -LiteralPath $outFile -Force -ErrorAction SilentlyContinue

$nsiScript = Join-Path $repoRoot 'src\VBox\Installer\win\NSIS\VBoxHostInstaller.nsi'
if (-not (Test-Path -LiteralPath $nsiScript)) {
    throw "NSIS installer script not found: $nsiScript"
}

Invoke-Checked 'Compile the unsigned NSIS host installer' {
    & $makensis /V3 `
        "/DPAYLOAD_DIR=$PayloadDir" `
        "/DPRODUCT_VERSION=$version" `
        "/DOUT_FILE=$outFile" `
        "/DVBOX_VENDOR=$vendor" `
        "/DVBOX_VENDOR_SHORT=$vendorShort" `
        "/DVBOX_PRODUCT=$product" `
        $nsiScript
}

# ---------------------------------------------------------------------------
# Verify what was actually produced -- never trust a zero exit code alone.
# ---------------------------------------------------------------------------
if (-not (Test-Path -LiteralPath $outFile)) {
    throw "makensis.exe reported success but did not produce $outFile."
}
$item = Get-Item -LiteralPath $outFile
if ($item.Length -eq 0) {
    throw "$outFile was produced but is zero bytes."
}
if ($item.Name -notmatch '^VirtualBox-[0-9][0-9.]*-Setup\.exe$') {
    throw "Unexpected installer file name: $($item.Name)."
}
$signature = Get-AuthenticodeSignature -LiteralPath $outFile
if ($signature.Status -ne 'NotSigned') {
    throw "Expected an unsigned installer (code signing is permanently disabled for this project): $($item.Name) ($($signature.Status))."
}

$hash = (Get-FileHash -LiteralPath $outFile -Algorithm SHA256).Hash
Set-Content -LiteralPath (Join-Path $OutputDir 'SHA256SUMS.txt') `
    -Value ("{0}  {1}" -f $hash.ToLowerInvariant(), $item.Name) -Encoding UTF8

Write-Host "Unsigned NSIS installer: $outFile"
Write-Host "Size: $([Math]::Round($item.Length / 1MB, 1)) MiB"
Write-Host "SHA-256: $hash"
Write-Host 'Completed without code signing. This installer will show an "unknown publisher" warning; that is expected.'
