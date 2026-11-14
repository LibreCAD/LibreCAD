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
DMG_PATH=
TIMEOUT_SECONDS=60
WORK_DIR=
MOUNT_POINT=
MOUNTED=0

usage() {
    cat <<EOF
Usage: $PROGRAM_NAME --dmg PATH [--timeout SECONDS]

Mount a LibreCAD DMG read-only and run its bundled application with --help.
The app receives an isolated temporary home directory. A timeout bounds startup.
EOF
}

fail() {
    echo "$PROGRAM_NAME: error: $*" >&2
    exit 1
}

cleanup() {
    local status=$?
    set +e
    if [[ $MOUNTED -eq 1 ]]; then
        if ! hdiutil detach -quiet "$MOUNT_POINT"; then
            hdiutil detach -quiet -force "$MOUNT_POINT" || status=1
        fi
    fi
    [[ -z $WORK_DIR || ! -d $WORK_DIR ]] || rm -rf "$WORK_DIR"
    trap - EXIT
    exit "$status"
}

while [[ $# -gt 0 ]]; do
    case $1 in
        --dmg)
            [[ $# -ge 2 ]] || fail "--dmg requires a path"
            DMG_PATH=$2
            shift 2
            ;;
        --dmg=*)
            DMG_PATH=${1#*=}
            shift
            ;;
        --timeout)
            [[ $# -ge 2 ]] || fail "--timeout requires seconds"
            TIMEOUT_SECONDS=$2
            shift 2
            ;;
        --timeout=*)
            TIMEOUT_SECONDS=${1#*=}
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

[[ -n $DMG_PATH ]] || fail "--dmg is required"
[[ -f $DMG_PATH && ! -L $DMG_PATH ]] || fail "DMG must be a regular file: $DMG_PATH"
[[ $TIMEOUT_SECONDS =~ ^[1-9][0-9]*$ ]] || fail "timeout must be a positive integer"
command -v hdiutil >/dev/null 2>&1 || fail "hdiutil is required"
command -v python3 >/dev/null 2>&1 || fail "python3 is required"

DMG_PATH=$(cd "$(dirname "$DMG_PATH")" && printf '%s/%s\n' "$(pwd -P)" "$(basename "$DMG_PATH")")
WORK_DIR=$(mktemp -d "${TMPDIR:-/tmp}/librecad-dmg-runtime.XXXXXX")
MOUNT_POINT="$WORK_DIR/mount"
ISOLATED_HOME="$WORK_DIR/home"
mkdir -p "$MOUNT_POINT" "$ISOLATED_HOME"
trap cleanup EXIT
trap 'exit 130' HUP INT TERM

if ! hdiutil attach -quiet -nobrowse -readonly -mountpoint "$MOUNT_POINT" \
    "$DMG_PATH" > "$WORK_DIR/mount.log" 2>&1
then
    cat "$WORK_DIR/mount.log" >&2
    fail "could not mount DMG read-only"
fi
MOUNTED=1

APP_BINARY="$MOUNT_POINT/LibreCAD.app/Contents/MacOS/LibreCAD"
[[ -x $APP_BINARY ]] || fail "LibreCAD executable is missing from the DMG"

python3 - "$APP_BINARY" "$ISOLATED_HOME" "$TIMEOUT_SECONDS" <<'PY'
import os
import subprocess
import sys

binary, home, timeout = sys.argv[1], sys.argv[2], int(sys.argv[3])
env = os.environ.copy()
env["HOME"] = home

try:
    result = subprocess.run(
        [binary, "--help"],
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        errors="replace",
        timeout=timeout,
        env=env,
        check=False,
    )
except subprocess.TimeoutExpired as error:
    output = error.stdout or ""
    if isinstance(output, bytes):
        output = output.decode(errors="replace")
    print(output, end="")
    print(f"test-osx-dmg-runtime.sh: application startup exceeded {timeout}s", file=sys.stderr)
    sys.exit(124)

print(result.stdout, end="")
if result.returncode != 0:
    print(f"test-osx-dmg-runtime.sh: application exited with {result.returncode}", file=sys.stderr)
    sys.exit(1)
if "Usage: librecad" not in result.stdout:
    print("test-osx-dmg-runtime.sh: expected LibreCAD help output was absent", file=sys.stderr)
    sys.exit(1)
PY

echo "$PROGRAM_NAME: mounted-DMG application startup passed"
