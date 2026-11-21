/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2026 LibreCAD.org
** Copyright (C) 2026 Dongxu Li
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

// Typed WCS coordinates ("!x,y" and "!distance<angle", issue #3293). A WCS
// polar point lies at the distance along the angle, measured with the
// drawing's angle base and direction, and the current UCS does not move it.

#include <cmath>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <QString>

#include "lc_actiontestsupport.h"
#include "lc_coordinates_parser.h"
#include "lc_graphicviewport.h"
#include "rs_graphic.h"
#include "rs_vector.h"

namespace {

struct ParserFixture {
    const bool m_qtReady{lc::test::application() != nullptr};
    RS_Graphic m_graphic;
    lc::test::TestGraphicView m_view;

    ParserFixture() {
        m_graphic.initForNewDocument();
        m_view.setDocument(&m_graphic);
    }

    RS_Vector parse(const QString& input) {
        const LC_CoordinatesParser parser{&m_view};
        bool containsCoordinate = false;
        const RS_CoordinateEvent event = parser.parseCoordinate(input, containsCoordinate);
        REQUIRE(containsCoordinate);
        REQUIRE(event.isValid());
        return event.getCoordinate();
    }

    // a UCS turned by 90 degrees, with its origin moved away from the WCS one
    void useRotatedUCS() {
        m_view.getViewPort()->createUCS(RS_Vector(10., 20.), M_PI_2);
    }
};

void checkPoint(const RS_Vector& actual, const double x, const double y) {
    CHECK(actual.x == Catch::Approx(x).margin(1e-9));
    CHECK(actual.y == Catch::Approx(y).margin(1e-9));
}

} // namespace

TEST_CASE("WCS polar input is a point at the distance along the angle", "[coordinates_parser]") {
    ParserFixture fixture;
    checkPoint(fixture.parse("!100<30"), 100. * std::cos(M_PI / 6.), 50.);
    checkPoint(fixture.parse("!100<90"), 0., 100.);
    checkPoint(fixture.parse("!2<180"), -2., 0.);
    checkPoint(fixture.parse("!0<45"), 0., 0.);
}

TEST_CASE("WCS input ignores the relative zero and the UCS", "[coordinates_parser]") {
    ParserFixture fixture;
    fixture.m_view.getViewPort()->setRelativeZero(RS_Vector(5., 5.));
    fixture.useRotatedUCS();

    checkPoint(fixture.parse("!100<30"), 100. * std::cos(M_PI / 6.), 50.);
    checkPoint(fixture.parse("!3,4"), 3., 4.);
    // plain polar input is in the UCS, so it lands somewhere else
    const RS_Vector ucsPolar = fixture.parse("100<30");
    CHECK_FALSE(ucsPolar.distanceTo(RS_Vector(100. * std::cos(M_PI / 6.), 50.)) < 1e-6);
}

TEST_CASE("WCS polar input follows the drawing's angle base and direction", "[coordinates_parser]") {
    ParserFixture fixture;
    fixture.m_graphic.setAnglesBase(M_PI_2);
    checkPoint(fixture.parse("!100<0"), 0., 100.);
    checkPoint(fixture.parse("!100<90"), -100., 0.);

    // without a UCS, "!" reads the angle exactly as plain polar input does
    for (const bool counterClockwise : {true, false}) {
        fixture.m_graphic.setAnglesCounterClockwise(counterClockwise);
        for (const QString angle : {"0", "30", "90", "200"}) {
            const RS_Vector wcs = fixture.parse("!100<" + angle);
            const RS_Vector plain = fixture.parse("100<" + angle);
            checkPoint(wcs, plain.x, plain.y);
        }
    }
}
