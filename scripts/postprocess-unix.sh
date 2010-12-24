#!/bin/bash

RESOURCEDIR="`pwd`/unix/resources"
TSDIR="`pwd`/ts"

# Postprocess for unix
mkdir -p $RESOURCEDIR/fonts
mkdir -p $RESOURCEDIR/patterns
cp support/patterns/*.dxf $RESOURCEDIR/patterns
cp support/fonts/*.cxf $RESOURCEDIR/fonts
find support/library -type d -not -path "*.svn*"  | sed s/support// | xargs -IFILES  mkdir $RESOURCEDIR/FILES
find support/library -type f -iname *.dxf -not -path "*.svn*"  | sed s/support// | xargs -IFILES  cp support/FILES $RESOURCEDIR/FILES

# Generate translations
lrelease librecad.pro
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
