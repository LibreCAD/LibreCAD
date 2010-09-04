/****************************************************************************
**
** This file is part of the CADuntu project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (caduntu@rvt.dds.nl)
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

#include "rs_actionmodifymoverotate.h"

#include "rs_snapper.h"



RS_ActionModifyMoveRotate::RS_ActionModifyMoveRotate(
    RS_EntityContainer& container,
    RS_GraphicView& graphicView)
        :RS_PreviewActionInterface("Move and Rotate Entities",
                           container, graphicView) {
}

QAction* RS_ActionModifyMoveRotate::createGUIAction(RS2::ActionType /*type*/, QObject* /*parent*/) {
	// tr("Move and Rotate")
    QAction* action = new QAction(tr("M&ove and Rotate"), NULL);
	action->setIcon(QIcon(":/extui/modifymoverotate.png"));
	action->setStatusTip(tr("Move and Rotate Entities"));
    return action;
}

void RS_ActionModifyMoveRotate::init(int status) {
    RS_ActionInterface::init(status);
}



void RS_ActionModifyMoveRotate::trigger() {

    RS_DEBUG->print("RS_ActionModifyMoveRotate::trigger()");

    RS_Modification m(*container, graphicView);
    m.moveRotate(data);

    finish();

    RS_DIALOGFACTORY->updateSelectionWidget(container->countSelected());
}



void RS_ActionModifyMoveRotate::mouseMoveEvent(RS_MouseEvent* e) {
    RS_DEBUG->print("RS_ActionModifyMoveRotate::mouseMoveEvent begin");

    if (getStatus()==SetReferencePoint ||
            getStatus()==SetTargetPoint) {

        RS_Vector mouse = snapPoint(e);
        switch (getStatus()) {
        case SetReferencePoint:
            data.referencePoint = mouse;
            break;

        case SetTargetPoint:
            if (data.referencePoint.valid) {
                targetPoint = mouse;
                data.offset = targetPoint-data.referencePoint;

                deletePreview();
                preview->addSelectionFrom(*container);
                preview->rotate(data.referencePoint, data.angle);
                preview->move(data.offset);
                drawPreview();
            }
            break;

        default:
            break;
        }
    }

    RS_DEBUG->print("RS_ActionModifyMoveRotate::mouseMoveEvent end");
}



void RS_ActionModifyMoveRotate::mouseReleaseEvent(RS_MouseEvent* e) {
    if (RS2::qtToRsButtonState(e->button())==RS2::LeftButton) {
        RS_CoordinateEvent ce(snapPoint(e));
        coordinateEvent(&ce);
    } else if (RS2::qtToRsButtonState(e->button())==RS2::RightButton) {
        deletePreview();
        init(getStatus()-1);
    }
}



void RS_ActionModifyMoveRotate::coordinateEvent(RS_CoordinateEvent* e) {
    if (e==NULL) {
        return;
    }

    RS_Vector pos = e->getCoordinate();

    switch (getStatus()) {
    case SetReferencePoint:
        data.referencePoint = pos;
        setStatus(SetTargetPoint);
        break;

    case SetTargetPoint:
        targetPoint = pos;

        setStatus(ShowDialog);
        data.offset = targetPoint - data.referencePoint;
        if (RS_DIALOGFACTORY->requestMoveRotateDialog(data)) {
            trigger();
            //finish();
        }
        break;

    default:
        break;
    }
}


void RS_ActionModifyMoveRotate::commandEvent(RS_CommandEvent* e) {
    RS_String c = e->getCommand().lower();

    if (checkCommand("help", c)) {
        RS_DIALOGFACTORY->commandMessage(msgAvailableCommands()
                                         + getAvailableCommands().join(", "));
        return;
    }

    switch (getStatus()) {
    case SetReferencePoint:
    case SetTargetPoint:
		// RVT_PORT changed from if (c==checkCommand("angle", c)) {
        if (checkCommand("angle", c)) {
            deletePreview();
            lastStatus = (Status)getStatus();
            setStatus(SetAngle);
        }
        break;

    case SetAngle: {
            bool ok;
            double a = RS_Math::eval(c, &ok);
            if (ok==true) {
                data.angle = RS_Math::deg2rad(a);
            } else {
                RS_DIALOGFACTORY->commandMessage(tr("Not a valid expression"));
            }
            RS_DIALOGFACTORY->requestOptions(this, true, true);
            setStatus(lastStatus);
        }
        break;
    }
}



RS_StringList RS_ActionModifyMoveRotate::getAvailableCommands() {
    RS_StringList cmd;

    switch (getStatus()) {
    case SetReferencePoint:
    case SetTargetPoint:
        cmd += command("angle");
        break;

    default:
        break;
    }

    return cmd;
}



void RS_ActionModifyMoveRotate::showOptions() {
    //std::cout << "RS_ActionModifyMoveRotate::showOptions()\n";

    RS_ActionInterface::showOptions();

    RS_DIALOGFACTORY->requestOptions(this, true);
}



void RS_ActionModifyMoveRotate::hideOptions() {
    //std::cout << "RS_ActionModifyMoveRotate::hideOptions()\n";

    RS_ActionInterface::hideOptions();

    RS_DIALOGFACTORY->requestOptions(this, false);
}


void RS_ActionModifyMoveRotate::updateMouseButtonHints() {
    switch (getStatus()) {
    case SetReferencePoint:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Specify reference point"),
                                            tr("Cancel"));
        break;
    case SetTargetPoint:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Specify target point"),
                                            tr("Back"));
        break;
    case SetAngle:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Enter rotation angle:"),
                                            tr("Back"));
        break;
    default:
        RS_DIALOGFACTORY->updateMouseWidget("", "");
        break;
    }
}



void RS_ActionModifyMoveRotate::updateMouseCursor() {
    graphicView->setMouseCursor(RS2::CadCursor);
}



void RS_ActionModifyMoveRotate::updateToolBar() {
    switch (getStatus()) {
    case SetReferencePoint:
    case SetTargetPoint:
        RS_DIALOGFACTORY->requestToolBar(RS2::ToolBarSnap);
        break;
    default:
        RS_DIALOGFACTORY->requestToolBar(RS2::ToolBarModify);
        break;
    }
}


// EOF
