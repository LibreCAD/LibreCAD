/*
 * This file is part of LibreCAD, a 2D CAD program.
 * Copyright (C) 2026 LibreCAD.org
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "../lc_offsetworkerkernel.h"
#include <catch2/catch_test_macros.hpp>
#include <algorithm>
#include <cmath>
#include <limits>

namespace req = LC_OffsetWorkerRequest;
namespace result = LC_OffsetWorkerResult;
namespace trim = LC_OffsetCacheTrim;
namespace geo = LC_EllipseOffsetGeometry;
namespace {
req::Token token{7, 11, 13, 17};
LC_EllipseOffsetDefinition definition() {
    return {{{0, 0}, {1, 0}, 1, 0, .001, false}, 1, false, false, 1e-4};
}
RS_SplineData lineCache() {
    RS_SplineData data{1, false};
    data.controlPoints = {{-3, 0}, {3, 0}};
    data.knotslist = {0, 0, 1, 1}; data.weights = {1, 1};
    return data;
}
RS_SplineData arcCache() {
    auto data = lineCache();
    data.controlPoints = {{2, 0}, {2 * std::cos(.001), 2 * std::sin(.001)}};
    return data;
}
const result::Native& native(const result::Reply& reply, std::size_t operation) {
    REQUIRE(reply.failure == result::Failure::None);
    CHECK(reply.operation == operation);
    CHECK(reply.token.requestId == token.requestId); CHECK(reply.token.generation == token.generation);
    CHECK(reply.token.sourceRevision == token.sourceRevision); CHECK(reply.token.undoEpoch == token.undoEpoch);
    REQUIRE(std::holds_alternative<result::Native>(reply.result));
    const auto& output = std::get<result::Native>(reply.result);
    if (operation != 0 || output.refusal == LC_OffsetCurveRefusal::None ||
        output.refusal == LC_OffsetCurveRefusal::ThroughNotOnCache) {
        CHECK(output.ordinaryStatus == result::OrdinaryStatus::None); CHECK(output.ordinary.empty());
        CHECK(output.ordinaryUsage.cubicPieces == 0); CHECK(output.ordinaryUsage.outputEntities == 0);
        CHECK(output.ordinaryUsage.deepEntities == 0);
    }
    return output;
}
const trim::Result& trimmed(const result::Reply& reply, std::size_t operation) {
    REQUIRE(reply.failure == result::Failure::None); CHECK(reply.operation == operation);
    REQUIRE(std::holds_alternative<trim::Result>(reply.result));
    return std::get<trim::Result>(reply.result);
}
void work(const geo::CheckWork& a, const geo::CheckWork& b) {
    CHECK(a.cacheSpans == b.cacheSpans); CHECK(a.vertices == b.vertices);
    CHECK(a.distancePairs == b.distancePairs); CHECK(a.deepest == b.deepest);
    CHECK(a.orderingComparisons == b.orderingComparisons);
}
void certified(const result::Curve& curve) {
    const auto checked = geo::checkCache(curve.cache, curve.definition, curve.worldTolerance);
    REQUIRE(checked); REQUIRE(checked.analysis);
    CHECK(curve.closed == checked.analysis->closed());
    REQUIRE_FALSE(curve.grips.empty()); CHECK(curve.grips.size() <= 256);
    for (const auto& point : curve.grips) CHECK(geo::isFinite2D(point));
    CHECK(geo::isFinite2D(curve.midpoint));
}
}

TEST_CASE("offset child kernel generates owned checked caches with actual stage counters",
          "[lc-offsetcurve][worker-kernel]") {
    for (const bool inward : {false, true}) {
        auto def = definition(); def.distance = inward ? .25 : 1.; def.towardCentre = inward;
        req::Request input{token, req::Generate{def, {}, {}}};
        LC_OffsetWorkerKernel::Trace trace;
        const auto reply = LC_OffsetWorkerKernel::run(input, &trace);
        const auto& output = native(reply, 0);
        REQUIRE(output); REQUIRE(output.curve); CHECK(output.changed);
        CHECK(output.engineRuns == 1); REQUIRE(output.engineStatus);
        CHECK(*output.engineStatus == LC_CurveOffsetStatus::Ok);
        REQUIRE(output.checkReason); CHECK(*output.checkReason == LC_OffsetCheckResult::Reason::None);
        CHECK(trace.nativePrepareEntered); CHECK(trace.checkEntered); CHECK_FALSE(trace.trimEntered);
        CHECK(trace.computationCompleted); work(trace.checkWork, output.checkWork);
        work(trace.throughWork, {}); certified(*output.curve);
        CHECK(output.curve->definition.distance == def.distance);
        CHECK(output.curve->definition.towardCentre == inward);
        CHECK(input.token.requestId == token.requestId);
    }
}

TEST_CASE("offset child Through checks use the remaining C allowance and retain refused work",
          "[lc-offsetcurve][worker-kernel][through]") {
    req::Generate op{definition(), {}, RS_Vector{2, 0}};
    const auto good = LC_OffsetWorkerKernel::run({token, op});
    const auto& accepted = native(good, 0);
    REQUIRE(accepted.curve); CHECK(accepted.refusal == LC_OffsetCurveRefusal::None);
    work(accepted.throughWork, {0, 1, 1});
    const auto c = accepted.checkWork;
    op.limits.checkLimits.maxVertices = c.vertices;
    LC_OffsetWorkerKernel::Trace trace;
    const auto refused = LC_OffsetWorkerKernel::run({token, op}, &trace);
    const auto& limited = native(refused, 0);
    CHECK(limited.refusal == LC_OffsetCurveRefusal::ThroughNotOnCache);
    CHECK_FALSE(limited.changed); CHECK_FALSE(limited.curve);
    work(limited.checkWork, c); work(limited.throughWork, {});
    CHECK(trace.computationCompleted);
    op.limits = {}; op.throughPoint = RS_Vector{50, 50};
    const auto far = LC_OffsetWorkerKernel::run({token, op}, &trace);
    const auto& missed = native(far, 0);
    CHECK(missed.refusal == LC_OffsetCurveRefusal::ThroughNotOnCache); CHECK_FALSE(missed.curve);
    CHECK(missed.throughWork.vertices > 0); CHECK(missed.throughWork.distancePairs > 0);
    work(trace.throughWork, missed.throughWork);
}

TEST_CASE("offset child transports checked ordinary lines from the same refused engine invocation",
          "[lc-offsetcurve][worker-kernel][ordinary]") {
    for (bool reversed : {false, true}) {
        auto def = definition(); def.cacheReversed = reversed; def.basis.center = {1e7, 0};
        const auto baseline = LC_OffsetWorkerKernel::run({token, req::Generate{def, {}, {}}});
        const auto& accepted = native(baseline, 0); REQUIRE(accepted.curve);
        for (bool display : {false, true}) {
            req::Generate op{def, {}, {}};
            if (display) op.limits.outputBudget.maxDeepEntities = 31;
            else op.limits.checkLimits.maxVertices = 1;
            // A native C refusal occurs before any Through check, even if requested.
            if (!display) op.throughPoint = RS_Vector{50, 50};
            const req::Request input{token, op};
            LC_OffsetWorkerKernel::Trace trace;
            const auto reply = LC_OffsetWorkerKernel::run(input, &trace);
            const auto& output = native(reply, 0);
            CHECK(output.refusal == (display ? LC_OffsetCurveRefusal::NotCacheRepresentable : LC_OffsetCurveRefusal::CheckFailed));
            CHECK(output.engineStatus == LC_CurveOffsetStatus::Ok); CHECK(output.engineRuns == 1);
            CHECK_FALSE(output.changed); CHECK_FALSE(output.curve);
            REQUIRE(output.ordinaryStatus == result::OrdinaryStatus::Complete);
            REQUIRE(output.ordinary.size() == 1); REQUIRE(std::holds_alternative<req::Segment>(output.ordinary[0]));
            const auto& line = std::get<req::Segment>(output.ordinary[0]);
            auto data = accepted.curve->cache; data.controlPoints = {line.start, line.end};
            CHECK(req::Values::encodeCache(data) == req::Values::encodeCache(accepted.curve->cache));
            REQUIRE(geo::checkCache(data, accepted.curve->definition, accepted.curve->worldTolerance));
            CHECK(output.ordinaryUsage.cubicPieces == 1); CHECK(output.ordinaryUsage.outputEntities == 1);
            CHECK(output.ordinaryUsage.deepEntities == 1);
            CHECK(output.sampleUnits == accepted.sampleUnits);
            CHECK(output.attemptedOutputUsage.cubicPieces == accepted.attemptedOutputUsage.cubicPieces);
            CHECK(trace.nativePrepareEntered); CHECK(trace.checkEntered); CHECK(trace.computationCompleted);
            CHECK_FALSE(trace.throughEntered); work(output.throughWork, {}); work(trace.checkWork, output.checkWork);
            if (display) work(output.checkWork, accepted.checkWork);
            else { CHECK(output.checkWork.vertices <= 1); CHECK(output.checkReason == LC_OffsetCheckResult::Reason::LimitExceeded); }
            const auto encoded = result::encode(reply, input); REQUIRE(encoded);
            const auto decoded = result::decode(*encoded, input); REQUIRE(decoded);
            const auto& transported = native(*decoded, 0);
            CHECK(transported.ordinaryStatus == result::OrdinaryStatus::Complete);
            REQUIRE(transported.ordinary.size() == 1);
            CHECK(req::Values::encodeCutter(transported.ordinary[0]) == req::Values::encodeCutter(output.ordinary[0]));
        }
    }
}

TEST_CASE("offset child transports real empty and several-piece ordinary producer results",
          "[lc-offsetcurve][worker-kernel][ordinary]") {
    const double pi = std::acos(-1.0);
    for (bool several : {false, true}) {
        auto def = definition(); def.towardCentre = true;
        def.basis = several ? RS_EllipseData{{0, 0}, {100, 0}, .05, pi / 12, 7 * pi / 4, false} :
            RS_EllipseData{{0, 0}, {5, 0}, 1};
        def.distance = several ? 4 : 5; def.relTolerance = several ? 1e-5 : 1e-4;
        const req::Request input{token, req::Generate{def, {}, {}}};
        const auto reply = LC_OffsetWorkerKernel::run(input);
        const auto& output = native(reply, 0);
        CHECK(output.refusal == (several ? LC_OffsetCurveRefusal::SeveralPieces : LC_OffsetCurveRefusal::NothingLeft));
        CHECK(output.engineStatus == LC_CurveOffsetStatus::Ok); CHECK(output.engineRuns == 1);
        CHECK_FALSE(output.curve); CHECK_FALSE(output.changed); CHECK_FALSE(output.checkReason);
        REQUIRE(output.ordinaryStatus == result::OrdinaryStatus::Complete);
        CHECK(output.ordinary.empty() == !several);
        CHECK(output.ordinaryUsage.outputEntities == output.ordinary.size());
        std::size_t spans = 0;
        for (const auto& value : output.ordinary) {
            if (std::holds_alternative<req::Segment>(value)) ++spans;
            else {
                REQUIRE(std::holds_alternative<RS_SplineData>(value));
                const auto& data = std::get<RS_SplineData>(value);
                REQUIRE((data.degree == 1 || data.degree == 3));
                spans += (data.controlPoints.size() - 1) / data.degree;
            }
        }
        CHECK(output.ordinaryUsage.cubicPieces == output.attemptedOutputUsage.cubicPieces);
        CHECK(spans <= makeDefaultOffsetSourceBudget().maxCubicPieces);
        CHECK(spans == output.ordinaryUsage.cubicPieces + (several ? 1 : 0));
        if (several) CHECK(output.ordinary.size() > 1);
        else CHECK(output.ordinaryUsage.deepEntities == 0);
        const auto encoded = result::encode(reply, input); REQUIRE(encoded);
        REQUIRE(result::decode(*encoded, input));
    }
}

TEST_CASE("offset child validates original raw cache once without generating or repairing it",
          "[lc-offsetcurve][worker-kernel][raw]") {
    req::ValidateCache op{arcCache(), definition(), .01, {}};
    const auto original = RS_Spline::geometryFingerprint(op.cache);
    const auto expected = geo::checkCache(op.cache, op.definition, op.worldTolerance);
    REQUIRE(expected);
    LC_OffsetWorkerKernel::Trace trace;
    const auto reply = LC_OffsetWorkerKernel::run({token, op}, &trace);
    const auto& output = native(reply, 1);
    REQUIRE(output.curve); CHECK(output.changed); CHECK(output.engineRuns == 0);
    CHECK_FALSE(output.engineStatus); CHECK_FALSE(trace.nativePrepareEntered);
    CHECK(trace.checkEntered); CHECK(trace.computationCompleted);
    work(output.checkWork, expected.work); work(trace.checkWork, expected.work);
    CHECK(RS_Spline::geometryFingerprint(output.curve->cache) == original);
    CHECK(RS_Spline::geometryFingerprint(op.cache) == original); certified(*output.curve);
    for (const int malformed : {0, 1, 2, 3}) {
        auto bad = op;
        if (malformed == 0) bad.cache.degree = 2;
        if (malformed == 1) bad.cache.weights[0] = 2;
        if (malformed == 2) bad.cache.fitPoints.push_back({0, 0});
        if (malformed == 3) bad.cache.knotslist[2] = -1;
        const auto raw = RS_Spline::geometryFingerprint(bad.cache);
        const auto rejected = LC_OffsetWorkerKernel::run({token, bad}, &trace);
        const auto& failed = native(rejected, 1);
        CHECK_FALSE(failed.curve); CHECK_FALSE(failed.changed); CHECK(failed.engineRuns == 0);
        CHECK(failed.refusal == LC_OffsetCurveRefusal::CheckFailed);
        REQUIRE(failed.checkReason); CHECK(*failed.checkReason == LC_OffsetCheckResult::Reason::Layout);
        CHECK(RS_Spline::geometryFingerprint(bad.cache) == raw);
    }
    op.limits.maxVertices = 0;
    const auto exhausted = LC_OffsetWorkerKernel::run({token, op});
    const auto& noBudget = native(exhausted, 1);
    CHECK_FALSE(noBudget.curve); work(noBudget.checkWork, {});
}

TEST_CASE("offset child CheckThrough checks raw membership without generation C or native adoption",
          "[lc-offsetcurve][worker-kernel][through]") {
    for (const std::size_t degree : {1u, 3u}) {
        req::CheckThrough op{lineCache(), definition(), .01, {-3, 0}, {}};
        if (degree == 3) {
            op.cache.degree = 3; op.cache.controlPoints = {{-3, 0}, {-1, 0}, {1, 0}, {3, 0}};
            op.cache.knotslist = {0, 0, 0, 0, 1, 1, 1, 1}; op.cache.weights = {1, 1, 1, 1};
        }
        const auto raw = req::Values::encodeCache(op.cache); REQUIRE(raw);
        geo::WorkBudget budget{op.limits};
        const auto layout = geo::inspectCacheLayout(op.cache, budget);
        REQUIRE(layout.status == geo::CacheLayoutStatus::Ok);
        auto expected = budget.consumed(); ++expected.vertices; ++expected.distancePairs;
        LC_OffsetWorkerKernel::Trace trace;
        const req::Request input{token, op};
        const auto reply = LC_OffsetWorkerKernel::run(input, &trace);
        const auto& output = native(reply, 4);
        REQUIRE(output); CHECK_FALSE(output.curve); CHECK_FALSE(output.changed);
        CHECK(output.engineRuns == 0); CHECK_FALSE(output.engineStatus); CHECK_FALSE(output.checkReason);
        CHECK_FALSE(trace.nativePrepareEntered); CHECK_FALSE(trace.checkEntered); CHECK_FALSE(trace.trimEntered);
        CHECK(trace.throughEntered); CHECK(trace.computationCompleted);
        work(output.checkWork, {}); work(trace.checkWork, {});
        work(output.throughWork, expected); work(trace.throughWork, expected);
        CHECK(output.outputUsage.cubicPieces == 0); CHECK(output.outputUsage.outputEntities == 0);
        CHECK(output.outputUsage.deepEntities == 0); CHECK(output.sampleUnits == 0);
        CHECK(req::Values::encodeCache(op.cache) == raw);
        const auto encoded = result::encode(reply, input); REQUIRE(encoded);
        const auto decoded = result::decode(*encoded, input); REQUIRE(decoded);
        work(native(*decoded, 4).throughWork, expected);
        for (const RS_Vector point : {RS_Vector{0, 0}, RS_Vector{50, 50}}) {
            op.point = point;
            const auto checked = LC_OffsetWorkerKernel::run({token, op}, &trace);
            const auto& membership = native(checked, 4);
            CHECK(membership.refusal == (point.x == 0 ? LC_OffsetCurveRefusal::None :
                LC_OffsetCurveRefusal::ThroughNotOnCache));
            CHECK_FALSE(membership.curve); CHECK_FALSE(membership.changed);
            CHECK(membership.throughWork.vertices > layout.work.vertices);
            CHECK(membership.throughWork.distancePairs > 0);
            work(trace.throughWork, membership.throughWork);
            work(membership.checkWork, {}); CHECK_FALSE(membership.checkReason);
            REQUIRE(result::encode(checked, {token, op}));
        }
    }
}

TEST_CASE("offset child CheckThrough retains bounded layout and membership refusal prefixes",
          "[lc-offsetcurve][worker-kernel][through][limits]") {
    req::CheckThrough op{lineCache(), definition(), .01, {-3, 0}, {}};
    LC_OffsetWorkerKernel::Trace trace;
    for (int malformed : {0, 1, 2, 3, 4}) {
        auto bad = op;
        if (malformed == 0) bad.cache.degree = 2;
        if (malformed == 1) bad.cache.weights[0] = 2;
        if (malformed == 2) bad.cache.fitPoints.push_back({0, 0});
        if (malformed == 3) bad.cache.knotslist[2] = -1;
        if (malformed == 4) bad.cache.savedOpenKnots.push_back(1);
        geo::WorkBudget budget{bad.limits};
        const auto layout = geo::inspectCacheLayout(bad.cache, budget);
        REQUIRE(layout.status != geo::CacheLayoutStatus::Ok);
        const req::Request input{token, bad};
        const auto reply = LC_OffsetWorkerKernel::run(input, &trace);
        const auto& failed = native(reply, 4);
        CHECK(failed.refusal == LC_OffsetCurveRefusal::ThroughNotOnCache);
        CHECK_FALSE(failed.curve); CHECK_FALSE(failed.changed); CHECK_FALSE(failed.checkReason);
        work(failed.checkWork, {}); work(failed.throughWork, budget.consumed());
        work(trace.throughWork, failed.throughWork); REQUIRE(result::encode(reply, input));
    }
    for (int limit = 0; limit < 6; ++limit) {
        CAPTURE(limit);
        auto limited = op; limited.point = {0, 0};
        if (limit == 0) limited.limits.maxCacheSpans = 0;
        if (limit == 1) limited.limits.maxVertices = 0;
        if (limit == 2) limited.limits.maxVertices = 2;
        if (limit == 3) limited.limits.maxDistancePairs = 0;
        if (limit == 4) limited.limits.maxDepth = 0;
        if (limit == 5) limited.limits.maxOrderingComparisons = 0;
        const req::Request input{token, limited};
        const auto reply = LC_OffsetWorkerKernel::run(input, &trace);
        const auto& output = native(reply, 4);
        if (limit < 5) CHECK(output.refusal == LC_OffsetCurveRefusal::ThroughNotOnCache);
        CHECK(output.throughWork.cacheSpans <= limited.limits.maxCacheSpans);
        CHECK(output.throughWork.vertices <= limited.limits.maxVertices);
        CHECK(output.throughWork.distancePairs <= limited.limits.maxDistancePairs);
        CHECK(output.throughWork.deepest <= limited.limits.maxDepth);
        CHECK(output.throughWork.orderingComparisons <= limited.limits.maxOrderingComparisons);
        if (limit == 2) work(output.throughWork, {1, 2, 0});
        // A zero pair allowance rejects the WorkBudget before layout entry.
        if (limit == 3) work(output.throughWork, {});
        work(output.checkWork, {}); CHECK_FALSE(output.checkReason);
        work(trace.throughWork, output.throughWork); REQUIRE(result::encode(reply, input));
    }
    for (int invalid = 0; invalid < 3; ++invalid) {
        auto bad = op;
        if (invalid == 0) bad.worldTolerance = 0;
        if (invalid == 1) bad.worldTolerance = -1;
        if (invalid == 2) bad.definition.basis.majorP = {0, 0};
        const req::Request input{token, bad};
        const auto reply = LC_OffsetWorkerKernel::run(input, &trace);
        const auto& output = native(reply, 4);
        CHECK(output.refusal == LC_OffsetCurveRefusal::InvalidInput);
        work(output.checkWork, {}); work(output.throughWork, {});
        CHECK_FALSE(trace.throughEntered); CHECK_FALSE(trace.checkEntered); CHECK_FALSE(trace.nativePrepareEntered);
        CHECK(trace.computationCompleted); REQUIRE(result::encode(reply, input));
    }
    op.point.x = std::numeric_limits<double>::quiet_NaN();
    const auto unadmitted = LC_OffsetWorkerKernel::run({token, op}, &trace);
    CHECK(unadmitted.failure == result::Failure::InternalFailure);
    CHECK_FALSE(trace.throughEntered); CHECK_FALSE(trace.computationCompleted);
}

TEST_CASE("offset child raw trim adapters preserve original polynomial and conic payloads",
          "[lc-offsetcurve][worker-kernel][trim]") {
    for (const bool reverse : {false, true}) {
        auto data = lineCache();
        data.controlPoints = {{-3, .5}, {3, .5}};
        if (reverse) std::reverse(data.controlPoints.begin(), data.controlPoints.end());
        const auto original = RS_Spline::geometryFingerprint(data);
        const req::Cutter circle = RS_CircleData{{0, 0}, 2};
        LC_OffsetWorkerKernel::Trace trace;
        const auto reply = LC_OffsetWorkerKernel::run({token, req::TrimCache{data, circle, {{0, .5}}, {}}}, &trace);
        const auto& output = trimmed(reply, 2);
        REQUIRE(output.refusal == trim::Refusal::None); REQUIRE(output.fragments.size() == 2);
        CHECK(output.rootCount == 2); CHECK(output.fragmentCount == 2);
        CHECK(trace.trimEntered); CHECK(trace.computationCompleted);
        CHECK_FALSE(trace.nativePrepareEntered); CHECK_FALSE(trace.checkEntered);
        CHECK(trace.trimWork.intersections.totalBoxPairs == output.work.intersections.totalBoxPairs);
        CHECK(RS_Spline::geometryFingerprint(data) == original);
        const auto paired = LC_OffsetWorkerKernel::run({token,
            req::TrimConic{data, circle, {{0, 2}}, trim::Pick{{0, .5}}, {}}});
        const auto& two = trimmed(paired, 3);
        REQUIRE(two.refusal == trim::Refusal::None);
        CHECK(two.fragments.size() == 2); CHECK_FALSE(two.conicFragments.empty());
        auto bad = data; bad.weights[0] = 2;
        const auto malformed = LC_OffsetWorkerKernel::run({token,
            req::TrimCache{data, bad, {{0, 0}}, {}}});
        const auto& refused = trimmed(malformed, 2);
        CHECK(refused.refusal == trim::Refusal::UnsupportedLayout); CHECK(refused.fragments.empty());
    }
    // An ordinary ellipse constructor rounds these angles to a whole ellipse.
    // The child adapter must leave them raw for strict trim admission.
    RS_EllipseData tiny{{0, 0}, {2, 0}, 1, 0, 1e-12, false};
    const auto tinyReply = LC_OffsetWorkerKernel::run({token,
        req::TrimCache{lineCache(), tiny, {{0, 0}}, {}}});
    const auto& tinyResult = trimmed(tinyReply, 2);
    CHECK(tinyResult.refusal != trim::Refusal::None); CHECK(tinyResult.fragments.empty());
    CHECK(tinyResult.rootCount < 2);
}

TEST_CASE("offset child rejects unadmitted values before entering computation",
          "[lc-offsetcurve][worker-kernel]") {
    auto op = req::Generate{definition(), {}, {}};
    op.limits.checkLimits.maxVertices = LC_OffsetCheckLimits{}.maxVertices + 1;
    LC_OffsetWorkerKernel::Trace trace;
    const auto reply = LC_OffsetWorkerKernel::run({token, op}, &trace);
    CHECK(reply.failure == result::Failure::InternalFailure);
    CHECK(std::holds_alternative<std::monostate>(reply.result));
    CHECK_FALSE(trace.computationCompleted); CHECK_FALSE(trace.nativePrepareEntered);
    CHECK_FALSE(trace.checkEntered); CHECK_FALSE(trace.trimEntered);
}

TEST_CASE("offset child serializes actual lowered-budget refusal prefixes without raising caps",
          "[lc-offsetcurve][worker-kernel][limits]") {
    for (unsigned limit = 0; limit < 8; ++limit) {
        CAPTURE(limit);
        auto def = definition();
        def.basis = {{0, 0}, {100, 0}, .5, 0, 3.14159265358979323846, false};
        req::Generate op{def, {}, {}};
        switch (limit) {
        case 0: op.limits.maxSamples = 1; break;
        case 1: op.limits.maxIntersectionPairs = 1; break;
        case 2: op.limits.outputBudget.maxCubicPieces = 1; break;
        case 3: op.limits.outputBudget.maxDeepEntities = 1; break;
        case 4: op.limits.checkLimits.maxDistancePairs = 1; break;
        case 5: op.limits.checkLimits.maxVertices = 1; break;
        case 6: op.limits.checkLimits.maxDepth = 1; break;
        case 7: op.limits.checkLimits.maxOrderingComparisons = 0; break;
        }
        const req::Request request{token, op};
        const auto reply = LC_OffsetWorkerKernel::run(request);
        const auto& output = native(reply, 0);
        CHECK_FALSE(output); CHECK_FALSE(output.curve); CHECK_FALSE(output.changed);
        const auto encoded = result::encode(reply, request);
        REQUIRE(encoded);
        const auto decoded = result::decode(*encoded, request);
        REQUIRE(decoded);
        const auto& retained = std::get<result::Native>(decoded->result);
        CHECK(retained.refusal == output.refusal); CHECK(retained.engineStatus == output.engineStatus);
        CHECK(retained.checkReason == output.checkReason); CHECK(retained.engineRuns == output.engineRuns);
        work(retained.checkWork, output.checkWork); work(retained.throughWork, output.throughWork);
        CHECK(retained.sampleUnits == output.sampleUnits);
        CHECK(retained.attemptedDeepCountComplete == output.attemptedDeepCountComplete);
        CHECK(retained.attemptedOutputUsage.deepEntities == output.attemptedOutputUsage.deepEntities);
    }
}
