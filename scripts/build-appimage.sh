#!/bin/bash
#
# Build LibreCAD AppImage
#

set -e
set -x

ARCH=$(arch)

# LibreCAD (LC_) directories
LC_ROOTDIR=$(dirname "$0")
LC_ROOTDIR=$(realpath "$LC_ROOTDIR")
LC_UNIXDIR="$LC_ROOTDIR/../unix"
LC_DESKTOP="$LC_ROOTDIR/../desktop"

# AppImage (AI_) directories
AI_ROOTDIR="$LC_ROOTDIR/../AppImage"
AI_TOOLDIR="$AI_ROOTDIR/tools"
AI_APP_DIR="$AI_ROOTDIR/AppDir"

# linuxdeployqt tool (LT_)
LT_VERSION="6"
LT_TOOLIMG="linuxdeployqt-$LT_VERSION-$ARCH.AppImage"
LT_LINKURL="https://github.com/probonopd/linuxdeployqt/releases/download"
LT_LINKURL="$LT_LINKURL/$LT_VERSION/$LT_TOOLIMG"


# Version for this AppImage
export VERSION=$(git describe --tags)


# Clean old AppDir if it exists
[ -d "$AI_APP_DIR" ] && rm -rf "$AI_APP_DIR"

# Create directories
mkdir -p "$AI_ROOTDIR" "$AI_TOOLDIR" "$AI_APP_DIR"
mkdir -p "$AI_APP_DIR/usr/bin"

# Copy LibreCAD files
cp -a "$LC_UNIXDIR"/* "$AI_APP_DIR/usr/bin/"

# Strip binaries
find "$AI_APP_DIR/usr/bin/" -type f -perm /a+x -exec strip {} \;

# Desktop file
cp -a "$LC_DESKTOP/librecad.desktop" "$AI_APP_DIR/"

# linuxdeployqt tool refuses to process this desktop file because
# of a missing ';'. Fixing this.
sed -i -e 's|^MimeType=image/vnd.dxf$|&;|' "$AI_APP_DIR/librecad.desktop"

# Icon file
cp -a "$LC_DESKTOP/graphics_icons_and_splash/Icon LibreCAD/Icon_Librecad.svg" \
    "$AI_APP_DIR/librecad.svg"


# Create AppRun file
cat << EOF > "$AI_APP_DIR/AppRun"
#!/bin/sh
#
# AppRun file for LibreCAD AppImage. It allows running either librecad
# or ttf2lff binaries. Command line arguments are also supported.
#
# To see LibreCAD arguments list, run:
#
# ./LibreCAD-$VERSION-$ARCH.AppImage --help
#
#
# To see ttf2lff arguments list, run:
#
# ./LibreCAD-$VERSION-$ARCH.AppImage ttf2lff --help
#
#
# To see dxf2pdf arguments list (part of librecad binary), run:
#
# ./LibreCAD-$VERSION-$ARCH.AppImage dxf2pdf --help
#

if [ "ttf2lff" = "\$1" ]; then
    shift
    exec \$APPDIR/usr/bin/ttf2lff \$@
else
    exec \$APPDIR/usr/bin/librecad \$@
fi
EOF
chmod +x "$AI_APP_DIR/AppRun"


# Get linuxdeployqt tool if it doesn't exist already
if [ ! -e "$AI_TOOLDIR/$LT_TOOLIMG" ]; then
    wget -P "$AI_TOOLDIR/" "$LT_LINKURL"
    chmod +x "$AI_TOOLDIR/$LT_TOOLIMG"
fi


# Build AppImage
pushd "$AI_ROOTDIR" >/dev/null
"$AI_TOOLDIR/$LT_TOOLIMG" \
    "$AI_APP_DIR/usr/bin/librecad" \
    -appimage \
    -bundle-non-qt-libs
popd >/dev/null
echo 'Done.'

