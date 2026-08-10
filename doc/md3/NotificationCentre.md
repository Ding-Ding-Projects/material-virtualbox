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
explicit **Mark all as read** action. Each row now has a visible focus ring, an
accessible name and description, and a keyboard disclosure path: <kbd>Enter</kbd>
or <kbd>Space</kbd> expands or collapses its details without requiring a pointer.
Each row also has a keyboard-reachable selection checkbox. **Select visible** and
**Invert selection** are scoped to the active query; **Mark selected as read**
persists the selected state. **Export view** writes the selected rows when a
selection exists, otherwise the visible filtered rows, as bounded versioned JSON
through an atomic `QSaveFile`.

The destructive **Clear history** action is exposed through an app-owned
super-confirmation. The dialog states the exact retained-record count and the
`md3-notifications.json` target, requires two independently operated
acknowledgements, keeps a full-range authorization slider disabled until both
are checked, animates a bounded progress bar while the slider moves, and shows
a distinct ready state at 100 percent. **Emergency exit** and Escape cancel
the operation, and focus returns to **Clear history** after either cancellation
or completion. Only the fully authorized path calls `clear()`; the ordinary
review surface never clears records implicitly.

After an authorized clear, the model keeps one bounded recovery snapshot in
`md3-notifications-undo.json` and exposes **Undo last clear** in the review
surface. The snapshot is validated with the same schema, field, count, and
payload limits as live history; a failed snapshot never blocks the clear, and a
successful restore consumes the snapshot and rewrites the live file atomically.
The shared `UIMd3History` journal also records the pre-clear state as an
append-only `notification history cleared` revision and records a successful
restore as `notification history restored`. A later non-blocking notification
records `notification history changed`, which invalidates the one-step Undo
action; the bounded JSON snapshot remains as a fallback when Git is unavailable.
The local-history browser exposes a **Restore selected state** action when the
selected revision carries a validated notification-state payload. The request
is routed back to this owner, parsed through the same bounded schema, written
atomically, and recorded as a new `notification history restored` revision.
Appearance/theme revisions are routed to `UIMd3Theme`, which validates the
versioned palette, typography, density, named-theme, and per-element payload
before applying it and recording the restore. Settings/runtime revisions remain
export-only until their owning surfaces supply adapters; the browser never
guesses how to apply unknown bytes. Transient toast presentation, bulk dismiss or delete,
provider-authored markdown rendering, selection semantics for every row control,
and full per-row restore accessibility remain later lanes.

## Configuration and localization

The language service keys include `md3.notifications.search`,
`md3.notifications.clear`, and `md3.notifications.undoClear`, with English
and Traditional Chinese/Cantonese values registered beside the implementation.
The placeholder, action labels, and accessible names update when the language
service changes mode. The query is local widget state; it is not written into
VirtualBox extra data and is cleared when the center is recreated.

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
success. The recovery snapshot uses the same bounded atomic format beneath the
app-data directory and is consumed after a successful restore. Export opens
the native save picker, bounds the JSON payload, and fails closed if the
destination cannot be written. Clear authorization is modal only because it is
a destructive decision; the history model itself remains non-blocking. The
shared local journal uses a fixed local Git identity, never configures a
remote, and falls back to the atomic files when Git cannot initialize or
commit. Surface-specific adapters for settings/runtime state, all-record diff
views, retention controls, selection semantics for every row control, and full
per-row restore accessibility remain explicit verification gaps.

## Verification

The implementation is in
`src/VBox/Frontends/VirtualBox/src/notificationcenter/UINotificationCenter.{h,cpp}`
and
`src/VBox/Frontends/VirtualBox/src/md3/UIMd3NotificationCentre.{h,cpp}`, and
`src/VBox/Frontends/VirtualBox/src/notificationcenter/UINotificationObjectItem.{h,cpp}`.
The focused source contract checks the field wiring, the critical-item predicate,
focusable row disclosure, keyboard Enter/Return/Space handling, visible focus,
accessible names/descriptions, the bounded JSON model, selectable-row/export/restore actions, the notification
history restore adapter, the shared
`UIMd3History` lifecycle and SHA-256 journal, the two-acknowledgement/full-
slider clear gate, focus return, and UICommon target ownership. The modeless
review surface connects its persistent model, language, and theme signals only
once; mutations emit `sigChanged()` and no longer perform a second direct
refresh. This prevents repeated callbacks after reopen and redundant list
rebuilds after each mutation. Local Windows x64 kBuild runs for `UICommon`,
`VirtualBox`, and `VirtualBoxVM` all exited 0 on the exact source tree for this
lane. Native captures remain blocked by the unregistered COM/SDS development
runtime and are not replaced by design previews.

## Suggested articles

- [Settings search](SettingsSearch.md) — the shared plain/regex field contract.
- [Command palette](CommandPalette.md) — searchable actions and focus return.
- [Tab navigation](TabNavigation.md) — local menu and overflow filtering.
- [Local history journal](History.md) — append-only clear/restore records and bounded state validation.
- [Design coverage ledger](DesignCoverage.md) — archive rows and remaining lanes.
