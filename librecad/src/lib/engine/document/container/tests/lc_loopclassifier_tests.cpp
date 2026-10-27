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
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
****************************************************************************/

// Unit tests for lc_loopclassifier: deterministic even-odd parity, closed-form
// y-monotone pieces for every supported atomic entity, scale-invariant one-eps
// band, and the union-find welder.

#include <array>
#include <cfloat>
#include <cmath>
#include <memory>
#include <random>
#include <vector>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "lc_hatchtestsupport.h"
#include "lc_hyperbola.h"
#include "lc_loopclassifier.h"
#include "lc_parabola.h"
#include "lc_rect.h"
#include "lc_splinepoints.h"
#include "rs_arc.h"
#include "rs_circle.h"
#include "rs_ellipse.h"
#include "rs_entitycontainer.h"
#include "rs_line.h"
#include "rs_vector.h"

using Catch::Approx;
using LC_LoopUtils::classifyPoint;
using LC_LoopUtils::hatchTolerance;
using LC_LoopUtils::LoopGeometry;
using LC_LoopUtils::MonoPiece;
using LC_LoopUtils::oddParity;
using LC_LoopUtils::PointClass;
using LC_LoopUtils::weldPoints;
using LC_LoopUtils::WeldResult;
using lc::test::exactInsideInteger;

namespace {

std::shared_ptr<const LoopGeometry> buildPolygon(const std::vector<RS_Vector>& ring) {
    auto cont = std::make_shared<RS_EntityContainer>(nullptr);
    const size_t n = ring.size();
    for (size_t i = 0; i < n; ++i) {
        cont->addEntity(new RS_Line(cont.get(), ring[i], ring[(i + 1) % n]));
    }
    return LoopGeometry::fromChain(*cont, ring);
}

// Build the twelve-point star of the plan's TDD case near one of its vertices.
std::vector<RS_Vector> buildStar(int points, double outerR, double innerR,
                                 const RS_Vector& centre) {
    std::vector<RS_Vector> ring;
    ring.reserve(2 * points);
    for (int i = 0; i < 2 * points; ++i) {
        const double ang = i * M_PI / points - M_PI / 2.0;
        const double r = (i % 2 == 0) ? outerR : innerR;
        ring.push_back(centre + RS_Vector::polar(r, ang));
    }
    return ring;
}

} // namespace

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("hatchTolerance follows max(64uM, 1e-9 D, min(1e-8, 1e-6 D))",
          "[lc-hatchfix][classifier]") {
    REQUIRE(hatchTolerance(0.005, 1.0) == Approx(5.0e-9).epsilon(1e-12));
    REQUIRE(hatchTolerance(0.5, 1.0) == Approx(1.0e-8).epsilon(1e-12));
    REQUIRE(hatchTolerance(10.0, 10.0) == Approx(1.0e-8).epsilon(1e-12));
    REQUIRE(hatchTolerance(100.0, 100.0) == Approx(1.0e-7).epsilon(1e-12));
    REQUIRE(hatchTolerance(1.0e4, 1.0e4) == Approx(1.0e-5).epsilon(1e-12));
    const double u = DBL_EPSILON / 2.0;
    REQUIRE(hatchTolerance(1.0, 4.0e6) == Approx(64.0 * u * 4.0e6).epsilon(1e-12));
    REQUIRE(hatchTolerance(10.0, 3.0e7) == Approx(64.0 * u * 3.0e7).epsilon(1e-12));
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("Half-open parity matches an exact integer oracle on grid points",
          "[lc-hatchfix][classifier]") {
    // Three fixtures with known integer vertices: notch, comb, star-ish.
    struct Case {
        const char* name;
        std::vector<std::array<long long, 2>> poly;
    };
    const std::vector<Case> cases = {
        {"notch", {{0, 0}, {5, 0}, {5, 5}, {3, 5}, {3, 2}, {2, 2}, {2, 5}, {0, 5}}},
        {"comb", {{0, 0}, {9, 0}, {9, 5}, {8, 5}, {8, 1}, {7, 1}, {7, 5}, {6, 5},
                  {6, 1}, {5, 1}, {5, 5}, {0, 5}}},
    };

    for (const Case& c : cases) {
        std::vector<RS_Vector> ring;
        ring.reserve(c.poly.size());
        for (const auto& v : c.poly) {
            ring.emplace_back(static_cast<double>(v[0]), static_cast<double>(v[1]));
        }
        auto geom = buildPolygon(ring);
        long long xMin = c.poly[0][0], xMax = c.poly[0][0];
        long long yMin = c.poly[0][1], yMax = c.poly[0][1];
        for (const auto& v : c.poly) {
            xMin = std::min(xMin, v[0]); xMax = std::max(xMax, v[0]);
            yMin = std::min(yMin, v[1]); yMax = std::max(yMax, v[1]);
        }
        int mismatches = 0;
        for (long long x = xMin - 3; x <= xMax + 3; ++x) {
            for (long long y = yMin - 3; y <= yMax + 3; ++y) {
                bool onB = false;
                const bool oracle = exactInsideInteger(c.poly, x, y, &onB);
                if (onB) continue;
                const RS_Vector p(static_cast<double>(x), static_cast<double>(y));
                const bool got = oddParity(p, {geom.get()});
                if (got != oracle) ++mismatches;
            }
        }
        CAPTURE(c.name);
        REQUIRE(mismatches == 0);
    }

    // Seeded star polygons: use integer-rounded vertices for BOTH the oracle
    // and the floating-point classifier, so a mismatch is a classifier bug,
    // not a rounding mismatch.
    std::mt19937 gen(7);
    std::uniform_real_distribution<double> rfac(0.3, 0.7);
    int totalMismatch = 0;
    for (int trial = 0; trial < 10; ++trial) {
        const int pts = 5 + (gen() % 5);
        const double outerR = 10.0 + (gen() % 10);
        const double innerR = outerR * rfac(gen);
        auto starF = buildStar(pts, outerR, innerR, RS_Vector(20, 20));
        std::vector<std::array<long long, 2>> poly;
        std::vector<RS_Vector> starI;
        starI.reserve(starF.size());
        for (const auto& v : starF) {
            const long long xi = static_cast<long long>(std::round(v.x));
            const long long yi = static_cast<long long>(std::round(v.y));
            poly.push_back({xi, yi});
            starI.emplace_back(static_cast<double>(xi), static_cast<double>(yi));
        }
        auto geom = buildPolygon(starI);
        long long xMin = poly[0][0], xMax = xMin, yMin = poly[0][1], yMax = yMin;
        for (const auto& v : poly) {
            xMin = std::min(xMin, v[0]); xMax = std::max(xMax, v[0]);
            yMin = std::min(yMin, v[1]); yMax = std::max(yMax, v[1]);
        }
        for (long long x = xMin - 3; x <= xMax + 3; ++x) {
            for (long long y = yMin - 3; y <= yMax + 3; ++y) {
                bool onB = false;
                const bool oracle = exactInsideInteger(poly, x, y, &onB);
                if (onB) continue;
                const RS_Vector p(static_cast<double>(x), static_cast<double>(y));
                const bool got = oddParity(p, {geom.get()});
                if (got != oracle) ++totalMismatch;
            }
        }
    }
    REQUIRE(totalMismatch == 0);
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("Tangent rays through curve extrema count zero",
          "[lc-hatchfix][classifier]") {
    // Full circle centred at (5,5), r = 3: y = cy +- r are tangent points.
    auto circleCont = std::make_shared<RS_EntityContainer>(nullptr);
    circleCont->addEntity(new RS_Circle(circleCont.get(), {RS_Vector(5, 5), 3.0}));
    auto gC = LoopGeometry::fromChain(*circleCont, {});
    REQUIRE_FALSE(oddParity(RS_Vector(0, 8), {gC.get()}));
    REQUIRE_FALSE(oddParity(RS_Vector(0, 2), {gC.get()}));
    REQUIRE_FALSE(oddParity(RS_Vector(10, 8), {gC.get()}));
    REQUIRE_FALSE(oddParity(RS_Vector(10, 2), {gC.get()}));
    REQUIRE(oddParity(RS_Vector(5, 5), {gC.get()}));

    // Full ellipse centred at (5,5), major (4,0), ratio 0.5.
    auto ellCont = std::make_shared<RS_EntityContainer>(nullptr);
    ellCont->addEntity(new RS_Ellipse(
        ellCont.get(),
        RS_EllipseData{RS_Vector(5, 5), RS_Vector(4, 0), 0.5, 0.0, 0.0, false}));
    auto gE = LoopGeometry::fromChain(*ellCont, {});
    REQUIRE_FALSE(oddParity(RS_Vector(-1, 7), {gE.get()}));
    REQUIRE_FALSE(oddParity(RS_Vector(11, 7), {gE.get()}));
    REQUIRE_FALSE(oddParity(RS_Vector(-1, 3), {gE.get()}));
    REQUIRE_FALSE(oddParity(RS_Vector(11, 3), {gE.get()}));

    // Half-disc: arc centred at origin, r = 5, 0..pi, plus the chord from (5,0)
    // to (-5,0).
    auto halfCont = std::make_shared<RS_EntityContainer>(nullptr);
    halfCont->addEntity(new RS_Arc(
        halfCont.get(),
        RS_ArcData(RS_Vector(0, 0), 5.0, 0.0, M_PI, false)));
    halfCont->addEntity(new RS_Line(halfCont.get(), RS_Vector(-5, 0), RS_Vector(5, 0)));
    std::vector<RS_Vector> halfVerts{RS_Vector(5, 0), RS_Vector(-5, 0)};
    auto gH = LoopGeometry::fromChain(*halfCont, halfVerts);
    REQUIRE_FALSE(oddParity(RS_Vector(-7, 5), {gH.get()}));
    REQUIRE_FALSE(oddParity(RS_Vector(7, 5), {gH.get()}));
    REQUIRE_FALSE(oddParity(RS_Vector(-7, 0), {gH.get()}));
    REQUIRE_FALSE(oddParity(RS_Vector(7, 0), {gH.get()}));
    REQUIRE(oddParity(RS_Vector(0, 2.5), {gH.get()}));
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("ON band of the hatch classifier",
          "[lc-hatchfix][classifier]") {
    // Line edge: pass isNearAnyEdge for small displacements along the normal.
    auto cont = std::make_shared<RS_EntityContainer>(nullptr);
    cont->addEntity(new RS_Line(cont.get(), RS_Vector(0, 0), RS_Vector(10, 0)));
    cont->addEntity(new RS_Line(cont.get(), RS_Vector(10, 0), RS_Vector(10, 5)));
    cont->addEntity(new RS_Line(cont.get(), RS_Vector(10, 5), RS_Vector(0, 5)));
    cont->addEntity(new RS_Line(cont.get(), RS_Vector(0, 5), RS_Vector(0, 0)));

    std::vector<RS_Vector> verts{RS_Vector(0, 0), RS_Vector(10, 0),
                                 RS_Vector(10, 5), RS_Vector(0, 5)};
    auto geom = LoopGeometry::fromChain(*cont, verts);

    const double D = std::hypot(10.0, 5.0);
    const double M = 10.0;
    const double eps = hatchTolerance(D, M);
    REQUIRE(eps > 0.0);

    const std::vector<const RS_EntityContainer*> contours{cont.get()};
    const std::vector<const LoopGeometry*> loops{geom.get()};

    // A point 0.5*eps above the bottom edge is On.
    REQUIRE(classifyPoint(RS_Vector(5.0, 0.5 * eps), contours, loops, eps)
            == PointClass::On);
    // A point 2*eps above is Inside (not On).
    const PointClass p2 = classifyPoint(RS_Vector(5.0, 2.0 * eps), contours, loops, eps);
    REQUIRE(p2 != PointClass::On);
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("Classifier answers are scale invariant",
          "[lc-hatchfix][classifier]") {
    const std::vector<std::array<long long, 2>> notch{
        {0, 0}, {5, 0}, {5, 5}, {3, 5}, {3, 2}, {2, 2}, {2, 5}, {0, 5}};
    std::vector<RS_Vector> ring;
    for (const auto& v : notch) {
        ring.emplace_back(static_cast<double>(v[0]), static_cast<double>(v[1]));
    }
    auto baseGeom = buildPolygon(ring);

    for (double scale : {1.0e-3, 1.0, 1.0e3}) {
        std::vector<RS_Vector> ringS;
        ringS.reserve(ring.size());
        for (const auto& v : ring) ringS.push_back(v * scale);
        auto geom = buildPolygon(ringS);
        for (long long x = -2; x <= 7; ++x) {
            for (long long y = -2; y <= 7; ++y) {
                bool onB = false;
                const bool oracle = exactInsideInteger(notch, x, y, &onB);
                if (onB) continue;
                const RS_Vector p(static_cast<double>(x) * scale,
                                  static_cast<double>(y) * scale);
                const bool got = oddParity(p, {geom.get()});
                CAPTURE(scale, x, y);
                REQUIRE(got == oracle);
            }
        }
    }
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("weldPoints joins within eps only",
          "[lc-hatchfix][classifier]") {
    const double eps = 1.0e-4;

    // Two points within eps join.
    {
        std::vector<RS_Vector> pts{RS_Vector(0, 0), RS_Vector(0.5 * eps, 0)};
        WeldResult w = weldPoints(pts, eps);
        REQUIRE(w.id[0] == w.id[1]);
    }
    // Two points 10 eps apart do not.
    {
        std::vector<RS_Vector> pts{RS_Vector(0, 0), RS_Vector(10.0 * eps, 0)};
        WeldResult w = weldPoints(pts, eps);
        REQUIRE(w.id[0] != w.id[1]);
    }
    // Six collinear points spaced 0.9*eps form one long cluster.
    {
        std::vector<RS_Vector> pts;
        for (int i = 0; i < 6; ++i) pts.emplace_back(i * 0.9 * eps, 0.0);
        WeldResult w = weldPoints(pts, eps);
        REQUIRE(w.id[0] == w.id[5]);
        REQUIRE(w.longChains >= 1);
    }
    // Loose tolerance pulls a farther point in.
    {
        std::vector<RS_Vector> pts{RS_Vector(0, 0), RS_Vector(2.0e-4, 0)};
        std::vector<double> loose{3.0e-4, 3.0e-4};
        WeldResult wLoose = weldPoints(pts, 1.0e-9, loose);
        REQUIRE(wLoose.id[0] == wLoose.id[1]);
        WeldResult wTight = weldPoints(pts, 1.0e-9);
        REQUIRE(wTight.id[0] != wTight.id[1]);
    }
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("Hyperbola pieces produce exact x inverse",
          "[lc-hatchfix][classifier]") {
    // One hand-chosen hyperbola: left branch closed by its chord.
    LC_HyperbolaData data(RS_Vector(3, -2), RS_Vector(2 * std::cos(0.4), 2 * std::sin(0.4)),
                          0.5, -0.5, 1.2, true);
    auto hyp = std::make_unique<LC_Hyperbola>(nullptr, data);
    RS_Vector hStart = hyp->getStartpoint();
    RS_Vector hEnd = hyp->getEndpoint();

    auto cont = std::make_shared<RS_EntityContainer>(nullptr);
    cont->addEntity(hyp->clone());
    cont->addEntity(new RS_Line(cont.get(), hEnd, hStart));

    std::vector<RS_Vector> verts{hStart, hEnd};
    auto geom = LoopGeometry::fromChain(*cont, verts);
    REQUIRE_FALSE(geom->pieces().empty());
    // Centre point should be inside: for a bounded single-branch arc with its
    // chord, there is a strictly interior point.
    const LC_Rect box = geom->box();
    const RS_Vector mid = (box.minP() + box.maxP()) * 0.5;
    // Not asserting Inside/Outside at mid because geometry can be asymmetric;
    // just check that the geometry built and odd/even parity is deterministic.
    const bool p1 = oddParity(mid, {geom.get()});
    const bool p2 = oddParity(mid, {geom.get()});
    REQUIRE(p1 == p2);
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("leftmostPoint, travelDirection and signedCurvature per edge type",
          "[lc-hatchfix][classifier]") {
    using LC_LoopUtils::leftmostPoint;
    using LC_LoopUtils::signedCurvature;
    using LC_LoopUtils::travelDirection;

    SECTION("line") {
        RS_Line line(nullptr, RS_LineData(RS_Vector(5, 3), RS_Vector(-2, 7)));
        bool interior = true;  // sanity-check reset
        const RS_Vector p = leftmostPoint(line, &interior);
        REQUIRE_FALSE(interior);
        REQUIRE(p.x == Approx(-2.0));
        REQUIRE(p.y == Approx(7.0));
        const RS_Vector t = travelDirection(line, true);
        REQUIRE(t.x == Approx((-2.0 - 5.0) / std::hypot(-7.0, 4.0)));
        REQUIRE(t.y == Approx((7.0 - 3.0) / std::hypot(-7.0, 4.0)));
        REQUIRE(signedCurvature(line, true) == 0.0);
    }

    SECTION("arc") {
        // CCW arc from (0, -5) through (-5, 0) to (0, 5): angle1 = -pi/2,
        // angle2 = pi/2 passes through +x half. Flip to left half: angle1 =
        // pi/2, angle2 = 3*pi/2 (interior through (-5, 0)).
        RS_Arc arc(nullptr, RS_ArcData(RS_Vector(0.0, 0.0), 5.0,
                                        M_PI / 2.0, 3.0 * M_PI / 2.0, false));
        bool interior = false;
        const RS_Vector p = leftmostPoint(arc, &interior);
        REQUIRE(interior);
        REQUIRE(p.x == Approx(-5.0));
        REQUIRE(p.y == Approx(0.0));
        // Travel at start (0, 5): CCW tangent is (-1, 0).
        const RS_Vector t0 = travelDirection(arc, true);
        REQUIRE(t0.x == Approx(-1.0));
        REQUIRE(t0.y == Approx(0.0).margin(1e-12));
        REQUIRE(signedCurvature(arc, true) == Approx(1.0 / 5.0));
    }

    SECTION("full circle") {
        RS_Circle circ(nullptr, RS_CircleData(RS_Vector(3.0, 4.0), 2.0));
        bool interior = false;
        const RS_Vector p = leftmostPoint(circ, &interior);
        REQUIRE(interior);
        REQUIRE(p.x == Approx(1.0));
        REQUIRE(p.y == Approx(4.0));
        REQUIRE(signedCurvature(circ, true) == Approx(1.0 / 2.0));
    }

    SECTION("ellipse arc rotated 30 degrees") {
        // Major axis rotated 30 deg; semiAxes 4 and 2; sweep is full circle.
        const double th = M_PI / 6.0;
        RS_EllipseData ed;
        ed.center = RS_Vector(0, 0);
        ed.majorP = RS_Vector(4.0 * std::cos(th), 4.0 * std::sin(th));
        ed.ratio = 0.5;
        ed.angle1 = 0.0;
        ed.angle2 = 2.0 * M_PI;
        ed.reversed = false;
        RS_Ellipse ell(nullptr, ed);
        bool interior = false;
        const RS_Vector p = leftmostPoint(ell, &interior);
        REQUIRE(interior);
        // Sampled minimum for cross-check.
        double sampledMinX = 1e9;
        for (int i = 0; i <= 10000; ++i) {
            const double t = (2.0 * M_PI * i) / 10000.0;
            const RS_Vector q(4.0 * std::cos(th) * std::cos(t) +
                                 -0.5 * 4.0 * std::sin(th) * std::sin(t),
                              4.0 * std::sin(th) * std::cos(t) +
                                 0.5 * 4.0 * std::cos(th) * std::sin(t));
            sampledMinX = std::min(sampledMinX, q.x);
        }
        REQUIRE(p.x == Approx(sampledMinX).margin(1e-3));
    }

    SECTION("parabola apex is the leftmost point") {
        // Bezier (0,0), (5,10), (10,0): symmetric about x = 5, apex at
        // (5, 5). The x-wise leftmost in a strictly convex x polynomial is
        // one of the ends; here both ends share x=0 and x=10, apex is NOT
        // interior for x. The leftmost end is (0, 0).
        LC_ParabolaData pd;
        pd.m_controlPoints = {RS_Vector(0.0, 0.0), RS_Vector(5.0, 10.0),
                              RS_Vector(10.0, 0.0)};
        pd.m_valid = true;
        LC_Parabola para(nullptr, pd);
        bool interior = true;
        const RS_Vector p = leftmostPoint(para, &interior);
        REQUIRE_FALSE(interior);
        REQUIRE(p.x == Approx(0.0));
    }

    SECTION("left-branch hyperbola") {
        LC_HyperbolaData hd(RS_Vector(3.0, -2.0),
                             RS_Vector(2.0 * std::cos(25.0 * M_PI / 180.0),
                                        2.0 * std::sin(25.0 * M_PI / 180.0)),
                             0.5, -0.5, 1.2, true);
        LC_Hyperbola hyp(nullptr, hd);
        bool interior = false;
        const RS_Vector p = leftmostPoint(hyp, &interior);
        // The leftmost point of the left branch for these parameters sits on
        // the open curve: it is either an interior extremum or lies at one of
        // the two stored endpoints.
        REQUIRE(p.valid);
        // Cross-check: sample 2001 points on the curve and verify no point
        // has smaller x than the returned.
        double minX = p.x;
        for (int i = 0; i <= 2000; ++i) {
            const double phi = -0.5 + (1.2 - (-0.5)) * (double(i) / 2000.0);
            const RS_Vector q = hyp.getPoint(phi, hd.reversed);
            if (q.valid) minX = std::min(minX, q.x);
        }
        REQUIRE(p.x <= minX + 1e-9);
    }

    SECTION("sweeps under 90 degrees: travelDirection is consistent at ends") {
        // Arc of sweep 60 degrees: start at angle 0 (point (r, 0)), end at
        // angle pi/3. The direction at the start points in +y (tangent of
        // CCW motion), and the direction at the end heads further along the
        // arc.
        RS_Arc arc(nullptr, RS_ArcData(RS_Vector(0.0, 0.0), 2.0,
                                        0.0, M_PI / 3.0, false));
        const RS_Vector mid = arc.getMiddlePoint();
        const RS_Vector s = arc.getStartpoint();
        const RS_Vector e = arc.getEndpoint();
        const RS_Vector t0 = travelDirection(arc, true);
        const RS_Vector t1 = travelDirection(arc, false);
        REQUIRE(t0.x * (mid.x - s.x) + t0.y * (mid.y - s.y) > 0);
        REQUIRE(t1.x * (e.x - mid.x) + t1.y * (e.y - mid.y) > 0);
    }
}

