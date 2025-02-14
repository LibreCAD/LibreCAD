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
#include "rs_pythoncore.h"
#include "rs_scriptingapi.h"

#include "qc_applicationwindow.h"

RS_Document* RS_PythonCore::getDocument() const
{
    return QC_ApplicationWindow::getAppWindow()->getDocument();
}

RS_EntityContainer* RS_PythonCore::getContainer() const
{
    auto& appWin = QC_ApplicationWindow::getAppWindow();
    RS_GraphicView* graphicView = appWin->getGraphicView();
    return graphicView->getContainer();
}

RS_Graphic* RS_PythonCore::getGraphic() const
{
    auto& appWin=QC_ApplicationWindow::getAppWindow();
    RS_Document* d = appWin->getDocument();

    if (d && d->rtti()==RS2::EntityGraphic)
    {
        RS_Graphic* graphic = (RS_Graphic*)d;
        if (graphic==NULL) {
            return NULL;
        }
        return graphic;
    }
    return NULL;
}

void RS_PythonCore::command(const char *cmd)
{
    QString scmd = cmd;
    scmd = scmd.simplified();
    QStringList coms = scmd.split(" ");

    for(auto & s : coms)
    {
        RS_SCRIPTINGAPI->command(s);
    }
}

PyObject *RS_PythonCore::entlast() const
{
    unsigned int id = RS_SCRIPTINGAPI->entlast();
    return id > 0 ? Py_BuildValue("i", (int)id) : Py_None;
}

PyObject *RS_PythonCore::entdel(unsigned int id) const
{
    return RS_SCRIPTINGAPI->entdel(id) ? Py_BuildValue("i", (int)id) : Py_None;
}

PyObject *RS_PythonCore::entsel(const char* prombt) const
{
    QString prom = "Select object:";
    unsigned long id;
    RS_Vector result;

    if (std::strcmp(prombt, ""))
    {
        prom = prombt;
    }

    if (Py_CommandEdit != nullptr)
    {
        Py_CommandEdit->setPrompt(QObject::tr(qUtf8Printable(prom)));
        Py_CommandEdit->setFocus();
        Py_CommandEdit->doProcess(false);
    }

    if (RS_SCRIPTINGAPI->entsel(QObject::tr(qUtf8Printable(prom)), id, result))
    {
        if (Py_CommandEdit != nullptr)
        {
            Py_CommandEdit->resetPrompt();
        }

        return Py_BuildValue("(i(ddd))", id, result.x, result.y, result.z);
    }

    if (Py_CommandEdit != nullptr)
    {
        Py_CommandEdit->resetPrompt();
    }

    Py_RETURN_NONE;
}
