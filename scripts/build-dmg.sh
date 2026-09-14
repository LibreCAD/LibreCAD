#!/bin/bash

# ********************************************************************************
# This file is part of the LibreCAD project, a 2D CAD program
#
# Copyright (C) 2026 LibreCAD.org
# Copyright (C) 2026 Dongxu Li (github.com/dxli)
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

set -euo pipefail

PROGRAM_NAME=${0##*/}
SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd -P)
ROOT_DIR=$(cd "$SCRIPT_DIR/.." && pwd -P)
TARGETS=()
FORWARDED_ARGS=()
OUTPUT_SPECIFIED=0

usage() {
    cat <<EOF
Usage: $PROGRAM_NAME [--deployment-target=VERSION ...] [build-osx options]

With no deployment target, this is a compatibility wrapper around build-osx.sh.
Use --targets=VERSION,VERSION to build an explicit target matrix. The installed
Xcode SDK is selected by qmake; obsolete SDK names are not hard-coded here.
EOF
}

fail() {
    echo "$PROGRAM_NAME: error: $*" >&2
    exit 1
}

while [[ $# -gt 0 ]]; do
    case $1 in
        --deployment-target=*)
            TARGETS+=("${1#*=}")
            shift
            ;;
        --targets=*)
            IFS=',' read -r -a parsed_targets <<< "${1#*=}"
            TARGETS+=("${parsed_targets[@]}")
            shift
            ;;
        --output=*)
            OUTPUT_SPECIFIED=1
            FORWARDED_ARGS+=("$1")
            shift
            ;;
        -h|--help)
            usage
            "$SCRIPT_DIR/build-osx.sh" --help
            exit 0
            ;;
        *)
            FORWARDED_ARGS+=("$1")
            shift
            ;;
    esac
done

if [[ ${#TARGETS[@]} -eq 0 ]]; then
    exec "$SCRIPT_DIR/build-osx.sh" "${FORWARDED_ARGS[@]}"
fi

if [[ ${#TARGETS[@]} -gt 1 && $OUTPUT_SPECIFIED -eq 1 ]]; then
    fail "--output cannot be shared by multiple deployment targets"
fi

for target in "${TARGETS[@]}"; do
    [[ $target =~ ^[0-9]+\.[0-9]+([.][0-9]+)?$ ]] ||
        fail "invalid deployment target '$target'"
    build_args=("${FORWARDED_ARGS[@]}" "--deployment-target=$target")
    if [[ $OUTPUT_SPECIFIED -eq 0 ]]; then
        build_args+=("--output=$ROOT_DIR/LibreCAD-macOS-$target.dmg")
    fi
    "$SCRIPT_DIR/build-osx.sh" "${build_args[@]}"
done
