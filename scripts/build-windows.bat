call set-windows-env.bat

cd ..
qmake.exe librecad.pro -r -spec win32-g++
mingw32-make.exe clean
mingw32-make.exe 
cd .\scripts
call build-win-setup.bat
