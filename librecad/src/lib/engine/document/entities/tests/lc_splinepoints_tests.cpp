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

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <vector>

#include "lc_splinepoints.h"
#include "rs_vector.h"

using Catch::Approx;

namespace {
bool near(const RS_Vector& a, const RS_Vector& b, const double tol = 1e-12) {
    return a.valid && b.valid && a.distanceTo(b) <= tol;
}

/** A spline built from these control points, as they are. */
LC_SplinePoints fromControlPoints(const std::vector<RS_Vector>& controls, const bool closed = false) {
    LC_SplinePointsData data(closed, false);
    data.useControlPoints = true;
    data.controlPoints = controls;
    return LC_SplinePoints(nullptr, data);
}

LC_CurveJet jetAt(const LC_SplinePoints& spline, const double t,
                  const LC_CurveEvaluationSide side = LC_CurveEvaluationSide::Interior) {
    LC_CurveJet jet;
    REQUIRE(spline.tryEvaluateJet(t, side, jet));
    return jet;
}
} // namespace

TEST_CASE("LC_SplinePoints segments are tagged by kind", "[spline][jet][LC_SplinePoints]") {
    LC_SplinePointsSegment segment;

    SECTION("no control points") {
        const LC_SplinePoints empty = fromControlPoints({});
        CHECK(empty.getSegmentCount() == 0);
        CHECK_FALSE(empty.tryGetSegment(0, segment));
    }
    SECTION("one control point is a point") {
        const LC_SplinePoints point = fromControlPoints({{2.0, 3.0}});
        REQUIRE(point.getSegmentCount() == 1);
        REQUIRE(point.tryGetSegment(0, segment));
        CHECK(segment.kind == LC_SplinePointsSegment::Kind::Point);
        CHECK(near(segment.start, RS_Vector{2.0, 3.0}));
    }
    SECTION("two control points are a line, never a quadratic") {
        // getQuadPoints() returns 2 here; callers treating it as a boolean then
        // evaluated a control point it never set.
        const LC_SplinePoints line = fromControlPoints({{0.0, 0.0}, {4.0, 2.0}});
        REQUIRE(line.getSegmentCount() == 1);
        REQUIRE(line.tryGetSegment(0, segment));
        CHECK(segment.kind == LC_SplinePointsSegment::Kind::Line);
        CHECK(near(segment.start, RS_Vector{0.0, 0.0}));
        CHECK(near(segment.end, RS_Vector{4.0, 2.0}));
        CHECK_FALSE(segment.control.valid);
    }
    SECTION("three control points are one quadratic") {
        const LC_SplinePoints quad = fromControlPoints({{0, 0}, {1, 2}, {2, 0}});
        REQUIRE(quad.getSegmentCount() == 1);
        REQUIRE(quad.tryGetSegment(0, segment));
        CHECK(segment.kind == LC_SplinePointsSegment::Kind::Quadratic);
        CHECK(near(segment.control, RS_Vector{1, 2}));
    }
    SECTION("an open spline has one quadratic per interior control point") {
        const LC_SplinePoints open = fromControlPoints({{0, 0}, {1, 2}, {3, 2}, {4, 0}, {6, 1}});
        REQUIRE(open.getSegmentCount() == 3);
        REQUIRE(open.tryGetSegment(0, segment));
        CHECK(near(segment.start, RS_Vector{0, 0}));      // first control point
        CHECK(near(segment.end, RS_Vector{2, 2}));        // halfway to the next
        REQUIRE(open.tryGetSegment(2, segment));
        CHECK(near(segment.start, RS_Vector{3.5, 1}));
        CHECK(near(segment.end, RS_Vector{6, 1}));        // last control point
        CHECK_FALSE(open.tryGetSegment(3, segment));
    }
    SECTION("a closed spline has one quadratic per control point") {
        const LC_SplinePoints closed = fromControlPoints({{0, 0}, {2, 0}, {2, 2}, {0, 2}}, true);
        REQUIRE(closed.getSegmentCount() == 4);
        REQUIRE(closed.tryGetSegment(0, segment));
        CHECK(near(segment.start, RS_Vector{0, 1}));      // wraps to the last control point
        CHECK(near(segment.control, RS_Vector{0, 0}));
        CHECK(near(segment.end, RS_Vector{1, 0}));
    }
}

TEST_CASE("LC_SplinePoints::tryEvaluateJet on a line is affine", "[spline][jet][LC_SplinePoints]") {
    const LC_SplinePoints line = fromControlPoints({{1.0, 1.0}, {5.0, 3.0}});
    for (const double t : {0.0, 0.25, 0.5, 1.0}) {
        const LC_CurveJet jet = jetAt(line, t);
        CHECK(near(jet.point, RS_Vector{1.0, 1.0} * (1.0 - t) + RS_Vector{5.0, 3.0} * t));
        CHECK(near(jet.first, RS_Vector{4.0, 2.0}));
        CHECK(near(jet.second, RS_Vector{0.0, 0.0}));
    }
}

TEST_CASE("LC_SplinePoints::tryEvaluateJet on quadratic segments", "[spline][jet][LC_SplinePoints]") {
    const std::vector<RS_Vector> controls{{0, 0}, {1, 2}, {3, 2}, {4, 3}};
    const LC_SplinePoints spline = fromControlPoints(controls);
    REQUIRE(spline.getSegmentCount() == 2);

    // segment 0: (0,0) - (1,2) - (2,2)
    const LC_CurveJet mid = jetAt(spline, 0.5);
    CHECK(near(mid.point, RS_Vector{1.0, 1.5}));
    CHECK(near(mid.first, RS_Vector{2.0, 2.0}));
    CHECK(near(mid.second, RS_Vector{0.0, -4.0}));

    // The join at t = 1 is C1: both limits agree on the point and first derivative,
    // while the second derivative jumps.
    const LC_CurveJet left = jetAt(spline, 1.0, LC_CurveEvaluationSide::Left);
    const LC_CurveJet right = jetAt(spline, 1.0, LC_CurveEvaluationSide::Right);
    CHECK(near(left.point, RS_Vector{2.0, 2.0}));
    CHECK(near(right.point, RS_Vector{2.0, 2.0}));
    CHECK(near(left.first, right.first));
    CHECK(near(left.first, controls[2] - controls[1]));
    CHECK(near(left.second, RS_Vector{0.0, -4.0}));
    CHECK(near(right.second, RS_Vector{0.0, 2.0}));

    // Central differences inside a segment.
    const double h = 1e-6;
    const double t = 1.3;
    const LC_CurveJet at = jetAt(spline, t);
    CHECK(near(at.first, (jetAt(spline, t + h).point - jetAt(spline, t - h).point) / (2.0 * h), 1e-6));
}

TEST_CASE("LC_SplinePoints::tryEvaluateJet on a single point and at the domain ends",
          "[spline][jet][LC_SplinePoints]") {
    const LC_SplinePoints point = fromControlPoints({{2.0, 3.0}});
    const LC_CurveJet jet = jetAt(point, 0.5);
    CHECK(near(jet.point, RS_Vector{2.0, 3.0}));
    CHECK(near(jet.first, RS_Vector{0.0, 0.0}));

    const LC_SplinePoints quad = fromControlPoints({{0, 0}, {1, 2}, {2, 0}});
    LC_CurveJet out;
    CHECK_FALSE(quad.tryEvaluateJet(-0.5, LC_CurveEvaluationSide::Interior, out));
    CHECK_FALSE(out.point.valid);
    CHECK_FALSE(quad.tryEvaluateJet(1.5, LC_CurveEvaluationSide::Interior, out));
    CHECK_FALSE(quad.tryEvaluateJet(0.0, LC_CurveEvaluationSide::Left, out));
    CHECK_FALSE(quad.tryEvaluateJet(1.0, LC_CurveEvaluationSide::Right, out));
    CHECK(quad.tryEvaluateJet(1.0, LC_CurveEvaluationSide::Interior, out));
    CHECK(near(out.point, RS_Vector{2, 0}));
    CHECK_FALSE(fromControlPoints({}).tryEvaluateJet(0.0, LC_CurveEvaluationSide::Interior, out));
    CHECK_FALSE(fromControlPoints({{0, 0}, {NAN, 1}, {2, 0}})
                    .tryEvaluateJet(0.5, LC_CurveEvaluationSide::Interior, out));
}

TEST_CASE("LC_SplinePoints jets follow the fit points they were built from",
          "[spline][jet][LC_SplinePoints]") {
    // Three fit points: one quadratic through all three.
    LC_SplinePointsData data(false, false);
    data.splinePoints = {{0, 0}, {2, 2}, {4, 0}};
    const LC_SplinePoints fit(nullptr, data);
    REQUIRE(fit.getSegmentCount() == 1);
    CHECK(near(jetAt(fit, 0.0).point, RS_Vector{0, 0}));
    CHECK(near(jetAt(fit, 1.0).point, RS_Vector{4, 0}));
    bool passesMiddle = false;
    for (int i = 0; i <= 1000 && !passesMiddle; ++i) {
        passesMiddle = jetAt(fit, i / 1000.0).point.distanceTo(RS_Vector{2, 2}) < 1e-2;
    }
    CHECK(passesMiddle);
}
