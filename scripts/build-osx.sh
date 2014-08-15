#!/bin/bash -xe

# this script builds LibreCAD in OS/X
# The dependency required to be able to build LibreCAD:
# qt, boost, muparser

# the LibreCAD source folder 

SCRIPTPATH="$(dirname "$0")"
PATH=~/Qt5.3.1/5.3/clang_64/bin:$PATH
QMAKE_CMD=qmake

cd "${SCRIPTPATH}"/..
# have to clean up any existing binary files to avoid crashes of bug#422
rm -rf LibreCAD.app

$QMAKE_CMD -r -spec macx-clang
make distclean
$QMAKE_CMD -r -spec macx-clang
#undefined symbol x86_64: https://qt-project.org/forums/viewthread/35646
find . -iname makefile -exec sed -i '' \
	-e 's:mmacosx-version-min=10.[1-8]:mmacosx-version-min=10.9:g' \
	-e 's:MacOSX10.[1-8].sdk:MacOSX10.9.sdk:g'  \
	'{}' ';'

make -j4
rm -f LibreCAD.dmg
macdeployqt LibreCAD.app -verbose=2 -dmg
