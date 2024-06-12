#ifndef LC_REFLINE_H
#define LC_REFLINE_H

#include "rs_line.h"

class LC_RefLine:public RS_Line
{
public:
    LC_RefLine(RS_EntityContainer *parent, const RS_Vector &pStart, const RS_Vector &pEnd);
    RS2::EntityType rtti() const override;
    RS_Entity *clone() const override;
};

#endif // LC_REFLINE_H
