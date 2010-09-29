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

#include "rs_actiondrawline.h"
#include "rs_actioneditundo.h"
#include "rs_snapper.h"



RS_ActionDrawLine::RS_ActionDrawLine(RS_EntityContainer& container,
                                     RS_GraphicView& graphicView)
        :RS_PreviewActionInterface("Draw lines",
                           container, graphicView) {

	RS_DEBUG->print("RS_ActionDrawLine::RS_ActionDrawLine");
    reset();
    history.setAutoDelete(true);
	RS_DEBUG->print("RS_ActionDrawLine::RS_ActionDrawLine: OK");
}



RS_ActionDrawLine::~RS_ActionDrawLine() {}


QAction* RS_ActionDrawLine::createGUIAction(RS2::ActionType /*type*/, QObject* /*parent*/) {
	//tr("Line: 2 Points")
    QAction* action = new QAction(tr("&2 Points"), NULL);
	action->setIcon(QIcon(":/extui/linesnormal.png"));
    //action->zetStatusTip(tr("Draw lines"));
    return action;
}



void RS_ActionDrawLine::reset() {
	RS_DEBUG->print("RS_ActionDrawLine::reset");
    data = RS_LineData(RS_Vector(false), RS_Vector(false));
    start = RS_Vector(false);
    history.clear();
	RS_DEBUG->print("RS_ActionDrawLine::reset: OK");
}



void RS_ActionDrawLine::init(int status) {
	RS_DEBUG->print("RS_ActionDrawLine::init");
    RS_PreviewActionInterface::init(status);

    reset();
	RS_DEBUG->print("RS_ActionDrawLine::init: OK");
}



void RS_ActionDrawLine::trigger() {
    RS_PreviewActionInterface::trigger();

    RS_Line* line = new RS_Line(container, data);
    line->setLayerToActive();
    line->setPenToActive();
    container->addEntity(line);

    // upd. undo list:
    if (document!=NULL) {
        document->startUndoCycle();
        document->addUndoable(line);
        document->endUndoCycle();
    }

	graphicView->redraw(RS2::RedrawDrawing);
    graphicView->moveRelativeZero(line->getEndpoint());
    RS_DEBUG->print("RS_ActionDrawLine::trigger(): line added: %d",
                    line->getId());
}



void RS_ActionDrawLine::mouseMoveEvent(RS_MouseEvent* e) {
    RS_DEBUG->print("RS_ActionDrawLine::mouseMoveEvent begin");

    RS_DEBUG->print("RS_ActionDrawLine::mouseMoveEvent: snap point");
    RS_Vector mouse = snapPoint(e);
    RS_DEBUG->print("RS_ActionDrawLine::mouseMoveEvent: snap point: OK");
    if (getStatus()==SetEndpoint && data.startpoint.valid) {
    	RS_DEBUG->print("RS_ActionDrawLine::mouseMoveEvent: update preview");
        deletePreview();
        preview->addEntity(new RS_Line(preview,
                                       RS_LineData(data.startpoint, mouse)));
    	RS_DEBUG->print("RS_ActionDrawLine::mouseMoveEvent: draw preview");
        drawPreview();
    }

    RS_DEBUG->print("RS_ActionDrawLine::mouseMoveEvent end");
}



void RS_ActionDrawLine::mouseReleaseEvent(RS_MouseEvent* e) {
    if (RS2::qtToRsButtonState(e->button())==RS2::LeftButton) {
        RS_CoordinateEvent ce(snapPoint(e));
        coordinateEvent(&ce);
    } else if (RS2::qtToRsButtonState(e->button())==RS2::RightButton) {
        deletePreview();
        init(getStatus()-1);
    }
}



void RS_ActionDrawLine::coordinateEvent(RS_CoordinateEvent* e) {
	RS_DEBUG->print("RS_ActionDrawLine::coordinateEvent");
    if (e==NULL) {
		RS_DEBUG->print("RS_ActionDrawLine::coordinateEvent: event was NULL");
        return;
    }

    RS_Vector mouse = e->getCoordinate();

    switch (getStatus()) {
    case SetStartpoint:
        data.startpoint = mouse;
        history.clear();
        history.append(new RS_Vector(mouse));
        start = data.startpoint;
        setStatus(SetEndpoint);
        graphicView->moveRelativeZero(mouse);
        updateMouseButtonHints();
        break;

    case SetEndpoint:
        data.endpoint = mouse;
        history.append(new RS_Vector(mouse));
        trigger();
        data.startpoint = data.endpoint;
        updateMouseButtonHints();
        //graphicView->moveRelativeZero(mouse);
        break;

    default:
        break;
    }
	RS_DEBUG->print("RS_ActionDrawLine::coordinateEvent: OK");
}



void RS_ActionDrawLine::commandEvent(RS_CommandEvent* e) {
	RS_DEBUG->print("RS_ActionDrawLine::commandEvent");
    RS_String c = e->getCommand().lower();

    switch (getStatus()) {
    case SetStartpoint:
        if (checkCommand("help", c)) {
            RS_DIALOGFACTORY->commandMessage(msgAvailableCommands()
                                             + getAvailableCommands().join(", "));
            return;
        }
        break;

    case SetEndpoint:
        if (checkCommand("close", c)) {
            close();
            updateMouseButtonHints();
            return;
        }

        if (checkCommand("undo", c)) {
            undo();
            updateMouseButtonHints();
            return;
        }
        break;

    default:
        break;
    }
	RS_DEBUG->print("RS_ActionDrawLine::commandEvent: OK");
}



RS_StringList RS_ActionDrawLine::getAvailableCommands() {
    RS_StringList cmd;

    switch (getStatus()) {
    case SetStartpoint:
        break;
    case SetEndpoint:
        if (history.count()>=2) {
            cmd += command("undo");
        }
        if (history.count()>=3) {
            cmd += command("close");
        }
        break;
    default:
        break;
    }

    return cmd;
}



void RS_ActionDrawLine::updateMouseButtonHints() {
    switch (getStatus()) {
    case SetStartpoint:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Specify first point"),
                                            tr("Cancel"));
        break;
    case SetEndpoint: {
            RS_String msg = "";

            if (history.count()>=3) {
                msg += RS_COMMANDS->command("close");
                msg += "/";
            }
            if (history.count()>=2) {
                msg += RS_COMMANDS->command("undo");
            }

            if (history.count()>=2) {
                RS_DIALOGFACTORY->updateMouseWidget(
                    tr("Specify next point or [%1]").arg(msg),
                    tr("Back"));
            } else {
                RS_DIALOGFACTORY->updateMouseWidget(
                    tr("Specify next point"),
                    tr("Back"));
            }
        }
        break;
    default:
        RS_DIALOGFACTORY->updateMouseWidget("", "");
        break;
    }
}


void RS_ActionDrawLine::showOptions() {
	RS_DEBUG->print("RS_ActionDrawLine::showOptions");
    RS_ActionInterface::showOptions();

    RS_DIALOGFACTORY->requestOptions(this, true);
	RS_DEBUG->print("RS_ActionDrawLine::showOptions: OK");
}



void RS_ActionDrawLine::hideOptions() {
    RS_ActionInterface::hideOptions();

    RS_DIALOGFACTORY->requestOptions(this, false);
}


void RS_ActionDrawLine::updateMouseCursor() {
    graphicView->setMouseCursor(RS2::CadCursor);
}


void RS_ActionDrawLine::updateToolBar() {
    if (!isFinished()) {
        RS_DIALOGFACTORY->requestToolBar(RS2::ToolBarSnap);
    } else {
        RS_DIALOGFACTORY->requestToolBar(RS2::ToolBarLines);
    }
}

void RS_ActionDrawLine::close() {
    if (history.count()>2 && start.valid) {
        data.endpoint = start;
        trigger();
        setStatus(SetStartpoint);
        graphicView->moveRelativeZero(start);
    } else {
        RS_DIALOGFACTORY->commandMessage(
            tr("Cannot close sequence of lines: "
               "Not enough entities defined yet."));
    }
}

void RS_ActionDrawLine::undo() {
    if (history.count()>1) {
        history.removeLast();
        deletePreview();
        graphicView->setCurrentAction(
            new RS_ActionEditUndo(true, *container, *graphicView));
        data.startpoint = *history.last();
        graphicView->moveRelativeZero(data.startpoint);
    } else {
        RS_DIALOGFACTORY->commandMessage(
            tr("Cannot undo: "
               "Not enough entities defined yet."));
    }
}

// EOF
