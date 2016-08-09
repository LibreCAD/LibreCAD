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
#include "rs_actionpolylinesegment.h"

#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_arc.h"
#include "rs_line.h"
#include "rs_polyline.h"
#include "rs_debug.h"

namespace {
std::initializer_list<RS2::EntityType>
entityType{RS2::EntityLine, RS2::EntityPolyline, RS2::EntityArc};
}

RS_ActionPolylineSegment::RS_ActionPolylineSegment(RS_EntityContainer& container,
        RS_GraphicView& graphicView)
        :RS_PreviewActionInterface("Create Polyline Existing from Segments",
						   container, graphicView) {
	actionType=RS2::ActionPolylineSegment;
}

void RS_ActionPolylineSegment::init(int status) {
    RS_ActionInterface::init(status);
	targetEntity = nullptr;
	//Experimental feature: trigger action, if already has selected entities
	if (container->countSelected(true, entityType)) {
		//find a selected entity
		//TODO, find a better starting point
		for (RS_Entity* e : *container) {
			if (e->isSelected() &&
					std::count(entityType.begin(), entityType.end(), e->rtti())) {
				targetEntity = e;
				break;
			}
		}
		if (targetEntity) {
			convertPolyline(targetEntity, true);
			RS_DIALOGFACTORY->commandMessage(tr("Polyline created"));
			graphicView->redraw();
			RS_DIALOGFACTORY->updateSelectionWidget(container->countSelected(),container->totalSelectedLength());
			finish(false);
			return;
		}
	}
}

/**
 * Utility function for convertPolyline
 * Appends in current the vertex from toAdd reversing if reversed is true
 * The first vertex is not added and the last is returned instead of added
 *
 * @retval RS_Vector with the last vertex not inserted
 *
 * @author Rallaz
 */
RS_Vector RS_ActionPolylineSegment::appendPol(RS_Polyline *current, RS_Polyline *toAdd, bool reversed) {

    QList<RS_Entity*> entities;

	for(auto v: *toAdd){
        if (reversed)
            entities.prepend(v);
        else
            entities.append(v);
    }
//bad polyline without vertex
    if (entities.isEmpty())
        return RS_Vector(false);

    double bulge=0.0;
    RS_Entity* e = entities.takeFirst() ;

//First polyline vertex
    if (e->rtti() == RS2::EntityArc) {
        if (reversed)
            current->setNextBulge(((RS_Arc*)e)->getBulge()*-1);
        else
            current->setNextBulge(((RS_Arc*)e)->getBulge());
    }

    while (!entities.isEmpty()) {
         e = entities.takeFirst();
         if (e->rtti()==RS2::EntityArc) {
         if (reversed)
             bulge = ((RS_Arc*)e)->getBulge()*-1;
         else
             bulge = ((RS_Arc*)e)->getBulge();
         } else
             bulge = 0.0;
         if (reversed)
             current->addVertex(e->getEndpoint(),bulge, false);
         else
             current->addVertex(e->getStartpoint(),bulge, false);
    }
    if (reversed)
        return e->getStartpoint();
    else
        return e->getEndpoint();
}

/**
 * Rearranges the lines, arcs or opened polylines entities
 *  in this container, non-recoursive.
 * document can not be null
 *
 * @retval true contour are closed
 * @retval false if the contour is not closed
 *
 * @author Rallaz
 */
bool RS_ActionPolylineSegment::convertPolyline(RS_Entity* selectedEntity, bool useSelected) {

    RS_DEBUG->print("RS_ActionPolylineSegment::convertPolyline");

    QList<RS_Entity*> remaining;
    QList<RS_Entity*> completed;
    RS_Vector start = selectedEntity->getStartpoint();
    RS_Vector end = selectedEntity->getEndpoint();
	if (!useSelected || (selectedEntity && selectedEntity->isSelected()))
		completed.append(selectedEntity);
//get list with useful entities

	for (RS_Entity* e1 : *container) {
		if (useSelected && !e1->isSelected()) continue;
        if (e1->isLocked() || !e1->isVisible() || e1 == selectedEntity) continue;
        if (e1->rtti()==RS2::EntityLine || e1->rtti()==RS2::EntityArc
                || e1->rtti()==RS2::EntityPolyline) {
            if (targetEntity->rtti()==RS2::EntityPolyline && ((RS_Polyline*)targetEntity)->isClosed())
                continue;
            if (e1 == selectedEntity)
                continue;
            remaining.append(e1);
        }
    }

    // find all connected entities:
    bool done = true;
    do {
        done = true;
        for (int i=(remaining.size() -1) ; i>=0; --i) {
            RS_Entity* e=remaining.at(i);
            if (e->getEndpoint().distanceTo(start) < 1.0e-4) {
                completed.prepend( e);
                start = e->getStartpoint();
                remaining.removeAt(i);
                done = false;
            } else if (e->getStartpoint().distanceTo(start) < 1.0e-4) {
                completed.prepend( e);
                start = e->getEndpoint();
                remaining.removeAt(i);
                done = false;
            } else if (e->getEndpoint().distanceTo(end) < 1.0e-4) {
                completed.append( e);
                end = e->getStartpoint();
                remaining.removeAt(i);
                done = false;
            } else if (e->getStartpoint().distanceTo(end) < 1.0e-4) {
                completed.append( e);
                end = e->getEndpoint();
                remaining.removeAt(i);
                done = false;
            }
        }
    } while (!done);

//cleanup for no more needed list
    remaining.clear();

    bool closed = false;
    if (document) {
        document->startUndoCycle();

        bool revert = false;
        double bulge = 0.0;
        if (end.distanceTo(start) < 1.0e-4)
            closed = true;

        RS_Polyline* newPolyline = new RS_Polyline(container, RS_PolylineData(RS_Vector(false), RS_Vector(false), closed));
        newPolyline->setLayerToActive();
        newPolyline->setPenToActive();

//complete polyline
        end =start;
        while (!completed.isEmpty()) {
            RS_Entity* e2= completed.takeFirst();
            e2->setUndoState(true);
            document->addUndoable(e2);
            if (e2->getStartpoint().distanceTo(end) < 1.0e-4) {
                revert = false;
                start = e2->getStartpoint();
                end = e2->getEndpoint();
            } else {
                revert = true;
                start = e2->getEndpoint();
                end = e2->getStartpoint();
            }
            if (e2->rtti()==RS2::EntityArc) {
                if (revert)
                    bulge = ((RS_Arc*)e2)->getBulge()*-1;
                else
                    bulge = ((RS_Arc*)e2)->getBulge();
            } else
                bulge = 0.0;
            if (e2->rtti()==RS2::EntityPolyline) {
                newPolyline->addVertex(start, bulge);
                end = appendPol(newPolyline, (RS_Polyline*)e2, revert);
            } else
                newPolyline->addVertex(start, bulge);
        }

        if (closed)
            newPolyline->setClosed(true);
        else
            newPolyline->addVertex(end, bulge);
        newPolyline->endPolyline();
        container->addEntity(newPolyline);

        if (graphicView) {
                graphicView->drawEntity(newPolyline);
        }

        document->addUndoable(newPolyline);
        document->endUndoCycle();
    }
    RS_DEBUG->print("RS_ActionPolylineSegment::convertPolyline: OK");
    return closed;
}

void RS_ActionPolylineSegment::trigger() {

    RS_DEBUG->print("RS_ActionPolylineSegment::trigger()");

        if (targetEntity /*&& selectedSegment && targetPoint.valid */) {
        targetEntity->setHighlighted(false);
        graphicView->drawEntity(targetEntity);
//RLZ: do not use container->optimizeContours(); because it invalidate targetEntity
//        container->optimizeContours();
        convertPolyline(targetEntity);

		targetEntity = nullptr;
        setStatus(ChooseEntity);

        RS_DIALOGFACTORY->updateSelectionWidget(container->countSelected(),container->totalSelectedLength());
    }
////////////////////////////////////////2006/06/15
                graphicView->redraw();
////////////////////////////////////////
}

void RS_ActionPolylineSegment::mouseReleaseEvent(QMouseEvent* e) {
    if (e->button()==Qt::LeftButton) {
        switch (getStatus()) {
        case ChooseEntity:
			targetEntity = catchEntity(e, entityType);

			if (targetEntity==nullptr) {
                RS_DIALOGFACTORY->commandMessage(tr("No Entity found."));
			} else if (targetEntity->rtti()==RS2::EntityPolyline && ((RS_Polyline*)targetEntity)->isClosed()){
                RS_DIALOGFACTORY->commandMessage(
                        tr("Entity can not be a closed polyline."));
            } else {
				//TODO, verify topology of selected
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
    } else if (e->button()==Qt::RightButton) {
        deleteSnapper();
        if (targetEntity) {
            targetEntity->setHighlighted(false);
            graphicView->drawEntity(targetEntity);
////////////////////////////////////////2006/06/15
                graphicView->redraw();
////////////////////////////////////////
        }
        init(getStatus()-1);
    }
/*    if (e->button())==Qt::LeftButton) {
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
		RS_DIALOGFACTORY->updateMouseWidget();
        break;
    }
}


void RS_ActionPolylineSegment::updateMouseCursor() {
    graphicView->setMouseCursor(RS2::SelectCursor);
}

// EOF
