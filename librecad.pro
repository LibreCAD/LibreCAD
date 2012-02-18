TEMPLATE = subdirs
TARGET = librecad
CONFIG += ordered

SUBDIRS     = \
    libraries \
    librecad \
    plugins \
    tools

exists( custom.pro ):include( custom.pro )
