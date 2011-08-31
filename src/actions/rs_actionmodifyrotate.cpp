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

#include "rs_actionmodifyrotate.h"

#include <QAction>
#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
//#include "rs_commandevent.h"



RS_ActionModifyRotate::RS_ActionModifyRotate(RS_EntityContainer& container,
        RS_GraphicView& graphicView)
        :RS_PreviewActionInterface("Rotate Entities",
                           container, graphicView) {}


QAction* RS_ActionModifyRotate::createGUIAction(RS2::ActionType /*type*/, QObject* /*parent*/) {
	// tr("Rotate")
    QAction* action = new QAction(tr("&Rotate"), NULL);
	action->setIcon(QIcon(":/extui/modifyrotate.png"));
    //action->zetStatusTip(tr("Rotate Entities"));
    return action;
}

void RS_ActionModifyRotate::init(int status) {
    RS_ActionInterface::init(status);
}



void RS_ActionModifyRotate::trigger() {

    RS_DEBUG->print("RS_ActionModifyRotate::trigger()");

    RS_Modification m(*container, graphicView);
    m.rotate(data);

    RS_DIALOGFACTORY->updateSelectionWidget(container->countSelected());
}



void RS_ActionModifyRotate::mouseMoveEvent(QMouseEvent* e) {
    RS_DEBUG->print("RS_ActionModifyRotate::mouseMoveEvent begin");
        RS_Vector mouse = snapPoint(e);
    switch (getStatus()) {
           case setCenterPoint:
        case setReferencePoint:
                   break;

            case setTargetPoint:
                   data.angle=data.center.angleBetween(referencePoint, mouse);
                   std::cout<<"data.angle= "<<data.angle<<std::endl;
                deletePreview();
                preview->addSelectionFrom(*container);
                preview->rotate(data.center,data.angle);
                drawPreview();
    }

    RS_DEBUG->print("RS_ActionModifyRotate::mouseMoveEvent end");
}



void RS_ActionModifyRotate::mouseReleaseEvent(QMouseEvent* e) {
    if (e->button()==Qt::LeftButton) {
        RS_CoordinateEvent ce(snapPoint(e));
        coordinateEvent(&ce);
    } else if (e->button()==Qt::RightButton) {
        deletePreview();
        init(getStatus()-1);
    }
}



void RS_ActionModifyRotate::coordinateEvent(RS_CoordinateEvent* e) {
    if (e==NULL) {
        return;
    }

    RS_Vector pos = e->getCoordinate();

    switch (getStatus()) {
    case setCenterPoint:
        centerPoint = pos;
        setStatus(setReferencePoint);
        break;
    case setReferencePoint:
        referencePoint = pos;
        setStatus(setTargetPoint);
        break;
    case setTargetPoint:
        targetPoint = pos;
        setStatus(ShowDialog);
        if (RS_DIALOGFACTORY->requestRotateDialog(data)) {
            data.center = centerPoint;
            data.angle = data.center.angleBetween(referencePoint, targetPoint);
            trigger();
            finish();
        }
        break;

    default:
        break;
    }
}



void RS_ActionModifyRotate::updateMouseButtonHints() {
    switch (getStatus()) {
        case setCenterPoint:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Specify rotation center"),
                                            tr("Back"));
        break;

    case setReferencePoint:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Specify reference point"),
                                            tr("Back"));
        break;
        case setTargetPoint:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Specify target point to rotate to"),
                                            tr("Back"));
    default:
        RS_DIALOGFACTORY->updateMouseWidget("", "");
        break;
    }
}



void RS_ActionModifyRotate::updateMouseCursor() {
    graphicView->setMouseCursor(RS2::CadCursor);
}



void RS_ActionModifyRotate::updateToolBar() {
    switch (getStatus()) {
    case setCenterPoint:
        RS_DIALOGFACTORY->requestToolBar(RS2::ToolBarSnap);
        break;
    case ShowDialog:
        RS_DIALOGFACTORY->requestToolBar(RS2::ToolBarModify);
        break;
    default:
        break;
    }
}


// EOF
