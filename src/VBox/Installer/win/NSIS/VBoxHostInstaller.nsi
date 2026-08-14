; $Id$
;; @file
; VBoxHostInstaller.nsi - the one and only Windows installer this project
; ships, for the VirtualBox Windows HOST product (VirtualBox Manager,
; VBoxSVC, VBoxSDS, the Guest Additions ISO, and the host kernel drivers).
; It replaced an unsigned Squirrel.Windows package this project shipped here
; until 2026-08-14; Squirrel is a per-user file unpacker with no elevation
; and could not register COM, install VBoxSDS, or install kernel drivers.
;
; This is deliberately NOT wired into the kBuild "packing" pipeline. It is a
; standalone script compiled directly against an already staged release
; payload by tools\build-windows-nsis-installer.ps1, which
; tools\build-windows.ps1 -Mode Installer and
; .github/workflows/windows-package-release.yml both invoke as their one
; installer-build step from out\win.amd64\release\bin. See doc\installer for
; the full write-up: what this installer does, what it deliberately cannot
; do without code signing, and how it was verified.
;

;
; Copyright (C) 2026 Oracle and/or its affiliates.
;
; This file is part of VirtualBox base platform packages, as
; available from https://www.virtualbox.org.
;
; This program is free software; you can redistribute it and/or
; modify it under the terms of the GNU General Public License
; as published by the Free Software Foundation, in version 3 of the
; License.
;
; This program is distributed in the hope that it will be useful, but
; WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
; General Public License for more details.
;
; You should have received a copy of the GNU General Public License
; along with this program; if not, see <https://www.gnu.org/licenses>.
;
; SPDX-License-Identifier: GPL-3.0-only
;

Unicode true

; ---------------------------------------------------------------------------
; Build inputs.
;
; Required command line defines (see tools\build-windows-nsis-installer.ps1):
;   /DPAYLOAD_DIR=<absolute path to the staged release bin directory>
;   /DPRODUCT_VERSION=<major.minor.build, e.g. 7.2.97, matching Version.kmk>
;
; Optional command line defines (sensible defaults match Version.kmk):
;   /DOUT_FILE=<absolute path of the installer .exe to produce>
;   /DVBOX_VENDOR=<full vendor name>
;   /DVBOX_VENDOR_SHORT=<short vendor name, used for the registry/ARP key>
;   /DVBOX_PRODUCT=<product display name>
; ---------------------------------------------------------------------------

!ifndef PAYLOAD_DIR
  !error "PAYLOAD_DIR must be defined on the command line, e.g. /DPAYLOAD_DIR=C:\...\out\win.amd64\release\bin"
!endif
!ifndef PRODUCT_VERSION
  !error "PRODUCT_VERSION must be defined on the command line, e.g. /DPRODUCT_VERSION=7.2.97"
!endif

!ifndef VBOX_VENDOR
  !define VBOX_VENDOR "Oracle and/or its affiliates"
!endif
!ifndef VBOX_VENDOR_SHORT
  !define VBOX_VENDOR_SHORT "Oracle"
!endif
!ifndef VBOX_PRODUCT
  !define VBOX_PRODUCT "${VBOX_VENDOR_SHORT} VirtualBox"
!endif
!ifndef OUT_FILE
  !define OUT_FILE "${__FILEDIR__}\VirtualBox-${PRODUCT_VERSION}-Setup.exe"
!endif

!define PRODUCT_NAME      "${VBOX_PRODUCT}"
!define PRODUCT_PUBLISHER "${VBOX_VENDOR}"
; VIProductVersion requires exactly four numeric parts; Version.kmk only
; carries major.minor.build, so pad with a fixed fourth component.
!define PRODUCT_VERSION_4 "${PRODUCT_VERSION}.0"
!define ARP_KEY           "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}"
!define REGISTRY_KEY_ROOT "Software\${VBOX_VENDOR_SHORT}\VirtualBox"

Name "${PRODUCT_NAME}"
OutFile "${OUT_FILE}"
InstallDir "$PROGRAMFILES64\VirtualBox"
InstallDirRegKey HKLM "${REGISTRY_KEY_ROOT}" "InstallDir"
RequestExecutionLevel admin
SetCompressor /SOLID lzma
ShowInstDetails show
ShowUnInstDetails show

VIProductVersion "${PRODUCT_VERSION_4}"
VIAddVersionKey "FileVersion"     "${PRODUCT_VERSION}"
VIAddVersionKey "ProductName"     "${PRODUCT_NAME}"
VIAddVersionKey "ProductVersion"  "${PRODUCT_VERSION}"
VIAddVersionKey "CompanyName"     "${PRODUCT_PUBLISHER}"
VIAddVersionKey "FileDescription" "${PRODUCT_NAME} installer (UNSIGNED build -- code signing is permanently disabled for this project)"
VIAddVersionKey "LegalCopyright"  "(C) ${VBOX_VENDOR}"

!include "LogicLib.nsh"
!include "FileFunc.nsh"
!insertmacro GetSize
!include "x64.nsh"
!include "nsProcess.nsh"

!include "VBoxHostInstallerCommon.nsh"

; ---------------------------------------------------------------------------
; Modern UI.
; ---------------------------------------------------------------------------
!include "MUI2.nsh"
!define MUI_ABORTWARNING

; Embed the same icon VirtualBox.exe itself carries (offline, no URL involved,
; unlike an <iconUrl> that would need a live reachable HTTPS endpoint) so the
; installer, its Start Menu/Desktop shortcuts, and the installed app agree.
; ${__FILEDIR__} is this script's own directory, five levels below the
; repository root -- same relative depth as the license file included below.
!define MUI_ICON   "${__FILEDIR__}\..\..\..\..\..\src\VBox\Artwork\win\OSE\VirtualBox_win.ico"
!define MUI_UNICON "${__FILEDIR__}\..\..\..\..\..\src\VBox\Artwork\win\OSE\VirtualBox_win.ico"

; Make the unsigned-build fact impossible to miss: it is stated on the very
; first page, not only in the log and the Finish-page driver summary.
!define MUI_WELCOMEPAGE_TITLE "Welcome to the ${PRODUCT_NAME} ${PRODUCT_VERSION} Setup Wizard"
!define MUI_WELCOMEPAGE_TEXT "This will install ${PRODUCT_NAME} ${PRODUCT_VERSION} on your computer.$\r$\n$\r$\nIMPORTANT: this build is UNSIGNED -- code signing is permanently disabled for this project. Windows will show an unknown-publisher warning, and any kernel driver this setup attempts to install (support, USB) will most likely be refused by Driver Signature Enforcement. VirtualBox Manager will still install and run; starting a virtual machine will not work until the affected driver(s) are permitted to load on this system. This setup will never change Driver Signature Enforcement, BCD test-signing, or any other system security setting.$\r$\n$\r$\nClick Next to continue."
!insertmacro MUI_PAGE_WELCOME
; This project's licensed base package text; ${__FILEDIR__} is this script's
; own directory, five levels below the repository root.
!insertmacro MUI_PAGE_LICENSE "${__FILEDIR__}\..\..\..\..\..\doc\License-gpl-3.0.rtf"
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!define MUI_FINISHPAGE_RUN "$INSTDIR\VirtualBox.exe"
!define MUI_FINISHPAGE_RUN_TEXT "Start ${PRODUCT_NAME} now"
!define MUI_FINISHPAGE_SHOWREADME "$INSTDIR\VBoxHostInstaller_Install.log"
!define MUI_FINISHPAGE_SHOWREADME_TEXT "View the installer log"
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_LANGUAGE "English"

; ---------------------------------------------------------------------------
; .onInit - elevation and platform checks.
; ---------------------------------------------------------------------------
Function .onInit
  ${IfNot} ${RunningX64}
    MessageBox MB_ICONSTOP "${PRODUCT_NAME} requires 64-bit Windows."
    Abort
  ${EndIf}
  SetRegView 64

  ; RequestExecutionLevel admin already asks Windows to elevate this process
  ; via UAC; verify it actually landed elevated before doing anything, and
  ; fail clearly rather than attempting a partial, silently-broken install.
  UserInfo::GetAccountType
  Pop $0
  ${If} $0 != "Admin"
    MessageBox MB_ICONSTOP "${PRODUCT_NAME} setup must be run with Administrator privileges.$\r$\n$\r$\nRight-click the installer and choose $\"Run as administrator$\", or, for a silent install, launch it from an elevated command prompt."
    Abort
  ${EndIf}

  StrCpy $G_DriverInstallFailed "0"
FunctionEnd

Function un.onInit
  SetRegView 64
  UserInfo::GetAccountType
  Pop $0
  ${If} $0 != "Admin"
    MessageBox MB_ICONSTOP "Uninstalling ${PRODUCT_NAME} requires Administrator privileges."
    Abort
  ${EndIf}
FunctionEnd

; ---------------------------------------------------------------------------
; Install.
; ---------------------------------------------------------------------------
; A single leading "-" section name: this installer has exactly one,
; mandatory, non-optional component, so it is never shown as a selectable
; item on a components page (which this installer does not use).
Section "-VirtualBox" SecMain
  SetShellVarContext all
  CreateDirectory "$INSTDIR"
  SetOutPath "$INSTDIR"

  StrCpy $G_LogFile "$INSTDIR\VBoxHostInstaller_Install.log"
  FileOpen $9 "$G_LogFile" w
  FileClose $9
  ${HostLog} "=== ${PRODUCT_NAME} ${PRODUCT_VERSION} installer started ==="
  ${HostLog} "Install directory: $INSTDIR"
  ${HostLog} "This build is UNSIGNED: code signing is permanently disabled for this project."

  ; ---- Payload -----------------------------------------------------------
  ${HostLog} "Copying program files from the staged release payload..."
  File /r "${PAYLOAD_DIR}\*.*"

  ; ---- COM registration and the VBoxSDS service --------------------------
  ; None of this requires a signed kernel driver, so it always runs and is
  ; required for a working VirtualBox Manager. Without it, VirtualBox.exe
  ; fails to start with "Failed to acquire the VirtualBox COM object" /
  ; REGDB_E_CLASSNOTREG, because VBoxProxyStub.dll, VBoxC.dll, VBoxSVC.exe's
  ; own COM registration, and the VBoxSDS Windows service are all
  ; unregistered on a bare file copy (which is all Squirrel.Windows, a
  ; per-user unpacker with no elevation, is able to do).
  ; Register through regsvr32 rather than NSIS's RegDLL.  RegDLL calls plain
  ; LoadLibrary, which resolves a DLL's own dependencies from the process
  ; directory, System32 and PATH -- but NOT from the directory the DLL itself
  ; lives in.  VBoxProxyStub.dll imports VBoxRT.dll, which only exists beside
  ; it in $INSTDIR, so RegDLL fails with ERROR_MOD_NOT_FOUND (126) even though
  ; nothing is actually missing.  Verified on a real install: LoadLibraryExW
  ; with LOAD_WITH_ALTERED_SEARCH_PATH loads all three DLLs cleanly while plain
  ; LoadLibraryW fails on every one.  regsvr32 uses the altered search path, so
  ; it resolves siblings correctly.
  ${HostLog} "Registering COM in-process servers (VBoxProxyStub.dll, VBoxC.dll)..."
  ${HostExecAbort} "$\"$SYSDIR\regsvr32.exe$\" /s $\"$INSTDIR\VBoxProxyStub.dll$\""
  ${HostExecAbort} "$\"$SYSDIR\regsvr32.exe$\" /s $\"$INSTDIR\VBoxC.dll$\""

  ${HostLog} "Registering the VBoxSVC out-of-process COM server (VBoxSVC.exe /RegServer)..."
  ${HostExecAbort} "$\"$INSTDIR\VBoxSVC.exe$\" /RegServer"

  ${HostLog} "Registering the VBoxSDS COM class and installing the VBoxSDS Windows service (VBoxSDS.exe --regservice)..."
  ${HostExecAbort} "$\"$INSTDIR\VBoxSDS.exe$\" --regservice"
  ${HostLog} "The VBoxSDS service is installed demand-start (SERVICE_DEMAND_START), depending on RPCSS; Windows starts it automatically the first time a client activates the VBoxSDS COM class -- it does not need to be started here."

  ; ---- Kernel drivers: best effort, never fatal, always honestly reported.
  ; See doc\installer\WindowsHostInstallerNSIS.md for exactly which drivers
  ; are attempted, why VBoxNetAdp6/VBoxNetLwf are deliberately NOT attempted
  ; here, and why an unsigned build is expected to have every one of these
  ; refused by Driver Signature Enforcement on a default 64-bit Windows
  ; system.
  ${HostLog} "Attempting kernel driver installation (best effort)..."
  ${AttemptDriverInstall} "VBoxSup" "" \
      "VBoxSup.inf" "drivers\VBoxSup\VBoxSup.inf" "VBoxSup\VBoxSup.inf" "drivers\VBoxSup.inf"
  ${AttemptDriverInstall} "VBoxUSBMon" "" \
      "VBoxUSBMon.inf" "drivers\USBMon\VBoxUSBMon.inf" "drivers\VBoxUSBMon\VBoxUSBMon.inf" "VBoxUSBMon\VBoxUSBMon.inf"
  ${AttemptDriverInstall} "VBoxUSB" "USB\VID_80EE&PID_CAFE" \
      "VBoxUSB.inf" "drivers\USB\VBoxUSB.inf" "drivers\VBoxUSB\VBoxUSB.inf" "VBoxUSB\VBoxUSB.inf"
  ${HostLog} "VBoxNetAdp6 (host-only networking) and VBoxNetLwf (bridged networking) are network components that must be registered through the Windows Network Configuration (INetCfg) API, which this installer does not implement; they are not attempted here. See doc\installer\WindowsHostInstallerNSIS.md."

  ; ---- Start Menu / Desktop shortcuts -------------------------------------
  CreateDirectory "$SMPROGRAMS\${PRODUCT_NAME}"
  CreateShortcut "$SMPROGRAMS\${PRODUCT_NAME}\${PRODUCT_NAME}.lnk" "$INSTDIR\VirtualBox.exe"
  CreateShortcut "$SMPROGRAMS\${PRODUCT_NAME}\Uninstall ${PRODUCT_NAME}.lnk" "$INSTDIR\uninstall.exe"
  CreateShortcut "$DESKTOP\${PRODUCT_NAME}.lnk" "$INSTDIR\VirtualBox.exe"

  ; ---- Registry ------------------------------------------------------------
  WriteRegStr HKLM "${REGISTRY_KEY_ROOT}" "InstallDir" "$INSTDIR"
  WriteRegStr HKLM "${REGISTRY_KEY_ROOT}" "Version"    "${PRODUCT_VERSION}"

  ; ---- Uninstaller and Add/Remove Programs metadata -------------------------
  WriteUninstaller "$INSTDIR\uninstall.exe"
  WriteRegStr   HKLM "${ARP_KEY}" "DisplayName"          "${PRODUCT_NAME}"
  WriteRegStr   HKLM "${ARP_KEY}" "DisplayVersion"        "${PRODUCT_VERSION}"
  WriteRegStr   HKLM "${ARP_KEY}" "Publisher"             "${PRODUCT_PUBLISHER}"
  WriteRegStr   HKLM "${ARP_KEY}" "InstallLocation"       "$INSTDIR"
  WriteRegStr   HKLM "${ARP_KEY}" "DisplayIcon"           "$INSTDIR\VirtualBox.exe"
  WriteRegStr   HKLM "${ARP_KEY}" "UninstallString"       '"$INSTDIR\uninstall.exe"'
  WriteRegStr   HKLM "${ARP_KEY}" "QuietUninstallString"  '"$INSTDIR\uninstall.exe" /S'
  WriteRegDWORD HKLM "${ARP_KEY}" "NoModify" 1
  WriteRegDWORD HKLM "${ARP_KEY}" "NoRepair" 1
  ${GetSize} "$INSTDIR" "/S=0K" $0 $1 $2
  WriteRegDWORD HKLM "${ARP_KEY}" "EstimatedSize" "$0"

  ${HostLog} "=== Installation finished. Any kernel driver refused: $G_DriverInstallFailed ==="

  ${If} $G_DriverInstallFailed == "1"
  ${AndIfNot} ${Silent}
    MessageBox MB_ICONEXCLAMATION|MB_OK "${PRODUCT_NAME} was installed and VirtualBox Manager is ready to use.$\r$\n$\r$\nOne or more kernel drivers could not be installed because this build is unsigned and Windows Driver Signature Enforcement refused to load them. Starting or running virtual machines will not work until the affected driver(s) are permitted to load on this system.$\r$\n$\r$\nSee $\"$G_LogFile$\" for the exact reason. This installer does not and will not change Driver Signature Enforcement, BCD test-signing, or any other system security setting."
  ${EndIf}
SectionEnd

; ---------------------------------------------------------------------------
; Uninstall.
; ---------------------------------------------------------------------------
Section "Uninstall"
  SetShellVarContext all
  StrCpy $G_LogFile "$INSTDIR\VBoxHostInstaller_Uninstall.log"
  FileOpen $9 "$G_LogFile" w
  FileClose $9
  ${HostLog} "=== ${PRODUCT_NAME} uninstaller started ==="

  ; Warn (but do not force-kill) if VirtualBox or its services look to be
  ; running; an in-use file simply gets scheduled for delete-on-reboot below.
  nsProcess::_FindProcess "VirtualBox.exe"
  Pop $0
  ${If} $0 == 0
    ${HostLog} "VirtualBox.exe appears to be running."
    ${IfNot} ${Silent}
      MessageBox MB_OK|MB_ICONEXCLAMATION "Please close VirtualBox Manager and any running virtual machines before continuing. Uninstall will proceed regardless, but files still in use will be removed on next reboot."
    ${EndIf}
  ${EndIf}
  nsProcess::_Unload

  ; ---- Kernel drivers: best effort, non-fatal, run before files are deleted.
  ${AttemptDriverUninstall} "VBoxSup" \
      "VBoxSup.inf" "drivers\VBoxSup\VBoxSup.inf" "VBoxSup\VBoxSup.inf" "drivers\VBoxSup.inf"
  ${AttemptDriverUninstall} "VBoxUSBMon" \
      "VBoxUSBMon.inf" "drivers\USBMon\VBoxUSBMon.inf" "drivers\VBoxUSBMon\VBoxUSBMon.inf" "VBoxUSBMon\VBoxUSBMon.inf"
  ${AttemptDriverUninstall} "VBoxUSB" \
      "VBoxUSB.inf" "drivers\USB\VBoxUSB.inf" "drivers\VBoxUSB\VBoxUSB.inf" "VBoxUSB\VBoxUSB.inf"

  ; ---- COM / service unregistration ----------------------------------------
  ${If} ${FileExists} "$INSTDIR\VBoxSDS.exe"
    ${HostExecLogOnly} "Unregister VBoxSDS service/COM class" "$\"$INSTDIR\VBoxSDS.exe$\" --unregservice"
  ${EndIf}
  ${If} ${FileExists} "$INSTDIR\VBoxSVC.exe"
    ${HostExecLogOnly} "Unregister VBoxSVC COM server" "$\"$INSTDIR\VBoxSVC.exe$\" /UnregServer"
  ${EndIf}
  ; regsvr32 /u for the same reason RegDLL is not used above: UnRegDLL calls
  ; plain LoadLibrary and cannot resolve VBoxRT.dll sitting beside these two.
  ${If} ${FileExists} "$INSTDIR\VBoxC.dll"
    ${HostExecLogOnly} "Unregister VBoxC.dll" "$\"$SYSDIR\regsvr32.exe$\" /s /u $\"$INSTDIR\VBoxC.dll$\""
  ${EndIf}
  ${If} ${FileExists} "$INSTDIR\VBoxProxyStub.dll"
    ${HostExecLogOnly} "Unregister VBoxProxyStub.dll" "$\"$SYSDIR\regsvr32.exe$\" /s /u $\"$INSTDIR\VBoxProxyStub.dll$\""
  ${EndIf}

  ; ---- Shortcuts and registry -----------------------------------------------
  Delete "$SMPROGRAMS\${PRODUCT_NAME}\${PRODUCT_NAME}.lnk"
  Delete "$SMPROGRAMS\${PRODUCT_NAME}\Uninstall ${PRODUCT_NAME}.lnk"
  RMDir  "$SMPROGRAMS\${PRODUCT_NAME}"
  Delete "$DESKTOP\${PRODUCT_NAME}.lnk"
  DeleteRegKey HKLM "${ARP_KEY}"
  DeleteRegKey HKLM "${REGISTRY_KEY_ROOT}"

  ; ---- Files -----------------------------------------------------------------
  ${HostLog} "Removing installed files from $INSTDIR..."
  Delete "$INSTDIR\uninstall.exe"
  RMDir /r "$INSTDIR"
SectionEnd
