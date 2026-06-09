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

#include "lc_draw_bounding_box_options_filler.h"

#include "lc_action_draw_bounding_box.h"

void LC_DrawBoundingBoxOptionsFiller::fillToolOptionsContainer(LC_PropertyContainer* container) {
    auto action = static_cast<LC_ActionDrawBoundingBox*>(m_action);
    addLinearDistance({"a_offset", tr("Offset"), tr("Offset of created box from actual bounding box")}, [action]()-> double {
                       return action->getOffset();
                   }, [action](double val)-> void {
                       action->setOffset(val);
                   }, container);

    addBoolean({"a_cornerPoints", tr("Corner points only"), tr("If selected, only corner points will be drawn for bounding box. Otherwise, edge lines will be drawn")}, [action]()-> bool {
                      return action->isCornerPointsOnly();
                  }, [action](bool val)-> void {
                      action->setCornersOnly(val);
                  }, container);

    addBoolean({"a_polyline", tr("Polyline"), tr("If selected, bounding box is drawn as polyline")}, [action]()-> bool {
                          return action->isCreatePolyline();
                      }, [action](bool val)-> void {
                          action->setCreatePolyline(val);
                      }, container);

    addBoolean({"a_polyline", tr("Selection As Group"), tr("If checked, bounding box for the entire selection will be created. Otherwise, it will be created for individual entities")}, [action]()-> bool {
                      return action->isSelectionAsGroup();
                  }, [action](bool val)-> void {
                      action->setSelectionAsGroup(val);
                  }, container);
}
