@echo off
rem $Id$
rem One-click unsigned Squirrel.Windows installer wrapper for VirtualBox.
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
