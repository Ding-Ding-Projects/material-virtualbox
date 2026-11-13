# Material shell

A Material 3 Expressive front end for VirtualBox, built from the boards in
[`design/redesign-2026-11`](../../../../design/redesign-2026-11/README.md).

```
 ┌──────────────────────────────┐   stdio, one JSON line per call   ┌──────────────────────┐   COM   ┌─────────┐
 │ app/  (desktop web front end)│ ───────────────────────────────▶ │ VBoxMaterialBridge   │ ──────▶ │ VBoxSVC │
 │ main/ renderer/              │ ◀─────────────────────────────── │ bridge/*.cpp (kBuild)│         └─────────┘
 └──────────────────────────────┘                                   └──────────────────────┘
                    machines run headless; screen.* streams the display and sends keyboard and mouse back
```

**The Material shell is the only user interface.** `Config.kmk` forces
`VBOX_WITH_QTGUI` and `VBOX_WITH_DEBUGGER_GUI` off with `override`, so no build
of this tree produces the Qt manager (`VirtualBox.exe`), the Qt machine window
(`VirtualBoxVM`) or the Qt debugger. The release workflow and
`tools/build-windows-nsis-installer.ps1` refuse a payload that contains any of
them, and the installer deletes ones left by an older install.

- **The back end does not change.** Every action goes to the Main API in
  `VBoxSVC`, so this shell, the Qt manager and `VBoxManage` always agree.
- **The machine screen lives in the shell.** Starting a machine launches it
  headless; the shell attaches a second bridge process to it and streams PNG
  frames (up to about 15 a second) while sending keyboard scan codes and
  absolute mouse events back. Right Ctrl releases the keyboard.
- **The renderer is sandboxed.** `contextIsolation` and `sandbox` are on, Node is
  off, and `main/preload.cjs` exposes a fixed list of bridge methods only.

## Bridge protocol

Requests and responses are single lines of JSON on the bridge's stdin and stdout:

```json
{"id": 4, "method": "machine.update", "params": {"id": "<uuid>", "memoryMB": 8192}}
{"id": 4, "result": {"id": "<uuid>", "name": "Dev Box", "memoryMB": 8192, ...}}
{"id": 5, "error": {"message": "The machine is not running", "hrc": -2147467259}}
```

| Method | Params | Does |
|---|---|---|
| `host.info` | | Version, host OS, processors, memory |
| `machines.list` | | Every registered machine with its state |
| `machine.details` | `id` | Firmware, video memory, storage, network, shared folders |
| `machine.create` | `name`, `osTypeId`, `group?`, `memoryMB?`, `cpus?`, `diskGB?` | Creates, applies OS defaults, registers, optionally adds a dynamic VDI |
| `machine.start` | `id`, `type?` (`gui`, `headless`, `separate`) | Launches the machine process |
| `machine.control` | `id`, `action` (`show`, `pause`, `resume`, `reset`, `acpiShutdown`, `powerOff`, `saveState`) | Controls a running machine |
| `machine.update` | `id`, `name?`, `memoryMB?`, `cpus?`, `description?` | Changes settings and saves them |
| `machine.delete` | `id`, `confirmName` | Unregisters and deletes files; the name must match |
| `snapshots.list` / `snapshots.take` | `id` (+ `name`, `description?`) | Snapshot tree / new snapshot |
| `media.list` | `kind?` (`disk`, `optical`, `floppy`) | Registered media and what uses them |
| `networks.list` | | NAT networks and host interfaces |
| `logs.read` | `id`, `index?`, `offset?` | Up to 512 KB of a machine log |
| `extpacks.list` | | Installed extension packs |
| `metrics.sample` | | Latest CPU and RAM samples for the host and running machines |
| `screen.attach` / `screen.detach` | `id` | Takes a shared lock on a running machine for its screen |
| `screen.frame` | `id` | The current screen as base64 PNG with its size |
| `screen.keys` | `id`, `scancodes` | Set 1 scan codes, up to 256 per call |
| `screen.mouse` | `id`, `x`, `y`, `dz?`, `dw?`, `buttons?` | Absolute pointer, zero-based guest pixels |
| `screen.resize` | `id`, `width`, `height` | Asks the guest for a new resolution |

## Building

- **Bridge:** part of every full build (`VBOX_WITH_MATERIAL_SHELL` is always on);
  on its own, `kmk -C src/VBox/Frontends/MaterialShell`.
- **Front end:** in `app/`, `npm test` runs the unit tests with no installs;
  `npm install && npm run package:win` produces the Windows folder. Put
  `VBoxMaterialBridge.exe` next to `MaterialVirtualMachine.exe`, or set
  `MVB_BRIDGE_PATH`.
- **Release:** the Windows release workflow stages the packaged shell under
  `bin\Shell`, so the installer ships it and points every shortcut at it, and
  attaches `MaterialVirtualMachine-Shell-win-x64.zip` as well. A shell failure now
  fails the release, since there is no other user interface.

## Running

- `npm start` connects through the bridge. Without a bridge the window says so
  plainly on every screen.
- `npm run demo` (or `--demo`) answers from built-in sample data, with a banner on
  every screen saying that nothing is real.

## What is in the front end

Machines (library, details, snapshots, logs, live resource use, start, show,
pause, shut down, power off, snapshot, export, delete), machine settings (name,
memory, processors), the New virtual machine wizard, Media with bulk selection,
Network topology, Extensions, the log viewer with regex highlighting, live
Activity charts, History with restore-as-new-revision, Preferences (seed colour,
three language modes, four motion levels, low stimulation, school mode), the
tab manager, first-run setup, the command palette (Ctrl+Shift+F), the regex
builder on every search field, the two-key super confirmation, the export sheet
and the notification centre.

## Known limits

- The bridge has not been compiled on this branch's authoring machine; the
  release workflow is its first build, and a compile error there now fails the
  whole build.
- Frames are PNG screenshots, not a video stream: expect about 10 to 15 frames a
  second, fine for desktops and installers, not for games.
- `VBoxManage startvm` without `--type headless` fails, because there is no
  `VirtualBoxVM` to start; that is intended.
- Settings pages other than General and System are read-only in the shell for
  now; the classic Qt settings dialog still edits them.
- No virtual machine can start on a host that refuses the unsigned hypervisor
  driver. The shell reports that failure as VBoxSVC returns it.
