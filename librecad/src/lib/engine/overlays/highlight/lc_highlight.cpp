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
#include "rs_debug.h"
#include "rs_pen.h"

LC_Highlight::LC_Highlight()= default;

void LC_Highlight::addEntity(const RS_Entity* entity, const bool selected) {
    if (entity == nullptr || entity->isDeleted()) {
        return;
    }
    RS_Entity *clone = entity->clone();
    // fixme - sand - review this, probably it's better to return proxy on highlight. That might be useful for images etc,
    // fixme - sand and potentially may simplify drawing of overlay
    // RS_Entity *clone = entity->cloneProxy();
    if (entity->rtti() == RS2::EntityInsert) {
        clone->update();
    }
    const RS_Pen pen = entity->getPen(true);
    clone->setPen(pen);
    const bool inVisualSnap = entity->getFlag(RS2::FlagInVisualSnap);
    if (inVisualSnap) {
        clone->setFlag(RS2::FlagInVisualSnap);
    }
    else {
        clone->setHighlighted(true);
    }
    if (selected) {
        clone->setSelectionFlag(true);  // fixme - selection - overlay?
    }

    m_entitiesMap.insert(entity->getId(), clone);
    push_back(clone);
}

bool LC_Highlight::removeEntity(RS_Entity *entity){
    bool result = false;
    if (entity != nullptr){
        const unsigned long long entityId = entity->getId();
        RS_Entity *duplicate = m_entitiesMap.value(entityId, nullptr);
        if (duplicate != nullptr){
            const bool ret = removeEntity(duplicate);
            if (ret) {
                delete duplicate;
            }
            m_entitiesMap.remove(entityId);
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
    for (auto it = m_entitiesMap.keyValueBegin(); it != m_entitiesMap.keyValueEnd(); ++it) {
        const RS_Entity* clone = it->second;
        if (clone->getParent() == this){
            delete clone;
        }
    }
    m_entitiesMap.clear();
    while (!isEmpty()) {
        delete last();
        pop_back();
    }
}

void LC_Highlight::addEntitiesToContainer(RS_EntityContainer *container){
    // fixme - sand - review foreach cycles and replace to range-based
    for (RS_Entity* e: *this) {
        e->reparent(container);
        container->addEntity(e);
    }
}
