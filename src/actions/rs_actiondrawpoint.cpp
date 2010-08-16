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

#include "rs_actiondrawpoint.h"
#include "rs_snapper.h"
#include "rs_point.h"



RS_ActionDrawPoint::RS_ActionDrawPoint(RS_EntityContainer& container,
                                       RS_GraphicView& graphicView)
        :RS_PreviewActionInterface("Draw Points",
                           container, graphicView) {}


QAction* RS_ActionDrawPoint::createGUIAction(RS2::ActionType /*type*/, QObject* /*parent*/) {
	/* RVT_PORT    QAction* action = new QAction(tr("Points"), tr("&Points"),
	 QKeySequence(), NULL); */
	QAction* action = new QAction(tr("Points"),  NULL);
    action->setStatusTip(tr("Draw Points"));

    return action;
}


void RS_ActionDrawPoint::trigger() {
    if (pt.valid) {
        RS_Point* point = new RS_Point(container, RS_PointData(pt));
        container->addEntity(point);

        if (document) {
            document->startUndoCycle();
            document->addUndoable(point);
            document->endUndoCycle();
        }

		graphicView->redraw(RS2::RedrawDrawing);
        graphicView->moveRelativeZero(pt);
    }
}



void RS_ActionDrawPoint::mouseMoveEvent(RS_MouseEvent* e) {
    snapPoint(e);
}



void RS_ActionDrawPoint::mouseReleaseEvent(RS_MouseEvent* e) {
    if (RS2::qtToRsButtonState(e->button())==RS2::LeftButton) {
        RS_CoordinateEvent ce(snapPoint(e));
        coordinateEvent(&ce);
    } else if (RS2::qtToRsButtonState(e->button())==RS2::RightButton) {
        init(getStatus()-1);
    }
}



void RS_ActionDrawPoint::coordinateEvent(RS_CoordinateEvent* e) {
    if (e==NULL) {
        return;
    }

    RS_Vector mouse = e->getCoordinate();

    pt = mouse;
    trigger();
}



void RS_ActionDrawPoint::commandEvent(RS_CommandEvent* e) {
    RS_String c = e->getCommand().lower();

    if (checkCommand("help", c)) {
        if (RS_DIALOGFACTORY!=NULL) {
            RS_DIALOGFACTORY->commandMessage(msgAvailableCommands()
                                             + getAvailableCommands().join(", "));
        }
        return;
    }
}



RS_StringList RS_ActionDrawPoint::getAvailableCommands() {
    RS_StringList cmd;
    return cmd;
}


void RS_ActionDrawPoint::updateMouseButtonHints() {
    if (RS_DIALOGFACTORY!=NULL) {
        switch (getStatus()) {
        case 0:
            RS_DIALOGFACTORY->updateMouseWidget(tr("Specify location"), tr("Cancel"));
            break;
        default:
            RS_DIALOGFACTORY->updateMouseWidget("", "");
            break;
        }
    }
}



void RS_ActionDrawPoint::updateMouseCursor() {
    graphicView->setMouseCursor(RS2::CadCursor);
}



void RS_ActionDrawPoint::updateToolBar() {
    if (RS_DIALOGFACTORY!=NULL) {
        if (!isFinished()) {
            RS_DIALOGFACTORY->requestToolBar(RS2::ToolBarSnap);
        } else {
            RS_DIALOGFACTORY->requestToolBar(RS2::ToolBarPoints);
        }
    }
}

// EOF
