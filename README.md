# Material Virtual Machine

Material Virtual Machine is a Material Design 3 rewrite of the Qt desktop
front end for VirtualBox. It keeps the VirtualBox engine, COM/XPCOM contracts,
machine models, action pools, and kBuild targets intact while moving the
application-owned presentation toward one coherent Qt 6 design system.

**Documentation:** [Material Virtual Machine documentation](https://ding-ding-projects.github.io/material-virtualbox/)

**Install status:** no verified installer is published yet. Build from source
with the [canonical VirtualBox prerequisites and commands](#build-and-prerequisites).

> **Implementation status:** the shared theme, style, language, persisted brand,
> native manager title bar, manager navigation rail, manager tab strip, command
> palette, appearance editor, full guided shared regex builder, four-scope tab
> manager, single-page settings shell, settings search field, and the
> notification-center search and keyboard-accessible notification rows are wired
> into the existing VirtualBox frontend.
> The shared style now gives stock Qt controls Material semantic colors, shape,
> state layers, focus treatment, disabled presentation, and bounded minimum
> anatomy across manager, settings, wizard, and runtime-owned pages.
> Wizard, tool, and runtime shells remain in progress; the local history
> browser now restores validated notification and appearance/theme revisions
> through their owning services.
> Build and release claims below are deliberately bounded.

## Contents

- [Design package](#design-package)
- [Runtime screenshots](#runtime-screenshots)
- [Architecture](#architecture)
- [Build and prerequisites](#build-and-prerequisites)
- [Verification boundaries](#verification-boundaries)
- [CI and Pages](#ci-and-pages)
- [Contributing](#contributing)
- [Security and license](#security-and-license)

<details>
<summary><strong>Design package, native evidence, and implemented surfaces</strong></summary>

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

## Runtime screenshots

Screenshots in this section are reserved for captures from the built native
application. The current Windows runtime launch reaches the real executable but
stops before the manager shell at `REGDB_E_CLASSNOTREG` because the checkout's
`VirtualBoxClient` COM registration is incomplete. That genuine failure capture
is retained in the session evidence, but it is not presented as a successful
Material 3 manager screenshot. The design thumbnail and static HTML previews do
not count. The gallery will grow only with real manager, settings, wizard, tool,
notification, and runtime captures from the rewritten build.

The capture contract, required screenshot matrix, and current COM/service evidence
are maintained in [`doc/md3/RuntimeCapture.md`](doc/md3/RuntimeCapture.md).

Global Preferences and per-machine Settings now compose their existing page
models inside a bounded Material selector, one selected page card, and a compact
search/Basic/Expert header. An active query intentionally restores cross-page
results. Global Preferences alone offers a collapsed, persisted language and
appearance panel; Machine Settings no longer carries global controls. The shell
is documented in [`doc/md3/SettingsShell.md`](doc/md3/SettingsShell.md), while
the individual scheme, seed, density, typography, and display-brand controls
remain documented in
[`doc/md3/AppearanceSettings.md`](doc/md3/AppearanceSettings.md). Native visual
evidence is still subject to the capture gate above.

MD3 widgets with stable keys also expose a bounded per-element appearance
editor from their context menu and <kbd>Shift</kbd>+right-click. The editor
persists seed, typeface, corner radius, scale, and weight overrides and has a
reset path. Its live preview and editor-local regex search are live. The editor
also saves and applies complete named themes (including element overrides),
rejects malformed or out-of-range values transactionally, and returns focus
after close; the complete Word-depth typography picker remains tracked as open
design-coverage work.

The manager command palette is documented in
[`doc/md3/CommandPalette.md`](doc/md3/CommandPalette.md). `Ctrl+Shift+F` is
wired to live, category-grouped manager commands through a stable owner and
command-id registry with Home/Machines/Media/Network/Cloud/Resources/Extensions
destinations, action-pool enabled-state explanations, focusable unavailable rows,
UIMd3Language-backed English/Cantonese/bilingual copy with independent funny-level
refresh, exact handler-owned focus, bounded accessible results, and focus return;
its native capture remains pending the COM registration gate.

The frameless Windows manager title bar is documented in
[`doc/md3/TitleBar.md`](doc/md3/TitleBar.md). The 48-pixel header now presents a
compact application mark/name, manager subtitle, command-palette pill,
notification state, and icon window actions. Its real menu, window, palette,
and notification paths preserve existing authority while the hidden legacy
menu model opens on demand. Native snap, high-DPI, and bilingual captures remain
part of the deferred runtime evidence matrix.

The complete production composition is documented in
[`doc/md3/ManagerShell.md`](doc/md3/ManagerShell.md): a 48-pixel header,
48-pixel on-demand workspace strip, 92-pixel desktop rail with a compact
searchable navigation action below 1000 logical pixels, destination heading
with contextual action pills, and padded Machines chooser/workspace cards. The
serial Windows gate rebuilt and linked `UICommon` and `VirtualBox`, then
confirmed `VirtualBoxVM` was already up to date against that shared library;
real native capture remains a separate runtime gate.

The manager's 92-pixel navigation rail uses stacked icon-and-label destinations
in the prototype order. Its buttons must select the existing `UIToolType`
models, preserve expert-mode restrictions, open workspace tabs on demand, show
keyboard focus, and reflect the active theme. Up/Down traversal includes the
fixed Preferences destination, unavailable items explain the unmet condition,
and bundled icon masks are recolored for the active Material role and display
scale. A capture that cannot show those live behaviors is not accepted as GUI
proof. The underlying `UIToolsItem`
names and accessibility descriptions use stable `md3.tool.*` keys through the
persisted Material language service, refreshing on English/Cantonese/bilingual
and funny-level changes through a unique language connection while retaining
the existing Qt translation listener fallback.

The manager tab strip is documented in
[`doc/md3/TabNavigation.md`](doc/md3/TabNavigation.md). It delegates tab
selection to the existing global-tools model, opens visited destinations on
demand, migrates only the exact legacy generated seven-tab layout to one pinned
available destination, persists groups/pins/current selection under a scoped
VirtualBox extra-data key, keeps empty bulk-close queries safe, and exposes a
searchable Move… into group…
picker with member counts and an inline create-group path; the 48-pixel strip
now reserves 48 px focusable New tab, Tab manager, More-tabs, and inline close
targets. Overflow activation reveals the selected tab, active-tab close emits
one final fallback state, and the group picker is screen-bounded and rejects
hidden filtered results. A dedicated transparent child exposes only the real
PageTab roles; the other accessible actions remain siblings. Strip chrome also
offers bounded group creation and renaming plus a per-group Edit appearance…
action while retaining the local search field. Keyboard context menus reuse
the stable current tab so
<kbd>Shift+F10</kbd> exposes real tab-management actions; pointer chrome keeps
its strip-level menu. Activating a member of a collapsed group temporarily
reveals that tab without overwriting the group's collapsed preference.
<kbd>Ctrl+Shift+T</kbd> opens four independent discovery scopes (current strip,
group names, every individual group, and every registered window), each with
its own full guided regex builder. Separate containing and inverse bulk-close
fields require a reviewable, pinned-safe preview and re-resolve the model before
closing. Drag reordering, dockable strip orientations, complete group lifecycle,
settings/runtime adoption, tab-state history, and native capture remain open.

[`doc/md3/RegexBuilder.md`](doc/md3/RegexBuilder.md) documents the complete
plain-text-first builder shared by every Material search field: guided literals,
classes, anchors, groups, alternation and numeric quantifiers; raw PCRE2-compatible
syntax and `i`/`m`/`s`/`x` flags; local sample matches and capture groups;
copy/atomic JSON export; independent visible state; one-worker generation
control; a 300 ms UI deadline; bounded inputs/results; focus return; and
screen-bounded or menu-inline scrolling.

[`doc/md3/StockControlStyle.md`](doc/md3/StockControlStyle.md) documents the
shared `QProxyStyle` bridge for the existing stock Qt controls. It applies the
live semantic palette and typography to buttons, fields, choices, tabs, menus,
lists, headers, sliders, scroll bars, progress, toolbars, status bars, and
group-box frames while preserving their existing models, validation,
accessibility roles, and action ownership. Dedicated VirtualBox-painted widgets
remain separate rewrite lanes, and native visual proof remains deferred.

The existing notification center now has a Material 3 search field in its
extended view. [`doc/md3/NotificationCentre.md`](doc/md3/NotificationCentre.md)
documents plain-text and bounded regex filtering across real notification
metadata while preserving critical-item and blocking-operation behavior, and
retains non-blocking snapshots in a bounded searchable history model.
The manager header's Notifications button opens that review surface and shows
an unread marker. The history rows now support filtered selection, inversion,
selected-read updates, and bounded JSON export. **Clear history** uses an
app-owned destructive gate with the exact record count, two acknowledgements,
a full-range slider, animated progress, Emergency exit/Escape, and focus return;
the review surface also offers one bounded **Undo last clear** recovery
snapshot. Legacy notification rows expose an accessible name, visible keyboard
focus, and Enter/Return/Space details disclosure; pointer-only expansion is no
longer required. The shared [`doc/md3/History.md`](doc/md3/History.md) journal records
clear, restore, and later notification-change revisions in an isolated local
Git repository when Git is available, with an atomic-file fallback. The full
history browser is now reachable with `Ctrl+H` and from the command palette;
it provides plain-text-first search with the anchored regex builder,
action/date filters, bounded JSONL export, integrity verification, and a
validated Restore notification and appearance/theme state actions for supported
revisions. Surface-specific settings/runtime restore adapters, provider-authored
rendering, bulk dismiss/delete, complete row selection/restore semantics, and
native capture remain open lanes. This is
not a claim that the legacy notification surface has been fully replaced.

The existing New VM, New virtual disk, clone, import, and export flows now
compose an embedded Material 3 wizard shell around their real
`UINativeWizardPage` stacks. [`doc/md3/WizardShell.md`](doc/md3/WizardShell.md)
documents the translated page title, step summary, completion state, localized
current/completed/upcoming step descriptions, named page stack, and accessible
Back/Next/Finish/Cancel/Help actions while preserving validation/progress
contracts. Cross-wizard tabs, full validation summary details, runtime
accessibility-tree captures, and native runtime captures remain open.

The handoff requires accounting for all 69 archive entries. The maintained
ledger is [`doc/md3/DesignCoverage.md`](doc/md3/DesignCoverage.md), with its
reproducible hash list in [`doc/md3/ArchiveManifest.sha256`](doc/md3/ArchiveManifest.sha256).
Regenerate both with `pwsh -NoProfile -ExecutionPolicy Bypass -File
tools/md3/generate-design-coverage.ps1` after changing the design package.

</details>

<details>
<summary><strong>Architecture and target ownership</strong></summary>

## Architecture

This is one VirtualBox frontend, not a parallel demo application.

- **Manager:** refactor `UIVirtualBoxManager` and `UIVirtualBoxWidget` visually
  while retaining `UIActionPoolManager`, `UIChooser`, `UIToolPane`, and their
  existing models and signals. `UIMd3NavigationRail` now presents the global
  tool selection while the original model remains the source of truth.
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
- **Bundled assets:** the manager rail consumes the checked-in MD3 icon set
  through `src/md3/UIMd3Icons.qrc`, with a deterministic Qt standard-icon
  fallback if an individual resource cannot be loaded.
- **Tabbed manager navigation:** `UIMd3TabStrip` presents the global tools as
  browser-style tabs while preserving the existing `UIToolType` authority and
  using the shared UICommon extra-data persistence path.
- **Persistence:** use VirtualBox extra data and existing settings APIs. Do
  not introduce a second preferences database.

The target visual system is Qt 6 Material Design 3 with seed `#6750A4`, dark
first-run presentation, comfortable density, a frameless platform-aware title
bar, keyboard and screen-reader support, and preserved translations. The
visual rewrite must not weaken hardening, authentication, destructive-action
confirmation, or VM/session safety.

</details>

<details>
<summary><strong>Build from source and local prerequisites</strong></summary>

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

**Current verification boundary:** the evidence below was built from exact
settings-shell source commit [`152327ec9fe04455de55def8bd8e943138b75737`](https://github.com/Ding-Ding-Projects/material-virtualbox/commit/152327ec9fe04455de55def8bd8e943138b75737)
on a Windows x64 development host, using MSVC 14.44, Windows SDK
10.0.26100.0, Qt 6.8.3 with the official `qtscxml` add-on, and the bundled
kBuild executable. It is compile/link evidence, not a runtime or release claim.

| Target | Local result | Completed (UTC-04:00) | Installed artifact SHA-256 |
| --- | --- | --- | --- |
| `UICommon` | Exit 0, changed settings source compiled and linked | 2026-08-09 14:55:25 | `BC308CC34B9C8D749E67C0C625159E23C5EB9067EBF1B3E472DAB26E6D342893` |
| `VirtualBox` | Exit 0, dependency-current | 2026-08-09 14:45:57 | `127314B61C157DB98DF75990A10AE76E6216B135F614193BB1876E81CBC1FC0B` |
| `VirtualBoxVM` | Exit 0, dependency-current | 2026-08-09 14:46:24 | `2B6075A37083CCA25AE2DAB113C3B20A1136F76E35F8F864A0F93ACD4AA13F22` |

The serial target commands used the repository's configured environment and
checked `UICommon`, `VirtualBox`, and `VirtualBoxVM` separately. This proves the
changed settings translation units compiled and linked, and that the manager
and runtime targets remained dependency-current on that host. Full runtime manager
capture remains blocked by the checkout's unregistered `VirtualBoxClient` COM
runtime (`REGDB_E_CLASSNOTREG`); no mock screenshot is counted as GUI proof.
CI-only correction
[`f8c3bfffb55b8c8f0f0f884645afc1f0e99ade25`](https://github.com/Ding-Ding-Projects/material-virtualbox/commit/f8c3bfffb55b8c8f0f0f884645afc1f0e99ade25)
completed validation [run 31330546183](https://github.com/Ding-Ding-Projects/material-virtualbox/actions/runs/31330546183)
and Pages [run 31330546180](https://github.com/Ding-Ding-Projects/material-virtualbox/actions/runs/31330546180)
successfully; the deployed Home page and `SettingsShell.md` article returned
HTTP 200 with the new content.

</details>

<details>
<summary><strong>Verification boundaries and remaining runtime gates</strong></summary>

## Verification boundaries

Static inspection and the native build prove the design package, the 69-entry
ledger, the shared MD3 theme/style integration, and the compiled VirtualBox
target. The following remain open:

- the complete release packaging path;
- Qt widget, accessibility, keyboard, localization, and persistence tests;
- Windows frameless title-bar, DPI, snap-layout, and focus validation;
- manager, settings, wizard, manager-tool, notification, and runtime screenshot
  capture from a built artifact;
- installer, release, update, and signed/unsigned artifact verification.

When the toolchain is available, run the narrowest relevant gates first, then
the full GUI target. Validation Kit and runtime testcase commands are
documented in [`AGENTS.md`](AGENTS.md); they are not represented as passed by
this README.

</details>

<details>
<summary><strong>Continuous integration and GitHub Pages</strong></summary>

## CI and Pages

This mirror contains MD3 validation and GitHub Pages workflows under
`.github/workflows/`. Validation
[run 31330546183](https://github.com/Ding-Ding-Projects/material-virtualbox/actions/runs/31330546183)
and Pages
[run 31330546180](https://github.com/Ding-Ding-Projects/material-virtualbox/actions/runs/31330546180)
passed for CI-only corrective child commit
[`f8c3bfffb55b8c8f0f0f884645afc1f0e99ade25`](https://github.com/Ding-Ding-Projects/material-virtualbox/commit/f8c3bfffb55b8c8f0f0f884645afc1f0e99ade25)
of settings-shell source commit
[`152327ec9fe04455de55def8bd8e943138b75737`](https://github.com/Ding-Ding-Projects/material-virtualbox/commit/152327ec9fe04455de55def8bd8e943138b75737);
the published landing page and `SettingsShell.md` article both returned HTTP
200. These are source-contract and static-site results, not proof that the
native manager launched; the COM runtime boundary and real GUI capture remain
open.

When publication work is added, it must build from the intended commit, keep
artifact and test evidence separate, publish only verified outputs, and expose
the documentation site from the repository homepage. A green static check
must not be described as a successful GUI build or release.

</details>

<details>
<summary><strong>Contributing to the native frontend</strong></summary>

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

</details>

<details>
<summary><strong>Security reporting and license</strong></summary>

## Security and license

Report vulnerabilities using the process in [`SECURITY.md`](SECURITY.md), not
through public issue details. VirtualBox is distributed under GPLv3 as shown
in [`COPYING`](COPYING), with additional third-party license notices in
[`THIRD_PARTY_LICENSES.txt`](THIRD_PARTY_LICENSES.txt).

Copyright (C) 2025 Oracle and/or its affiliates.

</details>
