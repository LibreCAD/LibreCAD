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

// Region boundaries (R1): the region a closed source encloses under a fill
// rule, grown or shrunk by a disk, as closed cycles with the result on the left.

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

#include "lc_curveoffset.h"
#include "rs_spline.h"

namespace {

RS_Spline closedSpline(const std::vector<RS_Vector>& controls) {
    RS_Spline built(nullptr, RS_SplineData(3, false));
    for (const RS_Vector& p : controls) {
        built.addControlPoint(p);
    }
    built.setClosed(true);
    return RS_Spline(nullptr, built.getData()); // never a copy of the container
}

RS_Vector bezierAt(const std::array<RS_Vector, 4>& b, const double s) {
    const double r = 1.0 - s;
    return b[0] * (r * r * r) + b[1] * (3.0 * r * r * s) + b[2] * (3.0 * r * s * s) + b[3] * (s * s * s);
}

/** Dense samples of the closed source, as a polygon. */
std::vector<RS_Vector> polygonOf(const RS_Spline& s, const int samples = 1500) {
    double t0 = 0.0;
    double t1 = 0.0;
    REQUIRE(s.getParameterDomain(t0, t1));
    std::vector<RS_Vector> points;
    for (int i = 0; i < samples; ++i) {
        LC_CurveJet j;
        REQUIRE(s.tryEvaluateJet(t0 + (t1 - t0) * i / samples, LC_CurveEvaluationSide::Interior, j));
        points.push_back(j.point);
    }
    return points;
}

int winding(const std::vector<RS_Vector>& polygon, const RS_Vector& p) {
    double turn = 0.0;
    for (size_t i = 0; i < polygon.size(); ++i) {
        const RS_Vector u = polygon[i] - p;
        const RS_Vector v = polygon[(i + 1) % polygon.size()] - p;
        turn += std::atan2(u.x * v.y - u.y * v.x, u.x * v.x + u.y * v.y);
    }
    return static_cast<int>(std::lround(turn / (2.0 * M_PI)));
}

double distanceToSegment(const RS_Vector& p, const RS_Vector& a, const RS_Vector& b) {
    const RS_Vector ab = b - a;
    const double len2 = ab.squared();
    const double t = len2 > 0.0 ? std::clamp(RS_Vector::dotP(p - a, ab) / len2, 0.0, 1.0) : 0.0;
    return p.distanceTo(a + ab * t);
}

bool includedBy(const int w, const LC_CurveFillRule rule) {
    return rule == LC_CurveFillRule::EvenOdd ? (w % 2 != 0) : (w != 0);
}

/** The polygon edges between faces that differ under the fill rule: the region's boundary. */
std::vector<std::pair<RS_Vector, RS_Vector>> regionEdges(const std::vector<RS_Vector>& polygon,
                                                         const LC_CurveFillRule rule) {
    std::vector<std::pair<RS_Vector, RS_Vector>> edges;
    for (size_t i = 0; i < polygon.size(); ++i) {
        const RS_Vector a = polygon[i];
        const RS_Vector b = polygon[(i + 1) % polygon.size()];
        const RS_Vector mid = (a + b) * 0.5;
        const RS_Vector n = RS_Vector{-(b - a).y, (b - a).x} / (b - a).magnitude() * 1e-4;
        if (includedBy(winding(polygon, mid + n), rule) != includedBy(winding(polygon, mid - n), rule)) {
            edges.emplace_back(a, b);
        }
    }
    return edges;
}

struct CycleCheck {
    /** Largest | distance to the region's boundary - d | over the output. */
    double worstDistance{0.0};
    /** Output points on the wrong side of the region (inside when growing, outside when shrinking). */
    int wrongSide{0};
    /** Signed areas of the cycles, positive counter-clockwise. */
    std::vector<double> areas;
    bool closed{true};
};

CycleCheck checkCycles(const LC_CurveOffsetGeometryResult& result, const std::vector<RS_Vector>& polygon,
                       const LC_CurveFillRule rule, const double d, const bool grow) {
    const auto edges = regionEdges(polygon, rule);
    CycleCheck check;
    for (const LC_OffsetBranch& cycle : result.branches) {
        check.closed = check.closed && cycle.closed &&
                       cycle.cubicPieces.back().bezier[3] == cycle.cubicPieces.front().bezier[0];
        std::vector<RS_Vector> points;
        for (const LC_OffsetCubicPiece& piece : cycle.cubicPieces) {
            for (int k = 0; k < 8; ++k) {
                points.push_back(bezierAt(piece.bezier, k / 8.0));
            }
        }
        double area = 0.0;
        for (size_t i = 0; i < points.size(); ++i) {
            const RS_Vector& a = points[i];
            const RS_Vector& b = points[(i + 1) % points.size()];
            area += 0.5 * (a.x * b.y - a.y * b.x);
        }
        check.areas.push_back(area);
        for (size_t i = 0; i < points.size(); i += 3) {
            double nearest = RS_MAXDOUBLE;
            for (const auto& [a, b] : edges) {
                nearest = std::min(nearest, distanceToSegment(points[i], a, b));
            }
            check.worstDistance = std::max(check.worstDistance, std::abs(nearest - d));
            if (includedBy(winding(polygon, points[i]), rule) == grow) {
                ++check.wrongSide;
            }
        }
    }
    return check;
}

LC_CurveOffsetGeometryResult region(const RS_Spline& source, const double d, const bool grow,
                                    const LC_CurveFillRule rule = LC_CurveFillRule::NonZero) {
    LC_CurveOffsetOptions options = LC_CurveOffset::makeDirectOptions(source, d);
    options.mode = LC_CurveOffsetMode::RegionBoundary;
    options.fillRule = rule;
    return LC_CurveOffset::buildDirectBranches(
        source, LC_CurveOffset::makeSideRequest(grow ? LC_CurveOffsetSide::Right : LC_CurveOffsetSide::Left, d),
        options, LC_CurveOffset::makeDirectSourceBudget());
}

// A ring that crosses nothing, a bow tie whose lobes wind +1 and -1 about their
// waist at the origin, and a limacon whose inner loop is wound twice.
const std::vector<RS_Vector> g_ring{{0, 0}, {40, -10}, {60, 30}, {20, 50}, {-15, 25}};
const std::vector<RS_Vector> g_bowTie{{20, 10}, {20, -10}, {-20, 10}, {-20, -10}};

std::vector<RS_Vector> limacon() {
    std::vector<RS_Vector> points;
    for (int k = 0; k < 16; ++k) {
        const double a = 2.0 * M_PI * k / 16.0;
        const double r = 10.0 + 22.0 * std::cos(a);
        points.emplace_back(r * std::cos(a), r * std::sin(a));
    }
    return points;
}

} // namespace

TEST_CASE("A simple region grows and shrinks to one cycle each", "[curve-offset][region]") {
    const RS_Spline ring = closedSpline(g_ring);
    const std::vector<RS_Vector> polygon = polygonOf(ring);
    for (const bool grow : {true, false}) {
        INFO((grow ? "grow" : "shrink"));
        const LC_CurveOffsetGeometryResult result = region(ring, 2.0, grow);
        REQUIRE(result.status == LC_CurveOffsetStatus::Ok);
        REQUIRE(result.branches.size() == 1);
        const CycleCheck check = checkCycles(result, polygon, LC_CurveFillRule::NonZero, 2.0, grow);
        CHECK(check.closed);
        CHECK(check.worstDistance < 0.02); // against a 1500-point polygon of the source
        CHECK(check.wrongSide == 0);
        CHECK(check.areas.front() > 0.0); // the result on the left
    }
}

TEST_CASE("A bow tie shrinks into its two lobes and grows into one region", "[curve-offset][region]") {
    const RS_Spline bowTie = closedSpline(g_bowTie);
    const std::vector<RS_Vector> polygon = polygonOf(bowTie);
    for (const LC_CurveFillRule rule : {LC_CurveFillRule::NonZero, LC_CurveFillRule::EvenOdd}) {
        const LC_CurveOffsetGeometryResult shrunk = region(bowTie, 1.0, false, rule);
        REQUIRE(shrunk.status == LC_CurveOffsetStatus::Ok);
        CHECK(shrunk.sourceIntersections == 1);
        REQUIRE(shrunk.branches.size() == 2);
        const CycleCheck in = checkCycles(shrunk, polygon, rule, 1.0, false);
        CHECK(in.closed);
        CHECK(in.worstDistance < 0.02);
        CHECK(in.wrongSide == 0);
        for (const double area : in.areas) {
            CHECK(area > 0.0);
        }

        const LC_CurveOffsetGeometryResult grown = region(bowTie, 1.0, true, rule);
        REQUIRE(grown.status == LC_CurveOffsetStatus::Ok);
        REQUIRE(grown.branches.size() == 1);
        const CycleCheck out = checkCycles(grown, polygon, rule, 1.0, true);
        CHECK(out.closed);
        CHECK(out.worstDistance < 0.02);
        CHECK(out.wrongSide == 0);
        CHECK(out.areas.front() > 0.0);
    }
}

TEST_CASE("The fill rule decides whether a doubly wound lobe is a hole", "[curve-offset][region]") {
    const RS_Spline source = closedSpline(limacon());
    const std::vector<RS_Vector> polygon = polygonOf(source);
    REQUIRE(winding(polygon, RS_Vector{3.0, 0.0}) == 2); // inside the inner loop

    // NonZero: the inner loop is part of the region, which grows to its outline alone
    const LC_CurveOffsetGeometryResult nonZero = region(source, 1.0, true, LC_CurveFillRule::NonZero);
    REQUIRE(nonZero.status == LC_CurveOffsetStatus::Ok);
    REQUIRE(nonZero.branches.size() == 1);
    const CycleCheck solid = checkCycles(nonZero, polygon, LC_CurveFillRule::NonZero, 1.0, true);
    CHECK(solid.closed);
    CHECK(solid.worstDistance < 0.02);
    CHECK(solid.wrongSide == 0);

    // EvenOdd: the inner loop is a hole, whose boundary runs clockwise and shrinks as the region grows
    const LC_CurveOffsetGeometryResult evenOdd = region(source, 1.0, true, LC_CurveFillRule::EvenOdd);
    REQUIRE(evenOdd.status == LC_CurveOffsetStatus::Ok);
    REQUIRE(evenOdd.branches.size() == 2);
    const CycleCheck holed = checkCycles(evenOdd, polygon, LC_CurveFillRule::EvenOdd, 1.0, true);
    CHECK(holed.closed);
    CHECK(holed.worstDistance < 0.02);
    CHECK(holed.wrongSide == 0);
    const auto [smaller, larger] = std::minmax_element(holed.areas.begin(), holed.areas.end(),
                                                       [](double l, double r) { return std::abs(l) < std::abs(r); });
    CHECK(*smaller < 0.0);
    CHECK(*larger > 0.0);
}

TEST_CASE("A region does not depend on the direction of its source", "[curve-offset][region]") {
    std::vector<RS_Vector> reversed = g_bowTie;
    std::reverse(reversed.begin(), reversed.end());
    const RS_Spline forward = closedSpline(g_bowTie);
    const RS_Spline backward = closedSpline(reversed);
    for (const bool grow : {true, false}) {
        const LC_CurveOffsetGeometryResult a = region(forward, 1.0, grow);
        const LC_CurveOffsetGeometryResult b = region(backward, 1.0, grow);
        REQUIRE(a.status == LC_CurveOffsetStatus::Ok);
        REQUIRE(b.status == LC_CurveOffsetStatus::Ok);
        REQUIRE(a.branches.size() == b.branches.size());
        const CycleCheck ca = checkCycles(a, polygonOf(forward), LC_CurveFillRule::NonZero, 1.0, grow);
        const CycleCheck cb = checkCycles(b, polygonOf(backward), LC_CurveFillRule::NonZero, 1.0, grow);
        // the same cycles, both counter-clockwise around what they bound
        std::vector<double> areasA = ca.areas;
        std::vector<double> areasB = cb.areas;
        std::sort(areasA.begin(), areasA.end());
        std::sort(areasB.begin(), areasB.end());
        for (size_t i = 0; i < areasA.size(); ++i) {
            CHECK(std::abs(areasA[i] - areasB[i]) < 1e-3 * std::abs(areasA[i]));
        }
    }
}

TEST_CASE("A pinched or tiny region gives cycles or an explicit ambiguity", "[curve-offset][region]") {
    // Shrinking a peanut by exactly half its waist pinches it to a point; a
    // bow tie with one tiny lobe has a face barely larger than the tolerance.
    const RS_Spline peanut = closedSpline({{-24, 0}, {-20, 12}, {-10, 14}, {-4, 4}, {4, 4}, {10, 14}, {20, 12},
                                           {24, 0}, {20, -12}, {10, -14}, {4, -4}, {-4, -4}, {-10, -14},
                                           {-20, -12}});
    const std::vector<RS_Vector> peanutPolygon = polygonOf(peanut);
    double halfWidth = RS_MAXDOUBLE;
    for (const RS_Vector& p : peanutPolygon) {
        if (std::abs(p.x) < 0.5 && p.y > 0.0) {
            halfWidth = std::min(halfWidth, p.y);
        }
    }
    const RS_Spline tinyLobe = closedSpline({{20, 10}, {20, -10}, {-1, 0.5}, {-1, -0.5}});
    struct Case {
        const RS_Spline* source;
        double d;
        bool grow;
    };
    for (const Case& c : {Case{&peanut, halfWidth, false}, Case{&tinyLobe, 0.5, true}, Case{&tinyLobe, 0.05, false}}) {
        const LC_CurveOffsetGeometryResult result = region(*c.source, c.d, c.grow);
        INFO("distance " << c.d << (c.grow ? " grow" : " shrink"));
        CHECK((result.status == LC_CurveOffsetStatus::Ok || result.status == LC_CurveOffsetStatus::AmbiguousTopology));
        if (result.status == LC_CurveOffsetStatus::Ok) {
            const CycleCheck check =
                checkCycles(result, polygonOf(*c.source), LC_CurveFillRule::NonZero, c.d, c.grow);
            CHECK(check.closed);
            CHECK(check.wrongSide == 0);
        }
        else {
            CHECK(result.branches.empty());
        }
    }
}

TEST_CASE("Region cycles materialize, their corner arcs as circles", "[curve-offset][region]") {
    // At a crossing opposite sectors are equal, so a region covering two of the
    // four needs no arc; one covering three, as the limacon's under NonZero,
    // has a reflex corner there, which shrinking rounds with an arc.
    const RS_Spline source = closedSpline(limacon());
    LC_CurveOffsetOptions options = LC_CurveOffset::makeDirectOptions(source, 1.0);
    options.mode = LC_CurveOffsetMode::RegionBoundary;
    const LC_CurveOffsetGeometryResult geometry = LC_CurveOffset::buildDirectBranches(
        source, LC_CurveOffset::makeSideRequest(LC_CurveOffsetSide::Left, 1.0), options,
        LC_CurveOffset::makeDirectSourceBudget());
    REQUIRE(geometry.status == LC_CurveOffsetStatus::Ok);
    REQUIRE(geometry.branches.size() == 1);
    size_t arcs = 0;
    for (const LC_OffsetCubicPiece& piece : geometry.branches.front().cubicPieces) {
        if (piece.provenance.arcCentre.valid) {
            ++arcs;
            CHECK(bezierAt(piece.bezier, 0.5).distanceTo(piece.provenance.arcCentre) ==
                  Catch::Approx(1.0).margin(1e-6));
        }
    }
    CHECK(arcs > 0);
    const CycleCheck check = checkCycles(geometry, polygonOf(source), LC_CurveFillRule::NonZero, 1.0, false);
    CHECK(check.closed);
    CHECK(check.worstDistance < 0.02);
    CHECK(check.wrongSide == 0);
    CHECK(check.areas.front() > 0.0);

    const LC_CurveOffsetMaterializationResult entities =
        LC_CurveOffset::materializeBranches(source, geometry, options, LC_CurveOffset::makeDirectSourceBudget());
    REQUIRE(entities.status == LC_CurveOffsetStatus::Ok);
    REQUIRE_FALSE(entities.entities.empty());
    CHECK(entities.entities.back()->getEndpoint() == entities.entities.front()->getStartpoint());
    for (size_t i = 1; i < entities.entities.size(); ++i) {
        CHECK(entities.entities[i]->getStartpoint() == entities.entities[i - 1]->getEndpoint());
    }
}
