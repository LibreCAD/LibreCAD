#!/bin/bash 

export QT_ROOT="/cygdrive/c/Qt"
export QT_VERSION=5.0.2
export QT_DIR="${QT_ROOT}/Qt${QT_VERSION}/${QT_VERSION}"
export NSIS="/cygdrive/c/Program Files (x86)/NSIS"

export PATH="${QT_DIR}"/mingw47_32/bin:"${QT_DIR}"/../Tools/MinGW/bin:"${NSIS}"/bin:$PATH

cd ../..
mkdir LibreCAD-Release
cd LibreCAD-Release
qmake.exe ../LibreCAD/librecad.pro -r -spec win32-g++
mingw32-make.exe clean
mingw32-make.exe 
cd ../LibreCAD/scripts/postprocess-windows
makensis.exe /X"SetCompressor /FINAL lzma" nsis-5.0.nsi
