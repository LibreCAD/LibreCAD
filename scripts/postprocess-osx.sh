#!/bin/bash

RESOURCEDIR=Caduntu.app/Contents/Resources

# Postprocess for osx                                                                                                                                                
mkdir $RESOURCEDIR/fonts
mkdir $RESOURCEDIR/patterns
cp support/patterns/*.dxf $RESOURCEDIR/patterns
cp support/fonts/*.cxf $RESOURCEDIR/fonts    
