/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
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

#include "rs_actiondrawarc3p.h"

#include <QAction>
#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_actiondrawarc.h"
#include "rs_commands.h"
#include "rs_commandevent.h"



RS_ActionDrawArc3P::RS_ActionDrawArc3P(RS_EntityContainer& container,
                                       RS_GraphicView& graphicView)
        :RS_PreviewActionInterface("Draw arcs 3P",
                           container, graphicView) {
    reset();
}



RS_ActionDrawArc3P::~RS_ActionDrawArc3P() {}


QAction* RS_ActionDrawArc3P::createGUIAction(RS2::ActionType /*type*/, QObject* /*parent*/) {
	// tr("Arc: 3 Points"
	QAction* action = new QAction(tr("&3 Points"), NULL);
	action->setIcon(QIcon(":/extui/arcs3p.png"));
 	//action->zetStatusTip(tr("Draw arcs with 3 points"));
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


void RS_ActionDrawArc3P::mouseMoveEvent(QMouseEvent* e) {
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



void RS_ActionDrawArc3P::mouseReleaseEvent(QMouseEvent* e) {
    if (e->button()==Qt::LeftButton) {
        RS_CoordinateEvent ce(snapPoint(e));
        coordinateEvent(&ce);
    } else if (e->button()==Qt::RightButton) {
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
    QString c = e->getCommand().toLower();

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



QStringList RS_ActionDrawArc3P::getAvailableCommands() {
    QStringList cmd;
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

