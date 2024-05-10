call set-windows-env.bat

pushd ..
qmake6.exe librecad.pro -r -spec win32-clang-g++
if not _%1==_NoClean (
	mingw32-make.exe clean
)
mingw32-make.exe -j8
if NOT exist windows\LibreCAD.exe (
	echo "Building windows\LibreCAD.exe failed!"
	exit /b /1
)
windeployqt6.exe windows\LibreCAD.exe --release --compiler-runtime
popd
call build-win-setup.bat
