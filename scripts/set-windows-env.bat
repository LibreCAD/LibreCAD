@echo off
rem set-windows-env.bat - ensure makensis.exe is on PATH for NSIS packaging.
rem Qt env vars are expected to come from jurplel/install-qt-action (QT_ROOT_DIR / Qt6_DIR);
rem this script does not override them.

if "%NSIS_DIR%"=="" set "NSIS_DIR=C:\Program Files\NSIS"

echo %PATH% | findstr /I /C:"%NSIS_DIR%" >nul
if errorlevel 1 set "PATH=%NSIS_DIR%;%PATH%"

if exist "%~dp0custom-windows.bat" call "%~dp0custom-windows.bat"
