/*******************************************************************************
 *
 This file is part of the LibreCAD project, a 2D CAD program

 Copyright (C) 2024 LibreCAD.org
 Copyright (C) 2024 sand1024

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 ******************************************************************************/
#include "lc_actionsplineappendpoint.h"

#include "lc_splinepoints.h"
#include "rs_entity.h"
#include "rs_spline.h"

namespace {
    // fixme - sand - think about support parabola as other splines
    const EntityTypeList g_enTypeList = {RS2::EntitySpline, RS2::EntitySplinePoints/*, RS2::EntityParabola*/};
}

LC_ActionSplineAppendPoint::LC_ActionSplineAppendPoint(LC_ActionContext *actionContext)
    :LC_ActionSplineModifyBase("SplineAppendPoint",actionContext, RS2::ActionDrawSplinePointAppend) {
}

void LC_ActionSplineAppendPoint::doCompleteTrigger() {
    moveRelativeZero(m_vertexPoint);
}

void LC_ActionSplineAppendPoint::onMouseMove(RS_Vector mouse, int status, LC_MouseEvent *e) {
    switch (status) {
        case SetEntity: {
            auto entity = catchEntityByEvent(e, g_enTypeList);
            if (entity != nullptr){
                if (mayModifySplineEntity(entity)) {
                    highlightHoverWithRefPoints(entity, true);
                }
            }
            break;
        }
        case SetBeforeControlPoint:{
            double dist;
            mouse = getRelZeroAwarePoint(e, mouse);
            RS_Vector nearestPoint = m_entityToModify->getNearestEndpoint(mouse, &dist);
            if (nearestPoint.valid) {
                bool appendToStart = nearestPoint == m_entityToModify->getStartpoint();
                previewRefSelectablePoint(mouse);
                RS_Entity *previewUpdatedEntity = createModifiedSplineEntity(m_entityToModify, mouse, appendToStart);
                if (previewUpdatedEntity != nullptr) {
                    previewEntity(previewUpdatedEntity);
                }
            }
            break;
        }
        case SetControlPoint:{
            previewRefSelectablePoint(mouse);
            bool appendMode = m_directionFromStart;
            if (e->isShift){
                double dist;
                RS_Vector nearestPoint = m_entityToModify->getNearestEndpoint(mouse, &dist);
                appendMode = nearestPoint == m_entityToModify->getStartpoint();
            }
            RS_Entity *previewUpdatedEntity = createModifiedSplineEntity(m_entityToModify, mouse, appendMode);
            if (previewUpdatedEntity != nullptr) {
                previewEntity(previewUpdatedEntity);
            }
            break;
        }
        default:
            break;
    }
}

void LC_ActionSplineAppendPoint::onMouseLeftButtonRelease(int status, LC_MouseEvent *e) {
    switch (status){
        case SetEntity:{
            auto entity = catchEntityByEvent(e, g_enTypeList);
            if (entity != nullptr && mayModifySplineEntity(entity)){
                m_entityToModify = entity;
                m_entityToModify->setSelected(true);
                redrawDrawing();
                setStatus(SetBeforeControlPoint);
            }
            break;
        }
        case SetBeforeControlPoint: {
            RS_Vector mouse = e->snapPoint;
            mouse = getRelZeroAwarePoint(e, mouse);
            double dist;
            RS_Vector nearestPoint = m_entityToModify->getNearestRef(mouse, &dist);
            if (nearestPoint.valid){
                m_directionFromStart = nearestPoint == m_entityToModify->getStartpoint();
                fireCoordinateEvent(mouse);
            }
            break;
        }
        case SetControlPoint:{
            RS_Vector mouse = e->snapPoint;
            if (e->isShift){
                double dist;
                RS_Vector nearestPoint = m_entityToModify->getNearestEndpoint(mouse, &dist);
                if (nearestPoint.valid) {
                    m_directionFromStart = nearestPoint == m_entityToModify->getStartpoint();
                }
            }
            fireCoordinateEvent(mouse);
        }
        default:
            break;
    }
}

void LC_ActionSplineAppendPoint::onCoordinateEvent(int status, [[maybe_unused]]bool isZero, const RS_Vector &pos) {
    switch (status){
        case SetBeforeControlPoint:{
            m_vertexPoint = pos;
            trigger();
            setStatus(SetControlPoint);
            break;
        }
        case SetControlPoint:{
            m_vertexPoint = pos;
            trigger();
            break;
        }
        default:
            break;
    }
}

// todo - sand - should we allow append to closed splines?
bool LC_ActionSplineAppendPoint::mayModifySplineEntity(RS_Entity *e) {
    switch (e->rtti()) {
        case RS2::EntitySplinePoints: {
//            auto *splinePoints = dynamic_cast<LC_SplinePoints *>(e);
//            return !splinePoints->isClosed();
            return true;
        }
        case RS2::EntitySpline: {
//            auto *spline = dynamic_cast<RS_Spline *>(e);
//            return !spline->isClosed();
            return true;
        }
        case RS2::EntityParabola:
            return true;
        default:
            return false;
    }
}

RS_Entity *LC_ActionSplineAppendPoint::createModifiedSplineEntity(RS_Entity *e, RS_Vector controlPoint, bool fromStart) {
    RS_Entity* result = nullptr;
    switch (e->rtti()){
        case RS2::EntitySplinePoints:{
            auto* splinePoints = dynamic_cast<LC_SplinePoints *>(e->clone());
            LC_SplinePointsData &data = splinePoints->getData();
            bool hasSplinePoints = data.splinePoints.size() > 0; // handle spline point data magic :(
            if (fromStart){
                if (hasSplinePoints) {
                    data.splinePoints.insert(data.splinePoints.begin(), controlPoint);
                }
                else {
                    data.controlPoints.insert(data.controlPoints.begin(), controlPoint);
                }
            }
            else{
                if (hasSplinePoints){
                    data.splinePoints.push_back(controlPoint);
                }
                else {
                    data.controlPoints.push_back(controlPoint);
                }
            }

            splinePoints->update();
            result = splinePoints;
            break;
        }
        case RS2::EntitySpline:{
            auto* spline = dynamic_cast<RS_Spline *>(e);
            RS_SplineData data = spline->getData();
            if (fromStart){
                data.controlPoints.insert(data.controlPoints.begin(), controlPoint);
            }
            else{
                data.controlPoints.push_back(controlPoint);
            }
            data.knotslist.clear();
            auto* modifiedSpline= new RS_Spline(nullptr, data);
            modifiedSpline->update();
            result = modifiedSpline;
            break;
        }
        case RS2::EntityParabola:{
            // fixme - sand - complete - there should be ordinary spline instead of parabola?
            break;
        }
        default:
            break;

    }
    return result;
}

void LC_ActionSplineAppendPoint::updateMouseButtonHints() {
    switch (getStatus()){
        case SetEntity:{
            updateMouseWidgetTRCancel(tr("Select spline or spline points entity"));
            break;
        }
        case SetBeforeControlPoint: {
            updateMouseWidgetTRCancel(tr("Specify first control point"),MOD_SHIFT_RELATIVE_ZERO);
            break;
        }
        case SetControlPoint: {
            updateMouseWidgetTRCancel(tr("Specify control point"), MOD_SHIFT_LC("Append to alternative Endpoint"));
            break;
        }
        default:
            break;
    }
}
