/*******************************************************************************
 *
 This file is part of the LibreCAD project, a 2D CAD program

 Copyright (C) 2024 LibreCAD.org
 Copyright (C) 2024 sand1024

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 ******************************************************************************/

#include "lc_highlight.h"
#include "rs_entity.h"

LC_Highlight::LC_Highlight()= default;

void LC_Highlight::addEntity(RS_Entity* entity, bool selected) {
    if (entity == nullptr || entity->isUndone()) {
        return;
    }
    RS_Entity *duplicatedEntity = entity->clone();
    RS_Pen pen = entity->getPen(true);
    duplicatedEntity->setPen(pen);

    duplicatedEntity->setHighlighted(true);
    if (selected) {
        duplicatedEntity->setSelected(true);
    }

    entitiesMap.insert(entity, duplicatedEntity);
//    entity->setTransparent(true);
    entities.append(duplicatedEntity);
}

bool LC_Highlight::removeEntity(RS_Entity *entity){
    bool result = false;
    if (entity != nullptr){
        RS_Entity *duplicate = entitiesMap.value(entity, nullptr);
        if (duplicate != nullptr){
            entity->setTransparent(false);
            bool ret = entities.removeOne(duplicate);
            if (ret) {
                delete duplicate;
            }
            entitiesMap.remove(entity);
            if (entity->getParent() == this){
                delete entity;
            }
            result = true;
        }
    }
    return result;
}
// fixme - return bool value if actually cleared
void LC_Highlight::clear(){
    for (auto it = entitiesMap.keyValueBegin(); it != entitiesMap.keyValueEnd(); ++it) {
        RS_Entity *entity = it->first;
        entity->setTransparent(false);
        if (entity->getParent() == this){
            delete entity;
        }
    }
    entitiesMap.clear();
    while (!entities.isEmpty()) {
        delete entities.takeFirst();
    }
}

void LC_Highlight::addEntitiesToContainer(RS_EntityContainer *container){
    // fixme - sand - review foreach cycles and replace to range-based
    for (const auto e: std::as_const(entities)) {
        e->reparent(container);
        container->addEntity(e);
    }
}
