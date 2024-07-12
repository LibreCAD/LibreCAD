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
        foreach (auto e, entities) {
            e->reparent(container);
            container->addEntity(e);
        }
}
