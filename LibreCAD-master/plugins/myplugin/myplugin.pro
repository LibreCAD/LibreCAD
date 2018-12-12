#-------------------------------------------------
#
# Project created by QtCreator 2018-11-22T20:14:47
#
#-------------------------------------------------


QT       += gui
QT       += widgets
TEMPLATE  = lib
CONFIG   += plugin
VERSION   = 0.0.1
TARGET    = $$qtLibraryTarget(myplugin)

GENERATED_DIR = ../../generated/plugin/myplugin
# Use common project definitions.
include(../../common.pri)

# For plugins
INCLUDEPATH    += ../../librecad/src/plugins

SOURCES += \
    lc_myplugin.cpp \
    mypluginwnd.cpp \
    clipper.cpp


HEADERS += \
    lc_myplugin.h \
    mypluginwnd.h \
    clipper.hpp

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
