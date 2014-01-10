#-------------------------------------------------
#
# Project created by QtCreator 2011-03-22T19:33:11
#
#-------------------------------------------------

QT       -= core gui
TEMPLATE = lib

CONFIG += static warn_on

DESTDIR = ../../generated/lib

VERSION = 0.5.11

DLL_NAME = dxfrw
TARGET = $$DLL_NAME

GENERATED_DIR = ../../generated/lib/libdxfrw
# Use common project definitions.
include(../../common.pri)
QMAKE_CXXFLAGS_DEBUG -= -std=c++0x -std=c++11
QMAKE_CXXFLAGS -= -std=c++0x -std=c++11

# svg support
QT -= svg

# DEFINES += DRW_DBG

SOURCES += \
    src/libdxfrw.cpp \
    src/drw_entities.cpp \
    src/drw_objects.cpp \
    src/intern/drw_textcodec.cpp \
    src/intern/dxfreader.cpp \
    src/intern/dxfwriter.cpp

HEADERS += \
    src/libdxfrw.h \
    src/drw_base.h \
    src/drw_entities.h \
    src/drw_objects.h \
    src/intern/drw_textcodec.h \
    src/intern/dxfreader.h \
    src/intern/dxfwriter.h \
    src/drw_interface.h \
    src/intern/drw_cptables.h \
    src/intern/drw_cptable950.h \
    src/intern/drw_cptable949.h \
    src/intern/drw_cptable936.h \
    src/intern/drw_cptable932.h

