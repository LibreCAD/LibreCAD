# This file is part of LibreCAD, a 2D CAD program.
# Copyright (C) 2026 LibreCAD.org
# Copyright (C) 2026 Dongxu Li
# SPDX-License-Identifier: GPL-2.0-or-later

# Explicit core dependencies for the optional native OpenType backend. Qt's
# internal HarfBuzz implementation is not a public development dependency.
option(LIBRECAD_ENABLE_OPENTYPE
    "Build native OpenType shaping (does not change legacy font selection)" OFF)

add_library(lc_opentype_dependencies INTERFACE)
add_library(LibreCAD::OpenType ALIAS lc_opentype_dependencies)

if(NOT LIBRECAD_ENABLE_OPENTYPE)
    target_compile_definitions(lc_opentype_dependencies INTERFACE LC_WITH_OPENTYPE=0)
    # The font converter remains independently optional in legacy-only builds.
    find_package(Freetype QUIET)
    message(STATUS "LibreCAD OpenType backend: disabled (legacy LFF only)")
    return()
endif()

find_package(Freetype 2.10 QUIET)
if(NOT Freetype_FOUND)
    message(FATAL_ERROR
        "Native OpenType outlines require FreeType >= 2.10. Install its "
        "development package or set Freetype_ROOT/CMAKE_PREFIX_PATH. "
        "Use -DLIBRECAD_ENABLE_OPENTYPE=OFF for a legacy-only build.")
endif()

# Prefer pkg-config on Unix: its imported target retains the package's public
# link requirements. Native Windows toolchains normally provide a CMake package.
if(UNIX)
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(LIBRECAD_HARFBUZZ QUIET IMPORTED_TARGET harfbuzz>=2.6.0)
        if(TARGET PkgConfig::LIBRECAD_HARFBUZZ)
            set(_lc_harfbuzz_target PkgConfig::LIBRECAD_HARFBUZZ)
            set(_lc_harfbuzz_version "${LIBRECAD_HARFBUZZ_VERSION}")
        endif()
    endif()
endif()

if(NOT _lc_harfbuzz_target)
    find_package(harfbuzz 2.6 CONFIG QUIET)
    if(TARGET harfbuzz::harfbuzz)
        set(_lc_harfbuzz_target harfbuzz::harfbuzz)
        set(_lc_harfbuzz_version "${HARFBUZZ_VERSION}")
    endif()
endif()

# Support installations without a package configuration, including explicit
# -DLIBRECAD_HARFBUZZ_INCLUDE_DIR=... -DLIBRECAD_HARFBUZZ_LIBRARY=... overrides.
# A static archive with further dependencies should instead supply a package
# target or pkg-config metadata so its complete link interface is available.
if(NOT _lc_harfbuzz_target)
    find_path(LIBRECAD_HARFBUZZ_INCLUDE_DIR NAMES hb.h PATH_SUFFIXES harfbuzz)
    find_library(LIBRECAD_HARFBUZZ_LIBRARY NAMES harfbuzz libharfbuzz)
    if(LIBRECAD_HARFBUZZ_INCLUDE_DIR AND LIBRECAD_HARFBUZZ_LIBRARY
            AND EXISTS "${LIBRECAD_HARFBUZZ_INCLUDE_DIR}/hb-version.h")
        file(STRINGS "${LIBRECAD_HARFBUZZ_INCLUDE_DIR}/hb-version.h"
            _lc_harfbuzz_version_line REGEX "^#define HB_VERSION_STRING ")
        string(REGEX REPLACE "^#define HB_VERSION_STRING +\"([^\"]+)\".*" "\\1"
            _lc_harfbuzz_version "${_lc_harfbuzz_version_line}")
        if(_lc_harfbuzz_version VERSION_GREATER_EQUAL 2.6.0)
            add_library(lc_harfbuzz UNKNOWN IMPORTED)
            set_target_properties(lc_harfbuzz PROPERTIES
                IMPORTED_LOCATION "${LIBRECAD_HARFBUZZ_LIBRARY}"
                INTERFACE_INCLUDE_DIRECTORIES "${LIBRECAD_HARFBUZZ_INCLUDE_DIR}")
            set(_lc_harfbuzz_target lc_harfbuzz)
        endif()
    endif()
endif()

if(NOT _lc_harfbuzz_target)
    message(FATAL_ERROR
        "Native OpenType shaping requires HarfBuzz >= 2.6.0 and FreeType >= 2.10. "
        "Install their development packages, set harfbuzz_DIR/CMAKE_PREFIX_PATH "
        "or LIBRECAD_HARFBUZZ_INCLUDE_DIR and LIBRECAD_HARFBUZZ_LIBRARY. "
        "Use -DLIBRECAD_ENABLE_OPENTYPE=OFF for a legacy-only build.")
endif()

target_compile_definitions(lc_opentype_dependencies INTERFACE LC_WITH_OPENTYPE=1)
target_link_libraries(lc_opentype_dependencies INTERFACE
    ${_lc_harfbuzz_target} Freetype::Freetype)
message(STATUS "LibreCAD OpenType backend: HarfBuzz ${_lc_harfbuzz_version}, FreeType ${FREETYPE_VERSION_STRING}")
