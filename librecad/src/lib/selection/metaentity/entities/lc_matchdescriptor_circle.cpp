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

#include "lc_matchdescriptor_circle.h"

#include "rs_circle.h"

void LC_MatchDescriptorCircle::init(QMap<RS2::EntityType, LC_EntityMatchDescriptor*>& map) {
    const auto entity = new LC_TypedEntityMatchDescriptor<RS_Circle>(tr("Circle"), RS2::EntityCircle);
    initCommonEntityAttributesProperties<RS_Circle>(entity);
    entity->addVectorX("centerX", [](const RS_Circle* e) {
        return e->getCenter();
    }, tr("Center X"), tr("X coordinate for center point"));

    entity->addVectorY("centerY", [](const RS_Circle* e) {
        return e->getCenter();
    }, tr("Center Y"), tr("Y coordinate for center point"));

    entity->addLength("radius", [](const RS_Circle* e) {
        return e->getRadius();
    }, tr("Radius"), tr("Radius of circle"));

    entity->addLength("diameter", [](const RS_Circle* e) {
        return e->getRadius() * 2.0;
    }, tr("Diameter"), tr("Diameter of circle"));

    entity->addLength("circumference", [](const RS_Circle* e) {
        return e->getRadius() * 2.0 * M_PI;
    }, tr("Circumference", "circle"), tr("Circumference of circle"));

    entity->addLength("area", [](const RS_Circle* e) {
        const double radius = e->getRadius();
        return M_PI * radius * radius;
    }, tr("Area"), tr("Area of circle"));

    map.insert(RS2::EntityCircle, entity);
}
