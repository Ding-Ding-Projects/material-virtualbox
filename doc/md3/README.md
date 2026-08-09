# Material Design 3 GUI rewrite

This section records the native Qt implementation of the supplied Material Design 3 archive. The HTML files and `support.js` are design authorities only; production behavior remains in the existing `VirtualBox`, `VirtualBoxVM`, and `UICommon` targets.

## Coverage and evidence

- [`DesignCoverage.md`](DesignCoverage.md) is the 69-entry design ledger.
- [`ArchiveManifest.sha256`](ArchiveManifest.sha256) is generated from the checked-in archive files.
- `tools/md3/generate-design-coverage.ps1` regenerates both files deterministically.
- The implementation starts with the shared theme lifecycle and persistence bridge, then composes manager, settings, wizard, tool, and runtime surfaces around their existing models and action pools.
- [`NavigationRail.md`](NavigationRail.md) documents the first visible manager shell integration and its runtime capture gate.

Build and runtime evidence must name the exact target, commit, host, Qt version, and whether the result is source-only, built, headless, or release-verified. Missing `svn`, `kmk`, or `scm` tools are reported as environment blockers rather than inferred passes.
