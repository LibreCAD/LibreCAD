#-------------------------------------------------
#
# Project created by QtCreator 2011-03-22T19:33:11
#
#-------------------------------------------------

QT       -= core gui
TEMPLATE = lib

CONFIG += static warn_on

DESTDIR = ../../generated/lib

VERSION = 4.3

DLL_NAME = fparser
TARGET = $$DLL_NAME

GENERATED_DIR = ../../generated/lib/fparser
# Use common project definitions.
include(../../settings.pro)
include(../../common.pro)

SOURCES += \
    fparser.cc

HEADERS += \
    fparser.hh

