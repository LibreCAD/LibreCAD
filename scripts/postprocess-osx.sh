#!/bin/bash

THISDIR="`pwd`"
RESOURCEDIR="`pwd`/LibreCAD.app/Contents"
TSDIR="`pwd`/librecad/ts"
DOCDIR="`pwd`/librecad/support/doc"

# Generate Help Files
cd "$DOCDIR"
qcollectiongenerator LibreCADdoc.qhcp

cd "$THISDIR"

# Postprocess for osx
mkdir -p $RESOURCEDIR/Resources/fonts
mkdir -p $RESOURCEDIR/Resources/patterns
mkdir -p $RESOURCEDIR/Resources/doc
mkdir -p $RESOURCEDIR/PlugIns
cp librecad/support/patterns/*.dxf $RESOURCEDIR/Resources/patterns
cp librecad/support/fonts/*.lff $RESOURCEDIR/Resources/fonts
cp librecad/support/doc/*.qhc $RESOURCEDIR/Resources/doc
cp librecad/support/doc/*.qch $RESOURCEDIR/Resources/doc

cp -r /opt/local/share/qt4/plugins/sqldrivers $RESOURCEDIR/PlugIns/sqldrivers

# Generate translations
lrelease librecad/src/src.pro
mkdir -p $RESOURCEDIR/Resources/qm
 
# Go into translations directory
cd "$TSDIR"
for tf in *.qm
do
	cp $tf $RESOURCEDIR/Resources/qm/$tf
done

