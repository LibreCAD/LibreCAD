/*******************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 * Copyright (C) 2026 LibreCAD.org
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later version.
 ******************************************************************************/

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <memory>
#include <utility>

#include <boost/multiprecision/cpp_int.hpp>

#include "lc_ellipseoffsetgeometry.h"
#include "rs_spline.h"

namespace geo = LC_EllipseOffsetGeometry;

namespace {

constexpr double startAngle = .25;
constexpr double endAngle = .2501;
constexpr double tolerance = .01;
constexpr std::size_t noSpan = std::size_t(-1);
const geo::CheckWork preSpent{2, 3, 5, 1, 7};

struct Fixture {
    RS_SplineData cache;
    LC_EllipseOffsetDefinition definition;
};

void checkSameWork(const geo::CheckWork& actual, const geo::CheckWork& expected) {
    CHECK(actual.cacheSpans == expected.cacheSpans);
    CHECK(actual.vertices == expected.vertices);
    CHECK(actual.distancePairs == expected.distancePairs);
    CHECK(actual.deepest == expected.deepest);
    CHECK(actual.orderingComparisons == expected.orderingComparisons);
}

void checkWorkBounds(const geo::CheckWork& work, const geo::CheckWork& entered,
                     const LC_OffsetCheckLimits& limits) {
    CHECK(work.cacheSpans >= entered.cacheSpans); CHECK(work.cacheSpans <= limits.maxCacheSpans);
    CHECK(work.vertices >= entered.vertices); CHECK(work.vertices <= limits.maxVertices);
    CHECK(work.distancePairs >= entered.distancePairs); CHECK(work.distancePairs <= limits.maxDistancePairs);
    CHECK(work.deepest >= entered.deepest); CHECK(work.deepest <= limits.maxDepth);
    CHECK(work.orderingComparisons >= entered.orderingComparisons);
    CHECK(work.orderingComparisons <= limits.maxOrderingComparisons);
}

LC_OffsetCheckLimits exactLimits(const geo::CheckWork& work) {
    return {work.cacheSpans, work.vertices, work.distancePairs, work.deepest, work.orderingComparisons};
}

void checkSameInterval(const LC_Interval& a, const LC_Interval& b) {
    CHECK(a.lo() == b.lo()); CHECK(a.hi() == b.hi());
}

void checkSameCache(const RS_SplineData& a, const RS_SplineData& b) {
    CHECK(a.degree == b.degree); CHECK(a.type == b.type);
    CHECK(a.knotslist == b.knotslist); CHECK(a.weights == b.weights);
    CHECK(a.m_closedFlag == b.m_closedFlag); CHECK(a.m_periodicFlag == b.m_periodicFlag);
    REQUIRE(a.controlPoints.size() == b.controlPoints.size());
    for (std::size_t i = 0; i < a.controlPoints.size(); ++i) {
        CHECK(a.controlPoints[i].x == b.controlPoints[i].x);
        CHECK(a.controlPoints[i].y == b.controlPoints[i].y);
        CHECK(a.controlPoints[i].z == b.controlPoints[i].z);
        CHECK(a.controlPoints[i].valid == b.controlPoints[i].valid);
    }
}

Fixture circleArc(const bool slowParameter = false, const bool cacheReversed = false) {
    Fixture fixture;
    fixture.definition.basis = {RS_Vector{0, 0}, RS_Vector{1, 0}, 1, startAngle, endAngle, false};
    fixture.definition.distance = 1;
    fixture.definition.cacheReversed = cacheReversed;
    fixture.definition.relTolerance = tolerance;
    auto& cache = fixture.cache;
    cache.degree = 3;
    cache.knotslist = {0, 0, 0, 0, 1, 1, 1, 2, 2, 2, 2};
    // The exact offset of the unit circle by +1 is the concentric radius-2 arc.
    const auto point = [](const double angle) { return RS_Vector{2 * std::cos(angle), 2 * std::sin(angle)}; };
    const auto tangent = [](const double angle) { return RS_Vector{-2 * std::sin(angle), 2 * std::cos(angle)}; };
    const std::array<double, 3> angles{{startAngle, .5 * (startAngle + endAngle), endAngle}};
    cache.controlPoints.push_back(point(angles[0]));
    for (std::size_t span = 0; span < 2; ++span) {
        const auto first = cache.controlPoints.back(), last = point(angles[span + 1]);
        const double h = (angles[span + 1] - angles[span]) / 3;
        std::array<RS_Vector, 4> controls{{first, first + tangent(angles[span]) * h,
                                        last - tangent(angles[span + 1]) * h, last}};
        if (slowParameter && span == 0) {
            const auto chord = last - first;
            const double epsilon = std::ldexp(1.0, -8);
            const double scale = 1.0 / 9.0 + epsilon;
            controls[1] = first + chord / 3;
            controls[2] = first + chord * (1.0 / 3.0 + (-2.0 / 9.0 + epsilon) / (3 * scale));
            // q'(t) is proportional to (t-1/3)^2+epsilon. Prove that the
            // actual rounded controls still have strictly positive chord projection.
            using Rational = boost::multiprecision::cpp_rational;
            std::array<Rational, 3> projected;
            for (std::size_t i = 0; i < projected.size(); ++i) {
                projected[i] = 3 * ((Rational(controls[i + 1].x) - Rational(controls[i].x)) * Rational(chord.x) +
                    (Rational(controls[i + 1].y) - Rational(controls[i].y)) * Rational(chord.y));
            }
            const Rational a = projected[0] - 2 * projected[1] + projected[2];
            const Rational b = 2 * (projected[1] - projected[0]);
            REQUIRE(a > 0); REQUIRE(4 * a * projected[0] - b * b > 0);
        }
        cache.controlPoints.insert(cache.controlPoints.end(), controls.begin() + 1, controls.end());
    }
    cache.weights.assign(cache.controlPoints.size(), 1);
    if (cacheReversed) { std::reverse(cache.controlPoints.begin(), cache.controlPoints.end()); }
    return fixture;
}

void checkSuccess(const LC_OffsetCheckResult& result, const geo::WorkBudget& budget) {
    CAPTURE(result.reason, result.work.cacheSpans, result.work.vertices, result.work.distancePairs,
            result.work.deepest, result.work.orderingComparisons);
    REQUIRE(result.reason == LC_OffsetCheckResult::Reason::None);
    REQUIRE(bool(result)); REQUIRE(result.analysis); CHECK(result.span == noSpan);
    REQUIRE(budget.status() == geo::BudgetStatus::Ok);
    checkSameWork(result.work, budget.consumed());
    const auto& proof = *result.analysis;
    REQUIRE(proof.layout().spanCount() == 2);
    REQUIRE(proof.edges().size() == 1);
    CHECK_FALSE(proof.closed()); CHECK_FALSE(proof.cut());
    REQUIRE(proof.startTerminal()); REQUIRE(proof.endTerminal());
    CHECK(proof.startTerminal()->kind == geo::ReferenceTerminalKind::SourceStart);
    CHECK(proof.endTerminal()->kind == geo::ReferenceTerminalKind::SourceEnd);
    REQUIRE_FALSE(proof.pieces().empty());
    std::array<double, 2> covered{{0, 0}};
    std::size_t previousSpan = 0;
    for (const auto& piece : proof.pieces()) {
        REQUIRE(piece.normalizedSpan < covered.size());
        CHECK(piece.normalizedSpan >= previousSpan); previousSpan = piece.normalizedSpan;
        CHECK(piece.edge == proof.edges().front());
        CHECK(piece.candidateParameter.lo() == covered[piece.normalizedSpan]);
        REQUIRE(piece.candidateParameter.lo() < piece.candidateParameter.hi());
        covered[piece.normalizedSpan] = piece.candidateParameter.hi();
        REQUIRE(piece.candidateDepth <= budget.depthLimit());
        CHECK(piece.referenceDepth <= budget.depthLimit() - piece.candidateDepth);
    }
    CHECK(covered[0] == 1); CHECK(covered[1] == 1);
}

void checkSamePath(const geo::CacheTopologyAnalysis& a, const geo::CacheTopologyAnalysis& b) {
    CHECK(a.edges() == b.edges()); CHECK(a.closed() == b.closed());
    REQUIRE(a.startTerminal()); REQUIRE(b.startTerminal()); REQUIRE(a.endTerminal()); REQUIRE(b.endTerminal());
    CHECK(a.startTerminal()->occurrence.value == b.startTerminal()->occurrence.value);
    CHECK(a.endTerminal()->occurrence.value == b.endTerminal()->occurrence.value);
    CHECK(a.startTerminal()->node.value == b.startTerminal()->node.value);
    CHECK(a.endTerminal()->node.value == b.endTerminal()->node.value);
    REQUIRE(a.pieces().size() == b.pieces().size());
    for (std::size_t i = 0; i < a.pieces().size(); ++i) {
        const auto& x = a.pieces()[i]; const auto& y = b.pieces()[i];
        CHECK(x.span == y.span); CHECK(x.normalizedSpan == y.normalizedSpan); CHECK(x.edge == y.edge);
        CHECK(x.candidateDepth == y.candidateDepth); CHECK(x.referenceDepth == y.referenceDepth);
        checkSameInterval(x.candidateParameter, y.candidateParameter);
        checkSameInterval(x.referenceParameter, y.referenceParameter);
        checkSameInterval(x.geometryError, y.geometryError);
        checkSameInterval(x.clearanceLowerBound, y.clearanceLowerBound);
    }
}

enum class Resource { Spans, Vertices, Pairs, Depth, Ordering };

void checkOneLess(const Fixture& fixture, const Resource resource) {
    const auto original = fixture.cache;
    geo::WorkBudget measured;
    REQUIRE(measured.adoptWork(preSpent));
    const auto complete = geo::checkCache(fixture.cache, fixture.definition, tolerance, measured);
    checkSuccess(complete, measured);
    auto limits = exactLimits(complete.work);
    switch (resource) {
    case Resource::Spans:
        REQUIRE(limits.maxCacheSpans > preSpent.cacheSpans); --limits.maxCacheSpans; break;
    case Resource::Vertices:
        REQUIRE(limits.maxVertices > preSpent.vertices); --limits.maxVertices; break;
    case Resource::Pairs:
        REQUIRE(limits.maxDistancePairs > preSpent.distancePairs); --limits.maxDistancePairs; break;
    case Resource::Depth:
        REQUIRE(limits.maxDepth > preSpent.deepest); --limits.maxDepth; break;
    case Resource::Ordering:
        REQUIRE(limits.maxOrderingComparisons > preSpent.orderingComparisons); --limits.maxOrderingComparisons; break;
    }
    geo::WorkBudget budget{limits};
    REQUIRE(budget.status() == geo::BudgetStatus::Ok); REQUIRE(budget.adoptWork(preSpent));
    const auto refused = geo::checkCache(fixture.cache, fixture.definition, tolerance, budget);
    CAPTURE(resource, refused.reason, complete.work.deepest);
    REQUIRE(refused.reason == LC_OffsetCheckResult::Reason::LimitExceeded);
    CHECK_FALSE(bool(refused)); CHECK_FALSE(refused.analysis);
    REQUIRE(budget.status() == geo::BudgetStatus::LimitExceeded);
    checkSameWork(refused.work, budget.consumed()); checkWorkBounds(refused.work, preSpent, limits);
    const auto stopped = budget.consumed();
    const auto retry = geo::checkCache(fixture.cache, fixture.definition, tolerance, budget);
    CHECK(retry.reason == LC_OffsetCheckResult::Reason::LimitExceeded); CHECK_FALSE(retry.analysis);
    checkSameWork(retry.work, stopped); checkSameWork(budget.consumed(), stopped);
    CHECK_FALSE(budget.consumePieces(1)); CHECK_FALSE(budget.consumeVertices(1));
    CHECK_FALSE(budget.consumePairs(1)); CHECK_FALSE(budget.consumeOrderingComparisons(1));
    checkSameWork(budget.consumed(), stopped);
    checkSameCache(fixture.cache, original);
    // No partially certified path escapes, and an earlier successful owner is unaffected.
    REQUIRE(complete.analysis); CHECK(complete.analysis->layout().spanCount() == 2);
    CHECK(complete.analysis->startTerminal()->kind == geo::ReferenceTerminalKind::SourceStart);
    CHECK(complete.analysis->endTerminal()->kind == geo::ReferenceTerminalKind::SourceEnd);
}

} // namespace

TEST_CASE("cache topology admits exact cumulative resource caps",
          "[lc-offsetcurve][geometry][cache-topology-chain][cache-topology-budget]") {
    SECTION("Every measured cumulative ceiling succeeds exactly, in either cache order") {
        for (const bool slow : {false, true}) for (const bool reversed : {false, true}) {
            CAPTURE(slow, reversed);
            const auto fixture = circleArc(slow, reversed);
            geo::WorkBudget measured; REQUIRE(measured.adoptWork(preSpent));
            const auto complete = geo::checkCache(fixture.cache, fixture.definition, tolerance, measured);
            checkSuccess(complete, measured);
            const auto limits = exactLimits(complete.work);
            geo::WorkBudget exact{limits}; REQUIRE(exact.adoptWork(preSpent));
            const auto accepted = geo::checkCache(fixture.cache, fixture.definition, tolerance, exact);
            checkSuccess(accepted, exact); checkSameWork(accepted.work, complete.work);
            checkSamePath(*accepted.analysis, *complete.analysis);
            checkWorkBounds(accepted.work, preSpent, limits);
            CHECK(exact.remainingPieces() == 0); CHECK(exact.remainingVertices() == 0);
            CHECK(exact.remainingPairs() == 0); CHECK(exact.remainingOrderingComparisons() == 0);
        }
    }
    SECTION("One-less ordering comparison ceiling refuses without a partial certificate") {
        for (const bool reversed : {false, true}) { checkOneLess(circleArc(false, reversed), Resource::Ordering); }
    }
}

TEST_CASE("cache topology one-less cache span cap refuses atomically",
          "[lc-offsetcurve][geometry][cache-topology-chain][cache-topology-budget]") {
    for (const bool reversed : {false, true}) { checkOneLess(circleArc(false, reversed), Resource::Spans); }
}

TEST_CASE("cache topology one-less vertex cap refuses atomically",
          "[lc-offsetcurve][geometry][cache-topology-chain][cache-topology-budget]") {
    for (const bool reversed : {false, true}) { checkOneLess(circleArc(false, reversed), Resource::Vertices); }
}

TEST_CASE("cache topology one-less distance pair cap refuses atomically",
          "[lc-offsetcurve][geometry][cache-topology-chain][cache-topology-budget]") {
    for (const bool reversed : {false, true}) { checkOneLess(circleArc(false, reversed), Resource::Pairs); }
}

TEST_CASE("cache topology one-less combined depth cap refuses atomically",
          "[lc-offsetcurve][geometry][cache-topology-chain][cache-topology-budget]") {
    for (const bool reversed : {false, true}) {
        const auto fixture = circleArc(true, reversed);
        geo::WorkBudget budget;
        const auto complete = geo::checkCache(fixture.cache, fixture.definition, tolerance, budget);
        checkSuccess(complete, budget);
        REQUIRE(complete.work.deepest > 1);
        REQUIRE(std::any_of(complete.analysis->pieces().begin(), complete.analysis->pieces().end(), [](const auto& piece) {
            return piece.candidateDepth != 0 && piece.referenceDepth != 0;
        }));
        checkOneLess(fixture, Resource::Depth);
    }
}

TEST_CASE("cache topology preserves shared pre-spent work and sticky refusal",
          "[lc-offsetcurve][geometry][cache-topology-chain][cache-topology-budget]") {
    const auto fixture = circleArc();
    geo::WorkBudget plain;
    const auto baseline = geo::checkCache(fixture.cache, fixture.definition, tolerance, plain);
    checkSuccess(baseline, plain);
    auto expected = baseline.work;
    expected.cacheSpans += preSpent.cacheSpans; expected.vertices += preSpent.vertices;
    expected.distancePairs += preSpent.distancePairs; expected.orderingComparisons += preSpent.orderingComparisons;
    expected.deepest = std::max(expected.deepest, preSpent.deepest);
    std::unique_ptr<const geo::CacheTopologyAnalysis> saved;
    RS_Vector savedStart;
    {
        auto local = fixture;
        geo::WorkBudget budget{exactLimits(expected)};
        REQUIRE(budget.adoptWork(preSpent));
        auto complete = geo::checkCache(local.cache, local.definition, tolerance, budget);
        checkSuccess(complete, budget); checkSameWork(complete.work, expected);
        checkSamePath(*complete.analysis, *baseline.analysis);
        savedStart = local.cache.controlPoints.front(); saved = std::move(complete.analysis);
        const auto before = budget.consumed();
        const auto refused = geo::checkCache(local.cache, local.definition, tolerance, budget);
        REQUIRE(refused.reason == LC_OffsetCheckResult::Reason::LimitExceeded); CHECK_FALSE(refused.analysis);
        REQUIRE(budget.status() == geo::BudgetStatus::LimitExceeded);
        checkSameWork(refused.work, budget.consumed()); checkWorkBounds(refused.work, before, exactLimits(expected));
        const auto stopped = budget.consumed();
        const auto retry = geo::checkCache(local.cache, local.definition, tolerance, budget);
        CHECK(retry.reason == LC_OffsetCheckResult::Reason::LimitExceeded); CHECK_FALSE(retry.analysis);
        checkSameWork(retry.work, stopped); checkSameWork(budget.consumed(), stopped);
        local.cache.controlPoints.front() = RS_Vector{12345, -9876};
        local.cache.controlPoints.clear(); local.cache.knotslist.clear(); local.cache.weights.clear();
        local.definition.basis.center = RS_Vector{12345, -9876};
        local.definition.basis.angle1 = 1;
    }
    // The cache, definition and shared caller budget have died; all proof data remain owned.
    REQUIRE(saved); checkSamePath(*saved, *baseline.analysis);
    REQUIRE(saved->layout().spanCount() == 2);
    CHECK(saved->layout().spans[0].controls[0].x == savedStart.x);
    CHECK(saved->layout().spans[0].controls[0].y == savedStart.y);
    CHECK(saved->reference().events().providers().frame().sourceSweep().start == startAngle);
}
