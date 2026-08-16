# Material Virtual Machine roadmap

The rewrite is delivered in native Qt slices that preserve VirtualBox behavior
and remain independently buildable. Status is evidence-based; an implementation
row remains open until its required source, interaction, runtime, accessibility,
CI, documentation, and capture evidence exists.

## The ceiling this roadmap can never cross

**No virtual machine can start.** The host hypervisor driver `VBoxSup.sys` is
unsigned, 64-bit Windows will not load an unsigned kernel driver, and code
signing is permanently prohibited for this project. It is a policy consequence,
not a defect, and no installer change can resolve it. No item below closes it,
and none should be written as if it might: the only thing that moves it is the
machine's owner choosing to permit unsigned drivers, which is a decision with an
owner outside this codebase. Every runtime-window row in
[`doc/md3/CaptureMatrix.md`](doc/md3/CaptureMatrix.md) is ceilinged by it rather
than merely unattempted.

## Windows build and packaging (2026-08-13 status)

0. **Pipeline hardening** — seven fixes landed today, each removing one
   failure a clean CI checkout surfaced (missing host binaries, missing Qt
   modules, a rejected module name, an auto-selected v142 MSVC toolset, a
   toolset-selection loop bug, a mixed-separator redistributable path, and a
   fatal MSVC warning on a GCC-only macro). Every fix is linked to its
   verified commit SHA in [`CHANGELOG.md`](CHANGELOG.md). The **Material 3
   validation** workflow is **red on `main`** and has been since
   `8762579681594cf8ba6beb8ccec77576baa5199a` — this line claimed it was green
   until 2026-08-16; see the state table at the top of
   [`HANDOFF.md`](HANDOFF.md) for the three failing runs. The **Windows
   package and release** workflow previously failed later in the
   pipeline on `STATUS_STACK_BUFFER_OVERRUN` in the packaging step's own
   `tstVMStructSize`/`tstAsmStructs` self-check. That blocker has since been
   fixed: every non-draft `v7.2.97-ci.*` release from `v7.2.97-ci.97` onward
   carries a real unsigned NSIS installer named `VirtualBox-7.2.97-Setup.exe`
   together with `SHA256SUMS.txt`. `v7.2.97-ci.96` is **not** one
   of them — it is the retired Squirrel.Windows package and carries no
   `VirtualBox-7.2.97-Setup.exe`; `ci.97` is the first NSIS release. **Which
   release is newest, and what commit it targets, is answered by the
   [Releases page](https://github.com/Ding-Ding-Projects/material-virtualbox/releases)
   and not by this file** — no tag is pinned in this prose, because each earlier
   attempt to pin one was stale within days. **Nothing here claims any of those
   installers has been downloaded, installed, or launched**; see the retraction
   at the head of [`CHANGELOG.md`](CHANGELOG.md).
   See [`HANDOFF.md`](HANDOFF.md) for the full current-state summary and exact
   CI run links.
1. **Real capture evidence** — [`doc/md3/CaptureMatrix.md`](doc/md3/CaptureMatrix.md)
   enumerates every manager, settings, wizard, tool, runtime, and installer
   surface and state that still needs a genuine screenshot, and is the
   authority for the current captured/open count. A few rows are captured. One
   of them shows the manager shell, and it came from an **already-present
   installed** `VirtualBox.exe` at worktree tip `cb9f573030e` — not from this
   checkout's own build, which still fails at `REGDB_E_CLASSNOTREG` for want of
   a registered `VBoxSDS` (tracked in
   [`doc/md3/RuntimeCapture.md`](doc/md3/RuntimeCapture.md)). The rest remain
   `Not captured`, blocked either by that COM boundary, by needing a running VM,
   or simply not yet attempted. Closing the remaining rows is the next gate —
   except the runtime-window rows, which the ceiling above closes permanently.

## In progress

1. **Manager tools** — the existing Extensions, Media, Network, Cloud, and VM
   Activity Overview panes now share a bounded Material search and appearance
   card without replacing their models. Continue with bulk actions, export,
   Logs, detached manager windows, complete accessibility, and truthful
   empty/error states.
2. **Runtime chrome** — compose native Material controls around
   `UIMachineWindow` and `UIMachineView` without changing guest rendering,
   input capture, session state, multi-monitor ownership, or fullscreen and
   seamless semantics.
3. **Surface completion** — close remaining settings, wizard, notification,
   history, command-palette, tabs, appearance, language, export, bulk-action,
   and accessibility gaps recorded in the 69-entry design ledger.
4. **Native evidence** — build and launch the real application, capture every
   required state at supported display scales and language modes, and keep the
   captures tied to exact commits and artifacts.

## Implemented foundations

- **Theme correctness** — seed palettes are generated through the native
  HCT/CAM16 implementation instead of the former HSL stand-in. The compiled
  reference-vector testcase covers 127 checks, contrast pairs, gamut handling,
  and realised tone error; [`doc/md3/TonalPalette.md`](doc/md3/TonalPalette.md)
  records the implementation and verification boundary.

## Release readiness

- Local CI-equivalent checks and all required native targets pass on the exact
  candidate commit.
- GitHub Actions validation and Pages deployment complete successfully on that
  same commit.
- A real unsigned Windows installer is built through the supported repository
  scripts and verified by path, size, commit, and SHA-256.
- One unique non-draft release contains the verified installer, complete release
  evidence, workflow timing, and required metadata.
- The default branch contains every intended change, and no secondary checkout,
  branch, or stash retains unintegrated work.

Detailed implementation authority and evidence rules are in
[`doc/md3/CodexHandoff.md`](doc/md3/CodexHandoff.md); row-level status is in
[`doc/md3/DesignCoverage.md`](doc/md3/DesignCoverage.md).
