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

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <functional>
#include <iomanip>
#include <limits>
#include <vector>

#include "lc_curveoffset.h"
#include "lc_splinepoints.h"
#include "rs_line.h"
#include "rs_spline.h"

using Catch::Approx;

namespace {

RS_Spline makeSpline(const size_t degree, const std::vector<RS_Vector>& controls,
                     const std::vector<double>& knots, std::vector<double> weights = {}) {
    RS_SplineData data(static_cast<int>(degree), false);
    data.controlPoints = controls;
    data.knotslist = knots;
    data.weights = weights.empty() ? std::vector<double>(controls.size(), 1.0) : std::move(weights);
    return RS_Spline(nullptr, data);
}

LC_SplinePoints fromControlPoints(const std::vector<RS_Vector>& controls, const bool closed = false) {
    LC_SplinePointsData data(closed, false);
    data.useControlPoints = true;
    data.controlPoints = controls;
    return LC_SplinePoints(nullptr, data);
}

RS_Spline quarterCircle(const RS_Vector& center = RS_Vector{0.0, 0.0}, const double r = 1.0) {
    return makeSpline(2, {center + RS_Vector{r, 0.0}, center + RS_Vector{r, r}, center + RS_Vector{0.0, r}},
                      {0, 0, 0, 1, 1, 1}, {1.0, std::sqrt(0.5), 1.0});
}

// A cubic with an inflection.
RS_Spline sCurve(const RS_Vector& shift = RS_Vector{0.0, 0.0}) {
    return makeSpline(3, {shift + RS_Vector{0, 0}, shift + RS_Vector{4, 6}, shift + RS_Vector{8, -6},
                          shift + RS_Vector{12, 0}},
                      {0, 0, 0, 0, 1, 1, 1, 1});
}

LC_CurveOffsetGeometryResult offsetToSide(const RS_Entity& source, const LC_CurveOffsetSide side,
                                          const double distance) {
    const LC_CurveOffsetOptions options = LC_CurveOffset::makeDirectOptions(source, distance);
    return LC_CurveOffset::buildDirectBranches(source, LC_CurveOffset::makeSideRequest(side, distance), options,
                                               LC_CurveOffset::makeDirectSourceBudget());
}

LC_CurveOffsetGeometryResult offsetTowards(const RS_Entity& source, const RS_Vector& point, const double distance) {
    const LC_CurveOffsetOptions options = LC_CurveOffset::makeDirectOptions(source, distance);
    return LC_CurveOffset::buildDirectBranches(source, LC_CurveOffset::makeDirectionRequest(point, distance),
                                               options, LC_CurveOffset::makeDirectSourceBudget());
}

using EvalFn = std::function<bool(double, LC_CurveEvaluationSide, LC_CurveJet&)>;

EvalFn evaluator(const RS_Spline& s) {
    return [&s](double t, LC_CurveEvaluationSide side, LC_CurveJet& jet) { return s.tryEvaluateJet(t, side, jet); };
}

EvalFn evaluator(const LC_SplinePoints& s) {
    return [&s](double t, LC_CurveEvaluationSide side, LC_CurveJet& jet) { return s.tryEvaluateJet(t, side, jet); };
}

RS_Vector bezierAt(const std::array<RS_Vector, 4>& b, const double s) {
    const double r = 1.0 - s;
    return b[0] * (r * r * r) + b[1] * (3.0 * r * r * s) + b[2] * (3.0 * r * s * s) + b[3] * (s * s * s);
}

/** The exact offset C + d N at t. */
RS_Vector exactOffset(const EvalFn& eval, const double t, const LC_CurveEvaluationSide side, const double d) {
    LC_CurveJet c;
    REQUIRE(eval(t, side, c));
    return c.point + RS_Vector{-c.first.y, c.first.x} / c.first.magnitude() * d;
}

/**
 * The largest distance between the pieces and the exact offset C + d N at the
 * same source parameter, at samples the fitter never used; also checks the
 * pieces are contiguous and follow the source parameter, and that each branch
 * starts where the previous one ends (at a cusp). A stall may leave a gap in
 * the parameter, over which the exact offset moves no more than @p stall.
 */
double maxDeviation(const LC_CurveOffsetGeometryResult& result, const EvalFn& eval, const double stall = 0.0) {
    REQUIRE(result.status == LC_CurveOffsetStatus::Ok);
    REQUIRE_FALSE(result.branches.empty());
    double worst = 0.0;
    double previousT1 = result.branches.front().cubicPieces.front().provenance.sourceT0;
    const LC_OffsetCubicPiece* previous = nullptr;
    for (const LC_OffsetBranch& branch : result.branches) {
    REQUIRE_FALSE(branch.cubicPieces.empty());
    for (size_t i = 0; i < branch.cubicPieces.size(); ++i) {
        const LC_OffsetCubicPiece& piece = branch.cubicPieces[i];
        const auto& p = piece.provenance;
        CHECK(p.sourceT0 < p.sourceT1);
        if (p.sourceT0 != previousT1) {
            CHECK(p.sourceT0 > previousT1);
            CHECK(exactOffset(eval, previousT1, LC_CurveEvaluationSide::Interior, p.signedDistance)
                      .distanceTo(exactOffset(eval, p.sourceT0, LC_CurveEvaluationSide::Interior,
                                              p.signedDistance)) <= stall);
        }
        previousT1 = p.sourceT1;
        CHECK(p.signedDistance == result.signedDistance);
        if (previous != nullptr) {
            CHECK(piece.bezier[0] == previous->bezier[3]);
        }
        previous = &piece;
        for (int k = 0; k <= 37; ++k) {
            const double s = k / 37.0;
            const double t = p.sourceT0 + s * (p.sourceT1 - p.sourceT0);
            LC_CurveJet c;
            const auto side = (k == 0) ? LC_CurveEvaluationSide::Right
                              : (k == 37) ? LC_CurveEvaluationSide::Left
                                          : LC_CurveEvaluationSide::Interior;
            REQUIRE(eval(t, side, c));
            const RS_Vector n = RS_Vector{-c.first.y, c.first.x} / c.first.magnitude();
            const RS_Vector exact = c.point + n * p.signedDistance;
            worst = std::max(worst, bezierAt(piece.bezier, s).distanceTo(exact));
        }
    }
    }
    return worst;
}

} // namespace

TEST_CASE("Default and partial requests are rejected before any evaluation",
          "[curve-offset][direct][validation]") {
    const RS_Spline source = sCurve();
    const LC_CurveOffsetOptions good = LC_CurveOffset::makeDirectOptions(source, 1.0);
    const LC_CurveOffsetRequest request = LC_CurveOffset::makeSideRequest(LC_CurveOffsetSide::Left, 1.0);
    const LC_OffsetSourceBudget budget = LC_CurveOffset::makeDirectSourceBudget();
    REQUIRE(LC_CurveOffset::buildDirectBranches(source, request, good, budget).status == LC_CurveOffsetStatus::Ok);

    // every default is a sentinel that fails, never a silent zero tolerance or open cap
    CHECK(LC_CurveOffset::buildDirectBranches(source, request, LC_CurveOffsetOptions{}, budget).status ==
          LC_CurveOffsetStatus::InvalidRequest);
    CHECK(LC_CurveOffset::buildDirectBranches(source, request, good, LC_OffsetSourceBudget{}).status ==
          LC_CurveOffsetStatus::InvalidRequest);
    CHECK(LC_CurveOffset::buildDirectBranches(source, LC_CurveOffsetRequest{}, good, budget).status ==
          LC_CurveOffsetStatus::InvalidDistance);

    LC_CurveOffsetOptions partial = good;
    partial.tolerance.fit = std::nan("");
    CHECK(LC_CurveOffset::buildDirectBranches(source, request, partial, budget).status ==
          LC_CurveOffsetStatus::InvalidRequest);
    partial = good;
    partial.maxSamples = 0;
    CHECK(LC_CurveOffset::buildDirectBranches(source, request, partial, budget).status ==
          LC_CurveOffsetStatus::InvalidRequest);
    partial = good;
    partial.mode = LC_CurveOffsetMode::RegionBoundary; // an open curve encloses no region
    CHECK(LC_CurveOffset::buildDirectBranches(source, request, partial, budget).status ==
          LC_CurveOffsetStatus::InvalidSource);

    for (const double bad : {0.0, -1.0, std::nan(""), std::numeric_limits<double>::infinity()}) {
        CHECK(LC_CurveOffset::buildDirectBranches(source, LC_CurveOffset::makeSideRequest(LC_CurveOffsetSide::Left, bad),
                                                  good, budget)
                  .status == LC_CurveOffsetStatus::InvalidDistance);
    }
    // a direction request needs a direction point
    CHECK(LC_CurveOffset::buildDirectBranches(source, LC_CurveOffset::makeDirectionRequest(RS_Vector{false}, 1.0),
                                              good, budget)
              .status == LC_CurveOffsetStatus::InvalidRequest);
    // the factory refuses what has no finite scale
    CHECK(std::isnan(LC_CurveOffset::makeDirectOptions(source, 0.0).tolerance.fit));
    CHECK(std::isnan(LC_CurveOffset::makeDirectOptions(source, std::nan("")).tolerance.fit));

    // factory output is deterministic
    const LC_CurveOffsetOptions again = LC_CurveOffset::makeDirectOptions(source, 1.0);
    CHECK(again.tolerance.fit == good.tolerance.fit);
    CHECK(again.tolerance.requestedGeometry == good.tolerance.requestedGeometry);
    const LC_CurveOffsetTolerances& t = good.tolerance;
    CHECK(t.evaluation + t.fit + t.nodeMerge == Approx(t.requestedGeometry));
}

TEST_CASE("A straight source offsets exactly to either side", "[curve-offset][direct]") {
    const LC_SplinePoints line = fromControlPoints({{0.0, 0.0}, {10.0, 0.0}});
    const RS_Spline linear = makeSpline(1, {{0.0, 0.0}, {10.0, 0.0}}, {0, 0, 1, 1});
    for (const RS_Entity* source : std::vector<const RS_Entity*>{&line, &linear}) {
        const LC_CurveOffsetGeometryResult left = offsetTowards(*source, RS_Vector{5.0, 7.0}, 2.0);
        REQUIRE(left.status == LC_CurveOffsetStatus::Ok);
        REQUIRE(left.branches.size() == 1);
        const LC_OffsetBranch& branch = left.branches.front();
        CHECK(branch.straight);
        REQUIRE(branch.cubicPieces.size() == 1);
        CHECK(branch.cubicPieces[0].bezier[0] == RS_Vector(0.0, 2.0));
        CHECK(branch.cubicPieces[0].bezier[3] == RS_Vector(10.0, 2.0));
        CHECK(left.signedDistance == 2.0);

        const LC_CurveOffsetGeometryResult right = offsetTowards(*source, RS_Vector{5.0, -0.1}, 2.0);
        REQUIRE(right.status == LC_CurveOffsetStatus::Ok);
        CHECK(right.signedDistance == -2.0);
        CHECK(right.branches.front().cubicPieces[0].bezier[3] == RS_Vector(10.0, -2.0));
    }
}

TEST_CASE("A parabola's offset follows its exact normals", "[curve-offset][direct]") {
    // B(u) = (10u, 20u(1-u)) turns clockwise, tightest at its apex: radius 2.5.
    const LC_SplinePoints parabola = fromControlPoints({{0.0, 0.0}, {5.0, 10.0}, {10.0, 0.0}});
    for (const double distance : {0.5, 2.0}) {
        for (const LC_CurveOffsetSide side : {LC_CurveOffsetSide::Left, LC_CurveOffsetSide::Right}) {
            const LC_CurveOffsetGeometryResult result = offsetToSide(parabola, side, distance);
            const LC_CurveOffsetOptions options = LC_CurveOffset::makeDirectOptions(parabola, distance);
            INFO("distance " << distance << " side " << static_cast<int>(side));
            REQUIRE(result.status == LC_CurveOffsetStatus::Ok);
            CHECK(maxDeviation(result, evaluator(parabola)) <= options.tolerance.requestedGeometry);
            CHECK(result.maxObservedError <= options.tolerance.fit);
        }
    }
    // inside its apex, at the radius of curvature, the offset stops dead at the
    // centre of curvature (5, 2.5) and runs on
    const LC_CurveOffsetGeometryResult atRadius = offsetToSide(parabola, LC_CurveOffsetSide::Right, 2.5);
    REQUIRE(atRadius.status == LC_CurveOffsetStatus::Ok);
    CHECK(atRadius.branches.size() == 1);
    const LC_CurveOffsetOptions options = LC_CurveOffset::makeDirectOptions(parabola, 2.5);
    CHECK(maxDeviation(atRadius, evaluator(parabola), options.tolerance.nodeMerge) <=
          options.tolerance.requestedGeometry);
    CHECK(offsetToSide(parabola, LC_CurveOffsetSide::Left, 3.0).status == LC_CurveOffsetStatus::Ok);
}

TEST_CASE("A circular arc offsets to a concentric arc", "[curve-offset][direct]") {
    const RS_Vector center{3.0, -2.0};
    const RS_Spline arc = quarterCircle(center, 10.0);
    struct Case {
        RS_Vector point; // the side to offset towards
        double radius;
    };
    // outside the counter-clockwise arc is its right side, inside its left
    for (const Case& c : {Case{center + RS_Vector{20.0, 20.0}, 13.0}, Case{center + RS_Vector{1.0, 1.0}, 7.0}}) {
        const LC_CurveOffsetGeometryResult result = offsetTowards(arc, c.point, 3.0);
        REQUIRE(result.status == LC_CurveOffsetStatus::Ok);
        const LC_CurveOffsetOptions options = LC_CurveOffset::makeDirectOptions(arc, 3.0);
        for (const LC_OffsetCubicPiece& piece : result.branches.front().cubicPieces) {
            for (int k = 0; k <= 20; ++k) {
                CHECK(bezierAt(piece.bezier, k / 20.0).distanceTo(center) ==
                      Approx(c.radius).margin(options.tolerance.requestedGeometry));
            }
        }
        CHECK(maxDeviation(result, evaluator(arc)) <= options.tolerance.requestedGeometry);
    }
}

TEST_CASE("An S-curve offsets on both sides through its inflection", "[curve-offset][direct]") {
    const RS_Spline s = sCurve();
    for (const LC_CurveOffsetSide side : {LC_CurveOffsetSide::Left, LC_CurveOffsetSide::Right}) {
        const LC_CurveOffsetGeometryResult result = offsetToSide(s, side, 0.75);
        REQUIRE(result.status == LC_CurveOffsetStatus::Ok);
        CHECK(maxDeviation(result, evaluator(s)) <=
              LC_CurveOffset::makeDirectOptions(s, 0.75).tolerance.requestedGeometry);
    }
}

TEST_CASE("The offset is regular or it is refused", "[curve-offset][direct][regularity]") {
    // Inside a unit arc the offset shrinks to a point at d = 1: a cusp for any
    // open offset curve. Either side of it the offset is regular.
    const RS_Spline arc = quarterCircle();
    const LC_CurveOffsetGeometryResult singular = offsetToSide(arc, LC_CurveOffsetSide::Left, 1.0);
    CHECK((singular.status == LC_CurveOffsetStatus::SingularOffset ||
           singular.status == LC_CurveOffsetStatus::AmbiguousRegularity));
    CHECK(singular.branches.empty());
    CHECK(offsetToSide(arc, LC_CurveOffsetSide::Left, 0.99).status == LC_CurveOffsetStatus::Ok);
    CHECK(offsetToSide(arc, LC_CurveOffsetSide::Left, 1.01).status == LC_CurveOffsetStatus::Ok);

    // A source whose tangent vanishes has no normal there.
    const RS_Spline cusp = makeSpline(3, {{0, 0}, {2, 1}, {0, 1}, {2, 0}}, {0, 0, 0, 0, 1, 1, 1, 1});
    const LC_CurveOffsetGeometryResult noTangent = offsetToSide(cusp, LC_CurveOffsetSide::Left, 0.1);
    CHECK((noTangent.status == LC_CurveOffsetStatus::UndefinedTangent ||
           noTangent.status == LC_CurveOffsetStatus::AmbiguousRegularity));
    CHECK(noTangent.branches.empty());
}

TEST_CASE("A kink is rounded where it turns away from the offset, its sides overlap where it turns towards it",
          "[curve-offset][direct][kink]") {
    // At (3, 0) the cubic turns left, from (1, -1) to (1, 2): the right side
    // leaves a gap, rounded by an arc of radius 0.2 about the kink; on the left
    // the two sides' offsets overlap, and each branch ends there.
    const RS_Spline kinked = makeSpline(3, {{0, 0}, {1, 1}, {2, 1}, {3, 0}, {4, 2}, {5, 2}, {6, 0}},
                                        {0, 0, 0, 0, 1, 1, 1, 2, 2, 2, 2});
    const LC_CurveOffsetGeometryResult right = offsetToSide(kinked, LC_CurveOffsetSide::Right, 0.2);
    REQUIRE(right.status == LC_CurveOffsetStatus::Ok);
    REQUIRE(right.branches.size() == 1);
    int arcPieces = 0;
    for (const LC_OffsetCubicPiece& piece : right.branches.front().cubicPieces) {
        if (piece.provenance.arcCentre.valid) {
            ++arcPieces;
            CHECK(piece.provenance.arcCentre.distanceTo(RS_Vector{3.0, 0.0}) < 1e-12);
            CHECK(piece.bezier[0].distanceTo(RS_Vector{3.0, 0.0}) == Approx(0.2).epsilon(1e-12));
            CHECK(piece.bezier[3].distanceTo(RS_Vector{3.0, 0.0}) == Approx(0.2).epsilon(1e-12));
        }
    }
    CHECK(arcPieces > 0);

    const LC_CurveOffsetGeometryResult left = offsetToSide(kinked, LC_CurveOffsetSide::Left, 0.2);
    REQUIRE(left.status == LC_CurveOffsetStatus::Ok);
    REQUIRE(left.branches.size() == 2);
    CHECK(left.branches[0].endEnd == LC_OffsetBranchEnd::Kink);
    CHECK(left.branches[1].startEnd == LC_OffsetBranchEnd::Kink);
    CHECK(left.branches[0].cubicPieces.back().bezier[3].distanceTo(left.branches[1].cubicPieces.front().bezier[0]) >
          0.1);

    // the same knot multiplicity with collinear handles is G1
    const RS_Spline smooth = makeSpline(3, {{0, 0}, {1, 1}, {2, 1}, {3, 1}, {4, 1}, {5, 2}, {6, 0}},
                                        {0, 0, 0, 0, 1, 1, 1, 2, 2, 2, 2});
    const LC_CurveOffsetGeometryResult result = offsetToSide(smooth, LC_CurveOffsetSide::Left, 0.2);
    REQUIRE(result.status == LC_CurveOffsetStatus::Ok);
    CHECK(maxDeviation(result, evaluator(smooth)) <=
          LC_CurveOffset::makeDirectOptions(smooth, 0.2).tolerance.requestedGeometry);
}

TEST_CASE("A cusp hidden at a join where the curvature jumps splits the offset there",
          "[curve-offset][direct][cusp]") {
    // Two quadratic segments meet with a common tangent, turning right. The
    // first bends with radii between 1.4 and 11.3; the second, past the join,
    // with radii of 28 and more. At a distance between those, 1 - d kappa is
    // negative throughout the first segment and positive throughout the second:
    // no box inside either span contains a zero, yet the offset turns back at
    // the join.
    const LC_SplinePoints joined = fromControlPoints({{0, 0}, {2, 2}, {4, 0}, {24.9, -21.1}});
    auto radiusAt = [&joined](const double t, const LC_CurveEvaluationSide side) {
        LC_CurveJet j;
        REQUIRE(joined.tryEvaluateJet(t, side, j));
        const double s = j.first.magnitude();
        return s * s * s / std::abs(j.first.x * j.second.y - j.first.y * j.second.x);
    };
    const double firstMax = radiusAt(0.0, LC_CurveEvaluationSide::Right);  // largest on the first segment
    const double secondMin = radiusAt(1.0, LC_CurveEvaluationSide::Right); // smallest on the second
    const double between = std::sqrt(firstMax * secondMin);
    REQUIRE(firstMax < between);
    REQUIRE(between < secondMin);
    // the inside of the turn is the right side: one branch per segment, meeting at the join
    const LC_CurveOffsetGeometryResult split = offsetToSide(joined, LC_CurveOffsetSide::Right, between);
    REQUIRE(split.status == LC_CurveOffsetStatus::Ok);
    REQUIRE(split.branches.size() == 2);
    CHECK(split.branches.front().cubicPieces.back().provenance.sourceT1 == 1.0);
    CHECK(maxDeviation(split, evaluator(joined)) <=
          LC_CurveOffset::makeDirectOptions(joined, between).tolerance.requestedGeometry);
    CHECK(offsetToSide(joined, LC_CurveOffsetSide::Right, 0.5).branches.size() == 1);
    CHECK(offsetToSide(joined, LC_CurveOffsetSide::Left, between).branches.size() == 1);
}

TEST_CASE("The nearest point just before a closed curve's seam decides the side", "[curve-offset][direct][side]") {
    // A thin closed strip, counter-clockwise, its seam on the top edge at
    // (10, 0.05). The point is 0.01 above the top edge, just before the seam,
    // and 0.06 above the bottom edge: the top edge, seen from outside, is nearer.
    const LC_SplinePoints strip = fromControlPoints({{0, 0.05}, {0, 0}, {20, 0}, {20, 0.05}}, true);
    const RS_Vector point{10.2, 0.06};
    const LC_OffsetSideResolution side =
        LC_CurveOffset::resolveSide(strip, point, LC_CurveOffset::makeDirectOptions(strip, 1.0));
    REQUIRE(side.status == LC_CurveOffsetStatus::Ok);
    CHECK(side.side == LC_CurveOffsetSide::Right);
}

TEST_CASE("A click outside a sharp turn at a closed curve's seam still has a side", "[curve-offset][direct][side]") {
    // The seam sits in a turn of 135 degrees within 1e-4 of parameter. The
    // nearest point lies just after the seam; refined from before it, the seam
    // itself, no minimum, is as near to the tolerance but on the incoming tangent.
    const LC_SplinePoints corner = fromControlPoints({{0, 0}, {10, 0}, {10, 10}, {0.001, 0.001}}, true);
    const LC_CurveOffsetOptions options = LC_CurveOffset::makeDirectOptions(corner, 1.0);
    for (const RS_Vector& point : {RS_Vector{-0.5, -1.0}, RS_Vector{-1.0, -1.0}}) {
        INFO("point " << point.x << ", " << point.y);
        const LC_OffsetSideResolution side = LC_CurveOffset::resolveSide(corner, point, options);
        REQUIRE(side.status == LC_CurveOffsetStatus::Ok);
        CHECK(side.side == LC_CurveOffsetSide::Right); // outside of a counter-clockwise curve
    }
}

TEST_CASE("The sample limit counts every branch of a cusped offset", "[curve-offset][direct][cusp]") {
    const RS_Spline parabola = makeSpline(2, {{-2, 4}, {0, -4}, {2, 4}}, {0, 0, 0, 1, 1, 1});
    const LC_CurveOffsetOptions options = LC_CurveOffset::makeDirectOptions(parabola, 1.0);
    const LC_CurveOffsetGeometryResult geometry = LC_CurveOffset::buildDirectBranches(
        parabola, LC_CurveOffset::makeSideRequest(LC_CurveOffsetSide::Left, 1.0), options,
        LC_CurveOffset::makeDirectSourceBudget());
    REQUIRE(geometry.status == LC_CurveOffsetStatus::Ok);
    REQUIRE(geometry.branches.size() == 3);
    // the least sample limit that materializes a geometry
    auto least = [&](const LC_CurveOffsetGeometryResult& g) {
        std::size_t lo = g.exactSamples;
        std::size_t hi = options.maxSamples;
        while (lo < hi) {
            LC_CurveOffsetOptions limited = options;
            limited.maxSamples = lo + (hi - lo) / 2;
            const bool ok = LC_CurveOffset::materializeBranches(parabola, g, limited,
                                                                LC_CurveOffset::makeDirectSourceBudget())
                                .status == LC_CurveOffsetStatus::Ok;
            (ok ? hi : lo) = ok ? limited.maxSamples : limited.maxSamples + 1;
        }
        return lo;
    };
    std::size_t perBranch = 0;
    for (const LC_OffsetBranch& branch : geometry.branches) {
        LC_CurveOffsetGeometryResult one = geometry;
        one.branches = {branch};
        perBranch += least(one) - geometry.exactSamples;
    }
    CHECK(least(geometry) == geometry.exactSamples + perBranch);
}

TEST_CASE("A direction point on the curve has no side", "[curve-offset][direct][side]") {
    const RS_Spline s = sCurve();
    LC_CurveJet onCurve;
    REQUIRE(s.tryEvaluateJet(0.3, LC_CurveEvaluationSide::Interior, onCurve));
    CHECK(offsetTowards(s, onCurve.point, 1.0).status == LC_CurveOffsetStatus::AmbiguousSide);
    // on the tangent line beyond an open end
    const LC_SplinePoints line = fromControlPoints({{0.0, 0.0}, {10.0, 0.0}});
    CHECK(offsetTowards(line, RS_Vector{15.0, 0.0}, 1.0).status == LC_CurveOffsetStatus::AmbiguousSide);
    // just off it, the side is defined
    CHECK(offsetTowards(line, RS_Vector{15.0, 0.5}, 1.0).status == LC_CurveOffsetStatus::Ok);
}

TEST_CASE("Reversing the source swaps the side a point selects, not the offset", "[curve-offset][direct][side]") {
    RS_Spline forward = sCurve();
    RS_Spline reversed = sCurve();
    reversed.revertDirection();
    const RS_Vector point{6.0, 4.0};
    const LC_CurveOffsetOptions options = LC_CurveOffset::makeDirectOptions(forward, 1.0);
    const LC_OffsetSideResolution a = LC_CurveOffset::resolveSide(forward, point, options);
    const LC_OffsetSideResolution b = LC_CurveOffset::resolveSide(reversed, point, options);
    REQUIRE(a.status == LC_CurveOffsetStatus::Ok);
    REQUIRE(b.status == LC_CurveOffsetStatus::Ok);
    CHECK(a.side != b.side);

    const LC_CurveOffsetGeometryResult fa = offsetTowards(forward, point, 1.0);
    const LC_CurveOffsetGeometryResult fb = offsetTowards(reversed, point, 1.0);
    REQUIRE(fa.status == LC_CurveOffsetStatus::Ok);
    REQUIRE(fb.status == LC_CurveOffsetStatus::Ok);
    const auto& pa = fa.branches.front().cubicPieces;
    const auto& pb = fb.branches.front().cubicPieces;
    CHECK(pa.front().bezier[0].distanceTo(pb.back().bezier[3]) < 1e-9);
    CHECK(pa.back().bezier[3].distanceTo(pb.front().bezier[0]) < 1e-9);

    // the side of a multi-copy offset is resolved once, for every distance
    const LC_OffsetSideResolution far = LC_CurveOffset::resolveSide(
        forward, point, LC_CurveOffset::makeDirectOptions(forward, 3.0));
    CHECK(far.side == a.side);
}

TEST_CASE("Closed sources offset to a closed chain; a flag without a seam is refused",
          "[curve-offset][direct][closed]") {
    const LC_SplinePoints ring = fromControlPoints({{0, 0}, {10, 0}, {12, 8}, {2, 9}}, true);
    const LC_CurveOffsetGeometryResult outward = offsetTowards(ring, RS_Vector{30.0, 30.0}, 1.0);
    REQUIRE(outward.status == LC_CurveOffsetStatus::Ok);
    const LC_OffsetBranch& branch = outward.branches.front();
    CHECK(branch.closed);
    CHECK(branch.cubicPieces.back().bezier[3] == branch.cubicPieces.front().bezier[0]);
    CHECK(maxDeviation(outward, evaluator(ring)) <=
          LC_CurveOffset::makeDirectOptions(ring, 1.0).tolerance.requestedGeometry);

    RS_Spline periodic(nullptr, RS_SplineData(3, false));
    for (const RS_Vector& p : {RS_Vector{0, 0}, RS_Vector{40, -10}, RS_Vector{60, 30}, RS_Vector{20, 50},
                               RS_Vector{-15, 25}}) {
        periodic.addControlPoint(p);
    }
    periodic.setClosed(true);
    const LC_CurveOffsetGeometryResult inward = offsetTowards(periodic, RS_Vector{22.0, 18.0}, 2.0);
    REQUIRE(inward.status == LC_CurveOffsetStatus::Ok);
    CHECK(inward.branches.front().closed);

    RS_SplineData wrapped(3, true);
    wrapped.type = RS_SplineData::SplineType::WrappedClosed;
    wrapped.controlPoints = {{0, 0}, {15, 25}, {40, 35}, {70, 20}, {80, 0}, {0, 0}, {15, 25}, {40, 35}};
    wrapped.weights.assign(wrapped.controlPoints.size(), 1.0);
    wrapped.knotslist = {0.0, 12.0, 35.0, 60.0, 100.0, 140.0, 180.0, 220.0, 260.0, 290.0, 320.0, 350.0};
    const RS_Spline gap(nullptr, wrapped);
    CHECK(offsetToSide(gap, LC_CurveOffsetSide::Left, 1.0).status == LC_CurveOffsetStatus::InvalidSource);
}

TEST_CASE("Offsets stay accurate far from the origin", "[curve-offset][direct]") {
    const RS_Spline near = sCurve();
    const RS_Spline far = sCurve(RS_Vector{1.0e6, -2.0e6});
    const LC_CurveOffsetGeometryResult a = offsetToSide(near, LC_CurveOffsetSide::Right, 0.5);
    const LC_CurveOffsetGeometryResult b = offsetToSide(far, LC_CurveOffsetSide::Right, 0.5);
    REQUIRE(a.status == LC_CurveOffsetStatus::Ok);
    REQUIRE(b.status == LC_CurveOffsetStatus::Ok);
    CHECK(maxDeviation(b, evaluator(far)) <= LC_CurveOffset::makeDirectOptions(far, 0.5).tolerance.requestedGeometry);
}

TEST_CASE("Unsupported and non-planar sources are refused", "[curve-offset][direct]") {
    RS_Spline raised = sCurve();
    raised.getData().controlPoints[1].z = 1.0;
    CHECK(offsetToSide(raised, LC_CurveOffsetSide::Left, 1.0).status == LC_CurveOffsetStatus::UnsupportedNonPlanar);
    CHECK_FALSE(LC_CurveOffset::isSupportedSource(RS_Line(nullptr, RS_LineData{{0, 0}, {1, 0}})));
    CHECK(LC_CurveOffset::isSupportedSource(sCurve()));
    CHECK(LC_CurveOffset::isSupportedSource(fromControlPoints({{0, 0}, {1, 1}, {2, 0}})));
}

TEST_CASE("Caps end the offset with no branch", "[curve-offset][direct][limits]") {
    const RS_Spline s = sCurve();
    const LC_CurveOffsetOptions options = LC_CurveOffset::makeDirectOptions(s, 0.5);
    const LC_CurveOffsetRequest request = LC_CurveOffset::makeSideRequest(LC_CurveOffsetSide::Left, 0.5);
    const LC_CurveOffsetGeometryResult full =
        LC_CurveOffset::buildDirectBranches(s, request, options, LC_CurveOffset::makeDirectSourceBudget());
    REQUIRE(full.status == LC_CurveOffsetStatus::Ok);
    const size_t pieces = full.branches.front().cubicPieces.size();
    REQUIRE(pieces > 1);

    LC_OffsetSourceBudget tight = LC_CurveOffset::makeDirectSourceBudget();
    tight.maxCubicPieces = pieces - 1;
    const LC_CurveOffsetGeometryResult capped = LC_CurveOffset::buildDirectBranches(s, request, options, tight);
    CHECK(capped.status == LC_CurveOffsetStatus::LimitExceeded);
    CHECK(capped.branches.empty());
    tight.maxCubicPieces = pieces;
    CHECK(LC_CurveOffset::buildDirectBranches(s, request, options, tight).status == LC_CurveOffsetStatus::Ok);

    LC_CurveOffsetOptions fewSamples = options;
    fewSamples.maxSamples = 10;
    CHECK(LC_CurveOffset::buildDirectBranches(s, request, fewSamples, LC_CurveOffset::makeDirectSourceBudget())
              .status == LC_CurveOffsetStatus::LimitExceeded);

    LC_CurveOffsetOptions shallow = options;
    shallow.maxSubdivisionDepth = 1;
    const LC_CurveOffsetGeometryResult unmet =
        LC_CurveOffset::buildDirectBranches(s, request, shallow, LC_CurveOffset::makeDirectSourceBudget());
    CHECK(unmet.status != LC_CurveOffsetStatus::Ok);
    CHECK(unmet.branches.empty());
}

TEST_CASE("Piece counts at the default tolerance", "[curve-offset][direct][calibration]") {
    // Recorded for the D1 tolerance choice: one output entity per piece.
    const RS_Spline s = sCurve();
    const RS_Spline arc = quarterCircle(RS_Vector{0, 0}, 50.0);
    const LC_SplinePoints parabola = fromControlPoints({{0.0, 0.0}, {5.0, 10.0}, {10.0, 0.0}});
    for (const double relative : {1e-4, 1e-5, 1e-6}) {
        auto count = [relative](const RS_Entity& source, const double d) {
            const LC_CurveOffsetOptions probe = LC_CurveOffset::makeDirectOptions(source, d);
            const double feature = probe.tolerance.requestedGeometry / LC_CurveOffset::kDefaultRelativeOffsetTolerance;
            const LC_CurveOffsetOptions options = LC_CurveOffset::makeDirectOptions(source, d, relative * feature);
            const LC_CurveOffsetGeometryResult r = LC_CurveOffset::buildDirectBranches(
                source, LC_CurveOffset::makeSideRequest(LC_CurveOffsetSide::Right, d), options,
                LC_CurveOffset::makeDirectSourceBudget());
            REQUIRE(r.status == LC_CurveOffsetStatus::Ok);
            return r.branches.front().cubicPieces.size();
        };
        const size_t sPieces = count(s, 0.75);
        const size_t arcPieces = count(arc, 5.0);
        const size_t parabolaPieces = count(parabola, 1.0);
        WARN("relative tolerance " << relative << ": S-curve " << sPieces << ", quarter circle " << arcPieces
                                   << ", parabola " << parabolaPieces << " pieces");
        CHECK(sPieces < 200);
    }
}

// ---------------------------------------------------------------------------
// Materialization (D1b)
// ---------------------------------------------------------------------------
namespace {
LC_CurveOffsetMaterializationResult materialize(const RS_Entity& source, const LC_CurveOffsetSide side,
                                                const double distance,
                                                const LC_OffsetSourceBudget budget = LC_CurveOffset::makeDirectSourceBudget()) {
    return LC_CurveOffset::createEntities(source, LC_CurveOffset::makeSideRequest(side, distance),
                                          LC_CurveOffset::makeDirectOptions(source, distance), budget);
}
} // namespace

TEST_CASE("A branch materializes as one cubic spline whose spans are its validated pieces",
          "[curve-offset][direct][materialize]") {
    const RS_Spline s = sCurve();
    const LC_CurveOffsetOptions options = LC_CurveOffset::makeDirectOptions(s, 0.75);
    const LC_CurveOffsetRequest request = LC_CurveOffset::makeSideRequest(LC_CurveOffsetSide::Left, 0.75);
    const LC_CurveOffsetGeometryResult geometry =
        LC_CurveOffset::buildDirectBranches(s, request, options, LC_CurveOffset::makeDirectSourceBudget());
    REQUIRE(geometry.status == LC_CurveOffsetStatus::Ok);
    REQUIRE(geometry.branches.size() == 1);
    const std::vector<LC_OffsetCubicPiece> before = geometry.branches.front().cubicPieces;
    REQUIRE(before.size() > 1);

    const LC_CurveOffsetMaterializationResult result =
        LC_CurveOffset::materializeBranches(s, geometry, options, LC_CurveOffset::makeDirectSourceBudget());
    REQUIRE(result.status == LC_CurveOffsetStatus::Ok);
    CHECK(result.validationLevel == LC_OffsetValidationLevel::SampledBidirectional);
    CHECK(result.maxObservedError <= options.tolerance.requestedGeometry);
    REQUIRE(result.entities.size() == 1);
    CHECK(result.usage.outputEntities == 1);
    CHECK(result.usage.cubicPieces >= before.size());

    const auto* spline = dynamic_cast<const RS_Spline*>(result.entities.front().get());
    REQUIRE(spline != nullptr);
    CHECK(spline->validate());
    CHECK(spline->getDegree() == 3);
    CHECK_FALSE(spline->isClosed());
    CHECK(spline->getParent() == nullptr);
    CHECK_FALSE(spline->isSelected());
    CHECK(result.usage.deepEntities == spline->count());
    // integer knots, of multiplicity 3 inside: each span is one piece's Bezier net
    const RS_SplineData& data = spline->getData();
    const size_t spans = result.usage.cubicPieces;
    REQUIRE(data.controlPoints.size() == 3 * spans + 1);
    REQUIRE(data.knotslist.size() == data.controlPoints.size() + 4);
    for (size_t k = 0; k < data.knotslist.size(); ++k) {
        const double expected = (k < 4) ? 0.0 : std::min(static_cast<double>((k - 1) / 3), static_cast<double>(spans));
        CHECK(data.knotslist[k] == expected);
    }
    // it follows the exact offset like the pieces did
    double t0 = 0.0;
    double t1 = 0.0;
    REQUIRE(spline->getParameterDomain(t0, t1));
    CHECK(t0 == 0.0);
    CHECK(t1 == static_cast<double>(spans));
    CHECK(spline->getStartpoint() == before.front().bezier[0]);
    CHECK(spline->getEndpoint() == before.back().bezier[3]);

    // materializing reads the branch; it never changes it
    const auto& after = geometry.branches.front().cubicPieces;
    REQUIRE(after.size() == before.size());
    for (size_t i = 0; i < before.size(); ++i) {
        CHECK(after[i].bezier == before[i].bezier);
    }
}

TEST_CASE("A straight source materializes as one exact line", "[curve-offset][direct][materialize]") {
    const LC_SplinePoints line = fromControlPoints({{0.0, 0.0}, {10.0, 0.0}});
    const LC_CurveOffsetMaterializationResult result = materialize(line, LC_CurveOffsetSide::Right, 2.0);
    REQUIRE(result.status == LC_CurveOffsetStatus::Ok);
    REQUIRE(result.entities.size() == 1);
    const auto* offset = dynamic_cast<const RS_Line*>(result.entities.front().get());
    REQUIRE(offset != nullptr);
    CHECK(offset->getStartpoint() == RS_Vector(0.0, -2.0));
    CHECK(offset->getEndpoint() == RS_Vector(10.0, -2.0));
    CHECK(result.maxObservedError <= 1e-12);
}

TEST_CASE("Output copies only the source's layer and pen", "[curve-offset][direct][materialize]") {
    RS_Spline s = sCurve();
    const RS_Pen pen{RS_Color{200, 10, 30}, RS2::Width04, RS2::DashLine};
    s.setPen(pen);
    s.setSelectionFlag(true);
    s.setHighlighted(true);
    const LC_CurveOffsetMaterializationResult result = materialize(s, LC_CurveOffsetSide::Right, 0.5);
    REQUIRE(result.status == LC_CurveOffsetStatus::Ok);
    for (const std::unique_ptr<RS_Entity>& entity : result.entities) {
        CHECK(entity->getPen(false) == pen);
        CHECK_FALSE(entity->isSelected());
        CHECK_FALSE(entity->isHighlighted());
        CHECK(entity->getParent() == nullptr);
    }
}

TEST_CASE("Limits and failures return no entity", "[curve-offset][direct][materialize][limits]") {
    const RS_Spline s = sCurve();
    const LC_CurveOffsetMaterializationResult full = materialize(s, LC_CurveOffsetSide::Left, 0.75);
    REQUIRE(full.status == LC_CurveOffsetStatus::Ok);
    const std::size_t pieces = full.usage.cubicPieces;
    REQUIRE(pieces > 1);

    LC_OffsetSourceBudget budget = LC_CurveOffset::makeDirectSourceBudget();
    budget.maxCubicPieces = pieces - 1;
    const LC_CurveOffsetMaterializationResult tooMany = materialize(s, LC_CurveOffsetSide::Left, 0.75, budget);
    CHECK(tooMany.status == LC_CurveOffsetStatus::LimitExceeded);
    CHECK(tooMany.entities.empty());

    budget = LC_CurveOffset::makeDirectSourceBudget();
    budget.maxDeepEntities = full.usage.deepEntities - 1;
    const LC_CurveOffsetMaterializationResult tooDeep = materialize(s, LC_CurveOffsetSide::Left, 0.75, budget);
    CHECK(tooDeep.status == LC_CurveOffsetStatus::LimitExceeded);
    CHECK(tooDeep.entities.empty());

    budget.maxDeepEntities = full.usage.deepEntities;
    CHECK(materialize(s, LC_CurveOffsetSide::Left, 0.75, budget).status == LC_CurveOffsetStatus::Ok);

    // a failed geometry never reaches an entity
    const RS_Spline arc = quarterCircle();
    const LC_CurveOffsetMaterializationResult singular = materialize(arc, LC_CurveOffsetSide::Left, 1.0);
    CHECK(singular.status != LC_CurveOffsetStatus::Ok);
    CHECK(singular.entities.empty());

    // a failed geometry result handed to the materializer is refused as it is
    LC_CurveOffsetGeometryResult failed;
    failed.status = LC_CurveOffsetStatus::SingularOffset;
    CHECK(LC_CurveOffset::materializeBranches(s, failed, LC_CurveOffset::makeDirectOptions(s, 1.0),
                                              LC_CurveOffset::makeDirectSourceBudget())
              .status == LC_CurveOffsetStatus::SingularOffset);
}

TEST_CASE("A closed source materializes as one open spline whose ends meet in a smooth stretch",
          "[curve-offset][direct][materialize][closed]") {
    // LibreCAD's closed spline is a wrapped one, which a chain of cubic pieces is
    // not: the chain is an open spline whose ends coincide, starting in the
    // middle of a piece so that the seam is no corner.
    const LC_SplinePoints ring = fromControlPoints({{0, 0}, {10, 0}, {12, 8}, {2, 9}}, true);
    const LC_CurveOffsetMaterializationResult result = materialize(ring, LC_CurveOffsetSide::Right, 1.0);
    REQUIRE(result.status == LC_CurveOffsetStatus::Ok);
    REQUIRE(result.entities.size() == 1);
    const auto& spline = dynamic_cast<const RS_Spline&>(*result.entities.front());
    CHECK_FALSE(spline.isClosed());
    CHECK(spline.getEndpoint() == spline.getStartpoint());
    double t0 = 0.0;
    double t1 = 0.0;
    REQUIRE(spline.getParameterDomain(t0, t1));
    LC_CurveJet start;
    LC_CurveJet end;
    REQUIRE(spline.tryEvaluateJet(t0, LC_CurveEvaluationSide::Right, start));
    REQUIRE(spline.tryEvaluateJet(t1, LC_CurveEvaluationSide::Left, end));
    CHECK(std::abs(RS_Vector::crossP(start.first, end.first).z) <=
          1e-9 * start.first.magnitude() * end.first.magnitude());
}

TEST_CASE("Offsets of 20-control-point cubics, timed", "[.benchmark]") {
    // The interactive target: below 50 ms for one regular, open cubic with 20
    // control points at the default tolerance, Direct and Trimmed, measured
    // without sanitizers.
    struct Shape {
        const char* name;
        std::vector<RS_Vector> points;
        double distance;
    };
    std::vector<Shape> shapes{{"wave", {}, 1.0}, {"spiral", {}, 2.0}, {"cubic", {}, 5.0}};
    for (int i = 0; i < 20; ++i) {
        shapes[0].points.emplace_back(10.0 * i, 15.0 * std::sin(0.7 * i));
        const double angle = 0.35 * i;
        shapes[1].points.emplace_back((40.0 + 4.0 * i) * std::cos(angle), (40.0 + 4.0 * i) * std::sin(angle));
        shapes[2].points.emplace_back(10.0 * i, 0.02 * (i - 10.0) * (i - 10.0) * (i - 10.0));
    }
    for (const Shape& shape : shapes) {
        RS_SplineData d(3, false);
        d.controlPoints = shape.points;
        d.knotslist = {0, 0, 0, 0};
        for (int k = 1; k <= 16; ++k) {
            d.knotslist.push_back(k);
        }
        d.knotslist.insert(d.knotslist.end(), 4, 17.0);
        d.weights.assign(20, 1.0);
        const RS_Spline source(nullptr, d);
        for (const LC_CurveOffsetMode mode : {LC_CurveOffsetMode::Direct, LC_CurveOffsetMode::Trimmed}) {
            std::vector<double> ms;
            size_t pieces = 0;
            for (int run = 0; run < 41; ++run) {
                LC_CurveOffsetOptions options = LC_CurveOffset::makeDirectOptions(source, shape.distance);
                options.mode = mode;
                const auto start = std::chrono::steady_clock::now();
                const LC_CurveOffsetMaterializationResult r = LC_CurveOffset::createEntities(
                    source, LC_CurveOffset::makeSideRequest(LC_CurveOffsetSide::Left, shape.distance), options,
                    LC_CurveOffset::makeDirectSourceBudget());
                ms.push_back(std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count());
                REQUIRE(r.status == LC_CurveOffsetStatus::Ok);
                pieces = r.entities.size();
            }
            std::sort(ms.begin(), ms.end());
            const double median = ms[ms.size() / 2];
            const double p95 = ms[ms.size() * 95 / 100];
            WARN(shape.name << (mode == LC_CurveOffsetMode::Direct ? " direct: " : " trimmed: ") << pieces
                            << " entities, median " << median << " ms, p95 " << p95 << " ms");
            CHECK(median < 50.0);
        }
    }
}

// ---------------------------------------------------------------------------
// Cusp branches (D2)
// ---------------------------------------------------------------------------
namespace {
/** y = x^2 on [-2, 2], at x = 4t - 2: radius of curvature (1 + 4x^2)^(3/2) / 2, 0.5 at the vertex. */
RS_Spline unitParabola() {
    return makeSpline(2, {{-2, 4}, {0, -4}, {2, 4}}, {0, 0, 0, 1, 1, 1});
}

/** Where the radius of y = x^2 equals d, as parameters of unitParabola(): its cusps inside. */
std::array<double, 2> parabolaCusps(const double d) {
    const double x = std::sqrt((std::cbrt(4.0 * d * d) - 1.0) / 4.0);
    return {(2.0 - x) / 4.0, (2.0 + x) / 4.0};
}
} // namespace

TEST_CASE("Inside its radius of curvature the offset is one branch; past it, it splits at the cusps",
          "[curve-offset][cusp]") {
    const RS_Spline parabola = unitParabola();
    // inside the vertex radius: regular
    CHECK(offsetToSide(parabola, LC_CurveOffsetSide::Left, 0.4).branches.size() == 1);

    for (const double d : {0.501, 0.51, 0.75, 1.0, 1.5}) {
        INFO("distance " << d);
        const LC_CurveOffsetGeometryResult result = offsetToSide(parabola, LC_CurveOffsetSide::Left, d);
        REQUIRE(result.status == LC_CurveOffsetStatus::Ok);
        // before the first cusp, the swallowtail between them, after the second
        REQUIRE(result.branches.size() == 3);
        CHECK_FALSE(result.branches[0].reversed);
        CHECK(result.branches[1].reversed);
        CHECK_FALSE(result.branches[2].reversed);
        CHECK(result.branches[0].startEnd == LC_OffsetBranchEnd::Free);
        CHECK(result.branches[0].endEnd == LC_OffsetBranchEnd::Cusp);
        CHECK(result.branches[2].endEnd == LC_OffsetBranchEnd::Free);
        const LC_CurveOffsetOptions options = LC_CurveOffset::makeDirectOptions(parabola, d);
        CHECK(maxDeviation(result, evaluator(parabola)) <= options.tolerance.requestedGeometry);
        const std::array<double, 2> cusps = parabolaCusps(d);
        for (size_t k = 0; k < 2; ++k) {
            const LC_OffsetCubicPiece& end = result.branches[k].cubicPieces.back();
            CHECK(end.provenance.sourceT1 == Approx(cusps[k]).margin(1e-9));
            // the cusp: the exact offset there, where its derivative vanishes
            LC_CurveJet c;
            REQUIRE(parabola.tryEvaluateJet(cusps[k], LC_CurveEvaluationSide::Interior, c));
            const RS_Vector exact = c.point + RS_Vector{-c.first.y, c.first.x} / c.first.magnitude() * d;
            CHECK(end.bezier[3].distanceTo(exact) <= options.tolerance.requestedGeometry);
            CHECK(result.branches[k + 1].cubicPieces.front().bezier[0] == end.bezier[3]);
        }
        for (const LC_OffsetBranch& branch : result.branches) {
            CHECK_FALSE(branch.closed);
        }
    }
}

TEST_CASE("A swallowtail within twice the merge tolerance is collapsed before anything else", "[curve-offset][cusp]") {
    // A hair past the vertex radius the cusps are resolved but the swallowtail
    // between them spans less than twice the merge tolerance: the branches
    // either side meet at its middle as one branch.
    const RS_Spline parabola = unitParabola();
    for (const double d : {0.5 + 1e-6, 0.5 + 1e-7}) {
        INFO("distance " << d);
        const LC_CurveOffsetGeometryResult result = offsetToSide(parabola, LC_CurveOffsetSide::Left, d);
        REQUIRE(result.status == LC_CurveOffsetStatus::Ok);
        REQUIRE(result.branches.size() == 1);
        CHECK_FALSE(result.branches.front().reversed);
        const LC_CurveOffsetOptions options = LC_CurveOffset::makeDirectOptions(parabola, d);
        CHECK(maxDeviation(result, evaluator(parabola), 2.0 * options.tolerance.nodeMerge) <=
              options.tolerance.requestedGeometry);
    }
}

TEST_CASE("At a radius of curvature the offset stalls and runs on, one branch", "[curve-offset][cusp][stall]") {
    const RS_Spline parabola = unitParabola();
    // At the vertex radius 1 - d kappa touches zero without changing sign: the
    // offset slows to a stop at the centre of curvature (0, 0.5) and runs on in
    // the same direction. A hair past it the two cusps lie closer than the
    // merge tolerance, which leaves the same shape.
    for (const double d : {0.5, 0.5 + 1e-13}) {
        INFO("distance " << d);
        const LC_CurveOffsetGeometryResult result = offsetToSide(parabola, LC_CurveOffsetSide::Left, d);
        REQUIRE(result.status == LC_CurveOffsetStatus::Ok);
        REQUIRE(result.branches.size() == 1);
        const LC_CurveOffsetOptions options = LC_CurveOffset::makeDirectOptions(parabola, d);
        CHECK(maxDeviation(result, evaluator(parabola), options.tolerance.nodeMerge) <=
              options.tolerance.requestedGeometry);
        // the stall is a shared end of two pieces, at the centre of curvature
        const auto& pieces = result.branches.front().cubicPieces;
        const bool atCentre = std::any_of(pieces.begin(), pieces.end(), [&](const LC_OffsetCubicPiece& piece) {
            return piece.bezier[3].distanceTo(RS_Vector{0.0, 0.5}) <= options.tolerance.requestedGeometry;
        });
        CHECK(atCentre);
        // and the same way every time
        CHECK(offsetToSide(parabola, LC_CurveOffsetSide::Left, d).branches.front().cubicPieces.size() ==
              pieces.size());
    }
    // a whole span of zeros: the arc's offset shrinks to its centre
    const LC_CurveOffsetGeometryResult point = offsetToSide(quarterCircle(), LC_CurveOffsetSide::Left, 1.0);
    CHECK(point.status != LC_CurveOffsetStatus::Ok);
    CHECK(point.branches.empty());
}

TEST_CASE("A stall is found whatever the curve's orientation", "[curve-offset][cusp][stall]") {
    // Rotated, the interval bounds of 1 - d kappa no longer cancel, and the
    // unproved run around the vertex is hundreds of boxes long.
    for (const double angle : {0.0, M_PI / 6.0, M_PI / 4.0, 1.0}) {
        RS_Spline parabola = unitParabola();
        parabola.rotate(RS_Vector{0.3, -0.2}, angle);
        for (const double d : {0.5, 0.5 * (1.0 + 1e-9), 0.5 * (1.0 - 1e-9), 0.5 * (1.0 + 1e-6)}) {
            INFO("angle " << angle << " distance " << std::setprecision(17) << d);
            const LC_CurveOffsetGeometryResult result = offsetToSide(parabola, LC_CurveOffsetSide::Left, d);
            REQUIRE(result.status == LC_CurveOffsetStatus::Ok);
            const LC_CurveOffsetOptions options = LC_CurveOffset::makeDirectOptions(parabola, d);
            CHECK(maxDeviation(result, evaluator(parabola), options.tolerance.nodeMerge) <=
                  options.tolerance.requestedGeometry);
        }
    }
}

TEST_CASE("Near the radius of curvature the offset fits far from the origin too", "[curve-offset][cusp][stall]") {
    // Where the offset all but stops its exact tangent is mostly cancellation;
    // the fit weighs a tangent error by how far the offset moves.
    for (const RS_Vector& shift : {RS_Vector{1.0e4, -2.0e4}, RS_Vector{1.0e6, -2.0e6}}) {
        RS_Spline parabola = unitParabola();
        parabola.move(shift);
        for (const double d : {0.49999, 0.499999, 0.5, 0.5 * (1.0 + 1e-6)}) {
            INFO("shift " << shift.x << " distance " << std::setprecision(17) << d);
            const LC_CurveOffsetGeometryResult result = offsetToSide(parabola, LC_CurveOffsetSide::Left, d);
            REQUIRE(result.status == LC_CurveOffsetStatus::Ok);
            const LC_CurveOffsetOptions options = LC_CurveOffset::makeDirectOptions(parabola, d);
            CHECK(maxDeviation(result, evaluator(parabola), 2.0 * options.tolerance.nodeMerge) <=
                  options.tolerance.requestedGeometry);
        }
    }
}

TEST_CASE("A stall across a closed curve's seam closes the branch", "[curve-offset][cusp][stall][closed]") {
    // A uniform closed cubic, symmetric about the x axis, whose seam lies at its
    // sharpest point on the positive x axis: the curvature is largest there.
    RS_Spline lemon(nullptr, RS_SplineData(3, false));
    // (the seam lies at the second control point's influence centre)
    for (const RS_Vector& p : {RS_Vector{2, -1}, RS_Vector{4, 0}, RS_Vector{2, 1}, RS_Vector{-2, 1}, RS_Vector{-4, 0},
                               RS_Vector{-2, -1}}) {
        lemon.addControlPoint(p);
    }
    lemon.setClosed(true);
    double t0 = 0.0;
    double t1 = 0.0;
    REQUIRE(lemon.getParameterDomain(t0, t1));
    LC_CurveJet seam;
    REQUIRE(lemon.tryEvaluateJet(t0, LC_CurveEvaluationSide::Right, seam));
    REQUIRE(std::abs(seam.point.y) < 1e-12);
    const double kappa = RS_Vector::crossP(seam.first, seam.second).z / std::pow(seam.first.magnitude(), 3.0);
    REQUIRE(std::abs(kappa) > 0.0);
    const double rho = 1.0 / std::abs(kappa);
    // the centre of curvature at the seam lies to the side the curve turns
    const LC_CurveOffsetSide inward = kappa > 0.0 ? LC_CurveOffsetSide::Left : LC_CurveOffsetSide::Right;
    const LC_CurveOffsetGeometryResult result = offsetToSide(lemon, inward, rho);
    REQUIRE(result.status == LC_CurveOffsetStatus::Ok);
    REQUIRE(result.branches.size() == 1);
    CHECK(result.branches.front().closed);
    const LC_CurveOffsetOptions options = LC_CurveOffset::makeDirectOptions(lemon, rho);
    CHECK(maxDeviation(result, evaluator(lemon), options.tolerance.nodeMerge) <= options.tolerance.requestedGeometry);
}

TEST_CASE("Cusp branches materialize as entities meeting at the cusps", "[curve-offset][cusp][materialize]") {
    const RS_Spline parabola = unitParabola();
    const LC_CurveOffsetMaterializationResult result = materialize(parabola, LC_CurveOffsetSide::Left, 1.0);
    REQUIRE(result.status == LC_CurveOffsetStatus::Ok);
    REQUIRE(result.entities.size() >= 3);
    for (size_t i = 1; i < result.entities.size(); ++i) {
        CHECK(result.entities[i]->getStartpoint() == result.entities[i - 1]->getEndpoint());
    }
    // the swallowtail between the cusps runs against the source: its entities
    // go right to left while the source goes left to right
    size_t reversed = 0;
    for (const std::unique_ptr<RS_Entity>& entity : result.entities) {
        reversed += entity->getEndpoint().x < entity->getStartpoint().x ? 1 : 0;
    }
    CHECK(reversed > 0);
    CHECK(reversed < result.entities.size());

    // more branches than allowed
    LC_CurveOffsetOptions options = LC_CurveOffset::makeDirectOptions(parabola, 1.0);
    options.maxOutputBranches = 2;
    const LC_CurveOffsetGeometryResult capped = LC_CurveOffset::buildDirectBranches(
        parabola, LC_CurveOffset::makeSideRequest(LC_CurveOffsetSide::Left, 1.0), options,
        LC_CurveOffset::makeDirectSourceBudget());
    CHECK(capped.status == LC_CurveOffsetStatus::LimitExceeded);
    CHECK(capped.branches.empty());
}

TEST_CASE("A closed source's cusped offset is a cycle of open branches", "[curve-offset][cusp]") {
    RS_Spline ring(nullptr, RS_SplineData(3, false));
    for (const RS_Vector& p : {RS_Vector{0, 0}, RS_Vector{40, -10}, RS_Vector{60, 30}, RS_Vector{20, 50},
                               RS_Vector{-15, 25}}) {
        ring.addControlPoint(p);
    }
    ring.setClosed(true);
    double t0 = 0.0;
    double t1 = 0.0;
    REQUIRE(ring.getParameterDomain(t0, t1));
    // its tightest bend, from samples
    double tightest = RS_MAXDOUBLE;
    for (int i = 0; i < 2000; ++i) {
        LC_CurveJet j;
        REQUIRE(ring.tryEvaluateJet(t0 + (t1 - t0) * i / 2000.0, LC_CurveEvaluationSide::Interior, j));
        const double s = j.first.magnitude();
        tightest = std::min(tightest, s * s * s / std::abs(j.first.x * j.second.y - j.first.y * j.second.x));
    }
    // inwards (the ring turns left), well past the tightest bend
    const double d = 1.5 * tightest;
    const LC_CurveOffsetGeometryResult result = offsetToSide(ring, LC_CurveOffsetSide::Left, d);
    REQUIRE(result.status == LC_CurveOffsetStatus::Ok);
    REQUIRE(result.branches.size() >= 2);
    CHECK(result.branches.size() % 2 == 0); // 1 - d kappa changes sign an even number of times around
    for (size_t i = 0; i < result.branches.size(); ++i) {
        const LC_OffsetBranch& branch = result.branches[i];
        const LC_OffsetBranch& next = result.branches[(i + 1) % result.branches.size()];
        CHECK_FALSE(branch.closed);
        CHECK(next.cubicPieces.front().bezier[0] == branch.cubicPieces.back().bezier[3]);
    }
    // every piece against the exact offset
    const LC_CurveOffsetOptions options = LC_CurveOffset::makeDirectOptions(ring, d);
    for (const LC_OffsetBranch& branch : result.branches) {
        for (const LC_OffsetCubicPiece& piece : branch.cubicPieces) {
            const auto& p = piece.provenance;
            for (int k = 1; k < 16; ++k) {
                const double s = k / 16.0;
                LC_CurveJet c;
                REQUIRE(ring.tryEvaluateJet(p.sourceT0 + s * (p.sourceT1 - p.sourceT0),
                                            LC_CurveEvaluationSide::Interior, c));
                const RS_Vector exact = c.point + RS_Vector{-c.first.y, c.first.x} / c.first.magnitude() * d;
                CHECK(bezierAt(piece.bezier, s).distanceTo(exact) <= options.tolerance.requestedGeometry);
            }
        }
    }
}
