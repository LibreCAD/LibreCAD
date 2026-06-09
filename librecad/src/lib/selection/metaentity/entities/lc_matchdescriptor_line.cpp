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

#include "lc_matchdescriptor_line.h"

#include "rs_line.h"

void LC_MatchDescriptorLine::init(QMap<RS2::EntityType, LC_EntityMatchDescriptor*>& map) {
    const auto entity = new LC_TypedEntityMatchDescriptor<RS_Line>(tr("Line"), RS2::EntityLine);
    initCommonEntityAttributesProperties<RS_Line>(entity);
    entity->addVectorX("startX", [](const RS_Line* e) {
        return e->getStartpoint();
    }, tr("Start X"), tr("X coordinate for start point"));

    entity->addVectorY("startY", [](const RS_Line* e) {
        return e->getStartpoint();
    }, tr("Start Y"), tr("Y coordinate for start point"));

    entity->addVectorX("endX", [](const RS_Line* e) {
        return e->getEndpoint();
    }, tr("End X"), tr("X coordinate for end point"));

    entity->addVectorY("endY", [](const RS_Line* e) {
        return e->getEndpoint();
    }, tr("End Y"), tr("Y coordinate for end point"));

    entity->addVectorX("middleX", [](const RS_Line* e) {
        return e->getMiddlePoint();
    }, tr("Middle X"), tr("X coordinate for middle point"));

    entity->addVectorY("middleY", [](const RS_Line* e) {
        return e->getMiddlePoint();
    }, tr("Middle Y"), tr("Y coordinate for middle point"));

    entity->addLength("length", [](const RS_Line* e) {
        return e->getLength();
    }, tr("Length"), tr("Length of line"));

    entity->addAngle("angle1", [](const RS_Line* e) {
        return e->getAngle1();
    }, tr("Angle 1"), tr("Angle from the 0.0 to start point"));

    entity->addAngle("angle2", [](const RS_Line* e) {
        return e->getAngle2();
    }, tr("Angle 2"), tr("Angle from the 0.0 to end point"));

    entity->add<double>("incline", [](const RS_Line* e) {
        return e->getAngle1();
    }, tr("Incline Angle"), tr("Angle of the line inclination to x-axis"), LC_PropertyMatcherTypes::INCLINATION);

    entity->addLength("deltaX", [](const RS_Line* e) {
        const double dif = e->getEndpoint().getX() - e->getStartpoint().getX();
        return std::abs(dif);
    }, tr("Delta X"), tr("Horizontal distance between endpoints"));

    entity->addLength("deltaY", [](const RS_Line* e) {
        const double dif = e->getEndpoint().getY() - e->getStartpoint().getY();
        return std::abs(dif);
    }, tr("Delta Y"), tr("Vertical distance between endpoints"));

    map.insert(RS2::EntityLine, entity);
}
