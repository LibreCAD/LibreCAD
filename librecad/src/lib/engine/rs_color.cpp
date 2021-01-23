/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2020 A. Stebich (librecad@mail.lordofbikes.de)
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
**
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software
** Foundation and appearing in the file gpl-2.0.txt included in the
** packaging of this file.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
**
** This copyright notice MUST APPEAR in all copies of the script!
**
**********************************************************************/


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
 * @return Distance between colors in percent, value ranging from 0 (identical)
 *         to 100 (maximum difference)
 */
int RS_Color::colorDistance(const RS_Color& c) const {

    int myRed {red()};
    int otherRed {c.red()};
    int redMean {(myRed + otherRed) / 2};

    // Convert difference value to percentage using maximum color difference (764.834 / 100)
    return std::lround( std::sqrt( std::pow(otherRed - myRed, 2) * (512 + redMean) / 256
                                   + std::pow(c.green() - green(), 2) * 4
                                   + std::pow(c.blue() - blue(), 2) * (767 - redMean) / 256)
                        / 7.64834);
}

std::ostream& operator << (std::ostream& os, const RS_Color& c) {
       os << " color: " << c.name().toLatin1().data()
       << " flags: " << (c.getFlag(RS2::FlagByLayer) ? "RS2::FlagByLayer " : "")
       << (c.getFlag(RS2::FlagByBlock) ? "RS2::FlagByBlock " : "");
       return os;
}
