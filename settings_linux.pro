

SUBDIRS += ttf2lff
INSTALLDIR = ../../unix


#
# add ubuntu muparser include dir
#

exists("/usr/include/muParser") {
   INCLUDEPATH += "/usr/include/muParser"
}
