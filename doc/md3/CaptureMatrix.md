# Capture matrix

This is the enumerated tracking table for the real screenshot evidence the
release gate requires. [`RuntimeCapture.md`](RuntimeCapture.md) is the
*contract* — what counts as a genuine capture and how to take one.
This document is the *list* — every surface and state that must be captured,
and whether it has been yet.

**Current status: row 1 below is captured; every other row remains
`Not captured`.** No row here may be marked captured from a mock, a design
thumbnail, a prototype HTML preview, an image from an unrelated build, or a
hand-edited image. A row stays `Not captured` until a real image exists from
the exact built artifact, committed to this repository or otherwise
reproducibly retrievable, with its commit, target, Qt version, display scale,
and language mode recorded beside it, exactly as `RuntimeCapture.md` requires.

### First real capture, and a discrepancy it surfaces

This repository previously had **no capture tool of any kind** -- every row
below existed only as an entry in this table. [`CaptureHarness.md`](CaptureHarness.md)
documents the harness now built (`tools/capture/Invoke-CaptureHarness.ps1`):
a real off-screen-desktop PrintWindow capture tool, with its own self-test
proving its black-frame detector actually rejects a blank capture instead of
trusting `PrintWindow`'s return value.

Proving that harness end to end against `VirtualBox.exe` at
`%LOCALAPPDATA%\VirtualBox\app-7.2.97\VirtualBox.exe` produced row 1 below --
and it is **not** the `REGDB_E_CLASSNOTREG` failure this document's blocker B
and [`RuntimeCapture.md`](RuntimeCapture.md) describe. The manager shell
rendered fully and functionally: navigation rail, title bar, search, the Home
destination's "Get started with VirtualBox" content, a live notification
reading "Can't enumerate USB ..." (a genuine runtime diagnostic -- not
something a static mock would produce), and a first-run "Please choose
Experience Mode!" prompt. No COM/SDS error appeared anywhere in the captured
window.

This does not mean blocker B is resolved for every row, and this capture pass
does not claim that: it reflects one build, at one moment, in one environment,
captured by one lane while a separate installer lane was independently and
actively working on this exact machine's `VirtualBox` install directory (see
the `-Notes` field on the row 1 capture below, and
[`CaptureHarness.md`](CaptureHarness.md)'s "portability wrinkle" section for
the environment quirks hit along the way). It is reported here plainly because
the evidence contradicts the blocker table's current wording for at least this
one build/environment, and a capture matrix that hid that would be exactly the
kind of silent gap this document exists to prevent. Rows 2 and 4-39 remain
`Not captured` on their own honest merits -- nobody has captured them yet --
not because blocker B is assumed to still apply to them.

## Why every row is blocked, and by what

Two independent blockers currently prevent every row below from being closed.
Closing one does not close the other.

| Blocker | Affects | Detail |
| --- | --- | --- |
| **A. No verified installer or release exists yet.** | Rows 40–45 (Installer and update surfaces) | `gh release list` returns no releases for this repository as of this audit. The **Windows package and release** workflow is currently red — see [`CHANGELOG.md`](../../CHANGELOG.md#known-issue--the-packaging-step-now-fails-later-on-something-new) for the exact current failure (`STATUS_STACK_BUFFER_OVERRUN` in the packaging step's own `tstVMStructSize`/`tstAsmStructs` self-check, [run 31731859854](https://github.com/Ding-Ding-Projects/material-virtualbox/actions/runs/31731859854)). There is nothing installable to photograph. |
| **B. The development build's COM/SDS classes are not registered in this environment.** | Rows 1–39 (every manager, settings, wizard, tool, notification, and runtime surface) | Documented in [`RuntimeCapture.md`](RuntimeCapture.md): the built `VirtualBox.exe` reaches the COM boundary and fails with `REGDB_E_CLASSNOTREG` because no `VBoxSDS` Windows service is registered for this checkout. System-wide registration requires a separately authorized administrator operation that has not been granted. The manager shell itself cannot currently be reached to photograph. |

A prior session's genuine `REGDB_E_CLASSNOTREG` failure capture is described
in `README.md` as "retained in the session evidence" from an earlier run.
This audit did not find that image file committed anywhere in this
repository (a repository-wide search for capture-shaped `.png`/`.jpg` files
turned up only pre-existing upstream documentation diagrams and vendored
third-party assets, none of which are application captures), so it is not
counted as evidence here and is not linked from any row below.

**Update:** row 1 below is now captured, and the failure state that capture
would have documented did not reproduce — see "First real capture, and a
discrepancy it surfaces" above. If the `REGDB_E_CLASSNOTREG` capture described
above still exists outside this checkout, it remains worth committing as
historical evidence of blocker B's *previous* effect, labelled honestly with
the environment/build/date it came from, since row 1's current capture does
not prove blocker B never applies -- only that it did not apply to the one
build and environment this harness was proven against.

## Manager

| # | Surface | State / variant | Status | Blocker |
| --- | --- | --- | --- | --- |
| 1 | Manager shell | Cold launch, no VMs registered (empty state) | **Captured**: [`captures/manager-shell--01--Qt683QWindowIcon--Material-Virtual-Machine-Manager.png`](captures/manager-shell--01--Qt683QWindowIcon--Material-Virtual-Machine-Manager.png) — real `PrintWindow` capture via `tools/capture/Invoke-CaptureHarness.ps1` on a named off-screen desktop, validated non-uniform (18/144 distinct sampled grid colors). Target: `%LOCALAPPDATA%\VirtualBox\app-7.2.97\VirtualBox.exe`. Window: class `Qt683QWindowIcon`, title "Material Virtual Machine Manager", 1024×975px. Commit: this worktree's tip at capture time, `cb9f573030e` (the exact source commit of *this specific installed binary* is not independently verified by this lane — see the note below and the manifest's `notes` field). Qt: 6.8.3 (from the class name). Display scale: 100%. Language mode: English. Profile state: default user profile, no VMs registered. Full provenance: [`captures/capture-manifest.json`](captures/capture-manifest.json). | — |
| 2 | Manager shell | Populated chooser with registered VMs | Not captured | B |
| 3 | Manager shell | `REGDB_E_CLASSNOTREG` failure dialog | Not captured in this repository | B was previously described as producing this on every launch; it did **not** reproduce when row 1 was captured (see "First real capture, and a discrepancy it surfaces" above) — the manager shell rendered fully instead. Still listed under blocker B pending re-verification of whether/when this failure state still occurs. |
| 4 | Navigation rail | Expanded, ≥1000 logical px | Not captured | B |
| 5 | Navigation rail | Compact/searchable, <1000 logical px | Not captured | B |
| 6 | Command palette | `Ctrl+Shift+F` open, category-grouped results | Not captured | B |
| 7 | Notification centre | Empty history | Not captured | B |
| 8 | Notification centre | Populated history, filtered/selected rows | Not captured | B |
| 9 | Notification centre | Clear-history two-key confirmation gate | Not captured | B |
| 10 | Tab strip | Single tab | Not captured | B |
| 11 | Tab strip | Multiple tabs, overflow menu open | Not captured | B |
| 12 | Tab strip | Tab manager / group picker (`Ctrl+Shift+T`) | Not captured | B |
| 13 | Local history browser | `Ctrl+H`, action/date filter applied | Not captured | B |
| 14 | Appearance editor | Per-element panel, `Shift`+right-click entry | Not captured | B |
| 15 | Title bar | Frameless header, window controls, snap layout | Not captured | B |

## Settings

| # | Surface | State / variant | Status | Blocker |
| --- | --- | --- | --- | --- |
| 16 | Global Preferences | Default landing page | Not captured | B |
| 17 | Global Preferences | Search active, plain-text mode | Not captured | B |
| 18 | Global Preferences | Search active, regex builder open | Not captured | B |
| 19 | Machine Settings | Default landing page | Not captured | B |
| 20 | Machine Settings | Validation error state | Not captured | B |
| 21 | Settings shell | Narrow-width layout | Not captured | B |

## Wizards

| # | Surface | State / variant | Status | Blocker |
| --- | --- | --- | --- | --- |
| 22 | New VM wizard | First page | Not captured | B |
| 23 | New VM wizard | Validation error | Not captured | B |
| 24 | New VM wizard | Summary / completion page | Not captured | B |
| 25 | New VM wizard | Cancellation confirmation | Not captured | B |
| 26 | Import/Export/Clone wizard | Compact rail collapse, <720 logical px | Not captured | B |

## Manager tools

| # | Surface | State / variant | Status | Blocker |
| --- | --- | --- | --- | --- |
| 27 | Extensions | List + Material search/appearance card | Not captured | B |
| 28 | Media | List + Material search/appearance card | Not captured | B |
| 29 | Network | List + Material search/appearance card | Not captured | B |
| 30 | Cloud | List + Material search/appearance card | Not captured | B |
| 31 | VM Activity Overview | List + Material search/appearance card | Not captured | B |

## Runtime window

| # | Surface | State / variant | Status | Blocker |
| --- | --- | --- | --- | --- |
| 32 | Runtime window | Normal windowed | Not captured | B |
| 33 | Runtime window | Fullscreen | Not captured | B |
| 34 | Runtime window | Scaled / high-DPI (150%, 200%) | Not captured | B |
| 35 | Runtime window | Narrow window | Not captured | B |

## Cross-cutting states (apply across rows 1–35 where the surface exists)

| # | Surface | State / variant | Status | Blocker |
| --- | --- | --- | --- | --- |
| 36 | Any surface above | Light theme | Not captured | B |
| 37 | Any surface above | Dark/contrast theme | Not captured | B |
| 38 | Any surface above | Bilingual (English + Cantonese) language mode | Not captured | B |
| 39 | Any surface above | Visible keyboard-focus state | Not captured | B |

## Installer and update surfaces

| # | Surface | State / variant | Status | Blocker |
| --- | --- | --- | --- | --- |
| 40 | Squirrel installer | `Setup.exe` first-run screen | Not captured | A |
| 41 | Squirrel installer | Install progress | Not captured | A |
| 42 | Squirrel installer | Completion screen | Not captured | A |
| 43 | Unsigned-publisher warning | OS unknown-publisher / SmartScreen dialog (expected and disclosed, not hidden, per the permanent no-signing policy) | Not captured | A |
| 44 | Auto-update | Non-blocking "ready to restart" banner | Not captured | A |
| 45 | Auto-update | Offline / invalid-feed fallback state | Not captured | A |

## How a row gets closed

1. Clear the row's blocker: for blocker A, ship a verified installer from a
   clean-checkout run of `build-installer.bat` or the CI packaging job once
   it goes green; for blocker B, obtain an authorized COM/SDS registration
   path for the capture host, or another sanctioned runtime host that does
   not require one.
2. Capture the exact built artifact per the `RuntimeCapture.md` contract:
   the cheap headless route, the real window, the real commit.
3. Commit the image, update this row's Status to a link to the image plus
   the commit/target/Qt-version/scale/language-mode line the contract
   requires, and update the README's "Runtime screenshots" section so it is
   no longer described as empty.
4. Never mark a row captured to make this table look more complete than the
   evidence behind it. A guard or reviewer that trusts this table's own
   "Not captured" wording is exactly the auditability this document exists
   to provide — do not spend it.

Suggested articles: [`RuntimeCapture.md`](RuntimeCapture.md) for the capture
contract itself, [`CaptureHarness.md`](CaptureHarness.md) for the tool that
now implements it, [`DesignCoverage.md`](DesignCoverage.md) for the underlying
69-entry implementation ledger these surfaces come from, and
[`../../CHANGELOG.md`](../../CHANGELOG.md) for why blocker A is currently in
effect.
