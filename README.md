# Material Virtual Machine

Material Virtual Machine is a Material Design 3 rewrite of the Qt desktop
front end for VirtualBox. It keeps the VirtualBox engine, COM/XPCOM contracts,
machine models, action pools, and kBuild targets intact while moving the
application-owned presentation toward one coherent Qt 6 design system.

> **Implementation status:** the first production integration lane is landed in
> the working tree: shared `UIMd3Theme` persistence and `UIMd3Style` palette
> propagation are wired through the existing `UICommon` target and application
> lifecycle. The manager, settings, wizard, tool, and runtime shells remain
> in progress. Build and release claims below are deliberately bounded.

## Contents

- [Design package](#design-package)
- [Architecture](#architecture)
- [Build and prerequisites](#build-and-prerequisites)
- [Verification boundaries](#verification-boundaries)
- [CI and Pages](#ci-and-pages)
- [Contributing](#contributing)
- [Security and license](#security-and-license)

## Design package

The checked-in [`design/`](design/) directory is the authoritative review
package for the rewrite. It contains five native-Qt reference prototypes,
shared interaction data, C++/kBuild starter references, icons, and the
implementation handoff.

| Surface | Prototype |
| --- | --- |
| Virtual machine manager | [`Manager.dc.html`](design/Manager.dc.html) |
| Machine settings | [`VM Settings.dc.html`](design/VM%20Settings.dc.html) |
| Runtime window | [`Runtime Window.dc.html`](design/Runtime%20Window.dc.html) |
| Wizards | [`Wizards.dc.html`](design/Wizards.dc.html) |
| Manager tools and preferences | [`Managers.dc.html`](design/Managers.dc.html) |

No application screenshots are published yet. The archive contains a design
thumbnail, but it is intentionally not presented as runtime evidence. README
Screenshots will be added only after the native Qt app is built, launched, and
captured from the rewritten surfaces; mockups and static HTML previews do not
count.

The handoff requires accounting for all 69 archive entries. The maintained
ledger is [`doc/md3/DesignCoverage.md`](doc/md3/DesignCoverage.md), with its
reproducible hash list in [`doc/md3/ArchiveManifest.sha256`](doc/md3/ArchiveManifest.sha256).
Regenerate both with `pwsh -NoProfile -ExecutionPolicy Bypass -File
tools/md3/generate-design-coverage.ps1` after changing the design package.

## Architecture

This is one VirtualBox frontend, not a parallel demo application.

- **Manager:** refactor `UIVirtualBoxManager` and `UIVirtualBoxWidget` visually
  while retaining `UIActionPoolManager`, `UIChooser`, `UIToolPane`, and their
  existing models and signals.
- **Runtime:** add Material chrome around `UIMachineWindow` and
  `UIMachineView`; do not replace guest display, capture, session, or
  multi-monitor ownership.
- **Settings and wizards:** host the existing `UISettingsPage` and
  `UINativeWizardPage` implementations in Material shells without bypassing
  validation, serializers, progress, or cleanup.
- **Shared UI:** place genuinely shared tokens, theme, style, search, language,
  notification, history, accessibility, and safe utility code in the
  `UICommon` boundary. Manager-only code belongs to `VirtualBox`; runtime-only
  chrome belongs to `VirtualBoxVM`.
- **Persistence:** use VirtualBox extra data and existing settings APIs. Do
  not introduce a second preferences database.

The target visual system is Qt 6 Material Design 3 with seed `#6750A4`, dark
first-run presentation, comfortable density, a frameless platform-aware title
bar, keyboard and screen-reader support, and preserved translations. The
visual rewrite must not weaken hardening, authentication, destructive-action
confirmation, or VM/session safety.

## Build and prerequisites

VirtualBox is a large native project. Follow the canonical
[build instructions](https://www.virtualbox.org/wiki/Build_instructions) and
the local coding guidance before attempting a full build.

For an external checkout, the supported shape is:

```text
py -3 configure.py
kmk
```

For a narrower target after the environment is configured:

```text
kmk -C src/VBox/Frontends/VirtualBox
```

The build requires a compatible compiler, Qt 6 development files, kBuild
(`kmk`), platform SDKs, and the other dependencies selected by
`configure.py`. Do not copy generated output or machine-local settings into
the source tree; use `LocalConfig.kmk` for local overrides.

**Current checkout boundary:** this Windows checkout now has a verified native
`VirtualBox` target build. The local build used the bundled kBuild executable,
MSVC 14.44, Windows SDK 10.0.26100.0, Qt 6.8.3 plus the official `qtscxml`
add-on, WDK headers, Mako, GNU Bison/Flex/M4, NASM, and `xsltproc`. The
generated executable links and installs successfully. Full runtime manager
capture remains blocked by the checkout's unregistered `VirtualBoxClient` COM
runtime (`REGDB_E_CLASSNOTREG`); no mock screenshot is counted as GUI proof.

## Verification boundaries

Static inspection and the native build prove the design package, the 69-entry
ledger, the shared MD3 theme/style integration, and the compiled VirtualBox
target. The following remain open:

- compilation of `VirtualBoxVM` and the complete release packaging path;
- Qt widget, accessibility, keyboard, localization, and persistence tests;
- Windows frameless title-bar, DPI, snap-layout, and focus validation;
- manager, settings, wizard, manager-tool, notification, and runtime screenshot
  capture from a built artifact;
- installer, release, update, and signed/unsigned artifact verification.

When the toolchain is available, run the narrowest relevant gates first, then
the full GUI target. Validation Kit and runtime testcase commands are
documented in [`AGENTS.md`](AGENTS.md); they are not represented as passed by
this README.

## CI and Pages

This mirror contains MD3 validation and GitHub Pages workflows under
`.github/workflows/`. Validation run `31299627844` passed for commit
`f1efef9ac03`, and Pages run `31299627815` passed for the same commit; the
published site at <https://ding-ding-projects.github.io/material-virtualbox/>
returned HTTP 200. The next runs verify the title-bar commit
`97249e1bfe6`.

When publication work is added, it must build from the intended commit, keep
artifact and test evidence separate, publish only verified outputs, and expose
the documentation site from the repository homepage. A green static check
must not be described as a successful GUI build or release.

## Contributing

Read [`AGENTS.md`](AGENTS.md), [`CONTRIBUTING.md`](CONTRIBUTING.md), the Qt
coding section in [`doc/VBox-CodingGuidelines.cpp`](doc/VBox-CodingGuidelines.cpp),
the makefile guidance, and [`SECURITY.md`](SECURITY.md) before editing.

Keep changes narrow and tied to the real VirtualBox models and action pools.
Do not add HTML, React, browser-engine, CDN, or network dependencies to the
production frontend. Add new source files with the repository's standard
headers and properties, update the relevant manual and changelog entries, run
`scm` checks when available, and report every unrun gate and external blocker.

The canonical repository is SVN; this checkout is also mirrored on GitHub.
Do not assume the GitHub mirror contains newer internal fixes. Commits,
branches, tags, and publication should follow the repository owner's explicit
workflow.

## Security and license

Report vulnerabilities using the process in [`SECURITY.md`](SECURITY.md), not
through public issue details. VirtualBox is distributed under GPLv3 as shown
in [`COPYING`](COPYING), with additional third-party license notices in
[`THIRD_PARTY_LICENSES.txt`](THIRD_PARTY_LICENSES.txt).

Copyright (C) 2025 Oracle and/or its affiliates.
