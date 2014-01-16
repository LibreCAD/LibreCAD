#-------------------------------------------------
#
# Project created by QtCreator 2011-03-22T19:33:11
#
#-------------------------------------------------

QT       += gui
TEMPLATE = lib
CONFIG += plugin
VERSION = 1.0.0
PLUGIN_NAME=picfile

GENERATED_DIR = ../../generated/plugin/picfile
# Use common project definitions.
include(../../common.pri)

# For plugins
INCLUDEPATH    += ../../librecad/src/plugins

SOURCES += picfile.cpp
HEADERS += picfile.h

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
