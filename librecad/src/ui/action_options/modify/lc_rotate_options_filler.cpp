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

#include "lc_rotate_options_filler.h"

#include "lc_action_modify_rotate.h"
#include "lc_property_double_interactivepick_view.h"

void LC_RotateOptionsFiller::fillToolOptionsContainer(LC_PropertyContainer* container) {
    auto action = static_cast<LC_ActionModifyRotate*>(m_action);

    addBoolean({
                   "a_centerFirst",
                   tr("By Center"),
                   tr("If selected, first it is necessary to specify center point. Otherwise - reference point")
               }, [action]()-> bool {
                   return action->isCenterPointFirst();
               }, [action](bool val)-> void {
                   action->setCenterPointFirst(val);
               }, container);

    addBoolean({"a_angleFree", tr("Free Angle"), tr("Specifies whether angle is defined by entered value or by mouse position")},
               [action]()-> bool {
                   return action->isFreeAngle();
               }, [action](bool val)-> void {
                   action->setFreeAngle(val);
               }, container);

    addRawAngle({"a_angle", tr("Angle"), tr("Angle to rotate entity around rotation center point")}, [action]() {
                    return action->getAngle();
                }, [action](double val) {
                    action->setAngle(val);
                }, container, [action](LC_PropertyViewDescriptor& d) {
                    d.viewName = LC_PropertyDoubleInteractivePickView::VIEW_NAME;
                    return action->isFreeAngle();
                });

    addBoolean({
                   "a_angleRelative",
                   tr("Relative"),
                   tr("If selected, rotation angle is defined as relative angle to reference point, instead of absolute angle")
               }, [action]()-> bool {
                   return action->isRelativeAngle();
               }, [action](bool val)-> void {
                   action->setRelativeAngle(val);
               }, container, [action](LC_PropertyViewDescriptor& d) {
                   return !action->isFreeAngle();
               });

    addBoolean({
                   "a_rotateTwice",
                   tr("Rotate Twice"),
                   tr("If checked, second rotation is around reference point")
               }, [action]()-> bool {
                   return action->isRotateAlsoAroundReferencePoint();
               }, [action](bool val)-> void {
                   action->setRotateAlsoAroundReferencePoint(val);
               }, container);

    addBoolean({"a_angle2Free", tr("Free Angle 2"), tr("Free selection of factor should be performed by mouse if checked")},
               [action]()-> bool {
                   return action->isFreeRefPointAngle();
               }, [action](bool val)-> void {
                   action->setFreeRefPointAngle(val);
               }, container, [action](LC_PropertyViewDescriptor& d) {
                   return !action->isRotateAlsoAroundReferencePoint();
               });

    addRawAngle({"a_angle2", tr("Angle2"), tr("Angle for additional rotation around refrence point")}, [action]() {
                    return action->getAngle();
                }, [action](double val) {
                    action->setAngle(val);
                }, container, [action](LC_PropertyViewDescriptor& d) {
                    d.viewName = LC_PropertyDoubleInteractivePickView::VIEW_NAME;
                    return !action->isRotateAlsoAroundReferencePoint() || action->isFreeRefPointAngle();
                });

    addBoolean({"a_angle2Abs", tr("Absolute Angle 2"), tr("If absolute, reference point will be rotated but entity will be rotated by second angle only, otherwise angles will summ up")},
                  [action]()-> bool {
                      return action->isRefPointAngleAbsolute();
                  }, [action](bool val)-> void {
                      action->setRefPointAngleAbsolute(val);
                  }, container, [action](LC_PropertyViewDescriptor& d) {
                      return !action->isRotateAlsoAroundReferencePoint();
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
