call set-windows-env.bat

cd postprocess-windows
makensis.exe /X"SetCompressor /FINAL lzma" /V4 nsis-5.1.nsi
cd ..
