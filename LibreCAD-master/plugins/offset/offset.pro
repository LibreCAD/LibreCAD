#-------------------------------------------------
#
# Project created by QtCreator 2018-11-29T15:45:37
#
#-------------------------------------------------

QT       += gui
QT       += widgets
TEMPLATE  = lib
CONFIG   += plugin
VERSION   = 0.0.1
TARGET    = $$qtLibraryTarget(offset)


GENERATED_DIR = ../../generated/plugin/offset
# Use common project definitions.
include(../../common.pri)

# For plugins
INCLUDEPATH    += ../../librecad/src/plugins

SOURCES += \
    clipper.cpp \
    lc_offset.cpp \
    lc_offsetdlg.cpp

HEADERS  += \
    clipper.hpp \
    lc_offset.h \
    lc_offsetdlg.h


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
