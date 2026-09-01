#!/usr/bin/env bash
#
# This file is part of the LibreCAD project, a 2D CAD program
#
# Copyright (C) 2026 LibreCAD (librecad.org)
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
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
# Incrementally build one validation lane and optionally run one focused
# Catch2 selector. No tree is removed or configured unless --configure is used.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
# Keep the normal loop on the dedicated, reduced Ninja graph. BUILD_DIR (or
# the explicit --build-dir option below) remains available for checkpoint
# toolchains and sanitizer trees.
BUILD_DIR="${BUILD_DIR:-$ROOT/build/dwg-fast}"
QMAKE_BUILD_DIR="${QMAKE_BUILD_DIR:-$ROOT/build-qmake6-codex}"
if [[ -z "${JOBS:-}" ]]; then
  JOBS="$(getconf _NPROCESSORS_ONLN 2>/dev/null || printf '2')"
  ((JOBS > 8)) && JOBS=8
fi
SCOPE="cmake"
QMAKE_TARGET="libdxfrw"
TEST_SPEC=""
TEST_FILE=""
TEST_SUITE="auto"
ALLOW_SLOW=0
FAST=0
FULL=0
FAIL_FAST=0
CONFIGURE=0
NO_BUILD=0
BUILD_TARGET=""
TEST_ONLY=0
CHANGED=0
AUTO_CONFIGURE=1
QUIET=0
# The focused run itself reports an unmatched selector, so a second Catch2
# discovery process is unnecessary in the inner loop. Use --discover when
# validating a new selector or target mapping.
TRUST_SELECTOR=1
COMPILE_ONLY=0
TEST_FILE_TARGET=0
EXTRA_BUILD_TARGETS=()

# Keep the source-to-target map in one place. The --changed path uses it to
# avoid selecting a per-source target that was not actually generated; the
# explicit --test-file path uses the same map for deterministic validation.
fast_test_target_for_stem() {
  case "$1" in
  dwg_buffer_round_trip_tests|dwg_decompress18_tests|\
  dwg_entity_encode_round_trip_tests|dwg_header_encode_round_trip_tests|\
  dwg_handseed_tests|dynblock_tests|evalgraph_tests|\
  acsh_tests|acsh_shapes_tests|\
  dwg_object_encode_round_trip_tests|dwg_object_frame_tests|dwg_safety_tests|\
  dxf_attribute_tests|dxf_object_tests|field_dxf_tests|\
  mleader_dxf_context_tests|mesh_tests)
    printf 'libdxfrw_%s\n' "$1"
    ;;
  dwg_header_app_vars_tests|dxf_roundtrip_tests|dxf_string_codec_tests|\
  eed_tests|entity_metadata_tests|\
  i18n_caret_nfc_tests|i18n_codec_tests|datastorage_tests)
    printf 'librecad_%s\n' "$1"
    ;;
  acis_wireframe_tests)
    printf 'librecad_acis_wireframe_tests\n'
    ;;
  dwg_smoke_tests)
    printf 'librecad_dwg_smoke_tests\n'
    ;;
  dwg_write_smoke_tests)
    printf 'librecad_dwg_write_fast_tests\n'
    ;;
  *)
    return 1
    ;;
  esac
}

usage() {
  cat <<'EOF'
Usage: scripts/dwg-fast-check.sh [options]

  --cmake                  Use the CMake lane (default).
  --qmake                  Use the qmake6 lane only.
  --both                   Build both lanes, but run tests once in CMake.
  --qmake-target NAME      qmake target: libdxfrw (default) or librecad.
  --build-dir DIR          CMake build directory (default: build/dwg-fast).
  --test SPEC              Run one focused Catch2 selector after the build.
  --test-file PATH         Build the smallest registered target for one test
                           source, then run SPEC. Slow/corpus/external cases
                           fixture/external-reader cases remain excluded unless
                           explicitly enabled.
  --suite NAME             Focused test lane: auto, dwg, dxf, or all
                           (default: auto; infer from [dwg*]/[dxf*] tags).
  --fast                   Use the parser-only DWG/DXF structural target
                           (the default for focused format tests).
  --full                   Use the complete librecad_tests target.
  --allow-slow             Include hidden/slow tests (opt-in); does not
                           promote a micro lane to the full target.
  --skip-slow              Exclude hidden/slow tests (default).
  --abort                  Stop the selected test run at the first failure.
  --no-build               Reuse the existing test/binary targets.
  --test-only              Reuse the existing target and selector; implies
                           --no-build --trust-selector and requires --test.
                           Pair with --test-file or --build-target when the
                           selector does not identify the executable. Fails
                           if relevant sources/build metadata are newer.
  --changed                Select one incremental lane from the working tree.
                           A single changed fast-test source maps to its
                           micro target; production changes build only the
                           smallest core/archive target. Prefer --test-file
                           with --test in a dirty worktree; otherwise a
                           deliberate family selector may widen the target.
  --compile-only           Build only the selected CMake target; run no tests.
  --quiet                  Capture verbose test diagnostics; print them only
                           when the selected test fails.
  --discover               Validate the selector with Catch2 before running it.
                           Use this when changing a selector or target mapping.
  --trust-selector         Skip selector preflight (the default fast path).
  --no-discover            Alias for --trust-selector.
  --build-target NAME      CMake target for a compile-only check. Use a
                           librecad_* target for filter/application coverage.
  --configure              Force one CMake configuration pass. New trees use
                           Ninja when available; existing trees stay warm.
  --no-configure           Do not initialize a missing CMake tree; fail with
                           setup instructions instead.
  --jobs N                 Parallel build jobs (default: $JOBS).
  -h, --help               Show this help.

Examples:
  scripts/dwg-fast-check.sh
  scripts/dwg-fast-check.sh --test-file \
    librecad/src/lib/filters/tests/dwg_safety_tests.cpp \
    --test '[dwg][safety]'
  scripts/dwg-fast-check.sh --test-file \
    librecad/src/lib/filters/tests/dwg_write_smoke_tests.cpp \
    --test '[.dwg_emit_framing]' --allow-slow
  scripts/dwg-fast-check.sh --fast --test '[dwg][safety]'
  scripts/dwg-fast-check.sh --fast --suite dwg --test '[dwg][safety]'
  scripts/dwg-fast-check.sh --fast --suite dxf --test '[dxf]'
  scripts/dwg-fast-check.sh --full --test '[dwg][read]'
  scripts/dwg-fast-check.sh --build-target librecad_dwg_fast_tests
  scripts/dwg-fast-check.sh --build-target librecad_filter_compile_check
  scripts/dwg-fast-check.sh --build-target libdxfrw_test_core
  scripts/dwg-fast-check.sh --test '[.dwg_emit_framing]' \
    --build-target librecad_dwg_write_fast_tests --allow-slow
  scripts/dwg-fast-check.sh --test-file \
    librecad/src/lib/filters/tests/evalgraph_tests.cpp \
    --test '[dwg][evalgraph][parity][fixture]' --allow-slow
  scripts/dwg-fast-check.sh --test '[datastorage]' --build-target librecad_dwg_fast_tests
  scripts/dwg-fast-check.sh --test-only --test '[datastorage]' \
    --build-target librecad_dwg_fast_tests --trust-selector
  scripts/dwg-fast-check.sh --test-only --test '[entity_metadata]' \
    --build-target librecad_entity_metadata_tests --trust-selector
  scripts/dwg-fast-check.sh --test-only --test-file \
    librecad/src/lib/filters/tests/dxf_roundtrip_tests.cpp \
    --test 'DXF round-trip via RS_FilterDXFRW preserves unmodeled object + entity' \
    --no-discover
  scripts/dwg-fast-check.sh --no-build --test '[dwg][safety]'
  scripts/dwg-fast-check.sh --compile-only
  scripts/dwg-fast-check.sh --build-target libdxfrw_json_dump
  scripts/dwg-fast-check.sh --qmake --qmake-target libdxfrw
  scripts/dwg-fast-check.sh --both --test '[dxf][block]'
  scripts/dwg-fast-check.sh --test-only --test-file \
    librecad/src/lib/filters/tests/dwg_safety_tests.cpp \
    --test '[dwg][safety]'
  scripts/dwg-fast-check.sh --changed --compile-only
  scripts/dwg-fast-check.sh --changed --test 'dwgBuffer: modular shorts reject overlong continuation'
  scripts/dwg-fast-check.sh --full --test '*' --allow-slow
EOF
}

die() {
  printf 'error: %s\n' "$*" >&2
  exit 2
}

while (($# > 0)); do
  case "$1" in
  --cmake) SCOPE="cmake"; shift ;;
  --qmake) SCOPE="qmake"; shift ;;
  --both) SCOPE="both"; shift ;;
  --qmake-target)
    (($# >= 2)) || die "--qmake-target needs a value"
    QMAKE_TARGET="$2"
    shift 2
    ;;
  --build-dir)
    (($# >= 2)) || die "--build-dir needs a path"
    BUILD_DIR="$2"
    shift 2
    ;;
  --test)
    (($# >= 2)) || die "--test needs a Catch2 selector"
    TEST_SPEC="$2"
    shift 2
    ;;
  --test-file)
    (($# >= 2)) || die "--test-file needs a source path"
    TEST_FILE="$2"
    shift 2
    ;;
  --suite)
    (($# >= 2)) || die "--suite needs a value"
    TEST_SUITE="$2"
    shift 2
    ;;
  --fast) FAST=1; shift ;;
  --full) FULL=1; shift ;;
  --allow-slow) ALLOW_SLOW=1; shift ;;
  --skip-slow) ALLOW_SLOW=0; shift ;;
  --abort) FAIL_FAST=1; shift ;;
  --no-build) NO_BUILD=1; shift ;;
  --test-only)
    # A cached repeat is explicitly asking to run a selector that was already
    # validated. Avoid spawning Catch2's list-tests process on every edit.
    NO_BUILD=1
    TEST_ONLY=1
    TRUST_SELECTOR=1
    shift
    ;;
  --changed) CHANGED=1; shift ;;
  --compile-only) COMPILE_ONLY=1; shift ;;
  --quiet) QUIET=1; shift ;;
  --discover) TRUST_SELECTOR=0; shift ;;
  --trust-selector|--no-discover) TRUST_SELECTOR=1; shift ;;
  --build-target)
    (($# >= 2)) || die "--build-target needs a CMake target"
    BUILD_TARGET="$2"
    shift 2
    ;;
  --configure) CONFIGURE=1; shift ;;
  --no-configure) AUTO_CONFIGURE=0; shift ;;
  --jobs)
    (($# >= 2)) || die "--jobs needs a positive integer"
    JOBS="$2"
    shift 2
    ;;
  -h|--help) usage; exit 0 ;;
  *) die "unknown option '$1' (use --help)" ;;
  esac
done

[[ "$JOBS" =~ ^[1-9][0-9]*$ ]] || die "jobs must be a positive integer"
case "$QMAKE_TARGET" in
libdxfrw|librecad) ;;
*) die "qmake target must be libdxfrw or librecad" ;;
esac

# The changed-file mode keeps the default path incremental without guessing
# across several test translation units. It includes staged, unstaged, and
# untracked source files, while ignoring build products and other generated
# workspace material. A single changed test source maps to its micro target;
# several related edits use the smallest format aggregate.
if ((CHANGED)); then
  command -v git >/dev/null 2>&1 || die "--changed requires git"
  changed_paths=()
  while IFS= read -r changed_path; do
    [[ -n "$changed_path" ]] && changed_paths+=("$changed_path")
  done < <(
    {
      git -C "$ROOT" diff --name-only HEAD
      git -C "$ROOT" ls-files --others --exclude-standard
    } | sed '/^[[:space:]]*$/d' | sort -u
  )
  relevant_paths=()
  changed_test_files=()
  changed_test_dwg=0
  changed_test_dxf=0
  changed_test_linked=0
  changed_writer_smoke=0
  changed_core=0
  changed_filter=0
  for changed_path in "${changed_paths[@]}"; do
    case "$changed_path" in
    libraries/libdxfrw/src/*)
      relevant_paths+=("$changed_path")
      changed_core=1
      ;;
    librecad/src/lib/filters/tests/*.cpp)
      relevant_paths+=("$changed_path")
      changed_test_files+=("$changed_path")
      changed_test_stem="${changed_path##*/}"
      changed_test_stem="${changed_test_stem%.cpp}"
      case "$changed_test_stem" in
      dwg_write_smoke_tests)
        changed_writer_smoke=1
        changed_test_linked=1
        ;;
      dwg_buffer_round_trip_tests|dwg_decompress18_tests|\
      dwg_entity_encode_round_trip_tests|dwg_header_encode_round_trip_tests|\
      dwg_handseed_tests|dynblock_tests|evalgraph_tests|\
      acsh_tests|acsh_shapes_tests|\
      dwg_object_encode_round_trip_tests|\
      dwg_object_frame_tests|dwg_safety_tests|dxf_attribute_tests|\
      dxf_object_tests|field_dxf_tests|mesh_tests)
        ;;
      *)
        changed_test_linked=1
        ;;
      esac
      case "$changed_test_stem" in
      dwg_*|dynblock_tests|evalgraph_tests|acsh_tests|acsh_shapes_tests|\
      eed_tests|datastorage_tests|mesh_tests) changed_test_dwg=1 ;;
      dxf_*|field_dxf_tests|entity_metadata_tests|mesh_tests) changed_test_dxf=1 ;;
      esac
      ;;
    librecad/src/lib/filters/rs_filterdxfrw.cpp|\
    librecad/src/lib/filters/rs_filterdxfrw.h)
      relevant_paths+=("$changed_path")
      changed_filter=1
      ;;
    CMakeLists.txt|libraries/libdxfrw/CMakeLists.txt|\
    libraries/libdxfrw/libdxfrw_sources.cmake)
      relevant_paths+=("$changed_path")
      ;;
    esac
  done
  ((${#relevant_paths[@]} > 0)) ||
    die "--changed found no DWG/DXF source or fast-test changes"
  # A single mapped test source can stay source-scoped even when the same
  # edit also changes production code, provided the caller named a behavior
  # selector. Without a selector, retain compile-only behavior and do not
  # link a test executable just because a production file is dirty.
  if [[ -z "$TEST_FILE" && -z "$BUILD_TARGET" &&
        ${#changed_test_files[@]} -eq 1 ]] &&
      { ((changed_core == 0 && changed_filter == 0)) ||
        [[ -n "$TEST_SPEC" ]]; }; then
    changed_test_name="${changed_test_files[0]##*/}"
    changed_test_name="${changed_test_name%.cpp}"
    # Some complete-test sources are deliberately outside the fast graph.
    # Leave those on the aggregate fallback instead of inventing a target.
    if fast_test_target_for_stem "$changed_test_name" >/dev/null; then
      TEST_FILE="${changed_test_files[0]}"
    fi
  fi
  if [[ -z "$TEST_FILE" && -z "$BUILD_TARGET" ]]; then
    if ((COMPILE_ONLY)) && ((changed_core)) && ((changed_filter)); then
      # The two independent targets share the warm tree but neither target
      # depends on the other. Build both serially so a mixed core/filter edit
      # cannot receive a false compile-only pass.
      BUILD_TARGET="libdxfrw_test_core"
      EXTRA_BUILD_TARGETS+=(librecad_filter_compile_check)
    elif ((COMPILE_ONLY)) && ((changed_core)); then
      BUILD_TARGET="libdxfrw_test_core"
    elif ((COMPILE_ONLY)) && ((changed_filter)); then
      BUILD_TARGET="librecad_filter_compile_check"
    elif ((changed_writer_smoke)) && [[ -z "$TEST_SPEC" ]]; then
      if ((${#changed_test_files[@]} > 1)); then
        die "--changed includes dwg_write_smoke_tests with other tests; pass --test-file or --build-target"
      fi
      BUILD_TARGET="librecad_dwg_write_fast_tests"
    elif ((${#changed_test_files[@]} > 1)); then
      if ((changed_test_linked)); then
        if ((changed_test_dwg && changed_test_dxf)); then
          BUILD_TARGET="librecad_fast_tests"
        elif ((changed_test_dwg)); then
          BUILD_TARGET="librecad_dwg_fast_tests"
        elif ((changed_test_dxf)); then
          BUILD_TARGET="librecad_dxf_fast_tests"
        else
          BUILD_TARGET="librecad_fast_tests"
        fi
      elif ((changed_test_dwg && changed_test_dxf)); then
        BUILD_TARGET="libdxfrw_fast_tests"
      elif ((changed_test_dwg)); then
        BUILD_TARGET="libdxfrw_dwg_fast_tests"
      else
        BUILD_TARGET="libdxfrw_dxf_fast_tests"
      fi
    elif ((changed_core)); then
      BUILD_TARGET="libdxfrw_test_core"
    elif ((changed_filter)); then
      BUILD_TARGET="librecad_filter_compile_check"
    else
      # A project-file change needs the normal configured dependency graph;
      # the archive is the cheapest target that proves it is still coherent.
      BUILD_TARGET="libdxfrw_test_core"
    fi
  fi
  printf '== changed incremental lane: %s\n' "${relevant_paths[*]}"
fi

if [[ -n "$TEST_FILE" ]]; then
  [[ -z "$BUILD_TARGET" ]] ||
    die "--test-file cannot be combined with --build-target"
  if [[ "$TEST_FILE" = /* ]]; then
    test_file_rel="${TEST_FILE#"$ROOT"/}"
    [[ "$test_file_rel" != "$TEST_FILE" ]] ||
      die "--test-file must be inside the checkout"
  else
    test_file_rel="${TEST_FILE#./}"
  fi
  case "$test_file_rel" in
  librecad/src/lib/filters/tests/*.cpp) ;;
  *) die "--test-file must name a registered DWG/DXF test source" ;;
  esac
  test_file_stem="${test_file_rel##*/}"
  test_file_stem="${test_file_stem%.cpp}"
  BUILD_TARGET="$(fast_test_target_for_stem "$test_file_stem")" ||
    die "test source '$test_file_rel' is not in a registered fast-test lane"
  TEST_FILE_TARGET=1
fi

case "$TEST_SUITE" in
auto|dwg|dxf|all) ;;
*) die "test suite must be auto, dwg, dxf, or all" ;;
esac

# Keep a format-local selector on the smallest parser executable without
# making callers repeat --suite for every focused test. Explicit --suite
# values remain authoritative, and unqualified selectors stay cross-format.
if [[ "$TEST_SUITE" == auto ]]; then
  selector_has_dwg=0
  selector_has_dxf=0
  [[ "$TEST_SPEC" == *"[dwg]"* || "$TEST_SPEC" == *"[dwg-"* ||
    "$TEST_SPEC" == *"[dwg_"* ]] && selector_has_dwg=1
  [[ "$TEST_SPEC" == *"[dxf]"* || "$TEST_SPEC" == *"[dxf-"* ||
    "$TEST_SPEC" == *"[dxf_"* ]] && selector_has_dxf=1
  if ((selector_has_dwg && !selector_has_dxf)); then
    TEST_SUITE="dwg"
  elif ((selector_has_dxf && !selector_has_dwg)); then
    TEST_SUITE="dxf"
  else
    TEST_SUITE="all"
  fi
fi

# --changed without a test file intentionally selects the cheapest compile
# target. When a selector is also requested, that archive is not runnable;
# promote it to the smallest matching executable so the command remains a
# useful incremental build-and-test shortcut.
if ((CHANGED)) && [[ -n "$TEST_SPEC" && "$TEST_FILE_TARGET" != 1 ]]; then
  if ((changed_filter)); then
    # RS_FilterDXFRW is compiled into librecad_lib, so a filter edit must use
    # the LibreCAD-linked family target when a behavior selector is supplied.
    # This remains format-local whenever the selector identifies one.
    case "$TEST_SUITE" in
    dwg) BUILD_TARGET="librecad_dwg_fast_tests" ;;
    dxf) BUILD_TARGET="librecad_dxf_fast_tests" ;;
    all) BUILD_TARGET="librecad_fast_tests" ;;
    esac
  else
    case "$BUILD_TARGET" in
    libdxfrw_test_core)
      case "$TEST_SUITE" in
      dwg) BUILD_TARGET="libdxfrw_dwg_fast_tests" ;;
      dxf) BUILD_TARGET="libdxfrw_dxf_fast_tests" ;;
      all) BUILD_TARGET="libdxfrw_fast_tests" ;;
      esac
      ;;
    librecad_filter_compile_check)
      BUILD_TARGET="librecad_fast_tests"
      ;;
    esac
  fi
fi

case "$BUILD_TARGET" in
""|librecad_filter_compile_check|librecad_lib|librecad_tests|librecad_fast_tests|librecad_dwg_fast_tests|librecad_dwg_smoke_tests|librecad_acis_wireframe_tests|librecad_dwg_write_fast_tests|librecad_dxf_fast_tests|librecad_dwg_header_app_vars_tests|librecad_dxf_roundtrip_tests|librecad_dxf_string_codec_tests|librecad_eed_tests|librecad_entity_metadata_tests|librecad_i18n_caret_nfc_tests|librecad_i18n_codec_tests|librecad_datastorage_tests|libdxfrw_test_core|libdxfrw_fast_tests|libdxfrw_dwg_fast_tests|libdxfrw_dxf_fast_tests|libdxfrw_slow_tests|libdxfrw_json_dump|dwg_tarch_test) ;;
libdxfrw_dwg_buffer_round_trip_tests|libdxfrw_dwg_decompress18_tests|\
libdxfrw_dwg_entity_encode_round_trip_tests|libdxfrw_dwg_header_encode_round_trip_tests|\
libdxfrw_dwg_handseed_tests|libdxfrw_dynblock_tests|libdxfrw_evalgraph_tests|\
libdxfrw_acsh_tests|libdxfrw_acsh_shapes_tests|\
libdxfrw_dwg_object_encode_round_trip_tests|libdxfrw_dwg_object_frame_tests|\
libdxfrw_dwg_safety_tests|libdxfrw_dxf_attribute_tests|libdxfrw_dxf_object_tests|\
libdxfrw_field_dxf_tests|libdxfrw_mleader_dxf_context_tests) ;;
*) ((TEST_FILE_TARGET)) || die "unsupported CMake target '$BUILD_TARGET'" ;;
esac

if [[ -n "$TEST_SPEC" && -n "$BUILD_TARGET" &&
      "$TEST_FILE_TARGET" != 1 &&
      "$BUILD_TARGET" != "librecad_tests" &&
      "$BUILD_TARGET" != "librecad_fast_tests" &&
      "$BUILD_TARGET" != "librecad_dwg_fast_tests" &&
      "$BUILD_TARGET" != "librecad_dwg_smoke_tests" &&
      "$BUILD_TARGET" != "librecad_acis_wireframe_tests" &&
      "$BUILD_TARGET" != "librecad_dwg_write_fast_tests" &&
      "$BUILD_TARGET" != "librecad_dxf_fast_tests" &&
      "$BUILD_TARGET" != "libdxfrw_slow_tests" &&
      "$BUILD_TARGET" != "librecad_dwg_header_app_vars_tests" &&
      "$BUILD_TARGET" != "librecad_dxf_roundtrip_tests" &&
      "$BUILD_TARGET" != "librecad_dxf_string_codec_tests" &&
      "$BUILD_TARGET" != "librecad_eed_tests" &&
      "$BUILD_TARGET" != "librecad_entity_metadata_tests" &&
      "$BUILD_TARGET" != "librecad_i18n_caret_nfc_tests" &&
      "$BUILD_TARGET" != "librecad_i18n_codec_tests" &&
      "$BUILD_TARGET" != "librecad_datastorage_tests" &&
      "$BUILD_TARGET" != "libdxfrw_fast_tests" &&
      "$BUILD_TARGET" != "libdxfrw_dwg_fast_tests" &&
      "$BUILD_TARGET" != "libdxfrw_dxf_fast_tests" &&
      "$BUILD_TARGET" != "libdxfrw_dwg_buffer_round_trip_tests" &&
      "$BUILD_TARGET" != "libdxfrw_dwg_decompress18_tests" &&
      "$BUILD_TARGET" != "libdxfrw_dwg_entity_encode_round_trip_tests" &&
      "$BUILD_TARGET" != "libdxfrw_dwg_header_encode_round_trip_tests" &&
      "$BUILD_TARGET" != "libdxfrw_dwg_handseed_tests" &&
      "$BUILD_TARGET" != "libdxfrw_dynblock_tests" &&
      "$BUILD_TARGET" != "libdxfrw_evalgraph_tests" &&
      "$BUILD_TARGET" != "libdxfrw_acsh_tests" &&
      "$BUILD_TARGET" != "libdxfrw_acsh_shapes_tests" &&
      "$BUILD_TARGET" != "libdxfrw_dwg_object_encode_round_trip_tests" &&
      "$BUILD_TARGET" != "libdxfrw_dwg_object_frame_tests" &&
      "$BUILD_TARGET" != "libdxfrw_dwg_safety_tests" &&
      "$BUILD_TARGET" != "libdxfrw_dxf_attribute_tests" &&
      "$BUILD_TARGET" != "libdxfrw_dxf_object_tests" &&
      "$BUILD_TARGET" != "libdxfrw_field_dxf_tests" &&
      "$BUILD_TARGET" != "libdxfrw_mleader_dxf_context_tests" ]]; then
  die "--test requires a test build target or no explicit build target"
fi
if ((FAST)) && [[ -n "$BUILD_TARGET" &&
      "$BUILD_TARGET" != "librecad_fast_tests" &&
      "$BUILD_TARGET" != "librecad_dwg_fast_tests" &&
      "$BUILD_TARGET" != "librecad_dxf_fast_tests" &&
      "$BUILD_TARGET" != "librecad_acis_wireframe_tests" &&
      "$BUILD_TARGET" != "libdxfrw_fast_tests" &&
      "$BUILD_TARGET" != "libdxfrw_dwg_fast_tests" &&
      "$BUILD_TARGET" != "libdxfrw_dxf_fast_tests" &&
      "$BUILD_TARGET" != "libdxfrw_dwg_buffer_round_trip_tests" &&
      "$BUILD_TARGET" != "libdxfrw_dwg_decompress18_tests" &&
      "$BUILD_TARGET" != "libdxfrw_dwg_entity_encode_round_trip_tests" &&
      "$BUILD_TARGET" != "libdxfrw_dwg_header_encode_round_trip_tests" &&
      "$BUILD_TARGET" != "libdxfrw_dwg_handseed_tests" &&
      "$BUILD_TARGET" != "libdxfrw_dynblock_tests" &&
      "$BUILD_TARGET" != "libdxfrw_evalgraph_tests" &&
      "$BUILD_TARGET" != "libdxfrw_acsh_tests" &&
      "$BUILD_TARGET" != "libdxfrw_acsh_shapes_tests" &&
      "$BUILD_TARGET" != "libdxfrw_dwg_object_encode_round_trip_tests" &&
      "$BUILD_TARGET" != "libdxfrw_dwg_object_frame_tests" &&
      "$BUILD_TARGET" != "libdxfrw_dwg_safety_tests" &&
      "$BUILD_TARGET" != "libdxfrw_dxf_attribute_tests" &&
      "$BUILD_TARGET" != "libdxfrw_dxf_object_tests" &&
      "$BUILD_TARGET" != "libdxfrw_field_dxf_tests" &&
      "$BUILD_TARGET" != "libdxfrw_mleader_dxf_context_tests" ]]; then
  die "--fast cannot be combined with a different --build-target"
fi
if ((FAST)) && ((FULL)); then
  die "--fast and --full are mutually exclusive"
fi
if ((ALLOW_SLOW)) && ((FAST)); then
  die "--fast cannot include [.slow] tests; use the full target with --allow-slow"
fi
if ((FULL)) && [[ -n "$BUILD_TARGET" && "$BUILD_TARGET" != "librecad_tests" ]]; then
  die "--full requires the librecad_tests target"
fi
if ((NO_BUILD)) && ((CONFIGURE)); then
  die "--no-build cannot be combined with --configure"
fi
if ((TEST_ONLY)) && [[ -z "$TEST_SPEC" ]]; then
  die "--test-only requires --test"
fi
if ((COMPILE_ONLY)) && [[ -n "$TEST_SPEC" ]]; then
  die "--compile-only cannot be combined with --test"
fi
if ((COMPILE_ONLY)) && ((TEST_ONLY)); then
  die "--compile-only cannot be combined with --test-only"
fi
if ((COMPILE_ONLY)) && ((FULL)); then
  die "--compile-only cannot be combined with --full"
fi
if ((COMPILE_ONLY)) && ((NO_BUILD)); then
  die "--compile-only cannot be combined with --no-build"
fi
if ((COMPILE_ONLY)) && [[ "$SCOPE" == qmake ]]; then
  die "--compile-only is a CMake-only option"
fi
if ((NO_BUILD)) && [[ -n "$BUILD_TARGET" ]]; then
  case "$BUILD_TARGET" in
  librecad_tests|librecad_fast_tests|librecad_dwg_fast_tests|\
  librecad_dwg_smoke_tests|librecad_acis_wireframe_tests|\
  librecad_dwg_write_fast_tests|librecad_dxf_fast_tests|\
  librecad_dwg_header_app_vars_tests|librecad_dxf_roundtrip_tests|\
  librecad_dxf_string_codec_tests|librecad_eed_tests|librecad_entity_metadata_tests|\
  librecad_i18n_caret_nfc_tests|librecad_i18n_codec_tests|librecad_datastorage_tests|\
  libdxfrw_fast_tests|libdxfrw_dwg_fast_tests|libdxfrw_slow_tests|\
  libdxfrw_dxf_fast_tests|libdxfrw_dwg_buffer_round_trip_tests|\
  libdxfrw_dwg_decompress18_tests|libdxfrw_dwg_entity_encode_round_trip_tests|\
  libdxfrw_dwg_header_encode_round_trip_tests|libdxfrw_dwg_handseed_tests|\
  libdxfrw_dynblock_tests|libdxfrw_evalgraph_tests|\
  libdxfrw_acsh_tests|libdxfrw_acsh_shapes_tests|\
  libdxfrw_dwg_object_encode_round_trip_tests|\
  libdxfrw_dwg_object_frame_tests|libdxfrw_dwg_safety_tests|\
  libdxfrw_dxf_attribute_tests|libdxfrw_dxf_object_tests|\
  libdxfrw_field_dxf_tests|libdxfrw_mleader_dxf_context_tests) ;;
  *) ((TEST_FILE_TARGET)) ||
    die "--no-build with --build-target requires a test executable target" ;;
  esac
  [[ -n "$TEST_SPEC" ]] ||
    die "--no-build with --build-target requires --test"
fi

# Reject slow or hidden selectors before any configure/build work. This keeps
# an accidental corpus or fixture request from paying for a target that will
# not run.
if [[ -n "$TEST_SPEC" && "$ALLOW_SLOW" == 0 ]]; then
  [[ "$TEST_SPEC" != *"[."* ]] ||
    die "selector includes a hidden test; use --allow-slow for opt-in checks"
  [[ "$TEST_SPEC" != *".slow"* ]] ||
    die "selector includes [.slow]; use --allow-slow for corpus tests"
  [[ "$TEST_SPEC" != *"[slow]"* ]] ||
    die "selector includes [slow]; use --allow-slow for slow tests"
  [[ "$TEST_SPEC" != *"[corpus]"* ]] ||
    die "selector includes [corpus]; use --allow-slow for corpus tests"
  [[ "$TEST_SPEC" != *"[external]"* ]] ||
    die "selector includes [external]; use --allow-slow for external tests"
  [[ "$TEST_SPEC" != *"[external-reader]"* ]] ||
    die "selector includes [external-reader]; use --allow-slow for external tests"
  [[ "$TEST_SPEC" != *"[fixture]"* ]] ||
    die "selector includes [fixture]; use --allow-slow for fixture tests"
fi

# A no-build focused check may be requested before its per-source executable
# has ever been built. Reuse the already-built grouped lane when it contains
# the source; this keeps --no-build honest while avoiding a needless compile.
if ((NO_BUILD)) && ((TEST_FILE_TARGET)); then
  micro_binary="$BUILD_DIR/$BUILD_TARGET"
  if [[ ! -x "$micro_binary" ]]; then
    fallback_target=""
    case "$BUILD_TARGET" in
    librecad_*)
      case "$test_file_stem" in
      dwg_smoke_tests)
        ;;
      dwg_*|eed_tests|datastorage_tests)
        fallback_target="librecad_dwg_fast_tests"
        ;;
      dxf_*|entity_metadata_tests)
        fallback_target="librecad_dxf_fast_tests"
        ;;
      *)
        fallback_target="librecad_fast_tests"
        ;;
      esac
      ;;
    libdxfrw_*)
      case "$test_file_stem" in
      evalgraph_tests|acsh_tests|acsh_shapes_tests)
        fallback_target="libdxfrw_slow_tests"
        ;;
      dxf_*|field_dxf_tests)
        fallback_target="libdxfrw_dxf_fast_tests"
        ;;
      *)
        fallback_target="libdxfrw_dwg_fast_tests"
        ;;
      esac
      ;;
    esac
    if [[ -n "$fallback_target" && -x "$BUILD_DIR/$fallback_target" ]]; then
      printf '== per-source target unavailable; reusing grouped target: %s\n' \
        "$fallback_target"
      BUILD_TARGET="$fallback_target"
    fi
  fi
fi

run_cmake=0
run_qmake=0
case "$SCOPE" in
cmake) run_cmake=1 ;;
qmake) run_qmake=1 ;;
both) run_cmake=1; run_qmake=1 ;;
*) die "internal error: invalid scope '$SCOPE'" ;;
esac

if ((FAST)) && ((run_cmake == 0)); then
  die "--fast requires the CMake lane"
fi

if ((run_cmake)); then
  configure_fast_tree() {
    cmake_args=(-S "$ROOT" -B "$BUILD_DIR"
      -DBUILD_TESTS=ON
      -DLIBRECAD_REGISTER_FULL_TESTS=OFF
      -DLIBRECAD_REGISTER_INTEGRATION_TESTS=OFF
      -DLIBRECAD_REGISTER_FORMAT_TESTS=OFF
      -DLIBRECAD_REGISTER_CROSS_READ_SMOKE=OFF)
    if [[ ! -f "$BUILD_DIR/CMakeCache.txt" ]]; then
      if [[ -z "${CMAKE_GENERATOR:-}" ]] && command -v ninja >/dev/null 2>&1; then
        cmake_args+=(-G Ninja)
      fi
    if command -v ccache >/dev/null 2>&1; then
      cmake_args+=(-DCMAKE_CXX_COMPILER_LAUNCHER=ccache)
    elif command -v sccache >/dev/null 2>&1; then
      cmake_args+=(-DCMAKE_CXX_COMPILER_LAUNCHER=sccache)
    fi
    fi
    cmake "${cmake_args[@]}"
  }
  if ((CONFIGURE)); then
    configure_fast_tree
  elif [[ ! -f "$BUILD_DIR/CMakeCache.txt" ]]; then
    ((AUTO_CONFIGURE)) ||
      die "CMake tree is not configured: rerun without --no-configure or use --configure"
    printf '== initializing incremental CMake tree: %s\n' "$BUILD_DIR"
    configure_fast_tree
  fi
  if [[ -n "$BUILD_TARGET" ]]; then
    cmake_target="$BUILD_TARGET"
  elif ((COMPILE_ONLY)); then
    # Stop at the archive: this is the cheapest check after a core edit.
    cmake_target="libdxfrw_test_core"
  elif [[ -n "$TEST_SPEC" || "$FAST" == 1 || "$FULL" == 1 ]]; then
    cmake_target="librecad_fast_tests"
    ((FULL)) && cmake_target="librecad_tests"
    if ((FULL == 0)); then
      case "$TEST_SUITE" in
      dwg) cmake_target="libdxfrw_dwg_fast_tests" ;;
      dxf) cmake_target="libdxfrw_dxf_fast_tests" ;;
      all) cmake_target="libdxfrw_fast_tests" ;;
      esac
    fi
  else
    # A no-selector invocation is compile-only. Linking a test executable is
    # an explicit request because a source edit may need only the archive.
    cmake_target="libdxfrw_test_core"
  fi
  if ((NO_BUILD)); then
    printf '== reusing CMake target: %s\n' "$cmake_target"
  else
    printf '== incremental CMake build: %s (%s jobs)\n' "$cmake_target" "$JOBS"
    cmake --build "$BUILD_DIR" --target "$cmake_target" --parallel "$JOBS"
    for extra_target in "${EXTRA_BUILD_TARGETS[@]}"; do
      printf '== incremental CMake build: %s (%s jobs)\n' "$extra_target" "$JOBS"
      cmake --build "$BUILD_DIR" --target "$extra_target" --parallel "$JOBS"
    done
  fi
fi

if ((run_qmake)); then
  case "$QMAKE_TARGET" in
  libdxfrw) qmake_dir="$QMAKE_BUILD_DIR/libraries/libdxfrw" ;;
  librecad) qmake_dir="$QMAKE_BUILD_DIR/librecad" ;;
  esac
  [[ -f "$qmake_dir/Makefile" ]] ||
    die "qmake6 tree is not configured at $qmake_dir"
  if ((NO_BUILD)); then
    printf '== reusing qmake6 target: %s\n' "$QMAKE_TARGET"
  else
    printf '== incremental qmake6 build: %s (%s jobs)\n' "$QMAKE_TARGET" "$JOBS"
    make -C "$qmake_dir" all -j"$JOBS"
  fi
fi

if [[ -n "$TEST_SPEC" ]]; then
  ((run_cmake)) || die "--test requires the CMake lane"
  test_target="${BUILD_TARGET:-libdxfrw_fast_tests}"
  ((FULL)) && test_target="librecad_tests"
  if ((FULL == 0)) && [[ -z "$BUILD_TARGET" ]]; then
    case "$TEST_SUITE" in
      dwg) test_target="libdxfrw_dwg_fast_tests" ;;
      dxf) test_target="libdxfrw_dxf_fast_tests" ;;
      all) test_target="libdxfrw_fast_tests" ;;
    esac
  fi
  if ((FAST)) && [[ -z "$BUILD_TARGET" && "$TEST_SUITE" == "all" ]]; then
    test_target="libdxfrw_fast_tests"
  fi
  test_binary="$BUILD_DIR/$test_target"
  [[ -x "$test_binary" ]] ||
    die "test executable is missing at $test_binary"
  validation_stamp="$BUILD_DIR/.dwg-fast-check/$test_target.stamp"
  if ((NO_BUILD == 0)); then
    mkdir -p "${validation_stamp%/*}"
    # The target build is the authoritative freshness boundary. CMake may
    # regenerate build files without relinking an unchanged executable, so
    # comparing CMakeLists.txt directly with the binary would reject a valid
    # warm build on the next --test-only repeat.
    touch "$validation_stamp"
  fi
  if ((TEST_ONLY)); then
    freshness_reference="$test_binary"
    [[ -f "$validation_stamp" ]] && freshness_reference="$validation_stamp"

    # Ninja already has the complete compiler dependency graph, including
    # generated-header and transitive-header dependencies. A dry run is
    # cheaper and more accurate than walking every source tree with find.
    if [[ -f "$BUILD_DIR/build.ninja" ]] && command -v ninja >/dev/null 2>&1; then
      ninja_plan="$(ninja -C "$BUILD_DIR" -n "$test_target" 2>&1)" ||
        die "cannot inspect cached Ninja target; rerun without --test-only"
      ninja_plan="$(printf '%s\n' "$ninja_plan" | sed \
        '/^ninja: Entering directory /d; /^ninja: no work to do\.$/d')"
      [[ -z "$ninja_plan" ]] ||
        die "test target is stale; rerun without --test-only to rebuild incrementally"
    else
      # Keep a conservative fallback for a non-Ninja CMake tree.
      cached_source_roots=("$ROOT/libraries/libdxfrw/src")
      cached_source_files=()
      if [[ -n "$TEST_FILE" ]]; then
        test_source="$ROOT/$test_file_rel"
        [[ -f "$test_source" ]] ||
          die "test source is missing: $test_file_rel"
        cached_source_files+=("$test_source")
      else
        cached_source_roots+=("$ROOT/librecad/src/lib/filters/tests")
        [[ "$test_target" == librecad_* ]] &&
          cached_source_roots+=("$ROOT/librecad/src/lib/filters")
      fi
      for cached_source_file in "${cached_source_files[@]}"; do
        if [[ "$cached_source_file" -nt "$freshness_reference" ]]; then
          die "test binary is older than $cached_source_file; rerun without --test-only to rebuild incrementally"
        fi
      done
      for cached_source_root in "${cached_source_roots[@]}"; do
        [[ -d "$cached_source_root" ]] || continue
        newer_source="$(find "$cached_source_root" -type f \( \
          -name '*.cpp' -o -name '*.h' -o -name '*.hpp' -o -name '*.ipp' -o -name '*.inl' \
          \) -newer "$freshness_reference" -print -quit)"
        [[ -z "$newer_source" ]] ||
          die "test binary is older than $newer_source; rerun without --test-only to rebuild incrementally"
      done
    fi
    for cached_build_file in "$ROOT/CMakeLists.txt" \
      "$ROOT/libraries/libdxfrw/CMakeLists.txt" \
      "$ROOT/libraries/libdxfrw/libdxfrw_sources.cmake"; do
      if [[ -f "$cached_build_file" && "$cached_build_file" -nt "$freshness_reference" ]]; then
        die "test binary predates $cached_build_file; rerun with --configure and without --test-only"
      fi
    done
  fi
  if ((ALLOW_SLOW)); then
    test_specs=("$TEST_SPEC")
  else
    test_specs=("$TEST_SPEC" "~[.]" "~[.slow]" "~[slow]" "~[corpus]"
      "~[external]" "~[external-reader]" "~[fixture]")
  fi
  if ((TRUST_SELECTOR == 0)); then
    test_count="$("$test_binary" "$TEST_SPEC" --list-tests --reporter compact 2>/dev/null |
      awk '/^  / { count++ } END { print count + 0 }')"
    ((test_count > 0)) || {
      if [[ "$test_target" != "librecad_tests" ]]; then
        die "selector '$TEST_SPEC' is not in $test_target; use --full or choose a fast-lane tag"
      fi
      die "selector '$TEST_SPEC' matched no tests"
    }
  fi
  printf '== focused Catch2 test: %s\n' "${test_specs[*]}"
  test_args=("${test_specs[@]}" --reporter compact)
  ((FAIL_FAST)) && test_args+=(--abort)
  test_output="$(mktemp "${TMPDIR:-/tmp}/dwg-fast-check.XXXXXX")"
  trap 'rm -f "$test_output"' EXIT
  set +e
  if ((QUIET)); then
    QT_QPA_PLATFORM=offscreen "$test_binary" "${test_args[@]}" >"$test_output" 2>&1
    test_status="$?"
  else
    QT_QPA_PLATFORM=offscreen "$test_binary" "${test_args[@]}" 2>&1 |
      tee "$test_output"
    test_status="${PIPESTATUS[0]}"
  fi
  set -e
  if ((QUIET)) && ((test_status != 0)); then
    cat "$test_output"
  elif ((QUIET)); then
    tail -n 3 "$test_output"
  fi
  ((test_status == 0)) || exit "$test_status"
  if grep -qE '^(No test cases matched|No tests ran)$' "$test_output"; then
    die "selector '$TEST_SPEC' matched no tests in $test_target"
  fi
else
  printf '%s\n' '== no test selector supplied; build-only fast check complete'
fi
