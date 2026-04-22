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

#include "lc_action_spline_modify_explode.h"

#include "lc_spline_explode_options_filler.h"
#include "lc_spline_explode_options_widget.h"
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
    :LC_ActionSplineModifyBase("ActionDrawSplineExplode", actionContext, m_actionType = RS2::ActionDrawSplineExplode) {
}

void LC_ActionSplineExplode::doSaveOptions() {
    save("UseCustomSegmentsCount",m_useCustomSegmentsCount);
    save("CustomSegmentsCount",  m_customSegmentsCount);
    save("ToPolyline", m_createPolyline);
    save("UseCurrentLayer", m_useCurrentLayer);
    save("UseCurrentAttributes", m_useCurrentAttributes);
    save("KeepOriginals", m_keepOriginals);
}

void LC_ActionSplineExplode::doLoadOptions() {
    m_useCustomSegmentsCount = loadBool("UseCustomSegmentsCount", true);
    m_customSegmentsCount = loadInt("CustomSegmentsCount", m_customSegmentsCount);
    m_createPolyline = loadBool("ToPolyline", true);
    m_useCurrentLayer = loadBool("UseCurrentLayer", true);
    m_useCurrentAttributes = loadBool("UseCurrentAttributes", true);
    m_keepOriginals = loadBool("KeepOriginals", true);
}

bool LC_ActionSplineExplode::doTriggerModifications(LC_DocumentModificationBatch& ctx) {
    int segmentsCount = obtainSegmentsCount();
    std::vector<RS_Vector> strokePoints;
    bool closed = false;
    fillStrokePoints(m_entityToModify, segmentsCount, strokePoints, closed);
    if (!strokePoints.empty()) {
        RS_Layer* layerToSet = nullptr;
        if (m_useCurrentLayer) {
            layerToSet = m_graphicView->getGraphic()->getActiveLayer();
        }
        else {
            layerToSet = m_entityToModify->getLayer();
        }

        RS_Pen penToUse;
        if (m_useCurrentAttributes) {
            penToUse = m_graphicView->getGraphic()->getActivePen();
        }
        else {
            penToUse = m_entityToModify->getPen(false);
        }

        ctx.dontSetActiveLayerAndPen();

        if (m_createPolyline) {
            auto createdEntity = createPolylineByVertexes(strokePoints, closed);
            createdEntity->setPen(penToUse);
            createdEntity->setLayer(layerToSet);
            ctx += createdEntity;
        }
        else {
            RS_Vector startPoint = strokePoints.at(0);
            RS_Vector firstPoint = startPoint;
            size_t size = strokePoints.size();
            for (size_t i = 1; i < size; i++) {
                RS_Vector end            = strokePoints.at(i);
                auto createdEntity = new RS_Line(m_document, startPoint, end);
                createdEntity->setPen(penToUse);
                createdEntity->setLayer(layerToSet);
                ctx += createdEntity;
                startPoint = end;
            }
            if (closed) {
                auto createdEntity = new RS_Line(m_document, startPoint, firstPoint);
                createdEntity->setPen(penToUse);
                createdEntity->setLayer(layerToSet);
                ctx += createdEntity;
            }
        }
        if (!m_keepOriginals) {
            ctx -= m_entityToModify;
        }
        return true;
    }
    return false;
}

void LC_ActionSplineExplode::doTriggerSelections(const LC_DocumentModificationBatch& ctx) {
    if (ctx.success) {
        if (m_keepOriginals) {
            unselect(m_entityToModify);
        }
        select(ctx.entitiesToAdd);
    }
}

void LC_ActionSplineExplode::doTriggerCompletion([[maybe_unused]]bool success) {
    m_entityToModify = nullptr;
}

bool LC_ActionSplineExplode::mayModifySplineEntity(RS_Entity* entity) {
    return entity != nullptr && g_enTypeList.contains(entity->rtti());
}

void LC_ActionSplineExplode::onMouseMove(const RS_Vector mouse, const int status, const LC_MouseEvent* e) {
    switch (status) {
        case SetEntity: {
            const auto entity = catchEntityByEvent(e, g_enTypeList);
            if (entity != nullptr){
                if (mayModifySplineEntity(entity)) {
                    highlightHoverWithRefPoints(entity, true);
                    const RS_Entity *previewExplodedEntity = createModifiedSplineEntity(entity, mouse, false);
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

void LC_ActionSplineExplode::setEntityToModify(RS_Entity* entity) {
    m_entityToModify = entity;
    trigger();
}

void LC_ActionSplineExplode::onMouseLeftButtonRelease([[maybe_unused]]int status, const LC_MouseEvent* e) {
    const auto entity = catchEntityByEvent(e, g_enTypeList);
    if (entity != nullptr) {
        if (mayModifySplineEntity(entity)) {
            setEntityToModify(entity);
        }
    }
}

RS_Entity *LC_ActionSplineExplode::createModifiedSplineEntity(RS_Entity *e, [[maybe_unused]]RS_Vector controlPoint, [[maybe_unused]]bool startDirection) {
    const int segmentsCount = obtainSegmentsCount();
    std::vector<RS_Vector> strokePoints;
    bool closed = false;
    fillStrokePoints(e, segmentsCount, strokePoints, closed);
    if (!strokePoints.empty()) {
        RS_Entity *pEntity = createPolylineByVertexes(strokePoints, closed);

        return pEntity;
    }
    return nullptr;
}

void  LC_ActionSplineExplode::fillStrokePoints(RS_Entity *e, const int segmentsCount, std::vector<RS_Vector> &strokePoints, bool &closed) const {
    switch (e->rtti()){
        case RS2::EntitySplinePoints:{
            const auto* splinePoints = static_cast<LC_SplinePoints *>(e);
            splinePoints->fillStrokePoints(segmentsCount, strokePoints);
            closed = splinePoints->isClosed();
            break;
        }
        case RS2::EntitySpline:{
            const auto* spline = static_cast<RS_Spline *>(e);
            spline->fillStrokePoints(segmentsCount, strokePoints);
            closed = spline->isClosed();
            break;
        }
        default:
            break;
    }
}

int LC_ActionSplineExplode::obtainSegmentsCount() const {
    int segmentsCount = 0;
    if (m_useCustomSegmentsCount){
        segmentsCount = m_customSegmentsCount;
    }
    else{
        segmentsCount = getSegmentsCountFromDrawing();
    }
    return segmentsCount;
}

RS_Entity *LC_ActionSplineExplode::createPolylineByVertexes(const std::vector<RS_Vector> &strokePoints, const bool closed) const {
    const auto result = new RS_Polyline(nullptr);
    const size_t size = strokePoints.size();
    for (size_t i= 0; i < size; i++){
        const RS_Vector vertex = strokePoints[i];
        result->addVertex(vertex);
    }
    result->setClosed(closed);
    return result;
}

void LC_ActionSplineExplode::updateActionPrompt() {
    switch (getStatus()){
        case SetEntity:{
            updatePromptTRCancel(tr("Select spline or spline points entity"));
            break;
        }
        default:
            break;
    }
}

LC_ActionOptionsWidget *LC_ActionSplineExplode::createOptionsWidget() {
    return new LC_SplineExplodeOptionsWidget();
}

LC_ActionOptionsPropertiesFiller* LC_ActionSplineExplode::createOptionsFiller() {
   return new LC_SplineExplodeOptionsFiller();
}

int LC_ActionSplineExplode::getSegmentsCountFromDrawing() const {
    const RS_Graphic* graphic = m_graphicView->getGraphic();
    int result=8;
    if (graphic != nullptr) {
        result = graphic->getVariableInt("$SPLINESEGS", result);
    }
    return result;
}
