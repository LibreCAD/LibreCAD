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

#include "lc_arc_tangential_options_filler.h"

#include "lc_action_draw_arc_tangential.h"
#include "lc_enum_descriptor.h"

void LC_ArcTangentialOptionsFiller::fillToolOptionsContainer(LC_PropertyContainer* container) {
    auto action = static_cast<LC_ActionDrawArcTangential*>(m_action);

    static LC_EnumDescriptor modeEnumDescriptor = {"modeEnumDescriptor", {{0, tr("Fixed Radius")}, {1, tr("Fixed Angle")}}};

    addEnum({"a_mode", tr("Draw mode"), tr("Specifies whether arc should be created with fixed radius or fixed central angle")},
            &modeEnumDescriptor, [action]() -> LC_PropertyEnumValueType {
                return action->isByRadius() ? 0 : 1;
            }, [action](const LC_PropertyEnumValueType& v)-> void {
                action->setByRadius(v == 0);
            }, container);

    const bool byRadius = action->isByRadius();
    if (byRadius) {
        addLinearDistance({"a_radius", tr("Radius"), tr("Radius of arc")}, [action]() {
                              return action->getRadius();
                          }, [action](double val) {
                              action->setRadius(val);
                          }, container);
    }
    else {
        addRawAngle({"a_angle", tr("Central Angle"), tr("Central angle of the arc")}, [action]() {
                        return action->getAngle();
                    }, [action](double val) {
                        action->setAngle(val);
                    }, container);
    }
}
