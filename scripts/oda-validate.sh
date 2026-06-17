#!/usr/bin/env bash
# oda-validate.sh -- best-effort ODA File Converter interop gate.
#
# The DWG/DXF roadmap needs an external commercial-reader check in addition to
# libdxfrw self round trips, ezdxf DXF audit, and libreDWG dwgread.  This script
# wraps the locally installed ODA File Converter CLI shape:
#
#   ODAFileConverter <input_dir> <output_dir> <version> <type> <recurse> <audit> [filter]
#
# Usage:
#   scripts/oda-validate.sh INPUT_DIR OUTPUT_DIR [VERSION] [TYPE] [RECURSE] [AUDIT]
#
# Examples:
#   scripts/oda-validate.sh tmp/in tmp/oda-dxf ACAD2018 DXF 0 1
#   ODA_FILTER='*.dwg' RUN_EZDXF_AUDIT=1 scripts/oda-validate.sh tmp/in tmp/oda-dxf
#
# Env overrides:
#   ODAFC             path to ODAFileConverter
#   ODA_VERSION       default output version (default: ACAD2018)
#   ODA_TYPE          default output type: DWG or DXF (default: DXF)
#   ODA_RECURSIVE     recurse flag passed to ODA (default: 0)
#   ODA_AUDIT         audit flag passed to ODA (default: 1)
#   ODA_FILTER        optional input filter, e.g. '*.dwg' or '*.dxf'
#   RUN_EZDXF_AUDIT   run scripts/ezdxf_audit.py for DXF outputs when set to 1
#   RUN_DWGREAD       run DWGREAD over DWG outputs when set to 1
#   DWGREAD           path to libreDWG dwgread

set -uo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
DEFAULT_ODAFC="/Applications/ODAFileConverter.app/Contents/MacOS/ODAFileConverter"
ALT_ODAFC="/Applications/ODA File Converter.app/Contents/MacOS/ODAFileConverter"
ODAFC="${ODAFC:-}"
DWGREAD="${DWGREAD:-$HOME/dev/libredwg/programs/dwgread}"

usage() {
    sed -n '3,24p' "$0" >&2
}

die_setup() {
    echo "error: $*" >&2
    exit 2
}

find_oda() {
    if [[ -n "$ODAFC" ]]; then
        echo "$ODAFC"
        return
    fi
    if [[ -x "$DEFAULT_ODAFC" ]]; then
        echo "$DEFAULT_ODAFC"
        return
    fi
    if [[ -x "$ALT_ODAFC" ]]; then
        echo "$ALT_ODAFC"
        return
    fi
    command -v ODAFileConverter 2>/dev/null || true
}

if [[ $# -lt 2 || "${1:-}" == "-h" || "${1:-}" == "--help" ]]; then
    usage
    exit 2
fi

INPUT_DIR="$1"
OUTPUT_DIR="$2"
VERSION="${3:-${ODA_VERSION:-ACAD2018}}"
OUT_TYPE="${4:-${ODA_TYPE:-DXF}}"
RECURSE="${5:-${ODA_RECURSIVE:-0}}"
AUDIT="${6:-${ODA_AUDIT:-1}}"

[[ -d "$INPUT_DIR" ]] || die_setup "input directory not found: $INPUT_DIR"

ODA_BIN="$(find_oda)"
[[ -n "$ODA_BIN" && -x "$ODA_BIN" ]] || die_setup "ODAFileConverter not found/executable (set ODAFC=...)"

mkdir -p "$OUTPUT_DIR" || die_setup "cannot create output directory: $OUTPUT_DIR"

echo "== ODA File Converter"
echo "   binary:  $ODA_BIN"
echo "   input:   $INPUT_DIR"
echo "   output:  $OUTPUT_DIR"
echo "   version: $VERSION"
echo "   type:    $OUT_TYPE"
echo "   recurse: $RECURSE"
echo "   audit:   $AUDIT"
if [[ -n "${ODA_FILTER:-}" ]]; then
    echo "   filter:  $ODA_FILTER"
fi

if [[ -n "${ODA_FILTER:-}" ]]; then
    "$ODA_BIN" "$INPUT_DIR" "$OUTPUT_DIR" "$VERSION" "$OUT_TYPE" "$RECURSE" "$AUDIT" "$ODA_FILTER"
else
    "$ODA_BIN" "$INPUT_DIR" "$OUTPUT_DIR" "$VERSION" "$OUT_TYPE" "$RECURSE" "$AUDIT"
fi
oda_status=$?
if [[ "$oda_status" -ne 0 ]]; then
    echo "ODA: FAIL (exit $oda_status)"
    echo "If this is a CLI-shape mismatch, run the converter once manually and set ODAFC/ODA_* env vars."
    exit 1
fi

case "${OUT_TYPE^^}" in
    DXF) pattern="*.dxf" ;;
    DWG) pattern="*.dwg" ;;
    *) pattern="*" ;;
esac

count="$(find "$OUTPUT_DIR" -type f -iname "$pattern" | wc -l | tr -d ' ')"
echo "ODA: PASS (produced $count $pattern file(s))"
if [[ "$count" == "0" ]]; then
    echo "error: ODA completed but no $pattern outputs were found" >&2
    exit 1
fi

if [[ "${OUT_TYPE^^}" == "DXF" && "${RUN_EZDXF_AUDIT:-0}" == "1" ]]; then
    [[ -f "$ROOT/scripts/ezdxf_audit.py" ]] || die_setup "missing scripts/ezdxf_audit.py"
    echo "== ezdxf audit of ODA DXF outputs"
    python3 "$ROOT/scripts/ezdxf_audit.py" "$OUTPUT_DIR"
fi

if [[ "${OUT_TYPE^^}" == "DWG" && "${RUN_DWGREAD:-0}" == "1" ]]; then
    [[ -x "$DWGREAD" ]] || die_setup "dwgread not found/executable at $DWGREAD (set DWGREAD=...)"
    echo "== libreDWG dwgread of ODA DWG outputs"
    dwg_fail=0
    while IFS= read -r -d '' file; do
        out="$("$DWGREAD" "$file" 2>&1)"
        if grep -qE 'Failed to decode|^ERROR 0x|Assertion failed' <<<"$out"; then
            echo "FAIL  $file"
            grep -m1 -E 'Failed to decode|^ERROR 0x|Assertion failed' <<<"$out" | sed 's/^[[:space:]]*/      /'
            dwg_fail=1
        else
            echo "OK    $file"
        fi
    done < <(find "$OUTPUT_DIR" -type f -iname '*.dwg' -print0 | sort -z)
    [[ "$dwg_fail" -eq 0 ]] || exit 1
fi

exit 0
