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

#include "lc_matchdescriptor_dimordinate.h"

#include "lc_dimordinate.h"

void LC_MatchDescriptorDimOrdinate::init(QMap<RS2::EntityType, LC_EntityMatchDescriptor*>& map, LC_ActionContext *actionContext) {
    const auto entity = new LC_DimensionEntityMatchDescriptor<LC_DimOrdinate>(tr("Dimension Ordinate"), RS2::EntityDimOrdinate);
    initCommonEntityAttributesProperties<LC_DimOrdinate>(entity);
    initCommonDimensionAttributes(entity, actionContext);

    entity->addVectorX("originX", [](const LC_DimOrdinate* e) {
        return e->getDefinitionPoint();
    }, tr("Origin Point X"), tr("X coordinate for dimension origin point"));

    entity->addVectorY("originY", [](const LC_DimOrdinate* e) {
        return e->getDefinitionPoint();
    }, tr("Origin Point Y"), tr("Y coordinate for dimension origin point"));

    entity->addIntChoice("ordinate", [](const LC_DimOrdinate* e) -> int {
        return e->isForXDirection() ? 1 : 0;
    }, tr("Ordinate"), tr("Direction of ordinate"), {{tr("X"), 1}, {tr("Y"), 0}});

    entity->addVectorX("featureX", [](const LC_DimOrdinate* e) {
        return e->getFeaturePoint();
    }, tr("Feature Point X"), tr("X coordinate for dimension feature point"));

    entity->addVectorY("featureY", [](const LC_DimOrdinate* e) {
        return e->getFeaturePoint();
    }, tr("Feature Point Y"), tr("Y coordinate for dimension feature point"));

    entity->addVectorX("leaderEndX", [](const LC_DimOrdinate* e) {
        return e->getLeaderEndPoint();
    }, tr("Leader End X"), tr("X coordinate for dimension leader end point"));

    entity->addVectorY("leaderEndY", [](const LC_DimOrdinate* e) {
        return e->getLeaderEndPoint();
    }, tr("Leader End Y"), tr("Y coordinate for dimension leader end point"));

    entity->addVectorX("textMiddleX", [](const LC_DimOrdinate* e) {
        return e->getMiddleOfText();
    }, tr("Text middle point X"), tr("X coordinate for text middle point"));

    entity->addVectorY("textMiddleY", [](const LC_DimOrdinate* e) {
        return e->getMiddleOfText();
    }, tr("Text middle point Y"), tr("Y coordinate for text middle point"));

    entity->addAngle("hdir", [](const LC_DimOrdinate* e) {
        return e->getHDir();
    }, tr("Horizontal"), tr("Andle of horizontal direction for this dimension"));

    map.insert(RS2::EntityDimOrdinate, entity);
}
