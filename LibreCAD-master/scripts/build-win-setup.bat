call set-windows-env.bat

if _%LC_NSIS_FILE%==_ (
    set LC_NSIS_FILE=nsis-5.4.nsi
)
pushd postprocess-windows
makensis.exe /X"SetCompressor /FINAL lzma" /V4 %LC_NSIS_FILE%
popd
