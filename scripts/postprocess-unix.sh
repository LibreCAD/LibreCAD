#!/bin/bash

RESOURCEDIR="`pwd`/unix/resources"
TSDIR="`pwd`/ts"

# Postprocess for unix
mkdir -p $RESOURCEDIR/fonts
mkdir -p $RESOURCEDIR/patterns
cp support/patterns/*.dxf $RESOURCEDIR/patterns
cp support/fonts/*.cxf $RESOURCEDIR/fonts

# Generate translations
lrelease caduntu.pro
mkdir -p $RESOURCEDIR/qm
 
# Go into translations directory
cd "$TSDIR"
for dir in actions cmd lib main ui
do 
    cd $dir
    for tf in *.qm
    do
		cp $tf $RESOURCEDIR/qm/$tf
    done
    
    cd "$TSDIR" 
done
