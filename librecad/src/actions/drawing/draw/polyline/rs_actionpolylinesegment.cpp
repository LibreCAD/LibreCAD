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


#include <QList>
#include <QMouseEvent>

#include "rs_actionpolylinesegment.h"
#include "rs_arc.h"
#include "rs_debug.h"
#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_preview.h"
#include "rs_polyline.h"

namespace {
QList<RS2::EntityType>
entityType{RS2::EntityLine, RS2::EntityPolyline, RS2::EntityArc};
}

RS_ActionPolylineSegment::RS_ActionPolylineSegment(RS_EntityContainer& container,
        RS_GraphicView& graphicView)
        :RS_PreviewActionInterface("Create Polyline Existing from Segments",
						   container, graphicView) {
	actionType=RS2::ActionPolylineSegment;
}

RS_ActionPolylineSegment::RS_ActionPolylineSegment(RS_EntityContainer& container,
                                                   RS_GraphicView& graphicView,
                                                   RS_Entity* target)
    :RS_PreviewActionInterface("Create Polyline Existing from Segments",
                               container, graphicView) {
    actionType=RS2::ActionPolylineSegment;
    targetEntity = target;
    initWithTarget = true;
}

void RS_ActionPolylineSegment::init(int status){
    RS_PreviewActionInterface::init(status);
    if (initWithTarget){
        initWithTarget = false;
        convertPolyline(container, targetEntity, false);
        commandMessage(tr("Polyline created"));
        graphicView->redraw();
        updateSelectionWidget();
        finish(false);
        return;
    }
    else {
        targetEntity = nullptr;
//Experimental feature: trigger action, if already has selected entities
        if (container->countSelected(true, entityType)){
//find a selected entity
//TODO, find a better starting point
            for (RS_Entity *e: *container) {
                if (e->isSelected() &&
                    std::count(entityType.begin(), entityType.end(), e->rtti())){
                    targetEntity = e;
                    break;
                }
            }
            if (targetEntity){
                convertPolyline(container, targetEntity, true);
                commandMessage(tr("Polyline created"));
                graphicView->redraw();
                updateSelectionWidget();
                finish(false);
                return;
            }
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

    QList<RS_Entity *> entities;

    for (auto v: *toAdd) {
        if (reversed)
            entities.prepend(v);
        else
            entities.append(v);
    }
//bad polyline without vertex
    if (entities.isEmpty())
        return RS_Vector(false);

    double bulge = 0.0;
    RS_Entity *e = entities.takeFirst();

//First polyline vertex
    if (isArc(e)) {
        if (reversed)
            current->setNextBulge(((RS_Arc *) e)->getBulge() * -1);
        else
            current->setNextBulge(((RS_Arc *) e)->getBulge());
    }

    while (!entities.isEmpty()) {
        e = entities.takeFirst();
        if (isArc(e)) {
            if (reversed)
                bulge = ((RS_Arc *) e)->getBulge() * -1;
            else
                bulge = ((RS_Arc *) e)->getBulge();
        } else {
            bulge = 0.0;
        }
        if (reversed)
            current->addVertex(e->getEndpoint(), bulge, false);
        else
            current->addVertex(e->getStartpoint(), bulge, false);
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
RS_Polyline* RS_ActionPolylineSegment::convertPolyline(RS_EntityContainer* cnt, RS_Entity *selectedEntity, bool useSelected, bool createOnly){

    RS_DEBUG->print("RS_ActionPolylineSegment::convertPolyline");

    QList<RS_Entity *> remaining;
    QList<RS_Entity *> completed;
    RS_Vector start = selectedEntity->getStartpoint();
    RS_Vector end = selectedEntity->getEndpoint();
    if (!useSelected || (selectedEntity && selectedEntity->isSelected())){
        completed.append(selectedEntity);
    }

//get list with useful entities

    for (RS_Entity *e1: *container) {
        if (useSelected && !e1->isSelected()) continue;
        if (e1->isLocked() || !e1->isVisible() || e1 == selectedEntity) continue;
        if (isLine(e1) || isArc(e1)
            || e1->rtti() == RS2::EntityPolyline){
            if (selectedEntity->rtti() == RS2::EntityPolyline && ((RS_Polyline *) selectedEntity)->isClosed())
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
        for (int i = (remaining.size() - 1); i >= 0; --i) {
            RS_Entity *e = remaining.at(i);
            const RS_Vector &endpoint = e->getEndpoint();
            if (endpoint.distanceTo(start) < 1.0e-4){  // fixme = RS_TOLERANCE??
                completed.prepend(e);
                start = e->getStartpoint();
                remaining.removeAt(i);
                done = false;
            } else if (e->getStartpoint().distanceTo(start) < 1.0e-4){
                completed.prepend(e);
                start = endpoint;
                remaining.removeAt(i);
                done = false;
            } else if (endpoint.distanceTo(end) < 1.0e-4){
                completed.append(e);
                end = e->getStartpoint();
                remaining.removeAt(i);
                done = false;
            } else if (e->getStartpoint().distanceTo(end) < 1.0e-4){
                completed.append(e);
                end = endpoint;
                remaining.removeAt(i);
                done = false;
            }
        }
    } while (!done);

    //cleanup for no more needed list
    remaining.clear();

    bool closed = false;
    RS_Polyline *newPolyline = nullptr;
    if (document != nullptr){
        if (!createOnly){
            document->startUndoCycle();
        }

        bool revert = false;
        double bulge = 0.0;
        if (end.distanceTo(start) < 1.0e-4)
            closed = true;

        newPolyline = new RS_Polyline(cnt, RS_PolylineData(RS_Vector(false), RS_Vector(false), closed));
        if (!createOnly){
            newPolyline->setLayerToActive();
            newPolyline->setPenToActive();
        }

//complete polyline
        end = start;
        while (!completed.isEmpty()) {
            RS_Entity *e2 = completed.takeFirst();
            if (!createOnly){
                e2->setUndoState(true);
                document->addUndoable(e2);
            }
            if (e2->getStartpoint().distanceTo(end) < 1.0e-4){
                revert = false;
                start = e2->getStartpoint();
                end = e2->getEndpoint();
            } else {
                revert = true;
                start = e2->getEndpoint();
                end = e2->getStartpoint();
            }
            if (e2->rtti() == RS2::EntityArc){
                if (revert)
                    bulge = ((RS_Arc *) e2)->getBulge() * -1;
                else
                    bulge = ((RS_Arc *) e2)->getBulge();
            } else
                bulge = 0.0;
            if (e2->rtti() == RS2::EntityPolyline){
                newPolyline->addVertex(start, bulge);
                end = appendPol(newPolyline, (RS_Polyline *) e2, revert);
            } else
                newPolyline->addVertex(start, bulge);
        }

        if (closed)
            newPolyline->setClosed(true);
        else
            newPolyline->addVertex(end, bulge);

        newPolyline->endPolyline();
        cnt->addEntity(newPolyline);

        if (!createOnly){
            if (graphicView){
                graphicView->drawEntity(newPolyline);
            }

            document->addUndoable(newPolyline);
            document->endUndoCycle();
        }
    }
    RS_DEBUG->print("RS_ActionPolylineSegment::convertPolyline: OK");
    return newPolyline;
}

void RS_ActionPolylineSegment::trigger(){

    RS_DEBUG->print("RS_ActionPolylineSegment::trigger()");

    if (targetEntity /*&& selectedSegment && targetPoint.valid */){
//        targetEntity->setHighlighted(false);
//        graphicView->drawEntity(targetEntity);
//RLZ: do not use container->optimizeContours(); because it invalidate targetEntity
//        container->optimizeContours();
        convertPolyline(container, targetEntity);

        targetEntity = nullptr;
        setStatus(ChooseEntity);

        updateSelectionWidget();
    }
    graphicView->redraw();
}

void RS_ActionPolylineSegment::mouseMoveEvent(QMouseEvent *event){
    snapPoint(event);
    deletePreview();
    deleteHighlights();
    RS_Entity* en = catchEntity(event, entityType);
    if (en != nullptr){
        highlightHover(en);
        if (!(en->rtti() == RS2::EntityPolyline && ((RS_Polyline *) en)->isClosed())){
            convertPolyline(preview.get(), en, false, true);
        }
    }
    drawPreview();
    drawHighlights();
}

void RS_ActionPolylineSegment::onMouseLeftButtonRelease(int status, QMouseEvent *e) {
    switch (status) {
        case ChooseEntity:
            targetEntity = catchEntity(e, entityType);
            if (targetEntity == nullptr){
                commandMessage(tr("No Entity found."));
            } else if (targetEntity->rtti() == RS2::EntityPolyline && ((RS_Polyline *) targetEntity)->isClosed()){
                commandMessage(tr("Entity can not be a closed polyline."));
            } else {
                //TODO, verify topology of selected
//                    targetEntity->setHighlighted(true);
//                    graphicView->drawEntity(targetEntity);

//                setStatus(SetReferencePoint);
                graphicView->redraw();
                trigger();
            }
            break;
        default:
            break;
    }
}

void RS_ActionPolylineSegment::onMouseRightButtonRelease(int status, [[maybe_unused]]  QMouseEvent *e) {
    deleteSnapper();
    if (targetEntity){
         graphicView->redraw();
    }
    initPrevious(status);
}

void RS_ActionPolylineSegment::updateMouseButtonHints(){
    switch (getStatus()) {
        case ChooseEntity:
            updateMouseWidgetTRCancel(tr("Choose one of the segments on the original polyline"));
            break;
        default:
            updateMouseWidget();
            break;
    }
}
RS2::CursorType RS_ActionPolylineSegment::doGetMouseCursor([[maybe_unused]] int status){
    return RS2::SelectCursor;
}
