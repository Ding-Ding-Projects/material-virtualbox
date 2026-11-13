# Material shell

A Material 3 Expressive front end for VirtualBox, built from the boards in
[`design/redesign-2026-11`](../../../../design/redesign-2026-11/README.md).

```
 ┌──────────────────────────────┐   stdio, one JSON line per call   ┌──────────────────────┐   COM   ┌─────────┐
 │ app/  (desktop web front end)│ ───────────────────────────────▶ │ VBoxMaterialBridge   │ ──────▶ │ VBoxSVC │
 │ main/ renderer/              │ ◀─────────────────────────────── │ bridge/*.cpp (kBuild)│         └─────────┘
 └──────────────────────────────┘                                   └──────────────────────┘
                                         machine.start (type "gui") launches the native VirtualBoxVM window
```

- **The back end does not change.** Every action goes to the Main API in
  `VBoxSVC`, so this shell, the Qt manager and `VBoxManage` always agree.
- **The running machine window stays native.** Starting or showing a machine
  opens the regular `VirtualBoxVM` window; the shell never draws the guest screen.
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

## Building

- **Bridge:** `kmk -C src/VBox/Frontends/MaterialShell` (template `VBoxMainClientExe`).
  It is also pulled into a full build when `VBOX_WITH_MATERIAL_SHELL=1`.
- **Front end:** in `app/`, `npm test` runs the unit tests with no installs;
  `npm install && npm run package:win` produces the Windows folder. Put
  `VBoxMaterialBridge.exe` next to `MaterialVirtualMachine.exe`, or set
  `MVB_BRIDGE_PATH`.
- **Release:** the Windows release workflow builds both on a best-effort step and
  attaches `MaterialVirtualMachine-Shell-win-x64.zip` when it succeeds.

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
  release workflow is its first build. If it fails there, the release still ships
  without the shell asset and the step log has the compiler error.
- Settings pages other than General and System are read-only in the shell for
  now; the classic Qt settings dialog still edits them.
- No virtual machine can start on a host that refuses the unsigned hypervisor
  driver. The shell reports that failure as VBoxSVC returns it.
