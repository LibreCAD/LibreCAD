rem @echo off
echo "Create development environment for LibreCAD"
echo "This batch script should be run in Command Prompt, i.e. cmd.exe"
echo "in the Adminstrator mode"
echo "This script would install chocolatey, boost, mingw, git, python, and cmake"
echo "If successful, LibreCAD be built using qmake or cmake"

setlocal enabledelayedexpansion

rem the librecad source folder
cd %~dp0\..

echo Starting build environment preparation for win64_mingw

rem Install dependencies via Chocolatey
where choco >nul 2>nul || (
    powershell -NoProfile -ExecutionPolicy Bypass -Command "[System.Net.ServicePointManager]::SecurityProtocol = [System.Net.ServicePointManager]::SecurityProtocol -bor 3072; iex ((New-Object System.Net.WebClient).DownloadString('https://community.chocolatey.org/install.ps1'))"
    set PATH=%PATH%;%ALLUSERSPROFILE%\chocolatey\bin
)
choco install boost-msvc-14.3 git python cmake -y

rem Install latest aqtinstall via pip
python -m pip install --upgrade pip
python -m pip install aqtinstall --upgrade

rem Set Qt min version
set QT_MIN_MAJOR=6
set QT_MIN_MINOR=8
set QT_MIN_PATCH=0

rem Set architecture
set QT_ARCH=win64_mingw

rem Search existing Qt
echo Searching for Qt...
set QT_COUNT=0
for /d %%V in (C:\Qt\6.*) do (
    set VERSION=%%~nV
    for /f "tokens=1,2,3 delims=." %%a in ("!VERSION!") do (
        set V_MAJOR=%%a
        set V_MINOR=%%b
        set V_PATCH=%%c
        if "!V_PATCH!"=="" set V_PATCH=0
    )
    set USE_THIS=0
    if !V_MAJOR! GTR !QT_MIN_MAJOR! set USE_THIS=1
    if !V_MAJOR! EQU !QT_MIN_MAJOR! (
        if !V_MINOR! GTR !QT_MIN_MINOR! set USE_THIS=1
        if !V_MINOR! EQU !QT_MIN_MINOR! (
            if !V_PATCH! GEQ !QT_MIN_PATCH! set USE_THIS=1
        )
    )
    if !USE_THIS!==1 (
        if exist "%%V\!QT_ARCH!" (
            set /a QT_COUNT+=1
            set QT_VERS[!QT_COUNT!]=!VERSION!
            set QT_ARCHS[!QT_COUNT!]=!QT_ARCH!
            set QT_PATHS[!QT_COUNT!]=%%V\!QT_ARCH!
        )
    )
)

if !QT_COUNT!==0 (
    echo No Qt found. Installing...
    set VERSIONS=
    for /f %%l in ('aqt list-qt windows desktop --spec "6"') do set VERSIONS=%%l
    set AVAIL_COUNT=0
    for %%v in (!VERSIONS!) do (
        for /f "tokens=1,2,3 delims=." %%a in ("%%v") do (
            set V_MAJOR=%%a
            set V_MINOR=%%b
            set V_PATCH=%%c
            if "!V_PATCH!"=="" set V_PATCH=0
        )
        set USE_THIS=0
        if !V_MAJOR! GTR !QT_MIN_MAJOR! set USE_THIS=1
        if !V_MAJOR! EQU !QT_MIN_MAJOR! (
            if !V_MINOR! GTR !QT_MIN_MINOR! set USE_THIS=1
            if !V_MINOR! EQU !QT_MIN_MINOR! (
                if !V_PATCH! GEQ !QT_MIN_PATCH! set USE_THIS=1
            )
        )
        if !USE_THIS!==1 (
            set /a AVAIL_COUNT+=1
            set AVAIL_VERS[!AVAIL_COUNT!]=%%v
        )
    )
    if !AVAIL_COUNT!==0 exit /b 1
    rem Select latest
    set MAX_MAJOR=0
    set MAX_MINOR=0
    set MAX_PATCH=0
    set QT_VERSION=
    for /l %%i in (1,1,!AVAIL_COUNT!) do (
        for /f "tokens=1,2,3 delims=." %%a in ("!AVAIL_VERS[%%i]!") do (
            set C_MAJOR=%%a
            set C_MINOR=%%b
            set C_PATCH=%%c
            if "!C_PATCH!"=="" set C_PATCH=0
        )
        set UPDATE=0
        if !C_MAJOR! GTR !MAX_MAJOR! set UPDATE=1
        if !C_MAJOR! EQU !MAX_MAJOR! if !C_MINOR! GTR !MAX_MINOR! set UPDATE=1
        if !C_MAJOR! EQU !MAX_MAJOR! if !C_MINOR! EQU !MAX_MINOR! if !C_PATCH! GTR !MAX_PATCH! set UPDATE=1
        if !UPDATE!==1 (
            set MAX_MAJOR=!C_MAJOR!
            set MAX_MINOR=!C_MINOR!
            set MAX_PATCH=!C_PATCH!
            set QT_VERSION=!AVAIL_VERS[%%i]!
        )
    )

    set ARCHES=
    for /f %%l in ('aqt list-qt windows desktop --arch !QT_VERSION!') do set ARCHES=%%l
    set AVAIL_ARCH_COUNT=0
    for %%a in (!ARCHES!) do (
        set ARCH_NAME=%%a
        if "!ARCH_NAME!"=="!QT_ARCH!" (
            set /a AVAIL_ARCH_COUNT+=1
            set AVAIL_ARCHS[!AVAIL_ARCH_COUNT!]=%%a
        )
    )
    set QT_ARCH=win64_mingw

    aqt install-qt --outputdir C:\Qt windows desktop !QT_VERSION! !QT_ARCH! -m qt5compat
    aqt install-tool --outputdir C:\Qt windows desktop tools_mingw1310
    aqt install-tool --outputdir C:\Qt windows desktop sdktool
    set QT_DIR=C:\Qt\!QT_VERSION!\!QT_ARCH!
)

set PATH=C:\Qt\!QT_VERSION!\mingw_64\bin;C:\Qt\Tools\mingw1310_64\bin;%PATH%

rem Set Boost
set DEFAULT_BOOST_LIB_VERSION=1.87.0
set BOOST_LIB_VERSION=!DEFAULT_BOOST_LIB_VERSION!
set BOOST_VERSION=!BOOST_LIB_VERSION:.=_!

echo Searching for Boost in C:\local...
set BOOST_FOUND=0
set DESIRED_BOOST_DIR=C:\local\boost_!BOOST_VERSION!
if exist "!DESIRED_BOOST_DIR!\boost\version.hpp" (
    set BOOST_DIR=!DESIRED_BOOST_DIR!
    set BOOST_FOUND=1
    echo Found existing Boost source at !BOOST_DIR!
)

if !BOOST_FOUND!==0 (
    echo No suitable Boost found. Installing...
    set BOOST_URL=https://archives.boost.io/release/!BOOST_LIB_VERSION!/source/boost_!BOOST_VERSION!.zip
    powershell -Command "Invoke-WebRequest -Uri '!BOOST_URL!' -OutFile 'boost.zip'"
    powershell -Command "Expand-Archive -Path 'boost.zip' -DestinationPath 'C:\local'"
    set BOOST_DIR=C:\local\boost_!BOOST_VERSION!
)

for /f "tokens=1,2 delims=." %%a in ("!BOOST_LIB_VERSION!") do set BOOST_LIB_SUFFIX=%%a_%%b

rem Assume LibreCAD source is in current dir or adjust

del /F /Q librecad\src\custom.pro
echo BOOST_DIR = !BOOST_DIR:\=/! >> librecad\src\custom.pro
rem echo BOOST_LIB = boost_program_options-mgw131-mt-x64-!BOOST_LIB_SUFFIX! >> librecad\src\custom.pro
rem echo BOOST_LIBDIR = !BOOST_LIBDIR:\=/! >> librecad\src\custom.pro

rem propare for submodule (Catch2)
git submodule init
git submodule update

rem qmake
qmake6 -r librecad.pro
mingw32-make -j6

echo Build completed. Executable in windows\release\librecad.exe
rem Unit tests by cmake
cmake -S . -B buildTests -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON -DCMAKE_PREFIX_PATH=%QT_DIR% -DBOOST_ROOT=%BOOST_DIR% 
cmake --build buildTests -j6

echo Unit tests in build-tests\librecad_tests.exe

pause
endlocal

