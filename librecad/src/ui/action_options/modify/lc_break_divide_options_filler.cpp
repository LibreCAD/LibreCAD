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

#include "lc_break_divide_options_filler.h"

#include "lc_action_modify_break_divide.h"

void LC_BreakDivideOptionsFiller::fillToolOptionsContainer(LC_PropertyContainer* container) {
    auto action = static_cast<LC_ActionModifyBreakDivide*>(m_action);

    addBoolean({
                  "a_remove",
                  tr("Remove segments"),
                  tr("If checked, sеgments between intersection points will be removed. If unchecked - the entitiy will be divided in intersection points")
              }, [action]()-> bool {
                  return action->isRemoveSegment();
              }, [action](bool val)-> void {
                  action->setRemoveSegment(val);
              }, container);

    addBoolean({
                  "a_selected",
                  tr("Remove selected"),
                  tr("If checked, selected segment will be removed. Otherwise, selected segment will survive and remaining segments will be removed")
              }, [action]()-> bool {
                  return action->isRemoveSelected();
              }, [action](bool val)-> void {
                  action->setRemoveSelected(val);
              }, container, [action](LC_PropertyViewDescriptor&) ->bool {
                  return !action->isRemoveSegment();
              });
}
