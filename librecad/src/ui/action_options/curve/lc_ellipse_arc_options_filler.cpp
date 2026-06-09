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

#include "lc_ellipse_arc_options_filler.h"

#include "lc_action_draw_ellipse_axis.h"

class RS_ActionDrawEllipseAxis;

void LC_EllipseArcOptionsFiller::fillToolOptionsContainer(LC_PropertyContainer* container) {
    auto action = static_cast<RS_ActionDrawEllipseAxis*>(m_action);

    addBoolean({"a_reversed", tr("Reversed"), tr("If selected, arc will be clockwise, otherwise - counterclockwise")}, [action]()-> bool {
                  return action->isReversed();
              }, [action](bool val)-> void {
                  action->setReversed(val);
              }, container);
}
