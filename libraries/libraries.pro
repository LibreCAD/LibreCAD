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

equals(build_muparser, "true"){
        message("build muparser from source")
        SUBDIRS += muparser
}else{
        message("Using external muparser lib")
}


