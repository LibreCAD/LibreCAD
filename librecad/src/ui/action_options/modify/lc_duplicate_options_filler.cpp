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

#include "lc_duplicate_options_filler.h"

#include "lc_action_modify_duplicate.h"
#include "lc_enum_descriptor.h"

void LC_DuplicateOptionsFiller::fillToolOptionsContainer(LC_PropertyContainer* container) {
    auto action = static_cast<LC_ActionModifyDuplicate*>(m_action);

    addBoolean({"a_inplace", tr("In place"), tr("If checked, duplicate will be positioned in original's coordinates")}, [action]()-> bool {
                   return action->isDuplicateInPlace();
               }, [action](bool val)-> void {
                   action->setDuplicateInPlace(val);
               }, container);

    bool inplace = action->isDuplicateInPlace();

    addLinearDistance({"a_offsetX", tr("Offset X"), tr("Horizontal offset of duplicate from original entity")}, [action]() {
                          return action->getOffsetX();
                      }, [action](double val) {
                          action->setOffsetX(val);
                      }, container, [inplace](LC_PropertyViewDescriptor&) {
                          return inplace;
                      });

    addLinearDistance({"a_offsetY", tr("Offset Y"), tr("Vertical offset of duplicate from original entity")}, [action]() {
                          return action->getOffsetY();
                      }, [action](double val) {
                          action->setOffsetY(val);
                      }, container, [inplace](LC_PropertyViewDescriptor&) {
                          return inplace;
                      });

    static LC_EnumDescriptor penTypeDescriptor = {
        "penTypeDescriptor",
        {
            {LC_AbstractActionWithPreview::PenApplyMode::PEN_ACTIVE, tr("Active")},
            {LC_AbstractActionWithPreview::PenApplyMode::PEN_ORIGINAL, tr("Original")},
            {LC_AbstractActionWithPreview::PenApplyMode::PEN_ORIGINAL_RESOLVED, tr("Original (Resolved)")}
        }
    };

    addEnum({"a_penMode", tr("Pen mode"), tr("Pen to apply to created duplicate")}, &penTypeDescriptor,
            [action]() -> LC_PropertyEnumValueType {
                return action->getPenMode();
            }, [action](const LC_PropertyEnumValueType& v)-> void {
                action->setPenMode(v);
            }, container, [inplace](LC_PropertyViewDescriptor&) {
                return inplace;
            });

    static LC_EnumDescriptor layerTypeDescriptor = {
        "LayerTypeDescriptor",
        {
            {LC_AbstractActionWithPreview::LayerApplyMode::LAYER_ACTIVE, tr("Active", "layer")},
            {LC_AbstractActionWithPreview::LayerApplyMode::LAYER_ORIGINAL, tr("Original", "layer")}
        }
    };

    addEnum({"a_layerMode", tr("Layer mode"), tr("Layer to which duplicate should be placed")}, &layerTypeDescriptor,
            [action]() -> LC_PropertyEnumValueType {
                return action->getLayerMode();
            }, [action](const LC_PropertyEnumValueType& v)-> void {
                action->setLayerMode(v);
            }, container, [inplace](LC_PropertyViewDescriptor&) {
                return inplace;
            });
}
