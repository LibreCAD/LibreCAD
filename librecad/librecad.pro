#-------------------------------------------------
#
# Project created by QtCreator 2011-03-22T19:33:11
#
#-------------------------------------------------

TEMPLATE = subdirs

TARGET = librecad

# -- list tokenized sub-project dirs
SUBDIRS = \
          app \
          lib \
          lib2 \
          lib3

# -- where to find the sub-projects
lib.subdir = ../libraries/muparser
lib2.subdir = ../libraries/libdxfrw
lib3.subdir = ../libraries/jwwlib
app.subdir = src

# -- what project depends on others
app.depends = lib lib2 lib3
