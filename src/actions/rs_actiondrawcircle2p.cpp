/****************************************************************************
**
** This file is part of the CADuntu project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (caduntu@rvt.dds.nl)
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
**
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by 
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
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

#include "rs_actiondrawcircle2p.h"
#include "rs_snapper.h"



RS_ActionDrawCircle2P::RS_ActionDrawCircle2P(RS_EntityContainer& container,
        RS_GraphicView& graphicView)
        :RS_PreviewActionInterface("Draw circles",
                           container, graphicView) {
    reset();
}



RS_ActionDrawCircle2P::~RS_ActionDrawCircle2P() {}


QAction* RS_ActionDrawCircle2P::createGUIAction(RS2::ActionType /*type*/, QObject* /*parent*/) {
	// tr("Circle: 2 Points")
	QAction* action = new QAction(tr("2 Points"),  NULL);
	action->setIcon(QIcon(":/extui/circles2p.png"));
    action->setStatusTip(tr("Draw circles with 2 points"));

    return action;
}


void RS_ActionDrawCircle2P::reset() {
    data.reset();
    point1 = RS_Vector(false);
    point2 = RS_Vector(false);
}



void RS_ActionDrawCircle2P::init(int status) {
    RS_PreviewActionInterface::init(status);

    reset();
}



void RS_ActionDrawCircle2P::trigger() {
    RS_PreviewActionInterface::trigger();

    preparePreview();
    if (data.isValid()) {
        RS_Circle* circle = new RS_Circle(container,
                                          data);
        circle->setLayerToActive();
        circle->setPenToActive();
        container->addEntity(circle);

        // upd. undo list:
        if (document!=NULL) {
            document->startUndoCycle();
            document->addUndoable(circle);
            document->endUndoCycle();
        }

        RS_Vector rz = graphicView->getRelativeZero();
		graphicView->redraw(RS2::RedrawDrawing);
        graphicView->moveRelativeZero(rz);

        setStatus(SetPoint1);
        reset();
    } else {
        if (RS_DIALOGFACTORY!=NULL) {
            RS_DIALOGFACTORY->requestWarningDialog(tr("Invalid Circle data."));
        }
    }
}



void RS_ActionDrawCircle2P::preparePreview() {
    data.reset();
    if (point1.valid && point2.valid) {
        RS_Circle circle(NULL, data);
        bool suc = circle.createFrom2P(point1, point2);
        if (suc) {
            data = circle.getData();
        }
    }
}


void RS_ActionDrawCircle2P::mouseMoveEvent(RS_MouseEvent* e) {
    RS_Vector mouse = snapPoint(e);
    switch (getStatus()) {
    case SetPoint1:
        point1 = mouse;
        break;

    case SetPoint2:
        point2 = mouse;
        preparePreview();
        if (data.isValid()) {
            RS_Circle* circle = new RS_Circle(preview, data);
            deletePreview();
            preview->addEntity(circle);
            drawPreview();
        }
        break;

    default:
        break;
    }
}



void RS_ActionDrawCircle2P::mouseReleaseEvent(RS_MouseEvent* e) {
    if (RS2::qtToRsButtonState(e->button())==RS2::LeftButton) {
        RS_CoordinateEvent ce(snapPoint(e));
        coordinateEvent(&ce);
    } else if (RS2::qtToRsButtonState(e->button())==RS2::RightButton) {
        deletePreview();
        init(getStatus()-1);
    }
}



void RS_ActionDrawCircle2P::coordinateEvent(RS_CoordinateEvent* e) {
    if (e==NULL) {
        return;
    }

    RS_Vector mouse = e->getCoordinate();

    switch (getStatus()) {
    case SetPoint1:
        point1 = mouse;
        graphicView->moveRelativeZero(mouse);
        setStatus(SetPoint2);
        break;

    case SetPoint2:
        point2 = mouse;
        graphicView->moveRelativeZero(mouse);
        trigger();
        break;

    default:
        break;
    }
}



void RS_ActionDrawCircle2P::commandEvent(RS_CommandEvent* e) {
    RS_String c = e->getCommand().lower();

    if (checkCommand("help", c)) {
        if (RS_DIALOGFACTORY!=NULL) {
            RS_DIALOGFACTORY->commandMessage(msgAvailableCommands()
                                             + getAvailableCommands().join(", "));
        }
        return;
    }

}



RS_StringList RS_ActionDrawCircle2P::getAvailableCommands() {
    RS_StringList cmd;
    return cmd;
}



void RS_ActionDrawCircle2P::updateMouseButtonHints() {
    if (RS_DIALOGFACTORY!=NULL) {
        switch (getStatus()) {
        case SetPoint1:
            RS_DIALOGFACTORY->updateMouseWidget(
                tr("Specify first point"), tr("Cancel"));
            break;
        case SetPoint2:
            RS_DIALOGFACTORY->updateMouseWidget(
                tr("Specify second point"), tr("Back"));
            break;
        default:
            RS_DIALOGFACTORY->updateMouseWidget("", "");
            break;
        }
    }
}



void RS_ActionDrawCircle2P::updateMouseCursor() {
    graphicView->setMouseCursor(RS2::CadCursor);
}



void RS_ActionDrawCircle2P::updateToolBar() {
    if (RS_DIALOGFACTORY!=NULL) {
        if (!isFinished()) {
            RS_DIALOGFACTORY->requestToolBar(RS2::ToolBarSnap);
        } else {
            RS_DIALOGFACTORY->requestToolBar(RS2::ToolBarCircles);
        }
    }
}


// EOF

