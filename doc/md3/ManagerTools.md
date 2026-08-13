# Manager tools

The first native manager-tools slice places one Material search and appearance
card above the existing Extensions, Media, Network, Cloud, and VM Activity
Overview panes. It does not create replacement records, duplicate a manager
model, or route an action around `UIActionPoolManager`.

## Behavior

`UIGlobalToolsWidget` binds `UIMd3ManagerToolSearch` to the widget currently
owned by `UIToolPane`. The card discovers the pane's existing tree and table
views and filters their display and tooltip roles. Parent tree rows remain
visible when a descendant matches. The result count covers the same bounded
set inspected by the filter.

Plain text is the default. The adjacent shared builder provides guided and raw
regular-expression construction, supported flags, bounded preview, copy and
JSON export, and focus return. The 48-by-48 appearance action opens the existing
non-modal editor on the exact active tool widget. Its icon is recolored from the
active Material surface role instead of presenting the source raster colors.

The card appears only for manager destinations that expose record views. Home
and Machines keep their purpose-built surfaces. The embedded Media contextual
toolbar no longer repeats its Search action beside the persistent card; the
standalone Virtual Media Manager retains its existing search behavior.

## Model and state ownership

Filtering changes only view row visibility. Before the first filter pass, the
card stores each row's existing hidden state with a persistent model index.
Clearing the query, switching tools, or destroying the card restores that
state. Model reset, insert, remove, data, and layout changes schedule one
debounced refresh so a busy activity model does not synchronously refilter on
every cell update.

No filter text or result is written to VirtualBox extra data. Selection,
sorting, editing, destructive actions, COM/XPCOM objects, and refresh ownership
remain with the original manager widgets.

## Bounds and failure behavior

- One pass visits at most 4,096 model rows.
- At most 64 columns are inspected for a row.
- Display and tooltip text are capped at 4,096 characters per field.
- Live model changes are coalesced for 120 milliseconds.
- When a model exceeds the row bound, additional records remain unchanged and
  visible; the status identifies that it covers only the first 4,096 records.
- A pane without a supported record view reports an explicit empty state.
- A stale or removed persistent index is ignored during restoration.

Search is entirely local. It makes no network request, does not log provider or
machine text, and never treats matched display data as markup or a command.

## Verification

The current working-tree implementation compiled and linked the Windows x64
`UICommon`, `VirtualBox`, and `VirtualBoxVM` targets with MSVC 14.44, Qt 6.8.3,
and Windows SDK 10.0.26100.0. This is local pre-commit evidence; the exact
published commit and CI/Pages runs are recorded after the lane is pushed.

The source contract checks shared-library ownership, all five real manager
destinations, row and field bounds, original-hidden-state restoration,
debounced live-model refresh, the exact appearance target, the 48-pixel action,
and removal of the duplicate embedded Media toolbar action.

Native interaction and capture remain blocked by the development checkout's
unregistered COM/SDS classes. Bulk actions, export, Logs integration, detached
manager-window composition, and the remaining prototype controls stay open in
the design ledger rather than being inferred from this slice.

## Suggested articles

- [Manager shell](ManagerShell.md)
- [Navigation rail](NavigationRail.md)
- [Full regex builder](RegexBuilder.md)
- [Appearance settings and per-element editing](AppearanceSettings.md)
- [Runtime capture contract](RuntimeCapture.md)
