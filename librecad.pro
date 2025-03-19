TEMPLATE = subdirs
TARGET = librecad
CONFIG += ordered
SUBDIRS     = \
    libraries \
    librecad \
    plugins \
    tools

# c++17 is now obligatory for LibreCAD
message(We will be using CPP17 features)
QMAKE_CXXFLAGS += -O2 -std=c++17
exists(custom.pro):include( custom.pro )

OTHER_FILES = \
    CHANGELOG.md \
    README.md \
    desktop/librecad.desktop \
    desktop/org.librecad.librecad.appdata.xml
