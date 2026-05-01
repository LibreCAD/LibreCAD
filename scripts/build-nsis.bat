@echo off
setlocal enabledelayedexpansion

rem build-nsis.bat - run NSIS to package the windeployqt'd build into an installer.
rem
rem Inputs (env vars; all optional):
rem   LC_ARCH         AMD64 (default) or ARM64 - passed to NSIS as /D<ARCH>
rem   LC_NSIS_FILE    NSIS script under scripts/postprocess-windows (default: nsis-msvc.nsi)
rem   SCMREVISION     Version string (e.g. 2.2.2-alpha-12-gabc1234). Auto-derived if empty/"unknown".
rem   VIProductVersion  4-part X.X.X.X for VS_VERSION_INFO. Auto-derived from SCMREVISION if empty.
rem   NSIS_DIR        Directory containing makensis.exe (default: C:\Program Files (x86)\NSIS)

call "%~dp0set-windows-env.bat"

if "%LC_NSIS_FILE%"=="" set "LC_NSIS_FILE=nsis-msvc.nsi"
if "%LC_ARCH%"==""      set "LC_ARCH=AMD64"

rem Repo root is one level above this script
for %%I in ("%~dp0..") do set "REPO_ROOT=%%~fI"

rem ── 1. SCMREVISION ────────────────────────────────────────────────────────
if "%SCMREVISION%"=="" set "SCMREVISION=unknown"
if /I "%SCMREVISION%"=="unknown" call :derive_scmrevision
echo [INFO] SCMREVISION=!SCMREVISION!

rem ── 2. VIProductVersion ───────────────────────────────────────────────────
if "%VIProductVersion%"=="" call :derive_viproductversion
echo [INFO] VIProductVersion=!VIProductVersion!
echo [INFO] LC_ARCH=!LC_ARCH!

rem ── 3. Run makensis ───────────────────────────────────────────────────────
pushd "%~dp0postprocess-windows"

set NSIS_FLAGS=/X"SetCompressor /FINAL lzma" /V4 /D!LC_ARCH!
set NSIS_FLAGS=!NSIS_FLAGS! /DSCMREVISION="!SCMREVISION!" /DVIProductVersion="!VIProductVersion!"

echo [INFO] Running: makensis !NSIS_FLAGS! !LC_NSIS_FILE!
makensis.exe !NSIS_FLAGS! "!LC_NSIS_FILE!"
set "_EC=!errorlevel!"
popd

if !_EC! neq 0 (
    echo [ERROR] NSIS packaging failed with exit code !_EC!
    exit /b !_EC!
)
echo [INFO] NSIS packaging completed successfully.
exit /b 0


:derive_scmrevision
rem Try git describe in repo root
where git >nul 2>nul
if !errorlevel! equ 0 (
    if exist "!REPO_ROOT!\.git" (
        for /f "delims=" %%i in ('git -C "!REPO_ROOT!" describe --always 2^>nul') do set "SCMREVISION=%%i"
    )
)
if /I not "!SCMREVISION!"=="unknown" goto :eof

rem Fallback: parse LC_VERSION from src.pro
set "line="
for /f "delims=" %%l in ('findstr /R /C:"^[ \t]*LC_VERSION[ \t]*=" "!REPO_ROOT!\librecad\src\src.pro" 2^>nul') do set "line=%%l"
if defined line (
    for /f "tokens=1,* delims==" %%a in ("!line!") do set "value=%%b"
    for /f "tokens=*" %%i in ("!value!") do set "value=%%i"
    if "!value:~0,1!"=="\"" if "!value:~-1!"=="\"" (
        set "SCMREVISION=!value:~1,-1!"
    ) else (
        set "SCMREVISION=!value!"
    )
)
if /I not "!SCMREVISION!"=="unknown" goto :eof

echo [WARNING] Could not determine SCMREVISION; using default
set "SCMREVISION=2.2.2-alpha"
goto :eof


:derive_viproductversion
set "input=!SCMREVISION!"
if "!input:~0,1!"=="v" set "input=!input:~1!"
set "CLEAN_VERSION="
for /L %%i in (0,1,50) do (
    set "char=!input:~%%i,1!"
    if "!char!"=="" goto :pad_vi
    echo !char!| findstr /R "[0-9.]" >nul
    if errorlevel 1 (
        goto :pad_vi
    ) else (
        set "CLEAN_VERSION=!CLEAN_VERSION!!char!"
    )
)
:pad_vi
for /f "tokens=1-4 delims=." %%a in ("!CLEAN_VERSION!") do (
    set "v1=%%a" & set "v2=%%b" & set "v3=%%c" & set "v4=%%d"
)
for %%v in (v1 v2 v3 v4) do if "!%%v!"=="" set "%%v=0"
set "VIProductVersion=!v1!.!v2!.!v3!.!v4!"
goto :eof
