

BOOST_DIR = /opt/local/include
BOOST_LIBDIR = /opt/local/lib
BOOST_LIB =

INSTALLDIR = ../../

SUBDIRS += ttf2lff

!exists("$$(MUPARSER_DIR)"){
    MUPARSER_DIR = /opt/local
} else {
    MUPARSER_DIR = $$(MUPARSER_DIR)
}
