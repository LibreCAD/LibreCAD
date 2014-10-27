call set-windows-env.bat

pushd postprocess-windows
makensis.exe /X"SetCompressor /FINAL lzma" /V4 nsis-5.3.nsi
popd
