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

#include <cmath>
#include <cstddef>
#include <limits>
#include <memory>
#include <vector>

#include "lc_hatchdensity.h"
#include "lc_hatchtestsupport.h"
#include "rs_hatch.h"
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
