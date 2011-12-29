#-------------------------------------------------
#
# Project created by QtCreator 2011-07-31T10:54:43
#
#-------------------------------------------------

QT       -= core

QT       -= gui

TARGET = ../unix/ttf2lff
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app
DEFINES += VERSION="\"0.0.0.2\""


SOURCES += main.cpp

INCLUDEPATH += /usr/include/freetype2

# LIBS += -L/usr/lib64 -lfreetype

LIBS += -lfreetype

OBJECTS_DIR = ../intermediate/lff/obj

# Include any custom.pro files for personal/special builds
exists( ../custom.pro ):include( ../custom.pro )

