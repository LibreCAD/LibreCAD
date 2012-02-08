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
#include "rs_debug.h"



RS_ActionInfoArea::RS_ActionInfoArea(RS_EntityContainer& container,
                                     RS_GraphicView& graphicView)
    :RS_PreviewActionInterface("Info Area",
                               container, graphicView) {}


QAction* RS_ActionInfoArea::createGUIAction(RS2::ActionType /*type*/, QObject* /*parent*/) {
    /*    QAction* action = new QAction(tr("Polygonal Area"),
                                  tr("&Polygonal Area"),
                                  QKeySequence(), NULL); */
    QAction* action = new QAction(tr("Polygonal &Area"), NULL);
    //action->zetStatusTip(tr("Measures the area of a polygon"));
    action->setIcon(QIcon(":/extui/infoarea.png"));
    return action;
}


void RS_ActionInfoArea::init(int status) {
    RS_ActionInterface::init(status);

    if(status==SetFirstPoint){
        deletePreview();
        ia.reset();
    }

    //RS_DEBUG->print( "RS_ActionInfoArea::init: %d" ,status );
}



void RS_ActionInfoArea::trigger() {

    RS_DEBUG->print("RS_ActionInfoArea::trigger()");
    display();

    init(SetFirstPoint);
}

//todo: we regenerate the whole preview, it's possible to generate needed lines only
/** display area circumference and preview of polygon **/
void RS_ActionInfoArea::display() {
    deletePreview();
    if(ia.size() < 1) {
        return;
    }
    switch(ia.size()){
    case 2:
        preview->addEntity(new RS_Line(preview,ia.at(0),ia.at(1)));
        break;
    default:
        for(int i=0;i<ia.size();i++){
            preview->addEntity(new RS_Line(preview,ia.at(i),ia.at((i+1) % ia.size())));
        }
        double area = ia.getArea();
        double circ = ia.getCircumference();

        RS_DIALOGFACTORY->commandMessage(tr("Circumference: %1").arg(circ));
        RS_DIALOGFACTORY->commandMessage(tr("Area: %1").arg(area));
        break;
    }
    drawPreview();

}


void RS_ActionInfoArea::mouseMoveEvent(QMouseEvent* e) {
    //RS_DEBUG->print("RS_ActionInfoArea::mouseMoveEvent begin");

    RS_Vector mouse = snapPoint(e);
    if ( getStatus()==SetNextPoint) {
        ia.push_back(mouse);
        display();
        ia.pop_back();
    }

    //RS_DEBUG->print("RS_ActionInfoArea::mouseMoveEvent end");
}



void RS_ActionInfoArea::mouseReleaseEvent(QMouseEvent* e) {
    if (e->button()==Qt::LeftButton) {
        RS_CoordinateEvent ce(snapPoint(e));
        coordinateEvent(&ce);
    } else if (e->button()==Qt::RightButton) {

        init(getStatus()-1);
    }
}



void RS_ActionInfoArea::coordinateEvent(RS_CoordinateEvent* e) {
    if (e==NULL) {
        return;
    }

    RS_Vector mouse = e->getCoordinate();
    if(ia.duplicated(mouse)) {
        ia.push_back(mouse);
        RS_DIALOGFACTORY->commandMessage(tr("Closing Point: %1/%2")
                                         .arg(mouse.x).arg(mouse.y));
        trigger();
        return;
    }
    graphicView->moveRelativeZero(mouse);

    ia.push_back(mouse);
    RS_DIALOGFACTORY->commandMessage(tr("Point: %1/%2")
                                     .arg(mouse.x).arg(mouse.y));
    switch (getStatus()) {
    case SetFirstPoint:
        setStatus(SetNextPoint);
        break;
    case SetNextPoint:
        display();
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
                    tr("Cancel"));
        break;
    default:
        RS_DIALOGFACTORY->updateMouseWidget("", "");
        break;
    }
}



void RS_ActionInfoArea::updateMouseCursor() {
    graphicView->setMouseCursor(RS2::CadCursor);
}

// EOF
