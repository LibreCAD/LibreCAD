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

#include "lc_points_lattice_options_filler.h"

#include "lc_action_draw_points_lattice.h"

void LC_PointsLatticeOptionsFiller::fillToolOptionsContainer(LC_PropertyContainer* container) {
    auto action = static_cast<LC_ActionDrawPointsLattice*>(m_action);

    addIntSpinbox({"a_colNumber", tr("Columns Count"), tr("Number of points by X lattice direction (count of lattice columns)")},
                  [action]()-> int {
                      return action->getColumnPointsCount();
                  }, [action](int val)-> void {
                      action->setColumnPointsCount(val);
                  }, container);

    addIntSpinbox({"a_rowNumber", tr("Rows Count"), tr("Number of points by lattice Y direction (count of lattice rows)")},
                  [action]()-> int {
                      return action->getRowPointsCount();
                  }, [action](int val)-> void {
                      action->setRowPointsCount(val);
                  }, container);

    addBoolean({
                   "a_adjust",
                   tr("Adjust last point"),
                   tr(
                       "If selected, last point of lattice will be adjusted accoriding to position of first point, so lattice will be rect or quadrangle")
               }, [action]()-> bool {
                   return action->isAdjustLastPointToFirst();
               }, [action](bool val)-> void {
                   action->setAdjustLastPointToFirst(val);
               }, container);
}
