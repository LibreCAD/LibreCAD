/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2025 LibreCAD.org
 * Copyright (C) 2025 sand1024
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


#include "lc_matchdescriptor_spline.h"

#include "rs_spline.h"

void LC_MatchDescriptorSpline::init(QMap<RS2::EntityType, LC_EntityMatchDescriptor*>& map) {
    const auto entity = new LC_TypedEntityMatchDescriptor<RS_Spline>(tr("Spline"), RS2::EntitySpline);
    initCommonEntityAttributesProperties<RS_Spline>(entity);

    entity->addInt("degree", [](const RS_Spline* e) {
        return e->getDegree();
    }, tr("Degree"), tr("Spline degree"));

    entity->addBoolean("closed", [](const RS_Spline* e) {
        return e->isClosed();
    }, tr("Closed"), tr("Determines whether spline is closed or not"));

    entity->addInt("points", [](const RS_Spline* e) {
       return static_cast<int>(e->getControlPoints().size());
    }, tr("Points"), tr("Number of spline's control points"));

    entity->addContainsXInList<std::vector<RS_Vector>>("controlPointX", [](const RS_Spline* e) {
        return e->getControlPoints();
    }, tr("Vertex X"), tr("X coordinate for one of vertexes"));

    entity->addContainsYInList<std::vector<RS_Vector>>("vertexY", [](const RS_Spline* e) {
        return e->getControlPoints();
    }, tr("Vertex Y"), tr("Y coordinate  for one of vertexes"));

    entity->addBoolean("length", [](const RS_Spline* e) {
        return e->getLength();
    }, tr("Length"), tr("Length of spline"));

    map.insert(RS2::EntitySpline, entity);
}
