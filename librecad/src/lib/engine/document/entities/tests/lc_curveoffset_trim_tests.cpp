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

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <functional>
#include <vector>

#include "lc_curveoffset.h"
#include "lc_splinepoints.h"
#include "rs_spline.h"

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
 */
Checked check(const LC_CurveOffsetGeometryResult& trimmed, const Curve& source, const double t0, const double t1,
              const double d) {
    Checked result;
    std::vector<std::pair<RS_Vector, RS_Vector>> chords;
    for (const LC_OffsetBranch& branch : trimmed.branches) {
        for (const LC_OffsetCubicPiece& piece : branch.cubicPieces) {
            RS_Vector previous = piece.bezier[0];
            for (int k = 1; k <= 24; ++k) {
                const RS_Vector p = bezierAt(piece.bezier, k / 24.0);
                chords.emplace_back(previous, p);
                previous = p;
                if (k % 8 == 0) {
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
        if (distanceTo(source, t0, t1, q) < d * (1.0 - 1e-4)) {
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
