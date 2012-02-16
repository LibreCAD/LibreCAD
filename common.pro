

# Store intermedia stuff somewhere else
OBJECTS_DIR = generated/obj
MOC_DIR = generated/moc
RCC_DIR = generated/rcc
TS_DIR = generated/ts
UI_DIR = generated/ui
UI_HEADERS_DIR = generated/ui
UI_SOURCES_DIR = generated/ui

# Copy command
win32 {
    COPY = copy /y
} else {
    COPY = cp
}

# Compiler settings
unix {
    macx {   
        CONFIG += x86 x86_64
        SUBDIRS += attf2lff
    }
    else {
        SUBDIRS += attf2lff
    }
}

# Installation location
win32 {
    INSTALLDIR = windows
}
unix {
    macx { 
        INSTALLDIR = LibreCAD.app/Contents
    }
    else { 
        INSTALLDIR = unix
    }
}


# Boost
win32 {

        # Use Boost on Windows.

        # Specify where boost is installed (this should have the boost
        # headers in ./boost/ and libraries in ./lib/).
        #
        # See
        #   http://www.boostpro.com/download/
        # and
        #   http://www.boost.org/doc/libs/1_47_0/more/getting_started/windows.html
        # on getting and using a boost library prebuild for MSVC.
        BOOST_DIR = $$(ProgramFiles)/boost/boost_1_48_0

        # BOOST_LIBS may specifiy boost import libraries (this are the libraries
        # without the `lib` prefix, which have a corresponding `.dll` file!).
        # They are only needed when dynamically linking against boost, otherwise 
        # leave it empty and the autolink process will take care of linking to
        # the correct (static) boost library. (Most of the boost components are
        # header-only anyway.)
        #
        # For example: `BOOST_LIBS = -lboost_regex-vc71-mt-1_47.lib`
        BOOST_LIBS =

        # Make boost known to compiler and linker.
        # This should also work on other platforms (but BOOST_LIBS may need
        # to be set to the required boost *.sl/*.so/*.dylib/*.a).
        INCLUDEPATH += "$${BOOST_DIR}"
        LIBS += -L"$${BOOST_DIR}/lib" $${BOOST_LIBS}
        HEADERS += "$${BOOST_DIR}"
        !build_pass:verbose:message(Using boost libraries in $${BOOST_DIR}.)

    # On windows, check for MSVC compilers - they need help on C99 
    # features and a hint to povide M_PI et al.
    win32-msvc.net|win32-msvc2003|win32-msvc2005|win32-msvc2008|win32-msvc2010 {
       !build_pass:verbose:message(Setting up support for MSVC.)
       DEFINES += EMU_C99 _USE_MATH_DEFINES
    }

    # The .NET 2003 compiler (at least) is touchy about its own headers ...
    win32-msvc2003 {
       # Silence "unused formal parameter" warnings about unused `_Iosbase` 
       # in the header file `xloctime` (a Vc7 header after all!).
       QMAKE_CXXFLAGS += /wd4100
    }
}

macx {
        !exists( /opt/local/lib/libboost* ) {
            error(Boost was not found, please install boost!)
        }

        # Use Boost on OSX
	# Install boost with : sudo port install boost

        # Specify where boost is installed (this should have the boost
        # headers in ./boost/ and libraries in ./lib/).
        BOOST_DIR = /opt/local/include/

        # BOOST_LIBS may specifiy boost import libraries (this are the libraries
        # without the `lib` prefix, which have a corresponding `.dll` file!).
        # They are only needed when dynamically linking against boost, otherwise 
        # leave it empty and the autolink process will take care of linking to
        # the correct (static) boost library. (Most of the boost components are 
        # header-only anyway.)
        # 
        # For example: `BOOST_LIBS = -lboost_regex-vc71-mt-1_47.lib`
        BOOST_LIBS = 

        # Make boost known to compiler and linker.
        # This should also work on other platforms (but BOOST_LIBS may need
        # to be set to the required boost *.sl/*.so/*.dylib/*.a).
        INCLUDEPATH += $${BOOST_DIR}
	# HEADERS += $${BOOST_DIR}
        LIBS += -L/opt/local/lib $${BOOST_LIBS}
        # !build_pass:verbose:message(Using boost libraries in $${BOOST_DIR}.)
        message(Using boost libraries in $${BOOST_DIR}.)
}

