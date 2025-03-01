QT   += core gui svg widgets

TARGET = lc_svgicons
TEMPLATE = lib
CONFIG += plugin

DEFINES += QT_DEPRECATED_WARNINGS

#INCLUDEPATH += \
#INCLUDEPATH += src/


SOURCES += src/lc_svgiconengineplugin.cpp \
    src/lc_svgiconengine.cpp

HEADERS += \
    src/lc_svgiconengine.h

DISTFILES += src/lc_svgicons.json


win32:RC_FILE = src/lc_svgicons.rc

win32-msvc*: QMAKE_LFLAGS_RELEASE += /RELEASE

target.path += $$[QT_INSTALL_PLUGINS]/iconengines
INSTALLS += target

message("LC Icon engine build")
