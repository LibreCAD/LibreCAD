#!/bin/bash -x

#build dmg installers for OS/X

OSNAME[6]="SnowLeopard"
OSNAME[7]="Lion"
OSNAME[8]="MountainLion"
OSNAME[9]="Mavericks"
OSNAME[10]="Yosemite"

OSSDK[6]="macosx10.6"
OSSDK[7]="macosx10.7"
OSSDK[8]="macosx10.8"
OSSDK[9]="macosx10.9"
OSSDK[10]="macosx10.10"

#path of this script file
SCRIPTPATH="$(dirname "$0")"
export PATH=/opt/local/bin:$PATH

for v in {6..10}
do
	"${SCRIPTPATH}"/build-osx.sh -qmake_opts="-spec mkspec/macports QMAKE_MAC_SDK=${OSSDK[$v]}"
	mv -v ${SCRIPTPATH}/../LibreCAD.dmg ${SCRIPTPATH}/../LibreCAD-${OSNAME[$v]}.dmg
done

