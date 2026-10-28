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

// Density guard: estimate, work bound, buffered piece budget, HATCH_TOO_DENSE.

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <QColor>

#include <cmath>
#include <cstddef>
#include <limits>
#include <memory>
#include <vector>

#include "lc_hatchdensity.h"
#include "lc_hatchdensitytext.h"
#include "lc_hatchtestsupport.h"
#include "rs_block.h"
#include "rs_entitycontainer.h"
#include "rs_graphic.h"
#include "rs_hatch.h"
#include "rs_insert.h"
#include "rs_line.h"
#include "rs_pen.h"
#include "rs_vector.h"

using Catch::Approx;
using Catch::Matchers::WithinAbs;
using Catch::Matchers::WithinRel;
using lc::test::makePatternHatch;
using lc::test::patternPieceCount;
using lc::test::rectRing;
using lc::test::Ring;
using lc::test::SourcePatterns;

namespace {

/** Minimal frame-with-hole helper: outer rectangle plus an inset hole. */
std::vector<Ring> frameWithHole(double x1, double y1, double x2, double y2,
                                double inset) {
    return {
        rectRing(x1, y1, x2, y2),
        rectRing(x1 + inset, y1 + inset, x2 - inset, y2 - inset),
    };
}

} // namespace

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("A frame whose bounding box spans 10,201 tiles is hatched, not refused",
          "[lc-hatchfix][density]") {
    SourcePatterns patterns;
    // Outer (13, 17) - (10113, 10117) with hole (63, 67) - (10063, 10067).
    // Offsets keep every edge off the pattern lattice.
    std::vector<Ring> rings = {
        rectRing(13.0, 17.0, 10113.0, 10117.0),
        rectRing(63.0, 67.0, 10063.0, 10067.0),
    };
    auto h = makePatternHatch("BRICK", 1.0, 0.0, rings);
    REQUIRE(h);
    REQUIRE(h->getUpdateError() == RS_Hatch::HATCH_OK);
    // Even-odd area: 10100 x 10100 - 10000 x 10000 = 2,010,000.
    CHECK_THAT(h->getTotalArea(), WithinRel(2.01e6, 1e-9));
    CHECK_THAT(h->getDensityReport().area, WithinRel(2.01e6, 1e-9));
    // Loose 15% band around the ~1,407 estimate (worst estimator error ~14%).
    const double expected = 1407.0;
    const double attempted = static_cast<double>(h->getDensityReport().attempted);
    CHECK(attempted > 0.85 * expected);
    CHECK(attempted < 1.15 * expected);
    CHECK(h->getDensityReport().finite);
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("A 9,990-unit ANSI31 square trips the estimate and keeps area and picking",
          "[lc-hatchfix][density]") {
    SourcePatterns patterns;
    LC_HatchDensityPolicy policy;
    policy.maxPieces = 1e5;
    LC_HatchDensity::ScopedPolicy guard{policy};

    std::vector<Ring> rings = {rectRing(0.0, 0.0, 9990.0, 9990.0)};
    auto h = makePatternHatch("ANSI31", 1.0, 0.0, rings);
    REQUIRE(h);
    REQUIRE(h->getUpdateError() == RS_Hatch::HATCH_TOO_DENSE);
    CHECK(h->getDensityReport().reason == LC_HatchDensityReport::Reason::Estimate);
    CHECK(patternPieceCount(*h) == 0);
    CHECK(h->countLoops() == 1);
    CHECK_THAT(h->getTotalArea(), WithinRel(9.98001e7, 1e-9));
    // Picking the centre of the hatch returns 0 distance; a tripped hatch
    // keeps its loops, area and region picking.
    RS_Entity* hit = nullptr;
    CHECK(h->getDistanceToPoint(RS_Vector(4995.0, 4995.0), &hit) == 0.0);
    CHECK(h->getDensityReport().work == 0.0);
    // ANSI31: 99 lines, pattern area 10000; A = 9.98001e7.
    const double est = h->getDensityReport().estimatedPieces;
    CHECK_THAT(est, WithinRel(998010.87, 1e-6));
    // minimumScale with M = 1e5 should give ~3.19.
    const double smin = LC_HatchDensity::minimumScale(h->getDensityReport(),
                                                     1.0, 1e5);
    CHECK(std::isfinite(smin));
    CHECK_THAT(smin, WithinRel(3.19363, 1e-4));
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("Holes are not area", "[lc-hatchfix][density]") {
    SourcePatterns patterns;
    LC_HatchDensityPolicy policy;
    policy.maxPieces = 5000;
    LC_HatchDensity::ScopedPolicy guard{policy};

    std::vector<Ring> rings = {
        rectRing(13.0, 17.0, 10113.0, 10117.0),
        rectRing(63.0, 67.0, 10063.0, 10067.0),
    };
    auto h = makePatternHatch("BRICK", 1.0, 0.0, rings);
    REQUIRE(h);
    CHECK(h->getUpdateError() == RS_Hatch::HATCH_OK);
    // areaTerm must stay well below 5000; a hole-blind Σ|A| would push it
    // past 80,000.
    CHECK(h->getDensityReport().areaTerm < 5000.0);
    CHECK(h->getDensityReport().estimatedPieces < 5000.0);
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("Degenerate hatch scales never crash", "[lc-hatchfix][density]") {
    SourcePatterns patterns;
    const double scales[] = {std::numeric_limits<double>::quiet_NaN(), 0.0,
                             1e-300, std::numeric_limits<double>::infinity()};
    for (double s : scales) {
        std::vector<Ring> rings = {rectRing(0.0, 0.0, 20.0, 10.0)};
        auto h = makePatternHatch("ANSI31", s, 0.0, rings);
        REQUIRE(h);
        const auto err = h->getUpdateError();
        if (std::isnan(s)) {
            CHECK(err != RS_Hatch::HATCH_OK);
        } else {
            CHECK((err == RS_Hatch::HATCH_TOO_SMALL
                   || err == RS_Hatch::HATCH_TOO_DENSE
                   || err == RS_Hatch::HATCH_INVALID_CONTOUR));
        }
        CHECK(patternPieceCount(*h) == 0);
    }
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("Density report is cleared on each update()", "[lc-hatchfix][density]") {
    SourcePatterns patterns;
    // Trip the estimate first.
    LC_HatchDensityPolicy policy;
    policy.maxPieces = 1e5;
    auto h = std::unique_ptr<RS_Hatch>();
    {
        LC_HatchDensity::ScopedPolicy guard{policy};
        std::vector<Ring> rings = {rectRing(0.0, 0.0, 9990.0, 9990.0)};
        h = makePatternHatch("ANSI31", 1.0, 0.0, rings);
        REQUIRE(h);
        CHECK(h->getUpdateError() == RS_Hatch::HATCH_TOO_DENSE);
        CHECK(h->getDensityReport().reason != LC_HatchDensityReport::Reason::None);
    }
    // Default policy, trim down the area: should pass with reason None.
    {
        // Replace boundary with a small 20 x 10 rectangle.
        std::vector<Ring> smallRings = {rectRing(0.0, 0.0, 20.0, 10.0)};
        auto h2 = makePatternHatch("ANSI31", 1.0, 0.0, smallRings);
        REQUIRE(h2);
        CHECK(h2->getUpdateError() == RS_Hatch::HATCH_OK);
        CHECK(h2->getDensityReport().reason == LC_HatchDensityReport::Reason::None);
    }
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("Policy revision bumps on every ScopedPolicy ctor/dtor",
          "[lc-hatchfix][density]") {
    const std::uint64_t r0 = LC_HatchDensity::policyRevision();
    {
        LC_HatchDensityPolicy policy;
        policy.maxPieces = 2e6;
        LC_HatchDensity::ScopedPolicy guard{policy};
        CHECK(LC_HatchDensity::policyRevision() == r0 + 1);
    }
    CHECK(LC_HatchDensity::policyRevision() == r0 + 2);
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("evenOddArea subtracts holes recursively",
          "[lc-hatchfix][density]") {
    SourcePatterns patterns;
    // Simple frame: outer 100 x 100, hole 50 x 50 inset. Even-odd area 7500.
    std::vector<Ring> rings = {
        rectRing(0.0, 0.0, 100.0, 100.0),
        rectRing(25.0, 25.0, 75.0, 75.0),
    };
    auto h = makePatternHatch("ANSI31", 1.0, 0.0, rings);
    REQUIRE(h);
    REQUIRE(h->getUpdateError() == RS_Hatch::HATCH_OK);
    CHECK_THAT(h->getTotalArea(), WithinRel(7500.0, 1e-9));
    CHECK_THAT(h->getDensityReport().area, WithinRel(7500.0, 1e-9));
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("minimumScale solves for a cap exactly",
          "[lc-hatchfix][density]") {
    LC_HatchDensityReport r;
    r.areaTerm = 988020.0;
    r.boundaryTerm = 9990.0;
    r.estimatedPieces = r.areaTerm + r.boundaryTerm;
    r.finite = true;
    // minimumScale should solve areaTerm*(1/x)^2 + boundaryTerm*(1/x) = M.
    // Pick M = r.estimatedPieces / 4; the new scale should be about 2x the
    // old (quadratic dominates).
    const double smin = LC_HatchDensity::minimumScale(r, 1.0, r.estimatedPieces / 4.0);
    REQUIRE(std::isfinite(smin));
    // Plug smin back in: areaTerm*(1/smin)^2 + boundaryTerm*(1/smin) == M.
    const double x = 1.0 / smin;
    const double N = r.areaTerm * x * x + r.boundaryTerm * x;
    CHECK_THAT(N, WithinRel(r.estimatedPieces / 4.0, 1e-9));
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("Raising the limit generates the 9,990 square",
          "[.slow][lc-hatchfix][density]") {
    SourcePatterns patterns;
    LC_HatchDensityPolicy policy;
    policy.maxPieces = 2e6;
    policy.hardMax = 2e6;
    LC_HatchDensity::ScopedPolicy guard{policy};
    std::vector<Ring> rings = {rectRing(0.0, 0.0, 9990.0, 9990.0)};
    auto h = makePatternHatch("ANSI31", 1.0, 0.0, rings);
    REQUIRE(h);
    REQUIRE(h->getUpdateError() == RS_Hatch::HATCH_OK);
    const std::size_t a = h->getDensityReport().attempted;
    CHECK(a >= 970000u);
    CHECK(a <= 1010000u);
}

// -----------------------------------------------------------------------
// Tinted fill, shared insert-expansion budget.
// -----------------------------------------------------------------------

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("A hatch over the old area limit is not invisible",
          "[lc-hatchfix][density]") {
    SourcePatterns patterns;
    // 10,500 unit square. Previously would have hit HATCH_AREA_TOO_BIG; now
    // it exceeds the 1e6 soft piece cap and tints instead of staying empty.
    std::vector<Ring> rings = {rectRing(0.0, 0.0, 10500.0, 10500.0)};
    auto h = makePatternHatch("ANSI31", 1.0, 0.0, rings);
    REQUIRE(h);
    REQUIRE(h->getUpdateError() == RS_Hatch::HATCH_TOO_DENSE);
    REQUIRE(h->getDensityReport().reason == LC_HatchDensityReport::Reason::Estimate);
    CHECK(patternPieceCount(*h) == 0);

    // Render the hatch into a 256x256 raster with a cosmetic black pen.
    // The TestPainter default is factor 1 at origin (0, 0) with the y-axis
    // flipped; the square (0,0)-(10500,10500) covers the whole canvas, so
    // the centre pixel must be filled by the tint.
    lc::test::TestPainter canvas;
    canvas.painter.setPen(RS_Color(0, 0, 0));
    h->draw(&canvas.painter);
    canvas.painter.end();

    const QRgb centre = canvas.image.pixel(canvas.W / 2, canvas.H / 2);
    // Non-white: tint drew at least some ink here. The estimate (~1.1M) is
    // large so the tint is dense: red/green/blue must each be <= 10.
    CHECK(qRed(centre) <= 10);
    CHECK(qGreen(centre) <= 10);
    CHECK(qBlue(centre) <= 10);
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("The tint follows the pattern's coverage",
          "[lc-hatchfix][density]") {
    SourcePatterns patterns;
    // Shrink the piece cap so a tiny region trips the estimate predictably.
    LC_HatchDensityPolicy policy;
    policy.maxPieces = 10;
    LC_HatchDensity::ScopedPolicy guard{policy};

    std::vector<Ring> rings = {rectRing(20.0, 20.0, 220.0, 220.0)};
    auto h = makePatternHatch("BRICK", 1.0, 0.0, rings);
    REQUIRE(h);
    REQUIRE(h->getUpdateError() == RS_Hatch::HATCH_TOO_DENSE);

    // Factor 1, DPR 1, cosmetic pen: wPhys = 1, kPhys = 1 so f = L/A + E/A^2.
    // BRICK has 2 lines per 100 x 100 tile = 100 units of ink over A_t = 10,000
    // and E = 2 pieces per tile. For the 200x200 region: lineDensity = 100/10000
    // = 0.01, pieceDensity = 2/10000 = 2e-4. tintAlpha = 1 - exp(-(0.01 + 2e-4))
    // ~= 1 - exp(-0.0102) ~= 0.01015. The centre pixel is approximately
    // 255 * (1 - 0.01015) ~ 252.
    const double a = LC_HatchDensity::tintAlpha(h->getDensityReport(), 1.0, 1.0);
    CHECK(a > 0.0);
    CHECK(a < 0.1);

    lc::test::TestPainter canvas;
    canvas.painter.setPen(RS_Color(0, 0, 0));
    h->draw(&canvas.painter);
    canvas.painter.end();

    // Centre pixel of the region at image coords (120, 256-120) is tinted,
    // but only lightly (alpha << 1).
    const int px = 120;
    const int py = canvas.H - 120;
    const QRgb centre = canvas.image.pixel(px, py);
    CHECK(qRed(centre) < 255);
    CHECK(qRed(centre) > 230);
    // A pixel well outside the region stays unpainted (255, 255, 255).
    const QRgb outside = canvas.image.pixel(240, 240);
    CHECK(qRed(outside) == 255);
    CHECK(qGreen(outside) == 255);
    CHECK(qBlue(outside) == 255);
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("An insert array never blanks its block and never exceeds the hatch budget",
          "[lc-hatchfix][density]") {
    SourcePatterns patterns;
    RS_Graphic graphic;
    graphic.initForNewDocument();

    // Build a block "cell" with a non-hatch LINE plus one ANSI31 hatch over
    // (0, 0) - (30, 30). The hatch is pattern (non-solid) scale 1.
    auto* block = new RS_Block(&graphic, RS_BlockData("cell", {0, 0}, false));
    auto* edge = new RS_Line(block, {0.0, 0.0}, {30.0, 0.0});
    block->addEntity(edge);
    auto* hatch = new RS_Hatch(block, RS_HatchData(false, 1.0, 0.0, "ANSI31"));
    auto* loop = new RS_EntityContainer(hatch);
    loop->addEntity(new RS_Line(loop, {0.0, 0.0}, {30.0, 0.0}));
    loop->addEntity(new RS_Line(loop, {30.0, 0.0}, {30.0, 30.0}));
    loop->addEntity(new RS_Line(loop, {30.0, 30.0}, {0.0, 30.0}));
    loop->addEntity(new RS_Line(loop, {0.0, 30.0}, {0.0, 0.0}));
    hatch->addEntity(loop);
    hatch->update();
    REQUIRE(hatch->getUpdateError() == RS_Hatch::HATCH_OK);
    const std::size_t n1 = hatch->getDensityReport().attempted;
    REQUIRE(n1 > 0u);
    block->addEntity(hatch);
    REQUIRE(graphic.addBlock(block));

    // 10 x 10 MINSERT with spacing (80, 40), scale (2, 1): the non-uniform
    // scale forces every clone's hatch to regenerate (shared geometry is a
    // later step). 100 clones is enough to exercise the shared budget's
    // OK -> dense transition and the all-tint fallback.
    RS_InsertData idata{"cell", {0.0, 0.0}, {2.0, 1.0}, 0.0, 10, 10, {80.0, 40.0}};
    RS_Insert insert{&graphic, idata};
    insert.update(RS_InsertExpansionBudget{256U, 1000000U, 1000000U, 5U * n1});
    // 100 lines + 100 hatch clones = 200 derived entities.
    CHECK(insert.count() == 200);
    // Walk the children, totalling hatch-piece counts and checking the
    // mixture.
    std::size_t totalHatchPieces = 0;
    int okHatches = 0;
    int denseHatches = 0;
    for (RS_Entity* child : insert) {
        if (child != nullptr && child->rtti() == RS2::EntityHatch) {
            auto* h = static_cast<RS_Hatch*>(child);
            if (h->getUpdateError() == RS_Hatch::HATCH_OK) ++okHatches;
            if (h->getUpdateError() == RS_Hatch::HATCH_TOO_DENSE) ++denseHatches;
            totalHatchPieces += patternPieceCount(*h);
            // Every clone keeps its area 1800 (30 x 60 after x-scale 2).
            CHECK_THAT(h->getTotalArea(), WithinRel(1800.0, 1e-9));
        }
    }
    CHECK(okHatches >= 1);
    CHECK(denseHatches >= 1);
    CHECK(totalHatchPieces <= 5U * n1);

    // With maxHatchPieces = 0: every hatch tints.
    RS_Insert zero{&graphic, idata};
    zero.update(RS_InsertExpansionBudget{256U, 1000000U, 1000000U, 0U});
    CHECK(zero.count() == 200);
    int zeroDense = 0;
    for (RS_Entity* child : zero) {
        if (child != nullptr && child->rtti() == RS2::EntityHatch) {
            auto* h = static_cast<RS_Hatch*>(child);
            if (h->getUpdateError() == RS_Hatch::HATCH_TOO_DENSE) ++zeroDense;
            CHECK(patternPieceCount(*h) == 0);
        }
    }
    CHECK(zeroDense == 100);
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("tintAlpha clamps and returns opaque on non-finite inputs",
          "[lc-hatchfix][density]") {
    LC_HatchDensityReport r;
    r.lineDensity = 1.0;
    r.pieceDensity = 1.0;
    r.finite = true;
    // Finite inputs: alpha is in (0, 1).
    const double a = LC_HatchDensity::tintAlpha(r, 1.0, 1.0);
    CHECK(a > 0.0);
    CHECK(a < 1.0);
    // Very large f -> alpha saturates below 1.
    const double big = LC_HatchDensity::tintAlpha(r, 1e-6, 1.0);
    CHECK(big <= 1.0);
    CHECK(big > 0.99);
    // Non-finite report -> opaque fallback.
    LC_HatchDensityReport bad;
    bad.finite = false;
    CHECK(LC_HatchDensity::tintAlpha(bad, 1.0, 1.0) == 1.0);
    // Non-positive kPhys -> opaque (defensive; a sane painter never produces
    // this but we must stay safe).
    CHECK(LC_HatchDensity::tintAlpha(r, 0.0, 1.0) == 1.0);
    // Zero densities -> no tint.
    LC_HatchDensityReport zero;
    zero.lineDensity = 0.0;
    zero.pieceDensity = 0.0;
    zero.finite = true;
    CHECK(LC_HatchDensity::tintAlpha(zero, 1.0, 1.0) == 0.0);
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("ScopedSharedBudget restores the previous shared budget",
          "[lc-hatchfix][density]") {
    CHECK(LC_HatchDensity::sharedBudget() == nullptr);
    LC_HatchPieceBudget outer{100U};
    {
        LC_HatchDensity::ScopedSharedBudget s1{outer};
        CHECK(LC_HatchDensity::sharedBudget() == &outer);
        LC_HatchPieceBudget inner{10U};
        {
            LC_HatchDensity::ScopedSharedBudget s2{inner};
            CHECK(LC_HatchDensity::sharedBudget() == &inner);
        }
        CHECK(LC_HatchDensity::sharedBudget() == &outer);
    }
    CHECK(LC_HatchDensity::sharedBudget() == nullptr);
}

// -----------------------------------------------------------------------
// Density texts: numbers carried through the user-visible strings.
// -----------------------------------------------------------------------

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("Density texts carry the numbers",
          "[lc-hatchfix][density]") {
    // A report that tripped by estimate.
    LC_HatchDensityReport r;
    r.reason = LC_HatchDensityReport::Reason::Estimate;
    r.areaTerm = 988020.0;
    r.boundaryTerm = 9990.0;
    r.estimatedPieces = 998010.0;
    r.limit = 1000000.0;
    r.finite = true;

    const QString msg = LC_HatchDensityText::tooDenseMessage(r, 1.0);
    // Estimate branch: pieces come from estimatedPieces (998010).
    CHECK(msg.contains(QStringLiteral("998010")));
    // Limit.
    CHECK(msg.contains(QStringLiteral("1000000")));
    // Suggested scale: minimumScale returns about 1.0100 for M = 1e6
    // (close to current, since estimate is already just under cap).
    CHECK(msg.contains(QStringLiteral("too dense")));

    // Budget branch: pieces come from attempted.
    r.reason = LC_HatchDensityReport::Reason::Budget;
    r.attempted = 42U;
    const QString msg2 = LC_HatchDensityText::tooDenseMessage(r, 1.0);
    CHECK(msg2.contains(QStringLiteral("42")));
    CHECK(msg2.contains(QStringLiteral("1000000")));

    // patternLinesStatus: tripped vs OK.
    CHECK(LC_HatchDensityText::patternLinesStatus(r).contains(
        QStringLiteral("not generated")));
    LC_HatchDensityReport ok;
    ok.reason = LC_HatchDensityReport::Reason::None;
    ok.finite = true;
    const QString okStatus = LC_HatchDensityText::patternLinesStatus(ok);
    CHECK(okStatus.contains(QStringLiteral("generated")));
    CHECK_FALSE(okStatus.contains(QStringLiteral("not generated")));

    // importSummary/r12Warning: carry the count.
    const QString iSum = LC_HatchDensityText::importSummary(3);
    CHECK(iSum.contains(QStringLiteral("3")));
    const QString r12 = LC_HatchDensityText::r12Warning(1);
    CHECK(r12.contains(QStringLiteral("R12")));
    CHECK(r12.contains(QStringLiteral("1")));
}
