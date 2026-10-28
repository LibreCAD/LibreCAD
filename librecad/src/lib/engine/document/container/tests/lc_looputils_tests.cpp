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

// Regression tests for island nesting and the per-hatch tolerance classifier.
// The master build decided parentage from each child's bounding-box centre,
// which is outside the parent whenever the parent is a U, L or C: 11 of 24
// probe shapes came back as a wrong tree, visible as filled lakes, missing
// islands and islands that merge with the outer wall. The new sorter picks a
// strictly interior point of the child and searches every candidate in the
// full area-sorted list with an epsilon-slack bbox test.

#include <algorithm>
#include <array>
#include <cfloat>
#include <cmath>
#include <functional>
#include <limits>
#include <memory>
#include <string>
#include <vector>

#include <random>

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/generators/catch_generators_range.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "lc_containertraverser.h"
#include "lc_hatchtestsupport.h"
#include "lc_hyperbola.h"
#include "lc_loopclassifier.h"
#include "lc_looputils.h"
#include "lc_parabola.h"
#include "rs_arc.h"
#include "rs_circle.h"
#include "rs_ellipse.h"
#include "rs_entitycontainer.h"
#include "rs_hatch.h"
#include "rs_line.h"
#include "rs_pattern.h"
#include "rs_patternlist.h"
#include "rs_polyline.h"
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
    // path makes it a hole; the per-hatch epsilon (here around 1e-8) does not.
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
TEST_CASE("Circle island inside a rotated half-ellipse nests on a SOLID hatch",
          "[lc-hatchfix][nesting]") {
    // Boundary: rotated half-ellipse a = 3, b = 2 at (10, 5), phi = 30 deg
    // (one loop with an elliptic-arc edge plus the diametric chord), with a
    // circle island r = 0.5 centred at (9.6, 5.7). The CW variant exercises
    // the extractor's reversal path (revertDirection swaps angles and flips
    // isReversed), where the LoopGeometry piece-boundary vertices used to
    // bind to the wrong endpoint on the reversed elliptic arc and overcount
    // the right-crossings through the island centre; both loops came back as
    // roots and the SOLID area was ellipse + circle = 10.21 instead of
    // 8.639379797...
    const double phi = 30.0 * M_PI / 180.0;
    const RS_Vector majorP(3.0 * std::cos(phi), 3.0 * std::sin(phi));

    auto buildEllipseLoop = [&](bool cwChordFirst) {
        auto cont = std::make_unique<RS_EntityContainer>(nullptr, true);
        RS_EllipseData ed;
        ed.center = RS_Vector(10, 5);
        ed.majorP = majorP;
        ed.ratio = 2.0 / 3.0;
        ed.angle1 = 0.0;
        ed.angle2 = M_PI;
        ed.reversed = false;
        const RS_Vector low  = RS_Vector(10, 5) - majorP;
        const RS_Vector high = RS_Vector(10, 5) + majorP;
        if (cwChordFirst) {
            // Chord then ellipse — gives negative area so the extractor must
            // reverse the whole loop (revertDirection on both edges).
            cont->addEntity(new RS_Line(cont.get(), low, high));
            cont->addEntity(new RS_Ellipse(cont.get(), ed));
        } else {
            cont->addEntity(new RS_Ellipse(cont.get(), ed));
            cont->addEntity(new RS_Line(cont.get(), low, high));
        }
        return cont;
    };
    auto buildIsland = []() {
        auto cont = std::make_unique<RS_EntityContainer>(nullptr, true);
        cont->addEntity(new RS_Circle(cont.get(),
            RS_CircleData{RS_Vector(9.6, 5.7), 0.5}));
        return cont;
    };

    const double expectedArea = M_PI * 3.0 * 2.0 / 2.0 - M_PI * 0.25;

    SECTION("ellipse first, line second (positive area, no reversal)") {
        std::vector<std::unique_ptr<RS_EntityContainer>> loops;
        loops.push_back(buildEllipseLoop(false));
        loops.push_back(buildIsland());
        auto hatch = makeHatch(std::move(loops),
            RS_HatchData(true, 1.0, 0.0, QStringLiteral("SOLID")));
        REQUIRE(hatch->countLoops() == 1);
        REQUIRE(hatch->countAllLoops() == 2);
        REQUIRE_THAT(hatch->getTotalArea(), WithinAbs(expectedArea, 1e-9));
    }

    SECTION("line first, ellipse second (negative area, reversed by extractor)") {
        std::vector<std::unique_ptr<RS_EntityContainer>> loops;
        loops.push_back(buildEllipseLoop(true));
        loops.push_back(buildIsland());
        auto hatch = makeHatch(std::move(loops),
            RS_HatchData(true, 1.0, 0.0, QStringLiteral("SOLID")));
        REQUIRE(hatch->countLoops() == 1);
        REQUIRE(hatch->countAllLoops() == 2);
        REQUIRE_THAT(hatch->getTotalArea(), WithinAbs(expectedArea, 1e-9));
    }
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
        auto tiles = loop.createTiles(*pattern, RS_Vector(0.0, 0.0));
        LC_HatchPieceBudget budget{std::numeric_limits<std::size_t>::max()};
        auto trimmed = loop.trimPatternEntities(*pattern, tiles, budget);
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
        auto tiles = loop.createTiles(*pattern, RS_Vector(0.0, 0.0));
        LC_HatchPieceBudget budget{std::numeric_limits<std::size_t>::max()};
        auto trimmed = loop.trimPatternEntities(*pattern, tiles, budget);
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

// --- Island nesting oracle ---------------------------------------------------
// 24 topology cases built in code from the research oracle. Each row carries
// the expected number of root loops, the sorted multiset of per-root depths,
// the total CCW area and a few probe points. Master is wrong on 11 of 24
// (A, H, J, K, M, P, T4, T7, C1, C5, N4): its tree uses each child's bbox
// centre, which is outside the parent whenever the parent is a U/L/C.

namespace {

// NOLINTBEGIN(readability-identifier-naming)

using RingList = std::vector<Ring>;

RingList ringsRECT() { return {lc::test::rectRing(0, 0, 300, 300)}; }

Ring pointsP() {
    // Letter U.
    return {RS_Vector(0, 0),     RS_Vector(300, 0),   RS_Vector(300, 300),
            RS_Vector(200, 300), RS_Vector(200, 100), RS_Vector(100, 100),
            RS_Vector(100, 300), RS_Vector(0, 300)};
}

Ring pointsI() {
    // Letter U inside P.
    return {RS_Vector(30, 40),   RS_Vector(270, 40),  RS_Vector(270, 270),
            RS_Vector(230, 270), RS_Vector(230, 80),  RS_Vector(70, 80),
            RS_Vector(70, 270),  RS_Vector(30, 270)};
}

Ring pointsI2() {
    return {RS_Vector(31, 43.5),  RS_Vector(269, 43.5), RS_Vector(269, 270),
            RS_Vector(231, 270),  RS_Vector(231, 83.5), RS_Vector(69, 83.5),
            RS_Vector(69, 270),   RS_Vector(31, 270)};
}

Ring pointsBAR() {
    return {RS_Vector(40, 30),  RS_Vector(260, 30),
            RS_Vector(260, 70), RS_Vector(40, 70)};
}

Ring pointsBIG() { return {RS_Vector(-50, -50), RS_Vector(350, -50),
                           RS_Vector(350, 350), RS_Vector(-50, 350)}; }

Ring rotP() {
    const Ring p = pointsP();
    Ring r;
    r.reserve(p.size());
    for (const RS_Vector& v : p) r.emplace_back(300.0 - v.y, v.x);
    return r;
}

Ring rotI() {
    const Ring p = pointsI();
    Ring r;
    r.reserve(p.size());
    for (const RS_Vector& v : p) r.emplace_back(300.0 - v.y, v.x);
    return r;
}

Ring pointsCOMB() {
    return {RS_Vector(0, 0),     RS_Vector(300, 0),   RS_Vector(300, 300),
            RS_Vector(240, 300), RS_Vector(240, 50),  RS_Vector(180, 50),
            RS_Vector(180, 200), RS_Vector(120, 200), RS_Vector(120, 50),
            RS_Vector(60, 50),   RS_Vector(60, 300),  RS_Vector(0, 300)};
}

Ring pointsINVU() {
    return {RS_Vector(70, 80),   RS_Vector(110, 80),  RS_Vector(110, 240),
            RS_Vector(190, 240), RS_Vector(190, 80),  RS_Vector(230, 80),
            RS_Vector(230, 280), RS_Vector(70, 280)};
}

Ring pointsSQ() {
    return {RS_Vector(0, 0), RS_Vector(40, 0),
            RS_Vector(40, 40), RS_Vector(0, 40)};
}

Ring pointsLOUT() {
    return {RS_Vector(0, 0),   RS_Vector(300, 0),   RS_Vector(300, 100),
            RS_Vector(100, 100), RS_Vector(100, 300), RS_Vector(0, 300)};
}

Ring pointsLINN() {
    return {RS_Vector(20, 20),   RS_Vector(280, 20),  RS_Vector(280, 80),
            RS_Vector(80, 80),   RS_Vector(80, 280),  RS_Vector(20, 280)};
}

// --- Curved cases helpers ---

/** Append edges of a CCW annular sector (two arcs + two chords) to loop. */
void appendAnnularSector(RS_EntityContainer& loop, double r1, double r2,
                         double a0, double a1,
                         const RS_Vector& c = RS_Vector(0, 0)) {
    // Outer CCW arc from a0 to a1.
    loop.addEntity(new RS_Arc(&loop, RS_ArcData(c, r2, a0, a1, false)));
    const RS_Vector outerEnd(c.x + r2 * std::cos(a1), c.y + r2 * std::sin(a1));
    const RS_Vector innerStart(c.x + r1 * std::cos(a1), c.y + r1 * std::sin(a1));
    loop.addEntity(new RS_Line(&loop, outerEnd, innerStart));
    // Inner reversed arc from a1 back to a0.
    loop.addEntity(new RS_Arc(&loop, RS_ArcData(c, r1, a1, a0, true)));
    const RS_Vector innerEnd(c.x + r1 * std::cos(a0), c.y + r1 * std::sin(a0));
    const RS_Vector outerStart(c.x + r2 * std::cos(a0), c.y + r2 * std::sin(a0));
    loop.addEntity(new RS_Line(&loop, innerEnd, outerStart));
}

/** bez_u(x0, y0, w, h, t): a U with QBez sag; wall thickness t. */
std::unique_ptr<RS_EntityContainer> bezU(double x0, double y0,
                                         double w, double h, double t) {
    auto cont = std::make_unique<RS_EntityContainer>(nullptr, true);
    const RS_Vector p1(x0 + t, y0 + h);
    const RS_Vector p2(x0, y0 + h);
    const RS_Vector p3(x0, y0 + t);
    const RS_Vector q0(x0, y0 + t);
    const RS_Vector q1(x0 + w / 2.0, y0 - t);
    const RS_Vector q2(x0 + w, y0 + t);
    const RS_Vector p4(x0 + w, y0 + t);
    const RS_Vector p5(x0 + w, y0 + h);
    const RS_Vector p6(x0 + w - t, y0 + h);
    const RS_Vector p7(x0 + w - t, y0 + 2 * t);
    const RS_Vector r0(x0 + w - t, y0 + 2 * t);
    const RS_Vector r1(x0 + w / 2.0, y0 + t);
    const RS_Vector r2(x0 + t, y0 + 2 * t);
    const RS_Vector p8(x0 + t, y0 + 2 * t);
    const RS_Vector p9(x0 + t, y0 + h);
    cont->addEntity(new RS_Line(cont.get(), p1, p2));
    cont->addEntity(new RS_Line(cont.get(), p2, p3));
    cont->addEntity(new LC_Parabola(cont.get(),
                                    LC_ParabolaData({q0, q1, q2})));
    cont->addEntity(new RS_Line(cont.get(), p4, p5));
    cont->addEntity(new RS_Line(cont.get(), p5, p6));
    cont->addEntity(new RS_Line(cont.get(), p6, p7));
    cont->addEntity(new LC_Parabola(cont.get(),
                                    LC_ParabolaData({r0, r1, r2})));
    cont->addEntity(new RS_Line(cont.get(), p8, p9));
    return cont;
}

/** Build a closed polygon container from a Ring. */
std::unique_ptr<RS_EntityContainer> polyContainer(const Ring& ring) {
    return makeLoopContainer(ring);
}

/** Build a container holding one full circle. */
std::unique_ptr<RS_EntityContainer> circleContainer(const RS_Vector& c, double r) {
    auto cont = std::make_unique<RS_EntityContainer>(nullptr, true);
    cont->addEntity(new RS_Circle(cont.get(), RS_CircleData(c, r)));
    return cont;
}

struct ProbePoint {
    double x;
    double y;
    bool fill;    // true = Inside, false = Outside (hole or exterior)
};

struct Case {
    const char* name;
    std::function<std::vector<std::unique_ptr<RS_EntityContainer>>()> build;
    int roots;
    std::vector<int> depths;         // sorted multiset
    double area;
    double areaTol;
    std::vector<ProbePoint> probes;
};

using CaseList = std::vector<Case>;

/** Build loops as a vector of containers from a list of Rings. */
std::vector<std::unique_ptr<RS_EntityContainer>>
loopsFromRings(const std::vector<Ring>& rings) {
    std::vector<std::unique_ptr<RS_EntityContainer>> out;
    out.reserve(rings.size());
    for (const Ring& r : rings) out.push_back(polyContainer(r));
    return out;
}

CaseList buildCases() {
    CaseList cases;
    cases.push_back({"A_U_in_U",
                     []() { return loopsFromRings({pointsP(), pointsI()}); },
                     1, {0, 1}, 45200.0, 1e-6,
                     {{150, 8.485, true}, {50, 175, false}}});
    cases.push_back({"B_U_in_rect",
                     []() { return loopsFromRings({lc::test::rectRing(0, 0, 300, 300),
                                                    pointsI()}); },
                     1, {0, 1}, 65200.0, 1e-6,
                     {{150, 150, true}, {50, 175, false}}});
    cases.push_back({"C_bar_in_U",
                     []() { return loopsFromRings({pointsP(), pointsBAR()}); },
                     1, {0, 1}, 61200.0, 1e-6,
                     {{50, 200, true}, {150, 50, false}}});
    cases.push_back({"H_3level",
                     []() { return loopsFromRings({pointsBIG(), pointsP(),
                                                    pointsI()}); },
                     1, {0, 1, 2}, 114800.0, 1e-6,
                     {{150, 150, true}, {150, 8.485, false}, {50, 175, true}}});
    cases.push_back({"J_C_in_C",
                     []() { return loopsFromRings({rotP(), rotI()}); },
                     1, {0, 1}, 45200.0, 1e-6,
                     {{291.5, 150, true}, {240, 150, false}}});
    cases.push_back({"K_U_in_U_shifted",
                     []() { return loopsFromRings({pointsP(), pointsI2()}); },
                     1, {0, 1}, 46306.0, 1e-6,
                     {{150, 8.485, true}, {150, 63.5, false}}});
    cases.push_back({"M_disjoint_invU_over_comb",
                     []() { return loopsFromRings({pointsCOMB(), pointsINVU()}); },
                     2, {0, 0}, 73200.0, 1e-6,
                     {{30, 125, true}, {90, 160, true}}});
    cases.push_back({"O_invU_with_far_square",
                     []() { return loopsFromRings({pointsSQ(), pointsINVU()}); },
                     2, {0, 0}, 20800.0, 1e-6,
                     {{20, 20, true}, {90, 160, true}}});
    cases.push_back({"P_L_wall",
                     []() { return loopsFromRings({pointsLOUT(), pointsLINN()}); },
                     1, {0, 1}, 22400.0, 1e-6,
                     {{150, 8.485, true}, {50, 180, false}}});
    cases.push_back({"T1_island_vertex_touch",
                     []() { return loopsFromRings({lc::test::rectRing(0, 0, 300, 300),
                                                    {RS_Vector(150, 0), RS_Vector(200, 50),
                                                     RS_Vector(150, 100), RS_Vector(100, 50)}}); },
                     1, {0, 1}, 85000.0, 1e-6,
                     {{150, 150, true}, {150, 75, false}}});
    cases.push_back({"T2_island_shares_edge_part",
                     []() { return loopsFromRings({lc::test::rectRing(0, 0, 300, 300),
                                                    {RS_Vector(60, 0), RS_Vector(180, 0),
                                                     RS_Vector(180, 90), RS_Vector(60, 90)}}); },
                     1, {0, 1}, 79200.0, 1e-6,
                     {{150, 150, true}, {120, 45, false}}});
    cases.push_back({"T3_notch_filler_outside_U",
                     []() { return loopsFromRings({pointsP(),
                                                    {RS_Vector(100, 100), RS_Vector(200, 100),
                                                     RS_Vector(200, 300), RS_Vector(100, 300)}}); },
                     2, {0, 0}, 90000.0, 1e-6,
                     {{50, 200, true}, {150, 200, true}}});
    cases.push_back({"T3b_notch_block_touch_bottom",
                     []() { return loopsFromRings({pointsP(),
                                                    {RS_Vector(120, 100), RS_Vector(180, 100),
                                                     RS_Vector(180, 200), RS_Vector(120, 200)}}); },
                     2, {0, 0}, 76000.0, 1e-6,
                     {{50, 200, true}, {150, 150, true}}});
    cases.push_back({"T4_U_in_U_touching_bottom",
                     []() { return loopsFromRings({pointsP(),
                                                    {RS_Vector(30, 0), RS_Vector(270, 0),
                                                     RS_Vector(270, 270), RS_Vector(230, 270),
                                                     RS_Vector(230, 80), RS_Vector(70, 80),
                                                     RS_Vector(70, 270), RS_Vector(30, 270)}}); },
                     1, {0, 1}, 35600.0, 1e-6,
                     {{291.5, 150, true}, {150, 40, false}}});
    cases.push_back({"T5_L_room_and_notch_room",
                     []() { return loopsFromRings({{RS_Vector(0, 0), RS_Vector(200, 0),
                                                     RS_Vector(200, 50), RS_Vector(100, 50),
                                                     RS_Vector(100, 150), RS_Vector(0, 150)},
                                                    {RS_Vector(100, 50), RS_Vector(200, 50),
                                                     RS_Vector(200, 150), RS_Vector(100, 150)}}); },
                     2, {0, 0}, 30000.0, 1e-6,
                     {{50, 100, true}, {150, 100, true}}});
    cases.push_back({"T6_island_in_hole_vertex_touch",
                     []() { return loopsFromRings({pointsBIG(),
                                                    lc::test::rectRing(0, 0, 300, 300),
                                                    {RS_Vector(150, 0), RS_Vector(250, 100),
                                                     RS_Vector(150, 200), RS_Vector(50, 100)}}); },
                     1, {0, 1, 2}, 90000.0, 1e-6,
                     {{150, -38.69, true}, {75, 8.485, false}, {150, 150, true}}});
    cases.push_back({"T7_U_in_U_shared_wall_3sides",
                     []() { return loopsFromRings({pointsP(),
                                                    {RS_Vector(0, 0), RS_Vector(300, 0),
                                                     RS_Vector(300, 300), RS_Vector(250, 300),
                                                     RS_Vector(250, 50), RS_Vector(50, 50),
                                                     RS_Vector(50, 300), RS_Vector(0, 300)}}); },
                     1, {0, 1}, 30000.0, 1e-6,
                     {{225, 291.5, true}, {25, 175, false}}});
    cases.push_back({"C1_arc_C_in_C",
                     []() {
                         std::vector<std::unique_ptr<RS_EntityContainer>> loops;
                         auto outer = std::make_unique<RS_EntityContainer>(nullptr, true);
                         appendAnnularSector(*outer, 50, 150, M_PI / 4.0, 7.0 * M_PI / 4.0);
                         auto inner = std::make_unique<RS_EntityContainer>(nullptr, true);
                         appendAnnularSector(*inner, 70, 130, M_PI / 3.0, 5.0 * M_PI / 3.0);
                         loops.push_back(std::move(outer));
                         loops.push_back(std::move(inner));
                         return loops;
                     },
                     1, {0, 1}, 21991.148575, 1e-5,
                     {{-142.1, 1.74e-14, true}, {-100, -3.553e-15, false}}});
    cases.push_back({"C2_circle_tangent_inside_circle",
                     []() {
                         std::vector<std::unique_ptr<RS_EntityContainer>> loops;
                         loops.push_back(circleContainer(RS_Vector(0, 0), 100.0));
                         loops.push_back(circleContainer(RS_Vector(70, 0), 30.0));
                         return loops;
                     },
                     1, {0, 1}, 28588.493148, 1e-5,
                     {{0, 50, true}, {70, -15, false}}});
    cases.push_back({"C3_circle_tangent_outside_U",
                     []() {
                         std::vector<std::unique_ptr<RS_EntityContainer>> loops;
                         loops.push_back(polyContainer(pointsP()));
                         loops.push_back(circleContainer(RS_Vector(150, 150), 50.0));
                         return loops;
                     },
                     2, {0, 0}, 77853.981634, 1e-5,
                     {{50, 200, true}, {150, 175, true}}});
    cases.push_back({"C4_bezier_U_in_ellipse",
                     []() {
                         std::vector<std::unique_ptr<RS_EntityContainer>> loops;
                         auto outer = std::make_unique<RS_EntityContainer>(nullptr, true);
                         RS_EllipseData ed;
                         ed.center = RS_Vector(150, 150);
                         ed.majorP = RS_Vector(200, 0);
                         ed.ratio = 0.8;
                         ed.setAngle1(0.0);
                         ed.setAngle2(0.0);
                         outer->addEntity(new RS_Ellipse(outer.get(), ed));
                         loops.push_back(std::move(outer));
                         loops.push_back(bezU(20, 40, 260, 230, 30));
                         return loops;
                     },
                     1, {0, 1}, 79330.964915, 1e-3,
                     {{150, 230, true}, {35, 185, false}}});
    cases.push_back({"C5_bezier_U_in_bezier_U",
                     []() {
                         std::vector<std::unique_ptr<RS_EntityContainer>> loops;
                         loops.push_back(bezU(0, 0, 300, 300, 100));
                         loops.push_back(bezU(20, 80, 260, 200, 20));
                         return loops;
                     },
                     1, {0, 1}, 53066.666667, 1e-3,
                     {{50, 250, true}, {30, 200, false}}});
    cases.push_back({"N1_sliver_island",
                     []() { return loopsFromRings({lc::test::rectRing(0, 0, 300, 300),
                                                    {RS_Vector(10, 1e-4),
                                                     RS_Vector(290, 1e-4),
                                                     RS_Vector(290, 1.1e-3),
                                                     RS_Vector(10, 1.1e-3)}}); },
                     1, {0, 1}, 89999.72, 1e-4,
                     {{150, 0.0006, false}}});
    // N4: a tangent island whose radius is computed exactly like 0.1*34*3 so
    // the bbox overshoots y = 0 by 1.8e-15. The master's exact bbox-in-bbox
    // check rejects it; the eps-slack check accepts.
    cases.push_back({"N4_tangent_island_bbox_ulp",
                     []() {
                         std::vector<std::unique_ptr<RS_EntityContainer>> loops;
                         loops.push_back(polyContainer(lc::test::rectRing(0, 0, 300, 300)));
                         const double r = 0.1 * 34.0 * 3.0;
                         loops.push_back(circleContainer(RS_Vector(150.0, 10.2), r));
                         return loops;
                     },
                     1, {0, 1}, 89673.1487, 1e-4,
                     {{150, 150, true}, {150, 5.1, false}}});
    return cases;
}

/** Walk LoopSorter's tree, collecting the depth of every node. */
void collectDepths(const LC_LoopUtils::LC_Loops& node, int depth,
                   std::vector<int>& depths) {
    depths.push_back(depth);
    for (const auto& child : node.getChildren()) {
        collectDepths(child, depth + 1, depths);
    }
}

/** Build an LC_Loops tree from a case's loop containers. */
std::shared_ptr<std::vector<LC_LoopUtils::LC_Loops>>
sorterTree(std::vector<std::unique_ptr<RS_EntityContainer>> loops) {
    LC_LoopUtils::LoopSorter sorter(std::move(loops));
    return sorter.getResults();
}

/** Classify a probe point by walking the LoopSorter tree. */
LC_LoopUtils::PointClass classifyProbe(
    const std::vector<LC_LoopUtils::LC_Loops>& roots, const RS_Vector& p) {
    // Odd depth of containing nodes = Inside. The classifier returns On if
    // the point lies within tolerance of any boundary edge; our probe points
    // avoid boundaries, so a non-Outside answer is accepted as Inside.
    for (const LC_LoopUtils::LC_Loops& root : roots) {
        const auto cls = root.classify(p);
        if (cls != LC_LoopUtils::PointClass::Outside) {
            return cls;
        }
    }
    return LC_LoopUtils::PointClass::Outside;
}

double totalArea(const std::vector<LC_LoopUtils::LC_Loops>& roots) {
    double a = 0.0;
    for (const auto& r : roots) a += r.getTotalArea();
    return a;
}

// NOLINTEND(readability-identifier-naming)

} // namespace

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("Island nesting by a strictly interior point: the 24 cases",
          "[lc-hatchfix][nesting]") {
    const CaseList cases = buildCases();
    const auto idx = GENERATE_COPY(Catch::Generators::range<std::size_t>(
        0, cases.size()));
    const Case& c = cases[idx];
    CAPTURE(c.name);
    auto loops = c.build();
    auto roots = sorterTree(std::move(loops));
    REQUIRE(roots);
    REQUIRE(static_cast<int>(roots->size()) == c.roots);

    std::vector<int> depths;
    for (const auto& r : *roots) collectDepths(r, 0, depths);
    std::sort(depths.begin(), depths.end());
    std::vector<int> expected = c.depths;
    std::sort(expected.begin(), expected.end());
    REQUIRE(depths == expected);

    REQUIRE_THAT(totalArea(*roots), WithinAbs(c.area, c.areaTol));

    for (const ProbePoint& p : c.probes) {
        const auto cls = classifyProbe(*roots, RS_Vector(p.x, p.y));
        INFO("probe (" << p.x << ", " << p.y << ") expect "
             << (p.fill ? "fill" : "hole"));
        if (p.fill) {
            REQUIRE(cls == LC_LoopUtils::PointClass::Inside);
        } else {
            REQUIRE(cls == LC_LoopUtils::PointClass::Outside);
        }
    }
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("N4: a tangent island whose bbox overshoots by 1.8e-15 is still a child",
          "[lc-hatchfix][nesting]") {
    // N4 requires that the circle's radius be computed in C++ exactly like
    // 0.1 * 34 * 3; a textual 10.2 does not reproduce the ULP overshoot.
    const double r = 0.1 * 34.0 * 3.0;
    REQUIRE(r > 10.2);
    REQUIRE(r - 10.2 < 1e-14);

    std::vector<std::unique_ptr<RS_EntityContainer>> loops;
    loops.push_back(polyContainer(lc::test::rectRing(0, 0, 300, 300)));
    loops.push_back(circleContainer(RS_Vector(150.0, 10.2), r));
    auto roots = sorterTree(std::move(loops));
    REQUIRE(roots);
    REQUIRE(roots->size() == 1u);

    std::vector<int> depths;
    for (const auto& x : *roots) collectDepths(x, 0, depths);
    std::sort(depths.begin(), depths.end());
    REQUIRE(depths == std::vector<int>({0, 1}));
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("Scan point searches all gaps", "[lc-hatchfix][nesting]") {
    // comb(k, w): k teeth each 1 wide separated by a narrow gap of width w.
    // The three widest gaps between adjacent teeth are of width w; the real
    // y-gap of width ~2 lies outside the teeth. A 3-gap scan would never
    // choose that outside gap, so the interior point lands in a slot of
    // width w. The all-gap scan prefers the outer gap and gives clear ~1.
    auto comb = [](int k, double w) {
        auto cont = std::make_unique<RS_EntityContainer>(nullptr, true);
        // Outer rectangle of height 2, width = teeth from x=0 to x=10 with a
        // 2-unit border on each side.
        const double xmin = -2.0;
        const double xmax = 10.0 + 2.0;
        const double ytop = 4.0;
        // Teeth sit on y = 0..1 at xi = i*(1 + w) for i in 0..k.
        // Build the outline as: bottom at y=0 across the whole base, up the
        // right side to ytop, back along the top, down to y=0 on the left.
        // The teeth are built as holes inside this outer rectangle (holes of
        // thickness 2 wide and tall from y=0..3 separated by w).
        Ring outer{RS_Vector(xmin, 0), RS_Vector(xmax, 0),
                   RS_Vector(xmax, ytop), RS_Vector(xmin, ytop)};
        for (size_t i = 0; i < outer.size(); ++i) {
            cont->addEntity(new RS_Line(
                cont.get(), outer[i], outer[(i + 1) % outer.size()]));
        }
        for (int i = 0; i < k; ++i) {
            const double x0 = i * (1.0 + w);
            Ring tooth{RS_Vector(x0, 1.0), RS_Vector(x0 + 1.0, 1.0),
                       RS_Vector(x0 + 1.0, 3.0), RS_Vector(x0, 3.0)};
            for (size_t j = 0; j < tooth.size(); ++j) {
                cont->addEntity(new RS_Line(
                    cont.get(), tooth[j], tooth[(j + 1) % tooth.size()]));
            }
        }
        return cont;
    };

    const double w = 1e-6;
    // Build the outer-with-teeth as one container, extract its loops.
    auto cont = comb(4, w);
    LC_LoopUtils::LoopExtractor ex(*cont);
    auto extracted = ex.extractLoops();
    REQUIRE(!extracted.empty());
    for (auto& el : extracted) {
        auto& vs = el.vertices;
        if (vs.empty()) continue;
        auto g = LC_LoopUtils::LoopGeometry::fromChain(*el.edges, vs);
        const RS_Vector p = LC_LoopUtils::interiorPoint(*g);
        REQUIRE(p.valid);
        // The chosen point must be strictly inside and have an exact-parity
        // odd crossing of the geometry.
        const std::vector<const LC_LoopUtils::LoopGeometry*> self{g.get()};
        REQUIRE(LC_LoopUtils::oddParity(p, self));
    }
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("Duplicate loops: the lower index parents",
          "[lc-hatchfix][nesting]") {
    // N2 — two identical RECT loops in two containers. The lower-index twin
    // parents the higher-index one, giving depths {0, 1}, area 0 (±1e-9) and
    // countLoops == 1.
    std::vector<std::unique_ptr<RS_EntityContainer>> loops;
    loops.push_back(polyContainer(lc::test::rectRing(0, 0, 300, 300)));
    loops.push_back(polyContainer(lc::test::rectRing(0, 0, 300, 300)));
    auto roots = sorterTree(std::move(loops));
    REQUIRE(roots);
    REQUIRE(roots->size() == 1u);

    std::vector<int> depths;
    for (const auto& r : *roots) collectDepths(r, 0, depths);
    std::sort(depths.begin(), depths.end());
    REQUIRE(depths == std::vector<int>({0, 1}));

    REQUIRE_THAT(totalArea(*roots), WithinAbs(0.0, 1e-9));
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("Tree does not depend on the pattern angle",
          "[lc-hatchfix][nesting]") {
    // The frame-independent nesting lemma: the tree of an L wall and a U-in-U
    // is the same at every hatch angle. Area is rotation-invariant; on master
    // the ill-chosen bbox-centre test gives 4100 instead of 1500 at angles
    // 0 / 30 / 90 degrees for the L wall.
    lc::test::SourcePatterns patterns;

    auto buildAt = [](const char* pattern, double angle,
                      const std::vector<Ring>& rings) {
        return lc::test::makePatternHatch(QString::fromUtf8(pattern), 1.0,
                                          angle, rings);
    };

    const double angles[] = {0.0, M_PI / 6.0, M_PI / 4.0,
                              M_PI / 3.0, M_PI / 2.0,
                              140.0 * M_PI / 180.0};

    // L wall: outer, with inner U-shaped hole.
    const Ring lOuter{RS_Vector(0, 0),   RS_Vector(100, 0),  RS_Vector(100, 20),
                      RS_Vector(20, 20), RS_Vector(20, 60),  RS_Vector(0, 60)};
    const Ring lInner{RS_Vector(5, 5),   RS_Vector(95, 5),   RS_Vector(95, 15),
                      RS_Vector(15, 15), RS_Vector(15, 55),  RS_Vector(5, 55)};
    const std::vector<Ring> lWall{lOuter, lInner};

    for (double a : angles) {
        INFO("L wall angle " << a);
        auto h = buildAt("ANSI31", a, lWall);
        REQUIRE_THAT(h->getTotalArea(), WithinAbs(1500.0, 1e-9));
    }

    // U-in-U: area is OUTER - INNER (both CCW), invariant under rotation.
    const Ring uuOuter{RS_Vector(0, 0),   RS_Vector(100, 0),   RS_Vector(100, 60),
                       RS_Vector(70, 60), RS_Vector(70, 20),   RS_Vector(30, 20),
                       RS_Vector(30, 60), RS_Vector(0, 60)};
    const Ring uuInner{RS_Vector(5, 5),   RS_Vector(95, 5),   RS_Vector(95, 55),
                       RS_Vector(75, 55), RS_Vector(75, 15),  RS_Vector(25, 15),
                       RS_Vector(25, 55), RS_Vector(5, 55)};
    const std::vector<Ring> uInU{uuOuter, uuInner};
    const auto hSolid = buildAt("SOLID", 0.0, uInU);
    const double uuArea = hSolid->getTotalArea();
    REQUIRE(uuArea > 0.0);
    for (double a : angles) {
        INFO("U-in-U angle " << a);
        auto h = buildAt("ANSI31", a, uInU);
        REQUIRE_THAT(h->getTotalArea(), WithinAbs(uuArea, 1e-9));
    }

    // L wall with a 140-unit outer leg: area invariant under rotation.
    const Ring lOuter2{RS_Vector(0, 0),   RS_Vector(140, 0),  RS_Vector(140, 20),
                       RS_Vector(20, 20), RS_Vector(20, 140), RS_Vector(0, 140)};
    const std::vector<Ring> lWall2{lOuter2};
    const auto hL2 = buildAt("SOLID", 0.0, lWall2);
    const double lArea = hL2->getTotalArea();
    for (double a : angles) {
        INFO("L wall 140 angle " << a);
        auto h = buildAt("ANSI31", a, lWall2);
        REQUIRE_THAT(h->getTotalArea(), WithinAbs(lArea, 1e-6));
    }
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("Nesting is identical when built 100 times",
          "[lc-hatchfix][nesting]") {
    const CaseList cases = buildCases();
    // Three representative cases: U-in-U, 3-level and the duplicate-N4 ULP.
    const char* names[] = {"A_U_in_U", "H_3level", "N4_tangent_island_bbox_ulp"};
    for (const char* nm : names) {
        auto match = std::find_if(cases.begin(), cases.end(),
                                   [nm](const Case& c) {
                                       return std::string(c.name) == nm;
                                   });
        REQUIRE(match != cases.end());
        std::vector<int> firstDepths;
        double firstArea = 0.0;
        for (int iter = 0; iter < 100; ++iter) {
            auto loops = match->build();
            auto roots = sorterTree(std::move(loops));
            REQUIRE(roots);
            std::vector<int> depths;
            for (const auto& r : *roots) collectDepths(r, 0, depths);
            std::sort(depths.begin(), depths.end());
            const double area = totalArea(*roots);
            if (iter == 0) {
                firstDepths = depths;
                firstArea = area;
            } else {
                INFO(nm << " iteration " << iter);
                REQUIRE(depths == firstDepths);
                REQUIRE(area == firstArea);
            }
        }
    }
}

// --- Pinched-loop (figure-8) splitting --------------------------------------
// The research oracle: a 7-line chain that forms a square sharing a single
// vertex with a triangle must extract as a 4-edge loop and a 3-edge loop,
// never as one 7-edge figure-8. Earlier builds merged the chain in 400 of
// 896 edge-orientation plus traversal-order variants; after only the
// oriented-start fix landed, 112 of 128 still merged. The extractor now
// splits the chain at every repeated start vertex.

namespace {

// NOLINTBEGIN(readability-identifier-naming)

/** The 7 lines of the figure-8 oracle (lobe1 = square, lobe2 = triangle). */
std::array<std::pair<RS_Vector, RS_Vector>, 7> figure8Edges() {
    return {{
        {RS_Vector(0, 0),    RS_Vector(100, 0)},
        {RS_Vector(100, 0),  RS_Vector(100, 100)},
        {RS_Vector(100, 100), RS_Vector(200, 150)},
        {RS_Vector(200, 150), RS_Vector(150, 200)},
        {RS_Vector(150, 200), RS_Vector(100, 100)},
        {RS_Vector(100, 100), RS_Vector(0, 100)},
        {RS_Vector(0, 100),  RS_Vector(0, 0)},
    }};
}

/** Build a container of 7 lines with per-edge stored orientation `rev[i]`
 *  (true = swap start/end) and container order given by `order`. */
std::unique_ptr<RS_EntityContainer> buildFigure8(
    const std::array<std::pair<RS_Vector, RS_Vector>, 7>& edges,
    const std::array<bool, 7>& rev,
    const std::array<int, 7>& order) {
    auto cont = std::make_unique<RS_EntityContainer>(nullptr, true);
    for (int i : order) {
        const auto& p = edges[i];
        const RS_Vector a = rev[i] ? p.second : p.first;
        const RS_Vector b = rev[i] ? p.first : p.second;
        cont->addEntity(new RS_Line(cont.get(), a, b));
    }
    return cont;
}

// NOLINTEND(readability-identifier-naming)

} // namespace

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("Pinched lobes never extract as one figure-8 loop",
          "[lc-hatchfix][nesting]") {
    // 2^7 stored-orientation variants x 7 rotations of the container order
    // = 896 configurations. In every one the extractor must give loops of
    // 4 and 3 edges, never 7. Master merges in 400/896.
    const auto edges = figure8Edges();
    std::array<int, 7> baseOrder = {0, 1, 2, 3, 4, 5, 6};
    int merged = 0;
    for (int mask = 0; mask < 128; ++mask) {
        std::array<bool, 7> rev{};
        for (int b = 0; b < 7; ++b) rev[b] = (mask >> b) & 1;
        for (int rot = 0; rot < 7; ++rot) {
            std::array<int, 7> order{};
            for (int i = 0; i < 7; ++i) order[i] = baseOrder[(i + rot) % 7];
            auto cont = buildFigure8(edges, rev, order);
            LC_LoopUtils::LoopExtractor ex(*cont);
            auto result = ex.extractLoops();
            CAPTURE(mask, rot);
            REQUIRE(result.size() >= 1u);
            // Collect edge counts across all extracted loops.
            std::vector<unsigned> counts;
            for (const auto& el : result) counts.push_back(el.edges->count());
            std::sort(counts.begin(), counts.end());
            if (counts.size() == 1 && counts[0] == 7u) {
                ++merged;
            }
            REQUIRE(counts != std::vector<unsigned>({7u}));
            REQUIRE(counts == std::vector<unsigned>({3u, 4u}));
        }
    }
    REQUIRE(merged == 0);
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("A figure-8 chain splits into two simple CCW loops",
          "[lc-hatchfix][nesting]") {
    // The research oracle chain (0,0),(0,1),(1,1),(1.5,2),(2,1.5),(1,1),(1,0).
    // Split at the repeated (1,1): the first lobe is the triangle
    // (1,1)-(1.5,2)-(2,1.5) with area 0.375 and the second lobe is the
    // quadrilateral (0,0)-(0,1)-(1,1)-(1,0) with area 1. Both are positive
    // once the extractor flips them to CCW.
    auto cont = std::make_unique<RS_EntityContainer>(nullptr, true);
    const RS_Vector pts[] = {
        RS_Vector(0, 0),   RS_Vector(0, 1),   RS_Vector(1, 1),
        RS_Vector(1.5, 2), RS_Vector(2, 1.5), RS_Vector(1, 1),
        RS_Vector(1, 0)};
    for (int i = 0; i < 7; ++i) {
        const RS_Vector a = pts[i];
        const RS_Vector b = pts[(i + 1) % 7];
        cont->addEntity(new RS_Line(cont.get(), a, b));
    }
    LC_LoopUtils::LoopExtractor ex(*cont);
    auto result = ex.extractLoops();
    REQUIRE(result.size() == 2u);
    std::vector<double> areas;
    for (const auto& el : result) {
        areas.push_back(el.edges->areaLineIntegral());
    }
    std::sort(areas.begin(), areas.end());
    REQUIRE_THAT(areas[0], WithinAbs(0.375, 1e-10));
    REQUIRE_THAT(areas[1], WithinAbs(1.0, 1e-10));
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("Pinched lobes in an L room: triangle lobe outside, square lobe a hole",
          "[lc-hatchfix][nesting]") {
    // Research scene: an L-shaped room Q plus the figure-8 chain. The square
    // lobe (0,0)-(100,100) is a hole inside the L room; the triangle lobe
    // (100,100),(200,150),(150,200) sits in the L's notch, outside the room,
    // and sorts as its own root. Total area 203750 at scale 100.
    const Ring lRoom{RS_Vector(-200, -200), RS_Vector(300, -200),
                     RS_Vector(300, 100),   RS_Vector(100, 100),
                     RS_Vector(100, 300),   RS_Vector(-200, 300)};

    std::vector<std::unique_ptr<RS_EntityContainer>> loops;
    loops.push_back(makeLoopContainer(lRoom));
    // The 7-line figure-8 chain in one container.
    auto chain = std::make_unique<RS_EntityContainer>(nullptr, true);
    const auto edges = figure8Edges();
    for (const auto& p : edges) {
        chain->addEntity(new RS_Line(chain.get(), p.first, p.second));
    }
    // Extract the chain's loops first and append them to loops.
    LC_LoopUtils::LoopExtractor ex(*chain);
    auto subLoops = ex.extract();
    for (auto& sl : subLoops) loops.push_back(std::move(sl));

    LC_LoopUtils::LoopSorter sorter(std::move(loops));
    auto roots = sorter.getResults();
    REQUIRE(roots);
    // Two roots: the L room (with the square lobe as its hole), and the
    // triangle lobe which lies in the L's notch.
    REQUIRE(roots->size() == 2u);

    std::vector<int> depths;
    for (const auto& r : *roots) collectDepths(r, 0, depths);
    std::sort(depths.begin(), depths.end());
    REQUIRE(depths == std::vector<int>({0, 0, 1}));

    double totArea = 0.0;
    for (const auto& r : *roots) totArea += r.getTotalArea();
    // L_room (210000) - square lobe (10000) + triangle lobe (3750) = 203750.
    REQUIRE_THAT(totArea, WithinAbs(203750.0, 1e-6));
}

namespace {

// Builds a loop container that mirrors how the filter writes a HATCH boundary
// with the polyline-boundary path type (bit 2): construct a transient RS_Polyline,
// addVertex every (point, bulge), setClosed, and clone each generated atomic
// entity into a fresh container that owns them.
std::unique_ptr<RS_EntityContainer> makePolylineLoopContainer(
    const std::vector<std::pair<RS_Vector, double>>& vertsWithBulge) {
    RS_PolylineData pd{RS_Vector(false), RS_Vector(false), 1};
    RS_Polyline polyline{nullptr, pd};
    for (const auto& vb : vertsWithBulge) {
        polyline.addVertex(vb.first, vb.second);
    }
    polyline.setClosed(true);
    auto cont = std::make_unique<RS_EntityContainer>(nullptr, true);
    for (const RS_Entity* e :
         lc::LC_ContainerTraverser{polyline, RS2::ResolveNone}.entities()) {
        RS_Entity* copy = e->clone();
        copy->setParent(cont.get());
        copy->setLayer(nullptr);
        cont->addEntity(copy);
    }
    return cont;
}

} // namespace

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("A bulge-1e-9 edge is a line and keeps its loop",
          "[lc-hatchfix][nesting]") {
    // A 10x10 polyline square at the origin; the right edge carries a tiny
    // bulge. 4*atan(1e-9) is below RS_TOLERANCE_ANGLE, so RS_Polyline::createVertex
    // must emit an RS_Line for that edge; the square keeps its four edges and
    // nests the thin bar at (2,2)-(8,2.00015) as its hole.
    //
    // On the parent build the near-flat bulge becomes an RS_Arc of radius
    // ~2.5e9; RS_ArcData::getAngleLength() rounds its sweep to 2*pi and
    // isClosed() takes it as a loop of its own (area ~pi*r^2 ~ 1.96e19) while
    // the remaining three edges fail to close.
    auto buildHatch = [](double bulge) {
        const std::vector<std::pair<RS_Vector, double>> verts{
            {RS_Vector(0.0, 0.0), 0.0},
            {RS_Vector(10.0, 0.0), bulge},  // right edge carries the bulge
            {RS_Vector(10.0, 10.0), 0.0},
            {RS_Vector(0.0, 10.0), 0.0},
        };
        auto outer = makePolylineLoopContainer(verts);
        auto bar = makeLoopContainer({
            RS_Vector(2.0, 2.0),           RS_Vector(8.0, 2.0),
            RS_Vector(8.0, 2.0 + 1.5e-4),  RS_Vector(2.0, 2.0 + 1.5e-4)});
        std::vector<std::unique_ptr<RS_EntityContainer>> loops;
        loops.push_back(std::move(outer));
        loops.push_back(std::move(bar));
        RS_HatchData data{true, 1.0, 0.0, QStringLiteral("SOLID")};
        return makeHatch(std::move(loops), data);
    };

    auto a = buildHatch(1.0e-9);
    auto b = buildHatch(0.0);

    REQUIRE(a->getUpdateError() == RS_Hatch::HATCH_OK);
    REQUIRE(b->getUpdateError() == RS_Hatch::HATCH_OK);
    REQUIRE(a->countAllLoops() == 2);
    REQUIRE(b->countAllLoops() == 2);
    // The outer container of the bulge hatch must come out as four line edges,
    // never as a nearly-full RS_Arc that gets taken for a closed loop.
    const RS_EntityContainer* aRoot = a->getBoundaryContainer(0);
    REQUIRE(aRoot != nullptr);
    REQUIRE(aRoot->count() == 4u);
    for (unsigned i = 0; i < aRoot->count(); ++i) {
        REQUIRE(aRoot->entityAt(i)->rtti() == RS2::EntityLine);
    }
    REQUIRE_THAT(a->getTolerance(), WithinRel(b->getTolerance(), 1e-12));
    // Square area 100 minus bar area 6 * 1.5e-4 = 9e-4.
    const double expected = 100.0 - 9.0e-4;
    REQUIRE_THAT(a->getTotalArea(), WithinAbs(expected, 1e-6));
    REQUIRE_THAT(b->getTotalArea(), WithinAbs(expected, 1e-6));
}

namespace {

// Build a container of RS_Line edges from a vector of vertex coordinates
// (the ring is closed by the last-to-first edge).
std::unique_ptr<RS_EntityContainer> makeNoisyPolygonLoop(
    const std::vector<RS_Vector>& vertices) {
    auto cont = std::make_unique<RS_EntityContainer>(nullptr, true);
    const std::size_t n = vertices.size();
    for (std::size_t i = 0; i < n; ++i) {
        cont->addEntity(new RS_Line(cont.get(), vertices[i], vertices[(i + 1) % n]));
    }
    return cont;
}

} // namespace

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("A 50-junction polygon with 1e-10 end noise always closes",
          "[lc-hatchfix][nesting]") {
    // Regular 50-gon of radius 100 at the origin. For each of 1000 seeds,
    // translate the polygon by a uniform offset in [0, 1e-6)^2 and move every
    // edge end by 1e-10 in a random direction (independent per end). The
    // extractor must still see one closed loop; master's exact 1e-8 key loses
    // around 47% of such loops.
    constexpr int kJunctions = 50;
    constexpr int kSeeds = 1000;
    constexpr double kNoise = 1.0e-10;
    constexpr double kOffsetScale = 1.0e-6;

    std::vector<RS_Vector> base;
    base.reserve(kJunctions);
    for (int i = 0; i < kJunctions; ++i) {
        const double a = 2.0 * M_PI * i / kJunctions;
        base.emplace_back(100.0 * std::cos(a), 100.0 * std::sin(a));
    }

    int closed = 0;
    for (int seed = 0; seed < kSeeds; ++seed) {
        std::mt19937 gen(static_cast<unsigned>(seed) + 1u);
        std::uniform_real_distribution<double> offs(0.0, kOffsetScale);
        std::uniform_real_distribution<double> ang(0.0, 2.0 * M_PI);
        const RS_Vector offset(offs(gen), offs(gen));
        std::vector<RS_Vector> perturbed(kJunctions);
        for (int i = 0; i < kJunctions; ++i) {
            const double theta = ang(gen);
            perturbed[i] = base[i] + offset
                           + RS_Vector(kNoise * std::cos(theta),
                                       kNoise * std::sin(theta));
        }
        auto cont = makeNoisyPolygonLoop(perturbed);
        LC_LoopUtils::LoopOptimizer opt(*cont);
        auto roots = opt.getResults();
        if (roots && roots->size() == 1) {
            ++closed;
        }
    }
    REQUIRE(closed == kSeeds);
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("D = 0.5 square with 2e-9 junction noise is kept",
          "[lc-hatchfix][nesting]") {
    // Small square whose diagonal 0.5 would make a strict scale-relative
    // tolerance 1e-9 * D = 5e-10, below the 2e-9 junction noise. The eps
    // floor min(1e-8, 1e-6*D) widens the tolerance to 1e-8 and keeps the
    // loop. Checked at the origin and at (100, 100); each over 1000 seeds.
    constexpr int kSeeds = 1000;
    constexpr double kSide = 0.5 / M_SQRT2;  // diagonal 0.5
    constexpr double kNoise = 2.0e-9;

    auto run = [&](const RS_Vector& origin, double noise) {
        int closed = 0;
        for (int seed = 0; seed < kSeeds; ++seed) {
            std::mt19937 gen(static_cast<unsigned>(seed) + 7u);
            std::uniform_real_distribution<double> ang(0.0, 2.0 * M_PI);
            std::vector<RS_Vector> corners{origin,
                                            origin + RS_Vector(kSide, 0.0),
                                            origin + RS_Vector(kSide, kSide),
                                            origin + RS_Vector(0.0, kSide)};
            std::vector<RS_Vector> jiggled(4);
            for (int i = 0; i < 4; ++i) {
                const double theta = ang(gen);
                jiggled[i] = corners[i]
                             + RS_Vector(noise * std::cos(theta),
                                         noise * std::sin(theta));
            }
            auto cont = makeNoisyPolygonLoop(jiggled);
            LC_LoopUtils::LoopOptimizer opt(*cont);
            auto roots = opt.getResults();
            if (roots && roots->size() == 1) {
                ++closed;
            }
        }
        return closed;
    };

    SECTION("origin, 2e-9 noise") {
        REQUIRE(run(RS_Vector(0.0, 0.0), kNoise) == kSeeds);
    }
    SECTION("far from origin, 2e-9 noise") {
        REQUIRE(run(RS_Vector(100.0, 100.0), kNoise) == kSeeds);
    }
    SECTION("origin, 5e-9 noise") {
        REQUIRE(run(RS_Vector(0.0, 0.0), 5.0e-9) == kSeeds);
    }
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("A bulge-1e-7 edge neither drops the loop nor loosens eps",
          "[lc-hatchfix][nesting]") {
    // Hatch A: a 10x10 polyline square whose right edge has bulge 1e-7. The
    // sweep 4 * atan(1e-7) ~ 4e-7 is above the near-flat threshold, so the
    // polyline builds an RS_Arc of radius ~2.5e7. Master's 1e-8 endpoint key
    // loses the join between that arc and the top edge. The local arc
    // tolerance max(eps, 64*u*(|c|+r)) ~ 3.6e-7 welds that gap and keeps the
    // square.
    //
    // Hatch B: the same hatch without the bulge. Both must have the same
    // per-hatch tolerance (the bulge contributes to M only through its chord
    // ends, which coincide with the two neighbouring vertices).
    //
    // The thin bar at (2, 2) - (8, 2.00015) is the inner island.
    auto buildHatch = [](double bulge) {
        const std::vector<std::pair<RS_Vector, double>> verts{
            {RS_Vector(0.0, 0.0), 0.0},
            {RS_Vector(10.0, 0.0), bulge},
            {RS_Vector(10.0, 10.0), 0.0},
            {RS_Vector(0.0, 10.0), 0.0},
        };
        auto outer = makePolylineLoopContainer(verts);
        auto bar = makeLoopContainer({
            RS_Vector(2.0, 2.0),           RS_Vector(8.0, 2.0),
            RS_Vector(8.0, 2.0 + 1.5e-4),  RS_Vector(2.0, 2.0 + 1.5e-4)});
        std::vector<std::unique_ptr<RS_EntityContainer>> loops;
        loops.push_back(std::move(outer));
        loops.push_back(std::move(bar));
        RS_HatchData data{true, 1.0, 0.0, QStringLiteral("SOLID")};
        return makeHatch(std::move(loops), data);
    };

    auto a = buildHatch(1.0e-7);
    auto b = buildHatch(0.0);

    REQUIRE(a->getUpdateError() == RS_Hatch::HATCH_OK);
    REQUIRE(b->getUpdateError() == RS_Hatch::HATCH_OK);
    REQUIRE(a->countAllLoops() == 2);
    REQUIRE(b->countAllLoops() == 2);
    // A's and B's bbox diagonals differ only by the arc's sagitta
    // (~5e-7 for a 10-unit chord with |b| = 1e-7); the per-hatch tolerance
    // picks up that fraction of 1e-9 * D. Practical equality on the 1e-8
    // scale is well within 1e-12 absolute.
    REQUIRE_THAT(a->getTolerance(), WithinAbs(b->getTolerance(), 1e-12));

    // The right edge is now an RS_Arc; the outer boundary still has 4 edges.
    const RS_EntityContainer* aRoot = a->getBoundaryContainer(0);
    REQUIRE(aRoot != nullptr);
    REQUIRE(aRoot->count() == 4u);
    bool sawArc = false;
    for (unsigned i = 0; i < aRoot->count(); ++i) {
        if (aRoot->entityAt(i)->rtti() == RS2::EntityArc) sawArc = true;
    }
    REQUIRE(sawArc);

    // A - B equals the signed segment area of the arc; for b > 0 on a CCW
    // square's right edge the segment bulges outward (positive). Compute it
    // from the RS_Arc the polyline built: segment = r^2 / 2 * (theta - sin theta).
    const double bulge = 1.0e-7;
    const double theta = 4.0 * std::atan(bulge);
    const double chord = 10.0;
    const double radius = std::abs(chord / (2.0 * std::sin(theta / 2.0)));
    const double segment = 0.5 * radius * radius * (theta - std::sin(theta));
    const double delta = a->getTotalArea() - b->getTotalArea();
    REQUIRE_THAT(delta, WithinAbs(segment, 3.0e-7));
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("An edge shorter than eps is dropped, one of 10 eps is kept",
          "[lc-hatchfix][nesting]") {
    // Square (0, 0) - (10, 10) whose bottom edge is split into three pieces by
    // a tiny midpoint displacement delta. delta = 0.5*eps -> the middle piece
    // is shorter than eps and the extractor drops it, leaving a 4-edge loop
    // with area 100. delta = 10*eps -> the piece is kept and the loop has 6
    // edges. eps for a square of diagonal ~14 at the origin is the 1e-8 floor,
    // so delta = 5e-9 and 1e-7 are the two cases.
    const double eps = 1.0e-8;

    auto buildHatch = [eps](double delta) {
        const double mid = 5.0 + delta;
        auto outer = std::make_unique<RS_EntityContainer>(nullptr, true);
        outer->addEntity(new RS_Line(outer.get(), RS_Vector(0, 0), RS_Vector(5, 0)));
        outer->addEntity(new RS_Line(outer.get(), RS_Vector(5, 0), RS_Vector(mid, 0)));
        outer->addEntity(new RS_Line(outer.get(), RS_Vector(mid, 0), RS_Vector(10, 0)));
        outer->addEntity(new RS_Line(outer.get(), RS_Vector(10, 0), RS_Vector(10, 10)));
        outer->addEntity(new RS_Line(outer.get(), RS_Vector(10, 10), RS_Vector(0, 10)));
        outer->addEntity(new RS_Line(outer.get(), RS_Vector(0, 10), RS_Vector(0, 0)));
        std::vector<std::unique_ptr<RS_EntityContainer>> loops;
        loops.push_back(std::move(outer));
        RS_HatchData data{true, 1.0, 0.0, QStringLiteral("SOLID")};
        return makeHatch(std::move(loops), data);
    };

    SECTION("delta = 0.5 * eps") {
        auto h = buildHatch(0.5 * eps);
        REQUIRE(h->getUpdateError() == RS_Hatch::HATCH_OK);
        REQUIRE(h->countAllLoops() == 1);
        REQUIRE(h->getBoundaryContainer(0)->count() == 5u);
        REQUIRE_THAT(h->getTotalArea(), WithinAbs(100.0, 1e-6));
    }
    SECTION("delta = 10 * eps") {
        auto h = buildHatch(10.0 * eps);
        REQUIRE(h->getUpdateError() == RS_Hatch::HATCH_OK);
        REQUIRE(h->countAllLoops() == 1);
        REQUIRE(h->getBoundaryContainer(0)->count() == 6u);
        REQUIRE_THAT(h->getTotalArea(), WithinAbs(100.0, 1e-6));
    }
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("Distinct vertices 10 eps apart stay distinct",
          "[lc-hatchfix][nesting]") {
    // Two 1x1 squares that touch at their nearest corners, offset by 10 eps
    // (1e-7) along the join axis. The welder must not merge the corners even
    // under the loose rule; the hatch gets two root loops with area 1 each.
    const double gap = 1.0e-7;

    auto left = makeLoopContainer(lc::test::rectRing(0.0, 0.0, 1.0, 1.0));
    auto right = makeLoopContainer(
        lc::test::rectRing(1.0 + gap, 0.0, 2.0 + gap, 1.0));

    std::vector<std::unique_ptr<RS_EntityContainer>> loops;
    loops.push_back(std::move(left));
    loops.push_back(std::move(right));
    RS_HatchData data{true, 1.0, 0.0, QStringLiteral("SOLID")};
    auto hatch = makeHatch(std::move(loops), data);

    REQUIRE(hatch->getUpdateError() == RS_Hatch::HATCH_OK);
    REQUIRE(hatch->countLoops() == 2);
    REQUIRE(hatch->countAllLoops() == 2);
    REQUIRE_THAT(hatch->getTotalArea(), WithinAbs(2.0, 1e-10));
}

namespace {

// Apply (scale, rotate about origin, translate) to every loop container in
// place. Transform order follows the Case description: scale first, rotate
// second, move last. Each container's children are RS_Entity atomics
// (RS_Line, RS_Arc, RS_Circle, RS_Ellipse, ...) which handle their own
// endpoint recomputation.
void transformLoops(std::vector<std::unique_ptr<RS_EntityContainer>>& loops,
                    double scale, double rotateDeg, const RS_Vector& offset) {
    const RS_Vector origin(0.0, 0.0);
    const double theta = rotateDeg * M_PI / 180.0;
    for (auto& loop : loops) {
        if (!loop) continue;
        if (scale != 1.0) {
            loop->scale(origin, RS_Vector(scale, scale));
        }
        if (std::abs(theta) > 0.0) {
            loop->rotate(origin, theta);
        }
        if (offset.x != 0.0 || offset.y != 0.0) {
            loop->move(offset);
        }
        loop->forcedCalculateBorders();
    }
}

} // namespace

namespace {

// Build a polyline-boundary loop container from an inline list of
// (point, bulge) tuples. The polyline is closed and its atomic edges are
// cloned into a fresh owning container, matching how the DXF filter writes
// a hatch boundary with the polyline bit set.
std::unique_ptr<RS_EntityContainer> makeBulgePolylineLoop(
    const std::vector<std::tuple<double, double, double>>& pts) {
    std::vector<std::pair<RS_Vector, double>> vs;
    vs.reserve(pts.size());
    for (const auto& p : pts) {
        vs.emplace_back(RS_Vector(std::get<0>(p), std::get<1>(p)), std::get<2>(p));
    }
    return makePolylineLoopContainer(vs);
}

} // namespace

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("Bulge paths far from the origin keep their loops",
          "[lc-hatchfix][nesting]") {
    // Two real-world hatch boundaries drawn with bulges, replayed at the
    // origin, at (5e5, 4e6) and at (3e7, 3e7). The welder's scale-relative
    // tolerance keeps the chain closed at every location; the master build's
    // fixed 1e-8 key loses one of these paths at D = 1e4 or above.
    //
    // The hatch-6 path has one closed boundary (5 vertices, 2 bulged).
    // The hatch-7 path has an outer rectangle (closed) plus an inner mixed
    // boundary (5 vertices, 3 bulged); the inner one nests inside the outer.
    auto buildHatch6 = []() {
        return makeBulgePolylineLoop({
            {90.52467476398633, 25.2544690703545, 0.0},
            {95.50272119367489, 25.2544690703545, 0.0712515982017834},
            {94.76722237490561, 27.39219767398249, 1.507885615214488},
            {92.5559157660329, 30.05747155033201, 0.0734835535140569},
            {90.52467476398633, 31.20054570052284, 0.0},
        });
    };
    auto buildHatch7Rect = []() {
        return makeBulgePolylineLoop({
            {116.8535739341333, 34.34631086375322, 0.0},
            {102.7338310135901, 34.34631086375322, 0.0},
            {102.7338310135901, 21.59331610156323, 0.0},
            {116.8535739341333, 21.59331610156323, 0.0},
        });
    };
    auto buildHatch7Inner = []() {
        return makeBulgePolylineLoop({
            {106.6036874636791, 31.35483848998607, -0.0734835535140573},
            {108.6349284657257, 30.21176433979524, -1.507885615214487},
            {110.8462350745984, 27.54649046344571, -0.0712515982017835},
            {111.5817338933677, 25.40876185981773, 0.0},
            {106.6036874636791, 25.40876185981773, 0.0},
        });
    };

    const std::vector<RS_Vector> offsets{
        RS_Vector(0, 0),
        RS_Vector(5.0e5, 4.0e6),
        RS_Vector(3.0e7, 3.0e7),
    };
    SECTION("hatch 6: one closed boundary") {
        for (const RS_Vector& off : offsets) {
            CAPTURE(off.x, off.y);
            std::vector<std::unique_ptr<RS_EntityContainer>> loops;
            loops.push_back(buildHatch6());
            if (off.x != 0.0 || off.y != 0.0) {
                for (auto& l : loops) {
                    l->move(off);
                    l->forcedCalculateBorders();
                }
            }
            RS_HatchData data{true, 1.0, 0.0, QStringLiteral("SOLID")};
            auto hatch = makeHatch(std::move(loops), data);
            REQUIRE(hatch->getUpdateError() == RS_Hatch::HATCH_OK);
            REQUIRE(hatch->countAllLoops() == 1);
        }
    }
    SECTION("hatch 7: rectangle with an inner island") {
        for (const RS_Vector& off : offsets) {
            CAPTURE(off.x, off.y);
            std::vector<std::unique_ptr<RS_EntityContainer>> loops;
            loops.push_back(buildHatch7Rect());
            loops.push_back(buildHatch7Inner());
            if (off.x != 0.0 || off.y != 0.0) {
                for (auto& l : loops) {
                    l->move(off);
                    l->forcedCalculateBorders();
                }
            }
            RS_HatchData data{true, 1.0, 0.0, QStringLiteral("SOLID")};
            auto hatch = makeHatch(std::move(loops), data);
            REQUIRE(hatch->getUpdateError() == RS_Hatch::HATCH_OK);
            REQUIRE(hatch->countAllLoops() == 2);
        }
    }
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("Loop counts and areas are scale and rotation invariant",
          "[lc-hatchfix][nesting]") {
    // Replay the 24 island-nesting cases through four frames: scale 1e-4, scale 1e4,
    // rotate 30 deg about the origin, and translate to (3e7, 3e7). Loop
    // counts and depths must match the as-stored case; area in the three
    // origin-based frames is identical to the original up to the arithmetic
    // of each primitive (relative 1e-9 is well within the per-hatch
    // tolerance of each frame). The translated frame is kept to show that
    // the id-based attach handles the shifted M even where the floor
    // 1e-6*D no longer dominates.
    const CaseList cases = buildCases();
    const auto idx = GENERATE_COPY(Catch::Generators::range<std::size_t>(
        0, cases.size()));
    const Case& c = cases[idx];

    struct Frame {
        const char* name;
        double scale;
        double rotateDeg;
        RS_Vector offset;
        double areaFactor;        // scaled area = baseArea * areaFactor
        bool checkAbs;            // enforce area oracle in this frame
    };
    const std::vector<Frame> frames{
        {"scale_1e-4",   1.0e-4,  0.0,  RS_Vector(0, 0),       1.0e-8,  true},
        {"scale_1e4",    1.0e4,   0.0,  RS_Vector(0, 0),       1.0e8,   true},
        {"rotate_30",    1.0,     30.0, RS_Vector(0, 0),       1.0,     true},
        {"far_offset",   1.0,     0.0,  RS_Vector(3.0e7, 3.0e7), 1.0,  false},
    };

    for (const Frame& f : frames) {
        CAPTURE(c.name, f.name);
        auto loops = c.build();
        transformLoops(loops, f.scale, f.rotateDeg, f.offset);

        auto roots = sorterTree(std::move(loops));
        REQUIRE(roots);
        REQUIRE(static_cast<int>(roots->size()) == c.roots);

        std::vector<int> depths;
        for (const auto& r : *roots) collectDepths(r, 0, depths);
        std::sort(depths.begin(), depths.end());
        std::vector<int> expected = c.depths;
        std::sort(expected.begin(), expected.end());
        REQUIRE(depths == expected);

        if (f.checkAbs) {
            const double expectedArea = c.area * f.areaFactor;
            // Allow a few ULPs of the expected area to absorb per-primitive
            // rotation/scale arithmetic; the as-stored tolerance was 1e-6.
            REQUIRE_THAT(totalArea(*roots),
                         WithinRel(expectedArea, 1.0e-9));
        }
    }
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("Welding at large D merges near-touching contours safely",
          "[lc-hatchfix][nesting]") {
    // Two squares in one container at D = 1e4, pinched at a single corner
    // where the two near-touching vertices sit 0.5 * eps = 5e-6 apart (the
    // join tolerance at D = 1e4 is 2e-5 = 1e-9 * D). The welder joins them
    // into a figure-8 chain; the extractor's max-left turn plus split-at-
    // repeat produces two simple loops whose total area equals the sum of
    // the two squares.
    const double side = 1.0e4;
    const double gap = 5.0e-6;  // << eps = 2e-5 at D = 2*side.

    auto cont = std::make_unique<RS_EntityContainer>(nullptr, true);
    // Left square (0, 0)-(side, side), CCW.
    cont->addEntity(new RS_Line(cont.get(), RS_Vector(0, 0),       RS_Vector(side, 0)));
    cont->addEntity(new RS_Line(cont.get(), RS_Vector(side, 0),    RS_Vector(side, side)));
    cont->addEntity(new RS_Line(cont.get(), RS_Vector(side, side), RS_Vector(0, side)));
    cont->addEntity(new RS_Line(cont.get(), RS_Vector(0, side),    RS_Vector(0, 0)));
    // Right square whose lower-left corner nearly touches the left square's
    // upper-right corner: pinch at ~(side, side).
    const double x0 = side + gap;
    const double y0 = side + gap;
    const double x1 = 2.0 * side + gap;
    const double y1 = 2.0 * side + gap;
    cont->addEntity(new RS_Line(cont.get(), RS_Vector(x0, y0), RS_Vector(x1, y0)));
    cont->addEntity(new RS_Line(cont.get(), RS_Vector(x1, y0), RS_Vector(x1, y1)));
    cont->addEntity(new RS_Line(cont.get(), RS_Vector(x1, y1), RS_Vector(x0, y1)));
    cont->addEntity(new RS_Line(cont.get(), RS_Vector(x0, y1), RS_Vector(x0, y0)));

    std::vector<std::unique_ptr<RS_EntityContainer>> loops;
    loops.push_back(std::move(cont));
    RS_HatchData data{true, 1.0, 0.0, QStringLiteral("SOLID")};
    auto hatch = makeHatch(std::move(loops), data);

    REQUIRE(hatch->getUpdateError() == RS_Hatch::HATCH_OK);
    REQUIRE(hatch->countLoops() == 2);
    const double expected = 2.0 * side * side;
    REQUIRE_THAT(hatch->getTotalArea(), WithinRel(expected, 1e-6));
}

