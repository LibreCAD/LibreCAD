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

// Offsets of ellipses and elliptic arcs by the curve-offset engine. Every
// result is checked against the exact ellipse: each point at the distance (P1,
// P2), running the source's way (P5), not crossing itself (P6), and closed or
// open as expected (P9, an open-typed spline whose ends meet counting as
// closed); and against the closed forms and oracle values of the plan.

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cmath>
#include <memory>
#include <tuple>
#include <vector>

#include "lc_curveoffset.h"
#include "lc_hyperbola.h"
#include "lc_offsetresultcheck.h"
#include "lc_parabola.h"
#include "rs_document.h"
#include "rs_ellipse.h"
#include "rs_modification.h"
#include "rs_spline.h"

namespace {

/** An ellipse or arc about the origin with semi-axis @p a along x, ratio @p ratio, eta from @p from to @p to. */
RS_Ellipse ellipse(const double a, const double ratio, const double from = 0.0, const double to = 0.0,
                   const bool reversed = false) {
    return RS_Ellipse{nullptr, RS_EllipseData{RS_Vector{0, 0}, RS_Vector{a, 0}, ratio, from, to, reversed}};
}

/** The part of the ellipse a test measures against: eta from @p from over @p sweep (negative runs back). */
struct Curve {
    const RS_Ellipse& ellipse;
    double from;
    double sweep;

    RS_Vector at(const double eta) const {
        return ellipse.getEllipsePoint(eta);
    }

    /** The direction it runs at eta. */
    RS_Vector tangent(const double eta) const {
        const RS_Vector major = ellipse.getMajorP();
        const RS_Vector minor = RS_Vector{-major.y, major.x} * ellipse.getRatio();
        return (minor * std::cos(eta) - major * std::sin(eta)) * (sweep >= 0.0 ? 1.0 : -1.0);
    }

    /** The least distance from @p p, refining narrow basins near
     * eccentric-ellipse vertices. */
    double distance(const RS_Vector& p, double* foot = nullptr) const {
        constexpr int samples = 4000;
        const double end = from + sweep;
        const double low = std::min(from, end);
        const double high = std::max(from, end);
        double bestAngle = from;
        double least = RS_MAXDOUBLE;
        const auto include = [&](const double angle) {
            if (angle < low || angle > high) {
                return;
            }
            const double candidate = p.distanceTo(at(angle));
            if (candidate < least) {
                least = candidate;
                bestAngle = angle;
            }
        };
        for (int i = 0; i <= samples; ++i) {
            include(from + sweep * static_cast<double>(i) / samples);
        }
        const auto refine = [&](const double center, const double radius) {
            double lo = std::max(low, center - radius);
            double hi = std::min(high, center + radius);
            for (int i = 0; i < 80 && hi - lo > 1e-13; ++i) {
                const double u = lo + (hi - lo) / 3.0;
                const double v = hi - (hi - lo) / 3.0;
                if (p.distanceTo(at(u)) < p.distanceTo(at(v))) {
                    hi = v;
                }
                else {
                    lo = u;
                }
            }
            include(0.5 * (lo + hi));
        };
        refine(bestAngle, std::abs(sweep) / samples);

        RS_Vector local = p - ellipse.getCenter();
        local.rotate(-ellipse.getAngle());
        const double a = ellipse.getMajorRadius();
        const double b = a * ellipse.getRatio();
        const double projectedAngle = std::atan2(local.y / b, local.x / a);
        for (int axisIndex = -8; axisIndex <= 8; ++axisIndex) {
            const double axis = axisIndex * M_PI_2;
            if (std::abs(std::remainder(projectedAngle - axis, 2.0 * M_PI)) > 0.02) {
                continue;
            }
            const double step = std::max(ellipse.getRatio() / 32.0, 1e-10);
            const double first = std::max(low, axis - 0.01);
            const double last = std::min(high, axis + 0.01);
            if (!(first < last)) {
                continue;
            }
            const int localSamples = std::max(1, static_cast<int>(std::ceil((last - first) / step)));
            const double previousLeast = least;
            for (int i = 0; i <= localSamples; ++i) {
                include(first + (last - first) * static_cast<double>(i) / localSamples);
            }
            if (least < previousLeast || (bestAngle >= first && bestAngle <= last)) {
                const double bracket = (last - first) / localSamples;
                refine(bestAngle, bracket);
            }
        }
        if (foot != nullptr) {
            *foot = bestAngle;
        }
        return least;
    }
};

/** A whole ellipse's curve, and an arc's, the way each runs. */
Curve curveOf(const RS_Ellipse& e) {
    if (!e.isEllipticArc()) {
        return {e, 0.0, e.isReversed() ? -2.0 * M_PI : 2.0 * M_PI};
    }
    double end = e.getAngle2();
    const double start = e.getAngle1();
    if (e.isReversed() && end > start) {
        end -= 2.0 * M_PI;
    }
    else if (!e.isReversed() && end < start) {
        end += 2.0 * M_PI;
    }
    return {e, start, end - start};
}

struct Chain {
    const RS_Spline* spline;
    double t0;
    double t1;

    RS_Vector at(const double t) const {
        LC_CurveJet jet;
        REQUIRE(spline->tryEvaluateJet(t, LC_CurveEvaluationSide::Interior, jet));
        return jet.point;
    }

    RS_Vector start() const {
        return at(t0);
    }
    RS_Vector end() const {
        return at(t1);
    }
    bool closes(const double tolerance) const {
        return start().distanceTo(end()) <= tolerance;
    }

    /** The least distance from @p p to the chain: dense samples, then a ternary search. */
    double distanceFrom(const RS_Vector& p) const {
        constexpr int samples = 2000;
        int best = 0;
        double least = RS_MAXDOUBLE;
        for (int i = 0; i <= samples; ++i) {
            const double d = p.distanceTo(at(t0 + (t1 - t0) * i / samples));
            if (d < least) {
                least = d;
                best = i;
            }
        }
        double lo = t0 + (t1 - t0) * std::max(best - 1, 0) / samples;
        double hi = t0 + (t1 - t0) * std::min(best + 1, samples) / samples;
        for (int k = 0; k < 100; ++k) {
            const double u = lo + (hi - lo) / 3.0;
            const double v = hi - (hi - lo) / 3.0;
            if (p.distanceTo(at(u)) < p.distanceTo(at(v))) {
                hi = v;
            }
            else {
                lo = u;
            }
        }
        return p.distanceTo(at(0.5 * (lo + hi)));
    }
};

struct Offset {
    LC_CurveOffsetStatus status{LC_CurveOffsetStatus::InvalidRequest};
    std::vector<std::unique_ptr<RS_Entity>> entities;
    std::vector<Chain> chains;
    LC_CurveOffsetGeometryResult::IntersectionDiagnostics intersectionDiagnostics;
    /** Twice the geometric tolerance the offset was made to: what every check allows. */
    double slack{0.0};
};

Offset offsetOf(const RS_Ellipse& e, const LC_CurveOffsetSide side, const double d, const double requestedTolerance = 0.0) {
    Offset result;
    const LC_CurveOffsetOptions options = LC_CurveOffset::makeOffsetOptions(e, d, requestedTolerance);
    result.slack = 2.0 * options.tolerance.requestedGeometry;
    LC_CurveOffsetMaterializationResult made =
        LC_CurveOffset::createEntities(e, LC_CurveOffset::makeSideRequest(side, d), options, LC_CurveOffset::makeDirectSourceBudget());
    result.status = made.status;
    result.intersectionDiagnostics = made.intersectionDiagnostics;
    result.entities = std::move(made.entities);
    for (const std::unique_ptr<RS_Entity>& entity : result.entities) {
        const auto* spline = dynamic_cast<const RS_Spline*>(entity.get());
        REQUIRE(spline != nullptr);
        Chain chain{spline, 0.0, 0.0};
        REQUIRE(spline->getParameterDomain(chain.t0, chain.t1));
        result.chains.push_back(chain);
    }
    return result;
}

RS_Vector inwardOffset(const RS_Ellipse& e, double eta, double d);

/** P1, P2, P5 and P6 of every chain, and P9: how many, and which close. */
void checkOffset(const Offset& offset, const Curve& source, const double d, const std::vector<bool>& closed,
                 const std::vector<RS_Vector>& knownFeatures = {}) {
    INFO("offset status=" << static_cast<int>(offset.status));
    REQUIRE(offset.status == LC_CurveOffsetStatus::Ok);
    REQUIRE(offset.chains.size() == closed.size());
    for (std::size_t c = 0; c < offset.chains.size(); ++c) {
        INFO("chain " << c);
        const Chain& chain = offset.chains[c];
        CHECK_FALSE(chain.spline->isClosed()); // open-typed, whether or not its ends meet
        CHECK(chain.closes(offset.slack) == closed[c]);
        constexpr int samples = 600;
        std::vector<LC_OffsetSegment> polygon;
        RS_Vector previous = chain.start();
        for (int k = 0; k <= samples; ++k) {
            // between the pieces' ends, where a trimmed corner may sit
            const double t = chain.t0 + (chain.t1 - chain.t0) * (k + 0.5) / (samples + 1);
            LC_CurveJet jet;
            REQUIRE(chain.spline->tryEvaluateJet(t, LC_CurveEvaluationSide::Interior, jet));
            double foot = 0.0;
            const double distance = source.distance(jet.point, &foot);
            INFO("at (" << jet.point.x << ", " << jet.point.y << ")");
            CHECK(distance >= d - offset.slack); // P1
            CHECK(distance <= d + offset.slack); // P2
            const bool nearFeature = std::any_of(knownFeatures.begin(), knownFeatures.end(),
                                                 [&](const RS_Vector& feature) { return feature.distanceTo(jet.point) < 1e-2 * d; });
            // a stretch within the tolerance of an end has no direction to
            // check: a tail past a cusp there is absorbed when it is that short
            const bool atEnd = jet.point.distanceTo(chain.start()) <= offset.slack || jet.point.distanceTo(chain.end()) <= offset.slack;
            if (!nearFeature && !atEnd) {
                CHECK(RS_Vector::dotP(jet.first, source.tangent(foot)) > 0.0); // P5
            }
            LC_OffsetSegment segment;
            segment.a = previous;
            segment.b = jet.point;
            polygon.push_back(segment);
            previous = jet.point;
        }
        LC_OffsetSegment last;
        last.a = previous;
        last.b = chain.end();
        polygon.push_back(last);
        // P6: no two sample segments but neighbours meet
        const std::size_t n = polygon.size();
        bool crossing = false;
        std::size_t crossingI = 0;
        std::size_t crossingJ = 0;
        for (std::size_t i = 0; i < n && !crossing; ++i) {
            for (std::size_t j = i + 2; j < n && !crossing; ++j) {
                if (closed[c] && i == 0 && j == n - 1) {
                    continue; // neighbours across the seam
                }
                if (segmentsIntersect(polygon[i], polygon[j], 0.0)) {
                    const auto confinedToKnownFeature = [&](const RS_Vector& feature) {
                        return polygon[i].a.distanceTo(feature) <= offset.slack && polygon[i].b.distanceTo(feature) <= offset.slack &&
                            polygon[j].a.distanceTo(feature) <= offset.slack && polygon[j].b.distanceTo(feature) <= offset.slack;
                    };
                    if (!std::any_of(knownFeatures.begin(), knownFeatures.end(), confinedToKnownFeature)) {
                        crossing = true;
                        crossingI = i;
                        crossingJ = j;
                    }
                }
            }
        }
        INFO("sampled crossing segments " << crossingI << " and " << crossingJ);
        if (crossing) {
            const auto& first = polygon[crossingI];
            const auto& second = polygon[crossingJ];
            CAPTURE(first.a.x, first.a.y, first.b.x, first.b.y, second.a.x, second.a.y, second.b.x, second.b.y);
        }
        CHECK_FALSE(crossing);
    }
    for (const RS_Vector& feature : knownFeatures) {
        INFO("known offset feature (" << feature.x << ", " << feature.y << ")");
        double nearest = RS_MAXDOUBLE;
        for (const Chain& chain : offset.chains) {
            nearest = std::min(nearest, chain.distanceFrom(feature));
        }
        CHECK(nearest <= offset.slack);
    }
}

/** The exact offset of the ellipse's point at @p eta, @p d towards its inside. */
RS_Vector inwardOffset(const RS_Ellipse& e, const double eta, const double d) {
    const RS_Vector major = e.getMajorP();
    const RS_Vector minor{-major.y * e.getRatio(), major.x * e.getRatio()};
    const RS_Vector tangent = (minor * std::cos(eta) - major * std::sin(eta)).normalized();
    const RS_Vector inward{-tangent.y, tangent.x};
    return e.getEllipsePoint(eta) + inward * d;
}

const LC_CurveOffsetSide kLeft = LC_CurveOffsetSide::Left;   // inside an ellipse that runs anticlockwise
const LC_CurveOffsetSide kRight = LC_CurveOffsetSide::Right; // outside it

} // namespace

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("Ellipse offsets are trimmed past b^2/a and vanish from b", "[curve-offset][ellipse]") {
    // a = 10, b = 5: the inward offset stalls at the vertices at b^2/a = 2.5,
    // cusps past it, and nothing is left from b = 5 on
    const RS_Ellipse e = ellipse(10.0, 0.5);
    const Curve source = curveOf(e);
    const double c = std::sqrt(75.0); // the focal distance
    SECTION("smooth below b^2/a") {
        for (const double d : {1.0, 2.45, 2.48}) {
            INFO("d = " << d);
            checkOffset(offsetOf(e, kLeft, d), source, d, {true});
        }
        checkOffset(offsetOf(e, kRight, 3.0), source, 3.0, {true});
    }
    SECTION("no more handles than twice the old sampler's points") {
        // #2581: the sampler made 53, 42 and 71 points of a spline through
        // points; at kEllipseRelativeOffsetTolerance, 1e-4, the engine makes
        // 100, 220 and 88 control points (at its default 1e-6: 232, 280, 232)
        const Offset one = offsetOf(e, kLeft, 1.0);
        REQUIRE(one.chains.size() == 1);
        REQUIRE(one.chains.front().spline->getNumberOfControlPoints() <= 106);
        const Offset outward = offsetOf(e, kRight, 3.0);
        REQUIRE(outward.chains.size() == 1);
        REQUIRE(outward.chains.front().spline->getNumberOfControlPoints() <= 142);
        // 0.05 short of the stall distance the offset turns sharply at the
        // vertices, and its pieces are bounded by the fit's tangent angle, not
        // the tolerance (196 even at 1e-3): twice the sampler's 42 is not met.
        // A regression guard at what is made, pending a decision on D15.
        const Offset nearStall = offsetOf(e, kLeft, 2.45);
        REQUIRE(nearStall.chains.size() == 1);
        CHECK(nearStall.chains.front().spline->getNumberOfControlPoints() <= 220);
    }
    SECTION("through the stall points at b^2/a") {
        for (const double d : {2.5, 2.5 * (1.0 - 1e-9), 2.5 * (1.0 + 1e-9)}) {
            INFO("d = " << d);
            checkOffset(offsetOf(e, kLeft, d), source, d, {true}, {{7.5, 0.0}, {-7.5, 0.0}});
        }
    }
    SECTION("past b^2/a, corners where the swallowtails are cut") {
        for (const double d : {3.0, 4.0}) {
            INFO("d = " << d);
            const double x = c * std::sqrt(1.0 - d * d / 25.0); // 6.9282 at 3, 5.1962 at 4
            checkOffset(offsetOf(e, kLeft, d), source, d, {true}, {{x, 0.0}, {-x, 0.0}});
        }
    }
    SECTION("nothing is left from b on") {
        for (const double d : {5.0, 5.0 * (1.0 - 1e-9), 6.0}) {
            INFO("d = " << d);
            const Offset offset = offsetOf(e, kLeft, d);
            CHECK(offset.status == LC_CurveOffsetStatus::Ok);
            CHECK(offset.chains.empty());
        }
    }
    SECTION("a = 5, b = 2: corners at 0.9 and 1") {
        const RS_Ellipse small = ellipse(5.0, 0.4);
        for (const double d : {0.9, 1.0}) {
            INFO("d = " << d);
            const double x = std::sqrt(21.0) * std::sqrt(1.0 - d * d / 4.0); // 4.0924 at 0.9, 3.9686 at 1
            checkOffset(offsetOf(small, kLeft, d), curveOf(small), d, {true}, {{x, 0.0}, {-x, 0.0}});
        }
    }
    SECTION("a ratio above 1: the long axis is the minor vector's") {
        // majorP (1, 0), ratio 2: 1 along x, 2 along y; its inradius is 1
        const RS_Ellipse tall = ellipse(1.0, 2.0);
        const double y = std::sqrt(3.0) * std::sqrt(1.0 - 0.49); // 1.2369
        checkOffset(offsetOf(tall, kLeft, 0.7), curveOf(tall), 0.7, {true}, {{0.0, y}, {0.0, -y}});
        const Offset none = offsetOf(tall, kLeft, 1.0);
        CHECK(none.status == LC_CurveOffsetStatus::Ok);
        CHECK(none.chains.empty());
    }
    SECTION("a 100:1 ellipse just inside b is not silently reported empty") {
        // a = 100, b = 1: shrinksAway() judged the whole ellipse shrunk away
        // from d alone being within its absolute tolerance band of b, but the
        // surviving curve's own half-extent there, sqrt(a^2-b^2)*sqrt(1-(d/b)^2),
        // is about 19.9 units at d = 0.98 - not negligible. removeLocalLoops's
        // ring path (both vertices trimmed past the cusp distance in the same
        // offset) used to starve the second loop's search room and report
        // AmbiguousTopology; fixed, it must find both corners exactly.
        const RS_Ellipse needleLike = ellipse(100.0, 0.01);
        const double x = std::sqrt(100.0 * 100.0 - 1.0) * std::sqrt(1.0 - 0.98 * 0.98); // ~19.8988
        checkOffset(offsetOf(needleLike, kLeft, 0.98), curveOf(needleLike), 0.98, {true}, {{x, 0.0}, {-x, 0.0}});
    }
    SECTION("a circle drawn as an ellipse, and a near circle") {
        const RS_Ellipse circle = ellipse(5.0, 1.0);
        const Offset tiny = offsetOf(circle, kLeft, 4.96);
        checkOffset(tiny, curveOf(circle), 4.96, {true});
        CHECK(tiny.chains.front().distanceFrom({0.04, 0.0}) <= tiny.slack); // radius 0.04
        CHECK(tiny.chains.front().distanceFrom({0.0, -0.04}) <= tiny.slack);
        const Offset none = offsetOf(circle, kLeft, 5.0);
        CHECK(none.status == LC_CurveOffsetStatus::Ok);
        CHECK(none.chains.empty());
        // just inside b: a message at worst, never garbage
        const RS_Ellipse nearCircle = ellipse(5.0, 0.999);
        const double d = 0.99 * 5.0 * 0.999;
        const Offset near = offsetOf(nearCircle, kLeft, d);
        CHECK((near.status == LC_CurveOffsetStatus::Ok || near.status == LC_CurveOffsetStatus::SingularOffset));
        if (near.status == LC_CurveOffsetStatus::Ok) {
            checkOffset(near, curveOf(nearCircle), d, {true});
        }
    }
    SECTION("the closed chain is an open-typed spline whose ends meet at a smooth point") {
        for (const double d : {1.0, 3.0}) {
            INFO("d = " << d);
            const Offset offset = offsetOf(e, kLeft, d);
            REQUIRE(offset.chains.size() == 1);
            const Chain& chain = offset.chains.front();
            CHECK_FALSE(chain.spline->isClosed());
            CHECK(chain.closes(offset.slack));
            LC_CurveJet atStart;
            LC_CurveJet atEnd;
            REQUIRE(chain.spline->tryEvaluateJet(chain.t0, LC_CurveEvaluationSide::Right, atStart));
            REQUIRE(chain.spline->tryEvaluateJet(chain.t1, LC_CurveEvaluationSide::Left, atEnd));
            // the same direction on both sides of the seam: not at a cut corner
            const double turn = std::atan2(std::abs(atStart.first.x * atEnd.first.y - atStart.first.y * atEnd.first.x),
                                           RS_Vector::dotP(atStart.first, atEnd.first));
            CHECK(turn < 1e-3);
        }
    }
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("Elliptic arcs are judged on their own sweep", "[curve-offset][ellipse]") {
    SECTION("60 to 120 degrees of a = 10, b = 5: untrimmed at 2, 3 and 10") {
        const RS_Ellipse arc = ellipse(10.0, 0.5, M_PI / 3.0, 2.0 * M_PI / 3.0);
        const std::vector<std::pair<double, RS_Vector>> rows{
            {2.0, {4.4453, 2.4086}}, {3.0, {4.1679, 1.4478}}, {10.0, {2.2265, -5.2776}}};
        for (const auto& [d, end] : rows) {
            INFO("d = " << d);
            const Offset offset = offsetOf(arc, kLeft, d);
            checkOffset(offset, curveOf(arc), d, {false});
            const Chain& chain = offset.chains.front();
            CHECK(chain.start().distanceTo(inwardOffset(arc, M_PI / 3.0, d)) <= offset.slack);
            CHECK(chain.end().distanceTo(inwardOffset(arc, 2.0 * M_PI / 3.0, d)) <= offset.slack);
            CHECK(chain.start().distanceTo(end) < 1e-4);
            CHECK(chain.end().distanceTo({-end.x, end.y}) < 1e-4);
        }
    }
    SECTION("-30 to 30 degrees of a = 10, b = 5: untrimmed at 1, a corner at 3") {
        const RS_Ellipse arc = ellipse(10.0, 0.5, -M_PI / 6.0, M_PI / 6.0);
        const Offset one = offsetOf(arc, kLeft, 1.0);
        checkOffset(one, curveOf(arc), 1.0, {false});
        CHECK(one.chains.front().start().distanceTo({8.0056, -1.7441}) < 1e-4);
        CHECK(one.chains.front().end().distanceTo({8.0056, 1.7441}) < 1e-4);
        const double x = std::sqrt(75.0) * 0.8; // 6.9282
        const Offset three = offsetOf(arc, kLeft, 3.0);
        checkOffset(three, curveOf(arc), 3.0, {false}, {{x, 0.0}});
        CHECK(three.chains.front().start().distanceTo({6.6963, -0.2322}) < 1e-4);
        CHECK(three.chains.front().end().distanceTo({6.6963, 0.2322}) < 1e-4);
    }
    SECTION("the upper half of a = 5, b = 2: no corner, the ends cut by the end caps") {
        const RS_Ellipse half = ellipse(5.0, 0.4, 0.0, M_PI);
        // where the offset leaves the circle of radius d about the arc's end
        // (5, 0), below it: (4.100123, -0.014880), (4.000885, -0.042053),
        // (3.524189, -0.268292), by bisection on the offset curve
        const std::vector<std::pair<double, RS_Vector>> rows{
            {0.9, {4.100123, -0.014880}}, {1.0, {4.000885, -0.042053}}, {1.5, {3.524189, -0.268292}}};
        for (const auto& [d, end] : rows) {
            INFO("d = " << d);
            const Offset offset = offsetOf(half, kLeft, d);
            checkOffset(offset, curveOf(half), d, {false});
            CHECK(offset.chains.front().start().distanceTo(end) < 1e-5);
            CHECK(offset.chains.front().end().distanceTo({-end.x, end.y}) < 1e-5);
            CHECK(offset.chains.front().start().distanceTo({5.0, 0.0}) == Catch::Approx(d).margin(offset.slack));
        }
    }
    SECTION("the upper half at b^2/a: a stall at each end") {
        const RS_Ellipse half = ellipse(5.0, 0.4, 0.0, M_PI);
        for (const double d : {0.8, 0.8 * (1.0 - 1e-9), 0.8 * (1.0 + 1e-9)}) {
            INFO("d = " << d);
            const Offset offset = offsetOf(half, kLeft, d);
            checkOffset(offset, curveOf(half), d, {false});
        }
    }
    SECTION("a quarter of a = 5, b = 2") {
        const RS_Ellipse quarter = ellipse(5.0, 0.4, 0.0, M_PI / 2.0);
        const Offset offset = offsetOf(quarter, kLeft, 1.0);
        checkOffset(offset, curveOf(quarter), 1.0, {false});
        CHECK(offset.chains.front().start().distanceTo({4.000885, -0.042053}) < 1e-5);
        CHECK(offset.chains.front().end().distanceTo({0.0, 1.0}) <= offset.slack);
    }
    SECTION("a small arc about the major vertex: a corner at 0.85, nothing at 1") {
        const RS_Ellipse small = ellipse(5.0, 0.4, -0.3, 0.3);
        const double x = std::sqrt(21.0) * std::sqrt(1.0 - 0.85 * 0.85 / 4.0); // 4.1481
        checkOffset(offsetOf(small, kLeft, 0.85), curveOf(small), 0.85, {false}, {{x, 0.0}});
        const Offset none = offsetOf(small, kLeft, 1.0);
        CHECK(none.status == LC_CurveOffsetStatus::Ok);
        CHECK(none.chains.empty());
    }
    SECTION("0.4 to 2.7 rad: untrimmed at 1.5, trimmed at its start only at 2.2") {
        const RS_Ellipse arc = ellipse(5.0, 0.4, 0.4, 2.7);
        const Offset untrimmed = offsetOf(arc, kLeft, 1.5);
        checkOffset(untrimmed, curveOf(arc), 1.5, {false});
        CHECK(untrimmed.chains.front().start().distanceTo(inwardOffset(arc, 0.4, 1.5)) <= untrimmed.slack);
        CHECK(untrimmed.chains.front().end().distanceTo(inwardOffset(arc, 2.7, 1.5)) <= untrimmed.slack);
        const Offset trimmed = offsetOf(arc, kLeft, 2.2);
        checkOffset(trimmed, curveOf(arc), 2.2, {false});
        CHECK(trimmed.chains.front().start().distanceTo(inwardOffset(arc, 0.4638, 2.2)) < 1e-3);
        CHECK(trimmed.chains.front().start().distanceTo(inwardOffset(arc, 0.4, 2.2)) > 2.0 * trimmed.slack);
        CHECK(trimmed.chains.front().end().distanceTo(inwardOffset(arc, 2.7, 2.2)) <= trimmed.slack);
    }
    SECTION("pi/3 to 2 pi/3 of a = 5, b = 2: untrimmed at 8, trimmed at both ends at 9.5") {
        const RS_Ellipse arc = ellipse(5.0, 0.4, M_PI / 3.0, 2.0 * M_PI / 3.0);
        const Offset untrimmed = offsetOf(arc, kLeft, 8.0);
        checkOffset(untrimmed, curveOf(arc), 8.0, {false});
        CHECK(untrimmed.chains.front().start().distanceTo(inwardOffset(arc, M_PI / 3.0, 8.0)) <= untrimmed.slack);
        CHECK(untrimmed.chains.front().end().distanceTo(inwardOffset(arc, 2.0 * M_PI / 3.0, 8.0)) <=
              untrimmed.slack);
        const Offset trimmed = offsetOf(arc, kLeft, 9.5);
        checkOffset(trimmed, curveOf(arc), 9.5, {false});
        CHECK(trimmed.chains.front().start().distanceTo(inwardOffset(arc, 1.1386, 9.5)) < 1e-3);
        CHECK(trimmed.chains.front().end().distanceTo(inwardOffset(arc, 2.0030, 9.5)) < 1e-3);
    }
    SECTION("a 7 degree arc about the minor vertex, outwards") {
        const double half = 7.0 * M_PI / 360.0;
        const RS_Ellipse arc = ellipse(5.0, 0.4, M_PI / 2.0 - half, M_PI / 2.0 + half);
        checkOffset(offsetOf(arc, kRight, 1.0), curveOf(arc), 1.0, {false});
    }
    SECTION("a reversed arc: the offset runs its way") {
        const RS_Ellipse forward = ellipse(5.0, 0.4, 0.0, M_PI);
        const RS_Ellipse reversed = ellipse(5.0, 0.4, M_PI, 0.0, true);
        // outwards: the right of the anticlockwise arc, the left of the clockwise one
        const Offset ahead = offsetOf(forward, kRight, 0.3);
        const Offset back = offsetOf(reversed, kLeft, 0.3);
        checkOffset(ahead, curveOf(forward), 0.3, {false});
        checkOffset(back, curveOf(reversed), 0.3, {false});
        CHECK(ahead.chains.front().start().distanceTo({5.3, 0.0}) <= ahead.slack);
        CHECK(back.chains.front().start().distanceTo({-5.3, 0.0}) <= back.slack);
        CHECK(back.chains.front().end().distanceTo({5.3, 0.0}) <= back.slack);
    }
}

namespace {
/** y = x^2/4 from (-4, 4) to (4, 4), its vertex at the origin. */
LC_ParabolaData bowl() {
    return LC_ParabolaData{std::array<RS_Vector, 3>{RS_Vector{-4.0, 4.0}, RS_Vector{0.0, -4.0}, RS_Vector{4.0, 4.0}}};
}

/** The exact least distance from @p p to y = x^2/4 over x in [-4, 4]. */
double distanceToBowl(const RS_Vector& p) {
    const auto at = [](const double x) { return RS_Vector{x, x * x / 4.0}; };
    constexpr int samples = 4000;
    int best = 0;
    double least = RS_MAXDOUBLE;
    for (int i = 0; i <= samples; ++i) {
        const double d = p.distanceTo(at(-4.0 + 8.0 * i / samples));
        if (d < least) {
            least = d;
            best = i;
        }
    }
    double lo = -4.0 + 8.0 * std::max(best - 1, 0) / samples;
    double hi = -4.0 + 8.0 * std::min(best + 1, samples) / samples;
    for (int k = 0; k < 200; ++k) {
        const double u = lo + (hi - lo) / 3.0;
        const double v = hi - (hi - lo) / 3.0;
        if (p.distanceTo(at(u)) < p.distanceTo(at(v))) {
            hi = v;
        }
        else {
            lo = u;
        }
    }
    return p.distanceTo(at(0.5 * (lo + hi)));
}

/** The one spline the pipeline made of @p source towards @p pick at @p d, checked at the distance. */
struct PipelineOffset {
    LC_DocumentModificationBatch ctx;
    const RS_Spline* spline = nullptr;
    double t0 = 0.0;
    double t1 = 0.0;

    ~PipelineOffset() {
        qDeleteAll(ctx.entitiesToAdd);
    }

    RS_Vector at(const double t) const {
        LC_CurveJet jet;
        REQUIRE(spline->tryEvaluateJet(t, LC_CurveEvaluationSide::Interior, jet));
        return jet.point;
    }

    double distanceFrom(const RS_Vector& p) const {
        double least = RS_MAXDOUBLE;
        for (int k = 0; k <= 20000; ++k) {
            least = std::min(least, at(t0 + (t1 - t0) * k / 20000.0).distanceTo(p));
        }
        return least;
    }
};

void offsetThroughPipeline(RS_Entity& source, const RS_Vector& pick, const double d, PipelineOffset& out) {
    RS_OffsetData data;
    data.coord = pick;
    data.distance = d;
    data.keepOriginals = true;
    const LC_OffsetBatchOutcome outcome =
        RS_Modification::offsetWithOutcome(data, {&source}, false, LC_OffsetBatchLimits{}, out.ctx);
    REQUIRE(outcome.sources.size() == 1);
    REQUIRE(outcome.sources.front().succeeded());
    REQUIRE(out.ctx.entitiesToAdd.size() == 1);
    out.spline = dynamic_cast<const RS_Spline*>(out.ctx.entitiesToAdd.front());
    REQUIRE(out.spline != nullptr);
    REQUIRE(out.spline->getParameterDomain(out.t0, out.t1));
}
} // namespace

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("A parabola shrunk past its vertex's radius of curvature has a corner on its axis",
          "[curve-offset][parabola]") {
    // inside y = x^2/4 by 3 (its vertex's radius of curvature is 2): the
    // offsets of its halves cross at (0, 3.25), 3 from (±√5, 5/4); the chain
    // runs from the offset of one end to that of the other
    LC_Parabola parabola(nullptr, bowl());
    PipelineOffset offset;
    offsetThroughPipeline(parabola, {0.0, 1.0}, 3.0, offset);
    const double slack = 2.0 * LC_CurveOffset::makeOffsetOptions(parabola, 3.0).tolerance.requestedGeometry;
    const RS_Vector rightEnd{4.0 - 6.0 / std::sqrt(5.0), 4.0 + 3.0 / std::sqrt(5.0)}; // (1.3167, 5.3416)
    CHECK(offset.at(offset.t0).distanceTo({-rightEnd.x, rightEnd.y}) <= slack);
    CHECK(offset.at(offset.t1).distanceTo(rightEnd) <= slack);
    CHECK(offset.distanceFrom({0.0, 3.25}) <= slack);
    for (int k = 0; k <= 200; ++k) {
        const RS_Vector p = offset.at(offset.t0 + (offset.t1 - offset.t0) * k / 200.0);
        CHECK(std::abs(distanceToBowl(p) - 3.0) <= slack);
    }
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("A reversed parabola is offset the way it now runs", "[curve-offset][parabola]") {
    // revertDirection() swapped its end points but left the spline data the
    // engine reads as it was, so the offset ran the old way
    LC_Parabola parabola(nullptr, bowl());
    parabola.revertDirection();
    CHECK(parabola.getStartpoint().distanceTo({4.0, 4.0}) < 1e-12);
    PipelineOffset offset;
    offsetThroughPipeline(parabola, {0.0, 1.0}, 3.0, offset);
    const double slack = 2.0 * LC_CurveOffset::makeOffsetOptions(parabola, 3.0).tolerance.requestedGeometry;
    CHECK(offset.at(offset.t0).distanceTo({4.0 - 6.0 / std::sqrt(5.0), 4.0 + 3.0 / std::sqrt(5.0)}) <= slack);
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("A parabola cannot be offset in place", "[curve-offset][parabola]") {
    // RS_Entity::offset() on a parabola moved its spline data and left the
    // parabola behind; its offset is no parabola
    LC_Parabola parabola(nullptr, bowl());
    CHECK_FALSE(parabola.offset(RS_Vector{0.0, 1.0}, 1.0));
    CHECK(parabola.getStartpoint().distanceTo({-4.0, 4.0}) < 1e-12);
    CHECK(parabola.getEndpoint().distanceTo({4.0, 4.0}) < 1e-12);
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("A hyperbola past its vertex's radius of curvature has a corner on its axis", "[curve-offset][hyperbola]") {
    // a = b = 3 (lc_curveoffset_tests.cpp): radius of curvature 3 at the
    // vertex (3, 0). Towards the focus by 4 the halves' offsets cross at
    // (5√2, 0), 4 from (5√2/2, ±√3.5); at 3 they stall there
    const LC_Hyperbola hyperbola(nullptr, LC_HyperbolaData{RS_Vector{0.0, 0.0}, RS_Vector{3.0, 0.0}, 1.0, -1.5, 1.5, false});
    const auto offsetBy = [&hyperbola](const double d) {
        return LC_CurveOffset::createEntities(hyperbola, LC_CurveOffset::makeDirectionRequest(RS_Vector{10.0, 0.0}, d),
                                              LC_CurveOffset::makeOffsetOptions(hyperbola, d),
                                              LC_CurveOffset::makeDirectSourceBudget());
    };
    const LC_CurveOffsetMaterializationResult four = offsetBy(4.0);
    REQUIRE(four.status == LC_CurveOffsetStatus::Ok);
    REQUIRE(four.entities.size() == 1);
    const auto* spline = dynamic_cast<const RS_Spline*>(four.entities.front().get());
    REQUIRE(spline != nullptr);
    double t0 = 0.0;
    double t1 = 0.0;
    REQUIRE(spline->getParameterDomain(t0, t1));
    double least = RS_MAXDOUBLE;
    for (int k = 0; k <= 20000; ++k) {
        LC_CurveJet jet;
        REQUIRE(spline->tryEvaluateJet(t0 + (t1 - t0) * k / 20000.0, LC_CurveEvaluationSide::Interior, jet));
        least = std::min(least, jet.point.distanceTo({5.0 * std::sqrt(2.0), 0.0}));
    }
    CHECK(least <= 2.0 * LC_CurveOffset::makeOffsetOptions(hyperbola, 4.0).tolerance.requestedGeometry);
    for (const double d : {3.0 * (1.0 - 1e-9), 3.0 * (1.0 + 1e-9)}) {
        INFO("d = " << d);
        const LC_CurveOffsetMaterializationResult atStall = offsetBy(d);
        CHECK(atStall.status == LC_CurveOffsetStatus::Ok);
        CHECK(atStall.entities.size() == 1);
    }
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("Thin and far ellipse offsets stay at the distance everywhere", "[curve-offset][ellipse]") {
    // a sampler lost a cap of the needle, and crossed the thin one's source
    const RS_Ellipse needle = ellipse(10.0, 0.01);
    checkOffset(offsetOf(needle, kRight, 10.0), curveOf(needle), 10.0, {true});
    const RS_Ellipse thin = ellipse(100.0, 0.03);
    checkOffset(offsetOf(thin, kRight, 0.1), curveOf(thin), 0.1, {true});
    const RS_Ellipse narrow = ellipse(10.0, 1e-4);
    checkOffset(offsetOf(narrow, kRight, 0.01), curveOf(narrow), 0.01, {true});
    const Offset erasedNarrow = offsetOf(narrow, kLeft, 0.01);
    CHECK(erasedNarrow.status == LC_CurveOffsetStatus::Ok);
    CHECK(erasedNarrow.entities.empty());
    const RS_Ellipse farNarrow = ellipse(100.0, 1e-4);
    checkOffset(offsetOf(farNarrow, kRight, 0.1), curveOf(farNarrow), 0.1, {true});
    const Offset erasedFarNarrow = offsetOf(farNarrow, kLeft, 0.1);
    CHECK(erasedFarNarrow.status == LC_CurveOffsetStatus::Ok);
    CHECK(erasedFarNarrow.entities.empty());
    const RS_Ellipse translatedNarrow{nullptr, RS_EllipseData{{1e8, -1e8}, {100.0, 0.0}, 1e-4, 0.0, 0.0, false}};
    checkOffset(offsetOf(translatedNarrow, kRight, 0.1), curveOf(translatedNarrow), 0.1, {true});
    const RS_Ellipse narrowArc = ellipse(100.0, 1e-4, 0.25, 1.5);
    checkOffset(offsetOf(narrowArc, kRight, 0.1), curveOf(narrowArc), 0.1, {false});
    const RS_Ellipse reversedNarrowArc = ellipse(100.0, 1e-4, 1.5, 0.25, true);
    checkOffset(offsetOf(reversedNarrowArc, kLeft, 0.1), curveOf(reversedNarrowArc), 0.1, {false});
    const RS_Ellipse translatedNarrowArc{
        nullptr, RS_EllipseData{{1e8, -1e8}, {100.0, 0.0}, 1e-4, 0.25, 1.5, false}};
    checkOffset(offsetOf(translatedNarrowArc, kRight, 0.1), curveOf(translatedNarrowArc), 0.1, {false});
    const RS_Vector translatedArcProbe =
        translatedNarrowArc.getEllipsePoint(0.875) + RS_Vector{1e-4 * std::cos(0.875), std::sin(0.875)}.normalized() * 0.1;
    const auto translatedArcSide =
        LC_CurveOffset::resolveSide(translatedNarrowArc, translatedArcProbe, LC_CurveOffset::makeOffsetOptions(translatedNarrowArc, 0.1));
    INFO("translated arc side status=" << static_cast<int>(translatedArcSide.status));
    CHECK(translatedArcSide.status == LC_CurveOffsetStatus::Ok);
    const RS_Ellipse translatedReversedNarrowArc{nullptr, RS_EllipseData{{1e8, -1e8}, {100.0, 0.0}, 1e-4, 1.5, 0.25, true}};
    checkOffset(offsetOf(translatedReversedNarrowArc, kLeft, 0.1), curveOf(translatedReversedNarrowArc), 0.1, {false});
    const double rotation = M_PI / 6.0;
    const RS_Vector rotatedMajor{100.0 * std::cos(rotation), 100.0 * std::sin(rotation)};
    const RS_Ellipse rotatedNarrowArc{nullptr, RS_EllipseData{{0.0, 0.0}, rotatedMajor, 1e-4, 0.25, 1.5, false}};
    checkOffset(offsetOf(rotatedNarrowArc, kRight, 0.1), curveOf(rotatedNarrowArc), 0.1, {false});
    const RS_Ellipse translatedRotatedReversedNarrowArc{
        nullptr, RS_EllipseData{{1e8, -1e8}, {50.0, 100.0 * std::sin(rotation)}, 1e-4, 1.5, 0.25, true}};
    checkOffset(offsetOf(translatedRotatedReversedNarrowArc, kLeft, 0.1), curveOf(translatedRotatedReversedNarrowArc), 0.1, {false});
    const double seamGap = 0.1;
    const RS_Ellipse shortSeamNarrowArc = ellipse(100.0, 1e-4, 2.0 * M_PI - seamGap, seamGap);
    checkOffset(offsetOf(shortSeamNarrowArc, kRight, 0.1), curveOf(shortSeamNarrowArc), 0.1, {false});
    const RS_Ellipse nearFullNarrowArc = ellipse(100.0, 1e-4, seamGap, 2.0 * M_PI - seamGap);
    checkOffset(offsetOf(nearFullNarrowArc, kRight, 0.1), curveOf(nearFullNarrowArc), 0.1, {false});
    const RS_Ellipse reversedNearFullNarrowArc = ellipse(100.0, 1e-4, 2.0 * M_PI - seamGap, seamGap, true);
    checkOffset(offsetOf(reversedNearFullNarrowArc, kLeft, 0.1), curveOf(reversedNearFullNarrowArc), 0.1, {false});
}

TEST_CASE("Extreme thin ellipse receives bounded refinement headroom", "[curve-offset][ellipse][boundary]") {
    const RS_Ellipse source = ellipse(10.0, 1e-4);
    const auto refinedOptions = LC_CurveOffset::makeDirectOptions(source, 0.01);
    CHECK(refinedOptions.maxSubdivisionDepth == LC_CurveOffset::kDefaultMaxSubdivisionDepth + 12);
    auto baseOptions = refinedOptions;
    baseOptions.maxSubdivisionDepth = LC_CurveOffset::kDefaultMaxSubdivisionDepth;
    const auto baseline = LC_CurveOffset::buildDirectBranches(source, LC_CurveOffset::makeSideRequest(kRight, 0.01), baseOptions,
                                                              LC_CurveOffset::makeDirectSourceBudget());
    CHECK(baseline.status == LC_CurveOffsetStatus::ToleranceNotMet);
    CHECK(baseline.branches.empty());

    const auto geometry = LC_CurveOffset::buildDirectBranches(source, LC_CurveOffset::makeSideRequest(kRight, 0.01), refinedOptions,
                                                              LC_CurveOffset::makeDirectSourceBudget());
    REQUIRE(geometry.status == LC_CurveOffsetStatus::Ok);
    REQUIRE(geometry.branches.size() == 1);
    CHECK(geometry.exactSamples > baseline.exactSamples);
    CHECK(geometry.exactSamples < refinedOptions.maxSamples);
    const auto materialized =
        LC_CurveOffset::materializeBranches(source, geometry, refinedOptions, LC_CurveOffset::makeDirectSourceBudget());
    REQUIRE(materialized.status == LC_CurveOffsetStatus::Ok);
    CHECK(materialized.validationLevel == LC_OffsetValidationLevel::SampledBidirectional);
    CHECK(materialized.maxObservedError <= refinedOptions.tolerance.requestedGeometry);
    REQUIRE(materialized.entities.size() == 1);
}

TEST_CASE("Extreme thin ellipse inward offsets bracket the major-vertex cusp", "[curve-offset][ellipse][cusp]") {
    const RS_Vector majorP{100.0, 0.0};
    const RS_Ellipse sources[]{ellipse(100.0, 1e-4),
                               RS_Ellipse{nullptr, RS_EllipseData{RS_Vector{100.0, -100.0}, majorP, 1e-4, 0.0, 0.0, false}}};
    constexpr double cuspRadius = 1e-6;
    for (const RS_Ellipse& source : sources) {
        for (const double distance : {0.5 * cuspRadius, 0.9 * cuspRadius, cuspRadius, 1.1 * cuspRadius, 2.0 * cuspRadius}) {
            CAPTURE(source.getCenter().x, source.getCenter().y, source.getAngle(), distance);
            const Offset inward = offsetOf(source, kLeft, distance, 0.01 * distance);
            CHECK(inward.status == LC_CurveOffsetStatus::Ok);
            checkOffset(inward, curveOf(source), distance, {true});
        }
    }
}

TEST_CASE("Rotated extreme thin ellipse cusps trim within the bounded intersection budget", "[curve-offset][ellipse][cusp]") {
    constexpr double distance = 0.5e-6;
    for (const double degrees : {0.0, 15.0, 30.0, 45.0, 60.0, 75.0, 90.0}) {
        const double angle = degrees * M_PI / 180.0;
        const RS_Vector majorP{100.0 * std::cos(angle), 100.0 * std::sin(angle)};
        const RS_Ellipse source{nullptr, RS_EllipseData{RS_Vector{0, 0}, majorP, 1e-4, 0.0, 0.0, false}};
        CAPTURE(degrees);
        CHECK(LC_CurveOffset::toleranceFor(source, distance, LC_CurveOffset::relativeOffsetTolerance(source)) == 0.1 * distance);
        const auto options = LC_CurveOffset::makeOffsetOptions(source, distance);
        const Offset inward = offsetOf(source, kLeft, distance, 0.1 * distance);
        CHECK(inward.status == LC_CurveOffsetStatus::Ok);
        checkOffset(inward, curveOf(source), distance, {true});
        CHECK(inward.intersectionDiagnostics.stage == LC_CurveOffsetGeometryResult::IntersectionDiagnostics::Stage::MaterializedOutput);
        CHECK(inward.intersectionDiagnostics.status == LC_IntersectionStatus::Ok);
        CHECK(inward.intersectionDiagnostics.work.totalBoxPairs < options.maxIntersectionPairs);
        if (degrees > 0.0 && degrees < 90.0) {
            CHECK(inward.intersectionDiagnostics.work.orientedProjectionExclusions > 0);
        }
        CHECK(inward.intersectionDiagnostics.work.totalBoxPairs ==
              inward.intersectionDiagnostics.work.candidateBoxPairs + inward.intersectionDiagnostics.work.refinementBoxPairs +
                  inward.intersectionDiagnostics.work.certificateBoxPairs);
    }
}

TEST_CASE("Translated rotated thin ellipse cusps preserve accurate trimming", "[curve-offset][ellipse][cusp]") {
    constexpr double distance = 0.5e-6;
    for (const RS_Vector center : {RS_Vector{100.0, -100.0}, RS_Vector{1e4, -1e4}}) {
        for (const double degrees : {30.0, 45.0}) {
            const double angle = degrees * M_PI / 180.0;
            const RS_Vector majorP{100.0 * std::cos(angle), 100.0 * std::sin(angle)};
            const RS_Ellipse source{nullptr, RS_EllipseData{center, majorP, 1e-4, 0.0, 0.0, false}};
            CAPTURE(center.x, center.y, degrees);
            CHECK(LC_CurveOffset::toleranceFor(source, distance, LC_CurveOffset::relativeOffsetTolerance(source)) == 0.1 * distance);
            const auto options = LC_CurveOffset::makeOffsetOptions(source, distance);
            const Offset inward = offsetOf(source, kLeft, distance, 0.1 * distance);
            CHECK(inward.status == LC_CurveOffsetStatus::Ok);
            checkOffset(inward, curveOf(source), distance, {true});
            CHECK(inward.intersectionDiagnostics.stage == LC_CurveOffsetGeometryResult::IntersectionDiagnostics::Stage::MaterializedOutput);
            CHECK(inward.intersectionDiagnostics.status == LC_IntersectionStatus::Ok);
            CHECK(inward.intersectionDiagnostics.work.totalBoxPairs < options.maxIntersectionPairs);
        }
    }
}

TEST_CASE("Rotated thin ellipse cusp threshold retains topology", "[curve-offset][ellipse][cusp]") {
    for (const double degrees : {30.0, 45.0}) {
        const double angle = degrees * M_PI / 180.0;
        const RS_Vector majorP{100.0 * std::cos(angle), 100.0 * std::sin(angle)};
        const RS_Ellipse source{nullptr, RS_EllipseData{RS_Vector{0, 0}, majorP, 1e-4, 0.0, 0.0, false}};
        for (const double distance : {0.9e-6, 1.0e-6, 1.1e-6, 2.0e-6}) {
            CAPTURE(degrees, distance);
            const Offset inward = offsetOf(source, kLeft, distance, 0.1 * distance);
            CHECK(inward.status == LC_CurveOffsetStatus::Ok);
            checkOffset(inward, curveOf(source), distance, {true});
        }
    }
}

TEST_CASE("Extreme thin elliptic arcs retain cusp-local topology", "[curve-offset][ellipse][cusp]") {
    struct Profile {
        double from;
        double to;
        bool reversed;
        double distance;
    };
    const std::array<Profile, 9> profiles{{
        {-0.2, 0.2, false, 0.5e-6},
        {-0.2, 0.2, false, 0.9e-6},
        {-0.2, 0.2, false, 1.0e-6},
        {-0.2, 0.2, false, 1.1e-6},
        {0.0, 0.2, false, 1.1e-6},
        {-0.2, 0.0, false, 1.1e-6},
        {0.2, -0.2, true, 1.1e-6},
        {0.2, -0.2, true, 0.9e-6},
        {0.2, -0.2, true, 1.0e-6},
    }};
    for (const Profile& profile : profiles) {
        const RS_Ellipse source = ellipse(100.0, 1e-4, profile.from, profile.to, profile.reversed);
        CAPTURE(profile.from, profile.to, profile.reversed, profile.distance);
        const Offset inward = offsetOf(source, profile.reversed ? kRight : kLeft, profile.distance, 0.1 * profile.distance);
        CHECK(inward.status == LC_CurveOffsetStatus::Ok);
        INFO("chains=" << inward.chains.size());
        checkOffset(inward, curveOf(source), profile.distance, {false});
    }
}

TEST_CASE("Translated and rotated thin elliptic-arc cusps remain bounded", "[curve-offset][ellipse][cusp]") {
    struct Profile {
        RS_Vector center;
        double degrees;
        double from;
        double to;
        bool reversed;
    };
    const std::array<Profile, 3> profiles{{
        {{100.0, -100.0}, 30.0, -0.2, 0.2, false},
        {{1e4, -1e4}, 45.0, -0.2, 0.2, false},
        {{100.0, -100.0}, 30.0, 0.2, -0.2, true},
    }};
    constexpr double distance = 0.5e-6;
    for (const Profile& profile : profiles) {
        const double angle = profile.degrees * M_PI / 180.0;
        const RS_Vector majorP{100.0 * std::cos(angle), 100.0 * std::sin(angle)};
        const RS_Ellipse source{nullptr, RS_EllipseData{profile.center, majorP, 1e-4, profile.from, profile.to, profile.reversed}};
        CAPTURE(profile.center.x, profile.center.y, profile.degrees, profile.from, profile.to, profile.reversed);
        const Offset inward = offsetOf(source, profile.reversed ? kRight : kLeft, distance, 0.1 * distance);
        CHECK(inward.status == LC_CurveOffsetStatus::Ok);
        checkOffset(inward, curveOf(source), distance, {false});
        CHECK(inward.intersectionDiagnostics.work.totalBoxPairs < LC_CurveOffset::makeOffsetOptions(source, distance).maxIntersectionPairs);
    }

    const RS_Ellipse unrepresentable{nullptr, RS_EllipseData{RS_Vector{1e8, -1e8}, RS_Vector{100.0, 0.0}, 1e-4, -0.2, 0.2, false}};
    CHECK(std::isnan(LC_CurveOffset::toleranceFor(unrepresentable, distance, LC_CurveOffset::relativeOffsetTolerance(unrepresentable))));
    const auto refused = LC_CurveOffset::buildDirectBranches(unrepresentable, LC_CurveOffset::makeSideRequest(kLeft, distance),
                                                             LC_CurveOffset::makeOffsetOptions(unrepresentable, distance),
                                                             LC_CurveOffset::makeDirectSourceBudget());
    CHECK(refused.status == LC_CurveOffsetStatus::InvalidRequest);
    CHECK(refused.branches.empty());
}

TEST_CASE("Transformed thin elliptic arcs retain cusp-threshold geometry", "[curve-offset][ellipse][cusp]") {
    struct Profile {
        RS_Vector center;
        double degrees;
        double from;
        double to;
        bool reversed;
        double distance;
    };
    const std::array<Profile, 12> profiles{{
        {{100.0, -100.0}, 30.0, -0.2, 0.2, false, 0.9e-6},
        {{100.0, -100.0}, 30.0, -0.2, 0.2, false, 1.0e-6},
        {{100.0, -100.0}, 30.0, -0.2, 0.2, false, 1.1e-6},
        {{1e4, -1e4}, 45.0, -0.2, 0.2, false, 0.9e-6},
        {{1e4, -1e4}, 45.0, -0.2, 0.2, false, 1.0e-6},
        {{1e4, -1e4}, 45.0, -0.2, 0.2, false, 1.1e-6},
        {{100.0, -100.0}, 30.0, 0.2, -0.2, true, 1.0e-6},
        {{1e4, -1e4}, 45.0, 0.2, -0.2, true, 0.9e-6},
        {{1e4, -1e4}, 45.0, 0.2, -0.2, true, 1.0e-6},
        {{1e4, -1e4}, 45.0, 0.2, -0.2, true, 1.1e-6},
        {{100.0, -100.0}, 30.0, 0.2, -0.2, true, 0.9e-6},
        {{100.0, -100.0}, 30.0, 0.2, -0.2, true, 1.1e-6},
    }};
    for (const Profile& profile : profiles) {
        const double angle = profile.degrees * M_PI / 180.0;
        const RS_Vector majorP{100.0 * std::cos(angle), 100.0 * std::sin(angle)};
        const RS_Ellipse source{nullptr, RS_EllipseData{profile.center, majorP, 1e-4, profile.from, profile.to, profile.reversed}};
        CAPTURE(profile.center.x, profile.center.y, profile.degrees, profile.from, profile.to, profile.reversed, profile.distance);
        const Offset inward = offsetOf(source, profile.reversed ? kRight : kLeft, profile.distance, 0.1 * profile.distance);
        CHECK(inward.status == LC_CurveOffsetStatus::Ok);
        const std::vector<RS_Vector> criticalPoint{inwardOffset(source, 0.0, profile.distance)};
        checkOffset(inward, curveOf(source), profile.distance, {false}, criticalPoint);
        CHECK(inward.intersectionDiagnostics.stage == LC_CurveOffsetGeometryResult::IntersectionDiagnostics::Stage::MaterializedOutput);
        CHECK(inward.intersectionDiagnostics.status == LC_IntersectionStatus::Ok);
    }
}

TEST_CASE("Rotated 1000-to-1 elliptic arcs retain cusp-threshold geometry", "[curve-offset][ellipse][cusp]") {
    constexpr double majorRadius = 100.0;
    constexpr double ratio = 1e-3;
    constexpr double cuspRadius = majorRadius * ratio * ratio;
    const double angle = M_PI / 6.0;
    const RS_Vector majorP{majorRadius * std::cos(angle), majorRadius * std::sin(angle)};
    const RS_Ellipse source{nullptr, RS_EllipseData{{100.0, -100.0}, majorP, ratio, -0.2, 0.2, false}};
    for (const double distance : {0.9 * cuspRadius, cuspRadius, 1.1 * cuspRadius}) {
        CAPTURE(distance, cuspRadius);
        const Offset inward = offsetOf(source, kLeft, distance, 0.1 * distance);
        CHECK(inward.status == LC_CurveOffsetStatus::Ok);
        const std::vector<RS_Vector> criticalPoint{inwardOffset(source, 0.0, distance)};
        checkOffset(inward, curveOf(source), distance, {false}, criticalPoint);
        CHECK(inward.intersectionDiagnostics.stage == LC_CurveOffsetGeometryResult::IntersectionDiagnostics::Stage::MaterializedOutput);
        CHECK(inward.intersectionDiagnostics.status == LC_IntersectionStatus::Ok);
    }
}

TEST_CASE("Rotated 100-to-1 elliptic arcs retain cusp-threshold geometry", "[curve-offset][ellipse][cusp]") {
    constexpr double majorRadius = 100.0;
    constexpr double ratio = 1e-2;
    constexpr double cuspRadius = majorRadius * ratio * ratio;
    const double angle = M_PI / 6.0;
    const RS_Vector majorP{majorRadius * std::cos(angle), majorRadius * std::sin(angle)};
    const RS_Ellipse source{nullptr, RS_EllipseData{{100.0, -100.0}, majorP, ratio, -0.2, 0.2, false}};
    for (const double distance : {0.9 * cuspRadius, cuspRadius, 1.1 * cuspRadius}) {
        CAPTURE(distance, cuspRadius);
        const Offset inward = offsetOf(source, kLeft, distance, 1e-4);
        CHECK(inward.status == LC_CurveOffsetStatus::Ok);
        const std::vector<RS_Vector> criticalPoint{inwardOffset(source, 0.0, distance)};
        checkOffset(inward, curveOf(source), distance, {false}, criticalPoint);
        CHECK(inward.intersectionDiagnostics.stage == LC_CurveOffsetGeometryResult::IntersectionDiagnostics::Stage::MaterializedOutput);
        CHECK(inward.intersectionDiagnostics.status == LC_IntersectionStatus::Ok);
        const RS_Ellipse normalizedSource{nullptr, RS_EllipseData{{0.0, 0.0}, majorP, ratio, -0.2, 0.2, false}};
        const double workerTolerance = LC_CurveOffset::toleranceFor(normalizedSource, distance, 1e-4);
        const auto workerOptions = LC_CurveOffset::makeDirectOptions(normalizedSource, distance, workerTolerance);
        const auto workerGeometry = LC_CurveOffset::buildDirectBranches(normalizedSource, LC_CurveOffset::makeSideRequest(kLeft, distance),
                                                                        workerOptions, LC_CurveOffset::makeDirectSourceBudget());
        INFO("worker-matched geometry status=" << static_cast<int>(workerGeometry.status) << ", samples=" << workerGeometry.exactSamples
                                               << ", intersection stage=" << static_cast<int>(workerGeometry.intersectionDiagnostics.stage)
                                               << ", pairs=" << workerGeometry.intersectionDiagnostics.work.totalBoxPairs);
        CHECK(workerGeometry.status == LC_CurveOffsetStatus::Ok);
        const auto workerMaterialized =
            LC_CurveOffset::materializeBranches(normalizedSource, workerGeometry, workerOptions, LC_CurveOffset::makeDirectSourceBudget());
        INFO("worker-matched materialization status=" << static_cast<int>(workerMaterialized.status)
                                                      << ", entities=" << workerMaterialized.entities.size()
                                                      << ", samples=" << workerMaterialized.sampleUnits);
        CHECK(workerMaterialized.status == LC_CurveOffsetStatus::Ok);
    }
}

TEST_CASE("Rotated 10-to-1 elliptic arcs retain cusp-threshold geometry", "[curve-offset][ellipse][cusp]") {
    constexpr double majorRadius = 100.0;
    constexpr double ratio = 1e-1;
    constexpr double cuspRadius = majorRadius * ratio * ratio;
    const double angle = M_PI / 6.0;
    const RS_Vector majorP{majorRadius * std::cos(angle), majorRadius * std::sin(angle)};
    const RS_Ellipse source{nullptr, RS_EllipseData{{100.0, -100.0}, majorP, ratio, -0.2, 0.2, false}};
    for (const double distance : {0.9 * cuspRadius, cuspRadius, 1.1 * cuspRadius}) {
        CAPTURE(distance, cuspRadius);
        const Offset inward = offsetOf(source, kLeft, distance, 1e-4);
        CHECK(inward.status == LC_CurveOffsetStatus::Ok);
        checkOffset(inward, curveOf(source), distance, {false});
        CHECK(inward.intersectionDiagnostics.stage == LC_CurveOffsetGeometryResult::IntersectionDiagnostics::Stage::MaterializedOutput);
        CHECK(inward.intersectionDiagnostics.status == LC_IntersectionStatus::Ok);
        const RS_Ellipse normalizedSource{nullptr, RS_EllipseData{{0.0, 0.0}, majorP, ratio, -0.2, 0.2, false}};
        const double workerTolerance = LC_CurveOffset::toleranceFor(normalizedSource, distance, 1e-4);
        const auto workerOptions = LC_CurveOffset::makeDirectOptions(normalizedSource, distance, workerTolerance);
        const auto workerGeometry = LC_CurveOffset::buildDirectBranches(normalizedSource, LC_CurveOffset::makeSideRequest(kLeft, distance),
                                                                        workerOptions, LC_CurveOffset::makeDirectSourceBudget());
        CHECK(workerGeometry.status == LC_CurveOffsetStatus::Ok);
        const auto workerMaterialized =
            LC_CurveOffset::materializeBranches(normalizedSource, workerGeometry, workerOptions, LC_CurveOffset::makeDirectSourceBudget());
        CHECK(workerMaterialized.status == LC_CurveOffsetStatus::Ok);
    }
}

TEST_CASE("Rotated 3-to-1 elliptic arcs retain cusp-threshold geometry", "[curve-offset][ellipse][cusp]") {
    constexpr double majorRadius = 100.0;
    constexpr double ratio = 1.0 / 3.0;
    constexpr double cuspRadius = majorRadius * ratio * ratio;
    const double angle = M_PI / 6.0;
    const RS_Vector majorP{majorRadius * std::cos(angle), majorRadius * std::sin(angle)};
    const RS_Ellipse source{nullptr, RS_EllipseData{{100.0, -100.0}, majorP, ratio, -0.2, 0.2, false}};
    for (const double distance : {0.9 * cuspRadius, cuspRadius, 1.1 * cuspRadius}) {
        CAPTURE(distance, cuspRadius);
        const Offset inward = offsetOf(source, kLeft, distance, 1e-4);
        CHECK(inward.status == LC_CurveOffsetStatus::Ok);
        checkOffset(inward, curveOf(source), distance, {false});
        CHECK(inward.intersectionDiagnostics.stage == LC_CurveOffsetGeometryResult::IntersectionDiagnostics::Stage::MaterializedOutput);
        CHECK(inward.intersectionDiagnostics.status == LC_IntersectionStatus::Ok);
    }
}

TEST_CASE("Rotated 2-to-1 elliptic arcs retain cusp-threshold geometry", "[curve-offset][ellipse][cusp]") {
    constexpr double majorRadius = 100.0;
    constexpr double ratio = 0.5;
    constexpr double cuspRadius = majorRadius * ratio * ratio;
    const double angle = M_PI / 6.0;
    const RS_Vector majorP{majorRadius * std::cos(angle), majorRadius * std::sin(angle)};
    const RS_Ellipse source{nullptr, RS_EllipseData{{100.0, -100.0}, majorP, ratio, -0.2, 0.2, false}};
    for (const double distance : {0.9 * cuspRadius, cuspRadius, 1.1 * cuspRadius}) {
        CAPTURE(distance, cuspRadius);
        const auto directOptions = LC_CurveOffset::makeDirectOptions(source, distance, 1e-4);
        const auto directGeometry = LC_CurveOffset::buildDirectBranches(source, LC_CurveOffset::makeSideRequest(kLeft, distance),
                                                                        directOptions, LC_CurveOffset::makeDirectSourceBudget());
        const Offset inward = offsetOf(source, kLeft, distance, 1e-4);
        CHECK(inward.status == LC_CurveOffsetStatus::Ok);
        if (distance > cuspRadius) {
            REQUIRE(directGeometry.status == LC_CurveOffsetStatus::Ok);
            CHECK(directGeometry.branches.size() == 3);
            CHECK(inward.entities.empty());
            CHECK(inward.chains.empty());
            CHECK(inward.intersectionDiagnostics.stage == LC_CurveOffsetGeometryResult::IntersectionDiagnostics::Stage::TrimBoundary);
            CHECK(inward.intersectionDiagnostics.status == LC_IntersectionStatus::Ok);
            continue;
        }
        checkOffset(inward, curveOf(source), distance, {false});
        CHECK(inward.intersectionDiagnostics.stage == LC_CurveOffsetGeometryResult::IntersectionDiagnostics::Stage::MaterializedOutput);
        CHECK(inward.intersectionDiagnostics.status == LC_IntersectionStatus::Ok);
    }
}

TEST_CASE("Rotated 1.5-to-1 elliptic arcs retain cusp-threshold geometry", "[curve-offset][ellipse][cusp]") {
    constexpr double majorRadius = 100.0;
    constexpr double ratio = 2.0 / 3.0;
    constexpr double cuspRadius = majorRadius * ratio * ratio;
    const double angle = M_PI / 6.0;
    const RS_Vector majorP{majorRadius * std::cos(angle), majorRadius * std::sin(angle)};
    const RS_Ellipse source{nullptr, RS_EllipseData{{100.0, -100.0}, majorP, ratio, -0.2, 0.2, false}};
    for (const double distance : {0.9 * cuspRadius, cuspRadius, 1.1 * cuspRadius}) {
        CAPTURE(distance, cuspRadius);
        const auto directOptions = LC_CurveOffset::makeDirectOptions(source, distance, 1e-4);
        const auto directGeometry = LC_CurveOffset::buildDirectBranches(source, LC_CurveOffset::makeSideRequest(kLeft, distance),
                                                                        directOptions, LC_CurveOffset::makeDirectSourceBudget());
        const Offset inward = offsetOf(source, kLeft, distance, 1e-4);
        CHECK(inward.status == LC_CurveOffsetStatus::Ok);
        if (distance > cuspRadius) {
            REQUIRE(directGeometry.status == LC_CurveOffsetStatus::Ok);
            CHECK(directGeometry.branches.size() == 1);
            CHECK(inward.entities.empty());
            CHECK(inward.chains.empty());
            CHECK(inward.intersectionDiagnostics.stage == LC_CurveOffsetGeometryResult::IntersectionDiagnostics::Stage::None);
            CHECK(inward.intersectionDiagnostics.status == LC_IntersectionStatus::Ok);
            continue;
        }
        checkOffset(inward, curveOf(source), distance, {false});
        CHECK(inward.intersectionDiagnostics.stage == LC_CurveOffsetGeometryResult::IntersectionDiagnostics::Stage::MaterializedOutput);
        CHECK(inward.intersectionDiagnostics.status == LC_IntersectionStatus::Ok);
    }
}

TEST_CASE("Rotated 1.25-to-1 elliptic arcs retain cusp-threshold geometry", "[curve-offset][ellipse][cusp]") {
    constexpr double majorRadius = 100.0;
    constexpr double ratio = 0.8;
    constexpr double cuspRadius = majorRadius * ratio * ratio;
    const double angle = M_PI / 6.0;
    const RS_Vector majorP{majorRadius * std::cos(angle), majorRadius * std::sin(angle)};
    const RS_Ellipse source{nullptr, RS_EllipseData{{100.0, -100.0}, majorP, ratio, -0.2, 0.2, false}};
    for (const double distance : {0.9 * cuspRadius, cuspRadius, 1.1 * cuspRadius}) {
        CAPTURE(distance, cuspRadius);
        const auto directOptions = LC_CurveOffset::makeDirectOptions(source, distance, 1e-4);
        const auto directGeometry = LC_CurveOffset::buildDirectBranches(source, LC_CurveOffset::makeSideRequest(kLeft, distance),
                                                                        directOptions, LC_CurveOffset::makeDirectSourceBudget());
        const Offset inward = offsetOf(source, kLeft, distance, 1e-4);
        CHECK(inward.status == LC_CurveOffsetStatus::Ok);
        if (distance > cuspRadius) {
            REQUIRE(directGeometry.status == LC_CurveOffsetStatus::Ok);
            CHECK(directGeometry.branches.size() == 1);
            CHECK(inward.entities.empty());
            CHECK(inward.chains.empty());
            CHECK(inward.intersectionDiagnostics.status == LC_IntersectionStatus::Ok);
            continue;
        }
        checkOffset(inward, curveOf(source), distance, {false});
        CHECK(inward.intersectionDiagnostics.stage == LC_CurveOffsetGeometryResult::IntersectionDiagnostics::Stage::MaterializedOutput);
        CHECK(inward.intersectionDiagnostics.status == LC_IntersectionStatus::Ok);
    }
}

TEST_CASE("Rotated 1.1-to-1 elliptic arcs retain cusp-threshold geometry", "[curve-offset][ellipse][cusp]") {
    constexpr double majorRadius = 100.0;
    constexpr double ratio = 10.0 / 11.0;
    constexpr double cuspRadius = majorRadius * ratio * ratio;
    const double angle = M_PI / 6.0;
    const RS_Vector majorP{majorRadius * std::cos(angle), majorRadius * std::sin(angle)};
    const RS_Ellipse source{nullptr, RS_EllipseData{{100.0, -100.0}, majorP, ratio, -0.2, 0.2, false}};
    for (const double distance : {0.9 * cuspRadius, cuspRadius, 1.1 * cuspRadius}) {
        CAPTURE(distance, cuspRadius);
        const auto directOptions = LC_CurveOffset::makeDirectOptions(source, distance, 1e-4);
        const auto directGeometry = LC_CurveOffset::buildDirectBranches(source, LC_CurveOffset::makeSideRequest(kLeft, distance),
                                                                        directOptions, LC_CurveOffset::makeDirectSourceBudget());
        const Offset inward = offsetOf(source, kLeft, distance, 1e-4);
        if (distance == cuspRadius) {
            CHECK(inward.status == LC_CurveOffsetStatus::LimitExceeded);
            CHECK(inward.entities.empty());
            CHECK(inward.chains.empty());
            continue;
        }
        CHECK(inward.status == LC_CurveOffsetStatus::Ok);
        if (distance > cuspRadius) {
            REQUIRE(directGeometry.status == LC_CurveOffsetStatus::Ok);
            CHECK(directGeometry.branches.size() == 1);
            CHECK(inward.entities.empty());
            CHECK(inward.chains.empty());
            CHECK(inward.intersectionDiagnostics.status == LC_IntersectionStatus::Ok);
            continue;
        }
        checkOffset(inward, curveOf(source), distance, {false});
        CHECK(inward.intersectionDiagnostics.stage == LC_CurveOffsetGeometryResult::IntersectionDiagnostics::Stage::MaterializedOutput);
        CHECK(inward.intersectionDiagnostics.status == LC_IntersectionStatus::Ok);
    }
}

TEST_CASE("Rotated 1.05-to-1 elliptic arcs retain cusp-threshold geometry", "[curve-offset][ellipse][cusp]") {
    constexpr double majorRadius = 100.0;
    constexpr double ratio = 20.0 / 21.0;
    constexpr double cuspRadius = majorRadius * ratio * ratio;
    const double angle = M_PI / 6.0;
    const RS_Vector majorP{majorRadius * std::cos(angle), majorRadius * std::sin(angle)};
    const RS_Ellipse source{nullptr, RS_EllipseData{{100.0, -100.0}, majorP, ratio, -0.2, 0.2, false}};
    for (const double distance : {0.9 * cuspRadius, cuspRadius, 1.1 * cuspRadius}) {
        CAPTURE(distance, cuspRadius);
        const auto directOptions = LC_CurveOffset::makeDirectOptions(source, distance, 1e-4);
        const auto directGeometry = LC_CurveOffset::buildDirectBranches(source, LC_CurveOffset::makeSideRequest(kLeft, distance),
                                                                        directOptions, LC_CurveOffset::makeDirectSourceBudget());
        const Offset inward = offsetOf(source, kLeft, distance, 1e-4);
        if (distance == cuspRadius) {
            CHECK(inward.status == LC_CurveOffsetStatus::LimitExceeded);
            CHECK(inward.entities.empty());
            CHECK(inward.chains.empty());
            continue;
        }
        CHECK(inward.status == LC_CurveOffsetStatus::Ok);
        if (distance > cuspRadius) {
            REQUIRE(directGeometry.status == LC_CurveOffsetStatus::Ok);
            CHECK(directGeometry.branches.size() == 1);
            CHECK(inward.entities.empty());
            CHECK(inward.chains.empty());
            CHECK(inward.intersectionDiagnostics.status == LC_IntersectionStatus::Ok);
            continue;
        }
        checkOffset(inward, curveOf(source), distance, {false});
        CHECK(inward.intersectionDiagnostics.stage == LC_CurveOffsetGeometryResult::IntersectionDiagnostics::Stage::MaterializedOutput);
        CHECK(inward.intersectionDiagnostics.status == LC_IntersectionStatus::Ok);
    }
}

TEST_CASE("Rotated 1.025-to-1 elliptic arcs retain cusp-threshold geometry", "[curve-offset][ellipse][cusp]") {
    constexpr double majorRadius = 100.0;
    constexpr double ratio = 40.0 / 41.0;
    constexpr double cuspRadius = majorRadius * ratio * ratio;
    const double angle = M_PI / 6.0;
    const RS_Vector majorP{majorRadius * std::cos(angle), majorRadius * std::sin(angle)};
    const RS_Ellipse source{nullptr, RS_EllipseData{{100.0, -100.0}, majorP, ratio, -0.2, 0.2, false}};
    for (const double distance : {0.9 * cuspRadius, cuspRadius, 1.1 * cuspRadius}) {
        CAPTURE(distance, cuspRadius);
        const auto directOptions = LC_CurveOffset::makeDirectOptions(source, distance, 1e-4);
        const auto directGeometry = LC_CurveOffset::buildDirectBranches(source, LC_CurveOffset::makeSideRequest(kLeft, distance),
                                                                        directOptions, LC_CurveOffset::makeDirectSourceBudget());
        const Offset inward = offsetOf(source, kLeft, distance, 1e-4);
        if (distance == cuspRadius) {
            CHECK(inward.status == LC_CurveOffsetStatus::LimitExceeded);
            CHECK(inward.entities.empty());
            CHECK(inward.chains.empty());
            continue;
        }
        CHECK(inward.status == LC_CurveOffsetStatus::Ok);
        if (distance > cuspRadius) {
            REQUIRE(directGeometry.status == LC_CurveOffsetStatus::Ok);
            CHECK(directGeometry.branches.size() == 1);
            CHECK(inward.entities.empty());
            CHECK(inward.chains.empty());
            CHECK(inward.intersectionDiagnostics.status == LC_IntersectionStatus::Ok);
            continue;
        }
        checkOffset(inward, curveOf(source), distance, {false});
        CHECK(inward.intersectionDiagnostics.stage == LC_CurveOffsetGeometryResult::IntersectionDiagnostics::Stage::MaterializedOutput);
        CHECK(inward.intersectionDiagnostics.status == LC_IntersectionStatus::Ok);
    }
}

TEST_CASE("Rotated 1.0125-to-1 elliptic arcs retain cusp-threshold geometry", "[curve-offset][ellipse][cusp]") {
    constexpr double majorRadius = 100.0;
    constexpr double ratio = 80.0 / 81.0;
    constexpr double cuspRadius = majorRadius * ratio * ratio;
    const double angle = M_PI / 6.0;
    const RS_Vector majorP{majorRadius * std::cos(angle), majorRadius * std::sin(angle)};
    const RS_Ellipse source{nullptr, RS_EllipseData{{100.0, -100.0}, majorP, ratio, -0.2, 0.2, false}};
    for (const double distance : {0.9 * cuspRadius, cuspRadius, 1.1 * cuspRadius}) {
        CAPTURE(distance, cuspRadius);
        const auto directOptions = LC_CurveOffset::makeDirectOptions(source, distance, 1e-4);
        const auto directGeometry = LC_CurveOffset::buildDirectBranches(source, LC_CurveOffset::makeSideRequest(kLeft, distance),
                                                                        directOptions, LC_CurveOffset::makeDirectSourceBudget());
        const Offset inward = offsetOf(source, kLeft, distance, 1e-4);
        if (distance == cuspRadius) {
            CHECK(inward.status == LC_CurveOffsetStatus::LimitExceeded);
            CHECK(inward.entities.empty());
            CHECK(inward.chains.empty());
            continue;
        }
        CHECK(inward.status == LC_CurveOffsetStatus::Ok);
        if (distance > cuspRadius) {
            REQUIRE(directGeometry.status == LC_CurveOffsetStatus::Ok);
            CHECK(directGeometry.branches.size() == 1);
            CHECK(inward.entities.empty());
            CHECK(inward.chains.empty());
            CHECK(inward.intersectionDiagnostics.status == LC_IntersectionStatus::Ok);
            continue;
        }
        checkOffset(inward, curveOf(source), distance, {false});
        CHECK(inward.intersectionDiagnostics.stage == LC_CurveOffsetGeometryResult::IntersectionDiagnostics::Stage::MaterializedOutput);
        CHECK(inward.intersectionDiagnostics.status == LC_IntersectionStatus::Ok);
    }
}

TEST_CASE("Rotated 1.00625-to-1 elliptic arcs retain cusp-threshold geometry", "[curve-offset][ellipse][cusp]") {
    constexpr double majorRadius = 100.0;
    constexpr double ratio = 160.0 / 161.0;
    constexpr double cuspRadius = majorRadius * ratio * ratio;
    const double angle = M_PI / 6.0;
    const RS_Vector majorP{majorRadius * std::cos(angle), majorRadius * std::sin(angle)};
    const RS_Ellipse source{nullptr, RS_EllipseData{{100.0, -100.0}, majorP, ratio, -0.2, 0.2, false}};
    for (const double distance : {0.9 * cuspRadius, cuspRadius, 1.1 * cuspRadius}) {
        CAPTURE(distance, cuspRadius);
        const auto directOptions = LC_CurveOffset::makeDirectOptions(source, distance, 1e-4);
        const auto directGeometry = LC_CurveOffset::buildDirectBranches(source, LC_CurveOffset::makeSideRequest(kLeft, distance),
                                                                        directOptions, LC_CurveOffset::makeDirectSourceBudget());
        const Offset inward = offsetOf(source, kLeft, distance, 1e-4);
        if (distance == cuspRadius) {
            CHECK(inward.status == LC_CurveOffsetStatus::LimitExceeded);
            CHECK(inward.entities.empty());
            CHECK(inward.chains.empty());
            continue;
        }
        CHECK(inward.status == LC_CurveOffsetStatus::Ok);
        if (distance > cuspRadius) {
            REQUIRE(directGeometry.status == LC_CurveOffsetStatus::Ok);
            CHECK(directGeometry.branches.size() == 1);
            CHECK(inward.entities.empty());
            CHECK(inward.chains.empty());
            CHECK(inward.intersectionDiagnostics.status == LC_IntersectionStatus::Ok);
            continue;
        }
        checkOffset(inward, curveOf(source), distance, {false});
        CHECK(inward.intersectionDiagnostics.stage == LC_CurveOffsetGeometryResult::IntersectionDiagnostics::Stage::MaterializedOutput);
        CHECK(inward.intersectionDiagnostics.status == LC_IntersectionStatus::Ok);
    }
}

TEST_CASE("Rotated 1.003125-to-1 elliptic arcs retain cusp-threshold geometry", "[curve-offset][ellipse][cusp]") {
    constexpr double majorRadius = 100.0;
    constexpr double ratio = 320.0 / 321.0;
    constexpr double cuspRadius = majorRadius * ratio * ratio;
    const double angle = M_PI / 6.0;
    const RS_Vector majorP{majorRadius * std::cos(angle), majorRadius * std::sin(angle)};
    const RS_Ellipse source{nullptr, RS_EllipseData{{100.0, -100.0}, majorP, ratio, -0.2, 0.2, false}};
    for (const double distance : {0.9 * cuspRadius, cuspRadius, 1.1 * cuspRadius}) {
        CAPTURE(distance, cuspRadius);
        const auto directOptions = LC_CurveOffset::makeDirectOptions(source, distance, 1e-4);
        const auto directGeometry = LC_CurveOffset::buildDirectBranches(source, LC_CurveOffset::makeSideRequest(kLeft, distance),
                                                                        directOptions, LC_CurveOffset::makeDirectSourceBudget());
        const Offset inward = offsetOf(source, kLeft, distance, 1e-4);
        if (distance == cuspRadius) {
            CHECK(inward.status == LC_CurveOffsetStatus::LimitExceeded);
            CHECK(inward.entities.empty());
            CHECK(inward.chains.empty());
            continue;
        }
        CHECK(inward.status == LC_CurveOffsetStatus::Ok);
        if (distance > cuspRadius) {
            REQUIRE(directGeometry.status == LC_CurveOffsetStatus::Ok);
            CHECK(directGeometry.branches.size() == 1);
            CHECK(inward.entities.empty());
            CHECK(inward.chains.empty());
            CHECK(inward.intersectionDiagnostics.status == LC_IntersectionStatus::Ok);
            continue;
        }
        checkOffset(inward, curveOf(source), distance, {false});
        CHECK(inward.intersectionDiagnostics.stage == LC_CurveOffsetGeometryResult::IntersectionDiagnostics::Stage::MaterializedOutput);
        CHECK(inward.intersectionDiagnostics.status == LC_IntersectionStatus::Ok);
    }
}

TEST_CASE("Rotated 1.0015625-to-1 elliptic arcs retain cusp-threshold geometry", "[curve-offset][ellipse][cusp]") {
    constexpr double majorRadius = 100.0;
    constexpr double ratio = 640.0 / 641.0;
    constexpr double cuspRadius = majorRadius * ratio * ratio;
    const double angle = M_PI / 6.0;
    const RS_Vector majorP{majorRadius * std::cos(angle), majorRadius * std::sin(angle)};
    const RS_Ellipse source{nullptr, RS_EllipseData{{100.0, -100.0}, majorP, ratio, -0.2, 0.2, false}};
    for (const double distance : {0.9 * cuspRadius, cuspRadius, 1.1 * cuspRadius}) {
        CAPTURE(distance, cuspRadius);
        const auto directOptions = LC_CurveOffset::makeDirectOptions(source, distance, 1e-4);
        const auto directGeometry = LC_CurveOffset::buildDirectBranches(source, LC_CurveOffset::makeSideRequest(kLeft, distance),
                                                                        directOptions, LC_CurveOffset::makeDirectSourceBudget());
        const Offset inward = offsetOf(source, kLeft, distance, 1e-4);
        if (distance == cuspRadius) {
            CHECK(inward.status == LC_CurveOffsetStatus::LimitExceeded);
            CHECK(inward.entities.empty());
            CHECK(inward.chains.empty());
            continue;
        }
        CHECK(inward.status == LC_CurveOffsetStatus::Ok);
        if (distance > cuspRadius) {
            REQUIRE(directGeometry.status == LC_CurveOffsetStatus::Ok);
            CHECK(directGeometry.branches.size() == 1);
            CHECK(inward.entities.empty());
            CHECK(inward.chains.empty());
            CHECK(inward.intersectionDiagnostics.status == LC_IntersectionStatus::Ok);
            continue;
        }
        checkOffset(inward, curveOf(source), distance, {false});
        CHECK(inward.intersectionDiagnostics.stage == LC_CurveOffsetGeometryResult::IntersectionDiagnostics::Stage::MaterializedOutput);
        CHECK(inward.intersectionDiagnostics.status == LC_IntersectionStatus::Ok);
    }
}

TEST_CASE("Rotated 1.00078125-to-1 elliptic arcs retain cusp-threshold geometry", "[curve-offset][ellipse][cusp]") {
    constexpr double majorRadius = 100.0;
    constexpr double ratio = 1280.0 / 1281.0;
    constexpr double cuspRadius = majorRadius * ratio * ratio;
    const double angle = M_PI / 6.0;
    const RS_Vector majorP{majorRadius * std::cos(angle), majorRadius * std::sin(angle)};
    const RS_Ellipse source{nullptr, RS_EllipseData{{100.0, -100.0}, majorP, ratio, -0.2, 0.2, false}};
    for (const double distance : {0.9 * cuspRadius, cuspRadius, 1.1 * cuspRadius}) {
        CAPTURE(distance, cuspRadius);
        const auto directOptions = LC_CurveOffset::makeDirectOptions(source, distance, 1e-4);
        const auto directGeometry = LC_CurveOffset::buildDirectBranches(source, LC_CurveOffset::makeSideRequest(kLeft, distance),
                                                                        directOptions, LC_CurveOffset::makeDirectSourceBudget());
        const Offset inward = offsetOf(source, kLeft, distance, 1e-4);
        if (distance == cuspRadius) {
            CHECK(inward.status == LC_CurveOffsetStatus::LimitExceeded);
            CHECK(inward.entities.empty());
            CHECK(inward.chains.empty());
            continue;
        }
        CHECK(inward.status == LC_CurveOffsetStatus::Ok);
        if (distance > cuspRadius) {
            REQUIRE(directGeometry.status == LC_CurveOffsetStatus::Ok);
            CHECK(directGeometry.branches.size() == 1);
            CHECK(inward.entities.empty());
            CHECK(inward.chains.empty());
            CHECK(inward.intersectionDiagnostics.status == LC_IntersectionStatus::Ok);
            continue;
        }
        checkOffset(inward, curveOf(source), distance, {false});
        CHECK(inward.intersectionDiagnostics.stage == LC_CurveOffsetGeometryResult::IntersectionDiagnostics::Stage::MaterializedOutput);
        CHECK(inward.intersectionDiagnostics.status == LC_IntersectionStatus::Ok);
    }
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("Both vertices trimmed past the cusp distance in the same offset", "[curve-offset][ellipse]") {
    // Where an eccentric ellipse's inward offset needs both major-axis
    // vertices trimmed at once, removeLocalLoops resolves the two swallowtail
    // loops in sequence; the second resolution runs through the self-referential
    // "ring" path (before == after), which used to be starved of search room by
    // the first resolution's own splice and wrongly reported AmbiguousTopology.
    // Each corner sits at the closed form sqrt(a^2-b^2)*sqrt(1-(d/b)^2).
    const std::vector<std::tuple<double, double, double>> cases{
        {20.0, 2.0, 1.8},  {20.0, 2.0, 1.9},   {20.0, 2.0, 1.99}, {100.0, 1.0, 0.98}, {10.0, 3.0, 2.95}, {10.0, 3.0, 2.998},
        {10.0, 2.5, 2.49}, {10.0, 2.5, 2.498}, {10.0, 4.0, 3.99}, {10.0, 4.0, 3.998}, {10.0, 2.0, 1.95},
    };
    for (const auto& [a, b, d] : cases) {
        INFO("a = " << a << ", b = " << b << ", d = " << d);
        const RS_Ellipse e = ellipse(a, b / a);
        const double x = std::sqrt(a * a - b * b) * std::sqrt(1.0 - (d / b) * (d / b));
        checkOffset(offsetOf(e, kLeft, d), curveOf(e), d, {true}, {{x, 0.0}, {-x, 0.0}});
    }
}

namespace {
double degrees(const double angle) {
    return angle * M_PI / 180.0;
}

/** x of the corners (x, 0) and (-x, 0) of an ellipse's inward offset trimmed past b^2/a, d below b. */
double cornerX(const double a, const double b, const double d) {
    return std::sqrt((a - b) * (a + b)) * std::sqrt(1.0 - (d / b) * (d / b));
}

/** The open chains of @p offset first, then those whose ends meet, each in its order. */
void openFirst(Offset& offset) {
    std::stable_partition(offset.chains.begin(), offset.chains.end(),
                          [&offset](const Chain& chain) { return !chain.closes(offset.slack); });
}
} // namespace

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("An elliptic arc's lens is not trimmed away with its wings", "[curve-offset][ellipse]") {
    // Missing the second corner hid the entire lens for d = 48.728 to 49.484.
    const RS_Ellipse arc = ellipse(100.0, 0.5, degrees(15.0), degrees(315.0));
    for (const double d : {48.75, 49.1823583129, 49.45}) {
        INFO("d = " << d);
        const double x = cornerX(100.0, 50.0, d); // 15.5977 at 49.1823583129
        checkOffset(offsetOf(arc, kLeft, d), curveOf(arc), d, {true}, {{x, 0.0}, {-x, 0.0}});
    }
    // from 60 to 285 degrees the swallowtail was left to noding: the same lens
    const RS_Ellipse shorter = ellipse(100.0, 0.5, degrees(60.0), degrees(285.0));
    const double d = 49.1823583129;
    const double x = cornerX(100.0, 50.0, d);
    checkOffset(offsetOf(shorter, kLeft, d), curveOf(shorter), d, {true}, {{x, 0.0}, {-x, 0.0}});
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("The wings past an elliptic arc's lens are trimmed", "[curve-offset][ellipse]") {
    // The missed corner kept hidden wings, making a 3.4-long lens 48.3 long.
    const RS_Ellipse arc = ellipse(100.0, 0.8, degrees(30.0), degrees(330.0));
    const double d = 79.992;
    const double x = cornerX(100.0, 80.0, d);
    checkOffset(offsetOf(arc, kLeft, d), curveOf(arc), d, {true}, {{x, 0.0}, {-x, 0.0}});
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("A trimmed fragment hidden anywhere is hidden throughout", "[curve-offset][ellipse]") {
    // A probe near the centre kept a fragment hidden by 1.17 near its other end.
    // Only the bottom-right offset between the endpoint circles survives.
    const RS_Ellipse arc = ellipse(100.0, 0.02, degrees(45.0), 0.0);
    const double d = 2.0;
    const Offset offset = offsetOf(arc, kLeft, d);
    checkOffset(offset, curveOf(arc), d, {false});
    const Chain& chain = offset.chains.front();
    CHECK(chain.start().distanceTo(arc.getStartpoint()) == Catch::Approx(d).margin(offset.slack)); // (72.548, 0.624)
    CHECK(chain.end().distanceTo(arc.getEndpoint()) == Catch::Approx(d).margin(offset.slack)); // (99.024, 1.746)
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("Trimmed elliptic arcs keep every part that is left", "[curve-offset][ellipse]") {
    // both failed with AmbiguousTopology: the fitted pieces crossed where
    // noding had missed a corner, as above
    SECTION("a = 100, b = 5 from 15 to 315 degrees: the lens, and the offset by the arc's start") {
        // from d = 3.53995 to 4.72925
        const RS_Ellipse arc = ellipse(100.0, 0.05, degrees(15.0), degrees(315.0));
        for (const double d : {3.6, 4.0, 4.7}) {
            INFO("d = " << d);
            const double x = cornerX(100.0, 5.0, d);
            Offset offset = offsetOf(arc, kLeft, d);
            openFirst(offset);
            checkOffset(offset, curveOf(arc), d, {false, true}, {{x, 0.0}, {-x, 0.0}});
            CHECK(offset.chains.front().start().distanceTo(inwardOffset(arc, degrees(15.0), d)) <= offset.slack);
        }
    }
    SECTION("a = 100, b = 50 from 10 to 350 degrees: the lens") {
        // from d = 48.7278 to 49.9994
        const RS_Ellipse arc = ellipse(100.0, 0.5, degrees(10.0), degrees(350.0));
        for (const double d : {48.8, 49.5, 49.99}) {
            INFO("d = " << d);
            const double x = cornerX(100.0, 50.0, d);
            checkOffset(offsetOf(arc, kLeft, d), curveOf(arc), d, {true}, {{x, 0.0}, {-x, 0.0}});
        }
    }
    SECTION("a = 100, b = 20 from 0 to 359 degrees: the lens, without the stub past its corner") {
        // A sub-tolerance stub past the right corner made the chain cross its start.
        const RS_Ellipse arc = ellipse(100.0, 0.2, 0.0, degrees(359.0));
        const double d = 4.039896572532948;
        const double x = cornerX(100.0, 20.0, d);
        checkOffset(offsetOf(arc, kLeft, d), curveOf(arc), d, {true}, {{x, 0.0}, {-x, 0.0}});
    }
    SECTION("a = 100, b = 80 from 240 to 285 degrees at 1.1 a: the offset from the start's circle") {
        // The corner rule must distinguish the fragment's own foot from the cap centre.
        const RS_Ellipse arc = ellipse(100.0, 0.8, degrees(240.0), degrees(285.0));
        const double d = 110.0;
        const Offset offset = offsetOf(arc, kLeft, d);
        checkOffset(offset, curveOf(arc), d, {false});
        CHECK(offset.chains.front().start().distanceTo(arc.getStartpoint()) == Catch::Approx(d).margin(offset.slack));
        CHECK(offset.chains.front().end().distanceTo(inwardOffset(arc, degrees(285.0), d)) <= offset.slack);
    }
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("Whole ellipses just past b^2/a or just short of b keep their offset", "[curve-offset][ellipse]") {
    // Sweep failures at their original distances: collapsed swallowtails still
    // crossed below tolerance, causing AmbiguousTopology or LimitExceeded.
    struct Case {
        double ratio;
        double rotation;
        RS_Vector centre;
        double d;
    };
    const RS_Vector origin{0.0, 0.0};
    const RS_Vector shifted{1234.5, -987.25};
    const RS_Vector distant{-50321.7, 70412.9};
    const RS_Vector remote{1000000.0, -2000000.0};
    const std::vector<Case> cases{
        {0.6, 0.0, origin, 36.0 * 1.001},
        {0.8, 0.0, origin, 64.064},
        {0.15, 0.0, origin, 2.2725},
        {0.6, 0.0, origin, 59.9994},
        {0.5, 30.0, shifted, 25.0025},
        {0.6, 30.0, shifted, 36.0036},
        {0.6, 30.0, shifted, 59.9994},
        {0.15, 30.0, shifted, 2.2725},
        {0.8, 30.0, shifted, 64.064},
        {0.9, 30.0, shifted, 81.00081},
        {0.9, 30.0, shifted, 81.0081},
        {0.95, 30.0, shifted, 90.25090250000001},
        {0.3, 137.3, distant, 9.0 * 1.001},
        {0.2, 137.3, distant, 4.004},
        {0.15, 137.3, distant, 2.2725},
        {0.6, 137.3, distant, 36.0 * 1.001},
        {0.6, 137.3, distant, 36.0036},
        {0.6, 137.3, distant, 59.9994},
        {0.7, 137.3, distant, 49.0049},
        {0.8, 137.3, distant, 64.064},
        {0.9, 137.3, distant, 81.00081},
        {0.9, 137.3, distant, 81.0081},
        {0.95, 137.3, distant, 90.259025},
        {0.95, 137.3, distant, 90.25090250000001},
        {0.25, 73.0, remote, 6.25625},
        {0.3, 73.0, remote, 9.0 * 1.001},
        {0.5, 73.0, remote, 25.025},
        {0.6, 73.0, remote, 36.0 * 1.001},
        {0.6, 73.0, remote, 36.0036},
        {0.6, 73.0, remote, 59.9994},
        {0.7, 73.0, remote, 49.0049},
        {0.7, 73.0, remote, 49.04899999999999},
        {0.8, 73.0, remote, 64.064},
        {0.8, 73.0, remote, 64.0064},
        {0.9, 73.0, remote, 81.0081},
        {0.95, 73.0, remote, 90.259025},
    };
    for (const Case& c : cases) {
        INFO("b/a = " << c.ratio << ", turned " << c.rotation << " about (" << c.centre.x << ", " << c.centre.y
                      << "), d = " << c.d);
        const RS_Vector along{std::cos(degrees(c.rotation)), std::sin(degrees(c.rotation))};
        const RS_Ellipse e{nullptr, RS_EllipseData{c.centre, along * 100.0, c.ratio, 0.0, 0.0, false}};
        const double x = cornerX(100.0, 100.0 * c.ratio, c.d);
        checkOffset(offsetOf(e, kLeft, c.d), curveOf(e), c.d, {true}, {c.centre + along * x, c.centre - along * x});
    }
    // Near a circle's radius, noding exhausted its budget at the piece joins.
    const RS_Ellipse circle = ellipse(100.0, 1.0);
    checkOffset(offsetOf(circle, kLeft, 99.99), curveOf(circle), 99.99, {true});
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("A trimmed offset runs on through the corner it is cut at", "[curve-offset][ellipse]") {
    // A cap crossing 0.026 past the corner left a hidden sliver and split the chain.
    // Reversing the source already gave one entity.
    const double d = 9.791476538117758;
    const double x = cornerX(100.0, 30.0, d);
    const RS_Ellipse arc = ellipse(100.0, 0.3, degrees(10.25), degrees(9.75));
    checkOffset(offsetOf(arc, kLeft, d), curveOf(arc), d, {false}, {{x, 0.0}, {-x, 0.0}});
    const RS_Ellipse reversed = ellipse(100.0, 0.3, degrees(9.75), degrees(10.25), true);
    checkOffset(offsetOf(reversed, kRight, d), curveOf(reversed), d, {false}, {{x, 0.0}, {-x, 0.0}});
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("A trimmed elliptic arc keeps a piece apart from its lens", "[curve-offset][ellipse]") {
    // A separate piece next to the arc's start is valid, not a hidden wing.
    const RS_Ellipse arc = ellipse(100.0, 0.2, degrees(15.0), degrees(315.0));
    const double d = 15.0;
    const double x = cornerX(100.0, 20.0, d);
    Offset offset = offsetOf(arc, kLeft, d);
    openFirst(offset);
    checkOffset(offset, curveOf(arc), d, {false, true}, {{x, 0.0}, {-x, 0.0}});
    const Chain& piece = offset.chains.front();
    CHECK(piece.start().distanceTo(inwardOffset(arc, degrees(15.0), d)) <= offset.slack);
    CHECK(piece.end().distanceTo(arc.getEndpoint()) == Catch::Approx(d).margin(offset.slack)); // (82.0199, -4.28813)
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("Nothing left of an elliptic arc's inward offset stays nothing farther in", "[curve-offset][ellipse]") {
    // A false empty result also terminates a multiple-copy series prematurely.
    const RS_Ellipse arc = ellipse(100.0, 0.5, degrees(15.0), degrees(315.0));
    for (int k = 0; k <= 24; ++k) {
        const double d = 48.05 + 0.1 * k;
        INFO("d = " << d);
        const LC_CurveOffsetMaterializationResult made =
            LC_CurveOffset::createEntities(arc, LC_CurveOffset::makeSideRequest(kLeft, d),
                                           LC_CurveOffset::makeOffsetOptions(arc, d),
                                           LC_CurveOffset::makeDirectSourceBudget());
        CHECK(made.status == LC_CurveOffsetStatus::Ok);
        CHECK(made.entities.empty() == (d > 50.0)); // a lens is left up to b
    }
    RS_Ellipse series = ellipse(100.0, 0.5, degrees(15.0), degrees(315.0));
    RS_OffsetData data;
    data.coord = RS_Vector{0.0, 0.0};
    data.distance = 0.5 * 49.1823583129;
    data.number = 2;
    data.multipleCopies = true;
    data.keepOriginals = true;
    LC_DocumentModificationBatch ctx;
    const LC_OffsetBatchOutcome outcome =
        RS_Modification::offsetWithOutcome(data, {&series}, false, LC_OffsetBatchLimits{}, ctx);
    REQUIRE(outcome.sources.size() == 1);
    CHECK(outcome.sources.front().complete());
    CHECK(ctx.entitiesToAdd.size() == 2);
    qDeleteAll(ctx.entitiesToAdd);
}
