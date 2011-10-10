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

/**
 * Stores a line type pattern.
 */
class RS_LineTypePattern {
public:
    RS_LineTypePattern(int count ...) {
        va_list ap;
        int i=0;
        num = count;

        va_start(ap, count);
        pattern = new double[num];
        for (i=0; i<count; i++) {
            pattern[i] = va_arg(ap, double);
        }
        va_end(ap);
    }

    ~RS_LineTypePattern() {
        delete[] pattern;
    }

public:
    double* pattern;
    int num;
};

//define all line patterns in pixels
//Author: Dongxu Li
static RS_LineTypePattern patternSolidLine(1, 50.0);

static RS_LineTypePattern patternDotLine(2, 2., -4.);
static RS_LineTypePattern patternDotLine2(2, 2, -6.);
static RS_LineTypePattern patternDotLineX2(2, 2, -2.);

static RS_LineTypePattern patternDashLine(2, 24, -12.0);
static RS_LineTypePattern patternDashLine2(2, 12.0, -6.0);
static RS_LineTypePattern patternDashLineX2(2, 48.0, -24.0);

static RS_LineTypePattern patternDashDotLine(4, 24.0, -12, 2., -12.);
static RS_LineTypePattern patternDashDotLine2(4, 12.0, -6., 2., -6.);
static RS_LineTypePattern patternDashDotLineX2(4, 48.0, -24., 2., -24.);

static RS_LineTypePattern patternDivideLine(
    6, 24.0, -12., 2., -12., 2., -12.);
static RS_LineTypePattern patternDivideLine2(
    6, 12.0, -6., 2., -6., 2., -6.);
static RS_LineTypePattern patternDivideLineX2(
    6, 48.0, -24., 2., -24., 2., -24.);

static RS_LineTypePattern patternCenterLine(4, 32.0, -6.0, 6.0, -6.0);
static RS_LineTypePattern patternCenterLine2(4, 16.0, -3.0, 3.0, -3.0);
static RS_LineTypePattern patternCenterLineX2(4, 64.0, -12.0, 12.0, -12.0);

static RS_LineTypePattern patternBorderLine(
        6, 12.0, -6.0, 12.0, -6., 2., -6.);
static RS_LineTypePattern patternBorderLine2(
        6, 6.0, -3.0, 6.0, -3., 2., -3.);
static RS_LineTypePattern patternBorderLineX2(
        6, 24.0, -12.0, 24.0, -12., 2., -12.);

static RS_LineTypePattern patternBlockLine(2, 2.5, -2.5);
static RS_LineTypePattern patternSelected(2, 2.0, -6.0);

#endif
