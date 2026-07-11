#!/usr/bin/env bash
# dwg-validate.sh — external write->reread framing validator (Phase 1, 1.7).
#
# Emits 5 trivial DWG fixtures (AC1015/18/24/27/32) via the libdxfrw writer
# (the hidden Catch2 test [.dwg_emit_framing]) and re-reads each with
# libreDWG's external `dwgread`, capturing its error mask. This is the
# write-path EXIT GATE for every write phase (2a/2b/3/4/5/6/9): a write change
# must not regress the external-read status of the AC1015/AC1018 fixtures.
#
# IMPORTANT (the core finding): libdxfrw's own reader re-reading its own output
# (in-repo self-consistency) is NECESSARY but NOT SUFFICIENT. The thin
# AC1027/AC1032 writers can pass self-consistency yet fail external dwgread.
# This script surfaces that divergence instead of hiding it.
#
# Usage:
#   scripts/dwg-validate.sh
#
# Env overrides:
#   BUILD_DIR   build directory (default: ./build)
#   DWGREAD     path to libreDWG dwgread (default: ~/dev/libreDWG/programs/dwgread)
#
# Exit status:
#   0  the GATE fixture (AC1015) reads via dwgread WITHOUT a fatal error mask.
#   1  the AC1015 gate regressed (a fatal external error appeared).
#   2  setup error (no dwgread / build failure / fixtures not emitted).
#
# OBSERVED BASELINE (libreDWG dwgread 0.13.3, 2026-05-31 — the first honest
# external read of libdxfrw output):
#   AC1015  PASS (with bit_read buffer-overflow warnings; dwgread ends SUCCESS)
#   AC1018  FAIL ("Failed to decode ... 0x100" — Data Section Page Map)
#   AC1024  FAIL (libreDWG assertion in decompress_R2004_section)
#   AC1027  FAIL (same R2004 assertion)
#   AC1032  FAIL ("Failed to decode ... 0x100")
# i.e. ONLY AC1015 currently survives an external reader. AC1018 and up are
# real interop gaps the in-repo self-consistency tests do not catch — this is
# the core finding 1.7 was built to surface. AC1018+ are reported but do NOT
# gate the exit status until their writers are hardened (Phase 9 / framing
# work); the gate enforces AC1015 only so write phases have a real, passing
# external check to not regress.

set -uo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="${BUILD_DIR:-$ROOT/build}"
DWGREAD="${DWGREAD:-$HOME/dev/libreDWG/programs/dwgread}"
TEST_BIN="$BUILD_DIR/librecad_tests"
FIXTURE_DIR="$ROOT/tmp/dwg-validate"

die() { echo "error: $*" >&2; exit 2; }

# The enforceable gate set vs the known-untrusted set. Only AC1015 currently
# passes an external read, so it is the sole hard gate; AC1018+ are reported
# as KNOWN-UNTRUSTED (real interop gaps, see header BASELINE) until hardened.
GATE_VERSIONS=(AC1015)
UNTRUSTED_VERSIONS=(AC1018 AC1024 AC1027 AC1032)

[[ -x "$DWGREAD" ]] || die "dwgread not found/executable at $DWGREAD (set DWGREAD=...)"
[[ -x "$TEST_BIN" ]] || die "librecad_tests not built at $TEST_BIN (run: scripts/dwg-iterate.sh build)"

echo "== dwgread: $("$DWGREAD" --version 2>&1 | head -1)"

# Emit the 5 fixtures via the hidden test. Run from ROOT so the test's relative
# tmp/dwg-validate path lands where this script looks.
echo "== emitting framing fixtures via [.dwg_emit_framing]"
( cd "$ROOT" && "$TEST_BIN" "[.dwg_emit_framing]" >/dev/null 2>&1 ) \
    || die "hidden emit test [.dwg_emit_framing] failed"

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
    local out; out="$("$DWGREAD" "$f" 2>&1)"
    local verline; verline="$(grep -oE 'AC10[0-9][0-9]' <<<"$out" | head -1)"
    if is_fatal "$out"; then
        echo "FAIL     $ver  ($(grep -m1 -E 'Failed to decode|^ERROR 0x|Assertion failed' <<<"$out" | sed 's/^[[:space:]]*//'))"
        return 1
    fi
    local warn=""
    grep -q 'buffer overflow' <<<"$out" && warn=" [overflow warnings]"
    echo "PASS     $ver${warn}"
    return 0
}

echo "== external dwgread re-read of emitted fixtures"
gate_fail=0
for v in "${GATE_VERSIONS[@]}"; do
    run_one "$v" || gate_fail=1
done

echo "== KNOWN-UNTRUSTED (thin/unverified writers; do NOT gate the exit)"
for v in "${UNTRUSTED_VERSIONS[@]}"; do
    run_one "$v" || true
done

echo
if [[ "$gate_fail" -ne 0 ]]; then
    echo "GATE: FAIL — the AC1015 external read regressed."
    echo "      (write-path phases 2a/2b/3/4/5/6/9 must keep AC1015 green)"
    exit 1
fi
echo "GATE: PASS — the AC1015 fixture reads via external dwgread without a fatal mask."
echo "NOTE: AC1018/AC1024/AC1027/AC1032 are KNOWN-UNTRUSTED — they currently FAIL"
echo "      external dwgread (see script header BASELINE). Hardening those writers"
echo "      is Phase 9 / framing work; the in-repo self-consistency loop alone does"
echo "      NOT prove external readability."
exit 0
