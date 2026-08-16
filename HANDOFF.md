# Handoff

Last updated: 2026-08-16.

This document was restructured on 2026-08-16 because it had become three passes
layered on top of one another: a current-state section, a superseded state table
sitting above it, the same ceiling stated twice, the changelog viewer narrated
twice, and a `## Repository state` section that opened with a correction and then
printed the stale list the correction was correcting. A reader had to reconstruct
the present from a stack of retractions. It is now one document: **current state,
stated once — then what this branch did — then what remains.** Everything genuinely
useful from the earlier passes is kept, verbatim where it is a record of a past
error, in [History](#history-earlier-passes-kept-for-the-record) at the end.

## Current state

Each row from a command run on 2026-08-16. Where any other document disagrees,
run the command.

| Fact | Value | Command |
|---|---|---|
| Remote `main` | `fe321a4fd6f6bfc7c2fbd7e0704e3f72d0eb2eee` | `git rev-parse origin/main` |
| This working branch | `claude/yum-tong-finish-20260816`. **Nothing on it is pushed.** No commit tally is recorded here — a running count is always wrong by the time it is read | `git rev-parse --abbrev-ref HEAD`, `git log --oneline origin/main..HEAD` |
| Branches on the remote | **seven** — `main`, `codex/native-windows-ci-build-20260809` (= `fe321a4`), `claude/external-editor-20260814`, `claude/full-ui-rewrite-with-ultracode-77b55c`, `claude/reality-audit-20260814`, `claude/virtualbox-agent-memory-oabvhw`, `worktree-wf_97c547ae-f89-4` | `git ls-remote --heads origin` |
| Latest release | **`v7.2.97-ci.108`** — "Dried Scallop Shrimp Dumpling · 瑤柱蝦餃", non-draft, target `fe321a4fd6f6bfc7c2fbd7e0704e3f72d0eb2eee`, published `2026-08-15T01:21:51Z`. **The [Releases page](https://github.com/Ding-Ding-Projects/material-virtualbox/releases) is the authority, not this row** — `README.md` and `ROADMAP.md` deliberately pin no tag at all any more, because both went stale within days of doing so | `gh release view v7.2.97-ci.108 --json tagName,isDraft,targetCommitish,publishedAt` |
| Its installer | `VirtualBox-7.2.97-Setup.exe`, **106,963,266 bytes**, SHA-256 `8a6e1e94bdd73c3a9c526b7b7e074521f06a2562b9f9020ea1a806f15fcda627` | `gh release view v7.2.97-ci.108 --json assets` |
| Its other assets | `SHA256SUMS.txt` (95 bytes), `hk-dish-0009-dried-scallop-shrimp-dumpling.png` (2,436,523 bytes) | same command |
| Windows package and release at `fe321a4` | **success**, run [`31851996367`](https://github.com/Ding-Ding-Projects/material-virtualbox/actions/runs/31851996367), 1h26m24s | `gh run list --commit fe321a4…` |
| Material 3 documentation Pages at `fe321a4` | **success**, run [`31851996366`](https://github.com/Ding-Ding-Projects/material-virtualbox/actions/runs/31851996366) | same command |
| Material 3 validation at `fe321a4` | **FAILURE**, runs [`31851996370`](https://github.com/Ding-Ding-Projects/material-virtualbox/actions/runs/31851996370) (on `main`), [`31856303192`](https://github.com/Ding-Ding-Projects/material-virtualbox/actions/runs/31856303192) (on the `v7.2.97-ci.108` tag ref), [`31851966893`](https://github.com/Ding-Ding-Projects/material-virtualbox/actions/runs/31851966893) (on `codex/native-windows-ci-build-20260809`) — `--commit` matches by `headSha` across every ref | same command |
| How long that gate has been red on `main` | **Since `8762579681594cf8ba6beb8ccec77576baa5199a`**, not since `fe321a4`. Three consecutive failures on `main`: [`31848342250`](https://github.com/Ding-Ding-Projects/material-virtualbox/actions/runs/31848342250) at `8762579` (`22:53:24Z`), [`31851883297`](https://github.com/Ding-Ding-Projects/material-virtualbox/actions/runs/31851883297) at `753602c` (`23:53:35Z`), [`31851996370`](https://github.com/Ding-Ding-Projects/material-virtualbox/actions/runs/31851996370) at `fe321a4` (`23:55:34Z`). Last green on `main`: [`31846486407`](https://github.com/Ding-Ding-Projects/material-virtualbox/actions/runs/31846486407) at `5f1adfac63`. **`ci.106` (target `8762579`) and `ci.107` (target `753602c`) both shipped with the gate already red** | `gh run list --workflow "Material 3 validation" --branch main --limit 15` |
| Releases that exist | ci.96, 97, 98, 99, 100, 101, 106, 107, 108. **ci.102–ci.105 were never created** — the tag counter is the workflow run number, not a release count | `gh release list --limit 10` |
| Feature inventory | **91 rows: 33 Implemented, 18 Partial, 39 Not implemented, 1 justified N/A** | `python tools/md3/count-inventory-rows.py` |

### Why `Material 3 validation` is red, and where the repair is

The workflow's "Compact README contract" step demanded the literal
`no verified installer is published yet`, and `README.md` had correctly stopped
saying that once real installers were published — so CI failed with
`Compact README contract missing: no verified installer is published yet`. A
document that had become *more* honest broke a gate that had not.

Commit `ca97ec93ccc06559d1035dd039ef56ca7b000a93` on this branch replaces the
stale assertion with assertions that pin what is true now, and adds negative
guards so the claim cannot be quietly upgraded to a signed installer. **It is not
pushed, so `main` is still red and will stay red until it is.** What has been
proved locally, on this host, is that the repaired step passes: the step is
de-indented verbatim out of the YAML into a scratch file outside the repository
and run from the repository root under PowerShell 7.6.5 — exit 0, re-run at each
tree that changes the step. See gate 2 in
[`doc/md3/LocalGates.md`](doc/md3/LocalGates.md).

**The single most misleading thing this repository said until this branch: that
gate was red on `main`, and no committed document recorded it.** `HANDOFF.md`,
`doc/md3/LocalGates.md` and `CHANGELOG.md` all described it as green; when they
finally recorded it, all three said "red since `fe321a4`", the tip, when it had
been red since `8762579`, an hour and two releases earlier.

## The ceiling

**No virtual machine can start.** The host hypervisor driver `VBoxSup.sys` is
unsigned, 64-bit Windows will not load an unsigned kernel driver, and code
signing is permanently prohibited for this project. It is a policy consequence,
not a defect, and no installer change can resolve it. Only the machine's owner
choosing to permit unsigned drivers moves it, and that decision has an owner
outside this codebase. The installer attempts driver installation and reports the
real per-driver outcome rather than failing silently or claiming success, which is
the correct behaviour under that ceiling.

This is stated once here. It is also on the three surfaces a newcomer reads —
`README.md`, `ROADMAP.md`, and `docs/index.html` in all three language modes —
and `md3-validation.yml` now pins it on the last two so it cannot be silently
dropped from the published site again.

## What can and cannot be said about running the application

This section replaces two sentences that earlier versions of this file carried in
two different places: *"The application builds, installs, launches and reaches a
working Manager."* **That claim is retracted, and `CHANGELOG.md` retracts it in
its own words at the head of the file.** It is written out here in full so nobody
reintroduces it a fourth time.

- **The Windows pipeline builds VirtualBox 7.2.97 end to end on a cold CI runner
  and publishes a real release** whose installer is the NSIS one — the single
  elevated installer that *can* register COM and install the `VBoxSDS` service.
  That is a build-and-package claim, and it is fully evidenced above.
- **Nothing is claimed about whether any published installer installs, registers
  COM, or reaches a working Manager.** No release — `ci.96` through `ci.101`,
  `ci.106` through `ci.108` — has been downloaded, installed or launched by any
  recorded pass in this repository.
- **This checkout's own build does not reach the manager shell.** It reaches the
  COM boundary and fails with `REGDB_E_CLASSNOTREG`, because no `VBoxSDS` service
  is registered for this checkout. That is blocker B in
  [`doc/md3/CaptureMatrix.md`](doc/md3/CaptureMatrix.md), and it affects rows
  1–39. **No image of that failure is committed anywhere in this repository**; a
  repository-wide search found none, so it is not counted as evidence.
- **Exactly one capture shows a manager shell**, row 1 of the capture matrix,
  taken at worktree tip `cb9f573030e` against an **already-present**
  `%LOCALAPPDATA%\VirtualBox\app-7.2.97\VirtualBox.exe` — not against this
  checkout's build and not against any release asset. In that one build, in that
  one environment, the shell rendered with no COM error. It is one image. It does
  not generalise to the nine releases and it does not lift blocker B.
- **Nothing in this branch was compiled.** This host has no `kmk`, no configured
  Qt and no MSVC workload. No claim on this branch rests on a build.

## What this branch landed

Read this list with `git log --oneline origin/main..HEAD` open; that command,
not a number written here, says how much is waiting to be pushed.

### An executable localization gate, and the real defect it found

`tools/md3/check-language-registry.ps1` (added in `ca97ec93ccc`) parses every
compile-time `registerText` registration under
`src/VBox/Frontends/VirtualBox/` and checks key conflicts, empty keys, empty
English or Cantonese strings, and English strings identical to their Cantonese
counterparts. **Its first run exited 1 on a genuine accessibility defect:**

```
FAIL  CONFLICTING KEY  'md3.wizard.current-page-description' registered 2x with 2 different texts:
```

`src/md3/UIMd3Wizard.cpp:51` registered it as `Title of the active wizard page.`
and `src/wizards/UINativeWizard.cpp:91` as `Current wizard page: %1`.
`registerText` ends in `QHash::insert`, which overwrites — so whichever
`registerMd3*Texts()` free function ran last won, and the other reader was always
wrong: either the native wizard's `.arg(pPage->title())` found no `%1` and
silently dropped the page title from the page stack's accessible description, or
the shell's title label announced a literal `%1`. Fixed in the same commit by
renaming the native wizard's key to `md3.wizard.page-stack-description` at its
registration and its one reader.

Two follow-ups made the gate able to fail rather than able to warn:
`7d5c9dece14` wired CI to pass `-StrictIdentical`, so an untranslated string —
the defect class that most directly means "this was never translated" — fails the
build instead of printing a `WARN` beside a green tick; and the allow-list, which
used to be silent, now prints `identical-allow-listed: N` with a written reason
per entry so an exemption cannot grow invisibly. The gate prints its own blind
spot on every clean run: two call sites in `UIVirtualBoxManager.cpp` take
key/English/Cantonese as runtime lambda parameters and are reported
`unparseable, NOT CHECKED`; CI freezes that count with `-MaxUnparseable 2` so a
third invisible call shape fails the build instead of disappearing.

### Two real workflow defects, fixed

Both in `d0fd94d3e08`, in `.github/workflows/md3-validation.yml`:

- **The evidence file was missing exactly when the news was good.** Both
  evidence-collection jobs ran `git status --short | Set-Content
  artifacts/git-status.txt`. `git status --short` emits nothing on a clean tree,
  and a PowerShell pipeline that yields no object never invokes `Set-Content` —
  so `git-status.txt` was absent precisely when it had the most reassuring thing
  to say, and a reader could not tell "clean tree" from "capture failed". Now
  captured into a variable and written unconditionally.
- **The network-free guard was a subset of its own promise.** It matched
  `<script src|<link |@import|fetch\(|XMLHttpRequest` and missed `url(` (CSS
  background images, cursors, `@font-face src`) and `@font-face` itself. Both
  added, and both proved able to throw before being committed.

### Two shipped WCAG 2.2 fixes on the documentation site

`2048ed0a7b6` added the three-mode (English / Cantonese / Bilingual) language
switcher to `docs/index.html`, mirroring `UIMd3Language`'s mode order, `%1 · %2`
bilingual join, empty-Cantonese fallback and English default. A 48-of-48 headless
browser harness declared it green — and then missed two real defects that an
independent verifier found in the same file. Both were fixed in `6b9be9b978b`:

- **The `role="status"` line was blank on a fresh load.** The IIFE called
  `render(storedMode())` and never the status renderer, so a reader reloading in
  Cantonese got a fully Cantonese page, a correctly checked Cantonese radio, and
  an empty status line until they touched the control. The load-time text is now
  written *without* announcing, by lifting the live-region `role` off the node
  before the first write and restoring it in a `requestAnimationFrame`.
- **The three switcher labels' Chinese halves carried no `lang` attribute** —
  WCAG 2.2 SC 3.1.2 Language of Parts. They are the only CJK visible in *all*
  three modes, including English mode where the page otherwise has zero `zh-HK`
  elements, so an English synthetic voice mispronounced the very control a
  Cantonese-seeking reader is hunting for. Fresh-load `[lang="zh-HK"]` element
  counts, before → after: English **0 → 3**, Cantonese **41 → 45**, bilingual
  **41 → 45**.

Gate 2 now pins both repairs — the three ordering literals, the structural shape
of all three labels, and an exact count of three literal `lang="zh-HK"`
occurrences — so these two cannot regress silently even though the one-off
browser harness, which is not checked in, cannot catch a behavioural regression
at all.

### The CI contract repair

`ca97ec93ccc` — see "Why `Material 3 validation` is red" above. Not pushed.

### Document correctness

`65c0e172c8d`, `7d5c9dece14` and `d0fd94d3e08` audited every document against the
live repository and the GitHub API. The durable rule that came out of it: **for a
fragile derived figure — a byte count, a YAML line range, a character percentage,
a running commit tally — the default action is DELETE, not update.** A sentence
carrying no number cannot rot. Numbers that survive are stable, load-bearing, and
printed next to the command that produces them. `CHANGELOG.md` records what was
deleted and why.

### This pass: the three surfaces a newcomer actually reads

A fresh-eyes review found the documents individually true and collectively
misleading. Repaired here:

- The retracted "reaches a working Manager" claim removed from both places it
  survived in this file, and replaced by the section above.
- `README.md`'s runtime-screenshots section no longer says a launch stops at
  `REGDB_E_CLASSNOTREG` in one paragraph and that the manager shell was reached
  and photographed four lines later. Both observations are stated, each labelled
  with which build it came from, and the withdrawn "retained in the session
  evidence" claim now agrees with `doc/md3/CaptureMatrix.md`.
- `README.md` and `ROADMAP.md` no longer pin `v7.2.97-ci.101` / `bb63f016` as
  "the newest". Neither pins any tag; both point at the Releases page.
- The ceiling added to `README.md`, `ROADMAP.md` and `docs/index.html` — the
  published site had **zero** mentions of `VBoxSup`, kernel drivers, or "cannot
  start a virtual machine" until now. On the site it renders in all three
  language modes with `VBoxSup.sys` written once in the markup and slotted into
  the Cantonese run, so the identifier is byte-identical by construction rather
  than by two copies that can drift. Verified by rendering all three modes in
  headless Chrome and reading the resulting DOM.
- `md3-validation.yml` gained contract literals pinning the ceiling on both
  `README.md` and `docs/index.html`, plus an occurrence-count entry freezing
  `VBoxSup.sys` at one. Each new assertion was proved able to fail — dropping the
  paragraph, adding a second copy of the identifier, un-slotting it, and deleting
  the README blockquote each threw and exited 1 — and the tree restored to exit 0
  afterwards.
- `README.md` now links `doc/md3/CompletenessInventory.md` and
  `doc/md3/LocalGates.md`, which it never did, and states the inventory scale
  beside the implementation-status blockquote instead of listing ~20 wired
  surfaces with no adjacent statement of how much is not done.

## What is NOT true yet, stated plainly

- **No virtual machine can start.** See [the ceiling](#the-ceiling). Unchanged and
  unchangeable here.
- **`VBoxNetAdp6` (host-only networking) and `VBoxNetLwf` (bridged networking) are
  not installed** by the installer. See
  `doc/installer/WindowsHostInstallerNSIS.md`.
- **`Material 3 validation` is red on `main`** and the repair is unpushed.
- **The capture matrix is mostly empty.** `doc/md3/CaptureMatrix.md` is the
  authority for the count; do not restate it from memory, and note that its
  runtime-window rows are ceilinged rather than merely unattempted.
- **Many canonical features remain absent or partial.**
  `doc/md3/CompletenessInventory.md` is the authority and is hand-written on
  purpose. Re-run `python tools/md3/count-inventory-rows.py` rather than
  hand-counting.
- **Four features on `main` have no test and no capture of any kind** — the
  changelog viewer, the emoji setting, the dim sum surprise and the vocabulary
  upload. That debt was taken on deliberately, not by accident.

## Release-criteria audit

The release-grade shutdown contract has fourteen non-negotiable gates and **it is
not satisfied**. Several are blocked by this environment rather than by the
repository, and those are marked as such.

| # | Gate | State | Evidence or blocker |
|---|---|---|---|
| 1 | Full inventory of jers, worktrees, stashes, tags, releases, divergence | **met** | Recorded in "Current state" above and in [History](#history-earlier-passes-kept-for-the-record) |
| 2 | Every valid change committed on its owning jer | **met** | All worktrees clean, nothing excluded silently |
| 3 | Recoverable work pushed before integration | **met** | Every lane pushed before any deletion |
| 4 | Remote not ahead of the working jer | **met** | `main` fast-forwarded; no Fay Gay encountered |
| 5 | Hand-written local-suite inventory, every suite passing | **NOT met** | `doc/md3/LocalGates.md` records 20 gates: 10 runnable and run, **9 blocked on the Windows/kBuild/Qt toolchain, and 1 (gate 18, Doxygen) not blocked at all — simply not attempted.** `doxygen` **is installed on this host** and no workflow in this repository invokes it; the pass judged a full four-Doxyfile run over the whole product not cheap and did not attempt it. That is a decision, and it is recorded as one. Gates 9–17 remain genuinely unrunnable for want of the toolchain. CI's `md3-validation` is **red on `main`** |
| 6 | Installable artifacts built locally and validated | **NOT met — environment** | No Windows toolchain, no Qt, no `kmk`. Artifacts are built by CI only |
| 7 | Original logo and packaged application icon verified in the artifact | **NOT met** | Not audited; requires the built artifact |
| 8 | Every canonical feature implemented per surface with full evidence | **NOT met** | **39** rows "Not implemented", 18 "Partial"; 7 features have zero implementation anywhere. Counts from `python tools/md3/count-inventory-rows.py`, not from memory |
| 9 | README and landing page carry a current real-capture matrix | **NOT met** | `doc/md3/CaptureMatrix.md` is the authority; most rows are uncaptured and each names its blocker. The remaining rows need a Windows host with a running VM or a registered host service, and the runtime-window rows are ceilinged permanently |
| 10 | Exactly one new non-draft release representing this pass | **NOT met** | The workflow publishes a release on every push to `main`, and earlier passes made several pushes. Intermediate releases exist by design |
| 11 | Dewed `main` has a green remote CI verdict | **NOT met** | Windows build **green** and Pages **green** at `fe321a4`; **Material 3 validation red**, and red since `8762579` — three commits and three releases before `fe321a4`. Repaired locally by `ca97ec93ccc`, **not pushed**, so this gate stays NOT met until it is |
| 12 | Every source jer tip proved an ancestor before deletion | **met** | Proved for all seven deleted items |
| 13 | Fresh mat day supplied before any deletion | **met** | Supplied; applied to the cleanup half only |
| 14 | Only `main` and the primary checkout remain | **NOT met** | Six merged branches remain on the remote (`git ls-remote --heads origin` returns seven heads; all six non-`main` heads are proved ancestors of `origin/main`). Cleanup is pending, not blocked by ancestry |

**7 of 14 gates met.** Gates 6, 7 and 9 need a Windows host and cannot be closed
from here at all. Gate 14 needs one permission. Gates 5, 8 and 10 need real work
in the repository. None should be marked done by weakening the gate.

## Next actions, in order

0. **Push this branch and get `Material 3 validation` green again.** `main` is red
   today and the fix is already written and locally proved. Run
   `git log --oneline origin/main..HEAD` to see exactly what is waiting; do not
   quote a count from any document, including this one. Nothing else in this list
   should be started before a reader can trust the CI badge. Do not close this by
   weakening the assertion — the repaired step keeps a real contract and adds
   negative guards against ever claiming a signed installer.
1. **Run the tests and captures these passes skipped**, against the merged tree.
   Four features now sit on `main` with no test and no capture evidence of any
   kind. That is the largest outstanding debt in this repository.
2. **Re-test `Ctrl+G` at runtime.** A source re-read at `fe321a4` found the
   `WiringAudit.md` defect **absent from the source at all four sites it names**
   (`doc/md3/WiringAudit.md` §4a's dated re-read block). That is a source
   re-read, not a runtime test — nothing was built, installed, launched or
   keyboard-tested. `v7.2.97-ci.108` is the first shipped installer containing
   both fixes, so a runtime re-test is now possible for the first time. The three
   `UsabilityProbe.md` defects were **not** re-examined at all and stay open.
3. **Work through `doc/md3/CompletenessInventory.md` and
   `doc/md3/CaptureMatrix.md`.** Both are deliberate, honest gap lists rather than
   checklists of what already exists.
4. **Make the browser harness a real gate.** The 48-check headless harness behind
   gate 20 is a one-off that is not checked in, so a regression in switcher
   *behaviour* — as opposed to the source literals gate 2 pins — would not be
   caught by anything.
5. **Leave the unsigned-driver ceiling alone** unless the machine's owner decides
   to permit unsigned drivers. Nothing in this repository can move it.

## Documentation

- `doc/installer/WindowsHostInstallerNSIS.md` — the single Windows installer: what
  it registers, what it cannot do unsigned, silent install, uninstall, failure
  modes.
- `doc/md3/LocalGates.md` — every locally runnable gate, its command and real
  result, and the gates this project should have and does not.
- `doc/md3/CompletenessInventory.md` — per-surface feature inventory.
- `doc/md3/CaptureMatrix.md` — every surface that must be captured, and why each
  uncaptured row is not captured.
- `doc/md3/CaptureHarness.md` — the runtime capture harness and how it validates
  its own output.
- `doc/md3/WiringAudit.md` — what the Material 3 manager is really wired to,
  traced to file and line.
- `doc/md3/UsabilityProbe.md` — 14 captures from driving the installed
  application.
- `doc/md3/Changelog.md` — the in-app changelog viewer.
- `doc/md3/SiteLanguage.md` — the documentation site's three-mode language
  switcher and its limits.
- `CHANGELOG.md` — each entry linked to its verified commit.

---

# History: earlier passes, kept for the record

Nothing below is current state. It is kept because a record of what a document
got wrong is worth more than a clean document, and because two feature rounds
landed real code whose limits are recorded nowhere else.

## Round one: the changelog viewer, and its first compile

An ultra-speed feature pass: one feature, its directly related records,
integration and a release. **No tests and no captures were run, by that pass's own
rule.**

The in-app changelog viewer (`UIMd3Changelog`) reached `main`, so it ships in a
real release instead of sitting on a side jer. Before integrating, three
independent reviewers checked the never-compiled C++ against working sibling
sources in `src/md3/` — one on build wiring and includes, one on Qt API
signatures and declared Qt modules, one on the singleton/lifecycle pattern. All
three returned no build blockers. **That was a static review, not a
compilation.**

Run `31840815630` on `bb63f016` then completed **success** after 01:26:08 — the
first time `UIMd3Changelog` had ever been compiled by anything — and published
`v7.2.97-ci.101`, non-draft, targeting that commit, with an installer of
106,884,648 bytes: **+11,910 bytes** against `ci.100`, consistent with a
~1,010-line class landing. The size arithmetic mattered as much as the green
tick: a build can go green having quietly packaged nothing new.

## Round two: three previously-absent features

A second ultra-speed round implemented three canonical features that
`doc/md3/CompletenessInventory.md` recorded as **"Not implemented on any
surface"** — each written in its own isolated worktree, each statically
compile-guarded, none compiled or tested at the time.

| Feature | Class | Inventory section |
|---|---|---|
| Emoji-in-dialogs toggle | `UIMd3EmojiSetting` | 3 |
| Dim sum startup surprise | `UIMd3DimSumSurprise` | 7 |
| Personal-vocabulary JSON upload | `UIMd3PersonalVocabulary` | 20 |

**Each carries an honest limit, recorded in its own inventory row rather than
glossed:**

- The emoji setting is infrastructure only. No dialog in the frontend calls its
  decorate helper yet, so enabling it changes nothing visible except the palette
  row's own label. Retrofitting the real message boxes is the follow-up.
- The dim sum surprise ships **no photograph, deliberately.** Images belong to a
  separate public catalog and must never be vendored, generated, or downloaded
  into this repository, so the toast renders an explicit "photo not included in
  this build" placeholder. That is the correct implementation, not a shortfall.
- The vocabulary service is real and validated, but no other surface routes its
  text through it yet, so a valid file currently has no visible effect outside the
  control's own status line.

### One risk was investigated rather than shipped on a hunch

A reviewer flagged that the emoji literals are astral-plane (4-byte UTF-8)
characters, that `Config.kmk` sets no MSVC `/utf-8`, and that no existing frontend
source contains any astral-plane character — so there was no precedent. That was
checked before integrating rather than left to a 90-minute build: **22 non-ASCII
sources already compile green with no byte-order mark**, including
`UIMd3Language.cpp`'s Cantonese, and the new files match that precedent exactly.
How the compiler decodes the file does not change between 3-byte and 4-byte
sequences, so this is a runtime-rendering question that already applies to every
Cantonese string in the tree, not a build blocker. If a future build does fail on
source encoding, that is where to look first, and the fix is tree-wide.

### The build verdicts these two rounds produced

- `bb63f016a5da05a9880ea957609695770004763c` → run
  [`31840815630`](https://github.com/Ding-Ding-Projects/material-virtualbox/actions/runs/31840815630),
  **success**, 1h26m14s, published `v7.2.97-ci.101`.
- `d548859c25785ec20d61e56f018ba25da5d2bb70` → run
  [`31843086131`](https://github.com/Ding-Ding-Projects/material-virtualbox/actions/runs/31843086131),
  **failure**, at step `Build and package the Windows installer`. Green then red:
  the three-feature integration commit did break that build.
- **It has since recovered.** `8762579681594cf8ba6beb8ccec77576baa5199a` — the
  first commit containing all three features whose build went green — succeeded as
  run [`31848342345`](https://github.com/Ding-Ding-Projects/material-virtualbox/actions/runs/31848342345)
  and published `v7.2.97-ci.106`; `fe321a4` succeeded as run `31851996367` and
  published `v7.2.97-ci.108`. **What is proved is that the three features now
  compile and ship. What is not proved is anything about their behaviour.**

## The reality audit's findings, and why they are leads rather than defects

`doc/md3/WiringAudit.md` and `doc/md3/UsabilityProbe.md` are the only evidence in
this repository produced by *driving* the built application rather than reading
its source, and `UsabilityProbe.md` carries 14 captures pinned to individual
claims. **Read them with their commit dates in hand, because both audited older
code than `main` now carries:**

- `WiringAudit.md` audited `0d9eda43cd0`. Its most serious finding — that `Ctrl+G`
  not opening Global Preferences is a real regression, systemic across three sites
  in the VM Runtime window — was named **before** the fixes `66c701d6` and
  `90aedcff` landed. Both are ancestors of `main` today. A source re-read at
  `fe321a4` found the described defect absent at all four sites the audit names;
  the reclaim walk and its helper are present in
  `UIVirtualBoxManager.cpp:2653-2672` and
  `UIMachineWindowNormal.cpp:243-254,273-274`. That is stronger than "not known to
  be open" and weaker than "verified working".
- `UsabilityProbe.md` drove an installed binary dated 2026-08-10, which the
  document itself flags as "an unknown number of commits behind". Its three ugly
  findings — the Create VM link opening the *import* wizard, a file row opening a
  mismatched submenu, and the remove-confirmation overlay not responding to clicks
  — are recorded with captures and are **unverified against the current
  installer**.

Its genuinely durable contributions, independent of build age:

- New VM and Global Preferences are both fully functional and reachable **through
  the command palette**, which overturns the previous lane's "needs interactive
  re-verification" hedge.
- Machines/Media/Network being disabled is **not a defect** — it is upstream
  VirtualBox behaviour faithfully reproduced, including the upstream route out of
  the restriction.
- The method is worth reusing: give a control real keyboard focus, `Tab` to it,
  **capture the focus ring**, and only then judge whether a mouse click at that
  same pixel does the same thing. That is what proved the click targeting — not
  the coordinate math — was at fault.

## Repository state at the 2026-08-14 integration

- Four branches existed on the remote at the start of the 2026-08-14 pass. Seven
  exist today; see the current-state table.
  - `main`
  - `claude/external-editor-20260814` — **already merged**, proved an ancestor of
    `main`.
  - `claude/reality-audit-20260814` — was unmerged, **now integrated** (2
    commits).
  - `worktree-wf_97c547ae-f89-4` — was unmerged, **now integrated** (1 commit),
    and this one mattered: it carried a complete, unlanded in-app changelog
    viewer, `UIMd3Changelog` (~1,010 lines) with the date filter, text search and
    per-entry commit links the house contract requires, plus its manager wiring,
    lifecycle and build entry. A cleanup pass that deleted merged branches without
    reading them would have thrown a whole feature away.
- Merged task branches and their worktrees were removed after each tip was proved
  an ancestor of the pushed `main`. No stashes existed at any point.

## The correction ledger

**These rows are a record of past errors. They are never edited or deleted, only
appended to.**

| The 2026-08-13 handoff said | The repository actually showed on 2026-08-14 |
|---|---|
| `main` is `7885a61e681` | `main` is `8850ddb7efb248e79dddd697118517d426bb61c4` |
| Latest release is `v7.2.97-ci.96` | Latest release is `v7.2.97-ci.100` |
| **The published release ships the Squirrel installer** | The published release ships the **NSIS** installer |
| Two branches on the remote | Four branches on the remote |
| CI run 31767416361's verdict is unknown | It completed **success** |

The Squirrel line is the one that mattered: it was the headline defect of the
previous pass, it was already fixed by the time the file was written, and anybody
reading it would have gone looking for a problem that no longer existed.

| The 2026-08-14 handoff said | The repository actually showed on 2026-08-16 |
|---|---|
| `main` is `8850ddb7efb248e79dddd697118517d426bb61c4` | `main` is `fe321a4fd6f6bfc7c2fbd7e0704e3f72d0eb2eee` (`git rev-parse origin/main`) |
| Latest release is `v7.2.97-ci.100` | Latest release is `v7.2.97-ci.108` (`gh release list`) |
| Four branches on the remote | **Seven** (`git ls-remote --heads origin`) |
| Local gates: 8 of 8 pass; CI verdict pending | 10 of 20 gates runnable and passing locally, **but `Material 3 validation` is red on `main`** and no document said so |

The last row is the one that mattered that time, for exactly the reason the
Squirrel line mattered before, only inverted: a reader trusting these documents
would have believed CI was green and gone looking for nothing.

| This document said, in three places, until 2026-08-16 | What is actually recorded |
|---|---|
| "The application builds, installs, launches and reaches a working Manager" (twice, once under `## Current state, in one line`) | `CHANGELOG.md` retracts exactly that: no release has been downloaded, installed or launched by any recorded pass. See "What can and cannot be said about running the application" |
| "No product source was written… this pass audited every document" | The branch carries an executable localization gate that found a real duplicate-key accessibility defect, two workflow defect fixes, and two shipped WCAG 2.2 fixes. See "What this branch landed" |
| "Push one commit" as Next action 0 | Eight commits were unpushed when that was written. The action now names `git log --oneline origin/main..HEAD` instead of a count |
| A superseded 2026-08-14 "What is verified" table sat **above** the current one | Moved below, into this history section |

## Superseded "What is verified" table, 2026-08-14

Every row below was true of commit `8850ddb7e` when it was written, and every run
id in it really did return `success`
(`gh run view <id> --json status,conclusion,headSha`). It is four releases out of
date and its overall implication — that everything is green — is false today. The
current verdicts are in the state table at the top of this file.

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
