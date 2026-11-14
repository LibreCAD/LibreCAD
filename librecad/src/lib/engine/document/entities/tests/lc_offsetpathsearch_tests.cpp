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
#include <exception>
#include <limits>
#include <numeric>
#include <stdexcept>
#include <tuple>
#include <utility>

#include "lc_offsetpathsearch.h"

namespace geo = LC_EllipseOffsetGeometry;

namespace {

const geo::CheckWork preSpent{2, 3, 5, 7, 11};

geo::CachePathJoin join(const std::size_t incoming, const std::size_t outgoing, const std::size_t event) {
    return {geo::CachePathJoinKind::PairEvent, incoming, outgoing, {100 + incoming}, {100 + outgoing}, event};
}

auto key(const geo::CachePathJoin& value) {
    return std::make_tuple(value.kind, value.incoming, value.outgoing, value.from.value, value.to.value, value.pairEvent);
}

struct Graph {
    std::size_t count = 0;
    bool closed = false;
    std::vector<std::size_t> offsets;
    std::vector<geo::CachePathJoin> transitions;
    std::vector<std::uint8_t> starts, ends;

    geo::OffsetPathGraphView view() const { return {offsets, transitions, starts, ends, count, closed}; }
};

Graph graph(const std::size_t count, const bool closed, const std::vector<geo::CachePathJoin>& transitions,
            const std::vector<std::size_t>& starts = {}, const std::vector<std::size_t>& ends = {}) {
    Graph out; out.count = count; out.closed = closed; out.offsets.push_back(0);
    for (std::size_t edge = 0; edge < count; ++edge) {
        for (const auto& transition : transitions) {
            if (transition.incoming == edge) { out.transitions.push_back(transition); }
        }
        out.offsets.push_back(out.transitions.size());
    }
    if (!closed) {
        out.starts.assign(count, 0); out.ends.assign(count, 0);
        for (const auto start : starts) { REQUIRE(start < count); out.starts[start] = 1; }
        for (const auto end : ends) { REQUIRE(end < count); out.ends[end] = 1; }
    }
    return out;
}

Graph twoCycles() {
    // Two cycles share edge 0: 0->1->0 and 0->2->3->0. No other return exists.
    return graph(4, true, {join(0, 1, 1), join(0, 2, 2), join(1, 0, 3), join(2, 3, 4), join(3, 0, 5)});
}

void checkSameWork(const geo::CheckWork& a, const geo::CheckWork& b) {
    CHECK(a.cacheSpans == b.cacheSpans); CHECK(a.vertices == b.vertices);
    CHECK(a.distancePairs == b.distancePairs); CHECK(a.deepest == b.deepest);
    CHECK(a.orderingComparisons == b.orderingComparisons);
}

LC_OffsetCheckLimits exactLimits(const geo::CheckWork& work) {
    return {work.cacheSpans, work.vertices, work.distancePairs, work.deepest, work.orderingComparisons};
}

void checkBounded(const geo::CheckWork& work, const geo::CheckWork& before, const LC_OffsetCheckLimits& limits) {
    CHECK(work.cacheSpans >= before.cacheSpans); CHECK(work.cacheSpans <= limits.maxCacheSpans);
    CHECK(work.vertices >= before.vertices); CHECK(work.vertices <= limits.maxVertices);
    CHECK(work.distancePairs >= before.distancePairs); CHECK(work.distancePairs <= limits.maxDistancePairs);
    CHECK(work.deepest >= before.deepest); CHECK(work.deepest <= limits.maxDepth);
    CHECK(work.orderingComparisons >= before.orderingComparisons);
    CHECK(work.orderingComparisons <= limits.maxOrderingComparisons);
}

void checkRoute(const Graph& input, const geo::OffsetPathRouteView& route) {
    REQUIRE_FALSE(route.edges.empty()); CHECK(route.closed == input.closed);
    REQUIRE(route.joins.size() == route.edges.size() - (input.closed ? 0 : 1));
    std::vector<bool> seen(input.count, false);
    for (const auto edge : route.edges) { REQUIRE(edge < input.count); CHECK_FALSE(seen[edge]); seen[edge] = true; }
    if (input.closed) { CHECK(route.edges.front() == *std::min_element(route.edges.begin(), route.edges.end())); }
    else { CHECK(input.starts[route.edges.front()] == 1); CHECK(input.ends[route.edges.back()] == 1); }
    for (std::size_t i = 0; i < route.joins.size(); ++i) {
        const auto& transition = route.joins[i];
        CHECK(transition.incoming == route.edges[i]);
        CHECK(transition.outgoing == route.edges[(i + 1) % route.edges.size()]);
        CHECK(std::count_if(input.transitions.begin(), input.transitions.end(), [&](const auto& candidate) {
            return key(candidate) == key(transition);
        }) == 1);
    }
}

using Routes = std::vector<std::vector<std::size_t>>;
geo::OffsetPathSearchResult collect(const Graph& input, geo::WorkBudget& budget, Routes& routes) {
    return geo::searchOffsetPaths(input.view(), budget, [&](const auto& route) {
        checkRoute(input, route); routes.push_back(route.edges); return geo::OffsetPathVisit::Continue;
    });
}

void checkGraphUnchanged(const Graph& actual, const Graph& original) {
    CHECK(actual.count == original.count); CHECK(actual.closed == original.closed);
    CHECK(actual.offsets == original.offsets); CHECK(actual.starts == original.starts); CHECK(actual.ends == original.ends);
    REQUIRE(actual.transitions.size() == original.transitions.size());
    for (std::size_t i = 0; i < actual.transitions.size(); ++i) { CHECK(key(actual.transitions[i]) == key(original.transitions[i])); }
}

} // namespace

TEST_CASE("cache topology rejects distinct directed event cycles",
          "[lc-offsetcurve][geometry][cache-topology-routing][offset-path-search]") {
    // C27 is a routing-kernel obligation, NOT a public ellipse/candidate geometry fixture.
    const auto input = twoCycles();
    geo::WorkBudget budget;
    std::size_t accepted = 0;
    Routes paths;
    const auto result = geo::searchOffsetPaths(input.view(), budget, [&](const auto& route) {
        checkRoute(input, route); paths.push_back(route.edges);
        return ++accepted == 2 ? geo::OffsetPathVisit::Stop : geo::OffsetPathVisit::Continue;
    });
    const Routes expected{{0, 1}, {0, 2, 3}};
    CHECK(paths == expected); CHECK(accepted == 2); CHECK(result.routesVisited == 2);
    CHECK(result.status == geo::OffsetPathSearchStatus::Stopped);
    CHECK(budget.status() == geo::BudgetStatus::Ok); checkSameWork(result.work, budget.consumed());
}

TEST_CASE("Offset path search removes cycle rotations but not different edge orders",
          "[lc-offsetcurve][geometry][offset-path-search]") {
    const auto input = graph(3, true, {join(0, 1, 1), join(0, 2, 2), join(1, 0, 3),
                                     join(1, 2, 4), join(2, 0, 5), join(2, 1, 6)});
    geo::WorkBudget budget; Routes paths;
    const auto result = collect(input, budget, paths);
    // Three 2-edge cycles and both opposite 3-edge orders; rotations add nothing.
    const Routes expected{{0, 1}, {0, 1, 2}, {0, 2}, {0, 2, 1}, {1, 2}};
    CHECK(paths == expected); CHECK(result.routesVisited == 5);
    CHECK(result.status == geo::OffsetPathSearchStatus::Exhausted);
}

TEST_CASE("Offset path search preserves exact alternative transition identities",
          "[lc-offsetcurve][geometry][offset-path-search]") {
    const auto input = graph(2, true, {join(0, 1, 10), join(0, 1, 20), join(1, 0, 30)});
    geo::WorkBudget budget;
    std::vector<std::size_t> events;
    const std::vector<std::size_t> expectedEdges{0, 1};
    const auto result = geo::searchOffsetPaths(input.view(), budget, [&](const auto& route) {
        checkRoute(input, route); REQUIRE(route.edges == expectedEdges);
        events.push_back(route.joins.front().pairEvent); return geo::OffsetPathVisit::Continue;
    });
    const std::vector<std::size_t> expected{10, 20};
    CHECK(events == expected); CHECK(result.routesVisited == 2);
    CHECK(result.status == geo::OffsetPathSearchStatus::Exhausted);
}

TEST_CASE("Offset path search keeps different edges at shared event ports distinct",
          "[lc-offsetcurve][geometry][offset-path-search]") {
    auto a = join(0, 0, 10), b = join(1, 1, 20);
    a.from = b.from = {77}; a.to = b.to = {88};
    const auto input = graph(2, true, {a, b});
    geo::WorkBudget budget; Routes paths;
    const auto result = collect(input, budget, paths);
    const Routes expected{{0}, {1}};
    CHECK(paths == expected); CHECK(result.routesVisited == 2);
    CHECK(result.status == geo::OffsetPathSearchStatus::Exhausted);
}

TEST_CASE("Offset path search emits only eligible open routes including eligible prefixes",
          "[lc-offsetcurve][geometry][offset-path-search]") {
    const auto input = graph(3, false, {join(0, 1, 1), join(1, 2, 2)}, {0, 1}, {1, 2});
    geo::WorkBudget budget; Routes paths;
    const auto result = collect(input, budget, paths);
    const Routes expected{{0, 1}, {0, 1, 2}, {1}, {1, 2}};
    CHECK(paths == expected); CHECK(result.routesVisited == 4);
    CHECK(result.status == geo::OffsetPathSearchStatus::Exhausted);
}

TEST_CASE("Offset path search needs both endpoint eligibilities for open routes",
          "[lc-offsetcurve][geometry][offset-path-search]") {
    for (const bool omitStart : {false, true}) {
        const auto input = graph(3, false, {join(0, 1, 1), join(1, 2, 2)},
                                 omitStart ? std::vector<std::size_t>{} : std::vector<std::size_t>{0},
                                 omitStart ? std::vector<std::size_t>{2} : std::vector<std::size_t>{});
        geo::WorkBudget budget; Routes paths;
        const auto result = collect(input, budget, paths);
        CHECK(paths.empty()); CHECK(result.routesVisited == 0);
        CHECK(result.status == geo::OffsetPathSearchStatus::Exhausted); CHECK(result.work.vertices == 0);
    }
}

TEST_CASE("Offset path search handles empty graphs and singleton closure without repeated laps",
          "[lc-offsetcurve][geometry][offset-path-search]") {
    for (const bool closed : {false, true}) {
        const auto input = graph(0, closed, {});
        geo::WorkBudget budget; Routes paths;
        const auto result = collect(input, budget, paths);
        CHECK(paths.empty()); CHECK(result.routesVisited == 0); CHECK(result.work.vertices == 0);
        CHECK(result.status == geo::OffsetPathSearchStatus::Exhausted);
    }
    const auto input = graph(1, true, {join(0, 0, 1)});
    geo::WorkBudget budget; Routes paths;
    const auto result = collect(input, budget, paths);
    const Routes expected{{0}};
    CHECK(paths == expected); CHECK(result.routesVisited == 1);
    CHECK(result.status == geo::OffsetPathSearchStatus::Exhausted);
}

TEST_CASE("Offset path search supports 64 edges without spending geometric refinement depth",
          "[lc-offsetcurve][geometry][offset-path-search]") {
    for (const bool closed : {false, true}) {
        std::vector<geo::CachePathJoin> transitions;
        for (std::size_t i = 0; i < 63; ++i) { transitions.push_back(join(i, i + 1, i)); }
        if (closed) { transitions.push_back(join(63, 0, 63)); }
        const auto input = graph(64, closed, transitions, closed ? std::vector<std::size_t>{} : std::vector<std::size_t>{0},
                                 closed ? std::vector<std::size_t>{} : std::vector<std::size_t>{63});
        geo::WorkBudget budget; REQUIRE(budget.adoptWork(preSpent)); Routes paths;
        const auto result = collect(input, budget, paths);
        REQUIRE(result.status == geo::OffsetPathSearchStatus::Exhausted); REQUIRE(paths.size() == 1);
        std::vector<std::size_t> expected(64); std::iota(expected.begin(), expected.end(), 0);
        CHECK(paths.front() == expected); CHECK(result.routesVisited == 1);
        CHECK(result.work.deepest == preSpent.deepest); CHECK(result.work.cacheSpans == preSpent.cacheSpans);
    }
}

TEST_CASE("Offset path search reproduces exact cumulative resource caps",
          "[lc-offsetcurve][geometry][offset-path-search]") {
    const auto input = twoCycles(), original = input;
    geo::WorkBudget baseline; REQUIRE(baseline.adoptWork(preSpent)); Routes expected;
    const auto measured = collect(input, baseline, expected);
    REQUIRE(measured.status == geo::OffsetPathSearchStatus::Exhausted);
    geo::WorkBudget exact{exactLimits(measured.work)}; REQUIRE(exact.adoptWork(preSpent)); Routes actual;
    const auto result = collect(input, exact, actual);
    CHECK(result.status == geo::OffsetPathSearchStatus::Exhausted); CHECK(result.routesVisited == 2);
    CHECK(actual == expected); checkSameWork(result.work, measured.work); checkSameWork(result.work, exact.consumed());
    CHECK(exact.remainingVertices() == 0); CHECK(exact.remainingPairs() == 0); CHECK(exact.remainingOrderingComparisons() == 0);
    checkGraphUnchanged(input, original);
}

TEST_CASE("Offset path search refuses one less resource without publishing incomplete routes",
          "[lc-offsetcurve][geometry][offset-path-search]") {
    const auto input = twoCycles();
    geo::WorkBudget baseline; REQUIRE(baseline.adoptWork(preSpent)); Routes expected;
    const auto measured = collect(input, baseline, expected);
    REQUIRE(measured.status == geo::OffsetPathSearchStatus::Exhausted);
    for (const unsigned dimension : {0u, 1u, 2u}) {
        CAPTURE(dimension);
        auto limits = exactLimits(measured.work);
        if (dimension == 0) { REQUIRE(limits.maxVertices > preSpent.vertices); --limits.maxVertices; }
        if (dimension == 1) { REQUIRE(limits.maxDistancePairs > preSpent.distancePairs); --limits.maxDistancePairs; }
        if (dimension == 2) { REQUIRE(limits.maxOrderingComparisons > preSpent.orderingComparisons); --limits.maxOrderingComparisons; }
        geo::WorkBudget budget{limits}; REQUIRE(budget.adoptWork(preSpent)); Routes paths;
        const auto result = collect(input, budget, paths);
        CHECK(result.status == geo::OffsetPathSearchStatus::LimitExceeded);
        CHECK(budget.status() == geo::BudgetStatus::LimitExceeded); CHECK(result.routesVisited == paths.size());
        REQUIRE(paths.size() <= expected.size()); CHECK(std::equal(paths.begin(), paths.end(), expected.begin()));
        checkSameWork(result.work, budget.consumed()); checkBounded(result.work, preSpent, limits);
        const auto stopped = budget.consumed();
        const auto retry = collect(input, budget, paths);
        CHECK(retry.status == geo::OffsetPathSearchStatus::LimitExceeded); CHECK(retry.routesVisited == 0);
        checkSameWork(retry.work, stopped); checkSameWork(budget.consumed(), stopped);
    }
}

TEST_CASE("Offset path search admits scratch roots and child records atomically",
          "[lc-offsetcurve][geometry][offset-path-search]") {
    SECTION("Markers cannot be allocated beyond the remaining record allowance") {
        const auto input = graph(3, false, {join(0, 1, 1), join(1, 2, 2)}, {0}, {2});
        LC_OffsetCheckLimits limits; limits.maxVertices = 2;
        geo::WorkBudget budget{limits}; Routes paths;
        const auto result = collect(input, budget, paths);
        CHECK(result.status == geo::OffsetPathSearchStatus::LimitExceeded); checkSameWork(result.work, {});
        CHECK(paths.empty());
    }
    SECTION("A root frame and its edge slot require both records") {
        const auto input = graph(1, false, {}, {0}, {0});
        LC_OffsetCheckLimits limits; limits.maxVertices = 2;
        geo::WorkBudget budget{limits}; Routes paths;
        const auto result = collect(input, budget, paths);
        CHECK(result.status == geo::OffsetPathSearchStatus::LimitExceeded); CHECK(paths.empty());
        CHECK(result.work.vertices == 1); CHECK(result.work.distancePairs == 3);
    }
    SECTION("A child frame edge and join require three records together") {
        const auto input = graph(2, false, {join(0, 1, 1)}, {0}, {1});
        LC_OffsetCheckLimits limits; limits.maxVertices = 6;
        geo::WorkBudget budget{limits}; Routes paths;
        const auto result = collect(input, budget, paths);
        CHECK(result.status == geo::OffsetPathSearchStatus::LimitExceeded); CHECK(paths.empty());
        // Root marker/frame/edge are live (4); one key-word hash entered (pair 6).
        // The three-record memo key/state/frame cannot fit the two remaining slots.
        CHECK(result.work.vertices == 4); CHECK(result.work.distancePairs == 6);
        CHECK(result.work.orderingComparisons == 0);
    }
}

TEST_CASE("Offset path search admits callback work before invoking a visitor",
          "[lc-offsetcurve][geometry][offset-path-search]") {
    const auto input = graph(1, false, {}, {0}, {0});
    for (const std::size_t cap : {7u, 8u}) {
        CAPTURE(cap);
        LC_OffsetCheckLimits limits; limits.maxDistancePairs = cap;
        geo::WorkBudget budget{limits}; Routes paths;
        const auto result = collect(input, budget, paths);
        CHECK(result.status == (cap == 8 ? geo::OffsetPathSearchStatus::Exhausted : geo::OffsetPathSearchStatus::LimitExceeded));
        CHECK(result.routesVisited == (cap == 8 ? 1 : 0)); CHECK(paths.size() == result.routesVisited);
        // Validation 2 + seed 1 + memo hash/copy/finish 3 + walk 1 + callback 1.
        CHECK(result.work.distancePairs == cap); CHECK(result.work.vertices == 6);
    }
}

TEST_CASE("Offset path search admits pair and ordering predicates together",
          "[lc-offsetcurve][geometry][offset-path-search]") {
    for (const bool duringValidation : {false, true}) {
        CAPTURE(duringValidation);
        const auto input = graph(1, true, duringValidation ?
            std::vector<geo::CachePathJoin>{join(0, 0, 1), join(0, 0, 2)} :
            std::vector<geo::CachePathJoin>{join(0, 0, 1)});
        LC_OffsetCheckLimits limits; limits.maxOrderingComparisons = 1;
        geo::WorkBudget budget{limits}; REQUIRE(budget.consumeOrderingComparisons(1)); Routes paths;
        const auto result = collect(input, budget, paths);
        CHECK(result.status == geo::OffsetPathSearchStatus::LimitExceeded); CHECK(paths.empty());
        CHECK(result.work.orderingComparisons == 1);
        CHECK(result.work.distancePairs == (duringValidation ? 4 : 7));
        CHECK(result.work.vertices == (duringValidation ? 0 : 6));
        // The refused next predicate cannot consume only its pair half.
        checkSameWork(result.work, budget.consumed());
    }
}

TEST_CASE("Offset path search honors geometric depth admitted by its shared budget visitor",
          "[lc-offsetcurve][geometry][offset-path-search]") {
    const auto input = graph(1, false, {}, {0}, {0});
    for (const unsigned cap : {8u, 9u}) {
        LC_OffsetCheckLimits limits; limits.maxDepth = cap;
        geo::WorkBudget budget{limits}; REQUIRE(budget.adoptWork(preSpent));
        const auto result = geo::searchOffsetPaths(input.view(), budget, [&](const auto&) {
            CHECK(budget.checkDepth(9) == (cap == 9)); return geo::OffsetPathVisit::Stop;
        });
        CHECK(result.status == (cap == 9 ? geo::OffsetPathSearchStatus::Stopped : geo::OffsetPathSearchStatus::LimitExceeded));
        CHECK(result.routesVisited == 1); CHECK(result.work.deepest == (cap == 9 ? 9 : preSpent.deepest));
        checkSameWork(result.work, budget.consumed()); checkBounded(result.work, preSpent, limits);
        CHECK(result.work.cacheSpans == preSpent.cacheSpans);
    }
}

TEST_CASE("Offset path search preserves sticky caller states before inspecting malformed input",
          "[lc-offsetcurve][geometry][offset-path-search]") {
    auto input = graph(0, true, {}); input.count = std::numeric_limits<std::size_t>::max();
    for (const bool exhausted : {false, true}) {
        geo::WorkBudget budget; REQUIRE(budget.adoptWork(preSpent));
        if (exhausted) { budget.latchLimitExceeded(); } else { budget.latchInvalidInput(); }
        const auto stopped = budget.consumed();
        const auto result = geo::searchOffsetPaths(input.view(), budget, {});
        CHECK(result.status == (exhausted ? geo::OffsetPathSearchStatus::LimitExceeded : geo::OffsetPathSearchStatus::InvalidInput));
        CHECK(result.routesVisited == 0); checkSameWork(result.work, stopped); checkSameWork(budget.consumed(), stopped);
    }
}

TEST_CASE("Offset path search rejects invalid CSR and duplicate semantic transitions before callbacks",
          "[lc-offsetcurve][geometry][offset-path-search]") {
    for (const unsigned invalidKind : {0u, 1u, 2u, 3u, 4u, 5u, 6u, 7u, 8u, 9u}) {
        CAPTURE(invalidKind);
        auto input = graph(2, true, {join(0, 1, 1), join(1, 0, 2)});
        if (invalidKind == 0) { input.offsets.pop_back(); }
        if (invalidKind == 1) { input.offsets[0] = 1; }
        if (invalidKind == 2) { input.offsets[1] = 3; }
        if (invalidKind == 3) { input.offsets.back() = 0; }
        if (invalidKind == 4) { input.transitions[0].incoming = 1; }
        if (invalidKind == 5) { input.transitions[0].outgoing = 2; }
        if (invalidKind == 6) { input.transitions[0].kind = static_cast<geo::CachePathJoinKind>(999); }
        if (invalidKind == 7) { input.starts = {1, 0}; }
        if (invalidKind == 8) { input = graph(2, false, {join(0, 1, 1)}, {0}, {1}); input.ends[1] = 2; }
        if (invalidKind == 9) { input = graph(2, true, {join(0, 1, 1), join(0, 1, 1), join(1, 0, 2)}); }
        geo::WorkBudget budget; Routes paths;
        const auto result = collect(input, budget, paths);
        CHECK(result.status == geo::OffsetPathSearchStatus::InvalidInput); CHECK(paths.empty()); CHECK(result.routesVisited == 0);
        CHECK(budget.status() == geo::BudgetStatus::InvalidInput);
        const auto stopped = budget.consumed();
        const auto retry = collect(input, budget, paths);
        CHECK(retry.status == geo::OffsetPathSearchStatus::InvalidInput); checkSameWork(retry.work, stopped);
    }
    const auto input = graph(0, true, {});
    geo::WorkBudget budget;
    const auto result = geo::searchOffsetPaths(input.view(), budget, {});
    CHECK(result.status == geo::OffsetPathSearchStatus::InvalidInput); checkSameWork(result.work, {});
}

TEST_CASE("Offset path search propagates original visitor exceptions without retaining input or budget",
          "[lc-offsetcurve][geometry][offset-path-search]") {
    const auto input = twoCycles(), original = input;
    const auto exception = std::make_exception_ptr(std::runtime_error("route visitor failure"));
    geo::WorkBudget budget; REQUIRE(budget.adoptWork(preSpent));
    std::size_t callbacks = 0;
    bool caught = false;
    try {
        geo::searchOffsetPaths(input.view(), budget, [&](const auto& route) -> geo::OffsetPathVisit {
            checkRoute(input, route); ++callbacks; std::rethrow_exception(exception);
        });
    } catch (...) { caught = true; CHECK(std::current_exception() == exception); }
    REQUIRE(caught); CHECK(callbacks == 1); CHECK(budget.status() == geo::BudgetStatus::Ok);
    CHECK(budget.consumed().vertices > preSpent.vertices); CHECK(budget.consumed().distancePairs > preSpent.distancePairs);
    CHECK(budget.consumed().deepest == preSpent.deepest); checkGraphUnchanged(input, original);
    const auto beforeRetry = budget.consumed();
    Routes paths;
    const auto retry = collect(input, budget, paths);
    const Routes expected{{0, 1}, {0, 2, 3}};
    CHECK(retry.status == geo::OffsetPathSearchStatus::Exhausted); CHECK(paths == expected);
    CHECK(retry.work.vertices > beforeRetry.vertices); CHECK(retry.work.distancePairs > beforeRetry.distancePairs);
}

TEST_CASE("Offset path search stops explicitly and cannot hide visitor budget refusal",
          "[lc-offsetcurve][geometry][offset-path-search]") {
    const auto input = twoCycles();
    SECTION("The first complete route can stop the enumeration") {
        geo::WorkBudget budget;
        const auto result = geo::searchOffsetPaths(input.view(), budget, [&](const auto& route) {
            checkRoute(input, route); return geo::OffsetPathVisit::Stop;
        });
        CHECK(result.status == geo::OffsetPathSearchStatus::Stopped); CHECK(result.routesVisited == 1);
    }
    SECTION("An invalid callback decision is not an implicit continue or success") {
        geo::WorkBudget budget;
        const auto result = geo::searchOffsetPaths(input.view(), budget, [](const auto&) {
            return static_cast<geo::OffsetPathVisit>(999);
        });
        CHECK(result.status == geo::OffsetPathSearchStatus::InvalidInput); CHECK(result.routesVisited == 1);
        CHECK(budget.status() == geo::BudgetStatus::InvalidInput);
    }
    SECTION("A stop does not override a failed atomic visitor work admission") {
        geo::WorkBudget budget;
        const auto result = geo::searchOffsetPaths(input.view(), budget, [&](const auto&) {
            CHECK_FALSE(budget.consumeVertices(budget.remainingVertices() + 1)); return geo::OffsetPathVisit::Stop;
        });
        CHECK(result.status == geo::OffsetPathSearchStatus::LimitExceeded); CHECK(result.routesVisited == 1);
        CHECK(budget.status() == geo::BudgetStatus::LimitExceeded);
    }
}

TEST_CASE("Memoized offset suffixes preserve different ordered prefix multiplicity",
          "[lc-offsetcurve][geometry][offset-path-search][cache-search-completion]") {
    const auto input = graph(5, false, {join(0,1,1), join(0,2,2), join(1,2,3), join(1,3,4),
        join(2,1,5), join(2,3,6), join(3,4,7)}, {0}, {4});
    geo::WorkBudget budget; Routes paths;
    const auto result = collect(input, budget, paths);
    const Routes expected{{0,1,2,3,4}, {0,1,3,4}, {0,2,1,3,4}, {0,2,3,4}};
    REQUIRE(result.status == geo::OffsetPathSearchStatus::Exhausted);
    CHECK(paths == expected); CHECK(result.routesVisited == 4);
    CHECK(result.structuralRoutes == 2); CHECK(result.memoHits > 0); CHECK(result.memoStates > 0);
    // Both five-edge prefixes reach (seed=0, edge=3, used={0,1,2,3}).
    // Sharing its one suffix must NEVER collapse the two whole route orders.
    checkSameWork(result.work, budget.consumed());
}

TEST_CASE("Memoized offset suffixes retain distinct transition prefixes and saturate at two",
          "[lc-offsetcurve][geometry][offset-path-search][cache-search-completion]") {
    const auto input = graph(3, false, {join(0,1,10), join(0,1,20), join(0,1,30), join(1,2,40)}, {0}, {2});
    geo::WorkBudget budget; std::vector<std::size_t> events;
    const auto result = geo::searchOffsetPaths(input.view(), budget, [&](const auto& route) {
        checkRoute(input, route); events.push_back(route.joins.front().pairEvent);
        return geo::OffsetPathVisit::Continue;
    });
    const std::vector<std::size_t> expected{10,20,30};
    REQUIRE(result.status == geo::OffsetPathSearchStatus::Exhausted);
    CHECK(events == expected); CHECK(result.routesVisited == 3); CHECK(result.structuralRoutes == 2);
    CHECK(result.memoStates == 3); CHECK(result.memoHits == 2);
}

TEST_CASE("Memoized offset dead suffixes prove zero without losing endpoint eligibility",
          "[lc-offsetcurve][geometry][offset-path-search][cache-search-completion]") {
    const auto input = graph(5, false, {join(0,1,1), join(0,2,2), join(1,2,3), join(1,3,4),
        join(2,1,5), join(2,3,6)}, {0}, {4});
    geo::WorkBudget budget; Routes paths;
    const auto result = collect(input, budget, paths);
    CHECK(result.status == geo::OffsetPathSearchStatus::Exhausted); CHECK(paths.empty());
    CHECK(result.structuralRoutes == 0); CHECK(result.routesVisited == 0);
    CHECK(result.memoHits > 0); CHECK(result.memoStates > 0);
}

TEST_CASE("Memoized offset state keys retain all used words and exact cumulative caps",
          "[lc-offsetcurve][geometry][offset-path-search][cache-search-completion]") {
    std::vector<geo::CachePathJoin> transitions;
    for (std::size_t i = 0; i < 64; ++i) { transitions.push_back(join(i, i+1, i)); }
    const auto input = graph(65, false, transitions, {0}, {64}), original = input;
    geo::WorkBudget measuredBudget; REQUIRE(measuredBudget.adoptWork(preSpent)); Routes expected;
    const auto measured = collect(input, measuredBudget, expected);
    REQUIRE(measured.status == geo::OffsetPathSearchStatus::Exhausted);
    REQUIRE(expected.size() == 1); CHECK(measured.structuralRoutes == 1); CHECK(measured.memoStates == 65);
    auto limits = exactLimits(measured.work);
    geo::WorkBudget exact{limits}; REQUIRE(exact.adoptWork(preSpent)); Routes actual;
    const auto accepted = collect(input, exact, actual);
    CHECK(accepted.status == geo::OffsetPathSearchStatus::Exhausted); CHECK(actual == expected);
    checkSameWork(accepted.work, measured.work);
    for (unsigned dimension = 0; dimension < 3; ++dimension) {
        auto shortLimits = limits;
        if (dimension == 0) { --shortLimits.maxVertices; }
        if (dimension == 1) { --shortLimits.maxDistancePairs; }
        if (dimension == 2) { --shortLimits.maxOrderingComparisons; }
        geo::WorkBudget shortBudget{shortLimits}; REQUIRE(shortBudget.adoptWork(preSpent)); Routes prefix;
        const auto refused = collect(input, shortBudget, prefix);
        CHECK(refused.status == geo::OffsetPathSearchStatus::LimitExceeded);
        checkBounded(refused.work, preSpent, shortLimits); checkSameWork(refused.work, shortBudget.consumed());
        const auto stopped = shortBudget.consumed();
        const auto retry = collect(input, shortBudget, prefix);
        CHECK(retry.status == geo::OffsetPathSearchStatus::LimitExceeded); CHECK(retry.routesVisited == 0);
        checkSameWork(retry.work, stopped);
    }
    checkGraphUnchanged(input, original);
}

TEST_CASE("Memoized structural multiplicity never becomes geometric acceptance",
          "[lc-offsetcurve][geometry][offset-path-search][cache-search-completion]") {
    const auto input = twoCycles();
    geo::WorkBudget budget; std::size_t visited = 0;
    const auto result = geo::searchOffsetPaths(input.view(), budget, [&](const auto& route) {
        checkRoute(input, route); ++visited;
        // A real geometry consumer may reject the first and accept the second;
        // the structural count cannot stop or fabricate that decision.
        return geo::OffsetPathVisit::Continue;
    });
    CHECK(result.status == geo::OffsetPathSearchStatus::Exhausted);
    CHECK(result.structuralRoutes == 2); CHECK(visited == 2); CHECK(result.routesVisited == 2);
}

TEST_CASE("Memoized route replay honors sticky cancellation after complete prefix admission",
          "[lc-offsetcurve][geometry][offset-path-search][cache-search-completion]") {
    const auto input = twoCycles();
    geo::WorkBudget budget; geo::CheckWork atCancellation; unsigned callbacks = 0;
    const auto result = geo::searchOffsetPaths(input.view(), budget, [&](const auto&) {
        ++callbacks; budget.latchInvalidInput(); atCancellation = budget.consumed();
        return geo::OffsetPathVisit::Continue;
    });
    CHECK(result.status == geo::OffsetPathSearchStatus::InvalidInput); CHECK(callbacks == 1);
    checkSameWork(result.work, atCancellation); checkSameWork(budget.consumed(), atCancellation);
    Routes paths;
    const auto retry = collect(input, budget, paths);
    CHECK(retry.status == geo::OffsetPathSearchStatus::InvalidInput); CHECK(paths.empty());
    checkSameWork(retry.work, atCancellation);
}
