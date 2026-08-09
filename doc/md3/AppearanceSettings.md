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
`UIMd3Theme`, supports reset, and returns focus to the edited widget. This
slice is intentionally marked in progress: the full archive contract still
needs a live preview, named-theme actions, the complete typography surface,
and the shared regex search field inside the editor. Those capabilities must
land before this article can claim complete per-element customization.

## Persistence and safety

Values are persisted by `UIMd3Theme` through the existing VirtualBox extra-data
store. No second preferences database, machine-wide registration, network
request, or guest/runtime operation is involved. The existing settings selector
and `UISettingsPageFrame::filterOut` remain authoritative for page visibility
and search; changing appearance does not discard pending settings edits.

## Verification

The static contract is covered by the UICommon source/MOC ownership in
`src/VBox/Frontends/VirtualBox/Makefile.kmk`. Native verification still
requires a built Windows application and a headless capture of Global
Preferences showing each scheme, valid and invalid seed input, font scale,
compact density, brand reset, keyboard focus, and persistence after restart.
The element editor additionally requires a real capture showing its context
menu search, apply/reset behavior, and focus return; no design thumbnail or
static HTML preview is accepted as a substitute.

Suggested articles: [`SettingsSearch.md`](SettingsSearch.md),
[`NavigationRail.md`](NavigationRail.md), and the repository
[build instructions](https://www.virtualbox.org/wiki/Build_instructions).
