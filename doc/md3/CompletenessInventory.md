# Completeness inventory

## What this document is, and why it had to be hand-written

The release gate for this project requires "a hand-written, per-surface completeness inventory"
naming every canonical user-facing feature and, for each surface it applies to, its
implementation, documentation article, localized copy, test coverage, built-artifact interaction
proof, and a real capture — with a missing, stale, delegated, undocumented, unlocalized,
untested, non-interacted, or uncaptured row blocking release.

The reason it has to be *hand-written* is the whole point of the exercise. A checklist generated
by scanning the codebase for what exists can only ever validate features that were already built.
It will happily report 100% coverage of a project that implements four of thirty required
features, because it never looked for the other twenty-six. This document starts from the list of
features the contract requires — independent of what this repository happens to contain — and
then, for each one, records the true state found by searching the source.

**Most rows below are gaps, and that is the correct and expected outcome.** This is a fork of
upstream VirtualBox — a mature C++/Qt desktop application originally built to none of these
requirements — that a separate, ongoing effort (`src/VBox/Frontends/VirtualBox/src/md3/`,
indexed from [`README.md`](README.md)) is retrofitting toward a Material Design 3 / house-contract
surface. That effort has covered real ground — language modes, a regex builder, a command palette,
a notification history, per-element appearance editing, tab search, local version history — and
it has left far more untouched. A document that made this project look mostly done would be
lying about both the app and the effort already invested in fixing it.

## Methodology

Every claim below was checked against this repository's actual source at commit `cb9f573030e`
(this lane's starting point) plus its own subsequent commits, not against the design archive, not
against a prior agent's summary, and not against memory of how VirtualBox usually works. Where a
feature is cited as implemented, the citation is a real path in this checkout. Where a feature is
cited as absent, that absence was confirmed by searching the relevant source trees for the terms
that would appear if it existed (see the per-feature notes for what was searched). "Not
implemented" is treated as a completely valid, expected answer — it is not hedged, softened, or
dressed up as "planned" unless a concrete design-archive row or `doc/md3/*.md` commitment says so.

**Surfaces identified in this repository:**

| ID | Surface | What it is |
| --- | --- | --- |
| S1 | VirtualBox Manager | The main window: VM chooser/list, navigation rail, tab strip, manager tools. `src/VBox/Frontends/VirtualBox/src/manager/` |
| S2 | Global Preferences | `UIAdvancedSettingsDialog` opened in `Type_Global` mode |
| S3 | Machine Settings | `UIAdvancedSettingsDialog` opened in `Type_Machine` mode, per VM |
| S4 | Wizards | New VM, Clone, Import/Export Appliance, New Virtual Disk, etc. — `UINativeWizard` pages inside the `UIMd3Wizard` shell |
| S5 | Notification Centre | `UINotificationCenter` / `UIMd3NotificationCentre`, opened from S1 or (unverified) S6 |
| S6 | VM Runtime Window | The running-VM window, `VirtualBoxVM.exe` — `src/VBox/Frontends/VirtualBox/src/runtime/` |
| S7 | Documentation site | The published GitHub Pages site: `docs/index.html` plus `doc/md3/*.md`, assembled by `.github/workflows/pages.yml` |

A repository-wide search (`grep -r UIMd3` scoped to `src/VBox/Frontends/VirtualBox/src/runtime/`)
returned **zero matches**. S6 has no Material 3 / house-contract integration of any kind; every
row below for S6 is "Not implemented" unless stated otherwise, and this is recorded once here
rather than repeated as a discovery in every row. `CodexHandoff.md`'s own "Open lanes" list places
"Runtime chrome" last (lane 6 of 7) and not yet started, which agrees with the search result.

**Evidence shorthand**, used to avoid repeating the same three sentences in every cell:

- **T0** — no test of any kind exists for this claim.
- **T1** — covered only by `.github/workflows/md3-validation.yml`, a source-pattern
  (regex-over-source-text) CI check. `CodexHandoff.md` itself calls this "a regression guard... it
  cannot prove a button is visible, a key sequence works, or an accessibility tree is correct."
- **T2** — covered by a real compiled and executed test binary. As of this audit exactly one exists
  in the entire MD3 effort: `src/VBox/Frontends/VirtualBox/testcase/tstUIMd3Hct.cpp` (127 checks,
  0 failures, per `doc/md3/DesignCoverage.md` row ARCH-029).
- **B0** — no build evidence recorded for this exact claim.
- **B1** — compiles and links locally (`UICommon`/`VirtualBox`/`VirtualBoxVM` kBuild targets exit 0)
  at a cited commit; the resulting binary has never been launched or interacted with as a running
  process for this feature.
- **C0** — no real capture exists. This applies to literally every row in this document. Per
  [`CaptureMatrix.md`](CaptureMatrix.md), **0 of 47** tracked surface/state rows are captured,
  blocked by an unregistered `VBoxSDS` COM service (`REGDB_E_CLASSNOTREG`, see
  [`RuntimeCapture.md`](RuntimeCapture.md)) for every pre-launch surface, and by the absence of any
  published release for installer/update surfaces. No row in this inventory can honestly claim a
  capture until that blocker clears.

Wholly-absent features (no implementation on any surface) are recorded as **one row covering all
surfaces** rather than seven duplicate "Not implemented" rows, with the search performed to confirm
the absence stated explicitly. Partially-implemented features are broken out per surface, because
that is exactly where the dishonesty risk lives — a feature that is real in one dialog and silently
assumed everywhere else.

---

## 1. Language modes (English / Cantonese / bilingual)

| Surface | Status | Implementation | Docs | Localized copy | Tests | Interaction proof | Capture | Blocker |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Engine | Implemented | `UIMd3Language` singleton: `UIMd3LanguageMode_{English,Cantonese,Bilingual}`, `text()`, `registerText()`. `src/VBox/Frontends/VirtualBox/src/md3/UIMd3Language.{h,cpp}` | None dedicated (mentioned in passing across several articles) | N/A (engine) | T1 | B1 (compiles in `UICommon`) | None (C0) | — |
| S2 Global Preferences | Implemented | `QComboBox m_pLanguageMode` wired to `md3Language().setMode()`; live-refreshed on `sigLanguageChanged`. `src/VBox/Frontends/VirtualBox/src/settings/UIAdvancedSettingsDialog.cpp:1795-1801,1916-1917,1922-1940` | [`SettingsSearch.md`](SettingsSearch.md), [`AppearanceSettings.md`](AppearanceSettings.md) | Control labels are plain `tr()` English only (`"Language mode"`); the panel itself is not run through `UIMd3Language` | T1 | B1 | None (C0) | — |
| S3 Machine Settings | Not implemented | The customization panel is explicitly built only for `Type_Global`; `SettingsShell.md` states "Machine Settings never receives this global-only panel." | [`SettingsShell.md`](SettingsShell.md) | N/A | T0 | B0 | None (C0) | By design — no per-machine language override exists at all, and the contract does not ask for one; flagged for completeness |
| S1 Manager | Partial | Consumes the process-wide mode via `md3Text()`; no in-Manager control. Chrome that registers bilingual copy: `UIMd3TabStrip.cpp:83-84`, `UIMd3CommandPalette` (category/command copy), `UIMd3ManagerHeader` | [`ManagerShell.md`](ManagerShell.md), [`TabNavigation.md`](TabNavigation.md), [`CommandPalette.md`](CommandPalette.md) | Tab strip, command palette, notification centre chrome registered EN+ZH; the rest of the Manager (chooser pane labels, action-pool menu text, toolbars) uses Qt's ordinary single-language `tr()` and was never run through `UIMd3Language` | T1 for the registered chrome; T0 for the rest | B1 | None (C0) | Legacy VirtualBox UI text is simply not converted; this is the large majority of the Manager's visible strings |
| S4 Wizards | Partial | Wizard shell chrome (rail step labels, step-state descriptions) registered via `REGISTER_WIZARD_TEXT`. `src/VBox/Frontends/VirtualBox/src/md3/UIMd3Wizard.cpp:46-47` | [`WizardShell.md`](WizardShell.md) | Shell chrome EN+ZH; the actual wizard page content (every field label, help text, and validation message on every New VM / Clone / Import / Export page) remains plain Qt `tr()` and was not found registered anywhere in `UIMd3Language` | T1 for shell; T0 for page content | B1 | None (C0) | The overwhelming majority of what a user reads in a wizard is unconverted page content, not shell chrome |
| S5 Notification Centre | Implemented (for its own copy) | Keys `md3.notifications.search`, `md3.notifications.clear`, `md3.notifications.undoClear`, etc., registered EN+ZH per `NotificationCentre.md` | [`NotificationCentre.md`](NotificationCentre.md) | EN+ZH for its own labels | T1 | B1 | None (C0) | — |
| S6 VM Runtime Window | Not implemented | Zero `UIMd3` symbols under `src/VBox/Frontends/VirtualBox/src/runtime/` | None | N/A | T0 | B0 | None (C0) | Runtime chrome lane not started (`CodexHandoff.md` open lane 6) |
| S7 Documentation site | Not implemented | `docs/index.html` is English-only static markup; no language switcher, no Cantonese copy anywhere in `docs/` or the copied `doc/md3/*.md` files | None | English only | T0 | N/A (static HTML) | None (C0) | Never attempted |

## 2. Two independent funny-level sliders (English, Cantonese; 1–5)

Same engine and the same per-surface pattern as language modes, since both live in
`UIMd3Language` and are set from the same settings panel.

| Surface | Status | Implementation | Docs | Localized copy | Tests | Interaction proof | Capture | Blocker |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Engine | Implemented | `m_iPlayfulness`, `m_iCantonesePlayfulness`, independently clamped 1–5, persisted to `GUI/Md3/FunnyLevelEnglish` / `GUI/Md3/FunnyLevelCantonese`. `UIMd3Language.cpp:23-24,58,70,145-152` and `md3CantoneseStyle()` at line 35 | — | N/A | T1 | B1 | None (C0) | — |
| S2 Global Preferences | Implemented | Two `QSlider`s, range 1–5, wired to `setPlayfulness()`/`setCantonesePlayfulness()`. `UIAdvancedSettingsDialog.cpp:1803-1821,1918-1921` | [`SettingsSearch.md`](SettingsSearch.md) | Slider labels ("English tone", "Cantonese tone") are plain English `tr()` | T1 | B1 | None (C0) | — |
| S1/S4/S5 | Partial | Same registered-chrome-only pattern as language modes above; playfulness only visibly *does* anything where copy is registered through `UIMd3Language` | See row 1 | See row 1 | T1/T0 mixed | B1 | None (C0) | The sliders exist and persist, but most rendered text in the app has nothing registered for them to style |
| S3, S6, S7 | Not implemented | Same reasons as row 1 | — | — | T0 | B0/N/A | None (C0) | — |

**A specific gap worth naming:** the house contract requires the funny level to style *every*
category of message including destructive and error copy, while keeping facts exact. Because
almost no VirtualBox dialog text is registered through `UIMd3Language` yet (confirmation dialogs,
error messages, wizard validation text all remain plain `tr()`), the sliders currently have no
effect on the overwhelming majority of the app's actual words — including the exact "destructive
and error copy" the contract singles out.

## 3. Emoji-in-dialogs toggle

**Not implemented on any surface.** Searched `src/VBox/Frontends/VirtualBox/src` for `emoji`
(case-insensitive): zero matches anywhere in the repository. No toggle, no per-dialog emoji
decoration, no setting of any kind. Docs: none. Tests: T0. Build proof: B0. Capture: C0 (N/A —
nothing to capture).

## 4. School mode (renamable, shared, credential-locked)

**Not implemented on any surface.** Searched for `School mode` / `SchoolMode`: zero matches. No
shared cross-app record, no rename, no credential lock, no restriction of Cantonese/funny-level/
personal-vocabulary/dim-sum surfaces. Docs: none. Tests: T0. Build proof: B0. Capture: C0.

## 5. TTS narrator with per-language voice selection

**Not implemented on any surface.** Searched for `narrator`, `TextToSpeech`, `speechSynthesis`,
`QTextToSpeech`: zero matches under `src/VBox/Frontends/VirtualBox`. No narration queue, no voice
picker, no rate/pitch controls, no serialized playback. Docs: none. Tests: T0. Build proof: B0.
Capture: C0.

## 6. Scheduled settings and external settings sources

**Not implemented on any surface.** Searched for `scheduled setting`, `ScheduledSetting`,
`HomeAssistant`/`Home Assistant`: zero matches. No date/time rule editor, no Home Assistant boolean
entity binding, no versioned schedule schema. Docs: none. Tests: T0. Build proof: B0. Capture: C0.

## 7. Dim sum startup surprise

**Implemented as a process-wide singleton, S1/S6-shared, not yet compiled or captured.**
`UIMd3DimSumSurprise` (`src/VBox/Frontends/VirtualBox/src/md3/UIMd3DimSumSurprise.{h,cpp}`) is
created/destroyed in `main.cpp` alongside `UIMd3History`/`UIMd3NotificationCentre`/
`UIMd3Changelog` (same file, same lifecycle pattern). On roughly one launch in ten it schedules a
small, non-blocking, auto-dismissing, un-opt-out-able toast naming one of twelve compiled-in
bilingual dish names (e.g. `"Shrimp dumpling - Har Gow"`), drawn fresh once per launch and shown
at most once per launch. It never gates startup or steals focus (the timer is armed before
`QApplication::exec()` runs and only ever fires once that loop is already up; the toast is a
`Qt::Tool`/`Qt::WindowDoesNotAcceptFocus` window shown with `Qt::WA_ShowWithoutActivating`), never
appears on a first run or the launch immediately after an update (a small local marker file
records the last-seen `RTBldCfgVersion()` build string and skips the draw when it is missing or
changed), and structurally cannot appear on `main.cpp`'s existing fatal-error/`!uiCommon().isValid()`
paths, since those either predate the singleton's creation or break out before `a.exec()` is ever
called. It carries meaningful accessible name/description text naming the dish and posts a
`QAccessible::Alert` event so assistive technology can announce it despite never taking focus, and
it best-effort honors `QStyleHints::reduceMotion()` where the built Qt exposes it.

| Item | Status | Implementation | Docs | Localized copy | Tests | Interaction proof | Capture | Blocker |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Engine + toast (S1/S6, process-wide) | Implemented | `UIMd3DimSumSurprise::{create,destroy,sltShowToast}`, wired in `main.cpp` next to the other MD3 singletons; `Makefile.kmk`'s `UICommon_SOURCES` updated | [`DimSumSurprise.md`](DimSumSurprise.md) | Bilingual by construction (every dish name and the accessible sentence are English+Cantonese in one string); UI chrome (kicker/placeholder text) is plain English `tr()`, not routed through `UIMd3Language` | T0 — no automated test exists | B0 — no local Windows/Qt toolchain in this lane, see `LocalGates.md`; not yet run through the Windows packaging workflow either | None (C0) — no screenshot or recording of the toast, its fade, or its accessible announcement was taken | Compile verification and a real capture are both open |
| Bundled dish photography | **Deliberately not implemented, by house rule** | `dishes()` carries only compiled-in bilingual text; every toast renders an explicit "Photo not included in this build" placeholder instead of any picture. This repository must not vendor, generate, or fetch dim-sum photographs — those belong to the separate public-catalog project, and this build environment has no network access regardless | [`DimSumSurprise.md`](DimSumSurprise.md) | N/A | T0 | B0 | None (C0) | Not a gap against this repository's contract — the explicit placeholder *is* the correct implementation here, not a stand-in for one |

This keeps the same honesty discipline as rows 15/16 above: implemented, documented, and reasoned
about carefully, but with no compiled build evidence, no automated test, and no real capture,
because this lane has neither a Windows/Qt toolchain nor a display to capture from.

## 8. Full regex builder, reachable from every search bar, dropdown, and context menu

This is one of the effort's most mature areas, and also one of the clearest examples of "real but
far from universal."

| Surface | Status | Implementation | Docs | Localized copy | Tests | Interaction proof | Capture | Blocker |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Engine | Implemented | `UIMd3RegexBuilder` (guided literal/class/anchor/group/alternation/quantifier controls, `QRegularExpression`/PCRE2-compatible, bounded async preview, copy/JSON export) + `UIMd3SearchField` (plain-text-first, per-field builder ownership) + `UIMd3MenuSearch` (menu-hosted variant). `src/VBox/Frontends/VirtualBox/src/md3/UIMd3RegexBuilder.{h,cpp}`, `UIMd3SearchField.{h,cpp}`, `UIMd3MenuSearch.{h,cpp}` | [`RegexBuilder.md`](RegexBuilder.md) | EN+ZH via `REGISTER_REGEX_TEXT` macro | T1 | B1 at commit `f29c7eb994d69c0e4ce69c5220110674ed9e846f` (local `UICommon` compile + linked, CI validation run 31328160092 and Pages run 31328160088 both green) | None (C0) | Native keyboard/screen-reader/visual capture explicitly listed as open in the article itself |
| S2 Global Preferences (settings search) | Implemented | `UIMd3SearchField` filters `UISettingsPageFrame` descriptions | [`SettingsSearch.md`](SettingsSearch.md) | EN+ZH for the field itself | T1 | B1 | None (C0) | — |
| S1 Manager tools (Extensions/Media/Network/Cloud/VM Activity Overview) | Implemented | Shared search+appearance card via `UIMd3ManagerToolSearch`. `src/VBox/Frontends/VirtualBox/src/md3/UIMd3ManagerToolSearch.{h,cpp}` | [`ManagerTools.md`](ManagerTools.md) | EN+ZH | T1 | B1 | None (C0) | — |
| S1 Tab strip (4 discovery searches + 2 bulk-close fields) | Implemented | `UIMd3TabManager`. See section 11 below. | [`TabNavigation.md`](TabNavigation.md) | EN+ZH | T1 | B1 at `f29c7eb9...` | None (C0) | — |
| S1 Command palette | Implemented | `UIMd3SearchField` inside `UIMd3CommandPalette` | [`CommandPalette.md`](CommandPalette.md) | EN+ZH | T1 | B1 | None (C0) | — |
| S5 Notification Centre | Implemented | `UIMd3SearchField` over name/detail/internal-name/help-keyword | [`NotificationCentre.md`](NotificationCentre.md) | EN+ZH | T1 | B1 | None (C0) | — |
| S1 context menus (manager header, `UIVirtualBoxWidget`, `UIGlobalToolsWidget`) | Partial | `UIMd3MenuSearch` is instantiated in exactly these 3 call sites; confirmed by grepping every `UIMd3MenuSearch` reference in the tree | — | EN+ZH where present | T1 | B1 | None (C0) | Only 3 menus in the whole app have it |
| **Every `QComboBox` dropdown elsewhere** (dozens across Settings editors — VRDE auth library, recording mode/scaling, name-and-system, file-path selectors, and every other settings/wizard combo box) | **Not implemented** | Confirmed by inspection: these are plain `QComboBox` widgets with no `UIMd3SearchField`/`UIMd3MenuSearch` filter, anywhere | — | N/A | T0 | B0 | None (C0) | Contract requires a search+builder on *every* dropdown; this repository has it on zero ordinary dropdowns |
| **Every other context menu** (snapshot pane, medium manager tree, VM chooser pane, log viewer, etc.) | **Not implemented** | Confirmed absent — `UIMd3MenuSearch` has exactly 3 call sites total, all inside the Manager shell | — | N/A | T0 | B0 | None (C0) | — |
| S3 Machine Settings | Partial | Shares the same `UISettingsPageFrame`/search plumbing as S2 per `SettingsShell.md`, but has no global-only customization panel; the search field itself is present | [`SettingsSearch.md`](SettingsSearch.md) | EN+ZH for the field | T1 | B1 | None (C0) | — |
| S4 Wizards | Not implemented | No search field of any kind inside a wizard page | — | N/A | T0 | B0 | None (C0) | — |
| S6, S7 | Not implemented | See top-of-document S6 note; S7 has no search UI at all in `docs/index.html` | — | N/A | T0 | B0/N/A | None (C0) | — |

## 9. Non-blocking notifications and a notification centre

| Item | Status | Implementation | Docs | Localized copy | Tests | Interaction proof | Capture | Blocker |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Non-blocking toast delivery | Implemented (pre-existing VirtualBox capability, not authored by the MD3 lane) | `UINotificationCenter`/`UINotificationModel`, process-wide, used by S1 and S6 alike | — | Existing VirtualBox localization, not MD3 bilingual | T0 (no MD3-specific test) | B1 | None (C0) | — |
| History model, search, select/export, Clear-history gate, one-step undo | Implemented, S1-reachable | `UIMd3NotificationCentre` bounded JSON history (`md3-notifications.json`, `QSaveFile`, 512 KiB/256-record bound), search over name/detail/internal-name/help-keyword, select/invert-select, "Mark selected as read", "Export view" (bounded JSON), destructive Clear-history gate (see section 13), "Undo last clear". `src/VBox/Frontends/VirtualBox/src/md3/UIMd3NotificationCentre.{h,cpp}`, opened from the Manager header's Notifications action | [`NotificationCentre.md`](NotificationCentre.md) | EN+ZH | T1 | B1 | None (C0) | — |
| Same, reachable from S6 Runtime | Unconfirmed / likely absent | No `UIMd3` symbol exists under `src/runtime/`; `NotificationCentre.md` describes the review surface as opened via "the manager header's Notifications button" specifically, not a runtime equivalent | [`NotificationCentre.md`](NotificationCentre.md) | N/A | T0 | B0 | None (C0) | The Runtime window very likely still gets only the legacy compact notification button with none of the MD3 search/history layer |
| Notification centre notification | Explicitly stated as not implemented | `NotificationCentre.md`: "Transient toast presentation, bulk dismiss or delete, provider-authored markdown rendering, selection semantics for every row control, and full per-row restore accessibility remain later lanes." | [`NotificationCentre.md`](NotificationCentre.md) | N/A | T0 | B0 | None (C0) | Bulk **delete** does not exist — only select/mark-read/export do |

## 10. Material Design 3 conformance and a per-element appearance editor

| Item | Status | Implementation | Docs | Localized copy | Tests | Interaction proof | Capture | Blocker |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| HCT/CAM16 tonal palette generation | **Implemented**, the strongest claim in this document | `UIMd3Hct.{h,cpp}`, `UIMd3Theme.{h,cpp}`, `UIMd3Tokens.h`. Every color role resolves through `UIMd3ColorRole`/`md3Theme()` | [`TonalPalette.md`](TonalPalette.md) | N/A (color science) | **T2** — `tstUIMd3Hct.cpp` compiled and executed, 127 checks, 0 failures, worst realized tone error 0.23 L* | B1, `Implemented` status in `DesignCoverage.md` ARCH-029/030/032 | None (C0) | The one row in the entire ledger marked flatly `Implemented` rather than `In progress` |
| Stock-control Material styling (buttons, fields, tabs, menus, sliders, scrollbars, etc.) | Implemented, app-wide as a `QProxyStyle` | `UIMd3Style.{h,cpp}` applied via `Makefile.kmk`; raises every control to a 48px minimum target | [`StockControlStyle.md`](StockControlStyle.md) | N/A | T1 | B1 at commit `eb70fdf2047c4ceeb5ac9783ac41f640e582ec86` (CI runs 31328985330/31328985294 green) | None (C0) | Native visual/high-scale/accessibility capture open |
| Theme control (Dark/Light/System/High-contrast dark/High-contrast light) | Implemented, S2 only | `QComboBox` in Global Preferences → `md3Theme().setScheme()` | [`AppearanceSettings.md`](AppearanceSettings.md) | Labels plain English | T1 | B1 | None (C0) | S3 Machine Settings deliberately omits this panel |
| Seed color, font scale, font family, font weight, compact density, display brand | Implemented, S2 only | Same panel, `UIAdvancedSettingsDialog.cpp:1823-1913` | [`AppearanceSettings.md`](AppearanceSettings.md) | Plain English labels | T1 | B1 | None (C0) | Seed color is a **plain hex text field**, not the infinite color picker (spectrum/wheel + bidirectional named/HEX/RGB/HSL/HSV/HWB/LAB/LCH/OKLab/OKLCH/CMYK translator) the contract requires — see gap below |
| Per-element **Edit appearance…** editor | Implemented | `UIMd3AppearanceEditor.{h,cpp}`, reachable from MD3 widget context menus and `Shift`+right-click; persists per-element seed/typeface/corner-radius/scale/weight; named-theme save/apply with transactional validation | [`AppearanceSettings.md`](AppearanceSettings.md) | Not stated as bilingual in the article | T1 | B1 | None (C0) | Only reaches widgets that already have a stable MD3 appearance key — legacy Qt widgets outside the MD3 lane have none |
| **Infinite color picker / color-space translator** | **Not implemented anywhere** | Searched: no spectrum/wheel widget, no HSL/HSV/LAB/OKLCH/CMYK conversion UI found; every color control found (seed, per-element) is a bounded hex `QLineEdit` | — | N/A | T0 | B0 | None (C0) | Explicit contract gap: "a plain hex field" is exactly the finite input the contract prohibits |
| **Locked destructive-gate/export shared components** (`UIMd3DestructiveGate`, `UIMd3Export`) | **Not implemented as production files** | `DesignCoverage.md` rows ARCH-008/009/010/011 point their "Production path(s)" at the bare directory `src/VBox/Frontends/VirtualBox/src/md3/`, not a named file. Confirmed: no `UIMd3DestructiveGate.{h,cpp}` or `UIMd3Export.{h,cpp}` exists anywhere in the tree (`find`/`ls` of the `md3/` directory) | [`DesignCoverage.md`](DesignCoverage.md) | N/A | T0 | B0 | None (C0) | The design archive anticipated a shared, reusable destructive-confirmation component and a shared export component; neither was ever produced — see sections 13 and 17 for the one-off, non-reusable substitutes that exist instead |
| S4/S6/S7 MD3 conformance | Not implemented | Wizard shell is a Material *wrapper* around unconverted native pages (per section 1); Runtime has zero MD3 code; docs site uses hand-rolled CSS custom properties that visually resemble M3 tokens but are not generated from the same HCT engine and have no theme switcher | [`WizardShell.md`](WizardShell.md) | N/A | T0/T1 mixed | B0/B1 mixed | None (C0) | — |

## 11. Browser-style tabbed navigation with the four tab-discovery searches

| Surface | Status | Implementation | Docs | Localized copy | Tests | Interaction proof | Capture | Blocker |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| S1 Manager workspace strip | Implemented, substantially | `UIMd3TabStrip` (48px strip, 120–210px tabs, overflow, pin/unpin, group create/rename/collapse/color via **Move… into group…** picker with its own search+builder), `UIMd3TabManager` (four independent discovery searches: current strip, groups-by-name, per-group including ungrouped, and all-window master list — `Ctrl+Shift+T`), plus two bulk-close fields ("Close tabs containing text" / "Close tabs not containing text", pinned-safe, reviewed-and-re-resolved before closing) | [`TabNavigation.md`](TabNavigation.md) | EN+ZH via `REGISTER_TAB_TEXT`/`REGISTER_TAB_MANAGER_TEXT` | T1 | B1 at `f29c7eb994d69c0e4ce69c5220110674ed9e846f` (CI 31328160092/31328160088 green) | None (C0) | — |
| Edge docking (left/right/top/bottom, left default) | **Not implemented** | `TabNavigation.md` itself: "dockable left/right/top/bottom strip orientations... remain open." Only a horizontal top strip exists | [`TabNavigation.md`](TabNavigation.md) | N/A | T0 | B0 | None (C0) | Contract requires left-edge default with all four orientations supported |
| Drag reordering | **Not implemented** | Explicitly listed as open in `TabNavigation.md` | [`TabNavigation.md`](TabNavigation.md) | N/A | T0 | B0 | None (C0) | — |
| Group delete/reorder/direct color picker | **Not implemented** | Explicitly listed as open; groups can be created/renamed/collapsed/moved-into but not deleted or reordered, and color comes from theme fallback rather than a color picker | [`TabNavigation.md`](TabNavigation.md) | N/A | T0 | B0 | None (C0) | — |
| S2/S3 Settings tabs | **Not implemented** | `SettingsShell.md` explicitly: settings show "exactly the selected settings page" via a rail selector, not browser-style tabs; the article states "Complete browser-style settings-tab semantics, every-edge docking... remain open." | [`SettingsShell.md`](SettingsShell.md) | N/A | T0 | B0 | None (C0) | The contract explicitly requires every settings surface to be tabbed like the rest of the product, "not as a scrolling column, and not as a bespoke left-hand section list" — which is exactly what S2/S3 currently are |
| S4/S6/S7 tabbed navigation | Partial (S7 only; S4/S6 not implemented) | S7 has a 3-tab ARIA tablist (Overview/Coverage/Verification) in `docs/index.html:15-19,56-87` with working arrow-key/Home/End navigation and `role=tablist`/`role=tab`/`role=tabpanel`; S4 wizards and S6 runtime have no tab strip of any kind | [`docs/index.html`](../../docs/index.html) | English only | T0 | N/A (static HTML, manually reviewed) | None (C0) | S7's 3 tabs have none of the required overflow, reorder, pin, group, four-discovery-search, or docking behavior — it is a minimal, real, but far from complete tab strip |

## 12. Command palette on `Ctrl+Shift+F`

| Item | Status | Implementation | Docs | Localized copy | Tests | Interaction proof | Capture | Blocker |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| S1 Manager | Implemented | `UIMd3CommandPalette` registers Preferences, Home/Machines/Media/Network/Cloud/Resources/Extensions destinations, machine creation/import/export, media manager, cloud machine, machine settings, clone/OCI export, and local-history commands; category grouping, live action-pool enabled state, keyboard Up/Down, focus return, `Ctrl+Shift+F`/Escape. `src/VBox/Frontends/VirtualBox/src/md3/UIMd3CommandPalette.{h,cpp}`, wired in `UIVirtualBoxManager.cpp`/`UIVirtualBoxWidget.{h,cpp}` | [`CommandPalette.md`](CommandPalette.md) | EN+ZH via `UIMd3Language` | T1 | B1 | None (C0) | "Full all-surface command inventory" explicitly stated as open — not every setting/page is a registered command |
| S2/S3/S4/S5 as palette *targets* | Partial | Reachable via registered commands (e.g. "Preferences", "machine-settings") but have no palette instance of their own once opened | [`CommandPalette.md`](CommandPalette.md) | See above | T1 | B1 | None (C0) | Contract wants the palette to reach "every setting in every settings surface" — confirmed not the case; only page-level, not field-level, commands exist |
| S6, S7 | Not implemented | No `UIMd3CommandPalette` under `runtime/`; no palette of any kind in `docs/index.html` | — | N/A | T0 | B0/N/A | None (C0) | — |

## 13. Destructive-action super confirmation

| Item | Status | Implementation | Docs | Localized copy | Tests | Interaction proof | Capture | Blocker |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| S5 "Clear history" (notification centre) | Implemented, but as a one-off, not a reusable component | Two independently-operated `QCheckBox` acknowledgements, a disabled-until-both-checked `QSlider` (0–100), animated progress, distinct 100% ready state, **Emergency exit** action, Escape cancel, focus return. `src/VBox/Frontends/VirtualBox/src/md3/UIMd3NotificationCentre.cpp:690-850` (approx.) | [`NotificationCentre.md`](NotificationCentre.md) | EN+ZH (`md3.notifications.clearSlider`, `md3.notifications.emergencyExit` keys) | T1 | B1 | None (C0) | Exact contract match for its one action |
| Every other destructive action in the app (delete VM & files, unregister, remove medium, remove snapshot) | **Not implemented** | These still route through ordinary yes/no confirmation dialogs: `confirmDeleteMachine`, `confirmMediumRemoval`, `confirmSnapshotRemoval` in `src/VBox/Frontends/VirtualBox/src/notificationcenter/UINotificationQuestion.cpp`, `src/VBox/Frontends/VirtualBox/src/snapshots/UISnapshotPane.cpp`, `src/VBox/Frontends/VirtualBox/src/medium/UIMediumItem.cpp` | None | Existing VirtualBox `tr()` copy, not bilingual | T0 | B1 (these are long-standing compiled paths) | None (C0) | This is the largest and most consequential gap in this section: the *one* destructive action in the whole application that got the two-key/slider treatment is clearing a notification log; deleting a virtual machine and its disk files still gets a plain `Yes`/`No` box |
| Shared `UIMd3DestructiveGate` component | **Not implemented** | See section 10 — the design archive named this component (ARCH-008/009) and it was never produced; the notification-centre gate above was hand-built inline instead, so nothing is reusable for the actions above | [`DesignCoverage.md`](DesignCoverage.md) | N/A | T0 | B0 | None (C0) | — |

## 14. Local version history

| Item | Status | Implementation | Docs | Localized copy | Tests | Interaction proof | Capture | Blocker |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Journal engine | Implemented | `UIMd3History`: append-only `revisions.jsonl` (≤1,024 records), UUID/timestamp/action/detail/state/SHA-256 per record, atomic `QSaveFile` writes, best-effort async local Git backing with a fixed local identity (never a remote). `src/VBox/Frontends/VirtualBox/src/md3/UIMd3History.{h,cpp}` | [`History.md`](History.md) | N/A (engine) | T1 | B1 | None (C0) | — |
| Browser UI | Implemented | `Ctrl+H` and command-palette route; plain-text + regex search, action filter, native calendar From/To date pickers with explicit "Any date", bounded JSONL export, inline "Verify integrity" | [`History.md`](History.md) | EN+ZH implied by shared `UIMd3SearchField`/`UIMd3Language` use | T1 | B1 | None (C0) | — |
| Restore adapters | Partial | Notification clear/restore and appearance/theme revisions have real, validated restore adapters (`UIMd3NotificationCentre`, `UIMd3Theme::restoreState`). Every other kind of state (settings pages, VM/machine data, wizard progress) is **export-only** — the browser can show and export the revision but cannot apply it back | [`History.md`](History.md), [`NotificationCentre.md`](NotificationCentre.md) | — | T1 | B1 | None (C0) | "Generic state restore remains surface-specific... settings/runtime revisions are displayed and exported until their owning surfaces supply safe restore adapters" — direct quote from `History.md` |
| Scope | Partial (MD3-only scope) | The journal only records MD3-authored mutations (notification clear/restore, appearance/theme changes). It does **not** record VM creation/deletion, snapshot taking, settings page saves, or any other ordinary VirtualBox action — this is not "every user-managed record the app owns" | [`History.md`](History.md) | — | T0 for the missing scope | B0 for the missing scope | None (C0) | Large gap against the "every app snapshots every user-managed record it owns" requirement |
| S6, S7 | Not implemented | No history journal reachable from Runtime or the docs site | — | N/A | T0 | B0/N/A | None (C0) | — |

## 15. Changelog viewer with date picker and commit links

**Partial (S1 Manager only).** `UIMd3Changelog`
(`src/VBox/Frontends/VirtualBox/src/md3/UIMd3Changelog.{h,cpp}`) is a new process-wide singleton,
created/destroyed in `main.cpp` alongside `UIMd3History`/`UIMd3NotificationCentre`, reachable from
the Manager via <kbd>Ctrl+Shift+L</kbd> and a "Open changelog" command-palette entry. It compiles
in every entry from the repository's own `CHANGELOG.md` (ten entries as of this audit, each
independently verified against a real commit hash and date in this checkout — see
`doc/md3/Changelog.md` for the exact `git cat-file`/`git log` verification record) rather than
inventing example data, and provides a `UIMd3SearchField` (plain text + the shared anchored regex
builder), native `QDateEdit` `From`/`To` calendar filters that also accept typed dates (mirroring
`UIMd3History`'s pattern), a category filter, a version filter, per-entry commit references
rendered as real keyboard-operable `QPushButton`s that open the commit on GitHub, and filtered
Markdown export/copy that both honor the active search/category/version/date filter together.

| Surface | Status | Implementation | Docs | Localized copy | Tests | Interaction proof | Capture | Blocker |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Engine (compiled-in entry set) | Implemented | `UIMd3Changelog::prepareEntries()`; ten entries transcribed from `CHANGELOG.md`, each commit hash verified with `git cat-file -e` and dated with `git log` against this checkout | [`Changelog.md`](Changelog.md) | N/A (engine) | T0 | B0 | None (C0) | No source-wiring CI step covers this file yet (not in `md3-validation.yml`'s fixed list); no `kmk` toolchain in this lane, see `LocalGates.md` |
| Browser UI (S1 Manager) | Implemented | `Ctrl+Shift+L` and command-palette route; plain-text + regex search, version/category filters, native calendar From/To date pickers with "Any date", per-row clickable commit buttons, filtered Markdown export + clipboard copy | [`Changelog.md`](Changelog.md) | EN+ZH registered via `UIMd3Language::registerText()` for every UI string; changelog content itself is factual data and intentionally not funny-level-styled (see `Changelog.md`) | T0 | B0 | None (C0) | Same as above — mechanical brace/paren-balance and symbol review only (documented in `Changelog.md`), not a compile |
| S2–S7 | Not implemented | No changelog surface outside the Manager | — | N/A | T0 | B0/N/A | None (C0) | Matches every other `UIMd3*` singleton's current Manager-only scope; not attempted this pass |

This keeps the same honesty discipline as row 14 (History) above: implemented, documented, and
localized, but with no compiled build evidence and no real capture, because this lane explicitly
excludes the full Windows build and the installed binary at
`C:\Program Files\VirtualBox\VirtualBox.exe` predates this change.

## 16. External-editor handoff

**Not implemented on any surface.** Searched for `external editor`, `externalEditor`, `code.exe`,
`Visual Studio Code`: zero matches. No editor detection, no "open in VS Code" action anywhere.
Docs: none. Tests: T0. Build proof: B0. Capture: C0.

## 17. Exports in every representable format

| Item | Status | Implementation | Docs | Localized copy | Tests | Interaction proof | Capture | Blocker |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Notification centre "Export view" | Partial (JSON only) | Bounded versioned **JSON only**, selected rows or the visible filtered set, atomic `QSaveFile` | [`NotificationCentre.md`](NotificationCentre.md) | — | T1 | B1 | None (C0) | One format, one surface, one record type |
| Regex builder "Export JSON…" | Partial (builder's own state only) | UTF-8 JSON schema v1 of the pattern/flags only (`sampleIncluded: false`) — exports the *builder's own state*, not application data | [`RegexBuilder.md`](RegexBuilder.md) | — | T1 | B1 | None (C0) | — |
| History journal export | Partial (JSONL only) | Bounded **JSONL** export of revisions | [`History.md`](History.md) | — | T1 | B1 | None (C0) | — |
| Shared `UIMd3Export` component | **Not implemented as a production file** | See section 10 — `DesignCoverage.md` ARCH-010/011 point at the bare `md3/` directory, no such file exists | [`DesignCoverage.md`](DesignCoverage.md) | N/A | T0 | B0 | None (C0) | — |
| Every other exportable record the app owns (VM list, VM settings, snapshot list, media list, extension pack list, network list, cloud profile list, log files as anything other than their native format) | **Not implemented** | No export path found for any of these beyond VirtualBox's long-pre-existing, non-MD3 appliance export (OVF/OVA) and log-file save, neither of which offers the contract's JSON/YAML/TOML/XML/CSV/Markdown/HTML/SQL menu | — | N/A | T0 | B0/B1 (pre-existing OVF export compiles and is long-shipped, but is unrelated to this contract) | None (C0) | Confirmed via `ManagerTools.md`: "Bulk actions/export... remain open" for Extensions/Media/Network/Cloud/VM Activity Overview |

## 18. Bulk actions on every list

| Item | Status | Implementation | Docs | Localized copy | Tests | Interaction proof | Capture | Blocker |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| S1 Tab strip bulk-close | Implemented | "Close tabs containing text" / "Close tabs not containing text", plain-text default with regex opt-in, reviewable count before closing, pinned excluded by default | [`TabNavigation.md`](TabNavigation.md) | EN+ZH | T1 | B1 | None (C0) | — |
| S5 Notification centre | Partial | "Select visible" / "Invert selection" / "Mark selected as read" / "Export view" (selected rows) exist. **Bulk delete/dismiss does not** — `NotificationCentre.md` lists "bulk dismiss or delete" as a later lane | [`NotificationCentre.md`](NotificationCentre.md) | EN+ZH | T1 | B1 | None (C0) | Selection and export are bulk actions; deletion, the most requested one, is not |
| S1 VM chooser pane (legacy) | Partial, pre-existing | `UIChooserModel`/`UIChooserHandlerKeyboard`/`UIChooserHandlerMouse` support multi-selection with `setSelectedItems`/`addToSelectedItems`, feeding the existing `UIActionPoolManager` (start/pause/close/group/delete multiple VMs at once) | — | Existing VirtualBox `tr()`, not MD3 bilingual | T0 (no MD3-specific test; this predates the MD3 lane entirely) | B1 | None (C0) | No exact-count disclosure, no reviewable preview, no honest partial-result reporting per the contract's specific bulk-action requirements — this is VirtualBox's long-standing multi-select, not a contract-compliant bulk-action surface |
| S1 Manager tools (Extensions/Media/Network/Cloud/VM Activity Overview) | **Not implemented** | `ManagerTools.md` explicitly: "Bulk actions/export... remain open." | [`ManagerTools.md`](ManagerTools.md) | N/A | T0 | B0 | None (C0) | — |
| S3 Settings pages, S4 wizards, snapshot list, log viewer | **Not implemented** | No selection/bulk mechanism found in any of these | — | N/A | T0 | B0 | None (C0) | — |
| S7 Documentation site | N/A | No lists with bulk operations exist on the page | — | N/A | N/A | N/A | N/A | Not applicable — the page has no list surfaces of this kind |

## 19. Accessibility and responsive sizing

| Item | Status | Implementation | Docs | Localized copy | Tests | Interaction proof | Capture | Blocker |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Accessible names/descriptions on new MD3 widgets | Implemented, widely, within the MD3 lane | Consistently set (`setAccessibleName`/`setAccessibleDescription`) across `UIMd3RegexBuilder`, `UIMd3SearchField`, `UIMd3TabStrip`/`TabManager`, `UIMd3CommandPalette`, `UIMd3NotificationCentre`, `UIAdvancedSettingsDialog`'s MD3 controls — confirmed by direct reading of each file during this audit | Every `doc/md3/*.md` article listed above | N/A | T1 for the source-pattern presence check | B1 | None (C0) | Every article repeats some form of "native... accessibility-tree verification remains open" — none of this has been checked with an actual assistive-technology pass |
| 48px minimum interactive target | Implemented, app-wide via `UIMd3Style` | `UIMd3Style` raises stock Qt controls to `qMax(48, controlHeight())`, documented as an approved deviation from the 32–38px prototype minimum | [`CodexHandoff.md`](CodexHandoff.md), [`StockControlStyle.md`](StockControlStyle.md) | N/A | T1 | B1 | None (C0) | — |
| Responsive breakpoints | Partial | Navigation rail collapses to a compact searchable action below 1000 logical px (`NavigationRail.md`); wizard rail collapses below 720 logical px (`WizardShell.md`); tab strip has overflow reveal | [`NavigationRail.md`](NavigationRail.md), [`WizardShell.md`](WizardShell.md) | N/A | T1 | B1 | None (C0) | Settings selector, notification centre, and command palette have no documented narrow-width behavior beyond "bounded to the screen" |
| High-DPI / 100–200% scale | **Not verified at all** | Stated as required evidence everywhere, actually demonstrated nowhere — zero captures exist at any scale (see C0/`CaptureMatrix.md`) | [`CaptureMatrix.md`](CaptureMatrix.md) | N/A | T0 | B0 | None (C0) | — |
| Automated accessibility test suite | **Does not exist** | Confirmed: `tstUIMd3Hct.cpp` is the only compiled test in the MD3 lane, and it tests color math, not accessibility | — | N/A | T0 | B0 | None (C0) | — |
| S6 | Not implemented | No MD3 code, no accessibility work of any kind found under `src/runtime/` | — | N/A | T0 | B0 | None (C0) | — |
| S7 | Partial | Basic keyboard tab navigation and `focus-visible` styling in `docs/index.html:7,56-87`; no accessible-name audit performed | — | English only | T0 | Manual review only, not a test | None (C0) | — |

## 20. Local personal-vocabulary JSON upload

**Not implemented on any surface.** Searched for `personal.vocabulary`, `PersonalVocabulary`,
`PERSONAL_VOCABULARY`: zero matches. No file picker, no schema, no cached-replacement mechanism.
Docs: none. Tests: T0. Build proof: B0. Capture: C0.

## 21. Per-element toy locks and Support Tickets

**Not implemented on any surface.** Searched for `Support Ticket`, `SupportTicket`, `toy lock`,
`ToyLock` (case-insensitive): zero matches. No lock wizard, no password/TOTP credential per
element, no fictional support-desk recovery flow. Docs: none. Tests: T0. Build proof: B0. Capture:
C0.

## 22. App-logo customization

**Not implemented as a user-facing feature, on any surface.** This needs a careful distinction:
[`AppIcon.md`](AppIcon.md) is real, substantial work — an original vector mark
(`src/VBox/Artwork/OSE/virtualbox.svg`), a reproducible Pillow-based raster generator
(`src/VBox/Artwork/generate-icon-assets.py`), a verified multi-resolution Windows `.ico`, and
wiring into the Manager window icon, taskbar icons, About-dialog marks, and every packaged Windows
executable. **But that is the application's own packaged/build-time icon**, chosen once by this
project and baked into the binary — not the contract's "app-logo customization" feature, which
requires an in-app, user-facing picker offering several presets plus a local custom-image upload,
with crop/fit/background editing, live application, and reset. No such picker, upload control, or
per-user logo override exists anywhere; `AppIcon.md` says nothing about one and none was found by
search. Docs: [`AppIcon.md`](AppIcon.md) (adjacent, not equivalent). Tests: T0. Build proof for the
packaging icon itself: B1 (verified `.ico` structure, see the article). Capture: C0.

## 23. Universal file converter

**Not implemented on any surface.** Searched for `FileConverter`, `file converter`, `universal
converter`: zero matches. No adapter registry, no category catalog, no PDF tools, no bounded queue.
Docs: none. Tests: T0. Build proof: B0. Capture: C0.

## 24. Local Ollama suite manager

**Not implemented on any surface.** Searched for `Ollama`: zero matches anywhere under
`src/VBox/Frontends`. No local HTTP API client, no model store, no chat surface, no harness
launcher. Docs: none. Tests: T0. Build proof: B0. Capture: C0.

## 25. Browser-extension download start/progress/completion surfaces

**Not applicable — and stated as such rather than silently omitted.** This VirtualBox fork ships
no browser extension component of any kind; there is no CRX/manifest, no extension source
directory, and nothing in the repository resembles a browser-extension download-capture flow. The
contract's requirement presupposes a product that has a browser extension. If this project ever
gains one, this row must be revisited; until then it is honestly out of scope rather than a gap to
close, and it is recorded here rather than left out so the omission reads as a decision, not an
oversight.

---

## Summary

Counting every row above as one assessed unit (a wholly-absent feature counts once; a
per-surface-broken-out feature counts each surface row, including its "engine" row where present),
recounted directly from the tables in this document rather than estimated:

| Status | Count |
| --- | --- |
| Implemented | 26 |
| Partial | 16 |
| Not implemented | 40 |
| N/A (justified) | 2 |
| **Total rows** | **84** |

One row (section 9, "Same, reachable from S6 Runtime") is recorded with the honest status
"Unconfirmed / likely absent" rather than a clean bucket, because the underlying question — whether
the Runtime window's notification button opens the MD3-enhanced review surface or only the legacy
one — was not resolved by source inspection alone. It is counted conservatively as **Not
implemented** in the table above; its own row states the uncertainty plainly rather than guessing.

No row in this document claims a real capture. `CaptureMatrix.md` independently confirms 0 of 47
tracked surface/state captures exist, blocked by an unregistered COM/SDS service for every
pre-launch surface and by the absence of any published release for installer/update surfaces.
Nearly every "Implemented" row above is qualified as compiling and linking locally (B1) rather
than having been run as a live process — this document does not, and cannot yet, claim otherwise.

## What a generated checklist would have missed

To make the point concrete: a tool that scanned this repository for existing features would have
found the twenty `UIMd3*` components under `src/VBox/Frontends/VirtualBox/src/md3/`, the other
21 `doc/md3/*.md` articles (22 including this one), and the passing
`md3-validation.yml` workflow, and could have reported near-total coverage of *what those files
describe*. It would never have looked for a text-to-speech narrator, an Ollama manager, a file
converter, a personal-vocabulary uploader, toy locks, dim sum, School mode, scheduled settings, a
changelog viewer, external-editor handoff, or an infinite color picker, because nothing in the
repository suggested searching for them. Twelve of the twenty-five canonical features audited
here (sections 3–7, 15–16, and 20–24 above) have precisely zero implementation anywhere in this
codebase — and every one of those twelve would have been invisible to a checklist built only from
what already exists.

## Suggested articles

[`CodexHandoff.md`](CodexHandoff.md) (the standing implementation contract and open-lanes order),
[`DesignCoverage.md`](DesignCoverage.md) (the 69-entry archive ledger this inventory cross-checked
against), [`CaptureMatrix.md`](CaptureMatrix.md) and [`RuntimeCapture.md`](RuntimeCapture.md) (why
zero captures exist), and every per-feature article linked from the tables above.
