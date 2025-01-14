#include "lc_ucs_mark.h"
#include "rs_graphicview.h"

LC_UCS_Mark::LC_UCS_Mark(RS_EntityContainer *parent, const RS_Vector &pos, double angle):RS_Point(parent, RS_PointData(pos)){
    m_angle = angle;
}

void LC_UCS_Mark::draw(RS_Painter *painter, RS_GraphicView *view, double &patternOffset) {
    view->drawCoordinateSystemMarker(painter,  view->toGui(getPos()), m_angle, false);
}
