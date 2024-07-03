#ifndef LC_REFELLIPSE_H
#define LC_REFELLIPSE_H

#include "rs_ellipse.h"

class LC_RefEllipse: public RS_Ellipse
{
public:
    LC_RefEllipse(RS_EntityContainer *parent, const RS_EllipseData &d);
    RS2::EntityType rtti() const override;
    RS_Entity *clone() const override;
};
#endif // LC_REFELLIPSE_H
