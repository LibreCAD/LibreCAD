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

#include "lc_actioncontext.h"
#include "lc_graphicviewport.h"
#include "rs_debug.h"
#include "rs_polyline.h"

RS_ActionPolylineAppend::RS_ActionPolylineAppend(LC_ActionContext *actionContext)
    :RS_ActionDrawPolyline(actionContext){
    m_actionType = RS2::ActionPolylineAppend;
}

void RS_ActionPolylineAppend::doInitialInit() {
    m_originalPolyline = nullptr;
}

void RS_ActionPolylineAppend::doInitWithContextEntity(RS_Entity* contextEntity, const RS_Vector& pos) {
    if (setPolylineToModify(contextEntity, pos)) {
        fireCoordinateEvent(pos);
    }
}

void RS_ActionPolylineAppend::doTrigger() {
    RS_DEBUG->print("RS_ActionPolylineAppend::trigger()");

    auto newPolyline = m_actionData->polyline;
    if (newPolyline == nullptr){
        return;
    }
    moveRelativeZero(newPolyline->getEndpoint()); // fixme - relative zero check!
    undoCycleReplace(m_originalPolyline, newPolyline);
    m_viewport->notifyChanged();

    RS_DEBUG->print("RS_ActionDrawPolyline::trigger(): polyline added: %lu",newPolyline->getId());
    m_originalPolyline = nullptr;
    m_actionData->polyline = nullptr;
}

void RS_ActionPolylineAppend::onMouseMoveEvent(int status, LC_MouseEvent *e) {
    switch (status) {
        case SetStartpoint: {
            deleteSnapper();

            auto polyline = dynamic_cast<RS_Polyline *>(catchAndDescribe(e));
            if (polyline != nullptr){
                highlightHover(polyline);

                if (m_showRefEntitiesOnPreview) {
                    auto entFirst = polyline->firstEntity();
                    auto entLast = polyline->lastEntity();

                    RS_Vector endpointToUse;

                    double dist = toGraphDX(m_catchEntityGuiRange) * 0.9;
                    const RS_Vector &mouse = e->graphPoint;
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
            highlightSelected(m_actionData->polyline);
            RS_ActionDrawPolyline::onMouseMoveEvent(status, e);
            break;
        }
        default:
            break;
    }
}

bool RS_ActionPolylineAppend::setPolylineToModify(RS_Entity* entity, const RS_Vector& mouse) {
    m_originalPolyline = dynamic_cast<RS_Polyline *>(entity);
    if (!m_originalPolyline) {
        commandMessage(tr("No Entity found."));
        return false;
    } else if (!isPolyline(m_originalPolyline)) {
        commandMessage(tr("Entity must be a polyline."));
        return false;
    } else if (m_originalPolyline->isClosed()) {
        commandMessage(tr("Can not append nodes in a closed polyline."));
        return false;
    } else {
        auto *op = m_originalPolyline;
        auto entFirst = op->firstEntity();
        auto entLast = op->lastEntity();

        double dist = toGraphDX(m_catchEntityGuiRange) * 0.9;

        if (entFirst == entLast) { // single segment of polyline
            double distToStart = m_originalPolyline->getStartpoint().distanceTo(mouse);
            double distToEnd = m_originalPolyline->getEndpoint().distanceTo(mouse);

            if (distToStart < distToEnd){
                m_actionData->point = m_originalPolyline->getStartpoint();
            }
            else{
                m_actionData->point = m_originalPolyline->getEndpoint();
            }

            auto *clone = dynamic_cast<RS_Polyline *>(m_originalPolyline->clone());
            m_actionData->polyline = clone;
            m_actionData->data = clone->getData();
            m_container->addEntity(clone);
        } else {
            auto nearestSegment = m_originalPolyline->getNearestEntity(mouse, &dist, RS2::ResolveNone);
            auto *clone = dynamic_cast<RS_Polyline *>(m_originalPolyline->clone());
            m_actionData->polyline = clone;
            m_actionData->data = clone->getData();
            m_container->addEntity(clone);
            m_prepend = false;
            if (nearestSegment == entFirst) {
                m_prepend = true;
                m_actionData->point = m_originalPolyline->getStartpoint();
            } else if (nearestSegment == entLast) {
                m_actionData->point = m_originalPolyline->getEndpoint();
            } else {
                commandMessage(tr("Click somewhere near the beginning or end of existing polyline."));
            }
        }
    }
    return true;
}

void RS_ActionPolylineAppend::onMouseLeftButtonRelease(int status, LC_MouseEvent *e) {
    if (status == SetStartpoint) {
        auto entity = catchEntityByEvent(e);
        const RS_Vector &mouse = e->graphPoint;
        if (setPolylineToModify(entity, mouse)) {
            fireCoordinateEventForSnap(e);
        }
    }
    else{
        RS_ActionDrawPolyline::onMouseLeftButtonRelease(status, e);
    }
}

void RS_ActionPolylineAppend::onMouseRightButtonRelease(int status, [[maybe_unused]]LC_MouseEvent *e) {
    if (status == SetNextPoint){
        trigger();
    }
    deleteSnapper();
    initPrevious(status);
}

void RS_ActionPolylineAppend::onCoordinateEvent(int status, [[maybe_unused]] bool isZero, const RS_Vector &pos) {
    RS_Vector mouse;
    if (m_calculatedSegment){
        mouse = m_actionData->calculatedEndpoint;
    }
    else{
        mouse = pos;
    }

    switch (status) {
        case SetStartpoint: {
            m_actionData->history.clear();
            m_actionData->history.append(m_actionData->point);
            m_actionData->bHistory.clear();
            m_actionData->bHistory.append(0.0);
            m_actionData->start = m_actionData->point;
            setStatus(SetNextPoint);
            moveRelativeZero(getPoint());
            updateMouseButtonHints();
            break;
        }
        case SetNextPoint: {
            if (!m_endPointSettingOn) {
                double bulge = 0.;
                if (m_mode == Ang) {
                    if (!m_prepend) {
                        if (m_alternateArc) {
                            int originalReversed = m_reversed;
                            m_reversed = m_reversed == -1 ? 1 : -1;
                            bulge = solveBulge(mouse);
                            m_reversed = originalReversed;
                        } else {
                            bulge = solveBulge(mouse);
                        }
                    }
                    else {
                        if (!m_alternateArc){
                            int originalReversed = m_reversed;
                            m_reversed = m_reversed == -1 ? 1 : -1;
                            bulge = solveBulge(mouse);
                            m_reversed = originalReversed;
                            RS_ArcData tmpArcData = m_actionData->arc_data;
                            tmpArcData.reversed = !tmpArcData.reversed;
                            RS_Arc arc = RS_Arc(nullptr, tmpArcData);
                            bulge = arc.getBulge();
                        }
                        else{
                            int originalReversed = m_reversed;
//                            m_reversed = m_reversed == -1 ? 1 : -1;
                            bulge = solveBulge(mouse);
                            m_reversed = originalReversed;
                            RS_ArcData tmpArcData = m_actionData->arc_data;
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
                    if (m_alternateArc) {
                        if (!m_prepend) {
                            RS_ArcData tmpArcData = m_actionData->arc_data;
                            tmpArcData.reversed = !tmpArcData.reversed;
                            RS_Arc arc = RS_Arc(nullptr, tmpArcData);
                            bulge = arc.getBulge();
                        }
                    }
                    else{
                        if (m_prepend) {
                            RS_ArcData tmpArcData = m_actionData->arc_data;
                            tmpArcData.reversed = !tmpArcData.reversed;
                            RS_Arc arc = RS_Arc(nullptr, tmpArcData);
                            bulge = arc.getBulge();
                        }
                    }
                }
                m_alternateArc = false;

                m_actionData->point = mouse;
                m_actionData->history.append(mouse);
                m_actionData->bHistory.append(bulge);
                m_actionData->polyline->setNextBulge(bulge);
                m_actionData->polyline->addVertex(mouse, 0.0, m_prepend);
                deletePreview(); // fixme - sand - clean this up
                deleteSnapper();
                updateMouseButtonHints();
            } else {
                m_endPointSettingOn = false;
                m_stepSizeSettingOn = true;
                m_endPointX = mouse.x;
                m_endPointY = mouse.y;
                updateMouseWidgetTRBack(tr("Enter number of polylines")); // fixme - check if this is correct
            }
            drawSnapper();
            moveRelativeZero(mouse);
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
