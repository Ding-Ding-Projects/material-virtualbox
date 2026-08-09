# Material 3 tab navigation

The manager now has a native Qt browser-style tab strip above the existing
global-tools surface. It is implemented by
`src/VBox/Frontends/VirtualBox/src/md3/UIMd3TabStrip.{h,cpp}` and is owned by
`UICommon`, so the same model can be reused by the manager and runtime targets.

## Behaviour

- The manager exposes Home, Machines, Media, Network, Cloud, Resources, and
  Extensions as real selectable tabs. Selecting a tab delegates to the existing
  `UIGlobalToolsWidget::setMenuToolType` path; the existing `UIToolType` model
  remains authoritative.
- Keyboard Left and Right move the active tab, while a pointer click selects a
  tab. The strip has a named `tab-strip` object and a visible active-state
  treatment from the shared Material 3 theme.
- Labels are registered with the shared language service and update live for
  English, playful Cantonese, and bilingual modes. The manager mirrors the
  existing chooser and expert-mode restrictions into disabled tab states, so a
  tab cannot bypass the underlying `UITools` policy.
- Tab actions include a local search field, pin or unpin, close, and
  `Edit tab appearance…`. `Move… into group…` opens a real keyboard-operable
  picker with its own bounded regex-capable search field, member counts, color
  labels, a no-group target, and an inline create-group path. Shift+right-click
  opens the appearance editor for the selected tab.
- Right-clicking strip chrome opens the strip appearance editor and a searchable
  group expand/collapse menu; every one of those local menus uses the shared
  anchored regex builder.
- When tabs overflow, the ellipsis is a real 48 px `QToolButton` with an
  accessible name and keyboard focus; it opens the same searchable overflow
  menu as the Down key path instead of silently clipping the remaining tabs.
- Tab groups have stable identifiers, names, colors, collapsed state, and
  membership. Pinning, grouping, and the active tab persist through the
  existing VirtualBox extra-data store under `GUI/Md3/Tabs` for the current
  manager surface.
- Close matching supports bounded plain-text and regular-expression predicates
  and protects pinned tabs unless the caller explicitly includes them. Empty
  queries produce no close set, preventing an accidental close-all operation.

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
wired into `UICommon`, and the native `VirtualBox` target has compiled the
new translation unit and its Qt MOC output. Full tab management remains in
progress: the four independent tab-discovery searches, drag reordering, group
rename/delete/reorder surfaces, bulk-close preview/confirmation, vertical
docking, per-tab `QAccessible::PageTab` children, surface-scoped persistence,
and runtime-window adoption still need
their own production lanes. The current strip does provide an interactive
overflow menu, group expand/collapse menu, visible focus ring, and
restriction-aware enabled states.
Native GUI capture is intentionally deferred in the current task; no design
thumbnail or static HTML preview is treated as runtime evidence.

Suggested articles: [Navigation rail](NavigationRail.md),
[Settings search](SettingsSearch.md), [Appearance settings](AppearanceSettings.md),
and the [design coverage ledger](DesignCoverage.md).
