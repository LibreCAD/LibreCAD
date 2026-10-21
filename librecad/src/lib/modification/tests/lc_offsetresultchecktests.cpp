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

// LC_OffsetResultCheck: the exact segment geometry, and the verdicts of the
// gate Modify > Offset applies to what RS_Polyline::offset() makes. Each
// legacy result is made with RS_Polyline::offset() on a copy, as the gate
// sees it.

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <memory>
#include <vector>

#include "lc_offsetresultcheck.h"
#include "rs_arc.h"
#include "rs_line.h"
#include "rs_math.h"
#include "rs_polyline.h"

namespace {

/** A vertex and the bulge of the segment that starts there. */
struct Vertex {
    double x;
    double y;
    double bulge = 0.0;
};

std::unique_ptr<RS_Polyline> polyline(const std::vector<Vertex>& vertices, const bool closed) {
    auto p = std::make_unique<RS_Polyline>(nullptr);
    for (const Vertex& v : vertices) {
        p->addVertex(RS_Vector{v.x, v.y}, v.bulge);
    }
    if (closed) {
        p->setClosed(true);
        p->endPolyline(); // adds the closing segment
    }
    return p;
}

struct Legacy {
    std::unique_ptr<RS_Polyline> offset;
    LC_OffsetCheckReport report;
};

/** RS_Polyline::offset() on a copy of @p source, and the check of what it made. */
Legacy legacyOffset(const RS_Polyline& source, const RS_Vector& pick, const double distance) {
    Legacy legacy;
    legacy.offset.reset(static_cast<RS_Polyline*>(source.clone()));
    REQUIRE(legacy.offset->offset(pick, distance)); // it always reports success
    legacy.report = checkLegacyPolylineOffset(source, *legacy.offset, distance);
    return legacy;
}

LC_OffsetCheckVerdict verdictOf(const RS_Polyline& source, const RS_Vector& pick, const double distance) {
    return legacyOffset(source, pick, distance).report.verdict;
}

/** The vertices of a polyline in order, the last as well. */
std::vector<RS_Vector> verticesOf(const RS_Polyline& p) {
    std::vector<RS_Vector> result{p.getStartpoint()};
    for (const RS_Entity* segment : p) {
        result.push_back(segment->getEndpoint());
    }
    return result;
}

LC_OffsetSegment line(const RS_Vector& a, const RS_Vector& b) {
    LC_OffsetSegment s;
    s.a = a;
    s.b = b;
    return s;
}

LC_OffsetSegment arc(const RS_Vector& centre, const double radius, const double start, const double sweep) {
    LC_OffsetSegment s;
    s.arc = true;
    s.centre = centre;
    s.radius = radius;
    s.startAngle = start;
    s.sweep = sweep;
    s.a = centre + RS_Vector{std::cos(start), std::sin(start)} * radius;
    s.b = centre + RS_Vector{std::cos(start + sweep), std::sin(start + sweep)} * radius;
    return s;
}

// the measured matrix's polylines
std::unique_ptr<RS_Polyline> rectangle() {
    return polyline({{0, 0}, {10, 0}, {10, 4}, {0, 4}}, true); // M-P1
}
std::unique_ptr<RS_Polyline> openU() {
    return polyline({{0, 5}, {0, 0}, {1, 0}, {1, 5}}, false); // M-P2
}
std::unique_ptr<RS_Polyline> semicircleU() {
    return polyline({{0, 0}, {4, 0, 1}, {4, 4}, {0, 4}}, false); // M-P5: a half circle of radius 2 about (4, 2)
}
std::unique_ptr<RS_Polyline> dumbbell() {
    // M-P6: two 4 x 4 squares joined by a neck 1 high
    return polyline({{0, 0}, {4, 0}, {4, 1.5}, {8, 1.5}, {8, 0}, {12, 0}, {12, 4}, {8, 4}, {8, 2.5}, {4, 2.5},
                     {4, 4}, {0, 4}},
                    true);
}
std::unique_ptr<RS_Polyline> bowTie() {
    return polyline({{0, 0}, {10, 10}, {10, 0}, {0, 10}}, true); // M-P7
}
std::unique_ptr<RS_Polyline> zigzag() {
    return polyline({{0, 0}, {1, 2}, {2, 0}, {3, 2}, {4, 0}, {5, 2}, {6, 0}}, false); // M-P9
}
std::unique_ptr<RS_Polyline> twoArcCircle() {
    return polyline({{0, 0, 1}, {4, 0, 1}}, true); // M-P11: a circle of radius 2 about (2, 0) as two bulges
}
std::unique_ptr<RS_Polyline> filletL() {
    // (0,0)-(9,0), a quarter circle of radius 1 about (9, 1) to (10, 1), then (10, 10)
    return polyline({{0, 0}, {9, 0, std::tan(M_PI / 8.0)}, {10, 1}, {10, 10}}, false);
}

} // namespace

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("Closed-form distances between lines and arcs", "[offset][check]") {
    const LC_OffsetSegment horizontal = line({0, 0}, {10, 0});
    CHECK(pointSegmentDistance({5, 3}, horizontal) == Catch::Approx(3.0));
    CHECK(pointSegmentDistance({-3, 4}, horizontal) == Catch::Approx(5.0));

    // a quarter arc of radius 5 about the origin, 0 to 90 degrees
    const LC_OffsetSegment quarter = arc({0, 0}, 5.0, 0.0, M_PI_2);
    CHECK(pointSegmentDistance({3, 4}, quarter) == Catch::Approx(0.0).margin(1e-12)); // on it
    CHECK(pointSegmentDistance({6, 8}, quarter) == Catch::Approx(5.0));               // radially outside
    CHECK(pointSegmentDistance({0, 0}, quarter) == Catch::Approx(5.0));               // its centre
    CHECK(pointSegmentDistance({5, -3}, quarter) == Catch::Approx(3.0));              // past its start: to (5, 0)
    // the same arc the other way round, clockwise from 90 degrees
    const LC_OffsetSegment clockwise = quarter.reversed();
    CHECK(pointSegmentDistance({6, 8}, clockwise) == Catch::Approx(5.0));
    CHECK(pointSegmentDistance({-3, 5}, clockwise) == Catch::Approx(3.0));

    // segment to segment: 0 where they cross, else the least end distance
    CHECK(segmentDistance(horizontal, line({5, -1}, {5, 1})) == 0.0);
    CHECK(segmentDistance(horizontal, line({12, 1}, {15, 5})) == Catch::Approx(std::sqrt(5.0)));
    // segment to arc: along the foot of the centre when it is inside both
    CHECK(segmentDistance(line({-10, 8}, {10, 8}), quarter) == Catch::Approx(3.0));
    CHECK(segmentDistance(line({-10, 3}, {10, 3}), quarter) == 0.0);
    // a segment on a line through the centre, but short of it: from its end to the arc's
    CHECK(segmentDistance(line({-1, -1}, {-2, -2}), quarter) == Catch::Approx(std::sqrt(37.0)));
    // arc to arc: on the line of the centres; concentric with shared directions, the radii's difference
    CHECK(segmentDistance(quarter, arc({20, 0}, 5.0, M_PI_2, M_PI)) == Catch::Approx(10.0));
    CHECK(segmentDistance(quarter, arc({0, 0}, 2.0, M_PI_4, M_PI)) == Catch::Approx(3.0));
    CHECK(segmentDistance(quarter, arc({0, 0}, 2.0, M_PI, M_PI_4)) == Catch::Approx(std::sqrt(29.0))); // (0,5) to (-2,0)
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("Closed-form intersections between lines and arcs", "[offset][check]") {
    const LC_OffsetSegment horizontal = line({0, 0}, {10, 0});
    CHECK(segmentIntersections(horizontal, line({5, -1}, {5, 1}), 0.0).size() == 1);
    CHECK_FALSE(segmentsIntersect(horizontal, line({5, 0.5}, {5, 1}), 0.0));
    // collinear: the two ends of the shared stretch
    const std::vector<RS_Vector> shared = segmentIntersections(horizontal, line({12, 0}, {4, 0}), 0.0);
    REQUIRE(shared.size() == 2);
    CHECK(shared[0].distanceTo({4, 0}) < 1e-12);
    CHECK(shared[1].distanceTo({10, 0}) < 1e-12);

    const LC_OffsetSegment quarter = arc({0, 0}, 5.0, 0.0, M_PI_2);
    // a line through the arc's circle meets the arc once, at (3, 4); its other root is off the sweep
    const std::vector<RS_Vector> once = segmentIntersections(line({-10, 4}, {10, 4}), quarter, 0.0);
    REQUIRE(once.size() == 1);
    CHECK(once[0].distanceTo({3, 4}) < 1e-12);
    // tangent to the circle at (0, 5), the arc's end
    CHECK(segmentIntersections(line({-1, 5}, {1, 5}), quarter, 1e-9).size() == 1);
    // two arcs of circles that cross twice, once within both sweeps
    const std::vector<RS_Vector> crossing = segmentIntersections(quarter, arc({6, 0}, 5.0, M_PI_2, M_PI), 0.0);
    REQUIRE(crossing.size() == 1);
    CHECK(crossing[0].distanceTo({3, 4}) < 1e-12);
    // arcs of one circle meet where their sweeps overlap
    CHECK(segmentIntersections(quarter, arc({0, 0}, 5.0, M_PI_4, M_PI), 0.0).size() == 2);
    CHECK_FALSE(segmentsIntersect(quarter, arc({0, 0}, 5.0, M_PI, M_PI_4), 0.0));
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("The source is oriented head to tail without being changed", "[offset][check]") {
    // the first segment is stored end first, the arc the wrong way round
    auto p = std::make_unique<RS_Polyline>(nullptr);
    auto* first = new RS_Line(p.get(), RS_LineData{{10, 0}, {0, 0}});
    auto* second = new RS_Arc(p.get(), RS_ArcData{{10, 2}, 2.0, M_PI_2, -M_PI_2, true}); // (10,4) to (10,0), clockwise
    p->RS_EntityContainer::addEntity(first);
    p->RS_EntityContainer::addEntity(second);
    const std::vector<LC_OffsetSegment> oriented = orientedSegments(*p);
    REQUIRE(oriented.size() == 2);
    CHECK(oriented[0].a.distanceTo({0, 0}) < 1e-12);
    CHECK(oriented[0].b.distanceTo({10, 0}) < 1e-12);
    CHECK(oriented[1].a.distanceTo({10, 0}) < 1e-12);
    CHECK(oriented[1].b.distanceTo({10, 4}) < 1e-12);
    CHECK(oriented[1].sweep == Catch::Approx(M_PI)); // now anticlockwise
    CHECK(first->getStartpoint() == RS_Vector(10, 0)); // untouched
    CHECK(second->isReversed());
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("Legacy polyline offsets get the verdicts of the measured matrix", "[offset][check]") {
    using Verdict = LC_OffsetCheckVerdict;
    SECTION("M-P1, the 10 x 4 rectangle") {
        const auto source = rectangle();
        const Legacy one = legacyOffset(*source, {5, 1.5}, 1.0);
        CHECK(one.report.verdict == Verdict::Valid);
        CHECK(one.report.nearest == Catch::Approx(1.0));
        // a legitimate sliver 2e-7 wide, the zero-area doubled loop, the inverted sliver, the inverted 4 x 2
        for (const double d : {1.9999999, 2.0, 2.0000001, 3.0}) {
            INFO("d = " << d);
            const Legacy shrunk = legacyOffset(*source, {5, 1.5}, d);
            CHECK(shrunk.report.verdict == Verdict::NothingLeft);
            CHECK(shrunk.report.collapsed);
        }
        // M-P1b: picked at its centre
        for (const double d : {2.0, 2.0000001, 3.0}) {
            INFO("d = " << d);
            CHECK(verdictOf(*source, {5, 2}, d) == Verdict::NothingLeft);
        }
    }
    SECTION("M-P2, the open U") {
        const auto source = openU();
        CHECK(verdictOf(*source, {0.5, 2.5}, 0.4) == Verdict::Valid);
        const Legacy doubled = legacyOffset(*source, {0.5, 2.5}, 0.5); // a zero-length bottom
        CHECK(doubled.report.verdict == Verdict::Invalid);
        CHECK(doubled.report.broken);
        const Legacy swapped = legacyOffset(*source, {0.5, 2.5}, 0.6); // the arms swapped
        CHECK(swapped.report.verdict == Verdict::NothingLeft);
        CHECK(swapped.report.entirelyNear);
    }
    SECTION("M-P3, M-P4 and M-P10: open and closed corners") {
        const auto l = polyline({{0, 10}, {0, 0}, {10, 0}}, false);
        CHECK(verdictOf(*l, {-1, -1}, 1.0) == Verdict::Valid);
        CHECK(verdictOf(*l, {1, 1}, 1.0) == Verdict::Valid);
        const auto v = polyline({{0, 0}, {5, 10}, {10, 0}}, false);
        CHECK(verdictOf(*v, {5, 5}, 1.0) == Verdict::Valid);
        CHECK(verdictOf(*v, {5, 5}, 3.0) == Verdict::Valid);
        const auto square = polyline({{0, 0}, {10, 0}, {10, 10}, {0, 10}}, true);
        CHECK(verdictOf(*square, {-0.5, 5}, 1.0) == Verdict::Valid);
    }
    SECTION("M-P5, the semicircle U") {
        const auto source = semicircleU();
        CHECK(verdictOf(*source, {4.5, 2}, 1.0) == Verdict::Valid);
        // the arc left as the source's whole circle, crossed by the lines
        const Legacy wrapped = legacyOffset(*source, {4.5, 2}, 2.0);
        CHECK(wrapped.report.verdict == Verdict::Invalid);
        CHECK(wrapped.report.reversed);
        CHECK(wrapped.report.near);
        CHECK(verdictOf(*source, {4, 2}, 2.0) == Verdict::Invalid); // M-P5c, picked at the arc's centre
        const Legacy past = legacyOffset(*source, {4.5, 2}, 3.0);   // the arc not offset, the lines crossing it
        CHECK(past.report.verdict == Verdict::NothingLeft);
        CHECK(past.report.entirelyNear);
    }
    SECTION("M-P6, the dumbbell") {
        const auto source = dumbbell();
        const Legacy ring = legacyOffset(*source, {1, 2}, 0.4);
        CHECK(ring.report.verdict == Verdict::Valid);
        CHECK(ring.offset->count() == 12);
        const Legacy loop = legacyOffset(*source, {1, 2}, 0.6); // one loop, four crossings at the neck
        CHECK(loop.report.verdict == Verdict::Invalid);
        CHECK(loop.report.crossing);
        CHECK_FALSE(loop.report.collapsed);
    }
    SECTION("M-P7, the bow tie") {
        const Legacy result = legacyOffset(*bowTie(), {9, 5}, 1.0);
        CHECK(result.report.verdict == Verdict::Invalid);
        CHECK(result.report.nearest == 0.0); // it crosses the source
        CHECK(result.report.near);
        CHECK(result.report.crossing);
        CHECK_FALSE(result.report.collapsed); // the source crosses itself: no area test
    }
    SECTION("M-P8, a repeated vertex") {
        const Legacy result = legacyOffset(*polyline({{0, 0}, {5, 0}, {5, 0}, {5, 5}}, false), {4, 1}, 1.0);
        CHECK(result.report.verdict == Verdict::Invalid);
        CHECK(result.report.broken); // two gaps and a zero-length segment
    }
    SECTION("M-P9, the zigzag") {
        const auto source = zigzag();
        // above it, although (3, -0.5) is below: a tie between two segments, not fixed until the engine offsets polylines
        const Legacy above = legacyOffset(*source, {3, -0.5}, 0.8);
        CHECK(above.report.verdict == Verdict::Valid);
        CHECK(above.offset->getStartpoint().y > 0.0);
        CHECK(verdictOf(*source, {3, 1}, 0.8) == Verdict::Valid);
        const Legacy stubs = legacyOffset(*source, {3, 1}, 1.2); // its end stubs run backwards
        CHECK(stubs.report.verdict == Verdict::Invalid);
        CHECK(stubs.report.reversed);
    }
    SECTION("M-P11, the two-arc circle") {
        const auto source = twoArcCircle();
        CHECK(verdictOf(*source, {2, 0.5}, 1.0) == Verdict::Valid);
        for (const double d : {2.0, 3.0}) {
            INFO("d = " << d);
            const Legacy unchanged = legacyOffset(*source, {2, 0.5}, d); // the source itself
            CHECK(unchanged.report.verdict == Verdict::NothingLeft);
            CHECK(unchanged.report.entirelyNear);
        }
    }
    SECTION("M-M1: the rectangle's copies at 1, 2 and 3") {
        const auto source = rectangle();
        CHECK(verdictOf(*source, {5, 1.5}, 1.0) == Verdict::Valid);
        CHECK(verdictOf(*source, {5, 1.5}, 2.0) == Verdict::NothingLeft);
        CHECK(verdictOf(*source, {5, 1.5}, 3.0) == Verdict::NothingLeft);
    }
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("Every verdict of the legacy check has a reason", "[offset][check]") {
    using Verdict = LC_OffsetCheckVerdict;
    SECTION("the bulged chamfer shrunk by 2: its arc wraps to 346.7 degrees") {
        const auto source = polyline({{0, 0}, {10, 0, 0.1}, {11, 1}, {11, 10}}, false);
        const Legacy result = legacyOffset(*source, {5, 1}, 2.0);
        CHECK(result.report.verdict == Verdict::Invalid);
        CHECK(result.report.reversed);
        const auto* wrapped = static_cast<const RS_Arc*>(result.offset->entityAt(1));
        CHECK(RS_Math::rad2deg(wrapped->getAngleLength()) == Catch::Approx(346.7).margin(0.1));
    }
    SECTION("the fillet L: its arc does not move, or grows on the other side") {
        const auto source = filletL();
        // 1.5 is past the fillet's radius: the arc stays where it is
        const Legacy still = legacyOffset(*source, {5, 0.5}, 1.5);
        CHECK(still.report.verdict == Verdict::Invalid);
        CHECK(still.report.reversed);
        // at 3 it grows to radius 4 away from the corner, across the source
        const Legacy across = legacyOffset(*source, {5, 0.5}, 3.0);
        CHECK(across.report.verdict == Verdict::Invalid);
        CHECK(across.report.near);
        CHECK(across.report.nearest == 0.0);
        // within the radius it is a proper offset
        CHECK(verdictOf(*source, {5, 0.5}, 0.5) == Verdict::Valid);
    }
    SECTION("the slot grown by 2: its notch walls leave a gap") {
        const auto source =
            polyline({{0, 0}, {20, 0}, {20, 10}, {11, 10}, {11, 4}, {9, 4}, {9, 10}, {0, 10}}, true);
        const Legacy result = legacyOffset(*source, {-1, 5}, 2.0);
        CHECK(result.report.verdict == Verdict::Invalid);
        CHECK(result.report.broken);
    }
    SECTION("the hairpin: its two offsets lie 2d apart") {
        const Legacy result = legacyOffset(*polyline({{0, 0}, {10, 0}, {5, 0}}, false), {5, 1}, 1.0);
        CHECK(result.report.verdict == Verdict::Invalid);
        CHECK(result.report.broken);
    }
    SECTION("the acute corner shrunk by 2: reversed past its start") {
        const Legacy result = legacyOffset(*polyline({{0, 0}, {10, 0}, {2, 3}}, false), {5, 0.5}, 2.0);
        CHECK(result.report.verdict == Verdict::Invalid);
        CHECK(result.report.reversed);
    }
    SECTION("the chamfer shrunk by 2: the chamfer's offset runs backwards") {
        const Legacy result = legacyOffset(*polyline({{0, 0}, {10, 0}, {11, 1}, {11, 10}}, false), {5, 1}, 2.0);
        CHECK(result.report.verdict == Verdict::Invalid);
        CHECK(result.report.reversed);
    }
    SECTION("the triangle shrunk past its inradius: inverted, nothing left") {
        const Legacy result = legacyOffset(*polyline({{0, 0}, {10, 0}, {5, 8}}, true), {5, 2}, 4.0);
        CHECK(result.report.verdict == Verdict::NothingLeft);
        CHECK(result.report.reversed);
    }
    SECTION("the L notch with a concave fillet grown by 2: the fillet does not move") {
        const auto source = polyline({{0, 0}, {20, 0}, {20, 10}, {11, 10, -std::tan(M_PI / 8.0)}, {10, 11}, {10, 20},
                                      {0, 20}},
                                     true);
        const Legacy result = legacyOffset(*source, {-1, 5}, 2.0);
        CHECK(result.report.verdict == Verdict::Invalid);
        CHECK(result.report.reversed);
    }
    SECTION("the semicircle U scaled by 1000, offset by 1e-4: a tiny offset is still an offset") {
        const auto source = polyline({{0, 0}, {4000, 0, 1}, {4000, 4000}, {0, 4000}}, false);
        const Legacy result = legacyOffset(*source, {4500, 2000}, 1e-4);
        CHECK(result.report.verdict == Verdict::Valid);
    }
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("Each criterion of the legacy check alone makes an offset invalid", "[offset][check]") {
    using Verdict = LC_OffsetCheckVerdict;
    const auto l = polyline({{0, 10}, {0, 0}, {10, 0}}, false);
    const double t = 1e-6 * std::hypot(10.0, 10.0); // the check's tolerance for it at d = 1
    SECTION("broken only: a gap of 10t at the corner") {
        auto gapped = std::make_unique<RS_Polyline>(nullptr);
        gapped->RS_EntityContainer::addEntity(new RS_Line(gapped.get(), RS_LineData{{-1, 10}, {-1, -1}}));
        gapped->RS_EntityContainer::addEntity(new RS_Line(gapped.get(), RS_LineData{{-1 + 10 * t, -1}, {10, -1}}));
        const LC_OffsetCheckReport report = checkLegacyPolylineOffset(*l, *gapped, 1.0);
        CHECK(report.verdict == Verdict::Invalid);
        CHECK(report.broken);
        CHECK_FALSE(report.reversed);
        CHECK_FALSE(report.crossing);
        CHECK_FALSE(report.near);
    }
    SECTION("crossing only: a result that crosses itself far from the source") {
        const auto source = polyline({{0, 0}, {10, 0}, {0, 5}, {10, 5}}, false);
        const auto offset = polyline({{0, -20}, {10, -20}, {0, -10}, {10, -30}}, false);
        const LC_OffsetCheckReport report = checkLegacyPolylineOffset(*source, *offset, 1.0);
        CHECK(report.verdict == Verdict::Invalid);
        CHECK(report.crossing);
        CHECK_FALSE(report.broken);
        CHECK_FALSE(report.reversed);
        CHECK_FALSE(report.near);
        CHECK(report.nearest > 1.0);
    }
    SECTION("near only: a result passing within d - 2t of a source vertex") {
        const auto source = polyline({{0, 0}, {10, 0}, {20, 0}}, false);
        const double tolerance = 1e-6 * 20.0;
        const auto offset = polyline({{0, 1}, {10, 1 - 2 * tolerance}, {20, 1}}, false);
        const LC_OffsetCheckReport report = checkLegacyPolylineOffset(*source, *offset, 1.0);
        CHECK(report.verdict == Verdict::Invalid);
        CHECK(report.near);
        CHECK(report.nearest == Catch::Approx(1.0 - 2 * tolerance).epsilon(1e-12));
        CHECK_FALSE(report.broken);
        CHECK_FALSE(report.reversed);
        CHECK_FALSE(report.crossing);
        CHECK_FALSE(report.entirelyNear);
    }
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("The legacy offsets the polyline tests of Modify Offset expect are valid", "[offset][check]") {
    // lc_action_modify_offset_tests.cpp, every test that makes a polyline: a
    // gate that refused one of them would change what the tool does today
    struct Case {
        const char* name;
        std::vector<Vertex> vertices;
        bool closed;
        RS_Vector pick;
        std::vector<RS_Vector> expected; // empty: not pinned by that test
    };
    const std::vector<Case> cases{
        {"two selected segments", {{10, 0}, {10, 10}, {20, 10}}, false, {12, 5}, {{11, 0}, {11, 9}, {20, 9}}},
        {"segments across the seam", {{0, 10}, {0, 0}, {10, 0}}, false, {-5, -5}, {{-1, 10}, {-1, -1}, {10, -1}}},
        {"every segment of the square", {{0, 0}, {10, 0}, {10, 10}, {0, 10}}, true, {-5, 5}, {}},
        {"a line and a half circle", {{0, 0}, {10, 0, 1.0}, {20, 0}}, false, {15, -7}, {}},
        {"the polyline with its segment", {{0, 0}, {10, 0}, {10, 10}}, false, {12, 5}, {}},
        {"starts and ends where its segments do",
         {{0, 0}, {10, 0}, {10, 10}, {20, 10}, {20, 20}},
         false,
         {12, 5},
         {{0, -1}, {11, -1}, {11, 9}, {21, 9}, {21, 20}}},
        {"holds its segments", {{0, 0}, {10, 0}, {10, 10}, {20, 10}}, false, {12, 5}, {}},
    };
    for (const Case& c : cases) {
        INFO(c.name);
        const auto source = polyline(c.vertices, c.closed);
        const Legacy legacy = legacyOffset(*source, c.pick, 1.0);
        CHECK(legacy.report.verdict == LC_OffsetCheckVerdict::Valid);
        CHECK(legacy.report.nearest == Catch::Approx(1.0));
        if (!c.expected.empty()) {
            const std::vector<RS_Vector> vertices = verticesOf(*legacy.offset);
            REQUIRE(vertices.size() == c.expected.size());
            for (std::size_t i = 0; i < vertices.size(); ++i) {
                CHECK(vertices[i].distanceTo(c.expected[i]) < 1e-9);
            }
        }
    }
    // the square's box and the half circle's radius, as those tests pin them
    const Legacy square = legacyOffset(*polyline({{0, 0}, {10, 0}, {10, 10}, {0, 10}}, true), {-5, 5}, 1.0);
    CHECK(square.offset->getMin().distanceTo({-1, -1}) < 1e-9);
    CHECK(square.offset->getMax().distanceTo({11, 11}) < 1e-9);
    const Legacy half = legacyOffset(*polyline({{0, 0}, {10, 0, 1.0}, {20, 0}}, false), {15, -7}, 1.0);
    REQUIRE(half.offset->entityAt(1)->rtti() == RS2::EntityArc);
    CHECK(static_cast<const RS_Arc*>(half.offset->entityAt(1))->getRadius() == Catch::Approx(6.0));
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("The legacy check of a long zigzag does linear work", "[offset][check]") {
    constexpr int segments = 10000;
    std::vector<Vertex> vertices;
    vertices.reserve(segments + 1);
    for (int i = 0; i <= segments; ++i) {
        vertices.push_back({static_cast<double>(i), (i % 2 == 0) ? 0.0 : 2.0});
    }
    const auto source = polyline(vertices, false);
    REQUIRE(source->count() == static_cast<unsigned>(segments));
    std::unique_ptr<RS_Polyline> offset{static_cast<RS_Polyline*>(source->clone())};
    REQUIRE(offset->offset(RS_Vector{3, 1}, 0.8)); // below, as M-P9b
    // the time it takes is measured by hand, in Release, not asserted
    const LC_OffsetCheckReport report = checkLegacyPolylineOffset(*source, *offset, 0.8);
    CHECK(report.verdict == LC_OffsetCheckVerdict::Valid);
    CHECK(report.cellsVisited <= 64u * segments);
}
