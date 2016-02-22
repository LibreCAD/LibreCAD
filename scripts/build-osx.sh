#!/bin/bash -xe

# this script builds LibreCAD in OS/X
# The dependency required to be able to build LibreCAD:
# qt, boost, muparser

# Options
# -p=|--qtpath= : Set's a specific path where to startqmake from example : build-osx.sh -p=/opt/Qt5.2.1/5.2.1/clang_64/bin
# default is set to /opt/local/bin/ for backwards compatibility of this script
#
# -q=|-qmake_opts= : Set's additional qmake options exaomple : -qmake_opts="QMAKE_MAC_SDK=macosx10.9"
# default is set to "-spec mkspec/macports" for backwards compatibility reasons of this script
#
# -no-p|--no-qtpath : Removes the default qtpath, this makes the defaukt search path take over to find qmake


# the LibreCAD source folder 

#use default gcc from MacPorts
# port install gcc49
# port select gcc mp-gcc49
# To use the default clang use teh below
# port select gcc none

#export PATH=/opt/local/bin:$PATH
#default qt from MacPorts
# specify QT_PATH to customize
SCRIPTPATH="$(dirname "$0")"
QT_PATH=/opt/local/bin/


QMAKE_OPTS="-spec mkspec/macports"

for i in "$@"
do
case $i in
    -q=*|-qmake_opts=*)
    QMAKE_OPTS="${i#*=}"
    ;;
    -p=*|--qtpath*=)
    QT_PATH="${i#*=}"
    if [[ $QT_PATH ]]
    then
        QT_PATH=${QT_PATH%/}/
    fi
    ;;
    -no-p|--no-qtpath)
    QT_PATH=
    ;;
    *)
            # unknown option
    ;;
esac
done

#validate QT_PATH
if [[ ! -f ${QT_PATH}qmake ]]
then
	QT_PATH=$(dirname "$(which qmake)")/
	if [[ -z $QT_PATH ]]
	then
		echo "can not locate qmake"
	fi
fi

QMAKE_CMD=${QT_PATH}qmake

$QMAKE_CMD -v

cd "${SCRIPTPATH}"/..

# have to clean up any existing binary files to avoid crashes of bug#422
rm -rf LibreCAD.app

# Run distclean if a previous version of Makefile exists
if [ -f Makefile ]; then
    $QMAKE_CMD $QMAKE_OPTS -r
    make distclean
fi

rm -rf generated
$QMAKE_CMD $QMAKE_OPTS -r

#undefined symbol x86_64: https://qt-project.org/forums/viewthread/35646
# RVT July 12 2015, this is now controlled with QMAKE_MAC_SDK
#find . -iname makefile -exec sed -i '' \
#	-e 's:mmacosx-version-min=10.[1-9]:mmacosx-version-min=10.8:g' \
#	-e 's:MacOSX10.[1-9].sdk:MacOSX10.8.sdk:g'  \
#	'{}' ';'

#to make it auto, use "make -j"
#hardcoded to 4 jobs, because "make -j" crashes our mac building box
make -j4

APP_FILE=LibreCAD
OUTPUT_DMG=${APP_FILE}.dmg
rm -f "${OUTPUT_DMG}"
${QT_PATH}macdeployqt ${APP_FILE}.app -verbose=2 -dmg

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
