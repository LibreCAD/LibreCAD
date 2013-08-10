#!/bin/sh

THISDIR="`pwd`"
LCDIR="${THISDIR}/librecad"
PIDIR="${THISDIR}/plugins"
RESOURCEDIR="${THISDIR}/unix/resources"
TSDIRLC="${LCDIR}/ts"
TSDIRPI="${PIDIR}/ts"
SPTDIR="${LCDIR}/support"
DOCDIR="${SPTDIR}/doc"

# Generate Help Files
cd "${DOCDIR}"
qcollectiongenerator LibreCADdoc.qhcp

cd "${THISDIR}"

# Postprocess for unix
mkdir -p "${RESOURCEDIR}"/fonts
mkdir -p "${RESOURCEDIR}"/patterns
mkdir -p "${RESOURCEDIR}"/doc
cp "${SPTDIR}"/patterns/*.dxf "${RESOURCEDIR}"/patterns
cp "${SPTDIR}"/fonts/*.lff* "${RESOURCEDIR}"/fonts
cp "${SPTDIR}"/doc/*.qch "${RESOURCEDIR}"/doc
cp "${SPTDIR}"/doc/*.qhc "${RESOURCEDIR}"/doc
find "${SPTDIR}"/library -type d | sed 's:^.*support/::' | xargs -IFILES  mkdir "${RESOURCEDIR}"/FILES
find "${SPTDIR}"/library -type f -iname *.dxf | sed 's/^.*support//' | xargs -IFILES  cp "${SPTDIR}"/FILES "${RESOURCEDIR}"/FILES

# Generate translations
lrelease "${LCDIR}"/src/src.pro
lrelease "${PIDIR}"/plugins.pro
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

