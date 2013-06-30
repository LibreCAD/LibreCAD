
!exists("$$(BOOST_DIR)"){
    BOOST_DIR = /boost/boost_1_53_0
    BOOST_LIBDIR = /boost/boost_1_53_0
} else {
    BOOST_DIR = $$(BOOST_DIR)
    BOOST_LIBDIR = $$(BOOST_DIR)
}
BOOST_LIBS =

!exists("$$(MUPARSER_DIR)"){
    MUPARSER_DIR = /muparser/muparser_v2_2_3
} else {
    MUPARSER_DIR = $$(MUPARSER_DIR)
}

INSTALLDIR = ../../windows


