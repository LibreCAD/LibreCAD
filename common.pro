

# Store intermedia stuff somewhere else
isEmpty(GENERATED_DIR){
 GENERATED_DIR = generated
}
 # Store intermedia stuff somewhere else
OBJECTS_DIR = $${GENERATED_DIR}/obj
MOC_DIR = $${GENERATED_DIR}/moc
RCC_DIR = $${GENERATED_DIR}/rcc
UI_DIR = $${GENERATED_DIR}/ui
UI_HEADERS_DIR = $${GENERATED_DIR}/ui
UI_SOURCES_DIR = $${GENERATED_DIR}/ui


# Copy command
win32 {
    COPY = copy /y
} else {
    COPY = cp
}


# Boost
exists($${BOOST_DIR}){
    INCLUDEPATH += "$${BOOST_DIR}"
    LIBS += -L"$${BOOST_LIBDIR}" $${BOOST_LIBS}
    HEADERS += "$${BOOST_DIR}"
}

!exists($${BOOST_DIR}) {
   # error(Boost was not found, please install boost!)
}
message(Using boost libraries in $${BOOST_DIR}.)

# Windows compiler settings
win32 {
    QMAKE_CXXFLAGS += -U__STRICT_ANSI__
    QMAKE_CFLAGS_THREAD -= -mthreads
    QMAKE_LFLAGS_THREAD -= -mthreads
    QMAKE_C++FLAGS_THREAD -= -mthreads
    QMAKE_L++FLAGS_THREAD -= -mthreads

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


# c++11 is now obligatory for LibreCAD
message(We will be using CPP11 features)
QMAKE_CXXFLAGS_DEBUG += -std=c++0x
QMAKE_CXXFLAGS += -std=c++0x


