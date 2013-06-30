#-------------------------------------------------
#
# Project created by QtCreator 2011-03-22T19:33:11
#
#-------------------------------------------------

QT       -= core gui
TEMPLATE = lib

CONFIG += static warn_on

DESTDIR = ../../generated/lib

VERSION = 0.5.8

DLL_NAME = dxfrw
TARGET = $$DLL_NAME

GENERATED_DIR = ../../generated/lib/libdxfrw
# Use common project definitions.
include(../../settings.pro)
include(../../common.pro)

# DEFINES += DRW_DBG

SOURCES += \
    src/libdxfrw.cpp \
    src/drw_entities.cpp \
    src/drw_objects.cpp \
    src/drw_textcodec.cpp \
    src/dxfreader.cpp \
    src/dxfwriter.cpp

HEADERS += \
    src/libdxfrw.h \
    src/drw_base.h \
    src/drw_entities.h \
    src/drw_objects.h \
    src/drw_textcodec.h \
    src/dxfreader.h \
    src/dxfwriter.h \
    src/drw_interface.h \
    src/drw_cptables.h \
    src/drw_cptable950.h \
    src/drw_cptable949.h \
    src/drw_cptable936.h \
    src/drw_cptable932.h

