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
QT_BIN=
MODE=adhoc
IDENTITY=${MACOS_CODESIGN_IDENTITY:-${CODESIGN_IDENTITY:-}}
ENTITLEMENTS="$SCRIPT_DIR/librecad-macos.entitlements"
EXPECTED_ARCHITECTURES=
REPORT_PATH=
MANIFEST_PATH=
SKIP_DEPLOY=0
WORK_DIR=

usage() {
    cat <<EOF
Usage: $PROGRAM_NAME --app PATH --qt-bin PATH [options]

Options:
  --mode MODE                 adhoc, developer-id, notarized, or unsigned
  --identity IDENTITY         Developer ID Application identity
  --entitlements PATH         Host entitlements for Developer ID modes
  --architectures LIST        Required architectures, comma or space separated
  --report PATH               Write final app verification log
  --manifest PATH             Write finalized app content manifest
  --skip-deploy               Verify/sign an app already processed by macdeployqt
  -h, --help                  Show this help

All deployment and pruning happens before this helper applies final signatures.
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
        --qt-bin)
            [[ $# -ge 2 ]] || fail "--qt-bin requires a path"
            QT_BIN=$2
            shift 2
            ;;
        --qt-bin=*)
            QT_BIN=${1#*=}
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
        --identity)
            [[ $# -ge 2 ]] || fail "--identity requires a value"
            IDENTITY=$2
            shift 2
            ;;
        --identity=*)
            IDENTITY=${1#*=}
            shift
            ;;
        --entitlements)
            [[ $# -ge 2 ]] || fail "--entitlements requires a path"
            ENTITLEMENTS=$2
            shift 2
            ;;
        --entitlements=*)
            ENTITLEMENTS=${1#*=}
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
        --manifest)
            [[ $# -ge 2 ]] || fail "--manifest requires a path"
            MANIFEST_PATH=$2
            shift 2
            ;;
        --manifest=*)
            MANIFEST_PATH=${1#*=}
            shift
            ;;
        --skip-deploy)
            SKIP_DEPLOY=1
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
[[ -n $QT_BIN ]] || fail "--qt-bin is required"
[[ -d $APP_PATH ]] || fail "application bundle does not exist: $APP_PATH"
[[ ! -L $APP_PATH ]] || fail "application bundle must not be a symbolic link: $APP_PATH"
[[ -x $QT_BIN/macdeployqt ]] || fail "macdeployqt not found under: $QT_BIN"
[[ -x $SCRIPT_DIR/verify-osx-package.sh ]] ||
    fail "package verifier is missing or not executable"

case $MODE in
    adhoc|unsigned)
        ;;
    developer-id|notarized)
        [[ -n $IDENTITY ]] || fail "$MODE mode requires --identity"
        [[ -f $ENTITLEMENTS ]] || fail "entitlements file does not exist: $ENTITLEMENTS"
        plutil -lint "$ENTITLEMENTS" >/dev/null ||
            fail "invalid entitlements plist: $ENTITLEMENTS"
        ;;
    *)
        fail "unsupported mode '$MODE'"
        ;;
esac

APP_PATH=$(cd "$(dirname "$APP_PATH")" && printf '%s/%s\n' "$(pwd -P)" "$(basename "$APP_PATH")")
QT_BIN=$(cd "$QT_BIN" && pwd -P)

if [[ -n $REPORT_PATH ]]; then
    REPORT_PARENT=$(dirname "$REPORT_PATH")
    [[ -d $REPORT_PARENT ]] || fail "report parent does not exist: $REPORT_PARENT"
    REPORT_PATH=$(cd "$REPORT_PARENT" && printf '%s/%s\n' "$(pwd -P)" "$(basename "$REPORT_PATH")")
    [[ ! -d $REPORT_PATH ]] || fail "report path is a directory: $REPORT_PATH"
    case $REPORT_PATH in
        "$APP_PATH"|"$APP_PATH"/*)
            fail "verification report must be outside the signed app bundle"
            ;;
    esac
fi
if [[ -n $MANIFEST_PATH ]]; then
    MANIFEST_PARENT=$(dirname "$MANIFEST_PATH")
    [[ -d $MANIFEST_PARENT ]] || fail "manifest parent does not exist: $MANIFEST_PARENT"
    MANIFEST_PATH=$(cd "$MANIFEST_PARENT" && printf '%s/%s\n' "$(pwd -P)" "$(basename "$MANIFEST_PATH")")
    [[ ! -d $MANIFEST_PATH ]] || fail "manifest path is a directory: $MANIFEST_PATH"
    case $MANIFEST_PATH in
        "$APP_PATH"|"$APP_PATH"/*)
            fail "manifest must be outside the signed app bundle"
            ;;
    esac
fi
if [[ -n $REPORT_PATH && -n $MANIFEST_PATH && $REPORT_PATH == "$MANIFEST_PATH" ]]; then
    fail "report and manifest paths must be different"
fi

# Failed retries must not leave a previous success report or manifest behind.
if [[ -n $REPORT_PATH ]]; then
    rm -f "$REPORT_PATH"
fi
if [[ -n $MANIFEST_PATH ]]; then
    rm -f "$MANIFEST_PATH"
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
WORK_DIR=$(mktemp -d "$TEMP_BASE/librecad-app-package.XXXXXX")
trap cleanup EXIT
trap 'exit 130' HUP INT TERM

is_macho() {
    file -b "$1" 2>/dev/null | grep -q 'Mach-O'
}

normalize_plugin_layout() {
    local legacy="$APP_PATH/Contents/Resources/plugins"
    local destination="$APP_PATH/Contents/PlugIns/LibreCAD"
    local plugin
    local target

    [[ -d $legacy ]] || return 0
    mkdir -p "$destination"
    while IFS= read -r -d '' plugin; do
        is_macho "$plugin" || continue
        target="$destination/$(basename "$plugin")"
        [[ ! -e $target ]] || fail "plug-in destination already exists: $target"
        mv "$plugin" "$target"
        log "moved built-in plug-in to Contents/PlugIns/LibreCAD: $(basename "$plugin")"
    done < <(find "$legacy" -maxdepth 1 -type f -print0)
    rmdir "$legacy" >/dev/null 2>&1 || true
}

collect_extra_executables() {
    local output=$1
    local candidate
    local main_executable

    main_executable=$(plutil -extract CFBundleExecutable raw -o - \
        "$APP_PATH/Contents/Info.plist" 2>/dev/null || true)
    : > "$output"

    if [[ -d $APP_PATH/Contents/MacOS ]]; then
        while IFS= read -r -d '' candidate; do
            [[ $(basename "$candidate") != "$main_executable" ]] || continue
            is_macho "$candidate" && printf '%s\n' "$candidate" >> "$output"
        done < <(find "$APP_PATH/Contents/MacOS" -type f -print0)
    fi
    if [[ -d $APP_PATH/Contents/PlugIns/LibreCAD ]]; then
        while IFS= read -r -d '' candidate; do
            is_macho "$candidate" && printf '%s\n' "$candidate" >> "$output"
        done < <(find "$APP_PATH/Contents/PlugIns/LibreCAD" -type f -print0)
    fi
    LC_ALL=C sort -u -o "$output" "$output"
}

framework_is_referenced() {
    local framework_name=$1
    local framework_path="$APP_PATH/Contents/Frameworks/$framework_name.framework"
    local binary

    while IFS= read -r -d '' binary; do
        case $binary in
            "$framework_path"/*)
                continue
                ;;
        esac
        is_macho "$binary" || continue
        if otool -L "$binary" | grep -Fq "/$framework_name.framework/"; then
            return 0
        fi
    done < <(find "$APP_PATH/Contents" -type f -print0)
    return 1
}

list_load_dependencies() {
    otool -l "$1" | awk '
        $1 == "cmd" {
            wanted = ($2 == "LC_LOAD_DYLIB" ||
                      $2 == "LC_LOAD_WEAK_DYLIB" ||
                      $2 == "LC_REEXPORT_DYLIB" ||
                      $2 == "LC_LOAD_UPWARD_DYLIB")
            next
        }
        wanted && $1 == "name" {
            print $2
            wanted = 0
        }
    '
}

prune_unusable_qt_plugins() {
    local plugin
    local dependency
    local relative
    local required_path

    [[ -d $APP_PATH/Contents/PlugIns ]] || return 0
    while IFS= read -r -d '' plugin; do
        case $plugin in
            "$APP_PATH/Contents/PlugIns/LibreCAD"/*)
                continue
                ;;
        esac
        is_macho "$plugin" || continue
        while IFS= read -r dependency; do
            case $dependency in
                @rpath/*.framework/*)
                    relative=${dependency#@rpath/}
                    required_path="$APP_PATH/Contents/Frameworks/$relative"
                    if [[ ! -e $required_path && ! -L $required_path ]]; then
                        rm -f "$plugin"
                        log "removed unusable Qt plug-in ${plugin#"$APP_PATH"/} (missing $dependency)"
                        break
                    fi
                    ;;
            esac
        done < <(list_load_dependencies "$plugin")
    done < <(find "$APP_PATH/Contents/PlugIns" -type f -print0)
}

prune_unused_virtual_keyboard() {
    local plugin="$APP_PATH/Contents/PlugIns/platforminputcontexts/libqtvirtualkeyboardplugin.dylib"
    local candidates=(QtQuick QtQml QtQmlModels QtQmlWorkerScript QtQmlMeta QtOpenGL)
    local framework
    local changed=1

    if [[ -f $plugin ]]; then
        rm -f "$plugin"
        log "removed unused Qt virtual keyboard plug-in"
    fi

    while [[ $changed -eq 1 ]]; do
        changed=0
        for framework in "${candidates[@]}"; do
            [[ -d $APP_PATH/Contents/Frameworks/$framework.framework ]] || continue
            if ! framework_is_referenced "$framework"; then
                rm -rf "$APP_PATH/Contents/Frameworks/$framework.framework"
                log "removed orphaned $framework.framework"
                changed=1
            fi
        done
    done
}

sanitize_external_rpaths() {
    local binary
    local rpath

    while IFS= read -r -d '' binary; do
        is_macho "$binary" || continue
        while IFS= read -r rpath; do
            [[ -n $rpath ]] || continue
            case $rpath in
                @loader_path|@loader_path/*|@executable_path|@executable_path/*)
                    ;;
                *)
                    install_name_tool -delete_rpath "$rpath" "$binary" \
                        >/dev/null 2>&1 ||
                        fail "cannot remove external LC_RPATH '$rpath' from $binary"
                    log "removed external LC_RPATH '$rpath' from ${binary#"$APP_PATH"/}"
                    ;;
            esac
        done < <(otool -l "$binary" | awk '$1 == "cmd" && $2 == "LC_RPATH" { getline; getline; print $2 }')
    done < <(find "$APP_PATH/Contents" -type f -print0)
}

sign_one() {
    local target=$1
    local sign_args=(--force)

    if [[ $MODE == adhoc ]]; then
        sign_args+=(--sign -)
    else
        sign_args+=(--options runtime --timestamp --sign "$IDENTITY")
    fi
    codesign "${sign_args[@]}" "$target" >/dev/null 2>&1 ||
        fail "cannot sign nested code: $target"
}

sign_application_inside_out() {
    local code_list="$WORK_DIR/code-to-sign.txt"
    local candidate
    local depth
    local main_executable
    local sign_args=(--force)

    main_executable=$(plutil -extract CFBundleExecutable raw -o - \
        "$APP_PATH/Contents/Info.plist" 2>/dev/null || true)
    : > "$code_list"

    while IFS= read -r -d '' candidate; do
        is_macho "$candidate" || continue
        [[ $candidate != "$APP_PATH/Contents/MacOS/$main_executable" ]] || continue
        depth=$(awk -F/ '{ print NF }' <<< "$candidate")
        printf '%s\t%s\n' "$depth" "$candidate" >> "$code_list"
    done < <(find "$APP_PATH/Contents" -type f -print0)

    while IFS= read -r -d '' candidate; do
        depth=$(awk -F/ '{ print NF }' <<< "$candidate")
        printf '%s\t%s\n' "$depth" "$candidate" >> "$code_list"
    done < <(find "$APP_PATH/Contents" -type d \( -name '*.framework' -o -name '*.app' -o -name '*.appex' -o -name '*.xpc' -o -name '*.plugin' \) -print0)

    LC_ALL=C sort -t $'\t' -k1,1nr -k2,2 -o "$code_list" "$code_list"
    while IFS=$'\t' read -r _ candidate; do
        [[ -n $candidate ]] || continue
        sign_one "$candidate"
    done < "$code_list"

    if [[ $MODE == adhoc ]]; then
        sign_args+=(--sign -)
    else
        sign_args+=(--options runtime --timestamp --entitlements "$ENTITLEMENTS" --sign "$IDENTITY")
    fi
    codesign "${sign_args[@]}" "$APP_PATH" >/dev/null 2>&1 ||
        fail "cannot sign application bundle: $APP_PATH"
}

normalize_plugin_layout

if [[ $SKIP_DEPLOY -eq 0 ]]; then
    EXTRA_EXECUTABLES="$WORK_DIR/extra-executables.txt"
    collect_extra_executables "$EXTRA_EXECUTABLES"
    MACDEPLOY_ARGS=("$APP_PATH" -verbose=1 -always-overwrite)
    # Qt 6.11 added -no-codesign and signs ad hoc by default; older macdeployqt never signs.
    MACDEPLOY_HELP=$("$QT_BIN/macdeployqt" -help 2>&1 || true)
    if [[ $MACDEPLOY_HELP == *-no-codesign* ]]; then
        MACDEPLOY_ARGS+=(-no-codesign)
    fi
    while IFS= read -r candidate; do
        [[ -n $candidate ]] || continue
        MACDEPLOY_ARGS+=("-executable=$candidate")
    done < "$EXTRA_EXECUTABLES"

    log "deploying Qt frameworks without signing"
    "$QT_BIN/macdeployqt" "${MACDEPLOY_ARGS[@]}"
else
    log "skipping Qt deployment for an existing deployed bundle"
fi
prune_unusable_qt_plugins
prune_unused_virtual_keyboard
sanitize_external_rpaths

preflight_args=(--app "$APP_PATH" --mode unsigned)
if [[ -n $EXPECTED_ARCHITECTURES ]]; then
    preflight_args+=(--architectures "$EXPECTED_ARCHITECTURES")
fi
"$SCRIPT_DIR/verify-osx-package.sh" "${preflight_args[@]}"

if [[ $MODE != unsigned ]]; then
    log "signing nested code inside-out in $MODE mode"
    sign_application_inside_out
else
    log "leaving bundle unsigned for local diagnosis"
fi

verify_args=(--app "$APP_PATH" --mode "$MODE")
if [[ -n $EXPECTED_ARCHITECTURES ]]; then
    verify_args+=(--architectures "$EXPECTED_ARCHITECTURES")
fi
if [[ -n $REPORT_PATH ]]; then
    verify_args+=(--report "$REPORT_PATH")
fi
if [[ -n $MANIFEST_PATH ]]; then
    verify_args+=(--manifest "$MANIFEST_PATH")
fi
"$SCRIPT_DIR/verify-osx-package.sh" "${verify_args[@]}"

log "finalized application: $APP_PATH"
