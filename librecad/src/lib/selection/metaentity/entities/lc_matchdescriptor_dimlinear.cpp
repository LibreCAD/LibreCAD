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


#include "lc_matchdescriptor_dimlinear.h"

#include "rs_dimlinear.h"

void LC_MatchDescriptorDimLinear::init(QMap<RS2::EntityType, LC_EntityMatchDescriptor*>& map, LC_ActionContext *actionContext) {
        const auto entity = new LC_DimensionEntityMatchDescriptor<RS_DimLinear>(tr("Dimension Linear"), RS2::EntityDimLinear);
    initCommonEntityAttributesProperties<RS_DimLinear>(entity);
    initCommonDimensionAttributes(entity, actionContext);

    entity->addVectorX("defX", [](const RS_DimLinear* e) {
        return e->getDefinitionPoint();
    }, tr("Definition Point X"), tr("X coordinate for definition point"));

    entity->addVectorY("defY", [](const RS_DimLinear* e) {
        return e->getDefinitionPoint();
    }, tr("Definition Point Y"), tr("Y coordinate for definition point"));

    entity->addVectorX("ext1X", [](const RS_DimLinear* e) {
        return e->getExtensionPoint1();
    }, tr("First Extension Point X"), tr("X coordinate for first extension point"));

    entity->addVectorY("ext1Y", [](const RS_DimLinear* e) {
        return e->getExtensionPoint1();
    }, tr("First Extension Point Y"), tr("Y coordinate for first extension point"));

    entity->addVectorX("ext2X", [](const RS_DimLinear* e) {
        return e->getExtensionPoint2();
    }, tr("Second Extension Point X"), tr("X coordinate for second extension point"));

    entity->addVectorY("ext2Y", [](const RS_DimLinear* e) {
        return e->getExtensionPoint2();
    }, tr("Second Extension Point Y"), tr("Y coordinate for second extension point"));

    entity->addVectorX("textMiddleX", [](const RS_DimLinear* e) {
        return e->getMiddleOfText();
    }, tr("Text middle point X"), tr("X coordinate for text middle point"));

    entity->addVectorY("textMiddleY", [](const RS_DimLinear* e) {
        return e->getMiddleOfText();
    }, tr("Text middle point Y"), tr("Y coordinate for text middle point"));

    entity->addAngle("angle", [](const RS_DimLinear* e) {
        return e->getAngle();
    }, tr("Angle"), tr("Dimension rotation angle"));

    entity->addAngle("obligue", [](const RS_DimLinear* e) {
        return e->getOblique();
    }, tr("Obligue"), tr("Dimension obligue angle"));

    map.insert(RS2::EntityDimLinear, entity);

}
