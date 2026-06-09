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

#include "lc_matchdescriptor_splinepoints.h"

#include "lc_splinepoints.h"

class LC_SplinePoints;

void LC_MatchDescriptorSplinePoints::init(QMap<RS2::EntityType, LC_EntityMatchDescriptor*>& map) {
    const auto entity = new LC_TypedEntityMatchDescriptor<LC_SplinePoints>(tr("Spline By Points"), RS2::EntitySplinePoints);
    initCommonEntityAttributesProperties<LC_SplinePoints>(entity);

    entity->addInt("numberOfPoints", [](const LC_SplinePoints* e) {
        return static_cast<int>(e->getNumberOfControlPoints());
    }, tr("Points Amount"), tr("Amount of spline points"));

    entity->addBoolean("closed", [](const LC_SplinePoints* e) {
        return e->isClosed();
    }, tr("Closed"), tr("Determines whether spline is closed or not"));

    entity->addBoolean("useControlPoints", [](LC_SplinePoints* e) {
        return e->getData().useControlPoints;
    }, tr("By Control Points"), tr("Determines whether spline is controlled by control points or fit points"));

    entity->addContainsXInList<std::vector<RS_Vector>>("pointX", [](const LC_SplinePoints* e) {
        return e->getPoints();
    }, tr("Point X"), tr("X coordinate of spline point"));

    entity->addContainsYInList<std::vector<RS_Vector>>("pointY", [](const LC_SplinePoints* e) {
        return e->getPoints();
    }, tr("Point Y"), tr("Y coordinate of spline point"));

    entity->addContainsXInList<std::vector<RS_Vector>>("controlPointX", [](const LC_SplinePoints* e) {
        return e->getControlPoints();
    }, tr("Vertex X"), tr("X coordinate for one of vertexes"));

    entity->addContainsYInList<std::vector<RS_Vector>>("vertexY", [](const LC_SplinePoints* e) {
        return e->getControlPoints();
    }, tr("Vertex Y"), tr("Y coordinate  for one of vertexes"));

    entity->addBoolean("length", [](const LC_SplinePoints* e) {
        return e->getLength();
    }, tr("Length"), tr("Length of spline"));

    map.insert(RS2::EntitySplinePoints, entity);
}
