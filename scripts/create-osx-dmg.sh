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
OUTPUT_PATH=
VOLUME_NAME=LibreCAD
BACKEND=direct
MODE=adhoc
EXPECTED_ARCHITECTURES=
REPORT_PATH=
WORK_DIR=

usage() {
    cat <<EOF
Usage: $PROGRAM_NAME --app PATH --output PATH [options]

Options:
  --volume-name NAME          DMG volume name (default: LibreCAD)
  --backend BACKEND           direct or convert (default: direct)
  --mode MODE                 App signature mode used for verification
  --architectures LIST        Required architectures, comma or space separated
  --report PATH               Write the mounted-DMG verification log
  -h, --help                  Show this help

This helper only constructs the image. It never deploys or signs the app.
EOF
}

fail() {
    echo "$PROGRAM_NAME: error: $*" >&2
    exit 1
}

cleanup() {
    local status=$?
    set +e
    if [[ -n $WORK_DIR && -d $WORK_DIR ]]; then
        rm -rf "$WORK_DIR"
    fi
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
        --output)
            [[ $# -ge 2 ]] || fail "--output requires a path"
            OUTPUT_PATH=$2
            shift 2
            ;;
        --output=*)
            OUTPUT_PATH=${1#*=}
            shift
            ;;
        --volume-name)
            [[ $# -ge 2 ]] || fail "--volume-name requires a value"
            VOLUME_NAME=$2
            shift 2
            ;;
        --volume-name=*)
            VOLUME_NAME=${1#*=}
            shift
            ;;
        --backend)
            [[ $# -ge 2 ]] || fail "--backend requires a value"
            BACKEND=$2
            shift 2
            ;;
        --backend=*)
            BACKEND=${1#*=}
            shift
            ;;
        --mode)
            [[ $# -ge 2 ]] || fail "--mode requires a value"
            MODE=$2
            shift 2
            ;;
        --mode=*)
            MODE=${1#*=}
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
        --report)
            [[ $# -ge 2 ]] || fail "--report requires a path"
            REPORT_PATH=$2
            shift 2
            ;;
        --report=*)
            REPORT_PATH=${1#*=}
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
[[ -n $OUTPUT_PATH ]] || fail "--output is required"
[[ -d $APP_PATH ]] || fail "application bundle does not exist: $APP_PATH"
[[ ! -L $APP_PATH ]] || fail "application bundle must not be a symbolic link: $APP_PATH"
[[ $OUTPUT_PATH == *.dmg ]] || fail "--output must end in .dmg"
[[ -x $SCRIPT_DIR/verify-osx-package.sh ]] ||
    fail "package verifier is missing or not executable"

case $BACKEND in
    direct|convert)
        ;;
    *)
        fail "unsupported backend '$BACKEND'"
        ;;
esac

case $MODE in
    adhoc|developer-id|notarized|unsigned)
        ;;
    *)
        fail "unsupported mode '$MODE'"
        ;;
esac

case $VOLUME_NAME in
    *$'\n'*|*/*|'')
        fail "invalid volume name"
        ;;
esac

APP_PATH=$(cd "$(dirname "$APP_PATH")" && printf '%s/%s\n' "$(pwd -P)" "$(basename "$APP_PATH")")
OUTPUT_PARENT=$(dirname "$OUTPUT_PATH")
[[ -d $OUTPUT_PARENT ]] || fail "output parent does not exist: $OUTPUT_PARENT"
OUTPUT_PATH=$(cd "$OUTPUT_PARENT" && printf '%s/%s\n' "$(pwd -P)" "$(basename "$OUTPUT_PATH")")
[[ ! -d $OUTPUT_PATH ]] || fail "output DMG path is a directory: $OUTPUT_PATH"
case $OUTPUT_PATH in
    "$APP_PATH"|"$APP_PATH"/*)
        fail "output DMG must be outside the signed app bundle"
        ;;
esac
if [[ -n $REPORT_PATH ]]; then
    REPORT_PARENT=$(dirname "$REPORT_PATH")
    [[ -d $REPORT_PARENT ]] || fail "report parent does not exist: $REPORT_PARENT"
    REPORT_PATH=$(cd "$REPORT_PARENT" && printf '%s/%s\n' "$(pwd -P)" "$(basename "$REPORT_PATH")")
    [[ ! -d $REPORT_PATH ]] || fail "report path is a directory: $REPORT_PATH"
    [[ $REPORT_PATH != "$OUTPUT_PATH" ]] || fail "report and DMG paths must be different"
    case $REPORT_PATH in
        "$APP_PATH"|"$APP_PATH"/*)
            fail "verification report must be outside the signed app bundle"
            ;;
    esac
fi

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

# A failed run must not leave an older image at the requested output path.
rm -f "$OUTPUT_PATH"
if [[ -n $REPORT_PATH ]]; then
    rm -f "$REPORT_PATH"
fi

VERIFY_MODE=$MODE
if [[ $VERIFY_MODE == notarized ]]; then
    # The application has a Developer ID signature at this point, but the
    # enclosing DMG has not yet been submitted or stapled.
    VERIFY_MODE=developer-id
fi

verify_app_args=(--app "$APP_PATH" --mode "$VERIFY_MODE")
if [[ -n $EXPECTED_ARCHITECTURES ]]; then
    verify_app_args+=(--architectures "$EXPECTED_ARCHITECTURES")
fi
"$SCRIPT_DIR/verify-osx-package.sh" "${verify_app_args[@]}"

WORK_DIR=$(mktemp -d "$(dirname "$OUTPUT_PATH")/.librecad-dmg.XXXXXX")
trap cleanup EXIT
trap 'exit 130' HUP INT TERM

STAGING_DIR="$WORK_DIR/staging"
CANDIDATE_DMG="$WORK_DIR/candidate.dmg"
mkdir -p "$STAGING_DIR"

ditto --rsrc --extattr --acl "$APP_PATH" "$STAGING_DIR/$(basename "$APP_PATH")"
ln -s /Applications "$STAGING_DIR/Applications"

ENTRY_COUNT=$(find "$STAGING_DIR" -mindepth 1 -maxdepth 1 -print | wc -l | tr -d ' ')
[[ $ENTRY_COUNT -eq 2 ]] || fail "unexpected staging layout ($ENTRY_COUNT entries)"

case $BACKEND in
    direct)
        hdiutil create -quiet -volname "$VOLUME_NAME" -srcfolder "$STAGING_DIR" \
            -format UDZO -fs HFS+ "$CANDIDATE_DMG"
        ;;
    convert)
        READ_WRITE_DMG="$WORK_DIR/read-write.dmg"
        hdiutil create -quiet -volname "$VOLUME_NAME" -srcfolder "$STAGING_DIR" \
            -format UDRW -fs HFS+ "$READ_WRITE_DMG"
        hdiutil convert -quiet "$READ_WRITE_DMG" -format UDZO -o "$CANDIDATE_DMG"
        ;;
esac

[[ -f $CANDIDATE_DMG ]] || fail "hdiutil did not create the candidate DMG"

verify_dmg_args=(
    --dmg "$CANDIDATE_DMG"
    --source-app "$APP_PATH"
    --mode "$VERIFY_MODE"
    --allow-unsigned-dmg
)
if [[ -n $EXPECTED_ARCHITECTURES ]]; then
    verify_dmg_args+=(--architectures "$EXPECTED_ARCHITECTURES")
fi
if [[ -n $REPORT_PATH ]]; then
    verify_dmg_args+=(--report "$REPORT_PATH")
fi
"$SCRIPT_DIR/verify-osx-package.sh" "${verify_dmg_args[@]}"

mv "$CANDIDATE_DMG" "$OUTPUT_PATH"
if [[ -n $REPORT_PATH ]]; then
    printf '%s: published_dmg=%s\n' "$PROGRAM_NAME" "$OUTPUT_PATH" >> "$REPORT_PATH"
fi
echo "$PROGRAM_NAME: created $OUTPUT_PATH using the $BACKEND backend"
