# Material 3 tab navigation

The manager now has a native Qt browser-style tab strip above the existing
global-tools surface. It is implemented by
`src/VBox/Frontends/VirtualBox/src/md3/UIMd3TabStrip.{h,cpp}` and is owned by
`UICommon`, so the same model can be reused by the manager and runtime targets.

## Behaviour

- The manager opens Home, Machines, Extensions, Media, Network, Cloud, and
  Resources as real selectable workspace tabs when their rail destinations are
  visited. Selecting a tab delegates to the existing
  `UIGlobalToolsWidget::setMenuToolType` path; the existing `UIToolType` model
  remains authoritative.
- A new or legacy generated layout begins with one pinned available destination
  instead of seven duplicate tabs. Migration applies only to the exact previous
  seven-tab, unpinned, ungrouped shape; customized tab state is preserved.
- Keyboard Left and Right move the active tab, while a pointer click selects a
  tab. The strip has a named `tab-strip` object and a visible active-state
  treatment from the shared Material 3 theme. A dedicated transparent child
  exposes a `PageTabList` containing only real `PageTab` controls with
  selected/disabled state, focus, and press actions. New tab, Tab manager,
  overflow, and independent 48-pixel close buttons remain accessible siblings
  while the parent keeps the Material painting.
- A keyboard-originated context-menu event (including <kbd>Shift+F10</kbd>)
  reuses the stable current tab when the event has no pointer hit, so the
  resulting menu exposes that tab's pin, move, close, and `Edit tab
  appearance…` actions. A pointer context menu on strip chrome still opens
  the strip-level group menu, and the tab action menu keeps its local search.
- Labels are registered with the shared language service and update live for
  English, playful Cantonese, and bilingual modes. The manager mirrors the
  existing chooser and expert-mode restrictions into disabled tab states, so a
  tab cannot bypass the underlying `UITools` policy.
- Tab actions include a local search field, pin or unpin, close, and
  `Edit tab appearance…`. `Move… into group…` opens a real keyboard-operable
  picker capped to the current screen, with scrolling, its own bounded
  regex-capable search field, member counts, color labels, a no-group target,
  and an inline create-group path. A hidden filtered result cannot be accepted.
  Shift+right-click opens the appearance editor for the selected tab.
- Right-clicking strip chrome opens the strip appearance editor and a searchable
  group-management menu. The menu keeps its local search field while exposing
  `Create group…`, bounded `Rename group…` editors, and
  `Edit group appearance…` actions keyed to each group's stable identifier;
  every one of those local menus uses the shared anchored regex builder.
- The strip is 48 px high with tab bodies bounded to 120--210 px and a 48 px
  inline close target. New tab, Tab manager, and overflow are real 48 px
  `QToolButton` controls with accessible names and keyboard focus. The New tab
  and Tab manager menus have independent regex-capable searches, and overflow
  opens the same searchable list as the Down key path. Activating a hidden
  overflow result advances a transient, non-persisted viewport until that tab
  is fully visible. If a long pinned region would otherwise consume the strip,
  the transient pinned and ordinary viewports retain at least one pinned tab
  while revealing the selected ordinary tab. Only fully visible tabs
  participate in pointer hit testing; the reserved trailing boundary prevents
  a clipped sliver from disappearing from both the strip and overflow list.
- Tab groups have stable identifiers, names, colors, collapsed state, and
  membership. Pinning, grouping, and the active tab persist through the
  existing VirtualBox extra-data store under `GUI/Md3/Tabs` for the current
  manager surface. Activating a member of a collapsed group reveals only that
  current tab without changing the group's persisted collapsed preference;
  moving a tab into a collapsed group likewise leaves the group collapsed.
- Close matching supports bounded plain-text and regular-expression predicates
  and protects pinned tabs unless the caller explicitly includes them. Empty
  queries produce no close set, preventing an accidental close-all operation.
- Closing the active tab resolves the nearest enabled visible fallback before
  saving, then emits one model update and one final current-tab notification;
  no intermediate empty selection is persisted or announced.

## Failure modes and security

The strip never owns machine, storage, network, cloud, or COM operations. It
only emits a destination selection and delegates the action to existing
VirtualBox models. Malformed persisted JSON is ignored; invalid group colors
fall back to the shared theme role; the persisted document is capped at 256 KiB,
groups and tabs are capped at 128 and 256 entries, and labels, identifiers, and
query patterns are bounded before they reach the model or regex engine.
The current persistence key is global to the GUI profile rather than scoped per
window or surface; runtime adoption must add a surface-scoped key and reconcile
destinations before this model is reused there.

## Verification

The MD3 validation workflow checks the tab-strip source and MOC header are
wired into `UICommon`. The commit-attributed local `VirtualBox` result in the
[manager-shell evidence table](ManagerShell.md#verification-and-remaining-work)
compiled the translation unit and its Qt MOC output. Full tab management remains in
progress: the four independent cross-window tab-discovery searches, drag
  reordering, group delete/reorder surfaces, bulk-close preview/confirmation,
  the full guided regex builder with capture output and copy/export, vertical
  docking, surface-scoped persistence, and runtime-window adoption
still need their own production lanes. The current
strip does provide on-demand manager workspaces, an interactive overflow menu,
group expand/collapse menu, visible focus ring, and restriction-aware enabled
states, PageTab accessibility children, independent close buttons, and
dedicated New tab and Tab manager actions. The compact navigation action exists
below 1000 logical pixels; a fully dockable left-edge tab strip remains open.
Native GUI capture is intentionally deferred in the current task; no design
thumbnail or static HTML preview is treated as runtime evidence.

Suggested articles: [Manager shell](ManagerShell.md),
[Navigation rail](NavigationRail.md),
[Settings search](SettingsSearch.md), [Appearance settings](AppearanceSettings.md),
and the [design coverage ledger](DesignCoverage.md).
