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

#include "rs_actioninfoarea.h"

#include <QAction>
#include "rs_dialogfactory.h"
#include "rs_graphicview.h"



RS_ActionInfoArea::RS_ActionInfoArea(RS_EntityContainer& container,
                                     RS_GraphicView& graphicView)
        :RS_PreviewActionInterface("Info Area",
                           container, graphicView) {}


QAction* RS_ActionInfoArea::createGUIAction(RS2::ActionType /*type*/, QObject* /*parent*/) {
/*    QAction* action = new QAction(tr("Polygonal Area"),
                                  tr("&Polygonal Area"),
                                  QKeySequence(), NULL); */
    QAction* action = new QAction(tr("Polygonal Area"), NULL);
    //action->zetStatusTip(tr("Measures the area of a polygon"));
    action->setIcon(QIcon(":/extui/infoarea.png"));
    return action;
}


void RS_ActionInfoArea::init(int status) {
    RS_ActionInterface::init(status);

    currentLine = NULL;
    closingLine = NULL;

    //std::cout << "RS_ActionInfoArea::init: " << status << "\n";
}



void RS_ActionInfoArea::trigger() {

    RS_DEBUG->print("RS_ActionInfoArea::trigger()");
    if (ia.isValid()) {
        ia.close();
        ia.calculate();
        double area = ia.getArea();
        double circ = ia.getCircumference();

        RS_DEBUG->print("RS_ActionInfoArea::trigger: area: %f", area);
        RS_DIALOGFACTORY->commandMessage(tr("Area: %1").arg(area));
        RS_DIALOGFACTORY->commandMessage(tr("Circumference: %1").arg(circ));
    }

    ia.reset();

    /*
    if (point1.valid && point2.valid) {
        double dist = point1.distanceTo(point2);
        QString str;
        str.sprintf("%.6f", dist);
        RS_DIALOGFACTORY->commandMessage(tr("Distance: %1").arg(str));
}
    */
}



void RS_ActionInfoArea::mouseMoveEvent(QMouseEvent* e) {
    //RS_DEBUG->print("RS_ActionInfoArea::mouseMoveEvent begin");

    if (getStatus()==SetFirstPoint ||
            getStatus()==SetNextPoint) {

        RS_Vector mouse = snapPoint(e);
        if(mouse.valid==false) return;

        switch (getStatus()) {
        case SetFirstPoint:
            break;

        case SetNextPoint:
            if (prev.valid) {
                deletePreview();
                if (currentLine!=NULL) {
                    preview->removeEntity(currentLine);
                    currentLine = NULL;
                }
                if (closingLine!=NULL) {
                    preview->removeEntity(closingLine);
                    closingLine = NULL;
                }

                currentLine = new RS_Line(preview,
                                          RS_LineData(prev,
                                                      mouse));
                preview->addEntity(currentLine);

				if (preview->count()>1) {
                	closingLine = new RS_Line(preview,
                                          RS_LineData(mouse,
                                                      point1));

                	preview->addEntity(closingLine);
				}

                drawPreview();
            }
            break;

        default:
            break;
        }
    }

    //RS_DEBUG->print("RS_ActionInfoArea::mouseMoveEvent end");
}



void RS_ActionInfoArea::mouseReleaseEvent(QMouseEvent* e) {
    if (e->button()==Qt::LeftButton) {
        RS_Vector vp=snapPoint(e);
        if(vp.valid==false) return;
        RS_CoordinateEvent ce(vp);
        coordinateEvent(&ce);
    } else if (e->button()==Qt::RightButton) {
        //deletePreview();

        // close the polygon (preview)
        if (getStatus()==SetNextPoint && prev.valid) {
            deletePreview();

            if (currentLine!=NULL) {
                preview->removeEntity(currentLine);
                currentLine = NULL;
            }
            if (closingLine!=NULL) {
                preview->removeEntity(closingLine);
                closingLine = NULL;
            }

            currentLine = new RS_Line(preview,
                                      RS_LineData(prev,
                                                  point1));

            preview->addEntity(currentLine);

            drawPreview();
        }

        trigger();
        init(getStatus()-1);
    }
}



void RS_ActionInfoArea::coordinateEvent(RS_CoordinateEvent* e) {
    if (e==NULL) {
        return;
    }

    RS_Vector mouse = e->getCoordinate();

    switch (getStatus()) {
    case SetFirstPoint:
        point1 = mouse;

        deletePreview();

        ia.addPoint(mouse);
        RS_DIALOGFACTORY->commandMessage(tr("Point: %1/%2")
                                         .arg(mouse.x).arg(mouse.y));

        graphicView->moveRelativeZero(point1);
        prev = mouse;

        setStatus(SetNextPoint);
        break;

    case SetNextPoint:
        if (point1.valid) {
            //point2 = mouse;
            /*deletePreview();
            */
            ia.addPoint(mouse);
            RS_DIALOGFACTORY->commandMessage(tr("Point: %1/%2")
                                             .arg(mouse.x).arg(mouse.y));

            currentLine = NULL;

            graphicView->moveRelativeZero(mouse);
            prev = mouse;

            // automatically detect that the polyline is now closed
            if (ia.isClosed()) {
                trigger();
                setStatus(SetFirstPoint);
            }
            //trigger();
            //setStatus(SetFirstPoint);
        }
        break;

    default:
        break;
    }
}


void RS_ActionInfoArea::updateMouseButtonHints() {
    switch (getStatus()) {
    case SetFirstPoint:
        RS_DIALOGFACTORY->updateMouseWidget(
            tr("Specify first point of polygon"),
            tr("Cancel"));
        break;
    case SetNextPoint:
        RS_DIALOGFACTORY->updateMouseWidget(
            tr("Specify next point of polygon"),
            tr("Terminate"));
        break;
    default:
        RS_DIALOGFACTORY->updateMouseWidget("", "");
        break;
    }
}



void RS_ActionInfoArea::updateMouseCursor() {
    graphicView->setMouseCursor(RS2::CadCursor);
}



void RS_ActionInfoArea::updateToolBar() {
    switch (getStatus()) {
    case SetFirstPoint:
    case SetNextPoint:
        RS_DIALOGFACTORY->requestToolBar(RS2::ToolBarSnap);
        break;
    default:
        RS_DIALOGFACTORY->requestToolBar(RS2::ToolBarInfo);
        break;
    }
}


// EOF
