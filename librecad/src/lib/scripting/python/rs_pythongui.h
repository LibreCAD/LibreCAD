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

#pragma once

#ifndef RS_PYTHONGUI_H
#define RS_PYTHONGUI_H

#include "rs_python.h"
#include "rs_vector.h"

class RS_PythonGui
{
public:
    RS_PythonGui() {}
    ~RS_PythonGui() {}

    void prompt(const char *prompt);
    void initGet(const char *str="", int bit=0);
    void MessageBox(const char *message);

    int GetIntDialog(const char *prompt);
    double GetDoubleDialog(const char *prompt);

    char readChar();
    const std::string OpenFileDialog(const char *title, const char *filename, const char *ext);
    const std::string GetStringDialog(const char *prompt);

    RS_Vector getPoint(const char *prompt = "", const RS_Vector basePoint=RS_Vector()) const;
    RS_Vector getCorner(const char *prompt = "", const RS_Vector &basePoint=RS_Vector()) const;

    PyObject *acadColorDlg(int color=0, bool by=true);
    PyObject *getDist(const char *prompt = "", const RS_Vector &basePoint=RS_Vector()) const;
    PyObject *getFiled(const char *title = "", const char *def = "", const char *ext = "", int flags=0);
    PyObject *getOrient(const char *prompt = "", const RS_Vector &basePoint=RS_Vector()) const;
    PyObject *getInt(const char *prompt = "") const;
    PyObject *getReal(const char *prompt = "") const;
    PyObject *getString(bool cr=false, const char *prompt = "") const;
    PyObject *getKword(const char *prompt = "");

};

#endif // RS_PYTHONGUI_H
