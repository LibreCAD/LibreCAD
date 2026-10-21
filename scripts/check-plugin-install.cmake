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
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
# USA.
# ********************************************************************************

cmake_minimum_required(VERSION 3.28)
get_filename_component(source_dir "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
get_filename_component(build_dir "${LIBRECAD_BUILD_DIR}" ABSOLUTE)
if(NOT EXISTS "${build_dir}/CMakeCache.txt")
    message(FATAL_ERROR "Build LibreCAD before checking plugin installation")
endif()
load_cache("${build_dir}" READ_WITH_PREFIX initial_ LIBRECAD_PLUGIN_INSTALL_DIR)
file(STRINGS "${build_dir}/CMakeCache.txt" plugin_setting
    REGEX "^LIBRECAD_PLUGIN_INSTALL_DIR:STRING=")
if(NOT plugin_setting)
    message(FATAL_ERROR "Plugin destinations must remain STRING cache entries")
endif()
if(CMAKE_HOST_APPLE)
    set(default_dir "bin/LibreCAD.app/Contents/PlugIns/LibreCAD")
    set(qmake_build_dir "../../LibreCAD.app/Contents/PlugIns/LibreCAD")
else()
    set(default_dir "bin/resources/plugins")
    set(qmake_build_dir "../../unix/resources/plugins")
endif()
if(NOT initial_LIBRECAD_PLUGIN_INSTALL_DIR STREQUAL default_dir)
    message(FATAL_ERROR "CI must exercise the default plugin destination first")
endif()

function(run_checked)
    execute_process(COMMAND ${ARGV} WORKING_DIRECTORY "${work_dir}"
        RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
    if(NOT result STREQUAL "0")
        string(SUBSTRING "${output}" 0 3000 output)
        message(FATAL_ERROR "${ARGV}\n${output}\n${error}")
    endif()
endfunction()

string(RANDOM LENGTH 8 ALPHABET 0123456789abcdef run_id)
set(work_dir "${build_dir}/plugin-install-check/${run_id}")
file(MAKE_DIRECTORY "${work_dir}")
# Windows CI uses CMake; qmake checks run in the existing Unix Qt environments.
if(NOT CMAKE_HOST_WIN32)
    find_program(qmake NAMES qmake6 qmake REQUIRED)
    file(WRITE "${work_dir}/check.pro"
        "include(\"${source_dir}/plugins/sample/sample.pro\")\n")
    file(APPEND "${work_dir}/check.pro" [=[
TEMPLATE = aux
EXPECTED_PLUGIN_DIR = $$quote($$EXPECTED_PLUGIN_DIR)
isEmpty(EXPECTED_PLUGIN_DIR) {
    contains(INSTALLS, target):error(Unexpected default plugin install rule)
} else {
    !equals(target.path, $$EXPECTED_PLUGIN_DIR):error(Incorrect plugin install destination)
    !contains(INSTALLS, target):error(Missing plugin install rule)
}
!equals(DESTDIR, $$EXPECTED_BUILD_DIR):error(Plugin build output changed)
]=])
endif()

# Reconfigure only; the preceding CI build supplies the sample module.
foreach(mode default relative absolute)
    set(prefix "${work_dir}/${mode}/prefix")
    if(mode STREQUAL "default")
        set(destination "${default_dir}")
        set(expected_qmake_dir "")
    elseif(mode STREQUAL "relative")
        set(destination "lib/librecad/plugins")
        set(expected_qmake_dir "${prefix}/${destination}")
    else()
        set(destination "${work_dir}/${mode}/custom plugins")
        set(expected_qmake_dir "${destination}")
    endif()
    run_checked("${CMAKE_COMMAND}" -S "${source_dir}" -B "${build_dir}"
        "-DLIBRECAD_PLUGIN_INSTALL_DIR=${destination}")
    load_cache("${build_dir}" READ_WITH_PREFIX actual_ LIBRECAD_PLUGIN_INSTALL_DIR)
    if(NOT actual_LIBRECAD_PLUGIN_INSTALL_DIR STREQUAL destination)
        message(FATAL_ERROR "Plugin override was changed: ${actual_LIBRECAD_PLUGIN_INSTALL_DIR}")
    endif()
    run_checked("${CMAKE_COMMAND}" "-DCMAKE_INSTALL_PREFIX=${prefix}"
        -DCMAKE_INSTALL_CONFIG_NAME=Release
        -P "${build_dir}/plugins/sample/cmake_install.cmake")
    if(IS_ABSOLUTE "${destination}")
        set(installed_dir "${destination}")
    else()
        set(installed_dir "${prefix}/${destination}")
    endif()
    file(GLOB installed_plugins "${installed_dir}/*sample*")
    if(NOT installed_plugins)
        message(FATAL_ERROR "Sample plugin was not installed to ${installed_dir}")
    endif()
    if(NOT CMAKE_HOST_WIN32)
        run_checked("${qmake}" -E "${work_dir}/check.pro"
            "LIBRECAD_PLUGIN_INSTALL_DIR=${expected_qmake_dir}"
            "EXPECTED_PLUGIN_DIR=${expected_qmake_dir}"
            "EXPECTED_BUILD_DIR=${qmake_build_dir}")
    endif()
    message(STATUS "Plugin installation check passed: ${mode}")
endforeach()
run_checked("${CMAKE_COMMAND}" -S "${source_dir}" -B "${build_dir}"
    "-DLIBRECAD_PLUGIN_INSTALL_DIR=${initial_LIBRECAD_PLUGIN_INSTALL_DIR}")
