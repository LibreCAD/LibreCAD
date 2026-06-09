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

#include "lc_matchdescriptor_hyperbola.h"

#include "lc_hyperbola.h"

void LC_MatchDescriptorHyperbola::init(QMap<RS2::EntityType, LC_EntityMatchDescriptor*>& map) {
    const auto entity = new LC_TypedEntityMatchDescriptor<LC_Hyperbola>(tr("Hyperbola"), RS2::EntityHyperbola);
    initCommonEntityAttributesProperties<LC_Hyperbola>(entity);
    entity->addVectorX("centerX", [](const LC_Hyperbola* e) {
        return e->getCenter();
    }, tr("Center X"), tr("X coordinate for center point"));
    entity->addVectorY("centerY", [](const LC_Hyperbola* e) {
        return e->getCenter();
    }, tr("Center Y"), tr("Y coordinate for center point"));

    entity->addVectorX("focus1X", [](const LC_Hyperbola* e) {
        return e->getFocus1();
    }, tr("Focus 1 X"), tr("X coordinate for first focus point"));
    entity->addVectorY("focus1Y", [](const LC_Hyperbola* e) {
        return e->getFocus1();
    }, tr("Focus 1 Y"), tr("Y coordinate for second focus point"));

    entity->addVectorX("focus2X", [](const LC_Hyperbola* e) {
        return e->getFocus2();
    }, tr("Focus 2 X"), tr("X coordinate for second focus point"));
    entity->addVectorY("focus2Y", [](const LC_Hyperbola* e) {
        return e->getFocus2();
    }, tr("Focus 2 Y"), tr("Y coordinate for second focus point"));

    entity->addLength("radiusMajor", [](const LC_Hyperbola* e) {
        return e->getMajorRadius();
    }, tr("Major Radius"), tr("Major radius of hyperbola"));

    entity->addLength("radiusMinor", [](const LC_Hyperbola* e) {
        return e->getMinorRadius();
    }, tr("Minor Radius"), tr("Minor radius of hyperbola"));

    entity->addLength("ratio", [](const LC_Hyperbola* e) {
        return e->getRatio();
    }, tr("Ratio"), tr("Ratio of hyperbola axes"));

    entity->addAngle("angle", [](const LC_Hyperbola* e) {
        return e->getAngle();
    }, tr("Angle"), tr("Angle of the hyperbola major axis"));

    entity->addVectorX("startX", [](const LC_Hyperbola* e) {
       return e->getStartpoint();
   }, tr("Start X"), tr("X coordinate for start point"));

    entity->addVectorY("startY", [](const LC_Hyperbola* e) {
        return e->getStartpoint();
    }, tr("Start Y"), tr("Y coordinate for start point"));

    entity->addVectorX("endX", [](const LC_Hyperbola* e) {
        return e->getEndpoint();
    }, tr("End X"), tr("X coordinate for end point"));

    entity->addVectorY("endY", [](const LC_Hyperbola* e) {
        return e->getEndpoint();
    }, tr("End Y"), tr("Y coordinate for end point"));

    entity->addAngle("angle1", [](const LC_Hyperbola* e) {
       return e->getAngle1();
   }, tr("Start Angle"), tr("Start point angle"));

    entity->addAngle("angle2", [](const LC_Hyperbola* e) {
        return e->getAngle2();
    }, tr("End Angle"), tr("End point angle"));

    entity->addDouble("eccentricity", [](const LC_Hyperbola* e) {
       return e->getEccentricity();
   }, tr("Eccentricity", "hyperbola"), tr("Eccentricity of the hyperbola"));

    entity->addLength("circumference", [](const LC_Hyperbola* e) {
       return e->getLength();
   }, tr("Circumference", "hyperbola"), tr("Circumference of the hyperbola"));

    entity->addLength("area", [](const LC_Hyperbola* e) {
        return e->areaLineIntegral();
    }, tr("Area"), tr("Area"));


    map.insert(RS2::EntityHyperbola, entity);
}
