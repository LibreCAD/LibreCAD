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

#include "lc_polyline_equidistant_options_filler.h"

#include "lc_action_polyline_equidistant.h"

void LC_PolylineEquidistantOptionsFiller::fillToolOptionsContainer(LC_PropertyContainer* container) {
    auto action = static_cast<LC_ActionPolylineEquidistant*>(m_action);

    addLinearDistance({"a_dist", tr("Spacing"), tr("Distance from original polyline")}, [action]() {
                          return action->getDistance();
                      }, [action](double val) {
                          action->setDistance(val);
                      }, container);

    addIntSpinbox({"a_number", tr("Copies"), tr("Number of copies to create")}, [action]()-> int {
                      return action->getCopiesNumber();
                  }, [action](int val)-> void {
                      action->setCopiesNumber(val);
                  }, container, 1);
}
