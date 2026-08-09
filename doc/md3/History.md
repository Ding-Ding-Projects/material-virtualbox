# Local history journal

The Material 3 GUI now has a shared, bounded local history journal for
user-managed GUI changes. It is deliberately separate from a user's virtual
machine or project folders: the journal lives beneath the application's
standard application-data directory in a `history` folder and never creates a
repository in user content.

## Behavior

`UIMd3History` stores up to 1,024 newest-first revisions in an atomic
`revisions.jsonl` file. Each record has a UUID, UTC timestamp, bounded action,
bounded detail, optional bounded state bytes, and a SHA-256 digest of those
state bytes. A malformed line, duplicate identifier, invalid timestamp,
oversized field, or digest mismatch is ignored without preventing the rest of
the journal from loading.

When the local `git` executable is available, the service initializes the
isolated folder as a repository and records each journal update with a local
identity (`VirtualBox` / `virtualbox@localhost`). Git failures never fail the
user operation: the atomic journal remains the source of truth and the
service marks Git backing unavailable for later writes. The journal is not a
transport and never touches the user's own repositories, credentials, remotes,
or network configuration.

The notification centre uses the journal for the destructive-history path:

1. Before an authorized **Clear history**, the bounded notification JSON is
   stored as the state of a `notification history cleared` revision.
2. **Undo last clear** restores that validated state and appends a
   `notification history restored` revision, so the restore is itself
   reviewable rather than rewriting history.
3. A newly posted notification appends a `notification history changed`
   revision and invalidates the one-step Undo action. A fallback
   `md3-notifications-undo.json` remains for installations where Git is not
   available.

The journal browser is intentionally bounded. Diff/restore controls for every
settings surface, retention management, and additional export formats remain
later lanes because only the owning surface can safely apply an opaque state.

The browser is now available from the manager with <kbd>Ctrl+H</kbd> and from
the command palette. It keeps plain-text search as the default, lets the same
field opt into its anchored regular-expression builder, adds an action filter,
native calendar-popup From/To fields (with an explicit **Any date** state), a
scrollable newest-first revision list, bounded JSONL export, and an inline
**Verify integrity** result. Selecting a supported notification or
appearance/theme revision enables a validated **Restore selected state** action;
the owning notification centre or theme service applies its bounded payload and
appends the restore. The existing
Network Manager shortcut is preserved as <kbd>Ctrl+Shift+H</kbd> so the history
shortcut has one owner.
Generic state restore remains surface-specific: notification clear/restore is
implemented both from their owning surfaces and from this browser, while
settings/runtime revisions are displayed and exported until their owning
surfaces supply safe restore adapters.

## Configuration and limits

The journal has no user-facing network or account configuration. The storage
location follows `QStandardPaths::AppDataLocation`; the schema is private to
the application and is versioned by its record shape. Payloads are capped at
1 MiB, state at 512 KiB, actions at 128 characters, details at 512 characters,
and the retained revision count at 1,024. Writes use `QSaveFile` and are
replaced atomically.

## Failure modes and security

An unwritable application-data directory, unavailable Git executable, failed
Git command, malformed JSON line, or invalid digest degrades to the last valid
atomic journal state. Notification clearing and restoring continue according
to their own visible confirmation and validation results; persistence failure
is never presented as a successful history write. State bytes are treated as
opaque bounded data and are never rendered as provider-authored markup.

The journal's Git commands use a fixed local identity and a bounded commit
message. No remote is configured, no data is pushed, and no credentials are
read. The isolated directory is still local user data, so operating-system
file permissions and the user's normal application-data protections remain the
security boundary.

## Verification

Production sources are
`src/VBox/Frontends/VirtualBox/src/md3/UIMd3History.{h,cpp}` and the manager
shortcut/command wiring is in
`src/VBox/Frontends/VirtualBox/src/manager/UIVirtualBoxManager.cpp`. The
UICommon target compiles the service and its MOC output; `main.cpp` creates it
after UICommon/theme/language initialization and destroys it before UICommon.
The source contract covers lifecycle, bounded JSON, SHA-256 validation, atomic
writes, clear/restore revision names, the Git fallback, browser plain-text and
regular-expression filters, action/date filtering, export, integrity
verification, and the <kbd>Ctrl+H</kbd> route. Focused
Windows builds of `UICommon`, `VirtualBox`, and `VirtualBoxVM` are the build
gate for this lane. Native capture is intentionally deferred in the current
task.

## Suggested articles

- [Notification centre](NotificationCentre.md) — the clear gate and one-step restore consumer.
- [Settings search](SettingsSearch.md) — the shared field and regex-builder contract.
- [Wizard shell](WizardShell.md) — the next surface that will record page actions.
- [Design coverage ledger](DesignCoverage.md) — archive rows and remaining implementation lanes.
