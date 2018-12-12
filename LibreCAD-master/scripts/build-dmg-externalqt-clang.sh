#!/bin/bash -x

# Build dmg installers for OS/X
#
# Qt came from a external installation and can be found using the default path
# You have to make sure it will find that version of qmake, and not the version of macports
# Compilation will happen with clang, make sure it's your default compiler set : sudo port select gcc none and test with gcc --version
# This will possible work without macports but I didn't test it (if you have boost installed somewhere)


OSNAME[7]="Lion"
OSNAME[8]="MountainLion"
OSNAME[9]="Mavericks"
OSNAME[10]="Yosemite"
OSNAME[11]="ElCapitan"
OSNAME[12]="Sierra"

OSSDK[7]="macosx10.7"
OSSDK[8]="macosx10.8"
OSSDK[9]="macosx10.9"
OSSDK[10]="macosx10.10"
OSSDK[11]="macosx10.11"
OSSDK[12]="macosx10.12"

#path of this script file
SCRIPTPATH="$(dirname "$0")"

for v in $(xcodebuild -showsdks | grep macosx | sed -e 's/.*macosx10\.//g')
do
	for t in $(seq 7 $v)
	do
		"${SCRIPTPATH}"/build-osx.sh --no-qtpath -qmake_opts="QMAKE_MAC_SDK=${OSSDK[$v]} QMAKE_MACOSX_DEPLOYMENT_TARGET=10.$t"
		mv -v ${SCRIPTPATH}/../LibreCAD.dmg ${SCRIPTPATH}/../LibreCAD-${OSNAME[$t]}-${OSSDK[$v]}.dmg
	done
done

