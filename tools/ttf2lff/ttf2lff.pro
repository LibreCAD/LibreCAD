#-------------------------------------------------
#
# Project created by QtCreator 2011-07-31T10:54:43
#
#-------------------------------------------------

include(../../common.pri)
include(./freetype.pri)

QT -= core gui svg
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app
DEFINES += VERSION="\"0.0.0.2\""

GENERATED_DIR = ../../generated/tools/ttf2lff
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

