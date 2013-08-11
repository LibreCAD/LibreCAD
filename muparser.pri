
unix {

    CONFIG += link_pkgconfig
    PKGCONFIG += muparser

}

win32 {

    !exists("$$(MUPARSER_DIR)"){
        MUPARSER_DIR = /muparser/muparser_v2_2_3
    } else {
        MUPARSER_DIR = $$(MUPARSER_DIR)
    }

    exists($${MUPARSER_DIR}){
        INCLUDEPATH += "$${MUPARSER_DIR}"/include
        LIBS += -L"$${MUPARSER_DIR}"/lib
        message("Using muParser libraries in $${MUPARSER_DIR}.")
    } else {
       message("muParser was not found, please install muParser!")
    }

}

