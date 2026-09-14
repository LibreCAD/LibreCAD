#!/bin/bash

# ********************************************************************************
# This file is part of the LibreCAD project, a 2D CAD program
#
# Copyright (C) 2011-2026 LibreCAD.org
# Copyright (C) 2011-2026 Dongxu Li (github.com/dxli)
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
# USA.
# ********************************************************************************

# sh path/to/script <.app directory for output bundle> <path to qt bin>

CONTENTSDIR="$1/Contents"
LRELEASE="$2/lrelease"
SCRIPTDIR="$(dirname "$0")"

RESOURCEDIR="$CONTENTSDIR/Resources"
TSDIRLC="$SCRIPTDIR/../librecad/ts"
TSDIRPI="$SCRIPTDIR/../plugins/ts"

# Postprocess for osx
mkdir -p "$RESOURCEDIR/fonts"
mkdir -p "$RESOURCEDIR/patterns"
mkdir -p "$RESOURCEDIR/library"
mkdir -p "$CONTENTSDIR/PlugIns/LibreCAD"
if [ -d "$SCRIPTDIR/../librecad/support/patterns" ]; then
	find "$SCRIPTDIR/../librecad/support/patterns" -maxdepth 1 -type f -name '*.dxf' \
		-exec cp {} "$RESOURCEDIR/patterns/" \;
fi
cp "$SCRIPTDIR/../librecad/support/fonts/"*.lff "$RESOURCEDIR/fonts/"
if [ -d "$SCRIPTDIR/../librecad/support/library" ]; then
	cp -r "$SCRIPTDIR/../librecad/support/library/" "$RESOURCEDIR/library/"
fi

if [ -x "$LRELEASE" ]
then
	# Generate translations
	"$LRELEASE" "$SCRIPTDIR/../librecad/src/src.pro"
	"$LRELEASE" "$SCRIPTDIR/../plugins/plugins.pro"
	mkdir -p "$RESOURCEDIR/qm"

	for tf in "$TSDIRLC/"*.qm
	do
		mv "$tf" "$RESOURCEDIR/qm/$(basename "$tf")"
	done

	for tf in "$TSDIRPI/"*.qm
	do
		mv "$tf" "$RESOURCEDIR/qm/$(basename "$tf")"
	done
else
	echo "WARNING: lrelease not found - Translations will not be generated"
fi
