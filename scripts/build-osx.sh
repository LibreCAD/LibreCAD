#!/bin/bash -xe

# this script builds LibreCAD in OS/X
# The dependency required to be able to build LibreCAD:
# qt, boost, muparser

# the LibreCAD source folder 

SCRIPTPATH="$(dirname "$0")"
PATH=~/Qt5.2.1/5.2.1/clang_64/bin:$PATH
QMAKE_CMD=qmake

cd "${SCRIPTPATH}"/..
# have to clean up any existing binary files to avoid crashes of bug#422
rm -rf LibreCAD.app

$QMAKE_CMD -r -spec macx-clang "build_muparser = true"
make distclean
$QMAKE_CMD -r -spec macx-clang "build_muparser = true"
make -j4
rm -f LibreCAD.dmg
macdeployqt LibreCAD.app -verbose=2 -dmg
