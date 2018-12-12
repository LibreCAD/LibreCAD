#-------------------------------------------------
#
# Project created by QtCreator 2011-03-22T19:33:11
#
#-------------------------------------------------

QT       += gui
TEMPLATE = lib
CONFIG += plugin
VERSION = 1.0.1
TARGET = $$qtLibraryTarget(align)

# Store intermedia stuff somewhere else
GENERATED_DIR = ../../generated/plugin/align
# Use common project definitions.
include(../../common.pri)

# For plugins
INCLUDEPATH    += ../../librecad/src/plugins

SOURCES += align.cpp

HEADERS += align.h

# Installation Directory
win32 {
        DESTDIR = ../../windows/resources/plugins
}
unix {
    macx { 
        DESTDIR = ../../LibreCAD.app/Contents/Resources/plugins
    }
    else { 
        DESTDIR = ../../unix/resources/plugins
    }
}
