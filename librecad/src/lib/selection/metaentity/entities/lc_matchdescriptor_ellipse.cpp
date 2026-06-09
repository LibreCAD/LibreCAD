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

#include "lc_matchdescriptor_ellipse.h"

#include "rs_ellipse.h"

void LC_MatchDescriptorEllipse::init(QMap<RS2::EntityType, LC_EntityMatchDescriptor*>& map) {
    const auto entity = new LC_TypedEntityMatchDescriptor<RS_Ellipse>(tr("Ellipse"), RS2::EntityEllipse);
    initCommonEntityAttributesProperties<RS_Ellipse>(entity);
    entity->addVectorX("centerX", [](const RS_Ellipse* e) {
        return e->getCenter();
    }, tr("Center X"), tr("X coordinate for center point"));

    entity->addVectorY("centerY", [](const RS_Ellipse* e) {
        return e->getCenter();
    }, tr("Center Y"), tr("Y coordinate for center point"));

    entity->addLength("radiusMajor", [](const RS_Ellipse* e) {
        return e->getMajorRadius();
    }, tr("Major Radius"), tr("Major radius of ellipse"));

    entity->addLength("radiusMinor", [](const RS_Ellipse* e) {
        return e->getMinorRadius();
    }, tr("Minor Radius"), tr("Minor radius of ellipse"));

    entity->addLength("ratio", [](const RS_Ellipse* e) {
        return e->getRatio();
    }, tr("Ratio"), tr("Ratio of ellipse axes"));

    entity->addAngle("angle", [](const RS_Ellipse* e) {
        return e->getAngle();
    }, tr("Angle"), tr("Angle of the ellipse major axis"));

    entity->addLength("circumference", [](const RS_Ellipse* e) {
        return e->getLength();
    }, tr("Circumference", "ellipse_or_arc"), tr("Circumference of the ellipse"));

    entity->addLength("area", [](const RS_Ellipse* e) {
        return e->areaLineIntegral();
    }, tr("Area"), tr("Area"));

    entity->addBoolean("isArc", [](const RS_Ellipse* e) {
        return e->isEllipticArc();
    }, tr("Is Arc"), tr("Whether ellipse is elliptic arc"));

    entity->addVectorX("startX", [](const RS_Ellipse* e) {
        return e->getStartpoint();
    }, tr("Arc Start X"), tr("X coordinate for start point"));

    entity->addVectorY("startY", [](const RS_Ellipse* e) {
        return e->getStartpoint();
    }, tr("Arc Start Y"), tr("Y coordinate for start point"));

    entity->addVectorX("endX", [](const RS_Ellipse* e) {
        return e->getEndpoint();
    }, tr("Arc End X"), tr("X coordinate for end point"));

    entity->addVectorY("endY", [](const RS_Ellipse* e) {
        return e->getEndpoint();
    }, tr("Arc End Y"), tr("Y coordinate for end point"));

    entity->addAngle("angleLen", [](const RS_Ellipse* e) {
        return e->getAngleLength();
    }, tr("Arc Angle Length"), tr("Angle length for arc"));

    entity->addAngle("angle1", [](const RS_Ellipse* e) {
        return e->getAngle1();
    }, tr("Arc Start Angle"), tr("Start angle of arc"));

    entity->addAngle("angle2", [](const RS_Ellipse* e) {
        return e->getAngle2();
    }, tr("Arc End Angle"), tr("End angle of arc"));

    map.insert(RS2::EntityEllipse, entity);
}
