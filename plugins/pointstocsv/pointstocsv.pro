#-------------------------------------------------
#
# Project created manually on 2023-10-30T19:53:00
#
#-------------------------------------------------

QT       += widgets
TEMPLATE = lib
CONFIG += plugin
VERSION = 1.0.1
TARGET = $$qtLibraryTarget(pointstocsv)

GENERATED_DIR = ../../generated/plugin/pointstocsv
# Use common project definitions.
include(../../common.pri)

# For plugins
INCLUDEPATH    += ../../librecad/src/plugins

SOURCES += pointstocsv.cpp

HEADERS += pointstocsv.h

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

