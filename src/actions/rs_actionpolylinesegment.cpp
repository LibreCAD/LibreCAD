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

#include "rs_actionpolylinesegment.h"
#include "rs_polyline.h"
#include "rs_snapper.h"



RS_ActionPolylineSegment::RS_ActionPolylineSegment(RS_EntityContainer& container,
        RS_GraphicView& graphicView)
        :RS_PreviewActionInterface("Create Polyline Existing from Segments",
                           container, graphicView) {}


QAction* RS_ActionPolylineSegment::createGUIAction(RS2::ActionType /*type*/, QObject* /*parent*/) {
    QAction* action = new QAction(tr("Create Polyline from Existing &Segments"), NULL);
	action->setShortcut(QKeySequence());
    action->setStatusTip(tr("Create Polyline from Existing Segments"));
    return action;
}

void RS_ActionPolylineSegment::init(int status) {
    RS_ActionInterface::init(status);
    targetEntity = NULL;
}

/**
 * Rearranges the atomic entities in this container in a way that connected
 * entities are stored in the right order and direction.
 * Non-recoursive. Only affects atomic entities in this container.
 * 
 * @retval true all contours were closed
 * @retval false at least one contour is not closed
 */
bool RS_ActionPolylineSegment::convertPolyline(RS_Entity* selectedEntity) {

	RS_DEBUG->print("RS_ActionPolylineSegment::convertPolyline");

	RS_Vector current(false);
	RS_Vector start(false);
	RS_Vector end(false);
	RS_EntityContainer tmp;

	bool closed = true;

	int pos = container->findEntity(selectedEntity);
	RS_Entity* e1=container->entityAt(pos);

	if (document!=NULL) {
		document->startUndoCycle();
	}
	if (document!=NULL) {
		if (e1!=NULL && e1->isEdge() && !e1->isContainer() &&
					!e1->isProcessed()) {

			RS_AtomicEntity* ce = (RS_AtomicEntity*)e1;

///////////////////////////////////////////////////
			ce->setUndoState(true);
			document->addUndoable(ce);
///////////////////////////////////////////////////

			// next contour start:
			ce->setProcessed(true);
			tmp.addEntity(ce->clone());
			current = ce->getStartpoint();
			end = ce->getEndpoint();

			// find first connected entities:
			for (int ei=pos-1; ei>=0; --ei) {
				RS_Entity* e2=container->entityAt(ei);

				if (e2!=NULL && e2->isEdge() && !e2->isContainer() &&
						!e2->isProcessed()) {

					RS_AtomicEntity* e = (RS_AtomicEntity*)e2;
///////////////////////////////////////////////////
					e->setUndoState(true);
					document->addUndoable(e);
///////////////////////////////////////////////////
					if (e->getEndpoint().distanceTo(current) <
							1.0e-4) {
						e->setProcessed(true);
						tmp.insertEntity(0,e->clone());
						current = e->getStartpoint();
					} else if (e->getStartpoint().distanceTo(current) <
							   1.0e-4) {
						e->setProcessed(true);
						RS_AtomicEntity* cl = (RS_AtomicEntity*)e->clone();
						cl->reverse();
						tmp.insertEntity(0,cl);
						current = cl->getStartpoint();
					}else
						break;
				}
			}

			if (current.distanceTo(end)>1.0e-4) {
				closed = false;
			}

			current = ce->getEndpoint();
			start = ce->getStartpoint();
			// find last connected entities:
			for (uint ei=pos+1; ei<container->count(); ++ei) {
				RS_Entity* e2=container->entityAt(ei);
///////////////////////////////////////////////////
				e2->setUndoState(true);
				document->addUndoable(e2);
///////////////////////////////////////////////////
				if (e2!=NULL && e2->isEdge() && !e2->isContainer() &&
						!e2->isProcessed()) {
					RS_AtomicEntity* e = (RS_AtomicEntity*)e2;
					if (e->getStartpoint().distanceTo(current) <
							1.0e-4) {
						e->setProcessed(true);
						tmp.addEntity(e->clone());
						current = e->getEndpoint();
					} else if (e->getEndpoint().distanceTo(current) <
							   1.0e-4) {
						e->setProcessed(true);
						RS_AtomicEntity* cl = (RS_AtomicEntity*)e->clone();
						cl->reverse();
						tmp.addEntity(cl);
						current = cl->getEndpoint();
					}else
						break;
				}
			}
			if (current.distanceTo(start)>1.0e-4) {
				closed = false;
			}
		}
	}
	if (document!=NULL) {
		document->endUndoCycle();
	}

	RS_Polyline* newPolyline = new RS_Polyline(container, RS_PolylineData(RS_Vector(false), RS_Vector(false), closed));
	newPolyline->setLayerToActive();
	newPolyline->setPenToActive();
	// add new polyline:
	bool first = true;
	RS_Entity* lastEntity = tmp.lastEntity();
	for (RS_Entity* en=tmp.firstEntity(); en!=NULL; en=tmp.nextEntity()) {
		en->setProcessed(false);
		double bulge = 0.0;
		if (en->rtti()==RS2::EntityArc) {
			bulge = ((RS_Arc*)en)->getBulge();
		} else {
			bulge = 0.0;
		}
		if (first) {
			newPolyline->setNextBulge(bulge);
			newPolyline->addVertex(((RS_AtomicEntity*)en)->getStartpoint());
			first = false;
		}
		if (en!=lastEntity || closed==false){
			newPolyline->setNextBulge(bulge);
			newPolyline->addVertex(((RS_AtomicEntity*)en)->getEndpoint());
		}
	}
	double bulge = lastEntity->rtti() == RS2::EntityArc? ((RS_Arc*)lastEntity)->getBulge():0.0;
	newPolyline->setNextBulge(bulge);
	newPolyline->endPolyline();
	container->addEntity(newPolyline);

	if (graphicView!=NULL) {
		graphicView->drawEntity(newPolyline);
	}

	if (document!=NULL) {
		document->startUndoCycle();
		document->addUndoable(newPolyline);
		document->endUndoCycle();
	}
	RS_DEBUG->print("RS_ActionPolylineSegment::convertPolyline: OK");
	return closed;
}

void RS_ActionPolylineSegment::trigger() {

    RS_DEBUG->print("RS_ActionPolylineSegment::trigger()");

	if (targetEntity!=NULL /*&& selectedSegment!=NULL && targetPoint.valid */) {
        targetEntity->setHighlighted(false);
        graphicView->drawEntity(targetEntity);
		container->optimizeContours();
		convertPolyline(targetEntity);

        targetEntity = NULL;
        setStatus(ChooseEntity);

        RS_DIALOGFACTORY->updateSelectionWidget(container->countSelected());
    }
////////////////////////////////////////2006/06/15
		graphicView->redraw();
////////////////////////////////////////
}



void RS_ActionPolylineSegment::mouseMoveEvent(RS_MouseEvent* e) {
    RS_DEBUG->print("RS_ActionPolylineSegment::mouseMoveEvent begin");

    switch (getStatus()) {
    case ChooseEntity:
        snapPoint(e);
        break;
    default:
        break;
    }


    RS_DEBUG->print("RS_ActionPolylineSegment::mouseMoveEvent end");
}



void RS_ActionPolylineSegment::mouseReleaseEvent(RS_MouseEvent* e) {
    if (RS2::qtToRsButtonState(e->button())==RS2::LeftButton) {
        switch (getStatus()) {
        case ChooseEntity:
            targetEntity = catchEntity(e);
            if (targetEntity==NULL) {
                RS_DIALOGFACTORY->commandMessage(tr("No Entity found."));
            } else if (targetEntity->rtti()!=RS2::EntityLine && targetEntity->rtti()!=RS2::EntityArc) {
                RS_DIALOGFACTORY->commandMessage(
                    tr("Entity must be a line or arc."));
            } else {
                targetEntity->setHighlighted(true);
                graphicView->drawEntity(targetEntity);
//                setStatus(SetReferencePoint);
////////////////////////////////////////2006/06/15
				graphicView->redraw();
////////////////////////////////////////
            	trigger();
            }
            break;
        default:
            break;
        }
    } else if (RS2::qtToRsButtonState(e->button())==RS2::RightButton) {
        deleteSnapper();
        if (targetEntity!=NULL) {
            targetEntity->setHighlighted(false);
            graphicView->drawEntity(targetEntity);
////////////////////////////////////////2006/06/15
		graphicView->redraw();
////////////////////////////////////////
        }
        init(getStatus()-1);
    }
/*    if (RS2::qtToRsButtonState(e->button())==RS2::LeftButton) {
        RS_CoordinateEvent ce(snapPoint(e));
        coordinateEvent(&ce);
    } else if (RS2::qtToRsButtonState(e->button())==RS2::RightButton) {
        deletePreview();
        deleteSnapper
        init(getStatus()-1);
    }
*/
}

void RS_ActionPolylineSegment::updateMouseButtonHints() {
    switch (getStatus()) {
    case ChooseEntity:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Choose one of the segments on the original polyline"),
                                            tr("Cancel"));
        break;
    default:
        RS_DIALOGFACTORY->updateMouseWidget("", "");
        break;
    }
}



void RS_ActionPolylineSegment::updateMouseCursor() {
    graphicView->setMouseCursor(RS2::CadCursor);
}



void RS_ActionPolylineSegment::updateToolBar() {
    switch (getStatus()) {
    case ChooseEntity:
        RS_DIALOGFACTORY->requestToolBar(RS2::ToolBarSnap);
        break;
    default:
        RS_DIALOGFACTORY->requestToolBar(RS2::ToolBarPolylines);
        break;
    }
}


// EOF
