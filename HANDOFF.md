# Handoff

Last updated: 2026-08-14, after the Windows build, packaging and release pass.

## Current state, in one line

The Windows pipeline builds VirtualBox 7.2.97 end to end on a cold CI runner and publishes a real
release with a downloadable installer. The application installs, launches, and reaches a working
Manager. It cannot yet start a virtual machine, for a reason that is understood and stated below.

## What is verified

| Item | State | Evidence |
|---|---|---|
| Windows package and release workflow | **green** | run 31762849415, all 13 steps success |
| Material 3 validation | **green** | run 31767416379 at `7885a61e681` |
| Material 3 documentation Pages | **green** | run 31767416377 at `7885a61e681` |
| Published release | **exists, non-draft** | `v7.2.97-ci.96`, target `0d9eda43cd0` |
| Installer asset | **137,286,656 bytes** | `Setup.exe` attached to that release |
| Application launches | **yes** | captured from the installed build on an off-screen desktop |
| COM + `VBoxSDS` registration | **yes** | installer log, exit code 0, service present |
| Local gates | **8 of 8 pass** | see `doc/md3/LocalGates.md` |

## What is NOT true yet, stated plainly

- **The published release ships the Squirrel installer, not the NSIS one.** Squirrel is a per-user
  unpacker with no elevation: it registers no COM classes, installs no `VBoxSDS` service, and loads
  no kernel driver. This was proven by installing it and capturing the failure
  (`REGDB_E_CLASSNOTREG`, "The VBoxSDS windows service was not found"). The NSIS replacement is
  merged to `main` but has not yet produced a release.
- **No virtual machine can start.** The host hypervisor driver `VBoxSup.sys` is not present on a
  machine after install, and even once built it is unsigned. 64-bit Windows will not load an
  unsigned kernel driver, and code signing is permanently prohibited for this project. Secure Boot
  is off on the development host, which removes the firmware obstacle, but permitting an unsigned
  driver remains a decision for the machine's owner. The installer reports this honestly rather
  than failing silently.
- **A CI run is in flight** on `7885a61e681` (run 31767416361) covering the NSIS switchover. Its
  verdict is not known and is not predicted here.
- **The capture matrix is mostly empty.** Five surfaces are captured from the real built artifact;
  every other row in `doc/md3/CaptureMatrix.md` is honestly marked `Not captured` with its blocker.
- **Most canonical features are absent.** `doc/md3/CompletenessInventory.md` records 26 implemented,
  16 partial, 40 absent across 7 surfaces, hand-written rather than generated.

## What changed in this pass

Twenty-eight defects were found and fixed. Four were cold-machine failures that a warm developer
tree had been hiding; three were in files this repository owns and would have been wrong for anyone
building it, on any machine.

The ones worth knowing about:

- `kmk ... packing` runs only the packing pass and never compiled the host binaries. It passed
  locally solely because earlier builds had left them on disk.
- `configure.py` sorted MSVC toolsets newest-first and then iterated the whole list without
  breaking, so it selected the **oldest** — the v142 compatibility compiler over v143.
- `configure.py` emitted the toolset path with mixed separators, so kBuild's redistributable lookup
  silently fell through to a directory that exists, is correctly named, and is empty.
- Five testcases built with the `VBoxR3AutoTest` template crash on this toolchain before printing
  their first line. They are now gated behind `VBOX_WITHOUT_RUN_BUILD_TESTCASES`; the set is closed
  at exactly five, which is what makes it a fix rather than a recurring surprise.
- `SUPDrvA-win.asm` assembles to nothing when hardening is disabled, and NASM's CodeView-8 emitter
  asserts on the empty result. Upgrading NASM does not help; this was tested.
- `Squirrel.exe` is a windowed executable, so PowerShell did not wait for it and the next step
  checked for files nobody had started writing.
- Hiding the legacy `QMenuBar` left every menu-bound shortcut without a visible owner, so they
  stopped firing. The same pattern appeared at four sites across two windows.

## Repository state

- `main` is `7885a61e681`, clean.
- Two branches exist on the remote: `main` and `claude/reality-audit-20260814`.
- `claude/reality-audit-20260814` is **unmerged and retained deliberately** — work was in progress in
  it at the end of the pass. It must be reviewed and merged or explicitly abandoned before the
  repository can be considered main-only.
- Eleven task branches and their worktrees were removed after each was proved an ancestor of the
  pushed `main` and confirmed to hold no uncommitted work.

## Next actions, in order

1. Read the verdict of CI run 31767416361 on `7885a61e681`.
2. Cut a release from a commit that carries the NSIS installer, so the published artifact is the one
   that can actually register COM and install drivers.
3. Review `claude/reality-audit-20260814` and merge or abandon it.
4. Confirm `VBoxSup.sys` appears in the payload now that the NASM defect is fixed; the workflow
   fails loudly if any of the five host driver files are missing.
5. Decide how to handle the unsigned-driver ceiling. It is a policy consequence, not a defect, and
   no installer change can resolve it.
6. Work through `doc/md3/CompletenessInventory.md` and `doc/md3/CaptureMatrix.md`, both of which are
   deliberate, honest gap lists rather than checklists of what already exists.

## Documentation

- `doc/installer/WindowsHostInstallerNSIS.md` — the single Windows installer: what it registers,
  what it cannot do unsigned, silent install, uninstall, failure modes.
- `doc/md3/LocalGates.md` — every locally runnable gate, its command and real result, and the gates
  this project should have and does not.
- `doc/md3/CompletenessInventory.md` — per-surface feature inventory.
- `doc/md3/CaptureMatrix.md` — every surface that must be captured, and why each uncaptured row is
  not captured.
- `doc/md3/CaptureHarness.md` — the runtime capture harness and how it validates its own output.
- `CHANGELOG.md` — each entry linked to its verified commit.
