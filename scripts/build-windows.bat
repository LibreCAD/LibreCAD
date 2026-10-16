@echo off
setlocal enabledelayedexpansion

call set-windows-env.bat

pushd ..

rem Detect if MSVC environment is active (cl.exe available)
where cl >nul 2>nul
if %errorlevel% equ 0 (
    echo Building with MSVC
    qmake.exe librecad.pro -r CONFIG+=release
    if not _%1==_NoClean (
        nmake clean
    )
    nmake release
) else (
    echo Building with MinGW
    qmake.exe librecad.pro -r -spec win32-g++
    if not _%1==_NoClean (
        mingw32-make.exe clean
    )
    mingw32-make.exe release -j4
)

if NOT exist windows\LibreCAD.exe (
    echo "Building windows\LibreCAD.exe failed!"
    popd
    exit /b 1
)

rem Improved windeployqt: verbose, force copy, standard plugin subdirs
dir windows\
rem mkdir windows\ts
mkdir windows\translations
for %%f in (librecad\ts\*.ts plugins\ts\*.ts) do (
    lrelease.exe "%%f"
)
rem lrelease.exe librecad\ts\*.ts -qm windows\ts\
echo windepolyqt: Current directory is: %CD%
windeployqt.exe --release windows\LibreCAD.exe librecad\ts --verbose 2 --force
dir windows\translations
copy windows\translations\* librecad\ts\
dir librecad\ts\

echo [INFO] Extracting version (SCMREVISION)...
set SCMREVISION=unknown

rem Parse default LC_VERSION from src.pro (find line starting with LC_VERSION= , allowing spaces)
for /f "delims=" %%l in ('findstr /R /C:"^[ \t]*LC_VERSION[ \t]*=" librecad\src\src.pro') do set "line=%%l"

if defined line (
    for /f "tokens=1,* delims==" %%a in ("!line!") do set "value=%%b"
    rem Trim leading/trailing spaces
    for /f "tokens=*" %%i in ("!value!") do set "value=%%i"
    rem Remove surrounding quotes if present
    if "!value:~0,1!"=="\"" if "!value:~-1!"=="\"" (
        set "SCMREVISION=!value:~1,-1!"
    ) else (
        set "SCMREVISION=!value!"
    )
)

rem Override with Git if available (mimics src.pro logic)
where git >nul 2>nul
if %errorlevel%==0 (
    if exist .git (
        for /f "delims=" %%i in ('git describe --always 2^>nul') do set SCMREVISION=%%i
    )
)

rem If MSYSGIT_DIR is set, use it as alternative (like in src.pro)
if NOT "_%MSYSGIT_DIR%"=="_" (
    for /f "delims=" %%i in ('"%MSYSGIT_DIR%\git.exe" describe --always 2^>nul') do set SCMREVISION=%%i
)

if "!SCMREVISION!"=="unknown" (
    echo [WARNING] Could not extract version. Using hardcoded default.
    set SCMREVISION=2.2.1.5
)

echo "SCMREVISION=%SCMREVISION%"

:: Input string (e.g., 2.2.1.3-*, 2.2.2-alpha)
set "input=%SCMREVISION%"
if "!input!"=="" set "input=2.2.1-rc*"
rem Strip the optional v prefix from annotated release tags.
if /I "!input:~0,1!"=="v" set "input=!input:~1!"

:: 1. Scan for the first character that is NOT 0-9 or .
set "CLEAN_VERSION="
for /L %%i in (0,1,50) do (
    set "char=!input:~%%i,1!"
    if "!char!"=="" goto :done_scan

    echo !char!| findstr /R "[0-9.]" >nul
    if errorlevel 1 (
        goto :done_scan
    ) else (
        set "CLEAN_VERSION=!CLEAN_VERSION!!char!"
    )
)

:done_scan
:: 2. Split and pad with zeros to ensure X.X.X.X format
for /f "tokens=1-4 delims=." %%a in ("!CLEAN_VERSION!") do (
    set "v1=%%a" & set "v2=%%b" & set "v3=%%c" & set "v4=%%d"
)

for %%v in (v1 v2 v3 v4) do if "!%%v!"=="" set "%%v=0"

set "VIProductVersion=!v1!.!v2!.!v3!.!v4!"

popd

call set-windows-env.bat

        echo "SCMREVISION=%SCMREVISION%"
if _%LC_NSIS_FILE%==_ (
    set LC_NSIS_FILE=nsis-msvc.nsi
)

pushd postprocess-windows

rem The installer has to match the program it packages, and the machine that
rem builds the program does not say what the program is: 64-bit Windows also
rem builds the 32-bit LibreCAD. So ask the program that was built.
set "LC_EXE_ARCH="
for /f "usebackq delims=" %%a in (`powershell.exe -NoProfile -ExecutionPolicy Bypass -File "..\exe-arch.ps1" "..\..\windows\LibreCAD.exe"`) do set "LC_EXE_ARCH=%%a"
echo [INFO] windows\LibreCAD.exe is built for: %LC_EXE_ARCH%

set NSIS_FLAGS=/X"SetCompressor /FINAL lzma" /V4
if /I "%LC_EXE_ARCH%"=="AMD64" (
    set NSIS_FLAGS=%NSIS_FLAGS% /DWIN64 /DAMD64
) else if /I not "%LC_EXE_ARCH%"=="x86" (
    echo [ERROR] No installer can be built: windows\LibreCAD.exe has to be a program for x86 or AMD64, and exe-arch.ps1 reported "%LC_EXE_ARCH%".
    popd
    exit /b 1
)

rem Pass the extracted SCMREVISION to NSIS
        echo "SCMREVISION=%SCMREVISION%"
set NSIS_FLAGS=%NSIS_FLAGS% /DSCMREVISION="!SCMREVISION!" /DVIProductVersion="!VIProductVersion!"

makensis.exe %NSIS_FLAGS% %LC_NSIS_FILE%
popd
