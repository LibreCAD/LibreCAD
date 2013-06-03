
set Qt_DIR=C:\Qt\Qt5.0.2\5.0.2
set NSIS_DIR=C:\Program Files (x86)\NSIS

if exist custom-windows.bat call custom-windows.bat

set PATH=%Qt_DIR%\mingw47_32\bin;%Qt_DIR%\..\Tools\MinGW\bin;%NSIS_DIR%\Bin;%PATH%

cd ../..
mkdir LibreCAD-Release
cd LibreCAD-Release
qmake.exe ../LibreCAD/librecad.pro -r -spec win32-g++
mingw32-make.exe clean
mingw32-make.exe 
cd ../LibreCAD/scripts/postprocess-windows
makensis.exe /X"SetCompressor /FINAL lzma" nsis-5.0.nsi
