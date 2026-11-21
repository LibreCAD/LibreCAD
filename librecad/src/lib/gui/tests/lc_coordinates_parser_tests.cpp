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
#include "rs_math.h"
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

TEST_CASE("WCS and plain polar input at 45 and -90 degrees", "[coordinates_parser]") {
    ParserFixture fixture;
    const double half = 100. * M_SQRT1_2;

    SECTION("without a UCS both read the same") {
        checkPoint(fixture.parse("!100<45"), half, half);
        checkPoint(fixture.parse("100<45"), half, half);
        checkPoint(fixture.parse("!100<-90"), 0., -100.);
        checkPoint(fixture.parse("100<-90"), 0., -100.);
    }

    SECTION("with a rotated UCS only plain input moves") {
        fixture.useRotatedUCS();
        checkPoint(fixture.parse("!100<45"), half, half);
        checkPoint(fixture.parse("!100<-90"), 0., -100.);
        const RS_Vector plain = fixture.parse("100<45");
        // 45 degrees in the UCS is 135 degrees in the WCS, from the UCS origin
        checkPoint(plain, 10. - half, 20. + half);
        // -90 degrees in the UCS is 0 degrees in the WCS
        checkPoint(fixture.parse("100<-90"), 110., 20.);
    }
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

TEST_CASE("Polar input on an axis is exact", "[coordinates_parser]") {
    ParserFixture fixture;
    struct Case {
        const char* angle;
        double x;
        double y;
    };
    for (const auto& [angle, x, y] : {Case{"0", 100., 0.}, Case{"90", 0., 100.}, Case{"180", -100., 0.},
                                      Case{"270", 0., -100.}, Case{"360", 100., 0.}, Case{"-90", 0., -100.},
                                      Case{"-180", -100., 0.}, Case{"-270", 0., 100.}, Case{"450", 0., 100.}}) {
        for (const QString prefix : {"", "!", "@"}) {
            const RS_Vector point = fixture.parse(prefix + "100<" + angle);
            CAPTURE(prefix.toStdString(), angle, point.x, point.y);
            // exact, not within a tolerance
            CHECK(point.x == x);
            CHECK(point.y == y);
        }
    }
}

TEST_CASE("Polar input off the axes matches RS_Vector::polar", "[coordinates_parser]") {
    ParserFixture fixture;
    for (const double degrees : {30., 45., 89.5, 135., 200., -135., 1000., 0.001}) {
        const RS_Vector expected = RS_Vector::polar(100., RS_Math::deg2rad(degrees));
        const QString angle = QString::number(degrees, 'g', 17);
        for (const QString prefix : {"", "!", "@"}) {
            CAPTURE(prefix.toStdString(), degrees);
            checkPoint(fixture.parse(prefix + "100<" + angle), expected.x, expected.y);
        }
    }
}

TEST_CASE("Whitespace around the prefix and the separator is ignored", "[coordinates_parser]") {
    ParserFixture fixture;
    fixture.m_view.getViewPort()->setRelativeZero(RS_Vector(1., 2.));
    const double cos30 = 100. * std::cos(M_PI / 6.);
    struct Case {
        const char* input;
        double x;
        double y;
    };
    for (const auto& [input, x, y] : {Case{"3 , 4", 3., 4.}, Case{"  3,4  ", 3., 4.}, Case{"@ 10 , 5", 11., 7.},
                                      Case{"@10 ,5", 11., 7.}, Case{"! 3 , 4", 3., 4.}, Case{"100 < 30", cos30, 50.},
                                      Case{"100< -90", 0., -100.}, Case{"@ 100 < 0", 101., 2.},
                                      Case{"! 100 < 30", cos30, 50.}, Case{"\t!100<30\t", cos30, 50.},
                                      Case{"2 * 3 , 4", 6., 4.}, Case{" 0 ", 0., 0.}, Case{" . ", 1., 2.},
                                      // a mixed number: 1 1/2 is 1.5
                                      Case{"1 1/2 , 2", 1.5, 2.}, Case{"!1 1/2,2 1/4", 1.5, 2.25},
                                      Case{"@ 1 1/2 , 0", 2.5, 2.}, Case{"1 1/2<0", 1.5, 0.}}) {
        CAPTURE(input);
        checkPoint(fixture.parse(QString::fromLatin1(input)), x, y);
    }
}

TEST_CASE("The coordinate syntax tells coordinates from commands", "[coordinates_parser]") {
    using Syntax = LC_CoordinatesParser::Syntax;
    for (const char* input : {"0", ".", ",", "3,4", "@3,4", "!3,4", "100<30", "@ 100 < -90", "! 100<30",
                              "1/2,2", "1 1/2,2", "2*3,4", "100<30d"}) {
        CAPTURE(input);
        CHECK(LC_CoordinatesParser::isCoordinate(QString::fromLatin1(input)));
    }
    for (const char* input : {"", "  ", "line", "zoomauto", "x", "@", "!", "@line", "1,", ",2", "<30", "100<",
                              "1,abc", "abc<30", "100<abc", "!abc", "!100"}) {
        CAPTURE(input);
        CHECK_FALSE(LC_CoordinatesParser::isCoordinate(QString::fromLatin1(input)));
    }
    // a command that is no coordinate goes on to the action; a broken coordinate does not
    CHECK(LC_CoordinatesParser::parseSyntax("zoomauto").kind == Syntax::None);
    CHECK(LC_CoordinatesParser::parseSyntax("@line").kind == Syntax::None);
    CHECK(LC_CoordinatesParser::parseSyntax("1,abc").kind == Syntax::Cartesian);
    CHECK(LC_CoordinatesParser::parseSyntax("100<abc").kind == Syntax::Polar);
    CHECK(LC_CoordinatesParser::parseSyntax("!abc").kind == Syntax::Cartesian);
}
