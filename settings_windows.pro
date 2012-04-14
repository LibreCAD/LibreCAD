

BOOST_DIR = /boost/boost_1_48_0
BOOST_LIBDIR = /boost/boost_1_48_0
BOOST_LIBS =

!exists("$${MUPARSER_DIR}"){
    MUPARSER_DIR = /muparser/muparser_v2_2_2
    MUPARSER_LIBDIR = "$${MUPARSER_DIR}"/lib
    MUPARSER_LIBS =
}

INSTALLDIR = ../../windows


