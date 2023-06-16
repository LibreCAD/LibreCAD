TEMPLATE = subdirs
TARGET = librecad
CONFIG += ordered

SUBDIRS     = \
    libraries \
    librecad \
    plugins \
    tools

# c++14 is now obligatory for LibreCAD
message(We will be using CPP14 features)

exists(custom.pro):include( custom.pro )

OTHER_FILES = \
    CHANGELOG.md \
    README.md \
    desktop/librecad.desktop \
    desktop/org.librecad.librecad.appdata.xml

