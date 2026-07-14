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

# svg support
QT -= svg

# DEFINES += DRW_DBG

SOURCES += \
    src/libdxfrw.cpp \
    src/libdwgr.cpp \
    src/drw_base.cpp \
    src/drw_header.cpp \
    src/drw_classes.cpp \
    src/drw_entities.cpp \
    src/drw_objects.cpp \
    src/drw_acis.cpp \
    src/intern/drw_textcodec.cpp \
    src/intern/dxfreader.cpp \
    src/intern/dxfwriter.cpp \
    src/intern/dwgreader.cpp \
    src/intern/dwgreaderR1_40.cpp \
    src/intern/dwgreaderR11.cpp \
    src/intern/dwgbuffer.cpp \
    src/intern/drw_dbg.cpp \
    src/intern/dwgreader21.cpp \
    src/intern/dwgreader18.cpp \
    src/intern/dwgreader15.cpp \
    src/intern/dwgutil.cpp \
    src/intern/rscodec.cpp \
    src/intern/dwgreader27.cpp \
    src/intern/dwgreader24.cpp \
    src/intern/dwgreader32.cpp \
    src/intern/proxygraphicdecoder.cpp \
    src/intern/dwgbufferw.cpp \
    src/intern/dwgwriter15.cpp \
    src/intern/dwgwriter18.cpp \
    src/intern/dwgwriter24.cpp \
    src/intern/dwgwriter27.cpp \
    src/intern/dwgwriter32.cpp

HEADERS += \
    src/libdxfrw.h \
    src/libdwgr.h \
    src/drw_interface.h \
    src/drw_base.h \
    src/drw_header.h \
    src/drw_classes.h \
    src/drw_entities.h \
    src/drw_objects.h \
    src/drw_acis.h \
    src/intern/drw_textcodec.h \
    src/intern/dxfreader.h \
    src/intern/dxfwriter.h \
    src/intern/dwgreader.h \
    src/intern/dwgreaderR1_40.h \
    src/intern/dwgreaderR11.h \
    src/intern/dwgbuffer.h \
    src/intern/drw_cptables.h \
    src/intern/drw_cptable950.h \
    src/intern/drw_cptable949.h \
    src/intern/drw_cptable936.h \
    src/intern/drw_cptable932.h \
    src/intern/drw_dbg.h \
    src/intern/dwgreader21.h \
    src/intern/dwgreader18.h \
    src/intern/dwgreader15.h \
    src/intern/dwgutil.h \
    src/intern/rscodec.h \
    src/intern/dwgreader27.h \
    src/intern/dwgreader24.h \
    src/intern/dwgreader32.h \
    src/intern/proxygraphicdecoder.h \
    src/intern/drw_reserve.h \
    src/intern/dwgbufferw.h \
    src/intern/dwgwriter.h \
    src/intern/dwgwriter15.h \
    src/intern/dwgwriter18.h \
    src/intern/dwgwriter24.h \
    src/intern/dwgwriter27.h \
    src/intern/dwgwriter32.h
