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


#ifdef RS_OPT_SIMPLEPYTHON
#include "rs_simplepython.h"


RS_SimplePython* RS_SimplePython::uniqueInstance = NULL;


/**
 *  Gets the one and only RS_SimplePython instance
 *  (creates a new one on first call only)
 *
 *  @return Pointer to the single instance of this
 * singleton class
 */
RS_SimplePython* RS_SimplePython::instance() {
    if(uniqueInstance==NULL) {
        uniqueInstance = new RS_SimplePython;
    }
    return uniqueInstance;
}


/**
 * Launches the given script.
 */
int RS_SimplePython::launch(const QString& script) {
    long answer;
    PyObject *modname, *mod, *mdict, *func, *rslt;
    //Py_SetProgramName(argv[0]);
    Py_Initialize();
    init_pyextension();
    modname = PyString_FromString(script);
    mod = PyImport_Import(modname);
    if (mod) {
        //printf( "mod\n");
        mdict = PyModule_GetDict(mod);

        // Borrowed reference to start function
        func = PyDict_GetItemString(mdict, "start");
        if (func) {
            //printf( "func\n");
            if (PyCallable_Check(func)) {
                //printf("calling..\n");
                rslt = PyObject_CallFunction(func, "(s)", "noparam");
                //printf("calling ok\n");
                if (rslt) {
                    //printf("c: rslt\n");
                    answer = PyInt_AsLong(rslt);
                    //printf("c: answer is: %ld\n", answer);
                    Py_XDECREF(rslt);
                }
            }
        } else {
            printf("no such function: start\n");
        }
        Py_XDECREF(mod);
    } else {
        printf("no such module: %s\n", script.latin1());
    }
    Py_XDECREF(modname);
    Py_Finalize();
    return 0;
}


/**
 * A test method exposed to Python 
 */
long inc(long i) {
    printf("c: inc called\n");
    printf("c: parameter from python: %ld\n", i);
    return ++i;
}

/**
 * The magic that exposes inc(). A wrapper function.
 */
static PyObject *py_inc(PyObject* /*self*/, PyObject* args) {
    long i;
    printf("c: py_inc called\n");
    if (!PyArg_ParseTuple(args, "l", &i))
        return NULL;
    return Py_BuildValue("l", inc(i));
}

/**
 * Adds a line to the current graphic document.
 */
void rsPyAddLine(double x1, double y1, double x2, double y2) {
    //printf("c: addLine called\n");
    //printf("c: parameter from python: %f\n", x1);

    RS_Graphic* graphic = RS_SIMPLEPYTHON->getGraphic();
    if (graphic) {
        graphic->addEntity(new RS_Line(graphic,
                                       RS_LineData(RS_Vector(x1, y1),
                                                   RS_Vector(x2, y2))));
    } else {
        std::cerr << "rsPyAddLine: No graphic object set.\n";
    }
}

/**
 * Python wrapper.
 */
static PyObject *py_rsPyAddLine(PyObject* /*self*/, PyObject* args) {
    double x1, y1, x2, y2;
    //printf("c: py_rsPyAddLine called\n");
    if (!PyArg_ParseTuple(args, "dddd", &x1, &y1, &x2, &y2)) {
        return NULL;
    }
    rsPyAddLine(x1, y1, x2, y2);
    return Py_BuildValue("d", 1);
}

/**
 * The librecad module's function table.
 */
static PyMethodDef rsLibreCADMethods[] =
    {
        {"inc",     py_inc,     1,
         "a silly example method"},
        {"rsPyAddLine", py_rsPyAddLine, 1,
         "adds a line to the current document"},
        {NULL,      NULL}       /* sentinel */
    };

/**
 * Python will call this when the librecad module is imported.
 */
void init_pyextension() {
    printf("c: adding module: librecad\n");
    PyImport_AddModule("librecad");
    Py_InitModule("librecad", rsLibreCADMethods);
    printf("c: module librecad: OK\n");
}

#endif
