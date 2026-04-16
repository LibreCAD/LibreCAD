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

#include "lc_rectangle_3_points_options_filler.h"

#include "lc_action_draw_rectangle_3points.h"

void LC_Rectangle3PointsOptionsFiller::fillToolOptionsContainer(LC_PropertyContainer* container) {
    auto action = static_cast<LC_ActionDrawRectangle3Points*>(m_action);

    addBoolean({"a_quadrangle", tr("Quadrangle"), tr("If checked, quadrangle will be created instead of rectangle")}, [action]()-> bool {
                     return action->isCreateQuadrangle();
                 }, [action](bool val)-> void {
                     action->setCreateQuadrangle(val);
                 }, container);

    const bool quadrangle = action->isCreateQuadrangle();
    if (quadrangle) {
        const bool fixedInnerAngle = action->isInnerAngleFixed();

        addBoolean({"a_hasFixedInnerAngle", tr("Fixed inner angle"), tr("If checked, inner angle of quadrangle will be fixed to specified value")}, [action]()-> bool {
                       return action->isInnerAngleFixed();
                   }, [action](bool val)-> void {
                       action->setInnerAngleFixed(val);
                   }, container);

        addRawAngleDegrees(
            {"a_innerAngle", tr("Inner Angle"), tr("Inner angle of quadrangle")},
            [action]() -> double { return action->getFixedInnerAngle(); },
            [action](const double& v) -> void { action->setFixedInnerAngle(v); }, container,
            [action](LC_PropertyViewDescriptor&) -> bool { return !action->isInnerAngleFixed(); });
    }

    addBoolean({"a_hasBaseAngle", tr("Fixed base angle"), tr("If checked, allows to specify rotation angle for rectangle.")}, [action]()-> bool {
                   return action->hasBaseAngle();
               }, [action](bool val)-> void {
                   action->setBaseAngleFixed(val);
               }, container);

    addRawAngleDegrees(
        {"a_angle", tr("Angle"), tr("Rotation angle")}, [action]() -> double { return action->getUcsAngleDegrees(); },
        [action](const double& v) -> void { action->setUcsAngleDegrees(v); }, container,
        [action](LC_PropertyViewDescriptor&) -> bool { return !action->hasBaseAngle(); });

    addBoolean({"a_polyline", tr("Polyline"), tr("If checked, rectangle will be created as polyline instead of individual segments")},
               [action]()-> bool {
                   return action->isUsePolyline();
               }, [action](bool val)-> void {
                   action->setUsePolyline(val);
               }, container);


    if (quadrangle) {
        createEdgesModeOption(container, action);
    }
    else {
        fillCornersMode(container, action);
    }
}
