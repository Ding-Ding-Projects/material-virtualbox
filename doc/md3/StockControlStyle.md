# Material 3 stock-control style

`UIMd3Style` is the shared `QProxyStyle` boundary for application-owned stock
Qt controls that remain inside existing VirtualBox pages. It makes those
controls visually coherent with the native Material 3 shell without replacing
their models, signals, validators, serializers, or action ownership.

## Behavior

The style is installed once after `UIMd3Theme` is created. It applies the
active semantic palette and body typography to the application, then refreshes
all existing widgets when `UIMd3Theme::sigThemeChanged` is emitted.

The current adapter covers:

- push, tool, check, and radio buttons;
- line edits, combo boxes, and spin boxes;
- horizontal and vertical sliders, scroll bars, and progress bars;
- tab bars, item views, headers, menus, toolbars, status bars, tooltips, and
  group-box frames;
- hover, pressed, selected, focused, and disabled presentation; and
- 48-pixel minimum rows or interaction targets for ordinary stock controls.

The palette uses Material surface, on-surface, outline, primary, secondary,
tertiary, and error roles. Interactive state layers are composited over their
container role rather than substituting translucent foreground color for the
container. Right-to-left and inverted progress and slider directions retain
their real value semantics.

## Configuration

There is no second style configuration store. Scheme, seed color, density,
font family, font scale, and font weight come from `UIMd3Theme` and use the
same persisted Global Preferences controls described in
[AppearanceSettings.md](AppearanceSettings.md).

The proxy style is owned by `UICommon`, so manager, settings, wizard, and
runtime targets consume the same implementation. Custom `UIMd3Widget`
subclasses continue to paint their own richer anatomy and per-element
appearance overrides.

## Accessibility and layout

The style does not replace Qt widgets or their accessibility interfaces. It
preserves their native roles, names, values, keyboard actions, focus policies,
and enabled state while adding a visible Material focus ring. Text-bearing
controls keep Qt's layout and elision behavior, and input, tab, menu, and list
rows receive bounded minimum anatomy for high-scale and touch use.

## Failure modes and compatibility

- If the Material theme singleton is unavailable, every paint and metric path
  falls back to the wrapped Qt style.
- Missing icons, invalid field values, and model restrictions remain the
  responsibility of their existing widgets and models; the style does not
  invent state or bypass validation.
- Native operating-system dialogs can retain platform presentation. This
  adapter covers stock Qt controls rendered inside the VirtualBox process; it
  does not claim to restyle external or operating-system-owned windows.
- Controls with dedicated VirtualBox painting may need a separate Material
  adapter. The proxy style is a shared compatibility layer, not evidence that
  every specialized surface has completed its rewrite.

## Security

Painting and sizing consume only bounded widget state and theme tokens. The
style performs no file, network, COM/XPCOM, guest, session, or credential
operation and does not alter action enabled predicates. Existing security and
validation boundaries remain authoritative.

## Verification

The changed source and Qt meta-object output compile in the Windows `UICommon`
target with Qt 6.8.3, MSVC 14.44, and Windows SDK 10.0.26100.0. The exact
commit, artifact hashes, and validation/Pages runs are recorded after the
source commit is published. This is compilation and link evidence only;
real-application visual, keyboard, assistive-technology, high-scale, and
bilingual captures remain part of the native runtime gate in
[RuntimeCapture.md](RuntimeCapture.md).

Suggested articles: [Appearance settings](AppearanceSettings.md),
[Manager shell](ManagerShell.md), [Wizard shell](WizardShell.md), and
[Runtime capture](RuntimeCapture.md).
