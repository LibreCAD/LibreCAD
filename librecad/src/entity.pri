TARGET = entity

GENERATED_SOURCES += \
    $${TARGET}_wrap.cxx

SWIG_INPUT += \
   $${TARGET}.i

SWIG_INCLUDE += \
    -I$$[QT_INSTALL_HEADERS]

SWIG_OUT_DIR += \
    $${INSTALLDIR}

SWIG_OUT_PYTHON += \
    $${INSTALLDIR}/librecad.py

SWIG_FLAGS += \
    -v -c++ -python -cpperraswarn -std=c++17 -w314

SWIG = swig

win32 {
    SWIG = swig.exe
    SWIG_INCLUDE ~= s,/,\\,g
    SWIG_OUT_DIR ~= s,/,\\,g
    SWIG_OUT_PYTHON ~= s,/,\\,g
}

swig.target = $$GENERATED_SOURCES
swig.input = SWIG_INPUT
swig.variable_out = $$GENERATED_SOURCES
swig.commands = $$SWIG $$SWIG_FLAGS $$SWIG_INCLUDE -outdir $$SWIG_OUT_DIR -o $$GENERATED_SOURCES ${QMAKE_FILE_NAME}
swig.output = $$GENERATED_SOURCES

QMAKE_EXTRA_COMPILERS += swig
QMAKE_EXTRA_TARGETS += swig
QMAKE_CLEAN += $$swig.target
QMAKE_CLEAN += $$SWIG_OUT_PYTHON

