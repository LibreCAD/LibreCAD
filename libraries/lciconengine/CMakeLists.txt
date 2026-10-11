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
if(APPLE)
    # Keep the custom .lci icon engine inside the installed app so packaging
    # and Qt's PlugIns lookup include it. A sibling bin/iconengines directory
    # is left behind when only LibreCAD.app is copied to another machine.
    install(TARGETS ${PLUGIN_NAME}
        LIBRARY DESTINATION bin/LibreCAD.app/Contents/PlugIns/iconengines)
else()
    install(TARGETS ${PLUGIN_NAME} BUNDLE DESTINATION bin LIBRARY DESTINATION bin/iconengines)
endif()

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
        LIBRARY_OUTPUT_NAME ${PLUGIN_NAME}  # Creates lc_svgiconplugin.dll
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)

if (WIN32)
    SET(RC_SOURCE src/lc_svgicons.rc)
    target_sources(${PLUGIN_NAME} PRIVATE ${RC_SOURCE})
endif ()
