
unix {
    CONFIG += link_pkgconfig
    PKGCONFIG += freetype2
}

win32 {
    # set this line to your freetype installation
    # download it from http://gnuwin32.sourceforge.net/packages/freetype.htm

    isEmpty( FREETYPE_DIR ) {
        FREETYPE_DIR = /Qt/freetype
    }

    exists($${FREETYPE_DIR}/include/ft2build.h) {
        INCLUDEPATH += "$${FREETYPE_DIR}/include" "$${FREETYPE_DIR}/include/freetype2"
        LIBS += -L"$${FREETYPE_DIR}/lib" -lfreetype

        message(ttf2lff using includes in $${FREETYPE_DIR}/include $${FREETYPE_DIR}/include/freetype2)
        message(ttf2lff using libs in $${FREETYPE_DIR}/lib)
    }
}

