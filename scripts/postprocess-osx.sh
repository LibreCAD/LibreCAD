#!/bin/bash

THISDIR="`pwd`"
RESOURCEDIR="`pwd`/LibreCAD.app/Contents/Resources"
TSDIR="`pwd`/ts"
DOCDIR="`pwd`/support/doc"

# Generate Help Files
cd "$DOCDIR"
qcollectiongenerator LibreCADdoc.qhcp

cd "$THISDIR"

# Postprocess for osx
mkdir -p $RESOURCEDIR/fonts
mkdir -p $RESOURCEDIR/patterns
mkdir -p $RESOURCEDIR/doc
cp support/patterns/*.dxf $RESOURCEDIR/patterns
cp support/fonts/*.cxf $RESOURCEDIR/fonts
cp support/doc/*.qhc $RESOURCEDIR/doc
cp support/doc/*.qch $RESOURCEDIR/doc
find support/library -type d -not -path "*.svn*"  | sed s/support// | xargs -IFILES  mkdir $RESOURCEDIR/FILES 
find support/library -type f -iname *.dxf -not -path "*.svn*"  | sed s/support// | xargs -IFILES  cp support/FILES $RESOURCEDIR/FILES 

# Generate translations
lrelease librecad.pro
mkdir -p $RESOURCEDIR/qm
 
# Go into translations directory
cd "$TSDIR"
for tf in *.qm
do
		cp $tf $RESOURCEDIR/qm/$tf
done

