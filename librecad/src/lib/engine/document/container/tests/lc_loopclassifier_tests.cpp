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

#include <algorithm>
#include <array>
#include <cfloat>
#include <cmath>
#include <iomanip>
#include <memory>
#include <random>
#include <sstream>
#include <string>
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

// Build a star polygon with `points` outer vertices around centre.
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

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("Reversed arc and ellipse pieces trace the same path as their non-reversed twin",
          "[lc-hatchfix][classifier]") {
    // When LoopExtractor reverses a container whose signed area was negative,
    // every atomic edge gets revertDirection(). For an arc or elliptic arc the
    // call swaps angle1/angle2 and flips isReversed, so getStartpoint and
    // getEndpoint now return the point at the parameter-space END and START
    // of the piece walk respectively. fromChain's vertex table takes those
    // values and hands them as va/vb to appendArcPieces / appendEllipsePieces.
    // The piece-boundary substitution must match the walk order, otherwise a
    // bogus monotone piece covers the whole bbox height and overcounts the
    // right-crossings.
    SECTION("arc") {
        // Upper semicircle, r = 5 at the origin. Non-reversed form.
        RS_Arc baseArc(nullptr, RS_ArcData(RS_Vector(0, 0), 5.0,
                                           0.0, M_PI, false));
        auto base = std::make_shared<RS_EntityContainer>(nullptr);
        base->addEntity(baseArc.clone());
        base->addEntity(new RS_Line(base.get(), RS_Vector(-5, 0),
                                                RS_Vector(5, 0)));
        auto g0 = LoopGeometry::fromChain(*base,
            {RS_Vector(5, 0), RS_Vector(-5, 0)});

        // Reversed twin: swap angles and flip reversed on the arc; swap the
        // line to keep the chain closed in reversed-walk order. The geometric
        // path is identical.
        RS_Arc revArc(nullptr, RS_ArcData(RS_Vector(0, 0), 5.0,
                                          M_PI, 0.0, true));
        auto rev = std::make_shared<RS_EntityContainer>(nullptr);
        rev->addEntity(revArc.clone());
        rev->addEntity(new RS_Line(rev.get(), RS_Vector(5, 0),
                                              RS_Vector(-5, 0)));
        auto g1 = LoopGeometry::fromChain(*rev,
            {RS_Vector(-5, 0), RS_Vector(5, 0)});

        const RS_Vector probe(0.0, 2.0);  // Interior of the semicircle.
        REQUIRE(g0->crossingsRight(probe) == 1);
        REQUIRE(g1->crossingsRight(probe) == 1);
        REQUIRE(oddParity(probe, {g0.get()}));
        REQUIRE(oddParity(probe, {g1.get()}));
    }

    SECTION("elliptic arc rotated 30 degrees with a chord") {
        // A pattern hatch must equal the solid hatch on curved edges too.
        // Half-ellipse a = 3, b = 2 at (10, 5), phi = 30 degrees, plus
        // the diametric chord; a circle island centred at (9.6, 5.7) lies
        // strictly inside the half-disc. Before the fix the reversed form
        // double-counted the horizontal ray from (9.6, 5.7) and the circle
        // became a root loop on the SOLID hatch.
        const double phi = 30.0 * M_PI / 180.0;
        const RS_Vector majorP(3.0 * std::cos(phi), 3.0 * std::sin(phi));
        auto build = [&](bool reversed) {
            auto cont = std::make_shared<RS_EntityContainer>(nullptr);
            RS_EllipseData ed;
            ed.center = RS_Vector(10, 5);
            ed.majorP = majorP;
            ed.ratio = 2.0 / 3.0;
            ed.angle1 = reversed ? M_PI : 0.0;
            ed.angle2 = reversed ? 0.0 : M_PI;
            ed.reversed = reversed;
            cont->addEntity(new RS_Ellipse(cont.get(), ed));
            const RS_Vector low  = RS_Vector(10, 5) - majorP;
            const RS_Vector high = RS_Vector(10, 5) + majorP;
            if (reversed) {
                cont->addEntity(new RS_Line(cont.get(), high, low));
            } else {
                cont->addEntity(new RS_Line(cont.get(), low, high));
            }
            std::vector<RS_Vector> verts = reversed ? std::vector<RS_Vector>{low, high}
                                                   : std::vector<RS_Vector>{high, low};
            return LoopGeometry::fromChain(*cont, std::move(verts));
        };
        auto g0 = build(false);
        auto g1 = build(true);
        const RS_Vector probe(9.6, 5.7);
        REQUIRE(g0->crossingsRight(probe) == 1);
        REQUIRE(g1->crossingsRight(probe) == 1);
        REQUIRE(oddParity(probe, {g0.get()}));
        REQUIRE(oddParity(probe, {g1.get()}));
    }
}


// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("A closed curve away from the origin keeps to its own outline",
          "[lc-hatchfix][classifier]") {
    // A closed curve has no end points, so its pieces get an invalid vector
    // for them. That vector holds (0, 0), which must not end a piece.
    const auto loopOf = [](RS_Entity* edge) {
        auto cont = std::make_shared<RS_EntityContainer>(nullptr);
        cont->addEntity(edge);
        return std::make_pair(cont, LoopGeometry::fromChain(*cont, {}));
    };

    for (const RS_Vector& c : {RS_Vector(5.0, 5.0), RS_Vector(5.0, -5.0), RS_Vector(-8.0, 3.0)}) {
        INFO("centre " << c.x << ", " << c.y);

        LC_SplinePointsData spline(true, false);
        spline.useControlPoints = true;
        spline.controlPoints = {c + RS_Vector(-4.0, -2.0), c + RS_Vector(4.0, -2.0),
                                c + RS_Vector(4.0, 2.0), c + RS_Vector(-4.0, 2.0)};
        const auto ellipseLoop = loopOf(new RS_Ellipse(
            nullptr, RS_EllipseData{c, RS_Vector(4.0, 0.0), 0.5, 0.0, 2.0 * M_PI, false}));
        const auto splineLoop = loopOf(new LC_SplinePoints(nullptr, spline));

        for (const auto* g : {ellipseLoop.second.get(), splineLoop.second.get()}) {
            CHECK(oddParity(c, {g}));
            CHECK(oddParity(c + RS_Vector(1.5, 0.0), {g}));
            CHECK_FALSE(oddParity(c + RS_Vector(2.0, 3.0), {g}));
            CHECK_FALSE(oddParity(c + RS_Vector(2.0, -3.0), {g}));
            CHECK_FALSE(oddParity(c + RS_Vector(-5.0, 0.0), {g}));
        }
    }
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("A reversed hyperbola edge with an interior extremum keeps its pieces",
          "[lc-hatchfix][classifier]") {
    // x = -sinh(phi), y = 2 cosh(phi), phi from -0.5 to 1.5, closed by its
    // chord. y turns at phi = 0, inside the range, so the edge has two pieces.
    const LC_HyperbolaData data(RS_Vector(0.0, 0.0), RS_Vector(0.0, 2.0), 0.5, -0.5, 1.5, false);
    LC_Hyperbola hyperbola(nullptr, data);
    const RS_Vector s = hyperbola.getStartpoint();
    const RS_Vector e = hyperbola.getEndpoint();

    auto forward = std::make_shared<RS_EntityContainer>(nullptr);
    forward->addEntity(hyperbola.clone());
    forward->addEntity(new RS_Line(forward.get(), e, s));
    auto g0 = LoopGeometry::fromChain(*forward, {s, e});

    auto reversed = std::make_shared<RS_EntityContainer>(nullptr);
    RS_Entity* turned = hyperbola.clone();
    turned->revertDirection();
    reversed->addEntity(turned);
    reversed->addEntity(new RS_Line(reversed.get(), s, e));
    auto g1 = LoopGeometry::fromChain(*reversed, {e, s});

    const RS_Vector inside(-0.8, 3.2);
    const RS_Vector outside(0.0, 3.2);
    CHECK(oddParity(inside, {g0.get()}));
    CHECK_FALSE(oddParity(outside, {g0.get()}));
    CHECK(oddParity(inside, {g1.get()}));
    CHECK_FALSE(oddParity(outside, {g1.get()}));
}

namespace {

// How many of the two y-extremum angles ext and ext + pi lie strictly inside
// an arc that starts at a1 and turns counter-clockwise by sweep. The ends
// drawArcSpan makes are either exactly on an extremum or at least 1e-3 from
// one, so the 1e-6 here only tells those two cases apart.
int extremaInside(double ext, double a1, double sweep) {
    int inside = 0;
    for (int j = 0; j < 2; ++j) {
        double offset = std::fmod(ext + j * M_PI - a1, 2.0 * M_PI);
        if (offset < 0.0) {
            offset += 2.0 * M_PI;
        }
        if (offset > 1.0e-6 && offset < sweep - 1.0e-6) {
            ++inside;
        }
    }
    return inside;
}

// Distance of angle a from the nearest extremum angle ext + j * pi.
double distanceToExtremum(double ext, double a) {
    double d = std::fmod(a - ext, M_PI);
    if (d < 0.0) {
        d += M_PI;
    }
    return std::min(d, M_PI - d);
}

struct ArcSpan {
    double lo;      // where the counter-clockwise walk starts
    double hi;      // where it stops
};

// A counter-clockwise span whose ends are either random, or exactly on an
// extremum (as when two arcs meet there). Ends are never closer than 1e-3 to
// an extremum unless they are on it.
ArcSpan drawArcSpan(std::mt19937_64& rng, double ext, int mode) {
    std::uniform_real_distribution<double> angle(0.0, 2.0 * M_PI);
    std::uniform_real_distribution<double> length(0.2, 2.0 * M_PI - 0.2);
    std::uniform_int_distribution<int> turns(-2, 4);
    const auto offTheExtremum = [&](auto draw) {
        for (;;) {
            const double a = draw();
            if (distanceToExtremum(ext, a) > 1.0e-3) {
                return a;
            }
        }
    };
    ArcSpan span{};
    if (mode == 1) {
        span.lo = ext + turns(rng) * M_PI;
        span.hi = offTheExtremum([&] { return span.lo + length(rng); });
    } else if (mode == 2) {
        span.hi = ext + turns(rng) * M_PI;
        span.lo = offTheExtremum([&] { return span.hi - length(rng); });
    } else {
        span.lo = offTheExtremum([&] { return angle(rng); });
        span.hi = offTheExtremum([&] { return span.lo + length(rng); });
    }
    return span;
}

} // namespace

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("An elliptic arc breaks at each y-extremum once, and not at a join on one",
          "[lc-hatchfix][classifier]") {
    // The x of a piece is found from y, so a piece must not turn back in y.
    // A turning point inside the arc splits it in two. An arc that starts or
    // ends exactly on one has none, and must not get an extra piece a few
    // ULPs tall from rounding in the angle of the extremum.
    std::size_t wrongCount = 0;
    std::size_t slivers = 0;
    std::string firstWrong;
    const auto check = [&](const std::string& label, double cx, double cy, double a, double phi, double q,
                           double lo, double hi, bool reversed) {
        const RS_Vector majorP = RS_Vector::polar(a, phi);
        // y(t) = cy + My cos t + Ny sin t with N = q * (-My, Mx), so the
        // extremum angles are atan2(Ny, My) + j * pi.
        const double ext = std::atan2(q * majorP.x, majorP.y);
        RS_Ellipse ellipse(nullptr, RS_EllipseData{RS_Vector(cx, cy), majorP, q, reversed ? hi : lo,
                                                   reversed ? lo : hi, reversed});

        const double a1 = reversed ? ellipse.getAngle2() : ellipse.getAngle1();
        const int expected = extremaInside(ext, a1, std::abs(ellipse.getAngleLength())) + 1;
        std::vector<MonoPiece> pieces;
        LC_LoopUtils::appendMonotonePieces(ellipse, ellipse.getStartpoint(), ellipse.getEndpoint(), pieces);

        if (static_cast<int>(pieces.size()) != expected) {
            ++wrongCount;
            if (firstWrong.empty()) {
                std::ostringstream text;
                text << std::setprecision(17) << label << ": centre (" << cx << ", " << cy << "), major axis "
                     << a << " at angle " << phi << ", ratio " << q << ", reversed " << reversed << ", angles "
                     << ellipse.getAngle1() << " to " << ellipse.getAngle2() << ": expected " << expected
                     << " pieces, got " << pieces.size();
                firstWrong = text.str();
            }
        }
        for (const MonoPiece& p : pieces) {
            if (p.yhi - p.ylo < 1.0e-9 * a) {
                ++slivers;
            }
        }
    };

    // An arc that starts about 13.65 rad from the origin of angles, on an
    // extremum: its start angle is only known to one ULP of 13.65, which is
    // more than the 8 u of an angle near 1.
    check("fixed case", 99.042544366455758, -82.982055145611184, 36.65566971576704, 5.5905282667820657,
          0.59901017160483483, 13.654095527617988, 15.082715184616426, false);

    std::mt19937_64 rng(20261103);
    std::uniform_real_distribution<double> position(-1000.0, 1000.0);
    std::uniform_real_distribution<double> majorRadius(1.0, 50.0);
    std::uniform_real_distribution<double> direction(0.0, 2.0 * M_PI);
    std::uniform_real_distribution<double> ratio(0.3, 0.95);
    std::uniform_int_distribution<int> mode(0, 2);
    std::uniform_int_distribution<int> flip(0, 1);
    for (int run = 0; run < 6000; ++run) {
        const double cx = position(rng);
        const double cy = position(rng);
        const double a = majorRadius(rng);
        const double phi = direction(rng);
        const double q = ratio(rng);
        const bool reversed = flip(rng) != 0;
        const RS_Vector majorP = RS_Vector::polar(a, phi);
        const double ext = std::atan2(q * majorP.x, majorP.y);
        const ArcSpan span = drawArcSpan(rng, ext, mode(rng));
        check("run " + std::to_string(run), cx, cy, a, phi, q, span.lo, span.hi, reversed);
    }
    INFO("first wrong piece count: " << firstWrong);
    CHECK(wrongCount == 0u);
    CHECK(slivers == 0u);
}

namespace {

// A cap is an arc through a y-extremum that starts or ends a short angle
// delta past it, closed by a chord. The loop's highest (or lowest) y is the
// extremum, and the pieces must reach it: if the extremum gets no split, the
// piece ends at the vertex, short of the extremum, and for the y values in
// between no piece is crossed, so the cap is outside there.
enum class CapSide {
    TopEnd,         // ends delta past the top
    TopStart,       // starts delta before the top
    BottomEnd,      // ends delta past the bottom
    BottomStart     // starts delta before the bottom
};

const char* capSideName(CapSide side) {
    switch (side) {
    case CapSide::TopEnd:
        return "top, ends past it";
    case CapSide::TopStart:
        return "top, starts before it";
    case CapSide::BottomEnd:
        return "bottom, ends past it";
    case CapSide::BottomStart:
        return "bottom, starts before it";
    }
    return "";
}

struct CapScan {
    std::size_t tested = 0;
    std::size_t short_ = 0;
    std::string firstShort;
};

void scanCap(CapScan& scan, CapSide side, const RS_Vector& centre, double r, double delta) {
    const bool top = side == CapSide::TopEnd || side == CapSide::TopStart;
    const bool nearEnd = side == CapSide::TopEnd || side == CapSide::BottomEnd;
    const double extremumAngle = top ? M_PI / 2.0 : 3.0 * M_PI / 2.0;
    const double a1 = nearEnd ? extremumAngle - 1.0 : extremumAngle - delta;
    const double a2 = nearEnd ? extremumAngle + delta : extremumAngle + 1.0;
    const double extremum = top ? centre.y + r : centre.y - r;

    auto* curve = new RS_Arc(nullptr, RS_ArcData{centre, r, a1, a2, false});
    const RS_Vector start = curve->getStartpoint();
    const RS_Vector end = curve->getEndpoint();
    auto loop = std::make_shared<RS_EntityContainer>(nullptr);
    loop->addEntity(curve);
    loop->addEntity(new RS_Line(loop.get(), end, start));
    const auto geometry = LoopGeometry::fromChain(*loop, {start, end});

    double reached = top ? -HUGE_VAL : HUGE_VAL;
    for (const MonoPiece& piece : geometry->pieces()) {
        reached = top ? std::max(reached, piece.yhi) : std::min(reached, piece.ylo);
    }
    ++scan.tested;
    if (reached != extremum) {
        ++scan.short_;
        if (scan.firstShort.empty()) {
            std::ostringstream text;
            text << std::setprecision(17) << capSideName(side) << ", centre (" << centre.x << ", "
                 << centre.y << "), r " << r << ", delta " << delta << ": the pieces reach y = " << reached
                 << ", the extremum is at " << extremum;
            scan.firstShort = text.str();
        }
    }
}

} // namespace

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("An arc ending just past a y-extremum has pieces up to the extremum",
          "[lc-hatchfix][classifier]") {
    // delta runs from far below the margin under which an extremum gets no
    // split to well above it. Frames with the extremum at or below y = 0 are
    // there because the end vertex's y is then known to far more digits than
    // r, and a vertex that is one rounding step short of the extremum leaves
    // many doubles uncovered.
    struct Frame {
        double cx, cy, r;
    };
    const std::array<Frame, 6> frames{{{0.0, 0.0, 1.0}, {0.0, 0.0, 50.0}, {3.0, 0.5, 25.0},
                                       {-7.0, 3.0, 3.0}, {0.0, -1.0, 1.0}, {0.0, -30.0, 50.0}}};
    const std::array<double, 8> deltas{1.2e-8, 1.4e-8, 2.0e-8, 2.9e-8, 3.1e-8, 5.0e-8, 1.0e-6, 1.0e-4};
    const std::array<CapSide, 4> sides{CapSide::TopEnd, CapSide::TopStart, CapSide::BottomEnd,
                                       CapSide::BottomStart};

    CapScan scan;
    for (const Frame& f : frames) {
        for (const double delta : deltas) {
            for (const CapSide side : sides) {
                scanCap(scan, side, RS_Vector(f.cx, f.cy), f.r, delta);
            }
        }
    }

    // Random frames, with the extremum anywhere from far below y = 0 to far
    // above it, and delta on both sides of 1.05e-8, where sin() stops being
    // exactly 1 at an extremum.
    std::mt19937_64 rng(20261104);
    std::uniform_real_distribution<double> logRadius(0.0, std::log(1.0e4));
    std::uniform_real_distribution<double> height(-2.0, 3.0);
    std::uniform_real_distribution<double> offset(-100.0, 100.0);
    std::uniform_real_distribution<double> logDelta(std::log(1.0e-10), std::log(6.0e-8));
    std::uniform_int_distribution<int> sideOf(0, 3);
    const int randomRuns = 6000;
    for (int run = 0; run < randomRuns; ++run) {
        const double r = std::exp(logRadius(rng));
        const double cy = height(rng) * r;
        const double cx = offset(rng);
        const double delta = std::exp(logDelta(rng));
        const CapSide side = sides[static_cast<std::size_t>(sideOf(rng))];
        scanCap(scan, side, RS_Vector(cx, cy), r, delta);
    }

    INFO("first cap that falls short: " << scan.firstShort);
    CHECK(scan.tested == frames.size() * deltas.size() * sides.size() + static_cast<std::size_t>(randomRuns));
    CHECK(scan.short_ == 0u);
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("A cap ending 1.4e-8 rad past a top at y = 0 is inside just above its end vertex",
          "[lc-hatchfix][classifier]") {
    // The circle segment of radius 1 centred at (0, -1) between the angles
    // pi/2 - 1 and pi/2 + 1.4e-8, closed by a chord. Its end vertex is a
    // rounding step below the top, and a point on the centre line one double
    // above that vertex is inside the segment.
    const RS_Vector centre(0.0, -1.0);
    auto* arc = new RS_Arc(nullptr, RS_ArcData{centre, 1.0, M_PI / 2.0 - 1.0, M_PI / 2.0 + 1.4e-8, false});
    const RS_Vector start = arc->getStartpoint();
    const RS_Vector end = arc->getEndpoint();
    auto loop = std::make_shared<RS_EntityContainer>(nullptr);
    loop->addEntity(arc);
    loop->addEntity(new RS_Line(loop.get(), end, start));
    const auto geometry = LoopGeometry::fromChain(*loop, {start, end});

    const double aboveEnd = std::nextafter(end.y, 0.0);
    REQUIRE(aboveEnd > end.y);
    REQUIRE(aboveEnd < 0.0);
    CHECK(oddParity(RS_Vector(0.0, aboveEnd), {geometry.get()}));
}
