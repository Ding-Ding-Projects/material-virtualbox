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

The harness is split across files under [`tools/capture/`](../../tools/capture/):

| File | Role |
| --- | --- |
| `VbxCaptureNative.cs` | The Win32 P/Invoke surface: `CreateDesktop`, `EnumDesktopWindows`, `PrintWindow`, the pixel-grid uniformity check, and (added for driving the application — see below) `PostMessage`-based click/key delivery, `OpenDesktop`/`SetThreadDesktop`, and `SetWindowPos`-based resize. Compiled at runtime via `Add-Type`. |
| `Import-CaptureNativeType.ps1` | A small shared loader every other script dot-sources. It exists solely because `Add-Type`'s assembly-reference resolution for `System.Drawing` turned out to differ across PowerShell/.NET hosts — see "A portability wrinkle" below. |
| `Invoke-CaptureHarness.ps1` | The single-shot harness: launch, wait for the first qualifying window, capture, validate, clean up, exit. Correct for one static surface; the target is always killed immediately afterward. |
| `Start-DrivenSession.ps1` | Launches a target on an off-screen desktop and **keeps it running** (blocking in a bounded wait loop) so a separate driver can send it input and capture it across several surfaces in one live session, rather than one launch-and-die per surface. Writes session state (desktop name, PID, window handle/class/title/size) to a caller-supplied JSON file once the window is ready. |
| `Invoke-DrivenAction.ps1` | Sends one input action (`click`, `keychord`, `keypress`, `text`, `resize`) or takes one labeled `capture` against the window recorded by `Start-DrivenSession.ps1`'s state file. Each invocation is a fresh, stateless process — see "Driving the application with background input" below for why that needs `AttachToDesktop`. |
| `Stop-DrivenSession.ps1` | Terminates the process `Start-DrivenSession.ps1` launched; that script's own loop then notices the process exited and closes the desktop handle itself, in its own `finally` block — never a force-kill of the session-holder script, which would skip its cleanup entirely. |
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

## Driving the application with background input

`Invoke-CaptureHarness.ps1` alone can only photograph whatever a target
happens to render on its own at launch. Reaching any *other* surface — a
different nav-rail destination, a resized layout, a menu, a dialog — needs
real input delivered to the window, without ever touching the operator's
visible desktop, cursor, keyboard focus, or foreground window. Three more
scripts extend the harness for exactly this, all built on the same
`VbxCaptureNative.cs`/`Import-CaptureNativeType.ps1` foundation:
`Start-DrivenSession.ps1` launches a target and keeps it alive;
`Invoke-DrivenAction.ps1` sends one input action or takes one labeled capture
against that running session; `Stop-DrivenSession.ps1` ends it. See each
script's own comment-based help for full parameter documentation.

```powershell
# 1. Start a session and keep it running in the background.
Start-Job { powershell.exe -NoProfile -File tools\capture\Start-DrivenSession.ps1 `
    -TargetPath 'C:\Program Files\VirtualBox\VirtualBox.exe' `
    -StateFile 'C:\path\to\scratch\session-state.json' -SessionSeconds 1800 }

# 2. Once the state file reports status "ready", drive it.
powershell.exe -NoProfile -File tools\capture\Invoke-DrivenAction.ps1 `
    -StateFile 'C:\path\to\scratch\session-state.json' `
    -Action click -X 57 -Y 158 -PostActionWaitMs 2000

powershell.exe -NoProfile -File tools\capture\Invoke-DrivenAction.ps1 `
    -StateFile 'C:\path\to\scratch\session-state.json' -Action capture `
    -OutputDir 'doc\md3\captures' -SurfaceLabel 'manager-home' `
    -Commit '<sha>' -QtVersion '6.8.3' -DisplayScale '100%' -LanguageMode 'English'

# 3. Clean up.
powershell.exe -NoProfile -File tools\capture\Stop-DrivenSession.ps1 `
    -StateFile 'C:\path\to\scratch\session-state.json'
```

### Why input delivery needs `PostMessage`, not `SendInput`

`SendInput`/`mouse_event`/`keybd_event` inject at the OS level against
whichever desktop is currently the **input desktop** — i.e. the one the real
user is looking at. Using them here would either do nothing (if the
off-screen desktop isn't the input desktop, which it deliberately never is)
or, worse, momentarily switch the input desktop and touch the operator's real
session. `PostMessage`/`SendMessage` instead target a **specific `HWND`'s
message queue** directly, regardless of which desktop is currently active or
visible, which is what keeps this fully off-screen. `ClickWindowPoint` and
`SendKeyChord`/`SendKeyPress`/`SendText` in `VbxCaptureNative.cs` are built on
`PostMessage` for exactly this reason.

### A window handle only resolves correctly for a thread attached to its desktop

`Invoke-DrivenAction.ps1` runs as a **brand-new process on every invocation**
(see its own header for why: it lets several separate driving steps share one
long-lived application session without one PowerShell process having to stay
interactively scriptable for the whole thing). The first attempt to act on a
window handle discovered by a *different* process (`Start-DrivenSession.ps1`)
failed outright: `GetClassName`/`GetWindowText` returned empty and
`GetWindowRect` returned `false` with a nonsense `Win32Error=203` — which
turned out to be a **red herring**, because the original `GetWindowRect`
P/Invoke signature lacked `SetLastError = true`, so `Marshal.GetLastWin32Error()`
was reading whatever error code some unrelated intervening call had last set,
not the real one. Both `GetWindowRect` and `GetClientRect` now carry
`SetLastError = true`.

Fixing that alone didn't resolve the handle, though. The actual fix is
`AttachToDesktop` (in `VbxCaptureNative.cs`): call `OpenDesktop` (by the short
desktop name, not the `WinSta0\...`-prefixed full name `Start-DrivenSession.ps1`
records) to get **this process its own open handle** to the off-screen
desktop before touching any `HWND` on it. Once that handle is open,
`GetWindowRect`/`GetClassName`/`PrintWindow`/`PostMessage` all resolve the
handle correctly from the new process. `SetThreadDesktop` — which would go
one step further and actually attach the calling *thread* to that desktop —
reliably **fails** with `ERROR_BUSY` (170) here, because PowerShell's own
hosting process already owns windows/hooks on its current desktop before this
script ever runs (`SetThreadDesktop` requires the calling thread own nothing
on its current desktop). That turned out not to matter: holding the
`OpenDesktop` handle alone was sufficient for every capability this harness
needs, so `AttachToDesktop` reports a `SetThreadDesktop` failure as a
non-fatal warning and proceeds with the `OpenDesktop` handle regardless.

### Reliable clicking needs a real movement path, not a single teleport

An initial `ClickWindowPoint` that posted one `WM_MOUSEMOVE` straight to the
target coordinate, immediately followed by `WM_LBUTTONDOWN`/`WM_LBUTTONUP`,
was **unreliable**: some clicks on clearly-enabled nav-rail rows had no
effect at all, repeatably, even after long waits and even after confirming
(by resampling `GetWindowRect`/`GetClientRect`, both consistently `(0,0)`
offset and matching the capture's own pixel dimensions) that the coordinate
math itself was correct. The fix was to make the synthetic input look more
like real input: `ClickWindowPoint` now walks several intermediate
`WM_MOUSEMOVE` steps from a neutral point toward the target, settles on the
final position, presses down, **holds the press for a nontrivial duration**
(Material ripple/press-state visuals appear to distinguish a real press from
an instantaneous down+up pair), moves again while the button is logically
down, then releases. This matches the behavior described in this project's
own shared operating notes about headless Qt input being unreliable when
delivered too mechanically.

### A real, reproducible finding: three nav-rail items are disabled here

Even with the improved click sequence, repeated clicks on the **Machines**,
**Media**, and **Network** nav-rail rows never had any visible effect, while
**Home**, **Extensions**, **Cloud**, and **Resources** all responded
(sometimes after a retry — rail clicks remained somewhat flaky even for
enabled items, most likely ordinary input-timing sensitivity rather than
anything more interesting). Eyeballing screenshots is not evidence, so this
was checked properly: a small probe script
(`sample-rail-brightness.ps1`, not committed — a throwaway diagnostic, kept
here only as a description of the method) samples the actual rendered pixel
luminance under each rail label. The result was decisive and consistent
across every capture checked:

```
Home         maxLuminance=228.0   (selected)
Machines     maxLuminance=87.2
Extensions   maxLuminance=199.0
Media        maxLuminance=87.2
Network      maxLuminance=87.2
Cloud        maxLuminance=199.0
Resources    maxLuminance=199.0
```

Machines, Media, and Network share one dim value; every other item shares a
much brighter one. This is a genuine Qt *disabled* state, not a coordinate
mistake in this lane's driving — a disabled Qt widget simply does not
respond to mouse input, which matches exactly what was observed. It is
consistent with the notification centre's own live "Can't enumerate USB
devices ..." diagnostic: Machines/Media/Network plausibly share a host-device
or `IHost`/COM enumeration path that is degraded in this environment, while
Extensions/Cloud/Resources (Extension Pack list, cloud profiles, and local
host CPU/RAM/disk stats respectively) do not depend on it. See
[`CaptureMatrix.md`](CaptureMatrix.md) rows 2, 28, and 29 for where this
lands as evidence.

### DPI mismatch, noted but not chased down

This lane's own driven captures consistently show the Manager window at
1024×975px, while a manual interactive verification of the same build (see
the top of this document) found 1280×1219px. `1280 / 1024 = 1.25` and
`1219 / 975 ≈ 1.25` — the same ratio on both axes. A plausible explanation is
that an off-screen desktop created via `CreateDesktop` reports a fixed 96 DPI
/ 100% scale regardless of the primary monitor's actual configured scale
(commonly 125% on many Windows installs), so a DPI-unaware or
system-DPI-aware app renders smaller there than it would on a normal
interactive desktop. This was **not** chased down further this pass — it is
recorded here so a future high-DPI capture (row 34 in `CaptureMatrix.md`)
starts from this observation instead of rediscovering it.

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

### Driving the application: what it has actually reached

Using `Start-DrivenSession.ps1` / `Invoke-DrivenAction.ps1` against the
NSIS-installed `C:\Program Files\VirtualBox\VirtualBox.exe`, this harness
extension went beyond the cold-launch Home screen for the first time:
switched the Manager's active nav-rail destination (reaching the Extensions
and Resources/VM Activity Overview tools, each captured and validated
non-uniform), resized the live window in place via `SetWindowPos` (reaching
the compact/collapsed nav-rail layout below the 1000 logical px threshold),
and opened/closed tabs via the tab strip. It also **conclusively
demonstrated** — not merely observed — that three nav-rail destinations
(Machines, Media, Network) are disabled in this environment, via
programmatic label-luminance sampling rather than a guess from a screenshot.
Attempts at `Ctrl+Shift+F` (command palette) and `Ctrl+G`/hamburger-click
(Global Preferences) did not reach those surfaces in this pass; see
`CaptureMatrix.md` rows 6 and 16 for exactly what was tried.

Suggested articles: [`RuntimeCapture.md`](RuntimeCapture.md) for the capture
evidence contract this tool implements, [`CaptureMatrix.md`](CaptureMatrix.md)
for the full list of surfaces still needing a real capture, and
[`../../CHANGELOG.md`](../../CHANGELOG.md) for the current state of blocker A
(no verified installer/release yet).
