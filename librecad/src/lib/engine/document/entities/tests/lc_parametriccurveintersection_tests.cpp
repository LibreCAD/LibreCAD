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

// Parameter-pair intersections of parametric branches, and noded offsets (T1).

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cmath>
#include <memory>
#include <vector>

#include "lc_curveoffset.h"
#include "lc_parametriccurveintersection.h"
#include "rs_spline.h"

using Catch::Approx;
using Kind = LC_ParametricIntersection::Kind;

namespace {

RS_Spline makeSpline(const size_t degree, const std::vector<RS_Vector>& controls, const std::vector<double>& knots) {
    RS_SplineData data(static_cast<int>(degree), false);
    data.controlPoints = controls;
    data.knotslist = knots;
    data.weights.assign(controls.size(), 1.0);
    return RS_Spline(nullptr, data);
}

RS_Spline line(const RS_Vector& a, const RS_Vector& b) {
    return makeSpline(1, {a, b}, {0, 0, 1, 1});
}

/** Each spline one branch, each of its knot spans one segment. */
class SplineCurves final : public LC_ParametricCurves {
public:
    explicit SplineCurves(std::vector<const RS_Spline*> splines) : m_splines{std::move(splines)} {
        for (size_t b = 0; b < m_splines.size(); ++b) {
            const std::vector<double> breaks = m_splines[b]->getBreakParameters();
            for (size_t i = 0; i + 1 < breaks.size(); ++i) {
                m_segments.push_back({b, breaks[i], breaks[i + 1]});
            }
        }
    }

    const std::vector<LC_ParametricSegment>& segments() const override {
        return m_segments;
    }

    std::size_t branchCount() const override {
        return m_splines.size();
    }

    std::ptrdiff_t next(const std::size_t branch) const override {
        return m_splines[branch]->isClosed() ? static_cast<std::ptrdiff_t>(branch) : -1;
    }

    bool evaluate(const std::size_t segment, const double t, RS_Vector& point, RS_Vector& derivative) const override {
        const LC_ParametricSegment& s = m_segments[segment];
        const LC_CurveEvaluationSide side = (t == s.t0)   ? LC_CurveEvaluationSide::Right
                                            : (t == s.t1) ? LC_CurveEvaluationSide::Left
                                                          : LC_CurveEvaluationSide::Interior;
        LC_CurveJet jet;
        if (!m_splines[s.branch]->tryEvaluateJet(t, side, jet)) {
            return false;
        }
        point = jet.point;
        derivative = jet.first;
        return true;
    }

    bool bound(const std::size_t segment, const double a, const double b, LC_Interval& x, LC_Interval& y,
               LC_Interval& dx, LC_Interval& dy) const override {
        LC_CurveJetBounds bounds;
        if (!m_splines[m_segments[segment].branch]->tryBoundJet(a, b, bounds)) {
            return false;
        }
        x = bounds.x;
        y = bounds.y;
        dx = bounds.dx;
        dy = bounds.dy;
        return true;
    }

private:
    std::vector<const RS_Spline*> m_splines;
    std::vector<LC_ParametricSegment> m_segments;
};

LC_IntersectionResult intersect(std::vector<const RS_Spline*> splines, const double tolerance = 1e-9) {
    LC_IntersectionOptions options;
    options.tolerance = tolerance;
    return findIntersections(SplineCurves{std::move(splines)}, options);
}

/** Crossings of polylines through dense samples of each spline: an independent count. */
size_t sampledCrossings(const std::vector<const RS_Spline*>& splines) {
    std::vector<std::vector<RS_Vector>> polylines;
    for (const RS_Spline* s : splines) {
        double t0 = 0.0;
        double t1 = 0.0;
        REQUIRE(s->getParameterDomain(t0, t1));
        std::vector<RS_Vector> points;
        for (int i = 0; i <= 1000; ++i) {
            points.push_back(s->getPointAt(t0 + (t1 - t0) * i / 1000.0));
        }
        polylines.push_back(points);
    }
    auto cross = [](const RS_Vector& a, const RS_Vector& b) { return a.x * b.y - a.y * b.x; };
    size_t count = 0;
    for (size_t p = 0; p < polylines.size(); ++p) {
        for (size_t q = p; q < polylines.size(); ++q) {
            for (size_t i = 0; i + 1 < polylines[p].size(); ++i) {
                for (size_t j = (p == q) ? i + 2 : 0; j + 1 < polylines[q].size(); ++j) {
                    if (p == q && splines[p]->isClosed() && i == 0 && j + 2 == polylines[q].size()) {
                        continue; // the seam joins them
                    }
                    const RS_Vector a = polylines[p][i];
                    const RS_Vector r = polylines[p][i + 1] - a;
                    const RS_Vector c = polylines[q][j];
                    const RS_Vector s = polylines[q][j + 1] - c;
                    const double d = cross(r, s);
                    if (d == 0.0) {
                        continue;
                    }
                    const double u = cross(c - a, s) / d;
                    const double v = cross(c - a, r) / d;
                    count += (u >= 0.0 && u < 1.0 && v >= 0.0 && v < 1.0) ? 1 : 0;
                }
            }
        }
    }
    return count;
}

} // namespace

TEST_CASE("A loop crosses itself once, found as two occurrences on one branch", "[curve-offset][topology]") {
    const RS_Spline loop = makeSpline(3, {{0, 0}, {15, 10}, {-5, 10}, {10, 0}}, {0, 0, 0, 0, 1, 1, 1, 1});
    REQUIRE(sampledCrossings({&loop}) == 1);
    const LC_IntersectionResult result = intersect({&loop});
    REQUIRE(result.status == LC_IntersectionStatus::Ok);
    REQUIRE(result.intersections.size() == 1);
    const LC_ParametricIntersection& x = result.intersections.front();
    CHECK(x.branchA == 0);
    CHECK(x.branchB == 0);
    CHECK(x.parameterA < x.parameterB);
    CHECK(x.kind == Kind::Transverse);
    // both occurrences are the point, to the tolerance
    CHECK(loop.getPointAt(x.parameterA).distanceTo(x.point) < 1e-9);
    CHECK(loop.getPointAt(x.parameterB).distanceTo(x.point) < 1e-9);
    // by symmetry about x = 5
    CHECK(x.point.x == Approx(5.0).margin(1e-9));
    CHECK(x.parameterA + x.parameterB == Approx(1.0).margin(1e-9));
}

TEST_CASE("A closed figure eight crosses itself at its waist", "[curve-offset][topology]") {
    RS_Spline eight(nullptr, RS_SplineData(3, false));
    for (const RS_Vector& p : {RS_Vector{20, 10}, RS_Vector{20, -10}, RS_Vector{-20, 10}, RS_Vector{-20, -10}}) {
        eight.addControlPoint(p);
    }
    eight.setClosed(true);
    REQUIRE(eight.validate());
    const size_t sampled = sampledCrossings({&eight});
    const LC_IntersectionResult result = intersect({&eight});
    REQUIRE(result.status == LC_IntersectionStatus::Ok);
    CHECK(result.intersections.size() == sampled);
    REQUIRE_FALSE(result.intersections.empty());
    CHECK(result.intersections.front().point.distanceTo(RS_Vector{0.0, 0.0}) < 1e-9);
}

TEST_CASE("A touch is a tangency; a near touch that crosses is two crossings", "[curve-offset][topology]") {
    const RS_Spline parabola = makeSpline(2, {{-2, 4}, {0, -4}, {2, 4}}, {0, 0, 0, 1, 1, 1}); // y = x^2
    const RS_Spline axis = line({-3, 0}, {3, 0});
    const LC_IntersectionResult touch = intersect({&parabola, &axis});
    REQUIRE(touch.status == LC_IntersectionStatus::Ok);
    REQUIRE(touch.intersections.size() == 1);
    CHECK(touch.intersections.front().kind == Kind::Tangent);
    CHECK(touch.intersections.front().point.distanceTo(RS_Vector{0.0, 0.0}) < 1e-6);

    // y = x^2 against y = 1e-4: crossings at x = -0.01 and 0.01
    const RS_Spline raised = line({-3, 1e-4}, {3, 1e-4});
    const LC_IntersectionResult crossing = intersect({&parabola, &raised});
    REQUIRE(crossing.status == LC_IntersectionStatus::Ok);
    REQUIRE(crossing.intersections.size() == 2);
    CHECK(crossing.intersections[0].point.x == Approx(-0.01).margin(1e-9));
    CHECK(crossing.intersections[1].point.x == Approx(0.01).margin(1e-9));
    for (const LC_ParametricIntersection& x : crossing.intersections) {
        CHECK(x.kind == Kind::Transverse);
    }
}

TEST_CASE("A triple point keeps all three incident pairs", "[curve-offset][topology]") {
    const RS_Spline a = line({0, 0}, {2, 2});
    const RS_Spline b = line({0, 2}, {2, 0});
    const RS_Spline c = line({1, -1}, {1, 3});
    const LC_IntersectionResult result = intersect({&a, &b, &c});
    REQUIRE(result.status == LC_IntersectionStatus::Ok);
    REQUIRE(result.intersections.size() == 3);
    CHECK((result.intersections[0].branchA == 0 && result.intersections[0].branchB == 1));
    CHECK((result.intersections[1].branchA == 0 && result.intersections[1].branchB == 2));
    CHECK((result.intersections[2].branchA == 1 && result.intersections[2].branchB == 2));
    for (const LC_ParametricIntersection& x : result.intersections) {
        CHECK(x.point.distanceTo(RS_Vector{1.0, 1.0}) < 1e-12);
        CHECK(x.kind == Kind::Transverse);
    }
}

TEST_CASE("A branch that ends on another meets it at an endpoint", "[curve-offset][topology]") {
    const RS_Spline base = line({0, 0}, {4, 0});
    const RS_Spline post = line({2, 0}, {2, 3});
    const LC_IntersectionResult result = intersect({&base, &post});
    REQUIRE(result.status == LC_IntersectionStatus::Ok);
    REQUIRE(result.intersections.size() == 1);
    CHECK(result.intersections.front().kind == Kind::Endpoint);
    CHECK(result.intersections.front().point.distanceTo(RS_Vector{2.0, 0.0}) < 1e-12);
}

TEST_CASE("Branches that retrace each other are ambiguous, not a crossing", "[curve-offset][topology]") {
    const RS_Spline curve = makeSpline(3, {{0, 0}, {4, 6}, {8, -6}, {12, 0}}, {0, 0, 0, 0, 1, 1, 1, 1});
    const RS_Spline copy = makeSpline(3, {{0, 0}, {4, 6}, {8, -6}, {12, 0}}, {0, 0, 0, 0, 1, 1, 1, 1});
    CHECK(intersect({&curve, &copy}).status == LC_IntersectionStatus::AmbiguousTopology);
    const RS_Spline first = line({0, 0}, {4, 0});
    const RS_Spline overlapping = line({2, 0}, {6, 0});
    CHECK(intersect({&first, &overlapping}).status == LC_IntersectionStatus::AmbiguousTopology);
}

TEST_CASE("The intersections do not depend on the order of the branches", "[curve-offset][topology]") {
    const RS_Spline loop = makeSpline(3, {{0, 0}, {15, 10}, {-5, 10}, {10, 0}}, {0, 0, 0, 0, 1, 1, 1, 1});
    const RS_Spline a = line({-1, 3}, {11, 3});
    const RS_Spline b = line({5, -2}, {5, 12});
    const LC_IntersectionResult forward = intersect({&loop, &a, &b});
    const LC_IntersectionResult backward = intersect({&b, &a, &loop});
    REQUIRE(forward.status == LC_IntersectionStatus::Ok);
    REQUIRE(backward.status == LC_IntersectionStatus::Ok);
    REQUIRE(forward.intersections.size() == backward.intersections.size());
    REQUIRE(forward.intersections.size() == sampledCrossings({&loop, &a, &b}));
    // the same incidences, with branch k of one order being branch 2 - k of the other
    for (const LC_ParametricIntersection& x : forward.intersections) {
        const bool found = std::any_of(backward.intersections.begin(), backward.intersections.end(),
                                       [&x](const LC_ParametricIntersection& y) {
                                           return y.point.distanceTo(x.point) < 1e-9 && y.kind == x.kind &&
                                                  ((2 - y.branchA == x.branchA && 2 - y.branchB == x.branchB) ||
                                                   (2 - y.branchA == x.branchB && 2 - y.branchB == x.branchA));
                                       });
        CHECK(found);
    }
}

TEST_CASE("A noded offset ends pieces at its crossing", "[curve-offset][topology]") {
    // Inside y = x^2 at distance 1 the offset has cusps at x = -0.383 and 0.383,
    // and its two outer branches cross on the axis where sqrt(1 + 4x^2) = 2:
    // at x = -+sqrt(3) / 2, in the point (0, 1.25).
    const RS_Spline parabola = makeSpline(2, {{-2, 4}, {0, -4}, {2, 4}}, {0, 0, 0, 1, 1, 1});
    LC_CurveOffsetOptions options = LC_CurveOffset::makeDirectOptions(parabola, 1.0);
    const LC_CurveOffsetRequest request = LC_CurveOffset::makeSideRequest(LC_CurveOffsetSide::Left, 1.0);

    const LC_CurveOffsetGeometryResult plain =
        LC_CurveOffset::buildDirectBranches(parabola, request, options, LC_CurveOffset::makeDirectSourceBudget());
    REQUIRE(plain.status == LC_CurveOffsetStatus::Ok);
    CHECK(plain.intersections.empty());

    options.nodeIntersections = true;
    const LC_CurveOffsetGeometryResult noded =
        LC_CurveOffset::buildDirectBranches(parabola, request, options, LC_CurveOffset::makeDirectSourceBudget());
    REQUIRE(noded.status == LC_CurveOffsetStatus::Ok);
    CHECK(noded.sourceIntersections == 0);
    REQUIRE(noded.branches.size() == 3);
    REQUIRE(noded.intersections.size() == 1);
    CHECK(noded.offsetIntersections == 1);
    const LC_ParametricIntersection& x = noded.intersections.front();
    CHECK(x.branchA == 0);
    CHECK(x.branchB == 2);
    CHECK(x.point.distanceTo(RS_Vector{0.0, 1.25}) < options.tolerance.requestedGeometry);
    CHECK(x.parameterA == Approx((2.0 - std::sqrt(3.0) / 2.0) / 4.0).margin(1e-9));
    CHECK(x.parameterB == Approx((2.0 + std::sqrt(3.0) / 2.0) / 4.0).margin(1e-9));
    // each occurrence is where two of its branch's pieces meet
    for (const auto& [branch, t] : {std::pair{x.branchA, x.parameterA}, std::pair{x.branchB, x.parameterB}}) {
        const std::vector<LC_OffsetCubicPiece>& pieces = noded.branches[branch].cubicPieces;
        size_t ends = 0;
        for (size_t k = 0; k + 1 < pieces.size(); ++k) {
            if (pieces[k].provenance.sourceT1 == t) {
                CHECK(pieces[k].bezier[3] == x.point);
                CHECK(pieces[k + 1].bezier[0] == x.point);
                ++ends;
            }
        }
        CHECK(ends == 1);
    }

    // The entities pass the check that their fits add no crossing: four meet at the node.
    const LC_CurveOffsetMaterializationResult entities = LC_CurveOffset::materializeBranches(
        parabola, noded, options, LC_CurveOffset::makeDirectSourceBudget());
    REQUIRE(entities.status == LC_CurveOffsetStatus::Ok);
    size_t atNode = 0;
    for (const std::unique_ptr<RS_Entity>& e : entities.entities) {
        atNode += (e->getStartpoint() == x.point) ? 1 : 0;
        atNode += (e->getEndpoint() == x.point) ? 1 : 0;
    }
    CHECK(atNode == 4);
}

TEST_CASE("A noded offset of a crossing source keeps every crossing", "[curve-offset][topology]") {
    // Offsets of a loop on either side cross themselves near the source's crossing.
    const RS_Spline loop = makeSpline(3, {{0, 0}, {15, 10}, {-5, 10}, {10, 0}}, {0, 0, 0, 0, 1, 1, 1, 1});
    for (const LC_CurveOffsetSide side : {LC_CurveOffsetSide::Left, LC_CurveOffsetSide::Right}) {
        LC_CurveOffsetOptions options = LC_CurveOffset::makeDirectOptions(loop, 0.2);
        options.nodeIntersections = true;
        const LC_CurveOffsetMaterializationResult result = LC_CurveOffset::createEntities(
            loop, LC_CurveOffset::makeSideRequest(side, 0.2), options, LC_CurveOffset::makeDirectSourceBudget());
        const LC_CurveOffsetGeometryResult geometry = LC_CurveOffset::buildDirectBranches(
            loop, LC_CurveOffset::makeSideRequest(side, 0.2), options, LC_CurveOffset::makeDirectSourceBudget());
        INFO("side " << static_cast<int>(side));
        REQUIRE(result.status == LC_CurveOffsetStatus::Ok);
        REQUIRE(geometry.status == LC_CurveOffsetStatus::Ok);
        CHECK(geometry.intersections.size() >= 1);
        CHECK(geometry.sourceIntersections == 1);
        for (const LC_ParametricIntersection& x : geometry.intersections) {
            size_t ends = 0;
            for (const std::unique_ptr<RS_Entity>& e : result.entities) {
                ends += (e->getStartpoint() == x.point) ? 1 : 0;
                ends += (e->getEndpoint() == x.point) ? 1 : 0;
            }
            CHECK(ends == 4);
        }
    }
}

TEST_CASE("A closed source's cusped offset is noded across its branch joins", "[curve-offset][topology]") {
    RS_Spline ring(nullptr, RS_SplineData(3, false));
    for (const RS_Vector& p : {RS_Vector{0, 0}, RS_Vector{40, -10}, RS_Vector{60, 30}, RS_Vector{20, 50},
                               RS_Vector{-15, 25}}) {
        ring.addControlPoint(p);
    }
    ring.setClosed(true);
    double t0 = 0.0;
    double t1 = 0.0;
    REQUIRE(ring.getParameterDomain(t0, t1));
    double tightest = RS_MAXDOUBLE;
    for (int i = 0; i < 2000; ++i) {
        LC_CurveJet j;
        REQUIRE(ring.tryEvaluateJet(t0 + (t1 - t0) * i / 2000.0, LC_CurveEvaluationSide::Interior, j));
        const double s = j.first.magnitude();
        tightest = std::min(tightest, s * s * s / std::abs(j.first.x * j.second.y - j.first.y * j.second.x));
    }
    const double d = 1.5 * tightest;
    LC_CurveOffsetOptions options = LC_CurveOffset::makeDirectOptions(ring, d);
    options.nodeIntersections = true;
    const LC_CurveOffsetRequest request = LC_CurveOffset::makeSideRequest(LC_CurveOffsetSide::Left, d);
    const LC_CurveOffsetGeometryResult geometry =
        LC_CurveOffset::buildDirectBranches(ring, request, options, LC_CurveOffset::makeDirectSourceBudget());
    REQUIRE(geometry.status == LC_CurveOffsetStatus::Ok);
    REQUIRE(geometry.branches.size() >= 2);
    const LC_CurveOffsetMaterializationResult entities =
        LC_CurveOffset::materializeBranches(ring, geometry, options, LC_CurveOffset::makeDirectSourceBudget());
    REQUIRE(entities.status == LC_CurveOffsetStatus::Ok);
    for (const LC_ParametricIntersection& x : geometry.intersections) {
        size_t ends = 0;
        for (const std::unique_ptr<RS_Entity>& e : entities.entities) {
            ends += (e->getStartpoint() == x.point) ? 1 : 0;
            ends += (e->getEndpoint() == x.point) ? 1 : 0;
        }
        CHECK(ends == 4);
    }
    // the swallowtail between each pair of cusps crosses the rest of the offset
    CHECK(geometry.intersections.size() >= geometry.branches.size() / 2);
}
