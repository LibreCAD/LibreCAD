#-------------------------------------------------
# CircleTools plugin for LibreCAD 2.2.x (Qt5)
#-------------------------------------------------

QT       += gui
TEMPLATE = lib
CONFIG  += plugin
VERSION  = 1.0.0
TARGET   = $$qtLibraryTarget(circletools)

GENERATED_DIR = ../../generated/plugin/circletools

# Use common project definitions.
include(../../common.pri)

# For plugins
INCLUDEPATH += ../../librecad/src/plugins

SOURCES += circletools.cpp
HEADERS += circletools.h
OTHER_FILES += circletools.json

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
