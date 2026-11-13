# Closeout prompt

```
Continue Ding-Ding-Projects/material-virtualbox on main.

State after the 2026-11-13 pass:
- design/redesign-2026-11 holds a 20-board redesign (two canvases of 10 boards).
- src/VBox/Frontends/MaterialShell is the ONLY user interface. Config.kmk forces
  VBOX_WITH_QTGUI off; the workflow and NSIS script reject any Qt UI file. It talks to
  VBoxSVC through VBoxMaterialBridge (line-delimited JSON on stdio); machines run
  headless and screen.* streams frames, keys and mouse into the shell.
- app/: `npm test` (12 tests, no installs), `npm run demo` for sample data,
  `npm run package:win` to package. The Windows release workflow builds the bridge
  and attaches MaterialVirtualMachine-Shell-win-x64.zip on a best-effort step.
- Upstream VirtualBox 9fe5508a2..b6bea7211 was ported in 4e10237fe.

First jobs:
1. Read the first Windows release run of the no-Qt build. Fix compile errors in the
   merged tree and VBoxMaterialBridge first (the bridge is now a required target).
2. Run the packaged shell against a real VBoxSVC and capture every screen.
3. Make Display, Storage, Network and Shared folders editable in the shell settings.
Do not claim a VM can start: the unsigned hypervisor driver ceiling still applies.
```
