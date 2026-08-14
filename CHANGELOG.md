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

Entries below remain under **[Unreleased]** when they have not yet appeared in
a published release.

## [Unreleased]

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

### Known issue — the packaging step now fails later, on something new

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

### Documentation

- Added this changelog, a capture-matrix tracking table
  ([`doc/md3/CaptureMatrix.md`](doc/md3/CaptureMatrix.md)), and brought
  `HANDOFF.md`, `ROADMAP.md`, and `README.md` current with the build/packaging
  state above.

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

## Related work not yet on `main`

Commit
[`1e59aca274c171b85a5e293e6fe8b70a04ccce27`](https://github.com/Ding-Ding-Projects/material-virtualbox/commit/1e59aca274c171b85a5e293e6fe8b70a04ccce27)
("Merge origin/main into the Material 3 runtime chrome branch") reconciles 17
commits of native Material Design 3 runtime window chrome with the Windows
build-tooling chain above, but only as of where `main` stood at commit 1 in
this list. It lives on branch `claude/full-ui-rewrite-reconciled-20260813`,
is not an ancestor of `main` (verified with
`git merge-base --is-ancestor`), and `main` has since advanced six further
commits that branch has not absorbed. It is not represented as shipped here;
it is recorded so the next integration pass knows it exists and is not
current.
