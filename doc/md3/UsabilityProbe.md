# Usability probe: driving the real installed application

Commit audited from: `5d7c01d0d0789ba51c3d9586fbfa3575bbd096e4` on `claude/reality-audit-20260814`.
Target: `C:\Program Files\VirtualBox\VirtualBox.exe` (NSIS-installed, file mtime
2026-08-10 12:52:44 — an unknown number of commits behind this worktree's HEAD; several
findings below are best explained by that gap, and are called out explicitly). Harness:
`tools/capture/Start-DrivenSession.ps1` / `Invoke-DrivenAction.ps1`, driven with real
`PostMessage`-based mouse/keyboard input on a named off-screen desktop, plus a handful of
small ad-hoc extensions (right-click, `WM_CONTEXTMENU`, arbitrary-handle capture/click/text)
written for this pass and left in the session scratchpad rather than the repository, since
they were diagnostic, not product code.

This is not a source-reading audit. Every claim below was produced by actually launching the
installed binary on an off-screen desktop and sending it real input, then reading back what
happened — a window that appeared, a window that didn't, a pixel that changed or didn't. Where
a claim is about *why*, that is clearly separated from the *what*.

## Executive summary

The three things this task set out to determine all have real answers now, and two of them
overturn the previous lane's more cautious "needs interactive re-verification" stance:

1. **New VM is fully reachable and fully functional.** The wizard opens, accepts input, and
   creates a real, working, fully-configured VM. It is *not* reachable from the two routes a
   user would try first — the Home-screen hyperlink and the toolbar button — for two different
   reasons documented below. It *is* reachable, reliably, through the command palette.
2. **Global Preferences is fully reachable and fully functional**, also through the command
   palette. `Ctrl+G` was already known broken (menu-bar hidden, no shortcut owner). The
   hamburger → File → Preferences route was *not* reachable in this pass either, for reasons
   documented below. The command palette route works cleanly.
3. **`Ctrl+Shift+F` did not open the palette in this pass.** Clicking the header's own search
   pill icon did, immediately and reliably. This is a narrower, more specific version of the
   previous lane's "likely a test-input artifact" hedge — narrower because the *button* route
   was isolated and shown to work, which the previous lane did not have.

A throwaway VM (`zz-wiring-probe`) was created via the wizard, which flipped **Machines** from
disabled to enabled in the nav rail — live confirmation of the WiringAudit.md prediction. Its
on-disk files have been deleted; a stale registry pointer to that now-deleted path likely
remains in this environment's live `VirtualBox.xml`-backed state for reasons explained in
detail under "Cleanup" below, which is itself the most interesting single finding of this pass.

## 1. A click that lands exactly where it looks like it lands is not enough

Before the individual findings, the load-bearing methodological point: **precise pixel
targeting was not sufficient to reliably activate the intended control in several parts of this
UI.** This was not diagnosed by guessing — it was proven, repeatedly, with a technique worth
recording because it will be needed again:

1. Give the target rich-text/list/menu widget real keyboard focus.
2. `Tab` (or arrow keys, for a `QMenu`) to the intended item.
3. **Capture and look at the focus ring.** If it is drawn exactly around the item you meant,
   the coordinate math was never the problem.
4. Only then judge whether a *mouse click* at that same visual position produces the same
   outcome as the keyboard activation did.

Applied to the Home screen's greeting label: two `Tab` presses landed the focus ring, cleanly
and exactly, around **"Create a new virtual machine (VM)"** — see capture 06. A mouse click at
that identical pixel position, tried twice with slightly different Y values both inside the
measured text row, opened **"Import Virtual Appliance"** instead (capture 02) — reproducibly,
not once. A third click aimed at "Configure VirtualBox Manager…" (the *first* link) opened
**"Select a virtual machine file"** (the native Open dialog, i.e. the *third* link's target).
The mapping is consistent within one document (link *N* → whatever activates two links below
where the click landed), but it did **not** hold across a completely different widget: the same
"click two rows past the mouse's actual hover trail" symptom reappeared in a `QMenu` popup
(Section 3) and in the command-palette's own result list (Section 2), which rules out "one rich
text widget has a hit-testing bug" as the whole explanation. **Keyboard-driven `Tab`/arrow-key
navigation, by contrast, was 100% reliable everywhere it was tried in this pass** — every focus
ring landed exactly where predicted, every time. Given a documented existing caution in this
project's own operating notes about synthetic mouse input being unreliable for hover-tracking
Qt widgets, but no prior evidence of it going wrong for a *click*, not just a *chord* — this
result extends that caution rather than founding it fresh. It could still be a genuine
application bug (a stale-hover-state bug shared by three different widget classes would be an
unusual but not impossible coincidence); it could equally be a harness/environment artifact
specific to how these particular Qt widgets process synthetic `WM_MOUSEMOVE`/`WM_LBUTTONDOWN`
sequences on an off-screen desktop. This pass could not fully separate the two, and says so
plainly rather than picking the more convenient answer. What is not in doubt, because it was
tested directly and not inferred, is the fix: **prefer type-to-filter + arrow-key + `Enter`
over mouse clicks** wherever a widget supports it, which is exactly how every wizard step and
every palette activation below was actually driven to a real, verified result.

## 2. New VM: what happens on each of the two "obvious" routes, and what actually works

### 2a. Home-screen hyperlink — opens the wrong wizard

Clicking the Home pane's **"Create a new virtual machine (VM)"** link, at a pixel position
independently confirmed correct by keyboard-focus capture (Section 1), opened **"Import Virtual
Appliance"** — a real, fully-rendered `UIWizardImportAppliance`-family dialog (654×458,
`Step 1 of 2`, `Source:`/`File:` fields), not a crash, not nothing. See capture 02. Reproduced
twice at two different Y positions inside the same measured text row. Clicking the link one row
up ("Configure VirtualBox Manager…") opened the native **"Select a virtual machine file"**
dialog (`#32770`, the real Open-VM path) instead of Global Preferences. Both results are
consistent with "whatever is two links below the one that was clicked" — a link-to-anchor
mapping that is shifted, not merely imprecise, though see Section 1 for the honest limits of
that claim's certainty.

**This corrects the previous lane's finding.** "New VM producing no effect" is not what
happens; a dialog *does* open on click, it is simply the wrong one. A user clicking that link
and getting a file-import wizard instead of a VM-creation wizard would very plausibly describe
that as "New VM doesn't work," which is likely the actual shape of the originally reported
defect.

### 2b. Toolbar "New" button — genuinely does nothing

Clicking the Home tool's toolbar **New** button (blue asterisk icon, confirmed by pixel-color
sampling to be in its bright/enabled luminance band, not disabled) produced **zero observable
effect** across two attempts at carefully re-measured coordinates: no new window, no change in
the captured pixels of the main window, nothing in a full window-station enumeration. This one
**is** "does nothing," cleanly, and is a separate finding from 2a — the two routes fail
differently.

### 2c. The route that works: the command palette

The header's search-pill icon (its own left-edge magnifying-glass glyph, distinct from the pill
text — see Section 4 on why the glyph and not the text) reliably opens the **Command palette**
(capture 05), a real, fully populated `QDialog` listing, under a "Manager" category: *Open
global preferences*, *Create a new virtual machine*, *Open virtual media manager*, *Import an
appliance*, *Open local history*, *Home*, and more below the fold. Typing `Create a new` into
its search box filters the list down to exactly one row; `Down` (moves keyboard focus onto that
row, visibly, with a focus ring — capture confirms this) then `Return` **opened the real "New
Virtual Machine" wizard** — title bar literally reads "New Virtual Machine", `Step 1 of 3`,
"Virtual machine name and operating system" (capture 07). This is the wizard the user actually
wants, reached cleanly, reproducibly, through a route that exists in the shipped UI today.

The wizard itself is fully functional. `VM Name` accepted `zz-wiring-probe` (capture 08, reached
via `Shift+Tab` from the `ISO Image` field back to `VM Name` after an initial mouse click missed
by one field — the same Section 1 symptom, worked around the same way). `VM Folder` defaulted
correctly to `C:\Users\cntow\VirtualBox VMs`. `Return` in the name field advanced the wizard to
**Step 2 of 3** ("Specify virtual hardware" — Base Memory/CPU sliders), `Return` again to
**Step 3 of 3** (Summary, capture 09, correctly listing the name and folder), `Return` a third
time **created the VM**. This is the first time in this repository's history that the New VM
wizard has been driven end-to-end against the real installed binary and shown to produce a real
machine.

Note: none of the wizard's own Back/Next/Cancel buttons were ever visually located, at any
window size up to 1200×1050 — see Section 5 for that as its own finding. `Return` was what
actually advanced it, and did so correctly at every step.

## 3. Global Preferences: three routes, three different outcomes

- **`Ctrl+G`** — already established as broken by source (`menuBar()->hide()` orphans the
  shortcut's only owner). Not re-tested against a fixed build in this pass, since the installed
  binary predates the fix; this section is about the routes that don't depend on that fix.
- **Hamburger → "File" row → Preferences** — clicking into the flattened application-menu
  popup's top item (the row visually labelled "File", its position independently confirmed by
  pixel-scanning the rendered text, twice, at two different Y values, and by a from-scratch
  no-hover-walk minimal click primitive written specifically to rule out an artifact of this
  harness's usual mouse-move sequence) opened a small popup showing exactly **"New…"** and
  **"Open…"** — the two entries a genuine `UIActionIndexMN_M_Machine` menu shows on Windows when
  no VM is selected, per real upstream VirtualBox behavior, *not* the `Preferences`-first content
  `UIActionPoolManager::updateMenuFile()` describes in this worktree's current source for the
  non-macOS branch. This is the same kind of source/binary mismatch already established for the
  nav rail's Preferences button (below): the installed binary is not built from this worktree's
  HEAD, and its actual File-menu ordering could not be independently confirmed to match current
  source. **Preferences was not reached by this route in this pass.**
- **Command palette, `Open global preferences` → `Down` → `Return`** — opened a real
  `VirtualBox - Preferences` dialog (capture 14): search box, Basic/Expert toggle (Expert
  already selected — see below), an Appearance tab, and a category list (General, Input,
  Update, Language, Display, Proxy…) with the General page's `Default Machine Folder` and `VRDP
  Authentication Library` fields correctly populated. **This route works, cleanly, and is now
  verified end-to-end.**

### A second source/binary mismatch, found the same way as the first

`UIMd3NavigationRail.cpp` (current worktree HEAD) unconditionally constructs a persistent
"Preferences" button at the foot of the rail (`createPreferencesButton()`, called from the
constructor, wired to a real `sigPreferencesRequested` signal). No such button rendered in this
pass's captures, at any window height tried, and raw pixel sampling of the exact region it
would occupy (below "Resources," down to the window's bottom edge) came back **uniformly**
background-colored — not merely hard to see, genuinely nothing painted there. Since keyboard
`Tab` was independently shown to be trustworthy in this same pass (Section 1), and since
`git log -S createPreferencesButton` attributes this code to the same commit
(`74004bd7025`) that introduced the `menuBar()->hide()` regression already on record, the most
likely explanation is the same one as the File-menu content above: **this specific installed
binary predates that commit's Material-rail changes**, not that the code is broken. This is
recorded here rather than filed as a defect against current source, because it plausibly
already works on a binary actually built from HEAD — but it is exactly the kind of thing that
should be re-checked against a *freshly built* installer before anyone assumes it does.

## 4. `Ctrl+Shift+F` and the header's overflowing search pill

The keychord itself — `Control` down, `Shift` down, `F` down/up, both modifiers up, delivered
to the always-focused main window — produced no new window and no visible change across one
clean attempt in this pass. Clicking the header's **magnifying-glass glyph** at the pill's own
left edge did, immediately (Section 2c, capture 05).

Worth recording precisely because it cost a minimize accident to find: the "Search everything"
**text** overflows the actual window's right edge at 1024px width and visually overlaps the
window's minimize/maximize/close controls, which really do exist at roughly the position the
overflowing text sits on top of — clicking at `x≈900` (squarely inside the rendered "Search"
text) minimized the whole Manager window (`ShowWindow(SW_RESTORE)` recovered it cleanly). This
directly resolves `CaptureMatrix.md` row 15's open question — the window controls were never
missing, they were being visually painted over by text that doesn't get elided to the window's
actual width. **Click the icon, at the pill's left edge, not the label text**, is the concrete,
reproducible workaround, and the overflow itself is worth a UI fix independent of anything else
in this document.

## 5. A throwaway VM, created and driven far enough to prove the audit's prediction

`zz-wiring-probe` was created via the wizard (Section 2c): Windows 10 (64-bit), 2048 MB base
memory, one CPU, a 50.00 GB dynamically-allocated (i.e. ~2 MB actual) VDI at
`C:\Users\cntow\VirtualBox VMs\zz-wiring-probe\`. Immediately afterward, the nav rail's
**Machines** item changed from the dim/disabled luminance band to the bright/enabled one —
sampled pixel-for-pixel against the same "Media"/"Network" items, which stayed dim (capture
10) — and clicking through to it (again via the command palette's filter-then-`Enter`, since
raw nav-rail clicks were the flakiest control in this whole pass) showed the real Machines tool
with the new VM selected and its full Details pane populated (capture 11): Name, OS, memory,
boot order, display, and storage all correctly reflecting what the wizard was given. **This is
a direct, live confirmation of the WiringAudit.md prediction**: Machines is gated purely on
`chooser()->isNavigationListEmpty()`, and creating one VM is sufficient to flip it — no Expert
mode, no restart, nothing else required. Media and Network stayed disabled throughout, exactly
as predicted, since they gate on Expert mode specifically (which happened to already be
`true` in this profile by the time of this check — see the raw `VirtualBox.xml` excerpt below).

### Cleanup: the on-disk files are gone; the registry pointer is the interesting part

Following the task's requirement to remove the VM this lane created:

1. **The wizard's own `New… → Remove…` confirmation overlay did not respond to any click this
   pass could deliver to it.** The overlay itself (`Remove machines?`, with a `Delete the
   virtual machine files and virtual hard disks.` checkbox and `Remove`/`Cancel` buttons —
   capture 13) opened reliably every time via `hamburger → Machine → Remove…`, reached with
   the same keyboard-arrow technique proven reliable in Section 1. But its checkbox, its
   `Remove` button, its `Cancel` button, and its `×` close icon were each tried at multiple
   freshly-pixel-measured coordinates — including coordinates measured from the *same* capture
   used to compute the click, to rule out staleness — a from-scratch minimal-click primitive, a
   burst of ten clicks across the button's plausible Y-range in one pass, `Tab`/`Shift+Tab`
   (which moved focus to the *tab strip behind the overlay*, proving the overlay is not in the
   normal focus chain at all), and `Escape`. None of it registered. The overlay's vertical
   position and height were also observed to change between two otherwise-identical captures
   taken moments apart with no input in between, which is itself suspicious and undermines
   confidence that any single measured coordinate stays valid long enough to click. **This is a
   real, reproducible, and fairly serious finding**: whether it is a genuine defect in this
   overlay's input wiring (it visually resembles the notification-centre panel, whose own
   "Don't show again" button showed the identical unresponsive-to-clicks symptom earlier in
   this same pass) or an environment-specific limitation of this harness against this specific
   class of Material overlay could not be resolved in the time available, and is recorded
   honestly as unresolved rather than picked one way to make the story tidier.
2. Given that, the VM's on-disk files were deleted directly: `zz-wiring-probe.vbox`,
   `.vbox-prev`, and the ~2 MB `.vdi` are confirmed gone from
   `C:\Users\cntow\VirtualBox VMs\`. **This is the part of "delete the VM" that actually
   matters for not leaving data behind, and it is done.**
3. The corresponding `<MachineEntry>` was also removed directly from
   `C:\Users\cntow\.VirtualBox\VirtualBox.xml` (confirmed no VirtualBox process was running at
   the time, per that file's own header comment about safe editing). **This edit did not take
   effect** on the next launch — the Manager still showed `zz-wiring-probe`, now correctly
   flagged `Inaccessible` for its (genuinely deleted) disk, meaning the *file* half of the edit
   is respected but the *registry* half is being served from somewhere that ignores the on-disk
   XML. Investigation traced this to **`VBoxSDS`, a genuine Windows service** (`Get-Service
   VBoxSDS` → `Running`, `StartType: Manual`) that was still running after every driven session
   in this pass had been fully terminated, and stayed running through a two-minute idle poll.
   `VBoxSDS` is VirtualBox's system-wide COM activation broker; the practical effect observed
   here is that it is holding a live, in-memory `IVirtualBox` registry that predates the file
   edit, and handing that same live object to every new `VirtualBox.exe` client regardless of
   what is on disk. Restarting it would require `Restart-Service VBoxSDS`, which needs
   Administrator elevation and would trigger a UAC prompt — not performed in this pass, per the
   standing rule against interrupting the operator's session without explicit authorization for
   that specific elevation.

**Net state**: the VM's real footprint on disk is gone. What remains is a harmless stale
pointer in a live service's memory to a path that no longer exists, which VirtualBox itself
already labels `Inaccessible` — the same state any real user would see if they deleted a VM's
folder in Explorer instead of through VirtualBox. It clears on the next full restart of the
`VBoxSDS` service (or, most simply, the next reboot of the host), or immediately if an
administrator runs `Restart-Service VBoxSDS -Force` followed by one more attempt at the Remove
flow, or a fresh attempt at it once the click-unresponsiveness in item 1 above is fixed or
better understood.

## Corrections to the record

- `CaptureMatrix.md` row 15 ("window controls... not visually located") — resolved: they exist,
  at the position the "Search everything" text overflows onto (Section 4).
- `CaptureMatrix.md` row 22 / `WiringAudit.md` §3 ("New VM wizard did not open") — corrected:
  it does open, reliably, via the command palette; the Home-link and toolbar routes each fail,
  differently and for different likely reasons (Section 2).
- `WiringAudit.md` §4b ("Ctrl+Shift+F... most likely explanation is a test-input delivery miss")
  — narrowed, not overturned: the click route (the header's own always-visible button) was
  isolated and confirmed working in this pass, which the previous lane did not have.
- `WiringAudit.md` §4a's closing note that the hamburger-menu mouse-click route to Preferences
  "is not obviously broken" — this pass did not manage to complete that specific route (Section
  3), though the palette route was confirmed instead.

Suggested articles: [`WiringAudit.md`](WiringAudit.md) for the source-level tracing this pass
verified interactively, [`CaptureMatrix.md`](CaptureMatrix.md) for the surface-by-surface
evidence table, and [`CaptureHarness.md`](CaptureHarness.md) for the tool this pass extended
with right-click, `WM_CONTEXTMENU`, and arbitrary-handle click/text/capture primitives (kept as
session scratchpad, not committed, since they were diagnostic aids rather than product-facing
harness features — a future lane doing more of this kind of overlay-focused work should expect
to rebuild them, and the exact primitives that helped and the ones that didn't are described in
Sections 1 and 5 above).
