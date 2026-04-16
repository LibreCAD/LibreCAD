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

#include "lc_line_from_point_to_line_options_filler.h"

#include "lc_action_draw_line_from_point_to_line.h"

class LC_ActionDrawLineFromPointToLine;

void LC_LineFromPointToLineOptionsFiller::fillToolOptionsContainer(LC_PropertyContainer* container) {
    auto action = static_cast<LC_ActionDrawLineFromPointToLine*>(m_action);

    addBoolean({"a_ortho", tr("Orthogonal"), tr("If checked, created line will be orthogonal to selected line")}, [action]()-> bool {
                   return action->getOrthogonal();
               }, [action](bool val)-> void {
                   action->setOrthogonal(val);
               }, container);

    addRawAngleDegrees(
        {"a_angle", tr("Angle"), tr("Angle between original line and created one")}, [action]() { return action->getAngleDegrees(); },
        [action](double val) { action->setAngleDegrees(val); }, container,
        [action](LC_PropertyViewDescriptor&) -> bool { return action->getOrthogonal(); });

    static LC_EnumDescriptor sizeTypDescriptor = {
        "sizeTypeDescriptor",
        {
            {LC_ActionDrawLineFromPointToLine::SizeType::SIZE_INTERSECTION, tr("To Intersection")},
            {LC_ActionDrawLineFromPointToLine::SizeType::SIZE_FIXED_LENGTH, tr("Fixed Length")}
        }
    };

    addEnum({
                "a_sizeType",
                tr("Size"),
                tr("Defines whether created line should be created to intersection point or should have fixed length")
            }, &sizeTypDescriptor, [action]() -> LC_PropertyEnumValueType {
                return action->getSizeMode();
            }, [action](const LC_PropertyEnumValueType& v)-> void {
                action->setSizeMode(v);
            }, container);

    const bool intersectionMode = action->getSizeMode() == LC_ActionDrawLineFromPointToLine::SizeType::SIZE_INTERSECTION;

    if (intersectionMode) {
        addLinearDistance({"a_offset", tr("End Offset"), tr("Offset for created line from intersection point")}, [action]() {
                              return action->getEndOffset();
                          }, [action](double val) {
                              action->setEndOffset(val);
                          }, container);
    }
    else {
        addLinearDistance({"a_length", tr("Length"), tr("Fixed length of created line")}, [action]() {
                              return action->getLength();
                          }, [action](double val) {
                              action->setLength(val);
                          }, container);

        static LC_EnumDescriptor snapTypeDescriptor = {
            "sizeTypeDescriptor",
            {
                {LC_ActionDrawLineFromPointToLine::SnapType::SNAP_START, tr("Start")},
                {LC_ActionDrawLineFromPointToLine::SnapType::SNAP_MIDDLE, tr("Middle")},
                {LC_ActionDrawLineFromPointToLine::SnapType::SNAP_END, tr("End")}
            }
        };

        addEnum({"a_snapType", tr("Snap"), tr("Snap point for created line")}, &snapTypeDescriptor, [action]() -> LC_PropertyEnumValueType {
                    return action->getLineSnapMode();
                }, [action](const LC_PropertyEnumValueType& v)-> void {
                    action->setLineSnapMode(v);
                }, container);
    }
}
