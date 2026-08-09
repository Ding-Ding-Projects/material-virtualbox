# Command palette

The native manager now exposes a shared command palette on `Ctrl+Shift+F`.
The palette is a non-modal, bounded Qt overlay backed by the commands that
live surfaces register at runtime; it does not invent destinations or keep
handlers after their owning manager is destroyed.

## Behavior

- The manager registers preferences, new-machine, media-manager, appliance-import, and
  local-history commands. Each registration uses the stable `manager` owner id;
  the visible `Manager` category is localized separately, so language changes do
  not strand cleanup or duplicate rows.
- Plain-text matching remains the default through `UIMd3SearchField`; its anchored regex builder supplies the optional regex mode.
- Results are keyboard-focusable MD3 buttons and activate with Enter or Space.
- Escape closes the overlay and returns focus to the originating widget.
- A target widget can be raised, focused, switched into its owning stacked page, and briefly highlighted after activation.
- Empty results state is explicit and accessible; overlay size is bounded with an internal scroll area.

## Failure and safety behavior

Commands with empty titles, sources, or handlers are ignored and bounded before
they enter the registry. Duplicate title/source pairs replace their handler
rather than accumulating stale rows. Manager commands are removed with the same
stable owner id when `UIVirtualBoxManager` is destroyed, so a closed manager
cannot be called through an old palette entry. Handler failures remain owned by
the existing VirtualBox action and dialog paths.

## Verification

`UIMd3CommandPalette.{h,cpp}` is shared through `UICommon_QT_MOCHDRS` and
`UICommon_SOURCES`; the manager shortcut and registrations remain in the
`VirtualBox` target. The focused Windows build completed successfully. Native
runtime capture of the palette still requires a usable COM-registered manager;
the current checkout exits at `REGDB_E_CLASSNOTREG`, so no design preview is
presented as application evidence.

Suggested articles: [`AppearanceSettings.md`](AppearanceSettings.md),
[`SettingsSearch.md`](SettingsSearch.md), and
[`RuntimeCapture.md`](RuntimeCapture.md).
