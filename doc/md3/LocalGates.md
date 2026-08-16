# Local gates inventory

This is a hand-written inventory of every locally runnable gate this
repository has, plus every gate a release-grade shutdown of this repository
should have and does not. It exists because a checklist that only validates
what it has already discovered can never notice something that was never
built — so this document was authored from what the project *should* verify,
not generated from what a script found lying around, and every row below was
independently verified rather than assumed passing.

**Rows 1–18 were measured at commit `cb9f573030e3f27d7b13314d90234e2b8de873c9`**, which was the tip
of `main` when this audit was written, on a Windows 11 host with PowerShell
7.6.4, Git for Windows, and no VirtualBox build toolchain (no `kmk`, no
configured Qt, no Visual Studio C++ workload) installed. Every "Not run" row
below names the exact reason; no gate is marked passed without quoted
evidence from an actual invocation.

**`cb9f573030e` is no longer the tip.** As of 2026-08-16, `git rev-parse origin/main` returns
`fe321a4fd6f6bfc7c2fbd7e0704e3f72d0eb2eee`; `cb9f573030e` is still an ancestor of it
(`git merge-base --is-ancestor` → true). Rows 19 and 20 were added later and each names the tree it
was actually run against, because a row that borrows the header's commit without having been run
there is exactly the kind of quiet inflation this document exists to prevent. Rows 1, 2 and 19 were
additionally **re-run on 2026-08-16** against working-branch tip
`992aa84304a62e60a2a532e3da2a3569cad480ad` under PowerShell **7.6.5**; their evidence cells carry
that re-run. Rows 3–8 and 20 were **not** re-run this pass and still describe the tree they were
run against; row 7's inputs are known to have moved and its cell says so.

## How this was built

1. Every `.github/workflows/*.yml` file was read in full to find what CI
   actually runs — per the principle that a project's CI steps *are* its own
   gates, whether or not they are also packaged as a standalone script.
2. Every step that is pure source/text validation (pattern matching against
   committed files, directory-listing checks, file-existence checks) was
   extracted from the workflow YAML into a standalone PowerShell file and
   executed locally from the repository root, exactly as CI invokes it.
3. Every step that requires compiling C++, linking Qt, or produces an
   installer was left unrun and is recorded as infeasible in this lane, with
   the precise missing dependency named.
4. `tools/count-release-lines.ps1` was run as documented in its own
   `.SYNOPSIS` and in the release workflow.
5. The remote GitHub Actions state for the exact commit above was read with
   `gh run list` / `gh run view` (read-only) rather than inferred.

## Workflows discovered

| Workflow file | Name | Triggers | What it validates |
| --- | --- | --- | --- |
| `.github/workflows/md3-validation.yml` | Material 3 validation | `push`, `workflow_dispatch` | Two jobs: `archive-and-source-contract` (design ledger regeneration, MD3 source-wiring text contracts, notification/tab source contracts, and — added after the header audit — the `tools/md3/check-language-registry.ps1` localization gate of row 19) and `tonal-palette-contract` (HCT colour-science source contract). Pure source/text checks — no compilation. |
| `.github/workflows/windows-package-release.yml` | Windows package and release | `push` to `main`, `workflow_dispatch` | Full Windows build: bootstraps Qt/WDK/SDK/vcpkg/NASM/WiX/NSIS, runs `configure.ps1` + `kmk`, packages an **unsigned NSIS installer** (`VirtualBox-7.2.97-Setup.exe`), verifies the packaged assets are unsigned, and publishes a GitHub Release. This is the only workflow that compiles anything. **Corrected 2026-08-16:** this row said "Squirrel" until now. Squirrel.Windows was retired on 2026-08-14 — `.github/workflows/windows-package-release.yml:480` says the NSIS step "replaces the retired unsigned Squirrel.Windows package", `:491` invokes `tools\build-windows-nsis-installer.ps1`, and `:499` reads from `out\win.amd64\release\nsis-installer`. |
| `.github/workflows/pages.yml` | Material 3 documentation Pages | `push` to `main`, `workflow_dispatch` | Copies `docs/*`, `doc/md3/*.md`, and `doc/md3/ArchiveManifest.sha256` into a static site and deploys it via `actions/deploy-pages@v4`. No content validation of its own (the copied `docs/index.html` content is validated separately, inside `md3-validation.yml`'s source-wiring step). |
| `.github/workflows/question-stale.yml` | Close stale questions | `schedule` (daily), `workflow_dispatch` | Issue-triage automation (`actions/stale@v9`). Not a code or release gate; listed here for completeness since it is a workflow this repository runs. |

## Remote CI state for the audited commit

Read with `gh run list --repo Ding-Ding-Projects/material-virtualbox` and
`gh run view <id> --json status,conclusion,headSha` (read-only; no run was
triggered or waited on by this audit).

| Workflow | Run | `headSha` | Status | Conclusion |
| --- | --- | --- | --- | --- |
| Material 3 validation | [31740514822](https://github.com/Ding-Ding-Projects/material-virtualbox/actions/runs/31740514822) | `cb9f573030e3f27d7b13314d90234e2b8de873c9` (this commit) | completed | **success** |
| Material 3 documentation Pages | [31740514871](https://github.com/Ding-Ding-Projects/material-virtualbox/actions/runs/31740514871) | `cb9f573030e3f27d7b13314d90234e2b8de873c9` (this commit) | completed | **success** |
| Windows package and release | [31740515001](https://github.com/Ding-Ding-Projects/material-virtualbox/actions/runs/31740515001) | `cb9f573030e3f27d7b13314d90234e2b8de873c9` (this commit) | completed | **failure** — resolved 2026-08-16, see below |

**Resolution of the row above, read on 2026-08-16.** This document recorded that run as
`in_progress` with "not yet known — do not infer a result" and then never came back to it.
`gh run view 31740515001 --repo Ding-Ding-Projects/material-virtualbox --json status,conclusion,headSha`
now returns `{"conclusion":"failure","headSha":"cb9f573030e3f27d7b13314d90234e2b8de873c9","status":"completed"}`.
**The run failed.**

That matters for what the paragraph below originally claimed. For context, the *previous* commit's
Windows package and release run
([31731859854](https://github.com/Ding-Ding-Projects/material-virtualbox/actions/runs/31731859854)
at `9efa7f522c6...`, 1h10m) failed with
`STATUS_STACK_BUFFER_OVERRUN` in `tstVMStructSize`/`tstAsmStructs`
(documented in [`CHANGELOG.md`](../../CHANGELOG.md)). The fast-forward that
produced the audited commit above merged a fix
(`VBOX_WITHOUT_VMM_RUN_STRUCT_TESTS=1`, visible in
`.github/workflows/windows-package-release.yml`) and this document treated the
then-running run as the first attempt at proving it. **That attempt did not
prove it — it failed.** The `STATUS_STACK_BUFFER_OVERRUN` blocker was not
verified cleared until later, when
[run 31840815630](https://github.com/Ding-Ding-Projects/material-virtualbox/actions/runs/31840815630)
at `bb63f016a5da05a9880ea957609695770004763c` completed **success** in 1h26m14s and published
`v7.2.97-ci.101`. This inventory still does not claim any Windows build is green "in general"; it
names the run.

### Remote CI state at the current tip of `main`, read 2026-08-16

`gh run list --repo Ding-Ding-Projects/material-virtualbox --commit fe321a4fd6f6bfc7c2fbd7e0704e3f72d0eb2eee`

| Workflow | Run | Conclusion |
| --- | --- | --- |
| Windows package and release | [31851996367](https://github.com/Ding-Ding-Projects/material-virtualbox/actions/runs/31851996367) | **success**, 1h26m24s — published `v7.2.97-ci.108` |
| Material 3 documentation Pages | [31851996366](https://github.com/Ding-Ding-Projects/material-virtualbox/actions/runs/31851996366) | **success** |
| Material 3 validation | [31856303192](https://github.com/Ding-Ding-Projects/material-virtualbox/actions/runs/31856303192), [31851996370](https://github.com/Ding-Ding-Projects/material-virtualbox/actions/runs/31851996370), [31851966893](https://github.com/Ding-Ding-Projects/material-virtualbox/actions/runs/31851966893) | **FAILURE** |

**`Material 3 validation` is red on `main` and has been since `fe321a4`.** The failing step is
`Validate MD3 source wiring` and the message is
`Compact README contract missing: no verified installer is published yet`. The step required that
literal in `README.md`; `README.md` had correctly stopped saying it once real installers were
published, so the assertion was demanding a false statement. Gate 2 below is that same step, and it
would have thrown on the tree at `fe321a4` for the same reason.

The repair is commit
[`ca97ec93ccc06559d1035dd039ef56ca7b000a93`](https://github.com/Ding-Ding-Projects/material-virtualbox/commit/ca97ec93ccc06559d1035dd039ef56ca7b000a93),
which replaces the stale literal with assertions pinning what is true now and adds two negative
guards so the claim cannot be upgraded to a *signed* installer without signing evidence. **It is on
`claude/yum-tong-finish-20260816` and is not pushed, so `main` is still red as this is written.**
Gate 2's re-run below is local evidence that the repaired step passes; it is not evidence that CI
has passed it, and no such evidence exists yet.

## Gate inventory

| # | Gate | Command | Scope | Run? | Result | Evidence |
| --- | --- | --- | --- | --- | --- | --- |
| 1 | MD3 design ledger + archive manifest regeneration | `./tools/md3/generate-design-coverage.ps1` then assert 69 `ARCH-` rows, 69 manifest lines, `git diff --exit-code -- doc/md3/DesignCoverage.md doc/md3/ArchiveManifest.sha256` | `doc/md3/DesignCoverage.md`, `doc/md3/ArchiveManifest.sha256`, `design/` archive | **RUN** | **PASS** | stdout: `Generated 69 archive rows.`; exit 0; `git status --short` after the run showed **no diff** — the committed ledger and manifest are byte-identical to a fresh regeneration. Runtime ~2.5s. **Re-run 2026-08-16 at `992aa84304a`** under PowerShell 7.6.5: stdout `Generated 69 archive rows.`, and `git diff --stat -- doc/md3/DesignCoverage.md doc/md3/ArchiveManifest.sha256` printed nothing — still byte-identical. |
| 2 | MD3 source-wiring contract (kBuild source lists, headers, wizard/palette/style/settings/manager/tab/regex/history/notification/theme source-text contracts, README + `docs/index.html` contracts) | Extracted verbatim from `md3-validation.yml`'s "Validate MD3 source wiring" step; run via `pwsh -NoProfile -File step2-md3-source-wiring.ps1` from the repo root | ~35 production `.cpp`/`.h` files under `src/VBox/Frontends/VirtualBox/src/md3/`, `Makefile.kmk`, `README.md`, `docs/index.html` | **RUN** | **PASS** | **Re-extracted and re-run after the `docs/index.html` language-switcher contract was added to this step**, because the earlier recorded run predates that change and no longer describes the step that exists. The step body is now 257 lines of YAML; it was de-indented into a scratch `step2-md3-source-wiring.ps1` outside the repository and invoked from the repository root under PowerShell 7.6.5. stdout: `step2 PASSED`; exit 0; runtime 0.62s. The earlier "~200 individual string/regex contracts" estimate is left uncorrected only in the sense that it was never counted precisely; the language-switcher block adds 13 required literals, 1 middle-dot literal, 3 negative guards, and 9 occurrence-count assertions on top of it. Seven deliberate mutations of `docs/index.html` (removing `role="radiogroup"`, renaming the mode list, changing the bilingual join to `/`, changing the persistence key, pasting a build hash into a Cantonese attribute, giving a switcher button `role="tab"`, and switching a node builder to `innerHTML`) were each confirmed to make this gate throw, and the file was restored and the gate re-run to `step2 PASSED` / exit 0 afterwards. **Independently re-run 2026-08-16 at `992aa84304a`**, because this is the step that is red in CI on `main` and a second pair of hands should confirm the repair rather than inherit it: lines 30–286 of `md3-validation.yml` were de-indented verbatim into a scratch `step2.ps1` outside the repository, invoked from the repository root under PowerShell 7.6.5 — stdout `step2 PASSED`, exit 0, 0.48s. **This is a local run against a tree that carries `ca97ec93ccc`. CI has never executed this version of the step; at `fe321a4`, which lacks the repair, CI's copy of this same step throws `Compact README contract missing: no verified installer is published yet` (run 31856303192).** |
| 3 | Notification search integration contract | Extracted from `md3-validation.yml`'s "Validate notification search integration" step | `src/VBox/Frontends/VirtualBox/src/notificationcenter/UINotificationCenter.cpp` | **RUN** | **PASS** | stdout: `step3 PASSED`; exit 0. Runtime ~0.5s. |
| 4 | Notification row accessibility contract | Extracted from `md3-validation.yml`'s "Validate notification row accessibility" step | `src/VBox/Frontends/VirtualBox/src/notificationcenter/UINotificationObjectItem.cpp` | **RUN** | **PASS** | stdout: `step4 PASSED`; exit 0. Runtime ~0.5s. |
| 5 | Tab group picker contract | Extracted from `md3-validation.yml`'s "Validate tab group picker integration" step | `src/VBox/Frontends/VirtualBox/src/md3/UIMd3TabStrip.{h,cpp}` | **RUN** | **PASS** | stdout: `step5 PASSED`; exit 0. Runtime ~0.5s. |
| 6 | Tonal-palette / HCT colour-science source contract | Extracted from `md3-validation.yml`'s `tonal-palette-contract` job step | `src/VBox/Frontends/VirtualBox/src/md3/UIMd3Hct.{h,cpp}`, `UIMd3Theme.cpp`, `Makefile.kmk` | **RUN** | **PASS** | stdout: `step6 PASSED`; exit 0. Runtime ~0.6s. Confirms no `M_PI`, no HSL/HSV approximation, and the `tstUIMd3Hct` kBuild wiring text is present — **this checks the wiring exists, not that the testcase compiles or its numeric assertions pass** (see gate 9). |
| 7 | Release line-count and attribution table | `pwsh -NoProfile -File tools/count-release-lines.ps1` | Every `git ls-files`-tracked file (66,320 files **at `cb9f573030e`**) | **RUN** at `cb9f573030e`; **NOT re-run** since | **PASS** (self-checks all satisfied; exit 0, not the documented failure exit code 2) | Project total 26,272 files / 9,955,529 lines / 9,070,265 non-blank (vendored excluded); grand total 66,320 files / 25,865,478 lines. Agent-written (surviving): 23,721 lines / 21,713 non-blank, from 192 discovered agent commits blamed across 161 candidate files (123 blamed, 38 binary). Runtime 135s. Full table in this audit's run log. **Input drift noted 2026-08-16:** `git ls-files \| wc -l` now returns **66,376**, so every figure in this cell describes a tree 56 files smaller than the current one. The 135-second script was **not** re-run this pass, so no corrected totals are offered — the PASS verdict is not disproven, it is simply older than the tree it describes, and anyone quoting these numbers should re-run the script first. |
| 8 | GitHub Pages static-site assembly (`pages.yml` build job's file-copy step) | `Copy-Item docs\* _site -Recurse -Force; Copy-Item doc\md3\*.md _site -Force; Copy-Item doc\md3\ArchiveManifest.sha256 _site -Force` | `docs/`, `doc/md3/*.md`, `doc/md3/ArchiveManifest.sha256` | **RUN** (locally reproduced into a scratch directory, not the real Pages job) | **PASS** | 23 files copied with no errors, exit 0, runtime ~0.5s. This only proves the copy step itself does not fail on missing source files; it does not exercise `actions/upload-pages-artifact` or `actions/deploy-pages`, which need a GitHub Pages environment this lane does not have. |
| 9 | `tstUIMd3Hct` compiled tonal-palette testcase | `kmk -C src/VBox/Frontends/VirtualBox` (target defined in `Makefile.kmk`, source at `src/VBox/Frontends/VirtualBox/testcase/tstUIMd3Hct.cpp`) | HCT/CAM16 colour-science numeric assertions | **NOT RUN** | — | **Blocker:** no `kmk` on `PATH` (`which kmk` failed), no configured Qt install, no `env.bat`/`AutoConfig.kmk` in this checkout (`configure.ps1` was never run here). Compiling a single testcase still requires the full kBuild + Qt + MSVC toolchain the task explicitly excludes. Gate 6 verifies only that this testcase's *source wiring* exists in `Makefile.kmk`, not that it compiles or that its assertions pass — `doc/md3/CodexHandoff.md` itself states "Source-pattern CI is a regression guard. It cannot prove … Prefer adding an executable gate; the `tonal-palette` job is the pattern to copy," which is exactly this gap. |
| 10 | IPRT / Runtime testcases | `kmk -C src/VBox/Runtime/testcase` (per `AGENTS.md` §5) | `src/VBox/Runtime/testcase/` | **NOT RUN** | — | Same blocker as gate 9: requires `kmk`, a configured build tree, and `VBOX_WITH_TESTCASES=1`. None are present in this checkout. |
| 11 | Storage testcases | `kmk -C src/VBox/Storage/testcase` (per `AGENTS.md` §5) | `src/VBox/Storage/testcase/` | **NOT RUN** | — | Same blocker as gate 9. |
| 12 | ValidationKit ISO / test suite | `kmk -C src/VBox/ValidationKit validationkit-iso` (per `AGENTS.md` §5); running the actual guest/host test suite additionally needs a built, installed VirtualBox and guest images (`src/VBox/ValidationKit/readme.txt`) | `src/VBox/ValidationKit/` | **NOT RUN** | — | Same build blocker as gate 9, compounded by needing an installed VirtualBox with guest VMs to execute the suite itself — explicitly out of scope per this task's instructions. |
| 13 | `scm` source-code massager / style linter | `scm` (compiled from `src/bldprogs/scm.cpp` and friends via kBuild; referenced in `AGENTS.md` §6/§14 and `README.md`'s contributing section) | Every tracked source file's header/style conventions | **NOT RUN** | — | `scm` is a compiled C++ program, not a script — `src/bldprogs/scm.cpp`, `scm.h`, `scmrw.cpp`, `scmparser.cpp`, etc. exist but there is no prebuilt `scm` binary anywhere in this checkout and building one needs the same kBuild toolchain gate 9 lacks. |
| 14 | Qt translation catalog regeneration (`lupdate`/`lrelease`) | `kmk` targets defined around line 2159–2184 of `src/VBox/Frontends/VirtualBox/Makefile.kmk` ("lupdate all languages (nls/\*.ts)") | 80 `.ts` files under `src/VBox/Frontends/VirtualBox/nls/` | **NOT RUN** | — | Requires Qt's `lupdate`/`lrelease` tools from a configured Qt install and `kmk`; neither is present. No Cantonese (`zh_HK`/`yue`) `.ts` file exists in `nls/` — the MD3 rewrite's English/Cantonese/bilingual modes are implemented through a separate in-code `UIMd3Language` text registry (`src/VBox/Frontends/VirtualBox/src/md3/UIMd3Language.h`), not the classic `.ts` pipeline, so this gate would not even cover the new UI's localization. **Updated 2026-08-16:** that localization coverage is now gate 19's job, not this one's; what gate 19 still cannot see is listed under "Removed from the list above, because the gate now exists". |
| 15 | Full Windows build (`tools/build-windows.ps1 -Mode Build`) | `tools/build-windows.ps1` (or CI's equivalent hand-rolled bootstrap + `configure.ps1` + `kmk`) | Entire product | **NOT RUN** | — | Explicitly excluded by this task's instructions (takes over an hour; needs Qt 6.8.3, WDK 10.0.22621, WDK 7.1 compatibility libraries, Windows SDK, vcpkg, NASM, WiX, MSVC v143). Read but not edited or invoked. |
| 16 | Unsigned NSIS installer packaging (`tools/build-windows.ps1 -Mode Installer`) | `tools/build-windows.ps1 -Mode Installer` | Packaged installer from a prior build's `out\win.amd64\release\bin` | **NOT RUN** | — | Depends on gate 15's build output existing first (`out/` does not exist in this checkout — verified with `ls out` → "No such file or directory"). Read but not invoked. **Corrected 2026-08-16:** this row was titled "Squirrel installer packaging". `tools/build-windows.ps1:2` describes itself as an "unsigned NSIS host installer packaging helper" and lines 8–18 record that the Squirrel.Windows path was deleted outright in 2026-08; `-Mode Installer` now compiles `src\VBox\Installer\win\NSIS\VBoxHostInstaller.nsi` via `tools\build-windows-nsis-installer.ps1`. |
| 17 | Custom NSIS log-enabled packaging tool build (`tools/prepare-nsis.ps1`) | `tools/prepare-nsis.ps1 -EnvironmentFile .\env.bat` | `tools/win.x86/nsis/v3.10-log-r1/` | **NOT RUN** | — | Requires `env.bat` (generated only by `configure.ps1`, which was never run in this checkout — no `env.bat`, `AutoConfig.kmk`, or `out/` present), plus 7-Zip, curl, Python, and a Visual Studio `vcvarsall.bat`, and itself invokes `kmk` to build zlib/`VBoxPeSetVersion` prerequisites. Read but not invoked. |
| 18 | Doxygen API documentation generation | `doxygen Doxyfile.Core` / `src/VBox/Frontends/VirtualBox/Doxyfile` / `src/VBox/Main/Doxyfile.Main` / `src/VBox/Runtime/Doxyfile` | Doxygen comment coverage across the tree | **NOT RUN** | — | `doxygen` (v. resolved at `/c/Strawberry/c/bin/doxygen`) is installed on this host, but **no workflow in this repository invokes Doxygen at all** — it is not a gate the project currently enforces anywhere, local or CI. Running a full multi-target Doxygen pass over this tree was judged not "cheap" (four separate large Doxyfiles covering the whole product) and was not attempted. Recorded here rather than silently run or silently ignored. |
| 19 | MD3 language registry completeness (localization gate) | `pwsh -NoProfile -File tools/md3/check-language-registry.ps1 -MaxUnparseable 2` | 427 compile-time `registerText` registrations resolving to 407 distinct keys across 22 sources under `src/VBox/Frontends/VirtualBox/` | **RUN** | **PASS** — but only after the defect it found on its own first run was fixed | Added after this document's header audit, so **this row was verified against a later tree than `cb9f573…`, not against that commit.** The first run exited 1: `FAIL  CONFLICTING KEY  'md3.wizard.current-page-description' registered 2x with 2 different texts:`, naming `src/VBox/Frontends/VirtualBox/src/md3/UIMd3Wizard.cpp:51 EN='Title of the active wizard page.'` and `src/VBox/Frontends/VirtualBox/src/wizards/UINativeWizard.cpp:91 EN='Current wizard page: %1'`. That is a real accessibility defect, not a cosmetic clash: `registerText` ends in `QHash::insert`, which overwrites, so whichever `registerMd3*Texts()` free function runs last wins and the other reader is always wrong — either `UINativeWizard.cpp`'s `.arg(pPage->title())` finds no `%1` and silently drops the page title from the page stack's accessible description, or `UIMd3Wizard.cpp`'s title label announces a literal `%1`. Fixed in commit [`ca97ec93ccc06559d1035dd039ef56ca7b000a93`](https://github.com/Ding-Ding-Projects/material-virtualbox/commit/ca97ec93ccc06559d1035dd039ef56ca7b000a93) by renaming the native wizard's key to `md3.wizard.page-stack-description` at both its registration and its one reader; the shared MD3 wizard shell keeps the generic name. The re-run printed `parsed registrations : 427`, `distinct keys        : 407`, `files scanned        : 22`, `violations: 0   warnings: 0`, `RESULT: clean`, exit 0, runtime ~1.9s. It also found 0 empty keys, 0 empty English strings and 0 empty Cantonese strings; 20 keys are registered more than once with *identical* text (deliberate co-registration by two owners) and pass by design; the only two identical English/Cantonese pairs — `md3.application` (product name) and `md3.regex.flags-placeholder` (PCRE2 flag letters `i m s x`) — sit in the script's hand-maintained allow-list with a written reason each, and any other identical pair warns (or fails under `-StrictIdentical`). **Standing blind spot the gate prints on every run, including clean ones:** 2 call sites in `src/VBox/Frontends/VirtualBox/src/manager/UIVirtualBoxManager.cpp` (lines 2711 and 2834) take key/English/Cantonese as runtime lambda parameters; the 41 registrations behind them (33 `registerManagerText`, 8 `registerGlobalToolCommand`) are counted as `unparseable, NOT CHECKED` and are **not** verified by this gate. CI passes `-MaxUnparseable 2` to freeze that number so a third invisible call shape fails the build instead of disappearing. **Independently re-run 2026-08-16 at `992aa84304a`** under PowerShell 7.6.5. Verbatim stdout, complete:<br>`== UIMd3Language registry completeness ==`<br>`  parsed registrations : 427`<br>`  distinct keys        : 407`<br>`  files scanned        : 22`<br>`  unparseable, NOT CHECKED : 2`<br>`    ? src/VBox/Frontends/VirtualBox/src/manager/UIVirtualBoxManager.cpp:2711  non-literal argument`<br>`        registerText(QString::fromLatin1(pszKey), strEnglish, strCantonese)`<br>`    ? src/VBox/Frontends/VirtualBox/src/manager/UIVirtualBoxManager.cpp:2834  non-literal argument`<br>`        registerText(QString::fromLatin1(pszKey), strFallback, strCantonese)`<br>`  violations: 0   warnings: 0`<br>`  RESULT: clean`<br>exit 0. Note that the gate prints its own blind spot on a clean run — the two unparseable sites are in the passing output above, not hidden behind a failure. |
| 20 | Documentation-site language switcher, exercised in a real browser | A scratch Node 26 script outside the repository driving Chrome `--headless=new` over the DevTools Protocol against `file:///…/docs/index.html`: real `Input.dispatchKeyEvent` key presses, `Network.enable` request capture, `Emulation.setScriptExecutionDisabled`, and `Page.captureScreenshot` | `docs/index.html` only | **RUN** | **PASS** — 48 of 48 checks, exit 0 | Verified behaviour, not source text: English default with zero CJK in the page copy; the switcher revealed only by script; `aria-checked`/roving `tabindex` correct in all three modes; ArrowRight/ArrowLeft/ArrowUp/ArrowDown/Home/End all select and wrap; the existing Overview/Coverage/Verification tab strip still answers ArrowRight and End with the right panel shown; `document.title`, the tablist accessible name and the `role="status"` live region all follow the mode; `localStorage` holds `0`/`1`/`2` under `md3.language.mode`, a reload restores the stored mode, `banana` falls back to English and `9` clamps to bilingual; every visible technical fact (3 build hashes, both commit SHAs, both run numbers, `MSVC 14.44`, `Windows SDK 10.0.26100.0`, `Qt 6.8.3`, `#6750A4`, the build timestamp) renders exactly once in English and once in Cantonese, and in bilingual mode only the two facts that sit inside a translated paragraph are cloned; the 23 distinct `href` values are byte-identical in all three modes; exactly one network request is made across a load and two mode changes, and it is the page itself; with script execution disabled the English lede and the whole Overview panel are still in the document and the switcher stays `hidden`. Screenshots of all three modes at 1100×900 were captured and read. **This harness is a one-off local verification and is not checked into the repository, so it is not a repeatable gate** — nothing in CI or in `tools/` reproduces it, and a future regression in switcher *behaviour* (as opposed to the source literals gate 2 pins) would not be caught automatically. |

## Gates this project should have and does not

This is the part a validation guard can never produce on its own, because it
only ever reports on gates that already exist.

- **No *automated* capture gate runs anywhere, in CI or locally.** **Corrected 2026-08-16:** this bullet used to read "No runtime capture / screenshot harness exists at all … every single row reads 'Not captured,' blocked by (A) no verified installer yet and (B) `REGDB_E_CLASSNOTREG`." All three of those clauses are now false and are corrected rather than softened. A harness does exist — `tools/capture/Invoke-CaptureHarness.ps1`, documented in [`CaptureHarness.md`](CaptureHarness.md). **8 of the 47** rows are captured (rows 1, 5, 6, 16, 22, 24, 27, 31; `grep -cE '^\| [0-9]+ \|' doc/md3/CaptureMatrix.md` → 47, the `**Captured**` count → 8) and 19 PNGs are committed under `doc/md3/captures/`. Blocker A is cleared: `gh release list` returns nine `v7.2.97-ci.*` releases and `v7.2.97-ci.108` carries a 106,963,266-byte `VirtualBox-7.2.97-Setup.exe`. What remains missing is the thing this list is for: **nothing invokes that harness automatically.** No workflow runs it, no gate fails when a surface regresses, and the 39 uncaptured rows are closed only by a human choosing to sit down at a Windows host. Every claim of "it renders" for the *application* that is not one of those 8 images still rests entirely on source-pattern text matching (gates 2–6 above), which `doc/md3/CodexHandoff.md` itself calls a "regression guard" that "cannot prove a button is visible, a key sequence works, or an accessibility tree is correct." Gate 20 drove a real browser, but against `docs/index.html` — a static documentation page — not against any built Qt binary, and its harness is not checked in either.
- **No automated accessibility verification.** The MD3 validation workflow checks that strings like `setAccessibleName`, `setAccessibleDescription`, and `QAccessible::Button` *appear in the source text*. Nothing verifies at runtime that the accessible tree is actually correct, that keyboard traversal reaches every control, that focus order is sane, or that screen-reader names resolve to the right values.
- **No automated dark/light/high-contrast/DPI verification.** `CaptureMatrix.md` rows 36–39 ("Light theme," "Dark/contrast theme," "Bilingual … language mode," "Visible keyboard-focus state") are cross-cutting requirements with zero automated coverage of any kind — not even a source-pattern check.
- **No installer functional/COM-registration test.** The Windows workflow verifies only that the produced assets are *unsigned* (`Get-AuthenticodeSignature` → `NotSigned`) and that the NSIS `VirtualBox-<version>-Setup.exe` exists. **Corrected 2026-08-16:** this bullet used to say "Squirrel assets" and to name `RELEASES` and a full `.nupkg`; neither exists in the NSIS path (`grep -n -i "RELEASES\|nupkg" .github/workflows/windows-package-release.yml` finds no such artifact), and Squirrel.Windows was retired on 2026-08-14. The gap itself is unchanged: nothing verifies the installer actually installs correctly, registers `VBoxSDS` and the COM classes it needs, or that an installed copy launches successfully. **And no installer test could ever lift the ceiling above it: `VBoxSup.sys` ships unsigned, 64-bit Windows refuses to load an unsigned kernel driver, code signing is permanently prohibited for this project, and therefore no virtual machine can start.** That is a policy consequence with an owner outside this codebase, not a missing gate.
- **No checked-in behavioural gate for the documentation site.** Gate 2 now pins the language switcher's source literals — the three mode names, the middle-dot bilingual join, the `md3.language.mode` key, the radiogroup ARIA, the no-`innerHTML`/no-subresource guards, and the per-fact occurrence counts — and that is a genuine regression guard against someone deleting or renaming the feature. It is still only text matching. Gate 20 proved the switcher actually *works* in a real browser, but its harness lives in a scratch directory outside the repository and is not reproducible by anyone else, so nothing checked in would notice if a future edit left the buttons present and inert, broke the roving `tabindex`, or made the Cantonese render swallow a link.
- **No lint or static-analysis gate, local or in CI.** `tools/pylintrc` exists but nothing in any workflow or documented local command invokes `pylint` against any tracked Python file (e.g. `configure.py`, `src/VBox/ValidationKit`'s Python sources). There is no `cppcheck`, `clang-tidy`, or equivalent C++ static-analysis pass anywhere in this repository. (Per this fork's standing user policy, GitHub Actions deliberately runs no tests and no lint — but that policy does not preclude a *local*, non-gating check existing for a human/agent to run before pushing, and none exists.)
- **No dependency or security scanning workflow.** No CodeQL, Dependabot, `npm audit`-equivalent, or SBOM/vulnerability-scanning workflow was found among the four discovered workflow files.
- **No automated verification of the release line-count self-check as a hard gate.** `tools/count-release-lines.ps1` (gate 7) does self-check its own arithmetic and exits 2 on a mismatch, but the release workflow's "Compose release content" step wraps it in `$ErrorActionPreference = 'Continue'` and `continue-on-error`, so a failing line count only degrades the release notes to an error message — it can never fail the release itself. That may be an intentional design choice (line counting is decoration, not a blocker, per this fork's release philosophy), but it means the "line counter" is not actually a *gate* today in the sense this document's other rows are; it is closer to best-effort reporting.
- **No auto-update feed validation.** `CaptureMatrix.md` rows 44–45 ("Auto-update — ready-to-restart banner" and "offline/invalid-feed fallback") exist as required capture rows but there is no evidence in this repository of an implemented auto-updater at all yet (no updater source files were found under a quick pass of `src/VBox/Frontends/VirtualBox/src/md3/` or `src/VBox/Installer/`); this is recorded as an open gap rather than assumed either way, since implementation-scope questions are out of this lane's remit.
- **No end-to-end "does the MD3 manager shell actually launch and respond to input" smoke test**, headless or otherwise — the closest thing on record is the `REGDB_E_CLASSNOTREG` failure documented in `RuntimeCapture.md`, which is a failure state, not a passing smoke test.

### Removed from the list above, because the gate now exists

A row leaves the missing-gates list only when something executable exists that a person can run and
watch fail. It is recorded here rather than deleted, with the hole it still leaves stated in the
same breath, so nobody mistakes "the gate exists" for "the risk is closed."

- **Localization completeness — was "No localization-completeness gate for the new MD3 text registry", now gate 19.**
  `tools/md3/check-language-registry.ps1` landed in
  [`ca97ec93ccc06559d1035dd039ef56ca7b000a93`](https://github.com/Ding-Ding-Projects/material-virtualbox/commit/ca97ec93ccc06559d1035dd039ef56ca7b000a93)
  and runs in `md3-validation.yml` with `-MaxUnparseable 2`. It verifies that every *statically
  visible* `registerText(key, english, cantonese)` call has a non-empty key, a non-empty English
  string and a non-empty Cantonese string, and that no key is registered twice with conflicting
  text. It failed on its own first run against a real defect, which is the only kind of first run
  worth trusting.
  **What it still does not cover, and what therefore remains genuinely ungated:** (a) keys that are
  read but never registered — 440 distinct `md3.*` string literals appear in the frontend sources
  against the 407 registered keys the gate can see, and that 33-key delta is **unaudited**; some of
  it is certainly behind the two invisible call sites, some may be genuinely unregistered, and this
  gate cannot tell them apart, so nobody should call it "33 missing keys" without checking each one;
  (b) `%1`/`%2` placeholder-arity agreement between the English and Cantonese strings
  (`md3.manager-tool.record-count` legitimately reorders them, so a naive arity check would be
  wrong); (c) which registration actually wins at run time, since `QHash::insert` overwrites and the
  winner depends on which `registerMd3*Texts()` free function runs last; (d) translation quality or
  house voice; (e) the 41 registrations behind `UIVirtualBoxManager.cpp:2711` and `:2834`, which it
  reports as unparseable rather than checking. The classic Qt `.ts`-file `lupdate`/`lrelease`
  pipeline (gate 14) covers none of this either, because the MD3 rewrite does not use `.ts` files
  for its own strings.

## Suggested articles

[`CaptureMatrix.md`](CaptureMatrix.md) and [`RuntimeCapture.md`](RuntimeCapture.md) for the
screenshot-evidence gap this inventory also documents; [`CodexHandoff.md`](CodexHandoff.md) for
the "source-pattern CI is a regression guard, not proof" framing this inventory is built on;
[`DesignCoverage.md`](DesignCoverage.md) for the 69-entry ledger gate 1 regenerates and checks;
[`../../CHANGELOG.md`](../../CHANGELOG.md) for the Windows packaging pipeline's fix history and
the exact prior failure gate 15/16/17 cannot currently reproduce locally.
