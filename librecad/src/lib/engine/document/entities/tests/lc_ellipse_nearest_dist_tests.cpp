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

// RS_Ellipse::getNearestDist() finds the point at an arc length from the end of an elliptic arc that
// the mouse is near (Snap Distance): the amount is signed, a positive one lengthens the arc at that end
// and a negative one shortens it (RS_Modification::trimAmount() passes -dist; Modify > Trim Amount does
// not take ellipses yet). It looked for the point with Halley's method on the length from the start
// angle, and the search did not end for some arcs, such as an arc whose angles are negative, so that the
// program stopped, and it lost the point for others, such as an arc that starts at 180 degrees. The
// checks measure the arc length to the point that comes back with a numerical integral of the speed of
// the ellipse.

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cmath>
#include <limits>
#include <random>

#include "lc_actiontestsupport.h"
#include "rs_ellipse.h"
#include "rs_graphic.h"
#include "rs_math.h"

namespace {

constexpr double kTwoPi = 2. * M_PI;

/// the length of the ellipse with the semi-axes \p a and \p b from the elliptic angle \p t1 up to \p t2
double integratedLength(const double a, const double b, const double t1, const double t2) {
    constexpr int panels = 4000;
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

/// the arc as it runs counterclockwise: from the angle \p s, by \p length
struct CounterclockwiseArc {
    double s = 0.;
    double length = 0.;
    RS_Vector start;
    RS_Vector end;
};

CounterclockwiseArc counterclockwiseArc(const RS_Ellipse& ellipse) {
    const RS_EllipseData& d = ellipse.getData();
    const double a = d.majorP.magnitude();
    const double b = a * d.ratio;
    double s = RS_Math::correctAngle(d.reversed ? d.angle2 : d.angle1);
    double t = RS_Math::correctAngle(d.reversed ? d.angle1 : d.angle2);
    if (t < s + RS_TOLERANCE_ANGLE) {
        t += kTwoPi;
    }
    return {s, integratedLength(a, b, s, t), ellipse.getEllipsePoint(s), ellipse.getEllipsePoint(t)};
}

/// The arc length, along the ellipse and counterclockwise from the start of the arc, to \p point.
double lengthFromStart(const RS_Ellipse& ellipse, const CounterclockwiseArc& arc, const RS_Vector& point) {
    const RS_EllipseData& d = ellipse.getData();
    const double a = d.majorP.magnitude();
    double t = RS_Math::correctAngle(ellipse.getEllipseAngle(point));
    if (t < arc.s) {
        t += kTwoPi;
    }
    return integratedLength(a, a * d.ratio, arc.s, t);
}

/// the distance between two arc lengths on an ellipse whose whole length is \p whole
double circularDistance(const double x, const double y, const double whole) {
    double r = std::fmod(std::abs(x - y), whole);
    return std::min(r, whole - r);
}

/// Where, counterclockwise from the start of the arc, the point of an amount has to be: the arc length
/// to the new end (the end the mouse is near, moved by \p amount) or to the new start; -1 if there is none
/// (the arc is shortened past its own start).
double wantedLength(const bool nearEnd, const double arcLength, const double amount, const double whole) {
    double wanted = nearEnd ? arcLength + amount : (amount > 0. ? whole - amount : -amount);
    if (nearEnd && wanted <= 0.) {
        return -1.;
    }
    wanted = std::fmod(wanted, whole);
    return wanted < 0. ? wanted + whole : wanted;
}

RS_EllipseData randomArc(std::mt19937& rng, const double ratio, const bool anyAngles) {
    std::uniform_real_distribution<double> unit(0., 1.);
    RS_EllipseData d;
    d.center = {unit(rng) * 200. - 100., unit(rng) * 200. - 100.};
    const double rotation = unit(rng) * kTwoPi;
    const double a = 1. + unit(rng) * 40.;
    d.majorP = RS_Vector{a * std::cos(rotation), a * std::sin(rotation)};
    d.ratio = ratio;
    d.angle1 = 0.05 + unit(rng) * (kTwoPi - 0.1);
    d.angle2 = std::fmod(d.angle1 + 0.2 + unit(rng) * (kTwoPi - 0.4), kTwoPi);
    if (anyAngles) {
        // the angles a file or the Properties dialog can leave: negative, or more than a turn
        d.angle1 += kTwoPi * std::floor((unit(rng) * 5.) - 3.);
        d.angle2 += kTwoPi * std::floor((unit(rng) * 5.) - 3.);
    }
    d.reversed = unit(rng) < 0.5;
    return d;
}

/// what getNearestDist() has to give for \p amount with the mouse at the start or the end of the arc
void requireDistancePoint(const RS_Ellipse& ellipse, const double amount, const bool nearEnd) {
    const RS_EllipseData& d = ellipse.getData();
    const CounterclockwiseArc arc = counterclockwiseArc(ellipse);
    const double a = d.majorP.magnitude();
    const double whole = integratedLength(a, a * d.ratio, 0., kTwoPi);
    const RS_Vector mouse = (nearEnd ? arc.end : arc.start) + RS_Vector{0.001, 0.001};
    CAPTURE(d.ratio, d.angle1, d.angle2, d.reversed, nearEnd, amount, arc.length, whole);

    double distance = 0.;
    const RS_Vector found = ellipse.getNearestDist(amount, mouse, &distance);
    const double wanted = wantedLength(nearEnd, arc.length, amount, whole);
    if (wanted < 0.) {
        // the arc is shortened to nothing: there is no such point
        REQUIRE_FALSE(found.valid);
        REQUIRE(distance == RS_MAXDOUBLE);
        return;
    }
    REQUIRE(found.valid);
    REQUIRE(distance == Catch::Approx(found.distanceTo(mouse)));
    REQUIRE(circularDistance(lengthFromStart(ellipse, arc, found), wanted, whole) < 1e-8 * whole);
}

RS_Ellipse makeArc(RS_Graphic& graphic, const double ratio, const double start, const double sweep, const bool reversed) {
    RS_EllipseData d;
    d.center = {12., -7.};
    d.majorP = {15. * std::cos(0.4), 15. * std::sin(0.4)};
    d.ratio = ratio;
    d.angle1 = reversed ? start + sweep : start;
    d.angle2 = reversed ? start : start + sweep;
    d.reversed = reversed;
    return RS_Ellipse(&graphic, d);
}

} // namespace

TEST_CASE("RS_Ellipse::getNearestDist returns for an arc with negative angles", "[rs_ellipse][nearest-dist]") {
    // This arc, with this mouse position and amount, made the search of getNearestDist() run forever.
    lc::test::application();
    RS_Graphic graphic;
    graphic.initForNewDocument();
    RS_EllipseData d;
    d.center = {90.913329577866904, 16.767194141768016};
    const double ratio = 3.5248716010564989;
    const double rotation = 0.66648851789044983;
    const double a = 19.24722533922187;
    d.majorP = RS_Vector{-ratio * a * std::sin(rotation), ratio * a * std::cos(rotation)};
    d.ratio = 1. / ratio;
    d.angle1 = 1.015474080703582 - M_PI_2; // both negative
    d.angle2 = 0.43451867766675178 - M_PI_2;
    d.reversed = true;
    const RS_Ellipse ellipse(&graphic, d);
    REQUIRE(d.angle1 < 0.);
    REQUIRE(d.angle2 < 0.);

    const RS_Vector mouse{44.299698483898304, 23.839727701685806};
    constexpr double amount = 8.0816527020983049;
    const RS_Vector found = ellipse.getNearestDist(amount, mouse);
    REQUIRE(found.valid);

    const CounterclockwiseArc arc = counterclockwiseArc(ellipse);
    const double whole = integratedLength(d.majorP.magnitude(), d.majorP.magnitude() * d.ratio, 0., kTwoPi);
    const bool nearEnd = mouse.distanceTo(arc.end) < mouse.distanceTo(arc.start);
    const double wanted = wantedLength(nearEnd, arc.length, amount, whole);
    REQUIRE(wanted > 0.);
    REQUIRE(circularDistance(lengthFromStart(ellipse, arc, found), wanted, whole) < 1e-6 * whole);
}

TEST_CASE("RS_Ellipse::getNearestDist finds the point for arcs that start on an axis or below zero",
          "[rs_ellipse][nearest-dist]") {
    // the start angles where the length of the arc from its start jumps between 0 and the whole ellipse
    // (at the start itself), and where the angles wrap: a hair either side of the axes and of 0
    lc::test::application();
    RS_Graphic graphic;
    graphic.initForNewDocument();
    const double starts[] = {0.,           1e-9,     -1e-9,      M_PI_2,          M_PI - 1e-9, M_PI,
                             M_PI + 1e-9,  -M_PI,    -M_PI_2,    3. * M_PI_2,     kTwoPi,      -kTwoPi - 0.3,
                             kTwoPi + 0.7, 0.3,      4.,         -5.};
    for (const double ratio : {0.05, 0.2, 0.45, 0.9, 1.}) {
        for (const double start : starts) {
            for (const double sweep : {0.3, 1.5, M_PI, 4.5}) {
                for (const bool reversed : {false, true}) {
                    const RS_Ellipse arc = makeArc(graphic, ratio, start, sweep, reversed);
                    if (!arc.isEllipticArc()) {
                        continue;
                    }
                    const double length = counterclockwiseArc(arc).length;
                    for (const double share : {0.03, 0.4}) {
                        for (const double sign : {-1., 1.}) {
                            requireDistancePoint(arc, sign * share * length, true);
                            requireDistancePoint(arc, sign * share * length, false);
                        }
                    }
                }
            }
        }
    }
}

TEST_CASE("RS_Ellipse::getNearestDist puts the point at the arc length asked for", "[rs_ellipse][nearest-dist]") {
    lc::test::application();
    RS_Graphic graphic;
    graphic.initForNewDocument();

    for (const unsigned seed : {2966U, 1U, 2U, 3U, 4U, 5U, 6U, 7U}) {
        std::mt19937 rng(seed);
        std::uniform_real_distribution<double> unit(0., 1.);
        for (int n = 0; n < 250; ++n) {
            // a third of them with the minor axis the longer, a third with any angles
            const double ratio = n % 3 == 0 ? 1.05 + unit(rng) * 4. : 0.05 + unit(rng) * 0.9;
            const RS_EllipseData d = randomArc(rng, ratio, n % 3 == 1);
            const RS_Ellipse ellipse(&graphic, d);
            if (!ellipse.isEllipticArc()) {
                continue;
            }
            const CounterclockwiseArc arc = counterclockwiseArc(ellipse);
            // shorter or longer by up to 95% of the arc; the search keeps to the arc length, whichever way
            // round it wraps
            const bool nearEnd = unit(rng) < 0.5;
            const double sign = unit(rng) < 0.5 ? -1. : 1.;
            const double share = 0.05 + (0.9 * unit(rng));
            CAPTURE(seed, n);
            requireDistancePoint(ellipse, sign * share * arc.length, nearEnd);
        }
    }
}

TEST_CASE("RS_Ellipse::getNearestDist at the ends and past them", "[rs_ellipse][nearest-dist]") {
    lc::test::application();
    RS_Graphic graphic;
    graphic.initForNewDocument();
    const RS_Ellipse arc = makeArc(graphic, 0.4, 1., 4., false); // most of the ellipse
    const CounterclockwiseArc ccw = counterclockwiseArc(arc);
    const double whole = integratedLength(15., 15. * 0.4, 0., kTwoPi);
    const RS_Vector nearEnd = ccw.end + RS_Vector{0.001, 0.001};
    const RS_Vector nearStart = ccw.start + RS_Vector{0.001, 0.001};

    SECTION("an amount of 0 is the end itself") {
        REQUIRE(arc.getNearestDist(0., nearEnd).distanceTo(ccw.end) < 1e-6);
        REQUIRE(arc.getNearestDist(0., nearStart).distanceTo(ccw.start) < 1e-6);
    }
    SECTION("lengthened all the way round it is the other end") {
        // the gap from the end back round to the start is whole - length: an arc lengthened by it is a whole turn
        REQUIRE(arc.getNearestDist(whole - ccw.length, nearEnd).distanceTo(ccw.start) < 1e-6);
        REQUIRE(arc.getNearestDist(whole - ccw.length, nearStart).distanceTo(ccw.end) < 1e-6);
    }
    SECTION("lengthened past a whole turn it goes round again") {
        const double amount = 0.6 * ccw.length; // 0.6 * length + length > whole for this arc
        REQUIRE(ccw.length + amount > whole);
        requireDistancePoint(arc, amount, true);
        requireDistancePoint(arc, amount, false);
    }
    SECTION("shortened to nothing there is no point at either end") {
        const double nan = std::numeric_limits<double>::quiet_NaN();
        const double inf = std::numeric_limits<double>::infinity();
        // arc.getLength() is the length the search measures with: an amount of minus that is the boundary
        for (const double amount : {-arc.getLength(), -1.2 * arc.getLength(), -5. * arc.getLength(), -inf, nan}) {
            CAPTURE(amount);
            double distance = 0.;
            REQUIRE_FALSE(arc.getNearestDist(amount, nearEnd, &distance).valid);
            REQUIRE(distance == RS_MAXDOUBLE);
            distance = 0.;
            REQUIRE_FALSE(arc.getNearestDist(amount, nearStart, &distance).valid);
            REQUIRE(distance == RS_MAXDOUBLE);
        }
    }
    SECTION("shortened to a hair of it the point is next to the other end") {
        const double amount = -(arc.getLength() - 1e-6);
        REQUIRE(arc.getNearestDist(amount, nearEnd).distanceTo(ccw.start) < 1e-5);
        REQUIRE(arc.getNearestDist(amount, nearStart).distanceTo(ccw.end) < 1e-5);
    }
    SECTION("a thin ellipse") {
        for (const double ratio : {1e-3, 1e-6, 1e-9}) {
            const RS_Ellipse thin = makeArc(graphic, ratio, 0.3, 1.5, false);
            const CounterclockwiseArc thinArc = counterclockwiseArc(thin);
            const RS_Vector found = thin.getNearestDist(0.2 * thinArc.length, thinArc.end + RS_Vector{0.001, 0.001});
            CAPTURE(ratio);
            REQUIRE(found.valid);
            if (ratio >= 1e-6) {
                requireDistancePoint(thin, 0.2 * thinArc.length, true);
            }
        }
    }
    SECTION("a thin ellipse near the end of its axis") {
        // the elliptic integral is off by more than the tolerance of the search there, so the search has to end
        // at the noise of the length instead of at the tolerance
        for (const double ratio : {2e-2, 2e-3, 5e-4, 1e-4}) {
            const RS_Ellipse thin = makeArc(graphic, ratio, 0., M_PI_2, false);
            const CounterclockwiseArc thinArc = counterclockwiseArc(thin);
            for (const double d : {3e-6, 1e-5, 3e-5, 1e-4, 3e-4}) {
                CAPTURE(ratio, d);
                // the start shortened by d: a point at d from the start
                const RS_Vector shortened = thin.getNearestDist(-d, thinArc.start + RS_Vector{1e-6, 1e-6});
                REQUIRE(shortened.valid);
                REQUIRE(shortened.distanceTo(thinArc.start) < 1e-2);
                // the end lengthened by d
                const RS_Vector lengthened = thin.getNearestDist(d, thinArc.end + RS_Vector{1e-6, 1e-6});
                REQUIRE(lengthened.valid);
                REQUIRE(lengthened.distanceTo(thinArc.end) < 1e-2);
            }
        }
    }
}
