@echo off
rem $Id$
rem One-click unsigned NSIS host installer wrapper for VirtualBox. Builds the
rem single elevated VirtualBox-<version>-Setup.exe (host binaries, Guest
rem Additions ISO, and host kernel drivers where present) -- not a Squirrel
rem package, which this project retired because it cannot elevate or register
rem COM/services/drivers.
rem
rem Copyright (C) 2026 Oracle and/or its affiliates.
rem SPDX-License-Identifier: GPL-3.0-only

setlocal

set "silentArg="
if /I "%~1"=="/s" set "silentArg=-Silent"
if /I "%~1"=="--silent" set "silentArg=-Silent"

powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%~dp0tools\build-windows.ps1" -Mode Installer %silentArg%
set "exitCode=%ERRORLEVEL%"
endlocal & exit /b %exitCode%
