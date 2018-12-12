
macx|win32|equals(build_muparser, "true")|!packagesExist(muparser){
    message("Using bundled muparser")
	MUPARSER_DIR = ../../libraries/muparser
	DEPENDPATH += $$MUPARSER_DIR/include \
				$$MUPARSER_DIR/src
	INCLUDEPATH += $$MUPARSER_DIR/include
	GEN_LIB_DIR = ../../generated/lib
	LIBS += -L$$GEN_LIB_DIR -lmuparser
	PRE_TARGETDEPS += $$GEN_LIB_DIR/libmuparser.a
}else{
    message("Using external muparser")
    CONFIG += link_pkgconfig
    PKGCONFIG += muparser
}

