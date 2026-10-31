/*
 * This file is part of LibreCAD, a 2D CAD program.
 * Copyright (C) 2026 LibreCAD.org
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include <catch2/catch_test_macros.hpp>

#include "lc_offsetcurve.h"
#include "rs_ellipse.h"

TEST_CASE("Offset sample demand survives successful and refused producer paths", "[lc-offsetcurve][again][effort]") {
    const RS_Ellipse source{nullptr, {{0, 0}, {8, 0}, 0.5}};
    const auto request = LC_CurveOffset::makeSideRequest(LC_CurveOffsetSide::Right, 0.5);
    auto options = LC_CurveOffset::makeOffsetOptions(source, 0.5);
    const auto budget = makeDefaultOffsetSourceBudget();
    const auto geometry = LC_CurveOffset::buildDirectBranches(source, request, options, budget);
    REQUIRE(geometry.status == LC_CurveOffsetStatus::Ok); REQUIRE(geometry.exactSamples > 0);
    const auto materialized = LC_CurveOffset::materializeBranches(source, geometry, options, budget);
    REQUIRE(materialized.status == LC_CurveOffsetStatus::Ok);
    CHECK(materialized.sampleUnits >= geometry.exactSamples);
    const auto combined = LC_CurveOffset::createEntities(source, request, options, budget);
    REQUIRE(combined.status == LC_CurveOffsetStatus::Ok);
    CHECK(combined.sampleUnits == materialized.sampleUnits);
    options.maxSamples = 1;
    const auto refusedGeometry = LC_CurveOffset::buildDirectBranches(source, request, options, budget);
    REQUIRE(refusedGeometry.status == LC_CurveOffsetStatus::LimitExceeded);
    const auto refused = LC_CurveOffset::createEntities(source, request, options, budget);
    REQUIRE(refused.status == LC_CurveOffsetStatus::LimitExceeded);
    CHECK(refused.sampleUnits == refusedGeometry.exactSamples); CHECK(refused.sampleUnits > 0);
    CHECK(refused.sampleUnits <= options.maxSamples + 1); CHECK(refused.entities.empty());
}

TEST_CASE("Offset materializer exhaustion retains the spent sample demand without partial output", "[lc-offsetcurve][again][effort]") {
    const RS_Ellipse source{nullptr, {{0, 0}, {8, 0}, 0.5}};
    const auto request = LC_CurveOffset::makeSideRequest(LC_CurveOffsetSide::Right, 0.5);
    auto options = LC_CurveOffset::makeOffsetOptions(source, 0.5);
    const auto budget = makeDefaultOffsetSourceBudget();
    const auto geometry = LC_CurveOffset::buildDirectBranches(source, request, options, budget);
    REQUIRE(geometry.status == LC_CurveOffsetStatus::Ok); REQUIRE(geometry.exactSamples > 0);
    options.maxSamples = geometry.exactSamples + 1;
    const auto refused = LC_CurveOffset::materializeBranches(source, geometry, options, budget);
    CHECK(refused.status == LC_CurveOffsetStatus::LimitExceeded);
    CHECK(refused.sampleUnits > geometry.exactSamples); CHECK(refused.sampleUnits <= options.maxSamples + 1);
    CHECK(refused.entities.empty()); CHECK(refused.usage.outputEntities == 0);
}

TEST_CASE("Native creation distinguishes attempted producer output from admitted display output", "[lc-offsetcurve][again][effort]") {
    const RS_Ellipse source{nullptr, {{0, 0}, {5, 0}, 1}};
    auto made = LC_OffsetCurve::create(source, 1, false);
    REQUIRE(made); REQUIRE(made.entity); CHECK(made.engineRuns == 1); CHECK(made.sampleUnits > 0);
    CHECK(made.attemptedOutputUsage.outputEntities == 1); CHECK(made.outputUsage.outputEntities == 1);
    const auto actual = measureOffsetOutput({made.entity.get()}, makeDefaultOffsetSourceBudget().maxDeepEntities);
    REQUIRE(actual.status == LC_OffsetTreeStatus::Ok); CHECK(made.outputUsage.deepEntities == actual.deepEntities);
    LC_OffsetRunLimits limits; limits.checkLimits.maxDistancePairs = 1;
    const auto refused = LC_OffsetCurve::create(source, 1, false, limits);
    REQUIRE_FALSE(refused); CHECK(refused.engineRuns == 1); CHECK(refused.sampleUnits > 0);
    REQUIRE(refused.checkReason); CHECK(*refused.checkReason == LC_OffsetCheckResult::Reason::LimitExceeded);
    CHECK(refused.attemptedOutputUsage.outputEntities == 1); CHECK(refused.outputUsage.outputEntities == 0);
    CHECK(refused.outputUsage.deepEntities == 0); CHECK_FALSE(refused.entity);
}

TEST_CASE("Offset output-cap failures retain completed preparation counts without publishing a prefix", "[lc-offsetcurve][again][effort]") {
    const RS_Ellipse source{nullptr, {{0, 0}, {8, 0}, 0.5}};
    auto options = LC_CurveOffset::makeDirectOptions(source, 0.5);
    auto budget = makeDefaultOffsetSourceBudget();
    auto geometry = LC_CurveOffset::buildDirectBranches(source,
        LC_CurveOffset::makeSideRequest(LC_CurveOffsetSide::Right, 0.5), options, budget);
    REQUIRE(geometry.status == LC_CurveOffsetStatus::Ok); REQUIRE(geometry.branches.size() == 1);
    const auto second = geometry.branches.front(); geometry.branches.push_back(second);
    SECTION("The second output exceeds the root cap") {
        budget.maxOutputEntities = 1;
        const auto refused = LC_CurveOffset::materializeBranches(source, geometry, options, budget);
        CHECK(refused.status == LC_CurveOffsetStatus::LimitExceeded); CHECK(refused.entities.empty());
        CHECK(refused.usage.outputEntities == 0); CHECK(refused.attemptedOutputUsage.outputEntities == 1);
        CHECK(refused.attemptedOutputUsage.cubicPieces > 0); CHECK(refused.attemptedOutputUsage.deepEntities > 0);
        CHECK(refused.attemptedDeepCountComplete);
    }
    SECTION("The first output's observed leaves exceed its allowance") {
        budget.maxDeepEntities = 1;
        const auto refused = LC_CurveOffset::materializeBranches(source, geometry, options, budget);
        CHECK(refused.status == LC_CurveOffsetStatus::LimitExceeded); CHECK(refused.entities.empty());
        CHECK(refused.usage.outputEntities == 0); CHECK(refused.attemptedOutputUsage.outputEntities == 1);
        CHECK(refused.attemptedOutputUsage.deepEntities == 1); CHECK_FALSE(refused.attemptedDeepCountComplete);
    }
}

TEST_CASE("Region offset candidates contribute to successful and exhausted sample demand", "[lc-offsetcurve][again][effort][region]") {
    const RS_Ellipse source{nullptr, {{0, 0}, {5, 0}, 1}};
    auto options = LC_CurveOffset::makeOffsetOptions(source, 0.5);
    options.mode = LC_CurveOffsetMode::RegionBoundary;
    const auto request = LC_CurveOffset::makeSideRequest(LC_CurveOffsetSide::Right, 0.5);
    const auto budget = makeDefaultOffsetSourceBudget();
    const auto geometry = LC_CurveOffset::buildDirectBranches(source, request, options, budget);
    REQUIRE(geometry.status == LC_CurveOffsetStatus::Ok); REQUIRE(geometry.exactSamples > 0);
    const auto made = LC_CurveOffset::createEntities(source, request, options, budget);
    REQUIRE(made.status == LC_CurveOffsetStatus::Ok); CHECK(made.sampleUnits >= geometry.exactSamples);
    options.maxSamples = 1;
    const auto refused = LC_CurveOffset::createEntities(source, request, options, budget);
    CHECK(refused.status == LC_CurveOffsetStatus::LimitExceeded); CHECK(refused.sampleUnits > 0);
    CHECK(refused.sampleUnits <= options.maxSamples + 1); CHECK(refused.entities.empty());
}
