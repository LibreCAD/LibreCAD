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

#include "lc_matchdescriptor_dimaligned.h"

#include "rs_dimaligned.h"

void LC_MatchDescriptorDimAligned::init(QMap<RS2::EntityType, LC_EntityMatchDescriptor*>& map, LC_ActionContext *actionContext) {
    const auto entity = new LC_DimensionEntityMatchDescriptor<RS_DimAligned>(tr("Dimension Aligned"), RS2::EntityDimAligned);
    initCommonEntityAttributesProperties<RS_DimAligned>(entity);
    initCommonDimensionAttributes(entity, actionContext);

    entity->addVectorX("defX", [](const RS_DimAligned* e) {
     return e->getDefinitionPoint();
 }, tr("Definition Point X"), tr("X coordinate for definition point"));

    entity->addVectorY("defY", [](const RS_DimAligned* e) {
        return e->getDefinitionPoint();
    }, tr("Definition Point Y"), tr("Y coordinate for definition point"));

    entity->addVectorX("ext1X", [](const RS_DimAligned* e) {
        return e->getExtensionPoint1();
    }, tr("First Extension Point X"), tr("X coordinate for first extension point"));

    entity->addVectorY("ext1Y", [](const RS_DimAligned* e) {
        return e->getExtensionPoint1();
    }, tr("First Extension Point Y"), tr("Y coordinate for first extension point"));

    entity->addVectorX("ext2X", [](const RS_DimAligned* e) {
        return e->getExtensionPoint2();
    }, tr("Second Extension Point X"), tr("X coordinate for second extension point"));

    entity->addVectorY("ext2Y", [](const RS_DimAligned* e) {
        return e->getExtensionPoint2();
    }, tr("Second Extension Point Y"), tr("Y coordinate for second extension point"));

    entity->addVectorX("textMiddleX", [](const RS_DimAligned* e) {
        return e->getMiddleOfText();
    }, tr("Text middle point X"), tr("X coordinate for text middle point"));

    entity->addVectorY("textMiddleX", [](const RS_DimAligned* e) {
        return e->getMiddleOfText();
    }, tr("Text middle point Y"), tr("Y coordinate for text middle point"));

    map.insert(RS2::EntityDimAligned, entity);

}
