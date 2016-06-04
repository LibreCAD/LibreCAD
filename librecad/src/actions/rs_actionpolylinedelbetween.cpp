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
#include "rs_actionpolylinedelbetween.h"

#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_modification.h"
#include "rs_polyline.h"
#include "rs_atomicentity.h"
#include "rs_preview.h"
#include "rs_debug.h"

struct RS_ActionPolylineDelBetween::Points {
	RS_Vector nodePoint1;
	RS_Vector nodePoint2;
};

RS_ActionPolylineDelBetween::RS_ActionPolylineDelBetween(RS_EntityContainer& container,
                RS_GraphicView& graphicView)
                :RS_PreviewActionInterface("Delete between two nodes",
												   container, graphicView)
				, pPoints(new Points{})
{
	actionType=RS2::ActionPolylineDelBetween;
}

RS_ActionPolylineDelBetween::~RS_ActionPolylineDelBetween()=default;


void RS_ActionPolylineDelBetween::init(int status) {
        RS_ActionInterface::init(status);
		delEntity = nullptr;
		delSegment = nullptr;
		pPoints->nodePoint1 = pPoints->nodePoint2 = RS_Vector(false);
}



void RS_ActionPolylineDelBetween::trigger() {

        RS_DEBUG->print("RS_ActionPolylineDelBetween::trigger()");

		if (delEntity && delSegment->isAtomic() && pPoints->nodePoint1.valid && pPoints->nodePoint2.valid) {

                delEntity->setHighlighted(false);
                graphicView->drawEntity(delEntity);

                RS_Modification m(*container, graphicView);
				delEntity = m.deletePolylineNodesBetween((RS_Polyline&)*delEntity, (RS_AtomicEntity&)*delSegment, pPoints->nodePoint1, pPoints->nodePoint2 );

				delEntity = delSegment = nullptr;
				pPoints->nodePoint1 = pPoints->nodePoint2 = RS_Vector(false);
                setStatus(SetNodePoint1);

                RS_DIALOGFACTORY->updateSelectionWidget(container->countSelected(),container->totalSelectedLength());
        }
////////////////////////////////////////2006/06/15
                graphicView->redraw();
////////////////////////////////////////
}



void RS_ActionPolylineDelBetween::mouseMoveEvent(QMouseEvent* e) {
        RS_DEBUG->print("RS_ActionPolylineDelBetween::mouseMoveEvent begin");

        switch (getStatus()) {
        case ChooseSegment:
                break;

        case SetNodePoint1:
                snapPoint(e);
                break;

        case SetNodePoint2:
                snapPoint(e);
                break;
        default:
                break;
        }

        RS_DEBUG->print("RS_ActionPolylineDelBetween::mouseMoveEvent end");
}



void RS_ActionPolylineDelBetween::mouseReleaseEvent(QMouseEvent* e) {
        if (e->button()==Qt::LeftButton) {
                switch (getStatus()) {
                case ChooseSegment:
                        delEntity = catchEntity(e);
						if (delEntity==nullptr) {
                                RS_DIALOGFACTORY->commandMessage(tr("No Entity found."));
                        } else if (delEntity->rtti()!=RS2::EntityPolyline) {

                                RS_DIALOGFACTORY->commandMessage(
                                        tr("Entity must be a polyline."));
                        } else {
							snapPoint(e);
								delSegment = nullptr;
                                double dist = graphicView->toGraphDX(snapRange)*0.9;
                                delSegment =  (RS_AtomicEntity*)((RS_Polyline*)delEntity)->getNearestEntity( RS_Vector(graphicView->toGraphX(e->x()),
                                                                        graphicView->toGraphY(e->y())), &dist, RS2::ResolveNone);
								if(delSegment == nullptr)
                                        break;
                                delEntity->setHighlighted(true);
                                graphicView->drawEntity(delEntity);
                                setStatus(SetNodePoint1);
////////////////////////////////////////2006/06/15
                graphicView->redraw();
////////////////////////////////////////
                        }
                        break;

                case SetNodePoint1:
						pPoints->nodePoint1 = snapPoint(e);
						if (delEntity==nullptr) {
                                RS_DIALOGFACTORY->commandMessage(tr("No Entity found."));
						} else if (!pPoints->nodePoint1.valid) {
                                RS_DIALOGFACTORY->commandMessage(tr("Deleting point is invalid."));
						} else if (!delEntity->isPointOnEntity(pPoints->nodePoint1)) {
                                RS_DIALOGFACTORY->commandMessage(
                                        tr("Deleting point is not on entity."));
                        }else{
                                setStatus(SetNodePoint2);
                        }
                        break;
                case SetNodePoint2:
						pPoints->nodePoint2 = snapPoint(e);
						if (delEntity==nullptr) {
                                RS_DIALOGFACTORY->commandMessage(tr("No Entity found."));
						} else if (!pPoints->nodePoint2.valid) {
                                RS_DIALOGFACTORY->commandMessage(tr("Deleting point is invalid."));
						} else if (!delEntity->isPointOnEntity(pPoints->nodePoint2)) {
                                RS_DIALOGFACTORY->commandMessage(
                                        tr("Deleting point is not on entity."));
                        } else {
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


void RS_ActionPolylineDelBetween::updateMouseButtonHints() {
        switch (getStatus()) {
        case ChooseSegment:
                RS_DIALOGFACTORY->updateMouseWidget(tr("Specify polyline to delete between two nodes"),
                                                                                        tr("Cancel"));
                break;
        case SetNodePoint1:
                RS_DIALOGFACTORY->updateMouseWidget(tr("Specify first node"),
                                                                                        tr("Back"));
                break;
        case SetNodePoint2:
                RS_DIALOGFACTORY->updateMouseWidget(tr("Specify second node"),
                                                                                        tr("Back"));
                break;
        default:
                RS_DIALOGFACTORY->updateMouseWidget();
                break;
        }
}



void RS_ActionPolylineDelBetween::updateMouseCursor() {
        graphicView->setMouseCursor(RS2::SelectCursor);
}

// EOF
