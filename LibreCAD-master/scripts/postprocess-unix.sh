#!/bin/sh

THISDIR="`pwd`"
LCDIR="${THISDIR}/librecad"
PIDIR="${THISDIR}/plugins"
RESOURCEDIR="${THISDIR}/unix/resources"
APPDATADIR="${THISDIR}/unix/appdata"
TSDIRLC="${LCDIR}/ts"
TSDIRPI="${PIDIR}/ts"
SPTDIR="${LCDIR}/support"
LRELEASE="lrelease"

cd "${THISDIR}"

# Postprocess for unix
mkdir -p "${RESOURCEDIR}"/fonts
mkdir -p "${RESOURCEDIR}"/patterns
cp "${SPTDIR}"/patterns/*.dxf "${RESOURCEDIR}"/patterns
cp "${SPTDIR}"/fonts/*.lff* "${RESOURCEDIR}"/fonts
find "${SPTDIR}"/library -type d | sed 's:^.*support/::' | xargs -IFILES  mkdir -p "${RESOURCEDIR}"/FILES
find "${SPTDIR}"/library -type f -iname *.dxf | sed 's/^.*support//' | xargs -IFILES  cp "${SPTDIR}"/FILES "${RESOURCEDIR}"/FILES

# Generate translations
${LRELEASE} "${LCDIR}"/src/src.pro
${LRELEASE} "${PIDIR}"/plugins.pro
mkdir -p "${RESOURCEDIR}"/qm

# Go into translations directory
cd "${TSDIRLC}"
for tf in *.qm
do
        cp "${tf}" "${RESOURCEDIR}/qm/${tf}"
done

cd "${TSDIRPI}"
for tf in *.qm
do
        cp "${tf}" "${RESOURCEDIR}/qm/${tf}"
done

# copy appdata.xml to unix/appdata/librecad.appdata.xml
mkdir -p "${APPDATADIR}"
cp "${SPTDIR}"/librecad.appdata.xml "${APPDATADIR}"/
