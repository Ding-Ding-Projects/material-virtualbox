# Dialog and message-box emojis

`UIMd3EmojiSetting` owns one persisted, opt-in preference that adds a relevant,
purely decorative emoji to the displayed message in `QIMessageBox`.

## Behavior

- The setting is disabled by default and stored under `GUI/Md3/EmojiDialogs`.
- Users can change it from the Manager command palette or from the language and
  appearance section of global settings.
- `QIMessageBox` maps its existing icon classification to information,
  question, warning, or error decoration. The existing icon, factual message,
  button labels, field labels, action labels, clipboard copy, and accessible
  names remain unchanged.
- Rich-text messages keep their leading `html`, `body`, `qt`, or `p` container;
  the decorative prefix is inserted inside that first tag so the markup remains
  valid and the emoji stays with the visible message.
- Turning the preference off returns the original message string unchanged.
  Existing message boxes are not rewritten while open; the next message box
  reflects the new setting.

## Failure modes and privacy

An unset or unreadable extra-data value is treated as disabled. The feature has
no network, credential, telemetry, or file-export path. Emoji are compiled-in
presentation strings and never replace semantic text.

## Verification

The integration is source-verified in `QIMessageBox::prepare()`, the settings
surface, and `UIMd3Language`. A configured Qt/Windows build and built-artifact
interaction capture remain required; this checkout does not contain an
initialized `kBuild` submodule or a configured frontend toolchain.

## Suggested articles

- [Completeness inventory](CompletenessInventory.md)
- [Material 3 documentation index](README.md)
- [Local gates](LocalGates.md)
