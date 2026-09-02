#!/bin/bash

# sh path/to/script <.app directory for output bundle> <path to qt bin>

CONTENTSDIR="$1/Contents"
LRELEASE="$2/lrelease"
SCRIPTDIR="$(dirname $0)"

RESOURCEDIR="$CONTENTSDIR/Resources"
TSDIRLC="$SCRIPTDIR/../librecad/ts"
TSDIRPI="$SCRIPTDIR/../plugins/ts"

# Postprocess for osx
mkdir -p "$RESOURCEDIR/fonts"
mkdir -p "$RESOURCEDIR/patterns"
mkdir -p "$RESOURCEDIR/library"
mkdir -p "$CONTENTSDIR/PlugIns"
if [ -d "$SCRIPTDIR/../librecad/support/patterns" ]; then
	find "$SCRIPTDIR/../librecad/support/patterns" -maxdepth 1 -type f -name '*.dxf' \
		-exec cp {} "$RESOURCEDIR/patterns/" \;
fi
cp "$SCRIPTDIR/../librecad/support/fonts/"*.lff "$RESOURCEDIR/fonts/"
if [ -d "$SCRIPTDIR/../librecad/support/library" ]; then
	cp -r "$SCRIPTDIR/../librecad/support/library/" "$RESOURCEDIR/library/"
fi

if [ -x $LRELEASE ]
then
	# Generate translations
	$LRELEASE "$SCRIPTDIR/../librecad/src/src.pro"
	$LRELEASE "$SCRIPTDIR/../plugins/plugins.pro"
	mkdir -p "$RESOURCEDIR/qm"

	for tf in "$TSDIRLC/"*.qm
	do
		mv "$tf" "$RESOURCEDIR/qm/$(basename $tf)"
	done

	for tf in "$TSDIRPI/"*.qm
	do
		mv "$tf" "$RESOURCEDIR/qm/$(basename $tf)"
	done
else
	echo "WARNING: lrelease not found - Translations will not be generated"
fi
