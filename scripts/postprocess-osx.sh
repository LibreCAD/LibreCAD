#!/bin/bash

THISDIR="`pwd`"
RESOURCEDIR="`pwd`/LibreCAD.app/Contents"
TSDIR="`pwd`/ts"
DOCDIR="`pwd`/support/doc"

# Generate Help Files
cd "$DOCDIR"
qcollectiongenerator LibreCADdoc.qhcp

cd "$THISDIR"

# Postprocess for osx
mkdir -p $RESOURCEDIR/Resources/fonts
mkdir -p $RESOURCEDIR/Resources/patterns
mkdir -p $RESOURCEDIR/Resources/doc
mkdir -p $RESOURCEDIR/PlugIns
cp support/patterns/*.dxf $RESOURCEDIR/Resources/patterns
cp support/fonts/*.lff $RESOURCEDIR/Resources/fonts
cp support/doc/*.qhc $RESOURCEDIR/Resources/doc
cp support/doc/*.qch $RESOURCEDIR/Resources/doc

cp -r /Developer/Applications/Qt/plugins/sqldrivers $RESOURCEDIR/PlugIns/sqldrivers

# Generate translations
lrelease src/src.pro
mkdir -p $RESOURCEDIR/Resources/qm
 
# Go into translations directory
cd "$TSDIR"
for tf in *.qm
do
	cp $tf $RESOURCEDIR/Resources/qm/$tf
done

