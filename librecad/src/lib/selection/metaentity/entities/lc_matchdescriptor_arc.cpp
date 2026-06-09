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

#include "lc_matchdescriptor_arc.h"

#include "rs_arc.h"

void LC_MatchDescriptorArc::init(QMap<RS2::EntityType, LC_EntityMatchDescriptor*>& map) {
    const auto entity = new LC_TypedEntityMatchDescriptor<RS_Arc>(tr("Arc"), RS2::EntityArc);
    initCommonEntityAttributesProperties<RS_Arc>(entity);

    entity->addVectorX("centerX", [](const RS_Arc* e) {
        return e->getCenter();
    }, tr("Center X"), tr("X coordinate for center point"));

    entity->addVectorY("centerY", [](const RS_Arc* e) {
        return e->getCenter();
    }, tr("Center Y"), tr("Y coordinate for center point"));

    entity->addLength("radius", [](const RS_Arc* e) {
        return e->getRadius();
    }, tr("Radius"), tr("Radius of arc"));

    entity->addBoolean("reversed", [](const RS_Arc* e) {
        return e->isReversed();
    }, tr("Is Reversed"), tr("Clockwise direction if reversed, counterclockwise otherwise"));

    entity->addLength("diameter", [](const RS_Arc* e) {
        return e->getRadius() * 2.0;
    }, tr("Diameter"), tr("Diameter of arc"));

    entity->addVectorX("startX", [](const RS_Arc* e) {
        return e->getStartpoint();
    }, tr("Start X"), tr("X coordinate for start point"));

    entity->addVectorY("startY", [](const RS_Arc* e) {
        return e->getStartpoint();
    }, tr("Start Y"), tr("Y coordinate for start point"));

    entity->addVectorX("endX", [](const RS_Arc* e) {
        return e->getEndpoint();
    }, tr("End X"), tr("X coordinate for end point"));

    entity->addVectorY("endY", [](const RS_Arc* e) {
        return e->getEndpoint();
    }, tr("End Y"), tr("Y coordinate for end point"));

    entity->addAngle("angleLen", [](const RS_Arc* e) {
        return e->getAngleLength();
    }, tr("Angle Length"), tr("Angle length for arc"));

    entity->addLength("circumference", [](const RS_Arc* e) {
        return e->getLength();
    }, tr("Circumference", "arc"), tr("Circumference of arc"));

    entity->addLength("chord", [](const RS_Arc* e) {
        return e->getStartpoint().distanceTo(e->getEndpoint());
    }, tr("Chord"), tr("Chord length (distance from start to end point)"));

    entity->addLength("sagitta", [](const RS_Arc* e) {
        return e->getSagitta();
    }, tr("Sagitta"), tr("Sagitta of the arc"));

    entity->addAngle("angle1", [](const RS_Arc* e) {
        return e->getAngle1();
    }, tr("Start Angle"), tr("Start angle of arc"));

    entity->addAngle("angle2", [](const RS_Arc* e) {
        return e->getAngle2();
    }, tr("End Angle"), tr("End angle of arc"));

    entity->addLength("bulge", [](const RS_Arc* e) {
        return e->getBulge();
    }, tr("Bulge"), tr("Bulge of arc "));

    map.insert(RS2::EntityArc, entity);
}
