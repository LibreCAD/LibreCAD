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

#include "rs_actioninfoangle.h"

#include "rs_information.h"
#include "rs_snapper.h"



RS_ActionInfoAngle::RS_ActionInfoAngle(RS_EntityContainer& container,
                                       RS_GraphicView& graphicView)
        :RS_PreviewActionInterface("Info Angle",
                           container, graphicView) {}


QAction* RS_ActionInfoAngle::createGUIAction(RS2::ActionType /*type*/, QObject* /*parent*/) {
/* RVT_PORT    QAction* action = new QAction(tr("Angle between two lines"),
                                  tr("&Angle between two lines"),
                                  QKeySequence(), NULL); */

    QAction* action = new QAction(tr("Angle between two lines"), NULL);
    //action->zetStatusTip(tr("Measures the angle between two lines"));
	action->setIcon(QIcon(":/extui/infoangle.png"));
    return action;
}


void RS_ActionInfoAngle::init(int status) {
    RS_ActionInterface::init(status);

}



void RS_ActionInfoAngle::trigger() {

    RS_DEBUG->print("RS_ActionInfoAngle::trigger()");

    if (entity1!=NULL && entity2!=NULL) {
        RS_VectorSolutions sol =
            RS_Information::getIntersection(entity1, entity2, false);

        if (sol.hasValid()) {
            intersection = sol.get(0);

            if (intersection.valid && point1.valid && point2.valid) {
                double angle1 = intersection.angleTo(point1);
                double angle2 = intersection.angleTo(point2);
                double angle = fabs(angle2-angle1);

                QString str;
                str.sprintf("%.6f", RS_Math::rad2deg(angle));
                RS_DIALOGFACTORY->commandMessage(tr("Angle: %1%2")
                                                 .arg(str).arg(QChar(0xB0)));
            }
        } else {
            RS_DIALOGFACTORY->commandMessage(tr("Lines are parallel"));
        }
    }
}



void RS_ActionInfoAngle::mouseMoveEvent(RS_MouseEvent* /*e*/) {
    RS_DEBUG->print("RS_ActionInfoAngle::mouseMoveEvent begin");

    switch (getStatus()) {
    case SetEntity1:
        break;

    case SetEntity2:
        break;

    default:
        break;
    }

    RS_DEBUG->print("RS_ActionInfoAngle::mouseMoveEvent end");
}



void RS_ActionInfoAngle::mouseReleaseEvent(RS_MouseEvent* e) {
    if (RS2::qtToRsButtonState(e->button())==RS2::LeftButton) {

        RS_Vector mouse(graphicView->toGraphX(e->x()),
                        graphicView->toGraphY(e->y()));

        switch (getStatus()) {
        case SetEntity1:
            entity1 = catchEntity(e);
            if (entity1!=NULL && entity1->rtti()==RS2::EntityLine) {
                point1 = entity1->getNearestPointOnEntity(mouse);
                setStatus(SetEntity2);
            }
            break;

        case SetEntity2:
            entity2 = catchEntity(e);
            if (entity2!=NULL && entity2->rtti()==RS2::EntityLine) {
                point2 = entity2->getNearestPointOnEntity(mouse);
                trigger();
                setStatus(SetEntity1);
            }
            break;

        default:
            break;
        }
    } else if (RS2::qtToRsButtonState(e->button())==RS2::RightButton) {
        deletePreview();
        init(getStatus()-1);
    }
}



void RS_ActionInfoAngle::updateMouseButtonHints() {
    switch (getStatus()) {
    case SetEntity1:
        RS_DIALOGFACTORY->updateMouseWidget(
            tr("Specify first line"),
            tr("Cancel"));
        break;
    case SetEntity2:
        RS_DIALOGFACTORY->updateMouseWidget(
            tr("Specify second line"),
            tr("Back"));
        break;
    default:
        RS_DIALOGFACTORY->updateMouseWidget("", "");
        break;
    }
}



void RS_ActionInfoAngle::updateMouseCursor() {
    graphicView->setMouseCursor(RS2::CadCursor);
}



void RS_ActionInfoAngle::updateToolBar() {
    switch (getStatus()) {
    case SetEntity1:
    case SetEntity2:
        RS_DIALOGFACTORY->requestToolBar(RS2::ToolBarSnap);
        break;
    default:
        RS_DIALOGFACTORY->requestToolBar(RS2::ToolBarInfo);
        break;
    }
}


// EOF
