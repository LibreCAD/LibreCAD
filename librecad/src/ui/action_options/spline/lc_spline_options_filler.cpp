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

#include "lc_spline_options_filler.h"

#include "lc_action_draw_spline.h"

void LC_SplineOptionsFiller::fillToolOptionsContainer(LC_PropertyContainer* container) {
    auto action = static_cast<LC_ActionDrawSpline*>(m_action);

    const bool drawSplineAction = action->rtti() == RS2::ActionDrawSpline;
    if (drawSplineAction) {
        addIntSpinbox({"a_splineDegree", tr("Spline degree"), tr("Defines degree of the spline")}, [action]() {
                      return action->getDegree();
                  }, [action](int val) {
                      action->setDegree(val);
                  }, container, 1, 3);
    }

    addBoolean({"a_closed", tr("Closed"), tr("If selected, closed spline is created")}, [action]()-> bool {
                   return action->isClosed();
               }, [action](bool val)-> void {
                   action->setClosed(val);
               }, container);

    createCommandsLine(container, "a_commands1", tr("Undo"), tr("Form a closed contour from lines drawn"), "",
                      "", [action](int linkIndex)-> void {
                          if (linkIndex == 0) {
                              action->undo();
                          }
                      }, tr("Generic commands for spline"));

}
