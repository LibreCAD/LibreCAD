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

#include "lc_line_angle_options_filler.h"

#include "lc_action_draw_line_angle.h"
#include "lc_enum_descriptor.h"

void LC_LineAngleOptionsFiller::fillToolOptionsContainer(LC_PropertyContainer* container) {
    auto action = static_cast<LC_ActionDrawLineAngle*>(m_action);
    const bool angleIsFixed = action->hasFixedAngle();

    if (!angleIsFixed) {
        addRawAngleDegrees({"a_angle", tr("Angle"), tr("Angle of line")}, [action]() {
                               return action->getUcsAngleDegrees();
                           }, [action](double val) {
                               action->setUcsAngleDegrees(val);
                           }, container);
    }

    static LC_EnumDescriptor descriptorFull = {
        "lenTypeDescriptorFull",
        {
            {LC_ActionDrawLineAngle::LengthType::LINE, tr("Line")},
            {LC_ActionDrawLineAngle::LengthType::BY_X, tr("By X")},
            {LC_ActionDrawLineAngle::LengthType::BY_Y, tr("By Y")},
            {LC_ActionDrawLineAngle::LengthType::FREE, tr("Free")},
        }
    };

    static LC_EnumDescriptor descriptorShort = {
        "lenTypeDescriptorFull",
        {{LC_ActionDrawLineAngle::LengthType::LINE, tr("Line")}, {LC_ActionDrawLineAngle::LengthType::FREE, tr("Free")},}
    };

    const bool notFixedLine = m_action->rtti() == RS2::ActionDrawLineAngle;

    const auto enumDescriptor = notFixedLine ? &descriptorFull : &descriptorShort;

    addEnum({"a_lenType", tr("Length type"), tr("Defines how to handle length parameter")}, enumDescriptor,
            [action]() -> LC_PropertyEnumValueType {
                return action->getLengthType();
            }, [action](const LC_PropertyEnumValueType& v)-> void {
                const auto type = static_cast<LC_ActionDrawLineAngle::LengthType>(v);
                action->setLengthType(type, false);
            }, container);

    addLinearDistance({"a_length", tr("Length"), tr("Length of line")}, [action]() {
                          return action->getLength();
                      }, [action](double val) {
                          action->setLength(val);
                      }, container);

    static LC_EnumDescriptor snapTypDescriptor = {
        "snapTypeDescriptor",
        {
            {LC_ActionDrawLineAngle::SnapMode::SNAP_START   , tr("Start")},
            {LC_ActionDrawLineAngle::SnapMode::SNAP_MIDDLE, tr("Middle")},
            {LC_ActionDrawLineAngle::SnapMode::SNAP_END, tr("End")}
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

    if (angleIsFixed) {
        const bool hasCustomAnglesBasis = m_action->hasNonDefaultAnglesBasis();
        if (hasCustomAnglesBasis) {
            addBoolean({
                           "a_toBasis",
                           tr("To Angles Basis"),
                           tr("If selected, line will be orhotogonal to axis of angles basis. Otherwise - to screen axis")
                       }, [action]()-> bool {
                           return action->isInAngleBasis();
                       }, [action](bool val)-> void {
                           action->setInAngleBasis(val);
                       }, container);
        }
    }
}
