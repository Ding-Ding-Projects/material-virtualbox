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

Exact source commit
[`eb70fdf2047c4ceeb5ac9783ac41f640e582ec86`](https://github.com/Ding-Ding-Projects/material-virtualbox/commit/eb70fdf2047c4ceeb5ac9783ac41f640e582ec86)
compiled and linked in the Windows `UICommon`, `VirtualBox`, and `VirtualBoxVM`
targets with Qt 6.8.3, MSVC 14.44, and Windows SDK 10.0.26100.0. Installed
artifact SHA-256 values were:

- `UICommon.dll`: `7C6F2ECD7C6A73A8667F76E05AB0D05C0200FF2C74E005B3C1014622E5507A80`;
- `VirtualBox.exe`: `27FF189DF9355F899BD7E2B1A370E7B3622DB51D9E6111FB51251F6569E2C27E`; and
- `VirtualBoxVM.exe`: `D03DEB228A0BF99337EE001C26B772218B996610F4080AD8BDD2780029A1AC1E`.

Exact-tip validation
[run 31328985330](https://github.com/Ding-Ding-Projects/material-virtualbox/actions/runs/31328985330)
and Pages
[run 31328985294](https://github.com/Ding-Ding-Projects/material-virtualbox/actions/runs/31328985294)
completed successfully. The deployed Home page and this article returned HTTP
200 with the new content. This is compilation, link, source-contract, and
publication evidence only; real-application visual, keyboard,
assistive-technology, high-scale, and bilingual captures remain part of the
native runtime gate in [RuntimeCapture.md](RuntimeCapture.md).

Suggested articles: [Appearance settings](AppearanceSettings.md),
[Manager shell](ManagerShell.md), [Wizard shell](WizardShell.md), and
[Runtime capture](RuntimeCapture.md).
