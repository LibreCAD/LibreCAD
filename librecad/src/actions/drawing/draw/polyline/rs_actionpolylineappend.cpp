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

void RS_ActionPolylineAppend::doTrigger() {
    RS_DEBUG->print("RS_ActionPolylineAppend::trigger()");

    auto newPolyline = pPoints->polyline;
    if (newPolyline == nullptr){
        return;
    }
    moveRelativeZero(newPolyline->getEndpoint()); // fixme - relative zero check!
    undoCycleReplace(originalPolyline, newPolyline);

    RS_DEBUG->print("RS_ActionDrawPolyline::trigger(): polyline added: %lu",newPolyline->getId());
    originalPolyline = nullptr;
    pPoints->polyline = nullptr;
}

void RS_ActionPolylineAppend::mouseMoveEvent(QMouseEvent *e){
    deleteHighlights();
    deletePreview();
    int status = getStatus();
    switch (status) {
        case SetStartpoint: {
            snapPoint(e);
            deleteSnapper();

            auto polyline = dynamic_cast<RS_Polyline *>(catchEntityOnPreview(e));
            if (polyline != nullptr){
                highlightHover(polyline);

                if (showRefEntitiesOnPreview) {
                    auto entFirst = polyline->firstEntity();
                    auto entLast = polyline->lastEntity();

                    RS_Vector endpointToUse;

                    double dist = graphicView->toGraphDX(catchEntityGuiRange) * 0.9;
                    const RS_Vector &mouse = toGraph(e);
                    const RS_Vector &startPoint = polyline->getStartpoint();
                    const RS_Vector &endPoint = polyline->getEndpoint();
                    if (entFirst == entLast) { // single segment of polyline
                        double distToStart = startPoint.distanceTo(mouse);
                        double distToEnd = endPoint.distanceTo(mouse);

                        if (distToStart < distToEnd) {
                            endpointToUse = startPoint;
                        } else {
                            endpointToUse = endPoint;
                        }
                    } else {
                        auto nearestSegment = polyline->getNearestEntity(mouse, &dist, RS2::ResolveNone);
                        if (nearestSegment == entFirst) {
                            endpointToUse = startPoint;
                        } else if (nearestSegment == entLast) {
                            endpointToUse = endPoint;
                        }
                    }
                    previewRefSelectablePoint(endpointToUse);
                }
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
    drawPreview();
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
            const RS_Vector &mouse = toGraph(e);
            if (entFirst == entLast) { // single segment of polyline
                double distToStart = originalPolyline->getStartpoint().distanceTo(mouse);
                double distToEnd = originalPolyline->getEndpoint().distanceTo(mouse);

                if (distToStart < distToEnd){
                    pPoints->point = originalPolyline->getStartpoint();
                }
                else{
                    pPoints->point = originalPolyline->getEndpoint();
                }

                auto *clone = dynamic_cast<RS_Polyline *>(originalPolyline->clone());
                pPoints->polyline = clone;
                pPoints->data = clone->getData();
                container->addEntity(clone);
            } else {
                auto nearestSegment = originalPolyline->getNearestEntity(mouse, &dist, RS2::ResolveNone);
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

void RS_ActionPolylineAppend::onCoordinateEvent(int status, [[maybe_unused]] bool isZero, const RS_Vector &pos) {
    RS_Vector mouse;
    if (m_calculatedSegment){
        mouse = pPoints->calculatedEndpoint;
    }
    else{
        mouse = pos;
    }

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
            if (!endPointSettingOn) {
                double bulge = 0.;
                if (m_mode == Ang) {
                    if (!prepend) {
                        if (alternateArc) {
                            int originalReversed = m_reversed;
                            m_reversed = m_reversed == -1 ? 1 : -1;
                            bulge = solveBulge(mouse);
                            m_reversed = originalReversed;
                        } else {
                            bulge = solveBulge(mouse);
                        }
                    }
                    else {
                        if (!alternateArc){
                            int originalReversed = m_reversed;
                            m_reversed = m_reversed == -1 ? 1 : -1;
                            bulge = solveBulge(mouse);
                            m_reversed = originalReversed;
                            RS_ArcData tmpArcData = pPoints->arc_data;
                            tmpArcData.reversed = !tmpArcData.reversed;
                            RS_Arc arc = RS_Arc(nullptr, tmpArcData);
                            bulge = arc.getBulge();
                        }
                        else{
                            int originalReversed = m_reversed;
//                            m_reversed = m_reversed == -1 ? 1 : -1;
                            bulge = solveBulge(mouse);
                            m_reversed = originalReversed;
                            RS_ArcData tmpArcData = pPoints->arc_data;
                            tmpArcData.reversed = !tmpArcData.reversed;
                            RS_Arc arc = RS_Arc(nullptr, tmpArcData);
                            bulge = arc.getBulge();
                        }
                    }
                }
                else{
                    bulge = solveBulge(mouse);
                }

                if (m_mode != Ang && m_mode != Line){
                    if (alternateArc) {
                        if (!prepend) {
                            RS_ArcData tmpArcData = pPoints->arc_data;
                            tmpArcData.reversed = !tmpArcData.reversed;
                            RS_Arc arc = RS_Arc(nullptr, tmpArcData);
                            bulge = arc.getBulge();
                        }
                    }
                    else{
                        if (prepend) {
                            RS_ArcData tmpArcData = pPoints->arc_data;
                            tmpArcData.reversed = !tmpArcData.reversed;
                            RS_Arc arc = RS_Arc(nullptr, tmpArcData);
                            bulge = arc.getBulge();
                        }
                    }
                }
                alternateArc = false;

                pPoints->point = mouse;
                pPoints->history.append(mouse);
                pPoints->bHistory.append(bulge);
                pPoints->polyline->setNextBulge(bulge);
                pPoints->polyline->addVertex(mouse, 0.0, prepend);
                deletePreview(); // fixme - sand - clean this up
                deleteSnapper();
                updateMouseButtonHints();
            } else {
                endPointSettingOn = false;
                stepSizeSettingOn = true;
                endPointX = mouse.x;
                endPointY = mouse.y;
                updateMouseWidgetTRBack(tr("Enter number of polylines")); // fixme - check if this is correct
            }
            drawSnapper();
            moveRelativeZero(mouse);
            break;

/*


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
            break;*/
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
            updateMouseButtonHintsForNextPoint();
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

RS2::CursorType RS_ActionPolylineAppend::doGetMouseCursor(int status) {
    if (status == SetStartpoint){
        return RS2::SelectCursor;
    }
    else {
        return RS_ActionDrawPolyline::doGetMouseCursor(status);
    }
}
