QT   += core gui svg widgets

include(../../settings.pri)

macx|unix {
    # qmake can accidentally pick up ignored MOC files left by an in-source
    # Qt build. Remove them before dependency scanning so the generated build
    # directory MOC is used for the active Qt version.
    system(rm -f "$$PWD/lc_svgiconengineplugin.moc" "$$PWD/moc_predefs.h")
}

INCLUDEPATH = $$OUT_PWD $$INCLUDEPATH
macx|unix:QMAKE_CXXFLAGS += -iquote $$OUT_PWD

TARGET = lc_svgicons
TEMPLATE = lib
CONFIG += plugin no_include_pwd

DEFINES += QT_DEPRECATED_WARNINGS

#INCLUDEPATH += \
#INCLUDEPATH += src/


SOURCES += src/lc_svgiconengineplugin.cpp \
    src/lc_svgiconengine.cpp

HEADERS += \
    src/lc_svgiconengine.h
    src/lc_svgiconengineplugin.h

DISTFILES += src/lc_svgicons.json


win32:RC_FILE = src/lc_svgicons.rc

# Without this, the plugin builds to qmake's default output location and
# never reaches windows\iconengines\ - windeployqt only knows about Qt's own
# plugins (e.g. qsvgicon.dll, which does land there), not this custom one,
# so nothing else copies it into the deployed tree or the NSIS installer.
# Matches the DESTDIR convention every other plugin .pro already uses
# (see plugins/*/*.pro), just pointed at Qt's iconengines plugin category
# instead of LibreCAD's own resources/plugins.
win32: DESTDIR = ../../windows/iconengines

win32-msvc*: QMAKE_LFLAGS_RELEASE += /RELEASE

target.path += $$[QT_INSTALL_PLUGINS]/iconengines
INSTALLS += target

message("LC Icon engine build")
