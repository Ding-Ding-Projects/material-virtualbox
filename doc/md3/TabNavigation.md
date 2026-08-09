# Material 3 tab navigation

The manager has a native Qt browser-style workspace strip above the existing
global-tools surface. `UIMd3TabStrip` and the four-scope `UIMd3TabManager` are
owned by `UICommon`; selection still delegates to the existing
`UIGlobalToolsWidget` and `UIToolType` authority.

## Behavior

- Home, Machines, Extensions, Media, Network, Cloud, and Resources open as
  real workspace tabs when visited. A new or precisely recognized legacy
  generated layout begins with one pinned available destination instead of
  seven duplicate tabs. Customized order, pins, groups, and selection are not
  mistaken for that migration shape.
- Pointer activation and Left/Right navigation select only available tabs.
  The 48-pixel strip paints 120--210-pixel tab bodies while transparent native
  controls expose one `PageTabList`, real `PageTab` children with press actions
  and selected/disabled state, visible focus, and independent 48-pixel close,
  New tab, overflow, and Tab manager buttons.
- Overflow never silently clips a tab. It has a focusable searchable action;
  activating a result advances transient pinned/unpinned viewports until the
  selected tab is fully visible. Only fully visible tab geometry accepts a
  pointer hit. Those viewports are intentionally not persisted.
- Normal tab context menus retain searchable pin/unpin, move, close, and
  **Edit tab appearance…** actions. <kbd>Shift+F10</kbd> targets the stable
  current tab when no pointer position exists, and Shift+right-click opens the
  anchored appearance editor directly. Context-menu shortcut labels are taken
  from the actual action bindings.
- Groups have bounded stable identifiers, names, colors, collapsed state, and
  membership. The screen-bounded **Move… into group…** picker lists real member
  counts, has its own search and full regex builder, rejects hidden results,
  supports creating a named group, and returns focus. Strip chrome exposes
  searchable create, rename, collapse/expand, and group-appearance actions.
  Activating or moving the current member of a collapsed group reveals only
  that member without rewriting the saved collapsed preference.
- Labels and accessible copy use stable Material language keys and update for
  English, Cantonese, bilingual, and independent funny-level changes. Live
  chooser and expert-mode restrictions remain authoritative; unavailable tabs
  cannot be activated through the strip, overflow, or manager.

## Four discovery searches

<kbd>Ctrl+Shift+T</kbd> opens a screen-bounded modeless tab manager with four
independent plain-text-first search scopes:

1. the current tab strip;
2. tab groups by visible name;
3. a separate search field owned by every individual group, including the
   ungrouped top level; and
4. a master list of all registered application windows and strips.

Every field owns its own adjacent full regular-expression builder and state.
Results identify the visible label, owning window, group, pin state, current
state, and availability. Activation raises the exact owning window, selects and
reveals the exact destination, and keeps a collapsed group's stored preference.
The result menu reuses the real tab actions without discarding the active query.

## Bulk close

The manager provides separate **Close tabs containing text** and **Close tabs
not containing text** fields. Both default to case-insensitive plain text, own
their own full regex builder, reject an empty or invalid query, and use the same
bounded predicate and flags. Pinned tabs remain excluded unless the user
explicitly includes them.

Before closing, the surface lists every affected tab and the exact count. Any
query, regex mode, flag, include-pinned choice, or tab-model change invalidates
the review. The identifiers are resolved again immediately before **Close
reviewed tabs** runs, and skipped state changes are reported honestly. Manager
workspace tabs are reopenable views and do not own machine or document data;
closing one performs no COM operation and does not delete backend state.

## Persistence, failure modes, and security

Order, pins, groups, collapsed state, membership, and current selection use a
versioned document below `GUI/Md3/Tabs/<surface>`. The manager owns the
`manager` scope and performs a compatible fallback read of the former global
key. Version 2 data is copied forward after validation. The owning bounded
destination catalog removes unknown restored tabs so a
stale identifier cannot remain as a dead destination.

Malformed or unsupported JSON is ignored. Raw persisted input is capped at
256 KiB before parsing; groups and tabs are capped at 128 and 256 entries;
identifiers, labels, group names, query patterns, and flags are bounded;
duplicates and orphan group references are removed; invalid colors fall back
to the active theme. Availability is never persisted because it belongs to
current manager restrictions. Runtime adoption must provide a different stable
scope and destination catalog.

The strip never owns machine, storage, network, cloud, or COM operations. It
changes only navigation state and delegates destination activation to existing
VirtualBox models.

## Verification and remaining work

The Material 3 validation workflow checks source/MOC/UICommon ownership, the
four hand-written search-scope identifiers, independent per-group fields,
regex flags, review/re-resolution behavior, scoped persistence, accessibility
actions, width/target bounds, and overflow reveal. Local Windows builds compile
and link `UICommon`, `VirtualBox`, and `VirtualBoxVM` against the same tree.

Remaining work includes drag reordering; group delete, reorder, and direct color
surfaces; dockable left/right/top/bottom strip orientations; settings and
runtime adoption; tab-state history/undo; and focused native keyboard,
screen-reader, narrow-layout, and high-DPI capture. Native capture is explicitly
deferred in the current task, and no design thumbnail or HTML prototype is
treated as runtime evidence.

Suggested articles: [Manager shell](ManagerShell.md),
[Navigation rail](NavigationRail.md),
[Regular-expression builder](RegexBuilder.md),
[Settings search](SettingsSearch.md), [Appearance settings](AppearanceSettings.md),
and the [design coverage ledger](DesignCoverage.md).
