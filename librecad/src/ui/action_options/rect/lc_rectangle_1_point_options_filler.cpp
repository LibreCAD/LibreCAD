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

#include "lc_rectangle_1_point_options_filler.h"

#include "lc_action_draw_rectangle_1point.h"

void LC_Rectangle1PointOptionsFiller::fillToolOptionsContainer(LC_PropertyContainer* container) {
    auto action = static_cast<LC_ActionDrawRectangle1Point*>(m_action);

    addLinearDistance({"a_width", tr("Width"), tr("Width of rectangle")}, [action]() {
                          return action->getWidth();
                      }, [action](double val) {
                          action->setWidth(val);
                      }, container);

    addLinearDistance({"a_height", tr("Height"), tr("Height of rectangle")}, [action]() {
                          return action->getHeight();
                      }, [action](double val) {
                          action->setHeight(val);
                      }, container);

    static LC_EnumDescriptor snapModeDescriptor = {
        "tickSnapTypeDescriptor",
        {
            {LC_ActionDrawRectangle1Point::SnapMode::SNAP_TOP_LEFT, tr("Top-left")},
            {LC_ActionDrawRectangle1Point::SnapMode::SNAP_TOP, tr("Top")},
            {LC_ActionDrawRectangle1Point::SnapMode::SNAP_TOP_RIGHT, tr("Top-right")},
            {LC_ActionDrawRectangle1Point::SnapMode::SNAP_LEFT, tr("Left")},
            {LC_ActionDrawRectangle1Point::SnapMode::SNAP_MIDDLE, tr("Middle")},
            {LC_ActionDrawRectangle1Point::SnapMode::SNAP_RIGHT, tr("Right")},
            {LC_ActionDrawRectangle1Point::SnapMode::SNAP_BOTTOM_LEFT, tr("Bottom-left")},
            {LC_ActionDrawRectangle1Point::SnapMode::SNAP_BOTTOM, tr("Bottom")},
            {LC_ActionDrawRectangle1Point::SnapMode::SNAP_BOTTOM_RIGHT, tr("Bottom-right")}
        }
    };

    addEnum({"a_snapMode", tr("Snap"), tr("Point of rectangle wich will be placed to insertion point. ")}, &snapModeDescriptor,
            [action]() -> LC_PropertyEnumValueType {
                return action->getInsertionPointSnapMode();
            }, [action](const LC_PropertyEnumValueType& v)-> void {
                action->setInsertionPointSnapMode(v);
            }, container);

    addBoolean({"a_hasBaseAngle", tr("Rotated"), tr("If checked, rectangle will be rotated on specified angle")}, [action]()-> bool {
                   return action->hasBaseAngle();
               }, [action](bool val)-> void {
                   action->setBaseAngleFixed(val);
               }, container);

    addBoolean(
        {"a_angleFree", tr("Angle is Free"),
         tr("If checked, angle will be specified by mouse position. Otherwise, specified value will be used.")},
        [action]() -> bool { return action->isBaseAngleFree(); }, [action](bool val) -> void { action->setBaseAngleFree(val); }, container,
        [action](LC_PropertyViewDescriptor&) -> bool { return !action->hasBaseAngle(); });

    addRawAngleDegrees(
        {"a_angle", tr("Angle"), tr("Rotation angle")}, [action]() -> double { return action->getUcsAngleDegrees(); },
        [action](const double& v) -> void { action->setUcsAngleDegrees(v); }, container,
        [action](LC_PropertyViewDescriptor&) -> bool { return !action->hasBaseAngle() || action->isBaseAngleFree(); });

    addBoolean({"a_polyline", tr("Polyline"), tr("If checked, rectangle will be created as polyline instead of individual segments")},
               [action]()-> bool {
                   return action->isUsePolyline();
               }, [action](bool val)-> void {
                   action->setUsePolyline(val);
               }, container);

    fillCornersMode(container, action);
}
