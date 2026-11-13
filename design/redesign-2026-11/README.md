# Material VirtualBox redesign, November 2026

Twenty boards in two canvases, made in the design tool and exported here board by
board. Every board is a self-contained `.dc.html` page: open it next to
`support.js` in a browser to see it live, with its interactions and motion.

## Part 1: core surfaces (`part1/project`)

| Board | File |
|---|---|
| Manager, machine library | `Main.dc.html` |
| Machine settings | `VM-Settings.dc.html` |
| New virtual machine wizard | `New-VM-Wizard.dc.html` |
| Running machine window | `Runtime-Window.dc.html` |
| Media manager | `Media-Manager.dc.html` |
| Network manager | `Network-Manager.dc.html` |
| Activity overview | `Activity-Overview.dc.html` |
| Extensions and extension packs | `Extensions.dc.html` |
| Log viewer | `Log-Viewer.dc.html` |
| Preferences | `Preferences.dc.html` |

## Part 2: product-wide features (`part2/project`)

| Board | File |
|---|---|
| Command palette (Ctrl+Shift+F) | `Main.dc.html` |
| Regular expression builder | `Regex-Builder.dc.html` |
| Tabs and tab manager | `Tab-Manager.dc.html` |
| Per-element appearance editor | `Appearance-Editor.dc.html` |
| Two-key super confirmation | `Super-Confirmation.dc.html` |
| Local history and changelog | `History.dc.html` |
| Export everything | `Export.dc.html` |
| Notifications and notification centre | `Notifications.dc.html` |
| First-run setup | `Onboarding.dc.html` |
| Motion and design tokens | `Motion-and-Tokens.dc.html` |

## Design language

- Material 3 Expressive, dark by default, seed `#6750A4`, Roboto Flex with the
  width axis used for display type, Roboto Mono for paths, logs and keys.
- Bento layouts, glass surfaces (62% container, 22 px blur), large shapes
  (28 to 36 px), springy state changes.
- Motion tokens live on the last board. Every board carries a complete
  `prefers-reduced-motion` path, and no state is shown by motion alone.

## Implementation route

The front end is being rebuilt from these boards as a desktop web front end
(`src/VBox/Frontends/MaterialShell`) that talks to the unchanged VirtualBox
Main API through a small native bridge. It is the only user interface: the Qt
GUI is no longer built, machines run headless, and the running-window board is
implemented as the shell's machine screen, streamed through the bridge.

## Screenshots

`shell-demo-machines.png`, `shell-demo-screens.png` and `shell-demo-screen.png` show the shell's
renderer in a browser against its built-in demo data (the yellow banner says
so on every screen). They are not captures of an installed release.
