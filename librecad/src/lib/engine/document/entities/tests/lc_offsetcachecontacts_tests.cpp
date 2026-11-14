/*******************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 * Copyright (C) 2026 LibreCAD.org
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later version.
 ******************************************************************************/

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <array>
#include <cmath>
#include <limits>
#include <memory>
#include <utility>
#include <vector>

#include <boost/multiprecision/cpp_bin_float.hpp>

#include "lc_offsetcachecontacts.h"

namespace geo = LC_EllipseOffsetGeometry;

namespace {

geo::ReferenceFrame contactFrame(const RS_Vector centre = RS_Vector{0, 0}) {
    const auto frame = geo::ReferenceFrame::create({centre, RS_Vector{1, 0}, 0.5, 0.25, 0.75, false}, 0.125);
    REQUIRE(frame);
    return *frame;
}

geo::CacheSpan line(const RS_Vector a, const RS_Vector b) {
    geo::CacheSpan span;
    span.degree = 1;
    span.controls[0] = a; span.controls[1] = b;
    span.parameter = LC_Interval::hull(0, 1);
    return span;
}

geo::CacheSpan cubic(const std::array<RS_Vector, 4>& controls) {
    geo::CacheSpan span;
    span.degree = 3; span.controls = controls;
    span.parameter = LC_Interval::hull(0, 1);
    return span;
}

geo::CacheLayoutResult layout(std::vector<geo::CacheSpan> spans) {
    geo::CacheLayoutResult result;
    result.status = geo::CacheLayoutStatus::Ok;
    result.spans = std::move(spans);
    for (std::size_t k = 0; k < result.spans.size(); ++k) {
        // Raw domains deliberately differ from the provider's unit intervals.
        result.spans[k].parameter = LC_Interval::hull(7 + 2 * k, 9 + 2 * k);
    }
    return result;
}

geo::CacheLayoutResult crossingLines() {
    return layout({line({0, 0}, {2, 2}), line({2, 2}, {0, 2}), line({0, 2}, {2, 0})});
}

geo::CacheLayoutResult adjacentQuadratics(const double scale = 3) {
    // For scale=3, all elevated controls are exact integers, so the crossing at .5 is exact too.
    // The scale=1 case uses the requested binary representations of thirds, with a nearby root.
    return layout({cubic({RS_Vector{0, 0}, RS_Vector{scale, -scale / 3},
                         RS_Vector{4 * scale / 3, 0}, RS_Vector{scale, scale}}),
                   cubic({RS_Vector{scale, scale}, RS_Vector{2 * scale / 3, 0},
                         RS_Vector{scale, -scale / 3}, RS_Vector{2 * scale, 0}})});
}

using ContactQuad = boost::multiprecision::cpp_bin_float_quad;
std::array<ContactQuad, 4> quadraticJet(const geo::CacheSpan& span, const ContactQuad& t) {
    const ContactQuad u = 1 - t;
    std::array<ContactQuad, 4> out{};
    for (unsigned coordinate = 0; coordinate < 2; ++coordinate) {
        const auto value = [&](const unsigned i) { return ContactQuad(coordinate == 0 ? span.controls[i].x : span.controls[i].y); };
        out[coordinate] = u * u * u * value(0) + 3 * u * u * t * value(1) +
            3 * u * t * t * value(2) + t * t * t * value(3);
        out[coordinate + 2] = 3 * (u * u * (value(1) - value(0)) +
            2 * u * t * (value(2) - value(1)) + t * t * (value(3) - value(2)));
    }
    return out;
}

void checkWork(const geo::CheckWork& a, const geo::CheckWork& b) {
    CHECK(a.cacheSpans == b.cacheSpans);
    CHECK(a.vertices == b.vertices);
    CHECK(a.distancePairs == b.distancePairs);
    CHECK(a.deepest == b.deepest);
    CHECK(a.orderingComparisons == b.orderingComparisons);
}

LC_OffsetCheckLimits exactLimits(const geo::CheckWork& work) {
    return {work.cacheSpans, work.vertices, work.distancePairs, work.deepest, work.orderingComparisons};
}

void checkContactProof(const LC_CertifiedRootSnapshot& root) {
    CHECK(root.event.proof == LC_IntersectionProof::IntervalUniqueTransverse);
    CHECK(root.event.kind == LC_ParametricIntersection::Kind::Transverse);
    CHECK(root.event.branchA == 0);
    CHECK(root.event.branchB == 0);
    CHECK(root.event.isolation.parameterA.isValid());
    CHECK(root.event.isolation.parameterB.isValid());
    CHECK(root.event.isolation.parameterA.contains(root.event.parameterA));
    CHECK(root.event.isolation.parameterB.contains(root.event.parameterB));
    CHECK(root.imageX.isValid());
    CHECK(root.imageY.isValid());
}

} // namespace

TEST_CASE("Cache contacts certify a line without claiming reference validity",
          "[lc-offsetcurve][geometry][cache-contacts]") {
    auto source = layout({line({0, 0}, {1, 0})});
    geo::WorkBudget budget;
    auto result = geo::analyseCacheContacts(source, contactFrame(), false, false, budget);
    REQUIRE(result.status == LC_IntersectionStatus::Ok);
    REQUIRE(result.analysis);
    CHECK(result.analysis->contactCount() == 0);
    CHECK(result.analysis->spanCount() == 1);
    CHECK_FALSE(result.analysis->closed());
    CHECK_FALSE(result.analysis->reversed());
    CHECK(result.analysis->curves().next(0) == -1);
    CHECK(result.analysis->rawSpanId(0) == 0);
    CHECK_FALSE(result.analysis->rawSpanId(1));
    CHECK(result.analysis->rawSpan(1) == nullptr);
    CHECK(result.analysis->rawSpan(0)->parameter.lo() == 7);
    CHECK(result.analysis->rawSpan(0)->parameter.hi() == 9);
    checkWork(result.work, budget.consumed());
}

TEST_CASE("Cache contacts own normalized cubic geometry and preserve raw span attribution",
          "[lc-offsetcurve][geometry][cache-contacts]") {
    std::unique_ptr<geo::CacheContactAnalysis> saved;
    for (const bool reversed : {false, true}) {
        {
            auto source = layout({cubic({RS_Vector{0, 0}, RS_Vector{1, 1}, RS_Vector{2, 1}, RS_Vector{3, 0}}),
                                  cubic({RS_Vector{3, 0}, RS_Vector{4, -1}, RS_Vector{5, -1}, RS_Vector{6, 0}})});
            auto frame = contactFrame();
            geo::WorkBudget budget;
            auto result = geo::analyseCacheContacts(source, frame, reversed, false, budget);
            REQUIRE(result.status == LC_IntersectionStatus::Ok);
            REQUIRE(result.analysis);
            CHECK(result.analysis->contactCount() == 0);
            saved = std::move(result.analysis);
            source.spans[0].controls[0] = RS_Vector{1e99, 1e99};
            source.spans.clear();
        }
        // Source, frame and budget have all died; the session must still be a complete provider.
        REQUIRE(saved);
        REQUIRE(saved->spanCount() == 2);
        CHECK(saved->curves().branchCount() == 1);
        CHECK(saved->curves().next(1) == -1);
        CHECK(saved->reversed() == reversed);
        for (std::size_t k = 0; k < 2; ++k) {
            CHECK(saved->rawSpanId(k) == (reversed ? 1 - k : k));
            const auto& segment = saved->curves().segments()[k];
            CHECK(segment.branch == 0);
            CHECK(segment.t0 == static_cast<double>(k));
            CHECK(segment.t1 == static_cast<double>(k + 1));
            RS_Vector point, derivative;
            REQUIRE(saved->curves().evaluate(k, segment.t0, point, derivative));
            CHECK(point.x == Catch::Approx(reversed ? 6.0 - 3.0 * k : 3.0 * k).margin(1e-12));
            CHECK(point.y == Catch::Approx(0).margin(1e-12));
            CHECK((reversed ? derivative.x < 0 : derivative.x > 0));
        }
        LC_Interval x, y, dx, dy;
        CHECK_FALSE(saved->curves().bound(2, 0, 1, x, y, dx, dy));
        CHECK_FALSE(x.isValid());
        CHECK_FALSE(saved->curves().bound(0, 0.75, 0.25, x, y, dx, dy));
        CHECK_FALSE(saved->curves().bound(0, -0.25, 0.25, x, y, dx, dy));
    }
}

TEST_CASE("Cache contacts admit whole Bernstein injectivity with stationary endpoints",
          "[lc-offsetcurve][geometry][cache-contacts]") {
    const auto source = layout({cubic({RS_Vector{0, 0}, RS_Vector{0, 0}, RS_Vector{1, 0}, RS_Vector{1, 0}})});
    for (const bool reversed : {false, true}) {
        geo::WorkBudget budget;
        auto result = geo::analyseCacheContacts(source, contactFrame(), reversed, false, budget);
        REQUIRE(result.status == LC_IntersectionStatus::Ok);
        REQUIRE(result.analysis);
        CHECK(result.analysis->contactCount() == 0);
        const auto& curves = result.analysis->curves();
        const auto proof = curves.certifyInjective(0, LC_Interval::hull(0, 1));
        REQUIRE(proof.available);
        CHECK(proof.segment == 0);
        CHECK(proof.parameter.lo() == 0);
        CHECK(proof.parameter.hi() == 1);
        CHECK_FALSE(curves.certifyInjective(0, LC_Interval::hull(-0.01, 1)).available);
        RS_Vector point, derivative;
        REQUIRE(curves.evaluate(0, 0, point, derivative));
        CHECK(derivative.x == 0);
        CHECK(derivative.y == 0);
        REQUIRE(curves.evaluate(0, 1, point, derivative));
        CHECK(derivative.x == 0);
        CHECK(derivative.y == 0);
    }
}

TEST_CASE("Cache contacts exclude only a proved construction join including a closed seam",
          "[lc-offsetcurve][geometry][cache-contacts]") {
    const auto square = layout({line({0, 0}, {1, 0}), line({1, 0}, {1, 1}),
                                line({1, 1}, {0, 1}), line({0, 1}, {0, 0})});
    for (const bool reversed : {false, true}) {
        geo::WorkBudget budget;
        auto result = geo::analyseCacheContacts(square, contactFrame(), reversed, true, budget);
        REQUIRE(result.status == LC_IntersectionStatus::Ok);
        REQUIRE(result.analysis);
        CHECK(result.analysis->closed());
        CHECK(result.analysis->curves().next(0) == 0);
        CHECK(result.analysis->contactCount() == 0);
        const auto& curves = result.analysis->curves();
        for (const auto box : {LC_IntersectionDomain{0, 1, LC_Interval::hull(0, 1), LC_Interval::hull(1, 2)},
                               LC_IntersectionDomain{0, 3, LC_Interval::hull(0, 1), LC_Interval::hull(3, 4)},
                               LC_IntersectionDomain{3, 0, LC_Interval::hull(3, 4), LC_Interval::hull(0, 1)}}) {
            const auto proof = curves.certifyIntersection(box);
            CHECK(proof.outcome == LC_AnalyticIntersectionCertificate::Outcome::Excluded);
            CHECK(proof.proof == LC_IntersectionProof::AnalyticExclusion);
            CHECK(proof.proofDomain.segmentA == box.segmentA);
            CHECK(proof.proofDomain.segmentB == box.segmentB);
        }
    }
    geo::WorkBudget budget;
    auto open = geo::analyseCacheContacts(layout({line({0, 0}, {1, 0})}), contactFrame(), false, true, budget);
    CHECK(open.status == LC_IntersectionStatus::InvalidInput);
    CHECK_FALSE(open.analysis);
}

TEST_CASE("Cache contacts detect the extra crossing of adjacent quadratic pieces",
          "[lc-offsetcurve][geometry][cache-contacts]") {
    for (const double scale : {1.0, 3.0}) {
        for (const bool reversed : {false, true}) {
            CAPTURE(scale, reversed);
            geo::WorkBudget budget;
            const auto source = adjacentQuadratics(scale);
            auto result = geo::analyseCacheContacts(source, contactFrame(), reversed, false, budget);
            CAPTURE(result.work.deepest, result.queryWork.deepestPreparation, result.queryWork.deepestPairRefinement,
                result.queryWork.deepestOrderingRefinement, result.queryWork.deepestRootRefinement,
                result.queryWork.totalBoxPairs, result.queryWork.generatedItems);
            REQUIRE(result.status == LC_IntersectionStatus::Ok);
            REQUIRE(result.analysis);
            REQUIRE(result.analysis->contactCount() == 1);
            const auto root = result.analysis->snapshot(result.analysis->contactHandle(0));
            REQUIRE(root);
            checkContactProof(*root);
            CHECK(root->event.isolation.segmentA == 0);
            CHECK(root->event.isolation.segmentB == 1);
            // Numeric event midpoints have no accuracy contract. Check the actual root enclosures.
            ContactQuad t = ContactQuad(0.5), u = t;
            for (unsigned i = 0; i < 12; ++i) {
                const auto a = quadraticJet(source.spans[0], t), b = quadraticJet(source.spans[1], u);
                const ContactQuad x = a[0] - b[0], y = a[1] - b[1];
                const ContactQuad determinant = -a[2] * b[3] + b[2] * a[3];
                REQUIRE(abs(determinant) > ContactQuad(scale * scale));
                t -= (-b[3] * x + b[2] * y) / determinant;
                u -= (-a[3] * x + a[2] * y) / determinant;
            }
            const auto point = quadraticJet(source.spans[0], t), other = quadraticJet(source.spans[1], u);
            REQUIRE(abs(point[0] - other[0]) < ContactQuad("1e-28"));
            REQUIRE(abs(point[1] - other[1]) < ContactQuad("1e-28"));
            const ContactQuad a = reversed ? ContactQuad(1 - u) : t;
            const ContactQuad b = reversed ? ContactQuad(2 - t) : ContactQuad(1 + u);
            CHECK(ContactQuad(root->event.isolation.parameterA.lo()) <= a);
            CHECK(a <= ContactQuad(root->event.isolation.parameterA.hi()));
            CHECK(ContactQuad(root->event.isolation.parameterB.lo()) <= b);
            CHECK(b <= ContactQuad(root->event.isolation.parameterB.hi()));
            CHECK(ContactQuad(root->imageX.lo()) <= point[0]); CHECK(point[0] <= ContactQuad(root->imageX.hi()));
            CHECK(ContactQuad(root->imageY.lo()) <= point[1]); CHECK(point[1] <= ContactQuad(root->imageY.hi()));
            const auto whole = result.analysis->curves().certifyIntersection(
                {0, 1, LC_Interval::hull(0, 1), LC_Interval::hull(1, 2)});
            CHECK(whole.outcome == LC_AnalyticIntersectionCertificate::Outcome::Unavailable);
            // A separate common-x proof is valid near the construction join, but not over the whole pair.
            const auto local = result.analysis->curves().certifyIntersection(
                {0, 1, LC_Interval::hull(0.875, 1), LC_Interval::hull(1, 1.125)});
            CHECK(local.outcome == LC_AnalyticIntersectionCertificate::Outcome::Excluded);
        }
    }
}

TEST_CASE("Cache contacts return strict owned isolations for nonadjacent line crossings",
          "[lc-offsetcurve][geometry][cache-contacts]") {
    for (const bool reversed : {false, true}) {
        geo::WorkBudget budget;
        auto result = geo::analyseCacheContacts(crossingLines(), contactFrame(), reversed, false, budget);
        CAPTURE(result.work.deepest, result.queryWork.deepestPreparation, result.queryWork.deepestPairRefinement,
            result.queryWork.deepestOrderingRefinement, result.queryWork.deepestRootRefinement,
            result.queryWork.totalBoxPairs, result.queryWork.generatedItems);
        REQUIRE(result.status == LC_IntersectionStatus::Ok);
        REQUIRE(result.analysis);
        REQUIRE(result.analysis->contactCount() == 1);
        const auto root = result.analysis->snapshot(result.analysis->contactHandle(0));
        REQUIRE(root);
        checkContactProof(*root);
        CHECK(root->event.isolation.segmentA == 0);
        CHECK(root->event.isolation.segmentB == 2);
        CHECK(root->event.isolation.parameterA.contains(0.5));
        CHECK(root->event.isolation.parameterB.contains(2.5));
        CHECK(root->imageX.contains(1));
        CHECK(root->imageY.contains(1));
        CHECK_FALSE(result.analysis->snapshot(result.analysis->contactHandle(1)));
    }
}

TEST_CASE("Cache contacts retain a cubic self loop instead of pruning a small image",
          "[lc-offsetcurve][geometry][cache-contacts]") {
    for (const double scale : {1.0, 1e-9}) {
        const auto source = layout({cubic({RS_Vector{0, 0}, RS_Vector{15 * scale, 10 * scale},
                                          RS_Vector{-5 * scale, 10 * scale}, RS_Vector{10 * scale, 0}})});
        geo::WorkBudget budget;
        auto result = geo::analyseCacheContacts(source, contactFrame(), false, false, budget);
        REQUIRE(result.status == LC_IntersectionStatus::Ok);
        REQUIRE(result.analysis);
        REQUIRE(result.analysis->contactCount() == 1);
        const auto root = result.analysis->snapshot(result.analysis->contactHandle(0));
        REQUIRE(root);
        checkContactProof(*root);
        CHECK(root->event.isolation.segmentA == 0);
        CHECK(root->event.isolation.segmentB == 0);
        CHECK(root->event.isolation.parameterA.hi() < root->event.isolation.parameterB.lo());
        using Quad = boost::multiprecision::cpp_bin_float_quad;
        const auto& controls = source.spans[0].controls;
        const Quad a = Quad(controls[3].x) - 3 * Quad(controls[2].x) + 3 * Quad(controls[1].x);
        // Equal middle Y controls imply u=1-t. Divide x(t)-x(1-t) by 2t-1.
        const Quad discriminant = 1 - 4 * Quad(controls[3].x) / a;
        const Quad t = (1 - sqrt(discriminant)) / 2, u = (1 + sqrt(discriminant)) / 2;
        CHECK(Quad(root->event.isolation.parameterA.lo()) <= t);
        CHECK(t <= Quad(root->event.isolation.parameterA.hi()));
        CHECK(Quad(root->event.isolation.parameterB.lo()) <= u);
        CHECK(u <= Quad(root->event.isolation.parameterB.hi()));
        const Quad b = 3 * (Quad(controls[2].x) - 2 * Quad(controls[1].x));
        const Quad c = 3 * Quad(controls[1].x);
        const Quad x = t * (c + t * (b + t * a));
        CHECK(Quad(root->imageX.lo()) <= x); CHECK(x <= Quad(root->imageX.hi()));
        const auto proof = result.analysis->curves().certifyInjective(0, LC_Interval::hull(0, 1));
        CHECK_FALSE(proof.available);
    }
}

TEST_CASE("Cache contacts refuse retracing and malformed layouts without partial credentials",
          "[lc-offsetcurve][geometry][cache-contacts]") {
    SECTION("Retracing is not a construction join proof") {
        geo::WorkBudget budget;
        const auto source = layout({line({0, 0}, {1, 0}), line({1, 0}, {0, 0})});
        const auto result = geo::analyseCacheContacts(source, contactFrame(), false, false, budget);
        CHECK((result.status == LC_IntersectionStatus::AmbiguousTopology ||
               result.status == LC_IntersectionStatus::LimitExceeded));
        CHECK_FALSE(result.analysis);
        checkWork(result.work, budget.consumed());
    }
    SECTION("Each malformed span is validated after a bounded setup admission") {
        auto source = layout({line({0, 0}, {1, 0}), line({1, 0}, {2, 0})});
        const auto kind = GENERATE(0, 1, 2, 3, 4, 5);
        if (kind == 0) { source.status = geo::CacheLayoutStatus::InvalidLayout; }
        if (kind == 1) { source.spans.clear(); }
        if (kind == 2) { source.spans[1].degree = 2; }
        if (kind == 3) { source.spans[1].controls[0].x = 1.125; }
        if (kind == 4) { source.spans[1].controls[1].x = std::numeric_limits<double>::infinity(); }
        if (kind == 5) { source.spans[1].parameter = LC_Interval::point(2); }
        geo::WorkBudget budget;
        auto result = geo::analyseCacheContacts(source, contactFrame(), false, false, budget);
        CHECK(result.status == LC_IntersectionStatus::InvalidInput);
        CHECK_FALSE(result.analysis);
        checkWork(result.work, budget.consumed());
    }
}

TEST_CASE("Cache contacts enforce exact cumulative caps and sticky refusal",
          "[lc-offsetcurve][geometry][cache-contacts]") {
    const auto source = adjacentQuadratics();
    const auto frame = contactFrame();
    const geo::CheckWork preSpent{2, 3, 5, 0, 7};
    geo::WorkBudget baseline;
    REQUIRE(baseline.adoptWork(preSpent));
    auto accepted = geo::analyseCacheContacts(source, frame, false, false, baseline);
    REQUIRE(accepted.status == LC_IntersectionStatus::Ok);
    REQUIRE(accepted.analysis);
    REQUIRE(accepted.work.deepest > 0);
    checkWork(accepted.work, baseline.consumed());
    for (const auto dimension : {0, 1, 2, 3, 4}) {
        CAPTURE(dimension);
        auto limits = exactLimits(accepted.work);
        if (dimension == 1) { --limits.maxVertices; }
        if (dimension == 2) { --limits.maxDistancePairs; }
        if (dimension == 3) { --limits.maxDepth; }
        if (dimension == 4) { --limits.maxOrderingComparisons; }
        geo::WorkBudget budget{limits};
        REQUIRE(budget.adoptWork(preSpent));
        auto result = geo::analyseCacheContacts(source, frame, false, false, budget);
        if (dimension == 0) {
            CHECK(result.status == LC_IntersectionStatus::Ok);
            REQUIRE(result.analysis);
            checkWork(result.work, accepted.work);
        } else {
            CHECK(result.status == LC_IntersectionStatus::LimitExceeded);
            CHECK(budget.status() == geo::BudgetStatus::LimitExceeded);
            CHECK_FALSE(result.analysis);
            checkWork(result.work, budget.consumed());
            const auto spent = budget.consumed();
            auto retry = geo::analyseCacheContacts(source, frame, false, false, budget);
            CHECK(retry.status == LC_IntersectionStatus::LimitExceeded);
            CHECK_FALSE(retry.analysis);
            checkWork(retry.work, spent);
        }
        CHECK(budget.consumed().cacheSpans <= limits.maxCacheSpans);
        CHECK(budget.consumed().vertices <= limits.maxVertices);
        CHECK(budget.consumed().distancePairs <= limits.maxDistancePairs);
        CHECK(budget.consumed().deepest <= limits.maxDepth);
        CHECK(budget.consumed().orderingComparisons <= limits.maxOrderingComparisons);
    }
}

TEST_CASE("Cache contacts bound setup before allocation and preserve terminal caller status",
          "[lc-offsetcurve][geometry][cache-contacts]") {
    const auto source = layout({line({0, 0}, {1, 0})});
    const auto frame = contactFrame();
    SECTION("The six setup records are admitted atomically") {
        LC_OffsetCheckLimits limits;
        limits.maxVertices = 5;
        geo::WorkBudget budget{limits};
        auto result = geo::analyseCacheContacts(source, frame, false, false, budget);
        CHECK(result.status == LC_IntersectionStatus::LimitExceeded);
        CHECK_FALSE(result.analysis);
        checkWork(result.work, {});
    }
    SECTION("Strict factory metadata admission follows charged provider setup") {
        LC_OffsetCheckLimits limits;
        limits.maxVertices = 7;
        geo::WorkBudget budget{limits};
        auto result = geo::analyseCacheContacts(source, frame, false, false, budget);
        CHECK(result.status == LC_IntersectionStatus::LimitExceeded);
        CHECK_FALSE(result.analysis);
        CHECK(result.work.vertices == 6);
        CHECK(result.work.distancePairs == 1);
    }
    SECTION("No input work follows an already invalid caller") {
        geo::WorkBudget budget;
        REQUIRE(budget.adoptWork({2, 3, 5, 1, 7}));
        budget.latchInvalidInput();
        const auto spent = budget.consumed();
        auto result = geo::analyseCacheContacts(source, frame, false, false, budget);
        CHECK(result.status == LC_IntersectionStatus::InvalidInput);
        CHECK_FALSE(result.analysis);
        checkWork(result.work, spent);
        CHECK(budget.status() == geo::BudgetStatus::InvalidInput);
    }
}

TEST_CASE("Cache contacts account retained contraction without borrowing a budget",
          "[lc-offsetcurve][geometry][cache-contacts]") {
    std::unique_ptr<geo::CacheContactAnalysis> owned;
    LC_CertifiedRootHandle handle;
    {
        geo::WorkBudget budget;
        auto result = geo::analyseCacheContacts(crossingLines(), contactFrame(), false, false, budget);
        REQUIRE(result.status == LC_IntersectionStatus::Ok);
        REQUIRE(result.analysis);
        REQUIRE(result.analysis->contactCount() == 1);
        handle = result.analysis->contactHandle(0);
        owned = std::move(result.analysis);
    }
    geo::CacheContactAnalysis moved{std::move(*owned)};
    owned.reset();
    const auto before = moved.snapshot(handle);
    REQUIRE(before);
    geo::WorkBudget followup;
    REQUIRE(followup.adoptWork({2, 3, 5, 0, 7}));
    const auto spent = followup.consumed();
    auto contracted = moved.contract(handle, followup);
    CHECK((contracted.status == LC_IntersectionStatus::Ok ||
           contracted.status == LC_IntersectionStatus::AmbiguousTopology));
    CHECK(followup.consumed().vertices == spent.vertices + contracted.work.generatedItems);
    CHECK(followup.consumed().distancePairs == spent.distancePairs + contracted.work.totalBoxPairs);
    CHECK(followup.consumed().orderingComparisons == spent.orderingComparisons + contracted.work.orderingComparisons);
    CHECK(followup.consumed().deepest == contracted.work.deepest());
    CHECK(followup.consumed().cacheSpans == spent.cacheSpans);
    if (contracted.status == LC_IntersectionStatus::Ok) {
        REQUIRE(contracted.snapshot);
        CHECK(contracted.snapshot->admittedContractions > before->admittedContractions);
        CHECK(contracted.snapshot->event.isolation.parameterA.lo() >= before->event.isolation.parameterA.lo());
        CHECK(contracted.snapshot->event.isolation.parameterA.hi() <= before->event.isolation.parameterA.hi());
    }
    geo::WorkBudget freshBudget;
    auto fresh = geo::analyseCacheContacts(crossingLines(), contactFrame(), false, false, freshBudget);
    REQUIRE(fresh.status == LC_IntersectionStatus::Ok); REQUIRE(fresh.analysis);
    const auto freshHandle = fresh.analysis->contactHandle(0);
    LC_OffsetCheckLimits limits; limits.maxDistancePairs = 1;
    geo::WorkBudget exhausted{limits}; REQUIRE(exhausted.consumePairs(1));
    auto refused = fresh.analysis->contract(freshHandle, exhausted);
    CHECK(refused.status == LC_IntersectionStatus::LimitExceeded);
    CHECK_FALSE(refused.snapshot);
    CHECK(exhausted.status() == geo::BudgetStatus::LimitExceeded);
    const auto refusalWork = exhausted.consumed();
    const auto retry = fresh.analysis->contract(freshHandle, exhausted);
    CHECK(retry.status == LC_IntersectionStatus::LimitExceeded);
    checkWork(exhausted.consumed(), refusalWork);
}

TEST_CASE("Cache contacts reject foreign proof handles without charging work",
          "[lc-offsetcurve][geometry][cache-contacts]") {
    geo::WorkBudget aBudget, bBudget;
    auto a = geo::analyseCacheContacts(crossingLines(), contactFrame(), false, false, aBudget);
    auto b = geo::analyseCacheContacts(crossingLines(), contactFrame(), false, false, bBudget);
    REQUIRE(a.status == LC_IntersectionStatus::Ok);
    REQUIRE(b.status == LC_IntersectionStatus::Ok);
    REQUIRE(a.analysis);
    REQUIRE(b.analysis);
    const auto handle = a.analysis->contactHandle(0);
    CHECK_FALSE(b.analysis->snapshot(handle));
    CHECK(b.analysis->hasValidProofs());
    const auto before = bBudget.consumed();
    const auto result = b.analysis->contract(handle, bBudget);
    CHECK(result.status == LC_IntersectionStatus::InvalidInput);
    CHECK_FALSE(result.snapshot);
    checkWork(bBudget.consumed(), before);
    CHECK(b.analysis->snapshot(b.analysis->contactHandle(0)));
    CHECK(b.analysis->hasValidProofs());
    // Historical query/refinement depth belongs to the old caller, not this rejected call.
    REQUIRE(b.analysis->snapshot(b.analysis->contactHandle(0)));
    const auto refinement = b.analysis->contract(b.analysis->contactHandle(0), bBudget);
    REQUIRE((refinement.status == LC_IntersectionStatus::Ok ||
             refinement.status == LC_IntersectionStatus::AmbiguousTopology));
    REQUIRE(bBudget.consumed().deepest > 0);
    LC_OffsetCheckLimits limits; limits.maxDepth = 1;
    geo::WorkBudget fresh{limits};
    const auto rejected = b.analysis->contract(handle, fresh);
    CHECK(rejected.status == LC_IntersectionStatus::InvalidInput);
    CHECK_FALSE(rejected.snapshot);
    CHECK(fresh.status() == geo::BudgetStatus::Ok);
    checkWork(fresh.consumed(), {});
}
