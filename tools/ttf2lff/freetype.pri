
unix {
    CONFIG += link_pkgconfig
    PKGCONFIG += freetype2
}

win32 {
    # set this line to your freetype installation
    # download it from http://gnuwin32.sourceforge.net/packages/freetype.htm

    isEmpty( FREETYPE_DIR ) {
        # FREETYPE_DIR was not set in custom.pro
        FREETYPE_DIR = /Qt/freetype
    }
    !exists("$$FREETYPE_DIR") {
        # check env for FREETYPE_DIR
        exists("$$(FREETYPE_DIR)"){
            FREETYPE_DIR = "$$(FREETYPE_DIR)"
        }
    }

    exists($${FREETYPE_DIR}/include/ft2build.h) {
        INCLUDEPATH += "$${FREETYPE_DIR}/include" "$${FREETYPE_DIR}/include/freetype2"
        LIBS += -L"$${FREETYPE_DIR}/lib" -lfreetype

        message(ttf2lff using includes in $${FREETYPE_DIR}/include and $${FREETYPE_DIR}/include/freetype2)
        message(ttf2lff using libs in $${FREETYPE_DIR}/lib)
    } else {
        message("freetype was not found in $${FREETYPE_DIR}, please install freetype or check settings in custom.pro!")
    }
}

