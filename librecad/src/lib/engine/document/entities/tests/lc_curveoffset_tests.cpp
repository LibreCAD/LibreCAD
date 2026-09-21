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

#include <array>
#include <cmath>
#include <functional>
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

/**
 * The largest distance between the pieces and the exact offset C + d N at the
 * same source parameter, at samples the fitter never used; also checks the
 * pieces are contiguous and follow the source parameter.
 */
double maxDeviation(const LC_CurveOffsetGeometryResult& result, const EvalFn& eval) {
    REQUIRE(result.status == LC_CurveOffsetStatus::Ok);
    REQUIRE(result.branches.size() == 1);
    const LC_OffsetBranch& branch = result.branches.front();
    REQUIRE_FALSE(branch.cubicPieces.empty());
    double worst = 0.0;
    double previousT1 = branch.cubicPieces.front().provenance.sourceT0;
    for (size_t i = 0; i < branch.cubicPieces.size(); ++i) {
        const LC_OffsetCubicPiece& piece = branch.cubicPieces[i];
        const auto& p = piece.provenance;
        CHECK(p.sourceT0 < p.sourceT1);
        CHECK(p.sourceT0 == previousT1);
        previousT1 = p.sourceT1;
        CHECK(p.signedDistance == result.signedDistance);
        if (i > 0) {
            CHECK(piece.bezier[0] == branch.cubicPieces[i - 1].bezier[3]);
        }
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
    partial.mode = LC_CurveOffsetMode::Trimmed; // not implemented: never run as Direct
    CHECK(LC_CurveOffset::buildDirectBranches(source, request, partial, budget).status ==
          LC_CurveOffsetStatus::InvalidRequest);

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
    // inside its apex, past the radius of curvature, the offset has cusps
    CHECK(offsetToSide(parabola, LC_CurveOffsetSide::Right, 3.0).status == LC_CurveOffsetStatus::SingularOffset);
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

    // An S-curve's tighter bend: past its curvature radius the offset has a cusp.
    const RS_Spline s = sCurve();
    const LC_CurveOffsetGeometryResult cusped = offsetToSide(s, LC_CurveOffsetSide::Left, 6.0);
    CHECK(cusped.status == LC_CurveOffsetStatus::SingularOffset);
    CHECK(cusped.branches.empty());

    // A source whose tangent vanishes has no normal there.
    const RS_Spline cusp = makeSpline(3, {{0, 0}, {2, 1}, {0, 1}, {2, 0}}, {0, 0, 0, 0, 1, 1, 1, 1});
    const LC_CurveOffsetGeometryResult noTangent = offsetToSide(cusp, LC_CurveOffsetSide::Left, 0.1);
    CHECK((noTangent.status == LC_CurveOffsetStatus::UndefinedTangent ||
           noTangent.status == LC_CurveOffsetStatus::AmbiguousRegularity));
    CHECK(noTangent.branches.empty());
}

TEST_CASE("A kink in the source is refused, a smooth join is not", "[curve-offset][direct]") {
    const RS_Spline kinked = makeSpline(3, {{0, 0}, {1, 1}, {2, 1}, {3, 0}, {4, 2}, {5, 2}, {6, 0}},
                                        {0, 0, 0, 0, 1, 1, 1, 2, 2, 2, 2});
    CHECK(offsetToSide(kinked, LC_CurveOffsetSide::Left, 0.2).status == LC_CurveOffsetStatus::DiscontinuousNormal);

    // the same knot multiplicity with collinear handles is G1
    const RS_Spline smooth = makeSpline(3, {{0, 0}, {1, 1}, {2, 1}, {3, 1}, {4, 1}, {5, 2}, {6, 0}},
                                        {0, 0, 0, 0, 1, 1, 1, 2, 2, 2, 2});
    const LC_CurveOffsetGeometryResult result = offsetToSide(smooth, LC_CurveOffsetSide::Left, 0.2);
    REQUIRE(result.status == LC_CurveOffsetStatus::Ok);
    CHECK(maxDeviation(result, evaluator(smooth)) <=
          LC_CurveOffset::makeDirectOptions(smooth, 0.2).tolerance.requestedGeometry);
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
