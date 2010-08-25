#!/bin/bash

RESOURCEDIR=unix/resources

# Postprocess for unix
mkdir -p $RESOURCEDIR/fonts
mkdir -p $RESOURCEDIR/patterns
cp support/patterns/*.dxf $RESOURCEDIR/patterns
cp support/fonts/*.cxf $RESOURCEDIR/fonts
