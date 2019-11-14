@echo off
setlocal

REM **Find dumpshx.exe**
setlocal ENABLEDELAYEDEXPANSION
for /d /r "c:\Program Files\Autodesk" %%i in (dumpshx.exe) do (
  @if exist "%%i" (
    @set DUMPSHX_EXE=%%i
    @echo !DUMPSHX_EXE!
    )
  )
if not exist "%DUMPSHX_EXE%" (
  for /d /r "c:\Program Files (x86)\Autodesk" %%i in (dumpshx.exe) do (
  @if exist "%%i" (
    @set DUMPSHX_EXE=%%i
    @echo !DUMPSHX_EXE!
    )
  )
)
if not exist "%DUMPSHX_EXE%" (
  echo dumpshx.exe not found!
  goto done
)

REM **Find LibreCAD user font path**
setlocal ENABLEEXTENSIONS
set KEY_NAME=HKCU\Software\LibreCAD\LibreCAD\Paths
set VALUE_NAME=Fonts
FOR /F "tokens=2*" %%A IN ('REG.exe query "%KEY_NAME%" /v "%VALUE_NAME%"') DO (set FONT_FOLDER=%%B)
echo %FONT_FOLDER%
if not exist "%FONT_FOLDER%" (
  echo LibreCAD font folder not found!
  goto done
)

md %FONT_FOLDER%\shp
REM **Convert every SHX in c:\Program Files\Autodesk**
for /r "c:\Program Files\Autodesk" %%F in (*.shx) do (
  "%DUMPSHX_EXE%" "%%~dpnxF" > "%FONT_FOLDER%\shp\%%~nF.shp"
)
for /r "c:\Program Files (x86)\Autodesk" %%F in (*.shx) do (
  "%DUMPSHX_EXE%" "%%~dpnxF" > "%FONT_FOLDER%\shp\%%~nF.shp"
)
REM **Convert every SHX in the current folder**
for %%F in (*.shx) do (
  "%DUMPSHX_EXE%" "%%~dpnxF" > "%FONT_FOLDER%\shp\%%~nF.shp"
)
:done
endlocal
