#-------------------------------------------------
#
# Project created by QtCreator 2011-03-22T19:33:11
#
#-------------------------------------------------

QT       -= core gui
TEMPLATE = lib

CONFIG += c++11 static warn_on

DESTDIR = ../../generated/lib

DLL_NAME = muparser
TARGET = $$DLL_NAME

GENERATED_DIR = ../../generated/lib/muparser
# Use common project definitions.
include(../../common.pri)
QMAKE_CXXFLAGS_DEBUG += -std=c++11
QMAKE_CXXFLAGS += -std=c++11

# svg support
QT -= svg

HEADERS += \
    include/muParserTokenReader.h \
    include/muParserToken.h \
    include/muParserStack.h \
    include/muParserFixes.h \
    include/muParserError.h \
    include/muParserDef.h \
    include/muParserCallback.h \
    include/muParserBytecode.h \
    include/muParserBase.h \
    include/muParser.h \
    include/muParserTemplateMagic.h

DEPENDPATH += include
INCLUDEPATH += $$DEPENDPATH

SOURCES += \
    src/muParserTokenReader.cpp \
    src/muParserError.cpp \
    src/muParserCallback.cpp \
    src/muParserBytecode.cpp \
    src/muParserBase.cpp \
    src/muParser.cpp
