#include "lc_refline.h"

LC_RefLine::LC_RefLine(RS_EntityContainer *parent, const RS_Vector &pStart, const RS_Vector &pEnd):RS_Line(parent, pStart, pEnd){}

RS2::EntityType LC_RefLine::rtti() const{
    return RS2::EntityRefLine;
}

RS_Entity *LC_RefLine::clone() const{
    auto* l = new LC_RefLine(*this);
    l->initId();
    return l;
}


