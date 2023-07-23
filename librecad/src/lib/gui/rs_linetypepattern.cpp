/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
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


#include <cmath>
#include <map>

#include "rs_linetypepattern.h"

#include "rs.h"

namespace {
//define all line patterns in pixels
const RS_LineTypePattern patternSolidLine={10.0};

const RS_LineTypePattern patternDotLineTiny{{0.15, -1.}};
const RS_LineTypePattern patternDotLine{{0.2, -6.2}};
const RS_LineTypePattern patternDotLine2{{0.2, -3.1}};
const RS_LineTypePattern patternDotLineX2{{0.2, -12.4}};

const RS_LineTypePattern patternDashLineTiny{{2., -1.}};
const RS_LineTypePattern patternDashLine{{12.0, -6.0}};
const RS_LineTypePattern patternDashLine2{{6.0, -3.0}};
const RS_LineTypePattern patternDashLineX2{{24.0, -12.0}};

const RS_LineTypePattern patternDashDotLineTiny{{2., -2., 0.15, -2.}};
const RS_LineTypePattern patternDashDotLine{{12.0, -5., 0.2, -5.}};
const RS_LineTypePattern patternDashDotLine2{{6.0, -2.5, 0.2, -2.5}};
const RS_LineTypePattern patternDashDotLineX2{{24.0, -8., 0.2, -8.}};

const RS_LineTypePattern patternDivideLineTiny{{2., -0.7, 0.15, -0.7, 0.15, -0.7}};
const RS_LineTypePattern patternDivideLine{{12.0, -4.9, 0.2, -4.9, 0.2, -4.9}};
const RS_LineTypePattern patternDivideLine2{{6.0, -1.9, 0.2, -1.9, 0.2, -1.9}};
const RS_LineTypePattern patternDivideLineX2{{24.0, -8., 0.2, -8., 0.2, -8.}};

const RS_LineTypePattern patternCenterLineTiny{{5., -1., 1., -1.}};
const RS_LineTypePattern patternCenterLine{{32.0, -6.0, 6.0, -6.0}};
const RS_LineTypePattern patternCenterLine2{{16.0, -3.0, 3.0, -3.0}};
const RS_LineTypePattern patternCenterLineX2{{64.0, -12.0, 12.0, -12.0}};

const RS_LineTypePattern patternBorderLineTiny{{2., -1., 2., -1., 0.15, -1.}};
const RS_LineTypePattern patternBorderLine{{12.0, -4.0, 12.0, -4., 0.2, -4.}};
const RS_LineTypePattern patternBorderLine2{{6.0, -3.0, 6.0, -3., 0.2, -3.}};
const RS_LineTypePattern patternBorderLineX2{{24.0, -8.0, 24.0, -8., 0.2, -8.}};

const RS_LineTypePattern patternBlockLine{{0.5, -0.5}};
const RS_LineTypePattern patternSelected{{1.0, -3.0}};
}

RS_LineTypePattern::RS_LineTypePattern(std::initializer_list<double> const& pattern):
	pattern(pattern)
    , num { pattern.size()}
{
    for(double l: pattern){
        totalLength += std::abs(l);
    }
}


const RS_LineTypePattern* RS_LineTypePattern::getPattern(RS2::LineType lineType)
{
    static std::map<RS2::LineType, const RS_LineTypePattern*> lineTypeToPattern = {
            {RS2::NoPen, &patternSolidLine},
            {RS2::SolidLine, &patternSolidLine},
            {RS2::DotLine, &patternDotLine},
            {RS2::DotLineTiny, &patternDotLineTiny},
            {RS2::DotLine2, &patternDotLine2},
            {RS2::DotLineX2, &patternDotLineX2},
            {RS2::DashLine, &patternDashLine},
            {RS2::DashLineTiny, &patternDashLineTiny},
            {RS2::DashLine2, &patternDashLine2},
            {RS2::DashLineX2, &patternDashLineX2},
            {RS2::DashDotLine, &patternDashDotLine},
            {RS2::DashDotLineTiny, &patternDashDotLineTiny},
            {RS2::DashDotLine2, &patternDashDotLine2},
            {RS2::DashDotLineX2, &patternDashDotLineX2},
            {RS2::DivideLine, &patternDivideLine},
            {RS2::DivideLineTiny, &patternDivideLineTiny},
            {RS2::DivideLine2, &patternDivideLine2},
            {RS2::DivideLineX2, &patternDivideLineX2},
            {RS2::CenterLine, &patternCenterLine},
            {RS2::CenterLineTiny, &patternCenterLineTiny},
            {RS2::CenterLine2, &patternCenterLine2},
            {RS2::CenterLineX2, &patternCenterLineX2},
            {RS2::BorderLine, &patternBorderLine},
            {RS2::BorderLineTiny, &patternBorderLineTiny},
            {RS2::BorderLine2, &patternBorderLine2},
            {RS2::BorderLineX2, &patternBorderLineX2},
            {RS2::LineByLayer, &patternBlockLine},
            {RS2::LineByBlock, &patternBlockLine},
            {RS2::LineSelected, &patternSelected}
            };
    if (lineTypeToPattern.count(lineType) == 0)
        return nullptr;
    return lineTypeToPattern[lineType];
}

