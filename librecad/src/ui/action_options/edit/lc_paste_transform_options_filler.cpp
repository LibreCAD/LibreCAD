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

#include "lc_paste_transform_options_filler.h"

#include "lc_action_edit_paste_transform.h"

void LC_PasteTransformOptionsFiller::fillToolOptionsContainer(LC_PropertyContainer* container) {
    auto action = static_cast<LC_ActionEditPasteTransform*>(m_action);

    addRawAngle(
        {"a_angle", tr("Angle"), tr("Rotation angle for pasted entities")}, [action]() { return action->getAngle(); },
        [action](double val) { action->setAngle(val); }, container,
        [action](LC_PropertyViewDescriptor&) -> bool { return action->isArrayCreated() && action->isSameAngles(); });

    addLinearDistance({"a_factor", tr("Scale factor"), tr("Scale factor for pasted entities")}, [action]() {
                          return action->getFactor();
                      }, [action](double val) {
                          action->setFactor(val);
                      }, container);

    addBoolean({"a_array", tr("Paste As Array"), tr("If checked, arrays of copies will be created on paste")}, [action]()-> bool {
                   return action->isArrayCreated();
               }, [action](bool val)-> void {
                   action->setArrayCreated(val);
               }, container);

    addIntSpinbox(
        {"a_x_number", tr("Columns number"), tr("Array columns number")}, [action]() -> int { return action->getArrayXCount(); },
        [action](int val) -> void { action->setArrayXCount(val); }, container, 1, -1,
        [action](LC_PropertyViewDescriptor&) -> bool { return !action->isArrayCreated(); });

    addIntSpinbox(
        {"a_y_number", tr("Rows number"), tr("Array rows number")}, [action]() -> int { return action->getArrayYCount(); },
        [action](int val) -> void { action->setArrayYCount(val); }, container, 1, -1,
        [action](LC_PropertyViewDescriptor&) -> bool { return !action->isArrayCreated(); });

    addLinearDistance(
        {"a_x_spacing", tr("Columns spacing"), tr("Spacing between insertion points of entities in columns")},
        [action]() { return action->getArraySpacingX(); }, [action](double val) { action->setArraySpacingX(val); }, container,
        [action](LC_PropertyViewDescriptor&) -> bool { return !action->isArrayCreated(); });

    addLinearDistance(
        {"a_y_spacing", tr("Rows spacing"), tr("Spacing between entities insertion points in rows")},
        [action]() { return action->getArraySpacingY(); }, [action](double val) { action->setArraySpacingY(val); }, container,
        [action](LC_PropertyViewDescriptor&) -> bool { return !action->isArrayCreated(); });

    addRawAngle(
        {"a_arrayAngle", tr("Array Angle"), tr("Rotation angle for array")}, [action]() { return action->getArrayAngle(); },
        [action](double val) { action->setArrayAngle(val); }, container,
        [action](LC_PropertyViewDescriptor&) -> bool { return !action->isArrayCreated(); });

    addBoolean(
        {"a_sameAngles", tr("Same Angles"),
         tr("If checked, the same rotation angle is used for each individual paste and whole array. Otherwise, different angles are used")},
        [action]() -> bool { return action->isSameAngles(); }, [action](bool val) -> void { action->setSameAngles(val); }, container,
        [action](LC_PropertyViewDescriptor&) -> bool { return !action->isArrayCreated(); });
}
