@echo off
echo "Create development environment for LibreCAD (Qt6 version)"
echo "This batch script should be run in Command Prompt, i.e. cmd.exe"
echo "in the Administrator mode"
echo "This script will install chocolatey, mingw, git, python, cmake, curl, and vscode"
echo "It will use pip for aqtinstall (latest version) instead of choco (outdated)"
echo "It will download Boost headers (header-only usage)"
echo "If successful, LibreCAD can be built using qmake or cmake"
echo "Assumes building the master branch for Qt6 support"

setlocal enabledelayedexpansion

rem Set source directory
set "SOURCE_DIR=%~dp0.."

echo LibreCAD source assumed at !SOURCE_DIR!

cd /d "!SOURCE_DIR!"

where choco >nul 2>nul || (
    powershell -NoProfile -ExecutionPolicy Bypass -Command "[System.Net.ServicePointManager]::SecurityProtocol = [System.Net.ServicePointManager]::SecurityProtocol -bor 3072; iex ((New-Object System.Net.WebClient).DownloadString('https://community.chocolatey.org/install.ps1'))"
    set "PATH=%PATH%;%ALLUSERSPROFILE%\chocolatey\bin"
    echo "PATH= %PATH%"
)

choco uninstall mingw qt5-default -y
choco install git -y
choco install python -y
choco install cmake -y
choco install curl -y
choco install vscode -y

rem Use pip for aqtinstall (latest version, choco is outdated at 2.1.0 vs pip 3.3.0+)
pip uninstall aqtinstall -y
pip3 install --upgrade pip
pip3 install aqtinstall --upgrade

rem Set Qt min version (for Qt6 migration in LibreCAD master)
set QT_MIN_MAJOR=6
set QT_MIN_MINOR=5
set QT_MIN_PATCH=0

rem Set architecture
set QT_ARCH=win64_mingw

rem Set MinGW min version (based on Qt6 requirements, e.g., >=11.2, but aiming for recent)
set MINGW_MIN_MAJOR=11
set MINGW_MIN_MINOR=2
set MINGW_MIN_PATCH=0

call :CheckQtInPath
if !QT_FOUND_IN_PATH!==1 goto :QtFound

call :FindExistingQt

if !QT_COUNT! GTR 0 (
    echo Found existing Qt installations in C:\Qt.
    call :SelectLatest !QT_COUNT! QT_VERS
    set QT_VERSION=!LATEST_VERSION!
    set QT_ARCH=!QT_ARCHS[!LATEST_INDEX!]!
    set QT_DIR=!QT_PATHS[!LATEST_INDEX!]!
    echo Using existing Qt !QT_VERSION! at !QT_DIR!
    goto :QtFound
)

echo No suitable Qt found in PATH or C:\Qt. Installing...
call :FindLatestQtOnline
set ARCHES=
for /f %%l in ('aqt list-qt windows desktop --arch !QT_VERSION!') do set "ARCHES=!ARCHES! %%l"
set AVAIL_ARCH_COUNT=0
for %%a in (!ARCHES!) do (
    set ARCH_NAME=%%a
    if "!ARCH_NAME!"=="!QT_ARCH!" (
        set /a AVAIL_ARCH_COUNT+=1
        set AVAIL_ARCHS[!AVAIL_ARCH_COUNT!]=%%a
    )
)
if !AVAIL_ARCH_COUNT!==0 (
    echo Architecture !QT_ARCH! not available for Qt !QT_VERSION!.
    echo Falling back to available arch if possible.
    set QT_ARCH=!AVAIL_ARCHS[1]!
)

aqt install-qt --outputdir C:\Qt windows desktop !QT_VERSION! !QT_ARCH! -m qt5compat qtsvg qttools
set "QT_DIR=C:\Qt\!QT_VERSION!\!QT_ARCH!"

:QtFound

call :CheckMinGWInPath
if !MINGW_FOUND_IN_PATH!==1 goto :MinGWFound

call :FindExistingMinGW

if !MINGW_COUNT! GTR 0 (
    echo Found existing MinGW installations in C:\Qt\Tools.
    call :SelectLatestMinGW !MINGW_COUNT! MINGW_VERS
    set MINGW_VERSION=!LATEST_VERSION!
    set MINGW_TOOL_NAME=!MINGW_TOOL_NAMES[!LATEST_INDEX!]!
    set MINGW_DIR=!MINGW_PATHS[!LATEST_INDEX!]!
    echo Using existing MinGW !MINGW_VERSION! at !MINGW_DIR!
    goto :MinGWFound
)

echo No suitable MinGW found in PATH or C:\Qt\Tools. Installing latest...
call :FindLatestMinGWOnline
aqt install-tool --outputdir C:\Qt windows desktop tools_mingw !MINGW_TOOL_NAME!
set "MINGW_DIR=C:\Qt\Tools\!MINGW_DIR_NAME!"

:MinGWFound

call :CheckInstallQtCreator

set PATH=%QT_DIR%\bin;%MINGW_DIR%\bin;%PATH%

rem Set Boost (header-only)
set DEFAULT_BOOST_LIB_VERSION=1.89.0
set BOOST_LIB_VERSION=!DEFAULT_BOOST_LIB_VERSION!
set BOOST_VERSION=!BOOST_LIB_VERSION:.=_!

echo Searching for Boost in C:\local...
set BOOST_FOUND=0
set DESIRED_BOOST_DIR=C:\local\boost_!BOOST_VERSION!
if exist "!DESIRED_BOOST_DIR!\boost\version.hpp" (
    set BOOST_DIR=!DESIRED_BOOST_DIR!
    set BOOST_FOUND=1
    echo Found existing Boost headers at !BOOST_DIR!
)

if !BOOST_FOUND!==0 (
    echo No suitable Boost found. Downloading headers...
    set BOOST_URL=https://archives.boost.io/release/!BOOST_LIB_VERSION!/source/boost_!BOOST_VERSION!.zip
    powershell -Command "Invoke-WebRequest -Uri '!BOOST_URL!' -OutFile 'boost.zip'"
    powershell -Command "Expand-Archive -Path 'boost.zip' -DestinationPath 'C:\local'"
    set BOOST_DIR=C:\local\boost_!BOOST_VERSION!
)

rem Prepare custom.pro
del /F /Q librecad\src\custom.pro
echo BOOST_DIR = !BOOST_DIR:\=/! >> librecad\src\custom.pro
echo BOOST_LIBDIR = !BOOST_DIR:\=/! >> librecad\src\custom.pro

rem Prepare for submodule (Catch2)
git submodule init
git submodule update

rem Build with qmake
echo %PATH%
rem qmake6 -r librecad.pro
if errorlevel 1 (
    echo qmake failed.
    exit /b 1
)
mingw32-make -j6
if errorlevel 1 (
    echo Make failed.
    exit /b 1
)

echo Build completed. Executable in windows\release\librecad.exe

rem Copy required DLLs for running the executable
set "EXE_DIR=windows\release"
if not exist "!EXE_DIR!" mkdir "!EXE_DIR!"
copy "!MINGW_DIR!\bin\libgcc_s_seh-1.dll" "!EXE_DIR!" >nul 2>nul
copy "!MINGW_DIR!\bin\libstdc++-6.dll" "!EXE_DIR!" >nul 2>nul
copy "!MINGW_DIR!\bin\libwinpthread-1.dll" "!EXE_DIR!" >nul 2>nul
copy "!QT_DIR!\bin\Qt6Core.dll" "!EXE_DIR!" >nul 2>nul
copy "!QT_DIR!\bin\Qt6Gui.dll" "!EXE_DIR!" >nul 2>nul
copy "!QT_DIR!\bin\Qt6PrintSupport.dll" "!EXE_DIR!" >nul 2>nul
copy "!QT_DIR!\bin\Qt6Svg.dll" "!EXE_DIR!" >nul 2>nul
copy "!QT_DIR!\bin\Qt6Widgets.dll" "!EXE_DIR!" >nul 2>nul
echo Required DLLs copied to !EXE_DIR!

rem Optional: Unit tests by cmake
echo Building unit tests...
cmake -G "MinGW Makefiles" -S . -B buildTests -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON -DCMAKE_PREFIX_PATH=%QT_DIR% -DBOOST_ROOT=%BOOST_DIR% -DCMAKE_C_COMPILER=%MINGW_DIR%\bin\gcc.exe -DCMAKE_CXX_COMPILER=%MINGW_DIR%\bin\g++.exe
if errorlevel 1 (
    echo CMake configuration failed.
) else (
    cmake --build buildTests -j6
    if errorlevel 1 (
        echo CMake build failed.
    ) else (
        echo Unit tests in buildTests\librecad_tests.exe
    )
)

pause
endlocal
goto :EOF

:CheckQtInPath
echo Checking for Qt in PATH...
set QT_FOUND_IN_PATH=0
where qmake6 >nul 2>nul
if errorlevel 1 goto :EOF
qmake6 --version > temp.txt 2>&1
for /f "tokens=*" %%a in ('findstr /C:"Using Qt version" temp.txt') do set QT_LIB_LINE=%%a
if not defined QT_LIB_LINE goto :EOF
for /f "tokens=4" %%b in ("!QT_LIB_LINE!") do set QT_VERSION=%%b
for /f "tokens=6" %%c in ("!QT_LIB_LINE!") do set LIB_PATH=%%c
del temp.txt
if not "!LIB_PATH:mingw=!"=="!LIB_PATH!" if not "!LIB_PATH:_64=!"=="!LIB_PATH!" (
    set QT_DIR=!LIB_PATH:\lib=!
    for /f "tokens=1,2,3 delims=." %%d in ("!QT_VERSION!") do (
        set V_MAJOR=%%d
        set V_MINOR=%%e
        set V_PATCH=%%f
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
        set QT_FOUND_IN_PATH=1
        echo Found suitable Qt !QT_VERSION! in PATH at !QT_DIR!
    )
)
goto :EOF

:FindExistingQt
echo Searching for Qt in C:\Qt...
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
            if exist "%%V\!QT_ARCH!\bin\qmake6.exe" (
                set /a QT_COUNT+=1
                set QT_VERS[!QT_COUNT!]=!VERSION!
                set QT_ARCHS[!QT_COUNT!]=!QT_ARCH!
                set QT_PATHS[!QT_COUNT!]=%%V\!QT_ARCH!
            )
        )
    )
)
goto :EOF

:SelectLatest
set COUNT=%1
set ARRAY_PREFIX=%2
set MAX_MAJOR=0
set MAX_MINOR=0
set MAX_PATCH=0
set LATEST_INDEX=0
set LATEST_VERSION=
for /l %%i in (1,1,%COUNT%) do (
    set VERSION=!%ARRAY_PREFIX%[%%i]!
    for /f "tokens=1,2,3 delims=." %%a in ("!VERSION!") do (
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
        set LATEST_INDEX=%%i
        set LATEST_VERSION=!VERSION!
    )
)
goto :EOF

:CheckMinGWInPath
echo Checking for MinGW in PATH...
set MINGW_FOUND_IN_PATH=0
where gcc >nul 2>nul
if errorlevel 1 goto :EOF
gcc -dumpversion > temp.txt 2>&1
set /p GCC_VERSION=<temp.txt
del temp.txt
for /f "tokens=1,2,3 delims=." %%a in ("!GCC_VERSION!") do (
    set V_MAJOR=%%a
    set V_MINOR=%%b
    set V_PATCH=%%c
    if "!V_PATCH!"=="" set V_PATCH=0
)
set USE_THIS=0
if !V_MAJOR! GTR !MINGW_MIN_MAJOR! set USE_THIS=1
if !V_MAJOR! EQU !MINGW_MIN_MAJOR! (
    if !V_MINOR! GTR !MINGW_MIN_MINOR! set USE_THIS=1
    if !V_MINOR! EQU !MINGW_MIN_MINOR! (
        if !V_PATCH! GEQ !MINGW_MIN_PATCH! set USE_THIS=1
    )
)
if !USE_THIS!==1 (
    for /f %%d in ('where gcc') do set MINGW_BIN=%%~dpd
    set MINGW_DIR=!MINGW_BIN:~0,-5!  rem remove \bin\
    set MINGW_FOUND_IN_PATH=1
    set MINGW_VERSION=!GCC_VERSION!
    echo Found suitable MinGW !MINGW_VERSION! in PATH at !MINGW_DIR!
)
goto :EOF

:FindExistingMinGW
echo Searching for MinGW in C:\Qt\Tools...
set MINGW_COUNT=0
for /d %%D in (C:\Qt\Tools\mingw*_64) do (
    if exist "%%D\bin\gcc.exe" (
        set DIR_NAME=%%~nD
        set VER_STR=!DIR_NAME:mingw=!
        set VER_STR=!VER_STR:_64=!
        set MAJOR=!VER_STR:~0,2!
        set MINOR=!VER_STR:~2,1!
        set PATCH=!VER_STR:~3,1!
        if "!PATCH!"=="" set PATCH=0
        set VERSION=!MAJOR!.!MINOR!.!PATCH!
        set USE_THIS=0
        if !MAJOR! GTR !MINGW_MIN_MAJOR! set USE_THIS=1
        if !MAJOR! EQU !MINGW_MIN_MAJOR! (
            if !MINOR! GTR !MINGW_MIN_MINOR! set USE_THIS=1
            if !MINOR! EQU !MINGW_MIN_MINOR! (
                if !PATCH! GEQ !MINGW_MIN_PATCH! set USE_THIS=1
            )
        )
        if !USE_THIS!==1 (
            set /a MINGW_COUNT+=1
            set MINGW_VERS[!MINGW_COUNT!]=!VERSION!
            set MINGW_PATHS[!MINGW_COUNT!]=%%D
            set MINGW_TOOL_NAMES[!MINGW_COUNT!]=qt.tools.!DIR_NAME:_64=_win64!
            set MINGW_DIR_NAMES[!MINGW_COUNT!]=!DIR_NAME!
        )
    )
)
goto :EOF

:SelectLatestMinGW
set COUNT=%1
set ARRAY_PREFIX=%2
set MAX_MAJOR=0
set MAX_MINOR=0
set MAX_PATCH=0
set LATEST_INDEX=0
set LATEST_VERSION=
for /l %%i in (1,1,%COUNT%) do (
    set VERSION=!%ARRAY_PREFIX%[%%i]!
    for /f "tokens=1,2,3 delims=." %%a in ("!VERSION!") do (
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
        set LATEST_INDEX=%%i
        set LATEST_VERSION=!VERSION!
    )
)
goto :EOF

:FindLatestMinGWOnline
set MINGW_VERS=
for /f %%l in ('aqt list-tool windows desktop tools_mingw') do set "MINGW_VERS=!MINGW_VERS! %%l"
set AVAIL_COUNT=0
for %%v in (!MINGW_VERS!) do (
    set TOOL_NAME=%%v
    if "!TOOL_NAME:qt.tools.mingw=!"=="!TOOL_NAME!" goto :skip
    set VER_STR=!TOOL_NAME:qt.tools.mingw=!
    set VER_STR=!VER_STR:_win64=!
    set MAJOR=!VER_STR:~0,2!
    set MINOR=!VER_STR:~2,1!
    set PATCH=!VER_STR:~3,1!
    if "!PATCH!"=="" set PATCH=0
    set VERSION=!MAJOR!.!MINOR!.!PATCH!
    set USE_THIS=0
    if !MAJOR! GTR !MINGW_MIN_MAJOR! set USE_THIS=1
    if !MAJOR! EQU !MINGW_MIN_MAJOR! (
        if !MINOR! GTR !MINGW_MIN_MINOR! set USE_THIS=1
        if !MINOR! EQU !MINGW_MIN_MINOR! (
            if !PATCH! GEQ !MINGW_MIN_PATCH! set USE_THIS=1
        )
    )
    if !USE_THIS!==1 (
        set /a AVAIL_COUNT+=1
        set AVAIL_VERS[!AVAIL_COUNT!]=!VERSION!
        set AVAIL_TOOL_NAMES[!AVAIL_COUNT!]=!TOOL_NAME!
        set AVAIL_DIR_NAMES[!AVAIL_COUNT!]=mingw!VER_STR!_64
    )
    :skip
)
if !AVAIL_COUNT!==0 (
    echo No available MinGW versions found.
    exit /b 1
)
call :SelectLatestMinGW !AVAIL_COUNT! AVAIL_VERS
set MINGW_VERSION=!LATEST_VERSION!
set MINGW_TOOL_NAME=!AVAIL_TOOL_NAMES[!LATEST_INDEX!]!
set MINGW_DIR_NAME=!AVAIL_DIR_NAMES[!LATEST_INDEX!]!
echo "MINGW_VERSION: %MINGW_VERSION%"
goto :EOF

:CheckInstallQtCreator
set QTCREATOR_DIR=C:\Qt\Tools\QtCreator
if not exist "!QTCREATOR_DIR!\bin\qtcreator.exe" (
    echo No Qt Creator found. Installing...
    aqt install-tool --outputdir C:\Qt windows desktop tools_qtcreator qt.tools.qtcreator
) else (
    echo Found existing Qt Creator at !QTCREATOR_DIR!
)
goto :EOF

:FindLatestQtOnline
set VERSIONS=
for /f %%l in ('aqt list-qt windows desktop --spec "6"') do set "VERSIONS=!VERSIONS! %%l"
set AVAIL_COUNT=0
for %%v in (!VERSIONS!) do (
    set VERSION=%%v
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
        set /a AVAIL_COUNT+=1
        set AVAIL_VERS[!AVAIL_COUNT!]=%%v
    )
)
if !AVAIL_COUNT!==0 (
    echo No available Qt versions found.
    exit /b 1
)
call :SelectLatest !AVAIL_COUNT! AVAIL_VERS
set QT_VERSION=!LATEST_VERSION!
echo "QT_VERSION: %QT_VERSION%"
goto :EOF
