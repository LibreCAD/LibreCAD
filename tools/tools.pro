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
    exists( "$$(FREETYPE_DIR)" ) {		# Is it set in the environment?
        SUBDIRS = ttf2lff
        message( "FREETYPE_DIR is set in the environment, building ttf2lff")
    } else:!isEmpty( FREETYPE_DIR ) {		# Is it set in custom.pro?
        SUBDIRS = ttf2lff
        message( "FREETYPE_DIR is set in custom.pro, building ttf2lff")
    } else {
        message($${FREETYPE_DIR})
        message( "FREETYPE_DIR is not set, ignoring ttf2lff")
    }
}

