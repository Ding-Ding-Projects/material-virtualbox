@echo off
setlocal EnableExtensions DisableDelayedExpansion

set "VBOX_BUILD_SILENT="
if /I "%SILENT%"=="1" set "VBOX_BUILD_SILENT=1"

:parse_arguments
if "%~1"=="" goto run_build
if /I "%~1"=="/s" (
    set "VBOX_BUILD_SILENT=1"
    shift
    goto parse_arguments
)
if /I "%~1"=="--silent" (
    set "VBOX_BUILD_SILENT=1"
    shift
    goto parse_arguments
)
echo [ERROR] Unknown argument: %~1
echo [ERROR] Supported silent flags are /s, --silent, or SILENT=1.
exit /b 2

:run_build
set "VBOX_BUILD_SCRIPT=%~dp0tools\md3\build-windows.ps1"
if not exist "%VBOX_BUILD_SCRIPT%" (
    echo [ERROR] Native build script is missing: %VBOX_BUILD_SCRIPT%
    exit /b 3
)

if defined VBOX_BUILD_SILENT (
    powershell.exe -NoLogo -NoProfile -NonInteractive -ExecutionPolicy Bypass -File "%VBOX_BUILD_SCRIPT%" -Silent
) else (
    powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%VBOX_BUILD_SCRIPT%"
)
set "VBOX_BUILD_EXIT=%ERRORLEVEL%"
if not "%VBOX_BUILD_EXIT%"=="0" exit /b %VBOX_BUILD_EXIT%

set "VBOX_BUILD_OUTPUT=%~dp0out\win.amd64\release"
call :require_output "%VBOX_BUILD_OUTPUT%\bin\UICommon.dll" || exit /b %ERRORLEVEL%
call :require_output "%VBOX_BUILD_OUTPUT%\bin\VirtualBox.exe" || exit /b %ERRORLEVEL%
call :require_output "%VBOX_BUILD_OUTPUT%\bin\VirtualBoxVM.exe" || exit /b %ERRORLEVEL%
call :require_output "%VBOX_BUILD_OUTPUT%\lib\UICommon.lib" || exit /b %ERRORLEVEL%

if defined VBOX_BUILD_SILENT exit /b 0

set "VBOX_BUILT_EXE=%VBOX_BUILD_OUTPUT%\bin\VirtualBox.exe"
echo.
echo [READY] The unsigned native application build is ready.
echo [NOTE] An uninstalled checkout can still require local COM registration before it starts.
choice /C YN /N /M "Run VirtualBox now? [Y/N] "
if errorlevel 2 exit /b 0
start "" "%VBOX_BUILT_EXE%"
exit /b 0

:require_output
if not exist "%~1" (
    echo [ERROR] The build reported success, but a required output is missing: %~1
    exit /b 4
)
if "%~z1"=="0" (
    echo [ERROR] The build reported success, but a required output is empty: %~1
    exit /b 5
)
exit /b 0
