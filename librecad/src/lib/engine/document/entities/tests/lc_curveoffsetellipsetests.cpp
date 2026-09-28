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
#include <vector>

#include "lc_curveoffset.h"
#include "lc_offsetresultcheck.h"
#include "rs_ellipse.h"
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

    /** The exact least distance from @p p, and the eta it is at: dense samples, then a ternary search. */
    double distance(const RS_Vector& p, double* foot = nullptr) const {
        constexpr int samples = 4000;
        int best = 0;
        double least = RS_MAXDOUBLE;
        for (int i = 0; i <= samples; ++i) {
            const double d = p.distanceTo(at(from + sweep * i / samples));
            if (d < least) {
                least = d;
                best = i;
            }
        }
        double lo = from + sweep * std::max(best - 1, 0) / samples;
        double hi = from + sweep * std::min(best + 1, samples) / samples;
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
        if (foot != nullptr) {
            *foot = 0.5 * (lo + hi);
        }
        return p.distanceTo(at(0.5 * (lo + hi)));
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
    /** Twice the geometric tolerance the offset was made to: what every check allows. */
    double slack{0.0};
};

Offset offsetOf(const RS_Ellipse& e, const LC_CurveOffsetSide side, const double d) {
    Offset result;
    const LC_CurveOffsetOptions options = LC_CurveOffset::makeOffsetOptions(e, d);
    result.slack = 2.0 * options.tolerance.requestedGeometry;
    LC_CurveOffsetMaterializationResult made = LC_CurveOffset::createEntities(
        e, LC_CurveOffset::makeSideRequest(side, d), options, LC_CurveOffset::makeDirectSourceBudget());
    result.status = made.status;
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

/** P1, P2, P5 and P6 of every chain, and P9: how many, and which close. */
void checkOffset(const Offset& offset, const Curve& source, const double d, const std::vector<bool>& closed,
                 const std::vector<RS_Vector>& corners = {}) {
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
            const bool nearCorner = std::any_of(corners.begin(), corners.end(), [&](const RS_Vector& corner) {
                return corner.distanceTo(jet.point) < 1e-2 * d;
            });
            // a stretch within the tolerance of an end has no direction to
            // check: a tail past a cusp there is absorbed when it is that short
            const bool atEnd = jet.point.distanceTo(chain.start()) <= offset.slack ||
                               jet.point.distanceTo(chain.end()) <= offset.slack;
            if (!nearCorner && !atEnd) {
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
        for (std::size_t i = 0; i < n && !crossing; ++i) {
            for (std::size_t j = i + 2; j < n && !crossing; ++j) {
                if (closed[c] && i == 0 && j == n - 1) {
                    continue; // neighbours across the seam
                }
                crossing = segmentsIntersect(polygon[i], polygon[j], 0.0);
            }
        }
        CHECK_FALSE(crossing);
    }
    for (const RS_Vector& corner : corners) {
        INFO("corner (" << corner.x << ", " << corner.y << ")");
        double nearest = RS_MAXDOUBLE;
        for (const Chain& chain : offset.chains) {
            nearest = std::min(nearest, chain.distanceFrom(corner));
        }
        CHECK(nearest <= offset.slack);
    }
}

/** The exact offset of the ellipse's point at @p eta, @p d towards its inside. */
RS_Vector inwardOffset(const RS_Ellipse& e, const double eta, const double d) {
    const double a = e.getMajorRadius();
    const double b = e.getMinorRadius();
    const RS_Vector normal = RS_Vector{b * std::cos(eta), a * std::sin(eta)}.normalized(); // axis-aligned only
    return e.getEllipsePoint(eta) - normal * d;
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

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("Thin and far ellipse offsets stay at the distance everywhere", "[curve-offset][ellipse]") {
    // a sampler lost a cap of the needle, and crossed the thin one's source
    const RS_Ellipse needle = ellipse(10.0, 0.01);
    checkOffset(offsetOf(needle, kRight, 10.0), curveOf(needle), 10.0, {true});
    const RS_Ellipse thin = ellipse(100.0, 0.03);
    checkOffset(offsetOf(thin, kRight, 0.1), curveOf(thin), 0.1, {true});
}
