#!/usr/bin/env bash
#
# dwg-review-lint.sh — static-analysis harness for LibreCAD's DWG/DXF subsystem.
#
# Runs, over the libdxfrw engine + the rs_filterdxfrw bridge (excluding the
# *.orig backups and the generated codepage tables):
#   1. a strict-warning syntax pass per engine TU (libdxfrw is Qt-free, so it
#      compiles standalone — no build tree required);
#   2. the clang static analyzer (clang --analyze) per engine TU;
#   3. cppcheck, if installed;
#   4. clang-tidy, if installed and a compile_commands.json is available.
#
# Everything degrades gracefully: missing tools are reported and skipped.
# Output goes to build-review-lint/ as per-tool logs plus a summary.
#
# Usage:  scripts/dwg-review-lint.sh [--quick]
#   --quick   skip the (slow) clang --analyze pass
#
set -uo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
ENGINE="$ROOT/libraries/libdxfrw/src"
FILTER="$ROOT/librecad/src/lib/filters"
OUT="$ROOT/build-review-lint"
QUICK=0
[ "${1:-}" = "--quick" ] && QUICK=1

mkdir -p "$OUT"

# Strict warning set. -Wsign-conversion is the dominant (mostly-benign) category
# in bit-parsing code; keep it on but expect to baseline it.
WARN="-Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion \
-Wold-style-cast -Wimplicit-int-conversion -Wfloat-equal -Wcast-align \
-Wunused -Wnon-virtual-dtor -Woverloaded-virtual"
STD="-std=c++17"
INC="-I $ENGINE -I $ENGINE/intern"

# Engine translation units (Qt-free; compile standalone). Codepage tables
# (drw_cptable*.h) and *.orig backups are intentionally excluded.
ENGINE_TUS=(
  drw_base.cpp drw_classes.cpp drw_entities.cpp drw_objects.cpp
  drw_header.cpp libdxfrw.cpp libdwgr.cpp
  intern/dwgbuffer.cpp intern/dwgbufferw.cpp intern/dwgreader.cpp
  intern/dwgreader15.cpp intern/dwgreader18.cpp intern/dwgreader21.cpp
  intern/dwgreader24.cpp intern/dwgreader27.cpp intern/dwgreader32.cpp
  intern/dwgwriter15.cpp intern/dwgwriter18.cpp intern/dwgwriter24.cpp
  intern/dwgwriter27.cpp intern/dwgwriter32.cpp
  intern/dwgutil.cpp intern/drw_textcodec.cpp intern/dxfreader.cpp
  intern/dxfwriter.cpp intern/rscodec.cpp
)

echo "== DWG/DXF static-analysis harness =="
echo "root: $ROOT"
echo "out:  $OUT"
echo

# ---------------------------------------------------------------------------
# 1. Strict-warning syntax pass (per category counts)
# ---------------------------------------------------------------------------
echo "[1/4] strict-warning pass (clang -fsyntax-only) ..."
: > "$OUT/warnings.log"
for tu in "${ENGINE_TUS[@]}"; do
  [ -f "$ENGINE/$tu" ] || continue
  clang++ $STD -fsyntax-only $WARN $INC "$ENGINE/$tu" 2>>"$OUT/warnings.log"
done
echo "  warnings by category:"
grep -oE '\[-W[a-z-]+\]' "$OUT/warnings.log" | sort | uniq -c | sort -rn | sed 's/^/    /'
echo "  total: $(grep -c 'warning:' "$OUT/warnings.log") (full log: $OUT/warnings.log)"
echo

# ---------------------------------------------------------------------------
# 2. clang static analyzer
# ---------------------------------------------------------------------------
if [ "$QUICK" = "0" ]; then
  echo "[2/4] clang static analyzer (clang --analyze) ..."
  : > "$OUT/analyzer.log"
  for tu in "${ENGINE_TUS[@]}"; do
    [ -f "$ENGINE/$tu" ] || continue
    clang++ $STD --analyze -Xclang -analyzer-output=text $INC \
      "$ENGINE/$tu" 2>>"$OUT/analyzer.log" >/dev/null
  done
  echo "  analyzer warnings: $(grep -c 'warning:' "$OUT/analyzer.log") (log: $OUT/analyzer.log)"
else
  echo "[2/4] clang static analyzer ... SKIPPED (--quick)"
fi
echo

# ---------------------------------------------------------------------------
# 3. cppcheck (optional)
# ---------------------------------------------------------------------------
if command -v cppcheck >/dev/null 2>&1; then
  echo "[3/4] cppcheck ..."
  cppcheck --std=c++17 --enable=warning,style,performance,portability \
    --inline-suppr --quiet -I "$ENGINE" -I "$ENGINE/intern" \
    "${ENGINE_TUS[@]/#/$ENGINE/}" "$FILTER/rs_filterdxfrw.cpp" \
    2>"$OUT/cppcheck.log"
  echo "  cppcheck issues: $(grep -c ':' "$OUT/cppcheck.log") (log: $OUT/cppcheck.log)"
else
  echo "[3/4] cppcheck ... NOT INSTALLED (brew install cppcheck) — skipped"
fi
echo

# ---------------------------------------------------------------------------
# 4. clang-tidy (optional; needs compile_commands.json)
# ---------------------------------------------------------------------------
DB="$ROOT/.qtc_clangd/compile_commands.json"
if command -v clang-tidy >/dev/null 2>&1 && [ -f "$DB" ]; then
  echo "[4/4] clang-tidy (compile DB: $DB) ..."
  CHECKS='-*,bugprone-*,cppcoreguidelines-*,clang-analyzer-*,performance-*,portability-*,misc-*,modernize-*,-modernize-use-trailing-return-type'
  : > "$OUT/clang-tidy.log"
  for tu in "${ENGINE_TUS[@]}"; do
    [ -f "$ENGINE/$tu" ] || continue
    clang-tidy -p "$(dirname "$DB")" --checks="$CHECKS" "$ENGINE/$tu" \
      >>"$OUT/clang-tidy.log" 2>/dev/null
  done
  clang-tidy -p "$(dirname "$DB")" --checks="$CHECKS" \
    "$FILTER/rs_filterdxfrw.cpp" >>"$OUT/clang-tidy.log" 2>/dev/null
  echo "  clang-tidy warnings: $(grep -c 'warning:' "$OUT/clang-tidy.log") (log: $OUT/clang-tidy.log)"
else
  echo "[4/4] clang-tidy ... NOT AVAILABLE (need clang-tidy + $DB) — skipped"
fi
echo
echo "== done. logs in $OUT/ =="
