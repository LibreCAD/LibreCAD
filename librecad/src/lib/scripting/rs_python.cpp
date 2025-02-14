/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
**
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software
** Foundation and appearing in the file gpl-2.0.txt included in the
** packaging of this file.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
**
** This copyright notice MUST APPEAR in all copies of the script!
**
**********************************************************************/

#ifdef RS_OPT_PYTHON
#define PYBIND11_NO_KEYWORDS
#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include "rs_python.h"

#include <QtGlobal>
#include <QJsonArray>
#include <QDebug>
#include <QByteArray>
#include <QMessageBox>
#include <QApplication>
#include <cassert>

QG_Py_CommandEdit *Py_CommandEdit = nullptr;

/**
 *
 * Python module emb
 *
 * (stdout, stderr buffer write)
 * 'emb' Based on an idea from:
 * Copyright (C) 2011 Mateusz Loskot <mateusz@loskot.net>
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http:*www.boost.org/LICENSE_1_0.txt)
 * Blog article: http:*mateusz.loskot.net/?p=2819
 *
 * fixed, ported to Py 3.12 and extended by:
 * Emanuel Strobel
 *
 */

namespace emb
{

typedef std::function<void(std::string)> stdout_write_type;
typedef std::function<void(std::string)> stderr_write_type;

struct Stdout
{
    PyObject_HEAD
        stdout_write_type write;
};

struct Stderr
{
    PyObject_HEAD
        stderr_write_type write;
};

PyObject* Stdout_write(PyObject* self, PyObject* args)
{
    std::size_t written(0);
    Stdout* selfimpl = reinterpret_cast<Stdout*>(self);
    if (selfimpl->write)
    {
        char* data;
        if (!PyArg_ParseTuple(args, "s", &data))
            return 0;

        std::string str(data);
        selfimpl->write(str);
        written = str.size();
    }
    return PyLong_FromSize_t(written);
}

PyObject* Stderr_write(PyObject* self, PyObject* args)
{
    std::size_t written(0);
    Stderr* selfimpl = reinterpret_cast<Stderr*>(self);
    if (selfimpl->write)
    {
        char* data;
        if (!PyArg_ParseTuple(args, "s", &data))
            return 0;

        std::string str(data);
        selfimpl->write(str);
        written = str.size();
    }
    return PyLong_FromSize_t(written);
}


PyObject* Stdout_flush(PyObject* self, PyObject* args)
{
    Q_UNUSED(self);
    Q_UNUSED(args);
    // no-op
    return Py_BuildValue("");
}

PyObject* Stderr_flush(PyObject* self, PyObject* args)
{
    Q_UNUSED(self);
    Q_UNUSED(args);
    // no-op
    return Py_BuildValue("");
}

PyMethodDef Stdout_methods[] =
    {
        {"write", Stdout_write, METH_VARARGS, "sys.stdout.write"},
        {"flush", Stdout_flush, METH_VARARGS, "sys.stdout.write"},
        {0, 0, 0, 0} // sentinel
};

PyMethodDef Stderr_methods[] =
    {
        {"write", Stderr_write, METH_VARARGS, "sys.stderr.write"},
        {"flush", Stderr_flush, METH_VARARGS, "sys.stderr.write"},
        {0, 0, 0, 0} // sentinel
};

PyTypeObject StdoutType = {
    PyVarObject_HEAD_INIT(0, 0)
    "emb.StdoutType",     /* tp_name */
    sizeof(Stdout),       /* tp_basicsize */
    0,                    /* tp_itemsize */
    0,                    /* tp_dealloc */
    0,                    /* tp_print */
    0,                    /* tp_getattr */
    0,                    /* tp_setattr */
    0,                    /* tp_reserved */
    0,                    /* tp_repr */
    0,                    /* tp_as_number */
    0,                    /* tp_as_sequence */
    0,                    /* tp_as_mapping */
    0,                    /* tp_hash  */
    0,                    /* tp_call */
    0,                    /* tp_str */
    0,                    /* tp_getattro */
    0,                    /* tp_setattro */
    0,                    /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,   /* tp_flags */
    "emb.Stdout objects", /* tp_doc */
    0,                    /* tp_traverse */
    0,                    /* tp_clear */
    0,                    /* tp_richcompare */
    0,                    /* tp_weaklistoffset */
    0,                    /* tp_iter */
    0,                    /* tp_iternext */
    Stdout_methods,       /* tp_methods */
    0,                    /* tp_members */
    0,                    /* tp_getset */
    0,                    /* tp_base */
    0,                    /* tp_dict */
    0,                    /* tp_descr_get */
    0,                    /* tp_descr_set */
    0,                    /* tp_dictoffset */
    0,                    /* tp_init */
    0,                    /* tp_alloc */
    0,                    /* tp_new */
    0,                    /* tp_free */
    0,                    /* tp_is_gc */
    0,                    /* tp_bases */
    0,                    /* tp_mro */
    0,                    /* tp_cache */
    0,                    /* tp_subclasses */
    0,                    /* tp_weaklist */
    0,                    /* tp_del */
    0,                    /* tp_version_tag */
    0,                    /* tp_finalize */
    0,                    /* tp_vectorcall */
#if PY_MINOR_VERSION > 11
    0,                    /* tp_watched */
#endif
};

PyTypeObject StderrType = {
    PyVarObject_HEAD_INIT(0, 0)
    "emb.StderrType",     /* tp_name */
    sizeof(Stderr),       /* tp_basicsize */
    0,                    /* tp_itemsize */
    0,                    /* tp_dealloc */
    0,                    /* tp_print */
    0,                    /* tp_getattr */
    0,                    /* tp_setattr */
    0,                    /* tp_reserved */
    0,                    /* tp_repr */
    0,                    /* tp_as_number */
    0,                    /* tp_as_sequence */
    0,                    /* tp_as_mapping */
    0,                    /* tp_hash  */
    0,                    /* tp_call */
    0,                    /* tp_str */
    0,                    /* tp_getattro */
    0,                    /* tp_setattro */
    0,                    /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,   /* tp_flags */
    "emb.Stderr objects", /* tp_doc */
    0,                    /* tp_traverse */
    0,                    /* tp_clear */
    0,                    /* tp_richcompare */
    0,                    /* tp_weaklistoffset */
    0,                    /* tp_iter */
    0,                    /* tp_iternext */
    Stderr_methods,       /* tp_methods */
    0,                    /* tp_members */
    0,                    /* tp_getset */
    0,                    /* tp_base */
    0,                    /* tp_dict */
    0,                    /* tp_descr_get */
    0,                    /* tp_descr_set */
    0,                    /* tp_dictoffset */
    0,                    /* tp_init */
    0,                    /* tp_alloc */
    0,                    /* tp_new */
    0,                    /* tp_free */
    0,                    /* tp_is_gc */
    0,                    /* tp_bases */
    0,                    /* tp_mro */
    0,                    /* tp_cache */
    0,                    /* tp_subclasses */
    0,                    /* tp_weaklist */
    0,                    /* tp_del */
    0,                    /* tp_version_tag */
    0,                    /* tp_finalize */
    0,                    /* tp_vectorcall */
#if PY_MINOR_VERSION > 11
    0,                    /* tp_watched */
#endif
};

PyModuleDef embmodule = {
    PyModuleDef_HEAD_INIT,
    "emb", NULL, -1, NULL , NULL, NULL, NULL, NULL
};

// Internal state
PyObject* g_stdout;
PyObject* g_stdout_saved;

PyObject* g_stderr;
PyObject* g_stderr_saved;

/**
 * extern 'C' Python module emb::emb
 */
PyMODINIT_FUNC PyInit_emb(void)
{
    g_stdout = 0;
    g_stdout_saved = 0;

    g_stderr = 0;
    g_stderr_saved = 0;

    StdoutType.tp_new = PyType_GenericNew;
    if (PyType_Ready(&StdoutType) < 0)
        return 0;

    StderrType.tp_new = PyType_GenericNew;
    if (PyType_Ready(&StderrType) < 0)
        return 0;

    PyObject* m = PyModule_Create(&embmodule);
    if (m)
    {
        Py_INCREF(&StdoutType);
        PyModule_AddObject(m, "Stdout", reinterpret_cast<PyObject*>(&StdoutType));

        Py_INCREF(&StderrType);
        PyModule_AddObject(m, "Stderr", reinterpret_cast<PyObject*>(&StderrType));
    }
    return m;
}

void set_stdout(stdout_write_type write)
{
    if (!g_stdout)
    {
        g_stdout_saved = PySys_GetObject("stdout"); // borrowed
        g_stdout = StdoutType.tp_new(&StdoutType, 0, 0);
    }

    Stdout* impl = reinterpret_cast<Stdout*>(g_stdout);
    impl->write = write;
    PySys_SetObject("stdout", g_stdout);
}

void set_stderr(stderr_write_type write)
{
    if (!g_stderr)
    {
        g_stderr_saved = PySys_GetObject("stderr"); // borrowed
        g_stderr = StderrType.tp_new(&StderrType, 0, 0);
    }

    Stderr* impl = reinterpret_cast<Stderr*>(g_stderr);
    impl->write = write;
    PySys_SetObject("stderr", g_stderr);
}

void reset_stdout()
{
    if (g_stdout_saved)
        PySys_SetObject("stdout", g_stdout_saved);

    Py_XDECREF(g_stdout);
    g_stdout = 0;
}

void reset_stderr()
{
    if (g_stderr_saved)
        PySys_SetObject("stderr", g_stderr_saved);

    Py_XDECREF(g_stderr);
    g_stderr = 0;
}

} // end namespace emb


/**
 * extern 'C' Python module librecad
 */
PyMODINIT_FUNC PyInit__librecad(void);

/**
 * static instance to class RS_Python
 */
RS_Python* RS_Python::unique = nullptr;

RS_Python* RS_Python::instance() {
    qInfo() << "[RS_Python] RS_Python::instance requested";
    if (unique == nullptr) {
        unique = new RS_Python();
    }
    return unique;
}

/**
 * class RS_Python (interpreter)
 */
RS_Python::RS_Python()
    //: m_main(initModule())
{
    qputenv("PYTHONPATH", QByteArray("."));

    PyImport_AppendInittab("_librecad", PyInit__librecad);
    PyImport_AppendInittab("emb", emb::PyInit_emb);

    Py_Initialize();

    addSysPath(".");

    PyImport_ImportModule("emb");
    PyImport_ImportModule("_librecad");
    m_pGlobalMain = PyImport_AddModule("__main__");
    PyModule_AddStringConstant(globalMain(), "__file__", "");
    m_pGlobalDict = PyModule_GetDict(globalMain());
    Py_INCREF(m_pGlobalDict);

    qInfo() << qUtf8Printable(Py_GetVersionString());
}

RS_Python::~RS_Python()
{
    qInfo() << "[RS_Python::~RS_Python] Py_FinalizeEx";
    Py_XDECREF(m_pGlobalDict);
    Py_FinalizeEx();
}

int RS_Python::addSysPath(const QString& path)
{
    // ADD ME TO runFile
    PyObject *pSysModule = PyImport_ImportModule("sys");
    if (!pSysModule) {
        PyErr_Print();
        PyErr_Clear();
        return -1;
    }

    PyObject *pPyPath = PyObject_GetAttrString(pSysModule, "path");
    if (!pPyPath) {
        PyErr_Print();
        PyErr_Clear();
        Py_XDECREF(pSysModule);
        return -1;
    }
    Py_XDECREF(pSysModule);

    PyObject *pFunc = PyObject_GetAttrString(pPyPath, "append");
    if (!pFunc) {
        PyErr_Print();
        PyErr_Clear();
        Py_XDECREF(pPyPath);
        return -1;
    }
    Py_XDECREF(pPyPath);

    PyObject *pRes = PyObject_CallFunction(pFunc, "s", qUtf8Printable(path));
    if (!pRes)
    {
        Py_XDECREF(pFunc);
        PyErr_Print();
        PyErr_Clear();
        return -1;
    }
    Py_XDECREF(pFunc);
    Py_XDECREF(pRes);

    return 0;
}

/**
 * to fflush print to stdout.
 */
int RS_Python::fflush(const QString& stream)
{
    return 0;
    QString com = "sys." + stream + ".flush()";
    int ret = PyRun_SimpleString(qUtf8Printable(com));

    if (PyErr_Occurred()) {
        PyErr_Print();
        PyErr_Clear();
        return -1;
    }

    return ret;
}

/**
 * Launches the given script in command line.
 */
int RS_Python::runFileCmd(const QString& name, QString& buf_out, QString& buf_err)
{
    //qDebug() << "[RS_Python::runFile]" << name;

    FILE *fp = fopen(qUtf8Printable(name), "r");
    if (!fp) {

        return -1;
    }

    if (Py_GlobalDict() == NULL) {
        qCritical() << "[RS_Python::runCommand] can not load dict of __main__";
        return -1;
    }

    //PyObject* pDict = PyDict_Copy(Py_GlobalDict());

    std::string buffer_err;
    std::string buffer_out;
    {
        // switch sys.stdout / sys.stderr to custom handler
        emb::stdout_write_type write_out = [&buffer_out] (std::string s) { buffer_out += s; };
        emb::set_stdout(write_out);
        emb::stderr_write_type write_err = [&buffer_err] (std::string s) { buffer_err += s; };
        emb::set_stderr(write_err);

        //PyObject* pRes = PyRun_File(fp, qUtf8Printable(name), Py_file_input, pDict, pDict);
        //PyObject* pRes = PyRun_File(fp, qUtf8Printable(name), Py_file_input, PyDict_New(), PyDict_New());
        PyObject* pRes = PyRun_File(fp, qUtf8Printable(name), Py_file_input, Py_GlobalDict(), Py_GlobalDict());


        if(!pRes) {
            PyErr_Print();
            PyErr_Clear(); //and clear them !
            fclose(fp);

            emb::reset_stdout();
            emb::reset_stderr();

            buf_err = buffer_err.c_str();
            //qCritical() << buffer_err.c_str();
            return -1;
        }
        //Py_XDECREF(pDict);
        Py_XDECREF(pRes);
    }

    fclose(fp);

    buf_out = buffer_out.c_str();
    buf_err = buffer_err.c_str();

    //qInfo() << buffer_out.c_str();

    return 0;
}

/**
 * Launches the given script.
 */
int RS_Python::runFile(const QString& name)
{
    FILE *fp = fopen(qUtf8Printable(name), "r");
    if (!fp) {
        return -1;
    }

    if (Py_GlobalDict() == NULL) {
        qCritical() << "[RS_Python::runFile] can not load dict of __main__";
        return -1;
    }

    std::string buffer_err = QObject::tr("File").toStdString() + ": " + name.toStdString() + "\n";
    {
        // sys.stderr to custom handler
        emb::stderr_write_type write_err = [&buffer_err] (std::string s) { buffer_err += s; };
        emb::set_stderr(write_err);

        PyObject* pRes = PyRun_File(fp, qUtf8Printable(name), Py_file_input, Py_GlobalDict(), Py_GlobalDict());

        if(PyErr_Occurred())
        {
            PyErr_Print();
            PyErr_Clear();
            emb::reset_stderr();

            QMessageBox msgBox;
            msgBox.setWindowTitle(QObject::tr("Python Error!"));
            msgBox.setText(QObject::tr("File: %1").arg(name));
            msgBox.setDetailedText(buffer_err.c_str());
            msgBox.setIcon(QMessageBox::Critical);
            msgBox.setStyleSheet("QTextEdit{min-height: 240px;} QLabel{min-width: 240px;}");
            msgBox.exec();
            return -1;
        }

        emb::reset_stderr();

        Py_XDECREF(pRes);
        fclose(fp);
    }

    return 0;
}

/**
 * Launches the a function from a module.
 */
int RS_Python::runModulFunc(const QString& module, const QString& func)
{
    return runString(module + "." + func + "()");
}

/**
 * run a simple py string
 */
int RS_Python::runString(const QString& str)
{
    //qDebug() << "[RS_Python::runString]" << str;
    int ret = PyRun_SimpleString(qUtf8Printable(str));

    if (PyErr_Occurred())
    {
        PyErr_Print();
        PyErr_Clear();
        return -1;
    }

    fflush("stdout");

    return ret;
}

/**
 * run a string from py command line
 */
int RS_Python::runCommand(const QString& command, QString& buf_out, QString& buf_err)
{
    //qDebug() << "[RS_Python::runCommand]" << command;

    //Py_BEGIN_ALLOW_THREADS

    if (Py_GlobalDict() == NULL) {
        qCritical() << "[RS_Python::runCommand] can not load dict of __main__";
        return -1;
    }

#if 0
    PyObject *module;
    module = PyImport_AddModule("__main__");
    PyObject* pDict = PyModule_GetDict(module);
#endif

    QString result = "";
    std::string buffer_err;
    std::string buffer_out;
    {
        // switch sys.stdout / sys.stderr to custom handler
        emb::stdout_write_type write_out = [&buffer_out] (std::string s) { buffer_out += s; };
        emb::set_stdout(write_out);
        emb::stderr_write_type write_err = [&buffer_err] (std::string s) { buffer_err += s; };
        emb::set_stderr(write_err);

        PyCompilerFlags cf = { 0, PY_MINOR_VERSION };
        cf.cf_flags |= PyCF_IGNORE_COOKIE;

        // Compile as an expression
        //PyObject* pCode = Py_CompileStringExFlags(qUtf8Printable(command), "<string>", Py_eval_input, nullptr, -1);
        //PyObject* pCode = Py_CompileStringFlags(qUtf8Printable(command), "<string>", Py_eval_input, &cf);
        //PyObject* pCode = Py_CompileStringFlags(qUtf8Printable(command), "<string>", Py_eval_input, &cf);
        //PyObject* pCode = _PyRun_SimpleStringFlagsWithName(qUtf8Printable(command), "<string>", &cf);
        //PyObject* pCode = Py_CompileString(qUtf8Printable(command), "<stdin>", Py_eval_input);

        //PyObject* pCode = Py_CompileStringFlags(qUtf8Printable(command), "<string>", Py_single_input, &cf);
        PyObject* pCode = Py_CompileStringFlags(qUtf8Printable(command), "<stdin>", Py_single_input, &cf);

        if(pCode == NULL || PyErr_Occurred())
        {
            // Not an expression?
            qDebug() << "[RS_Python::runCommand] code = NULL || PyErr_Occurred()";

            PyErr_Clear(); //and clear them !
            // Run as a string
            PyRun_SimpleString(qUtf8Printable(command));
        }
        else
        {
            PyObject* pRes = PyEval_EvalCode(pCode, Py_GlobalDict(), Py_GlobalDict());

            //PyObject* pRes = PyEval_EvalCode(pCode, pDict, pDict);
            if (!pRes)
            {

                qDebug() << "[RS_Python::runCommand] PyEval_EvalCode = NULL";

                PyErr_Print();
                PyErr_Clear(); //and clear them !
                Py_XDECREF(pCode);

                emb::reset_stdout();
                emb::reset_stderr();

                buf_out = buffer_out.c_str();
                buf_err = buffer_err.c_str();

                return -1;
            }

            Py_XDECREF(pCode);

            if (pRes != Py_None)
            {
                if(PyUnicode_Check(pRes))
                {
                    result = QString::fromUtf8(PyUnicode_AsUTF8(pRes));
                }
                qDebug() << "[RS_Python::runCommand] PyRun_SimpleString";
                PyRun_SimpleString(qUtf8Printable(command));
            }
            else
            {
                result = QString("None");
            }
            Py_XDECREF(pRes);
        }

        //PyGILState_Release(gil_state);
    }
    //Py_XDECREF(pDict);

    emb::reset_stdout();
    emb::reset_stderr();

    buf_out = buffer_out.c_str();
    buf_err = buffer_err.c_str();

    //qDebug() << "[RS_Python::runCommand] result:" << result;
    return 0;
}

/**
 * run a simple py string with return value (string)
 */
int RS_Python::evalString(const QString& command, QString& buf_out, QString& buf_err)
{
    std::string buffer_err;
    std::string buffer_out;
    {
        // switch sys.stdout / sys.stderr to custom handler
        emb::stdout_write_type write_out = [&buffer_out] (std::string s) { buffer_out += s; };
        emb::set_stdout(write_out);
        emb::stderr_write_type write_err = [&buffer_err] (std::string s) { buffer_err += s; };
        emb::set_stderr(write_err);

        PyObject *pRes = PyRun_String(qUtf8Printable(command), Py_eval_input, Py_GlobalDict(), PyDict_New());
        //PyObject *pRes = PyRun_String(command.toUtf8().constData(), Py_eval_input, Py_GlobalDict(), PyDict_New());

        if(PyErr_Occurred())
        {
            PyErr_Print();
            PyErr_Clear();
            emb::reset_stdout();
            emb::reset_stderr();
            buf_err = buffer_err.c_str();
            return -1;
        }

        emb::reset_stdout();
        emb::reset_stderr();

        // check whether the object is already a unicode string
        if(PyUnicode_Check(pRes))
        {
            buf_out = QString::fromUtf8(PyUnicode_AsUTF8(pRes));
            Py_XDECREF(pRes);
            return 0;
        }

        if (pRes == Py_None)
        {
            buf_out = buffer_out.c_str();
            Py_XDECREF(pRes);
            return 0;
        }




        buf_out = QStringLiteral("Traceback (most recent call last):\nFile \"<string>\", line 1, in <module>\nTypeError: value is not a String");
        Py_XDECREF(pRes);
    }

    return -1;
}

/**
 * run a simple py string with return value (long int)
 */
int RS_Python::evalInteger(const QString& command, int& result, QString& buf_err)
{
    std::string buffer_err;
    {
        // switch sys.stdout / sys.stderr to custom handler
        emb::stderr_write_type write_err = [&buffer_err] (std::string s) { buffer_err += s; };
        emb::set_stderr(write_err);

        PyObject *pRes = PyRun_String(qUtf8Printable(command), Py_eval_input, Py_GlobalDict(), PyDict_New());
        //PyObject *pRes = PyRun_String(qUtf8Printable(command), Py_eval_input, Py_GlobalDict(), PyDict_New());

        if(PyErr_Occurred())
        {
            PyErr_Print();
            PyErr_Clear();
            emb::reset_stderr();
            buf_err = buffer_err.c_str();
            return -1;
        }

        emb::reset_stderr();

        // check whether the object is a long
        if(PyLong_Check(pRes))
        {
            result = PyLong_AsLong(pRes);
            Py_XDECREF(pRes);
            return 0;
        }

        buf_err = QStringLiteral("Traceback (most recent call last):\nFile \"<string>\", line 1, in <module>\nTypeError: value is not an Integer");
        Py_XDECREF(pRes);
    }

    return -1;
}

/**
 * run a simple py string with return value (float)
 */
int RS_Python::evalFloat(const QString& command, double& result, QString& buf_err)
{
    std::string buffer_err;
    {
        // switch sys.stdout / sys.stderr to custom handler
        emb::stderr_write_type write_err = [&buffer_err] (std::string s) { buffer_err += s; };
        emb::set_stderr(write_err);

        PyObject *pRes = PyRun_String(qUtf8Printable(command), Py_eval_input, Py_GlobalDict(), PyDict_New());
        //PyObject *pRes = PyRun_String(qUtf8Printable(command), Py_eval_input, Py_GlobalDict(), PyDict_New());

        if(PyErr_Occurred())
        {
            PyErr_Print();
            PyErr_Clear();
            emb::reset_stderr();
            buf_err = buffer_err.c_str();
            return -1;
        }

        emb::reset_stderr();

        // check whether the object is already a long
        if(PyFloat_Check(pRes))
        {
            result = PyFloat_AsDouble(pRes);
            Py_XDECREF(pRes);
            return 0;
        }

        buf_err = QStringLiteral("Traceback (most recent call last):\nFile \"<string>\", line 1, in <module>\nTypeError: value is not a Float");
        Py_XDECREF(pRes);
    }

    return -1;
}

int RS_Python::execFileFunc(const QString& file, const QString& func)
{
    qDebug() << "[RS_Python::execFileFunc] file:" << file << "func:" << func;

    PyObject *pName = PyUnicode_FromString(qUtf8Printable(file));
    if (!pName) {
        PyErr_Print();
        PyErr_Clear();
        return -1;
    }

    PyObject *pModule = PyImport_Import(pName);
    if (!pModule) {
        PyErr_Print();
        PyErr_Clear();
        Py_XDECREF(pName);
        return -1;
    }
    Py_XDECREF(pName);

    PyObject *pFunc = PyObject_GetAttrString(pModule, qUtf8Printable(func));
    if (!pFunc) {
        PyErr_Print();
        PyErr_Clear();
        Py_XDECREF(pModule);
        return -1;
    }
    Py_XDECREF(pModule);

    if (PyCallable_Check(pFunc) == 0) {
        PyErr_Print();
        PyErr_Clear();
        Py_XDECREF(pFunc);
        return -1;
    }

    PyObject *pArgs = PyTuple_New(0);
    PyObject *pValue = PyObject_CallObject(pFunc, pArgs);
    if (!pValue) {
        PyErr_Print();
        PyErr_Clear();
        Py_XDECREF(pFunc);
        Py_XDECREF(pArgs);
        return -1;
    }
    if (PyErr_Occurred())
    {
        PyErr_Print();
        PyErr_Clear();
        Py_XDECREF(pFunc);
        Py_XDECREF(pArgs);
        Py_XDECREF(pValue);
        return -1;
    }

    Py_XDECREF(pFunc);
    Py_XDECREF(pArgs);

    qInfo() << "[RS_Python::execFileFunc] result of call:" << PyLong_AsLong(pValue);

    Py_XDECREF(pValue);

    return 0;
}

int RS_Python::execModuleFunc(const QString& module, const QString& func)
{
    qDebug() << "[RS_Python::execModuleFunc] module:" << module << "func:" << func;

    PyObject *pModule = PyImport_ImportModule(qUtf8Printable(module));
    if (!pModule) {
        PyErr_Print();
        PyErr_Clear();
        return -1;
    }

    PyObject *pDict = PyModule_GetDict(pModule);
    if (!pDict) {
        PyErr_Print();
        PyErr_Clear();
        Py_XDECREF(pModule);
        return -1;
    }
    Py_XDECREF(pModule);

    PyObject *pFunc = PyDict_GetItemString(pDict, qUtf8Printable(func));
    if (!pFunc) {
        PyErr_Print();
        PyErr_Clear();
        Py_XDECREF(pDict);
        return -1;
    }
    Py_XDECREF(pDict);

    if (!PyCallable_Check(pFunc)) {
        PyErr_Print();
        PyErr_Clear();
        Py_XDECREF(pFunc);
        return -1;
    }

    PyObject *pArgs = Py_BuildValue("(ss)", "spam", "eggs");
    PyObject *pValue = PyObject_CallObject(pFunc, pArgs);
    if (!pValue) {
        PyErr_Print();
        PyErr_Clear();
        Py_XDECREF(pFunc);
        Py_XDECREF(pArgs);
        return -1;
    }
    if (PyErr_Occurred())
    {
        PyErr_Print();
        PyErr_Clear();
        Py_XDECREF(pFunc);
        Py_XDECREF(pArgs);
        Py_XDECREF(pValue);
        return -1;
    }
    Py_XDECREF(pFunc);
    Py_XDECREF(pArgs);

    qInfo() << "[RS_Python::execModuleFunc] result of call:" << PyLong_AsLong(pValue);

    Py_XDECREF(pValue);

    return 0;
}

#endif
