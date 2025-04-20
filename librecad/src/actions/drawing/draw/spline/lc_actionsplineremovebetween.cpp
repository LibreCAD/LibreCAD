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

#include "lc_actionsplineremovebetween.h"

#include "lc_splinepoints.h"
#include "rs_entity.h"
#include "rs_spline.h"

namespace {
    const EntityTypeList g_enTypeList = {RS2::EntitySpline, RS2::EntitySplinePoints};
}

LC_ActionSplineRemoveBetween::LC_ActionSplineRemoveBetween(LC_ActionContext *actionContext)
    :LC_ActionSplineModifyBase("SplineRemovePointTwo", actionContext, RS2::ActionDrawSplinePointDelTwo) {
}

void LC_ActionSplineRemoveBetween::doCompleteTrigger() {
    m_directionFromStart = false;
}

void LC_ActionSplineRemoveBetween::doOnEntityNotCreated() {
    commandMessage(tr("Not enough points remains for the spline"));
}

void LC_ActionSplineRemoveBetween::onMouseMove(RS_Vector mouse, int status, LC_MouseEvent *e) {
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
            RS_Vector nearestPoint = m_entityToModify->getNearestRef(mouse, &dist);
            if (nearestPoint.valid) {
                previewRefSelectablePoint(nearestPoint);
            }
            break;
        }
        case SetControlPoint:{
            double dist;
            RS_Vector nearestPoint = m_entityToModify->getNearestRef(mouse, &dist);
            if (nearestPoint.valid) {
                previewRefPoint(m_selectedVertexPoint);
                previewRefSelectablePoint(nearestPoint);
                RS_Entity *previewUpdatedEntity = createModifiedSplineEntity(m_entityToModify, nearestPoint, e->isShift);
                if (previewUpdatedEntity != nullptr) {
                    previewEntity(previewUpdatedEntity);
                }
            }
            break;
        }
        default:
            break;
    }
}

void LC_ActionSplineRemoveBetween::onMouseLeftButtonRelease(int status, LC_MouseEvent *e) {
    switch (status){
        case SetEntity:{
            auto entity = catchEntityByEvent(e, g_enTypeList);
            if (entity != nullptr && mayModifySplineEntity(entity)){
                m_entityToModify = entity;
                m_entityToModify->setSelected(true);
                switch (m_entityToModify->rtti()){
                    case RS2::EntitySplinePoints:{
                        auto* sp = dynamic_cast<LC_SplinePoints *>(m_entityToModify);
                        m_splineIsClosed = sp->isClosed();
                        break;
                    }
                    case RS2::EntitySpline:{
                        auto* sp = dynamic_cast<RS_Spline*>(m_entityToModify);
                        m_splineIsClosed = sp->isClosed();
                        break;
                    }
                    default:
                        break;
                }

                redrawDrawing();
                setStatus(SetBeforeControlPoint);
            }
            break;
        }
        case SetBeforeControlPoint:{
            RS_Vector mouse = e->snapPoint;
            double dist;
            RS_Vector nearestPoint = m_entityToModify->getNearestRef(mouse, &dist);
            if (nearestPoint.valid){
                m_selectedVertexPoint = nearestPoint;
                setStatus(SetControlPoint);
            }
            break;
        }
        case SetControlPoint: {
            RS_Vector mouse = e->snapPoint;
            double dist;
            RS_Vector nearestPoint = m_entityToModify->getNearestRef(mouse, &dist);
            if (nearestPoint.valid){
                if (nearestPoint != m_vertexPoint) {
                    m_vertexPoint = nearestPoint;
                    m_directionFromStart = e->isShift;
                    trigger();
                }
            }
            break;
        }
        default:
            break;
    }
}

RS_Entity *LC_ActionSplineRemoveBetween::createModifiedSplineEntity(RS_Entity *e, RS_Vector controlPoint, bool startDirection) {
    RS_Entity* result = nullptr;
    bool deleteNotFoundPoints = startDirection && m_splineIsClosed;
    std::vector<RS_Vector> remainingPoints;
    switch (e->rtti()){
        case RS2::EntitySplinePoints:{
            auto* splinePoints = dynamic_cast<LC_SplinePoints *>(e->clone());
            LC_SplinePointsData &data = splinePoints->getData();
            unsigned int splinePointsCount = data.splinePoints.size();

            if (splinePointsCount > 0){
                collectPointsThatRemainsAfterDeletion(controlPoint, splinePointsCount, deleteNotFoundPoints, data.splinePoints, remainingPoints);
            }
            else {
                unsigned int controlPointsCount = data.controlPoints.size();
                collectPointsThatRemainsAfterDeletion(controlPoint, controlPointsCount, deleteNotFoundPoints, data.controlPoints, remainingPoints);
            }

            if (isValidSplinePointsData(remainingPoints.size(), m_splineIsClosed)) {
                data.splinePoints.clear();
                data.splinePoints = remainingPoints;
                splinePoints->update();
                result = splinePoints;
            }

            break;
        }
        case RS2::EntitySpline:{
            auto* spline = dynamic_cast<RS_Spline *>(e);
            RS_SplineData data = spline->getData();
            unsigned int count = data.controlPoints.size();

            collectPointsThatRemainsAfterDeletion(controlPoint, count, deleteNotFoundPoints, data.controlPoints, remainingPoints);

            if (isValidSplineData(remainingPoints.size(), m_splineIsClosed, spline->getDegree())){
                data.knotslist.clear();
                data.controlPoints.clear();
                data.controlPoints = remainingPoints;
                auto* modifiedSpline= new RS_Spline(nullptr, data);
                modifiedSpline->update();
                result = modifiedSpline;
            }
            break;
        }
        default:
            break;

    }
    return result;

}

void LC_ActionSplineRemoveBetween::collectPointsThatRemainsAfterDeletion(
    const RS_Vector &controlPoint, unsigned int splinePointsCount, bool deleteNotFoundPoints, std::vector<RS_Vector> &pointsVector,
    std::vector<RS_Vector> &remainingPoints) const {
    bool found = false;
    for (unsigned int i = 0; i < splinePointsCount; i++) {
        RS_Vector cp = pointsVector.at(i);
        if (cp == m_selectedVertexPoint || cp == controlPoint){
            if (deleteNotFoundPoints){
                if (found){
                    remainingPoints.push_back(cp);
                }
            }
            else if (!found){
                remainingPoints.push_back(cp);
            }
            found = !found;
        }
        if (deleteNotFoundPoints){
            if (found){
                remainingPoints.push_back(cp);
            }
        }
        else if (!found){
            remainingPoints.push_back(cp);
        }
    }
}

void LC_ActionSplineRemoveBetween::updateMouseButtonHints() {
    switch (getStatus()){
        case SetEntity:{
            updateMouseWidgetTRCancel(tr("Select spline or spline points entity"));
            break;
        }
        case SetBeforeControlPoint: {
            updateMouseWidgetTRCancel(tr("Select first point for deletion"));
            break;
        }
        case SetControlPoint: {
            updateMouseWidgetTRCancel(tr("Select second point for deletion"), m_splineIsClosed ? MOD_SHIFT_LC(tr("Delete from endpoints TO selected points")) : MOD_NONE);
            break;
        }
        default:
            break;
    }
}

bool LC_ActionSplineRemoveBetween::isValidSplinePointsData(unsigned long long int size, bool closed) {
    if (closed){
        return size > 3;
    }
    else {
        return size > 2;
    }
}

bool LC_ActionSplineRemoveBetween::isValidSplineData(unsigned long long int size, bool closed, int degree) {
    if (closed){
        return size > 3;
    }
    else{
        switch (degree){
            case 1:
                return size > 3;
            case 2:
                return size > 3;
            case 3:
                return size > 4;
            default:
                return false;
        }
    }
}
