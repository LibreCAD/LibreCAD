@echo off
setlocal enabledelayedexpansion
rem package-windows.bat - NSIS installer packaging with explicit validation
rem Required env vars:
rem   LC_ARCH      - target architecture: amd64 or arm64
rem   SCMREVISION  - version string from git describe (e.g. 2.2.2-alpha, v2.2.1-3-gabcd1234)
rem Optional:
rem   LC_NSIS_FILE - override NSIS script (default: nsis-msvc.nsi)

call set-windows-env.bat

rem ── 1. Validate LC_ARCH ────────────────────────────────────────────────────
if "!LC_ARCH!"=="" (
    echo [ERROR] LC_ARCH is not set. Must be 'amd64' or 'arm64'.
    exit /b 1
)
if /I "!LC_ARCH!"=="amd64" (
    set NSIS_ARCH_FLAGS=/DWIN64 /DAMD64
) else if /I "!LC_ARCH!"=="arm64" (
    set NSIS_ARCH_FLAGS=/DWIN64 /DARM64
) else (
    echo [ERROR] LC_ARCH must be 'amd64' or 'arm64', got: '!LC_ARCH!'
    exit /b 1
)
echo [INFO] LC_ARCH validated: !LC_ARCH!

rem ── 2. Validate SCMREVISION ────────────────────────────────────────────────
if "!SCMREVISION!"=="" (
    echo [ERROR] SCMREVISION is not set.
    exit /b 1
)
rem Must not be the literal word "unknown"
if /I "!SCMREVISION!"=="unknown" (
    echo [ERROR] SCMREVISION is 'unknown' - version could not be determined.
    exit /b 1
)
rem Must begin with a digit or 'v' followed by a digit (e.g. 2.2.2 or v2.2.2)
set "_first=!SCMREVISION:~0,1!"
if "!_first!"=="v" set "_first=!SCMREVISION:~1,1!"
echo !_first!| findstr /R "^[0-9]" >nul
if errorlevel 1 (
    echo [ERROR] SCMREVISION must start with a version number, got: '!SCMREVISION!'
    exit /b 1
)
echo [INFO] SCMREVISION validated: !SCMREVISION!

rem ── 3. Compute VIProductVersion (X.X.X.X) from SCMREVISION ────────────────
set "input=!SCMREVISION!"
rem Strip leading 'v'
if "!input:~0,1!"=="v" set "input=!input:~1!"
set "CLEAN_VERSION="
for /L %%i in (0,1,50) do (
    set "char=!input:~%%i,1!"
    if "!char!"=="" goto :vi_done
    echo !char!| findstr /R "[0-9.]" >nul
    if errorlevel 1 (
        goto :vi_done
    ) else (
        set "CLEAN_VERSION=!CLEAN_VERSION!!char!"
    )
)
:vi_done
for /f "tokens=1-4 delims=." %%a in ("!CLEAN_VERSION!") do (
    set "v1=%%a" & set "v2=%%b" & set "v3=%%c" & set "v4=%%d"
)
for %%v in (v1 v2 v3 v4) do if "!%%v!"=="" set "%%v=0"
set "VIProductVersion=!v1!.!v2!.!v3!.!v4!"
echo [INFO] VIProductVersion: !VIProductVersion!

rem ── 4. Run makensis ────────────────────────────────────────────────────────
if "!LC_NSIS_FILE!"=="" set LC_NSIS_FILE=nsis-msvc.nsi
set NSIS_FLAGS=/X"SetCompressor /FINAL lzma" /V4 !NSIS_ARCH_FLAGS!
set NSIS_FLAGS=!NSIS_FLAGS! /DSCMREVISION="!SCMREVISION!" /DVIProductVersion="!VIProductVersion!"
echo [INFO] Running: makensis !NSIS_FLAGS! !LC_NSIS_FILE!
pushd "%~dp0postprocess-windows"
makensis.exe !NSIS_FLAGS! !LC_NSIS_FILE!
set _EC=!errorlevel!
popd
if !_EC! neq 0 (
    echo [ERROR] makensis failed with exit code !_EC!
    exit /b !_EC!
)
echo [INFO] Installer built successfully.
exit /b 0
