#ifndef LC_REFCIRCLE_H
#define LC_REFCIRCLE_H

#include "rs_circle.h"

class LC_RefCircle :public RS_Circle{
public:
    LC_RefCircle(RS_EntityContainer *parent, const RS_Vector &center, double radius);
    RS2::EntityType rtti() const override;
    RS_Entity *clone() const override;
};

#endif // LC_REFCIRCLE_H
