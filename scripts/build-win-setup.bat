call set-windows-env.bat

cd postprocess-windows
makensis.exe /X"SetCompressor /FINAL lzma" nsis-5.1.nsi
cd ..
