#!/usr/bin/env bash
# dwg-validate.sh — external write->reread framing validator (Phase 1, 1.7).
#
# Emits 5 trivial DWG fixtures (AC1015/18/24/27/32) via the libdxfrw writer
# (the hidden Catch2 test [dwg_emit_framing]) and re-reads each with
# libreDWG's external `dwgread`, capturing its error mask. This is the
# write-path EXIT GATE for every write phase (2a/2b/3/4/5/6/9): a write change
# must not regress the external-read status of every emitted fixture.
#
# IMPORTANT (the core finding): libdxfrw's own reader re-reading its own output
# (in-repo self-consistency) is NECESSARY but NOT SUFFICIENT. This script
# checks the framing fixtures with an independent reader as well.
#
# Usage:
#   scripts/dwg-validate.sh
#
# Env overrides:
#   BUILD_DIR   build directory (default: ./build)
#   DWGREAD     path to libreDWG dwgread (default: ~/dev/libreDWG/programs/dwgread)
#
# Exit status:
#   0  every framing fixture reads via dwgread WITHOUT a fatal error mask.
#   1  an external framing gate regressed (a fatal external error appeared).
#   2  setup error (no dwgread / build failure / fixtures not emitted).
#
# OBSERVED BASELINE (libreDWG dwgread 0.13.3, 2026-07-17):
#   AC1015  PASS (with bit_read buffer-overflow warnings; dwgread ends SUCCESS)
#   AC1018  PASS (with bit_read buffer-overflow warnings; dwgread ends SUCCESS)
#   AC1024  PASS (with buffer-overflow warnings from unsupported fixture detail)
#   AC1027  PASS (with buffer-overflow warnings from unsupported fixture detail)
#   AC1032  PASS (with buffer-overflow warnings from unsupported fixture detail)
# The R2004-container repair uses canonical capacity-sized, 0x20-aligned data
# pages and fixed the former external 0x840 rejection. This is a structural
# framing gate only; feature-specific writer promotion still needs its own
# fixtures and ODA/AutoCAD validation where available.

set -uo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="${BUILD_DIR:-$ROOT/build}"
DWGREAD="${DWGREAD:-$HOME/dev/libreDWG/programs/dwgread}"
TEST_BIN="$BUILD_DIR/librecad_tests"
FIXTURE_DIR="$ROOT/tmp/dwg-validate"

die() { echo "error: $*" >&2; exit 2; }

# Every emitted version now passes the external structural framing gate.
GATE_VERSIONS=(AC1015 AC1018 AC1024 AC1027 AC1032)

[[ -x "$DWGREAD" ]] || die "dwgread not found/executable at $DWGREAD (set DWGREAD=...)"
[[ -x "$TEST_BIN" ]] || die "librecad_tests not built at $TEST_BIN (run: scripts/dwg-iterate.sh build)"

echo "== dwgread: $("$DWGREAD" --version 2>&1 | head -1)"

# Emit the 5 fixtures via the hidden test. Run from ROOT so the test's relative
# tmp/dwg-validate path lands where this script looks.
echo "== emitting framing fixtures via [dwg_emit_framing]"
( cd "$ROOT" && "$TEST_BIN" "[dwg_emit_framing]" >/dev/null 2>&1 ) \
    || die "hidden emit test [dwg_emit_framing] failed"

# A libreDWG fatal mask: a "Failed to decode" line, an "ERROR 0x" mask line,
# or an assertion abort. Buffer-overflow warnings that still end in SUCCESS are
# reported but do NOT count as fatal for the gate (libreDWG itself returns
# SUCCESS in that case).
is_fatal() {
    local out="$1"
    grep -qE 'Failed to decode|^ERROR 0x|Assertion failed' <<<"$out" && return 0
    # No SUCCESS anywhere and an explicit decode error → fatal.
    if ! grep -q 'SUCCESS' <<<"$out" && grep -q 'ERROR' <<<"$out"; then
        return 0
    fi
    return 1
}

run_one() {
    local ver="$1"
    local f="$FIXTURE_DIR/framing_${ver}.dwg"
    if [[ ! -f "$f" ]]; then
        echo "MISSING  $ver  (fixture not emitted: $f)"
        return 2
    fi
    # minJSON exposes FILEHEADER.version without depending on localized or
    # verbosity-dependent diagnostic text from the external reader.
    local out; out="$("$DWGREAD" -O minJSON "$f" 2>&1)"
    local verline; verline="$(sed -n 's/^[[:space:]]*"version": "\(AC10[0-9][0-9]\)".*/\1/p' <<<"$out" | head -1)"
    if is_fatal "$out"; then
        echo "FAIL     $ver  ($(grep -m1 -E 'Failed to decode|^ERROR 0x|Assertion failed' <<<"$out" | sed 's/^[[:space:]]*//'))"
        return 1
    fi
    if [[ "$verline" != "$ver" ]]; then
        echo "FAIL     $ver  (external reader reported ${verline:-no version})"
        return 1
    fi
    # These fixtures deliberately contain two LINE entities and one named
    # FRAME layer. Keep the receipts small and stable across DWG versions.
    local line_count; line_count="$(awk '/"entity": "LINE"/{count++} END{print count + 0}' <<<"$out")"
    local frame_count; frame_count="$(awk '/"name": "FRAME"/{count++} END{print count + 0}' <<<"$out")"
    if [[ "$line_count" != "2" || "$frame_count" != "1" ]]; then
        echo "FAIL     $ver  (external receipt: LINE=${line_count}, FRAME=${frame_count})"
        return 1
    fi
    local warn=""
    grep -q 'buffer overflow' <<<"$out" && warn=" [overflow warnings]"
    echo "PASS     $ver${warn} (LINE=${line_count}, FRAME=${frame_count})"
    return 0
}

echo "== external dwgread re-read of emitted fixtures"
gate_fail=0
for v in "${GATE_VERSIONS[@]}"; do
    run_one "$v" || gate_fail=1
done

echo
if [[ "$gate_fail" -ne 0 ]]; then
    echo "GATE: FAIL — an external DWG framing read regressed."
    echo "      (write-path phases must keep every emitted version green)"
    exit 1
fi
echo "GATE: PASS — every emitted fixture reads via external dwgread without a fatal mask."
echo "NOTE: this validates container framing only; feature-specific writer support"
echo "      remains subject to its own round-trip and external-oracle fixtures."
exit 0
