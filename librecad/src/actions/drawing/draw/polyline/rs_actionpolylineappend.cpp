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
// FIXME - SAND - DELETEING ORIGINAL POLYLINE - #1877
void RS_ActionPolylineAppend::trigger(){

    RS_DEBUG->print("RS_ActionPolylineAppend::trigger()");

    RS_PreviewActionInterface::trigger();

    auto newPolyline = pPoints->polyline;
    if (newPolyline == nullptr){
        return;
    }

    newPolyline->setLayerToActive();
    newPolyline->setPenToActive();

    graphicView->deleteEntity(originalPolyline);
    originalPolyline->changeUndoState();

// upd. undo list:
    if (document){
        document->startUndoCycle();
        document->addUndoable(newPolyline);
        document->removeUndoable(originalPolyline);
        document->endUndoCycle();
    }

// upd view
    deleteSnapper();
    moveRelativeZero(RS_Vector(0.0, 0.0));
    graphicView->drawEntity(newPolyline);
    moveRelativeZero(newPolyline->getEndpoint());
    drawSnapper();
    RS_DEBUG->print("RS_ActionDrawPolyline::trigger(): polyline added: %lu",
                    newPolyline->getId());
    originalPolyline = nullptr;
    pPoints->polyline = nullptr;
}

void RS_ActionPolylineAppend::mouseMoveEvent(QMouseEvent *e){
    int status = getStatus();
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
            highlightSelected(pPoints->polyline);
            RS_ActionDrawPolyline::mouseMoveEvent(e);
            break;
        }
        default:
            break;
    }

    drawHighlights();
}

void RS_ActionPolylineAppend::onMouseLeftButtonRelease(int status, QMouseEvent *e) {
    if (status == SetStartpoint) {
        originalPolyline = dynamic_cast<RS_Polyline *>(catchEntity(e));
        if (!originalPolyline) {
            commandMessage(tr("No Entity found."));
            return;
        } else if (!isPolyline(originalPolyline)) {
            commandMessage(tr("Entity must be a polyline."));
            return;
        } else if (originalPolyline->isClosed()) {
            commandMessage(tr("Can not append nodes in a closed polyline."));
            return;
        } else {
            snapPoint(e);
            auto *op = static_cast<RS_Polyline *>(originalPolyline);
            auto entFirst = op->firstEntity();
            auto entLast = op->lastEntity();
            double dist = graphicView->toGraphDX(catchEntityGuiRange) * 0.9;
            auto nearestSegment = originalPolyline->getNearestEntity(toGraph(e), &dist, RS2::ResolveNone);
            auto *clone = dynamic_cast<RS_Polyline *>(originalPolyline->clone());
            pPoints->polyline = clone;
            pPoints->data = clone->getData();
            container->addEntity(clone);
            prepend = false;
            if (nearestSegment == entFirst) {
                prepend = true;
                pPoints->point = originalPolyline->getStartpoint();
            } else if (nearestSegment == entLast) {
                pPoints->point = originalPolyline->getEndpoint();
            } else {
                commandMessage(tr("Click somewhere near the beginning or end of existing polyline."));
            }
        }
        fireCoordinateEventForSnap(e);
    }
    else{
        RS_ActionDrawPolyline::onMouseLeftButtonRelease(status, e);
    }
}

void RS_ActionPolylineAppend::onMouseRightButtonRelease(int status, [[maybe_unused]]QMouseEvent *e) {
    if (status == SetNextPoint){
        trigger();
    }
    deleteSnapper();
    initPrevious(status);
}

void RS_ActionPolylineAppend::onCoordinateEvent(int status, [[maybe_unused]] bool isZero, const RS_Vector &mouse) {
    switch (status) {
        case SetStartpoint: {
            pPoints->history.clear();
            pPoints->history.append(pPoints->point);
            pPoints->bHistory.clear();
            pPoints->bHistory.append(0.0);
            pPoints->start = pPoints->point;
            setStatus(SetNextPoint);
            moveRelativeZero(getPoint());
            updateMouseButtonHints();
            break;
        }
        case SetNextPoint: {
            moveRelativeZero(mouse);
            pPoints->point = mouse;
            pPoints->history.append(mouse);
            pPoints->bHistory.append(0.0);
            RS_Polyline *polyline = pPoints->polyline;
            if (polyline == nullptr){
                polyline = new RS_Polyline(container, getData());
                polyline->addVertex(getStart(), 0.0, prepend);
            }
            else{
                polyline->addVertex(mouse, 0.0, prepend);
                if (polyline->count() == 1){
                    polyline->setLayerToActive();
                    polyline->setPenToActive();
                }
                deleteSnapper();
                graphicView->drawEntity(polyline);
                drawSnapper();
            }
            updateMouseButtonHints();
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
