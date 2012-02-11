#-------------------------------------------------
#
# Project created by QtCreator 2011-03-22T19:33:11
#
#-------------------------------------------------

QT       -= core gui
TEMPLATE = lib

#CONFIG += dll \
CONFIG += static \
    warn_on

VERSION = 4.3

# Use common project definitions.
include(../common.pro)

DLL_NAME = fparser
TARGET = $$DLL_NAME

# DEFINES +=

win32 {
    COPY = copy /y
} else {
    COPY = cp
}
macx {
    CONFIG += x86 x86_64
}
SOURCES += \
    fparser.cc

HEADERS += \
    fparser.hh


DESTDIR = ../intermediate

# Store intermedia stuff somewhere else
OBJECTS_DIR = ../intermediate/obj
MOC_DIR = ../intermediate/moc
RCC_DIR = ../intermediate/rcc
TS_DIR = ../intermediate/ts
UI_DIR = ../intermediate/ui
UI_HERADERS_DIR = ../intermediate/ui
UI_SOURCES_DIR = ../intermediate/ui

