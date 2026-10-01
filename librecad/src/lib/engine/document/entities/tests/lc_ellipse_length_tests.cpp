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
******************************************************************************/

// The length of an elliptic arc, checked against a numerical integral of its speed. An arc that
// ends on an axis has an end angle that is a multiple of pi/2, or that rounding puts a hair below
// one, and RS_Ellipse::getEllipseLength() counted the half periods of the ellipse with a tolerance
// but took the rest of the angle without it: an angle a hair below a multiple of pi was counted as
// past it and still had a rest of nearly pi, so the arc got a length that was too long by up to a
// half ellipse, or a negative one.

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <limits>
#include <memory>
#include <random>

#include "lc_actiontestsupport.h"
#include "rs_ellipse.h"
#include "rs_graphic.h"
#include "rs_math.h"

namespace {

constexpr double kTwoPi = 2. * M_PI;

/// The length of the ellipse with the semi-axes \p a (at the parameter 0) and \p b from the elliptic
/// angle \p t1 up to \p t2, by Simpson's rule on its speed sqrt(a^2 sin^2 t + b^2 cos^2 t).
double integratedLength(const double a, const double b, const double t1, const double t2) {
    constexpr int panels = 6000;
    const double h = (t2 - t1) / panels;
    const auto speed = [a, b](const double t) {
        const double s = std::sin(t);
        const double c = std::cos(t);
        return std::sqrt((a * a * s * s) + (b * b * c * c));
    };
    double sum = speed(t1) + speed(t2);
    for (int i = 1; i < panels; ++i) {
        sum += speed(t1 + (i * h)) * (i % 2 == 0 ? 2. : 4.);
    }
    return sum * h / 3.;
}

/// The length of the arc from the elliptic angle \p start to \p end (both anywhere), as
/// RS_Ellipse::getEllipseLength() reads the angles: in [0, 2 pi), the end after the start, and an end
/// that does not differ from the start is a whole turn.
double referenceLength(const double a, const double b, const double start, const double end) {
    double t1 = RS_Math::correctAngle(start);
    double t2 = RS_Math::correctAngle(end);
    if (std::abs(t2 - t1) < RS_TOLERANCE_ANGLE) {
        return integratedLength(a, b, 0., kTwoPi);
    }
    if (t2 < t1) {
        t2 += kTwoPi;
    }
    return integratedLength(a, b, t1, t2);
}

} // namespace

TEST_CASE("RS_Ellipse::getEllipseLength has no jump at a multiple of pi", "[rs_ellipse][ellipse-length]") {
    lc::test::application();
    RS_Graphic graphic;
    graphic.initForNewDocument();

    // the ends on and around the axes, where the half periods change: rounding noise, and both sides
    // of the tolerance of the angles
    const double offsets[] = {0., 1e-15, -1e-15, 1e-12, -1e-12, 1e-9, -1e-9, 5e-9, -5e-9, 2e-8, -2e-8, 1e-5, -1e-5};
    for (const double ratio : {0.2, 0.5, 0.9, 1.}) {
        RS_EllipseData d;
        d.center = {10., -3.};
        d.majorP = {3., 0.};
        d.ratio = ratio;
        const RS_Ellipse ellipse(&graphic, d);
        const double a = 3.;
        const double b = 3. * ratio;
        for (int k1 = 0; k1 <= 4; ++k1) {
            for (int k2 = 0; k2 <= 4; ++k2) {
                for (const double o1 : offsets) {
                    for (const double o2 : offsets) {
                        const double start = (k1 * M_PI_2) + o1;
                        const double end = (k2 * M_PI_2) + o2;
                        CAPTURE(ratio, k1, k2, o1, o2);
                        // a hair of an arc is a difference of two lengths of about a: a few ulps of a absolute
                        REQUIRE(ellipse.getEllipseLength(start, end) ==
                                Catch::Approx(referenceLength(a, b, start, end)).epsilon(1e-9).margin(1e-13 * a));
                    }
                }
            }
        }
    }
}

TEST_CASE("RS_Ellipse::getLength of an arc that ends on an axis", "[rs_ellipse][ellipse-length]") {
    lc::test::application();
    RS_Graphic graphic;
    graphic.initForNewDocument();

    // arcs of rotated ellipses away from the origin, whose ends are put on the axes the way the
    // grips and Trim do it: the end point becomes the angle through getEllipseAngle(), with noise
    std::mt19937 rng(2966);
    std::uniform_real_distribution<double> unit(0., 1.);
    for (int n = 0; n < 4000; ++n) {
        RS_EllipseData d;
        d.center = {unit(rng) * 400. - 200., unit(rng) * 400. - 200.};
        const double rotation = unit(rng) * kTwoPi;
        const double a = 1. + unit(rng) * 40.;
        d.majorP = RS_Vector{a * std::cos(rotation), a * std::sin(rotation)};
        // every third one with the minor axis the longer, as a tool hands it over before the axes are swapped
        d.ratio = n % 3 == 0 ? 1.05 + unit(rng) * 4. : 0.1 + unit(rng) * 0.85;
        d.angle1 = 0.3;
        d.angle2 = 2.;
        d.reversed = unit(rng) < 0.5;
        RS_Ellipse arc(&graphic, d);

        // both ends on axes (the first also may be a random point), all four axis ends
        const int k1 = static_cast<int>(unit(rng) * 4.);
        const int k2 = (k1 + 1 + static_cast<int>(unit(rng) * 3.)) % 4;
        arc.moveStartpoint(arc.getEllipsePoint(k1 * M_PI_2));
        arc.moveEndpoint(arc.getEllipsePoint(k2 * M_PI_2));
        REQUIRE(arc.isEllipticArc());

        const double majorRadius = d.majorP.magnitude();
        const double expected = referenceLength(majorRadius, majorRadius * d.ratio, arc.isReversed() ? arc.getAngle2() : arc.getAngle1(),
                                                arc.isReversed() ? arc.getAngle1() : arc.getAngle2());
        CAPTURE(n, d.ratio, k1, k2, d.reversed, arc.getAngle1(), arc.getAngle2());
        REQUIRE(arc.getLength() == Catch::Approx(expected).epsilon(1e-8));
    }
}

TEST_CASE("RS_Ellipse::getEllipseLength of generic arcs is unchanged", "[rs_ellipse][ellipse-length]") {
    lc::test::application();
    RS_Graphic graphic;
    graphic.initForNewDocument();

    std::mt19937 rng(7);
    std::uniform_real_distribution<double> unit(0., 1.);
    for (int n = 0; n < 500; ++n) {
        RS_EllipseData d;
        d.majorP = {2. + (unit(rng) * 30.), 0.};
        d.ratio = 0.1 + unit(rng) * 0.9;
        const RS_Ellipse ellipse(&graphic, d);
        const double start = unit(rng) * kTwoPi;
        const double end = unit(rng) * kTwoPi;
        CAPTURE(n, start, end);
        REQUIRE(ellipse.getEllipseLength(start, end) ==
                Catch::Approx(referenceLength(d.majorP.x, d.majorP.x * d.ratio, start, end)).epsilon(1e-9));
    }
}

TEST_CASE("RS_Ellipse::getEllipseLength of a whole turn and of what is not an ellipse", "[rs_ellipse][ellipse-length]") {
    lc::test::application();
    RS_Graphic graphic;
    graphic.initForNewDocument();
    RS_EllipseData d;
    d.majorP = {10., 0.};
    d.ratio = 0.5;
    const RS_Ellipse ellipse(&graphic, d);
    const double whole = ellipse.getEllipseLength(0., 0.);
    const double nan = std::numeric_limits<double>::quiet_NaN();
    const double inf = std::numeric_limits<double>::infinity();

    SECTION("angles that differ by less than the tolerance are a whole turn") {
        for (const double start : {0., 0.5, M_PI_2, 3.}) {
            CAPTURE(start);
            REQUIRE(ellipse.getEllipseLength(start, start) == whole);
            REQUIRE(ellipse.getEllipseLength(start, start + 9e-9) == whole);
            REQUIRE(ellipse.getEllipseLength(start + 9e-9, start) == whole);
        }
    }
    SECTION("just beyond the tolerance they are a hair of an arc, or all of it but a hair") {
        REQUIRE(ellipse.getEllipseLength(0.5, 0.5 + 1.1e-8) < 1e-6);
        REQUIRE(ellipse.getEllipseLength(0.5, 0.5 + 1.1e-8) > 0.);
        REQUIRE(ellipse.getEllipseLength(0.5 + 1.1e-8, 0.5) == Catch::Approx(whole).margin(1e-6));
        REQUIRE(ellipse.getEllipseLength(0.5 + 1.1e-8, 0.5) < whole);
    }
    SECTION("angles that are not numbers have no length") {
        REQUIRE(ellipse.getEllipseLength(nan, 1.) == 0.);
        REQUIRE(ellipse.getEllipseLength(1., nan) == 0.);
        REQUIRE(ellipse.getEllipseLength(inf, 1.) == 0.);
        REQUIRE(ellipse.getEllipseLength(1., -inf) == 0.);
    }
    SECTION("a ratio that is not a number or is above 1 has no length, and makes no exception") {
        for (const double ratio : {nan, inf, 2., 1e300}) {
            RS_EllipseData bad = d;
            bad.ratio = ratio;
            CAPTURE(ratio);
            for (const bool arc : {false, true}) {
                bad.angle1 = arc ? 0.3 : 0.;
                bad.angle2 = arc ? 2. : 0.;
                std::unique_ptr<RS_Ellipse> made;
                REQUIRE_NOTHROW(made = std::make_unique<RS_Ellipse>(&graphic, bad));
                REQUIRE(made->getEllipseLength(0.3, 1.2) == 0.);
            }
        }
    }
}
