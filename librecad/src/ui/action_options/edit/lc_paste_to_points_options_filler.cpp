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

#include "lc_paste_to_points_options_filler.h"

#include "lc_action_edit_paste_to_points.h"

void LC_PasteToPointsOptionsFiller::fillToolOptionsContainer(LC_PropertyContainer* container) {
    auto action = static_cast<LC_ActionPasteToPoints*>(m_action);

    addRawAngle({"a_angle", tr("Angle"), tr("Rotation angle for pasted entities")}, [action]() -> double {
                    return action->getAngle();
                }, [action](const double& v)-> void {
                    action->setAngle(v);
                }, container);

    addLinearDistance({"a_factor", tr("Scale factor"), tr("Scale factor for pasted entities")}, [action]() {
                          return action->getScaleFactor();
                      }, [action](double val) {
                          action->setScaleFactor(val);
                      }, container);

    addBoolean({
                   "a_removePoints",
                   tr("Remove points"),
                   tr(
                       "If selected, the point entity used as insertion point will be deleted after paste operation. Otherwise, it will survive.")
               }, [action]()-> bool {
                   return action->isRemovePointAfterPaste();
               }, [action](bool val)-> void {
                   action->setRemovePointAfterPaste(val);
               }, container);
}
