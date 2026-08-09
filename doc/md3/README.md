# Material Design 3 GUI rewrite

This section records the native Qt implementation of the supplied Material Design 3 archive. The HTML files and `support.js` are design authorities only; production behavior remains in the existing `VirtualBox`, `VirtualBoxVM`, and `UICommon` targets.

## Coverage and evidence

- [`DesignCoverage.md`](DesignCoverage.md) is the 69-entry design ledger.
- [`ArchiveManifest.sha256`](ArchiveManifest.sha256) is generated from the checked-in archive files.
- `tools/md3/generate-design-coverage.ps1` regenerates both files deterministically.
- The implementation starts with the shared theme lifecycle and persistence bridge, then composes manager, settings, wizard, tool, and runtime surfaces around their existing models and action pools.
- [`NavigationRail.md`](NavigationRail.md) documents the first visible manager shell integration and its runtime capture gate.
- [`SettingsSearch.md`](SettingsSearch.md) documents the shared settings search field and its anchored regex builder.
- [`AppearanceSettings.md`](AppearanceSettings.md) documents the live Material scheme, seed, density, font-scale, and display-brand controls.
- [`CommandPalette.md`](CommandPalette.md) documents the `Ctrl+Shift+F` registry-backed command palette and focus-return behavior.
- [`NotificationCentre.md`](NotificationCentre.md) documents the existing center's plain-text/bounded-regex filtering, persistent bounded history, selectable rows, filtered JSON export, the two-acknowledgement/full-slider Clear history gate, and one-step Undo last clear recovery backed by the shared local history journal.
- [`History.md`](History.md) documents the isolated append-only local journal, SHA-256 state validation, Git best-effort backing, notification and appearance/theme revisions, and the modeless <kbd>Ctrl+H</kbd>/command-palette browser with action/date filters, regex search, JSONL export, integrity verification, and validated owner-routed restore adapters.
- [`WizardShell.md`](WizardShell.md) documents the embedded Material 3 shell around the existing native wizard pages, including localized step-state descriptions, the named page stack, and accessible native actions.
- [`TabNavigation.md`](TabNavigation.md) records the manager tab-strip behavior,
  including searchable group create/rename and per-group appearance actions,
  plus its remaining accessibility, grouping, and runtime gaps.
- [`RuntimeCapture.md`](RuntimeCapture.md) records the genuine native screenshot contract and the current COM/service blocker.

Build and runtime evidence must name the exact target, commit, host, Qt version, and whether the result is source-only, built, headless, or release-verified. Missing `svn`, `kmk`, or `scm` tools are reported as environment blockers rather than inferred passes.
