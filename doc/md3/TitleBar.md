# Material manager title bar

The manager uses `UIMd3ManagerHeader` as its Material 3 title-bar surface on
Windows. `UIVirtualBoxManager` keeps the frameless-window and native hit-test
boundary, so resize borders, snap regions, and the platform system menu remain
owned by the window implementation rather than by a simulated drag surface.

The 48-pixel header exposes an icon menu, application mark and display name,
manager subtitle, bounded command-palette pill, notification bell and unread
marker, and compact Minimize, Maximize/Restore, and Close actions. The existing
`QMainWindow` menu, window state, command registry, and notification centre
remain authoritative; each Material button delegates to those real actions.
The legacy menu bar is kept as the action model but hidden from the permanent
layout, and its menus open beside the header's menu action.
The header updates its display text, tooltip, and accessible name when the
persisted English/Cantonese/bilingual mode changes, including the dynamic
Maximize versus Restore state. Display-brand changes update the title label
without changing VirtualBox's technical identifiers or data paths.
Each window action occupies a 48 by 48 logical-pixel target and exposes a real
accessible Button role and press action. Each button also has a stable appearance
key, so a per-control customization
does not drift when its localized label changes. Icon variants center their
artwork independently of text-button padding.

At constrained widths the palette action becomes icon-only, the subtitle is
progressively hidden, and the visible display name is elided. The complete
display name remains available through the accessible name and tooltip.

Dragging is available from unused header space while the window is normal;
double-click toggles maximize/restore. Child controls retain their own pointer,
keyboard, and focus behavior, and the header does not intercept their clicks.
On Windows, native `HTMAXBUTTON` hit testing exposes Snap Layouts and native
caption hit testing preserves resize borders. The frameless window bridges the
non-client maximize press/release sequence back to the same Material control,
so pointer and keyboard activation cannot drift into separate behavior. System
move is used for normal dragging and maximized drag-to-restore. Restored
geometry is constrained to the active display's available geometry.

Existing raster artwork is rendered through theme-colored, device-pixel-ratio-
aware masks so dark/high-contrast roles and high display scales do not retain
the old glossy icon colors. The application menu uses the shared searchable
menu helper and anchored full regex builder.

## Failure and security boundaries

The title bar never changes COM registration, VM session state, guest capture,
or the existing window action pool. If the manager cannot activate because the
VirtualBox COM/SDS components are unregistered, this surface cannot be opened;
that launch failure is recorded rather than replaced with a design preview.

## Verification

The commit-attributed local Windows `VirtualBox` build in the
[manager-shell evidence table](ManagerShell.md#verification-and-remaining-work)
compiled the manager header and linked and installed the manager executable;
native verification remains open for real normal, maximized, restored,
snap-edge, high-DPI, keyboard-focus, and bilingual
captures after a sanctioned COM/SDS-capable runtime is available.

Suggested articles: [`ManagerShell.md`](ManagerShell.md),
[`AppearanceSettings.md`](AppearanceSettings.md),
[`TabNavigation.md`](TabNavigation.md), and [`RuntimeCapture.md`](RuntimeCapture.md).
