# Copyright (C) 2026 LibreCAD.org and Dongxu Li
# SPDX-License-Identifier: GPL-2.0-or-later

TEMPLATE = lib
TARGET = lcfribidi
CONFIG += static warn_on
CONFIG -= qt
include(../../settings.pri)
DESTDIR = ../../generated/lib
OBJECTS_DIR = ../../generated/lib/fribidi/obj
INCLUDEPATH += src
DEFINES += FRIBIDI_LIB_STATIC DONT_HAVE_FRIBIDI_CONFIG_H \
    HAVE_STDLIB_H=1 HAVE_STRING_H=1 HAVE_STRINGIZE=1
msvc: QMAKE_CFLAGS += /UDEBUG
else: QMAKE_CFLAGS += -UDEBUG
SOURCES += src/fribidi-bidi.c src/fribidi-run.c src/fribidi-brackets.c
HEADERS += $$files(src/*.h)
OTHER_FILES += src/brackets.tab.i src/brackets-type.tab.i COPYING README.librecad
