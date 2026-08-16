# Capture matrix

This is the enumerated tracking table for the real screenshot evidence the
release gate requires. [`RuntimeCapture.md`](RuntimeCapture.md) is the
*contract* — what counts as a genuine capture and how to take one.
This document is the *list* — every surface and state that must be captured,
and whether it has been yet.

**Current status: 8 of 47 rows are captured — rows 1, 5, 6, 16, 22, 24, 27, and 31 — and the other
39 remain `Not captured`.** Counted mechanically on 2026-08-16, not by eye:

```
$ grep -cE '^\| [0-9]+ \|' doc/md3/CaptureMatrix.md
47
$ grep -cE '^\| [0-9]+ \|.*\*\*Captured\*\*' doc/md3/CaptureMatrix.md
8
$ grep -cE '^\| [0-9]+ \|.*Not captured' doc/md3/CaptureMatrix.md
39
$ find doc/md3/captures -name '*.png' | wc -l
19
```

**`tools/md3/count-inventory-rows.py` does not count this document.** It is named here only to stop
a reader reaching for it: it parses `doc/md3/CompletenessInventory.md`, a different file with a
different unit of measure (feature-per-surface rows, not capture rows). Its verbatim output on
2026-08-16 is:

```
Source: doc\md3\CompletenessInventory.md
Sections with a per-surface table: 16
Sections counted as one unit:      9

| Status | Count |
| --- | ---: |
| Implemented | 33 |
| Partial | 18 |
| Not implemented | 39 |
| N/A (justified) | 1 |
| **Total rows** | **91** |
```

There is no committed script that counts *this* table; the three `grep` commands above are the
whole method, and they are printed so anyone can re-run them instead of trusting the sentence.

No row here may be marked captured from a mock, a design
thumbnail, a prototype HTML preview, an image from an unrelated build, or a
hand-edited image. A row stays `Not captured` until a real image exists from
the exact built artifact, committed to this repository or otherwise
reproducibly retrievable, with its commit, target, Qt version, display scale,
and language mode recorded beside it, exactly as `RuntimeCapture.md` requires.

Rows 5, 27, and 31 were closed by driving the live application with real
background input (mouse clicks and a window resize delivered directly to its
off-screen HWND — see [`CaptureHarness.md`](CaptureHarness.md)'s "Driving the
application with background input" section) rather than only photographing
whatever the app happened to render at cold launch. Several more rows were
attempted and honestly left `Not captured` with a specific reason recorded
per row — including a real, reproducible finding that the **Machines**,
**Media**, and **Network** nav-rail items are disabled in this environment
(confirmed by pixel-luminance sampling of their labels, not just by clicks
having no visible effect), which is new information about the shape of
blocker B beyond "the manager shell doesn't render at all."

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
kind of silent gap this document exists to prevent.

**Corrected 2026-08-16.** This paragraph used to end "Rows 2 and 4-39 remain `Not captured` on
their own honest merits." That was wrong on its face, and wrong against this document's own rows:
rows 5, 6, 16, 22, 24, 27 and 31 all sit inside 4–39 and all read **Captured**. The accurate
sentence is: **every row except 1, 5, 6, 16, 22, 24, 27 and 31 remains `Not captured`, on its own
honest merits — nobody has captured it yet — not because blocker B is assumed to still apply.**

## Why every row is blocked, and by what

Blocker A **is cleared as of 2026-08-16** and is rewritten below rather than left standing.
Blocker B is unchanged for the rows it affects. A third ceiling, C, was always in force for the
runtime-window rows and is now named explicitly instead of being left inside row 32's prose.

| Blocker | Affects | Detail |
| --- | --- | --- |
| ~~**A. No verified installer or release exists yet.**~~ **CLEARED 2026-08-16.** | Formerly rows 40–45; now **rows 40–43 are unblocked and simply unattempted**, and rows 44–45 move to blocker D | The original wording — "`gh release list` returns no releases for this repository as of this audit. The **Windows package and release** workflow is currently red … There is nothing installable to photograph" — is false on every clause today. `gh release list --repo Ding-Ding-Projects/material-virtualbox` returns nine `v7.2.97-ci.*` releases (ci.96–ci.101, ci.106–ci.108; ci.102–ci.105 were never created). The Windows workflow is **success** at `main`'s tip `fe321a4fd6f6bfc7c2fbd7e0704e3f72d0eb2eee` ([run 31851996367](https://github.com/Ding-Ding-Projects/material-virtualbox/actions/runs/31851996367), 1h26m24s). `v7.2.97-ci.108` is non-draft and carries `VirtualBox-7.2.97-Setup.exe` at **106,963,266 bytes**, SHA-256 `8a6e1e94bdd73c3a9c526b7b7e074521f06a2562b9f9020ea1a806f15fcda627`, plus `SHA256SUMS.txt` and a dim-sum photo. There is now something installable to photograph. **Unblocked is not captured:** rows 40–43 stay `Not captured` until somebody runs that installer on a Windows host and takes the images. |
| **B. The development build's COM/SDS classes are not registered in this environment.** | Rows 1–39 (every manager, settings, wizard, tool, notification, and runtime surface) | Documented in [`RuntimeCapture.md`](RuntimeCapture.md): the built `VirtualBox.exe` reaches the COM boundary and fails with `REGDB_E_CLASSNOTREG` because no `VBoxSDS` Windows service is registered for this checkout. System-wide registration requires a separately authorized administrator operation that has not been granted. The manager shell itself cannot currently be reached to photograph. (Row 1's capture shows this did not reproduce for one NSIS-installed build — see the discrepancy section above — so B is real but not universal.) |
| **C. No virtual machine can start, permanently.** | Rows 32–35 (every runtime-window surface) | The host hypervisor driver `VBoxSup.sys` ships **unsigned**, 64-bit Windows refuses to load an unsigned kernel driver, and **code signing is permanently prohibited for this project**. No change in this repository can move this. Only the machine's owner choosing to permit unsigned drivers would, and that is a decision with an owner outside this codebase. Rows 32–35 therefore cannot be closed from a normal Windows host at all, and should not be re-labelled "unattempted" — they are ceilinged. |
| **D. No auto-updater is implemented.** | Rows 44–45 | These two rows were parked under blocker A ("no installer exists"), which was never their real blocker and is now cleared anyway. A search of `src/VBox/Frontends/VirtualBox/src/md3/` for updater sources on 2026-08-16 matched only `UIMd3Changelog.{h,cpp}`; there is no update-feed client, no ready-to-restart banner, and no offline/invalid-feed fallback to photograph, in this or any shipped installer. `LocalGates.md` records the same absence from the gate side. |

A prior session's genuine `REGDB_E_CLASSNOTREG` failure capture was once
described in `README.md` as "retained in the session evidence" from an earlier
run. That sentence has since been withdrawn from `README.md`, which now agrees
with this document. This audit did not find that image file committed anywhere
in this repository (a repository-wide search for capture-shaped `.png`/`.jpg` files
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
| 2 | Manager shell | Populated chooser with registered VMs | Not captured | This lane is explicitly prohibited from creating/modifying/deleting a VM, so this row cannot be closed without violating that boundary. Additionally, the **Machines** nav-rail item itself was found DISABLED in this environment (see the driven-input finding below row 31) — even a read-only visit to the Machines list did not respond to input. |
| 3 | Manager shell | `REGDB_E_CLASSNOTREG` failure dialog | Not captured in this repository | B was previously described as producing this on every launch; it did **not** reproduce when row 1 was captured (see "First real capture, and a discrepancy it surfaces" above) — the manager shell rendered fully instead. Still listed under blocker B pending re-verification of whether/when this failure state still occurs. |
| 4 | Navigation rail | Expanded, ≥1000 logical px | Not captured | Row 1's own image already shows the expanded rail at 1024px width, but that row is scoped to "cold launch" specifically; this row is left open for a dedicated capture rather than silently reusing row 1's image under a second row. |
| 5 | Navigation rail | Compact/searchable, <1000 logical px | **Captured**: [`captures/manager-nav-rail-compact--01--Qt683QWindowIcon--Material-Virtual-Machine-Manager.png`](captures/manager-nav-rail-compact--01--Qt683QWindowIcon--Material-Virtual-Machine-Manager.png) — driven capture: the window was resized in place to 640×800 via `SetWindowPos` (`tools/capture/Invoke-DrivenAction.ps1 -Action resize`), a normal window-management API targeted at the specific off-screen HWND, never `SendInput`. Below the <1000 logical px threshold the left rail collapses to a single hamburger-style toggle and the top bar's search pill/New/Open buttons become compact icon-only affordances (search glyph, notification bell with an unread-count badge). Validated non-uniform (29/144 distinct sampled colors). Target: `C:\Program Files\VirtualBox\VirtualBox.exe` (NSIS-installed). Commit: `dd561e36499`. Qt: 6.8.3. Display scale: 100%. Language mode: English. Full provenance: [`captures/capture-manifest.json`](captures/capture-manifest.json). | — |
| 6 | Command palette | `Ctrl+Shift+F` open, category-grouped results | **Captured**: [`captures/usability-probe-20260814/05-command-palette-opened-via-search-icon-click.png`](captures/usability-probe-20260814/05-command-palette-opened-via-search-icon-click.png). `Ctrl+Shift+F` itself still produced no effect in a follow-up pass (consistent with the chord-reliability caution below), but clicking the header's own search-pill icon opens the same palette reliably — a real, fully populated, keyboard-navigable `QDialog` (Manager category: Open global preferences, Create a new virtual machine, Open virtual media manager, Import an appliance, Open local history, Home, …). See [`UsabilityProbe.md`](UsabilityProbe.md) §4 for the icon-vs-overflowing-label distinction that matters for hitting it reliably. | — |
| 7 | Notification centre | Empty history | Not captured | The notification centre is showing a live, populated "Can't enumerate USB devices ..." diagnostic in every capture from this session; reaching an empty-history state was not attempted since it would mean deliberately dismissing that notification, and this lane prioritized non-destructive, easily-repeatable captures. |
| 8 | Notification centre | Populated history, filtered/selected rows | Not captured | A populated notification (visible, undismissed) is present in every capture in this session, including row 1's — but this row specifically wants a **filtered/selected** state, which requires interacting with the notification centre's own filter/selection controls; that interaction was not attempted this pass. |
| 9 | Notification centre | Clear-history two-key confirmation gate | Not captured | B |
| 10 | Tab strip | Single tab | Not captured | Reachable in practice (the driven session returned to a single "Home" tab several times after tabs were closed) but not captured to a dedicated evidence file this pass; row 1's image already shows a single tab, but is scoped under "Manager shell / Cold launch" rather than this row. |
| 11 | Tab strip | Multiple tabs, overflow menu open | Not captured | The driven session had 2-3 tabs open simultaneously (Home + Extensions + Resources) at points during this pass, but 1024px width was never narrow enough to force the overflow menu, so this row's specific "overflow menu open" state was not reached. |
| 12 | Tab strip | Tab manager / group picker (`Ctrl+Shift+T`) | Not captured | Not attempted this pass. |
| 13 | Local history browser | `Ctrl+H`, action/date filter applied | Not captured | Not attempted this pass. |
| 14 | Appearance editor | Per-element panel, `Shift`+right-click entry | Not captured | Not attempted this pass; this lane's driven-input harness added plain left-click and key-chord delivery but not a modifier-click (Shift+right-click) primitive. |
| 15 | Title bar | Frameless header, window controls, snap layout | Not captured | The custom frameless Material header (no OS-drawn caption/border; hamburger, app icon+title, search, notification bell) is visible in every capture from this session, but window controls (minimize/maximize/close) were not visually located anywhere within the captured window bounds at any tested size, and snap-layout behavior was not tested. Worth a dedicated follow-up: either the controls render off the visible/captured area under some layout condition, or this build genuinely omits them from the frameless header. Not marking this row captured on partial evidence per this document's own rule. |

## Settings

| # | Surface | State / variant | Status | Blocker |
| --- | --- | --- | --- | --- |
| 16 | Global Preferences | Default landing page | **Captured**: [`captures/usability-probe-20260814/14-global-preferences-dialog-confirmed-working.png`](captures/usability-probe-20260814/14-global-preferences-dialog-confirmed-working.png). `Ctrl+G` did not work when this capture was taken, and the hamburger→File→Preferences route was not reached in a later pass either (see `UsabilityProbe.md` §3 for why — likely a source/binary mismatch, not a live bug). The command palette's `Open global preferences` entry, activated the same keyboard way as row 22, opens a real, fully functional `VirtualBox - Preferences` dialog: search box, Basic/Expert toggle, Appearance tab, and a populated General page. **Updated 2026-08-16:** this cell previously called `Ctrl+G` a "known regression, see `WiringAudit.md` §4a". A source re-read at `fe321a4` found the defect §4a described **absent from the source at all four sites it names** — see §4a's dated re-read block. That is not a runtime re-test; nothing was built, installed or keyboard-tested this pass, so the honest statement is that `Ctrl+G` failed *at the build this image came from* and its current behaviour is unknown. `v7.2.97-ci.108` is the first shipped installer containing both fixes, so a re-test is now possible. | — |
| 17 | Global Preferences | Search active, plain-text mode | Not captured | Blocked behind row 16. |
| 18 | Global Preferences | Search active, regex builder open | Not captured | Blocked behind row 16. |
| 19 | Machine Settings | Default landing page | Not captured | Requires a registered machine; this lane is prohibited from creating one. |
| 20 | Machine Settings | Validation error state | Not captured | Blocked behind row 19. |
| 21 | Settings shell | Narrow-width layout | Not captured | Blocked behind row 16 (no Settings surface was reached to resize). |

## Wizards

| # | Surface | State / variant | Status | Blocker |
| --- | --- | --- | --- | --- |
| 22 | New VM wizard | First page | **Captured**: [`captures/usability-probe-20260814/07-real-new-vm-wizard-reached-via-palette.png`](captures/usability-probe-20260814/07-real-new-vm-wizard-reached-via-palette.png). The Home-link and toolbar "New" routes each fail (differently — see [`UsabilityProbe.md`](UsabilityProbe.md) §2), but the command palette's `Create a new virtual machine` entry, activated by keyboard (type-to-filter → `Down` → `Return`), reliably opens the real `UIWizardNewVM`. Driven all the way through to a created VM; see `UsabilityProbe.md` for the full account and the row-31/row-2 update below. | — |
| 23 | New VM wizard | Validation error | Not captured | Not attempted this pass; blocker A/B no longer apply, this is simply unattempted. |
| 24 | New VM wizard | Summary / completion page | **Captured**: [`captures/usability-probe-20260814/09-wizard-summary-step3.png`](captures/usability-probe-20260814/09-wizard-summary-step3.png) — `Step 3 of 3`, correctly listing the configured VM name and folder before Finish. | — |
| 25 | New VM wizard | Cancellation confirmation | Not captured | Blocked behind row 22. |
| 26 | Import/Export/Clone wizard | Compact rail collapse, <720 logical px | Not captured | Not attempted this pass. |

## Manager tools

| # | Surface | State / variant | Status | Blocker |
| --- | --- | --- | --- | --- |
| 27 | Extensions | List + Material search/appearance card | **Captured**: [`captures/manager-extensions-tool--01--Qt683QWindowIcon--Material-Virtual-Machine-Manager.png`](captures/manager-extensions-tool--01--Qt683QWindowIcon--Material-Virtual-Machine-Manager.png) — driven capture showing the Extensions tool's list chrome: a "Search Extensions records" search field and an `Active \| Name` column header, with no rows (no extension packs installed in this profile) so per-row card content is not demonstrated. Validated non-uniform (12/144 distinct sampled colors). Reached via a driven click on the nav rail. Target: `C:\Program Files\VirtualBox\VirtualBox.exe` (NSIS-installed). Commit: `dd561e36499`. Qt: 6.8.3. Display scale: 100%. Language mode: English. Full provenance: [`captures/capture-manifest.json`](captures/capture-manifest.json). | — |
| 28 | Media | List + Material search/appearance card | Not captured | **Confirmed DISABLED in this environment**, not merely unreached: the "Media" rail label was measured with the harness's own pixel-luminance sampling at 87.2, identical to "Machines" and "Network" and far below the ~199-228 measured for every enabled item (Home/Extensions/Cloud/Resources). Multiple driven clicks at several coordinates within the Media row, at different points in the session, produced zero response. Consistent with blocker B's backend/COM condition — the visible "Can't enumerate USB ..." notification suggests host-device enumeration is already known to be degraded in this environment, and Media likely depends on the same `IHost` enumeration path. |
| 29 | Network | List + Material search/appearance card | Not captured | **Confirmed DISABLED in this environment** — same luminance evidence (87.2) and same repeated no-response result as row 28. See that row's note. |
| 30 | Cloud | List + Material search/appearance card | Not captured | Attempted (driven click on the rail's Cloud row) but not reached — unlike Machines/Media/Network, Cloud's label luminance measured 199 (matching the enabled items, not the disabled ones), so this is very likely the same rail-click flakiness observed intermittently on other enabled items in this session (see `CaptureHarness.md`'s driven-input notes on `ClickWindowPoint`) rather than a disabled control. Worth a fresh attempt in a follow-up pass. |
| 31 | VM Activity Overview | List + Material search/appearance card | **Captured**: [`captures/manager-vm-activity-overview--01--Qt683QWindowIcon--Material-Virtual-Machine-Manager.png`](captures/manager-vm-activity-overview--01--Qt683QWindowIcon--Material-Virtual-Machine-Manager.png) — driven capture of the "Resources" Global Tool: a "Search Resources records" search field plus three live Material card widgets (Host CPU Load, Host RAM Usage, Host File System) with real gauge/donut charts and numeric readouts that changed value on repeated polls, confirming a genuine live render rather than a cached frame. Validated non-uniform (14/144 distinct sampled colors). Target: `C:\Program Files\VirtualBox\VirtualBox.exe` (NSIS-installed). Commit: `dd561e36499`. Qt: 6.8.3. Display scale: 100%. Language mode: English. Full provenance: [`captures/capture-manifest.json`](captures/capture-manifest.json). | — |

## Runtime window

| # | Surface | State / variant | Status | Blocker |
| --- | --- | --- | --- | --- |
| 32 | Runtime window | Normal windowed | Not captured | Requires an actually running VM, which requires the VirtualBox host kernel drivers (VBoxDrv etc.). Those are not built for this checkout, and even if they were, this is an unsigned development build — Windows would refuse to load unsigned kernel drivers, so a runtime session cannot start here regardless of VM registration. This lane is also separately prohibited from creating/starting a VM. |
| 33 | Runtime window | Fullscreen | Not captured | Blocked behind row 32. |
| 34 | Runtime window | Scaled / high-DPI (150%, 200%) | Not captured | Blocked behind row 32. Separately: this session's own captures show the installed Manager window at 1024×975px against a manually-verified 1280×1219px window from an earlier interactive session on the same build — an exact 1.25× ratio on both axes, suggestive of a DPI-scale difference between an off-screen desktop (which may report a fixed 96 DPI / 100% regardless of the primary monitor's real scale) and a normal interactive desktop. Worth a dedicated investigation before trusting any high-DPI capture taken through this harness. |
| 35 | Runtime window | Narrow window | Not captured | Blocked behind row 32. |

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
| 40 | NSIS installer (`VirtualBox-<version>-Setup.exe`, the one and only Windows installer; Squirrel was retired 2026-08-14) | Welcome/license/directory pages | Not captured | **Unblocked since 2026-08-16, simply unattempted.** Blocker A is cleared: `v7.2.97-ci.108` ships a 106,963,266-byte `VirtualBox-7.2.97-Setup.exe`. Closing this row needs only a Windows host and somebody to run it. |
| 41 | NSIS installer | Install progress (`InstFiles` page, elevated) | Not captured | Same as row 40 — unblocked, unattempted. |
| 42 | NSIS installer | Finish page, including the driver-refused summary `MessageBox` when a kernel driver was refused | Not captured | Same as row 40 — unblocked, unattempted. Note this row is the one that photographs blocker C: the installer's own honest report that an unsigned kernel driver was refused. |
| 43 | Unsigned-publisher warning | OS unknown-publisher / SmartScreen dialog (expected and disclosed, not hidden, per the permanent no-signing policy) | Not captured | Same as row 40 — unblocked, unattempted. The dialog this row wants is a *consequence* of the permanent no-signing policy, not a defect to fix. |
| 44 | Auto-update | Non-blocking "ready to restart" banner | Not captured | **D** — no auto-updater is implemented anywhere in this repository. This row was mislabelled `A` ("no installer exists") until 2026-08-16; that was never its real blocker, and shipping an installer does not close it. |
| 45 | Auto-update | Offline / invalid-feed fallback state | Not captured | **D** — same as row 44. |
| 46 | Changelog viewer (`doc/md3/Changelog.md`) | `Ctrl+Shift+L`, no filter applied (cold open) | Not captured | **Rebuild blocker lifted 2026-08-16, still unattempted.** This row said no capture was possible without a full `kmk` build. That is no longer true: `UIMd3Changelog` landed in `aa52c21c9f4d3b1ac1fa961ed9fee70c75f91cb5`, which is an ancestor of `v7.2.97-ci.101`'s target `bb63f016a5da05a9880ea957609695770004763c` and of every release since, so `v7.2.97-ci.101` and later installers **do** contain this code. What is still needed is an install of one of them and somebody to press the keys — and blocker B may still apply to reaching the manager shell on a given host. |
| 47 | Changelog viewer | Search + category + date filter applied, showing a per-row commit button ~~and the off-default-branch marker~~ | Not captured | Same as row 46, plus one part of this row is currently unphotographable and says so: **no compiled-in entry carries the off-default-branch flag any more.** The only one that did (`1e59aca27…`) was marked because `CHANGELOG.md` claimed it was not an ancestor of `main`; it is one, so the claim was retired. The mechanism still exists in `UIMd3Changelog` (`fOnDefaultBranch`, the export suffix, the visible non-color-only label) and this sub-clause becomes capturable again the moment an entry genuinely needs it. See [`Changelog.md`](Changelog.md). |

## How a row gets closed

1. Clear the row's blocker: blocker A is already cleared — download the
   installer from the latest release; for blocker B, obtain an authorized
   COM/SDS registration path for the capture host, or another sanctioned
   runtime host that does not require one; for blocker C there is no route
   at all from this repository, because no virtual machine can start while
   `VBoxSup.sys` is unsigned and signing is permanently prohibited; for
   blocker D, the feature has to be built before it can be photographed.
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
[`../../CHANGELOG.md`](../../CHANGELOG.md) for the release history that cleared
blocker A and for the packaging failure that used to cause it.
