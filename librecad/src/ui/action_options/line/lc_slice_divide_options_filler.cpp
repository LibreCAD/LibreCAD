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

#include "lc_slice_divide_options_filler.h"

#include "lc_action_draw_slice_divide.h"
#include "lc_enum_descriptor.h"

void LC_SliceDivideOptionsFiller::fillToolOptionsContainer(LC_PropertyContainer* container) {
    auto action = static_cast<LC_ActionDrawSliceDivide*>(m_action);
    const bool forCircle = action->rtti() == RS2::ActionDrawSliceDivideCircle;

    if (forCircle) {
        addIntSpinbox({"a_tickCount", tr("Ticks Count"), "Count of ticks between edges of selected entity"}, [action]() {
                          return action->getTickCount();
                      }, [action](const LC_PropertyEnumValueType val) {
                          action->setTickCount(val);
                      }, container,  1, -1);
    }
    else {
        addBoolean({
                       "a_distFixed",
                       tr("Fixed Distance"),
                       tr(
                           "If checked, specified fixed distance between ticks will be used. Otherwise, distance will be calculated based on entity length and ticks count. ")
                   }, [action]()-> bool {
                       return action->isFixedDistance();
                   }, [action](bool val)-> void {
                       action->setFixedDistance(val);
                   }, container);

        if (action->isFixedDistance()) {
            addLinearDistance({"a_distance", tr("Distance"), tr("Distance between ticks")}, [action]() {
                                  return action->getDistance();
                              }, [action](double val) {
                                  action->setDistance(val);
                              }, container);
        }
        else {
            addIntSpinbox({"a_tickCount", tr("Ticks Count"), "Count of ticks between edges of selected entity"}, [action]() {
                              return action->getTickCount();
                          }, [action](const LC_PropertyEnumValueType val) {
                              action->setTickCount(val);
                          }, container,  1, -1);
        }
    }

    addLinearDistance({"a_tickLen", tr("Tick length"), tr("Length of tick")}, [action]() {
                          return action->getTickLength();
                      }, [action](double val) {
                          action->setTickLength(val);
                      }, container);

    addRawAngle({"a_angle", tr("Angle"), tr("Angle between tick and selected entity")}, [action]() {
                    return RS_Math::deg2rad(action->getTickAngleDegrees());
                }, [action](double val) {
                    action->setTickAngleDegrees(RS_Math::rad2deg(val));
                }, container);

    addBoolean({
                   "a_relAngle",
                   tr("Relative angle"),
                   tr("If checked, tick angle is related to selected entity, otherwise it is absolute angle.")
               }, [action]()-> bool {
                   return action->isTickAngleRelative();
               }, [action](bool val)-> void {
                   action->setTickAngleRelative(val);
               }, container);

    static LC_EnumDescriptor tickSnapTypeDescriptor = {
        "tickSnapTypeDescriptor",
        {
            {LC_ActionDrawSliceDivide::LINE_SNAP_FREE, tr("Free")},
            {LC_ActionDrawSliceDivide::LINE_SNAP_START, tr("Start")},
            {LC_ActionDrawSliceDivide::LINE_SNAP_MIDDLE, tr("Middle")},
            {LC_ActionDrawSliceDivide::LINE_SNAP_END, tr("End")}
        }
    };

    addEnum({"a_tickType", tr("Tick Snap"), tr("Defines which point of tick should be placed to intersection point with selected entity")},
            &tickSnapTypeDescriptor, [action]() -> LC_PropertyEnumValueType {
                return action->getTickSnapMode();
            }, [action](const LC_PropertyEnumValueType& v)-> void {
                action->setTickSnapMode(v);
            }, container);

    addLinearDistance({"a_offset", tr("Offset"), tr("Offset of tick snap point to intersection point")}, [action]() {
                          return action->getTickOffset();
                      }, [action](double val) {
                          action->setTickOffset(val);
                      }, container);

    static LC_EnumDescriptor edgeTickTypeDescriptor = {
        "tickSnapTypeDescriptor",
        {
            {LC_ActionDrawSliceDivide::DRAW_EDGE_NONE, tr("None")},
            {LC_ActionDrawSliceDivide::DRAW_EDGE_BOTH, tr("Both")},
            {LC_ActionDrawSliceDivide::DRAW_EDGE_START, tr("Start")},
            {LC_ActionDrawSliceDivide::DRAW_EDGE_END, tr("End")}
        }
    };

    addEnum({"a_edgeTick", tr("Edge Tick"), tr("Defines how ticks on entity edges should be placed. ")}, &edgeTickTypeDescriptor,
            [action]() -> LC_PropertyEnumValueType {
                return action->getDrawTickOnEdgeMode();
            }, [action](const LC_PropertyEnumValueType& v)-> void {
                action->setDrawTickOnEdgeMode(v);
            }, container);

    if (forCircle) {
        addRawAngle({"a_circleAngle", tr("Start Circle Angle"), tr("Start angle for circle from which ticks will start")}, [action]() {
                        return RS_Math::deg2rad(action->getCircleStartAngleDegrees());
                    }, [action](double val) {
                        action->setCircleStartTickAngleDegrees(RS_Math::rad2deg(val));
                    }, container);
    }

    addBoolean({"a_doDivide", tr("Divide"), tr("If checked, selected entity will be divided by tick intersection points")},
               [action]()-> bool {
                   return action->isDivideEntity();
               }, [action](bool val)-> void {
                   action->setDivideEntity(val);
               }, container);
}
