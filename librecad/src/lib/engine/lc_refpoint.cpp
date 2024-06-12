#include "rs_painter.h"
#include "rs_graphicview.h"
#include "lc_refpoint.h"

LC_RefPoint::LC_RefPoint(RS_EntityContainer* parent,
                        const RS_Vector & d,
                        double size, int mode)
    :RS_Point(parent, RS_PointData(d)), pdsize{size}, pdmode{mode} {
    calculateBorders ();
}

RS_Entity* LC_RefPoint::clone() const {
    auto* p = new LC_RefPoint(*this);
    p->initId();
    return p;
}

RS2::EntityType LC_RefPoint::rtti() const{
    return RS2::EntityRefPoint;
}

void LC_RefPoint::draw(RS_Painter *painter, RS_GraphicView *view, double &patternOffset){
    if (painter == nullptr || view == nullptr){
        return;
    }

    int screenPDSize = determinePointSreenSize(painter, view, pdsize);

//		RS_DEBUG->print(RS_Debug::D_ERROR,"RS_Point::draw X = %f, Y = %f, PDMODE = %d, PDSIZE = %f, ScreenPDSize = %i",guiPos.x,guiPos.y,pdmode,pdsize,screenPDSize);
        RS_Vector guiPos = view->toGui(getPos());

     painter->drawPoint(guiPos, pdmode, screenPDSize);

}
