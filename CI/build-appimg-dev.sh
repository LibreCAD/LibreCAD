#!/bin/bash
#

## script to build LibreCAD AppImage
## intended to be used on Travis CI
## does not run on modern systems because of linuxdeployqt
## for testing it can be called with parameter 'clean' to remove AppImage file and folder

ARCH=`uname -m`
VERSION_CODENAME=`cat /etc/*-release | grep VERSION_CODENAME | sed 's/.*=//g'`

# ensure that the script is called from LibreCAD root folder and executable exists
if [ ! -d "unix" ]; then
    echo "The script has to be called from LibreCAD root folder!"
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
    [ -d "test" ] && rm -Rf test
    [ -f "librecad.py" ] && rm -f librecad.py
    [ ! -f "*.AppImage" ] && rm *.AppImage
    [ ! -f "*.AppImage.zip" ] && rm *.AppImage.zip
    echo "cleaned LibreCAD AppImage files"
    exit
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
cp -r ../../unix/resources/qm appdir/usr/share/librecad/

cp ../../desktop/librecad.desktop appdir/usr/share/applications/
cp ../../desktop/org.librecad.librecad.appdata.xml appdir/usr/share/metainfo/

cp -r ../../librecad/support/doc/* appdir/usr/share/doc/librecad/
cp -r ../../librecad/support/fonts appdir/usr/share/librecad/
cp -r ../../librecad/support/library appdir/usr/share/librecad/
cp -r ../../librecad/support/patterns appdir/usr/share/librecad/

cp ../../CI/librecad.svg appdir/usr/share/icons/hicolor/scalable/apps/
convert -resize 256x256 ../../CI/librecad.svg appdir/usr/share/icons/hicolor/256x256/apps/librecad.png

wget -c https://github.com/$(wget -q https://github.com/probonopd/go-appimage/releases/expanded_assets/continuous -O - | grep "appimagetool-.*-${ARCH}.AppImage" | head -n 1 | cut -d '"' -f 2)
chmod +x appimagetool-*.AppImage
wget https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-${ARCH}.AppImage
wget https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-${ARCH}.AppImage
chmod +x *.AppImage
#ARCH=${ARCH} ./appimagetool-*.AppImage deploy appdir/usr/share/applications/librecad.desktop
./linuxdeploy-${ARCH}.AppImage --appdir appdir -e appdir/usr/bin/librecad -d appdir/usr/share/applications/librecad.desktop
./linuxdeploy-plugin-qt-${ARCH}.AppImage --appdir appdir
VERSION=`git describe --always` ./appimagetool-*.AppImage appdir/
mv LibreCAD-*-${ARCH}.AppImage LibreCAD-${VERSION_CODENAME}-${ARCH}.AppImage
git clone https://github.com/emanuel4you/LibreCAD-Developer-Examples.git test
cp unix/librecad.py .
zip -r LibreCAD-${VERSION_CODENAME}-${ARCH}-`date +%Y%m%d_%H%M%S`.AppImage.zip LibreCAD-${VERSION_CODENAME}-${ARCH}.AppImage test librecad.py
