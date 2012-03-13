#!/bin/sh

THISDIR="`pwd`"
LCDIR="${THISDIR}/librecad"
RESOURCEDIR="${THISDIR}/unix/resources"
TSDIR="${LCDIR}/ts"
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
find "${SPTDIR}"/library -type d | sed 's:^.*support/::' | xargs -IFILES  mkdir "${RESOURCEDIR}"/FILES
find "${SPTDIR}"/library -type f -iname *.dxf | sed 's/^.*support//' | xargs -IFILES  cp "${SPTDIR}"/FILES "${RESOURCEDIR}"/FILES

# Generate translations
lrelease "${LCDIR}"/src/src.pro
mkdir -p "${RESOURCEDIR}"/qm
 
# Go into translations directory
cd "${TSDIR}"
for tf in *.qm
do
        cp "${tf}" "${RESOURCEDIR}/qm/${tf}"
done

