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

#include <QAction>
#include <QMouseEvent>
#include "rs_actionpolylinetrim.h"

#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_modification.h"
#include "rs_polyline.h"
#include "rs_atomicentity.h"
#include "rs_debug.h"



RS_ActionPolylineTrim::RS_ActionPolylineTrim(RS_EntityContainer& container,
        RS_GraphicView& graphicView)
        :RS_PreviewActionInterface("Trim segments",
						   container, graphicView) {
	actionType=RS2::ActionPolylineTrim;
}

void RS_ActionPolylineTrim::init(int status) {
        RS_ActionInterface::init(status);
        delEntity = Segment1 = Segment2 = NULL;
}



void RS_ActionPolylineTrim::trigger() {

        RS_DEBUG->print("RS_ActionPolylineTrim::trigger()");

        if (delEntity && Segment1->isAtomic() && Segment2->isAtomic()) {

                delEntity->setHighlighted(false);
                graphicView->drawEntity(delEntity);

                RS_Modification m(*container, graphicView);
                delEntity = m.polylineTrim((RS_Polyline&)*delEntity, *Segment1, *Segment2 );

//		delEntity = NULL;
                Segment1 = Segment2 = NULL;
                setStatus(SetSegment1);

                RS_DIALOGFACTORY->updateSelectionWidget(container->countSelected(),container->totalSelectedLength());
        }
////////////////////////////////////////2006/06/15
        graphicView->redraw();
////////////////////////////////////////
}



void RS_ActionPolylineTrim::mouseMoveEvent(QMouseEvent* e) {
        RS_DEBUG->print("RS_ActionPolylineTrim::mouseMoveEvent begin");

        switch (getStatus()) {
        case ChooseEntity:
        break;

        case SetSegment1:
        case SetSegment2:
                snapPoint(e);
                break;

        default:
                break;
        }

        RS_DEBUG->print("RS_ActionPolylineTrim::mouseMoveEvent end");
}



void RS_ActionPolylineTrim::mouseReleaseEvent(QMouseEvent* e) {
        if (e->button()==Qt::LeftButton) {
                RS_Vector cPoint;
                switch (getStatus()) {
                case ChooseEntity:
                        delEntity = catchEntity(e);
                        if (delEntity==NULL) {
                                RS_DIALOGFACTORY->commandMessage(tr("No Entity found."));
                        } else if (delEntity->rtti()!=RS2::EntityPolyline) {
                                RS_DIALOGFACTORY->commandMessage(
                                        tr("Entity must be a polyline."));
                        } else {
                                delEntity->setHighlighted(true);
                                graphicView->drawEntity(delEntity);
                                setStatus(SetSegment1);
////////////////////////////////////////2006/06/15
                                graphicView->redraw();
////////////////////////////////////////
                        }
                        break;

                case SetSegment1:
                        cPoint = snapPoint(e);
                        if (delEntity==NULL) {
                                RS_DIALOGFACTORY->commandMessage(tr("No Entity found."));
                        } else if (!cPoint.valid) {
                                RS_DIALOGFACTORY->commandMessage(tr("Specifying point is invalid."));
                        } else if (!delEntity->isPointOnEntity(cPoint)) {
                                RS_DIALOGFACTORY->commandMessage(
                                        tr("No Segment found on entity."));
                        }else{
                                Segment1 = NULL;
                                        double dist = graphicView->toGraphDX(snapRange)*0.9;
                                Segment1 =  (RS_AtomicEntity*)((RS_Polyline*)delEntity)->getNearestEntity( RS_Vector(graphicView->toGraphX(e->x()),
                                 graphicView->toGraphY(e->y())), &dist, RS2::ResolveNone);
                                if(Segment1 == NULL)
                                        break;
                                setStatus(SetSegment2);
                        }
                        break;
                case SetSegment2:
                        cPoint = snapPoint(e);
                        if (delEntity==NULL) {
                                RS_DIALOGFACTORY->commandMessage(tr("No Entity found."));
                        } else if (!cPoint.valid) {
                                RS_DIALOGFACTORY->commandMessage(tr("Specifying point is invalid."));
                        } else if (!delEntity->isPointOnEntity(cPoint)) {
                                RS_DIALOGFACTORY->commandMessage(
                                        tr("No Segment found on entity."));
                        }else{
                                Segment2 = NULL;
                                        double dist = graphicView->toGraphDX(snapRange)*0.9;
                                Segment2 =  (RS_AtomicEntity*)((RS_Polyline*)delEntity)->getNearestEntity( RS_Vector(graphicView->toGraphX(e->x()),
                                 graphicView->toGraphY(e->y())), &dist, RS2::ResolveNone);
                                if(Segment2 == NULL)
                                        break;
                                deleteSnapper();
                                trigger();
                        }
                        break;

                default:
                        break;
                }
        } else if (e->button()==Qt::RightButton) {
                deleteSnapper();
                if (delEntity) {
                        delEntity->setHighlighted(false);
                        graphicView->drawEntity(delEntity);
////////////////////////////////////////2006/06/15
                        graphicView->redraw();
////////////////////////////////////////
                }
                init(getStatus()-1);
        }
}


void RS_ActionPolylineTrim::updateMouseButtonHints() {
        switch (getStatus()) {
        case ChooseEntity:
                RS_DIALOGFACTORY->updateMouseWidget(tr("Specify polyline to trim"),
                                                        tr("Cancel"));
                break;
        case SetSegment1:
                RS_DIALOGFACTORY->updateMouseWidget(tr("Specify first segment"),
                                                        tr("Back"));
                break;
        case SetSegment2:
                RS_DIALOGFACTORY->updateMouseWidget(tr("Specify second segment"),
                                                        tr("Back"));
                break;
        default:
                RS_DIALOGFACTORY->updateMouseWidget();
                break;
        }
}



void RS_ActionPolylineTrim::updateMouseCursor() {
        graphicView->setMouseCursor(RS2::SelectCursor);
}

// EOF
