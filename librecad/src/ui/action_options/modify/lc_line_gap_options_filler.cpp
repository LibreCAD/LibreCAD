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

#include "lc_line_gap_options_filler.h"

#include "lc_action_modify_line_gap.h"
#include "lc_enum_descriptor.h"

void LC_LineGapOptionsFiller::fillToolOptionsContainer(LC_PropertyContainer* container) {
    auto action = static_cast<LC_ActionModifyLineGap*>(m_action);

    addBoolean({"a_gapFree", tr("Free size"), tr("If cheched, the size of the gap is determined by mouse")}, [action]()-> bool {
                   return action->isFreeGapSize();
               }, [action](bool val)-> void {
                   action->setFreeGapSize(val);
               }, container);

    addLinearDistance({"a_gapSize", tr("Size"), tr("Size of the gap, if not free")}, [action]() {
                          return action->getGapSize();
                      }, [action](double val) {
                          action->setGapSize(val);
                      }, container, [action](LC_PropertyViewDescriptor&) {
                          return action->isFreeGapSize();
                      });

    static LC_EnumDescriptor snapTypeDescriptor = {
        "snapTypeDescriptor",
        {
            {LC_AbstractActionWithPreview::LINE_SNAP_FREE, tr("Free")},
            {LC_AbstractActionWithPreview::LINE_SNAP_START, tr("Start")},
            {LC_AbstractActionWithPreview::LINE_SNAP_MIDDLE, tr("Middle")},
            {LC_AbstractActionWithPreview::LINE_SNAP_END, tr("End")}
        }
    };

    addEnum({"a_snapType", tr("Line Snap"), tr("Snap point for gap on the line")}, &snapTypeDescriptor,
            [action]() -> LC_PropertyEnumValueType {
                return action->getLineSnapMode();
            }, [action](const LC_PropertyEnumValueType& v)-> void {
                action->setLineSnapMode(v);
            }, container);

    addLinearDistance({"a_snapDistance", tr("Distance"), tr("Distance from snap point")}, [action]() {
                          return action->getGapSnapDistance();
                      }, [action](double val) {
                          action->setGapSnapDistance(val);
                      }, container, [action](LC_PropertyViewDescriptor&) {
                          return action->getLineSnapMode() == LC_AbstractActionWithPreview::LINE_SNAP_FREE;
                      });

    static LC_EnumDescriptor gapSnapDescriptor = {
        "gapSnapDescriptor",
        {
            {LC_ActionModifyLineGap::GAP_SNAP_START, tr("Start")},
            {LC_ActionModifyLineGap::GAP_SNAP_MIDDLE, tr("Middle")},
            {LC_ActionModifyLineGap::GAP_SNAP_END, tr("End")}
        }
    };

    addEnum({"a_snapType", tr("Gap Snap"), tr("Snap point of gap to line snap point")}, &gapSnapDescriptor,
            [action]() -> LC_PropertyEnumValueType {
                return action->getGapSnapMode();
            }, [action](const LC_PropertyEnumValueType& v)-> void {
                action->setGapSnapMode(v);
            }, container, [action](LC_PropertyViewDescriptor&) {
                return action->isFreeGapSize();
            });
}
