#include "lc_refarc.h"

LC_RefArc::LC_RefArc(RS_EntityContainer *parent, const RS_ArcData &d):RS_Arc(parent, d){}

RS2::EntityType LC_RefArc::rtti() const{
    return RS2::EntityRefArc;
}

RS_Entity *LC_RefArc::clone() const{
    auto* a = new LC_RefArc(*this);
    a->initId();
    return a;
}
