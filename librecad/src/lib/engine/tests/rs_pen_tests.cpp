/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2026 LibreCAD.org
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
**********************************************************************/

// Regression tests for RS_Pen::isSameAs(), the predicate the renderers' pen
// cache asks before it keeps the pen the painter already holds. The caller that
// makes the dash-offset term necessary is exercised in
// librecad/src/lib/gui/tests/lc_graphicviewrenderer_tests.cpp.

#include <catch2/catch_test_macros.hpp>

#include "rs_color.h"
#include "rs_pen.h"

namespace {

RS_Pen testPen(const RS2::LineType lineType, const double screenWidth = 1.0) {
    RS_Pen pen{RS_Color(Qt::black), RS2::Width00, lineType};
    pen.setScreenWidth(screenWidth);
    return pen;
}

} // namespace

TEST_CASE("the painter dash offset tells two pens apart only when a pattern is drawn",
          "[pen][linetype]") {
    // RS_Painter::updateDashOffset() decrements the painter's running dash offset
    // by the length of every entity drawn, so the offset handed to isSameAs() has
    // left zero long before the next entity is prepared. Whether it can change
    // what the pen paints is the caller's answer, not the line type's.
    const double runningOffset = -37.5;

    const RS_Pen solid = testPen(RS2::SolidLine);
    CHECK(solid.isSameAs(solid, runningOffset, false));
    // The very same pen keeps the offset in its key as soon as the caller says a
    // pattern is about to be painted with it - which is what the widget renderer
    // does to a selected entity whose own line type resolved solid.
    CHECK_FALSE(solid.isSameAs(solid, runningOffset, true));

    for (const RS2::LineType lineType : {RS2::NoPen, RS2::LineByLayer, RS2::LineByBlock}) {
        const RS_Pen pen = testPen(lineType);
        CHECK(pen.isSameAs(pen, runningOffset, false));
    }

    // Where a pattern is drawn the offset stays in the key: two otherwise equal
    // dashed pens at different phases are two different pens, and dropping the
    // term outright would let the second entity inherit the first one's phase.
    const RS_Pen dashed = testPen(RS2::DashLine);
    CHECK_FALSE(dashed.isSameAs(dashed, runningOffset, true));
    CHECK(dashed.isSameAs(dashed, 0.0, true));
    RS_Pen phased = dashed;
    phased.setDashOffset(runningOffset);
    CHECK(phased.isSameAs(dashed, runningOffset, true));
    CHECK_FALSE(phased.isSameAs(dashed, runningOffset + 0.1, true));

    // Nothing but the offset term is relaxed: every other difference still tells
    // two solid pens apart under a running offset.
    RS_Pen coloured = solid;
    coloured.setColor(RS_Color(Qt::red));
    CHECK_FALSE(solid.isSameAs(coloured, runningOffset, false));

    RS_Pen wider = solid;
    wider.setWidth(RS2::Width01);
    CHECK_FALSE(solid.isSameAs(wider, runningOffset, false));

    RS_Pen translucent = solid;
    translucent.setAlpha(0.5f);
    CHECK_FALSE(solid.isSameAs(translucent, runningOffset, false));

    RS_Pen invalid = solid;
    invalid.setFlag(RS2::FlagInvalid);
    CHECK_FALSE(invalid.isSameAs(solid, runningOffset, false));
}
