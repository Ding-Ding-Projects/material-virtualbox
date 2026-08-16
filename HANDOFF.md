# Handoff

Last updated: 2026-08-16, by a release-grade shutdown pass whose product was documentation
correctness, not features.

Read the two "what this pass did" sections below together. The first round landed the in-app
changelog viewer; the second landed three more features on top of it. Neither ran tests or captures,
and both say so.

## Read this first: state at 2026-08-16

Everything below this section was written by earlier passes and several of its rows were wrong by
the time you read them. This section is the current truth, each line from a command run on
2026-08-16. Where an older section still disagrees, the older section is stale and is corrected in
place further down rather than deleted.

| Fact | Value | Command |
|---|---|---|
| Remote `main` | `fe321a4fd6f6bfc7c2fbd7e0704e3f72d0eb2eee` | `git rev-parse origin/main` |
| This working branch | `claude/yum-tong-finish-20260816`. **Nothing on it is pushed.** No commit count is recorded here: the previous version said "three commits ahead … this pass adds a fourth" when it was already four ahead and the pass made five, which is how a running tally always ends. Read it, don't trust it | `git rev-parse --abbrev-ref HEAD`, `git log --oneline origin/main..HEAD` |
| Branches on the remote | **seven** — `main`, `codex/native-windows-ci-build-20260809` (= `fe321a4`), `claude/external-editor-20260814`, `claude/full-ui-rewrite-with-ultracode-77b55c`, `claude/reality-audit-20260814`, `claude/virtualbox-agent-memory-oabvhw`, `worktree-wf_97c547ae-f89-4` | `git ls-remote --heads origin` |
| Latest release | **`v7.2.97-ci.108`** — "Dried Scallop Shrimp Dumpling · 瑤柱蝦餃", non-draft, target `fe321a4fd6f6bfc7c2fbd7e0704e3f72d0eb2eee`, published `2026-08-15T01:21:51Z` | `gh release view v7.2.97-ci.108 --json tagName,isDraft,targetCommitish,publishedAt` |
| Its installer | `VirtualBox-7.2.97-Setup.exe`, **106,963,266 bytes**, SHA-256 `8a6e1e94bdd73c3a9c526b7b7e074521f06a2562b9f9020ea1a806f15fcda627` | `gh release view v7.2.97-ci.108 --json assets` |
| Its other assets | `SHA256SUMS.txt` (95 bytes), `hk-dish-0009-dried-scallop-shrimp-dumpling.png` (2,436,523 bytes) | same command |
| Windows package and release at `fe321a4` | **success**, run [`31851996367`](https://github.com/Ding-Ding-Projects/material-virtualbox/actions/runs/31851996367), 1h26m24s | `gh run list --commit fe321a4…` |
| Material 3 documentation Pages at `fe321a4` | **success**, run [`31851996366`](https://github.com/Ding-Ding-Projects/material-virtualbox/actions/runs/31851996366) | same command |
| Material 3 validation at `fe321a4` | **FAILURE**, runs [`31851996370`](https://github.com/Ding-Ding-Projects/material-virtualbox/actions/runs/31851996370) (on `main`), [`31856303192`](https://github.com/Ding-Ding-Projects/material-virtualbox/actions/runs/31856303192) (on the `v7.2.97-ci.108` tag ref), [`31851966893`](https://github.com/Ding-Ding-Projects/material-virtualbox/actions/runs/31851966893) (on `codex/native-windows-ci-build-20260809`) — `--commit` matches by `headSha` across every ref | same command |
| How long that gate has been red on `main` | **Since `8762579681594cf8ba6beb8ccec77576baa5199a`**, not since `fe321a4`. Three consecutive failures on `main`: [`31848342250`](https://github.com/Ding-Ding-Projects/material-virtualbox/actions/runs/31848342250) at `8762579` (`22:53:24Z`), [`31851883297`](https://github.com/Ding-Ding-Projects/material-virtualbox/actions/runs/31851883297) at `753602c` (`23:53:35Z`), [`31851996370`](https://github.com/Ding-Ding-Projects/material-virtualbox/actions/runs/31851996370) at `fe321a4` (`23:55:34Z`). Last green on `main`: [`31846486407`](https://github.com/Ding-Ding-Projects/material-virtualbox/actions/runs/31846486407) at `5f1adfac63`. **`ci.106` (target `8762579`) and `ci.107` (target `753602c`) both shipped with the gate already red** | `gh run list --workflow "Material 3 validation" --branch main --limit 15` |
| Releases that exist | ci.96, 97, 98, 99, 100, 101, 106, 107, 108. **ci.102–ci.105 were never created** | `gh release list --limit 10` |
| Tracked files | **66,376** | `git ls-files \| wc -l` |

**The single most misleading thing this repository said until this pass: `Material 3 validation` is
red on `main`, and no committed document recorded it.** `HANDOFF.md`, `doc/md3/LocalGates.md` and
`CHANGELOG.md` all described it as green. **And when they finally recorded it, they understated it:**
all three said "red since `fe321a4`", the tip. It has been red since `8762579`, an hour and two
releases earlier — see the row above — so `v7.2.97-ci.106` and `v7.2.97-ci.107` shipped with the gate
already red, and their changelog entries read green by omission until this was corrected. The cause
is known and named: the workflow's "Compact
README contract" step demanded the literal `no verified installer is published yet`, and `README.md`
had correctly stopped saying that once real installers were published, so CI failed with
`Compact README contract missing: no verified installer is published yet`.

**The repair exists but has not been proved by CI.** Commit `ca97ec93ccc06559d1035dd039ef56ca7b000a93`
on this branch replaces the stale assertion with assertions that pin what is true now, and adds
negative guards so the claim cannot be quietly upgraded to a signed installer. It is **not pushed**,
so `main` is still red and will stay red until it is. What has been proved locally, on this host, is
that the repaired step passes: the step was de-indented verbatim out of the YAML into a scratch file
outside the repository and run from the repository root under PowerShell 7.6.5 — exit 0. That run was
repeated at the current branch tip, because the earlier one predated two commits that changed the
step itself; see gate 2 in [`doc/md3/LocalGates.md`](doc/md3/LocalGates.md).

### What this pass actually did

No product source was written. This pass audited every document against the live repository and the
GitHub API and corrected what had gone stale — `HANDOFF.md`, `doc/md3/LocalGates.md`,
`doc/md3/CompletenessInventory.md`, `doc/md3/CaptureMatrix.md`, `doc/md3/WiringAudit.md` and
`CHANGELOG.md`. Two lanes landed immediately before it: the CI contract repair plus the new
`tools/md3/check-language-registry.ps1` localization gate, and the documentation-site language
switcher. **Nothing was compiled — this host has no `kmk`, no configured Qt and no MSVC workload,
and no claim anywhere in this pass rests on a build.**

## The changelog viewer compiles, and shipped

**This is the headline, and it is verified rather than expected.** Run `31840815630` on `bb63f016`
completed **success** after 01:26:08 — the first time `UIMd3Changelog` had ever been compiled by
anything. It published `v7.2.97-ci.101`, non-draft, targeting that commit:

| Evidence | Value |
|---|---|
| Installer | `VirtualBox-7.2.97-Setup.exe`, **106,884,648 bytes** |
| Size delta vs `ci.100` | **+11,910 bytes**, consistent with a ~1,010-line class landing |
| SHA-256 | `bc9f86cbd21885e17837b438414ab4d61f51918e1ef59501be4bc58feb94f7db` |
| Also attached | `SHA256SUMS.txt`, dim-sum photo (Spinach Shrimp Dumpling · 菠菜蝦餃) |

The size arithmetic matters as much as the green tick. A build can go green having quietly packaged
nothing new; an installer that grew by about the right amount for the code added is the cheap second
signal that the artifact really contains the feature.

**Still unproven:** the three features landed after that commit (emoji toggle, dim sum surprise,
personal-vocabulary upload) are compiled by *later* runs, whose verdicts were not in when this was
written. A green verdict for `bb63f016` says nothing about them.

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
| 5 | Hand-written local-suite inventory, every suite passing | **NOT met** | `doc/md3/LocalGates.md` now records 20 gates: 10 runnable and run, **9 blocked on the Windows/kBuild/Qt toolchain, and 1 (gate 18, Doxygen) not blocked at all — simply not attempted.** **Corrected 2026-08-16, twice:** first, the "this container has no `pwsh`" claim in an earlier version of this row was wrong — this host has PowerShell 7.6.5, and gates 2 and 19 were re-run here this pass (exit 0; `RESULT: clean` exit 0). Second, "10 blocked on the toolchain" converted a choice into an impossibility. `doxygen` **is installed on this host** (`LocalGates.md` row 18 resolves it at `/c/Strawberry/c/bin/doxygen`) and no workflow in this repository invokes it; the pass judged a full four-Doxyfile run over the whole product not cheap and did not attempt it. That is a decision, and it is recorded as one. Gates 9–17 remain genuinely unrunnable for want of the Windows/kBuild/Qt toolchain. CI's `md3-validation` is **red on `main`** — see the state section at the top of this file |
| 6 | Installable artifacts built locally and validated | **NOT met — environment** | No Windows toolchain, no Qt, no `kmk`. Artifacts are built by CI only |
| 7 | Original logo and packaged application icon verified in the artifact | **NOT met** | Not audited in this pass; requires the built artifact |
| 8 | Every canonical feature implemented per surface with full evidence | **NOT met** | **39** rows "Not implemented", 18 "Partial"; 7 features have zero implementation anywhere. Counts from `python tools/md3/count-inventory-rows.py` run 2026-08-16, not from memory. See `CompletenessInventory.md` |
| 9 | README and landing page carry a current real-capture matrix | **NOT met** | `doc/md3/CaptureMatrix.md` records **8 of 47 rows captured** (rows 1, 5, 6, 16, 22, 24, 27, 31) and 39 not; still true on 2026-08-16 by `grep -cE '^\| [0-9]+ \|' doc/md3/CaptureMatrix.md` → 47 and the `**Captured**` count → 8. The remaining 39 need a Windows host with a running VM or a registered host service |
| 10 | Exactly one new non-draft release representing this pass | **NOT met** | The workflow publishes a release on every push to `main`, and this pass made several pushes. Intermediate releases exist by design |
| 11 | Dewed `main` has a green remote CI verdict | **NOT met** — and the previous "pending" had it backwards | At `fe321a4` the Windows build is **green** (run `31851996367`, 1h26m24s, published `v7.2.97-ci.108`) and Pages is **green** (run `31851996366`), but **Material 3 validation is red** — run `31851996370` on `main`, plus `31856303192` and `31851966893` on other refs at the same `headSha` — on the stale README assertion. **Red since `8762579`, three commits and three releases ago**, not since `fe321a4`; see the state table at the top. Repaired locally by `ca97ec93ccc06559d1035dd039ef56ca7b000a93`, which is **not pushed**, so this gate stays NOT met until it is |
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

**The table above is itself a record of past errors and its rows are never edited or deleted. The
2026-08-16 pass found four more, and appends them here rather than rewriting the ones above:**

| The 2026-08-14 handoff said | The repository actually showed on 2026-08-16 |
|---|---|
| `main` is `8850ddb7efb248e79dddd697118517d426bb61c4` | `main` is `fe321a4fd6f6bfc7c2fbd7e0704e3f72d0eb2eee` (`git rev-parse origin/main`) |
| Latest release is `v7.2.97-ci.100` | Latest release is `v7.2.97-ci.108` (`gh release list`) |
| Four branches on the remote | **Seven** (`git ls-remote --heads origin`) |
| Local gates: 8 of 8 pass; CI verdict pending | 10 of 20 gates runnable and passing locally, **but `Material 3 validation` is red on `main`** and no document said so |

The last row is the one that mattered this time, for exactly the reason the Squirrel line mattered
last time, only inverted: a reader trusting these documents would have believed CI was green and
gone looking for nothing.

## What is verified

**Superseded 2026-08-14 table, kept for the record.** Every row below was true of commit
`8850ddb7e` when it was written, and every run id in it really did return `success`
(`gh run view <id> --json status,conclusion,headSha`). It is four releases out of date and its
overall implication — that everything is green — is false today. The current table follows it.

| Item | State at `8850ddb7e` | Evidence |
|---|---|---|
| Windows package and release workflow | **green** | run `31784982408` at `8850ddb7e` |
| Material 3 validation | **green** | run `31784982426` at `8850ddb7e` |
| Material 3 documentation Pages | **green** | run `31784982448` at `8850ddb7e` |
| The NSIS switchover run (previously unknown) | **green** | run `31767416361` at `7885a61e6` |
| Published release | **exists, non-draft** | `v7.2.97-ci.100`, target `8850ddb7efb`, published 2026-08-14T10:06:47Z |
| Installer asset | **106,872,738 bytes** | `VirtualBox-7.2.97-Setup.exe`, SHA-256 `766c7e4e…3ba0bf` |
| Installer is the NSIS one | **yes** | built from `out/win.amd64/release/nsis-installer/` |
| Release evidence contract | **complete** | `SHA256SUMS.txt`, dim-sum photo, line-count table, workflow duration `01:24:45` |
| Local gates | **8 of 8 pass** | as recorded then in `doc/md3/LocalGates.md` |

### Verified at `fe321a4` (remote `main`) on 2026-08-16

| Item | State | Evidence |
|---|---|---|
| Windows package and release workflow | **green** | run `31851996367`, `headSha` `fe321a4fd6f…`, 1h26m24s |
| Material 3 documentation Pages | **green** | run `31851996366`, same `headSha` |
| Material 3 validation | **RED** | runs `31856303192`, `31851996370`, `31851966893`. Failing step `Validate MD3 source wiring`; message `Compact README contract missing: no verified installer is published yet` |
| Published release | **exists, non-draft** | `v7.2.97-ci.108`, target `fe321a4fd6f6bfc7c2fbd7e0704e3f72d0eb2eee`, published `2026-08-15T01:21:51Z` |
| Installer asset | **106,963,266 bytes** | `VirtualBox-7.2.97-Setup.exe`, SHA-256 `8a6e1e94bdd73c3a9c526b7b7e074521f06a2562b9f9020ea1a806f15fcda627` |
| Release evidence contract | **complete** | `SHA256SUMS.txt` (95 bytes) and `hk-dish-0009-dried-scallop-shrimp-dumpling.png` (2,436,523 bytes) attached |
| Local gates | **10 of 20 runnable; all 10 pass.** Not 8 of 8 — the inventory grew | `doc/md3/LocalGates.md`. Gates 2 and 19 re-run on this host this pass: `step2 PASSED` exit 0 (0.48s), and `RESULT: clean` exit 0 |
| The red gate's repair | **written, locally proved, NOT pushed** | `ca97ec93ccc06559d1035dd039ef56ca7b000a93` on `claude/yum-tong-finish-20260816`; the repaired step passes locally, CI has never seen it |

## What is NOT true yet, stated plainly

- **No virtual machine can start.** Unchanged and unchangeable here. The host hypervisor driver
  `VBoxSup.sys` is unsigned, 64-bit Windows will not load an unsigned kernel driver, and code
  signing is permanently prohibited for this project. The installer attempts driver installation and
  reports the real per-driver outcome rather than failing silently or claiming success. This is a
  policy consequence, not a defect, and no installer change can resolve it.
- **`VBoxNetAdp6` (host-only networking) and `VBoxNetLwf` (bridged networking) are not installed**
  by the installer. See `doc/installer/WindowsHostInstallerNSIS.md`.
- **The capture matrix is mostly, but not entirely, empty.** `doc/md3/CaptureMatrix.md` carries 39
  rows marked `Not captured` and **8 genuinely captured** (rows 1, 5, 6, 16, 22, 24, 27, 31), each
  uncaptured row naming its blocker. Read that file for the count rather than trusting this one — an
  earlier version of this handoff stated the figure two different ways and got both wrong.
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
  today (`git merge-base --is-ancestor` confirms). **Updated 2026-08-16: a source re-read at
  `fe321a4` found the described defect absent at all four sites the audit names — the reclaim walk
  and its helper are present in `UIVirtualBoxManager.cpp:2653-2672` and
  `UIMachineWindowNormal.cpp:243-254,273-274`, and the ordering argument holds. That is stronger
  than "not known to be open" and weaker than "verified working": nothing was built or launched, so
  whether `Ctrl+G` fires is still undecidable from here. The full verdict, its reasoning and its
  three named residuals are in `WiringAudit.md` §4a's dated re-read block.**
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

- `main` was `5f1adfac63` and clean when this section was written; it is `fe321a4fd6f6bfc7c2fbd7e0704e3f72d0eb2eee`
  as of 2026-08-16. **Check the tree rather than trusting this line** — every previous version of it
  named a commit that had already moved on, which is exactly why the sentence now says so.
- **Corrected 2026-08-16: seven branches exist on the remote, not four.** `git ls-remote --heads origin`
  returns `main`, `codex/native-windows-ci-build-20260809` (pointing at the same commit as `main`),
  `claude/external-editor-20260814`, `claude/full-ui-rewrite-with-ultracode-77b55c`,
  `claude/reality-audit-20260814`, `claude/virtualbox-agent-memory-oabvhw`, and
  `worktree-wf_97c547ae-f89-4`. The list below was written when four were known and is left as it
  was written.
- Four branches existed on the remote at the start of the 2026-08-14 pass:
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

0. **Push `ca97ec93ccc06559d1035dd039ef56ca7b000a93` and get `Material 3 validation` green again.**
   This is the top item because `main` is red today and the fix is already written and locally
   proved. Nothing else in this list should be started before a reader can trust the CI badge.
   Do not close this by weakening the assertion — the repaired step keeps a real contract and adds
   two negative guards against ever claiming a signed installer.

1. ~~Read two build verdicts, and keep them apart.~~ **ANSWERED on 2026-08-16, recorded here rather
   than deleted, because the reasoning the original action set up is what makes the answer readable.**
   The original action said: *the run on `bb63f016` compiles the changelog viewer alone; the run on
   `d548859c` compiles the three features on top of it. Green then red means the three new features
   broke it.*
   - `bb63f016a5da05a9880ea957609695770004763c` → run [`31840815630`](https://github.com/Ding-Ding-Projects/material-virtualbox/actions/runs/31840815630),
     **success**, 1h26m14s. It published `v7.2.97-ci.101`.
   - `d548859c25785ec20d61e56f018ba25da5d2bb70` → run [`31843086131`](https://github.com/Ding-Ding-Projects/material-virtualbox/actions/runs/31843086131),
     **failure**, at step `Build and package the Windows installer`.
   - So by the rule above: green then red, and the three-feature integration commit did break that
     build. **It has since recovered.** `8762579681594cf8ba6beb8ccec77576baa5199a` — the first commit
     containing all three features whose build went green — succeeded as run
     [`31848342345`](https://github.com/Ding-Ding-Projects/material-virtualbox/actions/runs/31848342345)
     and published `v7.2.97-ci.106`, and `fe321a4` succeeded as run `31851996367` and published
     `v7.2.97-ci.108`. **What is proved is that the three features now compile and ship. What is not
     proved is anything about their behaviour: still no test and no capture for any of them.**
2. ~~Confirm the release each successful run publishes.~~ **DONE.** `v7.2.97-ci.101` (target
   `bb63f016a5d`, 106,884,648 bytes), `v7.2.97-ci.106` (target `8762579681594`, 106,902,297 bytes),
   `v7.2.97-ci.107` (target `753602ced18e`, 106,981,357 bytes) and `v7.2.97-ci.108` (target
   `fe321a4fd6f`, 106,963,266 bytes) are all non-draft, all carry `VirtualBox-7.2.97-Setup.exe`
   plus `SHA256SUMS.txt` plus a dim-sum photo. Note that `ci.102`–`ci.105` do not exist; the tag
   counter is the workflow run number, not a release count.
3. **Run the tests and captures these passes skipped**, against the merged tree. Four features —
   the changelog viewer, the emoji setting, the dim sum surprise and the vocabulary upload — now sit
   on `main` with no test and no capture evidence of any kind. That is the largest outstanding debt
   in this repository and it was taken on deliberately, not by accident.
4. ~~Recompute the summary counts table at the foot of `doc/md3/CompletenessInventory.md`.~~ **DONE
   on 2026-08-16**, from `python tools/md3/count-inventory-rows.py` rather than by hand: Implemented
   33, Partial 18, Not implemented 39, N/A 1, total 91. The table had been left at Partial 17 /
   Not implemented 40 after the documentation-site language row moved from "Not implemented" to
   "Partial". Re-run the script after any row's status changes; do not hand-count.
5. **Half done.** The `Ctrl+G` finding was re-read at source level this pass and the verdict is
   recorded in `doc/md3/WiringAudit.md` §4a's dated re-read block: the defect the audit described is
   **absent from the source at all four sites it names**. That is a source re-read, not a runtime
   test — nothing was built, installed, launched or keyboard-tested, and whether `Ctrl+G` actually
   fires remains undecidable without running the application. `v7.2.97-ci.108` is the first shipped
   installer that contains both fixes, so a runtime re-test is now possible for the first time and
   is the remaining half of this action. The three `UsabilityProbe.md` defects were **not**
   re-examined at all and stay open.
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
- `doc/md3/SiteLanguage.md` — the documentation site's three-mode language switcher and its limits.
- `CHANGELOG.md` — each entry linked to its verified commit.
