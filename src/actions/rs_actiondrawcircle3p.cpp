/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
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

#include "rs_actiondrawcircle3p.h"

#include "rs_snapper.h"



RS_ActionDrawCircle3P::RS_ActionDrawCircle3P(RS_EntityContainer& container,
        RS_GraphicView& graphicView)
        :RS_PreviewActionInterface("Draw circles",
                           container, graphicView) {
    reset();
}



RS_ActionDrawCircle3P::~RS_ActionDrawCircle3P() {}


QAction* RS_ActionDrawCircle3P::createGUIAction(RS2::ActionType /*type*/, QObject* /*parent*/) {
	// "Circle: 3 Points"
    QAction* action = new QAction(tr("3 Points"), NULL);
	action->setIcon(QIcon(":/extui/circles3p.png"));
    //action->zetStatusTip(tr("Draw circles with 3 points"));
    return action;
}



void RS_ActionDrawCircle3P::reset() {
    data.reset();
    point1 = RS_Vector(false);
    point2 = RS_Vector(false);
    point3 = RS_Vector(false);
}



void RS_ActionDrawCircle3P::init(int status) {
    RS_PreviewActionInterface::init(status);

    reset();
}



void RS_ActionDrawCircle3P::trigger() {
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
        drawSnapper();

        setStatus(SetPoint1);
        reset();
    } else {
        RS_DIALOGFACTORY->requestWarningDialog(tr("Invalid circle data."));
    }
}


void RS_ActionDrawCircle3P::preparePreview() {
    data.reset();
    if (point1.valid && point2.valid && point3.valid) {
        RS_Circle circle(NULL, data);
        bool suc = circle.createFrom3P(point1, point2, point3);
        if (suc) {
            data = circle.getData();
        }
    }
}



void RS_ActionDrawCircle3P::mouseMoveEvent(RS_MouseEvent* e) {
    RS_Vector mouse = snapPoint(e);

    switch (getStatus()) {
    case SetPoint1:
        point1 = mouse;
        break;

    case SetPoint2:
        point2 = mouse;
        break;

    case SetPoint3:
        point3 = mouse;
        preparePreview();
        if (data.isValid()) {
            RS_Circle* circle = new RS_Circle(preview, data);

            deletePreview();
            preview->addEntity(circle);
            drawPreview();
        }
        break;
    }
}



void RS_ActionDrawCircle3P::mouseReleaseEvent(RS_MouseEvent* e) {
    if (e->button()==Qt::LeftButton) {
        RS_CoordinateEvent ce(snapPoint(e));
        coordinateEvent(&ce);
    } else if (e->button()==Qt::RightButton) {
        deletePreview();
        init(getStatus()-1);
    }
}



void RS_ActionDrawCircle3P::coordinateEvent(RS_CoordinateEvent* e) {
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



void RS_ActionDrawCircle3P::commandEvent(RS_CommandEvent* e) {
    RS_String c = e->getCommand().lower();

    if (checkCommand("help", c)) {
        RS_DIALOGFACTORY->commandMessage(msgAvailableCommands()
                                         + getAvailableCommands().join(", "));
        return;
    }
}



RS_StringList RS_ActionDrawCircle3P::getAvailableCommands() {
    RS_StringList cmd;
    return cmd;
}


void RS_ActionDrawCircle3P::updateMouseButtonHints() {
    switch (getStatus()) {
    case SetPoint1:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Specify first point"),
                                            tr("Cancel"));
        break;
    case SetPoint2:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Specify second point"),
                                            tr("Back"));
        break;
    case SetPoint3:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Specify third point"),
                                            tr("Back"));
        break;
    default:
        RS_DIALOGFACTORY->updateMouseWidget("", "");
        break;
    }
}



void RS_ActionDrawCircle3P::updateMouseCursor() {
    graphicView->setMouseCursor(RS2::CadCursor);
}



void RS_ActionDrawCircle3P::updateToolBar() {
    if (!isFinished()) {
        RS_DIALOGFACTORY->requestToolBar(RS2::ToolBarSnap);
    } else {
        RS_DIALOGFACTORY->requestToolBar(RS2::ToolBarCircles);
    }
}


// EOF

