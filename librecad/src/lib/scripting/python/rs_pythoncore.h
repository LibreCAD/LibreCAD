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

#ifndef RS_PYTHONCORE_H
#define RS_PYTHONCORE_H

#ifdef DEVELOPER

#include "Python.h"

#include "rs_document.h"
#include "rs_graphic.h"
#include "rs_entitycontainer.h"

class RS_PythonCore
{
public:
    RS_PythonCore() {}
    ~RS_PythonCore() {}

    void command(const char *cmd);
    PyObject *entlast() const;
    PyObject *entdel(const std::string &ename) const;
    PyObject *entget(const std::string &ename) const;
    PyObject *entmod(const PyObject &entity) const;
    PyObject *entnext(const std::string &ename) const;
    PyObject *entsel(const char* prompt = "") const;

    RS_Document *getDocument() const;
    RS_Graphic *getGraphic() const;
    RS_EntityContainer* getContainer() const;

};

#endif // DEVELOPER

#endif // RS_PYTHONCORE_H
