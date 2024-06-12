#include "lc_refcircle.h"


LC_RefCircle::LC_RefCircle(RS_EntityContainer *parent, const RS_Vector &center, double radius):
   RS_Circle(parent, RS_CircleData(center, radius)){}

RS2::EntityType LC_RefCircle::rtti() const{
    return RS2::EntityRefCircle;
}

RS_Entity *LC_RefCircle::clone() const{
    auto* a = new LC_RefCircle(*this);
    a->initId();
    return a;
}
