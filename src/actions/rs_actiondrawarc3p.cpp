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

#include "rs_actiondrawarc3p.h"

#include "rs_actiondrawarc.h"
#include "rs_commands.h"
#include "rs_snapper.h"
#include "rs_dialogfactory.h"



RS_ActionDrawArc3P::RS_ActionDrawArc3P(RS_EntityContainer& container,
                                       RS_GraphicView& graphicView)
        :RS_PreviewActionInterface("Draw arcs 3P",
                           container, graphicView) {
    reset();
}



RS_ActionDrawArc3P::~RS_ActionDrawArc3P() {}


QAction* RS_ActionDrawArc3P::createGUIAction(RS2::ActionType /*type*/, QObject* /*parent*/) {
/*RVT_PORT	QAction* action = new QAction(tr("Arc: 3 Points"),
								  tr("&3 Points"),
								  QKeySequence(), NULL); */
	QAction* action = new QAction(tr("Arc: 3 Points"), NULL);
 	action->setStatusTip(tr("Draw arcs with 3 points"));
		return action;
}


void RS_ActionDrawArc3P::reset() {
    data.reset();
    point1 = RS_Vector(false);
    point2 = RS_Vector(false);
    point3 = RS_Vector(false);
}



void RS_ActionDrawArc3P::init(int status) {
    RS_PreviewActionInterface::init(status);

    //reset();
}



void RS_ActionDrawArc3P::trigger() {
    RS_PreviewActionInterface::trigger();

    preparePreview();
    if (data.isValid()) {
        RS_Arc* arc = new RS_Arc(container,
                                 data);
        arc->setLayerToActive();
        arc->setPenToActive();
        container->addEntity(arc);

        // upd. undo list:
        if (document!=NULL) {
            document->startUndoCycle();
            document->addUndoable(arc);
            document->endUndoCycle();
        }

		graphicView->redraw(RS2::RedrawDrawing);
        graphicView->moveRelativeZero(arc->getEndpoint());

        setStatus(SetPoint1);
        reset();
    } else {
        //RS_DIALOGFACTORY->requestWarningDialog(tr("Invalid arc data."));
        RS_DIALOGFACTORY->commandMessage(tr("Invalid arc data."));
    }
}



void RS_ActionDrawArc3P::preparePreview() {
    data.reset();
    if (point1.valid && point2.valid && point3.valid) {
        RS_Arc arc(NULL, data);
        bool suc = arc.createFrom3P(point1, point2, point3);
        if (suc) {
            data = arc.getData();
        }
    }
}


void RS_ActionDrawArc3P::mouseMoveEvent(RS_MouseEvent* e) {
    RS_Vector mouse = snapPoint(e);

    switch (getStatus()) {
    case SetPoint1:
        point1 = mouse;
        break;

    case SetPoint2:
        point2 = mouse;
        if (point1.valid) {
            RS_Line* line = new RS_Line(preview, RS_LineData(point1, point2));

            deletePreview();
            preview->addEntity(line);
            drawPreview();
        }
        break;

    case SetPoint3:
        point3 = mouse;
        preparePreview();
        if (data.isValid()) {
            RS_Arc* arc = new RS_Arc(preview, data);

            deletePreview();
            preview->addEntity(arc);
            drawPreview();
        }
        break;

    default:
        break;
    }
}



void RS_ActionDrawArc3P::mouseReleaseEvent(RS_MouseEvent* e) {
    if (RS2::qtToRsButtonState(e->button())==RS2::LeftButton) {
        RS_CoordinateEvent ce(snapPoint(e));
        coordinateEvent(&ce);
    } else if (RS2::qtToRsButtonState(e->button())==RS2::RightButton) {
        deletePreview();
        init(getStatus()-1);
    }
}



void RS_ActionDrawArc3P::coordinateEvent(RS_CoordinateEvent* e) {
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
        setStatus(SetPoint3);
        break;

    case SetPoint3:
        point3 = mouse;
        trigger();
        break;

    default:
        break;
    }
}



void RS_ActionDrawArc3P::commandEvent(RS_CommandEvent* e) {
    RS_String c = e->getCommand().lower();

    if (checkCommand("help", c)) {
        RS_DIALOGFACTORY->commandMessage(msgAvailableCommands()
                                         + getAvailableCommands().join(", "));
        return;
    }

    if (RS_COMMANDS->checkCommand("center", c, rtti())) {
        finish();
        graphicView->setCurrentAction(
            new RS_ActionDrawArc(*container, *graphicView));
    }
}



RS_StringList RS_ActionDrawArc3P::getAvailableCommands() {
    RS_StringList cmd;
    return cmd;
}



void RS_ActionDrawArc3P::updateMouseButtonHints() {
    switch (getStatus()) {
    case SetPoint1:
        RS_DIALOGFACTORY->updateMouseWidget(
            tr("Specify startpoint or [Center]"),
            tr("Cancel"));
        break;
    case SetPoint2:
        RS_DIALOGFACTORY->updateMouseWidget(
            tr("Specify second point"), tr("Back"));
        break;
    case SetPoint3:
        RS_DIALOGFACTORY->updateMouseWidget(
            tr("Specify endpoint"), tr("Back"));
        break;
    default:
        RS_DIALOGFACTORY->updateMouseWidget("", "");
        break;
    }
}



void RS_ActionDrawArc3P::updateMouseCursor() {
    graphicView->setMouseCursor(RS2::CadCursor);
}



void RS_ActionDrawArc3P::updateToolBar() {
    if (!isFinished()) {
        RS_DIALOGFACTORY->requestToolBar(RS2::ToolBarSnap);
    } else {
        RS_DIALOGFACTORY->requestToolBar(RS2::ToolBarArcs);
    }
}


// EOF

