#-------------------------------------------------
#
# Project created by QtCreator 2011-03-22T19:33:11
#
#-------------------------------------------------

QT       -= core gui
TEMPLATE = lib

#CONFIG += dll \
CONFIG += static \
    warn_on

VERSION = 0.0.1

# Use common project definitions.
include(../common.pro)

DLL_NAME = dxfrw
TARGET = $$DLL_NAME

DEFINES += DRW_DBG

win32 {
    COPY = copy /y
} else {
    COPY = cp
}

macx { 
    CONFIG += x86 x86_64
}

SOURCES += \
    src/libdxfrw.cpp \
    src/drw_entities.cpp \
    src/drw_objects.cpp \
    src/dxfreader.cpp \
    src/dxfwriter.cpp

HEADERS += \
    src/libdxfrw.h \
    src/drw_base.h \
    src/drw_entities.h \
    src/drw_objects.h \
    src/dxfreader.h \
    src/dxfwriter.h \
    src/drw_interface.h

DESTDIR = ../intermediate

# Store intermedia stuff somewhere else
OBJECTS_DIR = ../intermediate/obj
MOC_DIR = ../intermediate/moc
RCC_DIR = ../intermediate/rcc
TS_DIR = ../intermediate/ts
UI_DIR = ../intermediate/ui
UI_HERADERS_DIR = ../intermediate/ui
UI_SOURCES_DIR = ../intermediate/ui

