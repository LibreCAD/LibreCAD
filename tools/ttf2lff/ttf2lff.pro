#-------------------------------------------------
#
# Project created by QtCreator 2011-07-31T10:54:43
#
#-------------------------------------------------

QT       -= core
QT       -= gui

CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app
DEFINES += VERSION="\"0.0.0.2\""

# Use common project definitions.
include(../../settings.pro)
include(../../common.pro)

SOURCES += main.cpp


unix {
    macx {
        TARGET=../../LibreCAD.app/Contents/MacOS/ttf2lff
    } else {
        TARGET=../../unix/ttf2lff
    }
}
win32 {
        TARGET=../../windows/ttf2lff
}

unix {
    macx {
        HAS_SDK=none
        exists (/usr/X11/include/ft2build.h) {HAS_SDK=X11}
        exists (/Developer/SDKs/MacOSX10.6.sdk/usr/X11/include/*) {HAS_SDK=10.6}
        exists (/Developer/SDKs/MacOSX10.7.sdk/usr/X11/include/*) {HAS_SDK=10.7}
        contains (HAS_SDK, none) {error(Freetype headers not found)}

        contains (HAS_SDK , 10.6) {
            INCLUDEPATH += /Developer/SDKs/MacOSX10.6.sdk/usr/X11/include/
            INCLUDEPATH += /Developer/SDKs/MacOSX10.6.sdk/usr/X11/include/freetype2
            LIBS+= -L/Developer/SDKs/MacOSX10.6.sdk/usr/X11/lib/
        }

        contains (HAS_SDK , 10.7) {
            INCLUDEPATH += /Developer/SDKs/MacOSX10.7.sdk/usr/X11/include/
            INCLUDEPATH += /Developer/SDKs/MacOSX10.7.sdk/usr/X11/include/freetype2
            LIBS+= -L/Developer/SDKs/MacOSX10.7.sdk/usr/X11/lib/
        }

        contains (HAS_SDK , X11) {
                CONFIG += x86
                INCLUDEPATH += /usr/X11/include/
                INCLUDEPATH += /usr/X11/include/freetype2
                LIBS+= -L/usr/X11/lib/
        }

    } else {
        INCLUDEPATH += /usr/include/freetype2
    }
    message(ttf2lff using libraries in $${LIBS}.)
}

LIBS += -lfreetype

