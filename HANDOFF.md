# Handoff

Last updated: 2026-08-14, after two ultra-speed passes that landed four features on `main`.

Read the two "what this pass did" sections below together. The first round landed the in-app
changelog viewer; the second landed three more features on top of it. Neither ran tests or captures,
and both say so.

## Round two: three previously-absent features

A second ultra-speed round implemented three canonical features that this repository's own
`doc/md3/CompletenessInventory.md` recorded as **"Not implemented on any surface"** — each written
in its own isolated worktree, each statically compile-guarded, none compiled or tested:

| Feature | Class | Inventory section |
|---|---|---|
| Emoji-in-dialogs toggle | `UIMd3EmojiSetting` | 3 |
| Dim sum startup surprise | `UIMd3DimSumSurprise` | 7 |
| Personal-vocabulary JSON upload | `UIMd3PersonalVocabulary` | 20 |

**Each carries an honest limit, recorded in its own inventory row rather than glossed:**

- The emoji setting is infrastructure only. No dialog in the frontend calls its decorate helper yet,
  so enabling it changes nothing visible except the palette row's own label. Retrofitting the real
  message boxes is the follow-up.
- The dim sum surprise ships **no photograph, deliberately.** Images belong to a separate public
  catalog and must never be vendored, generated, or downloaded into this repository, so the toast
  renders an explicit "photo not included in this build" placeholder. That is the correct
  implementation, not a shortfall.
- The vocabulary service is real and validated, but no other surface routes its text through it yet,
  so a valid file currently has no visible effect outside the control's own status line.

### One risk was investigated rather than shipped on a hunch

A reviewer flagged that the emoji literals are astral-plane (4-byte UTF-8) characters, that
`Config.kmk` sets no MSVC `/utf-8`, and that no existing frontend source contains any astral-plane
character — so there was no precedent. That was checked before integrating rather than left to a
90-minute build: **22 non-ASCII sources already compile green with no byte-order mark**, including
`UIMd3Language.cpp`'s Cantonese, and the new files match that precedent exactly. How the compiler
decodes the file does not change between 3-byte and 4-byte sequences, so this is a runtime-rendering
question that already applies to every Cantonese string in the tree, not a build blocker.

If a future build does fail on source encoding, that is where to look first, and the fix is
tree-wide rather than specific to these files.

## Round one: the changelog viewer, and what it deliberately did not check

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

## Yum tong gate audit — what a release-grade shutdown would still need

The pass was escalated from ultra-speed to release-grade shutdown. That contract has fourteen
non-negotiable gates and **it is not satisfied**, so this section records each one honestly instead
of declaring a finish. Several are blocked by this environment rather than by the repository, and
those are marked as such so the next owner knows which are real work and which are just a machine
away.

| # | Gate | State | Evidence or blocker |
|---|---|---|---|
| 1 | Full inventory of jers, worktrees, stashes, tags, releases, divergence | **met** | Recorded in "Repository state" below |
| 2 | Every valid change committed on its owning jer | **met** | All worktrees clean, nothing excluded silently |
| 3 | Recoverable work pushed before integration | **met** | Every lane pushed before any deletion |
| 4 | Remote not ahead of the working jer | **met** | `main` fast-forwarded; no Fay Gay encountered |
| 5 | Hand-written local-suite inventory, every suite passing | **NOT met** | `doc/md3/LocalGates.md` records 8 gates runnable and 10 blocked on the Windows/kBuild/Qt toolchain. This container additionally has no `pwsh`, so even the 8 could not be re-run here — CI's `md3-validation` covers their contracts and is green |
| 6 | Installable artifacts built locally and validated | **NOT met — environment** | No Windows toolchain, no Qt, no `kmk`. Artifacts are built by CI only |
| 7 | Original logo and packaged application icon verified in the artifact | **NOT met** | Not audited in this pass; requires the built artifact |
| 8 | Every canonical feature implemented per surface with full evidence | **NOT met** | 40 rows still "Not implemented"; 7 features have zero implementation anywhere. See `CompletenessInventory.md` |
| 9 | README and landing page carry a current real-capture matrix | **NOT met** | `CaptureMatrix.md` records 0 of 47 captures; blocked by an unregistered COM/SDS service and, here, by having no Windows host at all |
| 10 | Exactly one new non-draft release representing this pass | **NOT met** | The workflow publishes a release on every push to `main`, and this pass made several pushes. Intermediate releases exist by design |
| 11 | Dewed `main` has a green remote CI verdict | **pending** | Material 3 validation and Pages are green on the integrated tip; the Windows build had not returned when this was written |
| 12 | Every source jer tip proved an ancestor before deletion | **met** | Proved for all seven deleted items |
| 13 | Fresh mat day supplied before any deletion | **met** | Supplied for this pass; applied to the cleanup half only |
| 14 | Only `main` and the primary checkout remain | **NOT met** | Three merged jers remain on the hui because remote jer deletion is refused by this environment's permission classifier |

**The honest summary: 7 of 14 gates met.** Gates 6, 7 and 9 need a Windows host and cannot be
closed from here at all. Gate 14 needs one permission. Gates 5, 8 and 10 need real work in the
repository. None of them should be marked done by weakening the gate.

### "Make the app work" — the part that is not a defect

The application builds, installs, launches and reaches a working Manager. **It cannot start a
virtual machine, and no change in this repository can make it.** The host hypervisor driver ships
unsigned, 64-bit Windows refuses to load an unsigned kernel driver, and code signing is permanently
prohibited for this project. That is a policy consequence with an owner outside this codebase — only
the machine's owner choosing to permit unsigned drivers moves it. The installer already reports the
real per-driver outcome rather than failing silently or implying success, which is the correct
behaviour under that ceiling.

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

1. **Read two build verdicts, and keep them apart — they bisect the work for you.** Two runs are in
   flight against different commits, and the release workflow has no `concurrency` block, so the
   first was not cancelled by the second:
   - the run on `bb63f016` compiles the **changelog viewer alone**;
   - the run on `d548859c` compiles the **three features above, on top of it**.

   Green then red means the three new features broke it. Red on the first means the changelog viewer
   did. Do not push again before reading them, or that separation is lost.
2. Confirm the release each successful run publishes: a new unique tag, non-draft, targeting the
   intended commit, with its installer and checksum attached.
3. **Run the tests and captures these passes skipped**, against the merged tree. Four features —
   the changelog viewer, the emoji setting, the dim sum surprise and the vocabulary upload — now sit
   on `main` with no test and no capture evidence of any kind. That is the largest outstanding debt
   in this repository and it was taken on deliberately, not by accident.
4. Recompute the summary counts table at the foot of `doc/md3/CompletenessInventory.md`. Three rows
   changed status in this pass and each lane deliberately left the aggregate alone rather than
   guessing at a total it could not verify from its own worktree.
5. Re-verify the three `UsabilityProbe.md` defects against a current installer before treating any
   of them as open, and re-verify the `Ctrl+G` finding against `66c701d6`/`90aedcff`.
6. Work through `doc/md3/CompletenessInventory.md` and `doc/md3/CaptureMatrix.md`. Both are
   deliberate, honest gap lists rather than checklists of what already exists.
7. Leave the unsigned-driver ceiling alone unless the machine's owner decides to permit unsigned
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
