# Windows installer

This category documents the Windows packaging for the VirtualBox host product:
how the release payload becomes an installable package, what each installer
does and does not do, and the constraints that shape both (most importantly,
this project's permanent no-code-signing policy).

## Articles

- [`WindowsHostInstallerNSIS.md`](WindowsHostInstallerNSIS.md) documents the
  NSIS-based Windows host installer
  (`src/VBox/Installer/win/NSIS/VBoxHostInstaller.nsi`): what it installs and
  registers, exactly which kernel drivers it attempts to install and why two
  of them are deliberately left out, how it detects and reports a driver
  refused by Driver Signature Enforcement, its failure modes, silent
  installation, and uninstall behavior.

## Related packaging

- The existing unsigned Squirrel.Windows packaging
  (`tools/build-windows.ps1`) remains the currently wired release path; it is
  a per-user file unpacker with no elevation and therefore cannot register
  COM classes, install the `VBoxSDS` Windows service, or install kernel
  drivers. See `WindowsHostInstallerNSIS.md` for the exact failure this
  produces (`REGDB_E_CLASSNOTREG`) and why an elevated installer is required
  to fix it.
- This project's permanent prohibition on code signing (see the repository's
  shared build/release instructions) is why every kernel driver installation
  attempt documented here is expected to be refused by Windows Driver
  Signature Enforcement on a default 64-bit Windows system, and why no
  installer in this repository will ever attempt to change that system
  setting.
