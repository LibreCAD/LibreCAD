#!/bin/bash
#

## script to build LibreCAD AppImage
## intended to be used on Host or Travis CI
## does not run on modern systems because of linuxdeployqt
## for testing it can be called with parameter 'clean' to remove AppImage file and folder

# ensure that the script is called from LibreCAD root folder and executable exists
# you can use: export LC_ROOT=/path-to-your-lc-root
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

VERSION_CODENAME=`cat /etc/os-release | grep ^VERSION_CODENAME= | sed -e 's/.*=//'`
VERSION_ID=`cat /etc/os-release | grep ^VERSION_ID= | sed -e 's/.*=//g' -e 's/"//g'`
ID=`cat /etc/os-release | grep ^ID= | sed -e 's/.*=//'`
GIT_VERSION=`git describe --always`
LC_VERSION=`cat ${LC_ROOT}librecad/src/src.pro | grep LC_VERSION -m 1 | sed -e 's/.*=//' -e 's/"//g'`-${GIT_VERSION}
APPIMAGE=LibreCAD-${LC_VERSION}-${ARCH}.AppImage
ZIP_FILES="${APPIMAGE} librecad.desktop share install.sh uninstall.sh"
ZIPPED=LibreCAD-${LC_VERSION}_${ID}-${VERSION_ID}_${VERSION_CODENAME}-${DEB_ARCH}.AppImage.zip

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
sed -i -e "s/Exec=librecad/Exec=k=\`echo %k\` \&\& cd \`dirname \$k\` \&\& \.\/${APPIMAGE}/" -e "/TryExec.*/d" librecad.desktop

# add setup for user install AppImage
mkdir -p share/doc
mkdir -p share/icons/hicolor/32x32/mimetypes
mkdir -p share/icons/hicolor/16x16/mimetypes
mkdir -p share/icons/hicolor/scalable/mimetypes

cp -r appdir/usr/share/applications share
cp -r appdir/usr/share/icons share
cp -r appdir/usr/share/librecad share
cp -r appdir/usr/share/metainfo share
cp -r appdir/usr/share/doc/librecad share/doc

# add icons for user setup AppImage
git clone https://github.com/emanuel4you/LibreCAD-Icons.git icons
convert -resize 16x16 icons/mimetypes/16/image-vnd-dwg_16.svg share/icons/hicolor/16x16/mimetypes/image-vnd-dwg.png
convert -resize 16x16 icons/mimetypes/16/image-vnd-dxf_16.svg share/icons/hicolor/16x16/mimetypes/image-vnd-dxf.png
convert -resize 16x16 icons/mimetypes/16/text-x-common-lisp_16.svg share/icons/hicolor/16x16/mimetypes/text-x-common-lisp.png
convert -resize 16x16 icons/mimetypes/16/text-x-dcl-script_16.svg share/icons/hicolor/16x16/mimetypes/text-x-dcl-script.png
convert -resize 32x32 icons/mimetypes/32/image-vnd-dwg_32.svg share/icons/hicolor/32x32/mimetypes/image-vnd-dwg.png
convert -resize 32x32 icons/mimetypes/32/image-vnd-dxf_32.svg share/icons/hicolor/32x32/mimetypes/image-vnd-dxf.png
convert -resize 32x32 icons/mimetypes/32/text-x-common-lisp_32.svg share/icons/hicolor/32x32/mimetypes/text-x-common-lisp.png
convert -resize 32x32 icons/mimetypes/32/text-x-dcl-script_32.svg share/icons/hicolor/32x32/mimetypes/text-x-dcl-script.png
cp -R icons/mimetypes/32/* share/icons/hicolor/scalable/mimetypes
cp -R icons/mimetypes/16/* share/icons/hicolor/scalable/mimetypes
rm -Rf icons

sed -i -e "s/Exec=librecad/Exec=bash -c \"${APPIMAGE}\"/" -e "/TryExec.*/d" share/applications/librecad.desktop

cat << EOF >> install.sh
#!/bin/bash
#

## script to install LibreCAD AppImage
#
# to change install locations:
#
# export INSTALL_SHARE=/path-to-config
# export INSTALL_APP=/path-to-bin

INSTALL_SHARE=~/home/.local
INSTALL_APP=/usr/local/bin

if [ ! -d "share" ]; then
    echo "The script has to be called from LibreCAD AppImage root folder!"
    exit
fi

sudo echo || exit

mkdir -p \${INSTALL_SHARE}
cp -r share \${INSTALL_SHARE}
sudo mkdir -p \${INSTALL_APP}

echo "#!/bin/bash" > /tmp/${APPIMAGE}
echo "#" >> /tmp/${APPIMAGE}
echo "cd \`pwd\`" >> /tmp/${APPIMAGE}
echo "\`pwd\`/${APPIMAGE}" >> /tmp/${APPIMAGE}
chmod a+x /tmp/${APPIMAGE}
sudo mv /tmp/${APPIMAGE} \${INSTALL_APP}/${APPIMAGE}

echo "installed \${INSTALL_APP}/${APPIMAGE}"
exit
EOF

cat << EOF >> uninstall.sh
#!/bin/bash
#

## script to uninstall LibreCAD AppImage
#
# to change install locations:
#
# export INSTALL_SHARE=/path-to-config
# export INSTALL_APP=/path-to-bin

INSTALL_SHARE=~/home/.local
INSTALL_APP=/usr/local/bin

sudo echo || exit

[ -d "\${INSTALL_SHARE}/share/doc/librecad" ] && rm -Rf \${INSTALL_SHARE}/share/doc/librecad
[ -f "\${INSTALL_SHARE}/share/metainfo/org.librecad.librecad.appdata.xml" ] && rm -f \${INSTALL_SHARE}/share/metainfo/org.librecad.librecad.appdata.xml
[ -f "\${INSTALL_SHARE}/share/applications/librecad.desktop" ] && rm -f \${INSTALL_SHARE}/share/applications/librecad.desktop
[ -f "\${INSTALL_SHARE}/share/icons/hicolor/256x256/apps/librecad.png" ] && rm -f \${INSTALL_SHARE}/share/icons/hicolor/256x256/apps/librecad.png
[ -f "\${INSTALL_SHARE}/share/icons/hicolor/scalable/apps/librecad.svg" ] && rm -f \${INSTALL_SHARE}/share/icons/hicolor/scalable/apps/librecad.svg
[ -f "\${INSTALL_SHARE}/share/icons/hicolor/16x16/mimetypes/image-vnd-dwg.png" ] && rm -f \${INSTALL_SHARE}/share/icons/hicolor/16x16/mimetypes/image-vnd-dwg.png
[ -f "\${INSTALL_SHARE}/share/icons/hicolor/16x16/mimetypes/image-vnd-dxf.png" ] && rm -f \${INSTALL_SHARE}/share/icons/hicolor/16x16/mimetypes/image-vnd-dxf.png
[ -f "\${INSTALL_SHARE}/share/icons/hicolor/16x16/mimetypes/text-x-common-lisp.png" ] && rm -f \${INSTALL_SHARE}/share/icons/hicolor/16x16/mimetypes/text-x-common-lisp.png
[ -f "\${INSTALL_SHARE}/share/icons/hicolor/16x16/mimetypes/text-x-dcl-script.png" ] && rm -f \${INSTALL_SHARE}/share/icons/hicolor/16x16/mimetypes/text-x-dcl-script.png
[ -f "\${INSTALL_SHARE}/share/icons/hicolor/32x32/mimetypes/image-vnd-dwg.png" ] && rm -f \${INSTALL_SHARE}/share/icons/hicolor/32x32/mimetypes/image-vnd-dwg.png
[ -f "\${INSTALL_SHARE}/share/icons/hicolor/32x32/mimetypes/image-vnd-dxf.png" ] && rm -f \${INSTALL_SHARE}/share/icons/hicolor/32x32/mimetypes/image-vnd-dxf.png
[ -f "\${INSTALL_SHARE}/share/icons/hicolor/32x32/mimetypes/text-x-common-lisp.png" ] && rm -f \${INSTALL_SHARE}/share/icons/hicolor/32x32/mimetypes/text-x-common-lisp.png
[ -f "\${INSTALL_SHARE}/share/icons/hicolor/32x32/mimetypes/text-x-dcl-script.png" ] && rm -f \${INSTALL_SHARE}/share/icons/hicolor/32x32/mimetypes/text-x-dcl-script.png
[ -f "\${INSTALL_SHARE}/share/icons/hicolor/scalable/mimetypes/image-vnd-dwg_16.svg" ] && rm -f \${INSTALL_SHARE}/share/icons/hicolor/scalable/mimetypes/image-vnd-dwg_16.svg
[ -f "\${INSTALL_SHARE}/share/icons/hicolor/scalable/mimetypes/image-vnd-dxf_16.svg" ] && rm -f \${INSTALL_SHARE}/share/icons/hicolor/scalable/mimetypes/image-vnd-dxf_16.svg
[ -f "\${INSTALL_SHARE}/share/icons/hicolor/scalable/mimetypes/text-x-common-lisp_16.svg" ] && rm -f \${INSTALL_SHARE}/share/icons/hicolor/scalable/mimetypes/text-x-common-lisp_16.svg
[ -f "\${INSTALL_SHARE}/share/icons/hicolor/scalable/mimetypes/text-x-dcl-script_16.svg" ] && rm -f \${INSTALL_SHARE}/share/icons/hicolor/scalable/mimetypes/text-x-dcl-script_16.svg
[ -f "\${INSTALL_SHARE}/share/icons/hicolor/scalable/mimetypes/image-vnd-dwg_32.svg" ] && rm -f \${INSTALL_SHARE}/share/icons/hicolor/scalable/mimetypes/image-vnd-dwg_32.svg
[ -f "\${INSTALL_SHARE}/share/icons/hicolor/scalable/mimetypes/image-vnd-dxf_32.svg" ] && rm -f \${INSTALL_SHARE}/share/icons/hicolor/scalable/mimetypes/image-vnd-dxf_32.svg
[ -f "\${INSTALL_SHARE}/share/icons/hicolor/scalable/mimetypes/text-x-common-lisp_32.svg" ] && rm -f \${INSTALL_SHARE}/share/icons/hicolor/scalable/mimetypes/text-x-common-lisp_32.svg
[ -f "\${INSTALL_SHARE}/share/icons/hicolor/scalable/mimetypes/text-x-dcl-script_32.svg" ] && rm -f \${INSTALL_SHARE}/share/icons/hicolor/scalable/mimetypes/text-x-dcl-script_32.svg
[ -f "\${INSTALL_APP}/\${APPIMAGE}" ] && sudo rm -f \${INSTALL_APP}/\${APPIMAGE}

echo "uninstalled \${INSTALL_APP}/${APPIMAGE}"
exit
EOF

# add README.md
cat << EOF >> README.md
# LibreCAD ${LC_VERSION} AppImage ($ID)
${ZIPPED}

## To run AppImage, simply:
Extract
\`\`\`bash
$ unzip ${ZIPPED}
\`\`\`
Make it executable
\`\`\`bash
$ chmod a+x ${APPIMAGE}
\`\`\`
click on librecad.desktop, ${APPIMAGE} or exec in Terminal:
\`\`\`bash
$ ./${APPIMAGE}
\`\`\`

## To Install to /usr/local/bin, ~/home/.local and
extracted ${ZIPPED} e.g. to ~/home and open terminal in root folder of the AppImage
\`\`\`bash
$ chmod a+x install.sh
$ sudo install.sh
\`\`\`

## To Install to custom locations
extracted ${ZIPPED} e.g. to ~/home and open terminal in root folder of the AppImage
\`\`\`bash
$ sudo export INSTALL_SHARE=/path-to-config
$ sudo export INSTALL_APP=/path-to-bin
$ chmod a+x install.sh
$ sudo install.sh
\`\`\`

## To Uninstall from /usr/local/bin, ~/home/.local and
extracted ${ZIPPED} e.g. to ~/home and open terminal in root folder of the AppImage
\`\`\`bash
$ chmod a+x uninstall.sh
$ sudo uninstall.sh
\`\`\`

## To Uninstall from custom locations
extracted ${ZIPPED} e.g. to ~/home and open terminal in root folder of the AppImage
\`\`\`bash
$ sudo export INSTALL_SHARE=/path-to-config
$ sudo export INSTALL_APP=/path-to-bin
$ chmod a+x uninstall.sh
$ sudo uninstall.sh
\`\`\`

## Custom icons:
to use for librecad.desktop,
dwg, dxf, lisp, dcl mimetypes.
Mimetypes can be easily added via most file browsers.
Add text file bla.lisp, select file setting and change Common Lisp Source's icon.
(see: share/icons/hicolor/scalable, share/icons/hicolor/16x16/mimetypes, share/icons/hicolor/32x32/mimetypes, \$INSTALL_SHARE)

EOF

# test if developer build
if [ -f "unix/librecad.py" ]; then
    cp unix/librecad.py .
    git clone https://github.com/emanuel4you/LibreCAD-Developer-Examples.git test
    rm -Rf test/.*
    ZIP_FILES="${ZIP_FILES} test librecad.py README.md"
    echo "" >> README.md
    echo "## Developer build:" >> README.md
    echo "Examples folder test has to be in ${APPIMAGE}'s root directory" >> README.md
    echo "see test/README.md for testing LibreLisp, LibrePython, LibreDcl+" >> README.md
fi

# add marcdown newline
sed -i -e 's/$/  /g' README.md

zip -mr9 ${ZIPPED} ${ZIP_FILES}
