/*******************************************************************************
 *
 This file is part of the LibreCAD project, a 2D CAD program

 Copyright (C) 2026 LibreCAD.org

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 ******************************************************************************/

// D0 gates for spline offsetting: conservative jet enclosures and the
// regularity classification the D1 offset guard is built on.

#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <functional>
#include <vector>

#include "lc_curvejet.h"
#include "lc_splinepoints.h"
#include "rs_spline.h"

namespace {
RS_Spline makeSpline(const size_t degree, const std::vector<RS_Vector>& controls,
                     const std::vector<double>& knots, std::vector<double> weights = {}) {
    RS_SplineData data(static_cast<int>(degree), false);
    data.controlPoints = controls;
    data.knotslist = knots;
    data.weights = weights.empty() ? std::vector<double>(controls.size(), 1.0) : std::move(weights);
    return RS_Spline(nullptr, data);
}

LC_SplinePoints fromControlPoints(const std::vector<RS_Vector>& controls) {
    LC_SplinePointsData data(false, false);
    data.useControlPoints = true;
    data.controlPoints = controls;
    return LC_SplinePoints(nullptr, data);
}

// The unit quarter circle, counter-clockwise: signed curvature exactly +1, so
// the left normal points at the centre and d = 1 is the singular distance.
RS_Spline quarterCircle() {
    return makeSpline(2, {{1.0, 0.0}, {1.0, 1.0}, {0.0, 1.0}}, {0, 0, 0, 1, 1, 1},
                      {1.0, std::sqrt(0.5), 1.0});
}

using BoundFn = std::function<bool(double, double, LC_CurveJetBounds&)>;
using EvalFn = std::function<bool(double, LC_CurveEvaluationSide, LC_CurveJet&)>;

/** Every sampled jet on [a, b] lies inside the box's bounds. At the ends the
 *  limit from inside the box is taken, since derivatives may jump there. */
void checkEncloses(const BoundFn& bound, const EvalFn& eval, const double a, const double b) {
    LC_CurveJetBounds bounds;
    REQUIRE(bound(a, b, bounds));
    for (int i = 0; i <= 64; ++i) {
        const double t = (i == 64) ? b : a + (b - a) * i / 64.0;
        const LC_CurveEvaluationSide side = (i == 0)    ? LC_CurveEvaluationSide::Right
                                            : (i == 64) ? LC_CurveEvaluationSide::Left
                                                        : LC_CurveEvaluationSide::Interior;
        LC_CurveJet jet;
        REQUIRE(eval(t, side, jet));
        CHECK(bounds.x.contains(jet.point.x));
        CHECK(bounds.y.contains(jet.point.y));
        CHECK(bounds.dx.contains(jet.first.x));
        CHECK(bounds.dy.contains(jet.first.y));
        CHECK(bounds.ddx.contains(jet.second.x));
        CHECK(bounds.ddy.contains(jet.second.y));
        // the products, to the rounding of evaluating them from the jet
        const double speed2 = jet.first.x * jet.first.x + jet.first.y * jet.first.y;
        const double cross = jet.first.x * jet.second.y - jet.first.y * jet.second.x;
        const double rounding = 1e-12 * (speed2 + std::abs(jet.first.x * jet.second.y) + std::abs(jet.first.y * jet.second.x));
        CHECK(bounds.speedSquared().lo() <= speed2 + rounding);
        CHECK(bounds.speedSquared().hi() >= speed2 - rounding);
        CHECK(bounds.cross().lo() <= cross + rounding);
        CHECK(bounds.cross().hi() >= cross - rounding);
    }
}

enum class Regularity { Regular, NotProven };

/**
 * The D1 guard: [a, b] is regular for the offset at d when bisection reaches
 * boxes on which both the speed and 1 - d kappa provably exclude zero.
 */
Regularity classify(const BoundFn& bound, const double a, const double b, const double d,
                    const int depth = 0) {
    LC_CurveJetBounds bounds;
    if (!bound(a, b, bounds)) {
        return Regularity::NotProven;
    }
    if (bounds.speedSquared().isPositive() && bounds.offsetFactorNumerator(d).excludesZero()) {
        return Regularity::Regular;
    }
    if (depth >= 24) {
        return Regularity::NotProven;
    }
    const double mid = a + 0.5 * (b - a);
    return (classify(bound, a, mid, d, depth + 1) == Regularity::Regular &&
            classify(bound, mid, b, d, depth + 1) == Regularity::Regular)
               ? Regularity::Regular
               : Regularity::NotProven;
}
} // namespace

TEST_CASE("RS_Spline::tryBoundJet encloses the curve on the box", "[curve-offset][d0][bound]") {
    const RS_Spline bezier = makeSpline(3, {{0, 0}, {1, 3}, {2, -3}, {3, 0}}, {0, 0, 0, 0, 1, 1, 1, 1});
    const RS_Spline rational = makeSpline(3, {{0, 0}, {1, 2}, {3, 3}, {4, 1}, {6, 2}, {7, 0}},
                                          {0, 0, 0, 0, 0.3, 1.2, 2, 2, 2, 2}, {1.0, 0.7, 1.4, 1.0, 0.9, 1.0});
    for (const RS_Spline* spline : {&bezier, &rational}) {
        const BoundFn bound = [spline](double a, double b, LC_CurveJetBounds& out) {
            return spline->tryBoundJet(a, b, out, true);
        };
        const EvalFn eval = [spline](double t, LC_CurveEvaluationSide side, LC_CurveJet& out) {
            return spline->tryEvaluateJet(t, side, out);
        };
        const std::vector<double> breaks = spline->getBreakParameters();
        for (size_t i = 0; i + 1 < breaks.size(); ++i) {
            checkEncloses(bound, eval, breaks[i], breaks[i + 1]);
            const double third = (breaks[i + 1] - breaks[i]) / 3.0;
            checkEncloses(bound, eval, breaks[i] + third, breaks[i] + 2.0 * third);
        }
    }
    const RS_Spline arc = quarterCircle();
    checkEncloses([&arc](double a, double b, LC_CurveJetBounds& out) { return arc.tryBoundJet(a, b, out, true); },
                  [&arc](double t, LC_CurveEvaluationSide side, LC_CurveJet& out) {
                      return arc.tryEvaluateJet(t, side, out);
                  },
                  0.2, 0.7);
}

TEST_CASE("RS_Spline::tryBoundJet narrows as the box shrinks", "[curve-offset][d0][bound]") {
    const RS_Spline bezier = makeSpline(3, {{0, 0}, {1, 3}, {2, -3}, {3, 0}}, {0, 0, 0, 0, 1, 1, 1, 1});
    LC_CurveJetBounds wide;
    LC_CurveJetBounds narrow;
    REQUIRE(bezier.tryBoundJet(0.0, 1.0, wide));
    REQUIRE(bezier.tryBoundJet(0.5, 0.5 + 1e-6, narrow));
    CHECK(narrow.x.width() < 1e-5);
    CHECK(narrow.dy.width() < 1e-4);
    CHECK(narrow.ddy.width() < wide.ddy.width());
}

TEST_CASE("Derivative bounds stay tight on a tiny box far from the origin", "[curve-offset][d0][bound]") {
    // Differencing nearly equal Bezier points of a 1e-7 box divides their
    // rounding by 1e-7 for C' and by 1e-14 for C''; at coordinates near 1000
    // that swamped the second derivative. The derivatives' own control points
    // do not.
    const RS_Vector o{1234.5, -987.25};
    const RS_Spline cubic = makeSpline(3, {o, o + RS_Vector{1, 3}, o + RS_Vector{2, -3}, o + RS_Vector{3, 0}},
                                       {0, 0, 0, 0, 1, 1, 1, 1});
    const RS_Spline rational = makeSpline(2, {o + RS_Vector{1, 0}, o + RS_Vector{1, 1}, o + RS_Vector{0, 1}},
                                          {0, 0, 0, 1, 1, 1}, {1.0, std::sqrt(0.5), 1.0});
    for (const RS_Spline* s : {&cubic, &rational}) {
        LC_CurveJetBounds b;
        REQUIRE(s->tryBoundJet(0.3, 0.3 + 1e-7, b));
        CHECK(b.dx.width() < 1e-5);
        CHECK(b.dy.width() < 1e-5);
        CHECK(b.ddx.width() < 1e-4);
        CHECK(b.ddy.width() < 1e-4);
    }
    const LC_SplinePoints points = fromControlPoints({o, o + RS_Vector{5, 10}, o + RS_Vector{10, 0}});
    LC_CurveJetBounds b;
    REQUIRE(points.tryBoundJet(0.3, 0.3 + 1e-7, b));
    CHECK(b.dx.width() < 1e-5);
    CHECK(b.dy.width() < 1e-5);
}

TEST_CASE("Next to a vanishing tangent the products' own bounds keep the sign of the curvature",
          "[curve-offset][d0][bound]") {
    // A cubic whose first handle is doubled onto its start: C' = 0 there, and
    // just past it C' and C'' are all but parallel, C' x C'' ~ -18 t^2. The
    // products of their component intervals straddle zero on every box; the
    // Bezier coefficients of C' x C'' itself do not.
    const RS_Spline doubled = makeSpline(3, {{3, 1}, {3, 1}, {4, 1}, {5, 0}}, {0, 0, 0, 0, 1, 1, 1, 1});
    for (const double h : {1e-2, 1e-4, 1e-6}) {
        LC_CurveJetBounds b;
        REQUIRE(doubled.tryBoundJet(h, 2.0 * h, b, true));
        CHECK((b.dx * b.ddy - b.dy * b.ddx).containsZero());
        CHECK(b.crossProduct.isNegative());
        CHECK(b.cross().isNegative());
        CHECK(b.speedSquared().isPositive());
    }
    // unless asked for, and at a rational span, there are none: the components alone bound it
    LC_CurveJetBounds plain;
    REQUIRE(doubled.tryBoundJet(1e-2, 2e-2, plain));
    CHECK_FALSE(plain.crossProduct.isValid());
    const RS_Spline arc = quarterCircle();
    LC_CurveJetBounds b;
    REQUIRE(arc.tryBoundJet(0.2, 0.4, b, true));
    CHECK_FALSE(b.crossProduct.isValid());
    CHECK_FALSE(b.speedSquaredProduct.isValid());
    CHECK(b.cross().isValid());
}

TEST_CASE("RS_Spline::tryBoundJet refuses a box it cannot bound", "[curve-offset][d0][bound]") {
    LC_CurveJetBounds bounds;
    // across the repeated knot at t = 1, where the derivatives jump
    const RS_Spline kinked = makeSpline(3, {{0, 0}, {1, 1}, {2, 1}, {3, 0}, {4, 2}, {5, 2}, {6, 0}},
                                        {0, 0, 0, 0, 1, 1, 1, 2, 2, 2, 2});
    CHECK_FALSE(kinked.tryBoundJet(0.5, 1.5, bounds));
    CHECK(kinked.tryBoundJet(0.5, 1.0, bounds));
    CHECK(kinked.tryBoundJet(1.0, 1.5, bounds));
    // outside the domain, empty or reversed boxes
    CHECK_FALSE(kinked.tryBoundJet(-0.5, 0.5, bounds));
    CHECK_FALSE(kinked.tryBoundJet(1.5, 2.5, bounds));
    CHECK_FALSE(kinked.tryBoundJet(0.5, 0.5, bounds));
    CHECK_FALSE(kinked.tryBoundJet(0.6, 0.5, bounds));
    // a vanishing weight has no positive denominator
    const RS_Spline zeroWeight = makeSpline(2, {{1, 0}, {1, 1}, {0, 1}}, {0, 0, 0, 1, 1, 1}, {1.0, 0.0, 1.0});
    CHECK_FALSE(zeroWeight.tryBoundJet(0.2, 0.4, bounds));
}

TEST_CASE("LC_SplinePoints::tryBoundJet encloses its segments", "[curve-offset][d0][bound]") {
    const LC_SplinePoints spline = fromControlPoints({{0, 0}, {1, 2}, {3, 2}, {4, 3}, {6, 1}});
    const BoundFn bound = [&spline](double a, double b, LC_CurveJetBounds& out) {
        return spline.tryBoundJet(a, b, out, true);
    };
    const EvalFn eval = [&spline](double t, LC_CurveEvaluationSide side, LC_CurveJet& out) {
        return spline.tryEvaluateJet(t, side, out);
    };
    checkEncloses(bound, eval, 0.0, 1.0);
    checkEncloses(bound, eval, 1.25, 1.75);
    checkEncloses(bound, eval, 2.0, 3.0);
    LC_CurveJetBounds bounds;
    CHECK_FALSE(spline.tryBoundJet(0.5, 1.5, bounds)); // across a join

    const LC_SplinePoints line = fromControlPoints({{0, 0}, {4, 2}});
    checkEncloses([&line](double a, double b, LC_CurveJetBounds& out) { return line.tryBoundJet(a, b, out, true); },
                  [&line](double t, LC_CurveEvaluationSide side, LC_CurveJet& out) {
                      return line.tryEvaluateJet(t, side, out);
                  },
                  0.1, 0.9);
}

TEST_CASE("Offset regularity guard classifies the singular radius", "[curve-offset][d0][regularity]") {
    const RS_Spline arc = quarterCircle();
    const BoundFn bound = [&arc](double a, double b, LC_CurveJetBounds& out) {
        return arc.tryBoundJet(a, b, out);
    };
    // Outward (right side) is always regular; inward it is regular on either side
    // of the singular distance, reversed beyond it, and never at it: 1 - d kappa
    // vanishes identically there, so no subdivision can exclude zero.
    CHECK(classify(bound, 0.0, 1.0, -5.0) == Regularity::Regular);
    CHECK(classify(bound, 0.0, 1.0, 0.5) == Regularity::Regular);
    CHECK(classify(bound, 0.0, 1.0, 0.999) == Regularity::Regular);
    CHECK(classify(bound, 0.0, 1.0, 1.001) == Regularity::Regular);
    CHECK(classify(bound, 0.0, 1.0, 1.0) == Regularity::NotProven);
}

TEST_CASE("Offset regularity guard on a line and a zero tangent", "[curve-offset][d0][regularity]") {
    const LC_SplinePoints line = fromControlPoints({{0, 0}, {4, 2}});
    const BoundFn lineBound = [&line](double a, double b, LC_CurveJetBounds& out) {
        return line.tryBoundJet(a, b, out);
    };
    CHECK(classify(lineBound, 0.0, 1.0, 1e6) == Regularity::Regular);

    // A cusp of the source: control points folding back give C'(t) = 0 at t = 1/2.
    const RS_Spline cusp = makeSpline(3, {{0, 0}, {2, 1}, {0, 1}, {2, 0}}, {0, 0, 0, 0, 1, 1, 1, 1});
    const BoundFn cuspBound = [&cusp](double a, double b, LC_CurveJetBounds& out) {
        return cusp.tryBoundJet(a, b, out);
    };
    CHECK(classify(cuspBound, 0.0, 1.0, 0.1) == Regularity::NotProven);
    CHECK(classify(cuspBound, 0.0, 0.4, 0.1) == Regularity::Regular);
}
