
unix {
    equals(build_muparser, "true")|!packagesExist(muparser){
        message("Using bundled muparser")
        LIBS += ../../generated/lib/libmuparser.a
        DEPENDPATH += ../../libraries/muparser/src
    }else{
        message("Using external muparser")
        CONFIG += link_pkgconfig
        PKGCONFIG += muparser
    }

}

win32 {

    isEmpty( MUPARSER_DIR) {
        # MUPARSER_DIR was not set in custom.pro
        MUPARSER_DIR = /muparser/muparser_v2_2_3
    }
    !exists("$$MUPARSER_DIR") {
        # check env for MUPARSER_DIR
        exists("$$(MUPARSER_DIR)"){
            MUPARSER_DIR = "$$(MUPARSER_DIR)"
        }
    }

    exists("$$MUPARSER_DIR"){
        INCLUDEPATH += "$${MUPARSER_DIR}/include"
        LIBS += -L"$${MUPARSER_DIR}/lib" -lmuparser
        message("Using muParser libraries from $${MUPARSER_DIR}.")
    } else {
        message("muParser was not found in $${MUPARSER_DIR}, please install muParser or check settings in custom.pro!")
    }

}

