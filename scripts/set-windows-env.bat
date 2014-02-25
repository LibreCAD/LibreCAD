@echo off

if "%Qt_DIR%"=="" goto SetEnv
if "%NSIS_DIR%"=="" goto SetEnv
goto Exit

:SetEnv
set Qt_DIR=C:\Qt\Qt5.2.0\5.2.0
set NSIS_DIR=C:\Program Files (x86)\NSIS
set MINGW_VER=mingw48_32

if exist custom-windows.bat call custom-windows.bat
set PATH=%Qt_DIR%\%MINGW_VER%\bin;%Qt_DIR%\..\Tools\%MINGW_VER%\bin;%NSIS_DIR%;%PATH%

:Exit
echo on
