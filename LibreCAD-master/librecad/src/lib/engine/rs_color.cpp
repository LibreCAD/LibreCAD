#include <iostream>
#include "rs_color.h"

//This method is used for plugins
int RS_Color::toIntColor(void) const {
    if (isByLayer())
        return -1;
    if (isByBlock())
        return -2;
//    int tmp1 = red() << 16;
//    int tmp2 = green() << 8;
//    int tmp3 = tmp1+tmp2+blue();
    int cd = (red() << 16) + (green() << 8) + blue();
        return cd;

}

//This method is used for plugins
void RS_Color::fromIntColor(int co) {
    if (co == -1)
        setFlags(RS2::FlagByLayer);
    else if (co == -2)
        setFlags(RS2::FlagByBlock);
    else {
        setRed((co >> 16) & 0xFF);
        setGreen((co >> 8) & 0xFF);
        setBlue(co & 0xFF);
    }

}

std::ostream& operator << (std::ostream& os, const RS_Color& c) {
       os << " color: " << c.name().toLatin1().data()
       << " flags: " << (c.getFlag(RS2::FlagByLayer) ? "RS2::FlagByLayer " : "")
       << (c.getFlag(RS2::FlagByBlock) ? "RS2::FlagByBlock " : "");
       return os;
}
