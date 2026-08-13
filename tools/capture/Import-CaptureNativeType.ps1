#requires -Version 5.1
<#
.SYNOPSIS
    Shared loader for VbxCaptureNative.cs, tolerant of PowerShell/.NET's
    inconsistent Add-Type reference-resolution behavior.

.DESCRIPTION
    Dot-source this file, then call Import-VbxCaptureNativeType. It compiles
    tools/capture/VbxCaptureNative.cs exactly once per process (safe to call
    repeatedly) and makes the VbxCaptureHarness.Native type available.

    Why this needs a cascade instead of one Add-Type call: on classic
    Windows PowerShell 5.1 (.NET Framework), System.Drawing is one monolithic
    assembly and a bare `Add-Type -Path ...` with no -ReferencedAssemblies
    resolves Bitmap/Image/Color fine on its own. On PowerShell 7 running a
    recent split-apart .NET (verified against 7.6.4 / .NET 10 preview on
    this host), System.Drawing is a type-forwarding facade over
    System.Drawing.Common, which itself depends on System.Drawing.Primitives
    plus two more GDI+ implementation assemblies
    (System.Private.Windows.GdiPlus, System.Private.Windows.Core) -- and the
    moment ANY -ReferencedAssemblies value is supplied at all, Add-Type stops
    adding to its default reference set and starts REPLACING it, which also
    silently drops System.Collections.Generic.List<T> unless System.Collections
    is named explicitly too. None of this is discoverable except by
    compiling and reading the exact CS0246/CS0012/CS1069 that comes back, so
    rather than hard-code one host's answer, this tries progressively more
    explicit strategies and reports every failure if all of them miss.
#>

function Import-VbxCaptureNativeType {
    [CmdletBinding()]
    param(
        [string]$SourcePath = (Join-Path $PSScriptRoot 'VbxCaptureNative.cs')
    )

    if ('VbxCaptureHarness.Native' -as [type]) {
        return # already compiled in this process
    }

    if (-not (Test-Path -LiteralPath $SourcePath)) {
        throw "Required native source file not found: $SourcePath"
    }

    Add-Type -AssemblyName System.Drawing -ErrorAction SilentlyContinue

    $strategies = [ordered]@{
        'no explicit references (classic Windows PowerShell / monolithic System.Drawing)' = {
            Add-Type -Path $SourcePath -ErrorAction Stop
        }
        "'System.Drawing' only" = {
            Add-Type -Path $SourcePath -ReferencedAssemblies 'System.Drawing' -ErrorAction Stop
        }
        'explicit split-assembly set (System.Drawing.Common / .Primitives / .Collections / .Private.Windows.GdiPlus / .Private.Windows.Core)' = {
            Add-Type -Path $SourcePath -ReferencedAssemblies @(
                'System.Drawing.Common',
                'System.Drawing.Primitives',
                'System.Collections',
                'System.Private.Windows.GdiPlus',
                'System.Private.Windows.Core'
            ) -ErrorAction Stop
        }
    }

    $attemptErrors = @()
    foreach ($name in $strategies.Keys) {
        try {
            & $strategies[$name]
            if ('VbxCaptureHarness.Native' -as [type]) {
                Write-Verbose "[capture-harness] Native type compiled using strategy: $name"
                return
            }
        } catch {
            $attemptErrors += "  - $name`n    $($_.Exception.Message)"
        }
    }

    throw "Could not compile VbxCaptureNative.cs under any known Add-Type reference strategy on this PowerShell/.NET host ($($PSVersionTable.PSVersion), $($PSVersionTable.PSEdition)). Attempts:`n$($attemptErrors -join "`n")"
}
