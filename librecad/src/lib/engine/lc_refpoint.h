#ifndef LC_REFPOINT_H
#define LC_REFPOINT_H

#include "rs_point.h"

class LC_RefPoint:public RS_Point{
public:
    LC_RefPoint(RS_EntityContainer *parent, const RS_Vector &d, double size, int mode);
    RS2::EntityType rtti() const override;
    RS_Entity *clone()  const override;
    void draw(RS_Painter *painter, RS_GraphicView *view, double &patternOffset) override;
private:
    int pdmode;
    double pdsize;
};

#endif // LC_REFPOINT_H
