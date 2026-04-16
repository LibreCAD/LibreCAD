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


#include "lc_visual_snap_solution_visualizer.h"

#include "lc_highlight.h"
#include "lc_ref_snap_construction_line.h"
#include "lc_ref_snap_line.h"
#include "lc_ref_snap_mark.h"
#include "lc_visual_snap_data.h"
#include "lc_visual_snap_options.h"
#include "lc_visual_snap_solution.h"
#include "rs_preview.h"

namespace{
    constexpr double g_rayDirectionOffset = 10.0;

    QString getGuideEntityLabel(RS2::VisualSnapGuideEntityType guideType) {
        switch (guideType) {
            case RS2::VSNAP_LINE_VERTEX_VERTICAL: {
                return "Y";
            }
            case RS2::VSNAP_LINE_VERTEX_HORIZONTAL: {
               return "X";
            }
            case RS2::VSNAP_LINE_VERTEX_VERTEX: {
                return "pp";
            }
            case RS2::VSNAP_LINE_VERTEX_VERTEX_ORTHO: {
                return "ppO";
            }
            case RS2::VSNAP_LINE_VERTEX_ORTHO: {
                return "O";
            }
            case RS2::VSNAP_LINE_VERTEX_ANGLE_STEP: {
                return "/";
            }
            case RS2::VSNAP_LINE_ENDPOINT_TANGENT: {
                return  "T";
            }
            case RS2::VSNAP_LINE_ENDPOINT_NORMAL: {
                return  "N";
            }
            case RS2::VSNAP_LINE_ENDPOINT_ANGLE_STEP: {
                return "<";
            }
            case RS2::VSNAP_LINE_RAY: {
                return  "L";
            }
            case RS2::VSNAP_LINE_TANGENT1: {
                return  "T1";
            }
            case RS2::VSNAP_LINE_TANGENT2: {
                return "T2";
            }
            case RS2::VSNAP_POINT_MIDDLE: {
                return  "M";
            }
            case RS2::VSNAP_POINT_DISTANCE_EXPLICIT: {
                return  "~";
            }
            case RS2::VSNAP_POINT_DISTANCE_VERTEX: {
                return  "~V";
            }
            case RS2::VSNAP_LINE_DISTANCE_TANGENT2: {
                return  "T~";
            }
            case RS2::VSNAP_POINT_RELATIVE_DISTANCE: {
                return  "@~";
            }
            case RS2::VSNAP_POINT_RELATIVE_NORMAL: {
                return  "@N";
            }
            case RS2::VSNAP_POINT_RELATIVE_ANGLE_RAY: {
                return "@<";
            }
            case RS2::VSNAP_POINT_RELATIVE_VERTICAL_DX: {
                return  "@X";
            }
            case RS2::VSNAP_POINT_RELATIVE_HORIZONTAL_DY: {
                return  "@Y";
            }
            case RS2::VSNAP_DOC_ENTITY: {
                return  "E";
            }
            case RS2::VSNAP_LINE_RESTR_HORIZONTAL: {
                return  "!X";
            }
            case RS2::VSNAP_LINE_RESTR_VERTICAL: {
                return  "!Y";
            }
            default:
                break;
        }
        return "";
    }
}

void LC_VisualSnapSolutionVisualizer::visualizeSolution(RS_Preview* preview, LC_Highlight* highlight, LC_VisualSnapSolution& solution) const {
    // add marks
    solution.snapData->forEachItem([this, highlight, preview](LC_VisualSnapItem* i) {
        if (i->isVertexItem) {
            const auto p = i->vertex.get();
            if (p->flagHighlighted) {
                const auto mark = new LC_RefSnapMark(preview, p->wcsSnapCoordinate, m_options->vertexSizeHighlighted,
                                                     LC_RefSnapMark::HIGHLIGHTED);
                preview->addEntity(mark);
            }
            else {
                const auto mark = new LC_RefSnapMark(nullptr, p->wcsSnapCoordinate, m_options->vertexSizeNormal, LC_RefSnapMark::NORMAL);
                preview->addEntity(mark);
            }
        }
        else {
            if (i->docEntityRef != nullptr) {
                highlight->addEntity(i->docEntityRef->documentViewEntity.get(), false);
            };
        }
    });

    if (m_options->showGuidingEntitiesLabels) {
        std::vector<LC_RefSnapConstructionLine*> lines;
        std::list<RS_Entity*> clones;
        for (const auto& e : solution.guidingEntities) {
            // add to preview all except construction lines
            const auto clone = e.entity->clone();
            const int rtti = clone->rtti();

            if (rtti == RS2::EntitySnapConstructionLine) {
                auto line = static_cast<LC_RefSnapConstructionLine*>(clone);
                lines.push_back(line);
            }
            clones.push_back(clone);
        }

        for (const auto clone:clones) {
            preview->addEntity(clone);
        }
        assignGuidingEntitiesLabels(preview, clones);
        adjustGuidingEntitiesLabelsPositions(lines);
    }
    else  {
        for (const auto& e : solution.guidingEntities) {
            preview->addEntity(e.entity->clone());
        }
    }

    if (solution.restrictedPoint.valid) {
        preview->addEntity(new LC_RefSnapLine(solution.foundSnapPoint, solution.restrictedPoint));
    }

    for (const auto& v : solution.snapCandidatesToShow) {
        const auto mark = new LC_RefSnapMark(nullptr, v, m_options->vertexSizeProjected, LC_RefSnapMark::PROJECTED);
        preview->addEntity(mark);
    }
}

void LC_VisualSnapSolutionVisualizer::adjustGuidingEntitiesLabelsPositions(std::vector<LC_RefSnapConstructionLine*> lines) const {
    // sort lines first by angle, than by nearest point
    std::sort(lines.begin(), lines.end(), [](LC_RefSnapConstructionLine* a, LC_RefSnapConstructionLine* b) -> bool {
        const double angleA = a->getRefSnapInfo().wcsBaseAngle;
        const double angleB = b->getRefSnapInfo().wcsBaseAngle;

        if (LC_LineMath::isSameValue(angleA, angleB)) {
            // lines has same angle, check that they are in the same point
            const RS_Vector nearestPointA = a->getRefSnapInfo().nearestPoint;
            const RS_Vector nearestPointB = b->getRefSnapInfo().nearestPoint;

            if (LC_LineMath::isNotMeaningfulDistance(nearestPointA, nearestPointB)) {
                return false;
            }
            if (LC_LineMath::isSameValue(nearestPointA.x, nearestPointB.x)) {
                return nearestPointA.y < nearestPointB.y;
            }
            return nearestPointA.x < nearestPointB.x;
        }
        return angleA < angleB;
    });


    // as we have sorted lines, we try to set positions (offset) of labels in such way, that labels will not not overlap
    double previousAngle = RS_MAXDOUBLE;
    double currentOffset = 0.5;
    auto previousRefPoint = RS_Vector(RS_MAXDOUBLE, RS_MAXDOUBLE);
    auto previousStartPoint = RS_Vector(RS_MINDOUBLE, RS_MINDOUBLE);
    for (const auto l : lines) {
        const double angle = l->getRefSnapInfo().wcsBaseAngle;
        if (!LC_LineMath::isSameAngle(angle, previousAngle)) {
            currentOffset = 0.5;
            previousStartPoint = l->getStartpoint();
            previousAngle = angle;
        }
        RS_Vector nearestPoint = l->getRefSnapInfo().nearestPoint;
        if (LC_LineMath::isMeaningfulDistance(previousRefPoint, nearestPoint)) {
            previousRefPoint = nearestPoint;
            // lines with different base points may be still the same, so check by angle to start
            if (!LC_LineMath::isSameAngle(previousStartPoint.angleTo(nearestPoint), angle)) {
                currentOffset = 0.5;
                previousStartPoint = l->getStartpoint();
            }
        }
        currentOffset += 0.5;
        l->getRefSnapInfo().labelOffset = currentOffset;
    }
}

void LC_VisualSnapSolutionVisualizer::assignGuidingEntitiesLabels([[maybe_unused]] RS_EntityContainer* preview,
                                                                  std::list<RS_Entity*> clones) const {
    for (const auto clone:clones) {
        const auto snapEntity = dynamic_cast<LC_RefSnapEntity*>(clone);
        if (snapEntity->isActive()) {
            snapEntity->setFont(m_options->guidingEntitiesFontActive);
        }
        else {
            snapEntity->setFont(m_options->guidingEntitiesFont);
        }

        const QString label = getGuideEntityLabel(snapEntity->getRefSnapInfo().guideType);
        snapEntity->setLabel(label);
        snapEntity->setBaseLabelOffset(m_options->baseLabelOffsetPx);
    }
}

void LC_VisualSnapSolutionVisualizer::visualizeOrdinaryRestrictions(RS_Preview* preview, [[maybe_unused]] LC_Highlight* highlight) const {
    const RS_SnapMode* snapMode = m_snapper->getSnapMode();
    const RS_Vector relZero = m_snapper->getRelativeZero();
    switch (snapMode->restriction) {
        // fixme - add label
        // fixme why separate method and not in overall guiding points???
        case RS2::RestrictOrthogonal: {
            const auto lineVert = new LC_RefSnapConstructionLine(relZero,
                                                                 m_viewport->restrictVertical(relZero, relZero + g_rayDirectionOffset));
            preview->addEntity(lineVert);
            const auto lineHor = new LC_RefSnapConstructionLine(relZero,
                                                                m_viewport->restrictHorizontal(relZero, relZero + g_rayDirectionOffset));
            preview->addEntity(lineHor);
            break;
        }
        case RS2::RestrictHorizontal: {
            const auto lineHor = new LC_RefSnapConstructionLine(relZero,
                                                                m_viewport->restrictHorizontal(relZero, relZero + g_rayDirectionOffset));
            preview->addEntity(lineHor);
            break;
        }
        case RS2::RestrictVertical: {
            const auto lineVert = new LC_RefSnapConstructionLine(relZero,
                                                                 m_viewport->restrictVertical(relZero, relZero + g_rayDirectionOffset));
            preview->addEntity(lineVert);
            break;
        }
        default: {
            break;
        }
    }
}
