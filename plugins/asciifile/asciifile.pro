#-------------------------------------------------
#
# Project created by QtCreator 2011-03-22T19:33:11
#
#-------------------------------------------------

QT       += gui
TEMPLATE = lib
CONFIG += plugin
VERSION = 1.0.0

win32 {
    debug {
        TARGET = ../../debug/resources/plugins/asciifile

    } else {
        TARGET = ../../release/resources/plugins/asciifile
    }
}
unix {
    TARGET = ../../unix/resources/plugins/asciifile
}

INCLUDEPATH    += ../../src/plugins

# Store intermedia stuff somewhere else
OBJECTS_DIR = ../intermediate/obj
MOC_DIR = ../intermediate/moc
RCC_DIR = ../intermediate/rcc
TS_DIR = ../intermediate/ts
UI_DIR = ../intermediate/ui
UI_HERADERS_DIR = ../intermediate/ui
UI_SOURCES_DIR = ../intermediate/ui

SOURCES += asciifile.cpp

HEADERS += asciifile.h
