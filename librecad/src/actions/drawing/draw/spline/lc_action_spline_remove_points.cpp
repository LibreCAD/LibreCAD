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

#include "lc_action_spline_remove_points.h"

#include "lc_splinepoints.h"
#include "rs_entity.h"
#include "rs_spline.h"

namespace {
    const EntityTypeList g_enTypeList = {RS2::EntitySpline, RS2::EntitySplinePoints};
}

LC_ActionRemoveSplinePoints::LC_ActionRemoveSplinePoints(LC_ActionContext *actionContext)
    :LC_ActionSplineModifyBase("ActionDrawSplinePointRemove", actionContext,RS2::ActionDrawSplinePointRemove) {
}

void LC_ActionRemoveSplinePoints::doTriggerCompletion([[maybe_unused]]bool success) {
    if (!mayModifySplineEntity(m_entityToModify)) {
        unselect(m_entityToModify);
        setStatus(SetEntity);
    }
}

void LC_ActionRemoveSplinePoints::onMouseMove(const RS_Vector mouse, const int status, const LC_MouseEvent* e) {
    switch (status) {
        case SetEntity: {
            const auto entity = catchEntityByEvent(e, g_enTypeList);
            if (entity != nullptr){
               if (mayModifySplineEntity(entity)) {
                   highlightHoverWithRefPoints(entity, true);
               }
            }
            break;
        }
        case SetControlPoint:{
            double dist;
            const RS_Vector nearestPoint = m_entityToModify->getNearestRef(mouse, &dist);
            if (nearestPoint.valid) {
                previewRefSelectablePoint(nearestPoint);
                const RS_Entity *previewUpdatedEntity = createModifiedSplineEntity(m_entityToModify, nearestPoint, m_directionFromStart);
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

void LC_ActionRemoveSplinePoints::setEntityToModify(RS_Entity* entity) {
    m_entityToModify = entity;
    select(m_entityToModify);
    redrawDrawing();
    setStatus(SetControlPoint);
}

void LC_ActionRemoveSplinePoints::onMouseLeftButtonRelease(const int status, const LC_MouseEvent* e) {
    switch (status){
        case SetEntity:{
            const auto entity = catchEntityByEvent(e, g_enTypeList);
            if (entity != nullptr && mayModifySplineEntity(entity)){
                setEntityToModify(entity);
            }
            break;
        }
        case SetControlPoint: {
            const RS_Vector mouse = e->snapPoint;
            double dist;
            const RS_Vector nearestPoint = m_entityToModify->getNearestRef(mouse, &dist);
            if (nearestPoint.valid){
                m_vertexPoint = nearestPoint;
                trigger();
            }
            break;
        }
        default:
            break;
    }
}


RS_Entity *LC_ActionRemoveSplinePoints::createModifiedSplineEntity(RS_Entity *e, const RS_Vector controlPoint, [[maybe_unused]] bool direction) {
    RS_Entity* result = nullptr;
    switch (e->rtti()){
        case RS2::EntitySplinePoints:{
            auto* clone = static_cast<LC_SplinePoints *>(e->clone());
            LC_SplinePointsData &data = clone->getData();
            const size_t controlPointsCount = data.controlPoints.size();
            const size_t splinePointsCount = data.splinePoints.size();
            if (splinePointsCount > 0){
                for (size_t i = 0; i < splinePointsCount; i++) {
                    RS_Vector cp = data.splinePoints.at(i);
                    if (cp == controlPoint) {
                        data.splinePoints.erase(data.splinePoints.begin() + i);
                        clone->update();
                        result = clone;
                        break;
                    }
                }
            }
            else {
                for (size_t i = 0; i < controlPointsCount; i++) {
                    RS_Vector cp = data.controlPoints.at(i);
                    if (cp == controlPoint) {
                        data.controlPoints.erase(data.controlPoints.begin() + i);
                        clone->update();
                        result = clone;
                        break;
                    }
                }
            }
            break;
        }
        case RS2::EntitySpline:{
            const auto* spline = static_cast<RS_Spline *>(e);
            RS_SplineData data = spline->getData();
            const size_t count = data.controlPoints.size();
            for (size_t i = 0; i < count; i++ ){
                const RS_Vector cp = data.controlPoints.at(i);
                if (cp == controlPoint){
                    data.controlPoints.erase(data.controlPoints.begin() + i);
                    data.knotslist.clear();

                    auto* modifiedSpline= new RS_Spline(nullptr, data);
                    modifiedSpline->update();
                    result = modifiedSpline;
                    break;
                }
            }
            break;
        }
        default:
            break;

    }
    return result;
}

bool LC_ActionRemoveSplinePoints::mayModifySplineEntity(RS_Entity *e) {
    switch (e->rtti()){
        case RS2::EntitySplinePoints:{
            auto* splinePoints = static_cast<LC_SplinePoints *>(e);
            const size_t size = splinePoints->getData().controlPoints.size();
            if (splinePoints->isClosed()){
                return size > 3;
            }
            return size > 2;
        }
        case RS2::EntitySpline:{
            const auto* spline = static_cast<RS_Spline *>(e);
            const size_t size = spline->getData().controlPoints.size();
            if (spline->isClosed()){
                return size > 3;
            }
            const int degree = spline->getDegree();
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
        default:
            return false;
    }
}

void LC_ActionRemoveSplinePoints::updateActionPrompt() {
    switch (getStatus()){
        case SetEntity:{
            updatePromptTRCancel(tr("Select spline or spline points entity"));
            break;
        }
        case SetControlPoint: {
            updatePromptTRCancel(tr("Select control point of entity"));
            break;
        }
        default:
            break;
    }
}

void LC_ActionRemoveSplinePoints::setStatus(int status) {
    if (status == SetBeforeControlPoint){ // support for back operation
        status = SetEntity;
    }
    RS_ActionInterface::setStatus(status);
}
