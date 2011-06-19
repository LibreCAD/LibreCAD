#-------------------------------------------------
#
# Project created by QtCreator 2011-03-22T19:33:11
#
#-------------------------------------------------

TEMPLATE = subdirs
TARGET = plugins

QT += gui

SUBDIRS     = \
        asciifile \
        align \
        list \
        sameprop \
        sample


TRANSLATIONS =

# Store intermedia stuff somewhere else
OBJECTS_DIR = intermediate/obj
MOC_DIR = intermediate/moc
RCC_DIR = intermediate/rcc
TS_DIR = intermediate/ts
UI_DIR = intermediate/ui
UI_HERADERS_DIR = intermediate/ui
UI_SOURCES_DIR = intermediate/ui

# install
INSTALLDIR = ../unix/resources/plugins
win32 {
    INSTALLDIR = ../release/resources/plugins
}
unix {
    macx { 
	INSTALLDIR = ../LibreCAD.app/Contents/Resources/plugins
    }
    else { 
	INSTALLDIR = ../unix/resources/plugins
    }
}

docu.files = license.txt tetrapod.3dq data.3dq
docu.path = $$DOCUDIR

target.path = $$INSTALLDIR
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS plugins.pro README.plugins
sources.path = $$INSTALLDIR/tools/recad
INSTALLS += target sources

