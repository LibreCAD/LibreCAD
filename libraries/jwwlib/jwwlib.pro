#-------------------------------------------------
#
# Project created by QtCreator 2011-03-22T19:33:11
#
#-------------------------------------------------

QT       -= core gui
TEMPLATE = lib

CONFIG += static warn_on

DESTDIR = ../../generated/lib

VERSION = 0.0.1

DLL_NAME = jwwlib
TARGET = $$DLL_NAME

# Use common project definitions.
include(../../settings.pro)
include(../../common.pro)

INCLUDEPATH += \
    ../dxflib/src

SOURCES += \
    src/dl_jww.cpp \
    src/jwwdoc.cpp

HEADERS += \
    ../jwwlib/src/dl_jww.h \
    ../jwwlib/src/jwtype.h \
    ../jwwlib/src/jwwdoc.h


