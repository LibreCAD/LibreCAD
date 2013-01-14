#-------------------------------------------------
#
# Project created by QtCreator 2011-03-22T19:33:11
#
#-------------------------------------------------

TEMPLATE = subdirs

SUBDIRS     =

win32 {
#set this line with your freetype installation to build ttf2lff
#download it from http://gnuwin32.sourceforge.net/packages/freetype.htm
    FREETYPE_DIR = /Qt/freetype
    exists($${FREETYPE_DIR}/include/ft2build.h){
        SUBDIRS += ttf2lff
        message(freetype found building ttf2lff tool.)
    }
}
unix {
    macx {
        SUBDIRS += ttf2lff
    } else {
        SUBDIRS += ttf2lff
    }
}




