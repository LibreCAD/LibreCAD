#
#
# Makefile for fparser
#
#

#
# fparser installation directories
#
#LIBDIR = /usr/local/lib
#INCDIR = /usr/local/include/fparser
VERSION = 2.51

#LIBDIR = $(HOME)/local/lib
#INCDIR = $(HOME)/local/include/fparser

PREFIX ?= /usr

LIBDIR = $(DESTDIR)/$(PREFIX)/lib
INCDIR = $(DESTDIR)/$(PREFIX)/include/fparser

#
#------------------------ do not edit ------------------------
#
SHELL = /bin/sh

CC           = gcc
CXX          = gcc
CFLAGS       = -I./src -g -O2  -I/usr/X11/include -DPACKAGE_NAME=\"\" -DPACKAGE_TARNAME=\"\" -DPACKAGE_VERSION=\"\" -DPACKAGE_STRING=\"\" -DPACKAGE_BUGREPORT=\"\" -DUNIX=1 -DSTDC_HEADERS=1 
INSTALL      = /opt/local/bin/ginstall -c
INSTALL_DATA = ${INSTALL} -m 644
FIND         = find
MAKEDEPEND   = makedepend
AR           = ar
ARFLAGS      = -rs

BASE_DIR = ./src

BASE_SRC = \
		$(BASE_DIR)/fparser.cpp

BASE_INS = \
		$(BASE_DIR)/fparser.h

SRC = $(BASE_SRC)
OBJ = $(SRC:.cpp=.o)

INS =  $(BASE_INS)

.cpp.o:  
	$(CXX) $(CFLAGS) -c $< -o $@


all: prepare ./lib/libfparser.a

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

shared: ./lib/libfparser.so.$(VERSION)
	@echo

./lib/libfparser.a: $(OBJ) 
	test -d ./lib || mkdir ./lib
	$(AR) $(ARFLAGS) ./lib/libfparser.a $(OBJ)

./lib/libfparser.so.$(VERSION): $(OBJ)
	test -d ./lib || mkdir ./lib
	$(CC) -p -shared -o ./lib/libfparser.so.$(VERSION) $(OBJ) -lc -lm
	-(cd ./lib ; ln -s libfparser.so.$(VERSION) libfparser.so)

install: ./lib/libfparser.a
	test -d $(LIBDIR) || mkdir -p $(LIBDIR)
	test -d $(INCDIR) || mkdir -p $(INCDIR)
	$(INSTALL_DATA) ./lib/libfparser.a $(LIBDIR)
	#-(ln -s $(LIBDIR)/libfparser.a $(LIBDIR)/libfparser.a)
	for IFILE in $(INS); do \
		$(INSTALL_DATA) $$IFILE  $(INCDIR); \
	done

install-shared: ./lib/libfparser.so.$(VERSION)
	test -d $(LIBDIR) || mkdir -p $(LIBDIR)
	test -d $(INCDIR) || mkdir -p $(INCDIR)
	#$(INSTALL_DATA) ./lib/libfparser.so.$(VERSION) $(LIBDIR)
	-(ln -s $(LIBDIR)/libfparser.so.$(VERSION) $(LIBDIR)/libfparser.so)
	for IFILE in $(INS); do \
		$(INSTALL_DATA) $$IFILE  $(INCDIR); \
	done

clean:
	-rm -f $(OBJ)
	-rm -f ./lib/lib* Makefile.bak
	-rm -f ./include/fparser/*.h
	-$(FIND) . -name "*~" -exec rm -f {} \;
	-rm -f ./lib/libfparser.$(VERSION).a
  
distclean: clean
	-rm -f Makefile configure

depend:
	$(MAKEDEPEND) -- $(CFLAGS) -- $(SRC)
	@echo
	@echo "Run 'make' to build fparser library."
	@echo

# DO NOT DELETE THIS LINE -- make  depend  depends  on it.
