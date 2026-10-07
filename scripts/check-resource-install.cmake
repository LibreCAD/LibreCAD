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
    message(FATAL_ERROR "Configure LibreCAD before checking resource installation")
endif()
load_cache("${build_dir}" READ_WITH_PREFIX configured_
    CMAKE_GENERATOR CMAKE_MAKE_PROGRAM CMAKE_CXX_COMPILER Qt6_DIR QT_HOST_PATH)
if(NOT configured_CMAKE_GENERATOR STREQUAL "Ninja")
    message(FATAL_ERROR "This CI check requires the Ninja generator used by Pixi")
endif()

function(run_checked)
    execute_process(COMMAND ${ARGV}
        RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
    if(NOT result STREQUAL "0")
        string(SUBSTRING "${output}" 0 4000 output)
        message(FATAL_ERROR "${ARGV}\n${output}\n${error}")
    endif()
    set(command_output "${output}" PARENT_SCOPE)
endfunction()

# Check the executable's graph, not ALL, which also builds unrelated translation targets.
run_checked("${configured_CMAKE_MAKE_PROGRAM}" -C "${build_dir}" -t inputs librecad)
foreach(catalog librecad_de plugins_de)
    if(NOT command_output MATCHES "(^|\n)([^\n]*/)?${catalog}\\.qm(\n|$)")
        message(FATAL_ERROR "librecad does not depend on ${catalog}.qm")
    endif()
endforeach()
run_checked("${CMAKE_COMMAND}" --build "${build_dir}"
    --target release_translations --parallel 4)

string(RANDOM LENGTH 8 ALPHABET 0123456789abcdef run_id)
set(check_dir "${build_dir}/resource-install-check/${run_id}")
set(prefix "${check_dir}/installed resources")
run_checked("${CMAKE_COMMAND}" --install "${build_dir}" --config Release
    --prefix "${prefix}" --component Resources)
if(CMAKE_HOST_APPLE)
    set(data_dir "bin/LibreCAD.app/Contents/Resources")
elseif(CMAKE_HOST_WIN32)
    set(data_dir "bin/resources")
else()
    set(data_dir "share/librecad")
endif()

foreach(catalog_type librecad plugins)
    file(GLOB catalogs "${source_dir}/${catalog_type}/ts/*.ts")
    foreach(catalog IN LISTS catalogs)
        get_filename_component(name "${catalog}" NAME_WE)
        set(generated "${build_dir}/${name}.qm")
        set(installed "${prefix}/${data_dir}/qm/${name}.qm")
        if(NOT EXISTS "${installed}")
            message(FATAL_ERROR "Missing installed catalog: ${installed}")
        endif()
        file(SHA256 "${generated}" expected_hash)
        file(SHA256 "${installed}" installed_hash)
        if(NOT installed_hash STREQUAL expected_hash)
            message(FATAL_ERROR "Installed catalog differs from build output: ${name}")
        endif()
    endforeach()
endforeach()

foreach(resource_dir fonts patterns library)
    file(GLOB_RECURSE files LIST_DIRECTORIES FALSE
        RELATIVE "${source_dir}/librecad/support/${resource_dir}"
        "${source_dir}/librecad/support/${resource_dir}/*")
    foreach(file IN LISTS files)
        if(NOT EXISTS "${prefix}/${data_dir}/${resource_dir}/${file}")
            message(FATAL_ERROR "Missing installed ${resource_dir} file: ${file}")
        endif()
    endforeach()
endforeach()
if(NOT CMAKE_HOST_APPLE AND NOT CMAKE_HOST_WIN32)
    foreach(file man/man1/librecad.1 applications/librecad.desktop
            pixmaps/librecad.png mime/packages/librecad.xml
            metainfo/org.librecad.librecad.appdata.xml)
        if(NOT EXISTS "${prefix}/share/${file}")
            message(FATAL_ERROR "Missing desktop integration file: ${file}")
        endif()
    endforeach()
elseif(EXISTS "${prefix}/share/applications/librecad.desktop")
    message(FATAL_ERROR "Freedesktop integration must not be installed on this platform")
endif()

file(MAKE_DIRECTORY "${check_dir}/probe")
file(WRITE "${check_dir}/probe/main.cpp" [=[
#include <QCoreApplication>
#include <QDebug>
#include <QTranslator>

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    if (app.arguments().size() != 2) {
        return 2;
    }
    for (const QString& catalog : {QStringLiteral("librecad_de.qm"),
                                   QStringLiteral("plugins_de.qm")}) {
        QTranslator embedded, installed;
        if (!embedded.load(QStringLiteral(":/i18n/") + catalog)
            || !installed.load(catalog, app.arguments().at(1))) {
            qCritical() << "Cannot load embedded and installed catalog:" << catalog;
            return 1;
        }
    }
    return 0;
}
]=])
file(WRITE "${check_dir}/probe/CMakeLists.txt" [=[
cmake_minimum_required(VERSION 3.28)
project(LibreCADResourceProbe LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTORCC ON)
find_package(Qt6 REQUIRED COMPONENTS Core)
qt_add_executable(resource_probe main.cpp
    "${LIBRECAD_BUILD_DIR}/.qt/rcc/librecad_translations.qrc"
    "${LIBRECAD_BUILD_DIR}/.qt/rcc/librecad_plugin_translations.qrc")
target_link_libraries(resource_probe PRIVATE Qt6::Core)
file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/probe-path.txt"
    CONTENT "$<TARGET_FILE:resource_probe>")
]=])
run_checked("${CMAKE_COMMAND}" -S "${check_dir}/probe" -B "${check_dir}/probe-build"
    -G Ninja -DCMAKE_BUILD_TYPE=Release
    "-DCMAKE_MAKE_PROGRAM=${configured_CMAKE_MAKE_PROGRAM}"
    "-DCMAKE_CXX_COMPILER=${configured_CMAKE_CXX_COMPILER}"
    "-DQt6_DIR=${configured_Qt6_DIR}" "-DQT_HOST_PATH=${configured_QT_HOST_PATH}"
    "-DLIBRECAD_BUILD_DIR=${build_dir}")
run_checked("${CMAKE_COMMAND}" --build "${check_dir}/probe-build" --parallel 4)
file(READ "${check_dir}/probe-build/probe-path.txt" probe_path)
run_checked("${probe_path}" "${prefix}/${data_dir}/qm")
file(REMOVE_RECURSE "${check_dir}")
message(STATUS "Catalog dependencies, embedded translations and installed resources passed")
