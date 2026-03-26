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

#include "lc_spline_from_polyline_options_filler.h"

#include "lc_action_spline_from_polyline.h"

void LC_SplineFromPolylineOptionsFiller::fillToolOptionsContainer(LC_PropertyContainer* container) {
    auto action = static_cast<LC_ActionSplineFromPolyline*>(m_action);

    addIntSpinbox({"a_splineDegree", tr("Spline degree"), tr("Defines degree of the spline")}, [action]() {
                      return action->getSplineDegree();
                  }, [action](int val) {
                      action->setSplineDegree(val);
                  }, container, 1, 3);

    addBoolean({
                   "a_vertexIsFitPoint",
                   tr("Fit points"),
                   tr("If checked, polyline vertexes are considered as spline points and spline by points will be created. "
                       "Otherwise, they are considered as control points and ordinary spline will be created")
               }, [action]()-> bool {
                   return action->isUseFitPoints();
               }, [action](bool val)-> void {
                   action->setUseFitPoints(val);
               }, container, [action](LC_PropertyViewDescriptor& d) -> bool {
                   return action->getSplineDegree() != 2;
               });

    addIntSpinbox({"a_midpoints", tr("Midle points"), tr("Amount of middle points of polyline segment that will be added to spline. ")},
                  [action]() {
                      return action->getSegmentPoints();
                  }, [action](int val) {
                      action->setSegmentPoints(val);
                  }, container);

    addBoolean({"a_keepOriginals", tr("Keep originals"), tr("If unchecked, original polyline will be removed, otherwise it will survive")},
               [action]()-> bool {
                   return action->isKeepOriginals();
               }, [action](bool val)-> void {
                   action->setKeepOriginals(val);
               }, container);

    addBoolean({
                   "a_currentAttr",
                   tr("Current attributes"),
                   tr("If checked, current attributes will be used for created entities, otherwise - original ones")
               }, [action]()-> bool {
                   return action->isUseCurrentAttributes();
               }, [action](bool val)-> void {
                   action->setUseCurrentAttributes(val);
               }, container);

    addBoolean({
                   "a_currentLayer",
                   tr("Current layer"),
                   tr("If checked, created entities will be placed on current layer, otherwise they will be placed into original layers.")
               }, [action]()-> bool {
                   return action->isUseCurrentLayer();
               }, [action](bool val)-> void {
                   action->setUseCurrentLayer(val);
               }, container);
}
