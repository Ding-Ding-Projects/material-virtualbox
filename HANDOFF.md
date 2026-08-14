# Handoff

Last updated: 2026-08-14, after the ultra-speed pass that landed the changelog viewer on `main`.

## What this pass did, and what it deliberately did not check

This was an ultra-speed feature pass: one feature, its directly related records, integration and a
release. **No tests and no captures were run, by that pass's own rule.** That is stated here rather
than left for a reader to infer from their absence.

The one feature: the in-app changelog viewer (`UIMd3Changelog`) reached `main`, so it ships in a
real release instead of sitting on a side jer.

Before integrating, three independent reviewers checked the never-compiled C++ against working
sibling sources in `src/md3/` — one on build wiring and includes, one on Qt API signatures and
declared Qt modules, one on the singleton/lifecycle pattern. All three returned no build blockers.

**That is a static review, not a compilation.** It lowers the risk that the release build fails; it
does not prove the code compiles. The Windows workflow triggered by this integration is the first
real compile this code has ever had, and its verdict is the one that counts.

## Current state, in one line

The Windows pipeline builds VirtualBox 7.2.97 end to end on a cold CI runner and publishes a real
release whose installer is now the NSIS one — the single elevated installer that can register COM
and install the `VBoxSDS` service. The application installs, launches, and reaches a working
Manager. It still cannot start a virtual machine, for a reason that is understood, unchanged, and
stated below.

## What changed in this pass

No source was written for the product in this pass. Two branches that were sitting unmerged on the
remote were reviewed and integrated, and this document was corrected against the repository as it
actually is. The previous version of this file was wrong in four separate places, which is recorded
here rather than quietly edited away:

| The previous handoff said | The repository actually shows |
|---|---|
| `main` is `7885a61e681` | `main` is `8850ddb7efb248e79dddd697118517d426bb61c4` |
| Latest release is `v7.2.97-ci.96` | Latest release is `v7.2.97-ci.100` |
| **The published release ships the Squirrel installer** | The published release ships the **NSIS** installer |
| Two branches on the remote | Four branches on the remote |
| CI run 31767416361's verdict is unknown | It completed **success** |

The Squirrel line is the one that mattered: it was the headline defect of the previous pass, it was
already fixed by the time the file was written, and anybody reading it would have gone looking for a
problem that no longer existed.

## What is verified

Every row below was checked against the live repository and the GitHub API during this pass.

| Item | State | Evidence |
|---|---|---|
| Windows package and release workflow | **green** | run `31784982408` at `8850ddb7e` |
| Material 3 validation | **green** | run `31784982426` at `8850ddb7e` |
| Material 3 documentation Pages | **green** | run `31784982448` at `8850ddb7e` |
| The NSIS switchover run (previously unknown) | **green** | run `31767416361` at `7885a61e6` |
| Published release | **exists, non-draft** | `v7.2.97-ci.100`, target `8850ddb7efb`, published 2026-08-14T10:06:47Z |
| Installer asset | **106,872,738 bytes** | `VirtualBox-7.2.97-Setup.exe`, SHA-256 `766c7e4e…3ba0bf` |
| Installer is the NSIS one | **yes** | built from `out/win.amd64/release/nsis-installer/` |
| Release evidence contract | **complete** | `SHA256SUMS.txt`, dim-sum photo, line-count table, workflow duration `01:24:45` |
| Local gates | **8 of 8 pass** | see `doc/md3/LocalGates.md` |

## What is NOT true yet, stated plainly

- **No virtual machine can start.** Unchanged and unchangeable here. The host hypervisor driver
  `VBoxSup.sys` is unsigned, 64-bit Windows will not load an unsigned kernel driver, and code
  signing is permanently prohibited for this project. The installer attempts driver installation and
  reports the real per-driver outcome rather than failing silently or claiming success. This is a
  policy consequence, not a defect, and no installer change can resolve it.
- **`VBoxNetAdp6` (host-only networking) and `VBoxNetLwf` (bridged networking) are not installed**
  by the installer. See `doc/installer/WindowsHostInstallerNSIS.md`.
- **The capture matrix is mostly empty.** `doc/md3/CaptureMatrix.md` still carries 44 rows marked
  `Not captured`, each with its blocker named.
- **Many canonical features remain absent or partial.** `doc/md3/CompletenessInventory.md` is the
  authority and is hand-written on purpose. Do not restate its counts from memory or by hand-counting
  — read the document.

## The reality audit's findings, and why they are leads rather than defects

`doc/md3/WiringAudit.md` and `doc/md3/UsabilityProbe.md` landed in this pass. They are the only
evidence in this repository produced by *driving* the built application rather than reading its
source, and `UsabilityProbe.md` carries 14 captures pinned to individual claims.

**Read them with their commit dates in hand, because both audited older code than `main` now
carries:**

- `WiringAudit.md` audited `0d9eda43cd0`. Its most serious finding — that `Ctrl+G` not opening
  Global Preferences is a real regression, systemic across three sites in the VM Runtime window —
  was named **before** the fixes `66c701d6` and `90aedcff` landed. Both are ancestors of `main`
  today (`git merge-base --is-ancestor` confirms). **The finding is therefore not known to be open;
  it needs re-verification against a current build before anybody acts on it.**
- `UsabilityProbe.md` drove an installed binary dated 2026-08-10, which the document itself flags as
  "an unknown number of commits behind". Its three ugly findings — the Create VM link opening the
  *import* wizard, a file row opening a mismatched submenu, and the remove-confirmation overlay not
  responding to clicks — are recorded with captures and are **unverified against the current
  installer**.

Its genuinely durable contributions, independent of build age:

- New VM and Global Preferences are both fully functional and reachable **through the command
  palette**, which overturns the previous lane's "needs interactive re-verification" hedge.
- Machines/Media/Network being disabled is **not a defect** — it is upstream VirtualBox behaviour
  faithfully reproduced, including the upstream route out of the restriction.
- The method is worth reusing: give a control real keyboard focus, `Tab` to it, **capture the focus
  ring**, and only then judge whether a mouse click at that same pixel does the same thing. That is
  what proved the click targeting — not the coordinate math — was at fault.

## Repository state

- `main` is `8850ddb7efb`, clean.
- Four branches existed on the remote at the start of this pass:
  - `main`
  - `claude/external-editor-20260814` — **already merged**, proved an ancestor of `main`.
  - `claude/reality-audit-20260814` — was unmerged, **now integrated** (2 commits).
  - `worktree-wf_97c547ae-f89-4` — was unmerged, **now integrated** (1 commit), and this one
    mattered: it carried a complete, unlanded in-app changelog viewer, `UIMd3Changelog`
    (~1,010 lines) with the date filter, text search and per-entry commit links the house contract
    requires, plus its manager wiring, lifecycle and build entry. A cleanup pass that deleted merged
    branches without reading them would have thrown a whole feature away.
- All of that work is now **merged into `main` and pushed**, which is what triggered the release
  build for this pass.
- Merged task branches and their worktrees were removed after each tip was proved an ancestor of the
  pushed `main`. No stashes existed at any point.

## Next actions, in order

1. **Read the verdict of the Windows package and release run triggered by this integration.** It is
   the first compilation the changelog viewer has ever had. If it is red, the fix belongs in
   `UIMd3Changelog.cpp`/`.h` or in the `UIVirtualBoxManager.cpp` conflict resolution that kept both
   the external-editor and changelog palette commands — that resolution is the specific thing only a
   compile can prove.
2. Confirm the release that run publishes: a new unique tag, non-draft, targeting the integrated
   commit, with its installer and checksum attached.
3. **Run the tests and captures this pass skipped**, against the merged tree. The ultra-speed pass
   deliberately ran none, so the changelog viewer has no test or capture evidence of any kind yet.
4. Re-verify the three `UsabilityProbe.md` defects against a current installer before treating any
   of them as open, and re-verify the `Ctrl+G` finding against `66c701d6`/`90aedcff`.
5. Work through `doc/md3/CompletenessInventory.md` and `doc/md3/CaptureMatrix.md`. Both are
   deliberate, honest gap lists rather than checklists of what already exists.
6. Leave the unsigned-driver ceiling alone unless the machine's owner decides to permit unsigned
   drivers. Nothing in this repository can move it.

## Documentation

- `doc/installer/WindowsHostInstallerNSIS.md` — the single Windows installer: what it registers,
  what it cannot do unsigned, silent install, uninstall, failure modes.
- `doc/md3/LocalGates.md` — every locally runnable gate, its command and real result, and the gates
  this project should have and does not.
- `doc/md3/CompletenessInventory.md` — per-surface feature inventory.
- `doc/md3/CaptureMatrix.md` — every surface that must be captured, and why each uncaptured row is
  not captured.
- `doc/md3/CaptureHarness.md` — the runtime capture harness and how it validates its own output.
- `doc/md3/WiringAudit.md` — what the Material 3 manager is really wired to, traced to file and line.
- `doc/md3/UsabilityProbe.md` — 14 captures from driving the installed application.
- `doc/md3/Changelog.md` — the in-app changelog viewer.
- `CHANGELOG.md` — each entry linked to its verified commit.
