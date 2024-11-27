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

#include "lc_actionremovesplinepoints.h"
#include "rs_graphicview.h"
#include "lc_splinepoints.h"
#include "rs_document.h"
#include "rs_spline.h"
#include "lc_actionsplinemodifybase.h"

namespace {
    const EntityTypeList enTypeList = {RS2::EntitySpline, RS2::EntitySplinePoints};
}

LC_ActionRemoveSplinePoints::LC_ActionRemoveSplinePoints(RS_EntityContainer &container, RS_GraphicView &graphicView):LC_ActionSplineModifyBase("SplineRemovePoint", container, graphicView) {
    actionType = RS2::ActionDrawSplinePointRemove;
}

void LC_ActionRemoveSplinePoints::doAfterTrigger() {
    if (!mayModifySplineEntity(entityToModify)) {
        entityToModify->setSelected(false);
        setStatus(SetEntity);
    }
}

void LC_ActionRemoveSplinePoints::onMouseMove(RS_Vector mouse, int status, QMouseEvent *e) {
    switch (status) {
        case SetEntity: {
            auto entity = catchEntity(e, enTypeList);
            if (entity != nullptr){
               if (mayModifySplineEntity(entity)) {
                   highlightHoverWithRefPoints(entity, true);
               }
            }
            break;
        }
        case SetControlPoint:{
            double dist;
            RS_Vector nearestPoint = entityToModify->getNearestRef(mouse, &dist);
            if (nearestPoint.valid) {
                previewRefSelectablePoint(nearestPoint);
                RS_Entity *previewUpdatedEntity = createModifiedSplineEntity(entityToModify, nearestPoint, directionFromStart);
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

void LC_ActionRemoveSplinePoints::onMouseLeftButtonRelease(int status, QMouseEvent *e) {
    switch (status){
        case SetEntity:{
            auto entity = catchEntity(e, enTypeList);
            if (entity != nullptr && mayModifySplineEntity(entity)){
                entityToModify = entity;
                entityToModify->setSelected(true);
                graphicView->redraw(RS2::RedrawDrawing);
                setStatus(SetControlPoint);
            }
            break;
        }
        case SetControlPoint: {
            RS_Vector mouse = snapPoint(e);
            double dist;
            RS_Vector nearestPoint = entityToModify->getNearestRef(mouse, &dist);
            if (nearestPoint.valid){
                vertexPoint = nearestPoint;
                trigger();
            }
            break;
        }
        default:
            break;
    }
}


RS_Entity *LC_ActionRemoveSplinePoints::createModifiedSplineEntity(RS_Entity *e, RS_Vector controlPoint, [[maybe_unused]] bool direction) {
    RS_Entity* result = nullptr;
    switch (e->rtti()){
        case RS2::EntitySplinePoints:{
            auto* splinePoints = dynamic_cast<LC_SplinePoints *>(e->clone());
            LC_SplinePointsData &data = splinePoints->getData();
            unsigned int controlPointsCount = data.controlPoints.size();
            unsigned int splinePointsCount = data.splinePoints.size();
            if (splinePointsCount > 0){
                for (unsigned int i = 0; i < splinePointsCount; i++) {
                    RS_Vector cp = data.splinePoints.at(i);
                    if (cp == controlPoint) {
                        data.splinePoints.erase(data.splinePoints.begin() + i);
                        splinePoints->update();
                        result = splinePoints;
                        break;
                    }
                }
            }
            else {
                for (unsigned int i = 0; i < controlPointsCount; i++) {
                    RS_Vector cp = data.controlPoints.at(i);
                    if (cp == controlPoint) {
                        data.controlPoints.erase(data.controlPoints.begin() + i);
                        splinePoints->update();
                        result = splinePoints;
                        break;
                    }
                }
            }
            break;
        }
        case RS2::EntitySpline:{
            auto* spline = dynamic_cast<RS_Spline *>(e);
            RS_SplineData data = spline->getData();
            unsigned int count = data.controlPoints.size();
            for (unsigned int i = 0; i < count; i++ ){
                RS_Vector cp = data.controlPoints.at(i);
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
            auto* splinePoints = dynamic_cast<LC_SplinePoints *>(e);
            unsigned int size = splinePoints->getData().controlPoints.size();
            if (splinePoints->isClosed()){
                return size > 3;
            }
            else {
                return size > 2;
            }
        }
        case RS2::EntitySpline:{
            auto* spline = dynamic_cast<RS_Spline *>(e);
            unsigned int size = spline->getData().controlPoints.size();
            if (spline->isClosed()){
                return size > 3;
            }
            else{
                int degree = spline->getDegree();
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
        default:
            return false;
    }
}

void LC_ActionRemoveSplinePoints::updateMouseButtonHints() {
    switch (getStatus()){
        case SetEntity:{
            updateMouseWidgetTRCancel(tr("Select spline or spline points entity"));
            break;
        }
        case SetControlPoint: {
            updateMouseWidgetTRCancel(tr("Select control point of entity"));
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
