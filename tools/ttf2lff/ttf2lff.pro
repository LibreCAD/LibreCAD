#-------------------------------------------------
#
# Project created by QtCreator 2011-07-31T10:54:43
#
#-------------------------------------------------

QT -= core gui
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app
DEFINES += VERSION="\"0.0.0.2\""

GENERATED_DIR = ../../generated/tools/ttf2lff
include(../../common.pro)
include(../../freetype.pri)

SOURCES += main.cpp

unix {
    macx {
        TARGET = ../../LibreCAD.app/Contents/MacOS/ttf2lff
    } else {
        TARGET = ../../unix/ttf2lff
    }
}

win32 {
    TARGET = ../../../windows/ttf2lff
}

