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
#include "lc_convert.h"
#include "lc_coordinates_parser.h"
#include "lc_formatter.h"
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

TEST_CASE("Clockwise polar input is measured from the base", "[coordinates_parser]") {
    ParserFixture fixture;
    fixture.m_graphic.setAnglesBase(M_PI_2);
    fixture.m_graphic.setAnglesCounterClockwise(false);
    // base at north and clockwise, as in surveying: 90 is east and 180 is south
    checkPoint(fixture.parse("100<0"), 0., 100.);
    checkPoint(fixture.parse("100<90"), 100., 0.);
    checkPoint(fixture.parse("100<180"), 0., -100.);
    checkPoint(fixture.parse("!100<30"), 50., 100. * std::cos(M_PI / 6.));
}

TEST_CASE("Angles shown in the drawing's basis read back to the same angle", "[coordinates_parser]") {
    ParserFixture fixture;
    LC_GraphicViewport* viewport = fixture.m_view.getViewPort();
    for (const double base : {0., M_PI / 6., M_PI_2, M_PI}) {
        fixture.m_graphic.setAnglesBase(base);
        for (const bool counterClockwise : {true, false}) {
            fixture.m_graphic.setAnglesCounterClockwise(counterClockwise);
            for (const double angle : {0., 0.2, M_PI_2, 2., M_PI, 4.5}) {
                const double back = viewport->toAbsUCSAngle(viewport->toBasisUCSAngle(angle));
                CHECK(std::remainder(back - angle, 2. * M_PI) == Catch::Approx(0.).margin(1e-9));
            }
        }
    }
}

TEST_CASE("A bearing shown by LibreCAD reads back to the same direction", "[coordinates_parser]") {
    ParserFixture fixture;
    LC_GraphicViewport* viewport = fixture.m_view.getViewPort();
    struct Basis {
        double base;
        bool counterClockwise;
    };
    for (const bool rotatedUCS : {false, true}) {
        if (rotatedUCS) {
            fixture.useRotatedUCS();
        }
        for (const Basis basis : {Basis{0., true}, Basis{M_PI_2, false}, Basis{M_PI / 6., true}, Basis{M_PI / 6., false}}) {
            fixture.m_graphic.setAnglesBase(basis.base);
            fixture.m_graphic.setAnglesCounterClockwise(basis.counterClockwise);
            LC_Formatter formatter{viewport};
            formatter.updateByGraphic(&fixture.m_graphic);
            for (const double degrees : {10., 100., 200., 340., -20.}) {
                const double ucsBasisAngle = viewport->toBasisUCSAngle(RS_Math::deg2rad(degrees));
                const QString bearing = formatter.formatRawAngle(ucsBasisAngle, RS2::Surveyors);
                INFO("base " << basis.base << (basis.counterClockwise ? " ccw " : " cw ") << degrees << " shown as "
                     << bearing.toStdString());
                const RS_Vector expected = fixture.parse("100<" + QString::number(RS_Math::rad2deg(ucsBasisAngle), 'g', 17));
                const RS_Vector typed = fixture.parse("100<" + bearing);
                checkPoint(typed, expected.x, expected.y);
            }
        }
    }
}

TEST_CASE("A bearing points the same way whatever the angle base", "[coordinates_parser]") {
    ParserFixture fixture;
    for (const double base : {0., M_PI_2, M_PI / 6.}) {
        fixture.m_graphic.setAnglesBase(base);
        for (const bool counterClockwise : {true, false}) {
            fixture.m_graphic.setAnglesCounterClockwise(counterClockwise);
            checkPoint(fixture.parse("100<N0dE"), 0., 100.);
            checkPoint(fixture.parse("100<90b"), 100., 0.);
            checkPoint(fixture.parse("100<S45dW"), -100. * M_SQRT1_2, -100. * M_SQRT1_2);
        }
    }
    // north is the UCS +Y, and with "!" the WCS +Y
    fixture.useRotatedUCS();
    checkPoint(fixture.parse("100<N0dE"), -90., 20.);
    checkPoint(fixture.parse("!100<N0dE"), 0., 100.);
}

TEST_CASE("Angle amounts do not take bearings", "[coordinates_parser]") {
    ParserFixture fixture;
    double degrees = 0.;
    CHECK(LC_Convert::parseToToDoubleAngleDegrees("85d30'", degrees, 0.0, false));
    CHECK(degrees == Catch::Approx(85.5));
    CHECK_FALSE(LC_Convert::parseToToDoubleAngleDegrees("N45dE", degrees, 0.0, false));
    CHECK_FALSE(LC_Convert::parseToToDoubleAngleDegrees("45b", degrees, 0.0, false));
    CHECK(LC_Convert::parseToDirectionDegrees("45b", degrees, M_PI_2, false));
    CHECK(degrees == Catch::Approx(45.));
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

TEST_CASE("A minus in front of a bearing is refused", "[coordinates_parser]") {
    ParserFixture fixture;
    double degrees = 0.;
    CHECK(LC_Convert::parseToDirectionDegrees("N45dE", degrees, 0., true));
    CHECK(degrees == Catch::Approx(45.));
    CHECK_FALSE(LC_Convert::parseToDirectionDegrees("-N45dE", degrees, 0., true));
    CHECK_FALSE(LC_Convert::parseToDirectionDegrees(" -45b", degrees, 0., true));
    CHECK(LC_Convert::parseToDirectionDegrees("-30", degrees, 0., true));
    CHECK(degrees == Catch::Approx(-30.));
}

TEST_CASE("With Surveyor's units amounts are shown in degrees and absolute angles from world north",
          "[coordinates_parser]") {
    ParserFixture fixture;
    LC_GraphicViewport* viewport = fixture.m_view.getViewPort();
    fixture.m_graphic.setAngleFormat(RS2::Surveyors);
    fixture.m_graphic.setAnglesBase(0.);
    fixture.m_graphic.setAnglesCounterClockwise(true);
    LC_Formatter reference{viewport};
    reference.updateByGraphic(&fixture.m_graphic);
    // with the default basis an angle is its own absolute angle
    const QString east = reference.formatRawAngle(0.);
    const QString northWest = reference.formatRawAngle(3. * M_PI_4);

    for (const double base : {0., M_PI_2, M_PI / 6.}) {
        for (const bool counterClockwise : {true, false}) {
            fixture.m_graphic.setAnglesBase(base);
            fixture.m_graphic.setAnglesCounterClockwise(counterClockwise);
            LC_Formatter formatter{viewport};
            formatter.updateByGraphic(&fixture.m_graphic);
            CHECK(formatter.formatAbsoluteAngle(0.) == east);
            CHECK(formatter.formatAbsoluteAngle(3. * M_PI_4) == northWest);
            CHECK(formatter.formatAngleAmount(M_PI / 3.) == formatter.formatRawAngle(M_PI / 3., RS2::DegreesDecimal));
        }
    }
}
