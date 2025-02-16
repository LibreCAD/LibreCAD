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
#include "rs_pythondcl.h"
#include "rs_scriptingapi.h"


void RS_PythonDcl::endList()
{
    RS_SCRIPTINGAPI->endList();
}

void RS_PythonDcl::unloadDialog(int id)
{
    RS_SCRIPTINGAPI->unloadDialog(id);
}

void RS_PythonDcl::termDialog()
{
    RS_SCRIPTINGAPI->termDialog();
}

int RS_PythonDcl::loadDialog(const char *filename)
{
    return RS_SCRIPTINGAPI->loadDialog(filename);
}

bool RS_PythonDcl::newDialog(const char *name, int id)
{
    return RS_SCRIPTINGAPI->newDialog(name, id);
}

bool RS_PythonDcl::setTile(const char *key, const char *val)
{
    return RS_SCRIPTINGAPI->setTile(key, val);
}

bool RS_PythonDcl::modeTile(const char *key, int mode)
{
    return RS_SCRIPTINGAPI->modeTile(key, mode);
}

bool RS_PythonDcl::actionTile(const char *id, const char *action)
{
    return RS_SCRIPTINGAPI->actionTile(id, action);
}

int RS_PythonDcl::startDialog()
{
    return RS_SCRIPTINGAPI->startDialog();
}

PyObject* RS_PythonDcl::getTile(const char *key)
{
    std::string result;
    return RS_SCRIPTINGAPI->getTile(key, result) ? Py_BuildValue("s", result.c_str()) : Py_None;
}

PyObject* RS_PythonDcl::doneDialog(int res) const
{
    int x, y;
    return RS_SCRIPTINGAPI->doneDialog(res, x, y) ? Py_BuildValue("(ii)", x, y) : Py_None;
}

PyObject* RS_PythonDcl::getAttr(const char *key, const char *attr) const
{
    std::string result;
    return RS_SCRIPTINGAPI->getAttr(key, attr, result) ? Py_BuildValue("s", result.c_str()) : Py_None;
}

PyObject* RS_PythonDcl::startList(const char *key, int operation, int index)
{
    return RS_SCRIPTINGAPI->startList(key, operation, index) ? Py_BuildValue("s", key) : Py_None;
}

PyObject* RS_PythonDcl::addList(const char *val) const
{
    std::string result;
    return RS_SCRIPTINGAPI->addList(val, result) ? Py_BuildValue("s", result.c_str()) : Py_None;
}

PyObject* RS_PythonDcl::dimxTile(const char *key) const
{
    int x;
    return RS_SCRIPTINGAPI->dimxTile(key, x) ? Py_BuildValue("i", x) : Py_None;
}

PyObject* RS_PythonDcl::dimyTile(const char *key) const
{
    int y;
    return RS_SCRIPTINGAPI->dimxTile(key, y) ? Py_BuildValue("i", y) : Py_None;
}

PyObject* RS_PythonDcl::fillImage(int x, int y, int width, int height, int color) const
{
    return RS_SCRIPTINGAPI->fillImage(x, y, width, height, color) ? Py_BuildValue("i", color) : Py_None;
}

PyObject* RS_PythonDcl::vectorImage(int x1, int y1, int x2, int y2, int color) const
{
    return RS_SCRIPTINGAPI->vectorImage(x1, y1, x2, y2, color) ? Py_BuildValue("i", color) : Py_None;
}

PyObject* RS_PythonDcl::pixImage(int x1, int y1, int x2, int y2, const char *path) const
{
    return RS_SCRIPTINGAPI->pixImage(x1, y1, x2, y2, path) ? Py_BuildValue("s", path) : Py_None;
}

PyObject* RS_PythonDcl::slideImage(int x1, int y1, int x2, int y2, const char *path) const
{
    return RS_SCRIPTINGAPI->slideImage(x1, y1, x2, y2, path) ? Py_BuildValue("s", path) : Py_None;
}

PyObject* RS_PythonDcl::textImage(int x1, int y1, int x2, int y2, const char *text, int color) const
{
    return RS_SCRIPTINGAPI->textImage(x1, y1, x2, y2, text, color) ? Py_BuildValue("s", text) : Py_None;
}

PyObject* RS_PythonDcl::startImage(const char *key) const
{
    return RS_SCRIPTINGAPI->startImage(key) ? Py_BuildValue("s", key) : Py_None;
}

void RS_PythonDcl::endImage()
{
    RS_SCRIPTINGAPI->endImage();
}
