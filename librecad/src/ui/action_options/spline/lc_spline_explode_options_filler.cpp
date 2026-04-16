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

#include "lc_spline_explode_options_filler.h"

#include "lc_action_spline_modify_explode.h"

void LC_SplineExplodeOptionsFiller::fillToolOptionsContainer(LC_PropertyContainer* container) {
    auto action = static_cast<LC_ActionSplineExplode*>(m_action);

    addBoolean({
                   "a_hasCustomSegmentCount",
                   tr("Use custom segments"),
                   tr(
                       "If checked, it is possible to specify custom amount of line segments. Otherwise, the value from settings for current drawing will be used.")
               }, [action]()-> bool {
                   return action->isUseCustomSegmentsCount();
               }, [action](bool val)-> void {
                   action->setUseCustomSegmentsCount(val);
               }, container);

    addIntSpinbox(
        {"a_segmentsCount", tr("Segments number"), tr("Defines the number of line segments to be generated for each spline-fit polyline")},
        [action]() { return action->getCustomSegmentsCount(); }, [action](int val) { action->setSegmentsCountValue(val); }, container, 1,
        -1, [action](LC_PropertyViewDescriptor&) -> bool { return !action->isUseCustomSegmentsCount(); });

    addBoolean({
                   "a_toPolyline",
                   tr("To polyline"),
                   tr("If checked, line segments will be part of polyline. Otherwise, they will be individual lines")
               }, [action]()-> bool {
                   return action->isToPolyline();
               }, [action](bool val)-> void {
                   action->setUsePolyline(val);
               }, container);

    addBoolean({"a_keepOriginals", tr("Keep originals"), tr("If unchecked, original spline will be removed, otherwise it will survive")},
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
