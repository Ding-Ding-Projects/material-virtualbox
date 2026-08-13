# Material Virtual Machine handoff

The Material Design 3 rewrite is an incremental presentation layer inside the
existing VirtualBox Qt frontend. It does not introduce a parallel application
or replace VirtualBox models, action pools, COM/XPCOM contracts, page
serializers, validation, guest-display ownership, or session lifecycle.

## Windows build and packaging pipeline (2026-08-13)

This section is current as of commit
[`9efa7f522c69f3ce1b99940d13e96d534ee2a0c3`](https://github.com/Ding-Ding-Projects/material-virtualbox/commit/9efa7f522c69f3ce1b99940d13e96d534ee2a0c3)
(the tip of `main`, confirmed with `git log` and `git cat-file -e` in this
checkout). It reports the build/packaging lane specifically; the Material
Design 3 rewrite state further down is unchanged by this work.

**What changed.** Seven commits landed today, each fixing the next failure a
clean CI checkout surfaced after the previous fix: staging binaries that were
never built (a warm local tree had hidden this for eighteen prior builds),
missing Qt modules, a rejected Qt module name, an auto-selected MSVC v142
toolset instead of v143, a toolset-selection loop that kept the *last*
(oldest) sorted entry instead of the first, a mixed-separator redistributable
path that made kBuild search the wrong directory, and a fatal MSVC warning
(`C4668`) on a GCC-only macro Qt's own header checks safely. Full detail,
each linked to its verified commit SHA, is in
[`CHANGELOG.md`](CHANGELOG.md).

**Verification evidence, stated honestly.**

- The **Material 3 validation** workflow is green at this commit:
  [run 31731859865](https://github.com/Ding-Ding-Projects/material-virtualbox/actions/runs/31731859865).
- The **Windows package and release** workflow is **red** at this commit:
  [run 31731859854](https://github.com/Ding-Ding-Projects/material-virtualbox/actions/runs/31731859854),
  1h 10m runtime. It now reaches substantially further than any prior run —
  through Qt install, toolset selection, the redistributable path, and the
  full build — before failing in the packaging step's own self-check with
  `STATUS_STACK_BUFFER_OVERRUN` (`0xC0000409`) while running
  `tstVMStructSize.exe` and `tstAsmStructs.exe`. This is a new failure
  signature, not yet root-caused, and is unrelated to any of the seven fixes
  above.
- **CI has not gone green.** **No GitHub release has been published**
  (`gh release list` returns empty for this repository as of this audit).
  **No verified Windows installer exists.**
- This worktree (`vbx-wt-docs-captures`, used only to write this
  documentation) contains no build output of its own — there is no `out/`
  directory and nothing was built here. Any local installer artifacts
  produced elsewhere today were built on a warm, already-configured tree
  from earlier incremental builds, not from a clean checkout run of
  `build.bat` / `build-installer.bat`. They are not evidence that the
  one-click scripts work end to end on a machine with nothing installed,
  and must not be presented as such until a clean-checkout run actually
  completes.
- Real screenshots of the built application do not exist yet, for two
  independent reasons tracked row-by-row in
  [`doc/md3/CaptureMatrix.md`](doc/md3/CaptureMatrix.md): no installer has
  been built to photograph, and the manager shell itself cannot currently be
  reached in this environment (`REGDB_E_CLASSNOTREG`, detailed below and in
  [`doc/md3/RuntimeCapture.md`](doc/md3/RuntimeCapture.md)).

**Related work not yet integrated.** Commit
[`1e59aca274c171b85a5e293e6fe8b70a04ccce27`](https://github.com/Ding-Ding-Projects/material-virtualbox/commit/1e59aca274c171b85a5e293e6fe8b70a04ccce27)
reconciles the Material Design 3 runtime window chrome branch
(`claude/full-ui-rewrite-reconciled-20260813`) with `main` as it stood at the
first commit in this chain. `main` has since advanced six further commits
that branch has not absorbed, and `git merge-base --is-ancestor` confirms it
is not currently an ancestor of `main`. It is a different lane's work and is
not represented as shipped by this section.

**What remains / next owner for this lane.** Root-cause the
`STATUS_STACK_BUFFER_OVERRUN` failure in `tstVMStructSize`/`tstAsmStructs`
during the packaging step (owned by whichever lane holds
`.github/workflows/windows-package-release.yml` and the `src/VBox/VMM`
testcase build — outside this documentation lane's edit boundary). Once the
workflow is green, build and verify an installer from a genuinely clean
checkout, publish one real release, and only then begin closing the
`Not captured` rows in the capture matrix.

## Current state

- The shared theme, stock-control style, language service, full regex/search
  infrastructure, appearance editor, command palette, local history, and
  notification history are owned by `UICommon`.
- Theme palettes are derived through the native HCT/CAM16 implementation and
  compiled reference-vector testcase; the former HSL approximation is removed.
- The manager has a native frameless header, responsive navigation rail,
  on-demand workspace tabs, compact navigation, and contextual destination
  heading while preserving the existing tool and chooser models.
- Extensions, Media, Network, Cloud, and VM Activity Overview share a bounded
  Material search and appearance card over their existing item views. Original
  hidden rows are restored when a query or destination changes.
- Global Preferences and per-machine Settings use one bounded selected page at
  a time; search intentionally changes to cross-page results.
- Native wizards use a responsive Material step rail and page card, reconcile
  hidden Basic/Expert pages, scroll internally on compact Windows desktops, and
  retain the real page stack and native action handlers.
- The design ledger still marks every incomplete archive row `In progress`.
  Manager-tool completion, runtime chrome, native interaction evidence,
  installer/release work, and final accessibility proof remain open.

## Verification and blockers

The current manager-tools working tree passes `git diff --check` and local
Windows x64 `UICommon`, `VirtualBox`, and `VirtualBoxVM` builds. The new shared
source compiled, the manager bindings compiled, and all three targets linked
with exit 0. The archive/source contract, workflow structure, XML/HTML, exact
commit, and CI/Pages evidence are the remaining publication gates for this
lane.

The development executable cannot currently open the real manager because its
COM/SDS classes are not registered. Design HTML and thumbnails are not used as
runtime evidence. System-wide registration requires a separately authorized
administrator operation; until that boundary changes, real application capture
rows remain open.

## Next owner

Continue from [`doc/md3/CodexHandoff.md`](doc/md3/CodexHandoff.md), reconcile
each lane against [`doc/md3/DesignCoverage.md`](doc/md3/DesignCoverage.md), and
keep target ownership and existing VirtualBox behavior authoritative. The next
broad lane remains manager-tool completion, followed by runtime chrome. Do not
close the design objective from static or prototype evidence.
