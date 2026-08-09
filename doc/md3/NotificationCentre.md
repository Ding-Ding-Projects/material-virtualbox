# Notification center search

The native notification center remains the owner of the existing
`UINotificationModel` and `UINotificationObjectItem` lifecycle. The Material 3
lane adds a searchable extended view without replacing the model, blocking
question/progress paths, or changing the notification ownership rules.

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

## Configuration and localization

The language service key is `md3.notifications.search`, with English
`Search notifications` and Traditional Chinese/Cantonese `搜尋通知`. The
placeholder and accessible name update when the language service changes mode.
The query is local widget state; it is not written into VirtualBox extra data
and is cleared when the center is recreated.

## Failure modes and security

The search is a view filter only. It does not revoke, dismiss, delete, or alter
notifications, and it does not change the blocking semantics of questions or
progress operations. A missing theme or language singleton leaves the legacy
center usable and simply omits the optional themed field during early startup.

Patterns are evaluated by `UIMd3SearchField`, which bounds pattern length and
uses Qt's regular-expression engine. Notification text stays in-process; it is
not sent to a service or persisted. Provider-authored detail rendering,
reviewable notification history, bulk selection/export/dismiss, and destructive
clear-history confirmation remain separate open lanes.

## Verification

The implementation is in
`src/VBox/Frontends/VirtualBox/src/notificationcenter/UINotificationCenter.{h,cpp}`
and reuses the existing UICommon-owned `UIMd3SearchField` and
`UIMd3Language`. The focused source contract checks the field wiring, the
critical-item predicate, and the UICommon target ownership. The native
`VirtualBox` target is built through kBuild when the Deen No toolchain is
available; native captures are intentionally deferred for this lane.

## Suggested articles

- [Settings search](SettingsSearch.md) — the shared plain/regex field contract.
- [Command palette](CommandPalette.md) — searchable actions and focus return.
- [Tab navigation](TabNavigation.md) — local menu and overflow filtering.
- [Design coverage ledger](DesignCoverage.md) — archive rows and remaining lanes.
