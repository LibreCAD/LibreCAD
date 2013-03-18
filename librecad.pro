TEMPLATE = subdirs
TARGET = librecad
CONFIG += ordered

exists( custom.pro ):include( custom.pro )
SUBDIRS     = \
	libraries \
	librecad \
	plugins \
	tools

