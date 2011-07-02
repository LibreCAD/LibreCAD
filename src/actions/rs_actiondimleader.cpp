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

#include "rs_actiondimleader.h"

#include "rs_leader.h"


RS_ActionDimLeader::RS_ActionDimLeader(RS_EntityContainer& container,
                                       RS_GraphicView& graphicView)
        :RS_PreviewActionInterface("Draw leaders",
                           container, graphicView) {

    reset();
}



RS_ActionDimLeader::~RS_ActionDimLeader() {}


QAction* RS_ActionDimLeader::createGUIAction(RS2::ActionType /*type*/, QObject* /*parent*/) {
	// tr("Leader")
    QAction* action = new QAction(tr("&Leader"), NULL);
	action->setIcon(QIcon(":/extui/dimleader.png"));
    //action->zetStatusTip(tr("Leader Dimension"));
    return action;
}

void RS_ActionDimLeader::reset() {
    //data = RS_LineData(RS_Vector(false), RS_Vector(false));
    //start = RS_Vector(false);
    //history.clear();
    points.clear();
}



void RS_ActionDimLeader::init(int status) {
    RS_PreviewActionInterface::init(status);

    reset();
}



void RS_ActionDimLeader::trigger() {
    RS_PreviewActionInterface::trigger();

    if (points.count()>0) {

        RS_Leader* leader = new RS_Leader(container, RS_LeaderData(true));
        leader->setLayerToActive();
        leader->setPenToActive();

        for (int i = 0; i < points.size(); ++i) {
            leader->addVertex(points.at(i));
/*RLZ        for (RS_Vector* v=points.first(); v!=NULL; v=points.next()) {
            leader->addVertex(*v);*/
        }

        container->addEntity(leader);

        // upd. undo list:
        if (document!=NULL) {
            document->startUndoCycle();
            document->addUndoable(leader);
            document->endUndoCycle();
        }

        deletePreview();
        RS_Vector rz = graphicView->getRelativeZero();
		graphicView->redraw(RS2::RedrawDrawing);
        graphicView->moveRelativeZero(rz);
        //drawSnapper();

        RS_DEBUG->print("RS_ActionDimLeader::trigger(): leader added: %d",
                        leader->getId());
    }
}



void RS_ActionDimLeader::mouseMoveEvent(QMouseEvent* e) {
    RS_DEBUG->print("RS_ActionDimLeader::mouseMoveEvent begin");

    RS_Vector mouse = snapPoint(e);
//RLZ    if (getStatus()==SetEndpoint && points.last()!=NULL) {
    if (getStatus()==SetEndpoint && !points.isEmpty()) {
        deletePreview();

        // fill in lines that were already set:
        RS_Vector last(false);
        for (int i = 0; i < points.size(); ++i) {
/*        for (RS_Vector* v=points.first(); v!=NULL; v=points.next()) {*/
            RS_Vector v = points.at(i);
            if (last.valid) {
                preview->addEntity(new RS_Line(preview,
//RLZ                                               RS_LineData(last, *v)));
                                               RS_LineData(last, v)));
            }
//RLZ            last = *v;
            last = v;
        }

        if ( !points.isEmpty() ) {
            RS_Vector p = points.last();
            preview->addEntity(new RS_Line(preview,
                                       RS_LineData(p, mouse)));
        }
        drawPreview();
    }

    RS_DEBUG->print("RS_ActionDimLeader::mouseMoveEvent end");
}



void RS_ActionDimLeader::mouseReleaseEvent(QMouseEvent* e) {
    if (e->button()==Qt::LeftButton) {
        RS_CoordinateEvent ce(snapPoint(e));
        coordinateEvent(&ce);
    } else if (e->button()==Qt::RightButton) {
        if (getStatus()==SetEndpoint) {
            trigger();
            reset();
            setStatus(SetStartpoint);
        } else {
            deletePreview();
            init(getStatus()-1);
        }
    }
}



void RS_ActionDimLeader::keyPressEvent(QKeyEvent* e) {
    if (getStatus()==SetEndpoint && e->key()==Qt::Key_Enter) {
        trigger();
        reset();
        setStatus(SetStartpoint);
    }
}



void RS_ActionDimLeader::coordinateEvent(RS_CoordinateEvent* e) {
    if (e==NULL) {
        return;
    }

    RS_Vector mouse = e->getCoordinate();

    switch (getStatus()) {
    case SetStartpoint:
        //data.startpoint = mouse;
        points.clear();
//RLZ        points.append(new RS_Vector(mouse));
        points.append(mouse);
        //start = data.startpoint;
        setStatus(SetEndpoint);
        graphicView->moveRelativeZero(mouse);
        break;

    case SetEndpoint:
        //data.endpoint = mouse;
//RLZ        points.append(new RS_Vector(mouse));
        points.append(mouse);
        //trigger();
        //data.startpoint = data.endpoint;
        graphicView->moveRelativeZero(mouse);
        break;

    default:
        break;
    }
}



void RS_ActionDimLeader::commandEvent(RS_CommandEvent* e) {
    QString c = e->getCommand().toLower();

    if (checkCommand("help", c)) {
        if (RS_DIALOGFACTORY!=NULL) {
            RS_DIALOGFACTORY->commandMessage(msgAvailableCommands()
                                             + getAvailableCommands().join(", "));
        }
        return;
    }

    // enter to finish
    if (c=="") {
        trigger();
        reset();
        setStatus(SetStartpoint);
        //finish();
    }
}



QStringList RS_ActionDimLeader::getAvailableCommands() {
    QStringList cmd;

    return cmd;
}



void RS_ActionDimLeader::updateMouseButtonHints() {
    if (RS_DIALOGFACTORY!=NULL) {
        switch (getStatus()) {
        case SetStartpoint:
            RS_DIALOGFACTORY->updateMouseWidget(tr("Specify target point"),
                                                tr("Cancel"));
            break;
        case SetEndpoint:
            RS_DIALOGFACTORY->updateMouseWidget(tr("Specify next point"),
                                                tr("Finish"));
            break;
        default:
            RS_DIALOGFACTORY->updateMouseWidget("", "");
            break;
        }
    }
}


void RS_ActionDimLeader::showOptions() {
    RS_ActionInterface::showOptions();

    //RS_DIALOGFACTORY->requestOptions(this, true);
}



void RS_ActionDimLeader::hideOptions() {
    RS_ActionInterface::hideOptions();

    //RS_DIALOGFACTORY->requestOptions(this, false);
}


void RS_ActionDimLeader::updateMouseCursor() {
    graphicView->setMouseCursor(RS2::CadCursor);
}


void RS_ActionDimLeader::updateToolBar() {
    if (RS_DIALOGFACTORY!=NULL) {
        if (!isFinished()) {
            RS_DIALOGFACTORY->requestToolBar(RS2::ToolBarSnap);
        } else {
            RS_DIALOGFACTORY->requestToolBar(RS2::ToolBarDim);
        }
    }
}

// EOF
