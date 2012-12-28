#-------------------------------------------------
#
# Project created by QtCreator 2011-03-22T19:33:11
#
#-------------------------------------------------

TEMPLATE = subdirs

SUBDIRS     =

win32 {
        SUBDIRS += ttf2lff
}
unix {
    macx {
        SUBDIRS += ttf2lff
    } else {
        SUBDIRS += ttf2lff
    }
}




