#!/bin/bash -x

# this script builds LibreCAD in OS/X
# The dependency required to be able to build LibreCAD:
# qt, boost, muparser

# the LibreCAD source folder 

SCRIPTPATH="$(dirname "$0")"

cd "${SCRIPTPATH}"/..
qmake librecad.pro -r -spec mkspec/macports
make -j4
rm -f LibreCAD.dmg
macdeployqt LibreCAD.app -verbose=2 -dmg
