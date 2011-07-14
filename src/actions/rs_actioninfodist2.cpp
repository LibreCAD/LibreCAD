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

#include "rs_actioninfodist2.h"

#include <QAction>
#include "rs_dialogfactory.h"
#include "rs_graphicview.h"


RS_ActionInfoDist2::RS_ActionInfoDist2(RS_EntityContainer& container,
                                       RS_GraphicView& graphicView)
        :RS_PreviewActionInterface("Info Dist2",
                           container, graphicView) {}


QAction* RS_ActionInfoDist2::createGUIAction(RS2::ActionType /*type*/, QObject* /*parent*/) {
	// (tr("Distance Entity to Point")
    QAction* action = new QAction(tr("&Distance Entity to Point"), NULL);
    //action->zetStatusTip(tr("Measures the distance between an entity and a point"));
	action->setIcon(QIcon(":/extui/infodist2.png"));
    return action;
}

void RS_ActionInfoDist2::init(int status) {
    RS_ActionInterface::init(status);
}



void RS_ActionInfoDist2::trigger() {

    RS_DEBUG->print("RS_ActionInfoDist2::trigger()");

    if (point.valid && entity!=NULL) {
        double dist = entity->getDistanceToPoint(point);
        QString str;
        str.sprintf("%.6f", dist);
        RS_DIALOGFACTORY->commandMessage(tr("Distance: %1").arg(str));
    }
}



void RS_ActionInfoDist2::mouseMoveEvent(QMouseEvent* e) {
    RS_DEBUG->print("RS_ActionInfoDist2::mouseMoveEvent begin");

    switch (getStatus()) {
    case SetEntity:
        //entity = catchEntity(e);
        break;

    case SetPoint:
        if (entity!=NULL) {
            point = snapPoint(e);
        }
        break;

    default:
        break;
    }

    RS_DEBUG->print("RS_ActionInfoDist2::mouseMoveEvent end");
}



void RS_ActionInfoDist2::mouseReleaseEvent(QMouseEvent* e) {
    if (e->button()==Qt::LeftButton) {

        switch (getStatus()) {
        case SetEntity:
            entity = catchEntity(e);
            if (entity!=NULL) {
                setStatus(SetPoint);
            }
            break;

        case SetPoint: {
                RS_CoordinateEvent ce(snapPoint(e));
                coordinateEvent(&ce);
            }
            break;

        default:
            break;
        }
    } else if (e->button()==Qt::RightButton) {
        deletePreview();
        init(getStatus()-1);
    }
}


void RS_ActionInfoDist2::coordinateEvent(RS_CoordinateEvent* e) {
    if (e==NULL) {
        return;
    }

    if (getStatus()==SetPoint && entity!=NULL) {
        point = e->getCoordinate();
        graphicView->moveRelativeZero(point);
        trigger();
        setStatus(SetEntity);
    }
}



void RS_ActionInfoDist2::updateMouseButtonHints() {
    switch (getStatus()) {
    case SetEntity:
        RS_DIALOGFACTORY->updateMouseWidget(
            tr("Specify entity"),
            tr("Cancel"));
        break;
    case SetPoint:
        RS_DIALOGFACTORY->updateMouseWidget(
            tr("Specify point"),
            tr("Back"));
        break;
    default:
        RS_DIALOGFACTORY->updateMouseWidget("", "");
        break;
    }
}



void RS_ActionInfoDist2::updateMouseCursor() {
    graphicView->setMouseCursor(RS2::CadCursor);
}



void RS_ActionInfoDist2::updateToolBar() {
    switch (getStatus()) {
    case SetPoint:
    case SetEntity:
        RS_DIALOGFACTORY->requestToolBar(RS2::ToolBarSnap);
        break;
    default:
        RS_DIALOGFACTORY->requestToolBar(RS2::ToolBarInfo);
        break;
    }
}


// EOF
