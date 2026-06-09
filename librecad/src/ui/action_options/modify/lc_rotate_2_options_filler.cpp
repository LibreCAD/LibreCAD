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

#include "lc_rotate_2_options_filler.h"

#include "lc_action_modify_rotate_twice.h"
#include "lc_property_double_interactivepick_view.h"

void LC_Rotate2OptionsFiller::fillToolOptionsContainer(LC_PropertyContainer* container) {
    auto action = static_cast<LC_ActionModifyRotateTwice*>(m_action);

    addBoolean({
                   "a_mirrored",
                   tr("Angles mirrored"),
                   tr(
                       "Adjust secondary angle so the sum of angles (relative rotation angle) is 0. With such setting entity will be moved but not rotated")
               }, [action]()-> bool {
                   return action->isMirrorAngles();
               }, [action](bool val)-> void {
                   action->setMirrorAngles(val);
               }, container);

    addRawAngle({"a_angle1", tr("Primary angle"), tr("Angle for rotation over absolute reference point")}, [action]() {
                    return action->getAngle1();
                }, [action](double val) {
                    action->setAngle1(val);
                    if (action->isMirrorAngles()) {
                        action->setAngle2(RS_Math::deg2rad(RS_Math::rad2deg(-val)));
                    }
                }, container);

    addRawAngle({"a_angle2", tr("Secondary angle"), tr("Angle to rotate over secondary reference point")}, [action]() {
                    return action->getAngle2();
                }, [action](double val) {
                    action->setAngle2(val);
                }, container, [action](LC_PropertyViewDescriptor& d) {
                    d.viewName = LC_PropertyDoubleInteractivePickView::VIEW_NAME;
                    return action->isMirrorAngles();
                });

    addBoolean({"a_keepOriginals", tr("Keep originals"), tr("If checked, original entities will survive, otherwise they will be removed")},
               [action]()-> bool {
                   return action->isKeepOriginals();
               }, [action](bool val)-> void {
                   action->setKeepOriginals(val);
               }, container);

    addBoolean({"a_multiCopy", tr("Multiple"), tr("If checked, multiple copies will be created")}, [action]()-> bool {
                   return action->isUseMultipleCopies();
               }, [action](bool val)-> void {
                   action->setUseMultipleCopies(val);
               }, container);

    addIntSpinbox({"a_number", tr("Copies"), tr("Number of copies to create")}, [action]()-> int {
                      return action->getCopiesNumber();
                  }, [action](int val)-> void {
                      action->setCopiesNumber(val);
                  }, container, 1, -1, [action](LC_PropertyViewDescriptor&) {
                      return !action->isUseMultipleCopies();
                  });

    addBoolean({
                   "a_currentAttrs",
                   tr("Current attributes"),
                   tr("If checked, current attributes will be applied to created entities, otherwise original ones will be used")
               }, [action]()-> bool {
                   return action->isUseCurrentAttributes();
               }, [action](bool val)-> void {
                   action->setUseCurrentAttributes(val);
               }, container);

    addBoolean({
                   "a_currentLayer",
                   tr("Current layer"),
                   tr("If checked, created entities will be placed into current layer, otherwise they will be in original layers")
               }, [action]()-> bool {
                   return action->isUseCurrentLayer();
               }, [action](bool val)-> void {
                   action->setUseCurrentLayer(val);
               }, container);
}
