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

#include "rs_actiondrawlinepolygon2.h"

#include "rs_creation.h"
#include "rs_snapper.h"



RS_ActionDrawLinePolygon2::RS_ActionDrawLinePolygon2(
    RS_EntityContainer& container,
    RS_GraphicView& graphicView)
        :RS_PreviewActionInterface("Draw Polygons", container, graphicView) {

    corner1 = RS_Vector(false);
    corner2 = RS_Vector(false);

    number = 3;
}

QAction* RS_ActionDrawLinePolygon2::createGUIAction(RS2::ActionType /*type*/, QObject* /*parent*/) {
	// tr("Polygon")
    QAction* action = new QAction(tr("Polygo&n (Cor,Cor)"), NULL);
	action->setIcon(QIcon(":/extui/linespolygon2.png"));
    //action->zetStatusTip(tr("Draw polygon with two corners"));
    return action;
}


void RS_ActionDrawLinePolygon2::trigger() {
    RS_PreviewActionInterface::trigger();

    deletePreview();

    RS_Creation creation(container, graphicView);
    bool ok = creation.createPolygon2(corner1, corner2, number);

    if (!ok) {
        RS_DEBUG->print("RS_ActionDrawLinePolygon2::trigger:"
                        " No polygon added\n");
    }
}



void RS_ActionDrawLinePolygon2::mouseMoveEvent(RS_MouseEvent* e) {
    RS_DEBUG->print("RS_ActionDrawLinePolygon2::mouseMoveEvent begin");

    RS_Vector mouse = snapPoint(e);

    switch (getStatus()) {
    case SetCorner1:
        break;

    case SetCorner2:
        if (corner1.valid) {
            corner2 = mouse;
            deletePreview();

            RS_Creation creation(preview, NULL, false);
            creation.createPolygon2(corner1, corner2, number);

            drawPreview();
        }
        break;

    default:
        break;
    }
}



void RS_ActionDrawLinePolygon2::mouseReleaseEvent(RS_MouseEvent* e) {
    if (RS2::qtToRsButtonState(e->button())==RS2::LeftButton) {
        RS_CoordinateEvent ce(snapPoint(e));
        coordinateEvent(&ce);
    } else if (RS2::qtToRsButtonState(e->button())==RS2::RightButton) {
        deletePreview();
        init(getStatus()-1);
    }
}



void RS_ActionDrawLinePolygon2::coordinateEvent(RS_CoordinateEvent* e) {
    if (e==NULL) {
        return;
    }

    RS_Vector mouse = e->getCoordinate();

    switch (getStatus()) {
    case SetCorner1:
        corner1 = mouse;
        setStatus(SetCorner2);
        graphicView->moveRelativeZero(mouse);
        break;

    case SetCorner2:
        corner2 = mouse;
        trigger();
        break;

    default:
        break;
    }
}



void RS_ActionDrawLinePolygon2::updateMouseButtonHints() {
    if (RS_DIALOGFACTORY!=NULL) {
        switch (getStatus()) {
        case SetCorner1:
            RS_DIALOGFACTORY->updateMouseWidget(tr("Specify first corner"),
                                                tr("Cancel"));
            break;

        case SetCorner2:
            RS_DIALOGFACTORY->updateMouseWidget(tr("Specify second corner"),
                                                tr("Back"));
            break;

        case SetNumber:
            RS_DIALOGFACTORY->updateMouseWidget(tr("Number:"), tr("Back"));
            break;

        default:
            RS_DIALOGFACTORY->updateMouseWidget("", "");
            break;
        }
    }
}



void RS_ActionDrawLinePolygon2::showOptions() {
    RS_ActionInterface::showOptions();

    if (RS_DIALOGFACTORY!=NULL) {
        RS_DIALOGFACTORY->requestOptions(this, true);
    }
}



void RS_ActionDrawLinePolygon2::hideOptions() {
    RS_ActionInterface::hideOptions();

    if (RS_DIALOGFACTORY!=NULL) {
        RS_DIALOGFACTORY->requestOptions(this, false);
    }
}



void RS_ActionDrawLinePolygon2::commandEvent(RS_CommandEvent* e) {
    RS_String c = e->getCommand().lower();

    if (checkCommand("help", c)) {
        if (RS_DIALOGFACTORY!=NULL) {
            RS_DIALOGFACTORY->commandMessage(msgAvailableCommands()
                                             + getAvailableCommands().join(", "));
        }
        return;
    }

    switch (getStatus()) {
    case SetCorner1:
    case SetCorner2:
        if (checkCommand("number", c)) {
            deletePreview();
            lastStatus = (Status)getStatus();
            setStatus(SetNumber);
        }
        break;

    case SetNumber: {
            bool ok;
            int n = c.toInt(&ok);
            if (ok==true) {
                if (n>0 && n<10000) {
                    number = n;
                } else {
                    if (RS_DIALOGFACTORY!=NULL) {
                        RS_DIALOGFACTORY->commandMessage(tr("Not a valid number. "
                                                            "Try 1..9999"));
                    }
                }
            } else {
                if (RS_DIALOGFACTORY!=NULL) {
                    RS_DIALOGFACTORY->commandMessage(tr("Not a valid expression."));
                }
            }
            if (RS_DIALOGFACTORY!=NULL) {
                RS_DIALOGFACTORY->requestOptions(this, true, true);
            }
            setStatus(lastStatus);
        }
        break;

    default:
        break;
    }
}



RS_StringList RS_ActionDrawLinePolygon2::getAvailableCommands() {
    RS_StringList cmd;

    switch (getStatus()) {
    case SetCorner1:
    case SetCorner2:
        cmd += command("number");
        break;
    default:
        break;
    }

    return cmd;
}



void RS_ActionDrawLinePolygon2::updateMouseCursor() {
    graphicView->setMouseCursor(RS2::CadCursor);
}



void RS_ActionDrawLinePolygon2::updateToolBar() {
    if (RS_DIALOGFACTORY!=NULL) {
        if (!isFinished()) {
            RS_DIALOGFACTORY->requestToolBar(RS2::ToolBarSnap);
        } else {
            RS_DIALOGFACTORY->requestToolBar(RS2::ToolBarLines);
        }
    }
}


// EOF
