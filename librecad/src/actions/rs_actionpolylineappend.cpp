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
#include "rs_actionpolylineappend.h"

#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_commands.h"
#include "rs_polyline.h"
#include "rs_coordinateevent.h"
#include "rs_arc.h"
#include "rs_line.h"
#include "rs_preview.h"
#include "rs_debug.h"

struct RS_ActionDrawPolyline::Points {

	/**
	 * Line data defined so far.
	 */
	RS_PolylineData data;
	RS_ArcData arc_data;
	/**
	 * Polyline entity we're working on.
	 */
	RS_Polyline* polyline;

	/**
	 * last point.
	 */
	RS_Vector point;
	RS_Vector calculatedEndpoint;
	/**
	 * Start point of the series of lines. Used for close function.
	 */
	RS_Vector start;

	/**
	 * Point history (for undo)
	 */
		QList<RS_Vector> history;

	/**
	 * Bulge history (for undo)
	 */
		QList<double> bHistory;
};

RS_ActionPolylineAppend::RS_ActionPolylineAppend(RS_EntityContainer& container,
	RS_GraphicView& graphicView)
	:RS_ActionDrawPolyline(container, graphicView) {
	actionType=RS2::ActionPolylineAppend;
}

void RS_ActionPolylineAppend::trigger() {

	RS_DEBUG->print("RS_ActionPolylineAppend::trigger()");

	RS_PreviewActionInterface::trigger();

	if (!pPoints->polyline) {
		return;
	}

	// add the entity
	//RS_Polyline* polyline = new RS_Polyline(container, data);
	pPoints->polyline->setLayerToActive();
	pPoints->polyline->setPenToActive();

	// upd. undo list:
	if (document) {
		document->startUndoCycle();
                // RVT_PORT need to decide on how to handle undo cycles
                originalPolyline->setUndoState(true);
		document->addUndoable(originalPolyline);
		document->addUndoable(pPoints->polyline);
                document->endUndoCycle();
	}

	// upd view
	deleteSnapper();
	graphicView->moveRelativeZero(RS_Vector(0.0,0.0));
	graphicView->deleteEntity(originalPolyline);
	graphicView->drawEntity(pPoints->polyline);
	graphicView->moveRelativeZero(pPoints->polyline->getEndpoint());
	drawSnapper();
	RS_DEBUG->print("RS_ActionDrawPolyline::trigger(): polyline added: %d",
					pPoints->polyline->getId());

	pPoints->polyline = nullptr;
}


void RS_ActionPolylineAppend::mouseReleaseEvent(QMouseEvent* e) {
        if (e->button()==Qt::LeftButton) {
		if (getStatus()==SetStartpoint) {
			originalPolyline = dynamic_cast<RS_Polyline*>(catchEntity(e));
			if (!originalPolyline) {
				RS_DIALOGFACTORY->commandMessage(tr("No Entity found."));
                                return;
			} else if (originalPolyline->rtti()!=RS2::EntityPolyline) {
				RS_DIALOGFACTORY->commandMessage(
					tr("Entity must be a polyline."));
                                return;
                        } else if (originalPolyline->isClosed()) {
                            RS_DIALOGFACTORY->commandMessage(
                                    tr("Can not append nodes in a closed polyline."));
                            return;
                        } else {
				snapPoint(e);
				RS_Polyline* op=static_cast<RS_Polyline*>(originalPolyline);
				RS_Entity* entFirst = op->firstEntity();
				RS_Entity* entLast = op->lastEntity();
				double dist = graphicView->toGraphDX(snapRange)*0.9;
				RS_Entity* nearestSegment = originalPolyline->getNearestEntity( RS_Vector(graphicView->toGraphX(e->x()),
									graphicView->toGraphY(e->y())), &dist, RS2::ResolveNone);
				pPoints->polyline = static_cast<RS_Polyline*>(originalPolyline->clone());
				container->addEntity(pPoints->polyline);
				prepend = false;
				if (nearestSegment == entFirst){
					prepend = true;
					pPoints->point = originalPolyline->getStartpoint();
				}else if (nearestSegment == entLast){
					pPoints->point = originalPolyline->getEndpoint();
				}else{
					RS_DIALOGFACTORY->commandMessage(
						tr("Click somewhere near the beginning or end of existing polyline."));
				}
			}
		}
		RS_CoordinateEvent ce(snapPoint(e));
		coordinateEvent(&ce);
        } else if (e->button()==Qt::RightButton) {
		if (getStatus()==SetNextPoint) {
			trigger();
		}
		// deletePreview();
		//clearPreview();
		deleteSnapper();
		init(getStatus()-1);
	}
}

void RS_ActionPolylineAppend::coordinateEvent(RS_CoordinateEvent* e) {
	if (!e) {
		return;
	}

	RS_Vector mouse = e->getCoordinate();

	switch (getStatus()) {
	case SetStartpoint:
		pPoints->history.clear();
				pPoints->history.append(pPoints->point);
		pPoints->bHistory.clear();
				pPoints->bHistory.append(0.0);
		pPoints->start = pPoints->point;
		setStatus(SetNextPoint);
		graphicView->moveRelativeZero(pPoints->point);
		updateMouseButtonHints();
		break;

	case SetNextPoint:
		graphicView->moveRelativeZero(mouse);
		pPoints->point = mouse;
				pPoints->history.append(mouse);
				pPoints->bHistory.append(0.0);
				if (!pPoints->polyline) {
			pPoints->polyline = new RS_Polyline(container, pPoints->data);
			pPoints->polyline->addVertex(pPoints->start, 0.0, prepend);
		}
		if (pPoints->polyline) {
			pPoints->polyline->addVertex(mouse, 0.0, prepend);
//			polyline->setEndpoint(mouse);
			if (pPoints->polyline->count()==1) {
				pPoints->polyline->setLayerToActive();
				pPoints->polyline->setPenToActive();
			}
                        // RVT_PORT (can be deleted) deletePreview();
			//clearPreview();
			deleteSnapper();
			graphicView->drawEntity(pPoints->polyline);
			drawSnapper();
		}
		//trigger();
		//data.startpoint = data.endpoint;
                updateMouseButtonHints();
		//graphicView->moveRelativeZero(mouse);
		break;

	default:
		break;
	}
}

void RS_ActionPolylineAppend::updateMouseButtonHints() {
	switch (getStatus()) {
	case SetStartpoint:
		RS_DIALOGFACTORY->updateMouseWidget(tr("Specify the polyline somewhere near the beginning or end point"),
						tr("Cancel"));
		break;
	case SetNextPoint: {
                        QString msg = "";

						if (pPoints->history.size()>=3) {
				msg += RS_COMMANDS->command("close");
				msg += "/";
			}
						if (pPoints->history.size()>=2) {
				msg += RS_COMMANDS->command("undo");
			}

						if (pPoints->history.size()>=2) {
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
		RS_DIALOGFACTORY->updateMouseWidget();
		break;
	}
}

void RS_ActionPolylineAppend::undo() {
	if (pPoints->history.size()>1) {
		pPoints->history.removeLast();
		pPoints->bHistory.removeLast();
        deletePreview();
		pPoints->point = pPoints->history.last();
    } else {
        RS_DIALOGFACTORY->commandMessage(
            tr("Cannot undo: "
               "Not enough entities defined yet."));
    }

}

// EOF
