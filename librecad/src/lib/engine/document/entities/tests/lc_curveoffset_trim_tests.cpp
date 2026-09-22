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

// Trimmed offsets (T2): the parts of the Direct offset nearer to the source
// than the distance are removed, the rest kept, and what cannot be decided fails.

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <functional>
#include <iomanip>
#include <sstream>
#include <vector>

#include "lc_curveoffset.h"
#include "lc_splinepoints.h"
#include "rs_spline.h"

using Catch::Approx;

namespace {

using Curve = std::function<bool(double, LC_CurveJet&)>;

/** Distance from p to a curve on [t0, t1]: the nearest of dense samples, refined by Newton steps. */
double distanceTo(const Curve& curve, const double t0, const double t1, const RS_Vector& p) {
    double best = t0;
    double bestDistance = RS_MAXDOUBLE;
    constexpr int samples = 400;
    for (int i = 0; i <= samples; ++i) {
        const double t = t0 + (t1 - t0) * i / samples;
        LC_CurveJet j;
        if (curve(t, j) && j.point.distanceTo(p) < bestDistance) {
            bestDistance = j.point.distanceTo(p);
            best = t;
        }
    }
    for (int k = 0; k < 20; ++k) {
        LC_CurveJet j;
        if (!curve(best, j)) {
            break;
        }
        const RS_Vector r = j.point - p;
        const double h = RS_Vector::dotP(j.first, j.first) + RS_Vector::dotP(r, j.second);
        if (!(h > 0.0)) {
            break;
        }
        best = std::clamp(best - RS_Vector::dotP(r, j.first) / h, t0, t1);
        if (curve(best, j)) {
            bestDistance = std::min(bestDistance, j.point.distanceTo(p));
        }
    }
    return bestDistance;
}

RS_Vector bezierAt(const std::array<RS_Vector, 4>& b, const double s) {
    const double r = 1.0 - s;
    return b[0] * (r * r * r) + b[1] * (3.0 * r * r * s) + b[2] * (3.0 * r * s * s) + b[3] * (s * s * s);
}

double distanceToSegment(const RS_Vector& p, const RS_Vector& a, const RS_Vector& b) {
    const RS_Vector ab = b - a;
    const double len2 = ab.squared();
    const double t = len2 > 0.0 ? std::clamp(RS_Vector::dotP(p - a, ab) / len2, 0.0, 1.0) : 0.0;
    return p.distanceTo(a + ab * t);
}

struct Checked {
    /** The least distance from a kept point to the source. */
    double nearestKept{RS_MAXDOUBLE};
    /** The farthest a visible point of the Direct offset lies from what is kept. */
    double worstMissing{0.0};
};

/**
 * Two-way check of a Trimmed result against the source: every kept point lies
 * at least about d from the source, and every point of the exact offset whose
 * distance to the source is d (it is its own nearest) lies near a kept piece.
 * Points nearer than @p visibleFrom (by default d less a ten-thousandth) count
 * as hidden, or at a transition.
 */
Checked check(const LC_CurveOffsetGeometryResult& trimmed, const Curve& source, const double t0, const double t1,
              const double d, double visibleFrom = -1.0) {
    if (visibleFrom < 0.0) {
        visibleFrom = d * (1.0 - 1e-4);
    }
    Checked result;
    std::vector<std::pair<RS_Vector, RS_Vector>> chords;
    for (const LC_OffsetBranch& branch : trimmed.branches) {
        for (const LC_OffsetCubicPiece& piece : branch.cubicPieces) {
            RS_Vector previous = piece.bezier[0];
            for (int k = 1; k <= 96; ++k) {
                const RS_Vector p = bezierAt(piece.bezier, k / 96.0);
                chords.emplace_back(previous, p);
                previous = p;
                if (k % 32 == 0) {
                    result.nearestKept = std::min(result.nearestKept, distanceTo(source, t0, t1, p));
                }
            }
        }
    }
    constexpr int samples = 300;
    for (int i = 0; i <= samples; ++i) {
        const double t = t0 + (t1 - t0) * i / samples;
        LC_CurveJet c;
        if (!source(t, c)) {
            continue;
        }
        const RS_Vector q = c.point + RS_Vector{-c.first.y, c.first.x} / c.first.magnitude() * trimmed.signedDistance;
        if (distanceTo(source, t0, t1, q) < visibleFrom) {
            continue; // hidden, or at a transition
        }
        double nearest = RS_MAXDOUBLE;
        for (const auto& [a, b] : chords) {
            nearest = std::min(nearest, distanceToSegment(q, a, b));
        }
        result.worstMissing = std::max(result.worstMissing, nearest);
    }
    return result;
}

LC_CurveOffsetGeometryResult trim(const RS_Entity& source, const LC_CurveOffsetSide side, const double d) {
    LC_CurveOffsetOptions options = LC_CurveOffset::makeDirectOptions(source, d);
    options.mode = LC_CurveOffsetMode::Trimmed;
    return LC_CurveOffset::buildDirectBranches(source, LC_CurveOffset::makeSideRequest(side, d), options,
                                               LC_CurveOffset::makeDirectSourceBudget());
}

RS_Spline unitParabola() {
    RS_SplineData data(2, false);
    data.controlPoints = {{-2, 4}, {0, -4}, {2, 4}};
    data.knotslist = {0, 0, 0, 1, 1, 1};
    data.weights.assign(3, 1.0);
    return RS_Spline(nullptr, data);
}

// A cubic with an inflection.
RS_Spline sCurve(const RS_Vector& shift = RS_Vector{0.0, 0.0}) {
    RS_SplineData data(3, false);
    data.controlPoints = {shift + RS_Vector{0, 0}, shift + RS_Vector{4, 6}, shift + RS_Vector{8, -6},
                          shift + RS_Vector{12, 0}};
    data.knotslist = {0, 0, 0, 0, 1, 1, 1, 1};
    data.weights.assign(4, 1.0);
    return RS_Spline(nullptr, data);
}

Curve curveOf(const RS_Spline& s) {
    return [&s](const double t, LC_CurveJet& j) { return s.tryEvaluateJet(t, LC_CurveEvaluationSide::Interior, j); };
}

Curve curveOf(const LC_SplinePoints& s) {
    return [&s](const double t, LC_CurveJet& j) { return s.tryEvaluateJet(t, LC_CurveEvaluationSide::Interior, j); };
}

} // namespace

TEST_CASE("Trimming removes a local loop and keeps the offset around it", "[curve-offset][trim]") {
    // Inside y = x^2 at distance 1: the swallowtail between the cusps, and the
    // two stretches from the cusps to the crossing at (0, 1.25), are nearer
    // than 1 to the parabola; what remains is one curve with a corner there.
    const RS_Spline parabola = unitParabola();
    const LC_CurveOffsetGeometryResult result = trim(parabola, LC_CurveOffsetSide::Left, 1.0);
    REQUIRE(result.status == LC_CurveOffsetStatus::Ok);
    CHECK(result.removedIntervals == 3);
    REQUIRE(result.branches.size() == 1);
    const std::vector<LC_OffsetCubicPiece>& pieces = result.branches.front().cubicPieces;
    const double tolerance = LC_CurveOffset::makeDirectOptions(parabola, 1.0).tolerance.requestedGeometry;
    CHECK(pieces.front().bezier[0].distanceTo(RS_Vector{-2.0 + 4.0 / std::sqrt(17.0), 4.0 + 1.0 / std::sqrt(17.0)}) <
          tolerance);
    CHECK(pieces.back().bezier[3].distanceTo(RS_Vector{2.0 - 4.0 / std::sqrt(17.0), 4.0 + 1.0 / std::sqrt(17.0)}) <
          tolerance);
    const bool cornered = std::any_of(pieces.begin(), pieces.end(), [&](const LC_OffsetCubicPiece& piece) {
        return piece.bezier[3].distanceTo(RS_Vector{0.0, 1.25}) < tolerance;
    });
    CHECK(cornered);
    const Checked checked = check(result, curveOf(parabola), 0.0, 1.0, 1.0);
    CHECK(checked.nearestKept >= 1.0 - 2.0 * tolerance);
    CHECK(checked.worstMissing <= 2.0 * tolerance);

    // the outside of the bend has nothing to remove
    const LC_CurveOffsetGeometryResult outside = trim(parabola, LC_CurveOffsetSide::Right, 1.0);
    REQUIRE(outside.status == LC_CurveOffsetStatus::Ok);
    CHECK(outside.removedIntervals == 0);
    CHECK(outside.branches.size() == 1);
}

TEST_CASE("Trimming a narrow waist leaves the two lobes as separate components", "[curve-offset][trim]") {
    // A closed peanut: inside its waist the offsets of the two sides cross each
    // other, and everything between them is nearer than d to the other side.
    RS_Spline peanut(nullptr, RS_SplineData(3, false));
    for (const RS_Vector& p :
         {RS_Vector{-24, 0}, RS_Vector{-20, 12}, RS_Vector{-10, 14}, RS_Vector{-4, 4}, RS_Vector{4, 4},
          RS_Vector{10, 14}, RS_Vector{20, 12}, RS_Vector{24, 0}, RS_Vector{20, -12}, RS_Vector{10, -14},
          RS_Vector{4, -4}, RS_Vector{-4, -4}, RS_Vector{-10, -14}, RS_Vector{-20, -12}}) {
        peanut.addControlPoint(p);
    }
    peanut.setClosed(true);
    REQUIRE(peanut.validate());
    double t0 = 0.0;
    double t1 = 0.0;
    REQUIRE(peanut.getParameterDomain(t0, t1));
    // half the waist's width: its upper side's lowest point near x = 0
    double halfWidth = RS_MAXDOUBLE;
    for (int i = 0; i <= 4000; ++i) {
        LC_CurveJet j;
        REQUIRE(peanut.tryEvaluateJet(t0 + (t1 - t0) * i / 4000.0, LC_CurveEvaluationSide::Interior, j));
        if (std::abs(j.point.x) < 0.5 && j.point.y > 0.0) {
            halfWidth = std::min(halfWidth, j.point.y);
        }
    }
    REQUIRE(halfWidth < 8.0);
    const double d = 1.4 * halfWidth;
    // over the top from left to right, the curve turns clockwise: inside is its right
    const LC_CurveOffsetGeometryResult result = trim(peanut, LC_CurveOffsetSide::Right, d);
    REQUIRE(result.status == LC_CurveOffsetStatus::Ok);
    CHECK(result.removedIntervals > 0);
    REQUIRE(result.branches.size() == 2);
    for (const LC_OffsetBranch& lobe : result.branches) {
        // each lobe's offset is a loop of its own, on its own side of the waist
        CHECK(lobe.cubicPieces.back().bezier[3] == lobe.cubicPieces.front().bezier[0]);
    }
    const double x0 = result.branches[0].cubicPieces.front().bezier[0].x;
    const double x1 = result.branches[1].cubicPieces.front().bezier[0].x;
    CHECK(x0 * x1 < 0.0);
    const double tolerance = LC_CurveOffset::makeDirectOptions(peanut, d).tolerance.requestedGeometry;
    const Checked checked = check(result, curveOf(peanut), t0, t1, d);
    CHECK(checked.nearestKept >= d - 2.0 * tolerance);
    CHECK(checked.worstMissing <= 2.0 * tolerance);
}

TEST_CASE("Trimming fails where it cannot decide, rather than guessing", "[curve-offset][trim]") {
    // Straight arms exactly 2d apart: their offsets lie on one line, retracing
    // each other, so which parts are hidden is not a matter of sampling.
    LC_SplinePointsData keyhole(false, false);
    keyhole.useControlPoints = true;
    keyhole.controlPoints = {{-20, 0}, {-10, 0}, {0, 0},   {10, -8}, {24, -8},
                             {24, 18}, {10, 18}, {0, 10},  {-10, 10}, {-20, 10}};
    const LC_SplinePoints source(nullptr, keyhole);
    const LC_CurveOffsetGeometryResult retraced = trim(source, LC_CurveOffsetSide::Left, 5.0);
    CHECK(retraced.status == LC_CurveOffsetStatus::AmbiguousTopology);
    CHECK(retraced.branches.empty());
    // the outer side is decidable: that the inner offsets retrace does not bear on it
    const LC_CurveOffsetGeometryResult outer = trim(source, LC_CurveOffsetSide::Right, 5.0);
    CHECK(outer.status == LC_CurveOffsetStatus::Ok);
    CHECK(outer.removedIntervals == 0);

    // a distance budget too small to decide any point is an explicit failure
    const RS_Spline parabola = unitParabola();
    LC_CurveOffsetOptions starved = LC_CurveOffset::makeDirectOptions(parabola, 1.0);
    starved.mode = LC_CurveOffsetMode::Trimmed;
    starved.maxDistanceMapBoxes = 1;
    const LC_CurveOffsetGeometryResult undecided = LC_CurveOffset::buildDirectBranches(
        parabola, LC_CurveOffset::makeSideRequest(LC_CurveOffsetSide::Left, 1.0), starved,
        LC_CurveOffset::makeDirectSourceBudget());
    CHECK(undecided.status == LC_CurveOffsetStatus::LimitExceeded);
    CHECK(undecided.branches.empty());
}

TEST_CASE("A trimmed offset materializes as its kept pieces", "[curve-offset][trim]") {
    const RS_Spline parabola = unitParabola();
    LC_CurveOffsetOptions options = LC_CurveOffset::makeDirectOptions(parabola, 1.0);
    options.mode = LC_CurveOffsetMode::Trimmed;
    const LC_CurveOffsetMaterializationResult result = LC_CurveOffset::createEntities(
        parabola, LC_CurveOffset::makeSideRequest(LC_CurveOffsetSide::Left, 1.0), options,
        LC_CurveOffset::makeDirectSourceBudget());
    REQUIRE(result.status == LC_CurveOffsetStatus::Ok);
    REQUIRE_FALSE(result.entities.empty());
    for (size_t i = 1; i < result.entities.size(); ++i) {
        CHECK(result.entities[i]->getStartpoint() == result.entities[i - 1]->getEndpoint());
    }
}

TEST_CASE("An open spline that looks closed and smooth trims far from the origin too", "[curve-offset][trim]") {
    // Its offset's two free ends meet tangentially: an end contact, not one
    // whose sides can be told apart at these coordinates.
    for (const RS_Vector& shift : {RS_Vector{0.0, 0.0}, RS_Vector{1.0e4, -2.0e4}, RS_Vector{1.0e5, -2.0e5}}) {
        LC_SplinePointsData loop(false, false);
        loop.useControlPoints = true;
        for (const RS_Vector& p : {RS_Vector{0, 0}, RS_Vector{10, 0}, RS_Vector{10, 10}, RS_Vector{-10, 10},
                                   RS_Vector{-10, 0}, RS_Vector{0, 0}}) {
            loop.controlPoints.push_back(p + shift);
        }
        const LC_SplinePoints source(nullptr, loop);
        for (const double d : {0.5, 1.0, 2.0}) {
            for (const LC_CurveOffsetSide side : {LC_CurveOffsetSide::Left, LC_CurveOffsetSide::Right}) {
                INFO("shift " << shift.x << " distance " << d << " side " << static_cast<int>(side));
                CHECK(trim(source, side, d).status == LC_CurveOffsetStatus::Ok);
            }
        }
    }
}

TEST_CASE("An open spline whose ends coincide trims like any other", "[curve-offset][trim]") {
    // Its two end circles are the same circle: how they meet each other does
    // not bear on the offset, which only needs where it meets them.
    LC_SplinePointsData teardrop(false, false);
    teardrop.useControlPoints = true;
    teardrop.controlPoints = {{0, 0}, {20, 10}, {20, -10}, {0, 0}};
    const LC_SplinePoints source(nullptr, teardrop);
    for (const LC_CurveOffsetSide side : {LC_CurveOffsetSide::Left, LC_CurveOffsetSide::Right}) {
        INFO("side " << static_cast<int>(side));
        const LC_CurveOffsetGeometryResult result = trim(source, side, 1.0);
        CHECK(result.status == LC_CurveOffsetStatus::Ok);
    }
}

TEST_CASE("An outer offset trims although the inner side stalls at the same distance", "[curve-offset][trim]") {
    // The inner offset at the apex radius is the cutter the outer side is
    // trimmed against; it stalls at the centre of curvature instead of failing.
    LC_SplinePointsData arch(false, false);
    arch.useControlPoints = true;
    arch.controlPoints = {{0.0, 0.0}, {5.0, 10.0}, {10.0, 0.0}};
    const LC_SplinePoints parabola(nullptr, arch);
    for (const LC_CurveOffsetSide side : {LC_CurveOffsetSide::Left, LC_CurveOffsetSide::Right}) {
        INFO("side " << static_cast<int>(side));
        const LC_CurveOffsetGeometryResult result = trim(parabola, side, 2.5);
        CHECK(result.status == LC_CurveOffsetStatus::Ok);
        CHECK(result.branches.size() == 1);
    }
}

TEST_CASE("An offset with nothing to trim keeps its exact ends, anywhere in the plane", "[curve-offset][trim]") {
    // The half circles ahead of an open curve's ends meet its offset only at
    // the offset's own ends, tangentially; they cut nothing there.
    for (const RS_Vector& shift : {RS_Vector{0.0, 0.0}, RS_Vector{1.0e3, -2.0e3}, RS_Vector{1.0e6, -2.0e6}}) {
        INFO("shift " << shift.x << ", " << shift.y);
        const RS_Spline source = sCurve(shift);
        for (const LC_CurveOffsetSide side : {LC_CurveOffsetSide::Left, LC_CurveOffsetSide::Right}) {
            const LC_CurveOffsetGeometryResult trimmed = trim(source, side, 0.5);
            REQUIRE(trimmed.status == LC_CurveOffsetStatus::Ok);
            REQUIRE(trimmed.branches.size() == 1);
            CHECK(trimmed.removedIntervals == 0);
            const LC_CurveOffsetGeometryResult direct = LC_CurveOffset::buildDirectBranches(
                source, LC_CurveOffset::makeSideRequest(side, 0.5), LC_CurveOffset::makeDirectOptions(source, 0.5),
                LC_CurveOffset::makeDirectSourceBudget());
            REQUIRE(direct.status == LC_CurveOffsetStatus::Ok);
            CHECK(trimmed.branches.front().cubicPieces.front().bezier[0] ==
                  direct.branches.front().cubicPieces.front().bezier[0]);
            CHECK(trimmed.branches.front().cubicPieces.back().bezier[3] ==
                  direct.branches.front().cubicPieces.back().bezier[3]);
        }
    }
}

TEST_CASE("The outer offset trims whatever the inner side does at the same distance", "[curve-offset][trim]") {
    // The inner offset is only the cutter: where it runs is evaluated exactly,
    // and its cusps and stalls near the vertex radius are no concern.
    for (const double angle : {0.0, 7.0 * M_PI / 180.0, M_PI / 6.0}) {
        RS_Spline parabola = unitParabola();
        parabola.rotate(RS_Vector{0.0, 0.0}, angle);
        for (const double d : {0.5 - 1e-9, 0.5, 0.5 + 1e-6, 0.5 + 1e-4}) {
            INFO("angle " << angle << " distance " << d);
            const LC_CurveOffsetGeometryResult result = trim(parabola, LC_CurveOffsetSide::Right, d);
            REQUIRE(result.status == LC_CurveOffsetStatus::Ok);
            CHECK(result.branches.size() == 1);
            CHECK(result.removedIntervals == 0);
        }
    }
}

TEST_CASE("Near the radius of curvature the trimmed offset is one clean curve", "[curve-offset][trim]") {
    // Around the vertex radius 0.5 the swallowtail shrinks to nothing: stalled,
    // collapsed, or kept only where it crosses itself, and its wings, nearer
    // than the distance by less than the tolerance can show, are removed.
    for (const double angle : {0.0, M_PI / 6.0}) {
        RS_Spline parabola = unitParabola();
        parabola.rotate(RS_Vector{0.0, 0.0}, angle);
        for (const double d : {0.5 * (1.0 - 1e-9), 0.5, 0.5 * (1.0 + 1e-9), 0.5 * (1.0 + 1e-7),
                               0.5 * (1.0 + 1e-5), 0.5 * (1.0 + 1e-3), 0.51}) {
            INFO("angle " << angle << " distance " << d);
            const LC_CurveOffsetGeometryResult result = trim(parabola, LC_CurveOffsetSide::Left, d);
            REQUIRE(result.status == LC_CurveOffsetStatus::Ok);
            CHECK(result.branches.size() == 1);
            const double tolerance = LC_CurveOffset::makeDirectOptions(parabola, d).tolerance.requestedGeometry;
            const Checked checked = check(result, curveOf(parabola), 0.0, 1.0, d);
            CHECK(checked.nearestKept >= d - 2.0 * tolerance);
            CHECK(checked.worstMissing <= 2.0 * tolerance);
        }
    }
}

TEST_CASE("A cusp near an open end does not take the offset before it along", "[curve-offset][trim]") {
    // The left half of y = x^2 ends at its vertex, just past which the offset
    // at 0.501 has a cusp: the reversed tail from it to the offset's end is
    // hidden, and so is the stretch before the cusp back to where the offset
    // enters the half circle ahead of the end. The rest stays.
    RS_SplineData half(2, false);
    half.controlPoints = {{-2, 4}, {-1, 0}, {0, 0}};
    half.knotslist = {0, 0, 0, 1, 1, 1};
    half.weights.assign(3, 1.0);
    const RS_Spline source(nullptr, half);
    for (const double d : {0.501, 0.5 * (1.0 + 1e-7), 0.51}) {
        INFO("distance " << d);
        const LC_CurveOffsetGeometryResult result = trim(source, LC_CurveOffsetSide::Left, d);
        REQUIRE(result.status == LC_CurveOffsetStatus::Ok);
        REQUIRE(result.branches.size() == 1);
        const double tolerance = LC_CurveOffset::makeDirectOptions(source, d).tolerance.requestedGeometry;
        // between the cut and the cusp the offset is hidden by less than the
        // default band, some 3e-7: only points at the distance must be kept
        const Checked checked = check(result, curveOf(source), 0.0, 1.0, d, d * (1.0 - 1e-9));
        CHECK(checked.nearestKept >= d - 2.0 * tolerance);
        CHECK(checked.worstMissing <= 2.0 * tolerance);
    }
}

TEST_CASE("A closed curve shrunk past its largest radius of curvature leaves nothing", "[curve-offset][trim]") {
    // Inside a circle-like closed curve, deeper than any radius of curvature:
    // the offset runs against the curve everywhere, so all of it is hidden.
    RS_Spline ring(nullptr, RS_SplineData(3, false));
    for (int k = 0; k < 8; ++k) {
        const double a = k * M_PI / 4.0;
        ring.addControlPoint(RS_Vector{10.0 * std::cos(a), 10.0 * std::sin(a)});
    }
    ring.setClosed(true);
    const LC_CurveOffsetGeometryResult result = trim(ring, LC_CurveOffsetSide::Left, 20.0);
    REQUIRE(result.status == LC_CurveOffsetStatus::Ok);
    CHECK(result.branches.empty());
}

namespace {

/** A degree-1 spline through the points: a polyline, open or closed. */
RS_Spline polylineSpline(const std::vector<RS_Vector>& points, const bool closed = false) {
    if (closed) {
        RS_Spline spline(nullptr, RS_SplineData(1, false));
        for (const RS_Vector& p : points) {
            spline.addControlPoint(p);
        }
        spline.setClosed(true);
        return spline;
    }
    RS_SplineData data(1, false);
    data.controlPoints = points;
    data.knotslist = {0.0};
    for (size_t i = 0; i < points.size(); ++i) {
        data.knotslist.push_back(static_cast<double>(i));
    }
    data.knotslist.push_back(static_cast<double>(points.size() - 1));
    data.weights.assign(points.size(), 1.0);
    return RS_Spline(nullptr, data);
}

/** The chain of a one-branch result as its piece ends, and whether any piece is an arc. */
std::vector<RS_Vector> corners(const LC_OffsetBranch& branch, bool& arcs) {
    std::vector<RS_Vector> points{branch.cubicPieces.front().bezier[0]};
    arcs = false;
    for (const LC_OffsetCubicPiece& piece : branch.cubicPieces) {
        points.push_back(piece.bezier[3]);
        arcs = arcs || piece.provenance.arcCentre.valid;
    }
    return points;
}

bool passesThrough(const std::vector<RS_Vector>& points, const RS_Vector& p, const double tolerance) {
    return std::any_of(points.begin(), points.end(), [&](const RS_Vector& v) { return v.distanceTo(p) <= tolerance; });
}

double domainEnd(const RS_Spline& spline) {
    double t0 = 0.0;
    double t1 = 0.0;
    REQUIRE(spline.getParameterDomain(t0, t1));
    return t1;
}

} // namespace

TEST_CASE("A polyline spline offsets like a polyline: trimmed where it turns in, rounded where it turns out",
          "[curve-offset][trim][kink]") {
    // The research's reference cases (§6.2); a corner that turns away from the
    // offset gets an arc of radius d about it, as a round-joined buffer does.
    struct Case {
        const char* name;
        std::vector<RS_Vector> points;
        LC_CurveOffsetSide side;
        double d;
        RS_Vector first;
        RS_Vector last;
        std::vector<RS_Vector> through;
        bool rounded;
    };
    const std::vector<Case> cases{
        {"convex L", {{0, 0}, {10, 0}, {10, 10}}, LC_CurveOffsetSide::Right, 1.0, {0, -1}, {11, 10},
         {{10, -1}, {11, 0}}, true},
        {"concave L", {{0, 0}, {10, 0}, {10, 10}}, LC_CurveOffsetSide::Left, 1.0, {0, 1}, {9, 10}, {{9, 1}}, false},
        {"acute inside", {{0, 0}, {10, 0}, {5, 10}}, LC_CurveOffsetSide::Left, 1.0, {0, 1}, {4.105572809, 9.552786405},
         {{8.381966011, 1}}, false},
        {"chamfer vanishes", {{0, 0}, {10, 0}, {11, 1}, {11, 10}}, LC_CurveOffsetSide::Left, 2.0, {0, 2}, {9, 10},
         {{9, 2}}, false},
        {"hairpin", {{0, 0}, {10, 0}, {0, 0.5}}, LC_CurveOffsetSide::Right, 1.0, {0, -1}, {0.049937695, 1.498752},
         {{10, -1}}, true},
    };
    for (const Case& c : cases) {
        INFO(c.name);
        const RS_Spline source = polylineSpline(c.points);
        const LC_CurveOffsetGeometryResult result = trim(source, c.side, c.d);
        REQUIRE(result.status == LC_CurveOffsetStatus::Ok);
        REQUIRE(result.branches.size() == 1);
        const double tolerance = LC_CurveOffset::makeDirectOptions(source, c.d).tolerance.requestedGeometry;
        bool arcs = false;
        const std::vector<RS_Vector> points = corners(result.branches.front(), arcs);
        CHECK(points.front().distanceTo(c.first) <= 1e-6);
        CHECK(points.back().distanceTo(c.last) <= 1e-6);
        for (const RS_Vector& p : c.through) {
            CHECK(passesThrough(points, p, 2.0 * tolerance));
        }
        CHECK(arcs == c.rounded);
        const Checked checked = check(result, curveOf(source), 0.0, domainEnd(source), c.d);
        CHECK(checked.nearestKept >= c.d - 2.0 * tolerance);
        CHECK(checked.worstMissing <= 2.0 * tolerance);
    }
}

TEST_CASE("The overlap at a slight inward corner is cut, although by less than the tolerance can see",
          "[curve-offset][trim][kink]") {
    // Turning 0.005 rad towards the offset: the two sides' offsets overlap by
    // about 0.0025, nearer to the source than the distance by some 3e-6 at most,
    // but they end at the corner, which the offset cannot run on through.
    const RS_Spline source = polylineSpline({{0, 0}, {10, 0}, {20, 0.05}});
    const LC_CurveOffsetGeometryResult result = trim(source, LC_CurveOffsetSide::Left, 1.0);
    REQUIRE(result.status == LC_CurveOffsetStatus::Ok);
    REQUIRE(result.branches.size() == 1);
    const double tolerance = LC_CurveOffset::makeDirectOptions(source, 1.0).tolerance.requestedGeometry;
    const Checked checked = check(result, curveOf(source), 0.0, domainEnd(source), 1.0);
    CHECK(checked.nearestKept >= 1.0 - 2.0 * tolerance);
    CHECK(checked.worstMissing <= 2.0 * tolerance);
}

TEST_CASE("A polyline spline that turns back on itself is refused, at once", "[curve-offset][trim][kink]") {
    // (0,0) -> (10,0) -> (5,0): its two sides' offsets run along each other,
    // exactly the distance from both, which trimming cannot resolve; AutoCAD
    // refuses such a source too. Its Direct offset is rounded at the turn.
    const RS_Spline source = polylineSpline({{0, 0}, {10, 0}, {5, 0}});
    const auto started = std::chrono::steady_clock::now();
    const LC_CurveOffsetGeometryResult result = trim(source, LC_CurveOffsetSide::Left, 1.0);
    const double seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - started).count();
    CHECK(result.status == LC_CurveOffsetStatus::AmbiguousTopology);
    CHECK(seconds < 1.0);

    const LC_CurveOffsetGeometryResult direct = LC_CurveOffset::buildDirectBranches(
        source, LC_CurveOffset::makeSideRequest(LC_CurveOffsetSide::Left, 1.0),
        LC_CurveOffset::makeDirectOptions(source, 1.0), LC_CurveOffset::makeDirectSourceBudget());
    REQUIRE(direct.status == LC_CurveOffsetStatus::Ok);
    REQUIRE(direct.branches.size() == 1);
    bool arcs = false;
    const std::vector<RS_Vector> points = corners(direct.branches.front(), arcs);
    CHECK(arcs);
    CHECK(points.front().distanceTo(RS_Vector{0, 1}) <= 1e-9);
    CHECK(points.back().distanceTo(RS_Vector{5, -1}) <= 1e-9);
}

TEST_CASE("A polyline spline shrunk past its width leaves nothing", "[curve-offset][trim][kink]") {
    // an open U inwards by twice its width, and a 10 x 4 rectangle by 3 or 2 and a hair
    const LC_CurveOffsetGeometryResult u =
        trim(polylineSpline({{0, 0}, {10, 0}, {10, 1}, {0, 1}}), LC_CurveOffsetSide::Left, 2.0);
    REQUIRE(u.status == LC_CurveOffsetStatus::Ok);
    CHECK(u.branches.empty());
    const RS_Spline rectangle = polylineSpline({{0, 0}, {10, 0}, {10, 4}, {0, 4}}, true);
    for (const double d : {3.0, 2.5}) {
        INFO("distance " << d);
        const LC_CurveOffsetGeometryResult shrunk = trim(rectangle, LC_CurveOffsetSide::Left, d);
        REQUIRE(shrunk.status == LC_CurveOffsetStatus::Ok);
        CHECK(shrunk.branches.empty());
    }
}

TEST_CASE("A closed polyline spline shrinks with sharp corners and grows with round ones", "[curve-offset][trim][kink]") {
    const RS_Spline rectangle = polylineSpline({{0, 0}, {10, 0}, {10, 4}, {0, 4}}, true);
    const LC_CurveOffsetGeometryResult shrunk = trim(rectangle, LC_CurveOffsetSide::Left, 1.0);
    REQUIRE(shrunk.status == LC_CurveOffsetStatus::Ok);
    REQUIRE(shrunk.branches.size() == 1);
    CHECK(shrunk.branches.front().closed);
    const double tolerance = LC_CurveOffset::makeDirectOptions(rectangle, 1.0).tolerance.requestedGeometry;
    bool arcs = false;
    const std::vector<RS_Vector> inner = corners(shrunk.branches.front(), arcs);
    CHECK_FALSE(arcs);
    for (const RS_Vector& p : {RS_Vector{1, 1}, RS_Vector{9, 1}, RS_Vector{9, 3}, RS_Vector{1, 3}}) {
        CHECK(passesThrough(inner, p, 2.0 * tolerance));
    }

    const LC_CurveOffsetGeometryResult grown = trim(rectangle, LC_CurveOffsetSide::Right, 1.0);
    REQUIRE(grown.status == LC_CurveOffsetStatus::Ok);
    REQUIRE(grown.branches.size() == 1);
    CHECK(grown.branches.front().closed);
    const std::vector<RS_Vector> outer = corners(grown.branches.front(), arcs);
    CHECK(arcs);
    for (const RS_Vector& p : {RS_Vector{0, -1}, RS_Vector{10, -1}, RS_Vector{11, 0}, RS_Vector{11, 4},
                               RS_Vector{10, 5}, RS_Vector{0, 5}, RS_Vector{-1, 4}, RS_Vector{-1, 0}}) {
        CHECK(passesThrough(outer, p, 2.0 * tolerance));
    }
    const Checked checked = check(grown, curveOf(rectangle), 0.0, domainEnd(rectangle), 1.0);
    CHECK(checked.nearestKept >= 1.0 - 2.0 * tolerance);
}

TEST_CASE("A slot narrower than twice the distance is bridged by the grown outline", "[curve-offset][trim][kink]") {
    // Round corners about the notch's top corners meet above it.
    const RS_Spline slot =
        polylineSpline({{0, 0}, {20, 0}, {20, 10}, {11, 10}, {11, 4}, {9, 4}, {9, 10}, {0, 10}}, true);
    const LC_CurveOffsetGeometryResult grown = trim(slot, LC_CurveOffsetSide::Right, 2.0);
    REQUIRE(grown.status == LC_CurveOffsetStatus::Ok);
    REQUIRE(grown.branches.size() == 1);
    CHECK(grown.branches.front().closed);
    const double tolerance = LC_CurveOffset::makeDirectOptions(slot, 2.0).tolerance.requestedGeometry;
    bool arcs = false;
    const std::vector<RS_Vector> outline = corners(grown.branches.front(), arcs);
    // the two arcs about (9, 10) and (11, 10) cross above the notch's middle
    CHECK(passesThrough(outline, RS_Vector{10.0, 10.0 + std::sqrt(3.0)}, 2.0 * tolerance));
    for (const RS_Vector& p : outline) {
        CHECK(p.x >= -2.0 - tolerance);
        CHECK(p.x <= 22.0 + tolerance);
        CHECK(p.y >= -2.0 - tolerance);
        CHECK(p.y <= 12.0 + tolerance);
    }
    const Checked checked = check(grown, curveOf(slot), 0.0, domainEnd(slot), 2.0);
    CHECK(checked.nearestKept >= 2.0 - 2.0 * tolerance);
}

TEST_CASE("A point in a wide corner's wedge takes the corner's outer side", "[curve-offset][side][kink]") {
    // The polyline turns left by about 153 degrees at (10, 0). (11, 1) is nearest
    // to the corner, on its outside: the right, whichever one tangent says.
    const RS_Spline turn = polylineSpline({{0, 0}, {10, 0}, {0, 5}});
    const LC_OffsetSideResolution side =
        LC_CurveOffset::resolveSide(turn, RS_Vector{11.0, 1.0}, LC_CurveOffset::makeDirectOptions(turn, 1.0));
    REQUIRE(side.status == LC_CurveOffsetStatus::Ok);
    CHECK(side.side == LC_CurveOffsetSide::Right);
    CHECK(side.distance == Approx(std::sqrt(2.0)).epsilon(1e-9));
}

TEST_CASE("A region still refuses a source with a corner", "[curve-offset][region][kink]") {
    const RS_Spline square = polylineSpline({{0, 0}, {10, 0}, {10, 10}, {0, 10}}, true);
    LC_CurveOffsetOptions options = LC_CurveOffset::makeDirectOptions(square, 1.0);
    options.mode = LC_CurveOffsetMode::RegionBoundary;
    const LC_CurveOffsetGeometryResult result = LC_CurveOffset::buildDirectBranches(
        square, LC_CurveOffset::makeSideRequest(LC_CurveOffsetSide::Right, 1.0), options,
        LC_CurveOffset::makeDirectSourceBudget());
    // it rounds no corner of its own: refused, whichever step finds the corner first
    CHECK(result.status != LC_CurveOffsetStatus::Ok);
    CHECK(result.branches.empty());
}

namespace {

/** The largest curvature of a spline over its domain, sampled and refined. */
double largestCurvature(const RS_Spline& spline) {
    double t0 = 0.0;
    double t1 = 0.0;
    REQUIRE(spline.getParameterDomain(t0, t1));
    const auto curvature = [&spline](const double t) {
        LC_CurveJet j;
        REQUIRE(spline.tryEvaluateJet(t, LC_CurveEvaluationSide::Interior, j));
        return std::abs(RS_Vector::crossP(j.first, j.second).z) / std::pow(j.first.magnitude(), 3.0);
    };
    double best = t0;
    for (int i = 0; i <= 4000; ++i) {
        const double t = t0 + (t1 - t0) * i / 4000.0;
        if (curvature(t) > curvature(best)) {
            best = t;
        }
    }
    // golden-section refinement around the best sample
    double lo = std::max(t0, best - (t1 - t0) / 4000.0);
    double hi = std::min(t1, best + (t1 - t0) / 4000.0);
    for (int k = 0; k < 100; ++k) {
        const double a = hi - 0.6180339887498949 * (hi - lo);
        const double b = lo + 0.6180339887498949 * (hi - lo);
        (curvature(a) > curvature(b) ? hi : lo) = (curvature(a) > curvature(b)) ? b : a;
    }
    return std::max(curvature(lo), curvature(best));
}

} // namespace

TEST_CASE("Around a curvature maximum on a knot the trimmed offset is one clean curve", "[curve-offset][trim]") {
    // Two parabola pieces meeting at their common vertex, the largest curvature
    // exactly on the knot, where the rounded sign of 1 - d kappa is noise.
    RS_SplineData data(2, false);
    data.controlPoints = {{-2, 4}, {-1, 0}, {1, 0}, {2, 4}};
    data.knotslist = {0, 0, 0, 1, 2, 2, 2};
    data.weights.assign(4, 1.0);
    for (const double angle : {0.0, 10.0, 20.0, 30.0, 60.0}) {
        RS_Spline source(nullptr, data);
        source.rotate(RS_Vector{0.0, 0.0}, angle * M_PI / 180.0);
        const double rho = 1.0 / largestCurvature(source);
        for (const double factor : {1.0 - 1e-9, 1.0, 1.0 + 1e-9, 1.0 + 1e-6}) {
            const double d = rho * factor;
            INFO("angle " << angle << " distance " << std::setprecision(17) << d);
            const LC_CurveOffsetGeometryResult result = trim(source, LC_CurveOffsetSide::Left, d);
            REQUIRE(result.status == LC_CurveOffsetStatus::Ok);
            CHECK(result.branches.size() == 1);
            const double tolerance = LC_CurveOffset::makeDirectOptions(source, d).tolerance.requestedGeometry;
            const Checked checked = check(result, curveOf(source), 0.0, 2.0, d, d * (1.0 - 1e-9));
            CHECK(checked.nearestKept >= d - 2.0 * tolerance);
            CHECK(checked.worstMissing <= 2.0 * tolerance);
        }
    }
}

TEST_CASE("Near an asymmetric curvature maximum the trimmed offset is one clean curve", "[curve-offset][trim]") {
    // A cubic whose curvature rises and falls at different rates: the crossing
    // of the swallowtail's outer arms lies far from the middle of its cusps.
    RS_SplineData data(3, false);
    data.controlPoints = {{0, 0}, {6, 1}, {7, 3}, {6.5, 6}, {2, 9}};
    data.knotslist = {0, 0, 0, 0, 0.3, 1, 1, 1, 1};
    data.weights.assign(5, 1.0);
    const RS_Spline source(nullptr, data);
    const double rho = 1.0 / largestCurvature(source);
    for (const double factor : {1.0 - 1e-9, 1.0, 1.0 + 1e-9, 1.0 + 1e-6, 1.0 + 1e-4, 1.01}) {
        const double d = rho * factor;
        INFO("distance " << std::setprecision(17) << d << " factor " << factor);
        const LC_CurveOffsetGeometryResult result = trim(source, LC_CurveOffsetSide::Left, d);
        REQUIRE(result.status == LC_CurveOffsetStatus::Ok);
        CHECK(result.branches.size() == 1);
        const double tolerance = LC_CurveOffset::makeDirectOptions(source, d).tolerance.requestedGeometry;
        const Checked checked = check(result, curveOf(source), 0.0, 1.0, d, d * (1.0 - 1e-9));
        CHECK(checked.nearestKept >= d - 2.0 * tolerance);
        CHECK(checked.worstMissing <= 2.0 * tolerance);
    }
}

TEST_CASE("A trimmed offset with loops stays fast and whole far from the origin", "[curve-offset][trim]") {
    // Next to a cusp the offset's arms are closer than coordinates this large
    // can tell apart; with the reversed branches dropped first, nothing asks.
    for (const RS_Vector& shift : {RS_Vector{1.0e4, -2.0e4}, RS_Vector{1.0e5, -2.0e5}, RS_Vector{1.0e6, -2.0e6}}) {
        const RS_Spline source = sCurve(shift);
        for (const double d : {1.0, 3.0, 5.0}) {
            INFO("shift " << shift.x << " distance " << d);
            const auto started = std::chrono::steady_clock::now();
            const LC_CurveOffsetGeometryResult result = trim(source, LC_CurveOffsetSide::Left, d);
            const double seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - started).count();
            REQUIRE(result.status == LC_CurveOffsetStatus::Ok);
            CHECK_FALSE(result.branches.empty());
            CHECK(seconds < 1.0);
            const LC_CurveOffsetGeometryResult near = trim(sCurve(), LC_CurveOffsetSide::Left, d);
            REQUIRE(near.status == LC_CurveOffsetStatus::Ok);
            CHECK(result.branches.size() == near.branches.size());
        }
    }
}

TEST_CASE("A corner after a reversed stretch is rounded, and what the reversal hides is cut",
          "[curve-offset][trim][kink]") {
    // A line to (9, 0), a quarter circle of radius 1 about (9, 1) to (10, 1),
    // then a line to (15, 1); on the left at 3 the arc's offset runs backwards
    // (radius 1 - 3), and the corner at (10, 1) turns away: rounded about it.
    RS_SplineData data(2, false);
    data.controlPoints = {{0, 0}, {4.5, 0}, {9, 0}, {10, 0}, {10, 1}, {12.5, 1}, {15, 1}};
    data.knotslist = {0, 0, 0, 1, 1, 2, 2, 3, 3, 3};
    data.weights = {1.0, 1.0, 1.0, M_SQRT1_2, 1.0, 1.0, 1.0};
    const RS_Spline source(nullptr, data);
    const LC_CurveOffsetGeometryResult result = trim(source, LC_CurveOffsetSide::Left, 3.0);
    REQUIRE(result.status == LC_CurveOffsetStatus::Ok);
    REQUIRE(result.branches.size() == 1);
    const double tolerance = LC_CurveOffset::makeDirectOptions(source, 3.0).tolerance.requestedGeometry;
    bool arcs = false;
    const std::vector<RS_Vector> points = corners(result.branches.front(), arcs);
    CHECK(arcs);
    CHECK(points.front().distanceTo(RS_Vector{0, 3}) <= 1e-9);
    CHECK(points.back().distanceTo(RS_Vector{15, 4}) <= 1e-9);
    // y = 3 meets the round corner's circle, radius 3 about (10, 1), at x = 10 - sqrt(5)
    CHECK(passesThrough(points, RS_Vector{10.0 - std::sqrt(5.0), 3.0}, 2.0 * tolerance));
    CHECK(passesThrough(points, RS_Vector{10.0, 4.0}, 2.0 * tolerance));
    const Checked checked = check(result, curveOf(source), 0.0, 3.0, 3.0, 3.0 * (1.0 - 1e-9));
    CHECK(checked.nearestKept >= 3.0 - 2.0 * tolerance);
}

