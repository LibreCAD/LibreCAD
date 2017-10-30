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
# -no-p|--no-qtpath : Removes the default qtpath, this makes the default search path take over to find qmake
#
# -cert=|-codesign-identity= : Run macdeployqt -codesign=<identity> (requires Qt >= 5.4.0)
# Example: ./build-osx.sh -cert=123456789A
# Use 'security find-identity -v -p codesigning' to get a list of signing identities.
# Example: A000000000000000000000000000000000000001 "Developer ID Application: John Smith (123456789A)"

SCRIPTPATH="$(dirname "$0")"

for i in /opt/local/libexec /usr/local/opt /usr/local
do
    if [ -x "$i/qt5/bin/qmake" ]
    then
        QT_PATH=$i/qt5/bin/
        break
    fi
done
if [ -z "$QT_PATH" ]
then
    echo QT_PATH could not be determined, exiting >&2
    exit 1
fi

echo QT_PATH="$QT_PATH"

QMAKE_OPTS=""
CODESIGN_IDENTITY=""

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
    -cert=*|-codesign-identity=*)
    CODESIGN_IDENTITY="${i#*=}"
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
		exit 1
	fi
fi

QMAKE_CMD=${QT_PATH}qmake

# validate QT_VERSION
QT_VERSION=$(${QMAKE_CMD} -query QT_VERSION)
QT_VERSION_ARRAY=( ${QT_VERSION//./ } )
echo "QT_VERSION=${QT_VERSION_ARRAY[0]}.${QT_VERSION_ARRAY[1]}.${QT_VERSION_ARRAY[2]}"

# validate CODESIGN
if [[ $CODESIGN_IDENTITY ]]
then
	if [ "${QT_VERSION_ARRAY[0]}" -lt 5 ]
	then
		echo "macdeployqt -codesign requires QT_VERSION >= 5.4.0"
		exit 1
	else
		if [ "${QT_VERSION_ARRAY[1]}" -lt 4 ]
		then
	                echo "macdeployqt -codesign requires QT_VERSION >= 5.4.0"
			exit 1
		fi
	fi
fi

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

if [[ $CODESIGN_IDENTITY ]]
then
	${QT_PATH}macdeployqt ${APP_FILE}.app -verbose=2 -dmg -always-overwrite -codesign=$CODESIGN_IDENTITY
else
	${QT_PATH}macdeployqt ${APP_FILE}.app -verbose=2 -dmg -always-overwrite
fi

#bz2 compression
hdiutil convert -format UDBZ -ov -o "$OUTPUT_DMG" "$OUTPUT_DMG"

if [[ -f  "${OUTPUT_DMG}" ]]
then
	echo "DMG installer generated:"
	ls -lh "${OUTPUT_DMG}"
fi

rm -f "${TMP_DMG}"
if [[ $CODESIGN_IDENTITY ]]
then
	codesign -s $CODESIGN_IDENTITY -v $OUTPUT_DMG
fi
