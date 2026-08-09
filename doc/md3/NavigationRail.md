# Manager navigation rail

The manager's global tool selection is presented through a Material Design 3
navigation rail in `UIGlobalToolsWidget`. The rail is a native Qt widget; it is
not a second tool model and it does not bypass `UITools`, `UIToolsModel`,
`UIToolPane`, or `UIActionPoolManager`.

## Behaviour

- Home, Machines, Extensions, Media, Network, Cloud, and Resources are exposed
  in the prototype order as keyboard-focusable, checkable rail buttons.
- Activating a button calls the existing `setMenuToolType` path, so selection,
  tool-pane creation, cloud refresh, and action-pool behavior stay unchanged.
- Activating a destination also opens or raises its workspace tab. The strip
  therefore records visited work instead of repeating every rail destination
  at startup.
- Existing expert-mode and empty-chooser restrictions are mirrored into the
  rail as disabled states. The visible label remains present, and its tooltip,
  status text, and accessible description identify the unmet condition.
- The active `UIToolType` is synchronized from the existing model selection,
  including changes caused by COM events or persisted startup state.
- Preferences remains a fixed bottom destination and delegates to the existing
  `UIActionIndex_M_Application_S_Preferences` action.
- Up and Down move focus through the actual visual order, including
  Preferences, while skipping hidden or disabled destinations. Moving focus
  does not activate or change the selected tool.
- Below 1000 logical pixels of content width, the 92 px rail yields to a 48 px
  navigation action. Its anchored menu has its own plain-text search and full
  regex builder and delegates to the same `UIToolType` model.

## Appearance and accessibility

The rail consumes the shared Material 3 surface, outline, primary, and state
layer roles from `UIMd3Theme`. It has the prototype's fixed 92 px desktop width,
transparent 80 by 54 px outer destinations, a centered 54 by 30 px selected or
hover pill, 22 px icons, compact label typography, visible focus borders, and a
reduced-opacity disabled state. Long visible labels are elided while their full
text remains in the tooltip and accessible name. Theme updates
refresh live colors and typography without replacing the underlying tool
widgets.

The seven destinations use the bundled `src/md3/UIMd3Icons.qrc` resource,
with a Qt standard-icon fallback when an asset cannot be loaded. Source alpha
masks are recolored with active Material roles for normal, selected, and
unavailable states at the current device-pixel ratio. Theme, screen, and
display-scale changes rebuild those icons. The fallback does not create a
second selection model or change the existing action path.

The existing `UIToolsItem` labels and accessibility descriptions now resolve
through stable `md3.tool.*` keys in `UIMd3Language`. English, playful Hong Kong
Cantonese, and bilingual modes (including their independent funny levels) are
applied without changing the `UIToolType` model or the existing
`UITranslationEventListener` path. If the Material language service is not
available, each item retains its normal Qt translation as a truthful fallback.

`UIMd3Language::sigLanguageChanged` is connected with `Qt::UniqueConnection`,
so repeated tool-item preparation cannot accumulate refresh callbacks. The
same update refreshes the item name used by the tooltip and accessibility
interface, plus its description.

## Failure modes and security

The rail never creates or destroys virtual machines and never performs COM
operations itself. If a tool is restricted, its button is disabled and the
existing tool pane remains the authority. A failure to construct the global
tool pane is handled by the existing VirtualBox error path; the rail is only
an alternate selector surface.

## Verification

The MD3 validation workflow checks that the rail source and MOC header are
wired into the `VirtualBox` target, that its 92 px geometry, centered pill,
ordered focus traversal, display-aware icon tinting, localized disabled
reasons, and stacked labels remain present, and that all fourteen user-facing tool strings
(thirteen `UIToolType` labels plus the description) use stable `md3.tool.*` keys,
and that both the Qt translation
listener and unique Material-language connection remain present. The
uncommitted local UICommon, VirtualBox, and VirtualBoxVM results in the
[manager-shell evidence table](ManagerShell.md#verification-and-remaining-work)
are the compile evidence for the lane; runtime verification still requires a working `VirtualBoxClient` COM
registration and a real native capture of the rail in Home, Machines, and at
least one restricted expert-mode state. Design thumbnails and static previews
are not acceptable substitutes. Runtime evidence must additionally cover the
compact navigation action below 1000 logical pixels, Preferences activation,
long bilingual labels, and 100/125/150/200-percent scaling.

Suggested articles: [Material 3 GUI overview](README.md),
[Manager shell](ManagerShell.md), [Design coverage ledger](DesignCoverage.md), and the repository
[build instructions](../README.md#build-and-prerequisites).
