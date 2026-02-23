#!/bin/sh

THISDIR="`pwd`"
LC_DIR="${THISDIR}/librecad"
PL_DIR="${THISDIR}/plugins"
RESOURCEDIR="${THISDIR}/unix/resources"
APPDATADIR="${THISDIR}/unix/appdata"
DESKTOPDIR="${THISDIR}/desktop"
# For Qt6
LUPDATE="/usr/lib/qt6/bin/lupdate"
LRELEASE="/usr/lib/qt6/bin/lrelease"
# For Qt5
#LUPDATE="lupdate"
#LRELEASE="lrelease"

# Create dirs for unix directory
mkdir -p -- "${RESOURCEDIR}"/fonts "${RESOURCEDIR}"/patterns "${RESOURCEDIR}"/qm

# Copy files to unix directory
echo -n "Copy fonts to ${RESOURCEDIR}/fonts: "
cp "${LC_DIR}"/support/fonts/*.lff "${RESOURCEDIR}"/fonts
echo "Ok."

echo -n "Copy library to ${RESOURCEDIR}/library: "
cp -r "${LC_DIR}"/support/library "${RESOURCEDIR}"
echo "Ok."

echo -n "Copy patterns to ${RESOURCEDIR}/patterns: "
cp "${LC_DIR}"/support/patterns/*.dxf "${RESOURCEDIR}"/patterns
echo "Ok."

# Update translations
echo -n "Update translations from .cpp to .ts: "
${LUPDATE} -silent librecad/ -extensions cpp,ui -ts librecad/ts/*.ts
${LUPDATE} -silent plugins/ -extensions cpp,ui -ts plugins/ts/*.ts
echo "Ok."

echo -n "Generate translations from .ts to ${RESOURCEDIR}/qm/*.qm: "
${LRELEASE} -silent librecad/ts/*.ts
mv librecad/ts/*.qm ${RESOURCEDIR}/qm
${LRELEASE} -silent plugins/ts/*.ts
mv plugins/ts/*.qm ${RESOURCEDIR}/qm
echo "Ok."

# copy desktop and appdata files to unix/appdata/
mkdir -p "${APPDATADIR}"
cp "${DESKTOPDIR}"/librecad.desktop "${APPDATADIR}"/
cp "${DESKTOPDIR}"/org.librecad.librecad.appdata.xml "${APPDATADIR}"/
