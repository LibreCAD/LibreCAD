# Copyright (C) 2026 LibreCAD.org and Dongxu Li
# SPDX-License-Identifier: GPL-2.0-or-later

TEMPLATE = lib
TARGET = lcqtbidi
QT = core
CONFIG += static warn_on c++17
include(../../settings.pri)
DESTDIR = ../../generated/lib
OBJECTS_DIR = ../../generated/lib/qtbidi/obj
SOURCES += qt_bidi.cpp
HEADERS += qt_bidi.h
OTHER_FILES += README.librecad
