#include <iostream>
#include "rs_color.h"

std::ostream& operator << (std::ostream& os, const RS_Color& c) {
       os << " color: " << c.name().toLatin1().data()
       << " flags: " << (c.getFlag(RS2::FlagByLayer) ? "RS2::FlagByLayer " : "")
       << (c.getFlag(RS2::FlagByBlock) ? "RS2::FlagByBlock " : "");
       return os;
}
