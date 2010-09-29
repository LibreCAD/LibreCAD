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

#include "rs_actiondrawcirclecr.h"
#include "rs_snapper.h"

/**
 * Constructor.
 */
RS_ActionDrawCircleCR::RS_ActionDrawCircleCR(RS_EntityContainer& container,
        RS_GraphicView& graphicView)
        :RS_PreviewActionInterface("Draw circles CR",
                           container, graphicView) {

    reset();
}



RS_ActionDrawCircleCR::~RS_ActionDrawCircleCR() {}


QAction* RS_ActionDrawCircleCR::createGUIAction(RS2::ActionType /*type*/, QObject* /*parent*/) {
	// "Circle: Center, Radius"
    QAction* action = new QAction(tr("Center, &Radius"), NULL);
		action->setIcon(QIcon(":/extui/circlescr.png"));
    //action->zetStatusTip(tr("Draw circles with center and radius"));
    return action;
}


void RS_ActionDrawCircleCR::reset() {
    data = RS_CircleData(RS_Vector(false), 0.0);
}



void RS_ActionDrawCircleCR::init(int status) {
    RS_PreviewActionInterface::init(status);
}



void RS_ActionDrawCircleCR::trigger() {
    RS_PreviewActionInterface::trigger();

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
	graphicView->redraw(RS2::RedrawDrawing);
    graphicView->moveRelativeZero(circle->getCenter());

    setStatus(SetCenter);

    RS_DEBUG->print("RS_ActionDrawCircleCR::trigger(): circle added: %d",
                    circle->getId());
}



void RS_ActionDrawCircleCR::mouseMoveEvent(RS_MouseEvent* e) {
    RS_DEBUG->print("RS_ActionDrawCircleCR::mouseMoveEvent begin");

    RS_Vector mouse = snapPoint(e);
    switch (getStatus()) {
    case SetCenter:
        data.center = mouse;
        deletePreview();
        preview->addEntity(new RS_Circle(preview,
                                         data));
        drawPreview();
        break;
    }

    RS_DEBUG->print("RS_ActionDrawCircleCR::mouseMoveEvent end");
}



void RS_ActionDrawCircleCR::mouseReleaseEvent(RS_MouseEvent* e) {
    if (RS2::qtToRsButtonState(e->button())==RS2::LeftButton) {
        RS_CoordinateEvent ce(snapPoint(e));
        coordinateEvent(&ce);
    } else if (RS2::qtToRsButtonState(e->button())==RS2::RightButton) {
        deletePreview();
        init(getStatus()-1);
    }
}



void RS_ActionDrawCircleCR::coordinateEvent(RS_CoordinateEvent* e) {
    if (e==NULL) {
        return;
    }

    RS_Vector mouse = e->getCoordinate();

    switch (getStatus()) {
    case SetCenter:
        data.center = mouse;
        trigger();
        break;

    default:
        break;
    }
}



void RS_ActionDrawCircleCR::commandEvent(RS_CommandEvent* e) {
    RS_String c = e->getCommand().lower();

    if (checkCommand("help", c)) {
        RS_DIALOGFACTORY->commandMessage(msgAvailableCommands()
                                         + getAvailableCommands().join(", "));
        return;
    }

    switch (getStatus()) {
    case SetCenter:
        if (checkCommand("radius", c)) {
            deletePreview();
            setStatus(SetRadius);
        }
        break;

    case SetRadius: {
            bool ok;
            double r = RS_Math::eval(c, &ok);
            if (ok==true) {
                data.radius = r;
            } else {
                RS_DIALOGFACTORY->commandMessage(tr("Not a valid expression"));
            }
            RS_DIALOGFACTORY->requestOptions(this, true, true);
            setStatus(SetCenter);
        }
        break;

    default:
        break;
    }
}



RS_StringList RS_ActionDrawCircleCR::getAvailableCommands() {
    RS_StringList cmd;

    switch (getStatus()) {
    case SetCenter:
        cmd += command("radius");
        break;
    default:
        break;
    }

    return cmd;
}

void RS_ActionDrawCircleCR::updateMouseButtonHints() {
    switch (getStatus()) {
    case SetCenter:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Specify circle center"),
                                            tr("Cancel"));
        break;
    case SetRadius:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Specify circle radius"),
                                            tr("Back"));
        break;
    default:
        RS_DIALOGFACTORY->updateMouseWidget("", "");
        break;
    }
}



void RS_ActionDrawCircleCR::showOptions() {
    RS_ActionInterface::showOptions();

    RS_DIALOGFACTORY->requestOptions(this, true);
}



void RS_ActionDrawCircleCR::hideOptions() {
    RS_ActionInterface::hideOptions();

    RS_DIALOGFACTORY->requestOptions(this, false);
}



void RS_ActionDrawCircleCR::updateMouseCursor() {
    graphicView->setMouseCursor(RS2::CadCursor);
}



void RS_ActionDrawCircleCR::updateToolBar() {
    if (!isFinished()) {
        RS_DIALOGFACTORY->requestToolBar(RS2::ToolBarSnap);
    } else {
        RS_DIALOGFACTORY->requestToolBar(RS2::ToolBarCircles);
    }
}


// EOF

