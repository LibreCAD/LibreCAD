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
windeployqt.exe --release windows\LibreCAD.exe --verbose 2 --force
rem lrelease.exe librecad\ts\*.ts -qm windows\ts\
rem windeployqt.exe windows\LibreCAD.exe --release --verbose 2 --force

popd

call set-windows-env.bat

if _%LC_NSIS_FILE%==_ (
    set LC_NSIS_FILE=nsis-msvc.nsi
)

pushd postprocess-windows

rem Detect 64-bit build environment and pass /DWIN64 if applicable
set NSIS_FLAGS=/X"SetCompressor /FINAL lzma" /V4
if "%PROCESSOR_ARCHITECTURE%"=="AMD64" (
    set NSIS_FLAGS=%NSIS_FLAGS% /DWIN64
)

makensis.exe %NSIS_FLAGS% %LC_NSIS_FILE%
popd
call build-win-setup.bat
