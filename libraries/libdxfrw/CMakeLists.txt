cmake_minimum_required(VERSION 3.10)

project(DXFRW VERSION 1.0.1)

# set preferred cache variables
set(LIBDXFRW_BUILD_DOC TRUE CACHE BOOL "Build the documentation")
set(LIBDXFRW_BUILD_DWG2DXF TRUE CACHE BOOL "Build the dwg2dxf application")
set(LIBDXFRW_BUILD_TESTS TRUE CACHE BOOL "Build the DWG read tests (requires dwg2dxf)")


# set compiler warnings
if (MSVC)
    add_compile_options(/W3)
else()
    add_compile_options(-Wall -Wextra -pedantic)
endif()


include(GNUInstallDirs)

set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include("${CMAKE_CURRENT_LIST_DIR}/libdxfrw_sources.cmake")

set(libdxfrw_srcs ${LIBDXFRW_SOURCES})
set(libdxfrw_hdrs ${LIBDXFRW_PRIVATE_HEADERS})
set(installdxfrw_hdrs ${LIBDXFRW_PUBLIC_HEADERS})

set(LIBDXFRW_INSTALL_INCLUDEDIR ${CMAKE_INSTALL_INCLUDEDIR}/libdxfrw)

add_library(dxfrw ${libdxfrw_srcs} ${libdxfrw_hdrs})
set_target_properties(dxfrw PROPERTIES 
	VERSION "${PROJECT_VERSION}" 
	SOVERSION "${PROJECT_VERSION_MAJOR}"
	EXPORT_NAME libdxfrw
)
target_compile_features(dxfrw PUBLIC cxx_std_17)
target_include_directories(dxfrw
		PUBLIC
			$<INSTALL_INTERFACE:${LIBDXFRW_INSTALL_INCLUDEDIR}>
)

if(LIBDXFRW_BUILD_DOC)
    add_custom_command(OUTPUT doc/html/index.html
	    COMMAND doxygen ARGS ${CMAKE_CURRENT_SOURCE_DIR}/libdxfrw.dox
	    MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/libdxfrw.dox
	    VERBATIM)

    add_custom_target(doc
	    DEPENDS doc/html/index.html)
endif()

if(LIBDXFRW_BUILD_DWG2DXF)
    add_subdirectory(dwg2dxf)
endif()

if(LIBDXFRW_BUILD_TESTS AND LIBDXFRW_BUILD_DWG2DXF)
    enable_testing()
    add_subdirectory(tests)
endif()

# INSTALLATION
set(INSTALL_CONFIGDIR ${CMAKE_INSTALL_LIBDIR}/cmake/libdxfrw)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/libdxfrw.pc.in libdxfrw.pc @ONLY)

install(TARGETS dxfrw
		EXPORT libdxfrw-targets
		ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
		LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
		RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(EXPORT libdxfrw-targets
	FILE libdxfrwTargets.cmake
		NAMESPACE libdxfrw::
		DESTINATION ${INSTALL_CONFIGDIR}
)

include(CMakePackageConfigHelpers)
configure_package_config_file(${CMAKE_CURRENT_LIST_DIR}/cmake/libdxfrwConfig.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/libdxfrwConfig.cmake
        INSTALL_DESTINATION ${INSTALL_CONFIGDIR}
)
write_basic_package_version_file(
		${CMAKE_CURRENT_BINARY_DIR}/libdxfrwConfigVersion.cmake
		VERSION ${PROJECT_VERSION}
		COMPATIBILITY AnyNewerVersion
)

install(FILES
		${installdxfrw_hdrs}
		DESTINATION ${LIBDXFRW_INSTALL_INCLUDEDIR}
)
install(FILES
		${CMAKE_CURRENT_BINARY_DIR}/libdxfrw.pc
		DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)
install(FILES
		${CMAKE_CURRENT_BINARY_DIR}/libdxfrwConfig.cmake
		${CMAKE_CURRENT_BINARY_DIR}/libdxfrwConfigVersion.cmake
		DESTINATION ${INSTALL_CONFIGDIR}
)
