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
#define PYBIND11_NO_KEYWORDS
#define PY_SSIZE_T_CLEAN
#include <Python.h>
#ifndef RS_PYTHON_H
#define RS_PYTHON_H

#ifdef RS_OPT_PYTHON

#include <QString>

#define COPYRIGHT \
"\nType \"help\", \"copyright\", \"credits\" or \"license\" " \
    "for more information."

#define RS_PYTHON RS_Python::instance()

typedef struct v3 {
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
} v3_t;

/*
 * LibreCAD Python scripting support.
 *
 * rewritten by
 *
 * @author Emanuel Strobel
 *
 * 2024
 *
 */

class RS_Python
{
public:
    static RS_Python* instance();
    ~RS_Python();

    QString Py_GetVersionString() const { return QString("Python ") + QString(Py_GetVersion()) + QString(" on ") + QString(Py_GetPlatform()) + QString(COPYRIGHT); }

    int addSysPath(const QString& path);
    int runCommand(const QString& command, QString& buf_out, QString& buf_err);
    int runFileCmd(const QString& name, QString& buf_out, QString& buf_err);
    int runFile(const QString& name);
    int evalString(const QString& command, QString& result);
    int evalInteger(const QString& command, int& result);
    int evalFloat(const QString& command, double& result);
    int evalVector(const QString& command, v3_t& vec);
    int runString(const QString& str);
    int fflush(const QString& stream);
    int runModulFunc(const QString& module, const QString& func);
    int execFileFunc(const QString& file, const QString& func);
    int execModuleFunc(const QString& module, const QString& func);

private:
    RS_Python();
    static RS_Python* unique;
    PyObject* m_pGlobalMain;
    PyObject* m_pGlobalDict;
    PyObject* globalMain() { return m_pGlobalMain; }
    PyObject* Py_GlobalDict() { return m_pGlobalDict; }
};

#endif // RS_OPT_PYTHON

#endif // RS_PYTHON_H

