#!/bin/bash

THISDIR="`pwd`"
RESOURCEDIR="`pwd`/LibreCAD.app/Contents"
TSDIRLC="`pwd`/librecad/ts"
TSDIRPI="`pwd`/plugins/ts"
LRELEASE="lrelease"
if [ -z "$(which lrelease)" ] && [ -x "/opt/local/libexec/qt5/bin/lrelease" ]
then
	LRELEASE="/opt/local/libexec/qt5/bin/lrelease"
fi

cd "$THISDIR"

# Postprocess for osx
mkdir -p $RESOURCEDIR/Resources/fonts
mkdir -p $RESOURCEDIR/Resources/patterns
mkdir -p $RESOURCEDIR/PlugIns
cp librecad/support/patterns/*.dxf $RESOURCEDIR/Resources/patterns
cp librecad/support/fonts/*.lff $RESOURCEDIR/Resources/fonts

# Generate translations
$LRELEASE librecad/src/src.pro
$LRELEASE plugins/plugins.pro
mkdir -p $RESOURCEDIR/Resources/qm

# Go into translations directory
cd "$TSDIRLC"
for tf in *.qm
do
	cp $tf $RESOURCEDIR/Resources/qm/$tf
done

cd "$TSDIRPI"
for tf in *.qm
do
	cp $tf $RESOURCEDIR/Resources/qm/$tf
done
