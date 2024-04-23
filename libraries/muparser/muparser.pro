#-------------------------------------------------
#
# Project created by QtCreator 2011-03-22T19:33:11
#
#-------------------------------------------------

QT       -= core gui
TEMPLATE = lib

CONFIG += c++20 static warn_on

DESTDIR = ../../generated/lib

DLL_NAME = muparser
TARGET = $$DLL_NAME

GENERATED_DIR = ../../generated/lib/muparser
# Use common project definitions.
include(../../common.pri)

# svg support
QT -= svg

HEADERS += \
    include/muParserTokenReader.h \
    include/muParserToken.h \
    include/muParserFixes.h \
    include/muParserError.h \
    include/muParserDef.h \
    include/muParserInt.h \
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
    src/muParserInt.cpp \
    src/muParser.cpp

