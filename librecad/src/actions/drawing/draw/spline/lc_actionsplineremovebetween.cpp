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
#include <QInputEvent>
#include <QMouseEvent>

#include "lc_actionsplineremovebetween.h"
#include "rs_graphicview.h"
#include "rs_spline.h"
#include "lc_splinepoints.h"


namespace {
    const EntityTypeList enTypeList = {RS2::EntitySpline, RS2::EntitySplinePoints};
}

LC_ActionSplineRemoveBetween::LC_ActionSplineRemoveBetween(RS_EntityContainer &container, RS_GraphicView &graphicView):LC_ActionSplineModifyBase("SplineRemovePointTwo", container, graphicView) {
    actionType = RS2::ActionDrawSplinePointDelTwo;
}

void LC_ActionSplineRemoveBetween::doCompleteTrigger() {
    directionFromStart = false;
}

void LC_ActionSplineRemoveBetween::doOnEntityNotCreated() {
    commandMessage(tr("Not enough points remains for the spline"));
}

void LC_ActionSplineRemoveBetween::onMouseMove(RS_Vector mouse, int status, QMouseEvent *e) {
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
        case SetBeforeControlPoint:{
            double dist;
            RS_Vector nearestPoint = entityToModify->getNearestRef(mouse, &dist);
            if (nearestPoint.valid) {
                previewRefSelectablePoint(nearestPoint);
            }
            break;
        }
        case SetControlPoint:{
            double dist;
            RS_Vector nearestPoint = entityToModify->getNearestRef(mouse, &dist);
            if (nearestPoint.valid) {
                previewRefPoint(selectedVertexPoint);
                previewRefSelectablePoint(nearestPoint);
                RS_Entity *previewUpdatedEntity = createModifiedSplineEntity(entityToModify, nearestPoint, isShift(e));
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

void LC_ActionSplineRemoveBetween::onMouseLeftButtonRelease(int status, QMouseEvent *e) {
    switch (status){
        case SetEntity:{
            auto entity = catchEntity(e, enTypeList);
            if (entity != nullptr && mayModifySplineEntity(entity)){
                entityToModify = entity;
                entityToModify->setSelected(true);
                switch (entityToModify->rtti()){
                    case RS2::EntitySplinePoints:{
                        auto* sp = dynamic_cast<LC_SplinePoints *>(entityToModify);
                        splineIsClosed = sp->isClosed();
                        break;
                    }
                    case RS2::EntitySpline:{
                        auto* sp = dynamic_cast<RS_Spline*>(entityToModify);
                        splineIsClosed = sp->isClosed();
                        break;
                    }
                    default:
                        break;
                }

                graphicView->redraw(RS2::RedrawDrawing);
                setStatus(SetBeforeControlPoint);
            }
            break;
        }
        case SetBeforeControlPoint:{
            RS_Vector mouse = snapPoint(e);
            double dist;
            RS_Vector nearestPoint = entityToModify->getNearestRef(mouse, &dist);
            if (nearestPoint.valid){
                selectedVertexPoint = nearestPoint;
                setStatus(SetControlPoint);
            }
            break;
        }
        case SetControlPoint: {
            RS_Vector mouse = snapPoint(e);
            double dist;
            RS_Vector nearestPoint = entityToModify->getNearestRef(mouse, &dist);
            if (nearestPoint.valid){
                if (nearestPoint != vertexPoint) {
                    vertexPoint = nearestPoint;
                    directionFromStart = isShift(e);
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
    bool deleteNotFoundPoints = startDirection && splineIsClosed;
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

            if (isValidSplinePointsData(remainingPoints.size(), splineIsClosed)) {
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

            if (isValidSplineData(remainingPoints.size(), splineIsClosed, spline->getDegree())){
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
        if (cp == selectedVertexPoint || cp == controlPoint){
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
            updateMouseWidgetTRCancel(tr("Select second point for deletion"), splineIsClosed ? MOD_SHIFT_LC(tr("Delete from endpoints TO selected points")) : MOD_NONE);
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
