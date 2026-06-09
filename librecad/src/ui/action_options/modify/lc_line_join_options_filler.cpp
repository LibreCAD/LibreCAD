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

#include "lc_line_join_options_filler.h"

#include "lc_action_modify_line_join.h"
#include "lc_enum_descriptor.h"

void LC_LineJoinOptionsFiller::fillToolOptionsContainer(LC_PropertyContainer* container) {
    auto action = static_cast<LC_ActionModifyLineJoin*>(m_action);

    static LC_EnumDescriptor edgeTypeDescriptor = {
        "penTypeDescriptor",
        {
            {LC_ActionModifyLineJoin::EdgeMode::EDGE_EXTEND_TRIM, tr("Extend/Trim")},
            {LC_ActionModifyLineJoin::EdgeMode::EDGE_ADD_SEGMENT, tr("Add Segment")},
            {LC_ActionModifyLineJoin::EdgeMode::EDGE_NO_MODIFICATION, tr("No change")}
        }
    };

    addEnum({"a_edge1", tr("Line 1"), tr("Policy for joining line that was selected first")}, &edgeTypeDescriptor,
            [action]() -> LC_PropertyEnumValueType {
                return action->getLine1EdgeMode();
            }, [action](const LC_PropertyEnumValueType& v)-> void {
                action->setLine1EdgeMode(v);
            }, container);

    addEnum({"a_edge2", tr("Line 2"), tr("Policy for the line was selected second")}, &edgeTypeDescriptor,
            [action]() -> LC_PropertyEnumValueType {
                return action->getLine2EdgeMode();
            }, [action](const LC_PropertyEnumValueType& v)-> void {
                action->setLine2EdgeMode(v);
            }, container);

    static LC_EnumDescriptor penTypeDescriptor = {
        "penTypeDescriptor",
        {
            {LC_ActionModifyLineJoin::AttributesMode::ATTRIBUTES_ACTIVE_PEN_LAYER, tr("Active Pen")},
            {LC_ActionModifyLineJoin::AttributesMode::ATTRIBUTES_LINE_1, tr("Line 1")},
            {LC_ActionModifyLineJoin::AttributesMode::ATTRIBUTES_LINE_2, tr("Line 2")},
            {LC_ActionModifyLineJoin::AttributesMode::ATTRIBUTES_BOTH_LINES, tr("Both Lines")}
        }
    };

    addEnum({"a_penMode", tr("Pen mode"), tr("Defines how pen should be applied to created entities")}, &penTypeDescriptor,
            [action]() -> LC_PropertyEnumValueType {
                return action->getAttributesSource();
            }, [action](const LC_PropertyEnumValueType& v)-> void {
                action->setAttributesSource(v);
            }, container);

    const int line1EdgeMode = action->getLine1EdgeMode();
    const int line2EdgeMode = action->getLine2EdgeMode();

    bool mayNotCreatePolyline = line1EdgeMode == LC_ActionModifyLineJoin::EdgeMode::EDGE_NO_MODIFICATION || line2EdgeMode ==
        LC_ActionModifyLineJoin::EdgeMode::EDGE_NO_MODIFICATION;

    addBoolean({"a_polyline", tr("Polyline"), tr("If checked, polyline will be created instead of individual lines")}, [action]()-> bool {
                   return action->isCreatePolyline();
               }, [action](bool val)-> void {
                   action->setCreatePolyline(val);
               }, container,  [mayNotCreatePolyline](LC_PropertyViewDescriptor&) {
                   return mayNotCreatePolyline;
               });

    bool mayNotRemoveOriginals = !(line1EdgeMode == LC_ActionModifyLineJoin::EdgeMode::EDGE_EXTEND_TRIM || line2EdgeMode ==
        LC_ActionModifyLineJoin::EdgeMode::EDGE_EXTEND_TRIM);

    addBoolean({"a_removeOriginals", tr("Remove originals"), tr("If checked, original lines will be removed")}, [action]()-> bool {
                   return action->isRemoveOriginalLines();
               }, [action](bool val)-> void {
                   action->setRemoveOriginalLines(val);
               }, container, [mayNotRemoveOriginals](LC_PropertyViewDescriptor&) {
                   return mayNotRemoveOriginals;
               });
}
