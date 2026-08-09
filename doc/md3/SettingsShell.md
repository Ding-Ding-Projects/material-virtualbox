# Material settings shell

Global Preferences and per-machine Settings now compose the existing
`UISettingsSelector`, `UISettingsPageFrame`, serializers, validators, and
button box inside one Material 3 shell. The adapter changes presentation and
navigation only; it does not copy or replace any settings model.

## Layout and behavior

- A compact header contains the plain-text-first settings search, the existing
  Basic/Expert choice, and—only in Global Preferences—a persisted disclosure
  for language and appearance customization.
- Ordinary navigation shows exactly the selected settings page. This removes
  the former duplicate experience where the selector acted like a bookmark
  into one long scrolling document.
- An active plain-text or regular-expression search deliberately switches to a
  cross-page result view. Existing `UISettingsPageFrame::filterOut` behavior
  remains authoritative, and choosing a result still uses the real selector
  identity.
- The selector is a bounded 180–240-pixel rail with 48-pixel rows, rounded
  selected and hover states, theme-role colors, a visible focus ring, icon
  tinting, and elided visual labels whose complete text remains available to
  assistive technology.
- Each real settings page is presented in a rounded Material surface card. Page
  titles use the shared type scale and update when the live theme changes.
- Global language and appearance controls are collapsed by default, restore
  their disclosure state through VirtualBox extra data, and use a single
  label/control column so long localized values do not collide. Machine
  Settings never receives this global-only panel.

The bottom validation summary and native Apply/OK/Cancel behavior are
unchanged. Search and navigation never commit, discard, or reorder pending
page data.

## Persistence and failure modes

Only the customization panel's expanded/collapsed state is new shell state. It
uses `GUI/Md3/SettingsCustomizationExpanded`; an absent value means collapsed
without writing a redundant default. All actual settings values continue to
use their existing page serializers, while language and appearance values use
the existing `UIMd3Language` and `UIMd3Theme` services.

An invalid search pattern remains inside the anchored builder and does not
replace the active filter. If no page matches, the selector and page frames use
their existing filtering state rather than manufacturing a placeholder page.
The shell performs no guest, network, COM, credential, or machine-wide service
operation.

## Accessibility

The selector remains a native item view with keyboard navigation and complete
accessible labels. The search field and its full builder preserve their focus
return path. Labels are associated with their language, tone, palette,
typography, density, and display-brand controls, and all controls retain the
shared Material style's focus and disabled-state treatment.

Complete browser-style settings-tab semantics, every-edge docking, and genuine
screen-reader tree verification remain open. They are not inferred from the
source build.

## Verification

Exact source commit
[`152327ec9fe04455de55def8bd8e943138b75737`](https://github.com/Ding-Ding-Projects/material-virtualbox/commit/152327ec9fe04455de55def8bd8e943138b75737)
compiled the changed `UICommon` settings source and linked the shared DLL on a
Windows x64 development host. Serial `UICommon`, `VirtualBox`, and
`VirtualBoxVM` release targets each exited 0 with Qt 6.8.3, MSVC 14.44, and
Windows SDK 10.0.26100.0; only the known Qt/SDK C4668 and kBuild notices
remained. The installed artifact hashes were:

| Target | Result | SHA-256 |
| --- | --- | --- |
| `UICommon` | changed source compiled and linked | `BC308CC34B9C8D749E67C0C625159E23C5EB9067EBF1B3E472DAB26E6D342893` |
| `VirtualBox` | dependency-current, exit 0 | `127314B61C157DB98DF75990A10AE76E6216B135F614193BB1876E81CBC1FC0B` |
| `VirtualBoxVM` | dependency-current, exit 0 | `2B6075A37083CCA25AE2DAB113C3B20A1136F76E35F8F864A0F93ACD4AA13F22` |

The first validation run exposed a PowerShell continuation error in the new
workflow assertion before source evaluation. CI-only correction
[`f8c3bfffb55b8c8f0f0f884645afc1f0e99ade25`](https://github.com/Ding-Ding-Projects/material-virtualbox/commit/f8c3bfffb55b8c8f0f0f884645afc1f0e99ade25)
then completed [validation run
31330546183](https://github.com/Ding-Ding-Projects/material-virtualbox/actions/runs/31330546183)
and [Pages run
31330546180](https://github.com/Ding-Ding-Projects/material-virtualbox/actions/runs/31330546180)
successfully. The deployed Home page and this article both returned HTTP 200
with the new settings-shell content.

Native interaction and visual capture remain deferred. A later genuine build
capture must show Global Preferences and Machine Settings separately, ordinary
single-page navigation, cross-page search, collapsed and expanded global
customization, keyboard focus, bilingual copy, and 100/125/150/200% display
scale before this archive row can be called complete.

Suggested articles: [`SettingsSearch.md`](SettingsSearch.md),
[`AppearanceSettings.md`](AppearanceSettings.md),
[`StockControlStyle.md`](StockControlStyle.md), and
[`DesignCoverage.md`](DesignCoverage.md).
