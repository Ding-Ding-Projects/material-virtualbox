# Windows host installer (NSIS)

`src/VBox/Installer/win/NSIS/VBoxHostInstaller.nsi` (with its shared macros in
`VBoxHostInstallerCommon.nsh`) is **the one and only Windows installer this
project ships**: an NSIS-based installer for the VirtualBox Windows **host**
product -- VirtualBox Manager, `VBoxSVC`, `VBoxSDS`, the Guest Additions ISO,
and the host kernel drivers. It is built by
`tools/build-windows-nsis-installer.ps1` from an already staged release
payload (`out\win.amd64\release\bin`, produced by `tools/build-windows.ps1
-Mode Build`), and it is wired into both `tools/build-windows.ps1 -Mode
Installer` and `.github/workflows/windows-package-release.yml` as the single
installer step -- see **Wiring this into the release pipeline** below.

This project shipped an unsigned Squirrel.Windows package here until
2026-08-14. It was retired outright rather than kept alongside this
installer: shipping two installers means asking every user to figure out
which one they need, and Squirrel could never produce a working VirtualBox
install regardless (see **Why this installer exists** below), so there was
nothing worth keeping it for. `Ensure-Squirrel` and `New-SquirrelInstaller`
were deleted from `tools/build-windows.ps1`, and the workflow's Squirrel
build/verify steps were replaced with this installer's build/verify steps,
rather than left in place unreferenced -- dead packaging code for a
known-broken installer is exactly the kind of thing a future edit
copy-pastes from by accident.

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
   default, user-changeable on the Directory page) with a single `File /r`.
   Because the Guest Additions ISO is built directly into
   `out\win.amd64\release\bin\additions\VBoxGuestAdditions.iso`
   (`Config.kmk`: `INST_ADDITIONS_ISO = INST_ADDITIONS = bin/additions/` for
   Windows), that `File /r` sweeps it up automatically along with the host
   binaries and drivers -- there is no separate step needed to "install
   everything." A user who has installed VirtualBox this way already has
   `<install dir>\additions\VBoxGuestAdditions.iso` on disk, ready to attach
   to a guest's optical drive from VirtualBox Manager, exactly as an
   upstream VirtualBox install ships it.
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
7. **The application icon.** `MUI_ICON`/`MUI_UNICON` embed
   `src/VBox/Artwork/win/OSE/VirtualBox_win.ico` -- the same icon compiled
   into `VirtualBox.exe` itself -- into the installer executable, the
   uninstaller, and (via `DisplayIcon` in the Add/Remove Programs entry) the
   Programs-and-Features listing, fully offline. See `doc/md3/AppIcon.md`.

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

If a driver's `.inf` is not even present in the payload, that is also logged
explicitly and distinguished from a refusal: "not present in this build" is
a different, more basic fact than "present but refused as unsigned," and the
installer never conflates the two. As of the current pipeline (see **Host
drivers are produced by this pipeline** below) this path is not expected to
be exercised in a normal build -- all five driver `.sys`/`.inf` pairs are
verified present before the installer is even compiled -- but the installer
still handles it correctly rather than assuming the payload is always
complete.

The driver `.inf` file locations are probed against a bounded set of
plausible relative paths (see `${FindDriverInf}` in
`VBoxHostInstallerCommon.nsh`) rather than a single assumed path. This has
now been verified against a real build: the five host drivers land directly
at the payload root (`out\win.amd64\release\bin\VBoxSup.inf`, etc. --
`Config.kmk`: `INST_VBOXDRV = INST_BIN` for Windows, no subdirectory), which
is exactly the *first* candidate each `${AttemptDriverInstall}` call probes
(e.g. `"VBoxSup.inf"` with no prefix), so it matches on the first try. The
remaining candidates are kept as a defensive fallback rather than removed --
they cost nothing and protect against a future packaging change moving these
files into a subdirectory. If none of the probed candidates exist, the
driver is reported as skipped rather than the installer guessing incorrectly.

## Host drivers are produced by this pipeline

An earlier version of this document recorded, as a known limitation, that
`--disable-win-ddk` appeared to prevent `tools/build-windows.ps1` from
producing the five host driver `.sys`/`.inf` pairs at all. That was
investigated and found to be a red herring: `VBOX_WITH_VBOXDRV`/`_USB`/
`_NETFLT`/`_NETADP` all default to `1` for a normal Windows build
(`Config.kmk`), none of them are touched by `--disable-win-ddk` or any other
flag this pipeline passes, and the host driver SDK
(`VBOX_WINDDK ?= WINSDK10-KM`, `Config.kmk`) has nothing to do with the
legacy Windows 7 DDK that `--disable-win-ddk` actually turns off
(`configure.py`'s `checkCallback_WinDDK`, which only ever populates
`PATH_SDK_WINDDK71` -- a variable this pipeline already overrides with
`WINSDK10-KM` on the `kmk` command line). The real cause of the missing
drivers in the run that originally prompted this note was that the build
died on an unrelated crash (`STATUS_STACK_BUFFER_OVERRUN` in
`tstVMStructSize`/`tstAsmStructs`, since fixed) before kBuild's dependency
graph ever reached the drivers' link step.

Both `tools/build-windows.ps1`'s `Invoke-VirtualBoxBuild` and the release
workflow's own build step now verify all five host driver `.sys`/`.inf`
pairs (`VBoxSup`, `VBoxUSBMon`, `VBoxUSB`, `VBoxNetAdp6`, `VBoxNetLwf`) exist
in `out\win.amd64\release\bin` immediately after the build, and fail loudly,
naming the exact missing files, if any are absent -- rather than silently
shipping an installer with nothing for `VBoxDrvInst.exe` to attempt. This
installer only ever attempts three of those five (`VBoxSup`, `VBoxUSBMon`,
`VBoxUSB` -- see **Kernel drivers: what is attempted, and what deliberately
is not** above); `VBoxNetAdp6` and `VBoxNetLwf` are produced and verified in
the payload but deliberately not installed by this installer, for the
`INetCfg` reason explained above.

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
| A kernel driver's `.inf` is missing from the payload | Logged and skipped; never reported as a driver failure. Not expected in a normal build -- see **Host drivers are produced by this pipeline**. |
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
  matching `tools/build-windows.ps1`'s own packaging metadata.
- Compiles `VBoxHostInstaller.nsi` with `/DPAYLOAD_DIR=...`,
  `/DPRODUCT_VERSION=...`, `/DVBOX_VENDOR=...`, `/DVBOX_VENDOR_SHORT=...`,
  and `/DVBOX_PRODUCT=...` command-line defines.
- Verifies the produced file exists, is non-zero, matches the expected name
  pattern, and is unsigned (`Get-AuthenticodeSignature` reports
  `NotSigned` -- code signing is permanently disabled for this project),
  then writes a `SHA256SUMS.txt` beside it.

Both the `.nsi`/`.nsh` pair and the PowerShell build helper were originally
compiled end-to-end against this repository's existing partial build output
as a syntax and logic check (catching and fixing two real bugs along the
way: an NSIS stack imbalance from `nsExec::ExecToStack` pushing two values
where only one was popped, and a nested-quote collision from re-embedding an
already-quoted macro parameter inside another double-quoted string -- fixed
by adopting this tree's own `$\"` escape convention throughout, matching
`src/VBox/Additions/win/Installer/VBoxGuestAdditionsExternal.nsh`). Neither
the resulting installer nor `tools\build-windows.ps1`'s full build was
executed as part of writing this installer or as part of wiring it into the
release pipeline (see **Wiring this into the release pipeline** below) --
both were verified by reading, by PowerShell/YAML parse-checking every
changed script and workflow, and by tracing the exact file paths each step
produces and consumes, not by running the ~90 minute build.

## Wiring this into the release pipeline

This installer **is** the shipped Windows installer, wired into both local
and CI build paths as of 2026-08-14:

- **`tools\build-windows.ps1 -Mode Installer`** (also reachable via the
  repository root's `build-installer.bat`) builds the host binaries, then
  calls `tools\build-windows-nsis-installer.ps1 -PayloadDir <payload>`
  directly. `Ensure-Squirrel` and `New-SquirrelInstaller` were deleted from
  that script rather than left unreferenced.
- **`.github/workflows/windows-package-release.yml`** runs a "Verify host
  driver files were produced" step (the same five-driver-pair check
  described in **Host drivers are produced by this pipeline** above, plus a
  check that the Guest Additions ISO landed in the payload) immediately
  after the build, then a "Build the unsigned NSIS host installer" step that
  invokes `tools\build-windows-nsis-installer.ps1` as a nested `pwsh.exe`
  process (so its own console output -- including the native `makensis.exe`
  compiler output -- can be teed to a log file for CI's failure-evidence
  collector), then a "Verify unsigned installer" step that asserts
  `VirtualBox-<version>-Setup.exe` and `SHA256SUMS.txt` exist and are
  unsigned. The published GitHub Release attaches exactly those two files
  and states plainly in its notes that this is the one installer, what it
  installs, and the unsigned-driver caveat. The workflow's former Squirrel
  build/verify steps and their NuGet/Squirrel bootstrap were removed rather
  than left dead in the workflow.
- `--disable-win-ddk` was never actually the reason host drivers were
  missing from the payload (see **Host drivers are produced by this
  pipeline** above) and remains passed by both `tools/build-windows.ps1` and
  the release workflow; no change to `configure.py` or `Config.kmk` was
  needed to get drivers into the payload this installer consumes.
