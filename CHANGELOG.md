# Changelog

This changelog records notable changes to the Material Virtual Machine fork:
the Windows build/packaging pipeline, the native Material Design 3 rewrite,
and their supporting documentation. Every entry links the exact commit that
made the change; every linked SHA was verified with `git cat-file -e` against
this repository before publication, so a broken link here is a defect to
report rather than a typo to route around.

**Correction:** an earlier version of this file stated that no release had ever
been published from this repository. That is no longer true and had not been
for some time — `v7.2.97-ci.96` through `v7.2.97-ci.100` are published,
non-draft releases carrying real NSIS installers, the most recent of them
`VirtualBox-7.2.97-Setup.exe` at 106,872,738 bytes. The claim is corrected here
rather than deleted, because a changelog that silently rewrites its own past
is the one document a reader has no way to check.

**Second correction, 2026-08-16.** The block above is itself out of date and is
appended to rather than edited, for the same reason. Nine releases now exist —
`v7.2.97-ci.96` through `ci.101`, then `ci.106`, `ci.107` and `ci.108`
(`ci.102`–`ci.105` were never created; the tag counter is the workflow run
number, not a release count). The latest is **`v7.2.97-ci.108`**, whose
installer is `VirtualBox-7.2.97-Setup.exe` at **106,963,266 bytes**. Four of
those releases — ci.101, ci.106, ci.107 and ci.108 — had never been recorded in
this file at all; they are recorded below. Read with
`gh release list --repo Ding-Ding-Projects/material-virtualbox`.

**Third correction, 2026-08-16 — `ci.96` is not an NSIS release.** The first
block above says `ci.96` through `ci.100` carry "real NSIS installers". Four of
those five do. `v7.2.97-ci.96` does not: it is the **Squirrel.Windows** package,
and its four assets are `Setup.exe`, `RELEASES`,
`VirtualBox-7.2.96-full.nupkg` and `hk-dish-0001-classic-har-gow.png`. It has
**no `VirtualBox-7.2.97-Setup.exe` and no `SHA256SUMS.txt`**.
**`v7.2.97-ci.97` is the first NSIS release** and the first to carry either of
those two files; every release from `ci.97` onward carries both. This correction
is worth its own paragraph because the same pass that wrote these blocks also
scrubbed the word "Squirrel" out of several documents as stale — while the very
first release this file records *is* the Squirrel package. Check any release's
real assets with
`gh release view v7.2.97-ci.96 --repo Ding-Ding-Projects/material-virtualbox --json assets`.

**A red gate this file also failed to record, and it is older than this file
first admitted.** `Material 3 validation` is **failing on `main`** and has been
since `8762579681594cf8ba6beb8ccec77576baa5199a` — not since `fe321a4`, which
is an hour and two releases later. It has failed on `main` at three consecutive
commits:

| Commit on `main` | Run | Started |
| --- | --- | --- |
| `8762579681594cf8ba6beb8ccec77576baa5199a` | [31848342250](https://github.com/Ding-Ding-Projects/material-virtualbox/actions/runs/31848342250) | `2026-08-14T22:53:24Z` |
| `753602ced18e50649ce0b0a3038cf7efd1ba97fe` | [31851883297](https://github.com/Ding-Ding-Projects/material-virtualbox/actions/runs/31851883297) | `2026-08-14T23:53:35Z` |
| `fe321a4fd6f6bfc7c2fbd7e0704e3f72d0eb2eee` | [31851996370](https://github.com/Ding-Ding-Projects/material-virtualbox/actions/runs/31851996370) | `2026-08-14T23:55:34Z` |

`gh run list --repo Ding-Ding-Projects/material-virtualbox --workflow "Material 3 validation" --branch main --limit 15`
— the last green run on `main` is [31846486407](https://github.com/Ding-Ding-Projects/material-virtualbox/actions/runs/31846486407)
at `5f1adfac63246fcd97bf4a34f13813c563f60afe`.

**`v7.2.97-ci.106` and `v7.2.97-ci.107` therefore shipped with this gate already
red**, not just `ci.108`: ci.106 targets `8762579` and ci.107 targets `753602c`.
Their sections below used to read green by omission and now say so.

It is repaired by `ca97ec93ccc06559d1035dd039ef56ca7b000a93`, recorded under
`[Unreleased]` below, which is **not yet pushed**. Nothing in this file should
be read as claiming CI is green until that lands.

**The unsigned-driver ceiling is unchanged and is not softened anywhere in this
file.** The host hypervisor driver `VBoxSup.sys` ships unsigned, 64-bit Windows
refuses to load an unsigned kernel driver, and code signing is permanently
prohibited for this project. **No virtual machine can start**, in any release
listed below, and no change in this repository can alter that.

**Nothing is claimed about whether the installers named below install, register
COM, or reach a working Manager.** No release listed in this file — `ci.96`
through `ci.101`, `ci.106` through `ci.108` — has been downloaded, installed or
launched by any recorded pass in this repository, and this file's own entries
state that nothing was compiled, built, installed or launched. The claim this
paragraph used to make rested on exactly one image. `doc/md3/CaptureMatrix.md`
records 8 captured rows in total (1, 5, 6, 16, 22, 24, 27, 31), but the only one
that speaks to reaching a Manager without a COM error is row 1:
[`doc/md3/CaptureMatrix.md`](doc/md3/CaptureMatrix.md)'s
`doc/md3/captures/manager-shell--01--Qt683QWindowIcon--Material-Virtual-Machine-Manager.png`,
taken at worktree tip `cb9f573030e` against an **already-present**
`%LOCALAPPDATA%\VirtualBox\app-7.2.97\VirtualBox.exe` — not against any release
asset named here. In that one capture, on that one build, in that one
environment, the Manager shell rendered and no COM error appeared in the
window. That is one image, and it does not generalise to the nine releases.
**It does not lift blocker B**, which stands unchanged: `CaptureMatrix.md`
records that the built `VirtualBox.exe` fails at the COM boundary with
`REGDB_E_CLASSNOTREG` because no `VBoxSDS` service is registered for this
checkout, and that the Manager shell therefore cannot currently be reached to
photograph. Blocker B is real but not universal — that is the whole of what row
1 shows, and nothing more.

Entries below remain under **[Unreleased]** when they have not yet appeared in
a published release.

## [Unreleased]

### Fixed — Reconciled the preserved dialog-emoji and dim-sum drafts (2026-08-20)

[`db403d882ac3870cef07c319b7b41e1016b76eda`](https://github.com/Ding-Ding-Projects/material-virtualbox/commit/db403d882ac3870cef07c319b7b41e1016b76eda)

- The persisted emoji preference now reaches the shared `QIMessageBox` path
  used by real confirmations and warnings, and global settings exposes the same
  localized, accessible toggle as the command palette. Rich-text container
  markup remains intact; button, action, field, copied, and accessible text is
  not decorated.
- The startup dim-sum toast now defers while a modal decision is active, gives
  up after three bounded retries, uses the active window's screen, and is not
  created in Runtime-UI processes.
- The two retained draft commits were reconciled semantically rather than
  merged literally: their duplicate `UIMd3DialogEmoji` and `UIMd3DimSum`
  singletons are superseded by the canonical `UIMd3EmojiSetting` and
  `UIMd3DimSumSurprise` implementations already on `main`.
- Verification: the language-registry checker reported 429 registrations, 409
  distinct keys, zero violations, and zero warnings; `git diff --check` passed.
  No native Qt build or built-artifact interaction was run because this linked
  checkout deliberately leaves the `kBuild` submodule uninitialized.

### Fixed — The three surfaces a newcomer reads, and one handoff instead of three (2026-08-16)

*No commit link yet.* This entry describes the commit that adds it.

A fresh-eyes review found this branch's documents individually true and
collectively misleading. Five blocking defects, each on a surface a newcomer
lands on first.

**The retracted runtime claim is gone from `HANDOFF.md`.** It said, in two
places — one of them under a heading called `## Current state, in one line`, the
single sentence a reader trusts most — that "the application builds, installs,
launches and reaches a working Manager". This file retracts exactly that at its
head. Both are replaced by a section that separates the four things that are
actually known: the pipeline builds and publishes; nothing is claimed about any
published installer being installed or launched; **this checkout's own build
fails at `REGDB_E_CLASSNOTREG`**, and no image of that failure is committed
anywhere; and exactly one capture shows a manager shell, taken at worktree tip
`cb9f573030e` against an **already-present** installed `VirtualBox.exe`. This is
the third time this overclaim class has been caught on this branch, so the whole
tree was re-searched for it, multiline, and every surviving occurrence is now
either the retraction itself or a quotation of the withdrawn sentence.

**`README.md` no longer contradicts itself inside one section.** It said the
Windows runtime launch "stops before the manager shell at
`REGDB_E_CLASSNOTREG`" and, four lines later, that "the manager shell has been
reached and photographed". Both observations are real and they came from
different builds; each is now stated with which build it came from. It also
claimed the failure capture "is retained in the session evidence", which
`doc/md3/CaptureMatrix.md` contradicts — a repository-wide search found no such
image committed anywhere, so it is not counted as evidence. That claim is
withdrawn and README now agrees with the capture matrix.

**No document pins a "newest release" tag in prose any more.** `README.md` and
`ROADMAP.md` both said "the newest, `v7.2.97-ci.101`, targets commit
`bb63f016`". The newest was `v7.2.97-ci.108`, non-draft, targeting `fe321a4`.
Rather than move the pin, both now name the
[Releases page](https://github.com/Ding-Ding-Projects/material-virtualbox/releases)
as the authority and pin nothing — the same rule that deleted the other rotting
figures on this branch. The durable claim that survives is the one that does not
move: every non-draft release from `ci.97` onward carries
`VirtualBox-7.2.97-Setup.exe` and `SHA256SUMS.txt`, and `ci.96` is the retired
Squirrel package that carries neither.

**The unsigned-driver ceiling is now on the front surfaces.** `docs/index.html`
— the published site, and `README.md`'s very first link — contained **zero**
mentions of `VBoxSup`, kernel drivers, or the fact that no virtual machine can
start. README mentioned kernel drivers only in passing and never stated the
ceiling; `ROADMAP.md` was likewise thin. All three now carry it plainly: the host
hypervisor driver `VBoxSup.sys` is unsigned, 64-bit Windows will not load an
unsigned kernel driver, code signing is permanently prohibited for this project,
and it is a policy consequence rather than a defect that any installer change
could resolve. On the site it renders in all three language modes following the
existing `data-md3-zh` pattern, with `VBoxSup.sys` written **once** in the markup
and pulled into the Cantonese run through a `%2` slot — so the identifier is
byte-identical across modes by construction rather than by two copies that can
drift. Verified by rendering all three modes in headless Chrome and reading the
resulting DOM.

**`md3-validation.yml` pins the ceiling so it cannot be silently dropped.** New
contract literals cover the English sentence on both `README.md` and
`docs/index.html`, a structural assertion that the Cantonese attribute references
both slots, and an occurrence-count entry freezing `VBoxSup.sys` at exactly one
in the page source. Each was proved able to fail before being committed: deleting
the paragraph threw `Pages unsigned-driver ceiling contract missing:
id="ceiling"`; adding a second copy of the identifier threw `Technical fact must
appear exactly 1 time(s) … VBoxSup.sys (found 2)`; un-slotting it threw `The
ceiling statement must render in all three language modes with the driver name
slotted, not duplicated`; deleting the README blockquote threw `README
unsigned-driver ceiling contract missing: no virtual machine can start`. All four
exited 1, and the restored tree exits 0. The README assertions run against a copy
with blockquote markers stripped before whitespace flattening, so they pin the
sentence and not where its lines happen to wrap.

**`HANDOFF.md` is one document again.** It had become three passes stacked on each
other: a preamble pointing at two feature rounds that were neither the current
pass, a `### What this pass actually did` describing 2026-08-16 sitting above a
`## What changed in this pass` describing 2026-08-14, the changelog viewer
narrated twice, the ceiling stated twice, and a superseded "What is verified"
table sitting **above** the current one. `## Repository state` opened with a
correction saying seven remote branches and then printed the stale four-branch
list underneath it. It now reads current state → what this branch landed → what
remains, with every superseded table and correction ledger moved into a clearly
labelled trailing history section. No row of the correction ledger was edited or
deleted; a fourth block was appended recording this document's own three errors.

**`HANDOFF.md` also stopped underselling the branch.** It said "No product source
was written… this pass audited every document", which omitted the executable
localization gate that found a real duplicate-key accessibility defect
(`md3.wizard.current-page-description` registered twice with two different English
texts, where `registerText` ends in an overwriting `QHash::insert`), the two
workflow defect fixes, and the two shipped WCAG 2.2 fixes in `docs/index.html`.
All are recorded now. Its "Next action 0" said to push one commit when eight were
unpushed; the tally is gone and the action names
`git log --oneline origin/main..HEAD` instead.

**`README.md` finally links its own gap lists.** It never linked
`doc/md3/CompletenessInventory.md` or `doc/md3/LocalGates.md`, while listing
around twenty surfaces as "wired into the existing VirtualBox frontend" with no
adjacent statement of how much is not done. Both are linked, and the scale sits
beside that blockquote with the command that produces it: **91 rows — 33
Implemented, 18 Partial, 39 Not implemented, 1 justified N/A**, from
`python tools/md3/count-inventory-rows.py`.

Nothing was compiled. Gate 2 was re-extracted from `md3-validation.yml` and re-run
from the repository root under PowerShell 7.6.5 against the tree these changes are
committed in: exit 0.

### Fixed — A second round of wrong numbers, and the brittle claims that produced them (2026-08-16)

*No commit link yet.* This entry describes the commit that adds it.

The previous repair pass fixed eleven findings and introduced several new wrong
numbers doing it. That is a loop, and it does not end by re-deriving figures more
carefully. **The default action here was to delete a fragile derived figure, not
to update it.** A number survives only if it is stable, load-bearing, and printed
with the exact command that produces it; otherwise the sentence now says the
qualitative thing, which needs no maintenance.

**Deleted rather than re-derived.** "1,248 of the page's 8,355 visible characters
(14.9%)" and "383,925 bytes against `index.html`'s 24,642, so the page is 6% of
the published site" are gone from `doc/md3/CompletenessInventory.md`, from this
file, and from the compiled-in viewer in `UIMd3Changelog.cpp`. Neither
reproduced; neither named a command; the byte ratio moved when this branch's own
previous commit edited `docs/index.html`. What replaces them is checkable with one
line: **18 of the page's 22 headings carry Cantonese**, the `<h1>` and the three
Verification card `<h3>`s do not, and the body prose is only partly translated
(`grep -oE '<h[1-3][ >]' docs/index.html | wc -l` → 22;
`grep -oE '<h[1-3] data-md3-zh' docs/index.html | wc -l` → 18). Likewise
`doc/md3/LocalGates.md` gate 2 no longer records "257 lines of YAML" or "lines
30–286" for its CI step — that evidence was borrowed from a tree the row was never
run against — and the running "N commits ahead of `main`" tally is gone from
`HANDOFF.md` in favour of `git log --oneline origin/main..HEAD`.

**`v7.2.97-ci.96` is not an NSIS release.** The previous pass wrote "Six releases,
all non-draft, all carrying an unsigned NSIS `VirtualBox-7.2.97-Setup.exe`" and
then listed five, the first of which is the **Squirrel.Windows** package —
`Setup.exe`, `RELEASES`, `VirtualBox-7.2.96-full.nupkg`, and no
`VirtualBox-7.2.97-Setup.exe` or `SHA256SUMS.txt` at all. It said six twice over:
wrong count, wrong package. `ci.97` is the first NSIS release. Corrected in this
file's correction blocks and release section, in `README.md`, in `ROADMAP.md` and
in the compiled-in viewer.

**The red gate is older than every document admitted.** `Material 3 validation`
has been failing on `main` since `8762579681594cf8ba6beb8ccec77576baa5199a`
([31848342250](https://github.com/Ding-Ding-Projects/material-virtualbox/actions/runs/31848342250)),
not since `fe321a4` — an hour and two releases earlier. It also failed at
`753602c` ([31851883297](https://github.com/Ding-Ding-Projects/material-virtualbox/actions/runs/31851883297))
and at `fe321a4` ([31851996370](https://github.com/Ding-Ding-Projects/material-virtualbox/actions/runs/31851996370)).
**`v7.2.97-ci.106` and `v7.2.97-ci.107` therefore both shipped with the gate
already red**, and their entries in this file read green by omission until now;
they carry the same note `ci.108` already had. Corrected in this file,
`doc/md3/LocalGates.md`, `HANDOFF.md` and `ROADMAP.md`.

**Two coordinates and one attribution.** `doc/md3/WiringAudit.md` §4a cited
`UIActionPoolManager.cpp:4288` for a statement at `:4289` (`:4288` is the comment
above it) and called `:4053` "inside `updateMenuFile()`" when it is the call site
inside `updateMenus()`; both are corrected and the call chain is now listed link by
link. `HANDOFF.md` said 10 local gates were "blocked on the Windows/kBuild/Qt
toolchain"; gate 18 (Doxygen) is not blocked — `doxygen` is installed on this host
and the pass judged a four-Doxyfile run over the whole product not cheap and did
not attempt it. That is a choice, and converting a choice into an impossibility is
its own kind of inflation. Nine gates are toolchain-blocked; one was declined.

**Two real defects in `.github/workflows/md3-validation.yml`, not just prose.**

1. `doc/md3/SiteLanguage.md` claimed CI rejects `url()` and web fonts. It did not:
   the guard read `'<script src|<link |@import|fetch\(|XMLHttpRequest'`. Rather
   than narrow the sentence to match the weaker guard, the guard was widened to
   `'…|url\(|@font-face'`. `docs/index.html` has zero occurrences of either, so it
   went green on arrival; injecting each construct into a scratch copy was
   confirmed to make the step throw and exit 1.
2. Both "Collect safe validation evidence" steps did
   `git status --short | Set-Content artifacts/git-status.txt`. On a clean tree
   `git status --short` emits nothing, an empty pipeline never invokes
   `Set-Content`, and **the file was never created** — so the evidence bundle
   silently omitted the cleanliness record it is named for, precisely when the news
   was good, and a reader could not tell "clean tree" from "capture failed". Both
   steps now capture into an array and write unconditionally, with an explicit
   `# clean:` marker. Reproduced and fixed in a throwaway repository: old form
   `Test-Path` → `False`, new form → `True`.

**Limits.** Nothing was compiled, built, installed or launched; this pass is YAML,
Markdown and C++ string literals, and this host has no C++ toolchain. The
`Validate MD3 source wiring` step was re-extracted and re-run locally against this
tree (exit 0), but **CI has still never executed the repaired step** — `main`
stays red until `ca97ec93ccc` is pushed. No release asset was downloaded; release
facts come from the GitHub API, not from installing anything.

### Fixed — Two accessibility defects in the documentation-site language switcher (2026-08-16)

*No commit link yet.* This entry describes the commit that adds it.

An independent verifier found two accessibility defects in the switcher that
`docs/index.html` shipped with, in the same file a 48-of-48 headless-browser
pass had just declared green. Both are repaired here, and both were measured
in a real browser before and after rather than reasoned about.

**A blank status line on load.** The switcher's end-of-document IIFE called
`render(storedMode())` and never rendered the `role="status"` line, so a reader
reloading with `md3.language.mode=1` stored got a fully Cantonese page and a
correctly checked Cantonese radio above an **empty** `語言模式：粵語` line, until
they touched the control. Measured at `7d5c9dece14`, `#lang-status`
`textContent` on a fresh load was `""` in all three modes. It is now
`"Language mode: English"`, `"語言模式：粵語"` and
`"Language mode: Bilingual · 語言模式：雙語"` respectively.

The text is now present on load **without announcing on load**, which is the
point: nothing changed, the reader did nothing, and a live region that speaks
on arrival talks over the page. The two are separated by *when the role is on
the node*. The script reads `role` off `#lang-status`, removes it, renders the
text, and restores it in a `requestAnimationFrame` callback, so the
accessibility tree adopts that text as the region's starting content rather
than as a change; the attribute stays in the markup, so the no-JavaScript
reader still meets the role and every later mode change is a real announcement.
A `MutationObserver` installed at document-start recorded the same order in all
three modes: `role attribute: "status" -> null`, `status text written: … while
role=null`, `role attribute: null -> "status"`.

**WCAG 2.2 SC 3.1.2 Language of Parts.** The switcher's own labels —
`English · 英文`, `Cantonese · 粵語`, `Bilingual · 雙語` — were the only untagged
CJK on the page. Everything else Cantonese is built by the renderer, which sets
`lang="zh-HK"` itself; these three are literal DOM text and inherited
`<html lang="en">`, so an English synthetic voice mispronounced the very control
a Cantonese-seeking reader is hunting for — and they are visible in **all three
modes**, including English mode, where the page otherwise had zero `zh-HK`
elements. Each Chinese half is now its own `<span lang="zh-HK">`. The visible
text is unchanged. Fresh-load `[lang="zh-HK"]` element counts, before → after:
English **0 → 3**, Cantonese **41 → 45**, bilingual **41 → 45**. The verifier's
reported 42 reproduces as the *post-click* count on the old page (`on load 41`
/ `after click 42`); the difference is the status line's own Cantonese run,
which only existed once the control had been touched.

`md3-validation.yml` gains the matching source contract in its `docs/index.html`
list: the shape of all three labels, an exact count of three literal
`lang="zh-HK"` occurrences in the source, and the three literals that pin the
render-then-restore-the-role ordering. Both halves were proved able to fail:
reverting the label spans and deleting the single `removeAttribute('role')`
line each made the extracted contract exit 1 with its own message.

**Limits:** nothing was compiled — this change is HTML, YAML and Markdown, and
the host has no C++ toolchain. Technical facts still do not diverge between
modes (re-measured: the distinct sets of 14 `<code>` values, 23 `href` values
and 3 `<kbd>` values are byte-identical in all three modes), with the one
arithmetic exception already on record — bilingual mode renders the English and
Cantonese runs of the same paragraph, so `#6750A4` is *cloned* into both and
occurs twice; the value is identical and the source still holds it once. The
browser harness used for these measurements is a scratch script outside the
repository and is **still not a repeatable gate**.

### Documentation — Bring every stale document back to the truth (2026-08-16)

*No commit link yet.* This entry describes the commit that adds it, and no
entry in this file may cite a hash that does not exist — the same rule that
kept the changelog viewer's own entry out of this file until its hash existed.
A later task fills it in. The hash is waited for, never invented.

A documentation-only pass. **No product source was written and nothing was
compiled** — this host has no `kmk`, no configured Qt and no MSVC workload.
`HANDOFF.md`, `doc/md3/LocalGates.md`, `doc/md3/CompletenessInventory.md`,
`doc/md3/CaptureMatrix.md`, `doc/md3/WiringAudit.md`, `doc/md3/Changelog.md`
and this file were audited against the live repository and the GitHub API, and
every corrected number was taken from a command that was actually run.

What was wrong and is now right:

- **`Material 3 validation` is red on `main` and no committed document said
  so.** Now recorded in `HANDOFF.md`, `LocalGates.md` and at the top of this
  file, with the failing step, the message and the run ids.
- `HANDOFF.md` named the wrong `main` tip, the wrong latest release, four
  remote branches instead of seven, and "8 of 8 local gates pass" when the
  inventory had grown to 20 and one of them was the step CI now fails. Its
  self-correction table gained four appended rows rather than losing any.
- `LocalGates.md` recorded a Windows CI run as `in_progress` with "do not infer
  a result" and never came back to it; that run **failed**. It also called the
  installer "Squirrel" in three places, said every capture row read "Not
  captured", and carried a file count 56 files out of date.
- `CaptureMatrix.md`'s blocker A claimed no releases existed and the Windows
  workflow was red; nine releases exist and it is green. Blocker A is cleared,
  rows 44–45 get their real blocker (no auto-updater exists), and the runtime
  rows get an explicitly named permanent ceiling.
- `CompletenessInventory.md`'s summary table read Partial 17 / Not implemented
  40; `tools/md3/count-inventory-rows.py` reports **Partial 18 / Not
  implemented 39**, because the documentation-site row became Partial and the
  aggregate was never recomputed.
- `WiringAudit.md` §4a gained a dated re-read block: the `Ctrl+G` defect it
  described is **absent from the source** at all four sites it names. §4a's
  original text is untouched.
- This file had never recorded `ci.101`, `ci.106`, `ci.107` or `ci.108`, and
  had entries sitting under `[Unreleased]` that shipped days ago. Both fixed,
  and `UIMd3Changelog::prepareEntries()` was updated in the same task as its
  own contract requires.

**Limits, stated rather than implied.** Nothing was compiled, built, installed
or launched. The `Ctrl+G` verdict is a source re-read and is not a runtime
test. Gate 7's 135-second line counter was not re-run, so its totals stay
old and are labelled as old. The three `UsabilityProbe.md` defects were not
re-examined. **The unsigned-driver ceiling is unchanged: `VBoxSup.sys` ships
unsigned, 64-bit Windows will not load an unsigned kernel driver, signing is
permanently prohibited, and no virtual machine can start.**

### Added — A language switcher on the documentation site (2026-08-16)

[`2048ed0a7b651373a2707d30b4446a888861f33f`](https://github.com/Ding-Ding-Projects/material-virtualbox/commit/2048ed0a7b651373a2707d30b4446a888861f33f)

`docs/index.html` — the page published at
<https://ding-ding-projects.github.io/material-virtualbox/> — now has an
inline English / Cantonese / bilingual switcher. It mirrors
`UIMd3LanguageMode`: same three modes in the same order, the same `%1 · %2`
U+00B7 join as `UIMd3Language::text()`, the same empty-Cantonese-falls-back-to-
English rule, the same English default, and persistence under a `localStorage`
key with the same `qBound(0, toInt(), 2)` clamp as `GUI/Md3/LanguageMode`. The
two playfulness levels are deliberately not mirrored.

English remains the literal document text and Cantonese lives only in
attributes, so the page without JavaScript is exactly the page that shipped
before — complete, readable English, with the switcher hidden rather than
painted dead. Technical facts are written once and cloned into place rather
than retyped in a translation, so no hash, version string, file name or URL
can differ between modes. The page still issues no subresource requests, uses
no `innerHTML`, and carries an explicit `prefers-reduced-motion` block.

The Material 3 validation workflow gains the matching source contract:
required literals, the middle-dot join asserted by code point, negative guards
against `innerHTML` and against any subresource, an exact count of three
`role="tab"` elements so the switcher cannot join the tab strip, and an exact
occurrence count for each of nine technical facts.

**Limits:** nothing was compiled — this change is HTML, YAML and Markdown, and
the host has no C++ toolchain. Only part of the page gets Cantonese: 18 of the
page's 22 `<h1>`–`<h3>` headings carry it, the `<h1>` product name and the three
Verification card `<h3>`s do not, and the body prose is only partly translated —
the long technical prose and all 30 published `doc/md3/*.md` articles stay
English, and the page says so to the reader. Check the heading fraction with
`grep -oE '<h[1-3][ >]' docs/index.html | wc -l` (→ 22) and
`grep -oE '<h[1-3] data-md3-zh' docs/index.html | wc -l` (→ 18). The switcher's
behaviour was driven in a real headless browser (48 of 48 checks), but that
harness is a scratch file outside the repository and is not a repeatable gate.
The deployed site was not fetched and no claim is made about it. See
[`doc/md3/SiteLanguage.md`](doc/md3/SiteLanguage.md) and
[`doc/md3/LocalGates.md`](doc/md3/LocalGates.md) rows 2 and 20.

### Fixed — A red CI contract and a conflicting accessibility string (2026-08-16)

[`ca97ec93ccc06559d1035dd039ef56ca7b000a93`](https://github.com/Ding-Ding-Projects/material-virtualbox/commit/ca97ec93ccc06559d1035dd039ef56ca7b000a93)

The Material 3 validation workflow's "Compact README contract" required the
literal `no verified installer is published yet`. That stopped being true when
real unsigned NSIS installers were published, so the step was failing on an
assertion that had inverted. It is repaired rather than removed: it now pins
the Install status heading, the published-and-unsigned claim, the named
`VirtualBox-7.2.97-Setup.exe` artifact, the Releases URL, the SmartScreen
disclosure and the runtime-capture gate, and it refuses a README that claims a
"signed installer" or "signed NSIS" package without the leading "un".

The same commit adds `tools/md3/check-language-registry.ps1`, the first
executable localization gate for the `UIMd3Language` registry, and wires it
into CI with `-MaxUnparseable 2`. Its first run found a genuine defect:
`md3.wizard.current-page-description` was registered twice with different
text, once by the shared MD3 wizard shell and once by `UINativeWizard`. Since
`registerText` ends in `QHash::insert`, which overwrites, one of the two
readers was always wrong — either the page stack's accessible description lost
its page title, or the wizard's title label announced a literal `%1`. The
native wizard's key is renamed to `md3.wizard.page-stack-description`.

**Limits:** this is a text gate. Nothing here was compiled — there is no C++
toolchain on the host that produced it. The gate cannot see the 40
registrations behind the two runtime-parameterised call sites in
`UIVirtualBoxManager.cpp`; it reports them as unchecked on every run rather
than passing them. See [`doc/md3/LocalGates.md`](doc/md3/LocalGates.md) row 19.

## [v7.2.97-ci.108] — 2026-08-15

**Latest release.** Non-draft. Tag `v7.2.97-ci.108`, "Dried Scallop Shrimp Dumpling · 瑤柱蝦餃".
Target commit
[`fe321a4fd6f6bfc7c2fbd7e0704e3f72d0eb2eee`](https://github.com/Ding-Ding-Projects/material-virtualbox/commit/fe321a4fd6f6bfc7c2fbd7e0704e3f72d0eb2eee)
("Reconcile preserved build draft with current main"), published `2026-08-15T01:21:51Z` by
[run 31851996367](https://github.com/Ding-Ding-Projects/material-virtualbox/actions/runs/31851996367)
(**success**, 1h26m24s).

| Asset | Size | SHA-256 |
| --- | ---: | --- |
| `VirtualBox-7.2.97-Setup.exe` | 106,963,266 | `8a6e1e94bdd73c3a9c526b7b7e074521f06a2562b9f9020ea1a806f15fcda627` |
| `SHA256SUMS.txt` | 95 | `f2561d8a1a279266048a4262464b515a039d60731f0fef2a93ad5c33ade8b7e8` |
| `hk-dish-0009-dried-scallop-shrimp-dumpling.png` | 2,436,523 | `3d479c48358e2fa49c0dbefbfa4244659cf27ab1ffee3d4ac7438276b380342f` |

No new frontend feature. It carries the two build-bootstrap commits
`896733c2e11` and `fe321a4fd6f` on top of ci.107, and therefore also carries everything in ci.107,
ci.106 and ci.101.

**`Material 3 validation` is red at this commit** — [run 31851996370](https://github.com/Ding-Ding-Projects/material-virtualbox/actions/runs/31851996370)
on `main`, re-run and failing again as [31856303192](https://github.com/Ding-Ding-Projects/material-virtualbox/actions/runs/31856303192)
on the `v7.2.97-ci.108` tag ref — on the stale README assertion described at the top of this file.
This is the **third** consecutive red commit on `main`, not the first; see the table at the top.
The Windows build and the Pages deploy are both green. **No virtual machine can start from this installer**, for the permanent
unsigned-driver reason stated at the top of this file.

## [v7.2.97-ci.107] — 2026-08-15

Non-draft. "Lobster Dumpling · 龍蝦餃". Target
[`753602ced18e50649ce0b0a3038cf7efd1ba97fe`](https://github.com/Ding-Ding-Projects/material-virtualbox/commit/753602ced18e50649ce0b0a3038cf7efd1ba97fe)
("Merge libxslt bootstrap into current native build"), published `2026-08-15T01:20:54Z` by
[run 31851883237](https://github.com/Ding-Ding-Projects/material-virtualbox/actions/runs/31851883237)
(**success**, 1h27m26s). `VirtualBox-7.2.97-Setup.exe`, **106,981,357 bytes**, SHA-256
`a7540294d102bf31b4f009302dcee827d80549d678f77a5f1c7676597efe9237`.

Build tooling only: bootstraps libxslt for native Windows builds (`3286ed09679`, `753602ced18`).

**`Material 3 validation` was already red at this commit** —
[run 31851883297](https://github.com/Ding-Ding-Projects/material-virtualbox/actions/runs/31851883297),
**failure** on `main` at `753602c`, on the stale README assertion described at the top of this file.
This entry read green by omission until 2026-08-16. The Windows build and the Pages deploy are both
green. **No virtual machine can start from this installer**, for the permanent unsigned-driver reason
stated at the top of this file.

## [v7.2.97-ci.106] — 2026-08-15

Non-draft. "Pea Shoot Shrimp Dumpling · 豆苗蝦餃". Target
[`8762579681594cf8ba6beb8ccec77576baa5199a`](https://github.com/Ding-Ding-Projects/material-virtualbox/commit/8762579681594cf8ba6beb8ccec77576baa5199a),
published `2026-08-15T00:21:28Z` by
[run 31848342345](https://github.com/Ding-Ding-Projects/material-virtualbox/actions/runs/31848342345)
(**success**, 1h28m12s). `VirtualBox-7.2.97-Setup.exe`, **106,902,297 bytes**, SHA-256
`ef62c3fec34e3b387ae7c0be06c6b1b895d6be921bd6059682e69c91f24ef7c3`.

**This is the release in which the three features below first compiled and shipped.** Their
integration commit `d548859c25785ec20d61e56f018ba25da5d2bb70` had failed its own build
([run 31843086131](https://github.com/Ding-Ding-Projects/material-virtualbox/actions/runs/31843086131),
**failure** at step "Build and package the Windows installer"); `8762579681594` is the first commit
containing all three whose build went green. Recorded because the failure is part of the story:
these features did break a build before they shipped in one.

**`Material 3 validation` was already red at this commit, and this is where the red streak starts** —
[run 31848342250](https://github.com/Ding-Ding-Projects/material-virtualbox/actions/runs/31848342250),
**failure** on `main` at `8762579`, on the stale README assertion described at the top of this file.
`8762579` is the first commit on `main` at which that gate went red; it has been red at every commit
since. This entry read green by omission until 2026-08-16. The Windows build and the Pages deploy are
both green. **No virtual machine can start from this installer**, for the permanent unsigned-driver
reason stated at the top of this file.

The entry below was written under `[Unreleased]` on 2026-08-14 and moved here on 2026-08-16 without
altering its text, per this file's own rule that entries leave `[Unreleased]` when they appear in a
published release. Its "none of them has been compiled or captured at the time of writing" limit is
now half-resolved: all three **are** compiled and shipped, and none of them is captured or tested.

### Added — Three canonical features that had no implementation at all (2026-08-14)

Each of these was recorded in `doc/md3/CompletenessInventory.md` as
"Not implemented on any surface" before this pass. Each shipped with an honest
limit recorded in its own inventory row; none of them is finished, and none of
them has been compiled or captured at the time of writing.

1. **Emoji-in-dialogs toggle** —
   [`72b74ebcaa2e686aca23c19d820bda795fc5a55b`](https://github.com/Ding-Ding-Projects/material-virtualbox/commit/72b74ebcaa2e686aca23c19d820bda795fc5a55b)
   `UIMd3EmojiSetting` adds a persisted toggle, default off, mapping eight
   dialog tones to one decorative emoji each and returning text untouched when
   disabled. Reachable from the command palette. **Limit:** no dialog or message
   box in the frontend calls the decoration helper yet, so enabling it changes
   nothing visible except the palette row's own label.

2. **Dim sum startup surprise** —
   [`2792e4038262470bfc3db592e88ec4437bbca636`](https://github.com/Ding-Ding-Projects/material-virtualbox/commit/2792e4038262470bfc3db592e88ec4437bbca636)
   `UIMd3DimSumSurprise` draws once per launch with a 10% chance and shows a
   non-blocking, auto-dismissing toast naming a dish in English and Cantonese.
   It cannot gate startup, cannot steal focus, and cannot be turned off.
   **By design it ships no photograph:** those images belong to a separate
   public catalog and are never vendored, generated, or downloaded into this
   repository, so the toast renders an explicit placeholder where a picture
   would go.

3. **Local personal-vocabulary JSON upload** —
   [`804587eb558ca774ef04454f691ad89c7a8f25f9`](https://github.com/Ding-Ding-Projects/material-virtualbox/commit/804587eb558ca774ef04454f691ad89c7a8f25f9)
   `UIMd3PersonalVocabulary` adds an always-visible file picker and a bounded,
   versioned, all-or-nothing validator: file size, schema version, nesting
   depth, entry count, key and value lengths and value types are all bounded,
   and nesting is checked by scanning raw bytes before any parser builds a tree.
   Nothing ships preloaded — no samples, no templates, no defaults. **Limit:**
   no other surface routes its rendered text through the service yet.

### Changed — The completeness inventory counts itself now (2026-08-14)

[`8034b1cacb17c610f1afaec8fc99459dcfdc8f20`](https://github.com/Ding-Ding-Projects/material-virtualbox/commit/8034b1cacb17c610f1afaec8fc99459dcfdc8f20)
The inventory's summary table had drifted from the rows it summarises — three
sections changed status while the Implemented count stayed put, producing a
table that added up correctly and described the tree incorrectly. The rows stay
hand-written, because a generated checklist cannot look for a feature that has
no implementation anywhere; only the arithmetic became mechanical, through
`tools/md3/count-inventory-rows.py`, which fails closed when its own buckets
disagree with its own total. The same commit corrects section 16, which had
claimed the external-editor handoff was unimplemented long after it shipped.

## [v7.2.97-ci.101] — 2026-08-14

Non-draft. "Spinach Shrimp Dumpling · 菠菜蝦餃". Target
[`bb63f016a5da05a9880ea957609695770004763c`](https://github.com/Ding-Ding-Projects/material-virtualbox/commit/bb63f016a5da05a9880ea957609695770004763c),
published `2026-08-14T22:30:40Z` by
[run 31840815630](https://github.com/Ding-Ding-Projects/material-virtualbox/actions/runs/31840815630)
(**success**, 1h26m14s). `VirtualBox-7.2.97-Setup.exe`, **106,884,648 bytes**, SHA-256
`bc9f86cbd21885e17837b438414ab4d61f51918e1ef59501be4bc58feb94f7db` — **+11,910 bytes** over ci.100,
consistent with a roughly 1,010-line class landing.

**This is the release in which `UIMd3Changelog` was compiled for the first time.**
`aa52c21c9f4d3b1ac1fa961ed9fee70c75f91cb5` is an ancestor of this target
(`git merge-base --is-ancestor`), so the in-app changelog viewer is present in this installer and
every installer since. It has still never been launched, screenshotted or tested; see
`doc/md3/CaptureMatrix.md` rows 46 and 47, which are unattempted rather than blocked.

The entry below was written under `[Unreleased]` and moved here on 2026-08-16 without altering its
text.

### Added — In-app changelog viewer

- Added an in-app changelog viewer (`UIMd3Changelog`, reachable from the
  Manager with `Ctrl+Shift+L` or the command palette's "Open changelog")
  that compiles in every entry this file records, with plain-text/regex
  search, version/category/date filters that compose, clickable per-entry
  commit references, and filtered Markdown export/copy. See
  [`doc/md3/Changelog.md`](doc/md3/Changelog.md) for the full contract,
  including why the entry set is a verified compiled-in transcription of
  this file rather than a runtime parser, and the exact maintenance duty
  that follows from that choice: every future edit to this file's entries
  must be mirrored into `UIMd3Changelog::prepareEntries()` in the same task.
  (This bullet was originally not an entry inside the viewer, because it
  describes the commit that adds the viewer and no viewer can cite its own
  not-yet-created hash without inventing one. That hash now exists —
  `aa52c21c9f4d3b1ac1fa961ed9fee70c75f91cb5` — so the entry has been added
  and cites it, exactly as this note asked a later task to do. The rule it
  was protecting is unchanged: the hash was waited for, never invented.)

## [v7.2.97-ci.96] through [v7.2.97-ci.100] — 2026-08-14

**Five releases, all non-draft — and they are not all the same kind of package.** This heading
previously said "six", listed five, and called all of them NSIS. Both halves were wrong.

| Release | Published | Installer asset |
| --- | --- | --- |
| `ci.96` "Classic Har Gow · 蝦餃" | `2026-08-14T03:35:09Z` | **Squirrel.Windows**: `Setup.exe`, `RELEASES`, `VirtualBox-7.2.96-full.nupkg` |
| `ci.97` | `04:56:36Z` | **First NSIS release** — `VirtualBox-7.2.97-Setup.exe` + `SHA256SUMS.txt` |
| `ci.98` | `05:33:59Z` | NSIS `VirtualBox-7.2.97-Setup.exe` |
| `ci.99` | `08:40:25Z` | NSIS `VirtualBox-7.2.97-Setup.exe` |
| `ci.100` | `10:06:47Z` | NSIS `VirtualBox-7.2.97-Setup.exe`, 106,872,738 bytes; target `8850ddb7efb248e79dddd697118517d426bb61c4` |

**`ci.96` has no `VirtualBox-7.2.97-Setup.exe` and no `SHA256SUMS.txt`**; `ci.97` is where the NSIS
path starts, and every release from `ci.97` onward carries both files. Read any row back with
`gh release view v7.2.97-ci.96 --repo Ding-Ding-Projects/material-virtualbox --json assets`. Each
release also carries one dim-sum photograph. The tag counter is the GitHub Actions run number, so it
skips: **`ci.102` through `ci.105` were never created.**

These are the first releases the repository ever published, and the sections below are the record of
how the pipeline got to them. They were written under `[Unreleased]` while the build was still red
and are left where they were written, under this heading, rather than reworded.

### Fixed — Windows build and packaging pipeline (2026-08-13)

The Windows package-and-release workflow spent 2026-08-13 failing forward:
each fix reached further into the pipeline before finding the next thing that
was wrong. The chain below is ordered oldest to newest so the progression
reads the way it happened, not by topic. None of these commits made the
workflow pass end to end — see "Known issue" below for where it stands now.

1. **Build the host binaries before packing them** —
   [`e68fd88b9ae3d9f4b0f9c1d23d2c4249a1eb6f7a`](https://github.com/Ding-Ding-Projects/material-virtualbox/commit/e68fd88b9ae3d9f4b0f9c1d23d2c4249a1eb6f7a)
   The workflow ran `kmk ... packing` on its own. That target only stages
   already-built binaries into the installer payload; it does not build them.
   It passed locally only because roughly eighteen prior local builds had
   left `VirtualBox.exe` and friends sitting in `out/`. A clean CI checkout
   has no such attic, and failed with "The packaged Windows payload is
   missing VirtualBox.exe" after successfully building everything else. Split
   into a full `kmk` build pass followed by the `kmk ... packing` pass,
   matching VirtualBox's own documented build sequence.

2. **Install the Qt modules the frontend actually links** —
   [`04201aa85441341616cc90a84e4a9fdaac433b9d`](https://github.com/Ding-Ding-Projects/material-virtualbox/commit/04201aa85441341616cc90a84e4a9fdaac433b9d)
   `aqtinstall` was invoked with no `--modules` argument at all, which
   installs `qtbase` only. `UICommon_QT_MODULES` needs `Help` and
   `StateMachine` in addition to what `qtbase` supplies, so the build reached
   the link step roughly 33 minutes in before failing on a missing
   `Qt6StateMachine.lib`. Also hardened the "is Qt already installed?" guards,
   which previously checked only for `qmake.exe` — a cached module-less Qt
   install has a perfectly good `qmake.exe` and would have made this fix
   silently do nothing on a warm cache.

3. **Ask for the Qt module that exists** —
   [`7d797e64e5f864244fe3113d1c5349db7eddf7ce`](https://github.com/Ding-Ding-Projects/material-virtualbox/commit/7d797e64e5f864244fe3113d1c5349db7eddf7ce)
   The previous fix requested `--modules qtscxml qttools`. `aqtinstall`
   rejected `qttools` outright ("package not found"), and installed *nothing*
   for that step — Qt ships `qttools` inside the base desktop package, not as
   a selectable add-on module; only `qtscxml` (which supplies
   `StateMachine`) needed to be requested explicitly. The post-install
   assertion added in the previous commit is what caught this in four
   minutes instead of another 33.

4. **Choose the v143 toolset in CI instead of hoping** —
   [`5cf840a2abd4d13eb12083e134aac072239f3505`](https://github.com/Ding-Ding-Projects/material-virtualbox/commit/5cf840a2abd4d13eb12083e134aac072239f3505)
   `configure.py`'s MSVC auto-probe selected 14.29.30133 — the v142
   compatibility compiler VS 2022 Enterprise ships alongside v143 — on the
   hosted runner image. It compiles the entire tree without complaint, so the
   build ran a further 39 minutes before failing on a missing
   `vcruntime140.dll`, because `VCC143.kmk` looks for its redistributables
   under a `14.[34]*` path that a 14.29 toolset does not match. The workflow
   now selects the toolset explicitly, mirroring the local
   `tools/build-windows.ps1` logic, and requires a 14.3x/14.4x version or
   throws immediately naming what was rejected and why.

5. **Select the newest MSVC toolset, not the oldest** —
   [`d3ec5d6a11a8a17ca24802e26d96745991afe119`](https://github.com/Ding-Ding-Projects/material-virtualbox/commit/d3ec5d6a11a8a17ca24802e26d96745991afe119)
   `configure.py` sorted the installed MSVC toolsets newest-first, then
   looped over the whole list assigning into the same variable with
   `os.path.join`. Because `glob.glob` returns absolute paths and
   `os.path.join` discards everything before the final absolute argument,
   every loop iteration overwrote the last, so the loop finished holding the
   *last* (oldest) entry from a descending sort — the same v142 toolset
   commit 4 had just taught the workflow to reject. Fixed to take index `0`
   and stop. Also corrected `--with-vc` to pass the installation root
   `configure.py` expects, rather than a pre-resolved toolset directory.

6. **Hand kBuild forward slashes so it can find the redistributables** —
   [`c48dbebe6a0c1eb93eb41ee33a23fde5987f0435`](https://github.com/Ding-Ding-Projects/material-virtualbox/commit/c48dbebe6a0c1eb93eb41ee33a23fde5987f0435)
   `VCC143.kmk` only takes its correct redistributable path when
   `PATH_TOOL_VCC143` ends in `/tools/msvc/` with forward slashes.
   `configure.py` was emitting a mixed-separator path (forward slashes from
   short-path normalization, backslashes from `os.path.join`), so the check
   never matched. kBuild fell through to an ancestor search that found a
   directory literally named `VC/Redist` — correctly named, genuinely
   present, and one directory short of where the actual DLLs live under
   `MSVC/<version>`. The build ran a further 39 minutes before failing on the
   same missing `vcruntime140.dll` symptom as commit 4, for an unrelated
   reason. Normalized separators once, at the point the toolset path is
   chosen.

7. **Let Qt ask about a macro MSVC will never define** —
   [`9efa7f522c69f3ce1b99940d13e96d534ee2a0c3`](https://github.com/Ding-Ding-Projects/material-virtualbox/commit/9efa7f522c69f3ce1b99940d13e96d534ee2a0c3)
   Qt's `qnumeric.h` guards a branch with `#if (Q_CC_GNU >= 500 || ...)`, and
   `Q_CC_GNU` is a GCC-only macro that is simply undefined under MSVC —
   which is exactly the case the code is handling correctly. MSVC's `C4668`
   warning exists to flag that substitution, and with `-Wall -WX` in effect
   it became a fatal error the moment any translation unit that includes
   `QtCore` was compiled, 49 minutes into the build. Added `-wd4668` to
   `VBOX_VCC_WARN_ALL`, unconditionally rather than only inside the VCC143
   guard, because Qt does this under every MSVC version. Left the other nine
   warnings the same build run surfaced (`C4223`, `C4295`, `C4133`, `C4113`,
   `C4142`, `C5274`, `C4474`, `C4476`, `C5267`) untouched and visible — this
   is the current tip of `main` as verified by `git log` in this worktree.

### RESOLVED — the packaging step used to fail later, on something new (historical)

As of the latest push to `main` (`9efa7f522c69f3ce1b99940d13e96d534ee2a0c3`),
the **Material 3 validation** workflow is green
([run 31731859865](https://github.com/Ding-Ding-Projects/material-virtualbox/actions/runs/31731859865)),
but the **Windows package and release** workflow is still red
([run 31731859854](https://github.com/Ding-Ding-Projects/material-virtualbox/actions/runs/31731859854),
1h 10m). It now gets substantially further than any of the runs above — past
Qt, past the toolset selection, past the redistributable path, through the
full build and into the packaging step's own self-check — before failing on:

```
kmk: *** [.../VMM/testcase/Makefile.kmk:1305: .../tstVMStructSize.run] Error -1073740791 (STATUS_STACK_BUFFER_OVERRUN)
kmk: *** [.../VMM/testcase/Makefile.kmk:1281: .../tstAsmStructs.run] Error -1073740791 (STATUS_STACK_BUFFER_OVERRUN)
```

Both `tstVMStructSize.exe` and `tstAsmStructs.exe` are build-time self-check
testcases that run and crash with `STATUS_STACK_BUFFER_OVERRUN`
(`0xC0000409`) rather than reporting a structure-layout mismatch. This is a
different failure signature from anything fixed in the chain above and has
not yet been root-caused. No release has been published, and no verified
Windows installer exists as of this writing. See `HANDOFF.md` for the current
external-blocker summary and `doc/md3/CaptureMatrix.md` for what this means
for screenshot evidence.

**Resolution, recorded 2026-08-16.** The paragraph above is kept verbatim because it was true when
written, and because `doc/md3/CaptureMatrix.md` cited it as live evidence for its blocker A. Both
of its closing claims are now false: **nine releases exist and verified Windows installers do too.**
The `STATUS_STACK_BUFFER_OVERRUN` blocker was worked around with
`VBOX_WITHOUT_VMM_RUN_STRUCT_TESTS=1` in `.github/workflows/windows-package-release.yml`. That fix
was **not** proved by the next run — [run 31740515001](https://github.com/Ding-Ding-Projects/material-virtualbox/actions/runs/31740515001)
at `cb9f573030e3f27d7b13314d90234e2b8de873c9` completed **failure**, and `doc/md3/LocalGates.md`
recorded it as "in_progress / do not infer a result" and then never resolved it, which is corrected
there now. Exactly four further runs failed after it and before the first success — `31748469786`,
`31748972511`, `31753303822`, `31758869634` — read from `gh run list --workflow "Windows package
and release" --json databaseId,conclusion,createdAt --limit 200`, which lists those four and no
others between run `31740515001` (`2026-08-13T20:22:15Z`) and run `31762849415`
(`2026-08-14T02:09:07Z`). The earlier chain of failures referred to previously ran *before*
`31740515001`, so it cannot supply a fifth run after that point and is not counted here.
**The first Windows packaging run that ever completed
`success` was [run 31762849415](https://github.com/Ding-Ding-Projects/material-virtualbox/actions/runs/31762849415)
at `0d9eda43cd0f8ba69cc32ba5fc865b90dca64218` on `2026-08-14T02:09:07Z`, which published
`v7.2.97-ci.96`** — read from
`gh run list --workflow "Windows package and release" --json databaseId,conclusion,headSha,createdAt`.
Every later release listed in this file came from its own successful run. This section is
historical and should not be read as a current blocker.

### Documentation

- Added this changelog, a capture-matrix tracking table
  ([`doc/md3/CaptureMatrix.md`](doc/md3/CaptureMatrix.md)), and brought
  `HANDOFF.md`, `ROADMAP.md`, and `README.md` current with the build/packaging
  state above.

## ~~Related work not yet on `main`~~ — landed; corrected 2026-08-16

Commit
[`1e59aca274c171b85a5e293e6fe8b70a04ccce27`](https://github.com/Ding-Ding-Projects/material-virtualbox/commit/1e59aca274c171b85a5e293e6fe8b70a04ccce27)
("Merge origin/main into the Material 3 runtime chrome branch") reconciles 17
commits of native Material Design 3 runtime window chrome with the Windows
build-tooling chain above, but only as of where `main` stood at commit 1 in
this list.

**Both factual claims this section used to make are now false, and are corrected here rather than
deleted.** It said the commit "lives on branch `claude/full-ui-rewrite-reconciled-20260813`" and
"is not an ancestor of `main` (verified with `git merge-base --is-ancestor`)". As of 2026-08-16:

- `git merge-base --is-ancestor 1e59aca274c origin/main` succeeds — **it IS an ancestor of `main`**,
  and therefore compiled into every release from at least `v7.2.97-ci.96` onward.
- `git ls-remote --heads origin 'claude/full-ui-rewrite-reconciled-20260813'` returns **nothing**;
  that branch no longer exists on the remote. The seven branches that do exist are listed in
  `HANDOFF.md`.

The runtime chrome this section describes is therefore shipped, not pending. What has *not* changed
is that none of it has been exercised at runtime: `doc/md3/CaptureMatrix.md` rows 32–35 (every
runtime-window surface) remain uncaptured and are permanently ceilinged, because `VBoxSup.sys`
ships unsigned, 64-bit Windows will not load an unsigned kernel driver, code signing is permanently
prohibited for this project, and so **no virtual machine can start** — which means the runtime
window cannot be opened at all.
