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
#include "rs_linetypepattern.h"

#include <cstdlib>
#include <map>

#include "rs.h"

namespace {
//define all line patterns in pixels
const RS_LineTypePattern PATTERN_SOLID_LINE={10.0};

const RS_LineTypePattern PATTERN_DOT_LINE_TINY{{0.15, -1.}};
const RS_LineTypePattern PATTERN_DOT_LINE{{0.2, -6.2}};
const RS_LineTypePattern PATTERN_DOT_LINE2{{0.2, -3.1}};
const RS_LineTypePattern PATTERN_DOT_LINE_X2{{0.2, -12.4}};

const RS_LineTypePattern PATTERN_DASH_LINE_TINY{{2., -1.}};
const RS_LineTypePattern PATTERN_DASH_LINE{{12.0, -6.0}};
const RS_LineTypePattern PATTERN_DASH_LINE2{{6.0, -3.0}};
const RS_LineTypePattern PATTERN_DASH_LINE_X2{{24.0, -12.0}};

const RS_LineTypePattern PATTERN_DASH_DOT_LINE_TINY{{2., -2., 0.15, -2.}};
const RS_LineTypePattern PATTERN_DASH_DOT_LINE{{12.0, -5., 0.2, -5.}};
const RS_LineTypePattern PATTERN_DASH_DOT_LINE2{{6.0, -2.5, 0.2, -2.5}};
const RS_LineTypePattern PATTERN_DASH_DOT_LINE_X2{{24.0, -8., 0.2, -8.}};

const RS_LineTypePattern PATTERN_DIVIDE_LINE_TINY{{2., -0.7, 0.15, -0.7, 0.15, -0.7}};
const RS_LineTypePattern PATTERN_DIVIDE_LINE{{12.0, -4.9, 0.2, -4.9, 0.2, -4.9}};
const RS_LineTypePattern PATTERN_DIVIDE_LINE2{{6.0, -1.9, 0.2, -1.9, 0.2, -1.9}};
const RS_LineTypePattern PATTERN_DIVIDE_LINE_X2{{24.0, -8., 0.2, -8., 0.2, -8.}};

const RS_LineTypePattern PATTERN_CENTER_LINE_TINY{{5., -1., 1., -1.}};
const RS_LineTypePattern PATTERN_CENTER_LINE{{32.0, -6.0, 6.0, -6.0}};
const RS_LineTypePattern PATTERN_CENTER_LINE2{{16.0, -3.0, 3.0, -3.0}};
const RS_LineTypePattern PATTERN_CENTER_LINE_X2{{64.0, -12.0, 12.0, -12.0}};

const RS_LineTypePattern PATTERN_BORDER_LINE_TINY{{2., -1., 2., -1., 0.15, -1.}};
const RS_LineTypePattern PATTERN_BORDER_LINE{{12.0, -4.0, 12.0, -4., 0.2, -4.}};
const RS_LineTypePattern PATTERN_BORDER_LINE2{{6.0, -3.0, 6.0, -3., 0.2, -3.}};
const RS_LineTypePattern PATTERN_BORDER_LINE_X2{{24.0, -8.0, 24.0, -8., 0.2, -8.}};

const RS_LineTypePattern PATTERN_BLOCK_LINE{{0.5, -0.5}};
const RS_LineTypePattern PATTERN_SELECTED{{1.0, -3.0}};
}

RS_LineTypePattern::RS_LineTypePattern(const std::initializer_list<double>& pattern):
    pattern(pattern), num { pattern.size()}{
    for(const double l: pattern){
        totalLength += std::abs(l);
    }
}


const RS_LineTypePattern* RS_LineTypePattern::getPattern(const RS2::LineType lineType){
    static std::map<RS2::LineType, const RS_LineTypePattern*> lineTypeToPattern = {
            {RS2::NoPen, &PATTERN_SOLID_LINE},
            {RS2::SolidLine, &PATTERN_SOLID_LINE},
            {RS2::DotLine, &PATTERN_DOT_LINE},
            {RS2::DotLineTiny, &PATTERN_DOT_LINE_TINY},
            {RS2::DotLine2, &PATTERN_DOT_LINE2},
            {RS2::DotLineX2, &PATTERN_DOT_LINE_X2},
            {RS2::DashLine, &PATTERN_DASH_LINE},
            {RS2::DashLineTiny, &PATTERN_DASH_LINE_TINY},
            {RS2::DashLine2, &PATTERN_DASH_LINE2},
            {RS2::DashLineX2, &PATTERN_DASH_LINE_X2},
            {RS2::DashDotLine, &PATTERN_DASH_DOT_LINE},
            {RS2::DashDotLineTiny, &PATTERN_DASH_DOT_LINE_TINY},
            {RS2::DashDotLine2, &PATTERN_DASH_DOT_LINE2},
            {RS2::DashDotLineX2, &PATTERN_DASH_DOT_LINE_X2},
            {RS2::DivideLine, &PATTERN_DIVIDE_LINE},
            {RS2::DivideLineTiny, &PATTERN_DIVIDE_LINE_TINY},
            {RS2::DivideLine2, &PATTERN_DIVIDE_LINE2},
            {RS2::DivideLineX2, &PATTERN_DIVIDE_LINE_X2},
            {RS2::CenterLine, &PATTERN_CENTER_LINE},
            {RS2::CenterLineTiny, &PATTERN_CENTER_LINE_TINY},
            {RS2::CenterLine2, &PATTERN_CENTER_LINE2},
            {RS2::CenterLineX2, &PATTERN_CENTER_LINE_X2},
            {RS2::BorderLine, &PATTERN_BORDER_LINE},
            {RS2::BorderLineTiny, &PATTERN_BORDER_LINE_TINY},
            {RS2::BorderLine2, &PATTERN_BORDER_LINE2},
            {RS2::BorderLineX2, &PATTERN_BORDER_LINE_X2},
            {RS2::LineByLayer, &PATTERN_BLOCK_LINE},
            {RS2::LineByBlock, &PATTERN_BLOCK_LINE},
            {RS2::LineSelected, &PATTERN_SELECTED}
            };
    if (lineTypeToPattern.count(lineType) == 0) {
        // fixme - redundant check since its developer bug?
        return nullptr;
    }
    return lineTypeToPattern[lineType];
}
