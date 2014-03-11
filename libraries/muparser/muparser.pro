#-------------------------------------------------
#
# Project created by QtCreator 2011-03-22T19:33:11
#
#-------------------------------------------------

QT       -= core gui
TEMPLATE = lib

CONFIG += static warn_on

DESTDIR = ../../generated/lib

DLL_NAME = muparser
TARGET = $$DLL_NAME

GENERATED_DIR = ../../generated/lib/muparser
# Use common project definitions.
include(../../common.pri)
QMAKE_CXXFLAGS_DEBUG += -std=c++0x -std=c++11
QMAKE_CXXFLAGS += -std=c++0x -std=c++11

# svg support
QT -= svg

HEADERS += \
    src/muParserTokenReader.h \
    src/muParserToken.h \
    src/muParserStack.h \
    src/muParserFixes.h \
    src/muParserError.h \
    src/muParserDef.h \
    src/muParserCallback.h \
    src/muParserBytecode.h \
    src/muParserBase.h \
    src/muParser.h \
    src/muParserTemplateMagic.h

SOURCES += \
    src/muParserTokenReader.cpp \
    src/muParserError.cpp \
    src/muParserCallback.cpp \
    src/muParserBytecode.cpp \
    src/muParserBase.cpp \
    src/muParser.cpp
