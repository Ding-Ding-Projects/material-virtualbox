# Windows installer

This category documents the Windows packaging for the VirtualBox host product:
how the release payload becomes an installable package, what each installer
does and does not do, and the constraints that shape both (most importantly,
this project's permanent no-code-signing policy).

## Articles

- [`WindowsHostInstallerNSIS.md`](WindowsHostInstallerNSIS.md) documents the
  NSIS-based Windows host installer
  (`src/VBox/Installer/win/NSIS/VBoxHostInstaller.nsi`) -- **the one and
  only Windows installer this project ships**: what it installs and
  registers (including the Guest Additions ISO, swept up automatically as
  part of the staged payload), exactly which kernel drivers it attempts to
  install and why two of them are deliberately left out, how it detects and
  reports a driver refused by Driver Signature Enforcement, its failure
  modes, silent installation, uninstall behavior, and exactly how it is
  wired into `tools/build-windows.ps1` and
  `.github/workflows/windows-package-release.yml`.

## Related packaging

- This project shipped an unsigned Squirrel.Windows package here until
  2026-08-14. Squirrel is a per-user file unpacker with no elevation and
  therefore could not register COM classes, install the `VBoxSDS` Windows
  service, or install kernel drivers -- see `WindowsHostInstallerNSIS.md`
  for the exact failure this produced (`REGDB_E_CLASSNOTREG`) and why an
  elevated installer replaced it outright rather than shipping alongside it.
- This project's permanent prohibition on code signing (see the repository's
  shared build/release instructions) is why every kernel driver installation
  attempt documented here is expected to be refused by Windows Driver
  Signature Enforcement on a default 64-bit Windows system, and why no
  installer in this repository will ever attempt to change that system
  setting.
