#!/bin/bash -x
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
mkdir -p appdir/usr/share/librecad/qm
mkdir -p appdir/usr/lib/x86_64-linux-gnu/qt6
echo "copying Qt6 plugins"
export QPA_PLUGIN_FOLDER="$(find /usr/lib/x86_64-linux-gnu/qt6/ -type d -name plugins -print)"
# Only the plugin families LibreCAD actually loads at runtime: X11 and Wayland
# QPA backends (desktop + native Wayland support, #2100), IME/compose input,
# SVG icons, the raster formats surfaced in the image-import dialog, the TLS
# backend the release-checker's HTTPS request needs, and CUPS printing.
#
# The previous `rsync -Par ${QPA_PLUGIN_FOLDER}` copied the *entire* system
# Qt6 plugins tree, most of which is unrelated to anything LibreCAD links
# (verified against CMakeLists.txt: only Core/Gui/Widgets/PrintSupport/Svg/
# Network). appimagetool's deploy step then bundled every *transitive*
# shared library those extra plugins pull in - GTK3 with its full Cairo/
# Pango/ATK/AT-SPI stack (platformthemes/libqgtk3.so), the QML/Quick engine
# (qmltooling/*), and the Wayland *compositor* side (LibreCAD is a client,
# never a compositor). Bundling libqgtk3.so also lets Qt auto-select the
# GTK3 platform theme on GTK desktops, which supplies its own font settings
# instead of Qt's - the font half of #2734.
#
# Note that appimagetool re-deploys much of this from the system Qt no
# matter what is staged here: given QtGui it copies all of iconengines/,
# imageformats/ and platforminputcontexts/, given QtGui or libxcb-glx all
# of xcbglintegrations/, and given QtNetwork all of tls/. It also picks up
# printsupport/libcupsprintersupport.so and platforms/libqxcb.so - but
# nothing else out of platforms/. So the entries that genuinely have to be
# staged here are the non-xcb platform plugins (wayland, minimal,
# offscreen) and the wayland-* integration plugins, without which native
# Wayland support is lost; the rest are belt-and-braces. platformthemes/
# is never re-deployed, which is precisely why dropping the blanket copy
# is what removes libqgtk3.so.
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
        install -D "${src}" "appdir/usr/lib/x86_64-linux-gnu/qt6/plugins/${plugin}"
    else
        echo "warning: expected Qt6 plugin not found: ${src}" >&2
    fi
done
cp -r appdir/usr/lib/x86_64-linux-gnu/qt6/plugins/platforms appdir/usr/bin/
echo "copying xcb-cursor library"
find /usr/lib -name "libxcb-cursor.so*" -exec cp -L {} appdir/usr/lib/ \;

# strip binaries
strip unix/librecad
strip unix/resources/plugins/*/*.so

# copy executables and binary resources
cp unix/librecad appdir/usr/bin/
cp unix/resources/plugins/*/*.so appdir/usr/lib/librecad/
cp -r unix/*.qm appdir/usr/share/librecad/qm/

cp desktop/librecad.desktop appdir/usr/share/applications/
cp desktop/org.librecad.librecad.appdata.xml appdir/usr/share/metainfo/

cp -r librecad/support/doc/* appdir/usr/share/doc/librecad/
cp -r librecad/support/fonts appdir/usr/share/librecad/
if [ -d librecad/support/library ]; then
    cp -r librecad/support/library appdir/usr/share/librecad/
fi
if [ -d librecad/support/patterns ]; then
    cp -r librecad/support/patterns appdir/usr/share/librecad/
fi

cp CI/librecad.svg appdir/usr/share/icons/hicolor/scalable/apps/
convert -resize 256x256 CI/librecad.svg appdir/usr/share/icons/hicolor/256x256/apps/librecad.png

wget -c https://github.com/$(wget -q https://github.com/probonopd/go-appimage/releases/expanded_assets/continuous -O - | grep "appimagetool-.*-x86_64.AppImage" | head -n 1 | cut -d '"' -f 2)
chmod +x appimagetool-*.AppImage
# Bundle EVERYTHING
VERSION=`git describe --always` ARCH=x86_64 ./appimagetool-*.AppImage -s deploy appdir/usr/share/applications/*.desktop
VERSION=`git describe --always` ./appimagetool-*.AppImage ./appdir
chmod +x *.AppImage
