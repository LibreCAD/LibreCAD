#-------------------------------------------------
#
# Project created by QtCreator 2011-03-22T19:33:11
#
#-------------------------------------------------

QT       += gui
TEMPLATE = lib
CONFIG += plugin
VERSION = 1.0.1
PLUGIN_NAME=sameprop

SOURCES += sameprop.cpp

HEADERS += sameprop.h


# DLLDESTDIR = ../../unix/resources/plugins/
win32 {
    Debug {
        DLLDESTDIR = ../../debug/resources/plugins
        TARGET = $$PLUGIN_NAME

    } else {
        DLLDESTDIR = ../../release/resources/plugins
        TARGET = $$PLUGIN_NAME
    }
}
unix {
    macx { 
	TARGET = ../../LibreCAD.app/Contents/Resources/plugins/$$PLUGIN_NAME
    }
    else { 
	TARGET = ../../unix/resources/plugins/$$PLUGIN_NAME
    }
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

#DEFINES += sample_LIBRARY
