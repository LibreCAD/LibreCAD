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

#include "lc_matchdescriptor_polyline.h"

#include "rs_polyline.h"

void LC_MatchDescriptorPolyline::init(QMap<RS2::EntityType, LC_EntityMatchDescriptor*>& map) {
        const auto entity = new LC_TypedEntityMatchDescriptor<RS_Polyline>(tr("Polyline"), RS2::EntityPolyline);
    initCommonEntityAttributesProperties<RS_Polyline>(entity);
    entity->addVectorX("startX", [](const RS_Polyline* e) {
        return e->getStartpoint();
    }, tr("Start X"), tr("X coordinate for start point"));

    entity->addVectorY("startY", [](const RS_Polyline* e) {
        return e->getStartpoint();
    }, tr("Start Y"), tr("Y coordinate for start point"));

    entity->addVectorX("endX", [](const RS_Polyline* e) {
        return e->getEndpoint();
    }, tr("End X"), tr("X coordinate for end point"));

    entity->addVectorY("endY", [](const RS_Polyline* e) {
        return e->getEndpoint();
    }, tr("End Y"), tr("Y coordinate for end point"));

    entity->addContainsXInList<QList<RS_Vector>>("vertexX", [](const RS_Polyline* e) {
        return e->getVertexes();
    }, tr("Vertex X"), tr("X coordinate for one of vertexes"));

    entity->addContainsYInList<QList<RS_Vector>>("vertexY", [](const RS_Polyline* e) {
        return e->getVertexes();
    }, tr("Vertex Y"), tr("Y coordinate  for one of vertexes"));

    entity->addLength("length", [](const RS_Polyline* e) {
        return e->getLength();
    }, tr("Length"), tr("Length of polyline"));

    entity->addBoolean("closed", [](const RS_Polyline* e) {
        return e->isClosed();
    }, tr("Is Closed"), tr("Determines whether polyline is closed or not"));

    entity->addInt("segmentsCount", [](const RS_Polyline* e) {
        return e->count();
    }, tr("Segments Count"), tr("Amount of polyline's segment"));

    entity->addBoolean("hasArc", [](const RS_Polyline* e) {
        return e->containsArc();
    }, tr("Contains Arc"), tr("Defines whether polyline include arc"));

    map.insert(RS2::EntityPolyline, entity);
}
