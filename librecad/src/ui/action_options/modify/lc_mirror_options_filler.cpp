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

#include "lc_mirror_options_filler.h"

#include "lc_action_modify_mirror.h"

void LC_MirrorOptionsFiller::fillToolOptionsContainer(LC_PropertyContainer* container) {
    auto action = static_cast<LC_ActionModifyMirror*>(m_action);

    addBoolean({"a_mirrorToLine", tr("By line"), tr("If checked, mirror will be performed by selected line entity")}, [action]()-> bool {
                   return action->isMirrorToExistingLine();
               }, [action](bool val)-> void {
                   action->setMirrorToExistingLine(val);
               }, container);

    addBoolean(
        {"a_keepOriginals", tr("Keep originals"), tr("If unchecked, original entities will be removed, otherwise they will survive")},
        [action]()-> bool {
            return action->isKeepOriginals();
        }, [action](bool val)-> void {
            action->setKeepOriginals(val);
        }, container);

    addBoolean({
                   "a_currentAttrs",
                   tr("Current attributes"),
                   tr("If checked, current attributes will be used for mirrored entities, otherwise - original ones")
               }, [action]()-> bool {
                   return action->isUseCurrentAttributes();
               }, [action](bool val)-> void {
                   action->setUseCurrentAttributes(val);
               }, container);

    addBoolean({
                   "a_currentLayer",
                   tr("Current layer"),
                   tr("If checked, mirrored entities will be placed on current layer, otherwise they will be placed into original layers")
               }, [action]()-> bool {
                   return action->isUseCurrentLayer();
               }, [action](bool val)-> void {
                   action->setUseCurrentLayer(val);
               }, container);
}
