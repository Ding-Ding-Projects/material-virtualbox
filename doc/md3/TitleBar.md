# Material manager title bar

The manager uses `UIMd3ManagerHeader` as its Material 3 title-bar surface on
Windows. `UIVirtualBoxManager` keeps the frameless-window and native hit-test
boundary, so resize borders, snap regions, and the platform system menu remain
owned by the window implementation rather than by a simulated drag surface.

The header exposes Menu, Minimize, Maximize/Restore, Close, and Notifications
actions. The existing `QMainWindow` menu, window state, and notification centre
remain authoritative; each Material button delegates to those real actions.
The header updates its display text, tooltip, and accessible name when the
persisted English/Cantonese/bilingual mode changes, including the dynamic
Maximize versus Restore state. Display-brand changes update the title label
without changing VirtualBox's technical identifiers or data paths.

Dragging is available from unused header space while the window is normal;
double-click toggles maximize/restore. Child controls retain their own pointer,
keyboard, and focus behavior, and the header does not intercept their clicks.

## Failure and security boundaries

The title bar never changes COM registration, VM session state, guest capture,
or the existing window action pool. If the manager cannot activate because the
VirtualBox COM/SDS components are unregistered, this surface cannot be opened;
that launch failure is recorded rather than replaced with a design preview.

## Verification

The focused Windows `VirtualBox` build compiles the manager header and links the
manager executable. Native verification remains open for real normal,
maximized, restored, snap-edge, high-DPI, keyboard-focus, and bilingual
captures after a sanctioned COM/SDS-capable runtime is available.

Suggested articles: [`AppearanceSettings.md`](AppearanceSettings.md),
[`TabNavigation.md`](TabNavigation.md), and [`RuntimeCapture.md`](RuntimeCapture.md).
