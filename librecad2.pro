#-------------------------------------------------
#
# Project created by QtCreator 2011-03-22T19:33:11
#
#-------------------------------------------------

TEMPLATE = subdirs
TARGET = librecad

CONFIG += ordered
#QT += gui

SUBDIRS     = \
        libdxfrw \
        src \
        plugins


TRANSLATIONS =

# Store intermedia stuff somewhere else
OBJECTS_DIR = intermediate/obj
MOC_DIR = intermediate/moc
RCC_DIR = intermediate/rcc
TS_DIR = intermediate/ts
UI_DIR = intermediate/ui
UI_HERADERS_DIR = intermediate/ui
UI_SOURCES_DIR = intermediate/ui

# install
win32 {
    INSTALLDIR = ../release
}
unix {
    macx { 
        INSTALLDIR = ../LibreCAD.app/Contents
    }
    else { 
        INSTALLDIR = ../unix
    }
}

