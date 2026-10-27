/*
 * This file is part of LibreCAD, a 2D CAD program.
 * Copyright (C) 2026 LibreCAD.org
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "../lc_offsetworkerkernel.h"
#include <catch2/catch_test_macros.hpp>
#include <algorithm>
#include <cmath>

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
    return std::get<result::Native>(reply.result);
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
