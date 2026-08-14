# Material Design 3 GUI rewrite

This section records the native Qt implementation of the supplied Material Design 3 archive. The HTML files and `support.js` are design authorities only; production behavior remains in the existing `VirtualBox`, `VirtualBoxVM`, and `UICommon` targets.

## Coverage and evidence

- [`CodexHandoff.md`](CodexHandoff.md) is the standing implementation contract: ownership that stays with existing VirtualBox code, the precedence order when the prototype and the native contract disagree, the approved native deviations, and the definition of done. Read it before starting a surface.
- [`DesignCoverage.md`](DesignCoverage.md) is the 69-entry design ledger.
- [`ArchiveManifest.sha256`](ArchiveManifest.sha256) is generated from the checked-in archive files.
- `tools/md3/generate-design-coverage.ps1` regenerates both files deterministically.
- The implementation starts with the shared theme lifecycle and persistence bridge, then composes manager, settings, wizard, tool, and runtime surfaces around their existing models and action pools.
- [`TonalPalette.md`](TonalPalette.md) documents the HCT/CAM16 palette generation, the core-palette chroma rules, gamut handling, and the compiled tonal-palette testcase.
- [`ManagerShell.md`](ManagerShell.md) documents the responsive 48/48/92 desktop manager composition, compact navigation action below 1000 logical pixels, destination heading, on-demand workspaces, contextual actions, and padded machine cards.
- [`ManagerTools.md`](ManagerTools.md) documents the shared Material search and appearance card over the real Extensions, Media, Network, Cloud, and VM Activity Overview models, including bounded filtering and hidden-row restoration.
- [`NavigationRail.md`](NavigationRail.md) documents the first visible manager shell integration and its runtime capture gate.
- [`SettingsSearch.md`](SettingsSearch.md) documents the shared settings search field and its anchored regex builder.
- [`SettingsShell.md`](SettingsShell.md) documents the single-page Preferences/Machine Settings composition, cross-page search mode, and collapsed global-only customization panel.
- [`RegexBuilder.md`](RegexBuilder.md) documents the shared full guided builder, PCRE2-compatible dialect and flags, bounded asynchronous match/capture preview, copy/JSON export, accessibility, and per-field ownership.
- [`AppearanceSettings.md`](AppearanceSettings.md) documents the live Material scheme, seed, density, font-scale, display-brand controls, named-theme save/apply flow, and transactional per-element validation.
- [`StockControlStyle.md`](StockControlStyle.md) documents the shared semantic palette, state layers, focus treatment, and minimum anatomy applied to stock Qt controls without replacing their existing behavior.
- [`CommandPalette.md`](CommandPalette.md) documents the `Ctrl+Shift+F` registry-backed command palette and focus-return behavior.
- [`NotificationCentre.md`](NotificationCentre.md) documents the existing center's plain-text/bounded-regex filtering, persistent bounded history, selectable rows, filtered JSON export, the two-acknowledgement/full-slider Clear history gate, one-step Undo last clear recovery, and the idempotent single-refresh modeless lifecycle backed by the shared local history journal.
- [`History.md`](History.md) documents the isolated append-only local journal, SHA-256 state validation, Git best-effort backing, notification and appearance/theme revisions, and the modeless <kbd>Ctrl+H</kbd>/command-palette browser with action/date filters, regex search, JSONL export, integrity verification, and validated owner-routed restore adapters.
- [`WizardShell.md`](WizardShell.md) documents the responsive two-card Material 3 shell around the existing native wizard pages, including compact rail collapse, hidden-page mapping, internal page scrolling, localized step states, the named page stack, and accessible 48-pixel native actions.
- [`CodexHandoff.md`](CodexHandoff.md) records the native integration authority, target ownership, 69-entry ledger invariant, evidence boundary, and remaining implementation order for the next lane.
- [`TitleBar.md`](TitleBar.md) documents the frameless manager header, native Windows hit-testing, and language-aware window-control labels.
- [`TabNavigation.md`](TabNavigation.md) records the manager tab-strip behavior,
  including four independent tab-discovery searches, reviewable pinned-safe
  bulk close, scoped persistence, searchable group actions, keyboard context
  targeting, `PageTab` accessibility children, dedicated New tab/Tab manager
  actions, and the remaining docking, reordering, history, and runtime gaps.
- [`RuntimeCapture.md`](RuntimeCapture.md) records the genuine native screenshot contract and the current COM/service blocker.
- [`ExternalEditor.md`](ExternalEditor.md) documents the native "open in external editor" core: Visual Studio Code / Insiders / system-default detection, opening the VirtualBox configuration folder as a Code workspace root, the command-palette entry that reaches it, honest failure handling, and the persisted-picker follow-up.
- [`AppIcon.md`](AppIcon.md) documents the original Material mark, its committed SVG master and reproducible raster generator, the real multi-resolution Windows `.ico`, and every chrome/executable wiring point, including the icon now embedded in the shipped NSIS installer (`MUI_ICON`/`MUI_UNICON` in `VBoxHostInstaller.nsi`).
- [`CaptureHarness.md`](CaptureHarness.md) documents the runtime screenshot capture tool (`tools/capture/Invoke-CaptureHarness.ps1`): named off-screen desktop launch, dynamic window resolution and junk-window filtering, `PrintWindow` capture, a self-tested black-frame validator that never trusts `PrintWindow`'s return value alone, background input for driving the application without touching the visible desktop, and the JSON manifest it writes.
- [`CaptureMatrix.md`](CaptureMatrix.md) is the enumerated capture tracking table: every manager, settings, wizard, tool, runtime, and installer surface and state that must be photographed. Several rows are captured from the real installed build; every other row remains `Not captured`, with the exact blocker named per row.
- [`LocalGates.md`](LocalGates.md) is the hand-written local-suite inventory: every gate this repository can run locally, the exact command and result for each, why every non-runnable gate (compiled testcases, the full Windows build, `scm`, translation regeneration) is infeasible in a lane without the kBuild/Qt/MSVC toolchain, and the gates — runtime capture, accessibility, installer COM verification, MD3 localization completeness, lint/static analysis, security scanning — this project should have and currently does not.
- [`CompletenessInventory.md`](CompletenessInventory.md) is the hand-written, per-surface completeness
  inventory the release gate requires: every canonical house-contract feature checked against this
  repository's actual source, for every identified user-facing surface including the documentation
  site, with implementation paths, docs, localization, tests, build/interaction proof, capture
  status, and the exact blocker named per row. Most rows are gaps, and it explains why that is the
  correct and expected result of a genuinely hand-written audit rather than a generated checklist.

Build and runtime evidence must name the exact target, commit, host, Qt version, and whether the result is source-only, built, headless, or release-verified. Missing `svn`, `kmk`, or `scm` tools are reported as environment blockers rather than inferred passes.

## Windows build and packaging

- [`../../CHANGELOG.md`](../../CHANGELOG.md) records the Windows build/packaging pipeline's day-to-day fixes, each linked to its exact verified commit, and the current known issue blocking a green release build.
