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

#include "lc_line_radiant_options_filler.h"

#include "lc_action_draw_line_radiant.h"
#include "lc_enum_descriptor.h"

void LC_LineRadiantOptionsFiller::fillToolOptionsContainer(LC_PropertyContainer* container) {
    auto action = static_cast<LC_ActionDrawLineRadiant*>(m_action);

    addIntSpinbox({"a_ctiveRadiantIdx", tr("Radiant Index"), tr("Selection of radiant (center) point draw to")}, [action]() {
                      return action->getActiveRadiantIndex() + 1;
                  }, [action](LC_PropertyEnumValueType index) {
                      action->setActiveRadiantIndex(static_cast<LC_ActionDrawLineRadiant::RadiantIdx>(index - 1));
                  }, container, 1, 4);

    addVector({"a_activeRadiant", tr("Radiant Point"), tr("Specifies position of active radiant point")}, [action]() -> RS_Vector {
                  return action->getActiveRadiant();
              }, [action](const RS_Vector& v)-> void {
                  action->setActiveRadiantPoint(v);
              }, container);

    static LC_EnumDescriptor descriptor = {
        "lenTypeDescriptor",
        {
            {LC_ActionDrawLineRadiant::LenghtType::LINE, tr("Line")},
            {LC_ActionDrawLineRadiant::LenghtType::BY_X, tr("By X")},
            {LC_ActionDrawLineRadiant::LenghtType::BY_Y, tr("By Y")},
            {LC_ActionDrawLineRadiant::LenghtType::TO_POINT, tr("To Point")},
            {LC_ActionDrawLineRadiant::LenghtType::FREE, tr("Free")},
        }
    };

    addEnum({"a_lenType", tr("Length type"), tr("Defines how to handle length parameter")}, &descriptor,
            [action]() -> LC_PropertyEnumValueType {
                return action->getLenghType();
            }, [action](const LC_PropertyEnumValueType& v)-> void {
                const auto type = static_cast<LC_ActionDrawLineRadiant::LenghtType>(v);
                action->setLengthType(type);
            }, container);

    addLinearDistance({"a_length", tr("Length"), tr("Length of line to draw")}, [action]() {
                          return action->getLength();
                      }, [action](double val) {
                          action->setLength(val);
                      }, container);
}
