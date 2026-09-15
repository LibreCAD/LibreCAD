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
APP_PATH=
DMG_PATH=
SOURCE_APP=
MODE=adhoc
EXPECTED_ARCHITECTURES=
REPORT_PATH=
REPORT_ACTIVE=0
MANIFEST_PATH=
PENDING_MANIFEST=
ALLOW_UNSIGNED_DMG=0
SMOKE_TEST=0
WORK_DIR=
MOUNT_POINT=
ATTACHED=0

usage() {
    cat <<EOF
Usage:
  $PROGRAM_NAME --app PATH [options]
  $PROGRAM_NAME --dmg PATH [options]

Options:
  --mode MODE                 adhoc, developer-id, notarized, or unsigned
  --architectures LIST        Required architectures, comma or space separated
  --source-app PATH           Compare a mounted DMG app with this finalized app
  --report PATH               Write the verification log to PATH
  --manifest PATH             Write a content manifest for the verified app
  --allow-unsigned-dmg        Permit an unsigned outer DMG during staging only
  --smoke                     Run the packaged LibreCAD executable with --help
  -h, --help                  Show this help
EOF
}

fail() {
    local message="$PROGRAM_NAME: error: $*"
    echo "$message" >&2
    if [[ $REPORT_ACTIVE -eq 1 ]]; then
        printf '%s\n' "$message" >> "$REPORT_PATH" 2>/dev/null || true
    fi
    exit 1
}

log() {
    local message="$PROGRAM_NAME: $*"
    echo "$message"
    if [[ $REPORT_ACTIVE -eq 1 ]]; then
        printf '%s\n' "$message" >> "$REPORT_PATH"
    fi
}

require_tool() {
    command -v "$1" >/dev/null 2>&1 || fail "required tool not found: $1"
}

absolute_existing_path() {
    local path=$1
    local parent
    local name

    parent=$(dirname "$path")
    name=$(basename "$path")
    (cd "$parent" 2>/dev/null && printf '%s/%s\n' "$(pwd -P)" "$name") ||
        fail "path parent does not exist: $parent"
}

absolute_output_path() {
    local path=$1
    local parent
    local name

    parent=$(dirname "$path")
    name=$(basename "$path")
    [[ -d $parent ]] || fail "output parent does not exist: $parent"
    (cd "$parent" 2>/dev/null && printf '%s/%s\n' "$(pwd -P)" "$name") ||
        fail "cannot resolve output parent: $parent"
}

path_is_within() {
    local path=$1
    local root=$2

    case $path in
        "$root"|"$root"/*)
            return 0
            ;;
    esac
    return 1
}

detach_mounted_image() {
    if [[ $ATTACHED -eq 1 && -n $MOUNT_POINT ]]; then
        if hdiutil detach "$MOUNT_POINT" -quiet >/dev/null 2>&1 ||
            hdiutil detach "$MOUNT_POINT" -force -quiet >/dev/null 2>&1
        then
            ATTACHED=0
        else
            return 1
        fi
    fi
}

cleanup() {
    local status=$?
    local detach_failed=0
    set +e
    if ! detach_mounted_image; then
        detach_failed=1
        echo "$PROGRAM_NAME: error: failed to detach $MOUNT_POINT" >&2
        if [[ $REPORT_ACTIVE -eq 1 ]]; then
            printf '%s: error: failed to detach %s\n' \
                "$PROGRAM_NAME" "$MOUNT_POINT" >> "$REPORT_PATH"
        fi
        [[ $status -ne 0 ]] || status=1
    fi
    if [[ $detach_failed -eq 0 && -n $WORK_DIR && -d $WORK_DIR ]]; then
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
        --dmg)
            [[ $# -ge 2 ]] || fail "--dmg requires a path"
            DMG_PATH=$2
            shift 2
            ;;
        --dmg=*)
            DMG_PATH=${1#*=}
            shift
            ;;
        --source-app)
            [[ $# -ge 2 ]] || fail "--source-app requires a path"
            SOURCE_APP=$2
            shift 2
            ;;
        --source-app=*)
            SOURCE_APP=${1#*=}
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
        --manifest)
            [[ $# -ge 2 ]] || fail "--manifest requires a path"
            MANIFEST_PATH=$2
            shift 2
            ;;
        --manifest=*)
            MANIFEST_PATH=${1#*=}
            shift
            ;;
        --allow-unsigned-dmg)
            ALLOW_UNSIGNED_DMG=1
            shift
            ;;
        --smoke)
            SMOKE_TEST=1
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

case $MODE in
    adhoc|developer-id|notarized|unsigned)
        ;;
    *)
        fail "unsupported mode '$MODE'"
        ;;
esac

if [[ -n $APP_PATH && -n $DMG_PATH ]]; then
    fail "specify exactly one of --app or --dmg"
fi
if [[ -z $APP_PATH && -z $DMG_PATH ]]; then
    fail "specify --app or --dmg"
fi
if [[ -n $SOURCE_APP && -z $DMG_PATH ]]; then
    fail "--source-app is valid only with --dmg"
fi
if [[ $ALLOW_UNSIGNED_DMG -eq 1 && -z $DMG_PATH ]]; then
    fail "--allow-unsigned-dmg is valid only with --dmg"
fi

for tool in awk cmp codesign ditto file find hdiutil lipo otool plutil readlink shasum sort; do
    require_tool "$tool"
done
if [[ -n $DMG_PATH && $MODE != unsigned ]]; then
    require_tool spctl
fi
if [[ $MODE == notarized ]]; then
    require_tool xcrun
fi

if [[ -n $APP_PATH ]]; then
    APP_PATH=$(absolute_existing_path "$APP_PATH")
    [[ -d $APP_PATH && ! -L $APP_PATH ]] ||
        fail "application bundle is missing or is a symlink: $APP_PATH"
else
    DMG_PATH=$(absolute_existing_path "$DMG_PATH")
    if [[ -n $SOURCE_APP ]]; then
        SOURCE_APP=$(absolute_existing_path "$SOURCE_APP")
        [[ -d $SOURCE_APP && ! -L $SOURCE_APP ]] ||
            fail "source application is missing or is a symlink: $SOURCE_APP"
    fi
fi

if [[ -n $REPORT_PATH ]]; then
    REPORT_PATH=$(absolute_output_path "$REPORT_PATH")
    [[ ! -d $REPORT_PATH ]] || fail "report path is a directory: $REPORT_PATH"
fi
if [[ -n $MANIFEST_PATH ]]; then
    MANIFEST_PATH=$(absolute_output_path "$MANIFEST_PATH")
    [[ ! -d $MANIFEST_PATH ]] || fail "manifest path is a directory: $MANIFEST_PATH"
fi
if [[ -n $REPORT_PATH && -n $MANIFEST_PATH && $REPORT_PATH == "$MANIFEST_PATH" ]]; then
    fail "report and manifest paths must be different"
fi
if [[ -n $DMG_PATH ]]; then
    [[ $REPORT_PATH != "$DMG_PATH" ]] || fail "report path must not replace the DMG"
    [[ $MANIFEST_PATH != "$DMG_PATH" ]] || fail "manifest path must not replace the DMG"
fi
for protected_app in "$APP_PATH" "$SOURCE_APP"; do
    [[ -n $protected_app ]] || continue
    if [[ -n $REPORT_PATH ]] && path_is_within "$REPORT_PATH" "$protected_app"; then
        fail "verification report must be outside the signed app bundle"
    fi
    if [[ -n $MANIFEST_PATH ]] && path_is_within "$MANIFEST_PATH" "$protected_app"; then
        fail "manifest must be outside the signed app bundle"
    fi
done

# Remove old evidence before starting so a failed retry cannot leave an earlier
# successful manifest or report at the requested path. Unlinking first also
# prevents an existing symbolic or hard link from modifying its target.
if [[ -n $REPORT_PATH ]]; then
    rm -f "$REPORT_PATH"
fi
if [[ -n $MANIFEST_PATH ]]; then
    rm -f "$MANIFEST_PATH"
fi
if [[ -n $REPORT_PATH ]]; then
    : > "$REPORT_PATH"
    REPORT_ACTIVE=1
fi

TEMP_BASE=${TMPDIR:-/tmp}
[[ -d $TEMP_BASE ]] || fail "temporary directory does not exist: $TEMP_BASE"
TEMP_BASE=$(cd "$TEMP_BASE" && pwd -P)
for protected_app in "$APP_PATH" "$SOURCE_APP"; do
    [[ -n $protected_app ]] || continue
    if path_is_within "$TEMP_BASE" "$protected_app"; then
        TEMP_BASE=$(cd /tmp && pwd -P)
        break
    fi
done
TMPDIR="$TEMP_BASE/"
export TMPDIR
WORK_DIR=$(mktemp -d "$TEMP_BASE/librecad-package-verify.XXXXXX")
if [[ -n $MANIFEST_PATH ]]; then
    PENDING_MANIFEST="$WORK_DIR/verified-app.manifest"
fi
trap cleanup EXIT
trap 'exit 130' HUP INT TERM

is_macho() {
    file -b "$1" 2>/dev/null | grep -q 'Mach-O'
}

plist_value() {
    local plist=$1
    local key=$2
    local value

    if value=$(plutil -extract "$key" raw -o - "$plist" 2>/dev/null); then
        printf '%s\n' "$value"
        return 0
    fi
    /usr/libexec/PlistBuddy -c "Print :$key" "$plist" 2>/dev/null
}

canonical_existing_path() {
    local path=$1
    local target
    local depth=0
    local parent

    while [[ -L $path ]]; do
        ((depth += 1))
        [[ $depth -le 32 ]] || return 1
        target=$(readlink "$path") || return 1
        case $target in
            /*)
                path=$target
                ;;
            *)
                path="$(dirname "$path")/$target"
                ;;
        esac
    done
    [[ -f $path ]] || return 1
    parent=$(cd "$(dirname "$path")" 2>/dev/null && pwd -P) || return 1
    printf '%s/%s\n' "$parent" "$(basename "$path")"
}

dependency_candidate_allowed() {
    local app=$1
    local candidate=$2
    local resolved

    resolved=$(canonical_existing_path "$candidate") || return 1
    case $resolved in
        "$app"/*|/System/Library/*|/usr/lib/*)
            return 0
            ;;
    esac
    return 1
}

dependency_resolves() {
    local app=$1
    local binary=$2
    local dependency=$3
    local suffix
    local candidate
    local rpath
    local expanded

    case $dependency in
        /System/Library/*|/usr/lib/*)
            return 0
            ;;
        @loader_path/*)
            suffix=${dependency#@loader_path/}
            candidate="$(dirname "$binary")/$suffix"
            dependency_candidate_allowed "$app" "$candidate"
            return
            ;;
        @executable_path/*)
            suffix=${dependency#@executable_path/}
            candidate="$app/Contents/MacOS/$suffix"
            dependency_candidate_allowed "$app" "$candidate"
            return
            ;;
        @rpath/*)
            suffix=${dependency#@rpath/}
            candidate="$app/Contents/Frameworks/$suffix"
            if dependency_candidate_allowed "$app" "$candidate"; then
                return 0
            fi
            while IFS= read -r rpath; do
                [[ -n $rpath ]] || continue
                case $rpath in
                    @loader_path)
                        expanded=$(dirname "$binary")
                        ;;
                    @loader_path/*)
                        expanded="$(dirname "$binary")/${rpath#@loader_path/}"
                        ;;
                    @executable_path)
                        expanded="$app/Contents/MacOS"
                        ;;
                    @executable_path/*)
                        expanded="$app/Contents/MacOS/${rpath#@executable_path/}"
                        ;;
                    /*)
                        expanded=$rpath
                        ;;
                    *)
                        continue
                        ;;
                esac
                candidate="$expanded/$suffix"
                if dependency_candidate_allowed "$app" "$candidate"; then
                    return 0
                fi
            done < <(otool -l "$binary" | awk '$1 == "cmd" && $2 == "LC_RPATH" { getline; getline; print $2 }')
            return 1
            ;;
        *)
            return 1
            ;;
    esac
}

validate_rpaths() {
    local binary=$1
    local rpath

    while IFS= read -r rpath; do
        [[ -n $rpath ]] || continue
        case $rpath in
            @loader_path|@loader_path/*|@executable_path|@executable_path/*)
                ;;
            *)
                fail "forbidden LC_RPATH '$rpath' in $binary"
                ;;
        esac
    done < <(otool -l "$binary" | awk '$1 == "cmd" && $2 == "LC_RPATH" { getline; getline; print $2 }')
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

write_manifest() {
    local root=$1
    local output=$2
    local relative
    local target

    : > "$output"
    (
        cd "$root"
        find . -type f -exec shasum -a 256 {} + |
            LC_ALL=C sort | awk '{ print "FILE\t" $0 }'
        find . -type l -print | LC_ALL=C sort | while IFS= read -r relative; do
            target=$(readlink "$relative")
            printf 'LINK\t%s\t%s\n' "$target" "$relative"
        done
    ) >> "$output"
}

verify_developer_id_signature() {
    local target=$1
    local description=$2
    local require_runtime=$3
    local expected_team=${4:-}
    local details
    local observed_team

    details=$(codesign -d --verbose=4 "$target" 2>&1) ||
        fail "cannot inspect $description signature: $target"
    grep -q '^Authority=Developer ID Application:' <<< "$details" ||
        fail "$description is not signed with a Developer ID Application identity: $target"
    observed_team=$(awk -F= '$1 == "TeamIdentifier" { print $2; exit }' <<< "$details")
    [[ -n $observed_team && $observed_team != "not set" ]] ||
        fail "Developer ID $description has no TeamIdentifier: $target"
    if [[ -n $expected_team && $observed_team != "$expected_team" ]]; then
        fail "Developer ID $description uses team $observed_team, expected $expected_team: $target"
    fi
    grep -q '^Timestamp=' <<< "$details" ||
        fail "Developer ID $description has no trusted timestamp: $target"
    if [[ $require_runtime -eq 1 ]]; then
        grep -q 'flags=.*runtime' <<< "$details" ||
            fail "Developer ID $description does not enable the hardened runtime: $target"
    fi
}

signature_team_identifier() {
    local target=$1
    local details

    details=$(codesign -d --verbose=4 "$target" 2>&1) || return 1
    awk -F= '$1 == "TeamIdentifier" { print $2; exit }' <<< "$details"
}

verify_signature_mode() {
    local app=$1
    local details

    [[ $MODE != unsigned ]] || return 0

    case $MODE in
        adhoc)
            details=$(codesign -d --verbose=4 "$app" 2>&1) ||
                fail "cannot inspect application signature: $app"
            grep -q '^Signature=adhoc$' <<< "$details" ||
                fail "application is not ad-hoc signed: $app"
            ;;
        developer-id|notarized)
            verify_developer_id_signature "$app" application 1
            ;;
    esac
}

verify_app() {
    local app=$1
    local plist="$app/Contents/Info.plist"
    local executable_name
    local bundle_identifier
    local executable
    local architectures
    local required_arch
    local binary_architectures
    local binary
    local dependency
    local nested
    local app_team=
    local signature_details
    local smoke_app
    local smoke_executable
    local macho_list="$WORK_DIR/macho-list.txt"
    local nested_list="$WORK_DIR/nested-list.txt"

    [[ -d $app && ! -L $app ]] || fail "application bundle is missing or is a symlink: $app"
    [[ -f $plist ]] || fail "Info.plist does not exist: $plist"
    plutil -lint "$plist" >/dev/null || fail "invalid Info.plist: $plist"

    executable_name=$(plist_value "$plist" CFBundleExecutable) ||
        fail "CFBundleExecutable is missing from $plist"
    [[ -n $executable_name ]] || fail "CFBundleExecutable is empty in $plist"
    case $executable_name in
        */*|.|..)
            fail "CFBundleExecutable is not a bundle-local file name: $executable_name"
            ;;
    esac
    bundle_identifier=$(plist_value "$plist" CFBundleIdentifier) ||
        fail "CFBundleIdentifier is missing from $plist"
    [[ -n $bundle_identifier ]] || fail "CFBundleIdentifier is empty in $plist"
    [[ $bundle_identifier =~ ^[A-Za-z0-9][A-Za-z0-9.-]*$ ]] ||
        fail "CFBundleIdentifier is invalid: $bundle_identifier"
    executable="$app/Contents/MacOS/$executable_name"
    [[ -f $executable && -x $executable ]] ||
        fail "main executable is missing or not executable: $executable"
    is_macho "$executable" || fail "main executable is not Mach-O: $executable"

    while IFS= read -r -d '' binary; do
        is_macho "$binary" ||
            fail "non-Mach-O payload is stored in Contents/MacOS: $binary"
    done < <(find "$app/Contents/MacOS" -type f -print0)

    if [[ -d $app/Contents/Resources/plugins ]]; then
        while IFS= read -r -d '' binary; do
            if is_macho "$binary"; then
                fail "executable code is stored under Contents/Resources/plugins: $binary"
            fi
        done < <(find "$app/Contents/Resources/plugins" -type f -print0)
    fi

    architectures=${EXPECTED_ARCHITECTURES//,/ }
    if [[ -z $architectures ]]; then
        architectures=$(lipo -archs "$executable") ||
            fail "cannot read architectures from $executable"
    fi
    [[ -n $architectures ]] || fail "no expected architecture was determined"

    if [[ $MODE == developer-id || $MODE == notarized ]]; then
        app_team=$(signature_team_identifier "$app") ||
            fail "cannot read Developer ID team from application: $app"
        [[ -n $app_team && $app_team != "not set" ]] ||
            fail "Developer ID application has no TeamIdentifier: $app"
    fi

    : > "$macho_list"
    while IFS= read -r -d '' binary; do
        if is_macho "$binary"; then
            printf '%s\n' "$binary" >> "$macho_list"
        fi
    done < <(find "$app/Contents" -type f -print0)
    LC_ALL=C sort -o "$macho_list" "$macho_list"
    [[ -s $macho_list ]] || fail "application contains no Mach-O files: $app"

    while IFS= read -r binary; do
        binary_architectures=$(lipo -archs "$binary") ||
            fail "cannot inspect architectures: $binary"
        for required_arch in $architectures; do
            case " $binary_architectures " in
                *" $required_arch "*)
                    ;;
                *)
                    fail "missing architecture $required_arch in $binary (has: $binary_architectures)"
                    ;;
            esac
        done

        validate_rpaths "$binary"
        while IFS= read -r dependency; do
            [[ -n $dependency ]] || continue
            dependency_resolves "$app" "$binary" "$dependency" ||
                fail "unresolved or external dependency '$dependency' in $binary"
        done < <(list_load_dependencies "$binary")

        if [[ $MODE != unsigned ]]; then
            codesign --verify --strict --verbose=2 "$binary" >/dev/null 2>&1 ||
                fail "invalid nested code signature: $binary"
            if [[ -n $app_team ]]; then
                verify_developer_id_signature "$binary" "nested code" 1 "$app_team"
            fi
        fi
    done < "$macho_list"

    if [[ $MODE != unsigned ]]; then
        : > "$nested_list"
        while IFS= read -r -d '' nested; do
            printf '%s\n' "$nested" >> "$nested_list"
        done < <(find "$app/Contents" -type d \( -name '*.framework' -o -name '*.app' -o -name '*.appex' -o -name '*.xpc' -o -name '*.plugin' \) -print0)
        LC_ALL=C sort -o "$nested_list" "$nested_list"
        while IFS= read -r nested; do
            [[ -n $nested ]] || continue
            codesign --verify --strict --verbose=2 "$nested" >/dev/null 2>&1 ||
                fail "invalid nested bundle signature: $nested"
            if [[ -n $app_team ]]; then
                verify_developer_id_signature "$nested" "nested bundle" 1 "$app_team"
            fi
        done < "$nested_list"

        codesign --verify --deep --strict --verbose=2 "$app" >/dev/null 2>&1 ||
            fail "application signature verification failed: $app"
        verify_signature_mode "$app"
    fi

    if [[ $SMOKE_TEST -eq 1 ]]; then
        smoke_app="$WORK_DIR/smoke/$(basename "$app")"
        mkdir -p "$WORK_DIR/smoke"
        ditto --rsrc --extattr --acl "$app" "$smoke_app"
        smoke_executable="$smoke_app/Contents/MacOS/$executable_name"
        if ! ("$smoke_executable" --help >/dev/null 2>&1) 2>/dev/null; then
            fail "application --help smoke test failed in disposable copy"
        fi
    fi

    signature_details=unsigned
    if [[ $MODE != unsigned ]]; then
        signature_details=$(codesign -d --verbose=1 "$app" 2>&1 | tr '\n' ' ')
    fi
    log "app=$app"
    log "bundle_identifier=$bundle_identifier"
    log "mode=$MODE"
    log "architectures=$architectures"
    log "macho_count=$(wc -l < "$macho_list" | tr -d ' ')"
    log "signature=$signature_details"
}

verify_dmg() {
    local dmg=$1
    local app
    local expected_app_name=
    local entry
    local gatekeeper_assessment=
    local dmg_team=
    local app_team=
    local name
    local source_manifest="$WORK_DIR/source.manifest"
    local mounted_manifest="$WORK_DIR/mounted.manifest"
    local apps=()

    [[ -f $dmg ]] || fail "DMG does not exist: $dmg"
    hdiutil verify "$dmg" >/dev/null 2>&1 || fail "hdiutil verification failed: $dmg"

    if [[ $MODE == developer-id || $MODE == notarized ]]; then
        if [[ $ALLOW_UNSIGNED_DMG -eq 0 ]]; then
            codesign --verify --strict --verbose=2 "$dmg" >/dev/null 2>&1 ||
                fail "Developer ID DMG signature verification failed: $dmg"
            verify_developer_id_signature "$dmg" DMG 0
            dmg_team=$(signature_team_identifier "$dmg") ||
                fail "cannot read Developer ID team from DMG: $dmg"
        fi
    fi

    if [[ $MODE == notarized && $ALLOW_UNSIGNED_DMG -eq 0 ]]; then
        xcrun stapler validate "$dmg" >/dev/null 2>&1 ||
            fail "notarization ticket validation failed: $dmg"
        spctl --assess --type open --context context:primary-signature --verbose=2 "$dmg" >/dev/null 2>&1 ||
            fail "Gatekeeper rejected notarized DMG: $dmg"
    fi

    MOUNT_POINT="$WORK_DIR/mount"
    mkdir -p "$MOUNT_POINT"
    hdiutil attach -readonly -nobrowse -mountpoint "$MOUNT_POINT" "$dmg" >/dev/null 2>&1 ||
        fail "cannot mount DMG read-only: $dmg"
    ATTACHED=1

    if [[ -n $SOURCE_APP ]]; then
        expected_app_name=$(basename "$SOURCE_APP")
    fi

    shopt -s nullglob
    apps=("$MOUNT_POINT"/*.app)
    shopt -u nullglob
    [[ ${#apps[@]} -eq 1 ]] ||
        fail "DMG must contain exactly one application bundle (found ${#apps[@]})"
    app=$(absolute_existing_path "${apps[0]}")
    if [[ -n $expected_app_name && $(basename "$app") != "$expected_app_name" ]]; then
        fail "DMG app name '$(basename "$app")' does not match '$expected_app_name'"
    fi

    [[ -L $MOUNT_POINT/Applications ]] ||
        fail "DMG does not contain an Applications symlink"
    [[ $(readlink "$MOUNT_POINT/Applications") == /Applications ]] ||
        fail "DMG Applications symlink does not target /Applications"

    while IFS= read -r entry; do
        name=$(basename "$entry")
        case $name in
            Applications|"$(basename "$app")")
                ;;
            *)
                fail "unexpected top-level DMG entry: $name"
                ;;
        esac
    done < <(find "$MOUNT_POINT" -mindepth 1 -maxdepth 1 -print)

    verify_app "$app"

    if [[ -n $dmg_team ]]; then
        app_team=$(signature_team_identifier "$app") ||
            fail "cannot read Developer ID team from mounted application: $app"
        [[ $app_team == "$dmg_team" ]] ||
            fail "DMG team $dmg_team does not match application team $app_team"
    fi

    if [[ -n $SOURCE_APP ]]; then
        [[ -d $SOURCE_APP ]] || fail "source application does not exist: $SOURCE_APP"
        write_manifest "$SOURCE_APP" "$source_manifest"
        write_manifest "$app" "$mounted_manifest"
        if ! cmp -s "$source_manifest" "$mounted_manifest"; then
            fail "mounted application content differs from finalized source app"
        fi
        log "source_manifest_match=yes"
    fi

    if [[ $MODE == adhoc || $MODE == developer-id ]]; then
        if spctl --assess --type open --context context:primary-signature \
            "$dmg" >/dev/null 2>&1
        then
            gatekeeper_assessment=accepted
        else
            gatekeeper_assessment=rejected
        fi
        log "gatekeeper_assessment=$gatekeeper_assessment"
    fi

    if [[ -n $PENDING_MANIFEST ]]; then
        write_manifest "$app" "$PENDING_MANIFEST"
    fi

    log "dmg=$dmg"
    log "sha256=$(shasum -a 256 "$dmg" | awk '{print $1}')"
    log "mounted_layout=valid"
}

if [[ -n $APP_PATH ]]; then
    verify_app "$APP_PATH"
    if [[ -n $PENDING_MANIFEST ]]; then
        write_manifest "$APP_PATH" "$PENDING_MANIFEST"
    fi
else
    verify_dmg "$DMG_PATH"
    detach_mounted_image || fail "failed to detach $MOUNT_POINT"
fi

if [[ -n $PENDING_MANIFEST ]]; then
    mv "$PENDING_MANIFEST" "$MANIFEST_PATH" ||
        fail "cannot publish manifest: $MANIFEST_PATH"
fi
log "result=ok"
