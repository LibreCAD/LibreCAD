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

#include "lc_actionsplinefrompolyline.h"
#include "lc_containertraverser.h"

#include "lc_splinefrompolylineoptions.h"
#include "lc_splinepoints.h"
#include "rs_arc.h"
#include "rs_graphic.h"
#include "rs_graphicview.h"
#include "rs_line.h"
#include "rs_pen.h"
#include "rs_polyline.h"
#include "rs_spline.h"

LC_ActionSplineFromPolyline::LC_ActionSplineFromPolyline(LC_ActionContext *actionContext)
    :RS_PreviewActionInterface("ActionSplineFromPolyline", actionContext, RS2::ActionDrawSplineFromPolyline) {
}

void LC_ActionSplineFromPolyline::doTrigger() {
    if (m_document) {
        RS_Entity* createdEntity = createSplineForPolyline(m_entityToModify);

        if (createdEntity != nullptr) {
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
            setupAndAddCreatedEntity(createdEntity, layerToSet, penToUse);

            if (!m_keepOriginals){
                undoableDeleteEntity(m_entityToModify);
            }

            undoCycleEnd();

            m_entityToModify = nullptr;
        }
    }
}

void LC_ActionSplineFromPolyline::finish(bool updateTB) {
    RS_PreviewActionInterface::finish(updateTB);
}

void LC_ActionSplineFromPolyline::setupAndAddCreatedEntity(RS_Entity *createdEntity, RS_Layer *layerToSet, const RS_Pen &penToUse) {
    // todo - sand - isn't it a candidates for some reusable util?
    createdEntity->setParent(m_container);
    createdEntity->setPen(penToUse);
    createdEntity->setLayer(layerToSet);
    createdEntity->setSelected(true); // fixme - sand - check whether it should be selected??
    m_container->addEntity(createdEntity);
    undoableAdd(createdEntity);
}

void LC_ActionSplineFromPolyline::onMouseMoveEvent(int status, LC_MouseEvent *e) {
    switch (status) {
        case SetEntity: {
            auto polyline = catchAndDescribe(e, RS2::EntityPolyline);
            if (polyline != nullptr) {
                highlightHoverWithRefPoints(polyline, true);
                auto splinePreview = createSplineForPolyline(polyline);
                if (splinePreview != nullptr){
                    previewEntity(splinePreview);
                }
            }
            break;
        }
        default:
            break;
    }
}

void LC_ActionSplineFromPolyline::onMouseLeftButtonRelease(int status, LC_MouseEvent *e) {
    switch (status) {
        case SetEntity: {
            auto polyline = catchEntityByEvent(e, RS2::EntityPolyline);
            if (polyline != nullptr) {
                m_entityToModify = dynamic_cast<RS_Polyline *>(polyline);
                trigger();
            }
            break;
        }
        default:
            break;
    }
}

void LC_ActionSplineFromPolyline::onMouseRightButtonRelease([[maybe_unused]]int status,[[maybe_unused]] LC_MouseEvent *e) {
    setStatus(-1);
}

void LC_ActionSplineFromPolyline::updateMouseButtonHints() {
    switch (getStatus()){
        case SetEntity:{
            updateMouseWidgetTRCancel(tr("Select polyline to create spline or spline points"));
            break;
        }
        default:
            break;
    }
}

RS2::CursorType LC_ActionSplineFromPolyline::doGetMouseCursor([[maybe_unused]]int status) {
    return RS2::CadCursor;
}

LC_ActionOptionsWidget *LC_ActionSplineFromPolyline::createOptionsWidget() {
    return new LC_SplineFromPolylineOptions();
}

RS_Entity* LC_ActionSplineFromPolyline::createSplineForPolyline(RS_Entity *p) {
    auto* polyline = static_cast<RS_Polyline *>(p);
    bool closed = polyline->isClosed();
    if (m_vertexesAreFitPoints && m_splineDegree == 2){
        LC_SplinePointsData data = LC_SplinePointsData(closed, false);
        auto* result = new LC_SplinePoints(nullptr, data);

        std::vector<RS_Vector> controlPoints;
        fillControlPointsListFromPolyline(polyline, controlPoints);
        int count = controlPoints.size();
        if (closed){
            count--;
        }
        for (int i = 0; i < count; i++ ){
           RS_Vector point = controlPoints.at(i);
           result->addPoint(point);
        }
        result->update();
        return result;
    }
    else{
        std::vector<RS_Vector> controlPoints;
        fillControlPointsListFromPolyline(polyline, controlPoints);
        int count = controlPoints.size();
        if (closed){
            count--;
        }
        bool enoughPoints = false;
        switch (m_splineDegree) {
            case 1: {
                enoughPoints = count > 2;
                break;
            }
            case 2: {
                enoughPoints = count > 3;
                break;
            }
            case 3: {
                enoughPoints = count > 3;
                break;
            }
            default:
                enoughPoints = true;
        }

        if (enoughPoints) {
            RS_SplineData data = RS_SplineData(m_splineDegree, closed);
            auto* result = new RS_Spline(nullptr, data);
            for (int i = 0; i < count; i++) {
                RS_Vector point = controlPoints.at(i);
                result->addControlPoint(point);
            }
            result->update();
            return result;
        }
        else{
            return nullptr;
        }
    }
}

void LC_ActionSplineFromPolyline::fillControlPointsListFromPolyline(const RS_Polyline *polyline, std::vector<RS_Vector> &controlPoints) const {
    controlPoints.reserve(polyline->count() * (m_segmentMiddlePoints + 1) + 1);
    controlPoints.push_back(polyline->getStartpoint());
    for(RS_Entity* entity: lc::LC_ContainerTraverser{*polyline, RS2::ResolveAll}.entities()) {
        if (!entity->isAtomic()){
            continue;
        }
        int rtti = entity->rtti();
        switch (rtti) {
            case RS2::EntityArc: {
                auto *arc = dynamic_cast<RS_Arc *> (entity);

                // todo - determining middle point should be moved either to entity or to some utility
                double amin=arc->getAngle1();
                double amax=arc->getAngle2();
               
                double da;

                bool reversed = arc->isReversed();
                if (reversed){
                    da =fmod(amin-amax+2.*M_PI, 2.*M_PI);
                }
                else{
                    da =fmod(amax-amin+2.*M_PI, 2.*M_PI);
                }

                if ( da < RS_TOLERANCE ) {
                    da= 2.*M_PI; // whole circle
                }
                int counts=m_segmentMiddlePoints+1;

                RS_Vector center = arc->getCenter();
                double radius = arc->getRadius();

                for (int i = 1; i < counts; i++){
                    double angle;
                    if (reversed) {
                        angle = amin - da * (double(i) / double(counts));
                    }
                    else{
                        angle = amin + da * (double(i) / double(counts));
                    }
                    RS_Vector midPoint = center.relative(radius, angle);
                    controlPoints.push_back(midPoint);
                }

                controlPoints.push_back(arc->getEndpoint());
                break;
            }
            case RS2::EntityLine: {
                auto* line = dynamic_cast<RS_Line *>(entity);
                const RS_Vector &startPoint = line->getStartpoint();
                RS_Vector dvp(line->getEndpoint() - startPoint);
                // todo - move collection of mid points (actually, points that divide the entity to specific values to common entity interface or util
                for (int i = 1; i < m_segmentMiddlePoints+1; i++){
                    RS_Vector midPoint =  startPoint + dvp*(double(i)/double(m_segmentMiddlePoints));
                    controlPoints.push_back(midPoint);
                }
                controlPoints.push_back(line->getEndpoint());
                break;
            }
            default: // actually, only line and arc in polyline, yet still...
                break;
        }
    }
}
