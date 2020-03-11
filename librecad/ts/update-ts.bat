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

if exist "librecad_es_mx.xlf" (
  REM update the (Spain) Spanish translation with the (Latin America) version
  copy librecad_es_mx.xlf librecad_es.xlf
)

if exist "librecad_pt_br.xlf" (
  REM update the (Portugal) Portuguese translation with the (Brazil) version
  copy librecad_pt_br.xlf librecad_pt_pt.xlf
)

REM **Convert every TS in the current folder**
for %%F in (*.xlf) do (
  "%LCONVERT_EXE%" "%%~dpnxF" -o "%%~nF.ts"
)
:done
endlocal
