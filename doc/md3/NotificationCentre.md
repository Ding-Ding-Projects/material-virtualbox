# Notification center search

The native notification center remains the owner of the existing
`UINotificationModel` and `UINotificationObjectItem` lifecycle. The Material 3
lane adds a searchable extended view and a bounded persistent history model
without replacing the model, blocking question/progress paths, or changing the
notification ownership rules.

## Behavior

When the center is opened in extended mode, `UINotificationCenter` presents the
shared `UIMd3SearchField` with the placeholder **Search notifications**. Plain
text is the default and matching is case-insensitive. The adjacent regex
builder is the same bounded builder used by settings, tabs, and the command
palette; invalid patterns are rejected by the field and never reach the
notification model.

The predicate searches the real notification object fields:

- visible name;
- detail text;
- internal name; and
- help keyword.

Extended-mode critical-item filtering remains authoritative. A notification is
shown only when it is both allowed by the existing critical-item rule and
matches the current query. Clearing the query restores all permitted items.
Adding, removing, reordering, changing language, or entering/leaving extended
mode reapplies the same predicate, so the list cannot become stale after a
model mutation.

The search field is created only after the process-wide Material 3 theme is
available. The temporary startup center therefore keeps its original safe
construction path, while the manager/runtime center receives the themed search
surface. The field is hidden with the compact notification button and shown
with the extended list.

## Persistent history model

Non-blocking legacy notifications are snapshotted before the native model is
allowed to handle them. The snapshot stores the real title and detail, a short
semantic category, a UTC timestamp, an error flag, and an unread flag in
`UIMd3NotificationCentre`. Progress objects and messages marked critical for a
blocking question or decision are deliberately excluded, so a reviewable
history record cannot turn a modal operation into a toast or change its event
loop semantics.

The model is created after `UICommon`, the Material theme, and the language
service. It writes schema version `1` to the application-data file
`md3-notifications.json` through `QSaveFile`, bounds the file to 512 KiB and
256 records, trims each field, rejects malformed or duplicate records, and
keeps the newest records first. A failed read or atomic write leaves the
existing in-memory state usable and never blocks the operation that produced
the notification.

The manager header's **Notifications** button opens `showCentre()`, a modeless,
bounded review surface with its own `UIMd3SearchField`; plain text remains the
default and the adjacent regex builder searches title, detail, and category
locally. Rows use plain-text labels, preserve unread/error state, and expose an
explicit **Mark all as read** action. Each row also has a keyboard-reachable
selection checkbox. **Select visible** and **Invert selection** are scoped to
the active query; **Mark selected as read** persists the selected state. **Export
view** writes the selected rows when a selection exists, otherwise the visible
filtered rows, as bounded versioned JSON through an atomic `QSaveFile`.

The destructive **Clear history** action is intentionally not exposed yet: it
needs the app-wide super-confirmation, local history record, and undo path
before it can be a safe control. Transient toast presentation, bulk dismiss or
delete, provider-authored markdown rendering, and full per-row accessibility
roles remain later lanes.

## Configuration and localization

The language service key is `md3.notifications.search`, with English
`Search notifications` and Traditional Chinese/Cantonese `搜尋通知`. The
placeholder and accessible name update when the language service changes mode.
The query is local widget state; it is not written into VirtualBox extra data
and is cleared when the center is recreated.

## Failure modes and security

The search is a view filter only. It does not revoke, dismiss, delete, or alter
legacy notifications, and it does not change the blocking semantics of
questions or progress operations. A missing theme or language singleton leaves
the legacy center usable and simply omits the optional themed field during
early startup. History records are bounded local plain text; rich provider
content is not interpreted as markup, and malformed or oversized JSON is
discarded rather than executed or displayed.

Patterns are evaluated by `UIMd3SearchField`, which bounds pattern length and
uses Qt's regular-expression engine. The query stays in-process and is not
persisted. The history file is written atomically beneath the app-data
directory; persistence failure is non-blocking and is not reported as a false
success. Export opens the native save picker, bounds the JSON payload, and
fails closed if the destination cannot be written. Bulk dismiss/delete and
destructive clear-history confirmation remain separate open lanes.

## Verification

The implementation is in
`src/VBox/Frontends/VirtualBox/src/notificationcenter/UINotificationCenter.{h,cpp}`
and
`src/VBox/Frontends/VirtualBox/src/md3/UIMd3NotificationCentre.{h,cpp}`. The
focused source contract checks the field wiring, the critical-item predicate,
the bounded JSON model, selectable-row and export actions, lifecycle
creation/destruction, and UICommon target ownership. UICommon and the root
`VirtualBox` target are built through kBuild
when the Deen No toolchain is available; native captures are intentionally
deferred for this lane.

## Suggested articles

- [Settings search](SettingsSearch.md) — the shared plain/regex field contract.
- [Command palette](CommandPalette.md) — searchable actions and focus return.
- [Tab navigation](TabNavigation.md) — local menu and overflow filtering.
- [Design coverage ledger](DesignCoverage.md) — archive rows and remaining lanes.
