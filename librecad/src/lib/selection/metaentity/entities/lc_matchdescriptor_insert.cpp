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

#include "lc_matchdescriptor_insert.h"

#include "lc_actioncontext.h"
#include "rs_block.h"
#include "rs_document.h"
#include "rs_graphic.h"
#include "rs_insert.h"

class LC_ActionContext;

void LC_MatchDescriptorInsert::init(QMap<RS2::EntityType, LC_EntityMatchDescriptor*>& map, LC_ActionContext* actionContext) {
    const auto entity = new LC_TypedEntityMatchDescriptor<RS_Insert>(tr("Insert"), RS2::EntityInsert);
    initCommonEntityAttributesProperties<RS_Insert>(entity);

    entity->addStringList("name", [](const RS_Insert* e) {
                              return e->getName();
                          }, tr("Block name"), tr("Name of the inserted block"),
                          [actionContext](QList<std::pair<QString, QVariant>>& values)-> void {
                              auto* graphic = actionContext->getDocument()->getGraphic();
                              if (graphic != nullptr) {
                                  const auto blocksList = graphic->getBlockList();
                                  for (const auto b : *blocksList) {
                                      QString name = b->getName();
                                      values.push_back({name, QVariant(name)});
                                  }
                              }
                          });

    entity->addVectorX("insertX", [](const RS_Insert* e) {
        return e->getInsertionPoint();
    }, tr("Insert X"), tr("X coordinate for block's insertion point"));

    entity->addVectorY("insertY", [](const RS_Insert* e) {
        return e->getInsertionPoint();
    }, tr("Insert Y"), tr("Y coordinate for block's insertion point"));

    entity->addAngle("angle", [](const RS_Insert* e) {
        return e->getAngle();
    }, tr("Angle"), tr("Block rotation angle"));

    entity->addDouble("scaleX", [](const RS_Insert* e) {
        return e->getScale().getX();
    }, tr("Scale X"), tr("Block X scale"));

    entity->addDouble("scaleY", [](const RS_Insert* e) {
        return e->getScale().getY();
    }, tr("Scale Y"), tr("Block Y scale"));

    entity->addInt("cols", [](const RS_Insert* e) {
        return e->getCols();
    }, tr("Columns"), tr("Columns number"));

    entity->addDouble("spacingX", [](const RS_Insert* e) {
        return e->getSpacing().getX();
    }, tr("Spacing X"), tr("Block columns spacing (by  X)"));

    entity->addInt("rows", [](const RS_Insert* e) {
        return e->getRows();
    }, tr("Rows"), tr("Rows number"));

    entity->addDouble("spacingY", [](const RS_Insert* e) {
        return e->getSpacing().getY();
    }, tr("Spacing Y"), tr("Block rows spacing (by  Y)"));

    map.insert(RS2::EntityInsert, entity);

}
