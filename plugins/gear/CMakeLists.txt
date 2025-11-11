set(PLUGIN_NAME gear)

qt_add_plugin(
    ${PLUGIN_NAME}
    SHARED
    ${PLUGIN_HEADERS}
    gear.cpp
    gear.h
)

#qt_add_translations(${PLUGIN_NAME} TS_FILE_DIR ../ts TS_FILES ${PLUGIN_TS_FILES})
install(TARGETS ${PLUGIN_NAME} BUNDLE DESTINATION bin LIBRARY DESTINATION bin/resources/plugins)

set_target_properties(
    ${PLUGIN_NAME}
    PROPERTIES
    AUTOMOC ON
    AUTOUIC ON
)

 target_compile_definitions(${PLUGIN_NAME} PRIVATE
 DWGSUPPORT
 MUPARSER_STATIC
)

target_link_libraries(
    ${PLUGIN_NAME}
    PRIVATE
    Qt6::Core
    Qt6::Widgets
)

if(WIN32)
    SET(RC_SOURCE plugin.rc)
    target_sources( ${PLUGIN_NAME} PRIVATE ${RC_SOURCE})
endif()
