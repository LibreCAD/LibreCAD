/*******************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 * Copyright (C) 2026 LibreCAD.org
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later version.
 ******************************************************************************/

#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <limits>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

#include "lc_offsetcontactmatching.h"

namespace geo = LC_EllipseOffsetGeometry;
using Status = geo::CacheContactMatchStatus;

namespace {
const geo::CheckWork preSpent{2, 3, 5, 7, 11};

geo::CacheSpan line(const RS_Vector a, const RS_Vector b) {
    geo::CacheSpan span;
    span.degree = 1; span.controls[0] = a; span.controls[1] = b;
    span.parameter = LC_Interval::hull(11, 13);
    return span;
}

geo::CacheLayoutResult layout(std::vector<geo::CacheSpan> spans) {
    geo::CacheLayoutResult out;
    out.status = geo::CacheLayoutStatus::Ok; out.spans = std::move(spans);
    for (std::size_t i = 0; i < out.spans.size(); ++i) {
        out.spans[i].parameter = LC_Interval::hull(11+2*i, 13+2*i);
    }
    return out;
}

geo::CacheLayoutResult crossingLines(const RS_Vector origin = {0, 0}) {
    return layout({line(origin, origin+RS_Vector{2, 2}),
                   line(origin+RS_Vector{2, 2}, origin+RS_Vector{0, 2}),
                   line(origin+RS_Vector{0, 2}, origin+RS_Vector{2, 0})});
}

std::unique_ptr<geo::CacheContactAnalysis> query(const geo::CacheLayoutResult& source,
    const bool reversed = false, const bool closed = false, const RS_Vector centre = {0, 0}) {
    const auto frame = geo::ReferenceFrame::create({centre, RS_Vector{1, 0}, .5, .25, .75, false}, .125);
    REQUIRE(frame);
    geo::WorkBudget budget;
    auto result = geo::analyseCacheContacts(source, *frame, reversed, closed, budget);
    CAPTURE(result.status, result.work.vertices, result.work.distancePairs, result.work.deepest);
    REQUIRE(result.status == LC_IntersectionStatus::Ok); REQUIRE(result.analysis);
    return std::move(result.analysis);
}

geo::CacheContactPort port(const std::size_t node, const std::size_t segment, const double parameter) {
    geo::CacheContactPort result;
    result.referenceNode = node; result.positions[0] = {segment, parameter}; result.positionCount = 1;
    return result;
}

std::vector<geo::CacheContactPort> crossingPorts(const double parameter = .5) {
    return {port(77, 0, parameter), port(77, 2, parameter)};
}

void sameWork(const geo::CheckWork& a, const geo::CheckWork& b) {
    CHECK(a.cacheSpans == b.cacheSpans); CHECK(a.vertices == b.vertices);
    CHECK(a.distancePairs == b.distancePairs); CHECK(a.deepest == b.deepest);
    CHECK(a.orderingComparisons == b.orderingComparisons);
}

LC_OffsetCheckLimits limitsFor(const geo::CheckWork& work) {
    return {work.cacheSpans, work.vertices, work.distancePairs, work.deepest, work.orderingComparisons};
}

void complete(const geo::CacheContactMatchResult& result, const geo::CacheContactAnalysis& contacts,
    const geo::WorkBudget& budget) {
    CAPTURE(result.status, result.work.vertices, result.work.distancePairs, result.work.orderingComparisons);
    REQUIRE(result.status == Status::Matched); REQUIRE(bool(result));
    REQUIRE(result.matches.size() == contacts.contactCount());
    for (std::size_t i = 0; i < result.matches.size(); ++i) {
        CHECK(result.matches[i].rootIndex == i);
        CHECK(result.matches[i].firstPort == 0); CHECK(result.matches[i].secondPort == 1);
    }
    sameWork(result.work, budget.consumed());
}

void refusal(const geo::CacheContactMatchResult& result, const Status expected, const geo::WorkBudget& budget) {
    CAPTURE(result.status);
    CHECK(result.status == expected); CHECK_FALSE(bool(result)); CHECK(result.matches.empty());
    sameWork(result.work, budget.consumed());
}

static_assert(!std::is_constructible_v<geo::CacheContactAnalysis, LC_CertifiedRootSnapshot>);
static_assert(!std::is_constructible_v<LC_CertifiedRootHandle, LC_CertifiedRootSnapshot>);
} // namespace

TEST_CASE("Contact matching anchors owned transverse roots at exact dyadic ports",
          "[lc-offsetcurve][geometry][contact-matching]") {
    const auto contacts = query(crossingLines());
    REQUIRE(contacts->contactCount() == 1);
    const auto root = contacts->snapshot(contacts->contactHandle(0)); REQUIRE(root);
    CHECK(root->event.proof == LC_IntersectionProof::IntervalUniqueTransverse);
    CHECK(root->event.isolation.segmentA == 0); CHECK(root->event.isolation.segmentB == 2);
    CHECK(root->event.isolation.parameterA.contains(.5)); CHECK(root->event.isolation.parameterB.contains(2.5));
    geo::WorkBudget budget;
    const auto result = geo::matchCacheContacts(*contacts, crossingPorts(), budget);
    complete(result, *contacts, budget);
    CHECK(result.work.vertices == 3); // Two prepared ports and one complete match, not reserved capacity.
    CHECK(result.work.cacheSpans == 0); CHECK(result.work.deepest == 0);
}

TEST_CASE("Contact matching evaluates reversed raw controls in normalized segment order",
          "[lc-offsetcurve][geometry][contact-matching]") {
    const auto source = layout({line({0, 0}, {4, 4}), line({4, 4}, {0, 4}), line({0, 4}, {4, -8})});
    for (const bool reversed : {false, true}) {
        const auto contacts = query(source, reversed);
        REQUIRE(contacts->contactCount() == 1);
        CHECK(contacts->rawSpanId(0) == (reversed ? 2 : 0));
        geo::WorkBudget budget;
        complete(geo::matchCacheContacts(*contacts, crossingPorts(reversed ? .75 : .25), budget), *contacts, budget);
    }
}

TEST_CASE("Contact matching evaluates actual binary cubic Bernstein controls exactly",
          "[lc-offsetcurve][geometry][contact-matching]") {
    geo::CacheSpan a, b;
    a.degree = b.degree = 3;
    a.parameter = b.parameter = LC_Interval::hull(0, 1);
    a.controls = {RS_Vector{0, 0}, RS_Vector{3, -1}, RS_Vector{4, 0}, RS_Vector{3, 3}};
    b.controls = {RS_Vector{3, 3}, RS_Vector{2, 0}, RS_Vector{3, -1}, RS_Vector{6, 0}};
    const auto source = layout({a, b});
    for (const bool reversed : {false, true}) {
        const auto contacts = query(source, reversed);
        REQUIRE(contacts->contactCount() == 1);
        auto ports = crossingPorts(); ports[1].positions[0].segment = 1;
        geo::WorkBudget budget;
        complete(geo::matchCacheContacts(*contacts, ports, budget), *contacts, budget);
    }
}

TEST_CASE("Contact matching accepts coherent construction-knot and closed-seam port aliases",
          "[lc-offsetcurve][geometry][contact-matching]") {
    for (const bool closed : {false, true}) {
        auto source = crossingLines();
        if (closed) { source.spans.push_back(line({2, 0}, {0, 0})); }
        const auto contacts = query(source, false, closed);
        REQUIRE(contacts->contactCount() == 1);
        auto ports = crossingPorts();
        auto knot = port(88, 0, 1);
        knot.positions[1] = {1, 0}; knot.positionCount = 2;
        ports.push_back(knot);
        if (closed) {
            auto seam = port(99, 0, 0);
            seam.positions[1] = {3, 1}; seam.positionCount = 2;
            ports.push_back(seam);
        }
        geo::WorkBudget budget;
        complete(geo::matchCacheContacts(*contacts, ports, budget), *contacts, budget);
        CHECK(budget.consumed().vertices == ports.size()+1);
    }
}

TEST_CASE("Contact matching folds declared closed aliases but never a nonlocal occurrence",
          "[lc-offsetcurve][geometry][contact-matching]") {
    const auto source = layout({line({0, 0}, {1, 0}), line({1, 0}, {1, 1}),
                               line({1, 1}, {0, 1}), line({0, 1}, {0, 0})});
    const auto closed = query(source, false, true);
    auto alias = port(1, 0, 0); alias.positions[1] = {3, 1}; alias.positionCount = 2;
    geo::WorkBudget valid;
    CHECK(geo::matchCacheContacts(*closed, {alias}, valid).status == Status::Matched);
    // Equal coordinates at NON-seam ordinals do not become one port.
    const auto contacts = query(crossingLines());
    alias = port(1, 0, .5); alias.positions[1] = {2, .5}; alias.positionCount = 2;
    geo::WorkBudget invalid;
    refusal(geo::matchCacheContacts(*contacts, {alias}, invalid), Status::InvalidInput, invalid);
}

TEST_CASE("Contact matching rejects incoherent two-position port geometry",
          "[lc-offsetcurve][geometry][contact-matching]") {
    const auto contacts = query(crossingLines());
    auto ports = crossingPorts();
    ports[0].positions[1] = {0, .75}; ports[0].positionCount = 2;
    geo::WorkBudget budget;
    refusal(geo::matchCacheContacts(*contacts, ports, budget), Status::InvalidInput, budget);
}

TEST_CASE("Contact matching never substitutes numerical root seeds or tolerance proximity",
          "[lc-offsetcurve][geometry][contact-matching]") {
    const auto contacts = query(crossingLines());
    for (const double parameter : {std::nextafter(.5, 1.0), .5001, std::numeric_limits<double>::denorm_min()}) {
        CAPTURE(parameter);
        auto ports = crossingPorts(); ports[0].positions[0].parameter = parameter;
        geo::WorkBudget budget;
        refusal(geo::matchCacheContacts(*contacts, ports, budget), Status::Ambiguous, budget);
        CHECK(budget.status() == geo::BudgetStatus::Ok);
    }
}

TEST_CASE("Contact matching ignores forged public snapshot copies",
          "[lc-offsetcurve][geometry][contact-matching]") {
    const auto contacts = query(crossingLines());
    auto edited = contacts->snapshot(contacts->contactHandle(0)); REQUIRE(edited);
    edited->event.proof = LC_IntersectionProof::Uncertified;
    edited->event.isolation.segmentA = 99; edited->event.parameterA = .75;
    // No public snapshot or foreign handle argument exists in the matching API.
    geo::WorkBudget budget;
    complete(geo::matchCacheContacts(*contacts, crossingPorts(), budget), *contacts, budget);
}

TEST_CASE("Contact matching rejects moved-from proof owners before counts or geometry access",
          "[lc-offsetcurve][geometry][contact-matching]") {
    for (const bool includePorts : {false, true}) {
        CAPTURE(includePorts);
        const auto source = query(crossingLines());
        auto destination = std::make_unique<geo::CacheContactAnalysis>(std::move(*source));
        CHECK_FALSE(source->hasValidProofs()); REQUIRE(destination->hasValidProofs());
        geo::WorkBudget invalid; REQUIRE(invalid.adoptWork(preSpent));
        const auto refused = geo::matchCacheContacts(*source,
            includePorts ? crossingPorts() : std::vector<geo::CacheContactPort>{}, invalid);
        refusal(refused, Status::InvalidInput, invalid);
        sameWork(refused.work, preSpent);
        CHECK(invalid.status() == geo::BudgetStatus::InvalidInput);
        geo::WorkBudget valid;
        complete(geo::matchCacheContacts(*destination, crossingPorts(), valid), *destination, valid);
    }
}

TEST_CASE("Contact matching retains valid owner proofs after foreign-handle rejection",
          "[lc-offsetcurve][geometry][contact-matching]") {
    const auto contacts = query(crossingLines());
    const auto foreign = query(crossingLines());
    CHECK_FALSE(contacts->snapshot(LC_CertifiedRootHandle{}));
    CHECK_FALSE(contacts->snapshot(foreign->contactHandle(0)));
    REQUIRE(contacts->hasValidProofs());
    geo::WorkBudget budget;
    complete(geo::matchCacheContacts(*contacts, crossingPorts(), budget), *contacts, budget);
}

TEST_CASE("Contact matching rejects wrong normalized segments and opaque node authorization",
          "[lc-offsetcurve][geometry][contact-matching]") {
    const auto contacts = query(crossingLines());
    SECTION("Out-of-range normalized segment is invalid") {
        auto ports = crossingPorts(); ports[0].positions[0].segment = 3;
        geo::WorkBudget budget;
        refusal(geo::matchCacheContacts(*contacts, ports, budget), Status::InvalidInput, budget);
    }
    SECTION("A different valid segment cannot anchor the root") {
        auto ports = crossingPorts(); ports[0].positions[0].segment = 1;
        geo::WorkBudget budget;
        refusal(geo::matchCacheContacts(*contacts, ports, budget), Status::Ambiguous, budget);
    }
    SECTION("All distinct node IDs authorize no nonlocal root") {
        auto ports = crossingPorts(); ports[1].referenceNode = 78;
        geo::WorkBudget budget;
        refusal(geo::matchCacheContacts(*contacts, ports, budget), Status::Topology, budget);
    }
    SECTION("Other expected contacts cannot authorize a wrong-node exact root") {
        auto ports = crossingPorts(); ports[1].referenceNode = 78;
        ports.push_back(port(99, 0, 0)); ports.push_back(port(99, 0, 1));
        geo::WorkBudget budget;
        refusal(geo::matchCacheContacts(*contacts, ports, budget), Status::Topology, budget);
    }
}

TEST_CASE("Contact matching requires every actual root and every authorized port pair",
          "[lc-offsetcurve][geometry][contact-matching]") {
    const auto contacts = query(crossingLines());
    SECTION("No ports cannot authorize an actual root") {
        geo::WorkBudget budget;
        refusal(geo::matchCacheContacts(*contacts, {}, budget), Status::Topology, budget);
    }
    SECTION("A missing root-side port cannot authorize a root") {
        const auto ports = std::vector<geo::CacheContactPort>{port(77, 0, .5)};
        geo::WorkBudget budget;
        refusal(geo::matchCacheContacts(*contacts, ports, budget), Status::Topology, budget);
    }
    SECTION("One covered pair does not cover an extra same-node port") {
        auto ports = crossingPorts(); ports.push_back(port(77, 0, 1));
        geo::WorkBudget budget;
        refusal(geo::matchCacheContacts(*contacts, ports, budget), Status::Ambiguous, budget);
    }
    SECTION("Duplicate ordinal ports are not additional incidences") {
        auto ports = crossingPorts(); ports.push_back(port(88, 0, .5));
        geo::WorkBudget budget;
        refusal(geo::matchCacheContacts(*contacts, ports, budget), Status::InvalidInput, budget);
    }
}

TEST_CASE("Contact matching uses a complete empty query to prove missing required contacts",
          "[lc-offsetcurve][geometry][contact-matching]") {
    const auto contacts = query(layout({line({0, 0}, {2, 0})}));
    REQUIRE(contacts->contactCount() == 0);
    const std::vector<geo::CacheContactPort> ports{port(77, 0, .25), port(77, 0, .75)};
    geo::WorkBudget budget;
    refusal(geo::matchCacheContacts(*contacts, ports, budget), Status::Topology, budget);
    geo::WorkBudget simple;
    auto distinct = ports; distinct[1].referenceNode = 78;
    CHECK(geo::matchCacheContacts(*contacts, distinct, simple).status == Status::Matched);
    geo::WorkBudget empty;
    CHECK(geo::matchCacheContacts(*contacts, {}, empty).status == Status::Matched);
}

TEST_CASE("Contact matching handles translated world points without rounded identity",
          "[lc-offsetcurve][geometry][contact-matching]") {
    const RS_Vector origin{0x1p40, -0x1p40};
    const auto contacts = query(crossingLines(origin), false, false, origin);
    REQUIRE(contacts->contactCount() == 1);
    geo::WorkBudget budget;
    complete(geo::matchCacheContacts(*contacts, crossingPorts(), budget), *contacts, budget);
}

TEST_CASE("Contact matching owns results and uses geometry after input owners die",
          "[lc-offsetcurve][geometry][contact-matching]") {
    std::unique_ptr<geo::CacheContactAnalysis> contacts;
    {
        auto source = crossingLines(); contacts = query(source);
        source.spans[0].controls[0] = {1e99, -1e99}; source.spans.clear();
    }
    geo::CacheContactMatchResult result;
    {
        auto ports = crossingPorts(); geo::WorkBudget budget;
        result = geo::matchCacheContacts(*contacts, ports, budget);
        complete(result, *contacts, budget);
        ports.clear(); budget.latchLimitExceeded();
    }
    contacts.reset(); // Indices remain owned data, NOT a substitute proof credential.
    REQUIRE(result.matches.size() == 1); CHECK(result.matches[0].rootIndex == 0);
    CHECK(result.matches[0].firstPort == 0); CHECK(result.matches[0].secondPort == 1);
    CHECK(result.status == Status::Matched);
}

TEST_CASE("Contact matching reproduces exact caps with cumulative pre-spent work",
          "[lc-offsetcurve][geometry][contact-matching]") {
    const auto contacts = query(crossingLines()); const auto ports = crossingPorts();
    geo::WorkBudget baseline; REQUIRE(baseline.adoptWork(preSpent));
    const auto measured = geo::matchCacheContacts(*contacts, ports, baseline);
    complete(measured, *contacts, baseline);
    geo::WorkBudget exact{limitsFor(measured.work)}; REQUIRE(exact.adoptWork(preSpent));
    const auto repeated = geo::matchCacheContacts(*contacts, ports, exact);
    complete(repeated, *contacts, exact); sameWork(repeated.work, measured.work);
    CHECK(exact.remainingVertices() == 0); CHECK(exact.remainingPairs() == 0);
    CHECK(exact.remainingOrderingComparisons() == 0);
    CHECK(repeated.work.cacheSpans == preSpent.cacheSpans); CHECK(repeated.work.deepest == preSpent.deepest);
}

TEST_CASE("Contact matching refuses one-less caps atomically without a successful prefix",
          "[lc-offsetcurve][geometry][contact-matching]") {
    const auto contacts = query(crossingLines()); const auto ports = crossingPorts();
    geo::WorkBudget baseline; REQUIRE(baseline.adoptWork(preSpent));
    const auto measured = geo::matchCacheContacts(*contacts, ports, baseline);
    REQUIRE(measured.status == Status::Matched);
    for (const unsigned dimension : {0u, 1u, 2u}) {
        CAPTURE(dimension);
        auto limits = limitsFor(measured.work);
        if (dimension == 0) { --limits.maxVertices; }
        if (dimension == 1) { --limits.maxDistancePairs; }
        if (dimension == 2) { --limits.maxOrderingComparisons; }
        geo::WorkBudget budget{limits}; REQUIRE(budget.adoptWork(preSpent));
        const auto refused = geo::matchCacheContacts(*contacts, ports, budget);
        refusal(refused, Status::LimitExceeded, budget);
        CHECK(refused.work.vertices >= preSpent.vertices); CHECK(refused.work.vertices <= limits.maxVertices);
        CHECK(refused.work.distancePairs >= preSpent.distancePairs); CHECK(refused.work.distancePairs <= limits.maxDistancePairs);
        CHECK(refused.work.orderingComparisons >= preSpent.orderingComparisons);
        CHECK(refused.work.orderingComparisons <= limits.maxOrderingComparisons);
        const auto stopped = budget.consumed();
        auto malformed = ports; malformed[0].positionCount = 99;
        const auto retry = geo::matchCacheContacts(*contacts, malformed, budget);
        refusal(retry, Status::LimitExceeded, budget); sameWork(retry.work, stopped);
    }
}

TEST_CASE("Contact matching validates port shape and preserves sticky invalid input",
          "[lc-offsetcurve][geometry][contact-matching]") {
    const auto contacts = query(crossingLines());
    for (const unsigned invalid : {0u, 1u, 2u, 3u, 4u, 5u}) {
        CAPTURE(invalid);
        auto ports = crossingPorts();
        if (invalid == 0) { ports[1].positionCount = 0; }
        if (invalid == 1) { ports[1].positionCount = 3; }
        if (invalid == 2) { ports[1].referenceNode = std::size_t(-1); }
        if (invalid == 3) { ports[1].positions[0].parameter = -1; }
        if (invalid == 4) { ports[1].positions[0].parameter = std::numeric_limits<double>::quiet_NaN(); }
        if (invalid == 5) { ports[1].positions[0].parameter = std::numeric_limits<double>::infinity(); }
        geo::WorkBudget budget; REQUIRE(budget.adoptWork(preSpent));
        refusal(geo::matchCacheContacts(*contacts, ports, budget), Status::InvalidInput, budget);
        CHECK(budget.consumed().vertices == preSpent.vertices+1);
        const auto stopped = budget.consumed();
        refusal(geo::matchCacheContacts(*contacts, crossingPorts(), budget), Status::InvalidInput, budget);
        sameWork(budget.consumed(), stopped);
    }
    LC_OffsetCheckLimits invalidLimits; invalidLimits.maxVertices = 0;
    geo::WorkBudget budget{invalidLimits};
    refusal(geo::matchCacheContacts(*contacts, crossingPorts(), budget), Status::InvalidInput, budget);
    sameWork(budget.consumed(), {});
}
