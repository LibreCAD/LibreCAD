#!/bin/bash -xe

# this script builds LibreCAD in OS/X
# The dependency required to be able to build LibreCAD:
# qt, boost, muparser

# the LibreCAD source folder 

SCRIPTPATH="$(dirname "$0")"

#use default gcc from MacPorts
# port install gcc49
# port select gcc mp-gcc49

export PATH=/opt/local/bin:$PATH

#default qt from MacPorts
# specify QT_PATH to customize
QT_PATH=/opt/local/bin
QMAKE_CMD=$QT_PATH/qmake

$QMAKE_CMD -v

cd "${SCRIPTPATH}"/..
# have to clean up any existing binary files to avoid crashes of bug#422
rm -rf LibreCAD.app

$QMAKE_CMD -r -spec mkspec/macports
make distclean
$QMAKE_CMD -r -spec mkspec/macports
#undefined symbol x86_64: https://qt-project.org/forums/viewthread/35646
find . -iname makefile -exec sed -i '' \
	-e 's:mmacosx-version-min=10.[1-9]:mmacosx-version-min=10.8:g' \
	-e 's:MacOSX10.[1-9].sdk:MacOSX10.8.sdk:g'  \
	'{}' ';'

make -j4

APP_FILE=LibreCAD
OUTPUT_DMG=${APP_FILE}.dmg
rm -f "${OUTPUT_DMG}"
$QT_PATH/macdeployqt ${APP_FILE}.app -verbose=2 -dmg

TMP_DMG=$(mktemp temp-DMG.XXXXXXXXXX)

mv -vf "${OUTPUT_DMG}" "${TMP_DMG}"

#bz2 compression
rm -f $OUTPUT_DMG
hdiutil convert -format UDBZ "${TMP_DMG}" -o "$OUTPUT_DMG"
if [[ -f  "${OUTPUT_DMG}" ]]
then
	echo "DMG installer generated:"
	ls -lh "${OUTPUT_DMG}"
fi

rm -f "${TMP_DMG}"
