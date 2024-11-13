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

#include "lc_actionsplineaddpoint.h"
#include "rs_graphicview.h"
#include "lc_splinepoints.h"
#include "rs_spline.h"
#include "rs_document.h"

namespace {
    const EntityTypeList enTypeList = {RS2::EntitySpline, RS2::EntitySplinePoints};
}

LC_ActionSplineAddPoint::LC_ActionSplineAddPoint(RS_EntityContainer &container, RS_GraphicView &graphicView):RS_PreviewActionInterface("SplineAddPoint", container, graphicView) {
    actionType = RS2::ActionDrawSplinePointAdd;
}

void LC_ActionSplineAddPoint::trigger() {
    RS_PreviewActionInterface::trigger();
    RS_Entity* createdEntity = createModifiedSplineEntity(entityToModify, vertexPoint, insertAfterSelected);
    if (createdEntity != nullptr){
        if (document) {
            document->startUndoCycle();
            createdEntity->setSelected(true);
            createdEntity->setLayer(entityToModify->getLayer());
            createdEntity->setPen(entityToModify->getPen());
            createdEntity->setParent(entityToModify->getParent());
            container->addEntity(createdEntity);
            document->addUndoable(createdEntity);
            deleteEntityUndoable(entityToModify);
            document->endUndoCycle();
        }
        entityToModify = createdEntity;
        vertexPoint = RS_Vector(false);
        selectedVertexPoint = RS_Vector(false);
        deleteHighlights();

        insertAfterSelected = false;
        endpointIsSelected = false;

        moveRelativeZero(vertexPoint);

        setStatus(SetBeforeControlPoint);
    }
    updateSelectionWidget();
    graphicView->redraw();
}

void LC_ActionSplineAddPoint::mouseMoveEvent(QMouseEvent *e) {
    RS_Vector mouse = snapPoint(e);
    int status = getStatus();
    deleteHighlights();
    deletePreview();
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
            RS_Vector nearestRef = entityToModify->getNearestRef(mouse);
            if (nearestRef != entityToModify->getStartpoint()){
                RS_Vector nearestPointOnEntity = entityToModify->getNearestPointOnEntity(mouse, true);
                previewRefPoint(nearestPointOnEntity);
                previewRefSelectablePoint(nearestRef);
            }
            break;
        }
        case SetControlPoint:{
            previewRefSelectablePoint(mouse);
            RS_Vector nearestPointOnEntity = entityToModify->getNearestPointOnEntity(mouse, true);
            if (nearestPointOnEntity.valid){
                previewRefPoint(nearestPointOnEntity);
                previewRefPoint(selectedVertexPoint);
            }
            bool insertAfter = false;
            if (isShift(e)){
                if (!endpointIsSelected){ // don't let inserting after endpoint
                    insertAfter = true;
                }
            }
            RS_Entity *previewUpdatedEntity = createModifiedSplineEntity(entityToModify, mouse, insertAfter);
            if (previewUpdatedEntity != nullptr) {
                previewEntity(previewUpdatedEntity);
            }
            break;
        }
        default:
            break;
    }
    drawHighlights();
    drawPreview();
}

void LC_ActionSplineAddPoint::onMouseLeftButtonRelease(int status, QMouseEvent *e) {
    switch (status){
        case SetEntity:{
            auto entity = catchEntity(e, enTypeList);
            if (entity != nullptr && mayModifySplineEntity(entity)){
                entityToModify = entity;
                entityToModify->setSelected(true);
                graphicView->redraw(RS2::RedrawDrawing);
                setStatus(SetBeforeControlPoint);
            }
            break;
        }
        case SetBeforeControlPoint:{
            RS_Vector mouse = snapPoint(e);
            RS_Vector nearestRef = entityToModify->getNearestRef(mouse);
            if (nearestRef != entityToModify->getStartpoint()){
                selectedVertexPoint = nearestRef;
                moveRelativeZero(nearestRef);
                setStatus(SetControlPoint);
                endpointIsSelected = nearestRef == entityToModify->getEndpoint();
            }
            break;
        }
        case SetControlPoint:{
            RS_Vector mouse = snapPoint(e);
            vertexPoint = mouse;
            if (isShift(e)){
                if (!endpointIsSelected){ // don't let inserting after endpoint
                    insertAfterSelected = true;
                }
            }
            trigger();
        }
        default:
            break;
    }
}

RS_Entity *LC_ActionSplineAddPoint::createModifiedSplineEntity(RS_Entity *e, RS_Vector controlPoint, bool ajustPosition) {
    RS_Entity* result = nullptr;
    switch (e->rtti()){
        case RS2::EntitySplinePoints:{
            auto* splinePoints = dynamic_cast<LC_SplinePoints *>(e->clone());
            LC_SplinePointsData &data = splinePoints->getData();
            unsigned int controlPointsCount = data.controlPoints.size();
            int splinePointsCount = data.splinePoints.size();
            int insertIndexAdjustment = ajustPosition ? 1 : 0;
            if (splinePointsCount > 0){
                for (int i = 0; i < splinePointsCount; i++) {
                    RS_Vector cp = data.splinePoints.at(i);
                    if (cp == selectedVertexPoint) {
                        data.splinePoints.insert(data.splinePoints.begin()+ i + insertIndexAdjustment, controlPoint);
                        break;
                    }
                }
            }
            else{
                for (int i = 0; i < controlPointsCount; i++) {
                    RS_Vector cp = data.controlPoints.at(i);
                    if (cp == selectedVertexPoint) {
                        data.controlPoints.insert(data.controlPoints.begin()+ i + insertIndexAdjustment, controlPoint);
                        break;
                    }
                }
            }

            splinePoints->update();
            result = splinePoints;
            break;
        }
        case RS2::EntitySpline:{
            auto* spline = dynamic_cast<RS_Spline *>(e);
            RS_SplineData data = spline->getData();
            unsigned int controlPointsCount = data.controlPoints.size();
            int insertIndexAdjustment = ajustPosition ? 1 : 0;
            for (int i = 0; i < controlPointsCount; i++) {
                RS_Vector cp = data.controlPoints.at(i);
                if (cp == selectedVertexPoint) {
                    data.controlPoints.insert(data.controlPoints.begin()+ i + insertIndexAdjustment, controlPoint);
                    break;
                }
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


void LC_ActionSplineAddPoint::onMouseRightButtonRelease(int status, QMouseEvent *e) {
    deleteSnapper();
    deletePreview();
    drawPreview();
    int newStatus = status - 1;
    if (newStatus == SetEntity){
        clean();
    }
    setStatus(newStatus);
}

void LC_ActionSplineAddPoint::finish(bool updateTB) {
    clean();
    RS_PreviewActionInterface::finish(updateTB);
}

void LC_ActionSplineAddPoint::clean() {
    if (entityToModify){
        entityToModify->setSelected(false);
    }
    deletePreview();
    graphicView->redraw();
}

bool LC_ActionSplineAddPoint::mayModifySplineEntity(RS_Entity *pEntity) {
    return true;
}

void LC_ActionSplineAddPoint::updateMouseButtonHints() {
    switch (getStatus()){
        case SetEntity:{
            updateMouseWidgetTRCancel(tr("Select spline or spline points entity"));
            break;
        }
        case SetBeforeControlPoint: {
            updateMouseWidgetTRCancel(tr("Select control point to insert before"));
            break;
        }
        case SetControlPoint: {
            updateMouseWidgetTRCancel(tr("Specify control point"), endpointIsSelected ? MOD_NONE : MOD_SHIFT_LC(tr("Inserted AFTER selected point")));
            break;
        }
        default:
            break;
    }
}

RS2::CursorType LC_ActionSplineAddPoint::doGetMouseCursor(int status) {
    return RS2::CadCursor;
}
