
prefix =	/usr/local
exec_prefix =	${prefix}
bindir =	${exec_prefix}/bin

OBJF =		main.o \
            test_creationclass.o
EXTERN_LIBS = -L../lib -ldxflib -L/usr/X11R6/lib -lX11
INCLUDES = -I../src

all: main

clean:
	@echo "Cleaning project..."
	@find . -name '*.o' -exec rm "{}" \;
	@if [ -f test ]; then rm -f test; fi

distclean: clean
	rm -f config.log config.cache Makefile config.status

main: $(OBJF)
	c++ -o test $(OBJF) $(EXTERN_LIBS)

%.o : %.cpp
	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) $(INCLUDES) -O3 -Wall $< -o $@

doc :
	@echo "Building documentation"
	@doxygen doxy.cfg
