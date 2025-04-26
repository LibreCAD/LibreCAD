#include <iostream>
#include "rs_pen.h"


RS_Pen::RS_Pen() :
    RS_Pen(RS_Color{Qt::black}, RS2::WidthByLayer, RS2::LineByLayer)
{}

std::ostream& operator << (std::ostream& os, const RS_Pen& p) {
    //os << "style: " << p.style << std::endl;
    os << " pen color: " << p.getColor()
    << " pen width: " << p.getWidth()
    << " pen screen width: " << p.getScreenWidth()
    << " pen line type: " << p.getLineType()
    << " flags: " << (p.getFlag(RS2::FlagInvalid) ? "INVALID" : "")
    << std::endl;
    return os;
}
