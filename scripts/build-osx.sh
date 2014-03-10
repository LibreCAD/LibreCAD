#!/bin/bash -xe

# this script builds LibreCAD in OS/X
# The dependency required to be able to build LibreCAD:
# qt, boost, muparser

# the LibreCAD source folder 

SCRIPTPATH="$(dirname "$0")"

cd "${SCRIPTPATH}"/..
# have to clean up any existing binary files to avoid crashes of bug#422
rm -rf LibreCAD.app

qmake librecad.pro -r -spec mkspec/macports
make distclean
qmake librecad.pro -r -spec mkspec/macports
make -j4
rm -f LibreCAD.dmg
macdeployqt LibreCAD.app -verbose=2 -dmg
