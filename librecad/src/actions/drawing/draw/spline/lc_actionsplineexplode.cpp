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

#include "lc_actionsplineexplode.h"

#include "lc_splineexplodeoptions.h"
#include "lc_splinepoints.h"
#include "rs_entity.h"
#include "rs_graphic.h"
#include "rs_graphicview.h"
#include "rs_line.h"
#include "rs_pen.h"
#include "rs_polyline.h"
#include "rs_spline.h"

namespace {
    // fixme - sand - think about support parabola as other splines

    const EntityTypeList g_enTypeList = {RS2::EntitySpline, RS2::EntitySplinePoints/*, RS2::EntityParabola*/};
}

// fixme - potentially, the action may be expanded even more - and used to expand circle, arcs, and ellipses to lines.
// fixme - todo - sand - potentially, the action may be improved by specifying maximum distance of approximation (instead of segments count).
//  Need to think about this...
LC_ActionSplineExplode::LC_ActionSplineExplode(LC_ActionContext *actionContext)
    :LC_ActionSplineModifyBase("SplineExplode", actionContext, m_actionType = RS2::ActionDrawSplineExplode) {
}

void LC_ActionSplineExplode::doTrigger() {
    if (m_document) {
        int segmentsCount = obtainSegmentsCount();
        std::vector<RS_Vector> strokePoints;
        bool closed;
        fillStrokePoints(m_entityToModify, segmentsCount, strokePoints, closed);
        if (!strokePoints.empty()) {
            RS_Layer *layerToSet;
            if (m_useCurrentLayer) {
                layerToSet = m_graphicView->getGraphic()->getActiveLayer();
            } else {
                layerToSet = m_entityToModify->getLayer();
            }

            RS_Pen penToUse;
            if (m_useCurrentAttributes) {
                penToUse = m_graphicView->getGraphic()->getActivePen();
            } else {
                penToUse = m_entityToModify->getPen(false);
            }

            undoCycleStart();

            if (m_createPolyline) {
                RS_Entity *createdEntity = createPolylineByVertexes(strokePoints, closed);
                setupAndAddCreatedEntity(createdEntity, layerToSet, penToUse);
            } else {
                RS_Vector startPoint = strokePoints.at(0);
                RS_Vector firstPoint = startPoint;
                for (unsigned int i=1; i < strokePoints.size(); i++){
                    RS_Vector end = strokePoints.at(i);
                    RS_Entity* createdEntity = new RS_Line(m_container, startPoint, end);
                    setupAndAddCreatedEntity(createdEntity, layerToSet, penToUse);
                    startPoint = end;
                }
                if (closed){
                    RS_Entity* createdEntity = new RS_Line(m_container, startPoint, firstPoint);
                    setupAndAddCreatedEntity(createdEntity, layerToSet, penToUse);
                }

            }
            if (!m_keepOriginals){
                undoableDeleteEntity(m_entityToModify);
            }

            undoCycleEnd();
            m_entityToModify = nullptr;
        }
    }
}

void LC_ActionSplineExplode::setupAndAddCreatedEntity(RS_Entity *createdEntity, RS_Layer *layerToSet, const RS_Pen &penToUse) {
    createdEntity->setParent(m_container);
    createdEntity->setPen(penToUse);
    createdEntity->setLayer(layerToSet);
    createdEntity->setSelected(true); // fixme - sand - check whether it should be selected??
    m_container->addEntity(createdEntity);
    undoableAdd(createdEntity);
}

void LC_ActionSplineExplode::onMouseMove(RS_Vector mouse, int status, LC_MouseEvent *e) {
    switch (status) {
        case SetEntity: {
            auto entity = catchEntityByEvent(e, g_enTypeList);
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
}

void LC_ActionSplineExplode::onMouseLeftButtonRelease([[maybe_unused]]int status, LC_MouseEvent *e) {
    auto entity = catchEntityByEvent(e, g_enTypeList);
    if (entity != nullptr) {
        if (mayModifySplineEntity(entity)) {
            m_entityToModify = entity;
            trigger();
        }
    }
}

RS_Entity *LC_ActionSplineExplode::createModifiedSplineEntity(RS_Entity *e, [[maybe_unused]]RS_Vector controlPoint, [[maybe_unused]]bool startDirection) {
    int segmentsCount = obtainSegmentsCount();
    std::vector<RS_Vector> strokePoints;
    bool closed;
    fillStrokePoints(e, segmentsCount, strokePoints, closed);
    if (!strokePoints.empty()) {
        RS_Entity *pEntity = createPolylineByVertexes(strokePoints, closed);

        return pEntity;
    }
    return nullptr;
}

void  LC_ActionSplineExplode::fillStrokePoints(RS_Entity *e, int segmentsCount, std::vector<RS_Vector> &strokePoints, bool &closed) const {
    switch (e->rtti()){
        case RS2::EntitySplinePoints:{
            auto* splinePoints = static_cast<LC_SplinePoints *>(e);
            splinePoints->fillStrokePoints(segmentsCount, strokePoints);
            closed = splinePoints->isClosed();
            break;
        }
        case RS2::EntitySpline:{
            auto* spline = static_cast<RS_Spline *>(e);
            spline->fillStrokePoints(segmentsCount, strokePoints);
            closed = spline->isClosed();
            break;
        }
        default:
            break;
    }
}

int LC_ActionSplineExplode::obtainSegmentsCount() {
    int segmentsCount;
    if (m_useCustomSegmentsCount){
        segmentsCount = m_customSegmentsCount;
    }
    else{
        segmentsCount = getSegmentsCountFromDrawing();
    }
    return segmentsCount;
}

RS_Entity *LC_ActionSplineExplode::createPolylineByVertexes(const std::vector<RS_Vector> &strokePoints, bool closed) const {
    auto result = new RS_Polyline(nullptr);
    for (unsigned int i= 0; i < strokePoints.size(); i++){
        RS_Vector vertex = strokePoints[i];
        result->addVertex(vertex);
    }
    result->setClosed(closed);
    return result;
}

void LC_ActionSplineExplode::updateMouseButtonHints() {
    switch (getStatus()){
        case SetEntity:{
            updateMouseWidgetTRCancel(tr("Select spline or spline points entity"));
            break;
        }
        default:
            break;
    }
}

LC_ActionOptionsWidget *LC_ActionSplineExplode::createOptionsWidget() {
    return new LC_SplineExplodeOptions();
}

int LC_ActionSplineExplode::getSegmentsCountFromDrawing() {
    RS_Graphic* graphic = m_graphicView->getGraphic();
    int result=8;
    if (graphic) {
        result = graphic->getVariableInt("$SPLINESEGS", result);
    }
    return result;
}
