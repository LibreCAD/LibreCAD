exists( custom.pro ):include( custom.pro )
exists( custom.pri ):include( custom.pri )

TARGET = python

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
    -v -c++ -python -w509 -w503 -w476 -DDEVELOPER

SWIG = swig

# Make translations at the end of the process
unix {
    QT_CONFIG -= no-pkg-config
    CONFIG += link_pkgconfig
    PKGCONFIG += python3-embed
}

win32 {
    SWIG = swig.exe
    SWIG_INCLUDE ~= s,/,\\,g
    SWIG_OUT_DIR ~= s,/,\\,g
    SWIG_OUT_PYTHON ~= s,/,\\,g

    PYTHON_PATH = C:\Program Files\Python313
    PYTHON_INCLUDE = $${PYTHON_PATH}\include
    PYTHON_LIB = $${PYTHON_PATH}\libs\python313.lib

    INCLUDEPATH += $$PYTHON_INCLUDE
    HEADERS += $$PYTHON_INCLUDE
    LIBS += $$PYTHON_LIB
    QMAKE_CXXFLAGS += $$PYTHON_LIB
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
