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
    macx {   
	CONFIG += x86 x86_64
    }
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


exists( custom.pro ):include( custom.pro )
