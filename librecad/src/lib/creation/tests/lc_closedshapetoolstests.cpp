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

// #2888: the polylines Draw > Star, the polygon tools and the 1- and 2-point
// rectangles make ended on their start instead of being closed, so their
// offsets were open at the start. Each tool's shape builder is called through
// a probe, as the tool calls it; the closed shapes are then offset.

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <memory>
#include <vector>

#include "lc_action_draw_polygon_center_corner.h"
#include "lc_action_draw_rectangle_1point.h"
#include "lc_action_draw_rectangle_2points.h"
#include "lc_action_draw_star.h"
#include "lc_actiontestsupport.h"
#include "rs_arc.h"
#include "rs_document.h"
#include "rs_modification.h"
#include "rs_polyline.h"

namespace {

class StarProbe final : public LC_ActionDrawStar {
public:
    explicit StarProbe(LC_ActionContext* context) : LC_ActionDrawStar(context) {}

    using LC_ActionDrawStar::SetInnerPoint;
    using LC_ActionDrawStar::createShapePolyline;
    using LC_ActionDrawStar::m_centerPoint;
    using LC_ActionDrawStar::m_outerPoint;
};

class PolygonProbe final : public LC_ActionDrawLinePolygonCenterCorner {
public:
    explicit PolygonProbe(LC_ActionContext* context) : LC_ActionDrawLinePolygonCenterCorner(context) {}

    using LC_ActionDrawPolygonBase::PolygonInfo;
    using LC_ActionDrawPolygonBase::createShapePolyline;
};

class Rectangle1PointProbe final : public LC_ActionDrawRectangle1Point {
public:
    explicit Rectangle1PointProbe(LC_ActionContext* context) : LC_ActionDrawRectangle1Point(context) {}

    using LC_ActionDrawRectangle1Point::createPolyline;
};

class Rectangle2PointsProbe final : public LC_ActionDrawRectangle2Points {
public:
    explicit Rectangle2PointsProbe(LC_ActionContext* context) : LC_ActionDrawRectangle2Points(context) {}

    using LC_ActionDrawRectangle2Points::createPolyline;
    using LC_ActionDrawRectangle2Points::m_corner1;
};

/** A 10 x 4 rectangle as the 1-point tool makes it about (5, 2), straight or with corners of radius 1. */
std::unique_ptr<RS_Polyline> rectangle1Point(const int corners) {
    lc::test::ActionFixture<Rectangle1PointProbe> f;
    f.m_action->setInsertionPointSnapMode(LC_ActionDrawRectangle1Point::SNAP_MIDDLE);
    f.m_action->setWidth(10.0);
    f.m_action->setHeight(4.0);
    f.m_action->setCornersMode(corners);
    f.m_action->setCornerRadius(1.0);
    std::unique_ptr<RS_Polyline> result{f.m_action->createPolyline(RS_Vector{5.0, 2.0}).resultingPolyline};
    REQUIRE(result != nullptr);
    result->setParent(nullptr); // outlives the drawing it was made for
    return result;
}

/** The rectangle (0, 0)-(10, 4) as the 2-point tool makes it, straight or with corners of radius 1. */
std::unique_ptr<RS_Polyline> rectangle2Points(const int corners) {
    lc::test::ActionFixture<Rectangle2PointsProbe> f;
    f.m_action->setCornersMode(corners);
    f.m_action->setCornerRadius(1.0);
    f.m_action->m_corner1 = RS_Vector{0.0, 0.0};
    std::unique_ptr<RS_Polyline> result{f.m_action->createPolyline(RS_Vector{10.0, 4.0}).resultingPolyline};
    REQUIRE(result != nullptr);
    result->setParent(nullptr);
    return result;
}

/**
 * A 5-ray star about the origin, as Draw > Star makes it: outer vertices
 * at radius 10 from 90 degrees on, inner ones at radius 4 halfway between.
 */
std::unique_ptr<RS_Polyline> star(const bool outerRounded = false, const bool innerRounded = false) {
    lc::test::ActionFixture<StarProbe> f;
    f.m_action->setRaysNumber(5);
    f.m_action->setSymmetric(true);
    f.m_action->setOuterRounded(outerRounded);
    f.m_action->setRadiusOuter(1.5); // the rounding radii
    f.m_action->setInnerRounded(innerRounded);
    f.m_action->setRadiusInner(0.5);
    f.m_action->m_centerPoint = RS_Vector{0.0, 0.0};
    f.m_action->m_outerPoint = RS_Vector{0.0, 10.0};
    RS_Vector inner{4.0, 0.0}; // symmetric: only its distance counts
    QList<RS_Entity*> references;
    std::unique_ptr<RS_Polyline> result{
        f.m_action->createShapePolyline(inner, references, StarProbe::SetInnerPoint, false)};
    qDeleteAll(references);
    REQUIRE(result != nullptr);
    result->setParent(nullptr); // outlives the drawing it was made for
    return result;
}

/** A regular pentagon of circumradius 10 about the origin, a vertex at 90 degrees. */
std::unique_ptr<RS_Polyline> pentagon(const bool rounded = false) {
    lc::test::ActionFixture<PolygonProbe> f;
    f.m_action->setNumber(5);
    f.m_action->setCornersRounded(rounded);
    f.m_action->setRoundingRadius(1.0);
    PolygonProbe::PolygonInfo info;
    info.centerPoint = RS_Vector{0.0, 0.0};
    info.startingAngle = M_PI_2;
    info.vertexRadius = 10.0;
    std::unique_ptr<RS_Polyline> result{f.m_action->createShapePolyline(info, false)};
    REQUIRE(result != nullptr);
    return result;
}

/** Closed by its flag, the last segment ending on the start, none of no length. */
void checkClosed(const RS_Polyline& polyline, const unsigned segments) {
    CHECK(polyline.isClosed());
    REQUIRE(polyline.count() == segments);
    for (const RS_Entity* segment : polyline) {
        CHECK(segment->getLength() > 1e-6);
    }
    // the closing segment is its own, not a vertex put on the start
    CHECK(polyline.getStartpoint().distanceTo(polyline.getEndpoint()) > 1e-6);
    CHECK(polyline.last()->getEndpoint().distanceTo(polyline.getStartpoint()) < 1e-9);
    CHECK(polyline.first()->getStartpoint().distanceTo(polyline.getStartpoint()) < 1e-9);
}

/** The vertices of a closed polyline, each once. */
std::vector<RS_Vector> verticesOf(const RS_Polyline& polyline) {
    std::vector<RS_Vector> vertices;
    for (const RS_Entity* segment : polyline) {
        vertices.push_back(segment->getStartpoint());
    }
    return vertices;
}

/**
 * The mitred offset of the closed polygon @p vertices, running anticlockwise,
 * by @p d outwards (negative: inwards): each vertex where the offsets of its
 * two edges meet.
 */
std::vector<RS_Vector> mitredOffset(const std::vector<RS_Vector>& vertices, const double d) {
    const std::size_t n = vertices.size();
    const auto shifted = [&](const std::size_t i, RS_Vector& a, RS_Vector& b) {
        const RS_Vector p = vertices[i];
        const RS_Vector q = vertices[(i + 1) % n];
        const RS_Vector along = (q - p).normalized();
        const RS_Vector right{along.y, -along.x}; // outside an anticlockwise polygon
        a = p + right * d;
        b = q + right * d;
    };
    std::vector<RS_Vector> result;
    for (std::size_t i = 0; i < n; ++i) {
        RS_Vector a1{false};
        RS_Vector b1{false};
        RS_Vector a2{false};
        RS_Vector b2{false};
        shifted((i + n - 1) % n, a1, b1);
        shifted(i, a2, b2);
        const RS_Vector r = b1 - a1;
        const RS_Vector s = b2 - a2;
        const double t = ((a2.x - a1.x) * s.y - (a2.y - a1.y) * s.x) / (r.x * s.y - r.y * s.x);
        result.push_back(a1 + r * t);
    }
    return result;
}

/** Whether @p actual and @p expected hold the same points, within @p tolerance, in any order. */
bool samePoints(const std::vector<RS_Vector>& actual, const std::vector<RS_Vector>& expected, const double tolerance) {
    if (actual.size() != expected.size()) {
        return false;
    }
    for (const RS_Vector& e : expected) {
        bool found = false;
        for (const RS_Vector& a : actual) {
            found = found || a.distanceTo(e) <= tolerance;
        }
        if (!found) {
            return false;
        }
    }
    return true;
}

/** The one polyline Modify > Offset makes of @p source towards @p pick at @p d. */
struct OffsetResult {
    LC_DocumentModificationBatch ctx;
    const RS_Polyline* polyline = nullptr;

    ~OffsetResult() {
        qDeleteAll(ctx.entitiesToAdd);
    }
};

void offsetPolyline(RS_Polyline& source, const RS_Vector& pick, const double d, OffsetResult& out) {
    RS_OffsetData data;
    data.coord = pick;
    data.distance = d;
    data.keepOriginals = true;
    const LC_OffsetBatchOutcome outcome =
        RS_Modification::offsetWithOutcome(data, {&source}, false, LC_OffsetBatchLimits{}, out.ctx);
    REQUIRE(outcome.sources.size() == 1);
    REQUIRE(outcome.sources.front().succeeded());
    REQUIRE(out.ctx.entitiesToAdd.size() == 1);
    REQUIRE(out.ctx.entitiesToAdd.front()->rtti() == RS2::EntityPolyline);
    out.polyline = static_cast<const RS_Polyline*>(out.ctx.entitiesToAdd.front());
}

} // namespace

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("A star and a polygon are closed polylines, and their offsets close", "[offset][polygon]") {
    SECTION("a star with sharp rays") {
        const auto shape = star();
        checkClosed(*shape, 10);
        CHECK(shape->getStartpoint().distanceTo({0.0, 10.0}) < 1e-9);
    }
    SECTION("a star with rounded rays: the closing segment rounds the first outer vertex") {
        const auto shape = star(true, true);
        checkClosed(*shape, 20);
        REQUIRE(shape->last()->rtti() == RS2::EntityArc);
        const auto* closing = static_cast<const RS_Arc*>(shape->last());
        CHECK(closing->getRadius() == Catch::Approx(1.5)); // the outer rounding's, not the inner's 0.5
        CHECK(std::abs(closing->getCenter().x) < 1e-9);  // on the first ray, at 90 degrees
        CHECK(closing->getCenter().y > 4.0);
    }
    SECTION("a pentagon, sharp and rounded") {
        checkClosed(*pentagon(), 5);
        checkClosed(*pentagon(true), 10);
        REQUIRE(pentagon(true)->last()->rtti() == RS2::EntityArc);
    }
    SECTION("the pentagon grown by 1: its vertices at 10 + 1/cos 36 degrees") {
        auto shape = pentagon();
        OffsetResult result;
        offsetPolyline(*shape, {-6.18, 8.51}, 1.0, result); // outside its first edge
        CHECK(result.polyline->isClosed());
        REQUIRE(result.polyline->count() == 5);
        for (const RS_Vector& v : verticesOf(*result.polyline)) {
            CHECK(v.magnitude() == Catch::Approx(10.0 + 1.0 / std::cos(M_PI / 5.0)));
            CHECK(v.magnitude() == Catch::Approx(11.23607).margin(1e-5));
        }
    }
    SECTION("the star grown and shrunk by 1: the mitred rings") {
        auto shape = star();
        const std::vector<RS_Vector> source = verticesOf(*shape);
        OffsetResult grown;
        offsetPolyline(*shape, {-3.065, 7.275}, 1.0, grown); // outside its first edge
        CHECK(grown.polyline->isClosed());
        const std::vector<RS_Vector> outside = verticesOf(*grown.polyline);
        CHECK(samePoints(outside, mitredOffset(source, 1.0), 1e-9));
        // the oracle's ring
        CHECK(samePoints(outside,
                         {{0, 13.0457},       {3.0672, 4.2217},  {12.4072, 4.0313}, {4.9629, -1.6125},
                          {7.6681, -10.5542}, {0, -5.2183},      {-7.6681, -10.5542}, {-4.9629, -1.6125},
                          {-12.4072, 4.0313}, {-3.0672, 4.2217}},
                         1e-4));
        OffsetResult shrunk;
        offsetPolyline(*shape, {-0.7, 6.4}, 1.0, shrunk); // inside its first edge
        CHECK(shrunk.polyline->isClosed());
        const std::vector<RS_Vector> inside = verticesOf(*shrunk.polyline);
        CHECK(samePoints(inside, mitredOffset(source, -1.0), 1e-9));
        CHECK(samePoints(inside,
                         {{0, 6.9543},       {1.6350, 2.2505},  {6.6139, 2.1490},  {2.6456, -0.8596},
                          {4.0876, -5.6261}, {0, -2.7817},      {-4.0876, -5.6261}, {-2.6456, -0.8596},
                          {-6.6139, 2.1490}, {-1.6350, 2.2505}},
                         1e-4));
    }
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("The 1- and 2-point rectangles are closed polylines, and their offsets close", "[offset][rectangle]") {
    using Corners = LC_ActionDrawRectangleAbstract::CornersMode;
    SECTION("straight corners: four segments") {
        for (const auto& shape : {rectangle1Point(Corners::CORNER_STRAIGHT), rectangle2Points(Corners::CORNER_STRAIGHT)}) {
            checkClosed(*shape, 4);
            CHECK(samePoints(verticesOf(*shape), {{0, 0}, {10, 0}, {10, 4}, {0, 4}}, 1e-9));
        }
    }
    SECTION("rounded corners: closed, as they were") {
        for (const auto& shape : {rectangle1Point(Corners::CORNER_RADIUS), rectangle2Points(Corners::CORNER_RADIUS)}) {
            CHECK(shape->isClosed());
            CHECK(shape->count() == 8);
            CHECK(shape->last()->getEndpoint().distanceTo(shape->getStartpoint()) < 1e-9);
        }
    }
    SECTION("grown by 1: one closed rectangle") {
        auto shape = rectangle2Points(Corners::CORNER_STRAIGHT);
        OffsetResult result;
        offsetPolyline(*shape, {5.0, -1.0}, 1.0, result);
        CHECK(result.polyline->isClosed());
        CHECK(samePoints(verticesOf(*result.polyline), {{-1, -1}, {11, -1}, {11, 5}, {-1, 5}}, 1e-9));
    }
}
