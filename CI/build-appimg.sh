#!/bin/bash
#

## script to build LibreCAD AppImage
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
cp unix/resources/plugins/*.so appdir/usr/lib/librecad/
cp -r unix/resources/qm appdir/usr/share/librecad/

cp desktop/librecad.desktop appdir/usr/share/applications/
cp desktop/org.librecad.librecad.appdata.xml appdir/usr/share/metainfo/

cp -r librecad/support/doc/* appdir/usr/share/doc/librecad/
cp -r librecad/support/fonts appdir/usr/share/librecad/
cp -r librecad/support/library appdir/usr/share/librecad/
cp -r librecad/support/patterns appdir/usr/share/librecad/

cp CI/librecad.svg appdir/usr/share/icons/hicolor/scalable/apps/
convert -resize 256x256 CI/librecad.svg appdir/usr/share/icons/hicolor/256x256/apps/librecad.png
	for q_lib in libLLVM-15 libOpenGL libelf libsensors libQt6 libQt6Core5Compat libQt6QuickControls2 libQt6StateMachine libQt6WebView libQt6Xml libQt6Svg libQt6DBus libQt6WaylandClient libQt6WaylandEglClientHwIntegration libxcb
	do
	for f in $(find ../Qt -iname "${q_lib}*.so*" -print)
	do
		cp -av $f appdir/usr/lib/$(basename $f)
		chmod 755 appdir/usr/lib/$(basename $f)
		#cp -Lv $f appdir/usr/lib/$(echo $(basename $f)|sed -e 's:\.so.*$::').so
	done
	done
find appdir/

wget -c https://github.com/$(wget -q https://github.com/probonopd/go-appimage/releases/expanded_assets/continuous -O - | grep "appimagetool-.*-x86_64.AppImage" | head -n 1 | cut -d '"' -f 2)
chmod +x appimagetool-*.AppImage
./appimagetool-*.AppImage deploy appdir/usr/share/applications/librecad.desktop
VERSION=`git describe --always` ./appimagetool-*.AppImage appdir/
#wget -q https://github.com/omergoktas/linuxdeployqt/releases/download/latest/linuxdeployqt-x86_64.AppImage
#chmod +x linuxdeployqt-x86_64.AppImage
#./linuxdeployqt-x86_64.AppImage appdir/usr/bin/librecad -qmake=$(which qmake6) -appimage
#./appimagetool-*.AppImage -s deploy appdir/usr/bin/librecad.desktop
#VERSION=`git describe --always` ./appimagetool-*.AppImage appdir/
