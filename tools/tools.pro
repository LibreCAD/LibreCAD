#-------------------------------------------------
#
# Project created by QtCreator 2011-03-22T19:33:11
#
#-------------------------------------------------

exists( ../custom.pro ):include( ../custom.pro )

TEMPLATE = subdirs

unix {
    SUBDIRS = ttf2lff
}

win32 {
    !isEmpty( FREETYPE_DIR ) {
        SUBDIRS = ttf2lff
        message( "FREETYPE_DIR is set, building ttf2lff")
    }
    else {
        message( "FREETYPE_DIR is not set, ignoring ttf2lff")
    }
}

