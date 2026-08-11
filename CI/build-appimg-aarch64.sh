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
mkdir -p appdir/usr/lib/aarch64-linux-gnu/qt6
echo "copying Qt6 plugins"
export QPA_PLUGIN_FOLDER="$(find /usr/lib/aarch64-linux-gnu/qt6/ -type d -name plugins -print)"
# See CI/build-appimg.sh for the full rationale, including which categories
# appimagetool re-deploys wholesale regardless of what is staged here. In
# short: the blanket copy pulled in libraries nothing in LibreCAD ever opens
# (GTK3+Cairo+Pango+ATK, QML/Quick, the Wayland compositor side), and its
# GTK3 platform theme plugin supplies its own font settings on GTK desktops.
LC_QT_PLUGINS=(
    platforms/libqxcb.so
    platforms/libqwayland-egl.so
    platforms/libqwayland-generic.so
    platforms/libqminimal.so
    platforms/libqoffscreen.so
    xcbglintegrations/libqxcb-glx-integration.so
    xcbglintegrations/libqxcb-egl-integration.so
    platforminputcontexts/libcomposeplatforminputcontextplugin.so
    platforminputcontexts/libibusplatforminputcontextplugin.so
    iconengines/libqsvgicon.so
    imageformats/libqgif.so
    imageformats/libqico.so
    imageformats/libqjpeg.so
    imageformats/libqsvg.so
    imageformats/libqtiff.so
    tls/libqopensslbackend.so
    tls/libqcertonlybackend.so
    printsupport/libcupsprintersupport.so
    wayland-decoration-client/libbradient.so
    wayland-graphics-integration-client/libqt-plugin-wayland-egl.so
    wayland-shell-integration/libxdg-shell.so
)
for plugin in "${LC_QT_PLUGINS[@]}"; do
    src="${QPA_PLUGIN_FOLDER}/${plugin}"
    if [ -f "${src}" ]; then
        install -D "${src}" "appdir/usr/lib/aarch64-linux-gnu/qt6/plugins/${plugin}"
    else
        echo "warning: expected Qt6 plugin not found: ${src}" >&2
    fi
done
cp -r appdir/usr/lib/aarch64-linux-gnu/qt6/plugins/platforms appdir/usr/bin/
echo "copying xcb-cursor library"
find /usr/lib -name "libxcb-cursor.so*" -exec cp -L {} appdir/usr/lib/ \;

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
mkdir -p appdir/usr/share/icons/hicolor/scalable/apps/
cp CI/librecad.svg appdir/usr/share/icons/hicolor/scalable/apps/

ls -l appdir/usr/share/icons/hicolor/256x256/apps/librecad.png
ls -l appdir/usr/share/icons/hicolor/scalable/apps/
ls -l appdir/usr/share/applications/librecad.desktop

export QMAKE=$(which qmake6)
export EXTRA_QT_MODULES=svg

wget -c https://github.com/$(wget -q https://github.com/probonopd/go-appimage/releases/expanded_assets/continuous -O - | grep "appimagetool-.*-aarch64.AppImage" | head -n 1 | cut -d '"' -f 2)
chmod +x appimagetool-*.AppImage
wget https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-aarch64.AppImage
chmod +x *.AppImage
# Bundle EVERYTHING
VERSION=`git describe --always` ARCH=aarch64 ./appimagetool-*.AppImage -s deploy appdir/usr/share/applications/*.desktop
VERSION=`git describe --always` ./appimagetool-*.AppImage ./appdir
chmod +x *.AppImage

