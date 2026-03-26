/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2026 LibreCAD.org
 * Copyright (C) 2026 sand1024
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 * ********************************************************************************
 */

#include "lc_visual_snap_manager.h"

#include "lc_graphicviewport.h"
#include "lc_ref_snap_circle.h"
#include "lc_ref_snap_construction_line.h"
#include "lc_ref_snap_line.h"
#include "lc_ref_snap_mark.h"
#include "rs_creation.h"
#include "rs_entitycontainer.h"
#include "rs_information.h"
#include "rs_line.h"
#include "rs_settings.h"

struct EntityHolder;

namespace {
    LC_RefSnapConstructionLine* tryCreateCloseSnapConstructionLine(const RS_Vector& wcsPos, const double ucsSnapSize,
                                                                   const RS_Vector& start, const RS_Vector& end, double& dist) {
        const auto line = new LC_RefSnapConstructionLine(start, end);
        const auto nearestPointOnEntity = line->getNearestPointOnEntity(wcsPos, false, &dist);
        const double wcsDistance = nearestPointOnEntity.distanceTo(wcsPos);
        if (wcsDistance < ucsSnapSize) {
            return line;
        }
        delete line;
        return nullptr;
    }

    LC_RefSnapLine* tryCreateCloseSnapLine(const RS_Vector& wcsPos, const double ucsSnapSize, const RS_Vector& start, const RS_Vector& end,
                                           double& dist) {
        const auto line = new LC_RefSnapLine(nullptr, start, end);
        const auto nearestPointOnEntity = line->getNearestPointOnEntity(wcsPos, false, &dist);
        const double wcsDistance = nearestPointOnEntity.distanceTo(wcsPos);
        if (wcsDistance < ucsSnapSize) {
            return line;
        }
        delete line;
        return nullptr;
    }
}

void VisualSnapOptions::load() {
    LC_GROUP("Snap");
    {
        vertexSizeNormal = LC_GET_INT("VSVertexSize", 6);
        vertexSizeProjected = LC_GET_INT("VSProjectedSnapSize", 4);
        vertexSizeHighlighted = LC_GET_INT("VSHighlightedVertexSize", 10);
        delayMsSnapVertex = LC_GET_INT("VSSnapPointAddingDelay", 300);
        delayMsProjectedSnap = LC_GET_INT("VSVertexAddingDelay", 1200);
        delayMsDocumentEntity = LC_GET_INT("VSDocEntityAddingDelay", 1500);

        createAngleStepRaysForVertexes = LC_GET_BOOL("VSAngleSnapStepRaysVertexes", true);
        createAngleStepRaysForEntitiesEndpoints = LC_GET_BOOL("VSAngleSnapStepRaysRelative", true);
        createVertexVertexDistanceCircles = LC_GET_BOOL("VSVertexVertexDistanceCircles", true);

        autoAddSnappedPointToVisualSnap = LC_GET_BOOL("VSSnapAutoAddSnapPoint", true);
        manualVertexAddingRequiresCTRL = LC_GET_BOOL("VSSnapManualAddingWithCTRL", false);
        guidingEntitiesSnapDistance = LC_GET_INT("VSGuidingEntiitesCatchDistance", 24);
    }
    LC_GROUP_END();
}

LC_VisualSnapManager::LC_VisualSnapManager(RS_Snapper* snapper) : m_snapper{snapper} {
    m_timer.setSingleShot(true);
    m_options.load();
    connect(&m_timer, &QTimer::timeout, this, &LC_VisualSnapManager::performDelayedOperation);
}

LC_VisualSnapManager::~LC_VisualSnapManager() {
    clear();
}

VisualSnapSolution* LC_VisualSnapManager::solveVisualSnap(const RS_Vector& wcsPos) {
    const auto solution = new VisualSnapSolution();
    solveVisualSnap(wcsPos, *solution);
    m_solution.reset(solution);
    return solution;
}

VisualSnapSolution* LC_VisualSnapManager::getCurrentSolution() const {
    return m_solution.get();
}

void LC_VisualSnapManager::previewSolution(RS_EntityContainer* preview) {
    if (m_solution != nullptr) {
        m_mutex.lock();
        auto visualSnapSolution = m_solution.get();
        if (!visualSnapSolution->valid) {
            const RS_Vector wcsPos = visualSnapSolution->wcsPoint;
            solveVisualSnap(wcsPos);
            visualSnapSolution = m_solution.get();
        }
        previewVisualSnapSolution(preview, *visualSnapSolution);
        m_mutex.unlock();
    }
}

void LC_VisualSnapManager::refreshSolutionPreview(RS_EntityContainer* preview) {
    m_mutex.lock();
    if (m_solution != nullptr) {
        const RS_Vector wcsPoint = m_solution->wcsPoint;
        solveVisualSnap(wcsPoint);
        previewVisualSnapSolution(preview, *m_solution.get());
    }
    m_mutex.unlock();
}

// method used for debugging
void LC_VisualSnapManager::previewVisualSnapForPoint(RS_EntityContainer* preview, const RS_Vector& wcsPos) {
    VisualSnapSolution solution;
    solveVisualSnap(wcsPos, solution);
    solveVisualSnap(wcsPos, solution);
    previewVisualSnapSolution(preview, solution);
}

void LC_VisualSnapManager::previewOrdinaryRestrictions(RS_EntityContainer* preview) const {
    const RS_SnapMode* snapMode = m_snapper->getSnapMode();
    const RS_Vector relZero = m_snapper->getRelativeZero();
    constexpr double rayDirectionOffset = 10.0;
    switch (snapMode->restriction) {
        case RS2::RestrictOrthogonal: {
            const auto lineVert = new LC_RefSnapConstructionLine(relZero,
                                                                 m_viewport->restrictVertical(relZero, relZero + rayDirectionOffset));
            preview->addEntity(lineVert);
            const auto lineHor = new LC_RefSnapConstructionLine(relZero,
                                                                m_viewport->restrictHorizontal(relZero, relZero + rayDirectionOffset));
            preview->addEntity(lineHor);
            break;
        }
        case RS2::RestrictHorizontal: {
            const auto lineHor = new LC_RefSnapConstructionLine(relZero,
                                                                m_viewport->restrictHorizontal(relZero, relZero + rayDirectionOffset));
            preview->addEntity(lineHor);
            break;
        }
        case RS2::RestrictVertical: {
            const auto lineVert = new LC_RefSnapConstructionLine(relZero,
                                                                 m_viewport->restrictVertical(relZero, relZero + rayDirectionOffset));
            preview->addEntity(lineVert);
            break;
        }
        default: {
            break;
        }
    }
}

void LC_VisualSnapManager::previewVisualSnapSolution(RS_EntityContainer* preview, VisualSnapSolution& solution) const {
    // add marks
    for (const auto& i : m_itemsList) {
        if (i->isVertexItem) {
            const auto p = i->vertex.get();
            if (p->flagHighlighted) {
                const auto mark = new LC_RefSnapMark(preview, p->wcsSnapCoordinate, m_options.vertexSizeHighlighted,
                                                     LC_RefSnapMark::HIGHLIGHTED);
                preview->addEntity(mark);
            }
            else {
                const auto mark = new LC_RefSnapMark(nullptr, p->wcsSnapCoordinate, m_options.vertexSizeNormal, LC_RefSnapMark::NORMAL);
                preview->addEntity(mark);
            }
        }
    }
    for (const auto& e : solution.guideEntities) {
        preview->addEntity(e.entity);
    }

    if (solution.restrictedPoint.valid) {
        preview->addEntity(new LC_RefSnapLine(nullptr, solution.foundSnapPoint, solution.restrictedPoint));
    }

    for (const auto& v : solution.snapCandidatesToShow) {
        const auto mark = new LC_RefSnapMark(nullptr, v, m_options.vertexSizeProjected, LC_RefSnapMark::PROJECTED);
        preview->addEntity(mark);
    }
}

void LC_VisualSnapManager::solveVisualSnap(const RS_Vector& wcsPos, VisualSnapSolution& solution) {
    // const double ucsSnapSize = m_viewport->toUcsDX(m_snapSizePx);
    solution.wcsPoint = wcsPos;
    createOrthoRaysForVertexes(wcsPos, solution);
    createLineRays(wcsPos, solution);
    std::vector<PointHolder> specialPointSnapCandidates;
    createTangentialAndNormalRaysFromArcEndpoints(wcsPos, solution);
    createTangentialRaysFromVertexesToArcs(wcsPos, specialPointSnapCandidates, solution);
    createTangentialRaysBetwenCirclesArcs(wcsPos, specialPointSnapCandidates, solution);
    createExplicitlySetDistanceCirclesForVertexes(wcsPos, solution);
    createVertexToVertexLinesAndDistances(wcsPos, specialPointSnapCandidates, solution);
    createGuideEntitiesByDocumentEntities(wcsPos, solution);
    createNormalsFromVertexToEntities(wcsPos, solution);
    findSnapPoint(wcsPos, solution, specialPointSnapCandidates);
}

void LC_VisualSnapManager::findSnapPoint(const RS_Vector& wcsPos, VisualSnapSolution& solution, const std::vector<PointHolder>& specialPointSnapCandidates) const {
    // finding closest intersection point
    RS2::LC_VisualSnapEntersectionPoint minSnapIntersectionInfo;

    solution.restrictedOriginalSnapType = RS2::SnapType::NO_SNAP;

    double minDist = m_wcsSnapRange;
    RS_Vector minSnap{false};
    for (auto& e : solution.guideEntities) {
        e.processed = true;
        for (const auto& ee : solution.guideEntities) {
            if (ee.processed) {
                continue;
            }
            RS_VectorSolutions intersections = RS_Information::getIntersection(e.entity, ee.entity, true);
            if (!intersections.empty()) {
                double dist;
                const RS_Vector closestSnap = intersections.getClosest(wcsPos, &dist);
                if (dist < minDist) {
                    minSnap = closestSnap;
                    minSnapIntersectionInfo.entity1 = e.snapEntityType;
                    minSnapIntersectionInfo.entity2 = ee.snapEntityType;

                    if (e.snapEntityType == RS2::VSNAP_LINE_VERTEX_ANGLE_STEP ||
                        e.snapEntityType == RS2::VSNAP_LINE_ENDPOINT_ANGLE_STEP) {
                        minSnapIntersectionInfo.rayAngle1 = e.wcsRayAngleRad;
                    }
                    if (ee.snapEntityType == RS2::VSNAP_LINE_VERTEX_ANGLE_STEP ||
                        ee.snapEntityType == RS2::VSNAP_LINE_ENDPOINT_ANGLE_STEP) {
                        minSnapIntersectionInfo.rayAngle2 = ee.wcsRayAngleRad;
                    }
                    // LC_ERR << "min Intersection " << closestSnap << " " << e.snapEntityType << " " << ee.snapEntityType;
                }
            }
        }
    }

    for (const auto& ph : specialPointSnapCandidates) {
        RS_Vector v = ph.pos;
        const double dist = v.distanceTo(wcsPos);
        if (dist < minDist) {
            minDist = dist;
            minSnap = v;
            minSnapIntersectionInfo.entity1 = RS2::VSNAP_NONE;
            minSnapIntersectionInfo.entity2 = ph.snapEntityType;
            // LC_ERR << "min Point " << v << " " << ph.snapEntityType;
        }
    }

    for (const auto& item : m_itemsList) {
        const auto vertex = item->getVertex();
        if (vertex != nullptr) {
            RS_Vector v = vertex->wcsSnapCoordinate;
            const double dist = v.distanceTo(wcsPos);
            if (dist < minDist) {
                minDist = dist;
                minSnap = v;
                minSnapIntersectionInfo.entity1 = RS2::VSNAP_NONE;
                minSnapIntersectionInfo.entity2 = RS2::VSNAP_NONE;
                const auto snapType = vertex->snapType;
                if (snapType != RS2::SnapType::VISUAL_SNAP) {
                    solution.restrictedOriginalSnapType = snapType;
                }
                // LC_ERR << "min Point " << v << " " << ph.snapEntityType;
            }
        }
    }

    if (minSnap.valid) {
        solution.addSnapCandidate(minSnap);
        // LC_ERR << "Snap found";
    }
    else {
        // LC_ERR << "No Snap";
    }

    solution.setFoundSnapPoint(minSnap, minSnapIntersectionInfo);
}

void LC_VisualSnapManager::createOrthoRaysForVertexes(const RS_Vector& wcsPos, VisualSnapSolution& solution) const {
    // horizontal/vertical rays for points, ensure that only one closest vertical and horisontal ray will be used
    const double minDistVert = RS_MAXDOUBLE;
    const double minDistHor = RS_MAXDOUBLE;
    RS_Entity* minLineHor{nullptr};
    RS_Entity* minLineVert{nullptr};
    const double angleStepRad = m_snapper->getAngleStep();
    double wcsRaysStartAngle = 0.0;
    double wcsRaysEndAngle = 0.0;
    double wcsMPI_2Angle = 0.0;
    if (m_options.createAngleStepRaysForVertexes) {
        wcsRaysStartAngle = m_viewport->toWorldAngle(angleStepRad);
        wcsRaysEndAngle = m_viewport->toWorldAngle(M_PI);
        wcsMPI_2Angle = m_viewport->toWorldAngle(M_PI_2);
    }

    for (const auto& item : m_itemsList) {
        const auto vertex = item->getVertex();
        if (vertex != nullptr) {
            vertex->flagProcessed = false;
            double dist;
            const double rayDirectionOffset = 10.0;
            auto vertexWCSPoint = vertex->wcsSnapCoordinate;

            // create horizontal ray
            auto horizontalPoint = m_viewport->restrictHorizontal(vertexWCSPoint,
                                                                  RS_Vector(vertexWCSPoint.x + rayDirectionOffset, vertexWCSPoint.y));
            const auto lineHor = tryGuideConstructionLine(wcsPos, vertexWCSPoint, horizontalPoint, dist);
            if (lineHor != nullptr) {
                if (dist < minDistHor) {
                    delete minLineHor;
                    minLineHor = lineHor;
                }
                else {
                    delete lineHor;
                }
            }

            // create vertical ray
            auto verticalPoint = m_viewport->restrictVertical(vertexWCSPoint,
                                                              RS_Vector(vertexWCSPoint.x, vertexWCSPoint.y + rayDirectionOffset));

            const auto lineVert = tryGuideConstructionLine(wcsPos, vertexWCSPoint, verticalPoint, dist);
            if (lineVert != nullptr) {
                if (dist < minDistVert) {
                    delete minLineVert;
                    minLineVert = lineVert;
                }
                else {
                    delete lineVert;
                }
            }

            // create angle snap rays, if needed. Only one with minimal distance to mouse will be used
            if (m_options.createAngleStepRaysForVertexes) {
                double minDistance = RS_MAXDOUBLE;
                double minDistanceAngle = RS_MAXDOUBLE;
                RS_Entity* minDistanceRay = nullptr;
                double angle = wcsRaysStartAngle;

                do {
                    if (LC_LineMath::isNotMeaningful(std::abs(angle - wcsMPI_2Angle))) {
                        angle = angle + angleStepRad;
                        continue;
                    }
                    RS_Vector secondPoint = vertexWCSPoint.relative(rayDirectionOffset, angle);
                    const auto lineRay = tryGuideConstructionLine(wcsPos, vertexWCSPoint, secondPoint, dist);
                    if (lineRay != nullptr) {
                        if (dist < minDistance) {
                            if (minDistanceRay != nullptr) {
                                delete minDistanceRay;
                            }
                            minDistanceRay = lineRay;
                            minDistanceAngle = angle;
                            minDistance = dist;
                        }
                        else {
                            delete lineRay;
                        }
                    }
                    angle = angle + angleStepRad;
                }
                while (angle < wcsRaysEndAngle);

                if (minDistanceRay != nullptr) {
                    solution.addGuideEntity(minDistanceRay, RS2::VisualSnapGuideEntityType::VSNAP_LINE_VERTEX_ANGLE_STEP, minDistanceAngle);
                }
            }
        }
    }
    if (minLineHor != nullptr) {
        solution.addGuideEntity(minLineHor, RS2::VisualSnapGuideEntityType::VSNAP_LINE_VERTEX_HORIZONTAL);
    }
    if (minLineVert != nullptr) {
        solution.addGuideEntity(minLineVert, RS2::VisualSnapGuideEntityType::VSNAP_LINE_VERTEX_VERTICAL);
    }
}


void LC_VisualSnapManager::createLineRays(const RS_Vector& wcsPos, VisualSnapSolution& solution) const {
    constexpr double rayOffset = 10;
    // lines extensions rays
    for (const auto& item : m_itemsList) {
        const auto vertex = item->getVertex();
        if (vertex != nullptr) {
            const auto refLine = vertex->refLine;
            if (refLine != nullptr) {
                const bool nonHorizontalOrVertical = isLineIsNotHorizontalOrVerticalInUCS(refLine->getStartpoint(), refLine->getEndpoint());
                if (nonHorizontalOrVertical) {
                    double dist;
                    auto nearestPointOnEntity = refLine->getNearestPointOnEntity(wcsPos, false, &dist);
                    const double wcsDistance = nearestPointOnEntity.distanceTo(wcsPos);
                    if (wcsDistance < m_wcsSnapRange) {
                        solution.addGuideEntity(refLine->clone(), RS2::VisualSnapGuideEntityType::VSNAP_LINE_RAY);
                    }
                    const double wcsLineAngle = refLine->getDirection1();
                    const double wcsNormalLineAngle = wcsLineAngle + M_PI_2;

                    const auto normalLine = tryGuideConstructionLine(wcsPos, vertex->wcsSnapCoordinate,
                                                                     vertex->wcsSnapCoordinate.relative(rayOffset, wcsNormalLineAngle), dist);
                    if (normalLine != nullptr) {
                        solution.addGuideEntity(normalLine, RS2::VisualSnapGuideEntityType::VSNAP_LINE_ENDPOINT_NORMAL);
                    }
                }
            }
        }
    }
}

// todo - potentially, this function may be used not only for line entities - but also for rays between vertexes, and for endpoints of arcs...
// todo - yet this may lead to graphical mess and may complicate snapping??
void LC_VisualSnapManager::createRelativeRays(const RS_Vector& wcsPos, const RS_Vector& startPoint, const RS_Vector &endPoint, VisualSnapSolution& solution) const {
    // create angle step rays
    double wcsLineAngleRad = startPoint.angleTo(endPoint);
    const double angleStepRad = m_snapper->getAngleStep();
    const double wcsRaysStartAngle = m_viewport->toWorldAngle(angleStepRad + wcsLineAngleRad);
    const double wcsRaysEndAngle = m_viewport->toWorldAngle(M_PI + wcsLineAngleRad);
    const double wcs0Angle = m_viewport->toWorldAngle(0.0);
    const double wcsMPI2Angle = wcs0Angle + M_PI_2;
    const double wcsMPIAngle = wcsMPI2Angle + M_PI_2;
    const double wcsM32PIAngle = wcsMPIAngle + M_PI_2;
    const double wcsNormalAngle = wcsLineAngleRad + M_PI_2;

    double wcsAngle = wcsRaysStartAngle;
    double minDistanceStartPoint = RS_MAXDOUBLE;
    double minDistanceAngleStartPoint = RS_MAXDOUBLE;
    RS_Entity* minDistanceRayStartPoint = nullptr;
    double minDistanceEndPoint = RS_MAXDOUBLE;
    double minDistanceAngleEndPoint = RS_MAXDOUBLE;
    RS_Entity* minDistanceRayEndPoint = nullptr;
    do {
        if (LC_LineMath::isSameAngle(wcsAngle, wcs0Angle) || LC_LineMath::isSameAngle(wcsAngle, wcsMPI2Angle) ||
            LC_LineMath::isSameAngle(wcsAngle, wcsMPIAngle) || LC_LineMath::isSameAngle(wcsAngle, wcsM32PIAngle) ||
            LC_LineMath::isSameAngle(wcsAngle, wcsNormalAngle) || LC_LineMath::isSameAngle(wcsAngle, wcsLineAngleRad)) {
            wcsAngle = wcsAngle + angleStepRad;
            continue;
        }
        constexpr double rayDirectionOffset = 10.0;
        double dist;
        RS_Vector secondPointStart = startPoint.relative(rayDirectionOffset, wcsAngle);
        const auto rayStart = tryGuideConstructionLine(wcsPos, startPoint, secondPointStart, dist);
        if (rayStart != nullptr) {
            if (dist < minDistanceStartPoint) {
                delete minDistanceRayStartPoint;
                minDistanceStartPoint = dist;
                minDistanceRayStartPoint = rayStart;
                minDistanceAngleStartPoint = wcsAngle;
            }
            else {
                delete rayStart;
            }
        }

        RS_Vector secondPointEnd = endPoint.relative(rayDirectionOffset, wcsAngle);
        const auto rayEnd = tryGuideConstructionLine(wcsPos, endPoint, secondPointEnd, dist);
        if (rayEnd != nullptr) {
             if (dist < minDistanceEndPoint) {
                 delete minDistanceRayEndPoint;
                 minDistanceEndPoint = dist;
                 minDistanceRayEndPoint = rayEnd;
                 minDistanceAngleEndPoint = wcsAngle;
             }
        }
        wcsAngle = wcsAngle + angleStepRad;
    }
    while (wcsAngle < wcsRaysEndAngle);

    if (minDistanceRayStartPoint != nullptr) {
        solution.addGuideEntity(minDistanceRayStartPoint, RS2::VisualSnapGuideEntityType::VSNAP_LINE_ENDPOINT_ANGLE_STEP,
                                minDistanceAngleStartPoint - wcsLineAngleRad);
    }
    if (minDistanceRayEndPoint != nullptr) {
        solution.addGuideEntity(minDistanceRayEndPoint, RS2::VisualSnapGuideEntityType::VSNAP_LINE_ENDPOINT_ANGLE_STEP,
                                minDistanceAngleEndPoint - wcsLineAngleRad);
    }
}

void LC_VisualSnapManager::createDistanceCircle2Points(const RS_Vector& wcsPos, const RS_Vector& centerPoint, const RS_Vector& circlePoint, VisualSnapSolution& solution) const {
    const double distanceBetweenVertexes = centerPoint.distanceTo(circlePoint);
    const double distanceFromFirstToMouse = centerPoint.distanceTo(wcsPos);
    const double distanceDelta = std::abs(distanceBetweenVertexes - distanceFromFirstToMouse);
    if (distanceDelta < m_wcsSnapRange) {
        const auto circle = new LC_RefSnapCircle(centerPoint, distanceBetweenVertexes);
        solution.addGuideEntity(circle, RS2::VisualSnapGuideEntityType::VSNAP_POINT_DISTANCE_VERTEX);
    }
}

void LC_VisualSnapManager::createVertexToVertexLinesAndDistances(const RS_Vector& wcsPos, std::vector<PointHolder>& specialPointSnapCandidates, VisualSnapSolution& solution) const {
    clearVertexProcessedFlag();
    // lines between marked points - and snap is on these lines
    for (const auto& item : m_itemsList) {
        const auto vertexFirst = item->getVertex();
        if (vertexFirst != nullptr) {
            vertexFirst->flagProcessed = true;
            for (const auto& itemInner : m_itemsList) {
                const auto vertexInner = itemInner->getVertex();
                if (vertexInner != nullptr) {
                    if (vertexInner->flagProcessed) {
                        continue;
                    }
                    double dist;
                    auto firstVertexSnap = vertexFirst->wcsSnapCoordinate;
                    auto innerVertexSnap = vertexInner->wcsSnapCoordinate;
                    // create vertex-vertex line
                    const auto line = tryCreateCloseSnapLine(wcsPos, m_wcsSnapRange, firstVertexSnap, innerVertexSnap, dist);
                    if (line != nullptr) {
                        solution.addGuideEntity(line, RS2::VisualSnapGuideEntityType::VSNAP_LINE_VERTEX_VERTEX);
                        int middlePoints = 2; // just default value for middle point (if no snap to middle)
                        const RS_SnapMode* snapMode = m_snapper->getSnapMode();
                        if (snapMode->snapMiddle) {
                            middlePoints = m_snapper->getSnapMiddlePoints();
                            // higlight possible snaps
                            const double lineLength = line->getLength();
                            const int segmentsCount = middlePoints + 1;
                            const double segmentLength = lineLength / segmentsCount;
                            const double lineAngle = firstVertexSnap.angleTo(innerVertexSnap);

                            for (int i = 1; i < segmentsCount; i++) {
                                const double distanceOnLine = segmentLength * i;
                                // if we're still within line, calculate snap point for tick on the line
                                RS_Vector snapPosition = firstVertexSnap.relative(distanceOnLine, lineAngle);
                                const double distance = snapPosition.distanceTo(wcsPos);
                                if (distance < m_wcsSnapRange) {
                                    specialPointSnapCandidates.push_back(
                                        PointHolder(snapPosition, RS2::VisualSnapGuideEntityType::VSNAP_POINT_MIDDLE));
                                }
                                solution.addSnapCandidate(snapPosition);
                            }
                        }
                        // add strict middle regardless snap to middle is enabled and if one is enabled, yet it is odd
                        const bool odd = middlePoints % 2 == 0;
                        if (odd) {
                            RS_Vector actualMiddle = line->getMiddlePoint();
                            if (dist < m_wcsSnapRange) {
                                specialPointSnapCandidates.push_back(PointHolder(actualMiddle, RS2::VisualSnapGuideEntityType::VSNAP_POINT_MIDDLE));
                            }
                            solution.addSnapCandidate(actualMiddle);
                        }
                    }
                    else {
                        delete line;
                    }

                    // create distance circles with radius equal to the distance between vertexes
                    if (m_options.createVertexVertexDistanceCircles) {
                        createDistanceCircle2Points(wcsPos, firstVertexSnap, innerVertexSnap, solution);
                    }
                }
            }
        }
    }
}

void LC_VisualSnapManager::createNormalsFromVertexToEntities(const RS_Vector& wcsPos, VisualSnapSolution& solution) const {    
    for (const auto& item : m_itemsList) {
        const auto vertex = item->getVertex();
        if (vertex != nullptr) {
            for (const auto& itemInner : m_itemsList) {
                const auto guideEntity = itemInner->getGuideDocEntity();
                if (guideEntity != nullptr) {
                    double dist;
                    auto nearestPointOnEntity = guideEntity->getNearestPointOnEntity(vertex->wcsSnapCoordinate, false, &dist);
                    const auto normalLine = tryGuideConstructionLine(wcsPos, vertex->wcsSnapCoordinate, nearestPointOnEntity, dist);
                    if (normalLine != nullptr) {
                        solution.addGuideEntity(normalLine, RS2::VisualSnapGuideEntityType::VSNAP_LINE_VERTEX_ORTHO);
                    }
                }
            }
        }
    }
}

LC_RefSnapConstructionLine* LC_VisualSnapManager::tryGuideConstructionLine(const RS_Vector& wcsPos, const RS_Vector& start, const RS_Vector& end, double& dist) const{
    return tryCreateCloseSnapConstructionLine(wcsPos, m_wcsSnapRange, start, end, dist);
}

void LC_VisualSnapManager::createTangentialAndNormalRaysFromArcEndpoints(const RS_Vector& wcsPos, VisualSnapSolution& solution) const {
    // arc - tangents to endpoints points
    for (const auto& item : m_itemsList) {
        const auto vertex = item->getVertex();
        if (vertex != nullptr) {
            if (vertex->refArc != nullptr) {
                constexpr double rayDirectionOffset = 10.0;
                // tangent 1
                const double direction1 = RS_Math::correctAngle(vertex->refArc->getDirection1() + M_PI);
                RS_Vector startPoint = vertex->refArc->getStartpoint();
                RS_Vector second = startPoint.relative(rayDirectionOffset, direction1);
                double dist;

                const bool nonHorizontalOrVerticalTangent1 = isLineIsNotHorizontalOrVerticalInUCS(startPoint, second);
                if (nonHorizontalOrVerticalTangent1) {
                    const auto line = tryGuideConstructionLine(wcsPos, startPoint, second, dist);
                    if (line != nullptr) {
                        solution.addGuideEntity(line, RS2::VisualSnapGuideEntityType::VSNAP_LINE_ENDPOINT_TANGENT);
                    }
                }

                // tangent 1
                const double direction2 = RS_Math::correctAngle(vertex->refArc->getDirection2() + M_PI);
                RS_Vector endPoint = vertex->refArc->getEndpoint();
                second = endPoint.relative(rayDirectionOffset, direction2);

                const bool nonHorizontalOrVerticalTangent2 = isLineIsNotHorizontalOrVerticalInUCS(endPoint, second);
                if (nonHorizontalOrVerticalTangent2) {
                    const auto line = tryGuideConstructionLine(wcsPos, endPoint, second, dist);
                    if (line != nullptr) {
                        solution.addGuideEntity(line, RS2::VisualSnapGuideEntityType::VSNAP_LINE_ENDPOINT_TANGENT);
                    }
                }

                // normal 1
                auto center = vertex->refArc->getCenter();
                const bool nonHorizontalOrVerticalNormal1 = isLineIsNotHorizontalOrVerticalInUCS(startPoint, center);
                if (nonHorizontalOrVerticalNormal1) {
                    const auto lineToCenter1 = tryGuideConstructionLine(wcsPos, startPoint, center, dist);
                    if (lineToCenter1 != nullptr) {
                        solution.addGuideEntity(lineToCenter1, RS2::VisualSnapGuideEntityType::VSNAP_LINE_ENDPOINT_NORMAL);
                    }
                }

                // normal 2
                const bool nonHorizontalOrVerticalNormal2 = isLineIsNotHorizontalOrVerticalInUCS(endPoint, center);
                if (nonHorizontalOrVerticalNormal2) {
                    const auto lineToCenter2 = tryGuideConstructionLine(wcsPos, endPoint, center, dist);
                    if (lineToCenter2 != nullptr) {
                        solution.addGuideEntity(lineToCenter2, RS2::VisualSnapGuideEntityType::VSNAP_LINE_ENDPOINT_NORMAL);
                    }
                }
            }
        }
    }
}

void LC_VisualSnapManager::createGuideEntitiesByDocumentEntities(const RS_Vector& wcsPos, VisualSnapSolution& solution) const {
    for (const auto& item : m_itemsList) {
        if (!item->isVertexItem) {
            double dist;
            if (item->entityRef->guideEntity != nullptr) {
                const auto entity = item->entityRef->guideEntity.get();
                RS_Vector v = entity->getNearestPointOnEntity(wcsPos, true, &dist);
                if (dist < m_wcsSnapRange) {
                    solution.addGuideEntity(entity->clone(), RS2::VisualSnapGuideEntityType::VSNAP_DOC_ENTITY);
                }
                if (item->entityRef->documentEntity->rtti() == RS2::EntityLine) {
                    const auto& endpoint = entity->getEndpoint();
                    const auto& startPoint = entity->getStartpoint();
                    if (m_options.createAngleStepRaysForEntitiesEndpoints) {
                        createRelativeRays(wcsPos, startPoint, endpoint, solution);
                    }
                    if (m_options.createVertexVertexDistanceCircles) {
                        createDistanceCircle2Points(wcsPos, startPoint, endpoint, solution);
                        createDistanceCircle2Points(wcsPos, endpoint, startPoint, solution);
                    }
                }
            }
        }
    }
}

void LC_VisualSnapManager::clearVertexProcessedFlag() const {
    for (const auto& item : m_itemsList) {
        const auto vertex = item->getVertex();
        if (vertex != nullptr) {
            item->vertex->flagProcessed = false;
        }
    }
}

void LC_VisualSnapManager::createTangentialRaysFromVertexesToArcs(const RS_Vector& wcsPos, std::vector<PointHolder>& specialPointSnapCandidates, VisualSnapSolution& solution) const {
    clearVertexProcessedFlag();
    // tangent line from vertexes to arcs circles
    for (const auto& item : m_itemsList) {
        const RS_Entity* entity = nullptr;
        const auto vertex = item->getVertex();
        if (vertex != nullptr) {
            if (vertex->refArc != nullptr) {
                entity = vertex->refArc;
            }
            else if (vertex->refCircle != nullptr) {
                entity = vertex->refCircle;
            }
            else {
                continue;
            }
        }
        else {
            const auto ent = item->getGuideDocEntity();
            const RS2::EntityType type = ent->rtti();
            if (type == RS2::EntitySnapArc || type == RS2::EntitySnapCircle) {
                entity = ent;
            }
        }

        for (const auto& itemInner : m_itemsList) {
            if (itemInner == item) {
                continue;
            }

            const auto vertexInner = itemInner->getVertex();
            if (vertexInner != nullptr) {
                RS_Vector tangentPoint;
                RS_Vector altTangentPoint;
                auto vertexSnapCoord = vertexInner->wcsSnapCoordinate;
                const auto* tangentLine = RS_Creation::createTangent1(vertexSnapCoord, vertexSnapCoord, entity, tangentPoint,
                                                                      altTangentPoint);
                if (tangentLine != nullptr) {
                    double dist;
                    auto line = tryGuideConstructionLine(wcsPos, vertexSnapCoord, tangentPoint, dist);
                    if (line != nullptr) {
                        solution.addGuideEntity(line, RS2::VisualSnapGuideEntityType::VSNAP_LINE_TANGENT1);
                        specialPointSnapCandidates.
                            push_back(PointHolder(tangentPoint, RS2::VisualSnapGuideEntityType::VSNAP_LINE_TANGENT1));
                        solution.addSnapCandidate(tangentPoint);
                    }
                    if (altTangentPoint.valid) {
                        line = tryGuideConstructionLine(wcsPos, vertexSnapCoord, altTangentPoint, dist);
                        if (line != nullptr) {
                            specialPointSnapCandidates.push_back(PointHolder(altTangentPoint,
                                                                             RS2::VisualSnapGuideEntityType::VSNAP_LINE_TANGENT1));
                            solution.addGuideEntity(line, RS2::VisualSnapGuideEntityType::VSNAP_LINE_TANGENT1);
                            solution.addSnapCandidate(altTangentPoint);
                        }
                    }
                    delete tangentLine;
                }
            }
        }
    }
}

void LC_VisualSnapManager::createTangentialRaysBetwenCirclesArcs(const RS_Vector& wcsPos, std::vector<PointHolder>& specialPointSnapCandidates, VisualSnapSolution& solution) const {
    clearVertexProcessedFlag();
    // mutual tangents (arc-arc, arc-circle etc)
    for (const auto& itemFirst : m_itemsList) {
        const RS_Entity* entityFirst = nullptr;
        const auto vFirst = itemFirst->getVertex();
        if (vFirst != nullptr) {
            if (vFirst->refArc != nullptr) {
                entityFirst = vFirst->refArc;
            }
            else if (vFirst->refCircle != nullptr) {
                entityFirst = vFirst->refCircle;
            }
            else {
                continue;
            }
            vFirst->flagProcessed = true;
        }
        else {
            const auto ent = itemFirst->getGuideDocEntity();
            const RS2::EntityType type = ent->rtti();
            if (type == RS2::EntitySnapArc || type == RS2::EntitySnapCircle) {
                entityFirst = ent;
            }
            else {
                continue;
            }
        }
        for (const auto& itemSecond : m_itemsList) {
            const auto vSecond = itemSecond->getVertex();
            const RS_Entity* entitySecond = nullptr;
            if (vSecond != nullptr) {
                if (vSecond->flagProcessed) {
                    continue;
                }
                if (vSecond->refArc != nullptr) {
                    entitySecond = vSecond->refArc;
                }
                else if (vSecond->refCircle != nullptr) {
                    entitySecond = vSecond->refCircle;
                }
            }
            else {
                const auto ent = itemSecond->getGuideDocEntity();
                if (ent == entityFirst) {
                    continue;
                }
                const RS2::EntityType type = ent->rtti();
                if (type == RS2::EntitySnapArc || type == RS2::EntitySnapCircle) {
                    entitySecond = ent;
                }
                else {
                    continue;
                }
            }
            std::vector<std::unique_ptr<RS_Line>> tangentLines = RS_Creation::createTangent2(entityFirst, entitySecond);
            if (!tangentLines.empty()) {
                for (const auto& l : tangentLines) {
                    double dist;
                    auto start = l->getStartpoint();
                    auto end = l->getEndpoint();
                    const auto line = tryGuideConstructionLine(wcsPos, start, end, dist);
                    if (line != nullptr) {
                        solution.addGuideEntity(line, RS2::VisualSnapGuideEntityType::VSNAP_LINE_TANGENT2);
                        solution.addSnapCandidate(start); // snap middle
                        solution.addSnapCandidate(end);
                        specialPointSnapCandidates.push_back(PointHolder(start, RS2::VisualSnapGuideEntityType::VSNAP_LINE_TANGENT2));
                        specialPointSnapCandidates.push_back(PointHolder(end, RS2::VisualSnapGuideEntityType::VSNAP_LINE_TANGENT2));
                    }
                }
            }
        }
    }
}

void LC_VisualSnapManager::createExplicitlySetDistanceCirclesForVertexes(const RS_Vector& wcsPos, VisualSnapSolution& solution) const {
    // distances - radius from vertexes
    const RS_SnapMode* snap = m_snapper->getSnapMode();
    const bool useDistance = snap->snapDistance;
    if (useDistance) {
        const double distanceWCS = m_snapper->getSnapDistance();
        for (const auto& item : m_itemsList) {
            const auto vertex = item->getVertex();
            if (vertex != nullptr) {
                auto wcsSnapCoordinate = vertex->wcsSnapCoordinate;
                const double distance = wcsSnapCoordinate.distanceTo(wcsPos);
                const double distanceDelta = std::abs(distance - distanceWCS);
                if (distanceDelta < m_wcsSnapRange) {
                    const auto circle = new LC_RefSnapCircle(wcsSnapCoordinate, distanceWCS);
                    solution.addGuideEntity(circle, RS2::VisualSnapGuideEntityType::VSNAP_POINT_DISTANCE_EXPLICIT);
                }
            }
        }
    }
}

void LC_VisualSnapManager::previewVertex(RS_EntityContainer* preview, const LC_VisualSnapVertex* point, bool remove) const {
    if (remove) {
        auto entities = preview->getEntityList();
        RS_Entity* entityToRemove = nullptr;
        for (const auto e : entities) {
            if (e->rtti() == RS2::EntitySnapMark) {
                const auto mark = static_cast<LC_RefSnapMark*>(e);
                if (LC_LineMath::isNotMeaningfulDistance(point->wcsSnapCoordinate, mark->getPos())) {
                    entityToRemove = mark;
                    break;
                }
            }
        }
        if (entityToRemove != nullptr) {
            preview->removeEntity(entityToRemove);
        }

        const auto mark = new LC_RefSnapMark(nullptr, point->wcsSnapCoordinate, m_options.vertexSizeHighlighted,
                                             LC_RefSnapMark::HIGHLIGHTED_REMOVED);
        preview->addEntity(mark);
    }
    else {
        const auto mark = new LC_RefSnapMark(nullptr, point->wcsSnapCoordinate, m_options.vertexSizeHighlighted,
                                             LC_RefSnapMark::HIGHLIGHTED);
        preview->addEntity(mark);
    }
}


bool LC_VisualSnapManager::hasVertexesOrEntities() {
    m_mutex.lock();
    const bool result = !m_itemsList.empty();
    m_mutex.unlock();
    return result;
}

void LC_VisualSnapManager::tryProcessVertexDelayed(const RS2::SnapType snapType, const RS_Vector& wcsSnapPoint,
                                                   const RS_Vector& wcsGraphPoint, RS_Entity* entity1, [[maybe_unused]]RS_Entity* entity2) {
    if (snapType == RS2::ENDPOINT || snapType == RS2::ENTITY || snapType == RS2::MIDDLE || snapType == RS2::INTERSECTION || snapType ==
        RS2::CENTER || snapType == RS2::DISTANCE || snapType == RS2::VISUAL_SNAP) {
        const double dist = wcsSnapPoint.distanceTo(wcsGraphPoint);
        if (dist < m_wcsSnapRange) {
            addVertexDelayed(snapType, wcsSnapPoint, entity1); // fixme complete and support second entity for intersection?
        }
    }
}

void LC_VisualSnapManager::addVertexDelayed(const RS2::SnapType snapType, const RS_Vector& coord, RS_Entity* entity) {
    doSkipDelayedAdditions();
    m_vertexToProcess = createVisualSnapVertex(snapType, coord, entity);
    int interval;
    if (snapType == RS2::VISUAL_SNAP) {
        interval = getVisualSnapVertexAddingDelay();
    }
    else {
        interval = getSnapVertexAddingDelay();
    }
    m_timer.setInterval(interval);
    m_timer.start();
}

void LC_VisualSnapManager::processEntityDelayed(RS_Entity* entity) {
    doSkipDelayedAdditions();
    m_documentEntityToProcess = entity;
    const int interval = getDocumentEntityAddingDelay();
    m_timer.setInterval(interval);
    m_timer.start();
}

void LC_VisualSnapManager::clear() {
    m_mutex.lock();
    doSkipDelayedAdditions();
    for (const auto& p : m_itemsList) {
        if (p->isVertexItem) {
            // nothing to clear
        }
        else {
            p->entityRef->documentEntity->delFlag(RS2::FlagInVisualSnap);
        }
    }
    m_itemsList.clear();
    m_solution.reset(nullptr);
    m_snapper->onVisualSnapEntityRegistered(nullptr);
    m_mutex.unlock();
}

void LC_VisualSnapManager::invalidateSolution() {
    if (m_solution != nullptr) {
        m_solution->valid = false;
    }
}

void LC_VisualSnapManager::registerVertex() {
    registerVertex(m_vertexToProcess);
    m_vertexToProcess = nullptr;
}

void LC_VisualSnapManager::registerVertex(LC_VisualSnapVertex* vertex,bool removeExistingInSamePosition) {
    // check if vertex exists - remove it if it is so
    for (const auto& item : m_itemsList) {
        const auto v = item->getVertex();
        if (v != nullptr) {
            // double distance = p->wcsSnapCoordinate.distanceTo(vertex->wcsSnapCoordinate);
            // if (distance < m_wcsSnapRange) {
            if (LC_LineMath::isNotMeaningfulDistance(vertex->wcsSnapCoordinate, v->wcsSnapCoordinate)) {
                // remove existing if this is requested
                if (removeExistingInSamePosition) {
                    m_snapper->onVisualSnapPointRegistered(vertex, true);
                    // todo - remove by erase_if in C++ 20
                    auto itr = find_if(m_itemsList.begin(), m_itemsList.end(), [v](std::unique_ptr<VisualSnapItem>& it) {
                         return it->getVertex() == v;
                    });
                    if (itr != m_itemsList.end()) {
                        itr = m_itemsList.erase(itr);
                    }

                    invalidateSolution();
                }
                delete vertex;
                return;
            }
        }
    }
    // normal adding, vertex does not exists
    storeVertexRef(vertex);
    m_snapper->onVisualSnapPointRegistered(vertex, false);
    m_solution.reset(nullptr);
}

void LC_VisualSnapManager::registerDocumentEntity() {
    if (isNotInVisualSnap(m_documentEntityToProcess)) { // adding entity to visual snap
        // LC_ERR << "++++++++++++++++++++++++++++++++++++++++++++++++++++++++ registering entity " << m_entityToAdd->getId();
        const RS2::EntityType type = m_documentEntityToProcess->rtti();
        switch (type) {
            case RS2::EntityLine: {
                const auto entity = static_cast<RS_Line*>(m_documentEntityToProcess);
                const auto start = entity->getStartpoint();
                const auto end = entity->getEndpoint();
                entity->setFlag(RS2::FlagInVisualSnap);
                const auto snapEntity = new LC_RefSnapLine(nullptr, start, end);
                storeEntityRef(snapEntity, entity);
                registerEntityEndpoints(entity);
                m_solution.reset(nullptr);
                break;
            }
            case RS2::EntityCircle: {
                const auto entity = static_cast<RS_Circle*>(m_documentEntityToProcess);
                entity->setFlag(RS2::FlagInVisualSnap);
                const auto snapEntity = new LC_RefSnapCircle(entity->getCenter(), entity->getRadius());
                storeEntityRef(snapEntity, entity);
                m_solution.reset(nullptr);
                break;
            }
            case RS2::EntityArc: {
                const auto entity = static_cast<RS_Arc*>(m_documentEntityToProcess);
                entity->setFlag(RS2::FlagInVisualSnap);
                const auto snapEntity = new LC_RefSnapCircle(entity->getCenter(), entity->getRadius());
                storeEntityRef(snapEntity, entity);
                registerEntityEndpoints(entity);
                m_solution.reset(nullptr);
                break;
            }
            default:
                // note - so far the list of suported entities is limited by exsitence of their LC_RefSnapXX counterparts (they are
                // used for finding intesections as part of snap resolving, as well as for highlighting visual snap entities on preview.. .
                // yet, potentially? any entity may be added there, if more sophisticated intesection check algorytm will be used...
                break;
        }
    }
    else { // remove existing entity
        m_documentEntityToProcess->delFlag(RS2::FlagInVisualSnap);
        // todo - remove by erase_if in C++ 20
        auto itr = find_if(m_itemsList.begin(), m_itemsList.end(),
                           [this](std::unique_ptr<VisualSnapItem>& it) {
                               if (it->isVertexItem) {
                                   return false;
                               }
                               return it->entityRef->documentEntity == m_documentEntityToProcess;
                           });
        if (itr != m_itemsList.end()) {
            itr = m_itemsList.erase(itr);
        }
        m_solution.reset(nullptr);
        m_snapper->onVisualSnapEntityRegistered(nullptr);
    }
    m_documentEntityToProcess = nullptr;
}

void LC_VisualSnapManager::registerEntityEndpoints(RS_Entity* const entity) {
    const auto startVertex = createVisualSnapVertex(RS2::ENDPOINT, entity->getStartpoint(), entity);
    const auto endVertex = createVisualSnapVertex(RS2::ENDPOINT, entity->getEndpoint(), entity);
    registerVertex(startVertex);
    registerVertex(endVertex);
}

bool LC_VisualSnapManager::isNotInVisualSnap(RS_Entity* e) const {
    const bool notPresent = !e->getFlag(RS2::FlagInVisualSnap);
    return notPresent;
}

void LC_VisualSnapManager::storeEntityRef(RS_Entity* const snapEntity, RS_Entity* documentEntity) {
    m_itemsList.push_back(std::make_unique<VisualSnapItem>(snapEntity, documentEntity));
    invalidateSolution();
    m_snapper->onVisualSnapEntityRegistered(snapEntity->clone());
}

void LC_VisualSnapManager::storeVertexRef(LC_VisualSnapVertex* const vertex) {
    m_itemsList.push_back(std::make_unique<VisualSnapItem>(vertex));
    invalidateSolution();
}

void LC_VisualSnapManager::performDelayedOperation() {
    m_mutex.lock();
    if (m_vertexToProcess != nullptr) {
        registerVertex();
    }
    if (m_documentEntityToProcess != nullptr) {
        registerDocumentEntity();
    }
    m_mutex.unlock();
}

void LC_VisualSnapManager::doSkipDelayedAdditions() {
    if (m_vertexToProcess != nullptr || m_documentEntityToProcess != nullptr) {
        m_timer.stop();
        delete m_vertexToProcess;
        m_documentEntityToProcess = nullptr;
        m_vertexToProcess = nullptr;
    }
}

void LC_VisualSnapManager::skipDelayedOperations() {
    m_mutex.lock();
    doSkipDelayedAdditions();
    m_mutex.unlock();
}

void LC_VisualSnapManager::addGuidesForBasePoint(const RS_Vector& snapPoint, [[maybe_unused]]const RS_Vector& graphPoint, const RS_Vector& relZero) {
    m_mutex.lock();

    RS_Vector basePoint;
    RS_Vector currentPoint;

    if (m_lastSnappedBasePoint.valid) {
        basePoint = m_lastSnappedBasePoint;
    }
    else {
        basePoint = relZero;
    }

    currentPoint = snapPoint;

    const auto lastSnapVertex = new LC_VisualSnapVertex(basePoint, currentPoint);
    registerVertex(lastSnapVertex, false);
    const auto currentPointVertex = new LC_VisualSnapVertex(currentPoint,basePoint);
    registerVertex(currentPointVertex, false);
    m_mutex.unlock();
}

void LC_VisualSnapManager::saveLastSnappedPoint(const RS_Vector& v) {
    m_lastSnappedBasePoint = v;
}

void LC_VisualSnapManager::addSnappedPointAsVertex(const RS_Vector& v, const RS2::SnapType type, RS_Entity* entity, const bool clearOther) {
    if (clearOther) {
        clear();
    }
    m_mutex.lock();
    const auto vertex = createVisualSnapVertex(type, v, entity);
    storeVertexRef(vertex);
    saveLastSnappedPoint(v);
    m_mutex.unlock();
}

void LC_VisualSnapManager::removeLastAddition() {
    m_mutex.lock();
    if (!m_itemsList.empty()) {
        const auto &item = m_itemsList.back();
        if (item->isVertexItem) {
        }
        else {
            item->entityRef->documentEntity->delFlag(RS2::FlagInVisualSnap);
        }
        m_itemsList.remove(item);
        invalidateSolution();
    }
    m_mutex.unlock();
}

void LC_VisualSnapManager::setSnapRange(double range) {
    double graphSnapRange = m_snapper->toGraphDX(m_options.guidingEntitiesSnapDistance);
    double snapRangeToUse = std::min(graphSnapRange, range);
    m_wcsSnapRange = snapRangeToUse;
}

LC_VisualSnapVertex* LC_VisualSnapManager::createVisualSnapVertex(const RS2::SnapType snap, const RS_Vector& coord, RS_Entity* entity) {
    return new LC_VisualSnapVertex(snap, coord, entity);
}

int LC_VisualSnapManager::getVisualSnapVertexAddingDelay() const {
    return m_options.delayMsProjectedSnap;
}

int LC_VisualSnapManager::getSnapVertexAddingDelay() const {
    return m_options.delayMsSnapVertex;
}

int LC_VisualSnapManager::getDocumentEntityAddingDelay() const {
    return m_options.delayMsDocumentEntity;
}

bool LC_VisualSnapManager::isLineIsNotHorizontalOrVerticalInUCS(RS_Vector startPoint, RS_Vector endPoint) const {
    const RS_Vector ucsStart = m_viewport->toUCS(startPoint);
    const RS_Vector ucsEnd = m_viewport->toUCS(endPoint);
    const RS_Vector ucsDelta = ucsStart - ucsEnd;

    const bool horizontal = LC_LineMath::isNotMeaningful(ucsDelta.y);
    const bool vertical = LC_LineMath::isNotMeaningful(ucsDelta.x);
    return !horizontal && !vertical;
}
