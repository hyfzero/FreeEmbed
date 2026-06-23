@echo off
setlocal
pushd "%~dp0"

set "OUT_DIR=%~dp0Objects"
set "LNP=%OUT_DIR%\EZPro100_v20.lnp"
set "AXF=%OUT_DIR%\EZPro100_v20.axf"
set "SREC=%OUT_DIR%\EZPro100_APP_v20.srec"
set "ARMCC_BIN=C:\Keil_v5\ARM\ARMCC\bin"
set "ARMLINK=armlink"
set "FROMELF=fromelf"

where armlink >nul 2>nul
if errorlevel 1 if exist "%ARMCC_BIN%\armlink.exe" set "ARMLINK=%ARMCC_BIN%\armlink.exe"

where fromelf >nul 2>nul
if errorlevel 1 if exist "%ARMCC_BIN%\fromelf.exe" set "FROMELF=%ARMCC_BIN%\fromelf.exe"

if exist "%SREC%" del /q "%SREC%"

if not exist "%AXF%" (
    if not exist "%LNP%" exit /b 1
    "%ARMLINK%" --via "%LNP%"
    if errorlevel 1 exit /b 1
)

"%FROMELF%" --m32 --output "%SREC%" "%AXF%"
if exist "%SREC%" (
    popd
    exit /b 0
)
popd
exit /b 1
