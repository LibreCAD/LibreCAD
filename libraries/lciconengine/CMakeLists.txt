cmake_minimum_required(VERSION 3.22)
project(LibreCADIconsPlugin VERSION 2.2.2)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(Qt6_DIR "${MIN_GW_PATH_PREFIX}\\lib\\cmake\\Qt6")
set(Qt6Core_DIR "${MIN_GW_PATH_PREFIX}\\lib\\cmake\\Qt6Core")
set(Qt6Svg_DIR "${MIN_GW_PATH_PREFIX}\\lib\\cmake\\Qt6Svg")
set(Qt6CoreTools_DIR "${MIN_GW_PATH_PREFIX}\\lib\\cmake\\Qt6CoreTools")

set(PLUGIN_SOURCES
        src/lc_svgiconengine.h
        src/lc_svgicons.json
        src/lc_svgiconengineplugin.cpp
        src/lc_svgiconengine.cpp)

set(DLL_NAME lc_svgiconplugin)

add_library(${DLL_NAME} SHARED ${PLUGIN_SOURCES})

set_target_properties(${DLL_NAME} PROPERTIES
        LIBRARY_OUTPUT_NAME ${DLL_NAME}  # Creates lc_svgiconplugin.dll
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)

if(WIN32)
    SET(RC_SOURCE
        src/lc_svgicons.rc)
    target_sources(lc_svgiconplugin PRIVATE ${RC_SOURCE})

endif()

target_link_libraries(${DLL_NAME} PRIVATE Qt6::Core Qt6::Widgets Qt6::Gui Qt6::Svg)
