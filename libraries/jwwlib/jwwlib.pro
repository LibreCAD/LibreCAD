#-------------------------------------------------
#
# Project created by QtCreator 2011-03-22T19:33:11
#
#-------------------------------------------------

QT       -= core gui
TEMPLATE = lib

CONFIG += static warn_on

DESTDIR = ../../generated/lib

VERSION = 0.0.1

DLL_NAME = jwwlib
TARGET = $$DLL_NAME

GENERATED_DIR = ../../generated/lib/jwwlib
# Use common project definitions.
include(../../common.pri)

#INCLUDEPATH += \
#    ../dxflib/src

SOURCES += \
    src/dl_writer_ascii.cpp \
    src/dl_jww.cpp \
    src/jwwdoc.cpp

HEADERS += \
    ../jwwlib/src/dl_attributes.h \
    ../jwwlib/src/dl_codes.h \
    ../jwwlib/src/dl_creationinterface.h \
    ../jwwlib/src/dl_entities.h \
    ../jwwlib/src/dl_extrusion.h \
    ../jwwlib/src/dl_exception.h \
    ../jwwlib/src/dl_writer.h \
    ../jwwlib/src/dl_writer_ascii.h \
    ../jwwlib/src/dl_jww.h \
    ../jwwlib/src/jwtype.h \
    ../jwwlib/src/jwwdoc.h


