#
#
# Makefile for dxflib
#
#
VERSION = 2.0.4.8

#
# dxflib installation directories
#
LIBDIR = /usr/local/lib
INCDIR = /usr/local/include/dxflib

#PREFIX ?= /usr

#LIBDIR = $(DESTDIR)$(PREFIX)/lib
#INCDIR = $(DESTDIR)$(PREFIX)/include/dxflib


#LIBDIR = $(HOME)/local/lib
#INCDIR = $(HOME)/local/include/dxflib

#
#------------------------ do not edit ------------------------
#
SHELL = /bin/sh

CC           = gcc
CXX          = gcc
CFLAGS       = -I./src -g -O2  -I/usr/X11/include -DPACKAGE_NAME=\"\" -DPACKAGE_TARNAME=\"\" -DPACKAGE_VERSION=\"\" -DPACKAGE_STRING=\"\" -DPACKAGE_BUGREPORT=\"\" -DUNIX=1 -DSTDC_HEADERS=1 -DHAVE_SYS_TYPES_H=1 -DHAVE_SYS_STAT_H=1 -DHAVE_STDLIB_H=1 -DHAVE_STRING_H=1 -DHAVE_MEMORY_H=1 -DHAVE_STRINGS_H=1 -DHAVE_INTTYPES_H=1 -DHAVE_STDINT_H=1 -DHAVE_UNISTD_H=1 -DHAVE_LIMITS_H=1 
INSTALL      = /opt/local/bin/ginstall -c
INSTALL_DATA = ${INSTALL} -m 644
FIND         = find
MAKEDEPEND   = makedepend
AR           = ar
ARFLAGS      = -rs

BASE_DIR = ./src

BASE_SRC = \
		$(BASE_DIR)/dl_dxf.cpp \
		$(BASE_DIR)/dl_writer_ascii.cpp

BASE_INS = \
		$(BASE_DIR)/dl_attributes.h \
		$(BASE_DIR)/dl_codes.h \
		$(BASE_DIR)/dl_creationadapter.h \
		$(BASE_DIR)/dl_creationinterface.h \
		$(BASE_DIR)/dl_dxf.h \
		$(BASE_DIR)/dl_entities.h \
		$(BASE_DIR)/dl_exception.h \
		$(BASE_DIR)/dl_extrusion.h \
		$(BASE_DIR)/dl_writer.h \
		$(BASE_DIR)/dl_writer_ascii.h

SRC = $(BASE_SRC)
OBJ = $(SRC:.cpp=.o)

INS =  $(BASE_INS)

.cpp.o:  
	$(CXX) $(CFLAGS) -c $< -o $@


all: prepare ./lib/libdxf.a

prepare:
	test -d ./include || mkdir -p ./include
	-( cd ./include; rm -f *.h; \
	for hf in `find ../src -name '*.h'`; do \
		if [ "x$$OS" = "xWindows_NT" ]; then \
			cp "$$hf" .; \
		else \
			ln -s "$$hf" 2> /dev/null; \
		fi \
	done )

shared: ./lib/libdxf.so.$(VERSION)
	@echo

./lib/libdxf.a: $(OBJ) 
	test -d ./lib || mkdir ./lib
	$(AR) $(ARFLAGS) ./lib/libdxf.a $(OBJ)

./lib/libdxf.so.$(VERSION): $(OBJ)
	test -d ./lib || mkdir ./lib
	$(CC) -p -shared -o ./lib/libdxf.so.$(VERSION) $(OBJ) -lc -lm
	-(cd ./lib ; ln -s libdxf.so.$(VERSION) libdxf.so)

install: ./lib/libdxf.a
	test -d $(LIBDIR) || mkdir -p $(LIBDIR)
	test -d $(INCDIR) || mkdir -p $(INCDIR)
	$(INSTALL_DATA) ./lib/libdxf.a $(LIBDIR)
	#-(ln -s $(LIBDIR)/libdxf.a $(LIBDIR)/libdxf.a)
	for IFILE in $(INS); do \
		$(INSTALL_DATA) $$IFILE  $(INCDIR); \
	done

install-shared: ./lib/libdxf.so.$(VERSION)
	test -d $(LIBDIR) || mkdir -p $(LIBDIR)
	test -d $(INCDIR) || mkdir -p $(INCDIR)
	#$(INSTALL_DATA) ./lib/libdxf.so.$(VERSION) $(LIBDIR)
	-(ln -s $(LIBDIR)/libdxf.so.$(VERSION) $(LIBDIR)/libdxf.so)
	for IFILE in $(INS); do \
		$(INSTALL_DATA) $$IFILE  $(INCDIR); \
	done

clean:
	-(cd ./test ; make clean)
	-rm -f $(OBJ)
	-rm -f ./lib/lib* Makefile.bak
	-rm -f ./include/dxflib/*.h
	-$(FIND) . -name "*~" -exec rm -f {} \;
	-rm -f ./lib/libdxf.a
  
distclean: clean
	-rm -f Makefile configure

testing:	./lib/libdxf.a
	(cd ./test ; make)

docu:
	-(doxygen ./doxygen.cfg)

dist:	clean docu
	#-rm -r ./doc/classref/html/en/*
	-rm -r ./distribution/dxflib/*
	test -d ./distribution/dxflib || mkdir -p ./distribution/dxflib
	mkdir -p ./distribution/dxflib/doc
	-(cp -r ./src ./test ./distribution/dxflib/)
	-(cp -r ./doc/classref ./distribution/dxflib/doc/)
	-(cp * ./distribution/dxflib/)
	-(cd ./distribution/dxflib; rm config.log config.cache reinstall*; mkdir lib include)
	-(cd ./distribution; tar fvcz dxflib-$(VERSION).tar.gz dxflib)

depend:
	$(MAKEDEPEND) -- $(CFLAGS) -- $(SRC)
	@echo
	@echo "Run 'make' to build dxflib library."
	@echo

# DO NOT DELETE THIS LINE -- make  depend  depends  on it.
