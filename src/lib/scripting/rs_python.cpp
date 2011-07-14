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

#include "rs_python.h"

#ifdef RS_OPT_PYTHON

//
// This is exported from the Boost::Python library declarations
// that are declared inside rs_python_wrappers.cpp.
//
extern "C" void initlibrecad();

/**
 * The unique instance of the Python scripting engine
 */
RS_Python* RS_Python::uniqueInstance = NULL;

/**
 * Constructor
 */
RS_Python::RS_Python()
{
    graphic = NULL;
    Py_Initialize();
    initlibrecad();
}

/**
 *  Gets the one and only RS_Python instance
 *  (creates a new one on first call only)
 *
 *  @return Pointer to the single instance of this
 * singleton class
 */
RS_Python* RS_Python::instance() {
    if(uniqueInstance==NULL) {
        uniqueInstance = new RS_Python;
    }
    return uniqueInstance;
}


/**
 * Launches the given script.
 */
int RS_Python::launch(const QString& script) {
    PyObject *modname, *mod, *mdict, *func, *rslt;
    //Py_SetProgramName(argv[0]);

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
		    // The result value is currently not used
                    Py_XDECREF(rslt);
                } else
		{
		    // Give user some feed back what went wrong
	            printf("*** PYTHON RUNTIME ERROR ***\n");
		    PyErr_Print();
		}
            }
        } else {
            printf("no such function: start\n");
        }
        Py_XDECREF(mod);
    } else {
        printf("*** ERROR LOADING SCRIPT '%s' ***\n", script.latin1());
	PyErr_Print();	
    }
    Py_XDECREF(modname);
    //Py_Finalize();
    return 0;
}

#endif
