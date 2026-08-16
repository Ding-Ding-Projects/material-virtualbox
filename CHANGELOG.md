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

**A red gate this file also failed to record.** `Material 3 validation` is
**failing on `main`** and has been since `fe321a4fd6f6bfc7c2fbd7e0704e3f72d0eb2eee`
([run 31856303192](https://github.com/Ding-Ding-Projects/material-virtualbox/actions/runs/31856303192)).
It is repaired by `ca97ec93ccc06559d1035dd039ef56ca7b000a93`, recorded under
`[Unreleased]` below, which is **not yet pushed**. Nothing in this file should
be read as claiming CI is green until that lands.

**The unsigned-driver ceiling is unchanged and is not softened anywhere in this
file.** The host hypervisor driver `VBoxSup.sys` ships unsigned, 64-bit Windows
refuses to load an unsigned kernel driver, and code signing is permanently
prohibited for this project. **No virtual machine can start**, in any release
listed below, and no change in this repository can alter that. Every installer
named here installs, registers COM and reaches a working Manager; none of them
can run a VM.

Entries below remain under **[Unreleased]** when they have not yet appeared in
a published release.

## [Unreleased]

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
the host has no C++ toolchain. Only 14.9% of the page's visible characters get
Cantonese; the long technical prose and all 30 published `doc/md3/*.md`
articles stay English, and the page says so to the reader. The switcher's
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
toolchain on the host that produced it. The gate cannot see the 41
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

**`Material 3 validation` is red at this commit** — [run 31856303192](https://github.com/Ding-Ding-Projects/material-virtualbox/actions/runs/31856303192)
— on the stale README assertion described at the top of this file. The Windows build and the Pages
deploy are both green. **No virtual machine can start from this installer**, for the permanent
unsigned-driver reason stated at the top of this file.

## [v7.2.97-ci.107] — 2026-08-15

Non-draft. "Lobster Dumpling · 龍蝦餃". Target
[`753602ced18e50649ce0b0a3038cf7efd1ba97fe`](https://github.com/Ding-Ding-Projects/material-virtualbox/commit/753602ced18e50649ce0b0a3038cf7efd1ba97fe)
("Merge libxslt bootstrap into current native build"), published `2026-08-15T01:20:54Z` by
[run 31851883237](https://github.com/Ding-Ding-Projects/material-virtualbox/actions/runs/31851883237)
(**success**, 1h27m26s). `VirtualBox-7.2.97-Setup.exe`, **106,981,357 bytes**, SHA-256
`a7540294d102bf31b4f009302dcee827d80549d678f77a5f1c7676597efe9237`.

Build tooling only: bootstraps libxslt for native Windows builds (`3286ed09679`, `753602ced18`).

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

Six releases, all non-draft, all carrying an unsigned NSIS
`VirtualBox-7.2.97-Setup.exe`: ci.96 (`2026-08-14T03:35:09Z`, "Classic Har Gow · 蝦餃"), ci.97
(`04:56:36Z`), ci.98 (`05:33:59Z`), ci.99 (`08:40:25Z`) and ci.100 (`10:06:47Z`, target
`8850ddb7efb248e79dddd697118517d426bb61c4`, installer 106,872,738 bytes). The tag counter is the
GitHub Actions run number, so it skips: **`ci.102` through `ci.105` were never created.**

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
there now. Five further runs failed after it (`31748469786`, `31748972511`, `31753303822`,
`31758869634`, and the earlier chain above). **The first Windows packaging run that ever completed
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
