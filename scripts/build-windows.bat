@echo off
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

rem ------------------------------------------------------------------
rem Extract SCMREVISION from the built executable (from LC_VERSION in src.pro)
rem ------------------------------------------------------------------
echo [INFO] Extracting version (SCMREVISION) from LibreCAD.exe...
set SCMREVISION=unknown

rem Use strings.exe to find the LC_VERSION string
where strings >nul 2>nul
if %errorlevel% equ 0 (
    for /f "delims=" %%v in ('strings windows\LibreCAD.exe ^| findstr /b /c:"LC_VERSION"') do (
        set "LINE=%%v"
        rem Remove everything up to and including "LC_VERSION"
        set "SCMREVISION=!LINE:*LC_VERSION=!"
        rem Remove surrounding quotes and trim spaces
        set "SCMREVISION=!SCMREVISION:"=!"
        set "SCMREVISION=!SCMREVISION: =!"
    )
)

if "!SCMREVISION!"=="unknown" (
    echo [WARNING] Could not extract version from executable. Falling back to default.
    set SCMREVISION=2.2.1.3
)

popd

call set-windows-env.bat

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
set NSIS_FLAGS=%NSIS_FLAGS% /DSCMREVISION="!SCMREVISION!"

makensis.exe %NSIS_FLAGS% %LC_NSIS_FILE%
popd
