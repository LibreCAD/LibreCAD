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
APP_PATH=
EXPECTED_ARCHITECTURES=
TEST_IMAGES=0
WORK_DIR=

usage() {
    cat <<EOF
Usage: $PROGRAM_NAME --app PATH [options]

Options:
  --architectures LIST   Required architectures, comma or space separated
  --images               Exercise both native DMG backends
  -h, --help             Show this help

The input must be a finalized ad-hoc-signed application. Tests use temporary
copies and never mutate the source app.
EOF
}

fail() {
    echo "$PROGRAM_NAME: error: $*" >&2
    exit 1
}

cleanup() {
    local status=$?
    set +e
    [[ -z $WORK_DIR || ! -d $WORK_DIR ]] || rm -rf "$WORK_DIR"
    trap - EXIT
    exit "$status"
}

while [[ $# -gt 0 ]]; do
    case $1 in
        --app)
            [[ $# -ge 2 ]] || fail "--app requires a path"
            APP_PATH=$2
            shift 2
            ;;
        --app=*)
            APP_PATH=${1#*=}
            shift
            ;;
        --architectures)
            [[ $# -ge 2 ]] || fail "--architectures requires a list"
            EXPECTED_ARCHITECTURES=$2
            shift 2
            ;;
        --architectures=*)
            EXPECTED_ARCHITECTURES=${1#*=}
            shift
            ;;
        --images)
            TEST_IMAGES=1
            shift
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        *)
            fail "unknown option: $1"
            ;;
    esac
done

[[ -n $APP_PATH ]] || fail "--app is required"
[[ -d $APP_PATH ]] || fail "application bundle does not exist: $APP_PATH"
APP_PATH=$(cd "$(dirname "$APP_PATH")" && printf '%s/%s\n' "$(pwd -P)" "$(basename "$APP_PATH")")

verify_args=(--mode adhoc)
if [[ -n $EXPECTED_ARCHITECTURES ]]; then
    verify_args+=(--architectures "$EXPECTED_ARCHITECTURES")
fi
"$SCRIPT_DIR/verify-osx-package.sh" --app "$APP_PATH" "${verify_args[@]}"

TEMP_BASE=${TMPDIR:-/tmp}
[[ -d $TEMP_BASE ]] || fail "temporary directory does not exist: $TEMP_BASE"
TEMP_BASE=$(cd "$TEMP_BASE" && pwd -P)
case $TEMP_BASE in
    "$APP_PATH"|"$APP_PATH"/*)
        TEMP_BASE=$(cd /tmp && pwd -P)
        ;;
esac
TMPDIR="$TEMP_BASE/"
export TMPDIR
WORK_DIR=$(mktemp -d "$TEMP_BASE/LibreCAD package test.XXXXXX")
trap cleanup EXIT
trap 'exit 130' HUP INT TERM

SPACED_DIR="$WORK_DIR/App With Spaces"
TAMPERED_APP="$SPACED_DIR/$(basename "$APP_PATH")"
mkdir -p "$SPACED_DIR"
ditto --rsrc --extattr --acl "$APP_PATH" "$TAMPERED_APP"
"$SCRIPT_DIR/verify-osx-package.sh" --app "$TAMPERED_APP" "${verify_args[@]}"

printf '\n' >> "$TAMPERED_APP/Contents/Info.plist"
STALE_DMG="$WORK_DIR/stale-output.dmg"
printf 'stale\n' > "$STALE_DMG"
tamper_args=(
    --app "$TAMPERED_APP"
    --output "$STALE_DMG"
    --mode adhoc
)
if [[ -n $EXPECTED_ARCHITECTURES ]]; then
    tamper_args+=(--architectures "$EXPECTED_ARCHITECTURES")
fi
if "$SCRIPT_DIR/create-osx-dmg.sh" "${tamper_args[@]}" \
    > "$WORK_DIR/tamper.log" 2>&1
then
    fail "verifier accepted an application modified after signing"
fi
[[ ! -e $STALE_DMG ]] || fail "failed packaging left a stale output DMG"
echo "$PROGRAM_NAME: post-sign mutation was rejected and stale output removed"

if "$SCRIPT_DIR/verify-osx-package.sh" --app "$TAMPERED_APP" \
    --mode unsigned --architectures librecad_missing_arch \
    > "$WORK_DIR/architecture.log" 2>&1
then
    fail "verifier accepted a missing Mach-O architecture"
fi
grep -q 'missing architecture librecad_missing_arch' \
    "$WORK_DIR/architecture.log" ||
    fail "architecture test failed for an unexpected reason"
echo "$PROGRAM_NAME: missing architecture was rejected as expected"

ESCAPE_BINARY="$TAMPERED_APP/Contents/Frameworks/QtCore.framework/Versions/A/QtCore"
[[ -f $ESCAPE_BINARY ]] || fail "QtCore test binary does not exist"
install_name_tool -change /usr/lib/libSystem.B.dylib \
    /tmp/librecad-forbidden.dylib "$ESCAPE_BINARY" >/dev/null 2>&1
dependency_args=(--app "$TAMPERED_APP" --mode unsigned)
if [[ -n $EXPECTED_ARCHITECTURES ]]; then
    dependency_args+=(--architectures "$EXPECTED_ARCHITECTURES")
fi
if "$SCRIPT_DIR/verify-osx-package.sh" "${dependency_args[@]}" \
    > "$WORK_DIR/dependency.log" 2>&1
then
    fail "verifier accepted an external Mach-O dependency"
fi
grep -q 'unresolved or external dependency' "$WORK_DIR/dependency.log" ||
    fail "external dependency test failed for an unexpected reason"
echo "$PROGRAM_NAME: external dependency was rejected as expected"

if [[ $TEST_IMAGES -eq 1 ]]; then
    for backend in direct convert; do
        dmg="$WORK_DIR/LibreCAD $backend.dmg"
        create_args=(
            --app "$APP_PATH"
            --output "$dmg"
            --backend "$backend"
            --mode adhoc
        )
        if [[ -n $EXPECTED_ARCHITECTURES ]]; then
            create_args+=(--architectures "$EXPECTED_ARCHITECTURES")
        fi
        "$SCRIPT_DIR/create-osx-dmg.sh" "${create_args[@]}"
    done

    BROKEN_DMG="$WORK_DIR/LibreCAD truncated.dmg"
    cp "$dmg" "$BROKEN_DMG"
    truncate -s 4096 "$BROKEN_DMG"
    if "$SCRIPT_DIR/verify-osx-package.sh" --dmg "$BROKEN_DMG" \
        "${verify_args[@]}" > "$WORK_DIR/truncated.log" 2>&1
    then
        fail "verifier accepted a truncated DMG"
    fi
    echo "$PROGRAM_NAME: truncated DMG was rejected as expected"
fi

echo "$PROGRAM_NAME: result=ok"
