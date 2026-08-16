# Material Virtual Machine roadmap

The rewrite is delivered in native Qt slices that preserve VirtualBox behavior
and remain independently buildable. Status is evidence-based; an implementation
row remains open until its required source, interaction, runtime, accessibility,
CI, documentation, and capture evidence exists.

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
   fixed: published non-draft releases `v7.2.97-ci.97` through
   `v7.2.97-ci.101` each carry a real unsigned NSIS installer
   (`VirtualBox-7.2.97-Setup.exe`, ~106.9 MB). `v7.2.97-ci.96` is **not** one
   of them — it is the retired Squirrel.Windows package and carries no
   `VirtualBox-7.2.97-Setup.exe`; `ci.97` is the first NSIS release. The newest,
   `v7.2.97-ci.101`, targets commit `bb63f016` with a Windows build that
   completed successfully.
   See [`HANDOFF.md`](HANDOFF.md#windows-build-and-packaging-pipeline-2026-08-13)
   for the full current-state summary and exact CI run links.
1. **Real capture evidence** — [`doc/md3/CaptureMatrix.md`](doc/md3/CaptureMatrix.md)
   enumerates every manager, settings, wizard, tool, runtime, and installer
   surface and state that still needs a genuine screenshot, and is the
   authority for the current captured/open count. Several rows are now
   captured with the manager shell reached and photographed; the rest
   remain `Not captured`, blocked either by needing a running VM or an
   installed host service (kernel drivers, `VBoxSDS`) that this lane cannot
   provision (`REGDB_E_CLASSNOTREG`, tracked in
   [`doc/md3/RuntimeCapture.md`](doc/md3/RuntimeCapture.md)), or simply not
   yet attempted. Closing the remaining rows is the next gate.

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
