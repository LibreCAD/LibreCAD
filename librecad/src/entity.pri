TARGET = entity

GENERATED_SOURCES += \
    $${TARGET}_wrap.cxx

SWIG_INPUT += \
    entity.i

SWIG_USE_FILE += \
    -I$$[QT_INSTALL_LIBS]/qt6

SWIG_FLAGS += \
    -v -c++ -python

SWIG_INCLUDE += \
    -I${QMAKE_INCDIR}

SWIG += \
    /usr/bin/swig

swig.target = $${TARGET}_wrap.cxx
swig.input = SWIG_INPUT
swig.variable_out = $$GENERATED_SOURCES
swig.commands = $$SWIG $$SWIG_FLAGS $$SWIG_USE_FILE  -outdir $${INSTALLDIR} -o ${QMAKE_FILE_OUT} ${QMAKE_FILE_NAME}
swig.output = $${TARGET}_wrap.cxx

QMAKE_EXTRA_COMPILERS += swig
QMAKE_EXTRA_TARGETS += swig
QMAKE_CLEAN += $$swig.target
QMAKE_CLEAN += librecad.py
QMAKE_CLEAN += $$GENERATED_SOURCES
