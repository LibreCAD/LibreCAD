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

#include "rs_graphicview.h"
#include "rs_graphic.h"
#include "rs_polyline.h"
#include "lc_actionsplineexplode.h"
#include "lc_splineexplodeoptions.h"
#include "lc_splinepoints.h"
#include "rs_spline.h"

namespace {
    // fixme - sand - think about support parabola as other splines

    const EntityTypeList enTypeList = {RS2::EntitySpline, RS2::EntitySplinePoints/*, RS2::EntityParabola*/};
}

// fixme - potentially, the action may be expanded even more - and used to expand circle, arcs, and ellipses to lines.
// fixme - todo - sand - potentially, the action may be improved by specifying maximum distance of approximation (instead of segments count).
//  Need to think about this...
LC_ActionSplineExplode::LC_ActionSplineExplode(RS_EntityContainer &container, RS_GraphicView &graphicView)
    :LC_ActionSplineModifyBase("SplineExplode", container, graphicView) {
    actionType = RS2::ActionDrawSplineExplode;
}

void LC_ActionSplineExplode::trigger() {
    RS_PreviewActionInterface::trigger();
    if (document) {
        int segmentsCount = obtainSegmentsCount();
        std::vector<RS_Vector> strokePoints;
        fillStrokePoints(entityToModify, segmentsCount, strokePoints);
        if (!strokePoints.empty()) {

            RS_Layer *layerToSet;
            if (useCurrentLayer) {
                layerToSet = graphicView->getGraphic()->getActiveLayer();
            } else {
                layerToSet = entityToModify->getLayer();
            }

            RS_Pen penToUse;
            if (useCurrentAttributes) {
                penToUse = graphicView->getGraphic()->getActivePen();
            } else {
                penToUse = entityToModify->getPen(false);
            }

            document->startUndoCycle();
            if (createPolyline) {
                RS_Entity *createdEntity = createPolylineByVertexes(strokePoints);
                setupAndAddCreatedEntity(createdEntity, layerToSet, penToUse);
            } else {
                RS_Vector startPoint = strokePoints.at(0);
                for (unsigned int i=1; i < strokePoints.size(); i++){
                    RS_Vector end = strokePoints.at(i);
                    RS_Entity* createdEntity = new RS_Line(container, startPoint, end);
                    setupAndAddCreatedEntity(createdEntity, layerToSet, penToUse);
                    startPoint = end;
                }
            }
            if (!keepOriginals){
                deleteEntityUndoable(entityToModify);
            }

            entityToModify = nullptr;
            document->endUndoCycle();
        }
    }
    graphicView->redraw();
}

void LC_ActionSplineExplode::setupAndAddCreatedEntity(RS_Entity *createdEntity, RS_Layer *layerToSet, const RS_Pen &penToUse) {
    createdEntity->setParent(container);
    createdEntity->setPen(penToUse);
    createdEntity->setLayer(layerToSet);
    createdEntity->setSelected(true); // fixme - sand - check whether it should be selected??
    container->addEntity(createdEntity);
    document->addUndoable(createdEntity);
}

void LC_ActionSplineExplode::mouseMoveEvent(QMouseEvent *e) {
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
                    RS_Entity *previewExplodedEntity = createModifiedSplineEntity(entity, mouse, false);
                    if (previewExplodedEntity != nullptr){
                        previewEntity(previewExplodedEntity);
                    }
                }
            }
            break;
        }
        default:
            break;
    }
    drawHighlights();
    drawPreview();
}

void LC_ActionSplineExplode::onMouseLeftButtonRelease(int status, QMouseEvent *e) {
    auto entity = catchEntity(e, enTypeList);
    if (entity != nullptr) {
        if (mayModifySplineEntity(entity)) {
            entityToModify = entity;
            trigger();
        }
    }
}

RS_Entity *LC_ActionSplineExplode::createModifiedSplineEntity(RS_Entity *e, RS_Vector controlPoint, bool startDirection) {
    int segmentsCount = obtainSegmentsCount();
    std::vector<RS_Vector> strokePoints;
    fillStrokePoints(e, segmentsCount, strokePoints);
    if (!strokePoints.empty()) {
        return createPolylineByVertexes(strokePoints);
    }
    return nullptr;
}

void  LC_ActionSplineExplode::fillStrokePoints(RS_Entity *e, int segmentsCount, std::vector<RS_Vector> &strokePoints) const {
    switch (e->rtti()){
        case RS2::EntitySplinePoints:{
            auto* splinePoints = static_cast<LC_SplinePoints *>(e);
            splinePoints->fillStrokePoints(segmentsCount, strokePoints);
            break;
        }
        case RS2::EntitySpline:{
            auto* spline = static_cast<RS_Spline *>(e);
            spline->fillStrokePoints(segmentsCount, strokePoints);
            break;
        }
        default:
            break;
    }
}

int LC_ActionSplineExplode::obtainSegmentsCount() {
    int segmentsCount;
    if (useCustomSegmentsCount){
        segmentsCount = customSegmentsCount;
    }
    else{
        segmentsCount = getSegmentsCountFromDrawing();
    }
    return segmentsCount;
}

RS_Entity *LC_ActionSplineExplode::createPolylineByVertexes(const std::vector<RS_Vector> &strokePoints) const {
    auto result = new RS_Polyline(nullptr);
    for (int i= 0; i < strokePoints.size(); i++){
        RS_Vector vertex = strokePoints[i];
        result->addVertex(vertex);
    }
    return result;
}

void LC_ActionSplineExplode::updateMouseButtonHints() {
    switch (getStatus()){
        case SetEntity:{
            updateMouseWidgetTRCancel(tr("Select spline or spline points entity"));
            break;
        }
        /*case SetBeforeControlPoint: {
            updateMouseWidgetTRCancel(tr("Specify first control point"),MOD_SHIFT_RELATIVE_ZERO);
            break;
        }
        case SetControlPoint: {
            updateMouseWidgetTRCancel(tr("Specify control point"), MOD_SHIFT_LC("Append to alternative Endpoint"));
            break;
        }*/
        default:
            break;
    }
}

LC_ActionOptionsWidget *LC_ActionSplineExplode::createOptionsWidget() {
    return new LC_SplineExplodeOptions();
}

int LC_ActionSplineExplode::getSegmentsCountFromDrawing() {
    RS_Graphic* graphic = graphicView->getGraphic();
    int result=8;
    if (graphic) {
        result = graphic->getVariableInt("$SPLINESEGS", result);
    }
    return result;
}