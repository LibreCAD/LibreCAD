#-------------------------------------------------
#
# Project created by QtCreator 2011-03-22T19:33:11
#
#-------------------------------------------------

QT       -= core gui
TEMPLATE = lib

CONFIG += static warn_on

DESTDIR = ../../generated/lib

VERSION = 2.2.0.0

DLL_NAME = dxflib
TARGET = $$DLL_NAME

GENERATED_DIR = ../../generated/lib/dxflib
# Use common project definitions.
include(../../settings.pro)
include(../../common.pro)

SOURCES += \
    src/dl_dxf.cpp \
    src/dl_writer_ascii.cpp

HEADERS += \
    src/dl_attributes.h \
    src/dl_codes.h \
    src/dl_creationadapter.h \
    src/dl_creationinterface.h \
    src/dl_dxf.h \
    src/dl_entities.h \
    src/dl_exception.h \
    src/dl_extrusion.h \
    src/dl_writer.h \
    src/dl_writer_ascii.h


