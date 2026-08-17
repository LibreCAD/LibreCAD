
unix {
    # Honor an explicit FREETYPE_DIR (e.g. a minimal universal static Freetype
    # used for macOS universal builds); otherwise fall back to pkg-config.
    FT_DIR = $$FREETYPE_DIR
    isEmpty(FT_DIR): FT_DIR = $$(FREETYPE_DIR)
    !isEmpty(FT_DIR):exists($${FT_DIR}/include/freetype2/ft2build.h) {
        INCLUDEPATH += $${FT_DIR}/include/freetype2 $${FT_DIR}/include
        LIBS += -L$${FT_DIR}/lib -lfreetype
        message(ttf2lff using FREETYPE_DIR=$${FT_DIR})
    } else {
        CONFIG += link_pkgconfig
        PKGCONFIG += freetype2
    }
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
        LIBS += -L"$${FREETYPE_DIR}/lib" -lfreetype -lz

        message(ttf2lff using includes in $${FREETYPE_DIR}/include and $${FREETYPE_DIR}/include/freetype2)
        message(ttf2lff using libs in $${FREETYPE_DIR}/lib)
    } else {
        message("freetype was not found in $${FREETYPE_DIR}, please install freetype or check settings in custom.pro!")
    }
}

