
macx|win32|equals(build_muparser, "true")|!packagesExist(muparser){
    message("Using bundled muparser")
    LIBS += ../../generated/lib/libmuparser.a
    DEPENDPATH += ../../libraries/muparser/src
}else{
    message("Using external muparser")
    CONFIG += link_pkgconfig
    PKGCONFIG += muparser
}

