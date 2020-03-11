@echo off
setlocal

REM **Find lupdate.exe**
setlocal ENABLEDELAYEDEXPANSION
for /d /r "c:\qt" %%i in (lupdate.exe) do (
  @if exist "%%i" (
    @set LUPDATE_EXE=%%i
    @echo !LUPDATE_EXE!
    )
  )
if not exist "%LUPDATE_EXE%" (
  for /d /r "c:\Program Files (x86)\qt" %%i in (lupdate.exe) do (
  @if exist "%%i" (
    @set LUPDATE_EXE=%%i
    @echo !LUPDATE_EXE!
    )
  )
)
if not exist "%LUPDATE_EXE%" (
  echo dumpshx.exe not found!
  goto done
)

"%LUPDATE_EXE%" ..\plugins.pro

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

REM **Convert every TS in the current folder**
for %%F in (*.ts) do (
  "%LCONVERT_EXE%" "%%~dpnxF" -o "%%~nF.xlf"
)
:done
endlocal
