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
echo windepolyqt: Current directory is: %CD%
dir windows\
mkdir windows\ts
windeployqt.exe --release windows\LibreCAD.exe --verbose 2 --force
dir librecad\ts\*.ts
    for %%f in (librecad\ts\*.ts plugins\ts\*.ts) do (
      lrelease.exe "%%f" -qm ..\..\windows\ts\
    )
rem lrelease.exe librecad\ts\*.ts -qm windows\ts\
dir windows\ts
rem windeployqt.exe windows\LibreCAD.exe --release --verbose 2 --force

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
    set SCMREVISION=2.2.1.3
)

echo "SCMREVISION=%SCMREVISION%"

popd

call set-windows-env.bat

        echo "SCMREVISION=%SCMREVISION%"
if _%LC_NSIS_FILE%==_ (
    set LC_NSIS_FILE=nsis-msvc.nsi
)

pushd postprocess-windows

rem Detect 64-bit build environment and pass /DWIN64 if applicable
set NSIS_FLAGS=/X"SetCompressor /FINAL lzma" /V4
if "%PROCESSOR_ARCHITECTURE%"=="AMD64" (
    set NSIS_FLAGS=%NSIS_FLAGS% /DWIN64 /DAMD64
)

rem Pass the extracted SCMREVISION to NSIS
        echo "SCMREVISION=%SCMREVISION%"
set NSIS_FLAGS=%NSIS_FLAGS% /DSCMREVISION="!SCMREVISION!"

makensis.exe %NSIS_FLAGS% %LC_NSIS_FILE%
popd