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

VERSION = 2.2.0.0

DLL_NAME = dxflib
TARGET = $$DLL_NAME

# DEFINES +=

win32 {
    COPY = copy /y
} else {
    COPY = cp
}

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


DESTDIR = ../intermediate

# Store intermedia stuff somewhere else
OBJECTS_DIR = ../intermediate/obj
MOC_DIR = ../intermediate/moc
RCC_DIR = ../intermediate/rcc
TS_DIR = ../intermediate/ts
UI_DIR = ../intermediate/ui
UI_HERADERS_DIR = ../intermediate/ui
UI_SOURCES_DIR = ../intermediate/ui

# Include any custom.pro files for personal/special builds
exists( ../custom.pro ):include( ../custom.pro )

