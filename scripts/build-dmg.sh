#!/bin/bash -x

#build dmg installers for OS/X

OSNAME[6]="SnowLeopard"
OSNAME[7]="Lion"
OSNAME[8]="MountainLion"
OSNAME[9]="Mavericks"

#path of this script file
SCRIPTPATH="$(dirname "$0")"
cd "${SCRIPTPATH}"/..

SETTINGSFILE=settings.pri

for v in {6..9}
do
	if [ $v -gt 6 ]
	then
		sed -i'' -e '$d' $SETTINGSFILE
	fi
	echo "QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.$v" >> $SETTINGSFILE
	"${SCRIPTPATH}"/build-osx.sh
	mv -v LibreCAD.dmg "LibreCAD-${OSNAME[$v]}".dmg
done

