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

#include "lc_circle_by_arc_options_filler.h"

#include "lc_action_draw_circle_by_arc.h"

void LC_CircleByArcOptionsFiller::fillToolOptionsContainer(LC_PropertyContainer* container) {
    auto action = static_cast<LC_ActionDrawCircleByArc*>(m_action);

    addBoolean({"a_replaceArc", tr("Replace arc"), tr("If checked, original arc will be removed")}, [action]()-> bool {
                   return action->isReplaceArcByCircle();
               }, [action](bool val)-> void {
                   action->setReplaceArcByCircle(val);
               }, container);

    addLinearDistance(
        {"a_radiusShift", tr("Radius Shift"), tr("Delta of circle's radius to arc's radius")},
        [action]() { return action->getRadiusShift(); }, [action](double val) { action->setRadiusShift(val); }, container,
        [action](LC_PropertyViewDescriptor&) -> bool { return action->isReplaceArcByCircle(); });

    static LC_EnumDescriptor penTypeDescriptor = {
        "penTypeDescriptor",
        {
            {LC_AbstractActionWithPreview::PenApplyMode::PEN_ACTIVE, tr("Active")},
            {LC_AbstractActionWithPreview::PenApplyMode::PEN_ORIGINAL, tr("Original")},
            {LC_AbstractActionWithPreview::PenApplyMode::PEN_ORIGINAL_RESOLVED, tr("Original Resolved")}
        }
    };

    addEnum({"a_penType", tr("Pen to apply"), tr("Defines which pen should be applied to created circle")}, &penTypeDescriptor,
            [action]() -> LC_PropertyEnumValueType {
                return action->getPenMode();
            }, [action](const LC_PropertyEnumValueType& v)-> void {
                action->setPenMode(v);
            }, container);

    static LC_EnumDescriptor layerTypeDescriptor = {
        "layerTypeDescriptor",
        {
            {LC_AbstractActionWithPreview::LayerApplyMode::LAYER_ACTIVE, tr("Active")},
            {LC_AbstractActionWithPreview::LayerApplyMode::LAYER_ORIGINAL, tr("Original")}
        }
    };

    addEnum({"a_layerType", tr("Layer to apply"), tr("Defines which layer should created circle be placed in")}, &layerTypeDescriptor,
            [action]() -> LC_PropertyEnumValueType {
                return action->getLayerMode();
            }, [action](const LC_PropertyEnumValueType& v)-> void {
                action->setLayerMode(v);
            }, container);
}
