@echo off
setlocal

REM **Find lconvert.exe**
setlocal ENABLEDELAYEDEXPANSION
for /d /r "c:\qt" %%i in (lconvert.exe) do (
  @if exist "%%i" (
    @set LCONVERT_EXE=%%i
    @echo !LCONVERT_EXE!
    )
  )
if not exist "%LCONVERT_EXE%" (
  for /d /r "c:\Program Files (x86)\qt" %%i in (lconvert.exe) do (
  @if exist "%%i" (
    @set LCONVERT_EXE=%%i
    @echo !LCONVERT_EXE!
    )
  )
)
if not exist "%LCONVERT_EXE%" (
  echo dumpshx.exe not found!
  goto done
)

if exist "plugins_es_mx.xlf" (
  REM update the (Spain) Spanish translation with the (Latin America) version
  copy plugins_es_mx.xlf plugins_es.xlf
)

if exist "plugins_pt_br.xlf" (
  REM update the (Portugal) Portuguese translation with the (Brazil) version
  copy plugins_pt_br.xlf plugins_pt_pt.xlf
)

REM **Convert every TS in the current folder**
for %%F in (*.xlf) do (
  "%LCONVERT_EXE%" "%%~dpnxF" -o "%%~nF.ts"
)
:done
endlocal
