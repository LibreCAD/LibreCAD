/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2024 LibreCAD.org
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

#include<climits>
#include<map>
#include<vector>
#include<utility>
#include "rs.h"

namespace {
std::map<int, RS2::LineWidth> constructInt2LineWidth() {
    using namespace RS2;
    return  {
        {-3, WidthUnchanged},
        {-2, WidthDefault}, //for w = -3
        {-1, WidthByBlock}, //for w = -2
        {0, WidthByLayer}, //for w = -1
        // for w < 3, return Width00
        {3, Width00},
        {8, Width01},
        {12, Width02},
        {14, Width03},
        {17, Width04},
        {19, Width05},
        {23, Width06},
        {28, Width07},
        {33, Width08},
        {38, Width09},
        {46, Width10},
        {52, Width11},
        {57, Width12},
        {66, Width13},
        {76, Width14},
        {86, Width15},
        {96, Width16},
        {104, Width17},
        {114, Width18},
        {131, Width19},
        {150, Width20},
        {180, Width21},
        {206, Width22},
        {INT_MAX, Width23}
    };
}

const std::map<int, RS2::LineWidth> g_int2LineWidth = constructInt2LineWidth();
}

RS2::LineWidth RS2::intToLineWidth(int w) {
    // for w < 3, return Width00
    if (w <= 2)
        return Width00;
    auto it = g_int2LineWidth.find(w);
    return (it != g_int2LineWidth.cend()) ? it->second : Width00;
}
