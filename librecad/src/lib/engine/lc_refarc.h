#ifndef LC_REFARC_H
#define LC_REFARC_H

#include "rs_arc.h"

class LC_RefArc :public RS_Arc
{
public:
    LC_RefArc(RS_EntityContainer *parent, const RS_ArcData &d);
    RS2::EntityType rtti() const override;
    RS_Entity *clone() const override;
};

#endif // LC_REFARC_H
