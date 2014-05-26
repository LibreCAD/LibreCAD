QT       += gui
TEMPLATE = lib
CONFIG += plugin
VERSION = 1.0.1
PLUGIN_NAME=plotequation

GENERATED_DIR = ../../generated/plugin/plotequation
# Use common project definitions.
include(../../common.pri)
include(../../librecad/src/muparser.pri)

# For plugins
INCLUDEPATH    += ../../librecad/src/plugins
INCLUDEPATH    += $$DEPENDPATH

SOURCES += \
    plot.cpp \
    plotdialog.cpp

HEADERS += \
    plotdialog.h \
    plot.h


# DLLDESTDIR = ../../unix/resources/plugins/
win32 {
        DLLDESTDIR = ../../windows/resources/plugins
        TARGET = $$PLUGIN_NAME
}
unix {
    macx {
        TARGET = ../../LibreCAD.app/Contents/Resources/plugins/$$PLUGIN_NAME
    }
    else {
        TARGET = ../../unix/resources/plugins/$$PLUGIN_NAME
    }
}
