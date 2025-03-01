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
    return id > 0 ? Py_BuildValue("s", RS_SCRIPTINGAPI->getEntityName(id).c_str()) : Py_None;
}

PyObject *RS_PythonCore::entdel(const std::string &ename) const
{
    return RS_SCRIPTINGAPI->entdel(RS_SCRIPTINGAPI->getEntityId(ename)) ? Py_BuildValue("s", ename.c_str()) : Py_None;
}

PyObject *RS_PythonCore::entsel(const char* prompt) const
{
    QString prom = "Select object:";
    unsigned long id;
    RS_Vector result;

    if (std::strcmp(prompt, ""))
    {
        prom = prompt;
    }

    return RS_SCRIPTINGAPI->entsel(Py_CommandEdit,
                                   QObject::tr(qUtf8Printable(prom)),
                                   id,
                                   result) ? Py_BuildValue("(s(ddd))", RS_SCRIPTINGAPI->getEntityName(id).c_str(),
                                                                       result.x,
                                                                       result.y,
                                                                       result.z) : Py_None;
}

/*
 * static PyObject* foo(PyObject* self, PyObject* args){
    PyObect* list = PyList_New(100);
    for(int i = 0; i < 100; i++)
        PyList_SetItem(list, i, Py_BuildValue("i", 42));
    return list;
* }
*/

PyObject *RS_PythonCore::entget(const std::string &ename) const
{
    unsigned int id = RS_SCRIPTINGAPI->getEntityId(ename);

    auto& appWin = QC_ApplicationWindow::getAppWindow();
    RS_GraphicView* graphicView = appWin->getGraphicView();
    RS_EntityContainer* entityContainer = graphicView->getContainer();

    if(entityContainer->count())
    {
        for (auto e: *entityContainer) {
            if (e->getId() == id)
            {
#if 0
                RS_Pen pen = e->getPen(false);

                lclValueVec *entity = new lclValueVec;

                lclValueVec *ename = new lclValueVec(3);
                ename->at(0) = lcl::integer(-1);
                ename->at(1) = lcl::symbol(".");
                ename->at(2) = lcl::ename(en->value());

                lclValueVec *ebname = new lclValueVec(3);
                ebname->at(0) = lcl::integer(330);
                ebname->at(1) = lcl::symbol(".");
                ebname->at(2) = lcl::ename(e->getParent()->getId());
                entity->push_back(lcl::list(ebname));

                lclValueVec *handle = new lclValueVec(3);
                handle->at(0) = lcl::integer(5);
                handle->at(1) = lcl::symbol(".");
                handle->at(2) = lcl::string(en->valueStr());
                entity->push_back(lcl::list(handle));

                enum RS2::LineType lineType = pen.getLineType();
                if(lineType != RS2::LineByLayer)
                {
                    lclValueVec *lType = new lclValueVec(3);
                    lType->at(0) = lcl::integer(6);
                    lType->at(1) = lcl::symbol(".");
                    lType->at(2) = lcl::string(RS_FilterDXFRW::lineTypeToName(pen.getLineType()).toStdString());
                    entity->push_back(lcl::list(lType));
                }

                enum RS2::LineWidth lineWidth = pen.getWidth();
                if(lineWidth != RS2::WidthByLayer)
                {
                    lclValueVec *lWidth = new lclValueVec(3);
                    lWidth->at(0) = lcl::integer(48);
                    lWidth->at(1) = lcl::symbol(".");

                    int width = static_cast<int>(lineWidth);
                    if (width < 0)
                    {
                        lWidth->at(2) = lcl::integer(width);
                    }
                    else
                    {
                        lWidth->at(2) = lcl::ldouble(double(width) / 100.0);
                    }
                    entity->push_back(lcl::list(lWidth));

                }

                RS_Color color = pen.getColor();
                if(!color.isByLayer())
                {
                    int exact_rgb;
                    lclValueVec *col = new lclValueVec(3);
                    col->at(0) = lcl::integer(62);
                    col->at(1) = lcl::symbol(".");
                    col->at(2) = lcl::integer(RS_FilterDXFRW::colorToNumber(color, &exact_rgb));
                    entity->push_back(lcl::list(col));
                }

                lclValueVec *acdb = new lclValueVec(3);
                acdb->at(0) = lcl::integer(100);
                acdb->at(1) = lcl::symbol(".");
                acdb->at(2) = lcl::string("AcDbEntity");

                lclValueVec *mspace = new lclValueVec(3);
                mspace->at(0) = lcl::integer(67);
                mspace->at(1) = lcl::symbol(".");
                mspace->at(2) = lcl::integer(0);

                lclValueVec *layoutTabName = new lclValueVec(3);
                layoutTabName->at(0) = lcl::integer(100);
                layoutTabName->at(1) = lcl::symbol(".");
                layoutTabName->at(2) = lcl::string("Model");

                lclValueVec *layer = new lclValueVec(3);
                layer->at(0) = lcl::integer(8);
                layer->at(1) = lcl::symbol(".");
                layer->at(2) = lcl::string(e->getLayer()->getName().toStdString());

                lclValueVec *extrDir = new lclValueVec(4);
                extrDir->at(0) = lcl::integer(210);
                extrDir->at(1) = lcl::ldouble(0.0);
                extrDir->at(2) = lcl::ldouble(0.0);
                extrDir->at(3) = lcl::ldouble(1.0);

                entity->push_back(lcl::list(acdb));
                entity->push_back(lcl::list(mspace));
                entity->push_back(lcl::list(layoutTabName));
                entity->push_back(lcl::list(layer));
#endif
                switch (e->rtti())
                {
                    case RS2::EntityPoint:
                    {
                    }
                    break;
                default:
                    break;
                }

            }
        }
    }

    Py_RETURN_NONE;
}
