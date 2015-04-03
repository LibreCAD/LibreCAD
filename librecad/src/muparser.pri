
macx|win32|equals(build_muparser, "true")|!packagesExist(muparser){
    message("Using bundled muparser")
    LIBS += ../../generated/lib/libmuparser.a
	MUPARSER_DIR = ../../libraries/muparser
	DEPENDPATH += $$MUPARSER_DIR/include \
				$$MUPARSER_DIR/src
	INCLUDEPATH += $$MUPARSER_DIR/include
}else{
    message("Using external muparser")
    CONFIG += link_pkgconfig
    PKGCONFIG += muparser
}

