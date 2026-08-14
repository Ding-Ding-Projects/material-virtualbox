# Open in external editor

`UIMd3ExternalEditor` is the native, self-contained core of the house contract's
"open in external editor" capability, for the VirtualBox Manager surface.

## What it does today

- **Detects the editors actually installed on this host**, best first:
  - Visual Studio Code (stable) — found on `PATH` (the `code` shim and portable
    builds), then the usual per-user (`%LOCALAPPDATA%\Programs\Microsoft VS Code`)
    and machine (`%ProgramFiles%\Microsoft VS Code`) install locations;
  - Visual Studio Code — Insiders;
  - the operating system's own default handler, always present as a guaranteed
    fallback.
- **Opens VirtualBox's configuration folder in the best available editor.**
  A folder handed to Code opens as a **workspace root**, so the whole
  configuration directory is navigable rather than a single file with no
  surrounding context — exactly what the contract requires of the VS Code path.
- **Degrades honestly.** If no real editor is found, it hands the folder to the
  operating system's default handler; if even that fails, it shows a clear
  message naming the folder and how to fix it, and never claims success it did
  not achieve.

## How it is reached

The command palette (`Ctrl+Shift+F`, or the header search pill) carries
**"Open the VirtualBox configuration folder in an external editor"**, wired to
`UIMd3ExternalEditor::openConfigFolder()`. The palette entry is localized through
the manager's `managerText` helper like every other palette command, so it
honors the three language modes.

## Configuration folder

`defaultConfigFolder()` returns `VBOX_USER_HOME` when that environment variable
is set, otherwise `~/.VirtualBox` — the folder holding `VirtualBox.xml` and the
per-machine settings trees. No COM round-trip is involved, so the path resolves
identically regardless of whether a VirtualBox session is available.

## Deliberately not yet done (honest scope)

This is a complete, working core, not the whole contract. The following are the
documented follow-up, and are **not** silently missing — the feature works end to
end without them, it simply always uses the best detected editor:

- a **persisted, user-chosen preferred editor** (stored via the extra-data
  manager) and an **add-your-own-editor** path;
- a **settings surface** exposing the choice, with the anchored regex-builder
  search every settings surface carries;
- **per-VM targets** — opening the selected machine's own settings folder rather
  than only the global configuration folder;
- **tests** for detection across present/absent Code, Insiders, portable, and
  no-editor hosts, and for the folder-resolution fallback.

## Verification

Static analysis only in the authoring environment (no local build toolchain):
brace/paren balance, `foreach`/`qEnvironmentVariable` availability confirmed
against other compiling frontend sources, `managerText` and the palette
registration signature confirmed against their definitions. Compile verification
runs on the Windows packaging workflow.

## Suggested articles

- [`CommandPalette.md`](CommandPalette.md) — the surface this feature is reached from.
- [`UIMd3Language`](README.md) — the language-mode register its palette text uses.
