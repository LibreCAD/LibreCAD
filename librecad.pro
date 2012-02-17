TEMPLATE = subdirs
TARGET = librecad
CONFIG += ordered

SUBDIRS     = \
    libraries \
    librecad
#   plugins

exists( custom.pro ):include( custom.pro )
