
set Qt_DIR=C:\Qt\Qt5.0.2\5.0.2
set PATH=%Qt_DIR%\mingw47_32\bin;%Qt_DIR%\..\Tools\MinGW\bin;C:\Program Files (x86)\NSIS\Bin;%PATH%

cd ../..
mkdir LibreCAD-Release
cd Release
qmake.exe ../LibreCAD/librecad.pro -r -spec win32-g++
mingw32-make.exe 
cd ../LibreCAD/scripts/postprocess-windows
makensis.exe nsis-5.0.nsi
