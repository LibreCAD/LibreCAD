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

#include "rs_pythongui.h"
#include "rs_scriptingapi.h"


void RS_PythonGui::MessageBox(const char *message)
{
    RS_SCRIPTINGAPI->msgInfo(message);
}

const std::string RS_PythonGui::OpenFileDialog(const char *title, const char *filename, const char *ext)
{
    return RS_SCRIPTINGAPI->getFileNameDlg(title, filename, ext);
}

int RS_PythonGui::GetIntDialog(const char *prompt)
{
    return RS_SCRIPTINGAPI->getIntDlg(prompt);
}

double RS_PythonGui::GetDoubleDialog(const char *prompt)
{
    return RS_SCRIPTINGAPI->getDoubleDlg(prompt);
}

const std::string RS_PythonGui::GetStringDialog(const char *prompt)
{
    return RS_SCRIPTINGAPI->getStrDlg(prompt);
}

PyObject* RS_PythonGui::acadColorDlg(int color, bool by)
{
    int result;
    return RS_SCRIPTINGAPI->colorDialog(color,
                                        by,
                                        result) ? Py_BuildValue("i", result) : Py_None;
}

PyObject* RS_PythonGui::getString(bool cr, const char *prompt) const
{
    std::string result;
    return RS_SCRIPTINGAPI->getString(Py_CommandEdit,
                                      cr,
                                      prompt,
                                      result) ? Py_BuildValue("s", result.c_str()) : Py_None;

}

RS_Vector RS_PythonGui::getCorner(const char *prompt, const RS_Vector &basePoint) const
{            
    return RS_SCRIPTINGAPI->getCorner(Py_CommandEdit, qUtf8Printable(prompt), basePoint);
}

RS_Vector RS_PythonGui::getPoint(const char *prompt, const RS_Vector basePoint) const
{
    return RS_SCRIPTINGAPI->getPoint(Py_CommandEdit, qUtf8Printable(prompt), basePoint);
}

PyObject* RS_PythonGui::getDist(const char *prompt, const RS_Vector &basePoint) const
{
    double distance;
    return RS_SCRIPTINGAPI->getDist(Py_CommandEdit,
                                    qUtf8Printable(prompt),
                                    basePoint,
                                    distance) ? Py_BuildValue("d", distance) : Py_None;
}

PyObject* RS_PythonGui::getInt(const char *prompt) const
{
    int result;
    return RS_SCRIPTINGAPI->getInteger(Py_CommandEdit,
                                    prompt,
                                    result) ? Py_BuildValue("i", result) : Py_None;
}

PyObject* RS_PythonGui::getReal(const char *prompt) const
{
    double result;
    return RS_SCRIPTINGAPI->getReal(Py_CommandEdit,
                                    prompt,
                                    result) ? Py_BuildValue("d", result) : Py_None;
}

PyObject* RS_PythonGui::getFiled(const char *title, const char *def, const char *ext, int flags)
{
    std::string filename;
    return RS_SCRIPTINGAPI->getFiled(title,
                                     def,
                                     ext,
                                     flags,
                                     filename) ? Py_BuildValue("s", filename) : Py_None;
}

PyObject* RS_PythonGui::getOrient(const char *prompt, const RS_Vector &basePoint) const
{
    double radius;
    return RS_SCRIPTINGAPI->getOrient(Py_CommandEdit,
                                    prompt,
                                    basePoint,
                                    radius) ? Py_BuildValue("d", radius) : Py_None;
}

char RS_PythonGui::readChar()
{
    return RS_SCRIPTINGAPI->readChar();
}

PyObject* RS_PythonGui::getKword(const char *prompt)
{
    std::string result;
    return RS_SCRIPTINGAPI->getKeyword(Py_CommandEdit,
                                      prompt,
                                      result) ? Py_BuildValue("s", result.c_str()) : Py_None;
}

void RS_PythonGui::initGet(const char *str, int bit)
{
    RS_SCRIPTINGAPI->initGet(bit, str);
}

void RS_PythonGui::prompt(const char *prompt)
{
    RS_SCRIPTINGAPI->prompt(Py_CommandEdit, prompt);
}
