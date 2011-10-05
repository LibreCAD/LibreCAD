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

#include "rs_actionpolylineadd.h"

#include <QAction>
#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_modification.h"
#include "rs_polyline.h"



RS_ActionPolylineAdd::RS_ActionPolylineAdd(RS_EntityContainer& container,
        RS_GraphicView& graphicView)
        :RS_PreviewActionInterface("Add node",
                           container, graphicView) {}


QAction* RS_ActionPolylineAdd::createGUIAction(RS2::ActionType /*type*/, QObject* /*parent*/) {
        QAction* action = new QAction(tr("&Add node"), NULL);
        action->setShortcut(QKeySequence());
        action->setIcon(QIcon(":/extui/polylineadd.png"));
        action->setStatusTip(tr("Add polyline's node"));
        return action;
}

void RS_ActionPolylineAdd::init(int status) {
        RS_ActionInterface::init(status);
        addEntity = addSegment = NULL;
        addCoord = RS_Vector(false);
}



void RS_ActionPolylineAdd::trigger() {

        RS_PreviewActionInterface::trigger();
        RS_DEBUG->print("RS_ActionPolylineAdd::trigger()");

        if (addEntity!=NULL && addSegment->isAtomic() && addCoord.valid &&
                addSegment->isPointOnEntity(addCoord)) {

                addEntity->setHighlighted(false);
                graphicView->drawEntity(addEntity);

                RS_Modification m(*container, graphicView);
                addEntity = m.addPolylineNode((RS_Polyline&)*addEntity, (RS_AtomicEntity&)*addSegment, addCoord );

                addCoord = RS_Vector(false);

                RS_DIALOGFACTORY->updateSelectionWidget(container->countSelected(),container->totalSelectedLength());
        }
////////////////////////////////////////2006/06/15
                graphicView->redraw(RS2::RedrawDrawing);
////////////////////////////////////////
}



void RS_ActionPolylineAdd::mouseMoveEvent(QMouseEvent* e) {
        RS_DEBUG->print("RS_ActionPolylineAdd::mouseMoveEvent begin");

        switch (getStatus()) {
        case ChooseSegment:
                break;
        case SetAddCoord:
                snapPoint(e);
                break;
        default:
                break;
        }

        RS_DEBUG->print("RS_ActionPolylineAdd::mouseMoveEvent end");
}



void RS_ActionPolylineAdd::mouseReleaseEvent(QMouseEvent* e) {
        if (e->button()==Qt::LeftButton) {
                switch (getStatus()) {
                case ChooseSegment:
                        addEntity = catchEntity(e);
                        if (addEntity==NULL) {
                        RS_DIALOGFACTORY->commandMessage(tr("No Entity found."));
                        } else if (addEntity->rtti()!=RS2::EntityPolyline) {

                        RS_DIALOGFACTORY->commandMessage(
                                tr("Entity must be a polyline."));
                        } else {
                                addEntity->setHighlighted(true);
                                graphicView->drawEntity(addEntity);
                                setStatus(SetAddCoord);
////////////////////////////////////////2006/06/15
                                graphicView->redraw(RS2::RedrawDrawing);
////////////////////////////////////////
                        }
                        break;

                case SetAddCoord:
                        addCoord = snapPoint(e);
                        if (addEntity==NULL) {
                                RS_DIALOGFACTORY->commandMessage(tr("No Entity found."));
                        } else if (!addCoord.valid) {
                                RS_DIALOGFACTORY->commandMessage(tr("Adding point is invalid."));
                        } else {
                                RS_Vector clickCoord = snapPoint(e);
                                addSegment = NULL;
                                double dist = graphicView->toGraphDX(snapRange)*0.9;
                                addSegment =  ((RS_Polyline*)addEntity)->getNearestEntity( clickCoord, &dist, RS2::ResolveNone);
                                if(addSegment == NULL) {
                                        RS_DIALOGFACTORY->commandMessage(
                                                        tr("Adding point is not on entity."));
                                        break;
                                }
                                deleteSnapper();
                                trigger();
                        }
                        break;

                default:
                        break;
                }
        } else if (e->button()==Qt::RightButton) {
                deleteSnapper();
                if (addEntity!=NULL) {
                        addEntity->setHighlighted(false);
                        graphicView->drawEntity(addEntity);
////////////////////////////////////////2006/06/15
                        graphicView->redraw(RS2::RedrawDrawing);
////////////////////////////////////////
                }
                init(getStatus()-1);
        }
}


void RS_ActionPolylineAdd::updateMouseButtonHints() {
        switch (getStatus()) {
        case ChooseSegment:
                RS_DIALOGFACTORY->updateMouseWidget(tr("Specify polyline to add nodes"),
                                                tr("Cancel"));
                break;
        case SetAddCoord:
                RS_DIALOGFACTORY->updateMouseWidget(tr("Specify adding node's point"),
                                                tr("Back"));
                break;
        default:
                RS_DIALOGFACTORY->updateMouseWidget("", "");
                break;
        }
}



void RS_ActionPolylineAdd::updateMouseCursor() {
        graphicView->setMouseCursor(RS2::CadCursor);
}



void RS_ActionPolylineAdd::updateToolBar() {
    //not needed any more with new snap
    return;
        switch (getStatus()) {
        case SetAddCoord:
        case ChooseSegment:
                RS_DIALOGFACTORY->requestToolBar(RS2::ToolBarSnap);
                break;
        default:
                RS_DIALOGFACTORY->requestToolBar(RS2::ToolBarPolylines);
                break;
        }
}


// EOF
