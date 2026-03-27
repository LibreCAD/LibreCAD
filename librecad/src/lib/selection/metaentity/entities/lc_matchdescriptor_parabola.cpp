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

#include "lc_matchdescriptor_parabola.h"

#include "lc_parabola.h"

void LC_MatchDescriptorParabola::init(QMap<RS2::EntityType, LC_EntityMatchDescriptor*>& map) {
    const auto entity = new LC_TypedEntityMatchDescriptor<LC_Parabola>(tr("Parabola"), RS2::EntityParabola);
    initCommonEntityAttributesProperties<LC_Parabola>(entity);

    entity->addVectorX("focusX", [](const LC_Parabola* e) {
        return e->getFocus();
    }, tr("Focus X"), tr("X coordinate for parabola focus point"));

    entity->addVectorY("focusY", [](const LC_Parabola* e) {
        return e->getFocus();
    }, tr("Focus Y"), tr("Y coordinate for parabola focus point"));

    entity->addVectorX("vertexX", [](const LC_Parabola* e) {
        return e->getVertex();
    }, tr("Vertex X"), tr("X coordinate for parabola vertex point"));

    entity->addVectorY("vertexY", [](const LC_Parabola* e) {
        return e->getVertex();
    }, tr("Vertex Y"), tr("Y coordinate for parabola vertex point"));

    entity->addAngle("angle", [](LC_Parabola* e) {
        return e->getData().m_axis.angle();
    }, tr("Axis Angle"), tr("Angle of parabola axis"));

    entity->addLength("length", [](const LC_Parabola* e) {
        return e->getLength();
    }, tr("Length"), tr("Length of parabola"));

    map.insert(RS2::EntityParabola, entity);
}
