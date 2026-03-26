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

#include "lc_action_polyline_append.h"

#include "lc_action_draw_polyline.h"
#include "lc_actioncontext.h"
#include "lc_graphicviewport.h"
#include "rs_document.h"
#include "rs_pen.h"
#include "rs_polyline.h"

LC_ActionPolylineAppend::LC_ActionPolylineAppend(LC_ActionContext *actionContext)
    :LC_ActionDrawPolyline("ActionPolylineAppend", actionContext, RS2::ActionPolylineAppend){
}

void LC_ActionPolylineAppend::doInitialInit() {
    m_originalPolyline = nullptr;
}

void LC_ActionPolylineAppend::doInitWithContextEntity(RS_Entity* contextEntity, const RS_Vector& pos) {
    if (setPolylineToModify(contextEntity, pos)) {
        fireCoordinateEvent(pos);
    }
}

bool LC_ActionPolylineAppend::doTriggerModifications(LC_DocumentModificationBatch& ctx) {
    const auto newPolyline = m_actionData->polyline;
    if (newPolyline != nullptr) {
        newPolyline->setLayer(m_originalPolyline->getLayer(false));
        newPolyline->setPen(m_originalPolyline->getPen(false));

        ctx.dontSetActiveLayerAndPen();
        ctx.replace(m_originalPolyline, newPolyline);

        moveRelativeZero(newPolyline->getEndpoint());

        return true;
    }
    return false;
}

void LC_ActionPolylineAppend::doTriggerCompletion([[maybe_unused]]bool success) {
    m_originalPolyline = nullptr;
    m_actionData->polyline = nullptr;
}

void LC_ActionPolylineAppend::onMouseMoveEvent(const int status, const LC_MouseEvent* e) {
    switch (status) {
        case SetStartpoint: {
            deleteSnapper();

            const auto polyline = dynamic_cast<RS_Polyline *>(catchAndDescribe(e));
            if (polyline != nullptr){
                highlightHover(polyline);

                if (m_showRefEntitiesOnPreview) {
                    const auto entFirst = polyline->firstEntity();
                    const auto entLast = polyline->lastEntity();

                    RS_Vector endpointToUse;

                    double dist = toGraphDX(m_catchEntityGuiRange) * 0.9;
                    const RS_Vector &mouse = e->graphPoint;
                    const RS_Vector &startPoint = polyline->getStartpoint();
                    const RS_Vector &endPoint = polyline->getEndpoint();
                    if (entFirst == entLast) { // single segment of polyline
                        const double distToStart = startPoint.distanceTo(mouse);
                        const double distToEnd = endPoint.distanceTo(mouse);

                        if (distToStart < distToEnd) {
                            endpointToUse = startPoint;
                        } else {
                            endpointToUse = endPoint;
                        }
                    } else {
                        const auto nearestSegment = polyline->getNearestEntity(mouse, &dist, RS2::ResolveNone);
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
            LC_ActionDrawPolyline::onMouseMoveEvent(status, e);
            break;
        }
        default:
            break;
    }
}

bool LC_ActionPolylineAppend::setPolylineToModify(RS_Entity* entity, const RS_Vector& mouse) {
    m_originalPolyline = dynamic_cast<RS_Polyline *>(entity);
    if (m_originalPolyline == nullptr) {
        commandMessage(tr("No Entity found."));
        return false;
    }
    if (!isPolyline(m_originalPolyline)) {
        commandMessage(tr("Entity must be a polyline."));
        return false;
    }
    if (m_originalPolyline->isClosed()) {
        commandMessage(tr("Can not append nodes in a closed polyline."));
        return false;
    }
    const auto *op = m_originalPolyline;
    const auto entFirst = op->firstEntity();
    const auto entLast = op->lastEntity();

    double dist = toGraphDX(m_catchEntityGuiRange) * 0.9;

    if (entFirst == entLast) { // single segment of polyline
        const double distToStart = m_originalPolyline->getStartpoint().distanceTo(mouse);
        const double distToEnd = m_originalPolyline->getEndpoint().distanceTo(mouse);

        if (distToStart < distToEnd){
            m_actionData->point = m_originalPolyline->getStartpoint();
        }
        else{
            m_actionData->point = m_originalPolyline->getEndpoint();
        }

        auto *clone = static_cast<RS_Polyline *>(m_originalPolyline->clone());
        m_actionData->polyline = clone;
        m_actionData->data = clone->getData();
        undoCycleReplace(m_originalPolyline, clone);
        select(clone);
    } else {
        const auto nearestSegment = m_originalPolyline->getNearestEntity(mouse, &dist, RS2::ResolveNone);
        auto *clone = static_cast<RS_Polyline *>(m_originalPolyline->clone());
        m_actionData->polyline = clone;
        m_actionData->data = clone->getData();
        undoCycleReplace(m_originalPolyline, clone);
        select(clone);
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
    return true;
}

void LC_ActionPolylineAppend::onMouseLeftButtonRelease(const int status, const LC_MouseEvent* e) {
    if (status == SetStartpoint) {
        const auto entity = catchEntityByEvent(e);
        const RS_Vector &mouse = e->graphPoint;
        if (setPolylineToModify(entity, mouse)) {
            fireCoordinateEventForSnap(e);
        }
    }
    else{
        LC_ActionDrawPolyline::onMouseLeftButtonRelease(status, e);
    }
}

void LC_ActionPolylineAppend::onMouseRightButtonRelease(const int status, [[maybe_unused]] const LC_MouseEvent* e) {
    if (status == SetNextPoint){
        m_actionData->polyline = nullptr;
        trigger();
    }
    deleteSnapper();
    initPrevious(status);
}

void LC_ActionPolylineAppend::onCoordinateEvent(int status, [[maybe_unused]] bool isZero, const RS_Vector &pos) {
    RS_Vector mouse;
    if (m_calculatedSegment){
        mouse = m_actionData->calculatedEndpoint;
    }
    else{
        mouse = pos;
    }

    switch (status) {
        case SetStartpoint: {
            RS_Vector point = m_actionData->point;
            m_actionData->history.clear();
            m_actionData->history.append(point);
            m_actionData->bHistory.clear();
            m_actionData->bHistory.append(0.0);
            m_actionData->start = point;
            setStatus(SetNextPoint);
            addSnappedPointToVisualSnap(point);
            moveRelativeZero(point);
            updateActionPrompt();
            break;
        }
        case SetNextPoint: {
            if (!m_endPointSettingOn) {
                double bulge = 0.;
                if (m_mode == ArcFixedAngle) {
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
                            m_reversed = m_reversed == -1 ? 1 : -1; // fixme - sand - review this logic!!!
                            m_reversed = originalReversed;
                            RS_ArcData tmpArcData = m_actionData->arcData;
                            tmpArcData.reversed = !tmpArcData.reversed;
                            auto arc = RS_Arc(nullptr, tmpArcData);
                            bulge = arc.getBulge();
                        }
                        else{
                            int originalReversed = m_reversed;
//                            m_reversed = m_reversed == -1 ? 1 : -1;
                            m_reversed = originalReversed;
                            RS_ArcData tmpArcData = m_actionData->arcData;
                            tmpArcData.reversed = !tmpArcData.reversed;
                            auto arc = RS_Arc(nullptr, tmpArcData);
                            bulge = arc.getBulge();
                        }
                    }
                }
                else{
                    bulge = solveBulge(mouse);
                }

                if (m_mode != ArcFixedAngle && m_mode != Line){
                    if (m_alternateArc) {
                        if (!m_prepend) {
                            RS_ArcData tmpArcData = m_actionData->arcData;
                            tmpArcData.reversed = !tmpArcData.reversed;
                            auto arc = RS_Arc(nullptr, tmpArcData);
                            bulge = arc.getBulge();
                        }
                    }
                    else{
                        if (m_prepend) {
                            RS_ArcData tmpArcData = m_actionData->arcData;
                            tmpArcData.reversed = !tmpArcData.reversed;
                            auto arc = RS_Arc(nullptr, tmpArcData);
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
                updateActionPrompt();
            } else {
                m_endPointSettingOn = false;
                m_stepSizeSettingOn = true;
                m_endPointX = mouse.x;
                m_endPointY = mouse.y;
                updatePromptTRBack(tr("Enter number of polylines")); // fixme - check if this is correct
            }
            drawSnapper();
            addSnappedPointToVisualSnap(mouse);
            moveRelativeZero(mouse);
            break;
        }
        default:
            break;
    }
}

void LC_ActionPolylineAppend::updateActionPrompt(){
    switch (getStatus()) {
        case SetStartpoint: {
            updatePromptTRCancel(tr("Specify the polyline somewhere near the beginning or end point"));
            break;
        }
        case SetNextPoint: {
            updateMouseButtonHintsForNextPoint();
            break;
        }
        default:
            updatePrompt();
            break;
    }
}

void LC_ActionPolylineAppend::undo(){
    if (getHistory().size() > 1){
        getHistory().removeLast();
        getBHistory().removeLast();
        deletePreview();
        getPoint() = getHistory().last();
    } else {
        commandMessage(tr("Cannot undo: Not enough entities defined yet."));
    }
}

RS2::CursorType LC_ActionPolylineAppend::doGetMouseCursor(const int status) {
    if (status == SetStartpoint){
        return RS2::SelectCursor;
    }
    return LC_ActionDrawPolyline::doGetMouseCursor(status);
}
