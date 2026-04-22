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

#include "lc_line_snake_options_filler.h"

#include "lc_action_draw_line_snake.h"
#include "lc_enum_descriptor.h"

void LC_LineSnakeOptionsFiller::fillToolOptionsContainer(LC_PropertyContainer* container) {
    auto action = static_cast<LC_ActionDrawLineSnake*>(m_action);

    static LC_EnumDescriptor snapTypDescriptor = {
        "lineDirectionEnum",
        {
            {LC_AbstractActionDrawLine::Direction::DIRECTION_X, tr("X")},
            {LC_AbstractActionDrawLine::Direction::DIRECTION_Y, tr("Y")},
            {LC_AbstractActionDrawLine::Direction::DIRECTION_POINT, tr("Point")},
            {LC_AbstractActionDrawLine::Direction::DIRECTION_ANGLE, tr("Angle")}
        }
    };

    addEnum({"a_lineDirection", tr("Direction"), tr("Direction mode for line drawing")}, &snapTypDescriptor,
            [action]() -> LC_PropertyEnumValueType {
                return action->getDirection();
            }, [action](const LC_PropertyEnumValueType& v)-> void {
                action->setDirection(v);
            }, container);

    addRawAngleDegrees(
        {"a_angle", tr("Angle"), tr("Angle of line")}, [action]() { return action->getAngleDegrees(); },
        [action](double val) { action->setAngleDegrees(val); }, container,
        [action](LC_PropertyViewDescriptor&) -> bool {
            return action->getDirection() != LC_AbstractActionDrawLine::Direction::DIRECTION_ANGLE;
        });

    addBoolean(
        {"a_relAngle", tr("Relative angle"), tr("If checked, angle is relative to previous segment")},
        [action]() -> bool { return action->isAngleRelative(); }, [action](bool val) -> void { action->setAngleIsRelative(val); },
        container,
        [action](LC_PropertyViewDescriptor&) -> bool {
            return action->getDirection() != LC_AbstractActionDrawLine::Direction::DIRECTION_ANGLE;
        });

    createCommandsLine(container, "a_commands1", tr("Close"), tr("Form a closed contour from lines drawn"), tr("Polyline"),
                       tr("Creates polyline from line segments"), [action](int linkIndex)-> void {
                           if (linkIndex == 0) {
                               action->close();
                           }
                           else {
                               action->polyline();
                           }
                       }, tr("Generic commands for line"), action->mayClose(), action->mayClose());

    createCommandsLine(container, "a_commands2", tr("Undo"), tr("Undo the last line drawing"), tr("Redo"), tr("Redo the last line drawing"),
                       [action](int linkIndex)-> void {
                           if (linkIndex == 0) {
                               action->undo();
                           }
                           else {
                               action->redo();
                           }
                       }, tr("Generic commands for line"), action->mayUndo(), action->mayRedo());
}
