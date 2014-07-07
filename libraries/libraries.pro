#-------------------------------------------------
#
# Project created by QtCreator 2011-03-22T19:33:11
#
#-------------------------------------------------

TEMPLATE = subdirs

TARGET = libraries

SUBDIRS     = \
        libdxfrw \
        jwwlib

macx|win32|equals(build_muparser, "true")|!packagesExist(muparser){
        message("Using bundled muparser lib")
        SUBDIRS += muparser
}else{
        message("Using external muparser lib")
}


