QT       += gui
TEMPLATE  = lib
CONFIG   += plugin
VERSION   = 0.0.1
TARGET    = $$qtLibraryTarget(divide)

GENERATED_DIR = ../../generated/plugin/divide
# Use common project definitions.
include(../../common.pri)

# For plugins
INCLUDEPATH    += ../../librecad/src/plugins

SOURCES += divide.cpp \
    dividedlg.cpp


HEADERS += divide.h \
    dividedlg.h

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
