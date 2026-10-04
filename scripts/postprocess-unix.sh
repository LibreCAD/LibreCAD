#!/bin/sh

THISDIR="`pwd`"
LCDIR="${THISDIR}/librecad"
PIDIR="${THISDIR}/plugins"
RESOURCEDIR="${THISDIR}/unix/resources"
APPDATADIR="${THISDIR}/unix/appdata"
TSDIRLC="${LCDIR}/ts"
TSDIRPI="${PIDIR}/ts"
SPTDIR="${LCDIR}/support"
DESKTOPDIR="${THISDIR}/desktop"
LRELEASE="lrelease"
# Qt's own tool when qmake passes its bin directory: distributions name the
# one on PATH differently (lrelease-qt6, lrelease6) or not at all.
[ -x "${1}/lrelease" ] && LRELEASE="${1}/lrelease"
command -v "${LRELEASE}" >/dev/null 2>&1 ||
        echo "WARNING: lrelease not found - translations will not be generated" >&2

cd "${THISDIR}"

# Postprocess for unix
mkdir -p "${RESOURCEDIR}"/fonts
mkdir -p "${RESOURCEDIR}"/patterns
if [ -d "${SPTDIR}/patterns" ]; then
        find "${SPTDIR}/patterns" -maxdepth 1 -type f -name '*.dxf' \
                -exec cp {} "${RESOURCEDIR}/patterns" \;
fi
cp "${SPTDIR}"/fonts/*.lff* "${RESOURCEDIR}"/fonts
if [ -d "${SPTDIR}/library" ]; then
        find "${SPTDIR}"/library -type d | sed 's:^.*support/::' | xargs -IFILES  mkdir -p "${RESOURCEDIR}"/FILES
        find "${SPTDIR}"/library -type f -iname "*.dxf" | sed 's/^.*support//' | xargs -IFILES  cp "${SPTDIR}"/FILES "${RESOURCEDIR}"/FILES
fi

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

# copy desktop and appdata files to unix/appdata/
mkdir -p "${APPDATADIR}"
cp "${DESKTOPDIR}"/librecad.desktop "${APPDATADIR}"/
cp "${DESKTOPDIR}"/org.librecad.librecad.appdata.xml "${APPDATADIR}"/
