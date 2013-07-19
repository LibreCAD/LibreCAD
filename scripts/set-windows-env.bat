@echo off

if "%Qt_DIR%"=="" goto SetEnv
if "%NSIS_DIR%"=="" goto SetEnv
goto Exit

:SetEnv
set Qt_DIR=C:\Qt\Qt5.0.2\5.0.2
set NSIS_DIR=C:\Program Files (x86)\NSIS

if exist custom-windows.bat call custom-windows.bat
set PATH=%Qt_DIR%\mingw47_32\bin;%Qt_DIR%\..\Tools\MinGW\bin;%NSIS_DIR%\Bin;%PATH%

:Exit
echo on
