#!/bin/bash
#

## script to build LibreCAD AppImage
## intended to be used on Host or Travis CI
## does not run on modern systems because of linuxdeployqt
## for testing it can be called with parameter 'clean' to remove AppImage file and folder

# ensure that the script is called from LibreCAD root folder and executable exists
# you can use: export LC_ROOT=/path-to-your-lc-root/
#
LC_ROOT=../../
if [ ! -d "unix" ]; then
    echo "The script has to be called from LibreCAD build root folder!"
    exit
fi
if [ ! -f "unix/librecad" ]; then
    echo "Please build LibreCAD first, before calling this script!"
    exit
fi

# for testing purposes and manual use
# this script can be called with parameter clean
# to remove appdir and LibreCAD*.AppImage files
if [ 1 -eq $# ] && [ "clean" = "$1" ]; then
    [ -d "appdir" ] && rm -Rf appdir
    [ -d "icons" ] && rm -Rf icons
    [ -d "test" ] && rm -Rf test
    [ -f "librecad.py" ] && rm -f librecad.py
    find ./ -iname '*.AppImage*' -type f -exec rm -f {} \;
    echo "cleaned LibreCAD AppImage files"
    exit
fi

# setup VARS
ARCH=`uname -m`
if [ "${ARCH}" == "aarch64" ]; then
    DEB_ARCH=arm64
else
    DEB_ARCH=${ARCH}
fi

VERSION_CODENAME=`cat /etc/*-release | grep VERSION_CODENAME | sed -e 's/.*=//'`
DISTRIB_ID=`cat /etc/*-release | grep DISTRIB_ID | sed -e 's/.*=//g'`
DISTRIB_RELEASE=`cat /etc/*-release | grep DISTRIB_RELEASE | sed -e 's/.*=//'`
GIT_VERSION=`git describe --always`
LC_VERSION=`cat ${LC_ROOT}librecad/src/src.pro | grep LC_VERSION -m 1 | sed -e 's/.*=//' -e 's/"//g'`-${GIT_VERSION}
APPIMAGE=LibreCAD-${LC_VERSION}-${ARCH}.AppImage
ZIP_FILES="${APPIMAGE} librecad.desktop icons"
ZIPPED=LibreCAD-${LC_VERSION}_${DISTRIB_ID}-${DISTRIB_RELEASE}_\(${VERSION_CODENAME}\)-${DEB_ARCH}.AppImage.zip

# check for pubkey.asc
if [ ! -f "${LC_ROOT}pubkey.asc" ]; then
    wget https://raw.githubusercontent.com/probonopd/go-appimage/refs/heads/master/pubkey.asc
    mv pubkey.asc ${LC_ROOT}pubkey.asc
fi

# create folder structure
mkdir -p appdir/usr/bin
mkdir -p appdir/usr/lib/librecad
mkdir -p appdir/usr/share/applications
mkdir -p appdir/usr/share/librecad
mkdir -p appdir/usr/share/metainfo
mkdir -p appdir/usr/share/doc/librecad
mkdir -p appdir/usr/share/icons/hicolor/256x256/apps
mkdir -p appdir/usr/share/icons/hicolor/scalable/apps
mkdir -p appdir/usr/share/librecad

# strip binaries
strip unix/librecad
strip unix/resources/plugins/*.so

# copy executables and binary resources
cp unix/librecad appdir/usr/bin/
cp unix/resources/plugins/*.so appdir/usr/lib/librecad/
cp -r ${LC_ROOT}unix/resources/qm appdir/usr/share/librecad/

cp ${LC_ROOT}desktop/librecad.desktop appdir/usr/share/applications/
cp ${LC_ROOT}desktop/org.librecad.librecad.appdata.xml appdir/usr/share/metainfo/

cp -r ${LC_ROOT}librecad/support/doc/* appdir/usr/share/doc/librecad/
cp -r ${LC_ROOT}librecad/support/fonts appdir/usr/share/librecad/
cp -r ${LC_ROOT}librecad/support/library appdir/usr/share/librecad/
cp -r ${LC_ROOT}librecad/support/patterns appdir/usr/share/librecad/

cp ${LC_ROOT}CI/librecad.svg appdir/usr/share/icons/hicolor/scalable/apps/
convert -resize 256x256 ${LC_ROOT}CI/librecad.svg appdir/usr/share/icons/hicolor/256x256/apps/librecad.png

wget -c https://github.com/$(wget -q https://github.com/probonopd/go-appimage/releases/expanded_assets/continuous -O - | grep "appimagetool-.*-${ARCH}.AppImage" | head -n 1 | cut -d '"' -f 2)
chmod +x appimagetool-*.AppImage
wget https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-${ARCH}.AppImage
wget https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-${ARCH}.AppImage
chmod +x *.AppImage

./linuxdeploy-${ARCH}.AppImage --appdir appdir -e appdir/usr/bin/librecad -d appdir/usr/share/applications/librecad.desktop
./linuxdeploy-plugin-qt-${ARCH}.AppImage --appdir appdir
VERSION=${LC_VERSION} ./appimagetool-*.AppImage appdir/

# add librecad.desktop for user setup AppImage
cp appdir/usr/share/applications/librecad.desktop .
sed -i -e "s/Exec=librecad/Exec=${APPIMAGE}/" librecad.desktop

# add icons for user setup AppImage
git clone https://github.com/emanuel4you/LibreCAD-Icons.git icons
rm -Rf icons/.*
mkdir -p icons/apps/256x256
mkdir -p icons/apps/scalable

cp appdir/usr/share/icons/hicolor/256x256/apps/librecad.png icons/apps/256x256
cp appdir/usr/share/icons/hicolor/scalable/apps/librecad.svg icons/apps/scalable

# add README.md
cat << EOF >> README.md
#LibreCAD ${LC_VERSION} AppImage
${ZIPPED}

##To run AppImage, simply:
Extract
$ unzip ${ZIPPED}
Make it executable
$ chmod a+x ${APPIMAGE}
and run!
$ ./${APPIMAGE}

##To install:
copy ${APPIMAGE} to a path included in the environment:
e.g. /usr/local/bin, /usr/bin, /opt/usr/bin, ~/home/bin, ...
$ cp ${APPIMAGE} <path\>
copy librecad.desktop to a path included in the environment:
e.g. ~/home/.local/share/applications, /usr/share/applications
$ cp librecad.desktop <path\>

##Custom icons:
to use for librecad.desktop,
dwg, dxf, lisp, dcl mimetypes.
Mimetypes can be easily added via most file browsers.
Add text file bla.lisp, select file setting and change Common Lisp Source's icon.
EOF

# test if developer build
if [ -f "unix/librecad.py" ]; then
    cp unix/librecad.py .
    git clone https://github.com/emanuel4you/LibreCAD-Developer-Examples.git test
    rm -Rf test/.*
    ZIP_FILES="${ZIP_FILES} test librecad.py README.md"
    echo "" >> README.md
    echo "##Developer build:" >> README.md
    echo "copy test folder to home directory" >> README.md
    echo "see test/README.md for testing LibreLisp, LibrePython, LibreDcl" >> README.md
fi

sed -i -e 's/$/  /g' README.md

zip -mr9 ${ZIPPED} ${ZIP_FILES}
