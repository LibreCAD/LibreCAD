set(PLUGIN_NAME lc_svgiconengine)

qt_add_plugin(
        ${PLUGIN_NAME}
        SHARED
        src/lc_svgiconengine.h
        src/lc_svgicons.json
        src/lc_svgiconengineplugin.h
        src/lc_svgiconengineplugin.cpp
        src/lc_svgiconengine.cpp
)

#qt_add_translations(${PLUGIN_NAME} TS_FILE_DIR ../ts TS_FILES ${PLUGIN_TS_FILES})
install(TARGETS ${PLUGIN_NAME} BUNDLE DESTINATION bin LIBRARY DESTINATION bin/iconengines)

set_target_properties(
        ${PLUGIN_NAME}
        PROPERTIES
        AUTOMOC ON
        AUTOUIC ON
)

target_link_libraries(
        ${PLUGIN_NAME}
        PRIVATE
        Qt6::Core
        Qt6::Widgets
        Qt6::Gui
        Qt6::Svg
)

set_target_properties(${PLUGIN_NAME} PROPERTIES
        LIBRARY_OUTPUT_NAME ${PLUGIN_NAME}  # Creates lc_svgiconengine.dll
)

if (WIN32)
    SET(RC_SOURCE src/lc_svgicons.rc)
    target_sources(${PLUGIN_NAME} PRIVATE ${RC_SOURCE})

    # main.cpp loads icon engines from iconengines\ beside LibreCAD.exe, which
    # the packaging copies whole. qt_add_plugin() makes a MODULE library,
    # whose DLL follows LIBRARY_OUTPUT_DIRECTORY (a SHARED one would follow
    # RUNTIME_OUTPUT_DIRECTORY, so both are set). A generator expression stops
    # a multi-config generator from appending the configuration once more.
    get_property(LC_MULTI_CONFIG GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
    if (LC_MULTI_CONFIG)
        set(LC_ICONENGINES_DIR "${CMAKE_BINARY_DIR}/$<CONFIG>/iconengines")
    else ()
        set(LC_ICONENGINES_DIR "${CMAKE_BINARY_DIR}/iconengines")
    endif ()
    set_target_properties(${PLUGIN_NAME} PROPERTIES
            LIBRARY_OUTPUT_DIRECTORY "${LC_ICONENGINES_DIR}"
            RUNTIME_OUTPUT_DIRECTORY "${LC_ICONENGINES_DIR}"
    )
endif ()
