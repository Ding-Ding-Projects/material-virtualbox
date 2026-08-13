# Runtime capture harness

Before this document existed, this repository had **no runtime screenshot/capture
harness at all**. [`CaptureMatrix.md`](CaptureMatrix.md) enumerated 45+ surfaces
that must be photographed, and every single row read "Not captured" with no tool
in the repository capable of producing one. This document describes the tool that
now exists to close those rows honestly, one real capture at a time.

[`RuntimeCapture.md`](RuntimeCapture.md) remains the *contract* for what counts
as evidence. This document describes the *tool* that implements that contract:
where it lives, how it works, what it guards against, and how to run it.

## What it is

[`tools/capture/Invoke-CaptureHarness.ps1`](../../tools/capture/Invoke-CaptureHarness.ps1)
launches a target Windows GUI executable on a **named, off-screen Win32 desktop**
(created with `CreateDesktopW`), so the operator's visible desktop, cursor,
keyboard focus, and foreground window are never touched. It then:

1. Resolves the application's real top-level windows **dynamically at runtime**
   by enumerating every window on that off-screen desktop with
   `EnumDesktopWindows` — never a hard-coded handle, since a window handle is
   different every single launch.
2. Filters out the junk windows a Qt/Win32 process routinely spawns alongside
   its real UI (see "Window filtering" below).
3. Captures each surviving window with Win32 `PrintWindow(..., PW_RENDERFULLCONTENT)`,
   which works even though the window is unfocused and living on a desktop
   nobody is looking at.
4. **Validates every capture before trusting it** (see "Why every capture is
   validated" below) by reading the PNG back and sampling it on a grid; a
   capture whose sampled pixels are all one colour is rejected as `invalid`,
   never silently accepted as a legible screenshot.
5. Writes a JSON manifest recording, per capture: the surface label, the
   resolved window class and title, pixel dimensions, the output path, the
   target app path, a UTC timestamp, and whatever build/environment metadata
   the caller supplied (commit, Qt version, display scale, language mode,
   profile state).
6. Always cleans up — terminates the launched process tree and closes the
   off-screen desktop handle — in a `finally` block, on success or failure.

The harness is split across three files under [`tools/capture/`](../../tools/capture/):

| File | Role |
| --- | --- |
| `VbxCaptureNative.cs` | The actual Win32 P/Invoke surface: `CreateDesktop`, `EnumDesktopWindows`, `PrintWindow`, and the pixel-grid uniformity check, compiled at runtime via `Add-Type`. |
| `Import-CaptureNativeType.ps1` | A small shared loader both other scripts dot-source. It exists solely because `Add-Type`'s assembly-reference resolution for `System.Drawing` turned out to differ across PowerShell/.NET hosts — see "A portability wrinkle" below. |
| `Invoke-CaptureHarness.ps1` | The harness itself: desktop/process lifecycle, window discovery and filtering, capture, validation, and manifest writing. |
| `Test-UniformnessCheck.ps1` | A standalone self-test that proves the black-frame detector actually detects a black frame (and a white one, and correctly *accepts* real content) — see "Proving the validator works" below. |

## Window filtering: why "grab the first window" doesn't work

A Qt GUI process on Windows routinely owns several top-level windows that have
nothing to do with its actual UI:

- `"Default IME"` / class `IME` — the input-method window
- `"MSCTFIME UI"` / class `MSCTFIME UI` — the TSF input window
- class `CicLoaderWndClass` — the Cicero loader
- class `CiceroUIWndFrame` — the Cicero UI frame
- `"Touch Tooltip Window"` — the touch-tooltip helper
- UAC input-indicator windows
- class `Qt<version>ScreenChangeObserverWindow` — Qt's own screen-size observer
  window, sized to the **entire screen**, which is easy to mistake for the real
  application window if you filter only on size

Naively enumerating a process's windows and grabbing "the first one" reliably
lands on one of these — most often the 0x0 IME window — and produces a capture
failure that looks like the *application's* fault rather than the harness
filtering badly.

The harness recognises a real surface by elimination rather than by
allow-listing: a window qualifies only if it belongs to the launched process,
is visible (`IsWindowVisible`), has a non-empty title, has a real size
(configurable `-MinWidth`/`-MinHeight`, default 2×2), **and** does not match any
of the known-junk class/title patterns above. Junk windows are recognised by
denylist pattern, never real surfaces by an allow-list of specific Qt class
names — because the Qt version string embedded in those class names (`Qt683...`
today) changes every Qt upgrade, and a hard-coded allow-list would silently stop
matching the very next time Qt is bumped.

Window discovery also **arrives late**: a process's main window does not exist
the instant `CreateProcess` returns. The harness polls
(`-WindowTimeoutSeconds`, default 30, at `-PollIntervalMs` intervals, default
250ms) rather than assuming the window is there immediately, and reports
honestly — `status: "not_found"` in the manifest, exit code `2` — if nothing
ever appears within the timeout.

After the *first* qualifying window is found, the harness deliberately waits
`-SettleSeconds` (default 3.0) before doing a **final** enumeration pass, because
a second window (for example an asynchronous COM-failure error dialog appearing
a moment after the main window) needs time to appear and finish rendering. The
final pass captures **every** qualifying window found at that point, not just
the first one — one PNG and one manifest entry per window — since a single
invocation can legitimately surface more than one real surface at once (a main
window plus a modal dialog, for instance).

## Why every capture is validated, not trusted

A freshly allocated GDI bitmap is **uninitialised memory**, which reads back as
solid black. `PrintWindow` returns success (`TRUE`) whether or not the target
window actually painted anything into the device context it was handed — a
window that never handled `WM_PRINT`/`WM_PRINTCLIENT` for the requested content
can report success while leaving the bitmap exactly as it started: solid black.
A harness that trusts `PrintWindow`'s return value alone will cheerfully write
black rectangles into the capture matrix and call it a screenshot.

So after every capture, `Invoke-CaptureHarness.ps1` reads the saved PNG back and
samples it on a configurable grid (`-UniformGridSize`, default 12×12 = 144
points spread evenly across the image). If every sampled pixel is the exact
same colour, the image is reported `status: "invalid"` in the manifest — the
file is kept on disk for forensic inspection, but it is never marked
`"captured"` and must never be treated as evidence.

### Proving the validator works

A check that has never been watched fail proves nothing about what it would
catch. [`Test-UniformnessCheck.ps1`](../../tools/capture/Test-UniformnessCheck.ps1)
loads the exact same compiled `IsImageUniform` implementation the real harness
uses (via the shared `VbxCaptureNative.cs`/`Import-CaptureNativeType.ps1`, so
there is no second, driftable copy of the check) and asserts three cases:

1. A synthetic **solid black** image — the exact shape of the
   `PrintWindow`-reported-success-but-painted-nothing failure — **must** be
   flagged uniform (invalid).
2. A synthetic **solid white** image **must also** be flagged uniform, proving
   the detector checks for *any* uniform colour, not specifically "is it
   black" — a uniformly white frame is exactly as suspect as a black one.
3. A synthetic **checkerboard of many distinct colours**, standing in for a
   legible UI screenshot, **must not** be flagged uniform — proving the check
   doesn't also reject everything, which would make it useless.

Run it directly:

```powershell
pwsh -NoProfile -File tools/capture/Test-UniformnessCheck.ps1
```

All three cases passed on this repository's Windows 11 / PowerShell 7.6.4 /
.NET 10 host:

```
[PASS] solid black frame (the PrintWindow-failed-silently case): expected uniform=True, got uniform=True (1/144 distinct sampled colors)
[PASS] solid white frame (uniform-but-not-black control case): expected uniform=True, got uniform=True (1/144 distinct sampled colors)
[PASS] checkerboard of distinct colors (a legible-capture stand-in): expected uniform=False, got uniform=False (132/144 distinct sampled colors)
SELF-TEST PASSED: the uniform-image detector correctly flags solid-color frames (black AND white) as invalid, and correctly accepts varied content as valid.
```

## A portability wrinkle: `Add-Type -ReferencedAssemblies`

Getting `Add-Type` to compile `VbxCaptureNative.cs` reliably took longer than
writing the P/Invoke surface itself, and the reason is worth recording so the
next person doesn't rediscover it the hard way.

On this host (PowerShell 7.6.4 on a recent split-apart .NET), `System.Drawing`
is a type-forwarding facade over `System.Drawing.Common`, which itself depends
on `System.Drawing.Primitives` plus two GDI+ implementation assemblies
(`System.Private.Windows.GdiPlus`, `System.Private.Windows.Core`). Supplying
**any** value to `Add-Type -ReferencedAssemblies` — even just `'System.Drawing'`
— **replaces** `Add-Type`'s default reference set rather than adding to it, which
also silently drops `System.Collections.Generic.List<T>` out of scope (`CS0246`
on `List<>`) unless `System.Collections` is named explicitly too. None of this
is discoverable except by compiling and reading the exact `CS0246`/`CS0012`/
`CS1069` error that comes back one at a time.

Rather than hard-code one host's answer — which would likely break again on
classic Windows PowerShell 5.1 (.NET Framework), where `System.Drawing` is one
monolithic assembly and a bare `Add-Type -Path` with **no**
`-ReferencedAssemblies` works fine on its own — `Import-CaptureNativeType.ps1`
tries progressively more explicit strategies in order and reports every
attempt's exact error if all of them fail:

1. No `-ReferencedAssemblies` at all.
2. `-ReferencedAssemblies 'System.Drawing'` only.
3. The full explicit split-assembly set verified against this exact host.

Both `Invoke-CaptureHarness.ps1` and `Test-UniformnessCheck.ps1` dot-source this
loader rather than each compiling their own copy, so a fourth strategy only
ever needs adding in one place.

## Usage

```powershell
pwsh -NoProfile -File tools/capture/Invoke-CaptureHarness.ps1 `
    -TargetPath 'C:\path\to\VirtualBox.exe' `
    -OutputDir  'doc\md3\captures' `
    -SurfaceLabel 'manager-com-failure' `
    -WindowTimeoutSeconds 40 -SettleSeconds 6 `
    -Commit '<commit-sha>' -QtVersion '6.8.3' -DisplayScale '100%' -LanguageMode 'English' `
    -ProfileState 'default user profile, no VMs registered'
```

Key parameters (see the script's comment-based help,
`Get-Help tools/capture/Invoke-CaptureHarness.ps1 -Full`, for the complete list):

| Parameter | Purpose |
| --- | --- |
| `-TargetPath` / `-OutputDir` | Required: what to launch and where to write PNGs + the manifest. |
| `-SurfaceLabel` | Human label for this invocation, used as a filename prefix. Each captured window still records its *own* real title/class in the manifest. |
| `-WindowTimeoutSeconds` / `-SettleSeconds` / `-PollIntervalMs` | Bounded, honest waiting for late-arriving windows — see "Window filtering" above. |
| `-TargetPathWaitSeconds` | Optional bounded wait for `-TargetPath` itself to exist and become size-stable before launching, for a target whose install directory another process may be actively (re)writing. Default 0 (fail fast). Added after this exact race was observed live: a Squirrel install directory that a separate installer-development lane was actively cycling made the target executable exist, then not exist, then exist again as a different size, within seconds, while this harness was being proven end to end. |
| `-Commit` / `-QtVersion` / `-DisplayScale` / `-LanguageMode` / `-ProfileState` / `-Notes` | Provenance metadata recorded verbatim on every manifest entry from the run. None are guessed — an omitted value is recorded as `null`, never invented, per this project's evidence rules. |
| `-ManifestPath` | Defaults to `<OutputDir>/capture-manifest.json`. Existing entries are preserved and new captures are **appended**, so repeated invocations across different surfaces and different runs accumulate into one auditable record instead of clobbering each other. |

Exit codes:

| Code | Meaning |
| --- | --- |
| `0` | At least one window was captured and validated non-uniform. |
| `2` | No qualifying window ever appeared (timeout). |
| `3` | Window(s) were captured, but every capture was rejected as uniform. |
| `4` | The target process failed to launch. |
| `5` | Unexpected harness error, or a precondition (e.g. `-TargetPath`) was never satisfied. |

## What it has actually proven so far

Validated end-to-end against `C:\Windows\System32\winver.exe` (a guaranteed
classic Win32 dialog, chosen as a stable smoke-test target after Windows 11's
`notepad.exe` — now an MSIX-packaged app — turned out not to render a window
when launched directly onto a non-interactive desktop via raw `CreateProcess`,
which packaged apps require normal AppX activation for): the harness created an
off-screen desktop, launched the process on it without touching the visible
desktop, resolved the real `"About Windows"` dialog window dynamically, captured
it via `PrintWindow`, and correctly validated it as non-uniform (18 of 144
sampled grid points were distinct colours). The saved PNG is a fully legible
rendering of the real dialog.

It has also been run against the real installed target,
`VirtualBox.exe` at `%LOCALAPPDATA%\VirtualBox\app-7.2.97\VirtualBox.exe`. The
brief for building this harness expected the one reachable surface to be the
`REGDB_E_CLASSNOTREG` COM-failure dialog described in
[`RuntimeCapture.md`](RuntimeCapture.md) — but that is not what the harness
actually found: it resolved and captured a fully rendered, functional Manager
shell window (class `Qt683QWindowIcon`, title "Material Virtual Machine
Manager", 1024×975px) with no COM/SDS error visible anywhere, including a live
"Can't enumerate USB ..." runtime notification that a static mock would not
produce. This directly contradicts blocker B's current wording for at least
this one build and environment; see
[`CaptureMatrix.md`](CaptureMatrix.md) row 1 for the committed evidence and the
"First real capture, and a discrepancy it surfaces" note above that row for the
full, carefully-hedged account (one build, one moment, one environment,
captured while a separate installer lane was independently and actively
working on this same machine's `VirtualBox` install directory — this is not a
claim that blocker B is resolved everywhere).

A portability note encountered while proving this end to end, worth recording
because it cost real time to diagnose: on this host, the `pwsh` resolved from
`PATH` (`C:\Users\<user>\AppData\Local\Microsoft\WindowsApps\pwsh.exe`) is the
Microsoft Store MSIX-packaged build of PowerShell 7, which runs inside an
AppContainer with a **virtualized, restricted filesystem view** — it could not
see `%LOCALAPPDATA%\VirtualBox` at all (`Test-Path` returned `$false` even
though the directory demonstrably existed and was stable, confirmed with
native `cmd.exe /c dir` run correctly — the first several attempts used
`cmd.exe /c "..."` from Git Bash, which is the *separate*, previously-documented
MSYS-path-mangling trap that silently corrupts a `/c` flag; using `cmd //c`
avoided that and confirmed the file was real and stable all along). Classic
`powershell.exe` (Windows PowerShell 5.1, not MSIX-packaged) saw the same path
immediately, and running this harness through that instead
(`powershell.exe -File tools\capture\Invoke-CaptureHarness.ps1 ...`) is what
actually reached the real installed application. The harness's own
`Import-CaptureNativeType.ps1` cascade (see above) already handled the
`Add-Type` differences between these two PowerShell builds without any change
needed — only the choice of *which* `powershell`/`pwsh` binary to invoke
mattered here, not the harness's own code.

Suggested articles: [`RuntimeCapture.md`](RuntimeCapture.md) for the capture
evidence contract this tool implements, [`CaptureMatrix.md`](CaptureMatrix.md)
for the full list of surfaces still needing a real capture, and
[`../../CHANGELOG.md`](../../CHANGELOG.md) for the current state of blocker A
(no verified installer/release yet).
