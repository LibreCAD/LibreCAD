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

#include "lc_trim_amount_options_filler.h"

#include "lc_action_modify_trim_amount.h"

void LC_TrimAmountOptionsFiller::fillToolOptionsContainer(LC_PropertyContainer* container) {
    auto action = static_cast<LC_ActionModifyTrimAmount*>(m_action);

    addBoolean({"a_total", tr("Total"), tr("The input length is used as total length after trimming, instead of length increase")}, [action]()-> bool {
                return action->isDistanceTotalLength();
            }, [action](bool val)-> void {
                action->setDistanceIsTotalLength(val);
            }, container);

    addLinearDistance({"a_dist", tr("Length"), tr("Distance. Negative values for trimming, positive values for extending. Negative sign is ignored when trimming to final total length")}, [action]() {
                          return action->getDistance();
                      }, [action](double val) {
                          action->setDistance(val);
                      }, container);

    addBoolean({"a_trim", tr("Trim"), tr("If checked, trim/extension is performed for both sides of line.")}, [action]()-> bool {
                   return action->isSymmetricDistance();
               }, [action](bool val)-> void {
                   action->setSymmetricDistance(val);
               }, container,  [action](LC_PropertyViewDescriptor&) {
                   return action->isDistanceTotalLength();
               });
}
