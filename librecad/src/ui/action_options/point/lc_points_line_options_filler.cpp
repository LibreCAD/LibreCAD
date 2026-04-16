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

#include "lc_points_line_options_filler.h"

#include "lc_action_draw_points_line.h"

void LC_PointsLineOptionsFiller::fillToolOptionsContainer(LC_PropertyContainer* container) {
    auto action = static_cast<LC_ActionDrawPointsLine*>(m_action);

    const bool showAllControls = m_action->rtti() == RS2::ActionDrawPointsLine;

    if (showAllControls) {
        addBoolean({"a_hasAngle", tr("Fixed Angle"), tr("If checked, line of points will be drawn with specified angle")},
                   [action]()-> bool {
                       return action->getDirection() == LC_AbstractActionDrawLine::DIRECTION_ANGLE;
                   }, [action](bool val)-> void {
                       if (val) {
                           action->setSetAngleDirectionState();
                       }
                       else {
                           if (action->getDirection() == LC_AbstractActionDrawLine::DIRECTION_ANGLE) {
                               action->setSetPointDirectionState();
                           }
                       }
                   }, container);

        addRawAngleDegrees(
            {"a_angle", tr("Angle"), tr("Angle of line")}, [action]() -> double { return action->getAngleDegrees(); },
            [action](const double& v) -> void { action->setAngleValueDegrees(v); }, container,
            [action](LC_PropertyViewDescriptor&) -> bool { return action->getDirection() != LC_AbstractActionDrawLine::DIRECTION_ANGLE; });

        addIntSpinbox(
            {"a_pointsNumber", tr("Points number"), tr("Number of points")}, [action]() -> int { return action->getPointsCount(); },
            [action](int val) -> void { action->setPointsCount(val); }, container, 1, -1,
            [action](LC_PropertyViewDescriptor&) -> bool { return action->isWithinLineMode() && action->isFixedDistanceMode(); });

        addBoolean({
                       "a_hasFixedDistance",
                       tr("Fixed Distance"),
                       tr("If checked, fixed specified distance between points will be used. Otherwise, distance will be calculated.")
                   }, [action]()-> bool {
                       return action->isFixedDistanceMode();
                   }, [action](bool val)-> void {
                       action->setFixedDistanceMode(val);
                   }, container);

        addLinearDistance(
            {"a_pointsDistance", tr("Distance"), tr("Distance between points")}, [action]() { return action->getPointsDistance(); },
            [action](double val) { action->setPointsDistance(val); }, container,
            [action](LC_PropertyViewDescriptor&) -> bool { return !action->isFixedDistanceMode(); });

        addBoolean(
            {"a_fitLine", tr("Fit Line"),
             tr("Specifies whether all points should fit between start/end points of line or whether the length of line is calculated "
                "based on number of points and distance between points")},
            [action]() -> bool { return action->isWithinLineMode(); }, [action](bool val) -> void { action->setWithinLineMode(val); },
            container, [action](LC_PropertyViewDescriptor&) -> bool { return !action->isFixedDistanceMode(); });

        static LC_EnumDescriptor edgePointsDescriptor = {
            "tickSnapTypeDescriptor",
            {
                {LC_ActionDrawPointsLine::EdgesMode::DRAW_EDGE_NONE, tr("None")},
                {LC_ActionDrawPointsLine::EdgesMode::DRAW_EDGE_BOTH, tr("Both")},
                {LC_ActionDrawPointsLine::EdgesMode::DRAW_EDGE_START, tr("Start")},
                {LC_ActionDrawPointsLine::EdgesMode::DRAW_EDGE_END, tr("End")}
            }
        };

        addEnum({"a_edgePoints", tr("Edge Points"), tr("Controls how points on the edges (start/end points) should be created. ")},
                &edgePointsDescriptor, [action]() -> LC_PropertyEnumValueType {
                    return action->getEdgePointsMode();
                }, [action](const LC_PropertyEnumValueType& v)-> void {
                    action->setEdgePointsMode(v);
                }, container);
    }
    else {
        addIntSpinbox({"a_pointsNumber", tr("Points number"), tr("Number of points")}, [action]()-> int {
                          return action->getPointsCount();
                      }, [action](int val)-> void {
                          action->setPointsCount(val);
                      }, container);
    }
}
