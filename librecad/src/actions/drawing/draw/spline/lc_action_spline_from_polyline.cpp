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

#include "lc_action_spline_from_polyline.h"

#include "lc_containertraverser.h"
#include "lc_spline_from_polyline_options_filler.h"
#include "lc_spline_from_polyline_options_widget.h"
#include "lc_splinepoints.h"
#include "rs_arc.h"
#include "rs_graphic.h"
#include "rs_graphicview.h"
#include "rs_line.h"
#include "rs_pen.h"
#include "rs_polyline.h"
#include "rs_spline.h"

LC_ActionSplineFromPolyline::LC_ActionSplineFromPolyline(LC_ActionContext* actionContext)
    : LC_UndoableDocumentModificationAction("ActionDrawSplineFromPolyline", actionContext, RS2::ActionDrawSplineFromPolyline) {
}

void LC_ActionSplineFromPolyline::doSaveOptions() {
    save("Degree", m_splineDegree);
    save("MidPointsCount", m_segmentMiddlePoints);
    save("UseFitPoints", m_vertexesAreFitPoints);
    save("UseCurrentLayer", m_useCurrentLayer);
    save("UseCurrentAttributes", m_useCurrentAttributes);
    save("KeepOriginals", m_keepOriginals);
}

void LC_ActionSplineFromPolyline::doLoadOptions() {
    m_splineDegree = loadInt("Degree", 3);
    m_segmentMiddlePoints = loadInt("MidPointsCount", 2);
    m_vertexesAreFitPoints = loadBool("UseFitPoints", true);
    m_useCurrentLayer = loadBool("UseCurrentLayer", true);
    m_useCurrentAttributes = loadBool("UseCurrentAttributes", true);
    m_keepOriginals = loadBool("KeepOriginals", true);
}

void LC_ActionSplineFromPolyline::doInitWithContextEntity(RS_Entity* contextEntity, [[maybe_unused]] const RS_Vector& clickPos) {
    if (isPolyline(contextEntity)) {
        setEntityToModify(contextEntity);
    }
}

bool LC_ActionSplineFromPolyline::doTriggerModifications(LC_DocumentModificationBatch& ctx) {
    auto createdEntity = createSplineForPolyline(m_polyline);

    if (createdEntity != nullptr) {
        RS_Layer* layerToSet = nullptr;
        if (m_useCurrentLayer) {
            layerToSet = m_graphicView->getGraphic()->getActiveLayer();
        }
        else {
            layerToSet = m_polyline->getLayer();
        }

        RS_Pen penToUse;
        if (m_useCurrentAttributes) {
            penToUse = m_graphicView->getGraphic()->getActivePen();
        }
        else {
            penToUse = m_polyline->getPen(false);
        }
        createdEntity->setPen(penToUse);
        createdEntity->setLayer(layerToSet);

        ctx.dontSetActiveLayerAndPen();

        ctx += createdEntity;

        if (!m_keepOriginals) {
            ctx -= m_polyline;
        }
        return true;
    }
    return false;
}

void LC_ActionSplineFromPolyline::doTriggerSelections(const LC_DocumentModificationBatch& ctx) {
    if (ctx.success) {
        if (m_keepOriginals) {
            unselect(m_polyline);
        }
        select(ctx.entitiesToAdd);
    }
}

void LC_ActionSplineFromPolyline::doTriggerCompletion(const bool success) {
    if (success) {
        m_polyline = nullptr;
    }
}

void LC_ActionSplineFromPolyline::finish() {
    RS_PreviewActionInterface::finish();
}

void LC_ActionSplineFromPolyline::onMouseMoveEvent(const int status, const LC_MouseEvent* e) {
    switch (status) {
        case SetEntity: {
            const auto polyline = catchAndDescribe(e, RS2::EntityPolyline);
            if (polyline != nullptr) {
                highlightHoverWithRefPoints(polyline, true);
                const auto splinePreview = createSplineForPolyline(polyline);
                if (splinePreview != nullptr) {
                    previewEntity(splinePreview);
                }
            }
            break;
        }
        default:
            break;
    }
}

void LC_ActionSplineFromPolyline::setEntityToModify(RS_Entity* polyline) {
    m_polyline = dynamic_cast<RS_Polyline*>(polyline);
    trigger();
}

void LC_ActionSplineFromPolyline::onMouseLeftButtonRelease(const int status, const LC_MouseEvent* e) {
    switch (status) {
        case SetEntity: {
            const auto polyline = catchEntityByEvent(e, RS2::EntityPolyline);
            if (polyline != nullptr) {
                setEntityToModify(polyline);
            }
            break;
        }
        default:
            break;
    }
}

void LC_ActionSplineFromPolyline::onMouseRightButtonRelease([[maybe_unused]] int status, [[maybe_unused]] const LC_MouseEvent* e) {
    setStatus(-1);
}

void LC_ActionSplineFromPolyline::updateActionPrompt() {
    switch (getStatus()) {
        case SetEntity: {
            updatePromptTRCancel(tr("Select polyline to create spline or spline points"));
            break;
        }
        default:
            break;
    }
}

RS2::CursorType LC_ActionSplineFromPolyline::doGetMouseCursor([[maybe_unused]] int status) {
    return RS2::CadCursor;
}

LC_ActionOptionsWidget* LC_ActionSplineFromPolyline::createOptionsWidget() {
    return new LC_SplineFromPolylineOptionsWidget();
}

LC_ActionOptionsPropertiesFiller* LC_ActionSplineFromPolyline::createOptionsFiller() {
    return new LC_SplineFromPolylineOptionsFiller();
}

RS_Entity* LC_ActionSplineFromPolyline::createSplineForPolyline(RS_Entity* p) const {
    const auto* polyline = static_cast<RS_Polyline*>(p);
    const bool closed = polyline->isClosed();
    if (m_vertexesAreFitPoints && m_splineDegree == 2) {
        const auto data = LC_SplinePointsData(closed, false);
        auto* result = new LC_SplinePoints(nullptr, data);

        std::vector<RS_Vector> controlPoints;
        fillControlPointsListFromPolyline(polyline, controlPoints);
        size_t count = controlPoints.size();
        if (closed) {
            count--;
        }
        for (size_t i = 0; i < count; i++) {
            RS_Vector point = controlPoints.at(i);
            result->addPoint(point);
        }
        result->update();
        return result;
    }
    std::vector<RS_Vector> controlPoints;
    fillControlPointsListFromPolyline(polyline, controlPoints);
    size_t count = controlPoints.size();
    if (closed) {
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
            break;
    }

    if (enoughPoints) {
        const auto data = RS_SplineData(m_splineDegree, closed);
        auto* result = new RS_Spline(nullptr, data);
        for (size_t i = 0; i < count; i++) {
            RS_Vector point = controlPoints.at(i);
            result->addControlPoint(point);
        }
        result->update();
        return result;
    }
    return nullptr;
}

void LC_ActionSplineFromPolyline::fillControlPointsListFromPolyline(const RS_Polyline* polyline,
                                                                    std::vector<RS_Vector>& controlPoints) const {
    controlPoints.reserve(polyline->count() * (m_segmentMiddlePoints + 1) + 1);
    controlPoints.emplace_back(polyline->getStartpoint());
    for (RS_Entity* entity : lc::LC_ContainerTraverser{*polyline, RS2::ResolveAll}.entities()) {
        if (!isAtomic(entity)) {
            continue;
        }
        const int rtti = entity->rtti();
        switch (rtti) {
            case RS2::EntityArc: {
                const auto* arc = static_cast<RS_Arc*>(entity);

                // todo - determining middle point should be moved either to entity or to some utility
                const double amin = arc->getAngle1();
                const double amax = arc->getAngle2();

                double da;

                const bool reversed = arc->isReversed();
                if (reversed) {
                    da = fmod(amin - amax + 2. * M_PI, 2. * M_PI);
                }
                else {
                    da = fmod(amax - amin + 2. * M_PI, 2. * M_PI);
                }

                if (da < RS_TOLERANCE) {
                    da = 2. * M_PI; // whole circle
                }
                const int counts = m_segmentMiddlePoints + 1;

                RS_Vector center = arc->getCenter();
                const double radius = arc->getRadius();

                const double doubleCounts = counts;

                for (int i = 1; i < counts; i++) {
                    double angle;
                    const double doubleI = i;

                    if (reversed) {
                        angle = amin - da * (doubleI / doubleCounts);
                    }
                    else {
                        angle = amin + da * (doubleI / doubleCounts);
                    }
                    RS_Vector midPoint = center.relative(radius, angle);
                    controlPoints.push_back(midPoint);
                }

                controlPoints.emplace_back(arc->getEndpoint());
                break;
            }
            case RS2::EntityLine: {
                const auto* line = static_cast<RS_Line*>(entity);
                const RS_Vector& startPoint = line->getStartpoint();
                RS_Vector dvp(line->getEndpoint() - startPoint);
                // todo - move collection of mid points (actually, points that divide the entity to specific values to common entity interface or util

                const double segmentMiddlePoints = m_segmentMiddlePoints;
                for (int i = 1; i < m_segmentMiddlePoints + 1; i++) {
                    const double doubleI = i;
                    RS_Vector midPoint = startPoint + dvp * (doubleI / segmentMiddlePoints);
                    controlPoints.push_back(midPoint);
                }
                controlPoints.emplace_back(line->getEndpoint());
                break;
            }
            default: // actually, only line and arc in polyline, yet still...
                break;
        }
    }
}
