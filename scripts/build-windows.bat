call set-windows-env.bat

pushd ..

windeployqt.exe windows\LibreCAD.exe
if exist windows\ttf2lff.exe (
	windeployqt.exe windows\ttf2lff.exe
)
popd
call build-win-setup.bat
