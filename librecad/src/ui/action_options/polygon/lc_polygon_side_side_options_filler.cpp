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

#include "lc_polygon_side_side_options_filler.h"

#include "lc_action_draw_polygon_side_side.h"

void LC_PolygonSideSideOptionsFiller::fillToolOptionsContainer(LC_PropertyContainer* container) {
    LC_PolygonOptionsFiller::fillToolOptionsContainer(container);
    auto action = static_cast<LC_ActionDrawLinePolygonSideSide*>(m_action);

    addBoolean({
                   "a_rounded",
                   tr("Vertex to Vertex"),
                   tr("If selected, reference points are for vertexes of polygon. Otherwise, they are for middle point of edges. ")
               }, [action]()-> bool {
                   return action->isVertexVertexMode();
               }, [action](bool val)-> void {
                   action->setVertexVertexMode(val);
               }, container);
}
