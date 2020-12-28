#include <iostream>
#include <cmath>
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

/**
 * Color distance
 *
 * Calculate distance between two RGB colors using low-cost approximation of
 * human eye response by Thiadmer Riemersma. Formula explanation found here:
 * https://www.compuphase.com/cmetric.htm
 *
 * @author Thiadmer Riemersma
 * @author Jeremy Ruhland
 *
 * @param c Color to perform comparison against
 *
 * @return Distance between colors, a unitless value ranging from 0 (identical)
 *         to 1 (maximum difference)
 */
double RS_Color::colorDistance(const RS_Color& c) const {
    double redMean, rDiff, gDiff, bDiff, cDist;

    redMean = (((double) c.red()) + ((double) red())) / 2;

    rDiff = ((double) c.red()) - ((double) red());
    gDiff = ((double) c.green()) - ((double) green());
    bDiff = ((double) c.blue()) - ((double) blue());

    cDist = std::sqrt((((512 + redMean) * std::pow(rDiff, 2)) / 256) + 4 * std::pow(gDiff, 2) + (((767 - redMean) * std::pow(bDiff, 2)) / 256));

    // Convert difference value to percentage using maximum color difference
    return (cDist / 764.834);
}

std::ostream& operator << (std::ostream& os, const RS_Color& c) {
       os << " color: " << c.name().toLatin1().data()
       << " flags: " << (c.getFlag(RS2::FlagByLayer) ? "RS2::FlagByLayer " : "")
       << (c.getFlag(RS2::FlagByBlock) ? "RS2::FlagByBlock " : "");
       return os;
}
