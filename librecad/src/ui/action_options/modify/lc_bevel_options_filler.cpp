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

#include "lc_bevel_options_filler.h"

#include "lc_action_modify_bevel.h"

void LC_BevelOptionsFiller::fillToolOptionsContainer(LC_PropertyContainer* container) {
    auto action = static_cast<LC_ActionModifyBevel*>(m_action);

    addBoolean({"a_trim", tr("Trim"), tr("Check to trim both entities to the bevel")}, [action]()-> bool {
                 return action->isTrimOn();
             }, [action](bool val)-> void {
                 action->setTrim(val);
             }, container);

    addLinearDistance({"a_len1", tr("Length 1"), tr("Length of bevel in X direction")}, [action]() {
                          return action->getLength1();
                      }, [action](double val) {
                          action->setLength1(val);
                      }, container);

    addLinearDistance({"a_len2", tr("Lengh 2"), tr("Length of bevel in Y direction")}, [action]() {
                          return action->getLength2();
                      }, [action](double val) {
                          action->setLength2(val);
                      }, container);
}
