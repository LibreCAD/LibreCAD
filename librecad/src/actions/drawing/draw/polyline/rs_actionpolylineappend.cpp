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

#include <QMouseEvent>

#include "rs_actionpolylineappend.h"
#include "rs_commands.h"
#include "rs_coordinateevent.h"
#include "rs_debug.h"
#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_polyline.h"

RS_ActionPolylineAppend::RS_ActionPolylineAppend(
    RS_EntityContainer &container,
    RS_GraphicView &graphicView)
    :RS_ActionDrawPolyline(container, graphicView){
    actionType = RS2::ActionPolylineAppend;
}

void RS_ActionPolylineAppend::trigger(){

    RS_DEBUG->print("RS_ActionPolylineAppend::trigger()");

    RS_ActionDrawPolyline::trigger();

    if (!getPolyline()){
        return;
    }

// add the entity
//RS_Polyline* polyline = new RS_Polyline(container, data);
    getPolyline()->setLayerToActive();
    getPolyline()->setPenToActive();

// upd. undo list:
    if (document){
        document->startUndoCycle();
        // RVT_PORT need to decide on how to handle undo cycles
        originalPolyline->setUndoState(true);
        document->addUndoable(originalPolyline);
        document->addUndoable(getPolyline());
        document->endUndoCycle();
    }

// upd view
    deleteSnapper();
    moveRelativeZero(RS_Vector(0.0, 0.0));
    graphicView->deleteEntity(originalPolyline);
    graphicView->drawEntity(getPolyline());
    moveRelativeZero(getPolyline()->getEndpoint());
    drawSnapper();
    RS_DEBUG->print("RS_ActionDrawPolyline::trigger(): polyline added: %lu",
                    getPolyline()->getId());

    getPolyline() = nullptr;
}

void RS_ActionPolylineAppend::mouseMoveEvent(QMouseEvent *e){
    int status = getStatus();
//    deletePreview();
    deleteHighlights();
    switch (status) {
        case SetStartpoint: {
            snapPoint(e);
            auto polyline = dynamic_cast<RS_Polyline *>(catchEntity(e));
            if (polyline != nullptr){
                highlightHover(polyline);
            }
            break;
        }
        case SetNextPoint: {
            highlightSelected(originalPolyline);
            RS_ActionDrawPolyline::mouseMoveEvent(e);
            break;
        }
        default:
            break;
    }

    drawHighlights();
}

void RS_ActionPolylineAppend::onMouseLeftButtonRelease(int status, QMouseEvent *e) {
    if (status == SetStartpoint){
        originalPolyline = dynamic_cast<RS_Polyline *>(catchEntity(e));
        if (!originalPolyline){
            commandMessage(tr("No Entity found."));
            return;
        } else if (!isPolyline(originalPolyline)){
            commandMessage(tr("Entity must be a polyline."));
            return;
        } else if (originalPolyline->isClosed()){
            commandMessage(tr("Can not append nodes in a closed polyline."));
            return;
        } else {
            snapPoint(e);
            auto *op = static_cast<RS_Polyline *>(originalPolyline);
            RS_Entity *entFirst = op->firstEntity();
            RS_Entity *entLast = op->lastEntity();
            double dist = graphicView->toGraphDX(catchEntityGuiRange) * 0.9;
            RS_Entity *nearestSegment = originalPolyline->getNearestEntity(toGraph(e), &dist, RS2::ResolveNone);
            getPolyline() = dynamic_cast<RS_Polyline *>(originalPolyline->clone());
            container->addEntity(getPolyline());
            prepend = false;
            if (nearestSegment == entFirst){
                prepend = true;
                getPoint() = originalPolyline->getStartpoint();
            } else if (nearestSegment == entLast){
                getPoint() = originalPolyline->getEndpoint();
            } else {
                commandMessage(tr("Click somewhere near the beginning or end of existing polyline."));
            }
        }
    }
    fireCoordinateEventForSnap(e);
}

void RS_ActionPolylineAppend::onMouseRightButtonRelease(int status, [[maybe_unused]]QMouseEvent *e) {
    if (status == SetNextPoint){
        trigger();
    }
// deletePreview();
//clearPreview();
    deleteSnapper();
    initPrevious(status);
}

void RS_ActionPolylineAppend::onCoordinateEvent(int status, [[maybe_unused]] bool isZero, const RS_Vector &mouse) {
    switch (status) {
        case SetStartpoint: {
            getHistory().clear();
            getHistory().append(getPoint());
            getBHistory().clear();
            getBHistory().append(0.0);
            getStart() = getPoint();
            setStatus(SetNextPoint);
            moveRelativeZero(getPoint());
            updateMouseButtonHints();
            break;
        }
        case SetNextPoint: {
            moveRelativeZero(mouse);
            getPoint() = mouse;
            getHistory().append(mouse);
            getBHistory().append(0.0);
            if (!getPolyline()){
                getPolyline() = new RS_Polyline(container, getData());
                getPolyline()->addVertex(getStart(), 0.0, prepend);
            }
            if (getPolyline()){
                getPolyline()->addVertex(mouse, 0.0, prepend);
//			polyline->setEndpoint(mouse);
                if (getPolyline()->count() == 1){
                    getPolyline()->setLayerToActive();
                    getPolyline()->setPenToActive();
                }
                // RVT_PORT (can be deleted) deletePreview();
//clearPreview();
                deleteSnapper();
                graphicView->drawEntity(getPolyline());
                drawSnapper();
            }
//trigger();
//data.startpoint = data.endpoint;
            updateMouseButtonHints();
//graphicView->moveRelativeZero(mouse);
            break;
        }
        default:
            break;
    }
}

void RS_ActionPolylineAppend::updateMouseButtonHints(){
    switch (getStatus()) {
        case SetStartpoint: {
            updateMouseWidgetTRCancel(tr("Specify the polyline somewhere near the beginning or end point"));
            break;
        }
        case SetNextPoint: {
            QString msg = "";

            if (getHistory().size() >= 3){
                msg += command("close");
                msg += "/";
            }
            if (getHistory().size() >= 2){
                msg += command("undo");
            }

            if (getHistory().size() >= 2){
                updateMouseWidgetTRBack(tr("Specify next point or [%1]").arg(msg));
            } else {
                updateMouseWidgetTRBack(tr("Specify next point"));
            }
            break;
        }
        default:
            updateMouseWidget();
            break;
    }
}

void RS_ActionPolylineAppend::undo(){
    if (getHistory().size() > 1){
        getHistory().removeLast();
        getBHistory().removeLast();
        deletePreview();
        getPoint() = getHistory().last();
    } else {
        commandMessage(tr("Cannot undo: Not enough entities defined yet."));
    }
}
