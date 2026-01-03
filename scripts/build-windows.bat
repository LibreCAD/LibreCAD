@echo off
setlocal EnableDelayedExpansion

echo ==================================================
echo LibreCAD Windows Build Script (MSVC-focused)
echo ==================================================

call set-windows-env.bat

pushd ..

rem ------------------------------------------------------------------
rem Detect compiler: MSVC (cl.exe) or MinGW
rem ------------------------------------------------------------------
where cl >nul 2>nul
if %errorlevel% equ 0 (
    echo [INFO] Building with MSVC
    set MAKE=nmake
    qmake.exe librecad.pro -r CONFIG+=release
) else (
    echo [INFO] Building with MinGW (fallback)
    set MAKE=mingw32-make.exe
    qmake.exe librecad.pro -r -spec win32-g++
)

rem ------------------------------------------------------------------
rem Clean (unless NoClean argument passed)
rem ------------------------------------------------------------------
if not _%1==_NoClean (
    echo [INFO] Cleaning previous build...
    %MAKE% clean
)

rem ------------------------------------------------------------------
rem Build release
rem ------------------------------------------------------------------
echo [INFO] Building release...
%MAKE% release
if errorlevel 1 (
    echo [ERROR] Build failed!
    popd
    exit /b 1
)

rem ------------------------------------------------------------------
rem Verify executable was created
rem ------------------------------------------------------------------
if not exist windows\LibreCAD.exe (
    echo [ERROR] windows\LibreCAD.exe was not built!
    popd
    exit /b 1
)
echo [SUCCESS] windows\LibreCAD.exe built successfully.

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
    set SCMREVISION=2.2.x
)

echo [INFO] Detected SCMREVISION: !SCMREVISION!

rem ------------------------------------------------------------------
rem Create ts directory and generate LibreCAD translations (.qm)
rem ------------------------------------------------------------------
echo [INFO] Generating translation files (.qm)...
if not exist windows\ts mkdir windows\ts

set LRELEASE="%Qt5_DIR%\bin\lrelease.exe"
if not exist %LRELEASE% (
    echo [WARNING] lrelease.exe not found at %Qt5_DIR%\bin\lrelease.exe - trying PATH
    set LRELEASE=lrelease.exe
)

set TS_COUNT=0
for %%f in (librecad\ts\*.ts plugins\ts\*.ts) do (
    if exist "%%f" (
        %LRELEASE% "%%f" -qm windows\ts\
        if errorlevel 1 (
            echo [WARNING] Failed to process %%f
        ) else (
            set /a TS_COUNT+=1
        )
    )
)
echo [INFO] Processed !TS_COUNT! translation file(s)

rem ------------------------------------------------------------------
rem Deploy Qt dependencies
rem ------------------------------------------------------------------
echo [INFO] Running windeployqt...
set WINDEPLOYQT="%Qt5_DIR%\bin\windeployqt.exe"
if not exist %WINDEPLOYQT% (
    echo [ERROR] windeployqt.exe not found!
    popd
    exit /b 1
)

%WINDEPLOYQT% --release --verbose 2 --force windows\LibreCAD.exe

popd

rem ------------------------------------------------------------------
rem Prepare NSIS installer - pass SCMREVISION
rem ------------------------------------------------------------------
call set-windows-env.bat

if "_%LC_NSIS_FILE%"=="_" (
    set LC_NSIS_FILE=nsis-msvc.nsi
)

echo [INFO] Using NSIS script: %LC_NSIS_FILE%
echo [INFO] Passing SCMREVISION=!SCMREVISION! to NSIS

pushd postprocess-windows

set NSIS_FLAGS=/X"SetCompressor /FINAL lzma" /V4

rem Architecture detection (for AMD64 define in nsis-msvc.nsi)
if /I "%PROCESSOR_ARCHITECTURE%"=="AMD64" (
    set NSIS_FLAGS=%NSIS_FLAGS% /DWIN64 /DAMD64
) else if /I "%PROCESSOR_ARCHITEW6432%"=="AMD64" (
    set NSIS_FLAGS=%NSIS_FLAGS% /DWIN64 /DAMD64
)

rem Pass the extracted SCMREVISION to NSIS
set NSIS_FLAGS=%NSIS_FLAGS% /DSCMREVISION="!SCMREVISION!"

echo [INFO] makensis %NSIS_FLAGS% %LC_NSIS_FILE%
makensis.exe %NSIS_FLAGS% %LC_NSIS_FILE%
if errorlevel 1 (
    echo [ERROR] NSIS compilation failed!
    popd
    exit /b 1
)

echo [SUCCESS] Installer created in generated\ (version !SCMREVISION!)

popd

if exist build-win-setup.bat (
    call build-win-setup.bat
)

echo ==================================================
echo Build completed successfully! (SCMREVISION: !SCMREVISION!)
echo ==================================================

endlocal
exit /b 0