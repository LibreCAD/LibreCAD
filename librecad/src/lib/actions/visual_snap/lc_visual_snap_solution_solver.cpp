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

#include "lc_visual_snap_solution_solver.h"
#include "lc_ref_snap_circle.h"
#include "lc_ref_snap_construction_line.h"
#include "lc_ref_snap_entity.h"
#include "lc_ref_snap_line.h"
#include "lc_visual_snap_data.h"
#include "lc_visual_snap_options.h"
#include "lc_visual_snap_solution.h"
#include "rs.h"
#include "rs_creation.h"
#include "rs_information.h"
#include "rs_vector.h"
#include "rs_snapper.h"

namespace {
    constexpr double g_rayDirectionOffset = 10.0;

    void clearVertexProcessedFlag(LC_VisualSnapSolution& solution) {
        solution.snapData->forEachVertex([](LC_VisualSnapVertex* v) -> void {
            v->flagProcessed = false;
        });
    }

    void clearDocumentProcessedFlag(LC_VisualSnapSolution& solution) {
        solution.snapData->forEachDocRef([](LC_VisualSnapDocumentEntityRef* v) -> void {
            v->processed = false;
        });
    }
}

void LC_VisualSnapSolutionSolver::solveVisualSnap(const RS_Vector& wcsPos, LC_VisualSnapSolution& solution) {
    solution.wcsPoint = wcsPos;
    addOrthoRaysForVertexes(wcsPos, solution);
    addLineRays(wcsPos, solution);
    std::vector<LC_VisualSnapPointHolder> specialPointSnapCandidates;
    addTangentialAndNormalRaysFromArcEndpoint(wcsPos, solution);
    addTangentialRaysFromVertexesToArcs(wcsPos, specialPointSnapCandidates, solution);
    addTangentialRaysBetwenCirclesArcs(wcsPos, specialPointSnapCandidates, solution);
    addExplicitlySetDistanceCirclesForVertexes(wcsPos, solution);
    addVertexToVertexLinesAndDistances(wcsPos, specialPointSnapCandidates, solution);
    addGuideEntitiesByDocumentEntities(wcsPos, solution);
    addNormalsFromVertexToEntities(wcsPos, solution);
    addRelativePositionsEntities(wcsPos, solution);
    addOrdinaryRestrictionLines(wcsPos, solution);
    findSnapPoint(wcsPos, solution, specialPointSnapCandidates);
}

void LC_VisualSnapSolutionSolver::findSnapPoint(const RS_Vector& wcsPos, LC_VisualSnapSolution& solution,
                                                const std::vector<LC_VisualSnapPointHolder>& specialPointSnapCandidates) const {
    // finding closest intersection point
    RS2::LC_VisualSnapIntersectionInfo minSnapIntersectionInfo;

    solution.restrictedOriginalSnapType = RS2::SnapType::NO_SNAP;

    double minDist = m_wcsSnapRange;
    RS_Vector minSnap{false};
    for (auto& e : solution.guidingEntities) {
        e.processed = true;
        for (const auto& ee : solution.guidingEntities) {
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

                    if (e.snapEntityType == RS2::VSNAP_LINE_VERTEX_ANGLE_STEP || e.snapEntityType == RS2::VSNAP_LINE_ENDPOINT_ANGLE_STEP) {
                        minSnapIntersectionInfo.rayAngle1 = e.wcsRayAngleRad;
                    }
                    if (ee.snapEntityType == RS2::VSNAP_LINE_VERTEX_ANGLE_STEP || ee.snapEntityType ==
                        RS2::VSNAP_LINE_ENDPOINT_ANGLE_STEP) {
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

    solution.snapData->forEachVertex([&minDist, &minSnap, &minSnapIntersectionInfo, wcsPos, &solution](LC_VisualSnapVertex* vertex) {
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
    });

    if (minSnap.valid) {
        solution.addSnapCandidate(minSnap);
        // LC_ERR << "Snap found";
    }
    else {
        // LC_ERR << "No Snap";
    }

    solution.setFoundSnapPoint(minSnap, minSnapIntersectionInfo);
}

void LC_VisualSnapSolutionSolver::addOrthoAndAngleRaysForPoint(const RS_Vector& wcsPos, LC_VisualSnapSolution& solution,
                                                               const double angleStepRad, double wcsRaysStartAngle, double wcsRaysEndAngle,
                                                               double wcsMPI_2Angle, RS_Vector vertexWCSPoint) const {
    double dist;
    if (hasNoLinesForPoint(solution, vertexWCSPoint, RS2::VisualSnapGuideEntityType::VSNAP_LINE_VERTEX_HORIZONTAL)) {
        // create horizontal ray
        auto horizontalPoint = m_viewport->restrictHorizontal(vertexWCSPoint,
                                                              RS_Vector(vertexWCSPoint.x + g_rayDirectionOffset, vertexWCSPoint.y));
        const auto lineHor = tryCreateGuidingConstructionLine(wcsPos, vertexWCSPoint, horizontalPoint, dist,
                                                              RS2::VisualSnapGuideEntityType::VSNAP_LINE_VERTEX_HORIZONTAL);
        if (lineHor != nullptr) {
            solution.addGuidingEntity(lineHor);
        }
    }

    if (hasNoLinesForPoint(solution, vertexWCSPoint, RS2::VisualSnapGuideEntityType::VSNAP_LINE_VERTEX_VERTICAL)) {
        // create vertical ray
        auto verticalPoint = m_viewport->restrictVertical(vertexWCSPoint,
                                                          RS_Vector(vertexWCSPoint.x, vertexWCSPoint.y + g_rayDirectionOffset));

        const auto lineVert = tryCreateGuidingConstructionLine(wcsPos, vertexWCSPoint, verticalPoint, dist,
                                                               RS2::VisualSnapGuideEntityType::VSNAP_LINE_VERTEX_VERTICAL);

        if (lineVert != nullptr) {
            solution.addGuidingEntity(lineVert);
        }
    }

    // create angle snap rays, if needed. Only ones with minimal distance to mouse will be used
    if (m_options->createAngleStepRaysForVertexes) {
        double minDistance = m_wcsSnapRange;
        double minDistanceAngle = RS_MAXDOUBLE;
        LC_RefSnapConstructionLine* minDistanceRay = nullptr;
        double angle = wcsRaysStartAngle;

        do {
            if (LC_LineMath::isNotMeaningful(std::abs(angle - wcsMPI_2Angle))) {
                angle = angle + angleStepRad;
                continue;
            }
            RS_Vector secondPoint = vertexWCSPoint.relative(g_rayDirectionOffset, angle);
            const auto lineRay = tryCreateGuidingConstructionLine(wcsPos, vertexWCSPoint, secondPoint, dist, angle,
                                                                  RS2::VisualSnapGuideEntityType::VSNAP_LINE_VERTEX_ANGLE_STEP);
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
            solution.addGuidingEntity(minDistanceRay, minDistanceAngle);
        }
    }
}

void LC_VisualSnapSolutionSolver::addOrthoRaysForVertexes(const RS_Vector& wcsPos, LC_VisualSnapSolution& solution) const {
    // horizontal/vertical rays for points
    const double angleStepRad = m_snapper->getAngleStep();
    double wcsRaysStartAngle = 0.0;
    double wcsRaysEndAngle = 0.0;
    double wcsMPI_2Angle = 0.0;
    if (m_options->createAngleStepRaysForVertexes) {
        wcsRaysStartAngle = m_viewport->toWorldAngle(angleStepRad);
        wcsRaysEndAngle = m_viewport->toWorldAngle(M_PI);
        wcsMPI_2Angle = m_viewport->toWorldAngle(M_PI_2);
    }
    solution.snapData->forEachVertex(
        [this, wcsPos, &solution, angleStepRad, wcsRaysStartAngle, wcsRaysEndAngle, wcsMPI_2Angle](LC_VisualSnapVertex* vertex) {
            vertex->flagProcessed = false;
            auto vertexWCSPoint = vertex->wcsSnapCoordinate;
            addOrthoAndAngleRaysForPoint(wcsPos, solution, angleStepRad, wcsRaysStartAngle, wcsRaysEndAngle, wcsMPI_2Angle, vertexWCSPoint);
        });
}

void LC_VisualSnapSolutionSolver::addLineRayAndNormal(const RS_Vector& wcsPos, LC_VisualSnapSolution& solution,
                                                      const RS_Vector& wcsSnapCoordinate, const LC_RefSnapConstructionLine* refLine) const {
    double dist;
    // create ray that is in line direction
    auto endPoint = refLine->getEndpoint();
    auto startPoint = refLine->getStartpoint();

    const bool createLineRay = hasNoLinesForPoints(solution, endPoint, startPoint);
    if (createLineRay) {
        auto nearestPointOnEntity = refLine->getNearestPointOnEntity(wcsPos, false, &dist);
        const double wcsDistance = nearestPointOnEntity.distanceTo(wcsPos);
        bool withinSnapRange = wcsDistance < m_wcsSnapRange;
        if (withinSnapRange || m_options->showNotSnappableGuides) {
            const auto clone = static_cast<LC_RefSnapConstructionLine*>(refLine->clone());
            const double refLineAngle = startPoint.angleTo(endPoint);
            clone->setupForPoint(nearestPointOnEntity, RS2::VisualSnapGuideEntityType::VSNAP_LINE_RAY, withinSnapRange, refLineAngle);
            solution.addGuidingEntity(clone);
        }
    }

    // create ray that is normal to line (under 90 degrees)
    const double wcsLineAngle = refLine->getDirection1();
    const double wcsNormalLineAngle = wcsLineAngle + M_PI_2;

    RS_Vector secondPoint = wcsSnapCoordinate.relative(g_rayDirectionOffset, wcsNormalLineAngle);
    const auto normalLine = tryCreateGuidingConstructionLine(wcsPos, wcsSnapCoordinate, secondPoint, dist, wcsNormalLineAngle,
                                                             RS2::VisualSnapGuideEntityType::VSNAP_LINE_ENDPOINT_NORMAL);
    if (normalLine != nullptr) {
        solution.addGuidingEntity(normalLine);
    }
}

void LC_VisualSnapSolutionSolver::addLineRays(const RS_Vector& wcsPos, LC_VisualSnapSolution& solution) const {
    // lines extensions rays
    solution.snapData->forEachVertex([this, &wcsPos, &solution](LC_VisualSnapVertex* vertex) {
        const auto refLine = vertex->refLine;
        if (refLine != nullptr) {
            addLineRayAndNormal(wcsPos, solution, vertex->wcsSnapCoordinate, refLine);
        }
    });
}

void LC_VisualSnapSolutionSolver::addTangentialAndNormlRaysForArcEndpoint(const RS_Vector& wcsPos, LC_VisualSnapSolution& solution,
                                                                          const RS_Vector& wcsSnapCoordinate, LC_RefSnapArc* arc) const {
    RS_Vector startPoint = arc->getStartpoint();
    RS_Vector endPoint = arc->getEndpoint();
    RS_Vector endpointToUse;
    double direction;
    if (LC_LineMath::isNotMeaningfulDistance(wcsSnapCoordinate, startPoint)) {
        double angle1 = arc->getAngle1();
        direction = angle1 + M_PI_2;
        endpointToUse = startPoint;
    }
    else if (LC_LineMath::isNotMeaningfulDistance(wcsSnapCoordinate, endPoint)) {
        double angle2 = arc->getAngle2();
        direction = angle2 + M_PI_2;
        endpointToUse = endPoint;
    }
    else {
        return;
    }
    // tangent
    RS_Vector second = endpointToUse.relative(g_rayDirectionOffset, direction);
    double dist;

    const auto tangent = tryCreateGuidingConstructionLine(wcsPos, endpointToUse, second, dist, direction,
                                                          RS2::VisualSnapGuideEntityType::VSNAP_LINE_ENDPOINT_TANGENT);
    if (tangent != nullptr) {
        solution.addGuidingEntity(tangent);
    }

    // normal
    auto center = arc->getCenter();
    const auto lineToCenter = tryCreateGuidingConstructionLine(wcsPos, endpointToUse, center, dist,
                                                               RS2::VisualSnapGuideEntityType::VSNAP_LINE_ENDPOINT_NORMAL);
    if (lineToCenter != nullptr) {
        solution.addGuidingEntity(lineToCenter);
    }
}

void LC_VisualSnapSolutionSolver::addTangentialAndNormalRaysFromArcEndpoint(const RS_Vector& wcsPos,
                                                                            LC_VisualSnapSolution& solution) const {
    // arc - tangents to endpoints points
    solution.snapData->forEachVertex([this, wcsPos, &solution](LC_VisualSnapVertex* vertex) {
        if (vertex->refArc != nullptr) {
            LC_RefSnapArc* arc = vertex->refArc;
            auto wcsSnapCoordinate = vertex->wcsSnapCoordinate;
            addTangentialAndNormlRaysForArcEndpoint(wcsPos, solution, wcsSnapCoordinate, arc);
        }
    });
}

void LC_VisualSnapSolutionSolver::addTangentialRayFromVertexToEntity(const RS_Vector& wcsPos,
                                                                     std::vector<LC_VisualSnapPointHolder>& specialPointSnapCandidates,
                                                                     LC_VisualSnapSolution& solution, const RS_Entity* entity,
                                                                     const RS_Vector& vertexSnapCoord) const {
    RS_Vector tangentPoint;
    RS_Vector altTangentPoint;

    const auto* tangentLine = RS_Creation::createTangent1(vertexSnapCoord, vertexSnapCoord, entity, tangentPoint, altTangentPoint);
    if (tangentLine != nullptr) {
        double dist;
        auto line = tryCreateGuidingConstructionLine(wcsPos, vertexSnapCoord, tangentPoint, dist,
                                                     RS2::VisualSnapGuideEntityType::VSNAP_LINE_TANGENT1);
        if (line != nullptr) {
            solution.addGuidingEntity(line);
            specialPointSnapCandidates.push_back(LC_VisualSnapPointHolder(tangentPoint, RS2::VisualSnapGuideEntityType::VSNAP_LINE_TANGENT1));
            solution.addSnapCandidate(tangentPoint);
        }
        if (altTangentPoint.valid) {
            line = tryCreateGuidingConstructionLine(wcsPos, vertexSnapCoord, altTangentPoint, dist,
                                                    RS2::VisualSnapGuideEntityType::VSNAP_LINE_TANGENT1);
            if (line != nullptr) {
                specialPointSnapCandidates.push_back(LC_VisualSnapPointHolder(altTangentPoint, RS2::VisualSnapGuideEntityType::VSNAP_LINE_TANGENT1));
                solution.addGuidingEntity(line);
                solution.addSnapCandidate(altTangentPoint);
            }
        }
        delete tangentLine;
    }
}

void LC_VisualSnapSolutionSolver::addTangentialRaysFromVertexesToArcs(const RS_Vector& wcsPos,
                                                                      std::vector<LC_VisualSnapPointHolder>& specialPointSnapCandidates,
                                                                      LC_VisualSnapSolution& solution) const {
    clearVertexProcessedFlag(solution);
    // tangent line from vertexes to arcs circles
    solution.snapData->forEachDocRef([this,wcsPos, &solution, &specialPointSnapCandidates](LC_VisualSnapDocumentEntityRef* docRef) {
        const auto ent = docRef->guidingEntity.get();
        const RS2::EntityType type = ent->rtti();
        RS_Vector vertexStart{false};
        RS_Vector vertexEnd{false};
        const RS_Entity* entity = nullptr;
        if (type == RS2::EntitySnapArc) {
            vertexStart = ent->getStartpoint();
            vertexEnd = ent->getEndpoint();
            entity = ent;
        }
        else if (type == RS2::EntitySnapCircle) {
            entity = ent;
        }
        else {
            return;
        }

        solution.snapData->forEachVertex(
            [this,entity, vertexStart, vertexEnd, wcsPos, &solution, &specialPointSnapCandidates](LC_VisualSnapVertex* vertexInner) {
                auto vertexSnapCoord = vertexInner->wcsSnapCoordinate;
                // avoid situation where endpoints of arc are vertexes and so additional tangents are created
                if (LC_LineMath::isNotMeaningfulDistance(vertexSnapCoord, vertexStart) || LC_LineMath::isNotMeaningfulDistance(
                    vertexSnapCoord, vertexEnd)) {
                    return;
                }
                addTangentialRayFromVertexToEntity(wcsPos, specialPointSnapCandidates, solution, entity, vertexSnapCoord);
            });
    });
}

void LC_VisualSnapSolutionSolver::createTangentialBetwenTwoEntities(const RS_Vector& wcsPos, LC_VisualSnapSolution& solution,
                                                                    const RS_Entity* entityFirst, const RS_Entity* entitySecond,
                                                                    std::vector<LC_VisualSnapPointHolder>& specialPointSnapCandidates,
                                                                    RS2::VisualSnapGuideEntityType snapEntityType) const {
    std::vector<std::unique_ptr<RS_Line>> tangentLines = RS_Creation::createTangent2(entityFirst, entitySecond);
    if (!tangentLines.empty()) {
        for (const auto& l : tangentLines) {
            double dist;
            auto start = l->getStartpoint();
            auto end = l->getEndpoint();
            const auto line = tryCreateGuidingConstructionLine(wcsPos, start, end, dist, snapEntityType);
            if (line != nullptr) {
                solution.addGuidingEntity(line);
                solution.addSnapCandidate(start); // snap middle
                solution.addSnapCandidate(end);
                specialPointSnapCandidates.push_back(LC_VisualSnapPointHolder(start, snapEntityType));
                specialPointSnapCandidates.push_back(LC_VisualSnapPointHolder(end, snapEntityType));
            }
        }
    }
}

void LC_VisualSnapSolutionSolver::addTangentialRaysBetwenCirclesArcs(const RS_Vector& wcsPos,
                                                                     std::vector<LC_VisualSnapPointHolder>& specialPointSnapCandidates,
                                                                     LC_VisualSnapSolution& solution) const {
    clearDocumentProcessedFlag(solution);
    // mutual tangents (arc-arc, arc-circle etc) - only for document entities
    solution.snapData->forEachDocRef([this, wcsPos, &solution, &specialPointSnapCandidates](LC_VisualSnapDocumentEntityRef* docFirst) {
        unsigned long long firstEntityId = docFirst->originalEntityId;
        const auto entityFirst = docFirst->guidingEntity.get();
        const RS2::EntityType type = entityFirst->rtti();
        if (type == RS2::EntitySnapArc || type == RS2::EntitySnapCircle) {
            docFirst->processed = true;
            solution.snapData->forEachDocRef(
                [this, wcsPos, &solution, &specialPointSnapCandidates, entityFirst, firstEntityId](LC_VisualSnapDocumentEntityRef* secondDocRef) {
                    if (secondDocRef->processed) {
                        return;
                    }
                    unsigned long long secondEntityId = secondDocRef->originalEntityId;
                    if (firstEntityId == secondEntityId) {
                        return;
                    }
                    const auto ent = secondDocRef->guidingEntity.get();
                    if (ent == entityFirst) {
                        return;
                    }
                    const RS2::EntityType type = ent->rtti();
                    if (type == RS2::EntitySnapArc || type == RS2::EntitySnapCircle) {
                        createTangentialBetwenTwoEntities(wcsPos, solution, entityFirst, ent, specialPointSnapCandidates,
                                                          RS2::VisualSnapGuideEntityType::VSNAP_LINE_TANGENT2);
                    }
                });
        }
    });
}

void LC_VisualSnapSolutionSolver::addExplicitlySetDistanceCircle(const RS_Vector& wcsPos, const RS_Vector& wcsCenter, double distanceWCS,
                                                                 LC_VisualSnapSolution& solution) const {
    const double distance = wcsCenter.distanceTo(wcsPos);
    const double distanceDelta = std::abs(distance - distanceWCS);
    bool withinSnapRange = distanceDelta < m_wcsSnapRange;
    if (withinSnapRange || m_options->showNotSnappableGuides) {
        const auto circle = new LC_RefSnapCircle(wcsCenter, distanceWCS);
        circle->setupForPoint(circle->getNearestPointOnEntity(wcsPos), RS2::VSNAP_POINT_DISTANCE_EXPLICIT, withinSnapRange);
        solution.addGuidingEntity(circle);
    }
}

void LC_VisualSnapSolutionSolver::addExplicitlySetDistanceCirclesForVertexes(const RS_Vector& wcsPos,
                                                                             LC_VisualSnapSolution& solution) const {
    // distances - radius from vertexes
    const RS_SnapMode* snap = m_snapper->getSnapMode();
    const bool useDistance = snap->snapDistance;
    if (useDistance) {
        const double distanceWCS = m_snapper->getSnapDistance();
        solution.snapData->forEachVertex([this, wcsPos, distanceWCS, &solution](LC_VisualSnapVertex* vertex) {
            const auto wcsSnapCoordinate = vertex->wcsSnapCoordinate;
            addExplicitlySetDistanceCircle(wcsPos, wcsSnapCoordinate, distanceWCS, solution);
        });
    }
}

void LC_VisualSnapSolutionSolver::addVertexToVertexLines(const RS_Vector& wcsPos, std::vector<LC_VisualSnapPointHolder>& specialPointSnapCandidates,
                                                         LC_VisualSnapSolution& solution, const RS_Vector& firstVertexSnap,
                                                         const RS_Vector& innerVertexSnap) const {
    // create vertex-vertex only if there is snap-middle?
    double dist;
    // create vertex-vertex line
    const auto line = tryCreateVertexVertexLineAndOrthoLine(wcsPos, firstVertexSnap, innerVertexSnap, dist, solution);

    if (line != nullptr) {
        solution.addGuidingEntity(line);
        int middlePoints = 2; // just default value for middle point (if no snap to middle)
        const RS_SnapMode* snapMode = m_snapper->getSnapMode();
        if (snapMode->snapMiddle) {
            middlePoints = m_snapper->getSnapMiddlePoints();
            // higlight possible snaps
            const double lineLength = firstVertexSnap.distanceTo(innerVertexSnap);
            const int segmentsCount = middlePoints + 1;
            const double segmentLength = lineLength / segmentsCount;
            const double lineAngle = firstVertexSnap.angleTo(innerVertexSnap);

            for (int i = 1; i < segmentsCount; i++) {
                const double distanceOnLine = segmentLength * i;
                // if we're still within line, calculate snap point for tick on the line
                RS_Vector snapPosition = firstVertexSnap.relative(distanceOnLine, lineAngle);
                const double distance = snapPosition.distanceTo(wcsPos);
                if (distance < m_wcsSnapRange) {
                    specialPointSnapCandidates.push_back(LC_VisualSnapPointHolder(snapPosition, RS2::VisualSnapGuideEntityType::VSNAP_POINT_MIDDLE));
                }
                solution.addSnapCandidate(snapPosition);
            }
        }
        // add strict middle regardless snap to middle is enabled and if one is enabled, yet it is odd
        const bool odd = middlePoints % 2 == 0;
        if (odd) {
            RS_Vector actualMiddle = (firstVertexSnap + innerVertexSnap) / 2;
            if (dist < m_wcsSnapRange) {
                specialPointSnapCandidates.push_back(LC_VisualSnapPointHolder(actualMiddle, RS2::VisualSnapGuideEntityType::VSNAP_POINT_MIDDLE));
            }
            solution.addSnapCandidate(actualMiddle);
        }
    }
    else {
        delete line;
    }
}

void LC_VisualSnapSolutionSolver::addVertexToVertexMutualDistanceCircles(const RS_Vector& wcsPos, LC_VisualSnapSolution& solution,
                                                                         const RS_Vector& firstVertexSnap,
                                                                         const RS_Vector& innerVertexSnap) const {
    if (m_options->createVertexVertexDistanceCircles) {
        createDistanceCircle2Point(wcsPos, firstVertexSnap, innerVertexSnap, solution);
        createDistanceCircle2Point(wcsPos, innerVertexSnap, firstVertexSnap, solution);
    }
}

void LC_VisualSnapSolutionSolver::addVertexToVertexLinesAndDistances(const RS_Vector& wcsPos,
                                                                     std::vector<LC_VisualSnapPointHolder>& specialPointSnapCandidates,
                                                                     LC_VisualSnapSolution& solution) const {
    clearVertexProcessedFlag(solution);
    bool snapMiddle = m_snapper->getSnapMode()->snapMiddle;
    bool snapDistance = m_snapper->getSnapMode()->snapDistance;
    bool snapCenter = m_snapper->getSnapMode()->snapCenter;
    bool createVertexDistanceTangents = snapDistance && snapCenter && m_options->createVertexVertexDistanceCirclesTangents;
    double snapDistanceRadius = m_snapper->getSnapDistance();

    auto circleFirst = RS_Circle(RS_CircleData(RS_Vector(0, 0), snapDistanceRadius));
    auto circleSecond = RS_Circle(RS_CircleData(RS_Vector(0, 0), snapDistanceRadius));
    // lines between marked points - and snap is on these lines
    solution.snapData->forEachVertex(
        [snapMiddle, wcsPos, &solution, &specialPointSnapCandidates, this, &circleFirst, &circleSecond, snapDistance,
            createVertexDistanceTangents](LC_VisualSnapVertex* vertexFirst) {
            vertexFirst->flagProcessed = true;
            solution.snapData->forEachVertex(
                [vertexFirst,snapMiddle, wcsPos, &solution, &specialPointSnapCandidates, this, &circleFirst, &circleSecond, snapDistance,
                    createVertexDistanceTangents](LC_VisualSnapVertex* vertexInner) {
                    if (vertexInner->flagProcessed) {
                        return;
                    }
                    auto firstVertexSnap = vertexFirst->wcsSnapCoordinate;
                    auto innerVertexSnap = vertexInner->wcsSnapCoordinate;

                    if (snapMiddle) {
                        addVertexToVertexLines(wcsPos, specialPointSnapCandidates, solution, firstVertexSnap, innerVertexSnap);
                    }

                    // create distance circles with radius equal to the distance between vertexes
                    if (snapDistance) {
                        addVertexToVertexMutualDistanceCircles(wcsPos, solution, firstVertexSnap, innerVertexSnap);
                    }
                    if (createVertexDistanceTangents) {
                        circleFirst.setCenter(firstVertexSnap);
                        circleSecond.setCenter(innerVertexSnap);
                        createTangentialBetwenTwoEntities(wcsPos, solution, &circleFirst, &circleSecond, specialPointSnapCandidates,
                                                          RS2::VisualSnapGuideEntityType::VSNAP_LINE_DISTANCE_TANGENT2);
                    }
                });
        });
}

void LC_VisualSnapSolutionSolver::addGuideEntitiesForDocumentEntity(const RS_Vector& wcsPos, LC_VisualSnapSolution& solution,
                                                                    LC_VisualSnapDocumentEntityRef* docEntityRef) const {
    double dist;
    if (docEntityRef->guidingEntity != nullptr) {
        const auto entity = docEntityRef->guidingEntity.get();
        RS_Vector v = entity->getNearestPointOnEntity(wcsPos, true, &dist);
        bool withinSnapRange = dist < m_wcsSnapRange;
        if (withinSnapRange || m_options->showNotSnappableGuides) {
            RS_Entity* clone = entity->clone();
            solution.addGuidingEntity(clone, RS2::VisualSnapGuideEntityType::VSNAP_DOC_ENTITY);
        }
        auto entityRtti = docEntityRef->guidingEntity->rtti();
        if (entityRtti == RS2::EntitySnapLine) {
            const auto& endpoint = entity->getEndpoint();
            const auto& startPoint = entity->getStartpoint();
            if (m_options->createAngleStepRaysForEntitiesEndpoints) {
                tryAddRelativeRaysForStartAndEndPoints(wcsPos, startPoint, endpoint, solution);
            }
            bool snapDistance = m_snapper->getSnapMode()->snapDistance;
            if (snapDistance && m_options->createVertexVertexDistanceCircles) {
                createDistanceCircle2Point(wcsPos, startPoint, endpoint, solution);
                createDistanceCircle2Point(wcsPos, endpoint, startPoint, solution);
            }
        }
        else if (entityRtti == RS2::EntitySnapArc) {
            // fixme - review and complete - endpoints, normals, center
        }
    }
}

void LC_VisualSnapSolutionSolver::addGuideEntitiesByDocumentEntities(const RS_Vector& wcsPos, LC_VisualSnapSolution& solution) const {
    solution.snapData->forEachDocRef([this, wcsPos, &solution](LC_VisualSnapDocumentEntityRef* docEntityRef) {
        addGuideEntitiesForDocumentEntity(wcsPos, solution, docEntityRef);
    });
}

// todo - potentially, this function may be used not only for line entities - but also for rays between vertexes, and for endpoints of arcs...
// todo - yet this may lead to graphical mess and may complicate snapping??
void LC_VisualSnapSolutionSolver::tryAddRelativeRaysForStartAndEndPoints(const RS_Vector& wcsPos, const RS_Vector& startPoint,
                                                                         const RS_Vector& endPoint, LC_VisualSnapSolution& solution) const {
    // create angle step rays that are relative to the line (so 0 for such rays is the same as line direction).
    // for relative rays we'll always show only snappable rays, otherwise it will be total graphical mess on the screen
    const double wcsLineAngleRad = startPoint.angleTo(endPoint);
    const double angleStepRad = m_snapper->getAngleStep();
    const double wcsRaysStartAngle = m_viewport->toWorldAngle(angleStepRad + wcsLineAngleRad);
    const double wcsRaysEndAngle = m_viewport->toWorldAngle(M_PI + wcsLineAngleRad);

    double wcsAngle = wcsRaysStartAngle;
    double minDistanceStartPoint = RS_MAXDOUBLE;
    double minDistanceAngleStartPoint = RS_MAXDOUBLE;
    LC_RefSnapConstructionLine* minDistanceRayStartPoint = nullptr;
    double minDistanceEndPoint = RS_MAXDOUBLE;
    double minDistanceAngleEndPoint = RS_MAXDOUBLE;
    LC_RefSnapConstructionLine* minDistanceRayEndPoint = nullptr;
    do {
        // create rays for start point of line
        double dist;
        RS_Vector secondPointStart = startPoint.relative(g_rayDirectionOffset, wcsAngle);
        const auto rayStart = tryCreateGuidingConstructionLine(wcsPos, startPoint, secondPointStart, dist, wcsAngle,
                                                               RS2::VisualSnapGuideEntityType::VSNAP_LINE_ENDPOINT_ANGLE_STEP);
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

        // create rays for end point of line
        RS_Vector secondPointEnd = endPoint.relative(g_rayDirectionOffset, wcsAngle);
        const auto rayEnd = tryCreateGuidingConstructionLine(wcsPos, endPoint, secondPointEnd, dist, wcsAngle,
                                                             RS2::VisualSnapGuideEntityType::VSNAP_LINE_ENDPOINT_ANGLE_STEP);
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
        solution.addGuidingEntity(minDistanceRayStartPoint, minDistanceAngleStartPoint - wcsLineAngleRad);
    }
    if (minDistanceRayEndPoint != nullptr) {
        solution.addGuidingEntity(minDistanceRayEndPoint, minDistanceAngleEndPoint - wcsLineAngleRad);
    }
}

void LC_VisualSnapSolutionSolver::addNormalFromVertexToDocEntity(const RS_Vector& wcsPos, LC_VisualSnapSolution& solution,
                                                                 LC_VisualSnapVertex* vertex, LC_VisualSnapDocumentEntityRef* docEntityRef) const {
    const unsigned long long vertexEntityId = vertex->entityId;
    const unsigned long long secondEntityId = docEntityRef->originalEntityId;
    if (vertexEntityId == secondEntityId) {
        return;
    }
    double dist;
    auto guideEntity = docEntityRef->guidingEntity.get();
    auto nearestPointOnEntity = guideEntity->getNearestPointOnEntity(vertex->wcsSnapCoordinate, false, &dist);
    if (LC_LineMath::isMeaningfulDistance(nearestPointOnEntity, vertex->wcsSnapCoordinate)) {
        // additional check that vertex is not on entity
        const auto normalLine = tryCreateGuidingConstructionLine(wcsPos, vertex->wcsSnapCoordinate, nearestPointOnEntity, dist,
                                                                 RS2::VisualSnapGuideEntityType::VSNAP_LINE_VERTEX_ORTHO);
        if (normalLine != nullptr) {
            solution.addGuidingEntity(normalLine);
        }
    }
}

void LC_VisualSnapSolutionSolver::addNormalsFromVertexToEntities(const RS_Vector& wcsPos, LC_VisualSnapSolution& solution) const {
    solution.snapData->forEachVertex([wcsPos, &solution, this](LC_VisualSnapVertex* vertex) {
        solution.snapData->forEachDocRef([wcsPos, &solution, vertex, this](LC_VisualSnapDocumentEntityRef* docEntityRef) {
            addNormalFromVertexToDocEntity(wcsPos, solution, vertex, docEntityRef);
        });
    });
}

void LC_VisualSnapSolutionSolver::addRelativePositionEntities(const RS_Vector& wcsPos, LC_VisualSnapSolution& solution, const LC_RelativePositionData& data) const {
    if (data.valid) {
            const auto projectionPoint = data.wcsProjection;
            const auto basePoint = data.wcsBasePoint;
            const double angle = data.wcsAngle;

            if (data.explicitLength) {
                const auto distanceCircle = new LC_RefSnapCircle(basePoint, data.length);
                auto nearestPoint = distanceCircle->getNearestPointOnEntity(wcsPos);
                const double wcsDistance = nearestPoint.distanceTo(wcsPos);
                bool withinSnapRange = wcsDistance < m_wcsSnapRange;
                distanceCircle->setupForPoint(nearestPoint, RS2::VSNAP_POINT_RELATIVE_DISTANCE, withinSnapRange);
                solution.addGuidingEntity(distanceCircle);
            }

            double wcsAngleToBasePoint = projectionPoint.angleTo(basePoint);
            if (data.explicitAngle) {
                createAndAddGuidingConstructionLine(wcsPos, basePoint, projectionPoint, RS2::VSNAP_POINT_RELATIVE_ANGLE_RAY, solution,
                                                    wcsAngleToBasePoint);
            }

            if (data.showLengthNormal) {
                const RS_Vector normal = projectionPoint.relative(g_rayDirectionOffset, angle + M_PI_2);
                createAndAddGuidingConstructionLine(wcsPos, projectionPoint, normal, RS2::VSNAP_POINT_RELATIVE_NORMAL, solution,
                                                    wcsAngleToBasePoint);
            }

            if (data.explicitDX) {
                const auto horizontalPoint = m_viewport->restrictVertical(projectionPoint,
                                                                          RS_Vector(projectionPoint.x,
                                                                                    projectionPoint.y + g_rayDirectionOffset));

                createAndAddGuidingConstructionLine(wcsPos, projectionPoint, horizontalPoint, RS2::VSNAP_POINT_RELATIVE_VERTICAL_DX,
                                                    solution, wcsAngleToBasePoint);
            }

            if (data.explicitDY) {
                const auto verticalPoint = m_viewport->restrictHorizontal(projectionPoint,
                                                                          RS_Vector(projectionPoint.x + g_rayDirectionOffset,
                                                                                    projectionPoint.y));
                createAndAddGuidingConstructionLine(wcsPos, projectionPoint, verticalPoint, RS2::VSNAP_POINT_RELATIVE_HORIZONTAL_DY,
                                                    solution, wcsAngleToBasePoint);
            }
        }
}

void LC_VisualSnapSolutionSolver::addRelativePositionsEntities(const RS_Vector& wcsPos, LC_VisualSnapSolution& solution) const {
    solution.snapData->forEachRelativePosition([this, wcsPos, &solution](const LC_RelativePositionData& data) {
        addRelativePositionEntities(wcsPos, solution, data);
    });
}

void LC_VisualSnapSolutionSolver::addOrdinaryRestrictionLines(const RS_Vector& wcsPos, LC_VisualSnapSolution& solution) const {
    const RS_SnapMode* snapMode = m_snapper->getSnapMode();
    const RS_Vector relZero = m_snapper->getRelativeZero();
    double dist;
    switch (snapMode->restriction) {
        case RS2::RestrictOrthogonal: {
            const auto lineVert = new LC_RefSnapConstructionLine(relZero,
                                                                 m_viewport->restrictVertical(relZero, relZero + g_rayDirectionOffset));
            auto nearestPointOnEntity = lineVert->getNearestPointOnEntity(wcsPos, false, &dist);
            lineVert->setupForPoint(nearestPointOnEntity, RS2::VSNAP_LINE_RESTR_VERTICAL, false, M_PI_2);
            solution.addGuidingEntity(lineVert);

            const auto lineHor = new LC_RefSnapConstructionLine(relZero,
                                                                m_viewport->restrictHorizontal(relZero, relZero + g_rayDirectionOffset));

            nearestPointOnEntity = lineHor->getNearestPointOnEntity(wcsPos, false, &dist);
            lineHor->setupForPoint(nearestPointOnEntity, RS2::VSNAP_LINE_RESTR_HORIZONTAL, false, 0);
            solution.addGuidingEntity(lineHor);
            break;
        }
        case RS2::RestrictHorizontal: {
            const auto lineHor = new LC_RefSnapConstructionLine(relZero,
                                                                m_viewport->restrictHorizontal(relZero, relZero + g_rayDirectionOffset));
            auto nearestPointOnEntity = lineHor->getNearestPointOnEntity(wcsPos, false, &dist);
            lineHor->setupForPoint(nearestPointOnEntity, RS2::VSNAP_LINE_RESTR_HORIZONTAL, false, 0);
            solution.addGuidingEntity(lineHor);
            break;
        }
        case RS2::RestrictVertical: {
            const auto lineVert = new LC_RefSnapConstructionLine(relZero,
                                                                 m_viewport->restrictVertical(relZero, relZero + g_rayDirectionOffset));
            auto nearestPointOnEntity = lineVert->getNearestPointOnEntity(wcsPos, false, &dist);
            lineVert->setupForPoint(nearestPointOnEntity, RS2::VSNAP_LINE_RESTR_VERTICAL, false, M_PI_2);
            solution.addGuidingEntity(lineVert);
            break;
        }
        default: {
            break;
        }
    }
}

void LC_VisualSnapSolutionSolver::createDistanceCircle2Point(const RS_Vector& wcsPos, const RS_Vector& centerPoint,
                                                             const RS_Vector& circlePoint, LC_VisualSnapSolution& solution) const {
    const double distanceBetweenVertexes = centerPoint.distanceTo(circlePoint);
    const double distanceFromFirstToMouse = centerPoint.distanceTo(wcsPos);
    const double distanceDelta = std::abs(distanceBetweenVertexes - distanceFromFirstToMouse);
    bool withinSnapRange = distanceDelta < m_wcsSnapRange;
    if (withinSnapRange || m_options->showNotSnappableGuides) {
        // mouse is close to circle
        const auto circle = new LC_RefSnapCircle(centerPoint, distanceBetweenVertexes);
        circle->setupForPoint(circlePoint, RS2::VSNAP_POINT_DISTANCE_VERTEX, withinSnapRange);
        solution.addGuidingEntity(circle);
    }
}

LC_RefSnapConstructionLine* LC_VisualSnapSolutionSolver::tryCreateGuidingConstructionLine(const RS_Vector& wcsPos, const RS_Vector& start,
                                                                                          const RS_Vector& end, double& dist,
                                                                                          double wcsLineAngle,
                                                                                          RS2::VisualSnapGuideEntityType guideType) const {
    const auto line = new LC_RefSnapConstructionLine(start, end);
    const auto nearestPointOnEntity = line->getNearestPointOnEntity(wcsPos, false, &dist);
    const double wcsDistance = nearestPointOnEntity.distanceTo(wcsPos);
    bool withinSnapRange = wcsDistance < m_wcsSnapRange;
    if (withinSnapRange || m_options->showNotSnappableGuides) {
        line->setupForPoint(nearestPointOnEntity, guideType, withinSnapRange, wcsLineAngle);
        return line;
    }
    delete line;
    return nullptr;
}

LC_RefSnapConstructionLine* LC_VisualSnapSolutionSolver::tryCreateGuidingConstructionLine(const RS_Vector& wcsPos, const RS_Vector& start,
                                                                                          const RS_Vector& end, double& dist,
                                                                                          RS2::VisualSnapGuideEntityType guideType) const {
    const auto line = new LC_RefSnapConstructionLine(start, end);
    const auto nearestPointOnEntity = line->getNearestPointOnEntity(wcsPos, false, &dist);
    const double wcsDistance = nearestPointOnEntity.distanceTo(wcsPos);
    const bool withinSnapRange = wcsDistance < m_wcsSnapRange;
    if (withinSnapRange || m_options->showNotSnappableGuides) {
        const double wcsAngle = start.angleTo(end);
        line->setupForPoint(nearestPointOnEntity, guideType, withinSnapRange, wcsAngle);
        return line;
    }
    delete line;
    return nullptr;
}

LC_RefSnapConstructionLine* LC_VisualSnapSolutionSolver::tryCreateVertexVertexLineAndOrthoLine(
    const RS_Vector& wcsPos, const RS_Vector& start, const RS_Vector& end, double& dist, LC_VisualSnapSolution& solution) const {
    const auto line = new LC_RefSnapConstructionLine(start, end);
    const auto nearestPointOnEntity = line->getNearestPointOnEntity(wcsPos, false, &dist);
    const double wcsDistance = nearestPointOnEntity.distanceTo(wcsPos);
    const bool withinSnapRange = wcsDistance < m_wcsSnapRange;
    const double wcsLineAngle = start.angleTo(end);
    bool includeLine = false;
    if (withinSnapRange || m_options->showNotSnappableGuides) {
        line->setupForPoint(nearestPointOnEntity, RS2::VSNAP_LINE_VERTEX_VERTEX, withinSnapRange, wcsLineAngle);
        includeLine = true;
    }

    // try to build ortho line from previous snapped point to vertex-vertex line
    RS_Vector basePoint = solution.previousSnapPoint;
    if (basePoint.valid && LC_LineMath::isMeaningfulDistance(basePoint, start) && LC_LineMath::isMeaningfulDistance(basePoint, end)) {
        // check that base point does not lie on vertex-vertex line
        double distFormBase;
        const auto pointOnLine = line->getNearestPointOnEntity(basePoint, false, &distFormBase);
        if (LC_LineMath::isMeaningful(distFormBase)) {
            double wcsNormalAngle = wcsLineAngle + M_PI_2;
            const auto normalLine = new LC_RefSnapConstructionLine(basePoint, pointOnLine);
            double distanceToNormal;
            const auto nearestPointOnNormal = normalLine->getNearestPointOnEntity(wcsPos, false, &distanceToNormal);
            const double wcsNormalDistance = nearestPointOnNormal.distanceTo(wcsPos);
            const bool normalWithinSnapRange = wcsNormalDistance < m_wcsSnapRange;
            if (normalWithinSnapRange || m_options->showNotSnappableGuides) {
                normalLine->setupForPoint(nearestPointOnNormal, RS2::VSNAP_LINE_VERTEX_VERTEX_ORTHO, withinSnapRange, wcsNormalAngle);
                solution.addGuidingEntity(normalLine);
            }
            else {
                delete normalLine;
            }
        }
    }
    if (includeLine) {
        return line;
    }

    delete line;
    return nullptr;
}

void LC_VisualSnapSolutionSolver::createAndAddGuidingConstructionLine(const RS_Vector& wcsPos, const RS_Vector& start, const RS_Vector& end,
                                                                      RS2::VisualSnapGuideEntityType guideType,
                                                                      LC_VisualSnapSolution& solution, double wcsAngleToBasePoint) const {
    const auto verticalRay = new LC_RefSnapConstructionLine(start, end);
    auto nearestPoint = verticalRay->getNearestPointOnEntity(wcsPos);
    const double wcsDistance = nearestPoint.distanceTo(wcsPos);
    bool withinSnapRange = wcsDistance < m_wcsSnapRange;
    verticalRay->setupForPoint(nearestPoint, guideType, withinSnapRange, wcsAngleToBasePoint);
    solution.addGuidingEntity(verticalRay);
}

LC_RefSnapLine* LC_VisualSnapSolutionSolver::tryCreateCloseSnapLine(const RS_Vector& wcsPos, const RS_Vector& start, const RS_Vector& end,
                                                                    double& dist, RS2::VisualSnapGuideEntityType guideType) const {
    const auto line = new LC_RefSnapLine(start, end);
    const auto nearestPointOnEntity = line->getNearestPointOnEntity(wcsPos, false, &dist);
    const double wcsDistance = nearestPointOnEntity.distanceTo(wcsPos);
    bool withinSnapRange = wcsDistance < m_wcsSnapRange;
    if (withinSnapRange || m_options->showNotSnappableGuides) {
        const double wcsAngle = start.angleTo(end);
        line->setupForPoint(nearestPointOnEntity, guideType, withinSnapRange, wcsAngle);
        return line;
    }
    delete line;
    return nullptr;
}

bool LC_VisualSnapSolutionSolver::hasNoLinesForPoints(LC_VisualSnapSolution& solution, RS_Vector endPoint, RS_Vector startPoint) const {
    bool noLines = true;
    // ensure that line rays it not created already for another endpoint of line
    for (const auto guide : solution.guidingEntities) {
        if (LC_LineMath::isNotMeaningfulDistance(startPoint, guide.entity->getStartpoint())) {
            if (LC_LineMath::isNotMeaningfulDistance(endPoint, guide.entity->getEndpoint())) {
                noLines = false;
                break;
            }
        }
    }
    return noLines;
}

bool LC_VisualSnapSolutionSolver::hasNoLinesForPoint(LC_VisualSnapSolution& solution, RS_Vector point,
                                                     RS2::VisualSnapGuideEntityType guideType) const {
    bool noLines = true;
    // ensure that line rays it not created already for another endpoint of line
    for (const auto guide : solution.guidingEntities) {
        const auto snapEntity = dynamic_cast<const LC_RefSnapEntity*>(guide.entity);
        if (snapEntity->getGuideType() == guideType) {
            if (LC_LineMath::isNotMeaningfulDistance(point, guide.entity->getStartpoint()) || LC_LineMath::isNotMeaningfulDistance(
                point, guide.entity->getEndpoint())) {
                noLines = false;
                break;
            }
        }
    }
    return noLines;
}
