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

#include "lc_star_options_filler.h"

#include "lc_action_draw_star.h"

void LC_StarOptionsFiller::fillToolOptionsContainer(LC_PropertyContainer* container) {
    auto action = static_cast<LC_ActionDrawStar*>(m_action);

    addBoolean({"a_symmetric", tr("Symmetric"), tr("If checked, symmetric star will be created")}, [action]()-> bool {
                   return action->isSymmetric();
               }, [action](bool val)-> void {
                   action->setSymmetric(val);
               }, container);

    addIntSpinbox({"a_rays_number", tr("Rays number"), tr("Number of rays")}, [action]()-> int {
                      return action->getRaysNumber();
                  }, [action](int val)-> void {
                      action->setRaysNumber(val);
                  }, container);

    addBoolean({"a_roundOuter", tr("Round outer"), tr("If checked, outer edges will be rounded")}, [action]()-> bool {
                   return action->isOuterRounded();
               }, [action](bool val)-> void {
                   action->setOuterRounded(val);
               }, container);

    addLinearDistance(
        {"a_radiusOuter", tr("Radius Outer"), tr("Radius for outer rays (edges)")}, [action]() { return action->getRadiusOuter(); },
        [action](double val) { action->setRadiusOuter(val); }, container,
        [action](LC_PropertyViewDescriptor&) -> bool { return !action->isOuterRounded(); });

    addBoolean({"a_roundInner", tr("Round inner"), tr("If checked, inner edges will be rounded")}, [action]()-> bool {
                   return action->isInnerRounded();
               }, [action](bool val)-> void {
                   action->setInnerRounded(val);
               }, container);

    addLinearDistance(
        {"a_radiusInner", tr("Radius Inner"), tr("Radius for inner edges")}, [action]() { return action->getRadiusInner(); },
        [action](double val) { action->setRadiusInner(val); }, container,
        [action](LC_PropertyViewDescriptor&) -> bool { return !action->isInnerRounded(); });

    addBoolean({"a_polyline", tr("As polyline"), tr("If checked, the star will be drawn as polyline")}, [action]()-> bool {
                   return action->isPolyline();
               }, [action](bool val)-> void {
                   action->setPolyline(val);
               }, container);
}
