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

#include "lc_center_mark_options_filler.h"

#include "lc_action_draw_center_mark.h"
#include "lc_enum_descriptor.h"

class LC_ActionDrawCenterMark;

void LC_CenterMarkOptionsFiller::fillToolOptionsContainer(LC_PropertyContainer* container) {
    auto action = static_cast<LC_ActionDrawCenterMark*>(m_action);


    static LC_EnumDescriptor sizeTypeEnumDescriptor = {
        "sizeTypeEnumDescriptor",
        {
            {LC_ActionDrawCenterMark::CROSS_SIZE_EXTEND, tr("Extension")},
            {LC_ActionDrawCenterMark::CROSS_SIZE_LENGTH, tr("Total Length")},
            {LC_ActionDrawCenterMark::CROSS_SIZE_PERCENT, tr("Percent")}
        }
    };

    addEnum({"a_sizeType", tr("Mark Type"), tr("Defines how to handle size of cross")},
            &sizeTypeEnumDescriptor, [action]() -> LC_PropertyEnumValueType {
                return action->getCrossMode();
            }, [action](const LC_PropertyEnumValueType& v)-> void {
                action->setCrossMode(v);
            }, container);

    addLinearDistance({"a_lenX", tr("Horizontal"), tr("Horizontal dimension of cross")}, [action]()-> double {
                       return action->getLenX();
                   }, [action](double val)-> void {
                       action->setXLength(val);
                   }, container);

    addLinearDistance({"a_lenY", tr("Vertical"), tr("Vertical dimension of the cross")}, [action]()-> double {
                          return action->getLenY();
                      }, [action](double val)-> void {
                          action->setYLength(val);
                      }, container);

    addRawAngleDegrees({"a_angle", tr("Angle"), tr("Rotation angle for cross around center")}, [action]() {
                 return action->getCrossAngleDegrees();
             }, [action](double val) {
                 action->setCrossAngleDegrees(val);
             }, container);
}
