# Material appearance settings

Global Preferences now exposes the shared Material 3 appearance controls in
the existing `UIAdvancedSettingsDialog` surface. The settings dialog remains
the owner of page selection, filtering, validation, serialization, and dirty
state; these controls only call the process-wide `UIMd3Theme` service.

## Controls

- **Theme:** Dark, Light, System, High-contrast dark, or High-contrast light.
- **Seed color:** A bounded hexadecimal color value. Invalid input is rejected
  with an inline tooltip and never reaches the palette generator.
- **Font scale:** 75% through 200%, applied live to the shared Material type
  scale.
- **Font family:** The installed-family picker previews each face in its own
  typeface and applies the chosen family to the shared Material type scale.
- **Font weight:** A global Thin-through-Black override, or **Inherited** to
  retain each Material text role's shipped weight. The value is applied live
  and is bounded before persistence.
- **Compact density:** Changes the shared control-height and gutter tokens.
- **Display brand:** Changes visible product chrome only. Resetting restores
  `Material Virtual Machine`; technical identifiers, paths, COM names, and
  file formats are unchanged.

- **Funny levels:** English and Hong Kong Cantonese each have an independent
  1–5 slider. Level 1 keeps the registered copy serious; higher levels style
  the surrounding voice without changing the action or its factual meaning.
  Mode and both levels are written immediately to the existing extra-data
  store and emit `UIMd3Language::sigLanguageChanged` for live surfaces.

Every control has an accessible name, a keyboard path, and a progressive
tooltip explaining its effect. A theme change emits `UIMd3Theme::sigThemeChanged`
so the manager header, navigation rail, palette, and other subscribed widgets
refresh without restarting the application.

## Element editor lane

MD3 widgets with a stable appearance key now expose **Edit appearance…** from
their context menu and from <kbd>Shift</kbd>+right-click. The bounded editor
persists an element seed, typeface, corner radius, scale, and weight through
`UIMd3Theme`, supports reset, and returns focus to the edited widget. Its
anchored search remains plain-text-first with the full regex builder available,
and the live preview reflects the pending values without persisting them.

The same editor now offers a saved-theme picker, a bounded name field, and
truthful **Save named theme** / **Apply named theme** actions. Saving captures
the current persisted palette, typography, density, display brand, and every
element override; pending edits in the open editor are not silently included
until **Apply** succeeds. Applying validates the complete saved record before
touching the live theme, refreshes the editor preview, and reports success or
failure in an accessible inline status. Invalid names, malformed records, and
out-of-range element values are rejected transactionally, leaving the prior
live and saved state unchanged. Six- or eight-digit HEX colors retain their
alpha channel through persistence, export, import, and history. Full Word-depth typography and genuine native
capture remain open design-coverage work.

## Persistence and safety

Values are persisted by `UIMd3Theme` through the existing VirtualBox extra-data
store. No second preferences database, machine-wide registration, network
request, or guest/runtime operation is involved. The existing settings selector
and `UISettingsPageFrame::filterOut` remain authoritative for page visibility
and search; changing appearance does not discard pending settings edits.

Each validated theme or appearance mutation preflights the bounded state
payload before it is persisted, then records a state revision in
`UIMd3History` when the local history store accepts it. A history-store refusal
does not undo the already validated appearance change; it is reported as a
non-blocking persistence limitation rather than a false success. The history browser can restore those revisions
through `UIMd3Theme::restoreState`; version, palette, density, typography,
display brand, named-theme, and per-element fields are checked as one
transaction before the live theme changes, and the restore itself appends a
new revision. Malformed, non-finite, out-of-range, or oversized state is
rejected without changing the current appearance or saved-theme set.

## Verification

The static contract is covered by the UICommon source/MOC ownership in
`src/VBox/Frontends/VirtualBox/Makefile.kmk`. Native verification still
requires a built Windows application and a headless capture of Global
Preferences showing each scheme, valid and invalid seed input, font scale,
compact density, installed font-family preview, inherited and explicit font
weights, brand reset, keyboard focus, and persistence after restart.
The element editor additionally requires a real capture showing its context
menu search, regex builder, live preview, named-theme save/apply status,
transactional rejection, apply/reset behavior, and focus return; no design
thumbnail or static HTML preview is accepted as a substitute.

Suggested articles: [`SettingsSearch.md`](SettingsSearch.md),
[`NavigationRail.md`](NavigationRail.md), and the repository
[build instructions](https://www.virtualbox.org/wiki/Build_instructions).
