#include "lc_refellipse.h"

LC_RefEllipse::LC_RefEllipse(RS_EntityContainer *parent, const RS_EllipseData &d):RS_Ellipse(parent, d) {}

RS2::EntityType LC_RefEllipse::rtti() const{
    return RS2::EntityRefEllipse;
}

RS_Entity *LC_RefEllipse::clone() const{
    auto* a = new LC_RefEllipse(*this);
    a->initId();
    return a;
}
