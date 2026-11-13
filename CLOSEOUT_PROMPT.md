# Closeout prompt

```
Continue Ding-Ding-Projects/material-virtualbox on main.

State after the 2026-11-13 pass:
- design/redesign-2026-11 holds a 20-board redesign (two canvases of 10 boards).
- src/VBox/Frontends/MaterialShell is a new desktop web front end built from those
  boards. It talks to VBoxSVC through VBoxMaterialBridge (bridge/VBoxMaterialBridge.cpp,
  line-delimited JSON on stdio). The running machine window stays native VirtualBoxVM.
- app/: `npm test` (12 tests, no installs), `npm run demo` for sample data,
  `npm run package:win` to package. The Windows release workflow builds the bridge
  and attaches MaterialVirtualMachine-Shell-win-x64.zip on a best-effort step.
- Upstream VirtualBox 9fe5508a2..b6bea7211 was ported in 4e10237fe.

First jobs:
1. Read the first Windows release run after 4e10237fe. Fix any compile error in the
   merged tree first, then any in VBoxMaterialBridge (that step is continue-on-error).
2. Run the packaged shell against a real VBoxSVC and capture every screen.
3. Make Display, Storage, Network and Shared folders editable in the shell settings.
Do not claim a VM can start: the unsigned hypervisor driver ceiling still applies.
```
