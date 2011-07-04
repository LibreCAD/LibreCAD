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

#include "rs_actionpolylineappend.h"



RS_ActionPolylineAppend::RS_ActionPolylineAppend(RS_EntityContainer& container,
	RS_GraphicView& graphicView)
	:RS_ActionDrawPolyline(container, graphicView) {}

QAction* RS_ActionPolylineAppend::createGUIAction(RS2::ActionType /*type*/, QObject* /*parent*/) {
	QAction* action = new QAction(tr("A&ppend node"), NULL);
	action->setShortcut(QKeySequence());
	action->setStatusTip(tr("Append polyline's node"));
	return action;
}

void RS_ActionPolylineAppend::trigger() {

	RS_DEBUG->print("RS_ActionPolylineAppend::trigger()");

	RS_PreviewActionInterface::trigger();

	if (polyline==NULL) {
		return;
	}

	// add the entity
	//RS_Polyline* polyline = new RS_Polyline(container, data);
	polyline->setLayerToActive();
	polyline->setPenToActive();
	container->addEntity(polyline);

	// upd. undo list:
	if (document!=NULL) {
		document->startUndoCycle();
                // RVT_PORT need to decide on how to handle undy cycles
                // originalPolyline->setUndoState(true);
		document->addUndoable(originalPolyline);
		document->addUndoable(polyline);
                document->endUndoCycle();
	}

	// upd view
	deleteSnapper();
	graphicView->moveRelativeZero(RS_Vector(0.0,0.0));
	graphicView->deleteEntity(originalPolyline);
	graphicView->drawEntity(polyline);
	graphicView->moveRelativeZero(polyline->getEndpoint());
	drawSnapper();
	RS_DEBUG->print("RS_ActionDrawPolyline::trigger(): polyline added: %d",
					polyline->getId());

	polyline = NULL;
}


void RS_ActionPolylineAppend::mouseReleaseEvent(QMouseEvent* e) {
        if (e->button()==Qt::LeftButton) {
		if (getStatus()==SetStartpoint) {
			originalPolyline = (RS_Polyline*)catchEntity(e);
			if (originalPolyline==NULL) {
				RS_DIALOGFACTORY->commandMessage(tr("No Entity found."));
			} else if (originalPolyline->rtti()!=RS2::EntityPolyline) {
				RS_DIALOGFACTORY->commandMessage(
					tr("Entity must be a polyline."));
			} else {
				RS_Vector clickCoord = snapPoint(e);
				RS_Entity* entFirst = ((RS_Polyline*)originalPolyline)->firstEntity();
				RS_Entity* entLast = ((RS_Polyline*)originalPolyline)->lastEntity();
				double dist = graphicView->toGraphDX(snapRange)*0.9;
				RS_Entity* nearestSegment = originalPolyline->getNearestEntity( RS_Vector(graphicView->toGraphX(e->x()),
									graphicView->toGraphY(e->y())), &dist, RS2::ResolveNone);
				polyline = (RS_Polyline*)originalPolyline->clone();
				prepend = false;
				if (nearestSegment == entFirst){
					prepend = true;
					point = entFirst->getNearestEndpoint(clickCoord, &dist);
				}else if (nearestSegment == entLast){
					point = entLast->getNearestEndpoint(clickCoord, &dist);
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
	if (e==NULL) {
		return;
	}

	RS_Vector mouse = e->getCoordinate();

	switch (getStatus()) {
	case SetStartpoint:
		history.clear();
                history.append(point);
		bHistory.clear();
                bHistory.append(0.0);
		start = point;
		setStatus(SetNextPoint);
		graphicView->moveRelativeZero(point);
		updateMouseButtonHints();
		break;

	case SetNextPoint:
		graphicView->moveRelativeZero(mouse);
		point = mouse;
                history.append(mouse);
                bHistory.append(0.0);
                if (polyline==NULL) {
			polyline = new RS_Polyline(container, data);
			polyline->addVertex(start, 0.0, prepend);
		}
		if (polyline!=NULL) {
			polyline->addVertex(mouse, 0.0, prepend);
//			polyline->setEndpoint(mouse);
			if (polyline->count()==1) {
				polyline->setLayerToActive();
				polyline->setPenToActive();
				container->addEntity(polyline);
			}
			// RVT_PORT (can be deleted) deletePreview();
			//clearPreview();
			deleteSnapper();
			graphicView->drawEntity(polyline);
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

                        if (history.size()>=3) {
				msg += RS_COMMANDS->command("close");
				msg += "/";
			}
                        if (history.size()>=2) {
				msg += RS_COMMANDS->command("undo");
			}

                        if (history.size()>=2) {
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
// EOF
