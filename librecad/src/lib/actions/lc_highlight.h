#ifndef LC_HIGHLIGHT_H
#define LC_HIGHLIGHT_H

#include <QMap>
#include "rs_entitycontainer.h"

class LC_Highlight: public RS_EntityContainer{

public:
    LC_Highlight();
    void addEntity([[maybe_unused]]RS_Entity *entity) override {};
    void addEntity(RS_Entity *entity, bool selected = false);
    bool removeEntity(RS_Entity *entity) override;
    void clear() override;
    void addEntitiesToContainer(RS_EntityContainer* container);
protected:
   QMap<RS_Entity*, RS_Entity*> entitiesMap;

};

#endif // LC_HIGHLIGHT_H
