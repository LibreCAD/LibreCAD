
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

#ifndef LC_VISUALSNAPSOLUTIONSOLVER_H
#define LC_VISUALSNAPSOLUTIONSOLVER_H

#include <vector>

#include "lc_ref_snap_construction_line.h"
#include "lc_ref_snap_line.h"
#include "lc_relative_point_data.h"
#include "lc_visual_snap_solution.h"

struct LC_VisualSnapOptions;
class RS_Vector;
struct LC_VisualSnapPointHolder;
struct LC_VisualSnapSolution;

class LC_VisualSnapSolutionSolver {
public:
    LC_VisualSnapSolutionSolver(RS_Snapper* snapper, LC_VisualSnapOptions* options)
        : m_snapper(snapper), m_options(options) {
    }

    void solveVisualSnap(const RS_Vector& wcsPos, LC_VisualSnapSolution& solution) const;
    void findSnapPoint(const RS_Vector& wcsPos, LC_VisualSnapSolution& solution, const std::vector<LC_VisualSnapPointHolder>& specialPointSnapCandidates) const;
    void setSnapRange(double snapRangeToUse) {m_wcsSnapRange = snapRangeToUse;}
    void setViewport(LC_GraphicViewport* viewport) {
        m_viewport = viewport;
    }
protected:
    void addOrthoRaysForVertexes(const RS_Vector& wcsPos, LC_VisualSnapSolution& solution) const;
    void addLineRayAndNormal(const RS_Vector& wcsPos, LC_VisualSnapSolution& solution, const RS_Vector& wcsSnapCoordinate, const LC_RefSnapConstructionLine* refLine) const;
    bool hasNoLinesForPoints(const LC_VisualSnapSolution& solution, const RS_Vector& endPoint, const RS_Vector& startPoint) const;
    bool hasNoLinesForPoint(const LC_VisualSnapSolution& solution, const RS_Vector& point, RS2::VisualSnapGuideEntityType guideType) const;
    void addLineRays(const RS_Vector& wcsPos, LC_VisualSnapSolution& solution) const;
    void addTangentialAndNormlRaysForArcEndpoint(const RS_Vector& wcsPos, LC_VisualSnapSolution& solution, const RS_Vector& wcsSnapCoordinate, LC_RefSnapArc* arc) const;
    void addOrthoAndAngleRaysForPoint(const RS_Vector& wcsPos, LC_VisualSnapSolution& solution, double angleStepRad,
                                      double wcsRaysStartAngle, double wcsRaysEndAngle, double wcsMPI_2Angle, const RS_Vector& vertexWCSPoint) const;
    void addTangentialAndNormalRaysFromArcEndpoint(const RS_Vector& wcsPos, LC_VisualSnapSolution& solution) const;
    void addTangentialRayFromVertexToEntity(const RS_Vector& wcsPos, std::vector<LC_VisualSnapPointHolder>& specialPointSnapCandidates, LC_VisualSnapSolution& solution, const RS_Entity* entity, const
                                            RS_Vector& vertexSnapCoord) const;
    void addExplicitlySetDistanceCirclesForVertexes(const RS_Vector& wcsPos, LC_VisualSnapSolution& solution) const;
    void addVertexToVertexLines(const RS_Vector& wcsPos, std::vector<LC_VisualSnapPointHolder>& specialPointSnapCandidates,
                                LC_VisualSnapSolution& solution, const RS_Vector& firstVertexSnap, const RS_Vector& innerVertexSnap) const;
    void addVertexToVertexMutualDistanceCircles(const RS_Vector& wcsPos, LC_VisualSnapSolution& solution, const RS_Vector& firstVertexSnap,
                                                const RS_Vector& innerVertexSnap) const;
    void addVertexToVertexLinesAndDistances(const RS_Vector& wcsPos, std::vector<LC_VisualSnapPointHolder>& specialPointSnapCandidates, LC_VisualSnapSolution& solution) const;
    void addGuideEntitiesForDocumentEntity(const RS_Vector& wcsPos, LC_VisualSnapSolution& solution, LC_VisualSnapDocumentEntityRef* docEntityRef) const;
    void addGuideEntitiesByDocumentEntities(const RS_Vector& wcsPos, LC_VisualSnapSolution& solution) const;
    void tryAddRelativeRaysForStartAndEndPoints(const RS_Vector& wcsPos, const RS_Vector& startPoint, const RS_Vector& endPoint,
                            LC_VisualSnapSolution& solution) const;
    void addNormalsFromVertexToEntities(const RS_Vector& wcsPos, LC_VisualSnapSolution& solution) const;
    void addRelativePositionEntities(const RS_Vector& wcsPos, LC_VisualSnapSolution& solution, const LC_RelativePositionData& data) const;
    void addNormalFromVertexToDocEntity(const RS_Vector& wcsPos, LC_VisualSnapSolution& solution, LC_VisualSnapVertex* vertex,
                                        LC_VisualSnapDocumentEntityRef* docEntityRef) const;
    void addRelativePositionsEntities(const RS_Vector& wcsPos, LC_VisualSnapSolution& solution) const;
    void addOrdinaryRestrictionLines(const RS_Vector& wcsPos, LC_VisualSnapSolution& solution) const;
    void createDistanceCircle2Point(const RS_Vector& wcsPos, const RS_Vector& centerPoint, const RS_Vector& circlePoint,
                                    LC_VisualSnapSolution& solution) const;
    LC_RefSnapConstructionLine* tryCreateGuidingConstructionLine(const RS_Vector& wcsPos, const RS_Vector& start, const RS_Vector& end,
                                                                 double& dist, double wcsLineAngle,
                                                                 RS2::VisualSnapGuideEntityType guideType) const;
    LC_RefSnapConstructionLine* tryCreateGuidingConstructionLine(const RS_Vector& wcsPos, const RS_Vector& start, const RS_Vector& end,
                                                                 double& dist, RS2::VisualSnapGuideEntityType guideType) const;
    LC_RefSnapConstructionLine* tryCreateVertexVertexLineAndOrthoLine(const RS_Vector& wcsPos, const RS_Vector& start,
                                                                       const RS_Vector& end, double& dist,
                                                                       LC_VisualSnapSolution& solution) const;
    void createAndAddGuidingConstructionLine(const RS_Vector& wcsPos, const RS_Vector& start, const RS_Vector& end,
                                             RS2::VisualSnapGuideEntityType guideType, LC_VisualSnapSolution& solution,
                                             double wcsAngleToBasePoint) const;
    LC_RefSnapLine* tryCreateCloseSnapLine(const RS_Vector& wcsPos, const RS_Vector& start, const RS_Vector& end,
                                           double& dist, RS2::VisualSnapGuideEntityType guideType) const;
    void addTangentialRaysBetwenCirclesArcs(const RS_Vector& wcsPos, std::vector<LC_VisualSnapPointHolder>& specialPointSnapCandidates, LC_VisualSnapSolution& solution) const;
    void addExplicitlySetDistanceCircle(const RS_Vector& wcsPos, const RS_Vector& wcsCenter, double distanceWCS,
                                        LC_VisualSnapSolution& solution) const;
    void addTangentialRaysFromVertexesToArcs(const RS_Vector& wcsPos, std::vector<LC_VisualSnapPointHolder>& specialPointSnapCandidates, LC_VisualSnapSolution& solution) const;
    void createTangentialBetwenTwoEntities(const RS_Vector& wcsPos, LC_VisualSnapSolution& solution, const RS_Entity* entityFirst,
                                          const RS_Entity* entitySecond, std::vector<LC_VisualSnapPointHolder>& specialPointSnapCandidates,
                                          RS2::VisualSnapGuideEntityType snapEntityType) const;

    double m_wcsSnapRange{0.0};
    LC_GraphicViewport* m_viewport = nullptr;
    RS_Snapper* m_snapper{nullptr};
    LC_VisualSnapOptions* m_options {nullptr};
};
#endif
