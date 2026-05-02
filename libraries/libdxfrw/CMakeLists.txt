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
    add_compile_options(-Wall -Wextra -pedantic -Werror)
endif()


include(GNUInstallDirs)

set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# SOURCE FILES
set(libdxfrw_srcs
  src/intern/drw_dbg.cpp
  src/intern/drw_textcodec.cpp
  src/intern/dwgbuffer.cpp
  src/intern/dwgreader.cpp
  src/intern/dwgreader15.cpp
  src/intern/dwgreader18.cpp
  src/intern/dwgreader21.cpp
  src/intern/dwgreader24.cpp
  src/intern/dwgreader27.cpp
  src/intern/dwgreader32.cpp
  src/intern/dwgutil.cpp
  src/intern/dxfreader.cpp
  src/intern/dxfwriter.cpp
  src/intern/rscodec.cpp
  src/drw_base.cpp
  src/drw_classes.cpp
  src/drw_entities.cpp
  src/drw_header.cpp
  src/drw_objects.cpp
  src/libdwgr.cpp
  src/libdxfrw.cpp
)

# HEADER FILES
set(libdxfrw_hdrs
  src/intern/drw_cptable932.h
  src/intern/drw_cptable936.h
  src/intern/drw_cptable949.h
  src/intern/drw_cptable950.h
  src/intern/drw_cptables.h
  src/intern/drw_dbg.h
  src/intern/drw_textcodec.h
  src/intern/dwgbuffer.h
  src/intern/dwgreader.h
  src/intern/dwgreader15.h
  src/intern/dwgreader18.h
  src/intern/dwgreader21.h
  src/intern/dwgreader24.h
  src/intern/dwgreader27.h
  src/intern/dwgreader32.h
  src/intern/dwgutil.h
  src/intern/dxfreader.h
  src/intern/dxfwriter.h
  src/intern/rscodec.h
  src/main_doc.h
)

set(installdxfrw_hdrs
  src/drw_base.h
  src/drw_classes.h
  src/drw_entities.h
  src/drw_header.h
  src/drw_interface.h
  src/drw_objects.h
  src/libdwgr.h
  src/libdxfrw.h
)

set(LIBDXFRW_INSTALL_INCLUDEDIR ${CMAKE_INSTALL_INCLUDEDIR}/libdxfrw)

add_library(dxfrw ${libdxfrw_srcs} ${libdxfrw_hdrs})
set_target_properties(dxfrw PROPERTIES 
	VERSION "${PROJECT_VERSION}" 
	SOVERSION "${PROJECT_VERSION_MAJOR}"
	EXPORT_NAME libdxfrw
)
target_compile_features(dxfrw PUBLIC cxx_std_14)
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
