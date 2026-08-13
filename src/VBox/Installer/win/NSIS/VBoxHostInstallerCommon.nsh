; $Id$
;; @file
; VBoxHostInstallerCommon.nsh - Shared macros for the VirtualBox Windows host
; NSIS installer (logging, COM/service registration helpers, and best-effort,
; honestly-reported kernel driver installation).
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

; This file intentionally has no VBoxGuestAdditions* counterpart to include:
; it is a standalone helper set for the host product installer only, modelled
; on the logging and command-execution conventions used by
; src/VBox/Additions/win/Installer/VBoxGuestAdditionsLog.nsh and
; VBoxGuestAdditionsExternal.nsh, but kept self-contained so this installer
; does not depend on the Guest Additions installer sources.

!ifndef VBOXHOSTINSTALLERCOMMON_INCLUDED
!define VBOXHOSTINSTALLERCOMMON_INCLUDED

Var G_LogFile              ; Full path of the current run's plain text log file.
Var G_DriverInstallFailed  ; "1" once any kernel driver installation attempt was refused.

;;
; Writes a line to the NSIS build-in log (when compiled with a log-enabled
; makensis, as this project's tools\prepare-nsis.ps1 produces), to the
; installer UI details list, and -- once $G_LogFile has been set by the
; calling section -- to a plain text file under the install directory so the
; user can review exactly what happened after the installer window closes.
;
; @param   Text to log.
;
!macro _hostLog text
  Push $9
  LogText "${text}"
  IfSilent +2
    DetailPrint "${text}"
  ${If} $G_LogFile != ""
    FileOpen $9 "$G_LogFile" a
    ${If} $9 != ""
      FileSeek $9 0 END
      FileWrite $9 "${text}$\r$\n"
      FileClose $9
    ${EndIf}
  ${EndIf}
  Pop $9
!macroend
!define HostLog "!insertmacro _hostLog"

;;
; Runs a command line that is REQUIRED for a working VirtualBox Manager
; (COM registration, service installation). Logs the outcome and aborts the
; installation with a clear message if the command fails, mirroring the
; 'non-zero-exitcode=abort' behaviour of the Guest Additions installer's
; ${CmdExecute} macro.
;
; @param   cmdline  Command line (fully qualified and quoted). MUST use the
;                    $\" escape for every embedded double quote (never a bare
;                    "), exactly like every ${CmdExecute} call in
;                    src/VBox/Additions/win/Installer/VBoxGuestAdditionsW2KXP.nsh
;                    does -- this macro parameter is textually re-embedded
;                    inside several further double-quoted strings ($G_LogFile
;                    messages, the Abort text) as it is logged, and a bare "
;                    in its value would prematurely close those and split
;                    what should be one macro argument into several.
;
!macro _hostExecAbort cmdline
  Push $0
  Push $1
  ${HostLog} "Executing: ${cmdline}"
  ${If} ${Silent}
    nsExec::ExecToStack "${cmdline}"
    Pop $0
    Pop $1
  ${Else}
    nsExec::ExecToLog "${cmdline}"
    Pop $0
  ${EndIf}
  ${If} $0 != 0
    ${HostLog} "ERROR: command failed with exit code $0: ${cmdline}"
    Abort "A required setup step failed (exit code $0):$\r$\n${cmdline}$\r$\n$\r$\nSee $\"$G_LogFile$\" for details."
  ${Else}
    ${HostLog} "Command succeeded (exit code 0)."
  ${EndIf}
  Pop $1
  Pop $0
!macroend
!define HostExecAbort "!insertmacro _hostExecAbort"

;;
; Runs a command line that is expected to sometimes fail (kernel driver
; unregistration during uninstall) and only logs the outcome; never aborts.
;
; @param   description  Description for the log.
; @param   cmdline      Command line. Same $\" escaping requirement as
;                        ${HostExecAbort} above, and for the same reason.
;
!macro _hostExecLogOnly description cmdline
  Push $0
  ${HostLog} "${description}: ${cmdline}"
  nsExec::ExecToLog "${cmdline}"
  Pop $0
  ${HostLog} "${description} exit code: $0"
  Pop $0
!macroend
!define HostExecLogOnly "!insertmacro _hostExecLogOnly"

;;
; Locates a driver .inf file below $INSTDIR by probing a bounded set of
; candidate relative paths and stores the first match (or an empty string
; when none exist) in the output variable.
;
; The exact sub-directory a future full build stages host kernel drivers
; into (out\win.amd64\release\bin\...) has not been verified against this
; repository's current build pipeline -- see doc\installer for why -- so
; this probes the plausible layouts instead of assuming one.
;
; @param   outVar      Variable (e.g. $2) that receives the found path, or "".
; @param   candidate1..4  Up to four candidate paths, relative to $INSTDIR.
;                         Pass "" for an unused slot.
;
!macro _findDriverInf outVar candidate1 candidate2 candidate3 candidate4
  StrCpy ${outVar} ""
  ${If} '${candidate1}' != ''
  ${AndIf} ${FileExists} "$INSTDIR\${candidate1}"
    StrCpy ${outVar} "$INSTDIR\${candidate1}"
  ${ElseIf} '${candidate2}' != ''
  ${AndIf} ${FileExists} "$INSTDIR\${candidate2}"
    StrCpy ${outVar} "$INSTDIR\${candidate2}"
  ${ElseIf} '${candidate3}' != ''
  ${AndIf} ${FileExists} "$INSTDIR\${candidate3}"
    StrCpy ${outVar} "$INSTDIR\${candidate3}"
  ${ElseIf} '${candidate4}' != ''
  ${AndIf} ${FileExists} "$INSTDIR\${candidate4}"
    StrCpy ${outVar} "$INSTDIR\${candidate4}"
  ${EndIf}
!macroend
!define FindDriverInf "!insertmacro _findDriverInf"

;;
; Best-effort, HONEST kernel driver installation via the tree's own
; VBoxDrvInst.exe helper (the same tool and command pattern the Guest
; Additions NSIS installer uses -- see
; src/VBox/Additions/win/Installer/VBoxGuestAdditionsW2KXP.nsh).
;
; This NEVER aborts the installation and NEVER claims success it cannot
; prove: it inspects VBoxDrvInst.exe's own exit code and reports plainly
; when the driver is (a) simply not present in this build's payload, or
; (b) present but refused by Windows -- which, since this project never
; code-signs its drivers, is the expected outcome on a default 64-bit
; Windows installation with Driver Signature Enforcement active. It never
; touches Driver Signature Enforcement, BCD test-signing, or any other
; system security setting.
;
; @param   friendlyName  Short name used in log messages and the per-driver log file.
; @param   pnpId         Optional hardware/root ID to pass as --pnp-id (e.g. for a
;                         PnP-class driver being staged ahead of device arrival).
;                         Pass "" when the driver's INF installs via its
;                         [DefaultInstall] section without one.
; @param   candidate1..4  Up to four candidate relative paths (see ${FindDriverInf}).
;
!macro _driverInstallAttempt friendlyName pnpId candidate1 candidate2 candidate3 candidate4
  Push $0
  Push $1
  Push $2
  Push $3
  ${FindDriverInf} $2 "${candidate1}" "${candidate2}" "${candidate3}" "${candidate4}"
  ${If} $2 != ""
    ${HostLog} "Attempting kernel driver install: ${friendlyName} ($2)"
    StrCpy $3 '"$INSTDIR\VBoxDrvInst.exe" --logfile "$INSTDIR\VBoxDrvInst_${friendlyName}.log" install --inf-file "$2" --ignore-reboot'
    ${If} '${pnpId}' != ''
      StrCpy $3 '$3 --pnp-id "${pnpId}"'
    ${EndIf}
    ${HostLog} "Executing: $3"
    ${If} ${Silent}
      ; nsExec::ExecToStack pushes TWO values: the exit code, then the
      ; captured stdout/stderr text. Both must be popped or every later
      ; Pop in this script silently reads stale data off the user stack.
      nsExec::ExecToStack '$3'
      Pop $0
      Pop $1
    ${Else}
      nsExec::ExecToLog '$3'
      Pop $0
    ${EndIf}
    ; VBoxDrvInst.exe (see src/VBox/GuestHost/installation/VBoxDrvInst.cpp)
    ; returns 0 on success, 4 on success-but-reboot-needed
    ; (VBOXDRVINSTEXITCODE_REBOOT_NEEDED = RTEXITCODE_END), 5 on
    ; success-with-warning (VBOXDRVINSTEXITCODE_WARNING), and any other
    ; non-zero value on failure.
    ${If} $0 == 0
      ${HostLog} "Driver installed: ${friendlyName} (exit code 0)."
    ${ElseIf} $0 == 4
      ${HostLog} "Driver installed but a reboot is required to finish: ${friendlyName}."
      SetRebootFlag true
    ${ElseIf} $0 == 5
      ${HostLog} "Driver installed with a warning: ${friendlyName}. See $INSTDIR\VBoxDrvInst_${friendlyName}.log."
    ${Else}
      ${HostLog} "WARNING: driver installation for ${friendlyName} was refused (VBoxDrvInst.exe exit code $0). This build of VirtualBox is UNSIGNED (code signing is permanently disabled for this project); on a default 64-bit Windows system, Driver Signature Enforcement refuses to load an unsigned kernel driver, which is the expected cause here. VirtualBox Manager will still install and run, but starting a virtual machine will fail until ${friendlyName} is permitted to load. See $INSTDIR\VBoxDrvInst_${friendlyName}.log for the exact reason reported by Windows. This installer does not and will not modify Driver Signature Enforcement, BCD test-signing, or any other system security setting."
      StrCpy $G_DriverInstallFailed "1"
    ${EndIf}
  ${Else}
    ${HostLog} "Skipped driver ${friendlyName}: its .inf file was not found under the staged payload (probed: ${candidate1}, ${candidate2}, ${candidate3}, ${candidate4}). This build's payload does not include this host kernel driver -- see doc\installer\WindowsHostInstallerNSIS.md."
  ${EndIf}
  Pop $3
  Pop $2
  Pop $1
  Pop $0
!macroend
!define AttemptDriverInstall "!insertmacro _driverInstallAttempt"

;;
; Best-effort, non-fatal kernel driver uninstallation counterpart to
; ${AttemptDriverInstall}. Always logs the outcome; never aborts the
; uninstaller, since a driver that could never be installed (unsigned,
; refused by DSE) obviously cannot be "uninstalled" either, and that must
; not be treated as an uninstaller failure.
;
; @param   friendlyName  Short name used in log messages.
; @param   candidate1..4  Up to four candidate relative paths (see ${FindDriverInf}).
;
!macro _driverUninstallAttempt friendlyName candidate1 candidate2 candidate3 candidate4
  Push $0
  Push $2
  Push $3
  ${FindDriverInf} $2 "${candidate1}" "${candidate2}" "${candidate3}" "${candidate4}"
  ${If} $2 != ""
    ${HostLog} "Attempting kernel driver uninstall: ${friendlyName} ($2)"
    StrCpy $3 '"$INSTDIR\VBoxDrvInst.exe" --logfile "$TEMP\VBoxDrvInst_Uninstall_${friendlyName}.log" uninstall --inf-file "$2" --ignore-reboot'
    nsExec::ExecToLog '$3'
    Pop $0
    ${HostLog} "Driver uninstall for ${friendlyName} exit code: $0 (a non-zero code here is expected when the driver was never actually loaded, e.g. because it was refused as unsigned at install time)."
  ${Else}
    ${HostLog} "Skipped driver uninstall for ${friendlyName}: its .inf file is no longer present under $INSTDIR."
  ${EndIf}
  Pop $3
  Pop $2
  Pop $0
!macroend
!define AttemptDriverUninstall "!insertmacro _driverUninstallAttempt"

!endif ; !VBOXHOSTINSTALLERCOMMON_INCLUDED
