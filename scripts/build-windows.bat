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
windeployqt.exe --release ..\..\windows\LibreCAD.exe
rem windeployqt.exe windows\LibreCAD.exe --release --verbose 2 --force

popd

call build-win-setup.bat
