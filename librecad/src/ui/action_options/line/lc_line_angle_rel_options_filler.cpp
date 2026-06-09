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

#include "lc_line_angle_rel_options_filler.h"

#include "lc_action_draw_line_angle_rel.h"
#include "lc_enum_descriptor.h"

void LC_LineAngleRelOptionsFiller::fillToolOptionsContainer(LC_PropertyContainer* container) {
    auto action = static_cast<LC_ActionDrawLineAngleRel*>(m_action);

    addBoolean({"a_lenFree", tr("Free length"), tr("If checked, the length of line is defined by mouse position instead of setting")},
               [action]()-> bool {
                   return action->isLengthFree();
               }, [action](bool val)-> void {
                   action->setLengthIsFree(val);
               }, container);

    addLinearDistance(
        {"a_length", tr("Length"), tr("Length of line")}, [action]() { return action->getTickLength(); },
        [action](double val) { action->setTickLength(val); }, container,
        [action](LC_PropertyViewDescriptor&) -> bool { return action->isLengthFree(); });

    if (!action->isFixedAngleActionMode()) {
        // fixme - should there be different angles (wcs and raw) for abs and relative cases?
        addRawAngle({"a_angle", tr("Angle"), tr("Angle of line")}, [action]() {
                        return RS_Math::deg2rad(action->getTickAngleDegrees());
                    }, [action](double val) {
                        action->setTickAngleDegrees(RS_Math::rad2deg(val));
                    }, container);

        addBoolean({"a_relAngle", tr("Relative angle"), tr("If checked, angle is relative to angle of selected entity")},
                   [action]()-> bool {
                       return action->isAngleRelative();
                   }, [action](bool val)-> void {
                       action->setAngleIsRelative(val);
                   }, container);
    }

    static LC_EnumDescriptor snapTypDescriptor = {
        "snapTypeDescriptor",
        {
            {LC_AbstractActionWithPreview::LINE_SNAP_FREE, tr("Free")},
            {LC_AbstractActionWithPreview::LINE_SNAP_START, tr("Start")},
            {LC_AbstractActionWithPreview::LINE_SNAP_MIDDLE, tr("Middle")},
            {LC_AbstractActionWithPreview::LINE_SNAP_END, tr("End")}
        }
    };

    addEnum({
                "a_snapType",
                tr("Line Snap"),
                tr("Snap point for position of intersection point between created line and source entity (within original line)")
            }, &snapTypDescriptor, [action]() -> LC_PropertyEnumValueType {
                return action->getLineSnapMode();
            }, [action](const LC_PropertyEnumValueType& v)-> void {
                action->setLineSnapMode(v);
            }, container);

    addLinearDistance({"a_snapDistance", tr("Snap Distance"), "Distance of intersection point from specified line snap point"}, [action]() {
                          return action->getIntersectionOffsetDistance();
                      }, [action](double val) {
                          action->setIntersectionOffsetDistance(val);
                      }, container);

    static LC_EnumDescriptor tickSnapTypeDescriptor = {
        "tickSnapTypeDescriptor",
        {
            {LC_ActionDrawLineAngleRel::TICK_SNAP_START, tr("Start")},
            {LC_ActionDrawLineAngleRel::TICK_SNAP_MIDDLE, tr("Middle")},
            {LC_ActionDrawLineAngleRel::TICK_SNAP_END, tr("End")}
        }
    };

    addEnum({"a_lenType", tr("Tick Snap"), tr("Defines which part of created line will be snapped to intersection point")},
            &tickSnapTypeDescriptor, [action]() -> LC_PropertyEnumValueType {
                return action->getTickSnapMode();
            }, [action](const LC_PropertyEnumValueType& v)-> void {
                action->setTickSnapMode(v);
            }, container);

    addLinearDistance({"a_offset", tr("Offset"), tr("Offset of tick snap point from intersection point")}, [action]() {
                          return action->getTickOffset();
                      }, [action](double val) {
                          action->setTickOffset(val);
                      }, container);

    addBoolean({"a_divide", tr("Divide"), tr("If checked, original entity will be divided by intersection point.")}, [action]()-> bool {
                   return action->isDivideLine();
               }, [action](bool val)-> void {
                   action->setDivideLine(val);
               }, container);
}
