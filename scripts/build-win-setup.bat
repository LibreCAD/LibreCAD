@echo off
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
