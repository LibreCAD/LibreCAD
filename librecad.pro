#-------------------------------------------------
#
# Project created by QtCreator 2011-03-22T19:33:11
#
#-------------------------------------------------

TEMPLATE = subdirs
TARGET = librecad

CONFIG += ordered
#QT += gui

SUBDIRS     = \
        libdxfrw \
        dxflib \
        jwwlib \
        fparser \
        src \
        plugins

unix {
    macx {   }
    else {
        SUBDIRS += attf2lff
    }
}

TRANSLATIONS =

# install
win32 {
    INSTALLDIR = release
}
unix {
    macx { 
        INSTALLDIR = LibreCAD.app/Contents
    }
    else { 
#        INSTALLDIR = unix
    }
}

