call set-windows-env.bat

cd ..
qmake.exe librecad.pro -r -spec win32-g++
mingw32-make.exe clean
mingw32-make.exe -j4
if NOT exist windows\LibreCAD.exe (
	echo "Building windows\LibreCAD.exe failed!"
	exit /b /1
)
windeployqt.exe windows\LibreCAD.exe
cd .\scripts
call build-win-setup.bat
