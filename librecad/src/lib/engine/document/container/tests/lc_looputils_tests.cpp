/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2026 LibreCAD.org
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
****************************************************************************/

// Regression tests for the per-hatch tolerance classifier. The 1e-4 ON band
// in LoopSorter::findParent made thin slivers against a parent edge look
// inside the parent and a hyperbola island whose centre of mass sat near a
// wall drop out. The classifier now runs with a per-hatch ε and routes the
// pattern trimmer through the collinear-safe keepPiece rule.

#include <algorithm>
#include <array>
#include <cfloat>
#include <cmath>
#include <memory>
#include <vector>

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "lc_hatchtestsupport.h"
#include "lc_hyperbola.h"
#include "lc_loopclassifier.h"
#include "lc_looputils.h"
#include "rs_arc.h"
#include "rs_circle.h"
#include "rs_entitycontainer.h"
#include "rs_hatch.h"
#include "rs_line.h"
#include "rs_pattern.h"
#include "rs_vector.h"

using Catch::Matchers::WithinAbs;
using Catch::Matchers::WithinRel;
using lc::test::makeHatch;
using lc::test::Ring;

namespace {

Ring uRing() {
    // Letter U traced CCW.
    return {RS_Vector(0, 0),      RS_Vector(300, 0),   RS_Vector(300, 300),
            RS_Vector(200, 300),  RS_Vector(200, 100), RS_Vector(100, 100),
            RS_Vector(100, 300),  RS_Vector(0, 300)};
}

Ring thinBarAgainstUFloor() {
    // Sits exactly on the U's inner floor (y = 100) with height 1.5e-4.
    // Its bbox centre is at (150, 100 + 7.5e-5), which falls inside the
    // 1e-4 ON band isPointInsideContour applies, so the legacy nesting
    // path makes it a hole; the per-hatch ε (here around 1e-8) does not.
    return {RS_Vector(120, 100),           RS_Vector(180, 100),
            RS_Vector(180, 100 + 1.5e-4),  RS_Vector(120, 100 + 1.5e-4)};
}

/** Build a container of RS_Line edges that form a closed polygon. */
std::unique_ptr<RS_EntityContainer> makeLoopContainer(const Ring& ring) {
    auto cont = std::make_unique<RS_EntityContainer>(nullptr, true);
    for (size_t i = 0; i + 1 < ring.size(); ++i) {
        cont->addEntity(new RS_Line(cont.get(), ring[i], ring[i + 1]));
    }
    cont->addEntity(new RS_Line(cont.get(), ring.back(), ring.front()));
    return cont;
}

} // namespace

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("A thin bar outside a U that touches it is a root, not a hole",
          "[lc-hatchfix][nesting]") {
    // Solid hatch so m_data.angle plays no role; the test is topological.
    std::vector<Ring> rings{uRing(), thinBarAgainstUFloor()};
    RS_HatchData data{true, 1.0, 0.0, QStringLiteral("SOLID")};
    auto hatch = makeHatch(rings, data);

    REQUIRE(hatch->countLoops() == 2);
    REQUIRE(hatch->countAllLoops() == 2);

    const double uArea = 300.0 * 300.0 - 100.0 * 200.0;
    const double barArea = 60.0 * 1.5e-4;
    REQUIRE_THAT(hatch->getTotalArea(),
                 WithinAbs(uArea + barArea, 1e-6));
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("Hatch tolerance is the interim eps", "[lc-hatchfix][classifier]") {
    // Rectangle 4x3 placed at various origins. The per-hatch tolerance is
    //   epsilon = max(64*u*M, 1e-9*D, min(1e-8, 1e-6*D)),
    // with D = diag(4, 3) = 5 and M = largest |x|,|y| of the endpoints.
    auto buildHatch = [](const RS_Vector& ll) {
        std::vector<Ring> rings{{
            ll, ll + RS_Vector(4, 0), ll + RS_Vector(4, 3), ll + RS_Vector(0, 3)
        }};
        RS_HatchData data{true, 1.0, 0.0, QStringLiteral("SOLID")};
        return makeHatch(rings, data);
    };

    SECTION("origin") {
        auto h = buildHatch(RS_Vector(0.0, 0.0));
        REQUIRE_THAT(h->getTolerance(), WithinAbs(1.0e-8, 1e-20));
    }
    SECTION("1e6 offset") {
        auto h = buildHatch(RS_Vector(1.0e6, 1.0e6));
        // M = 1e6 + 4 → 64*u*M ≈ 7.1e-9; floor min(1e-8, 1e-6·5) = 1e-8 wins.
        REQUIRE_THAT(h->getTolerance(), WithinAbs(1.0e-8, 1e-20));
    }
    SECTION("1e7 offset") {
        auto h = buildHatch(RS_Vector(1.0e7, 1.0e7));
        // M = 1e7 + 4 → 64*u*M = 7.105430199771945e-8 wins over the floor.
        const double expected = 64.0 * (DBL_EPSILON / 2.0) * (1.0e7 + 4.0);
        REQUIRE_THAT(h->getTolerance(), WithinRel(expected, 1e-12));
    }
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("LC_Loops exposes its loop geometry", "[lc-hatchfix][nesting]") {
    // Rectangle 4x3 with a 1x1 hole: both the root and the hole must have
    // their own geometry vertex table.
    auto outer = makeLoopContainer(lc::test::rectRing(0, 0, 4, 3));
    auto hole = makeLoopContainer(lc::test::rectRing(1, 1, 2, 2));

    std::vector<std::unique_ptr<RS_EntityContainer>> loops;
    loops.push_back(std::move(outer));
    loops.push_back(std::move(hole));
    RS_HatchData data{true, 1.0, 0.0, QStringLiteral("SOLID")};
    auto hatch = makeHatch(std::move(loops), data);

    REQUIRE(hatch->countLoops() == 1);
    REQUIRE(hatch->countAllLoops() == 2);

    // The outer loop is the first root; the hole is its child.
    const auto* bRoot = hatch->getBoundaryContainer(0);
    const auto* bHole = hatch->getBoundaryContainer(1);
    REQUIRE(bRoot != nullptr);
    REQUIRE(bHole != nullptr);
    REQUIRE(bRoot->count() == 4u);
    REQUIRE(bHole->count() == 4u);
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("Hyperbola island is parented", "[lc-hatchfix][nesting]") {
    // Rectangle (-2,-5)-(3,0) with a left-branch hyperbola island centred
    // at (3,-2). The hyperbola area is 0.945631933837233 by the restored
    // 2.2.1 primitive, so the net area is
    // 25 - 0.945631933837233 = 24.054368066162767.
    auto outer = makeLoopContainer(lc::test::rectRing(-2, -5, 3, 0));

    // Hyperbola closed by a chord from end to start.
    LC_HyperbolaData hd;
    hd.center = RS_Vector(3.0, -2.0);
    hd.majorP = RS_Vector(2.0 * std::cos(25.0 * M_PI / 180.0),
                          2.0 * std::sin(25.0 * M_PI / 180.0));
    hd.ratio = 0.5;
    hd.angle1 = -0.5;
    hd.angle2 = 1.2;
    hd.reversed = true;

    auto hyp = std::make_unique<LC_Hyperbola>(nullptr, hd);
    const RS_Vector pStart = hyp->getStartpoint();
    const RS_Vector pEnd = hyp->getEndpoint();

    auto island = std::make_unique<RS_EntityContainer>(nullptr, true);
    island->addEntity(hyp.release());
    island->addEntity(new RS_Line(island.get(), pEnd, pStart));

    std::vector<std::unique_ptr<RS_EntityContainer>> loops;
    loops.push_back(std::move(outer));
    loops.push_back(std::move(island));
    RS_HatchData data{true, 1.0, 0.0, QStringLiteral("SOLID")};
    auto hatch = makeHatch(std::move(loops), data);

    REQUIRE(hatch->countLoops() == 1);
    REQUIRE(hatch->countAllLoops() == 2);

    const double expectedArea = 25.0 - 0.945631933837233;
    REQUIRE_THAT(hatch->getTotalArea(), WithinAbs(expectedArea, 1e-9));
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("Collinear pattern pieces: bottom and right drawn, top and left not",
          "[lc-hatchfix][pattern]") {
    // Rectangle [0,30] x [0,20]. Two synthetic patterns:
    //   P0 — horizontal lines y = 0 and y = 5 (tile 10 x 5);
    //   P90 — vertical lines x = 0 and x = 5 (tile 5 x 10).
    // Through LC_Loops::trimPatternEntities + keepPiece: lines on the tile's
    // own min (bottom, left in tile coords) draw along the boundary edges
    // that share the lattice; lines on the max (top, right in tile coords)
    // do not. Reduced to the rectangle: bottom and right edges get drawn,
    // top and left do not.
    auto buildPattern = [](bool horizontal) {
        auto p = std::make_unique<RS_Pattern>(QStringLiteral("TESTCOLL"));
        if (horizontal) {
            p->addEntity(new RS_Line(p.get(), RS_Vector(0, 0), RS_Vector(10, 0)));
            p->addEntity(new RS_Line(p.get(), RS_Vector(0, 5), RS_Vector(10, 5)));
        } else {
            p->addEntity(new RS_Line(p.get(), RS_Vector(0, 0), RS_Vector(0, 10)));
            p->addEntity(new RS_Line(p.get(), RS_Vector(5, 0), RS_Vector(5, 10)));
        }
        p->calculateBorders();
        return p;
    };

    // Build an outer container of RS_Line edges and run it through
    // LoopOptimizer to get an LC_Loops tree.
    auto makeOuter = []() {
        auto outer = std::make_unique<RS_EntityContainer>(nullptr, true);
        const Ring rect{RS_Vector(0, 0), RS_Vector(30, 0),
                        RS_Vector(30, 20), RS_Vector(0, 20)};
        for (size_t i = 0; i < rect.size(); ++i) {
            const RS_Vector& a = rect[i];
            const RS_Vector& b = rect[(i + 1) % rect.size()];
            outer->addEntity(new RS_Line(outer.get(), a, b));
        }
        return outer;
    };

    SECTION("horizontal pattern: bottom drawn, top not") {
        auto pattern = buildPattern(true);
        auto outer = makeOuter();
        LC_LoopUtils::LoopOptimizer opt(*outer);
        auto results = opt.getResults();
        REQUIRE(results);
        REQUIRE_FALSE(results->empty());
        const auto& loop = (*results)[0];
        auto trimmed = loop.trimPatternEntities(*pattern, RS_Vector(0.0, 0.0));
        REQUIRE(trimmed != nullptr);

        int bottom = 0, top = 0;
        for (const RS_Entity* en : *trimmed) {
            if (!en || en->rtti() != RS2::EntityLine) continue;
            const auto* ln = static_cast<const RS_Line*>(en);
            const double ya = ln->getStartpoint().y;
            const double yb = ln->getEndpoint().y;
            if (std::abs(ya) < 1e-6 && std::abs(yb) < 1e-6) ++bottom;
            if (std::abs(ya - 20.0) < 1e-6 && std::abs(yb - 20.0) < 1e-6) ++top;
        }
        REQUIRE(bottom > 0);
        REQUIRE(top == 0);
    }
    SECTION("vertical pattern: right drawn, left not") {
        auto pattern = buildPattern(false);
        auto outer = makeOuter();
        LC_LoopUtils::LoopOptimizer opt(*outer);
        auto results = opt.getResults();
        REQUIRE(results);
        REQUIRE_FALSE(results->empty());
        const auto& loop = (*results)[0];
        auto trimmed = loop.trimPatternEntities(*pattern, RS_Vector(0.0, 0.0));
        REQUIRE(trimmed != nullptr);

        int left = 0, right = 0;
        for (const RS_Entity* en : *trimmed) {
            if (!en || en->rtti() != RS2::EntityLine) continue;
            const auto* ln = static_cast<const RS_Line*>(en);
            const double xa = ln->getStartpoint().x;
            const double xb = ln->getEndpoint().x;
            if (std::abs(xa) < 1e-6 && std::abs(xb) < 1e-6) ++left;
            if (std::abs(xa - 30.0) < 1e-6 && std::abs(xb - 30.0) < 1e-6) ++right;
        }
        REQUIRE(right > 0);
        REQUIRE(left == 0);
    }
}

namespace {

/** Signature of a loop container used to compare extraction runs byte-for-byte. */
struct LoopSignature {
    std::vector<std::tuple<int, double, double, double, double>> items;
    bool operator==(const LoopSignature& o) const { return items == o.items; }
};

LoopSignature signatureOf(const std::vector<std::unique_ptr<RS_EntityContainer>>& loops) {
    LoopSignature sig;
    for (const auto& loop : loops) {
        if (!loop) continue;
        for (const RS_Entity* e : *loop) {
            if (!e) continue;
            const RS_Vector s = e->getStartpoint();
            const RS_Vector t = e->getEndpoint();
            sig.items.emplace_back(static_cast<int>(e->rtti()), s.x, s.y, t.x, t.y);
        }
        // Loop separator (invalid record): tells adjacent loops apart.
        sig.items.emplace_back(-1, 0.0, 0.0, 0.0, 0.0);
    }
    return sig;
}

/** Builds a 4-line unit square container (CCW). */
std::unique_ptr<RS_EntityContainer> makeSquare() {
    auto c = std::make_unique<RS_EntityContainer>(nullptr, true);
    const RS_Vector a(0.0, 0.0);
    const RS_Vector b(1.0, 0.0);
    const RS_Vector d(1.0, 1.0);
    const RS_Vector e(0.0, 1.0);
    c->addEntity(new RS_Line(c.get(), a, b));
    c->addEntity(new RS_Line(c.get(), b, d));
    c->addEntity(new RS_Line(c.get(), d, e));
    c->addEntity(new RS_Line(c.get(), e, a));
    c->calculateBorders();
    return c;
}

/** Builds a three-loop container (big square, hole, far square). */
std::unique_ptr<RS_EntityContainer> makeThreeLoops() {
    auto c = std::make_unique<RS_EntityContainer>(nullptr, true);
    // Outer 10x10.
    const Ring outer = lc::test::rectRing(0.0, 0.0, 10.0, 10.0);
    for (size_t i = 0; i < outer.size(); ++i) {
        c->addEntity(new RS_Line(c.get(), outer[i], outer[(i + 1) % outer.size()]));
    }
    // Inner 4x4 (hole).
    const Ring hole = lc::test::rectRing(3.0, 3.0, 7.0, 7.0);
    for (size_t i = 0; i < hole.size(); ++i) {
        c->addEntity(new RS_Line(c.get(), hole[i], hole[(i + 1) % hole.size()]));
    }
    // Disjoint 2x2 at (20,20).
    const Ring far = lc::test::rectRing(20.0, 20.0, 22.0, 22.0);
    for (size_t i = 0; i < far.size(); ++i) {
        c->addEntity(new RS_Line(c.get(), far[i], far[(i + 1) % far.size()]));
    }
    c->calculateBorders();
    return c;
}

} // namespace

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("Loop extraction does not depend on earlier extractions",
          "[lc-hatchfix][nesting]") {
    // Deterministic rule: given the same unit-square input, each extractor
    // run must produce the identical (rtti, start, end) sequence, regardless
    // of other extractors run before it. On the parent commit the
    // function-static mt19937 in findFirst() picks a different corner with
    // probability about 3/4 per call, so the sequence of 20 interleaved runs
    // fluctuates.
    std::vector<LoopSignature> signatures;
    signatures.reserve(20);
    LoopSignature baseline;
    bool haveBaseline = false;
    for (int iter = 0; iter < 20; ++iter) {
        // Interleave an unrelated 3-loop extraction between every pair.
        auto other = makeThreeLoops();
        LC_LoopUtils::LoopExtractor otherEx(*other);
        (void)otherEx.extract();  // consume

        auto square = makeSquare();
        LC_LoopUtils::LoopExtractor sqEx(*square);
        auto result = sqEx.extract();
        LoopSignature sig = signatureOf(result);
        if (!haveBaseline) {
            baseline = sig;
            haveBaseline = true;
        }
        signatures.push_back(std::move(sig));
    }
    for (size_t i = 0; i < signatures.size(); ++i) {
        INFO("iteration " << i);
        REQUIRE(signatures[i] == baseline);
    }
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("findFirst starts on the outer boundary when curves lie leftmost",
          "[lc-hatchfix][nesting]") {
    // Outer circle as two half arcs (centre (0,0), r = 100). The left arc's
    // leftmost extremum is (-100, 0) (interior). Plus a small square whose
    // leftmost vertex is (-90, -10). Extraction picks the leftmost point
    // (-100, 0) first, so the first loop has 2 arc edges and starts on the
    // left arc.
    auto c = std::make_unique<RS_EntityContainer>(nullptr, true);
    const RS_Vector cen(0.0, 0.0);
    const double r = 100.0;
    // Left arc: angle1 = pi/2, angle2 = 3*pi/2 (CCW through (-100, 0)).
    c->addEntity(new RS_Arc(c.get(),
                             RS_ArcData(cen, r, M_PI / 2.0, 3.0 * M_PI / 2.0, false)));
    // Right arc: angle1 = -pi/2, angle2 = pi/2 (CCW through (100, 0)).
    c->addEntity(new RS_Arc(c.get(),
                             RS_ArcData(cen, r, -M_PI / 2.0, M_PI / 2.0, false)));
    // Small square inside the left half, around (-80, 0).
    const Ring sq{RS_Vector(-90, -10), RS_Vector(-70, -10),
                  RS_Vector(-70, 10),  RS_Vector(-90, 10)};
    for (size_t i = 0; i < sq.size(); ++i) {
        c->addEntity(new RS_Line(c.get(), sq[i], sq[(i + 1) % sq.size()]));
    }
    c->calculateBorders();

    LC_LoopUtils::LoopExtractor ex(*c);
    auto result = ex.extractLoops();
    REQUIRE(result.size() == 2u);
    // The first extracted loop is the outer circle (two arcs); the second is
    // the inner square.
    REQUIRE(result[0].edges->count() == 2u);
    REQUIRE(result[1].edges->count() == 4u);
    // Both edges of the first loop must be arcs.
    for (unsigned k = 0; k < result[0].edges->count(); ++k) {
        REQUIRE(result[0].edges->entityAt(k)->rtti() == RS2::EntityArc);
    }
    // The lexicographically smallest vertex (-90, -10) belongs to the square
    // (the small inner island), not the outer boundary.
    bool squareHasVertex = false;
    for (const RS_Vector& v : result[1].vertices) {
        if (std::abs(v.x - (-90.0)) < 1e-9 && std::abs(v.y - (-10.0)) < 1e-9) {
            squareHasVertex = true;
            break;
        }
    }
    REQUIRE(squareHasVertex);
}
