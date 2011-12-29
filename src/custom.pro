# tin-pot@gmx.net 2011-12-27: 
# Build LibreCAD with VS .NET 2003, using Qt 4.3, emulate C99.

DEFINES += EMU_C99 _USE_MATH_DEFINES
CONFIG += assistant

HEADERS += \
    main/emu_qt44.h \
    main/emu_qt45.h \
    main/emu_c99.h

SOURCES += \
    main/emu_qt44.cpp \
    main/emu_qt45.cpp \
    main/emu_c99.cpp


