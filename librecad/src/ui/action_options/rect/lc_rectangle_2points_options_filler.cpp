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

#include "lc_rectangle_2points_options_filler.h"

#include "lc_action_draw_rectangle_1point.h"
#include "lc_action_draw_rectangle_2points.h"

void LC_Rectangle2PointsOptionsFiller::fillToolOptionsContainer(LC_PropertyContainer* container) {
    auto action = static_cast<LC_ActionDrawRectangle2Points*>(m_action);

    static LC_EnumDescriptor snapModeDescriptor = {
        "tickSnapTypeDescriptor",
        {
            {LC_ActionDrawRectangle2Points::SnapMode::SNAP_CORNER, tr("Corner")},
            {LC_ActionDrawRectangle2Points::SnapMode::SNAP_EDGE_VERT, tr("Mid-vertical")},
            {LC_ActionDrawRectangle2Points::SnapMode::SNAP_EDGE_HOR, tr("Mid-horizontal")},
            {LC_ActionDrawRectangle2Points::SnapMode::SNAP_MIDDLE, tr("Middle")}
        }
    };

    addEnum({"a_snapMode", tr("Start Snap"), tr("Defines which point of rectangle should be placed into start point position")},
            &snapModeDescriptor, [action]() -> LC_PropertyEnumValueType {
                return action->getInsertionPointSnapMode();
            }, [action](const LC_PropertyEnumValueType& v)-> void {
                action->setInsertionPointSnapMode(v);
            }, container);

    addEnum({"a_snapMode", tr("End Snap"), tr("Defines which point of rectangle should be placed into end point position")},
            &snapModeDescriptor, [action]() -> LC_PropertyEnumValueType {
                return action->getSecondPointSnapMode();
            }, [action](const LC_PropertyEnumValueType& v)-> void {
                action->setSecondPointSnapMode(v);
            }, container);

    addBoolean({"a_hasBaseAngle", tr("Rotated"), tr("If checked, allows to specify rotation angle for rectangle.")}, [action]()-> bool {
                   return action->hasBaseAngle();
               }, [action](bool val)-> void {
                   action->setBaseAngleFixed(val);
               }, container);

    addRawAngleDegrees(
        {"a_angle", tr("Angle"), tr("Rotation angle")}, [action]() -> double { return action->getUcsAngleDegrees(); },
        [action](const double& v) -> void { action->setUcsAngleDegrees(v); }, container,
        [action](LC_PropertyViewDescriptor&) -> bool { return !action->hasBaseAngle(); });

    addBoolean({"a_polyline", tr("Polyline"), tr("If checked, rectangle will be created as polyline instead of individual segments")},
               [action]()-> bool {
                   return action->isUsePolyline();
               }, [action](bool val)-> void {
                   action->setUsePolyline(val);
               }, container);

    fillCornersMode(container, action);
}
