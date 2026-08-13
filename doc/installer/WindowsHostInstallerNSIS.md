# Windows host installer (NSIS)

`src/VBox/Installer/win/NSIS/VBoxHostInstaller.nsi` (with its shared macros in
`VBoxHostInstallerCommon.nsh`) is an NSIS-based installer for the VirtualBox
Windows **host** product: VirtualBox Manager, `VBoxSVC`, `VBoxSDS`, and the
host kernel drivers. It is built by `tools/build-windows-nsis-installer.ps1`
from an already staged release payload (`out\win.amd64\release\bin`, the
same directory `tools/build-windows.ps1` packages into the currently wired
unsigned Squirrel.Windows installer).

## Why this installer exists

Squirrel.Windows is a per-user file unpacker with no elevation. Installing
VirtualBox with it copies files but cannot register COM classes, cannot
install or start the `VBoxSDS` Windows service, and cannot install kernel
drivers -- VirtualBox is a hypervisor and needs all three. The observable
result is that `VirtualBox.exe` starts, renders its UI, and then immediately
fails with:

```
Can't acquire VirtualBox!
Failed to acquire the VirtualBox COM object. The application will now terminate.
The VBoxSDS windows service was not found.
Result Code: REGDB_E_CLASSNOTREG (0x80040154)
Component: VirtualBoxClientWrap
```

This installer requests Administrator privileges (`RequestExecutionLevel
admin`) and performs the elevated setup steps Squirrel cannot, so that
VirtualBox Manager genuinely starts after installation.

## What it installs and registers

1. **Files.** The entire staged payload (`out\win.amd64\release\bin`) is
   copied into the chosen install directory (`$PROGRAMFILES64\VirtualBox` by
   default, user-changeable on the Directory page).
2. **In-process COM servers**, registered with NSIS's native `RegDLL`
   (`DllRegisterServer`):
   - `VBoxProxyStub.dll` -- exports `DllRegisterServer`/`DllUnregisterServer`
     (`src/VBox/Main/src-all/win/VBoxProxyStub.def`); handles proxy/stub
     marshalling and interface registration.
   - `VBoxC.dll` -- also self-registers
     (`src/VBox/Main/src-client/win/VBoxC.def`); this is the client-side COM
     wrapper named in the `VirtualBoxClientWrap` component of the crash
     above.
3. **The `VBoxSVC` out-of-process COM server**, via
   `VBoxSVC.exe /RegServer`. `VBoxSVC.exe`'s own command line help (see
   `src/VBox/Main/src-server/win/svcmain.cpp`) documents `/RegServer`,
   `/UnregServer`, and `/ReregServer` for exactly this purpose; the option
   table accepts the flag case-insensitively with a `--`, `-`, or `/` prefix.
4. **The `VBoxSDS` COM class and Windows service**, via
   `VBoxSDS.exe --regservice`. `VBoxSDS.cpp`
   (`src/VBox/Main/src-global/win/VBoxSDS.cpp`) implements `--regservice` /
   `--unregservice` / `--reregservice` as `registerService()` /
   `unregisterService()`, which in turn call `installService()` /
   `uninstallService()`. `installService()` creates a Windows service
   literally named `VBoxSDS` (`m_wszServiceName`, set from the literal
   `L"VBoxSDS"` passed to `CComServiceModule::init()` at the bottom of
   `VBoxSDS.cpp`) as `SERVICE_WIN32_OWN_PROCESS` /
   `SERVICE_DEMAND_START`, depending on `RPCSS`. Because it is
   demand-start, Windows starts it automatically the first time a client
   activates the `VBoxSDS` COM class -- this installer does not (and does
   not need to) start it explicitly.
5. **Start Menu and Desktop shortcuts**, an uninstaller
   (`$INSTDIR\uninstall.exe`), and Add/Remove Programs metadata (display
   name, version, publisher, install location, an `EstimatedSize` computed
   from the installed payload, and `NoModify`/`NoRepair`).
6. **A registry record** of the install directory and version under
   `HKLM\Software\<vendor short name>\VirtualBox`, matching the pattern the
   Guest Additions NSIS installer uses for its own product key.

None of the above requires a signed kernel driver, so all of it runs
unconditionally, and steps 3-4 abort the installation with a clear message
(pointing at the install log) if they fail -- these are load-bearing for a
working VirtualBox Manager, so a silent failure here would be worse than an
installer that stops and says so.

## Kernel drivers: what is attempted, and what deliberately is not

The installer attempts kernel driver installation through this tree's own
`VBoxDrvInst.exe` helper (present in the payload root), using the identical
command pattern the Guest Additions NSIS installer already uses for the
in-guest drivers (see
`src/VBox/Additions/win/Installer/VBoxGuestAdditionsW2KXP.nsh`):

```
VBoxDrvInst.exe --logfile "<per-driver log>" install --inf-file "<path to .inf>" --ignore-reboot [--pnp-id "<hardware id>"]
```

Three host drivers are attempted, each root-enumerated / non-PnP (installed
via their `.inf`'s `[DefaultInstall]` section, exactly like the
`VBoxGuest`/`VBoxMouse`/`VBoxVideo` guest drivers) or staged against a known
hardware ID:

| Driver | Service | Source `.inf` | How it installs |
| --- | --- | --- | --- |
| `VBoxSup` | `VBoxSup` | `src/VBox/HostDrivers/Support/win/VBoxSup.inf` | `[DefaultInstall]` root install (no `--pnp-id`) |
| `VBoxUSBMon` | `VBoxUSBMon` | `src/VBox/HostDrivers/VBoxUSB/win/mon/VBoxUSBMon.inf` | `[DefaultInstall]` root install (no `--pnp-id`) |
| `VBoxUSB` | `VBoxUSB` | `src/VBox/HostDrivers/VBoxUSB/win/dev/VBoxUSB.inf` | `[Manufacturer]`/model install staged for the emulated device's own hardware ID, `USB\VID_80EE&PID_CAFE` (taken directly from the `.inf`'s `[VBoxUSB...]` model section), passed as `--pnp-id` |

**`VBoxNetAdp6` (host-only networking) and `VBoxNetLwf` (bridged networking)
are deliberately NOT attempted by this installer.** Inspecting their `.inf`
files
(`src/VBox/HostDrivers/VBoxNetAdp/win/VBoxNetAdp6.inf`,
`src/VBox/HostDrivers/VBoxNetFlt/win/drv/VBoxNetLwf.inf`) and the existing
WiX/MSI custom-action helper
(`src/VBox/Installer/win/InstallHelper/VBoxInstallHelper.cpp`, functions
`createHostOnlyInterface()` / `updateHostOnlyInterfaces()`) shows these are
NDIS miniport/filter components that must be registered through the Windows
Network Configuration (`INetCfg`) COM API to actually create a host-only
adapter instance or bind a filter to the network stack -- plain
`SetupDiInstallDriver`-style staging (what `VBoxDrvInst.exe install` does)
is not sufficient to make them functional, and would risk leaving behind a
partially configured, non-functional network component. Implementing the
`INetCfg` integration was out of scope for this task; host-only and bridged
networking are not available after using this installer. This is stated
in the installer's own log and in this document rather than left as a
silent gap.

### Honest reporting when a driver is refused

`VBoxDrvInst.exe` (`src/VBox/GuestHost/installation/VBoxDrvInst.cpp`) returns
`RTEXITCODE_SUCCESS` (0) on success, `VBOXDRVINSTEXITCODE_REBOOT_NEEDED` (4)
on success-but-reboot-required, `VBOXDRVINSTEXITCODE_WARNING` (5) on
success-with-warning, and any other non-zero value on failure. The
installer's `${AttemptDriverInstall}` macro
(`VBoxHostInstallerCommon.nsh`) classifies every one of these outcomes
explicitly:

- **0** -- logged as installed.
- **4** -- logged as installed, and `SetRebootFlag true` is set so the
  Finish page's normal NSIS reboot prompt applies.
- **5** -- logged as installed with a warning, pointing at the per-driver
  log file `VBoxDrvInst_<name>.log` for detail.
- **Anything else** -- logged as a refusal, `$G_DriverInstallFailed` is set,
  and the message explains *why*: this build is unsigned (code signing is
  permanently disabled for this project) and, on a default 64-bit Windows
  system, Driver Signature Enforcement refuses to load an unsigned kernel
  driver. The message names the exact per-driver log file for the precise
  reason Windows reported, and states plainly that VirtualBox Manager will
  still work but starting a virtual machine will not, until the driver is
  permitted to load.

If any driver was refused, a single summary `MessageBox` is shown at the end
of installation (skipped in silent mode; the same information is always in
the log) repeating that this installer never modifies Driver Signature
Enforcement, BCD test-signing, or any other system security setting -- it
only reports what Windows itself decided.

If a driver's `.inf` is not even present in the payload -- which is
currently the case, see **Known build limitation** below -- that is also
logged explicitly and distinguished from a refusal: "not present in this
build" is a different, more basic fact than "present but refused as
unsigned," and the installer never conflates the two.

The driver `.inf` file locations are probed against a bounded set of
plausible relative paths (see `${FindDriverInf}` in
`VBoxHostInstallerCommon.nsh`) rather than a single assumed path, because the
exact sub-directory layout a full build stages these files into has not been
verified against this repository's current pipeline (again, see **Known
build limitation**). If none of the probed candidates exist, the driver is
reported as skipped rather than the installer guessing incorrectly.

## Known build limitation: this pipeline does not currently produce host drivers

`tools/build-windows.ps1`'s `Invoke-VirtualBoxBuild` passes `--disable-win-ddk`
to `configure.py`. Empirically, the payload this pipeline currently produces
(`out\win.amd64\release\bin`) contains `VBoxSVC.exe`, `VBoxSDS.exe`,
`VirtualBox.exe`, `VBoxProxyStub.dll`, `VBoxC.dll`, and `VBoxDrvInst.exe`,
but **no** `VBoxSup.sys`/`.inf`, `VBoxUSBMon.sys`/`.inf`, `VBoxUSB.sys`/`.inf`,
`VBoxNetAdp6.sys`/`.inf`, or `VBoxNetLwf.sys`/`.inf`.
`VBOX_WITHOUT_WIN_HOST_INSTALLER` (also set by that script) only affects
`src/VBox/Installer/win/Makefile.kmk` (the WiX/MSI installer build) and does
not, by itself, explain this; the absence of driver binaries and `.inf`
files points at `--disable-win-ddk` disabling the WDK-dependent kernel
driver build entirely for this pipeline.

This means that, run against the pipeline as it stands today, the driver
installation steps documented above will uniformly log "not present in this
build" rather than "refused as unsigned" -- there is nothing to attempt.
**This installer script and its build helper were written to be correct
once host drivers are produced** (their probing and honest-reporting logic
does not assume drivers exist), but making that happen requires a change to
`tools/build-windows.ps1` and/or `configure.py` -- both outside this task's
permitted file set. The concrete change needed is to drop
`--disable-win-ddk` (and make sure the WDK bootstrap `Ensure-WindowsKits`
already performs in that script is actually used for the driver compile, not
only for the NSIS-building prerequisite pass) so the "packing" step
produces the driver `.sys`/`.inf` files into the payload.

## Silent installation and uninstall

- `VirtualBox-<version>-Setup.exe /S` installs silently (native NSIS `/S`
  switch); `/S /D=<path>` (installer-`/D` must be the last argument, no
  quotes) overrides the install directory. Silent mode suppresses the
  Finish-page summary `MessageBox` but not the plain-text install log.
- `"$INSTDIR\uninstall.exe" /S` uninstalls silently; this is also the
  string written to the registry's `QuietUninstallString` value.
- Uninstall runs the same `VBoxDrvInst.exe uninstall --inf-file ...
  --ignore-reboot` calls (best-effort, logged, never fatal -- a driver that
  was refused as unsigned at install time was never actually loaded, so a
  non-zero uninstall exit code for it is expected and not an error) and
  `VBoxSDS.exe --unregservice` / `VBoxSVC.exe /UnregServer` /
  `UnRegDLL` for the two self-registering DLLs, all run *before* the
  installed files are deleted.
- Both the installer and uninstaller write a plain text log
  (`VBoxHostInstaller_Install.log` / `VBoxHostInstaller_Uninstall.log`)
  under the install directory, in addition to the NSIS built-in log
  produced by this repository's log-enabled `makensis.exe`
  (`tools/prepare-nsis.ps1`).

## Failure modes

| Situation | Behavior |
| --- | --- |
| Not run elevated | `.onInit` checks `UserInfo::GetAccountType`; if not `"Admin"`, shows a clear message and aborts before touching the filesystem. |
| Not 64-bit Windows | `.onInit` checks `${RunningX64}` and aborts with a clear message; this build only ships 64-bit host binaries. |
| `VBoxProxyStub.dll` / `VBoxC.dll` registration fails | `RegDLL` error is checked explicitly; the installer aborts rather than continuing with a Manager that cannot start. |
| `VBoxSVC.exe /RegServer` or `VBoxSDS.exe --regservice` fails | The installer aborts with the failing command and exit code, pointing at the log. |
| A kernel driver's `.inf` is missing from the payload | Logged and skipped; never reported as a driver failure (see **Known build limitation**). |
| A kernel driver is present but refused (expected: unsigned + Driver Signature Enforcement) | Logged in detail, summarized once at the end (non-silent only); installation is **not** aborted -- VirtualBox Manager still installs and runs. |
| VirtualBox is running at uninstall time | A non-blocking notice (via `nsProcess`) asks the user to close it first; uninstall proceeds regardless, and in-use files are scheduled for delete-on-reboot by the normal NSIS/Windows mechanism. |

## Building it

```powershell
# 1. Build the Windows host binaries first (unchanged, existing path):
tools\build-windows.ps1 -Mode Build

# 2. Compile this installer from that same staged payload:
tools\build-windows-nsis-installer.ps1
```

`tools\build-windows-nsis-installer.ps1`:

- Locates (or, given a configured `env.bat`, builds via
  `tools\prepare-nsis.ps1`) the custom log-enabled NSIS 3.10 package.
- Validates the staged payload contains the files this installer's COM/service
  registration depends on, and fails clearly, naming the missing files,
  rather than compiling an installer that would fail at install time.
- Reads the product version from `Version.kmk` and the vendor/product
  strings from `Config.kmk` (both read-only; neither file is modified),
  matching `tools/build-windows.ps1`'s own Squirrel packaging metadata.
- Compiles `VBoxHostInstaller.nsi` with `/DPAYLOAD_DIR=...`,
  `/DPRODUCT_VERSION=...`, `/DVBOX_VENDOR=...`, `/DVBOX_VENDOR_SHORT=...`,
  and `/DVBOX_PRODUCT=...` command-line defines.
- Verifies the produced file exists, is non-zero, matches the expected name
  pattern, and is unsigned (`Get-AuthenticodeSignature` reports
  `NotSigned` -- code signing is permanently disabled for this project),
  then writes a `SHA256SUMS.txt` beside it.

Both the `.nsi`/`.nsh` pair and the PowerShell build helper have been
compiled end-to-end against this repository's existing partial build output
as a syntax and logic check (catching and fixing two real bugs along the
way: an NSIS stack imbalance from `nsExec::ExecToStack` pushing two values
where only one was popped, and a nested-quote collision from re-embedding an
already-quoted macro parameter inside another double-quoted string -- fixed
by adopting this tree's own `$\"` escape convention throughout, matching
`src/VBox/Additions/win/Installer/VBoxGuestAdditionsExternal.nsh`). Neither
the resulting installer nor `tools\build-windows.ps1`'s full ~70 minute
build was executed as part of writing this installer.

## Wiring this into the release pipeline

This installer and its build script are intentionally **not** wired into
`.github/workflows/windows-package-release.yml`, `tools/build-windows.ps1`,
`configure.py`, or `Config.kmk` -- those files were out of scope for this
task. Switching the shipped installer over (or publishing both side by
side) requires:

1. Deciding whether the NSIS installer replaces or supplements the existing
   unsigned Squirrel.Windows path in `tools/build-windows.ps1` and the
   release workflow.
2. Dropping `--disable-win-ddk` (and confirming the WDK toolchain that
   script already bootstraps for the NSIS build prerequisite is also used
   for the actual driver compile) so the payload gains the host kernel
   driver `.sys`/`.inf` files this installer already knows how to probe for
   and install.
3. Adding a call to `tools\build-windows-nsis-installer.ps1` (and collecting
   its output artifact and `SHA256SUMS.txt`) alongside or instead of the
   existing `New-SquirrelInstaller` call.
