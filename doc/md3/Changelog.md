# In-app changelog viewer

The Material 3 GUI now has an in-app changelog viewer: every entry the
repository's own [`CHANGELOG.md`](../../CHANGELOG.md) records, not only the
newest, reachable from inside the running Manager instead of only from the
repository on disk.

## Where it lives

Production sources are
`src/VBox/Frontends/VirtualBox/src/md3/UIMd3Changelog.{h,cpp}`. The service is
created and destroyed alongside every other process-wide Material 3 singleton
in `src/VBox/Frontends/VirtualBox/src/main.cpp`, after `UIMd3Language` (the
service registers its own localized copy at creation) and independently of
`UIMd3History`/`UIMd3NotificationCentre` (no interdependency exists in either
direction). It is wired into the Manager window
(`src/VBox/Frontends/VirtualBox/src/manager/UIVirtualBoxManager.cpp`) exactly
the way `UIMd3History` is: a dedicated keyboard shortcut and a command palette
entry, both opening the same modeless dialog.

- **Keyboard shortcut:** <kbd>Ctrl+Shift+L</kbd> (application-wide, mirrors
  the existing <kbd>Ctrl+H</kbd> local-history and <kbd>Ctrl+Shift+F</kbd>
  command-palette shortcuts already registered on the same window).
- **Command palette:** "Open changelog" under the Manager category, alongside
  "Open local history" and "Open global preferences" -- reachable by opening
  the palette from the header's search affordance and typing to filter, which
  `doc/md3/UsabilityProbe.md` records as the reliable discoverable route in
  this build regardless of whether a given keyboard chord reaches the window.

This mirrors `doc/md3/History.md`'s own framing: the feature is Manager-only
for this pass, matching every other `UIMd3*` singleton's current scope (the
VM Runtime window has zero `UIMd3` integration of any kind, recorded once in
`doc/md3/CompletenessInventory.md` rather than repeated per feature).

## Why the entries are compiled in, not parsed from a file at runtime

The obvious design would parse `CHANGELOG.md` at runtime so the viewer and the
file can never drift. Two things rule that out for this pass:

1. **The installed application does not ship `CHANGELOG.md`.** It lives at
   the repository root, which is not part of any packaged Windows
   installer/build output. A runtime parser would have nothing to read once
   installed, and silently rendering an empty viewer is worse than not
   shipping the feature.
2. **`CHANGELOG.md`'s prose is not a fully regular grammar.** Numbered fix
   entries carry their own commit link; the "Documentation" bullet links a
   file path, not a commit; the "Known issue" and "Related work" sections are
   prose paragraphs, not list items. A hand-rolled Markdown parser strict
   enough to extract structured `{version, date, category, title, detail,
   commit}` tuples from that mixture reliably, without silently mis-parsing a
   paragraph into a garbled entry, is a larger and riskier undertaking than
   this pass's scope, and a *wrong* parse is strictly worse than an honest,
   verified transcription -- a viewer that shows a mangled entry is a viewer
   that lies about what shipped.

Instead, `UIMd3Changelog::prepareEntries()` compiles in a direct, verified
transcription of `CHANGELOG.md`'s real content. Every field was copied from
the file and every commit hash was independently checked against this exact
checkout before being written:

```
git cat-file -e <sha>              # confirms the object exists locally
git log -1 --format="%H|%ad" --date=short <sha>   # confirms the real date
```

All ten entries currently compiled in trace back to commits dated
`2026-08-13`, matching `CHANGELOG.md`'s own dated section headers; the
"Documentation" entry's commit
(`51ed3bc2ae78940357e56f5043a18ddbc9108123`) was located independently with
`git log --oneline --follow -- CHANGELOG.md`, since the changelog prose does
not link a commit for its own addition. **This is a known, explicitly
documented trade-off, not silently dropped in favor of a clean claim of
"live": the viewer is exactly as current as the last time an agent updated
`UIMd3Changelog::prepareEntries()` alongside `CHANGELOG.md` itself, and that
duty is recorded here so it is not forgotten.** Every project-changing task
that edits `CHANGELOG.md` must add the matching entries to
`prepareEntries()` in the same task -- the house contract's "keep docs
current in the same task" rule applies to this compiled-in mirror exactly as
it applies to any other documentation surface.

Nothing in this viewer invents a version, a date, or a change. Where the
source material offers no version bucket yet (`CHANGELOG.md` itself states
"No release has been published from this repository yet ... Everything below
is therefore recorded under [Unreleased]"), the viewer says exactly that, in
the same words, rather than inventing a version number.

## Behavior

- **Version, date, category, and commit per entry.** Every
  `UIMd3ChangelogEntry` carries a version bucket (currently always
  `"Unreleased"` -- see above), a real `QDate`, a short category
  (`Fixed`, `Known issue`, `Documentation`, `Related work`), an optional
  section/context line, a title, the full unabridged detail text transcribed
  from `CHANGELOG.md`, and the full 40-character commit hash plus its
  resolvable GitHub URL.
- **Commit references are real, clickable, and keyboard-operable.** Each row
  renders its short (7-character) commit hash on a dedicated flat
  `QPushButton`, not a `QLabel` link -- a `QLabel`'s hyperlink is
  mouse-only in Qt, and every interactive element in this codebase must be
  keyboard-reachable, so a real button (Tab-focusable, activates on
  <kbd>Space</kbd>/<kbd>Enter</kbd>, accessible name "Open commit `<sha>` in
  your browser", full-hash tooltip, minimum `48 x controlHeight()` touch
  target matching every other button in this codebase) is used instead.
  Activating it opens the commit on GitHub via `QDesktopServices::openUrl()`.
  The one entry whose cited commit is not yet reachable from the shipped
  default branch (the "Related work" row, `1e59aca27...`, per
  `CHANGELOG.md`'s own "Related work not yet on `main`" section) is marked
  with a visible, non-color-only "Not yet on the default branch" label so the
  viewer never implies something shipped that has not.
- **Plain-text search with the shared regex builder.** The search field is a
  `UIMd3SearchField` -- the same shared component `History.md` and
  `NotificationCentre.md` use -- searching title, detail, category, section,
  version, and commit hash. Plain text is the default; the field's anchored
  builder button opens the same `UIMd3RegexBuilder` documented in
  `RegexBuilder.md` for full pattern/flag control.
- **A real calendar date-range filter that also accepts typed dates.** `From`
  and `To` are native `QDateEdit` fields with `setCalendarPopup(true)` (an
  anchored month/year calendar) and an explicit `yyyy-MM-dd` display format
  that is also directly typable, exactly matching the `From`/`To` pattern
  already shipped in `UIMd3History`'s browser. An explicit "Any date" special
  value (shown at the `1900-01-01` sentinel) means neither bound is
  unintentionally active by default.
- **Category and version filters compose with search and dates.** A
  `QComboBox` per axis (`All versions` / `All categories` plus every distinct
  value present), combined with the date range and the search field --
  `visibleCentreRows()` applies all four together as a conjunction, so
  narrowing one never silently discards another.
- **Filtered export and copy.** `Export Markdown` writes the currently
  filtered entries (search + category + version + date range all applied) as
  a `.md` file via `QFileDialog::getSaveFileName()` and an atomic
  `QSaveFile`, in the same `## [Version]` / `### Category` / `- **Title**
  ([`sha`](url))` shape `CHANGELOG.md` itself uses. `Copy to clipboard` does
  the same rendering directly onto `QGuiApplication::clipboard()`, so a
  filtered view can be pasted into an issue or a chat without opening a file
  dialog.
- **The "Unreleased" framing is stated up front.** A dedicated notice label
  at the top of the dialog states plainly, in the active language mode, that
  no release has shipped yet and every visible entry is provisional -- so a
  user filtering by "Version: Unreleased" is never confused into thinking
  that string is itself a real version number.

## Language modes and funny levels

Every user-facing string in the dialog -- the title, the unreleased notice,
every filter label, every button, every status line, the commit-open
accessible name and tooltip, the off-branch marker -- is registered through
`UIMd3Language::registerText()` with both an English and a Cantonese string,
exactly like `UIMd3History`'s browser. `md3Language().text()` resolves the
active mode (English / Cantonese / bilingual) and applies both independent
funny-level suffixes automatically; nothing in this viewer bypasses that
shared mechanism, so the funny sliders style this feature's copy the same
way they style every other registered surface's. The changelog **content**
itself -- the transcribed titles, details, dates, and commit hashes -- is
factual data, not styled UI chrome, and is never run through the funny-level
suffix: the house contract requires the funny level to change voice, never
facts, and a changelog is exactly the kind of factual record that rule exists
to protect.

## Accessibility, sizing, and persistence

- Every row sets an accessible name (the entry title) and accessible
  description (the full detail text) on its containing `QFrame`, matching
  `UIMd3NotificationCentre`'s row pattern.
- Every interactive control -- the search field, both combo boxes, both date
  edits, every per-row commit button, Export, and Copy -- is a real Qt widget
  reachable by <kbd>Tab</kbd> with the platform's normal visible focus ring;
  none of this dialog's interactivity depends on mouse-only affordances.
- Commit buttons and the Export/Copy buttons use the same `QSize(48,
  controlHeight())` minimum touch target every other button in this codebase
  uses, so nothing here introduces an undersized control.
- All row and status labels use `setWordWrap(true)` and sit inside a
  `QScrollArea` with `setWidgetResizable(true)`, so narrow window widths
  reflow content instead of clipping it -- the same layout shape
  `UIMd3NotificationCentre`'s row list already uses at this width class.
- **Persisted state:** none by design, matching `UIMd3History`'s and
  `UIMd3NotificationCentre`'s browsers -- both reset their filters every time
  the dialog reopens rather than persisting search/category/date state across
  restarts. The changelog *data* itself is compiled into the binary (not
  user-editable state), so there is nothing per-user to persist beyond the
  filter widgets, and resetting them on every open **is** the reset behavior
  the house contract asks for. If a future task adds cross-session filter
  memory, it should follow the same `gEDataManager` extra-data pattern
  `UIMd3Language` already uses for its own persisted settings.

## Failure modes

- If `UIMd3Theme::instance()` is not yet available, `showCentre()` returns
  without creating a dialog rather than constructing one against an
  unavailable palette (matches `UIMd3History::showCentre()`).
- Export and Copy are both no-ops when the current filter matches zero
  entries, rather than writing/copying an empty file or clipboard payload
  that looks like a bug.
- A failed `QSaveFile::open()`/`write()`/`commit()` during export leaves the
  status label untouched rather than falsely reporting success; the method
  simply returns without updating `m_pStatus`.
- Opening a commit URL always calls `QDesktopServices::openUrl()`, which
  degrades to whatever the operating system's own "no default browser
  configured" handling is; this viewer does not attempt to detect or report
  that condition itself, matching every other outbound-link affordance in
  this codebase (none of which implement their own browser-detection logic).

## Verification

**Evidence for this feature, honestly stated using this repository's own
`T`/`B`/`C` evidence shorthand from `doc/md3/CompletenessInventory.md`:**

- **T0** -- no automated test exists yet. No `md3-validation.yml`
  source-wiring contract step covers `UIMd3Changelog` (it is not in that
  workflow's fixed file list), and no compiled testcase exists.
- **B0** -- no build evidence. This pass explicitly excludes running the
  full Windows build (`tools/build-windows.ps1`, over an hour). The two new
  source files were verified mechanically instead: every `{`/`}` and
  `(`/`)` pair in both `UIMd3Changelog.h` and `UIMd3Changelog.cpp` was
  counted and confirmed balanced (42/42 braces and 680/680 parentheses in the
  `.cpp`; 4/4 and 32/32 in the `.h`), every method declared in the header has
  a matching definition in the source file, every `UIMd3ColorRole_*`/
  `UIMd3TypeRole_*` token used is a real enumerator from `UIMd3Tokens.h`, and
  every Qt class used is included. This is careful manual/mechanical review,
  not a compiler -- it cannot catch a real type error the way `kmk` would.
- **C0** -- no real capture. The installed build at
  `C:\Program Files\VirtualBox\VirtualBox.exe` referenced elsewhere in this
  repository's capture evidence predates this change and does not contain
  this code; a screenshot of it would show nothing this feature added.
  Capturing this surface needs the same full rebuild `B0` above excludes.

This mirrors the honesty discipline `doc/md3/CompletenessInventory.md` and
`doc/md3/LocalGates.md` already apply throughout this repository: an absent
gate is recorded as absent rather than assumed passing, and no claim of
"it works" is made beyond what was actually verified.

## Suggested articles

- [`History.md`](History.md) -- the sibling append-only local-history
  browser this viewer's dialog shape, search field, and date-range filter
  pattern were copied from.
- [`NotificationCentre.md`](NotificationCentre.md) -- the source of this
  viewer's per-row `QFrame` layout (title / metadata / detail / footer).
- [`RegexBuilder.md`](RegexBuilder.md) -- the shared anchored regex builder
  every `UIMd3SearchField`, including this viewer's, exposes.
- [`CommandPalette.md`](CommandPalette.md) -- how "Open changelog" is
  registered and reached from `Ctrl+Shift+F`'s palette.
- [`../../CHANGELOG.md`](../../CHANGELOG.md) -- the real, authoritative
  source this viewer's compiled-in entries are transcribed from.
- [`CompletenessInventory.md`](CompletenessInventory.md) -- item 15,
  updated by this change from "Not implemented on any surface" to the
  honest partial state recorded above.
