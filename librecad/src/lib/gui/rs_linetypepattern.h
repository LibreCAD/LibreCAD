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

#include <vector>
#include <cstddef>

/**
 * Stores a line type pattern.
 */
struct RS_LineTypePattern {
	RS_LineTypePattern()=delete;
	RS_LineTypePattern(std::initializer_list<double> const& pattern);

	~RS_LineTypePattern()=default;

	std::vector<double> pattern;
    double totalLength;
	size_t num;
    //define all line patterns in pixels
    static const RS_LineTypePattern patternSolidLine;

    static const RS_LineTypePattern patternDotLineTiny;
    static const RS_LineTypePattern patternDotLine;
    static const RS_LineTypePattern patternDotLine2;
    static const RS_LineTypePattern patternDotLineX2;

    static const RS_LineTypePattern patternDashLineTiny;
    static const RS_LineTypePattern patternDashLine;
    static const RS_LineTypePattern patternDashLine2;
    static const RS_LineTypePattern patternDashLineX2;

    static const RS_LineTypePattern patternDashDotLineTiny;
    static const RS_LineTypePattern patternDashDotLine;
    static const RS_LineTypePattern patternDashDotLine2;
    static const RS_LineTypePattern patternDashDotLineX2;

    static const RS_LineTypePattern patternDivideLineTiny;
    static const RS_LineTypePattern patternDivideLine;
    static const RS_LineTypePattern patternDivideLine2;
    static const RS_LineTypePattern patternDivideLineX2;

    static const RS_LineTypePattern patternCenterLineTiny;
    static const RS_LineTypePattern patternCenterLine;
    static const RS_LineTypePattern patternCenterLine2;
    static const RS_LineTypePattern patternCenterLineX2;

    static const RS_LineTypePattern patternBorderLineTiny;
    static const RS_LineTypePattern patternBorderLine;
    static const RS_LineTypePattern patternBorderLine2;
    static const RS_LineTypePattern patternBorderLineX2;

    static const RS_LineTypePattern patternBlockLine;
    static const RS_LineTypePattern patternSelected;
};

#endif
