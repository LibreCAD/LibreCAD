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
#include "rs_insert.h"
#include "rs_hatch.h"
#include "rs_solid.h"

#include "rs_filterdxfrw.h"

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

PyObject *RS_PythonCore::entmake(PyObject *args) const
{
    Q_UNUSED(args);

    qDebug() << "[RS_PythonCore::entmake] - start";

    Py_RETURN_NONE;
}

PyObject *RS_PythonCore::entmod(PyObject *args) const
{
    qDebug() << "[RS_PythonCore::entmod] - start";

    PyObject *pList;

    if (!PyArg_ParseTuple(args, "O!", &PyList_Type, &pList)) {
        PyErr_SetString(PyExc_TypeError, "parameter must be a entity list.");
        Py_RETURN_NONE;
    }

    int gc;
    QString ename;
    PyObject *pTuple;
    PyObject *pGc;
    PyObject *pEname;
    Py_ssize_t n = PyList_Size(pList);

    for (int i=0; i<n; i++) {
        pTuple = PyList_GetItem(pList, i);
        if(!PyTuple_Check(pTuple)) {
            PyErr_SetString(PyExc_TypeError, "list items must be a tuple.");
            Py_RETURN_NONE;
        }
        pGc = PyTuple_GetItem(pTuple, 0);
        if(!PyLong_Check(pGc)) {
            PyErr_SetString(PyExc_TypeError, "first tuple item must be an integer.");
            Py_RETURN_NONE;
        }
        gc = PyLong_AsLong(pGc);
        qDebug() << "[RS_PythonCore::entmod] i:" << i << "GC:" << gc;

        if(gc == 0)
        {
            pEname = PyTuple_GetItem(pTuple, 1);
            if(!PyUnicode_Check(pEname)) {
                PyErr_SetString(PyExc_TypeError, "tuple item must be a string.");
                Py_RETURN_NONE;
            }
            ename = QString::fromUtf8(PyUnicode_AsUTF8(pEname));
            qDebug() << "[RS_PythonCore::entmod] ename:" << ename;
            break;
        }
    }

    if (ename == "")
        Py_RETURN_NONE;

    auto& appWin = QC_ApplicationWindow::getAppWindow();
    RS_GraphicView* graphicView = appWin->getGraphicView();
    RS_EntityContainer* entityContainer = graphicView->getContainer();

    if(entityContainer->count())
    {
        for (auto entity: *entityContainer)
        {
            if (entity->getId() == RS_SCRIPTINGAPI->getEntityId(qUtf8Printable(ename)))
            {
                qDebug() << "[RS_PythonCore::entmod] ename found!";
            }
        }
    }

    qDebug() << "[RS_PythonCore::entmod] - end";

    Py_RETURN_NONE;
}

PyObject *RS_PythonCore::entnext(const std::string &ename) const
{
    unsigned int id = 0;

    if (ename == "")
    {
        id = RS_SCRIPTINGAPI->entnext();
    }
    else
    {
        id = RS_SCRIPTINGAPI->entnext(RS_SCRIPTINGAPI->getEntityId(ename));
    }

    return id > 0 ? Py_BuildValue("s", RS_SCRIPTINGAPI->getEntityName(id).c_str()) : Py_None;
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
                int exact_rgb;
                RS_Pen pen = e->getPen(false);
                RS_Color color = pen.getColor();
                enum RS2::LineWidth lineWidth = pen.getWidth();
                int width = static_cast<int>(lineWidth);

                switch (e->rtti())
                {
                    case RS2::EntityPoint:
                    {
                        RS_Point* p = (RS_Point*)e;
                        return Py_BuildValue("[oooooooooooooo]",
                            Py_BuildValue("(is)", 0, "POINT"),
                            Py_BuildValue("(is)", -1, ename),
                            Py_BuildValue("(ii)", 330, p->getParent()->getId()),
                            Py_BuildValue("(is)", 5, RS_SCRIPTINGAPI->getEntityHndl(p->getId())),
                            Py_BuildValue("(is)", 6, RS_FilterDXFRW::lineTypeToName(pen.getLineType()).toStdString()),
                            Py_BuildValue("(ii)", 62, RS_FilterDXFRW::colorToNumber(color, &exact_rgb)),
                            width < 0 ? Py_BuildValue("(ii)", 48, width) : Py_BuildValue("(id)", 48, double(width) / 100.0),
                            Py_BuildValue("(is)", 100, "AcDbEntity"),
                            Py_BuildValue("(ii)", 67, 0),
                            Py_BuildValue("(is)", 100, "Model"),
                            Py_BuildValue("(is)", 8, p->getLayer()->getName().toStdString()),
                            Py_BuildValue("(is)", 100, "AcDbPoint"),
                            Py_BuildValue("(iddd)", 10, p->getPos().x, p->getPos().y, p->getPos().z),
                            Py_BuildValue("(iddd)", 210, 0.0, 0.0, 1.0)
                        );
                    }
                    break;
                    case RS2::EntityLine:
                    {
                        RS_Line* l = (RS_Line*)e;
                        return Py_BuildValue("[ooooooooooooooo]",
                            Py_BuildValue("(is)", 0, "LINE"),
                            Py_BuildValue("(is)", -1, ename),
                            Py_BuildValue("(ii)", 330, l->getParent()->getId()),
                            Py_BuildValue("(is)", 5, RS_SCRIPTINGAPI->getEntityHndl(l->getId())),
                            Py_BuildValue("(is)", 6, RS_FilterDXFRW::lineTypeToName(pen.getLineType()).toStdString()),
                            Py_BuildValue("(ii)", 62, RS_FilterDXFRW::colorToNumber(color, &exact_rgb)),
                            width < 0 ? Py_BuildValue("(ii)", 48, width) : Py_BuildValue("(id)", 48, double(width) / 100.0),
                            Py_BuildValue("(is)", 100, "AcDbEntity"),
                            Py_BuildValue("(ii)", 67, 0),
                            Py_BuildValue("(is)", 100, "Model"),
                            Py_BuildValue("(is)", 8, l->getLayer()->getName().toStdString()),
                            Py_BuildValue("(is)", 100, "AcDbLine"),
                            Py_BuildValue("(iddd)", 10, l->getStartpoint().x, l->getStartpoint().y, l->getStartpoint().z),
                            Py_BuildValue("(iddd)", 11, l->getEndpoint().x, l->getEndpoint().y, l->getEndpoint().z),
                            Py_BuildValue("(iddd)", 210, 0.0, 0.0, 1.0)
                        );
                    }
                    break;
                    case RS2::EntityPolyline:
                    {
                        RS_Polyline* pl = (RS_Polyline*)e;
                        bool is3d = false;
                        RS_VectorSolutions pnts = pl->getRefPoints();

                        for (auto &v : pl->getRefPoints())
                        {
                            if(v.z != 0.0)
                            {
                                is3d = true;
                                break;
                            }
                        }

                        int fl = 0;
                        fl |= 8;
                        if (pl->isClosed())
                        {
                            fl |= 1;
                        }

                        if (is3d)
                        {
                            return Py_BuildValue("[oooooooooooooo]",
                                Py_BuildValue("(is)", 0, "POLYLINE"),
                                Py_BuildValue("(is)", -1, ename),
                                Py_BuildValue("(ii)", 330, pl->getParent()->getId()),
                                Py_BuildValue("(is)", 5, RS_SCRIPTINGAPI->getEntityHndl(pl->getId())),
                                Py_BuildValue("(is)", 6, RS_FilterDXFRW::lineTypeToName(pen.getLineType()).toStdString()),
                                Py_BuildValue("(ii)", 62, RS_FilterDXFRW::colorToNumber(color, &exact_rgb)),
                                width < 0 ? Py_BuildValue("(ii)", 48, width) : Py_BuildValue("(id)", 48, double(width) / 100.0),
                                Py_BuildValue("(is)", 100, "AcDbEntity"),
                                Py_BuildValue("(ii)", 67, 0),
                                Py_BuildValue("(is)", 100, "Model"),
                                Py_BuildValue("(is)", 8, pl->getLayer()->getName().toStdString()),
                                Py_BuildValue("(is)", 100, "AcDb3dPolyline"),
                                Py_BuildValue("(ii)", 70, fl),
                                Py_BuildValue("(iddd)", 210, 0.0, 0.0, 1.0)
                            );
                        }
                        else
                        {
                            PyObject* list = PyList_New(pnts.size()*4 + 14);
                            PyList_SET_ITEM(list, 0, Py_BuildValue("(is)", 0, "LWPOLYLINE"));
                            PyList_SET_ITEM(list, 1, Py_BuildValue("(is)", -1, ename));
                            PyList_SET_ITEM(list, 2, Py_BuildValue("(ii)", 330, pl->getParent()->getId()));
                            PyList_SET_ITEM(list, 3, Py_BuildValue("(is)", 5, RS_SCRIPTINGAPI->getEntityHndl(pl->getId())));
                            PyList_SET_ITEM(list, 4, Py_BuildValue("(is)", 6, RS_FilterDXFRW::lineTypeToName(pen.getLineType()).toStdString()));
                            PyList_SET_ITEM(list, 5, Py_BuildValue("(ii)", 62, RS_FilterDXFRW::colorToNumber(color, &exact_rgb)));
                            PyList_SET_ITEM(list, 6, width < 0 ? Py_BuildValue("(ii)", 48, width) : Py_BuildValue("(id)", 48, double(width) / 100.0));
                            PyList_SET_ITEM(list, 7, Py_BuildValue("(is)", 100, "AcDbEntity"));
                            PyList_SET_ITEM(list, 8, Py_BuildValue("(ii)", 67, 0));
                            PyList_SET_ITEM(list, 9, Py_BuildValue("(is)", 100, "Model"));
                            PyList_SET_ITEM(list, 10, Py_BuildValue("(is)", 8, pl->getLayer()->getName().toStdString()));
                            PyList_SET_ITEM(list, 11, Py_BuildValue("(is)", 100, "AcDbPolyline"));
                            int n = 12;

                            for (auto &v : pl->getRefPoints())
                            {
                                PyList_SET_ITEM(list, n++, Py_BuildValue("(idd)", 10, v.x, v.y));
                                PyList_SET_ITEM(list, n++, Py_BuildValue("(id)", 40, 0.0));
                                PyList_SET_ITEM(list, n++, Py_BuildValue("(id)", 41, 0.0));
                                PyList_SET_ITEM(list, n++, Py_BuildValue("(id)", 42, 0.0));
                            }

                            PyList_SET_ITEM(list, n++, Py_BuildValue("(ii)", 70, fl));
                            PyList_SET_ITEM(list, n++, Py_BuildValue("(iddd)", 210, 0.0, 0.0, 1.0));

                            return list;
                        }
                    }
                    break;
                    case RS2::EntityArc:
                    {
                        RS_Arc* a = (RS_Arc*)e;
                        return Py_BuildValue("[ooooooooooooooooo]",
                            Py_BuildValue("(is)", 0, "ARC"),
                            Py_BuildValue("(is)", -1, ename),
                            Py_BuildValue("(ii)", 330, a->getParent()->getId()),
                            Py_BuildValue("(is)", 5, RS_SCRIPTINGAPI->getEntityHndl(a->getId())),
                            Py_BuildValue("(is)", 6, RS_FilterDXFRW::lineTypeToName(pen.getLineType()).toStdString()),
                            Py_BuildValue("(ii)", 62, RS_FilterDXFRW::colorToNumber(color, &exact_rgb)),
                            width < 0 ? Py_BuildValue("(ii)", 48, width) : Py_BuildValue("(id)", 48, double(width) / 100.0),
                            Py_BuildValue("(is)", 100, "AcDbEntity"),
                            Py_BuildValue("(ii)", 67, 0),
                            Py_BuildValue("(is)", 100, "Model"),
                            Py_BuildValue("(is)", 8, a->getLayer()->getName().toStdString()),
                            Py_BuildValue("(is)", 100, "AcDbArc"),
                            Py_BuildValue("(iddd)", 10, a->getCenter().x, a->getCenter().y, a->getCenter().z),
                            Py_BuildValue("(id)", 40, a->getRadius()),
                            Py_BuildValue("(id)", 50, a->getAngle1()),
                            Py_BuildValue("(id)", 51, a->getAngle2()),
                            Py_BuildValue("(iddd)", 210, 0.0, 0.0, 1.0)
                        );
                    }
                    break;
                    case RS2::EntityCircle:
                    {
                        RS_Circle* c = (RS_Circle*)e;
                        return Py_BuildValue("[ooooooooooooooo]",
                            Py_BuildValue("(is)", 0, "ARC"),
                            Py_BuildValue("(is)", -1, ename),
                            Py_BuildValue("(ii)", 330, c->getParent()->getId()),
                            Py_BuildValue("(is)", 5, RS_SCRIPTINGAPI->getEntityHndl(c->getId())),
                            Py_BuildValue("(is)", 6, RS_FilterDXFRW::lineTypeToName(pen.getLineType()).toStdString()),
                            Py_BuildValue("(ii)", 62, RS_FilterDXFRW::colorToNumber(color, &exact_rgb)),
                            width < 0 ? Py_BuildValue("(ii)", 48, width) : Py_BuildValue("(id)", 48, double(width) / 100.0),
                            Py_BuildValue("(is)", 100, "AcDbEntity"),
                            Py_BuildValue("(ii)", 67, 0),
                            Py_BuildValue("(is)", 100, "Model"),
                            Py_BuildValue("(is)", 8, c->getLayer()->getName().toStdString()),
                            Py_BuildValue("(is)", 100, "AcDbArc"),
                            Py_BuildValue("(iddd)", 10, c->getCenter().x, c->getCenter().y, c->getCenter().z),
                            Py_BuildValue("(id)", 40, c->getRadius()),
                            Py_BuildValue("(iddd)", 210, 0.0, 0.0, 1.0)
                        );
                    }
                    break;
                    case RS2::EntityEllipse:
                    {
                        RS_Ellipse* ellipse=static_cast<RS_Ellipse*>(e);
                        return Py_BuildValue("[oooooooooooooooo]",
                            Py_BuildValue("(is)", 0, "ELLIPSE"),
                            Py_BuildValue("(is)", -1, ename),
                            Py_BuildValue("(ii)", 330, ellipse->getParent()->getId()),
                            Py_BuildValue("(is)", 5, RS_SCRIPTINGAPI->getEntityHndl(ellipse->getId())),
                            Py_BuildValue("(is)", 6, RS_FilterDXFRW::lineTypeToName(pen.getLineType()).toStdString()),
                            Py_BuildValue("(ii)", 62, RS_FilterDXFRW::colorToNumber(color, &exact_rgb)),
                            width < 0 ? Py_BuildValue("(ii)", 48, width) : Py_BuildValue("(id)", 48, double(width) / 100.0),
                            Py_BuildValue("(is)", 100, "AcDbEntity"),
                            Py_BuildValue("(ii)", 67, 0),
                            Py_BuildValue("(is)", 100, "Model"),
                            Py_BuildValue("(is)", 8, ellipse->getLayer()->getName().toStdString()),
                            Py_BuildValue("(is)", 100, "AcDbEllipse"),
                            Py_BuildValue("(iddd)", 10, ellipse->getCenter().x, ellipse->getCenter().y, ellipse->getCenter().z),
                            Py_BuildValue("(iddd)", 11, ellipse->getMajorP().x, ellipse->getMajorP().y, ellipse->getMajorP().z),
                            Py_BuildValue("(id)", 40, ellipse->getRatio()),
                            Py_BuildValue("(iddd)", 210, 0.0, 0.0, 1.0)
                        );
                    }
                    break;
                    case RS2::EntityDimAligned:
                    {
                        RS_DimAligned* dal = (RS_DimAligned*)e;
                        return Py_BuildValue("[oooooooooooooooooo]",
                            Py_BuildValue("(is)", 0, "DIMENSION"),
                            Py_BuildValue("(is)", -1, ename),
                            Py_BuildValue("(ii)", 330, dal->getParent()->getId()),
                            Py_BuildValue("(is)", 5, RS_SCRIPTINGAPI->getEntityHndl(dal->getId())),
                            Py_BuildValue("(is)", 6, RS_FilterDXFRW::lineTypeToName(pen.getLineType()).toStdString()),
                            Py_BuildValue("(ii)", 62, RS_FilterDXFRW::colorToNumber(color, &exact_rgb)),
                            width < 0 ? Py_BuildValue("(ii)", 48, width) : Py_BuildValue("(id)", 48, double(width) / 100.0),
                            Py_BuildValue("(is)", 100, "AcDbEntity"),
                            Py_BuildValue("(ii)", 67, 0),
                            Py_BuildValue("(is)", 100, "Model"),
                            Py_BuildValue("(is)", 8, dal->getLayer()->getName().toStdString()),
                            Py_BuildValue("(is)", 100, "AcDbAlignedDimension"),
                            Py_BuildValue("(iddd)", 10, dal->getDefinitionPoint().x, dal->getDefinitionPoint().y,dal->getDefinitionPoint().z),
                            Py_BuildValue("(iddd)", 11, dal->getMiddleOfText().x, dal->getMiddleOfText().y,dal->getMiddleOfText().z),
                            Py_BuildValue("(iddd)", 13, dal->getExtensionPoint1().x, dal->getExtensionPoint1().y, dal->getExtensionPoint1().z),
                            Py_BuildValue("(iddd)", 14, dal->getExtensionPoint2().x, dal->getExtensionPoint2().y, dal->getExtensionPoint2().z),
                            Py_BuildValue("(id)", 41, dal->getLineSpacingFactor()),
                            Py_BuildValue("(iddd)", 210, 0.0, 0.0, 1.0)
                        );
                    }
                    break;
                    case RS2::EntityDimAngular:
                    {
                        RS_DimAngular* da = (RS_DimAngular*)e;
                        return Py_BuildValue("[oooooooooooooooooooo]",
                            Py_BuildValue("(is)", 0, "DIMENSION"),
                            Py_BuildValue("(is)", -1, ename),
                            Py_BuildValue("(ii)", 330, da->getParent()->getId()),
                            Py_BuildValue("(is)", 5, RS_SCRIPTINGAPI->getEntityHndl(da->getId())),
                            Py_BuildValue("(is)", 6, RS_FilterDXFRW::lineTypeToName(pen.getLineType()).toStdString()),
                            Py_BuildValue("(ii)", 62, RS_FilterDXFRW::colorToNumber(color, &exact_rgb)),
                            width < 0 ? Py_BuildValue("(ii)", 48, width) : Py_BuildValue("(id)", 48, double(width) / 100.0),
                            Py_BuildValue("(is)", 100, "AcDbEntity"),
                            Py_BuildValue("(ii)", 67, 0),
                            Py_BuildValue("(is)", 100, "Model"),
                            Py_BuildValue("(is)", 8, da->getLayer()->getName().toStdString()),
                            Py_BuildValue("(is)", 100, "AcDb3PointAngularDimension"),
                            Py_BuildValue("(iddd)", 10, da->getDefinitionPoint().x, da->getDefinitionPoint().y,da->getDefinitionPoint().z),
                            Py_BuildValue("(iddd)", 11, da->getMiddleOfText().x, da->getMiddleOfText().y,da->getMiddleOfText().z),
                            Py_BuildValue("(iddd)", 13, da->getDefinitionPoint1().x, da->getDefinitionPoint1().y, da->getDefinitionPoint1().z),
                            Py_BuildValue("(iddd)", 14, da->getDefinitionPoint2().x, da->getDefinitionPoint2().y, da->getDefinitionPoint2().z),
                            Py_BuildValue("(iddd)", 13, da->getDefinitionPoint3().x, da->getDefinitionPoint3().y, da->getDefinitionPoint3().z),
                            Py_BuildValue("(iddd)", 14, da->getDefinitionPoint4().x, da->getDefinitionPoint4().y, da->getDefinitionPoint4().z),
                            Py_BuildValue("(id)", 41, da->getLineSpacingFactor()),
                            Py_BuildValue("(iddd)", 210, 0.0, 0.0, 1.0)
                        );
                    }
                    break;
                    case RS2::EntityDimLinear:
                    {
                        RS_DimLinear* d = (RS_DimLinear*)e;
                        return Py_BuildValue("[oooooooooooooooooooo]",
                            Py_BuildValue("(is)", 0, "DIMENSION"),
                            Py_BuildValue("(is)", -1, ename),
                            Py_BuildValue("(ii)", 330, d->getParent()->getId()),
                            Py_BuildValue("(is)", 5, RS_SCRIPTINGAPI->getEntityHndl(d->getId())),
                            Py_BuildValue("(is)", 6, RS_FilterDXFRW::lineTypeToName(pen.getLineType()).toStdString()),
                            Py_BuildValue("(ii)", 62, RS_FilterDXFRW::colorToNumber(color, &exact_rgb)),
                            width < 0 ? Py_BuildValue("(ii)", 48, width) : Py_BuildValue("(id)", 48, double(width) / 100.0),
                            Py_BuildValue("(is)", 100, "AcDbEntity"),
                            Py_BuildValue("(ii)", 67, 0),
                            Py_BuildValue("(is)", 100, "Model"),
                            Py_BuildValue("(is)", 8, d->getLayer()->getName().toStdString()),
                            Py_BuildValue("(is)", 100, "AcDbAlignedDimension"),
                            Py_BuildValue("(iddd)", 10, d->getDefinitionPoint().x, d->getDefinitionPoint().y,d->getDefinitionPoint().z),
                            Py_BuildValue("(iddd)", 11, d->getMiddleOfText().x, d->getMiddleOfText().y, d->getMiddleOfText().z),
                            Py_BuildValue("(iddd)", 13, d->getExtensionPoint1().x, d->getExtensionPoint1().y, d->getExtensionPoint1().z),
                            Py_BuildValue("(iddd)", 14, d->getExtensionPoint2().x, d->getExtensionPoint2().y, d->getExtensionPoint2().z),
                            Py_BuildValue("(id)", 50, d->getRadius()),
                            Py_BuildValue("(id)", 41, d->getLineSpacingFactor()),
                            Py_BuildValue("(is)", 100, "AcDbRotatedDimension"),
                            Py_BuildValue("(iddd)", 210, 0.0, 0.0, 1.0)
                        );
                    }
                    break;
                    case RS2::EntityDimRadial:
                    {
                        RS_DimRadial* dr = (RS_DimRadial*)e;
                        return Py_BuildValue("[ooooooooooooooooo]",
                            Py_BuildValue("(is)", 0, "DIMENSION"),
                            Py_BuildValue("(is)", -1, ename),
                            Py_BuildValue("(ii)", 330, dr->getParent()->getId()),
                            Py_BuildValue("(is)", 5, RS_SCRIPTINGAPI->getEntityHndl(dr->getId())),
                            Py_BuildValue("(is)", 6, RS_FilterDXFRW::lineTypeToName(pen.getLineType()).toStdString()),
                            Py_BuildValue("(ii)", 62, RS_FilterDXFRW::colorToNumber(color, &exact_rgb)),
                            width < 0 ? Py_BuildValue("(ii)", 48, width) : Py_BuildValue("(id)", 48, double(width) / 100.0),
                            Py_BuildValue("(is)", 100, "AcDbEntity"),
                            Py_BuildValue("(ii)", 67, 0),
                            Py_BuildValue("(is)", 100, "Model"),
                            Py_BuildValue("(is)", 8, dr->getLayer()->getName().toStdString()),
                            Py_BuildValue("(is)", 100, "AcDbRadialDimension"),
                            Py_BuildValue("(iddd)", 10, dr->getDefinitionPoint().x, dr->getDefinitionPoint().y, dr->getDefinitionPoint().z),
                            Py_BuildValue("(iddd)", 11, dr->getMiddleOfText().x, dr->getMiddleOfText().y, dr->getMiddleOfText().z),
                            Py_BuildValue("(id)", 40, dr->getRadius()),
                            Py_BuildValue("(id)", 41, dr->getLineSpacingFactor()),
                            Py_BuildValue("(iddd)", 210, 0.0, 0.0, 1.0)
                        );
                    }
                    break;
                    case RS2::EntityDimDiametric:
                    {
                        RS_DimDiametric* dd = (RS_DimDiametric*)e;
                        return Py_BuildValue("[ooooooooooooooooo]",
                            Py_BuildValue("(is)", 0, "DIMENSION"),
                            Py_BuildValue("(is)", -1, ename),
                            Py_BuildValue("(ii)", 330, dd->getParent()->getId()),
                            Py_BuildValue("(is)", 5, RS_SCRIPTINGAPI->getEntityHndl(dd->getId())),
                            Py_BuildValue("(is)", 6, RS_FilterDXFRW::lineTypeToName(pen.getLineType()).toStdString()),
                            Py_BuildValue("(ii)", 62, RS_FilterDXFRW::colorToNumber(color, &exact_rgb)),
                            width < 0 ? Py_BuildValue("(ii)", 48, width) : Py_BuildValue("(id)", 48, double(width) / 100.0),
                            Py_BuildValue("(is)", 100, "AcDbEntity"),
                            Py_BuildValue("(ii)", 67, 0),
                            Py_BuildValue("(is)", 100, "Model"),
                            Py_BuildValue("(is)", 8, dd->getLayer()->getName().toStdString()),
                            Py_BuildValue("(is)", 100, "AcDbRadialDimension"),
                            Py_BuildValue("(iddd)", 10, dd->getDefinitionPoint().x, dd->getDefinitionPoint().y, dd->getDefinitionPoint().z),
                            Py_BuildValue("(iddd)", 11, dd->getMiddleOfText().x, dd->getMiddleOfText().y, dd->getMiddleOfText().z),
                            Py_BuildValue("(id)", 40, dd->getRadius()),
                            Py_BuildValue("(id)", 41, dd->getLineSpacingFactor()),
                            Py_BuildValue("(iddd)", 210, 0.0, 0.0, 1.0)
                        );

                    }
                    break;
                    case RS2::EntityInsert:
                    {
                        RS_Insert* i = (RS_Insert*)e;
                        return Py_BuildValue("[ooooooooooooooooooooo]",
                            Py_BuildValue("(is)", 0, "INSERT"),
                            Py_BuildValue("(is)", -1, ename),
                            Py_BuildValue("(ii)", 330, i->getParent()->getId()),
                            Py_BuildValue("(is)", 5, RS_SCRIPTINGAPI->getEntityHndl(i->getId())),
                            Py_BuildValue("(is)", 6, RS_FilterDXFRW::lineTypeToName(pen.getLineType()).toStdString()),
                            Py_BuildValue("(ii)", 62, RS_FilterDXFRW::colorToNumber(color, &exact_rgb)),
                            width < 0 ? Py_BuildValue("(ii)", 48, width) : Py_BuildValue("(id)", 48, double(width) / 100.0),
                            Py_BuildValue("(is)", 100, "AcDbEntity"),
                            Py_BuildValue("(ii)", 67, 0),
                            Py_BuildValue("(is)", 100, "Model"),
                            Py_BuildValue("(is)", 8, i->getLayer()->getName().toStdString()),
                            Py_BuildValue("(is)", 100, "AcDbBlockReference"),
                            Py_BuildValue("(iddd)", 10, i->getInsertionPoint().x, i->getInsertionPoint().y, i->getInsertionPoint().z),
                            Py_BuildValue("(id)", 41, i->getScale().x),
                            Py_BuildValue("(id)", 42, i->getScale().y),
                            Py_BuildValue("(id)", 50, i->getRadius()),
                            Py_BuildValue("(ii)", 70, i->getCols()),
                            Py_BuildValue("(ii)", 71, i->getRows()),
                            Py_BuildValue("(id)", 44, i->getSpacing().y),
                            Py_BuildValue("(id)", 45, i->getSpacing().x),
                            Py_BuildValue("(iddd)", 210, 0.0, 0.0, 1.0)
                        );
                    }
                    break;
                    case RS2::EntityMText:
                    {
                        RS_MText* t = (RS_MText*)e;
                        RS_MTextData::MTextDrawingDirection dir = t->getDrawingDirection();
                        unsigned int dxfDir = 5;

                        switch(dir)
                        {
                            case RS_MTextData::MTextDrawingDirection::LeftToRight:
                                dxfDir = 1;
                                break;
                            case RS_MTextData::MTextDrawingDirection::RightToLeft:
                                dxfDir = 2;
                                break;
                            case RS_MTextData::MTextDrawingDirection::TopToBottom:
                                dxfDir = 3;
                                break;
                            default:
                                dxfDir = 5;
                        }

                        RS_MTextData::MTextLineSpacingStyle style = t->getLineSpacingStyle();
                        unsigned int styleDxf = 2;

                        if (style == RS_MTextData::MTextLineSpacingStyle::AtLeast)
                        {
                            styleDxf = 1;
                        }

                        return Py_BuildValue("[ooooooooooooooooooooooo]",
                            Py_BuildValue("(is)", 0, "MTEXT"),
                            Py_BuildValue("(is)", -1, ename),
                            Py_BuildValue("(ii)", 330, t->getParent()->getId()),
                            Py_BuildValue("(is)", 5, RS_SCRIPTINGAPI->getEntityHndl(t->getId())),
                            Py_BuildValue("(is)", 6, RS_FilterDXFRW::lineTypeToName(pen.getLineType()).toStdString()),
                            Py_BuildValue("(ii)", 62, RS_FilterDXFRW::colorToNumber(color, &exact_rgb)),
                            width < 0 ? Py_BuildValue("(ii)", 48, width) : Py_BuildValue("(id)", 48, double(width) / 100.0),
                            Py_BuildValue("(is)", 100, "AcDbEntity"),
                            Py_BuildValue("(ii)", 67, 0),
                            Py_BuildValue("(is)", 100, "Model"),
                            Py_BuildValue("(is)", 8, t->getLayer()->getName().toStdString()),
                            Py_BuildValue("(is)", 100, "AcDbMText"),
                            Py_BuildValue("(iddd)", 10, t->getInsertionPoint().x, t->getInsertionPoint().y, t->getInsertionPoint().z),
                            Py_BuildValue("(id)", 40, t->getUsedTextHeight()),
                            Py_BuildValue("(id)", 41, t->getWidth()),
                            Py_BuildValue("(ii)", 71, t->getAlignment()),
                            Py_BuildValue("(id)", 72, dxfDir),
                            Py_BuildValue("(is)", 1, qUtf8Printable(t->getText())),
                            Py_BuildValue("(id)", 43, t->getHeight()),
                            Py_BuildValue("(id)", 50, t->getAngle()),
                            Py_BuildValue("(ii)", 73, styleDxf),
                            Py_BuildValue("(id)", 44, t->getLineSpacingFactor()),
                            Py_BuildValue("(iddd)", 210, 0.0, 0.0, 1.0)
                        );
                    }
                    break;
                    case RS2::EntityText:
                    {
                        RS_Text* t = (RS_Text*)e;
                        return Py_BuildValue("[oooooooooooooooooo]",
                            Py_BuildValue("(is)", 0, "TEXT"),
                            Py_BuildValue("(is)", -1, ename),
                            Py_BuildValue("(ii)", 330, t->getParent()->getId()),
                            Py_BuildValue("(is)", 5, RS_SCRIPTINGAPI->getEntityHndl(t->getId())),
                            Py_BuildValue("(is)", 6, RS_FilterDXFRW::lineTypeToName(pen.getLineType()).toStdString()),
                            Py_BuildValue("(ii)", 62, RS_FilterDXFRW::colorToNumber(color, &exact_rgb)),
                            width < 0 ? Py_BuildValue("(ii)", 48, width) : Py_BuildValue("(id)", 48, double(width) / 100.0),
                            Py_BuildValue("(is)", 100, "AcDbEntity"),
                            Py_BuildValue("(ii)", 67, 0),
                            Py_BuildValue("(is)", 100, "Model"),
                            Py_BuildValue("(is)", 8, t->getLayer()->getName().toStdString()),
                            Py_BuildValue("(is)", 100, "AcDbText"),
                            Py_BuildValue("(iddd)", 10, t->getInsertionPoint().x, t->getInsertionPoint().y, t->getInsertionPoint().z),
                            Py_BuildValue("(id)", 40, t->getUsedTextHeight()),
                            Py_BuildValue("(is)", 1, qUtf8Printable(t->getText())),
                            Py_BuildValue("(id)", 50, t->getAngle()),
                            Py_BuildValue("(is)", 7, qUtf8Printable(t->getStyle())),
                            Py_BuildValue("(iddd)", 210, 0.0, 0.0, 1.0)
                        );
                    }
                    break;
                    case RS2::EntityHatch:
                    {
                        RS_Hatch* h = (RS_Hatch*)e;
                        return h->isSolid() ?
                        Py_BuildValue("[ooooooooooooooooo]",
                            Py_BuildValue("(is)", 0, "HATCH"),
                            Py_BuildValue("(is)", -1, ename),
                            Py_BuildValue("(ii)", 330, h->getParent()->getId()),
                            Py_BuildValue("(is)", 5, RS_SCRIPTINGAPI->getEntityHndl(h->getId())),
                            Py_BuildValue("(is)", 6, RS_FilterDXFRW::lineTypeToName(pen.getLineType()).toStdString()),
                            Py_BuildValue("(ii)", 62, RS_FilterDXFRW::colorToNumber(color, &exact_rgb)),
                            width < 0 ? Py_BuildValue("(ii)", 48, width) : Py_BuildValue("(id)", 48, double(width) / 100.0),
                            Py_BuildValue("(is)", 100, "AcDbEntity"),
                            Py_BuildValue("(ii)", 67, 0),
                            Py_BuildValue("(is)", 100, "Model"),
                            Py_BuildValue("(is)", 8, h->getLayer()->getName().toStdString()),
                            Py_BuildValue("(is)", 100, "AcDbHatch"),
                            Py_BuildValue("(iddd)", 10, 0.0, 0.0, 0.0),
                            Py_BuildValue("(iddd)", 210, 0.0, 0.0, 1.0),
                            Py_BuildValue("(is)", 2, qUtf8Printable(h->getPattern())),
                            Py_BuildValue("(ii)", 70, 1)
                        )

                        : Py_BuildValue("[ooooooooooooooooooo]",
                            Py_BuildValue("(is)", 0, "HATCH"),
                            Py_BuildValue("(is)", -1, ename),
                            Py_BuildValue("(ii)", 330, h->getParent()->getId()),
                            Py_BuildValue("(is)", 5, RS_SCRIPTINGAPI->getEntityHndl(h->getId())),
                            Py_BuildValue("(is)", 6, RS_FilterDXFRW::lineTypeToName(pen.getLineType()).toStdString()),
                            Py_BuildValue("(ii)", 62, RS_FilterDXFRW::colorToNumber(color, &exact_rgb)),
                            width < 0 ? Py_BuildValue("(ii)", 48, width) : Py_BuildValue("(id)", 48, double(width) / 100.0),
                            Py_BuildValue("(is)", 100, "AcDbEntity"),
                            Py_BuildValue("(ii)", 67, 0),
                            Py_BuildValue("(is)", 100, "Model"),
                            Py_BuildValue("(is)", 8, h->getLayer()->getName().toStdString()),
                            Py_BuildValue("(is)", 100, "AcDbHatch"),
                            Py_BuildValue("(iddd)", 10, 0.0, 0.0, 0.0),
                            Py_BuildValue("(iddd)", 210, 0.0, 0.0, 1.0),
                            Py_BuildValue("(is)", 2, qUtf8Printable(h->getPattern())),
                            Py_BuildValue("(id)", 52, h->getAngle()),
                            Py_BuildValue("(id)", 41, h->getScale()),
                            Py_BuildValue("(ii)", 70, 0)
                        );

                    }
                    break;
                    case RS2::EntitySolid:
                    {
                        RS_Solid* sol = (RS_Solid*)e;
                        return sol->isTriangle() ?
                        Py_BuildValue("[oooooooooooooooo]",
                            Py_BuildValue("(is)", 0, "SOLID"),
                            Py_BuildValue("(is)", -1, ename),
                            Py_BuildValue("(ii)", 330, sol->getParent()->getId()),
                            Py_BuildValue("(is)", 5, RS_SCRIPTINGAPI->getEntityHndl(sol->getId())),
                            Py_BuildValue("(is)", 6, RS_FilterDXFRW::lineTypeToName(pen.getLineType()).toStdString()),
                            Py_BuildValue("(ii)", 62, RS_FilterDXFRW::colorToNumber(color, &exact_rgb)),
                            width < 0 ? Py_BuildValue("(ii)", 48, width) : Py_BuildValue("(id)", 48, double(width) / 100.0),
                            Py_BuildValue("(is)", 100, "AcDbEntity"),
                            Py_BuildValue("(ii)", 67, 0),
                            Py_BuildValue("(is)", 100, "Model"),
                            Py_BuildValue("(is)", 8, sol->getLayer()->getName().toStdString()),
                            Py_BuildValue("(is)", 100, "AcDbTrace"),
                            Py_BuildValue("(iddd)", 10, sol->getCorner(0).x, sol->getCorner(0).y, sol->getCorner(0).z),
                            Py_BuildValue("(iddd)", 11, sol->getCorner(1).x, sol->getCorner(1).y, sol->getCorner(1).z),
                            Py_BuildValue("(iddd)", 12, sol->getCorner(2).x, sol->getCorner(2).y, sol->getCorner(2).z),
                            Py_BuildValue("(iddd)", 210, 0.0, 0.0, 1.0)
                        )

                        : Py_BuildValue("[ooooooooooooooooo]",
                            Py_BuildValue("(is)", 0, "SOLID"),
                            Py_BuildValue("(is)", -1, ename),
                            Py_BuildValue("(ii)", 330, sol->getParent()->getId()),
                            Py_BuildValue("(is)", 5, RS_SCRIPTINGAPI->getEntityHndl(sol->getId())),
                            Py_BuildValue("(is)", 6, RS_FilterDXFRW::lineTypeToName(pen.getLineType()).toStdString()),
                            Py_BuildValue("(ii)", 62, RS_FilterDXFRW::colorToNumber(color, &exact_rgb)),
                            width < 0 ? Py_BuildValue("(ii)", 48, width) : Py_BuildValue("(id)", 48, double(width) / 100.0),
                            Py_BuildValue("(is)", 100, "AcDbEntity"),
                            Py_BuildValue("(ii)", 67, 0),
                            Py_BuildValue("(is)", 100, "Model"),
                            Py_BuildValue("(is)", 8, sol->getLayer()->getName().toStdString()),
                            Py_BuildValue("(is)", 100, "AcDbTrace"),
                            Py_BuildValue("(iddd)", 10, sol->getCorner(0).x, sol->getCorner(0).y, sol->getCorner(0).z),
                            Py_BuildValue("(iddd)", 11, sol->getCorner(1).x, sol->getCorner(1).y, sol->getCorner(1).z),
                            Py_BuildValue("(iddd)", 12, sol->getCorner(2).x, sol->getCorner(2).y, sol->getCorner(2).z),
                            Py_BuildValue("(iddd)", 13, sol->getCorner(3).x, sol->getCorner(3).y, sol->getCorner(3).z),
                            Py_BuildValue("(iddd)", 210, 0.0, 0.0, 1.0)
                        );
                    }
                    break;
                    case RS2::EntitySpline:
                    {
                        RS_Spline* spl = (RS_Spline*)e;
                        return Py_BuildValue("[oooooooooooooooooo]",
                            Py_BuildValue("(is)", 0, "SPLINE"),
                            Py_BuildValue("(is)", -1, ename),
                            Py_BuildValue("(ii)", 330, spl->getParent()->getId()),
                            Py_BuildValue("(is)", 5, RS_SCRIPTINGAPI->getEntityHndl(spl->getId())),
                            Py_BuildValue("(is)", 6, RS_FilterDXFRW::lineTypeToName(pen.getLineType()).toStdString()),
                            Py_BuildValue("(ii)", 62, RS_FilterDXFRW::colorToNumber(color, &exact_rgb)),
                            width < 0 ? Py_BuildValue("(ii)", 48, width) : Py_BuildValue("(id)", 48, double(width) / 100.0),
                            Py_BuildValue("(is)", 100, "AcDbEntity"),
                            Py_BuildValue("(ii)", 67, 0),
                            Py_BuildValue("(is)", 100, "Model"),
                            Py_BuildValue("(is)", 8, spl->getLayer()->getName().toStdString()),
                            Py_BuildValue("(is)", 100, "AcDbSpline"),
                            spl->isClosed() ? Py_BuildValue("(ii)", 70, 1) : Py_BuildValue("(ii)", 70, 8),
                            Py_BuildValue("(ii)", 71, spl->getDegree()),
                            Py_BuildValue("(ii)", 72, spl->getNumberOfKnots()),
                            Py_BuildValue("(ii)", 73, static_cast<int>(spl->getNumberOfControlPoints())),
                            Py_BuildValue("(ii)", 74, 0),
                            Py_BuildValue("(iddd)", 210, 0.0, 0.0, 1.0)
                        );
                    }
                    break;
                    case RS2::EntityImage:
                    {
                        RS_Image* img = (RS_Image*)e;
                        RS_VectorSolutions pnts = img->getCorners();

                        PyObject* list = PyList_New(pnts.size() + 23);
                        PyList_SET_ITEM(list, 0, Py_BuildValue("(is)", 0, "IMAGE"));
                        PyList_SET_ITEM(list, 1, Py_BuildValue("(is)", -1, ename));
                        PyList_SET_ITEM(list, 2, Py_BuildValue("(ii)", 330, img->getParent()->getId()));
                        PyList_SET_ITEM(list, 3, Py_BuildValue("(is)", 5, RS_SCRIPTINGAPI->getEntityHndl(img->getId())));
                        PyList_SET_ITEM(list, 4, Py_BuildValue("(is)", 6, RS_FilterDXFRW::lineTypeToName(pen.getLineType()).toStdString()));
                        PyList_SET_ITEM(list, 5, Py_BuildValue("(ii)", 62, RS_FilterDXFRW::colorToNumber(color, &exact_rgb)));
                        PyList_SET_ITEM(list, 6, width < 0 ? Py_BuildValue("(ii)", 48, width) : Py_BuildValue("(id)", 48, double(width) / 100.0));
                        PyList_SET_ITEM(list, 7, Py_BuildValue("(is)", 100, "AcDbEntity"));
                        PyList_SET_ITEM(list, 8, Py_BuildValue("(ii)", 67, 0));
                        PyList_SET_ITEM(list, 9, Py_BuildValue("(is)", 100, "Model"));
                        PyList_SET_ITEM(list, 10, Py_BuildValue("(is)", 8, img->getLayer()->getName().toStdString()));
                        PyList_SET_ITEM(list, 11, Py_BuildValue("(iddd)", 10, img->getInsertionPoint().x, img->getInsertionPoint().y, img->getInsertionPoint().z));
                        PyList_SET_ITEM(list, 12, Py_BuildValue("(iddd)", 11, img->getUVector().x, img->getUVector().y, img->getUVector().z));
                        PyList_SET_ITEM(list, 13, Py_BuildValue("(iddd)", 12, img->getVVector().x, img->getVVector().y, img->getVVector().z));
                        PyList_SET_ITEM(list, 14, Py_BuildValue("(iii)", 13, img->getWidth(), img->getHeight()));
                        PyList_SET_ITEM(list, 15, Py_BuildValue("(is)", 340, qUtf8Printable(img->getFile())));
                        PyList_SET_ITEM(list, 16, Py_BuildValue("(ii)", 70, 1));
                        PyList_SET_ITEM(list, 17, Py_BuildValue("(ii)", 280, 0));
                        PyList_SET_ITEM(list, 18, Py_BuildValue("(ii)", 281, img->getBrightness()));
                        PyList_SET_ITEM(list, 19, Py_BuildValue("(ii)", 282, img->getContrast()));
                        PyList_SET_ITEM(list, 20, Py_BuildValue("(ii)", 283, img->getFade()));
                        PyList_SET_ITEM(list, 21, Py_BuildValue("(ii)", 71, 1));
                        int n = 22;

                        for (auto &v : img->getCorners())
                        {
                            PyList_SET_ITEM(list, n++, Py_BuildValue("(iddd)", 14, v.x, v.y, v.z));
                        }

                        PyList_SET_ITEM(list, n++, Py_BuildValue("(iddd)", 210, 0.0, 0.0, 1.0));

                        return list;
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
