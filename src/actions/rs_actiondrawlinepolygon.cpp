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

#include "rs_actiondrawlinepolygon.h"

#include "rs_creation.h"
#include "rs_snapper.h"



RS_ActionDrawLinePolygon::RS_ActionDrawLinePolygon(
    RS_EntityContainer& container,
    RS_GraphicView& graphicView)
        :RS_PreviewActionInterface("Draw Polygons", container, graphicView) {

    center = RS_Vector(false);
    corner = RS_Vector(false);

    number = 3;
}

QAction* RS_ActionDrawLinePolygon::createGUIAction(RS2::ActionType /*type*/, QObject* /*parent*/) {
	// Polygon
    QAction* action = new QAction(tr("Pol&ygon (Cen,Cor)"),  NULL);
		action->setIcon(QIcon(":/extui/linespolygon.png"));
    action->setStatusTip(tr("Draw polygon with center and corner"));
    return action;
}


void RS_ActionDrawLinePolygon::trigger() {
    RS_PreviewActionInterface::trigger();

    deletePreview();

    RS_Creation creation(container, graphicView);
    bool ok = creation.createPolygon(center, corner, number);

    if (!ok) {
        RS_DEBUG->print("RS_ActionDrawLinePolygon::trigger:"
                        " No polygon added\n");
    }
}



void RS_ActionDrawLinePolygon::mouseMoveEvent(RS_MouseEvent* e) {
    RS_DEBUG->print("RS_ActionDrawLinePolygon::mouseMoveEvent begin");

    RS_Vector mouse = snapPoint(e);

    switch (getStatus()) {
    case SetCenter:
        break;

    case SetCorner:
        if (center.valid) {
            corner = mouse;
            deletePreview();

            RS_Creation creation(preview, NULL, false);
            creation.createPolygon(center, corner, number);

            drawPreview();
        }
        break;

    default:
        break;
    }
}



void RS_ActionDrawLinePolygon::mouseReleaseEvent(RS_MouseEvent* e) {
    if (RS2::qtToRsButtonState(e->button())==RS2::LeftButton) {
        RS_CoordinateEvent ce(snapPoint(e));
        coordinateEvent(&ce);
    } else if (RS2::qtToRsButtonState(e->button())==RS2::RightButton) {
        deletePreview();
        init(getStatus()-1);
    }
}



void RS_ActionDrawLinePolygon::coordinateEvent(RS_CoordinateEvent* e) {
    if (e==NULL) {
        return;
    }

    RS_Vector mouse = e->getCoordinate();

    switch (getStatus()) {
    case SetCenter:
        center = mouse;
        setStatus(SetCorner);
        graphicView->moveRelativeZero(mouse);
        break;

    case SetCorner:
        corner = mouse;
        trigger();
        break;

    default:
        break;
    }
}



void RS_ActionDrawLinePolygon::updateMouseButtonHints() {
    if (RS_DIALOGFACTORY!=NULL) {
        switch (getStatus()) {
        case SetCenter:
            RS_DIALOGFACTORY->updateMouseWidget(tr("Specify center"),
                                                "");
            break;

        case SetCorner:
            RS_DIALOGFACTORY->updateMouseWidget(tr("Specify a corner"), "");
            break;

        case SetNumber:
            RS_DIALOGFACTORY->updateMouseWidget(tr("Enter number:"), "");
            break;

        default:
            RS_DIALOGFACTORY->updateMouseWidget("", "");
            break;
        }
    }
}



void RS_ActionDrawLinePolygon::showOptions() {
    RS_ActionInterface::showOptions();

    if (RS_DIALOGFACTORY!=NULL) {
        RS_DIALOGFACTORY->requestOptions(this, true);
    }
}



void RS_ActionDrawLinePolygon::hideOptions() {
    RS_ActionInterface::hideOptions();

    if (RS_DIALOGFACTORY!=NULL) {
        RS_DIALOGFACTORY->requestOptions(this, false);
    }
}



void RS_ActionDrawLinePolygon::commandEvent(RS_CommandEvent* e) {
    RS_String c = e->getCommand().lower();

    if (checkCommand("help", c)) {
        if (RS_DIALOGFACTORY!=NULL) {
            RS_DIALOGFACTORY->commandMessage(msgAvailableCommands()
                                             + getAvailableCommands().join(", "));
        }
        return;
    }

    switch (getStatus()) {
    case SetCenter:
    case SetCorner:
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
                    RS_DIALOGFACTORY->commandMessage(tr("Not a valid expression"));
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



RS_StringList RS_ActionDrawLinePolygon::getAvailableCommands() {
    RS_StringList cmd;

    switch (getStatus()) {
    case SetCenter:
    case SetCorner:
        cmd += command("number");
        break;
    default:
        break;
    }

    return cmd;
}



void RS_ActionDrawLinePolygon::updateMouseCursor() {
    graphicView->setMouseCursor(RS2::CadCursor);
}



void RS_ActionDrawLinePolygon::updateToolBar() {
    if (RS_DIALOGFACTORY!=NULL) {
        if (!isFinished()) {
            RS_DIALOGFACTORY->requestToolBar(RS2::ToolBarSnap);
        } else {
            RS_DIALOGFACTORY->requestToolBar(RS2::ToolBarLines);
        }
    }
}


// EOF
