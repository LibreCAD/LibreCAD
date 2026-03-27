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

#include "lc_line_parallel_through_options_filler.h"

#include "lc_action_draw_line_parallel_through.h"

void LC_LineParallelThroughOptionsFiller::fillToolOptionsContainer(LC_PropertyContainer* container) {
    auto action = static_cast<LC_ActionDrawLineParallelThrough*>(m_action);

    addIntSpinbox({"a_number", tr("Number"), tr("Number of parallels to create")},
                  [action]()-> int {
                      return action->getNumber();
                  }, [action](int val)-> void {
                      action->setNumber(val);
                  }, container);

    addBoolean({
                   "a_within",
                   tr("Within"),
                   tr("If checked, parallels will be equally distributed between point and line, otherwise they will be distributed starting from point.")
               }, [action]()-> bool {
                   return action->isDistributeWithin();
               }, [action](bool val)-> void {
                   action->setDistributeWithin(val);
               }, container, [action](LC_PropertyViewDescriptor& d) -> bool {
                   return action->getNumber() == 1;
               });

    addBoolean({"a_symmetric", tr("Symmetric"), tr("If checked, parallels will be created on both sides of entity")}, [action]()-> bool {
                   return action->isSymmetric();
               }, [action](bool val)-> void {
                   action->setSymmetric(val);
               }, container);
}
