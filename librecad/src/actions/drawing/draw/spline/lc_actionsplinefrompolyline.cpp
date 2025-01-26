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
#include "lc_splinefrompolylineoptions.h"
#include "lc_splinepoints.h"
#include "rs_arc.h"
#include "rs_document.h"
#include "rs_graphic.h"
#include "rs_graphicview.h"
#include "rs_line.h"
#include "rs_polyline.h"
#include "rs_spline.h"

LC_ActionSplineFromPolyline::LC_ActionSplineFromPolyline(RS_EntityContainer &container, RS_GraphicView &graphicView)
:RS_PreviewActionInterface("ActionSplineFromPolyline",container, graphicView) {
    actionType = RS2::ActionDrawSplineFromPolyline;
}

void LC_ActionSplineFromPolyline::doTrigger() {
    if (document) {
        RS_Entity* createdEntity = createSplineForPolyline(entityToModify);

        if (createdEntity != nullptr) {
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

            undoCycleStart();
            setupAndAddCreatedEntity(createdEntity, layerToSet, penToUse);

            if (!keepOriginals){
                undoableDeleteEntity(entityToModify);
            }

            undoCycleEnd();

            entityToModify = nullptr;
        }
    }
}

void LC_ActionSplineFromPolyline::finish(bool updateTB) {
    RS_PreviewActionInterface::finish(updateTB);
}

void LC_ActionSplineFromPolyline::setupAndAddCreatedEntity(RS_Entity *createdEntity, RS_Layer *layerToSet, const RS_Pen &penToUse) {
    // todo - sand - isn't it a candidates for some reusable util?
    createdEntity->setParent(container);
    createdEntity->setPen(penToUse);
    createdEntity->setLayer(layerToSet);
    createdEntity->setSelected(true); // fixme - sand - check whether it should be selected??
    container->addEntity(createdEntity);
    undoableAdd(createdEntity);
}

void LC_ActionSplineFromPolyline::mouseMoveEvent(QMouseEvent *e) {
    deleteHighlights();
    deletePreview();
    RS_Vector mouse = snapPoint(e);
    int status = getStatus();
    switch (status) {
        case SetEntity: {
            auto polyline = catchEntityOnPreview(e, RS2::EntityPolyline);
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
    drawHighlights();
    drawPreview();
}

void LC_ActionSplineFromPolyline::onMouseLeftButtonRelease(int status, QMouseEvent *e) {
    switch (status) {
        case SetEntity: {
            auto polyline = catchEntity(e, RS2::EntityPolyline);
            if (polyline != nullptr) {
                entityToModify = dynamic_cast<RS_Polyline *>(polyline);
                trigger();
            }
            break;
        }
        default:
            break;
    }
}

void LC_ActionSplineFromPolyline::onMouseRightButtonRelease([[maybe_unused]]int status,[[maybe_unused]] QMouseEvent *e) {
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
    auto* polyline = reinterpret_cast<RS_Polyline *>(p);
    bool closed = polyline->isClosed();
    if (vertexesAreFitPoints && splineDegree == 2){
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
        switch (splineDegree) {
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
            RS_SplineData data = RS_SplineData(splineDegree, closed);
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
    controlPoints.reserve(polyline->count() * (segmentMiddlePoints + 1) + 1);
    controlPoints.push_back(polyline->getStartpoint());

    int index = 0;
    for (RS_Entity *entity = polyline->firstEntity(RS2::ResolveAll); entity; entity = polyline->nextEntity(RS2::ResolveAll)) {
        index++;
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
                int counts=segmentMiddlePoints+1;

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
                for (int i = 1; i < segmentMiddlePoints+1; i++){
                    RS_Vector midPoint =  startPoint + dvp*(double(i)/double(segmentMiddlePoints));
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
