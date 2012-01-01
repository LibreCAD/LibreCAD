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

DLL_NAME = jwwlib
TARGET = $$DLL_NAME

# DEFINES +=

win32 {
    COPY = copy /y
} else {
    COPY = cp
}

INCLUDEPATH += \
    ../dxflib/src

SOURCES += \
    src/dl_jww.cpp \
    src/jwwdoc.cpp

HEADERS += \
    ../jwwlib/src/dl_jww.h \
    ../jwwlib/src/jwtype.h \
    ../jwwlib/src/jwwdoc.h


DESTDIR = ../intermediate

# Store intermedia stuff somewhere else
OBJECTS_DIR = ../intermediate/obj
MOC_DIR = ../intermediate/moc
RCC_DIR = ../intermediate/rcc
TS_DIR = ../intermediate/ts
UI_DIR = ../intermediate/ui
UI_HERADERS_DIR = ../intermediate/ui
UI_SOURCES_DIR = ../intermediate/ui

