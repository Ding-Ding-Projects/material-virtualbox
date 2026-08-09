# Command palette

The native manager now exposes a shared command palette on `Ctrl+Shift+F`.
The palette is a non-modal, bounded Qt overlay backed by the commands that
live surfaces register at runtime; it does not invent destinations or keep
handlers after their owning manager is destroyed.

## Behavior

- The manager registers preferences, machine creation/import/export, media-manager,
  cloud-machine, machine-settings, clone/OCI-export, and local-history commands.
  Each registration uses the stable `manager` owner id plus a stable command id;
  the visible `Manager` category and command copy are registered through
  `UIMd3Language`, so English, playful Cantonese, bilingual mode, and the two
  independent funny-level sliders refresh without stranding cleanup, duplicate
  rows, or per-command appearance overrides.
- Plain-text matching remains the default through `UIMd3SearchField`; its anchored regex builder supplies the optional regex mode.
- Results are grouped by their localized category, match title/category/source/id,
  and are keyboard-focusable MD3 buttons. Up/Down moves between available and
  unavailable rows; an unavailable row stays focusable so its explanation can be
  announced, while Enter or Space is rejected until its live predicate is true.
- Existing action-pool commands keep their live enabled state. A disabled command
  remains discoverable with a localized explanation and cannot trigger a stale
  handler.
- Manager command titles, categories, and disabled reasons are rebuilt on the
  persisted `UIMd3Language` signal, including Cantonese and bilingual copy; each
  action-pool command keeps its stable owner/id pair.
- Escape closes the overlay and returns focus to the originating widget.
- Handler-owned destinations keep their own focus: the palette does not raise the
  manager again after opening Preferences, a wizard, a tool, or local history.
  Other registered commands may still raise, focus, switch, and briefly highlight
  an existing target widget.
- Empty results state is explicit and accessible; overlay size and position are
  bounded to the current screen, including narrow available geometries, with an
  internal scroll area. Refresh hides and disables old rows before deferred
  deletion so stale handlers cannot be clicked during a language or query update.

## Failure and safety behavior

Commands with empty titles, sources, or handlers are ignored and bounded before
they enter the registry. Duplicate source/id pairs replace their handler rather
than accumulating stale rows. Manager commands are removed with the same stable
owner id when `UIVirtualBoxManager` is destroyed, so a closed manager cannot be
called through an old palette entry. Action-pool policy remains authoritative;
the palette checks enabled state immediately before triggering. Handler failures
remain owned by the existing VirtualBox action and dialog paths.

## Verification

`UIMd3CommandPalette.{h,cpp}` is shared through `UICommon_QT_MOCHDRS` and
`UICommon_SOURCES`; the manager shortcut and registrations remain in the
`VirtualBox` target. The focused Windows build is the authoritative compile
gate for this lane. Native runtime capture of the palette still requires a usable COM-registered manager;
the current checkout exits at `REGDB_E_CLASSNOTREG`, so no design preview is
presented as application evidence.

Suggested articles: [`AppearanceSettings.md`](AppearanceSettings.md),
[`SettingsSearch.md`](SettingsSearch.md), and
[`RuntimeCapture.md`](RuntimeCapture.md).
