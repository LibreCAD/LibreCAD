

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

# Boost
INCLUDEPATH += "$${BOOST_DIR}"
LIBS += -L"$${BOOST_LIBDIR}" $${BOOST_LIBS}
HEADERS += "$${BOOST_DIR}"

!exists($${BOOST_DIR}) {
   # error(Boost was not found, please install boost!)
}
!build_pass:verbose:message(Using boost libraries in $${BOOST_DIR}.)

# Windows compiler settings
win32 {
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
