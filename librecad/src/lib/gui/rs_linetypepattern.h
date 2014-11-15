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


#ifndef RS_LINETYPEPATTERN_H
#define RS_LINETYPEPATTERN_H

#include <stdarg.h>
#include <cmath>

/**
 * Stores a line type pattern.
 */
struct RS_LineTypePattern {
    RS_LineTypePattern(int count ...);

    ~RS_LineTypePattern();

    double* pattern;
    double totalLength;
    int num;
    //define all line patterns in pixels
    const static RS_LineTypePattern patternSolidLine;

    const static RS_LineTypePattern patternDotLine;
    const static RS_LineTypePattern patternDotLine2;
    const static RS_LineTypePattern patternDotLineX2;

    const static RS_LineTypePattern patternDashLine;
    const static RS_LineTypePattern patternDashLine2;
    const static RS_LineTypePattern patternDashLineX2;

    const static RS_LineTypePattern patternDashDotLine;
    const static RS_LineTypePattern patternDashDotLine2;
    const static RS_LineTypePattern patternDashDotLineX2;

    const static RS_LineTypePattern patternDivideLine;
    const static RS_LineTypePattern patternDivideLine2;
    const static RS_LineTypePattern patternDivideLineX2;

    const static RS_LineTypePattern patternCenterLine;
    const static RS_LineTypePattern patternCenterLine2;
    const static RS_LineTypePattern patternCenterLineX2;

    const static RS_LineTypePattern patternBorderLine;
    const static RS_LineTypePattern patternBorderLine2;
    const static RS_LineTypePattern patternBorderLineX2;

    const static RS_LineTypePattern patternBlockLine;
    const static RS_LineTypePattern patternSelected;
};

#endif
