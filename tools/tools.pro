#-------------------------------------------------
#
# Project created by QtCreator 2011-03-22T19:33:11
#
#-------------------------------------------------

TEMPLATE = subdirs

SUBDIRS     =

win32 {
    # ttf2lff needs  to be download, instructions follow
}
unix {
    macx {
        SUBDIRS += ttf2lff
    } else {
        SUBDIRS += ttf2lff
    }
}




