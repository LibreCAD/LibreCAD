# This file is part of LibreCAD. SPDX-License-Identifier: GPL-2.0-or-later

# Reuse the application's source/feature lists without its GUI entry point.
# Keep object, moc, rcc and uic products separate from the application build.
LC_OFFSET_SRC = $$clean_path($$PWD/../src)
include(../src/src.pro)
DEFINES += LC_OFFSET_GEOMETRY_WORKER
CONFIG += console
CONFIG -= app_bundle
TARGET = LibreCADOffsetWorker
QMAKE_POST_LINK =
QMAKE_INFO_PLIST =
RC_FILE =
ICON =
TRANSLATIONS =

SOURCES += lib/offset_worker/lc_offsetworkerkernel.cpp \
           lib/offset_worker/lc_offsetworkermain.cpp
HEADERS += lib/offset_worker/lc_offsetworkerkernel.h

# src.pro is one directory beside this project; resolve its source paths there.
LC_OFFSET_SOURCES =
for(file, SOURCES): LC_OFFSET_SOURCES += $$absolute_path($$file, $$LC_OFFSET_SRC)
SOURCES = $$LC_OFFSET_SOURCES
LC_OFFSET_HEADERS =
for(file, HEADERS): LC_OFFSET_HEADERS += $$absolute_path($$file, $$LC_OFFSET_SRC)
HEADERS = $$LC_OFFSET_HEADERS
LC_OFFSET_FORMS =
for(file, FORMS): LC_OFFSET_FORMS += $$absolute_path($$file, $$LC_OFFSET_SRC)
FORMS = $$LC_OFFSET_FORMS
LC_OFFSET_RESOURCES =
for(file, RESOURCES): LC_OFFSET_RESOURCES += $$absolute_path($$file, $$LC_OFFSET_SRC)
RESOURCES = $$LC_OFFSET_RESOURCES
LC_OFFSET_INCLUDES =
for(directory, INCLUDEPATH): LC_OFFSET_INCLUDES += $$absolute_path($$directory, $$LC_OFFSET_SRC)
INCLUDEPATH = $$LC_OFFSET_INCLUDES
DEPENDPATH += $$LC_OFFSET_SRC

GENERATED_DIR = ../../generated/offset_worker
OBJECTS_DIR = $$GENERATED_DIR/obj
MOC_DIR = $$GENERATED_DIR/moc
RCC_DIR = $$GENERATED_DIR/rcc
UI_DIR = $$GENERATED_DIR/ui
UI_HEADERS_DIR = $$UI_DIR
UI_SOURCES_DIR = $$UI_DIR
macx: DESTDIR = $${INSTALLDIR}/LibreCAD.app/Contents/MacOS
