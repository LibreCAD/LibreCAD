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

#include "lc_polygon_options_filler.h"

#include "lc_action_draw_polygon_base.h"

void LC_PolygonOptionsFiller::fillToolOptionsContainer(LC_PropertyContainer* container) {
    auto action = static_cast<LC_ActionDrawPolygonBase*>(m_action);

    addIntSpinbox({"a_rays_number", tr("Edges number"), tr("Number of edges")}, [action]()-> int {
                      return action->getNumber();
                  }, [action](int val)-> void {
                      action->setNumber(val);
                  }, container);

    addBoolean({"a_polyline", tr("Polyline"), tr("If checked, the polygon will be drawn as polyline. Otherwise, individual entities will be created")}, [action]()-> bool {
                   return action->isPolyline();
               }, [action](bool val)-> void {
                   action->setPolyline(val);
               }, container);

    addBoolean({"a_rounded", tr("Rounded"), tr("If checked, vertexes of polygon will be rounded")}, [action]()-> bool {
                   return action->isCornersRounded();
               }, [action](bool val)-> void {
                   action->setCornersRounded(val);
               }, container);

    addLinearDistance(
        {"a_radiusOuter", tr("Radius"), tr("Rounding radius for vertex fillet")}, [action]() { return action->getRoundingRadius(); },
        [action](double val) { action->setRoundingRadius(val); }, container,
        [action](LC_PropertyViewDescriptor&) -> bool { return !action->isCornersRounded(); });
}
