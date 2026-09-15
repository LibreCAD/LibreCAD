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

QMAKE_OPTS=${QMAKE_OPTS:-}
QMAKE_CMD=${QMAKE_CMD:-}
QT_BIN=
QT_PATH_OPTION=
SIGN_MODE=${MACOS_SIGN_MODE:-adhoc}
SIGN_MODE_EXPLICIT=0
[[ -n ${MACOS_SIGN_MODE:-} ]] && SIGN_MODE_EXPLICIT=1
IDENTITY=${MACOS_CODESIGN_IDENTITY:-${CODESIGN_IDENTITY:-}}
ENTITLEMENTS="$SCRIPT_DIR/librecad-macos.entitlements"
DMG_BACKEND=direct
EXPECTED_ARCHITECTURES=
DEPLOYMENT_TARGET=
SKIP_BUILD=0
MAKE_JOBS=${MAKE_JOBS:-6}
APP_PATH="$ROOT_DIR/LibreCAD.app"
OUTPUT_DMG="$ROOT_DIR/LibreCAD.dmg"
VOLUME_NAME=LibreCAD
LEGACY_CERT=0
DIST_WORK_DIR=
CALLER_DIR=$(pwd -P)

usage() {
    cat <<EOF
Usage: $PROGRAM_NAME [options]

Build options:
  -p=PATH, --qtpath=PATH       Qt bin directory
  --qt-bin=PATH               Qt bin directory (preferred spelling)
  --no-qtpath                 Find qmake/macdeployqt through PATH
  -q=OPTS, -qmake_opts=OPTS   Additional qmake options
  --deployment-target=VERSION Set QMAKE_MACOSX_DEPLOYMENT_TARGET
  --jobs=N                    Parallel make jobs (default: $MAKE_JOBS)
  --skip-build                Package an existing application bundle
  --app=PATH                  Existing app path for --skip-build

Packaging options:
  --output=PATH               Output DMG (default: LibreCAD.dmg)
  --volume-name=NAME          DMG volume name (default: LibreCAD)
  --dmg-backend=BACKEND       direct or convert (default: direct)
  --architectures=LIST        Required architectures, comma separated
  --sign-mode=MODE            adhoc, developer-id, notarized, or unsigned
  --identity=IDENTITY         Developer ID Application identity
  --entitlements=PATH         Entitlements for Developer ID modes

Compatibility options:
  -cert=IDENTITY, -codesign-identity=IDENTITY

The default account-free mode applies a valid ad-hoc signature. It does not
provide Developer ID trust or Apple notarization.
EOF
}

fail() {
    echo "$PROGRAM_NAME: error: $*" >&2
    exit 1
}

log() {
    echo "$PROGRAM_NAME: $*"
}

cleanup() {
    local status=$?
    set +e
    if [[ -n $DIST_WORK_DIR && -d $DIST_WORK_DIR ]]; then
        rm -rf "$DIST_WORK_DIR"
    fi
    trap - EXIT
    exit "$status"
}

set_identity_mode() {
    IDENTITY=$1
    if [[ $SIGN_MODE_EXPLICIT -eq 0 ]]; then
        SIGN_MODE=developer-id
    fi
}

while [[ $# -gt 0 ]]; do
    case $1 in
        -q=*|-qmake_opts=*)
            QMAKE_OPTS=${1#*=}
            shift
            ;;
        -p=*|--qtpath=*)
            QT_PATH_OPTION=${1#*=}
            shift
            ;;
        --qt-bin=*)
            QT_PATH_OPTION=${1#*=}
            shift
            ;;
        --no-qtpath|-no-p)
            QT_PATH_OPTION=
            shift
            ;;
        -cert=*|-codesign-identity=*|--codesign-identity=*)
            IDENTITY=${1#*=}
            if [[ -n $IDENTITY ]]; then
                set_identity_mode "$IDENTITY"
            fi
            LEGACY_CERT=1
            shift
            ;;
        --identity=*)
            set_identity_mode "${1#*=}"
            shift
            ;;
        --sign-mode=*)
            SIGN_MODE=${1#*=}
            SIGN_MODE_EXPLICIT=1
            shift
            ;;
        --entitlements=*)
            ENTITLEMENTS=${1#*=}
            shift
            ;;
        --dmg-backend=*)
            DMG_BACKEND=${1#*=}
            shift
            ;;
        --architectures=*)
            EXPECTED_ARCHITECTURES=${1#*=}
            shift
            ;;
        --deployment-target=*)
            DEPLOYMENT_TARGET=${1#*=}
            shift
            ;;
        --jobs=*)
            MAKE_JOBS=${1#*=}
            shift
            ;;
        --skip-build)
            SKIP_BUILD=1
            shift
            ;;
        --app=*)
            APP_PATH=${1#*=}
            shift
            ;;
        --output=*)
            OUTPUT_DMG=${1#*=}
            shift
            ;;
        --volume-name=*)
            VOLUME_NAME=${1#*=}
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

absolute_from_caller() {
    case $1 in
        /*)
            printf '%s\n' "$1"
            ;;
        *)
            printf '%s/%s\n' "$CALLER_DIR" "$1"
            ;;
    esac
}

[[ -n $APP_PATH ]] || fail "--app must not be empty"
APP_PATH=$(absolute_from_caller "$APP_PATH")
OUTPUT_DMG=$(absolute_from_caller "$OUTPUT_DMG")
ENTITLEMENTS=$(absolute_from_caller "$ENTITLEMENTS")
if [[ -n $QT_PATH_OPTION ]]; then
    QT_PATH_OPTION=$(absolute_from_caller "$QT_PATH_OPTION")
fi
case $QMAKE_CMD in
    /*|'')
        ;;
    */*)
        QMAKE_CMD=$(absolute_from_caller "$QMAKE_CMD")
        ;;
esac

APP_PARENT=$(dirname "$APP_PATH")
[[ -d $APP_PARENT ]] || fail "application parent does not exist: $APP_PARENT"
APP_PATH=$(cd "$APP_PARENT" && printf '%s/%s\n' "$(pwd -P)" "$(basename "$APP_PATH")")
OUTPUT_PARENT=$(dirname "$OUTPUT_DMG")
[[ -d $OUTPUT_PARENT ]] || fail "output parent does not exist: $OUTPUT_PARENT"
OUTPUT_DMG=$(cd "$OUTPUT_PARENT" && printf '%s/%s\n' "$(pwd -P)" "$(basename "$OUTPUT_DMG")")
case $OUTPUT_DMG in
    "$APP_PATH"|"$APP_PATH"/*)
        fail "output DMG must be outside the application bundle"
        ;;
esac

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

case $SIGN_MODE in
    adhoc|unsigned)
        ;;
    developer-id|notarized)
        [[ -n $IDENTITY ]] || fail "$SIGN_MODE mode requires --identity"
        ;;
    *)
        fail "unsupported sign mode '$SIGN_MODE'"
        ;;
esac

if [[ $SIGN_MODE == developer-id || $SIGN_MODE == notarized ]]; then
    command -v security >/dev/null 2>&1 || fail "security tool is required for $SIGN_MODE mode"
    AVAILABLE_IDENTITIES=$(security find-identity -v -p codesigning 2>/dev/null || true)
    if ! grep -iF -- "$IDENTITY" <<< "$AVAILABLE_IDENTITIES" |
        grep -q '"Developer ID Application:'
    then
        fail "Developer ID Application identity is not available in the keychain: $IDENTITY"
    fi
fi

case $DMG_BACKEND in
    direct|convert)
        ;;
    *)
        fail "unsupported DMG backend '$DMG_BACKEND'"
        ;;
esac

[[ $MAKE_JOBS =~ ^[1-9][0-9]*$ ]] || fail "--jobs must be a positive integer"
[[ $OUTPUT_DMG == *.dmg ]] || fail "--output must end in .dmg"

if [[ $LEGACY_CERT -eq 1 ]]; then
    log "warning: -cert is deprecated; use --sign-mode and --identity"
fi
if [[ $SIGN_MODE == unsigned ]]; then
    log "warning: unsigned mode is for local diagnosis and is not publishable"
fi

if [[ $SIGN_MODE == notarized && -z ${NOTARIZE_KEYCHAIN_PROFILE:-} ]]; then
    if [[ -z ${NOTARIZE_APPLE_ID:-} || -z ${NOTARIZE_TEAM_ID:-} || -z ${NOTARIZE_PASSWORD:-} ]]; then
        fail "notarized mode requires NOTARIZE_KEYCHAIN_PROFILE or complete Apple ID credentials"
    fi
fi

find_qmake() {
    local candidate
    local candidates=()

    if [[ -n $QMAKE_CMD && -x $QMAKE_CMD ]]; then
        return 0
    fi

    if [[ -n $QT_PATH_OPTION ]]; then
        QT_PATH_OPTION=${QT_PATH_OPTION%/}
        candidates+=("$QT_PATH_OPTION/qmake6" "$QT_PATH_OPTION/qmake")
    fi
    if candidate=$(command -v qmake6 2>/dev/null); then
        candidates+=("$candidate")
    fi
    if candidate=$(command -v qmake 2>/dev/null); then
        candidates+=("$candidate")
    fi
    candidates+=(
        /opt/homebrew/bin/qmake6
        /usr/local/bin/qmake6
        /opt/local/libexec/qt6/bin/qmake6
        /usr/local/opt/qt/bin/qmake
    )

    for candidate in "${candidates[@]}"; do
        if [[ -x $candidate ]]; then
            QMAKE_CMD=$candidate
            return 0
        fi
    done
    return 1
}

if find_qmake; then
    QT_BIN=$("$QMAKE_CMD" -query QT_INSTALL_BINS)
elif [[ -n $QT_PATH_OPTION && -x ${QT_PATH_OPTION%/}/macdeployqt ]]; then
    QT_BIN=${QT_PATH_OPTION%/}
elif [[ $SKIP_BUILD -eq 1 ]] && command -v macdeployqt >/dev/null 2>&1; then
    QT_BIN=$(dirname "$(command -v macdeployqt)")
else
    fail "qmake and macdeployqt could not be located"
fi

[[ -x $QT_BIN/macdeployqt ]] || fail "macdeployqt not found under Qt bin path: $QT_BIN"
if [[ $SKIP_BUILD -eq 0 ]]; then
    [[ -n $QMAKE_CMD && -x $QMAKE_CMD ]] || fail "qmake is required for a full build"
fi

if [[ -n $DEPLOYMENT_TARGET ]]; then
    QMAKE_OPTS="${QMAKE_OPTS:+$QMAKE_OPTS }QMAKE_MACOSX_DEPLOYMENT_TARGET=$DEPLOYMENT_TARGET"
fi

log "Qt bin: $QT_BIN"
[[ -z $QMAKE_CMD ]] || log "qmake: $QMAKE_CMD"
log "sign mode: $SIGN_MODE"
log "DMG backend: $DMG_BACKEND"

cd "$ROOT_DIR"

if [[ $SKIP_BUILD -eq 0 ]]; then
    rm -rf LibreCAD.app

    QMAKE_ARGS=()
    if [[ -n $QMAKE_OPTS ]]; then
        read -r -a QMAKE_ARGS <<< "$QMAKE_OPTS"
    fi

    if [[ -f Makefile ]]; then
        "$QMAKE_CMD" "${QMAKE_ARGS[@]}" -r
        make distclean
    fi

    rm -rf generated
    "$QMAKE_CMD" "${QMAKE_ARGS[@]}" -r
    make -j"$MAKE_JOBS"
fi

[[ -d $APP_PATH ]] || fail "application bundle does not exist: $APP_PATH"
[[ ! -L $APP_PATH ]] || fail "application bundle must not be a symbolic link: $APP_PATH"

FINAL_REPORT="$OUTPUT_DMG.verification.txt"
APP_MANIFEST="$OUTPUT_DMG.app.manifest"
CHECKSUM_FILE="$OUTPUT_DMG.sha256"
rm -f "$OUTPUT_DMG" "$FINAL_REPORT" "$APP_MANIFEST" "$CHECKSUM_FILE"

package_args=(
    --app "$APP_PATH"
    --qt-bin "$QT_BIN"
    --mode "$SIGN_MODE"
    --entitlements "$ENTITLEMENTS"
    --manifest "$APP_MANIFEST"
)
if [[ -n $IDENTITY ]]; then
    package_args+=(--identity "$IDENTITY")
fi
if [[ -n $EXPECTED_ARCHITECTURES ]]; then
    package_args+=(--architectures "$EXPECTED_ARCHITECTURES")
fi
"$SCRIPT_DIR/package-osx-app.sh" "${package_args[@]}"

DIST_WORK_DIR=$(mktemp -d "$(dirname "$OUTPUT_DMG")/.librecad-distribution.XXXXXX")
trap cleanup EXIT
trap 'exit 130' HUP INT TERM
CANDIDATE_DMG="$DIST_WORK_DIR/$(basename "$OUTPUT_DMG")"

create_args=(
    --app "$APP_PATH"
    --output "$CANDIDATE_DMG"
    --volume-name "$VOLUME_NAME"
    --backend "$DMG_BACKEND"
    --mode "$SIGN_MODE"
)
if [[ -n $EXPECTED_ARCHITECTURES ]]; then
    create_args+=(--architectures "$EXPECTED_ARCHITECTURES")
fi
if [[ $SIGN_MODE == adhoc || $SIGN_MODE == unsigned ]]; then
    create_args+=(--report "$FINAL_REPORT")
fi
"$SCRIPT_DIR/create-osx-dmg.sh" "${create_args[@]}"

if [[ $SIGN_MODE == developer-id || $SIGN_MODE == notarized ]]; then
    codesign --force --timestamp --sign "$IDENTITY" "$CANDIDATE_DMG"
fi

if [[ $SIGN_MODE == notarized ]]; then
    if [[ -n ${NOTARIZE_KEYCHAIN_PROFILE:-} ]]; then
        xcrun notarytool submit "$CANDIDATE_DMG" \
            --keychain-profile "$NOTARIZE_KEYCHAIN_PROFILE" --wait
    else
        xcrun notarytool submit "$CANDIDATE_DMG" \
            --apple-id "$NOTARIZE_APPLE_ID" \
            --team-id "$NOTARIZE_TEAM_ID" \
            --password "$NOTARIZE_PASSWORD" \
            --wait
    fi
    xcrun stapler staple "$CANDIDATE_DMG"
fi

if [[ $SIGN_MODE == developer-id || $SIGN_MODE == notarized ]]; then
    verify_args=(
        --dmg "$CANDIDATE_DMG"
        --source-app "$APP_PATH"
        --mode "$SIGN_MODE"
        --report "$FINAL_REPORT"
    )
    if [[ -n $EXPECTED_ARCHITECTURES ]]; then
        verify_args+=(--architectures "$EXPECTED_ARCHITECTURES")
    fi
    "$SCRIPT_DIR/verify-osx-package.sh" "${verify_args[@]}"
fi

mv "$CANDIDATE_DMG" "$OUTPUT_DMG"
printf '%s: published_dmg=%s\n' "$PROGRAM_NAME" "$OUTPUT_DMG" >> "$FINAL_REPORT"
DIGEST=$(shasum -a 256 "$OUTPUT_DMG" | awk '{print $1}')
printf '%s  %s\n' "$DIGEST" "$(basename "$OUTPUT_DMG")" > "$CHECKSUM_FILE"

log "DMG installer generated: $OUTPUT_DMG"
log "SHA-256: $DIGEST"
log "verification report: $FINAL_REPORT"
