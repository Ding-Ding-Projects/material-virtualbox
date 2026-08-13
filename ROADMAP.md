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
   validation** workflow is green at the current commit; the **Windows
   package and release** workflow is still red, now failing later in the
   pipeline on `STATUS_STACK_BUFFER_OVERRUN` in the packaging step's own
   `tstVMStructSize`/`tstAsmStructs` self-check — a new, not-yet-root-caused
   failure. No release has been published and no verified installer exists.
   See [`HANDOFF.md`](HANDOFF.md#windows-build-and-packaging-pipeline-2026-08-13)
   for the full current-state summary and exact CI run links.
1. **Real capture evidence** — [`doc/md3/CaptureMatrix.md`](doc/md3/CaptureMatrix.md)
   enumerates every manager, settings, wizard, tool, runtime, and installer
   surface and state that still needs a genuine screenshot. Every row is
   currently `Not captured`, blocked either by the missing verified
   installer above or by this environment's unregistered COM/SDS classes
   (`REGDB_E_CLASSNOTREG`, tracked in
   [`doc/md3/RuntimeCapture.md`](doc/md3/RuntimeCapture.md)). Closing rows is
   the next gate after the packaging pipeline goes green.

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
