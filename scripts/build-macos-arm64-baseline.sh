#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 LibreCAD contributors
# Use the locked Pixi libraries with Apple's SDK/linker, without activating
# Conda's compiler wrappers. This also keeps build and install output separate.
set -euo pipefail
task_root=$(cd "$(dirname "$0")/.." && pwd -P)
cd "$task_root"
task_env=${LIBRECAD_BUILD_ENV:-"$task_root/.pixi/envs/default"}
task_cmake="$task_env/bin/cmake"
task_build="$task_root/build/macos-arm64-baseline"
[[ $(uname -s) == Darwin && $(uname -m) == arm64 ]] || {
    echo "This baseline requires a native Apple Silicon macOS shell." >&2; exit 1;
}
[[ -x $task_cmake && -x $task_env/bin/ninja ]] || {
    echo "Install pixi.lock first: pixi install --locked" >&2; exit 1;
}
task_sdk=$(xcrun --sdk macosx --show-sdk-path)
# A caller may already have activated Pixi. Keep its old linker and injected
# compiler flags away from the newer Apple SDK; libraries stay explicit below.
task_apple_bin=$(dirname "$(xcrun --find clang)")
export PATH="$task_apple_bin:/usr/bin:/bin:/usr/sbin:/sbin:$task_env/bin:$PATH"
unset CC CXX CFLAGS CXXFLAGS CPPFLAGS LDFLAGS CONDA_BUILD_SYSROOT SDKROOT
task_cxx_flags=${LIBRECAD_CXX_FLAGS:-}
# Some CLT distributions put the complete libc++ headers in the SDK rather
# than beside clang. Keep them ahead of Qt's CUPS C-header include directory.
if [[ ! -f "$task_apple_bin/../include/c++/v1/chrono" && -f "$task_sdk/usr/include/c++/v1/chrono" ]]; then
    task_cxx_flags="-I\"$task_sdk/usr/include/c++/v1\" $task_cxx_flags"
fi
"$task_cmake" --preset macos-arm64-baseline \
    -U 'CUPS_*' -U 'OPENGL_*' -U 'FIND_PACKAGE_MESSAGE_DETAILS_*' \
    -DCMAKE_PREFIX_PATH="$task_env" -DQT_HOST_PATH="$task_env" \
    -DCMAKE_MAKE_PROGRAM="$task_env/bin/ninja" -DCMAKE_OSX_SYSROOT="$task_sdk" \
    -DCMAKE_CXX_FLAGS="$task_cxx_flags" \
    -DCMAKE_OSX_DEPLOYMENT_TARGET="${LIBRECAD_MACOS_DEPLOYMENT_TARGET:-$(xcrun --sdk macosx --show-sdk-version)}"
"$task_cmake" --build --preset macos-arm64-baseline
# Include current plugins before native GUI tests; don't mix old plugin builds
# with a newly selected Apple toolchain.
"$task_cmake" --build "$task_build" --parallel 6
"$task_cmake" --install "$task_build"
"$task_env/bin/ctest" --preset macos-arm64-baseline --timeout 120
QT_QPA_PLATFORM=offscreen "$task_build/librecad_fast_tests" \
    '~[.]' '~[.slow]' '~[slow]' '~[corpus]' '~[external]' '~[external-reader]' '~[fixture]' --reporter compact
QT_QPA_PLATFORM=${LIBRECAD_GUI_TEST_PLATFORM:-cocoa} "$task_build/librecad_dock_layout_tests" '[dock-window]' --reporter compact
QT_QPA_PLATFORM=${LIBRECAD_GUI_TEST_PLATFORM:-cocoa} "$task_build/librecad_dock_layout_tests" '[phase2-preferences]' --reporter compact
QT_QPA_PLATFORM=offscreen "$task_build/librecad_drawing_cursor_tests" --reporter compact
QT_QPA_PLATFORM=offscreen "$task_build/librecad_command_input_tests" --reporter compact
echo "Baseline app: $task_root/build/macos-arm64-install/bin/LibreCAD.app"
echo "Package a COPY with scripts/package-osx-app.sh; see the Phase 0 report."
