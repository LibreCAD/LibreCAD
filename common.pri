
# include user-defined things in every qmake project
exists( custom.pro ):include( custom.pro )
exists( custom.pri ):include( custom.pri )

include( settings.pri )

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

# Windows compiler settings
win32 {
    QMAKE_CXXFLAGS += -U__STRICT_ANSI__
    QMAKE_CFLAGS_THREAD -= -mthreads
    QMAKE_CXXFLAGS_THREAD -= -mthreads
    QMAKE_LFLAGS_THREAD -= -mthreads
    #qt version check for mingw
    win32-g++ {
        contains(QT_VERSION, ^4\\.8\\.[0-4]) {
            DEFINES += QT_NO_CONCURRENT=0
        }
        # Silence warning: typedef '...' locally defined but not used [-Wunused-local-typedefs]
        # this was caused by boost headers and g++ 4.8.0 (Qt 5.1 / MinGW 4.8)
        greaterThan( QT_MAJOR_VERSION, 4 ) {
            QMAKE_CXXFLAGS += -Wno-unused-local-typedefs
        }
    }
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


QMAKE_CXXFLAGS_DEBUG += -g
QMAKE_CXXFLAGS += -g

# fix for GitHub Issue #880
# prevent QMake from using -isystem flag for system include path
# this breaks gcc 6 builds because of its #include_next feature
QMAKE_CFLAGS_ISYSTEM = ""

# svg support
QT += svg

greaterThan( QT_MAJOR_VERSION, 4) {
	CONFIG += c++11
}else{
	QMAKE_CXXFLAGS += -std=c++11
	QMAKE_CXXFLAGS_DEBUG += -std=c++11
}

# RVT July 12 2015, I believe we need these here
#macx{
#    QMAKE_CXXFLAGS_DEBUG += -mmacosx-version-min=10.8
#    QMAKE_CXXFLAGS += -mmacosx-version-min=10.8
#}
