#!/bin/bash
#

## script to build LibreCAD AppImage: arm64
## intended to be used on Travis CI
## does not run on modern systems because of linuxdeployqt
## for testing it can be called with parameter 'clean' to remove AppImage file and folder

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
    compgen -G "LibreCAD*.AppImage" >/dev/null && rm LibreCAD*.AppImage
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

# <<< FORCE BUNDLED QT PLUGINS >>>
# Add qt.conf next to the executable to force Qt to load plugins only from the bundled location
cat > appdir/usr/bin/qt.conf <<EOF
[Paths]
Plugins = ../plugins
EOF
# <<< END >>>

cp unix/resources/plugins/*.so appdir/usr/lib/librecad/
cp -r unix/resources/qm appdir/usr/share/librecad/

cp desktop/librecad.desktop appdir/usr/share/applications/
cp desktop/org.librecad.librecad.appdata.xml appdir/usr/share/metainfo/
cp -r librecad/support/doc/* appdir/usr/share/doc/librecad/
cp -r librecad/support/fonts appdir/usr/share/librecad/
cp -r librecad/support/library appdir/usr/share/librecad/
cp -r librecad/support/patterns appdir/usr/share/librecad/
mkdir -p appdir/usr/lib/aarch64-linux-gnu/qt5/plugins
cp -r /usr/lib/aarch64-linux-gnu/qt5/plugins/* appdir/usr/lib/aarch64-linux-gnu/qt5/plugins
mkdir -p appdir/usr/plugins/
cp -r /usr/lib/aarch64-linux-gnu/qt5/plugins/* appdir/usr/plugins/

cp CI/librecad.svg appdir/usr/share/icons/hicolor/scalable/apps/
convert -resize 256x256 CI/librecad.svg appdir/usr/share/icons/hicolor/256x256/apps/librecad.png
mkdir -p appdir/usr/share/icons/hicolor/scalable/apps/
cp CI/librecad.svg appdir/usr/share/icons/hicolor/scalable/apps/

ls -l appdir/usr/share/icons/hicolor/256x256/apps/librecad.png
ls -l appdir/usr/share/icons/hicolor/scalable/apps/
ls -l appdir/usr/share/applications/librecad.desktop

export QMAKE=$(which qmake)
export EXTRA_QT_MODULES=svg

wget -c https://github.com/$(wget -q https://github.com/probonopd/go-appimage/releases/expanded_assets/continuous -O - | grep "appimagetool-.*-aarch64.AppImage" | head -n 1 | cut -d '"' -f 2)
chmod +x appimagetool-*.AppImage
wget https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-aarch64.AppImage
wget https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-aarch64.AppImage
chmod +x *.AppImage
#ARCH=aarch64 ./appimagetool-*.AppImage deploy appdir/usr/share/applications/librecad.desktop
ARCH=aarch64 ./linuxdeploy-plugin-qt-aarch64.AppImage --appdir appdir
ARCH=aarch64 ./linuxdeploy-aarch64.AppImage --appdir appdir -e appdir/usr/bin/librecad -d appdir/usr/share/applications/librecad.desktop
VERSION=`git describe --always` ARCH=aarch64 ./appimagetool-*.AppImage appdir/
