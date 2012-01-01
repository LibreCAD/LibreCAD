
# Common project definitions for LibreCAD. This file gets
# included from the various *.pro files.

# On windows, check for MSVC compilers - they need help on C99 
# features and a hint to povide M_PI et al.

win32 {
    win32-msvc2003|win32-msvc2005|win32-msvc2008|win32-msvc2010 {
       verbose:message(Setting up support for MSVC.)
       DEFINES += EMU_C99 _USE_MATH_DEFINES
    }
}

