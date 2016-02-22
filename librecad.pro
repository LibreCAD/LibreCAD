TEMPLATE = subdirs
TARGET = librecad
CONFIG += ordered

SUBDIRS     = \
    libraries \
    librecad \
    plugins \
    tools

# c++11 is now obligatory for LibreCAD
message(We will be using CPP11 features)

exists( custom.pro ):include( custom.pro )
