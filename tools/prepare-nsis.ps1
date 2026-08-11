# $Id$
# Build the documented VirtualBox NSIS 3.10 log-enabled packaging tool.
#
# Copyright (C) 2026 Oracle and/or its affiliates.
# SPDX-License-Identifier: GPL-3.0-only

[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string] $EnvironmentFile,
    [switch] $Force
)

$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot
Set-Location -LiteralPath $repoRoot
$quote = [char]34
$nl = [char]10
$tab = [char]9

function Write-Utf8NoBom {
    param([Parameter(Mandatory = $true)] [string] $Path, [Parameter(Mandatory = $true)] [string] $Value)
    [IO.File]::WriteAllText($Path, $Value, [Text.UTF8Encoding]::new($false))
}

function Replace-Once {
    param(
        [Parameter(Mandatory = $true)] [string] $Text,
        [Parameter(Mandatory = $true)] [string] $Old,
        [Parameter(Mandatory = $true)] [string] $New,
        [Parameter(Mandatory = $true)] [string] $Description
    )
    $count = ([regex]::Matches($Text, [regex]::Escape($Old))).Count
    if ($count -ne 1) { throw "NSIS source patch expected one $Description match, found $count." }
    return $Text.Replace($Old, $New)
}

function Invoke-Checked {
    param([Parameter(Mandatory = $true)] [string] $Name, [Parameter(Mandatory = $true)] [scriptblock] $Action)
    Write-Host "==> $Name"
    & $Action
    if ($LASTEXITCODE -ne 0) { throw "$Name failed with exit code $LASTEXITCODE." }
}

$environmentPath = [IO.Path]::GetFullPath($EnvironmentFile)
if (-not (Test-Path -LiteralPath $environmentPath)) { throw "The configured VirtualBox environment file was not found: $environmentPath" }

$target = Join-Path $repoRoot 'tools\win.x86\nsis\v3.10-log-r1'
$marker = Join-Path $target 'VirtualBox-NSIS_CONFIG_LOG.txt'
$required = @(
    (Join-Path $target 'makensis.exe'),
    (Join-Path $target 'Include\nsProcess.nsh'),
    (Join-Path $target 'Plugins\x86-unicode\nsProcess.dll'),
    (Join-Path $target 'Plugins\x86-unicode\AccessControl.dll'),
    $marker
)
if (-not $Force -and @($required | Where-Object { -not (Test-Path -LiteralPath $_) }).Count -eq 0) {
    Write-Host 'Validated NSIS 3.10 log-enabled package already exists; reusing it.'
    exit 0
}

$programFilesX86 = [Environment]::GetEnvironmentVariable('ProgramFiles(x86)')
$toolRoot = Join-Path $env:LOCALAPPDATA 'VirtualBox\build-tools'
$downloadRoot = Join-Path $toolRoot 'downloads'
$sourceWork = Join-Path $toolRoot 'nsis-3.10-source'
New-Item -ItemType Directory -Force $toolRoot, $downloadRoot | Out-Null

$sevenZip = (Get-Command 7z.exe -ErrorAction SilentlyContinue).Source
if (-not $sevenZip) {
    $sevenZip = @(
        (Join-Path $env:ProgramFiles '7-Zip\7z.exe'),
        (Join-Path $programFilesX86 '7-Zip\7z.exe')
    ) | Where-Object { $_ -and (Test-Path -LiteralPath $_) } | Select-Object -First 1
}
if (-not $sevenZip) { throw '7-Zip is required to unpack the pinned NSIS source and plugin archives.' }
$curl = (Get-Command curl.exe -ErrorAction SilentlyContinue).Source
if (-not $curl) { throw 'curl.exe is required to download the pinned NSIS source and plugin archives.' }
$python = (Get-Command python.exe -ErrorAction SilentlyContinue).Source
if (-not $python) { throw 'Python 3 is required to install the pinned SCons release.' }

$sourceArchive = Join-Path $downloadRoot 'nsis-3.10-src.tar.bz2'
if (-not (Test-Path -LiteralPath $sourceArchive)) {
    & $curl -L --fail --silent --show-error --output $sourceArchive 'https://prdownloads.sourceforge.net/nsis/nsis-3.10-src.tar.bz2?download'
    if ($LASTEXITCODE -ne 0) { throw "NSIS source download failed with exit code $LASTEXITCODE." }
}
$sourceHash = (Get-FileHash -LiteralPath $sourceArchive -Algorithm SHA256).Hash.ToUpperInvariant()
if ($sourceHash -ne '11B54A6307AB46FEF505B2700AAF6F62847C25AA6EEBAF2AE0AAB2F17F0CB297') {
    throw "NSIS source SHA-256 mismatch: expected 11B54A6307AB46FEF505B2700AAF6F62847C25AA6EEBAF2AE0AAB2F17F0CB297, got $sourceHash."
}

Remove-Item -LiteralPath $sourceWork -Recurse -Force -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Force $sourceWork | Out-Null
Invoke-Checked 'Extract NSIS 3.10 source archive' { & $sevenZip x $sourceArchive "-o$sourceWork" '-y' }
$sourceTar = Get-ChildItem -LiteralPath $sourceWork -Filter '*.tar' -File | Select-Object -First 1
if (-not $sourceTar) { throw 'NSIS source archive did not produce its tar payload.' }
Invoke-Checked 'Extract NSIS 3.10 source tree' { & $sevenZip x $sourceTar.FullName "-o$sourceWork" '-y' }
$source = Join-Path $sourceWork 'nsis-3.10-src'
$sconstruct = Join-Path $source 'SConstruct'
if (-not (Test-Path -LiteralPath $sconstruct)) { throw 'NSIS source extraction did not provide SConstruct.' }

# This is the functional patch carried in doc/tools/win.x86/nsis/readme.tool:
# add Visual Studio 14.3 discovery and apply VBoxPeSetVersion to generated PE files.
$sourceText = [IO.File]::ReadAllText($sconstruct).Replace([Environment]::NewLine, $nl)
$old = "opts.Add(EnumVariable('MSVS_VERSION', 'MS Visual C++ version', os.environ.get('MSVS_VERSION'), allowed_values=('6.0', '7.0', '7.1', '8.0', '8.0Exp', '9.0', '9.0Exp', '10.0', '10.0Exp')))"
$new = "opts.Add(EnumVariable('MSVS_VERSION', 'MS Visual C++ version', os.environ.get('MSVS_VERSION'), allowed_values=('6.0', '7.0', '7.1', '8.0', '8.0Exp', '9.0', '9.0Exp', '10.0', '10.0Exp', '14.3')))"
$sourceText = Replace-Once -Text $sourceText -Old $old -New $new -Description 'MSVS_VERSION 14.3 support'
$old = "Import('SilentActionEcho IsPEExecutable SetPESecurityFlagsWorker MakeReproducibleAction')"
$new = "def VBoxPeSetVersion(aoTargets, oEnv = defenv): # vbox$nl    sVBoxSetPeVersion = os.environ.get('MY_VBOX_PE_SET_VERSION', 'VBoxPeSetVersion.exe');$nl    for oTarget in aoTargets:$nl        oAction = defenv.Action(sVBoxSetPeVersion + ' --nt4 ""%s""' % (oTarget.path,))$nl        oEnv.AddPostAction(oTarget, oAction)$($nl)$($nl)Import('SilentActionEcho IsPEExecutable SetPESecurityFlagsWorker MakeReproducibleAction')"
$sourceText = Replace-Once -Text $sourceText -Old $old -New $new -Description 'VBoxPeSetVersion helper'
$old = $tab + "env.SideEffect('%s/stub_%s.map' % (build_dir, stub), target)" + $nl
$new = $old + $tab + 'VBoxPeSetVersion(target, env); # vbox' + $nl
$sourceText = Replace-Once -Text $sourceText -Old $old -New $new -Description 'stub PE version hook'
$old = $tab + 'defenv.MakeReproducible(plugin)' + $nl
$new = $old + $tab + 'VBoxPeSetVersion(plugin, defenv); # vbox' + $nl
$sourceText = Replace-Once -Text $sourceText -Old $old -New $new -Description 'plugin PE version hook'
Write-Utf8NoBom -Path $sconstruct -Value $sourceText

Invoke-Checked 'Install SCons 4.8.1' { & $python -m pip install --disable-pip-version-check --user 'scons==4.8.1' }
$userBase = (& $python -m site --user-base 2>$null | Select-Object -First 1).Trim()
$scons = Join-Path $userBase 'Scripts\scons.exe'
if (-not (Test-Path -LiteralPath $scons)) { throw "SCons 4.8.1 was not materialized at the expected user-scoped path: $scons" }

$originalVboxOse = $env:VBOX_OSE
Remove-Item Env:VBOX_OSE -ErrorAction SilentlyContinue
try {
    Invoke-Checked 'Build the x86 NSIS zlib and PE-version prerequisites' {
        & cmd.exe /d /c "call $quote$environmentPath$quote && kmk KBUILD_TARGET_ARCH=x86 KBUILD_TYPE=release VBOX_OSE= VBOX_WITH_ADDITIONS=1 nsis-zlib-install VBoxPeSetVersion"
    }
} finally {
    if ($null -eq $originalVboxOse) { Remove-Item Env:VBOX_OSE -ErrorAction SilentlyContinue }
    else { $env:VBOX_OSE = $originalVboxOse }
}

$releaseRoot = Join-Path $repoRoot 'out\win.x86\release'
$zlibHeader = Get-ChildItem -LiteralPath $releaseRoot -Recurse -Filter 'zlib.h' -File -ErrorAction SilentlyContinue |
    Where-Object {
        (Test-Path -LiteralPath (Join-Path $_.DirectoryName 'zdll.lib')) -and
        (Test-Path -LiteralPath (Join-Path $_.DirectoryName 'zlib1.dll'))
    } | Select-Object -First 1
if (-not $zlibHeader) { throw 'The x86 NSIS zlib build did not provide zlib.h, zdll.lib, and zlib1.dll together.' }
$zlibRoot = $zlibHeader.DirectoryName
$peTool = Get-ChildItem -LiteralPath $releaseRoot -Recurse -Filter 'VBoxPeSetVersion.exe' -File -ErrorAction SilentlyContinue | Select-Object -First 1
if (-not $peTool) { throw 'The x86 prerequisite build did not provide VBoxPeSetVersion.exe.' }

$vswhere = Join-Path $programFilesX86 'Microsoft Visual Studio\Installer\vswhere.exe'
$vcvars = $null
if (Test-Path -LiteralPath $vswhere) {
    $vsInstall = (& $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath 2>$null | Select-Object -First 1).Trim()
    if ($vsInstall) {
        $candidate = Join-Path $vsInstall 'VC\Auxiliary\Build\vcvarsall.bat'
        if (Test-Path -LiteralPath $candidate) { $vcvars = Get-Item -LiteralPath $candidate }
    }
}
if (-not $vcvars) {
    $vcvars = Get-ChildItem -LiteralPath $programFilesX86 -Recurse -Filter 'vcvarsall.bat' -File -ErrorAction SilentlyContinue |
        Where-Object { $_.FullName -match '\\VC\\Auxiliary\\Build\\vcvarsall\.bat$' } | Select-Object -First 1
}
if (-not $vcvars) {
    $vcvars = Get-ChildItem -LiteralPath $env:ProgramFiles -Recurse -Filter 'vcvarsall.bat' -File -ErrorAction SilentlyContinue |
        Where-Object { $_.FullName -match '\\VC\\Auxiliary\\Build\\vcvarsall\.bat$' } | Select-Object -First 1
}
if (-not $vcvars) { throw 'Visual Studio vcvarsall.bat was not found for the x86 NSIS source build.' }

$sconsLog = Join-Path $sourceWork 'scons.log'
$zlibForScons = $zlibRoot.Replace('\', '/')
$peForScons = $peTool.FullName.Replace('\', '/')
$sconsForCmd = $scons.Replace('\', '/')
$sourceForCmd = $source.Replace('\', '/')
$vcvarsForCmd = $vcvars.FullName
$sconsCommand = "call $quote$vcvarsForCmd$quote x86 && set $quote" + 'CODESIGNER=' + "$quote && set $quote" + "MY_VBOX_PE_SET_VERSION=$peForScons$quote && cd /d $quote$sourceForCmd$quote && $quote$sconsForCmd$quote MSVC_USE_SCRIPT=None MSTOOLKIT=yes MSVS_VERSION=14.3 TARGET_ARCH=x86 UNICODE=yes SKIPUTILS=$quote" + 'NSIS Menu' + "$quote SKIPTESTS=all SKIPDOC=all APPEND_CCFLAGS=-arch:IA32 STRIP=1 STRIP_W32=1 NSIS_CONFIG_LOG=1 ZLIB_W32=$zlibForScons dist > $quote$sconsLog$quote 2>&1"
Invoke-Checked 'Build the NSIS 3.10 log-enabled distribution' { & cmd.exe /d /c $sconsCommand }

$instdist = Join-Path $source '.instdist'
$builtMakensis = Join-Path $instdist 'makensis.exe'
if (-not (Test-Path -LiteralPath $builtMakensis)) { throw 'The NSIS source build did not produce .instdist\makensis.exe.' }
Remove-Item -LiteralPath $target -Recurse -Force -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Force $target | Out-Null
Copy-Item -Path (Join-Path $instdist '*') -Destination $target -Recurse -Force

function Install-NsisPlugin {
    param(
        [Parameter(Mandatory = $true)] [string] $Name,
        [Parameter(Mandatory = $true)] [string] $Uri,
        [Parameter(Mandatory = $true)] [string] $Sha256,
        [Parameter(Mandatory = $true)] [string] $ExtractedPath,
        [Parameter(Mandatory = $true)] [string] $Destination
    )
    $archive = Join-Path $downloadRoot $Name
    if (-not (Test-Path -LiteralPath $archive)) {
        & $curl -L --fail --silent --show-error --output $archive $Uri
        if ($LASTEXITCODE -ne 0) { throw "NSIS plugin archive download failed for $Name (exit code $LASTEXITCODE)." }
    }
    $actual = (Get-FileHash -LiteralPath $archive -Algorithm SHA256).Hash.ToUpperInvariant()
    if ($actual -ne $Sha256) { throw "SHA-256 mismatch for $($Name): expected $Sha256, got $actual." }
    $extract = Join-Path $toolRoot ([IO.Path]::GetFileNameWithoutExtension($Name))
    Remove-Item -LiteralPath $extract -Recurse -Force -ErrorAction SilentlyContinue
    New-Item -ItemType Directory -Force $extract | Out-Null
    Invoke-Checked "Extract $Name" { & $sevenZip x $archive "-o$extract" '-y' }
    $sourcePath = Join-Path $extract $ExtractedPath
    if (-not (Test-Path -LiteralPath $sourcePath)) { throw "$Name did not provide $ExtractedPath." }
    New-Item -ItemType Directory -Force (Split-Path -Parent $Destination) | Out-Null
    Copy-Item -LiteralPath $sourcePath -Destination $Destination -Force
}

$nsProcessPluginSpec = @{
    Name = 'NsProcess-1.6.zip'
    Uri = 'https://nsis.sourceforge.io/mediawiki/images/1/18/NsProcess.zip'
    Sha256 = 'FC19FC66A5219A233570FAFD5DAEB0C9B85387B379F6DF5AC8898159A57C5944'
    ExtractedPath = 'Include\nsProcess.nsh'
    Destination = Join-Path $target 'Include\nsProcess.nsh'
}
Install-NsisPlugin @nsProcessPluginSpec
$nsProcessArchive = Join-Path $downloadRoot 'NsProcess-1.6.zip'
$nsProcessExtract = Join-Path $toolRoot 'NsProcess-1.6'
Remove-Item -LiteralPath $nsProcessExtract -Recurse -Force -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Force $nsProcessExtract | Out-Null
Invoke-Checked 'Extract NsProcess 1.6 Unicode plugin' { & $sevenZip x $nsProcessArchive "-o$nsProcessExtract" '-y' }
$nsProcessUnicode = Join-Path $nsProcessExtract 'Plugin\nsProcessW.dll'
if (-not (Test-Path -LiteralPath $nsProcessUnicode)) { throw 'NsProcess 1.6 archive did not provide Plugin\nsProcessW.dll.' }
New-Item -ItemType Directory -Force (Join-Path $target 'Plugins\x86-unicode') | Out-Null
Copy-Item -LiteralPath $nsProcessUnicode -Destination (Join-Path $target 'Plugins\x86-unicode\nsProcess.dll') -Force

$accessControlPluginSpec = @{
    Name = 'AccessControl-1.0.8.3.zip'
    Uri = 'https://nsis.sourceforge.io/mediawiki/images/4/4a/AccessControl.zip'
    Sha256 = '9AA60F9C5C023FDA2808AF216514D8913D2673BC522D944EE771DA032A1BDC10'
    ExtractedPath = 'Plugins\i386-unicode\AccessControl.dll'
    Destination = Join-Path $target 'Plugins\x86-unicode\AccessControl.dll'
}
Install-NsisPlugin @accessControlPluginSpec

$probeScript = Join-Path $sourceWork 'log-probe.nsi'
$probeOutput = Join-Path $sourceWork 'log-probe.exe'
Write-Utf8NoBom -Path $probeScript -Value ('OutFile ' + $quote + $probeOutput + $quote + $nl + 'LogText ' + $quote + 'VirtualBox NSIS_CONFIG_LOG probe' + $quote + $nl)
& (Join-Path $target 'makensis.exe') $probeScript
if ($LASTEXITCODE -ne 0) { throw 'The built makensis.exe rejected LogText; NSIS_CONFIG_LOG is not enabled.' }
Remove-Item -LiteralPath $probeScript, $probeOutput -Force -ErrorAction SilentlyContinue
Write-Utf8NoBom -Path $marker -Value ('NSIS 3.10 source SHA-256: ' + $sourceHash + $nl + 'NSIS_CONFIG_LOG: enabled' + $nl)

$missing = @($required | Where-Object { -not (Test-Path -LiteralPath $_) })
if ($missing.Count -gt 0) { throw "Custom NSIS package is incomplete: $($missing -join ', ')" }
Write-Host "Custom NSIS package ready at $target."
