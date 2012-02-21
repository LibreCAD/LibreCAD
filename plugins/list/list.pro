#-------------------------------------------------
#
# Project created by QtCreator 2011-03-22T19:33:11
#
#-------------------------------------------------

QT       += gui
TEMPLATE = lib
CONFIG += plugin
VERSION = 1.0.1
PLUGIN_NAME=list

GENERATED_DIR = ../../generated/plugin/list
# Use common project definitions.
include(../../common.pro)

# For plugins
INCLUDEPATH    += ../../librecad/src/plugins

SOURCES += list.cpp

HEADERS += list.h


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

