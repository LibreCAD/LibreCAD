/*
 * This file is part of LibreCAD, a 2D CAD program.
 * Copyright (C) 2026 LibreCAD.org
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "../lc_offsetworkerkernel.h"
#include "../lc_offsetworkertolerance.h"
#include "rs_ellipse.h"
#include "rs_line.h"
#include "rs_spline.h"
#include <catch2/catch_test_macros.hpp>
#include <algorithm>
#include <cmath>
#include <limits>
#include <tuple>

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
    if ((operation != 0 && operation != 7) || output.refusal == LC_OffsetCurveRefusal::None ||
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

TEST_CASE("offset worker op7 generates and transports a typed source without cache authority",
          "[lc-offsetcurve][worker-kernel][source-generation]") {
    RS_SplineData spline{2, false};
    spline.controlPoints = {{0, 0}, {1, 2}, {3, 0}};
    spline.knotslist = {0, 0, 0, 1, 1, 1};
    spline.weights = {1, .7, 1};
    LC_SplinePointsData points{false, false};
    points.useControlPoints = true;
    points.controlPoints = {{0, 0}, {1, 1}, {2, 0}};
    LC_SplinePointsData fitPoints{false, false};
    fitPoints.splinePoints = {{0, 0}, {1, 2}, {2, 0}};
    RS_SplineData degreeOne{1, false};
    degreeOne.controlPoints = {{0, 0}, {1, 1}, {2, 0}};
    degreeOne.knotslist = {0, 0, .5, 1, 1};
    RS_SplineData cubic{3, false};
    cubic.controlPoints = {{0, 0}, {1, 2}, {2, 2}, {3, 0}};
    cubic.knotslist = {0, 0, 0, 0, 1, 1, 1, 1};
    RS_Spline periodic{nullptr, RS_SplineData{3, false}};
    for (const RS_Vector& point : {RS_Vector{0, 0}, RS_Vector{40, -10}, RS_Vector{60, 30},
                                   RS_Vector{20, 50}, RS_Vector{-15, 25}})
        periodic.addControlPoint(point);
    periodic.setClosed(true);
    auto wrapped = periodic.getData();
    wrapped.weights.clear();
    const double pi = std::acos(-1.0);
    LC_OffsetPolylineData closedArcPolyline{{2, 0}, {2, 0}, true, {
        RS_ArcData{{0, 0}, 2, 0, pi / 2, false},
        RS_ArcData{{0, 0}, 2, pi / 2, pi, false},
        RS_ArcData{{0, 0}, 2, pi, 3 * pi / 2, false},
        RS_ArcData{{0, 0}, 2, 3 * pi / 2, 2 * pi, false}}};
    const std::array<LC_OffsetSourceGeometry, 15> sources{
        LC_OffsetSourceGeometry{RS_EllipseData{{0, 0}, {5, 0}, .5, 0, 1.5, false}},
        LC_OffsetSourceGeometry{spline},
        LC_OffsetSourceGeometry{points},
        LC_OffsetSourceGeometry{fitPoints},
        LC_OffsetSourceGeometry{degreeOne},
        LC_OffsetSourceGeometry{cubic},
        LC_OffsetSourceGeometry{LC_ParabolaData{{RS_Vector{0, 1}, RS_Vector{1, 0}, RS_Vector{2, 1}}}},
        LC_OffsetSourceGeometry{LC_HyperbolaData{{0, 0}, {2, 0}, .5, -.5, .5}},
        LC_OffsetSourceGeometry{RS_LineData{{0, 0}, {2, 0}}},
        LC_OffsetSourceGeometry{RS_ArcData{{0, 0}, 2, .25, 1.5, false}},
        LC_OffsetSourceGeometry{RS_CircleData{{0, 0}, 2}},
        LC_OffsetSourceGeometry{LC_OffsetPolylineData{{0, 0}, {2, 2}, false,
            {RS_ArcData{{1, 0}, 1, M_PI, 0, false}, RS_LineData{{2, 0}, {2, 2}}}}},
        LC_OffsetSourceGeometry{LC_OffsetPolylineData{{0, 0}, {0, 0}, true,
            {RS_LineData{{0, 0}, {2, 0}}, RS_LineData{{2, 0}, {2, 2}},
             RS_LineData{{2, 2}, {0, 2}}, RS_LineData{{0, 2}, {0, 0}}}}},
        LC_OffsetSourceGeometry{wrapped},
        LC_OffsetSourceGeometry{closedArcPolyline}
    };
    for (std::size_t i = 0; i < sources.size(); ++i) {
        CAPTURE(i);
        REQUIRE(sources[i].isWellFormed());
        if (i == 4) {
            CHECK(std::get<RS_SplineData>(sources[i].value).weights.empty());
            auto materialized = sources[i].makeEntity();
            const auto* splineEntity = dynamic_cast<const RS_Spline*>(materialized.get());
            REQUIRE(splineEntity);
            CHECK(splineEntity->getData().weights == std::vector<double>{1.0, 1.0, 1.0});
        }
        const req::Request input{token, req::GenerateSource{
            LC_OffsetSourceDefinition{sources[i], .25, 1e-4, false}, {}}};
        LC_OffsetWorkerKernel::Trace trace;
        const auto reply = LC_OffsetWorkerKernel::run(input, &trace);
        const auto& output = native(reply, 7);
        CHECK(output.engineRuns == 1);
        REQUIRE(output.engineStatus);
        CHECK(*output.engineStatus == LC_CurveOffsetStatus::Ok);
        CHECK_FALSE(output.curve);
        CHECK_FALSE(output.changed);
        CHECK(trace.nativePrepareEntered);
        CHECK(trace.computationCompleted);
        REQUIRE(output.ordinaryStatus == result::OrdinaryStatus::Complete);
        REQUIRE_FALSE(output.ordinary.empty());
        CHECK(output.ordinaryUsage.outputEntities == output.ordinary.size());
        CHECK(output.refusal == LC_OffsetCurveRefusal::NotCacheRepresentable);

        const auto wire = result::encode(reply, input);
        REQUIRE(wire);
        const auto decoded = result::decode(*wire, input);
        REQUIRE(decoded);
        const auto& transported = native(*decoded, 7);
        CHECK_FALSE(transported.curve);
        CHECK(transported.ordinary.size() == output.ordinary.size());
        CHECK(transported.ordinaryUsage.outputEntities == output.ordinaryUsage.outputEntities);
        if (i == 4) {
            auto equivalent = std::get<RS_SplineData>(sources[i].value);
            equivalent.weights.assign(equivalent.controlPoints.size(), 1.0);
            const req::Request explicitWeightsInput{token, req::GenerateSource{
                LC_OffsetSourceDefinition{LC_OffsetSourceGeometry{equivalent}, .25, 1e-4, false}, {}}};
            const auto explicitWeightsReply = LC_OffsetWorkerKernel::run(explicitWeightsInput);
            const auto explicitWeightsWire = result::encode(explicitWeightsReply, explicitWeightsInput);
            REQUIRE(explicitWeightsWire);
            CHECK(*wire == *explicitWeightsWire);
        }
    }
}

TEST_CASE("offset worker refuses tangent-constrained fit interpolation for generation and cache validation",
          "[lc-offsetcurve][worker-kernel][source-generation][fit-points]") {
    LC_SplinePointsData constrained{false, false};
    constrained.splinePoints = {{0, 0}, {1, 2}, {3, 1.5}, {4, 0}};
    constrained.fitTangentsConstrained = true;
    constrained.fitTolerance = 1e-10;
    const LC_OffsetSourceDefinition definition{
        LC_OffsetSourceGeometry{constrained}, 0.5, 1e-4, false};

    const req::Request generate{token, req::GenerateSource{definition, {}}};
    LC_OffsetWorkerKernel::Trace generateTrace;
    const auto generatedReply = LC_OffsetWorkerKernel::run(generate, &generateTrace);
    const auto& generated = native(generatedReply, 7);
    CHECK(generated.refusal == LC_OffsetCurveRefusal::Engine);
    CHECK(generated.engineRuns == 1);
    CHECK(generated.engineStatus == LC_CurveOffsetStatus::InvalidSource);
    CHECK(generated.ordinaryStatus == result::OrdinaryStatus::None);
    CHECK(generated.ordinary.empty());
    CHECK_FALSE(generated.changed);
    CHECK_FALSE(generated.curve);
    CHECK(generateTrace.nativePrepareEntered);
    CHECK(generateTrace.computationCompleted);

    // This is the cache the unconstrained display approximation would produce.
    // It must not certify the source whose exact interpolation uses tangents.
    LC_SplinePointsData displayApproximation = constrained;
    displayApproximation.fitTangentsConstrained = false;
    auto displaySource = LC_OffsetSourceGeometry{displayApproximation}.makeEntity();
    REQUIRE(displaySource);
    const auto options = LC_CurveOffset::makeDirectOptions(*displaySource, 0.5, 1e-4);
    const auto branches = LC_CurveOffset::buildDirectBranches(*displaySource,
        LC_CurveOffset::makeSideRequest(LC_CurveOffsetSide::Left, 0.5), options,
        LC_CurveOffset::makeDirectSourceBudget());
    REQUIRE(branches.status == LC_CurveOffsetStatus::Ok);
    auto materialized = LC_CurveOffset::materializeBranches(*displaySource, branches, options,
        LC_CurveOffset::makeDirectSourceBudget());
    REQUIRE(materialized.status == LC_CurveOffsetStatus::Ok);
    REQUIRE(materialized.entities.size() == 1);
    const auto* displayCache = dynamic_cast<const RS_Spline*>(materialized.entities.front().get());
    REQUIRE(displayCache);

    const req::Request validate{token, req::ValidateSourceCache{
        displayCache->getData(), definition, {}}};
    LC_OffsetWorkerKernel::Trace validateTrace;
    const auto validatedReply = LC_OffsetWorkerKernel::run(validate, &validateTrace);
    const auto& validated = native(validatedReply, 8);
    CHECK(validated.refusal == LC_OffsetCurveRefusal::NotCacheRepresentable);
    CHECK(validated.engineRuns == 1);
    CHECK(validated.engineStatus == LC_CurveOffsetStatus::InvalidSource);
    CHECK_FALSE(validated.changed);
    CHECK_FALSE(validated.curve);
    CHECK(validateTrace.nativePrepareEntered);
    CHECK(validateTrace.computationCompleted);
}

TEST_CASE("offset worker op8 accepts only the exact canonical source cache",
          "[lc-offsetcurve][worker-kernel][source-cache]") {
    const LC_OffsetSourceDefinition definition{
        LC_OffsetSourceGeometry{RS_LineData{{0, 0}, {4, 0}}}, .5, 1e-4, false};
    const auto generatedRequest = req::Request{token, req::GenerateSource{definition, {}}};
    const auto generatedReply = LC_OffsetWorkerKernel::run(generatedRequest);
    const auto& generated = native(generatedReply, 7);
    REQUIRE(generated.engineStatus == LC_CurveOffsetStatus::Ok);
    REQUIRE(generated.ordinaryStatus == result::OrdinaryStatus::Complete);
    REQUIRE(generated.ordinary.size() == 1);

    RS_SplineData cache;
    if (const auto* spline = std::get_if<RS_SplineData>(&generated.ordinary.front())) cache = *spline;
    else {
        const auto& line = std::get<req::Segment>(generated.ordinary.front());
        cache.degree = 1; cache.controlPoints = {line.start, line.end};
        cache.knotslist = {0, 0, 1, 1}; cache.weights = {1, 1};
    }
    const req::Request input{token, req::ValidateSourceCache{cache, definition, {}}};
    LC_OffsetWorkerKernel::Trace trace;
    const auto reply = LC_OffsetWorkerKernel::run(input, &trace);
    const auto& accepted = native(reply, 8);
    INFO("refusal=" << int(accepted.refusal) << " changed=" << accepted.changed << " runs=" << accepted.engineRuns
        << " status=" << (accepted.engineStatus ? int(*accepted.engineStatus) : -1)
        << " samples=" << accepted.sampleUnits << " attempted=" << accepted.attemptedOutputUsage.cubicPieces
        << "," << accepted.attemptedOutputUsage.outputEntities << "," << accepted.attemptedOutputUsage.deepEntities);
    CHECK(accepted.refusal == LC_OffsetCurveRefusal::None);
    CHECK(accepted.changed);
    CHECK(accepted.engineRuns == 1);
    CHECK(accepted.engineStatus == LC_CurveOffsetStatus::Ok);
    CHECK_FALSE(accepted.curve);
    CHECK(trace.nativePrepareEntered);
    CHECK(trace.computationCompleted);
    const auto wire = result::encode(reply, input);
    REQUIRE(wire);
    CHECK(result::decode(*wire, input));

    auto altered = std::get<req::ValidateSourceCache>(input.operation);
    altered.cache.controlPoints.back().y += 1;
    const auto rejectedRequest = req::Request{token, altered};
    const auto rejectedReply = LC_OffsetWorkerKernel::run(rejectedRequest);
    const auto& rejected = native(rejectedReply, 8);
    CHECK(rejected.refusal == LC_OffsetCurveRefusal::NotCacheRepresentable);
    CHECK_FALSE(rejected.changed);
    CHECK_FALSE(rejected.curve);
    const auto rejectedWire = result::encode(rejectedReply, rejectedRequest);
    REQUIRE(rejectedWire);
    CHECK(result::decode(*rejectedWire, rejectedRequest));
}

TEST_CASE("offset worker op8 promotes a wrapped periodic spline source when its cache is canonical",
          "[lc-offsetcurve][worker-kernel][source-cache][spline-variant]") {
    RS_Spline periodic{nullptr, RS_SplineData{3, false}};
    for (const RS_Vector& point : {RS_Vector{0, 0}, RS_Vector{40, -10}, RS_Vector{60, 30},
                                   RS_Vector{20, 50}, RS_Vector{-15, 25}})
        periodic.addControlPoint(point);
    periodic.setClosed(true);
    auto wrapped = periodic.getData();
    wrapped.weights.clear();
    const LC_OffsetSourceDefinition definition{
        LC_OffsetSourceGeometry{wrapped}, .25, 1e-4, false};

    const req::Request generatedRequest{token, req::GenerateSource{definition, {}}};
    const auto generatedReply = LC_OffsetWorkerKernel::run(generatedRequest);
    const auto& generated = native(generatedReply, 7);
    REQUIRE(generated.engineStatus == LC_CurveOffsetStatus::Ok);
    REQUIRE(generated.ordinaryStatus == result::OrdinaryStatus::Complete);
    REQUIRE(generated.ordinary.size() == 1);
    REQUIRE(std::holds_alternative<RS_SplineData>(generated.ordinary.front()));
    const auto cache = std::get<RS_SplineData>(generated.ordinary.front());

    const req::Request validationRequest{token, req::ValidateSourceCache{cache, definition, {}}};
    LC_OffsetWorkerKernel::Trace trace;
    const auto validationReply = LC_OffsetWorkerKernel::run(validationRequest, &trace);
    const auto& validated = native(validationReply, 8);
    CHECK(validated.refusal == LC_OffsetCurveRefusal::None);
    CHECK(validated.changed);
    CHECK(validated.engineRuns == 1);
    CHECK(trace.computationCompleted);
    const auto encoded = result::encode(validationReply, validationRequest);
    REQUIRE(encoded);
    CHECK(result::decode(*encoded, validationRequest));

    for (const double signedDistance : {-.5, -.25, .25, .5}) {
        CAPTURE(signedDistance);
        auto variantDefinition = definition;
        variantDefinition.signedDistance = signedDistance;
        const req::Request variantGeneration{token,
            req::GenerateSource{variantDefinition, {}}};
        const auto variantGeneratedReply = LC_OffsetWorkerKernel::run(variantGeneration);
        const auto& variantGenerated = native(variantGeneratedReply, 7);
        REQUIRE(variantGenerated.engineStatus == LC_CurveOffsetStatus::Ok);
        REQUIRE(variantGenerated.ordinaryStatus == result::OrdinaryStatus::Complete);
        REQUIRE(variantGenerated.ordinary.size() == 1);
        REQUIRE(std::holds_alternative<RS_SplineData>(variantGenerated.ordinary.front()));
        const auto& variantCache = std::get<RS_SplineData>(variantGenerated.ordinary.front());
        const req::Request variantValidation{token,
            req::ValidateSourceCache{variantCache, variantDefinition, {}}};
        const auto variantValidatedReply = LC_OffsetWorkerKernel::run(variantValidation);
        const auto& variantValidated = native(variantValidatedReply, 8);
        CHECK(variantValidated.refusal == LC_OffsetCurveRefusal::None);
        CHECK(variantValidated.changed);
    }

    auto altered = cache;
    altered.controlPoints.front().x += .125;
    const req::Request alteredRequest{token, req::ValidateSourceCache{altered, definition, {}}};
    const auto alteredReply = LC_OffsetWorkerKernel::run(alteredRequest);
    const auto& refused = native(alteredReply, 8);
    CHECK(refused.refusal == LC_OffsetCurveRefusal::NotCacheRepresentable);
    CHECK_FALSE(refused.changed);
}

TEST_CASE("ellipse, arc, and circle source caches validate across signed distances",
          "[lc-offsetcurve][worker-kernel][source-cache][conic][side]") {
    const double tau = 2.0 * M_PI;
    const std::array<LC_OffsetSourceGeometry, 8> sources{
        LC_OffsetSourceGeometry{RS_EllipseData{{0, 0}, {4, 0}, .6, 0, tau, false}},
        LC_OffsetSourceGeometry{RS_EllipseData{{3, -2}, {3, 0}, .7, .2, 2.1, false}},
        LC_OffsetSourceGeometry{RS_EllipseData{{-3, 1}, {2, 1}, .8, 1.8, .1, true}},
        LC_OffsetSourceGeometry{RS_EllipseData{{1, -2}, {2, 0}, 1.4, .3, 2.3, false}},
        LC_OffsetSourceGeometry{RS_EllipseData{{0, 0}, {2, 0}, .2, 0, tau, false}},
        LC_OffsetSourceGeometry{RS_EllipseData{{0, 0}, {2, 0}, .2, 0, 0, true}},
        LC_OffsetSourceGeometry{RS_ArcData{{-1, 2}, 2.5, .2, 2.4, true}},
        LC_OffsetSourceGeometry{RS_CircleData{{1, -1}, 2}}
    };
    std::size_t acceptedCaches = 0;
    std::size_t refusedCaches = 0;
    std::size_t expectedCases = 0;
    for (const auto& source : sources) {
        CAPTURE(static_cast<int>(source.kind()));
        REQUIRE(source.isWellFormed());
        const auto* sourceEllipse = std::get_if<RS_EllipseData>(&source.value);
        const bool highEccentricity = sourceEllipse && sourceEllipse->ratio == .2;
        const std::vector<double> distances = highEccentricity
            ? std::vector<double>{-.09, -.08, -.07, -.05, .05, .07, .08, .09}
            : std::vector<double>{-.4, -.1, .1, .4};
        expectedCases += distances.size();
        for (const double signedDistance : distances) {
            CAPTURE(signedDistance);
            const LC_OffsetSourceDefinition definition{source, signedDistance, 1e-4, false};
            const req::Request generationRequest{token,
                req::GenerateSource{definition, {}}};
            const auto generationReply = LC_OffsetWorkerKernel::run(generationRequest);
            const auto& generated = native(generationReply, 7);
            REQUIRE(generated.engineStatus == LC_CurveOffsetStatus::Ok);
            REQUIRE(generated.ordinaryStatus == result::OrdinaryStatus::Complete);
            REQUIRE_FALSE(generated.ordinary.empty());
            if (highEccentricity) {
                const bool inward = signedDistance *
                    (sourceEllipse->reversed ? -1.0 : 1.0) > 0.0;
                const double magnitude = std::abs(signedDistance);
                if (!inward || magnitude < .08)
                    REQUIRE(generated.ordinary.size() == 1);
                if (magnitude == .08)
                    CHECK(generated.ordinary.size() == 1);
                if (inward && magnitude == .09) CHECK(generated.ordinary.size() > 1);
            } else {
                REQUIRE(generated.ordinary.size() == 1);
            }

            RS_SplineData cache;
            if (const auto* spline = std::get_if<RS_SplineData>(&generated.ordinary.front())) {
                cache = *spline;
            } else {
                const auto* segment = std::get_if<req::Segment>(&generated.ordinary.front());
                REQUIRE(segment);
                cache.degree = 1;
                cache.controlPoints = {segment->start, segment->end};
                cache.knotslist = {0, 0, 1, 1};
                cache.weights = {1, 1};
            }

            const req::Request validationRequest{token,
                req::ValidateSourceCache{cache, definition, {}}};
            const auto validationReply = LC_OffsetWorkerKernel::run(validationRequest);
            const auto& validated = native(validationReply, 8);
            if (generated.ordinary.size() == 1) {
                CHECK(validated.refusal == LC_OffsetCurveRefusal::None);
                CHECK(validated.changed);
                ++acceptedCaches;
            } else {
                CHECK(validated.refusal == LC_OffsetCurveRefusal::NotCacheRepresentable);
                CHECK_FALSE(validated.changed);
                ++refusedCaches;
            }
            CHECK(validated.engineRuns == 1);
        }
    }
    CHECK(acceptedCaches + refusedCaches == expectedCases);
    CHECK(acceptedCaches >= 24);
    CHECK(refusedCaches > 0);
}

TEST_CASE("parabola source caches validate across orientation and curvature distances",
          "[lc-offsetcurve][worker-kernel][source-cache][parabola][side]") {
    const LC_ParabolaData bowl{{RS_Vector{0, 0}, RS_Vector{5, 10}, RS_Vector{10, 0}}};
    const LC_ParabolaData reversed{{RS_Vector{10, 0}, RS_Vector{5, 10}, RS_Vector{0, 0}}};
    const LC_ParabolaData rotated{{RS_Vector{3, 4}, RS_Vector{-7, 9}, RS_Vector{3, 14}}};
    const std::array<LC_OffsetSourceGeometry, 3> sources{
        LC_OffsetSourceGeometry{bowl},
        LC_OffsetSourceGeometry{reversed},
        LC_OffsetSourceGeometry{rotated}
    };
    std::size_t acceptedCaches = 0;
    std::size_t refusedCaches = 0;
    for (std::size_t sourceIndex = 0; sourceIndex < sources.size(); ++sourceIndex) {
        const auto& source = sources[sourceIndex];
        CAPTURE(sourceIndex);
        REQUIRE(source.kind() == LC_OffsetSourceGeometry::Kind::Parabola);
        REQUIRE(source.isWellFormed());
        for (const double signedDistance : {-.5, -.25, .25, .5, -2.0, 2.0,
                                            -2.5, 2.5, -3.0, 3.0}) {
            CAPTURE(signedDistance);
            const LC_OffsetSourceDefinition definition{source, signedDistance, 1e-4, false};
            const req::Request generationRequest{token,
                req::GenerateSource{definition, {}}};
            const auto generationReply = LC_OffsetWorkerKernel::run(generationRequest);
            const auto& generated = native(generationReply, 7);
            REQUIRE(generated.engineStatus == LC_CurveOffsetStatus::Ok);
            REQUIRE(generated.ordinaryStatus == result::OrdinaryStatus::Complete);
            REQUIRE_FALSE(generated.ordinary.empty());

            RS_SplineData cache;
            if (const auto* spline = std::get_if<RS_SplineData>(&generated.ordinary.front())) {
                cache = *spline;
            } else {
                const auto* segment = std::get_if<req::Segment>(&generated.ordinary.front());
                REQUIRE(segment);
                cache.degree = 1;
                cache.controlPoints = {segment->start, segment->end};
                cache.knotslist = {0, 0, 1, 1};
                cache.weights = {1, 1};
            }

            const req::Request validationRequest{token,
                req::ValidateSourceCache{cache, definition, {}}};
            const auto validationReply = LC_OffsetWorkerKernel::run(validationRequest);
            const auto& validated = native(validationReply, 8);
            if (generated.ordinary.size() == 1) {
                CHECK(validated.refusal == LC_OffsetCurveRefusal::None);
                CHECK(validated.changed);
                ++acceptedCaches;
            } else {
                CHECK(validated.refusal == LC_OffsetCurveRefusal::NotCacheRepresentable);
                CHECK_FALSE(validated.changed);
                ++refusedCaches;
            }
            CHECK(validated.engineRuns == 1);
        }
    }
    CHECK(acceptedCaches + refusedCaches == sources.size() * 10);
    CHECK(acceptedCaches > 0);
    CHECK(refusedCaches > 0);
}

TEST_CASE("parabola worker cache validation is stable across uniform scales",
          "[lc-offsetcurve][worker-kernel][source-cache][parabola][scale]") {
    std::size_t acceptedCaches = 0;
    std::size_t refusedCaches = 0;
    for (const double scale : {1e-8, 1e-3, 1e3, 1e12}) {
        CAPTURE(scale);
        const LC_ParabolaData parabola{{RS_Vector{0, 0}, RS_Vector{5 * scale, 10 * scale},
                                        RS_Vector{10 * scale, 0}}};
        const LC_OffsetSourceGeometry source{parabola};
        REQUIRE(source.isWellFormed());
        for (const double unitDistance : {-.5, .5, -2.0, 2.0, -2.5, 2.5, -3.0, 3.0}) {
            CAPTURE(unitDistance);
            const double signedDistance = unitDistance * scale;
            const LC_OffsetSourceDefinition definition{source, signedDistance, 1e-4, false};
            const req::Request generationRequest{token, req::GenerateSource{definition, {}}};
            const auto generationReply = LC_OffsetWorkerKernel::run(generationRequest);
            const auto& generated = native(generationReply, 7);
            REQUIRE(generated.engineStatus == LC_CurveOffsetStatus::Ok);
            REQUIRE(generated.ordinaryStatus == result::OrdinaryStatus::Complete);
            REQUIRE_FALSE(generated.ordinary.empty());

            auto sourceEntity = source.makeEntity();
            REQUIRE(sourceEntity);
            const auto effectiveOptions = LC_OffsetWorkerTolerance::directOptions(
                definition, *sourceEntity);
            REQUIRE(effectiveOptions);
            CHECK(effectiveOptions->tolerance.requestedGeometry < scale * 2e-3);

            RS_SplineData cache;
            if (const auto* spline = std::get_if<RS_SplineData>(&generated.ordinary.front())) {
                cache = *spline;
            } else {
                const auto* segment = std::get_if<req::Segment>(&generated.ordinary.front());
                REQUIRE(segment);
                cache.degree = 1;
                cache.controlPoints = {segment->start, segment->end};
                cache.knotslist = {0, 0, 1, 1};
                cache.weights = {1, 1};
            }

            const req::Request validationRequest{token,
                req::ValidateSourceCache{cache, definition, {}}};
            const auto validationReply = LC_OffsetWorkerKernel::run(validationRequest);
            const auto& validated = native(validationReply, 8);
            if (generated.ordinary.size() == 1) {
                CHECK(validated.refusal == LC_OffsetCurveRefusal::None);
                CHECK(validated.changed);
                ++acceptedCaches;
            } else {
                CHECK(validated.refusal == LC_OffsetCurveRefusal::NotCacheRepresentable);
                CHECK_FALSE(validated.changed);
                ++refusedCaches;
            }
            CHECK(validated.engineRuns == 1);
        }
    }
    CHECK(acceptedCaches + refusedCaches == 32);
    CHECK(acceptedCaches > 0);
    CHECK(refusedCaches > 0);
}

TEST_CASE("typed worker tolerance defaults relatively and scales with the source",
          "[lc-offsetcurve][parabola][scale][tolerance][worker-kernel]") {
    double previousNormalized = 0.0;
    for (const double scale : {1e-8, 1e-3, 1e3, 1e12}) {
        CAPTURE(scale);
        const LC_OffsetSourceGeometry source{LC_ParabolaData{{
            RS_Vector{0, 0}, RS_Vector{5 * scale, 10 * scale}, RS_Vector{10 * scale, 0}}}};
        REQUIRE(source.isWellFormed());
        auto sourceEntity = source.makeEntity();
        REQUIRE(sourceEntity);

        const LC_OffsetSourceDefinition defaultDefinition{source, 0.5 * scale, 0.0, false};
        const LC_OffsetSourceDefinition explicitDefinition{
            source, 0.5 * scale, LC_CurveOffset::relativeOffsetTolerance(*sourceEntity), false};
        const auto defaultOptions = LC_OffsetWorkerTolerance::directOptions(
            defaultDefinition, *sourceEntity);
        const auto explicitOptions = LC_OffsetWorkerTolerance::directOptions(
            explicitDefinition, *sourceEntity);
        REQUIRE(defaultOptions);
        REQUIRE(explicitOptions);
        CHECK(defaultOptions->tolerance.requestedGeometry ==
              explicitOptions->tolerance.requestedGeometry);
        const double normalizedTolerance = defaultOptions->tolerance.requestedGeometry / scale;
        if (previousNormalized > 0.0) {
            CHECK(std::abs(normalizedTolerance / previousNormalized - 1.0) < 1e-12);
        }
        previousNormalized = normalizedTolerance;

        const auto generated = LC_OffsetWorkerKernel::run({token,
            req::GenerateSource{defaultDefinition, {}}});
        CHECK(native(generated, 7).engineStatus == LC_CurveOffsetStatus::Ok);
    }
}

TEST_CASE("parabola worker reports the absolute lower distance bound explicitly",
          "[lc-offsetcurve][parabola][scale][refusal][worker-kernel]") {
    constexpr double scale = 1e-12;
    const LC_OffsetSourceGeometry source{LC_ParabolaData{{
        RS_Vector{0, 0}, RS_Vector{5 * scale, 10 * scale}, RS_Vector{10 * scale, 0}}}};
    REQUIRE(source.isWellFormed());
    const LC_OffsetSourceDefinition definition{source, 0.5 * scale, 1e-4, false};
    const auto reply = LC_OffsetWorkerKernel::run({token,
        req::GenerateSource{definition, {}}});
    const auto& generated = native(reply, 7);
    CHECK(generated.refusal == LC_OffsetCurveRefusal::TooSmall);
    CHECK(generated.engineRuns == 0);
    CHECK(generated.ordinaryStatus == result::OrdinaryStatus::None);
    CHECK(generated.ordinary.empty());
    CHECK_FALSE(generated.changed);
}

TEST_CASE("parabola worker cache validation preserves local detail at large coordinates",
          "[lc-offsetcurve][worker-kernel][source-cache][parabola][precision]") {
    const RS_Vector translation{1e9, -1e9};
    const LC_OffsetSourceGeometry centered{LC_ParabolaData{{
        RS_Vector{0, 0}, RS_Vector{5, 10}, RS_Vector{10, 0}}}};
    const LC_OffsetSourceGeometry translated{LC_ParabolaData{{
        RS_Vector{0, 0} + translation, RS_Vector{5, 10} + translation,
        RS_Vector{10, 0} + translation}}};
    REQUIRE(centered.isWellFormed());
    REQUIRE(translated.isWellFormed());

    for (const double signedDistance : {-.5, .5, -2.5, 2.5, -3.0, 3.0}) {
        CAPTURE(signedDistance);
        const LC_OffsetSourceDefinition centeredDefinition{
            centered, signedDistance, 1e-4, false};
        const LC_OffsetSourceDefinition translatedDefinition{
            translated, signedDistance, 1e-4, false};
        const auto centeredReply = LC_OffsetWorkerKernel::run({token,
            req::GenerateSource{centeredDefinition, {}}});
        const auto translatedReply = LC_OffsetWorkerKernel::run({token,
            req::GenerateSource{translatedDefinition, {}}});
        const auto& centeredOutput = native(centeredReply, 7);
        const auto& translatedOutput = native(translatedReply, 7);
        REQUIRE(centeredOutput.engineStatus == LC_CurveOffsetStatus::Ok);
        REQUIRE(translatedOutput.engineStatus == LC_CurveOffsetStatus::Ok);
        REQUIRE(centeredOutput.ordinaryStatus == result::OrdinaryStatus::Complete);
        REQUIRE(translatedOutput.ordinaryStatus == result::OrdinaryStatus::Complete);
        REQUIRE_FALSE(centeredOutput.ordinary.empty());
        REQUIRE_FALSE(translatedOutput.ordinary.empty());
        REQUIRE(centeredOutput.ordinary.size() == translatedOutput.ordinary.size());

        const auto firstCache = [](const result::Native& output) {
            if (const auto* spline = std::get_if<RS_SplineData>(&output.ordinary.front()))
                return *spline;
            const auto& segment = std::get<req::Segment>(output.ordinary.front());
            RS_SplineData cache{1, false};
            cache.controlPoints = {segment.start, segment.end};
            cache.knotslist = {0, 0, 1, 1};
            cache.weights = {1, 1};
            return cache;
        };
        const RS_SplineData centeredCache = firstCache(centeredOutput);
        const RS_SplineData translatedCache = firstCache(translatedOutput);
        const RS_Spline centeredSpline{nullptr, centeredCache};
        const RS_Spline translatedSpline{nullptr, translatedCache};
        double maximumLocalDeviation = 0.0;
        for (int sample = 0; sample <= 128; ++sample) {
            const double parameter = static_cast<double>(sample) / 128.0;
            const RS_Vector local = centeredSpline.getPointAt(parameter);
            const RS_Vector world = translatedSpline.getPointAt(parameter);
            REQUIRE(local.valid);
            REQUIRE(world.valid);
            maximumLocalDeviation = std::max(maximumLocalDeviation,
                std::hypot((world.x - translation.x) - local.x,
                           (world.y - translation.y) - local.y));
        }
        CHECK(maximumLocalDeviation < 1e-3);

        for (const auto& [definition, cache] : {
                 std::pair{centeredDefinition, centeredCache},
                 std::pair{translatedDefinition, translatedCache}}) {
            const auto validation = LC_OffsetWorkerKernel::run({token,
                req::ValidateSourceCache{cache, definition, {}}});
            const auto& validated = native(validation, 8);
            if (centeredOutput.ordinary.size() == 1) {
                CHECK(validated.refusal == LC_OffsetCurveRefusal::None);
                CHECK(validated.changed);
            } else {
                CHECK(validated.refusal == LC_OffsetCurveRefusal::NotCacheRepresentable);
                CHECK_FALSE(validated.changed);
            }
        }
    }
}

TEST_CASE("bounded hyperbola cache promotion preserves bounds and branch across offset sides",
          "[lc-offsetcurve][worker-kernel][source-cache][hyperbola]") {
    const std::array<LC_HyperbolaData, 8> variants{
        LC_HyperbolaData{{0, 0}, {3, 0}, .6, -1.2, 1.5, false},
        LC_HyperbolaData{{0, 0}, {3, 0}, .6, 1.5, -1.2, false},
        LC_HyperbolaData{{0, 0}, {3, 0}, .6, -.4, .4, true},
        LC_HyperbolaData{{0, 0}, {3, 0}, .6, -1.2, 1.5, true},
        LC_HyperbolaData{{0, 0}, {3, 0}, .6, .2, 1.2, false},
        LC_HyperbolaData{{0, 0}, {3, 0}, .6, -1.2, -.2, false},
        LC_HyperbolaData{{0, 0}, {3, 0}, .6, -3.0, 3.0, false},
        LC_HyperbolaData{{0, 0}, {3, 0}, .6, -3.0, 3.0, true}
    };
    std::size_t acceptedCaches = 0;
    std::size_t refusedCaches = 0;
    for (const auto& data : variants) {
        CAPTURE(data.angle1, data.angle2, data.reversed);
        for (const double signedDistance :
             {-.1, .1, -.3, .3, -.5, .5, -1.0, 1.0, -1.05, 1.05,
              -1.08, 1.08, -1.09, 1.09, -1.1, 1.1}) {
            CAPTURE(signedDistance);
            const LC_OffsetSourceDefinition definition{
                LC_OffsetSourceGeometry{data}, signedDistance, 1e-4, false};
            const req::Request generationRequest{token, req::GenerateSource{definition, {}}};
            const auto generationReply = LC_OffsetWorkerKernel::run(generationRequest);
            const auto& generated = native(generationReply, 7);
            REQUIRE(generated.engineStatus == LC_CurveOffsetStatus::Ok);
            REQUIRE(generated.ordinaryStatus == result::OrdinaryStatus::Complete);
            REQUIRE_FALSE(generated.ordinary.empty());
            REQUIRE(std::holds_alternative<RS_SplineData>(generated.ordinary.front()));
            const auto& cache = std::get<RS_SplineData>(generated.ordinary.front());
            const req::Request validationRequest{token,
                req::ValidateSourceCache{cache, definition, {}}};
            const auto validationReply = LC_OffsetWorkerKernel::run(validationRequest);
            const auto& validated = native(validationReply, 8);
            if (generated.ordinary.size() == 1) {
                CHECK(validated.refusal == LC_OffsetCurveRefusal::None);
                CHECK(validated.changed);
                ++acceptedCaches;
            } else {
                CHECK(validated.refusal == LC_OffsetCurveRefusal::NotCacheRepresentable);
                CHECK_FALSE(validated.changed);
                ++refusedCaches;
            }
            CHECK(validated.engineRuns == 1);
        }
    }
    CHECK(acceptedCaches + refusedCaches == variants.size() * 16);
    CHECK(refusedCaches > 0);
}

TEST_CASE("bounded hyperbola worker cache validation scales across curvature neighborhoods",
          "[lc-offsetcurve][worker-kernel][source-cache][hyperbola][scale]") {
    const std::array<LC_HyperbolaData, 8> variants{
        LC_HyperbolaData{{0, 0}, {3, 0}, .6, -1.2, 1.5, false}, LC_HyperbolaData{{0, 0}, {3, 0}, .6, 1.5, -1.2, false},
        LC_HyperbolaData{{0, 0}, {3, 0}, .6, -.4, .4, true},    LC_HyperbolaData{{0, 0}, {3, 0}, .6, -1.2, 1.5, true},
        LC_HyperbolaData{{0, 0}, {3, 0}, .6, .2, 1.2, false},   LC_HyperbolaData{{0, 0}, {3, 0}, .6, -1.2, -.2, false},
        LC_HyperbolaData{{0, 0}, {3, 0}, .6, -3.0, 3.0, false}, LC_HyperbolaData{{0, 0}, {3, 0}, .6, -3.0, 3.0, true}};
    constexpr std::array<double, 4> scales{1e-3, 1.0, 1e3, 1e12};
    constexpr std::array<double, 16> normalizedDistances{-.1,   .1,   -.3,   .3,   -.5,   .5,   -1.0, 1.0,
                                                         -1.05, 1.05, -1.08, 1.08, -1.09, 1.09, -1.1, 1.1};
    std::size_t acceptedCaches = 0;
    std::size_t refusedCaches = 0;
    std::array<std::array<double, normalizedDistances.size()>, variants.size()> unitScaleTolerances{};

    for (const double scale : scales) {
        CAPTURE(scale);
        for (std::size_t variantIndex = 0; variantIndex < variants.size(); ++variantIndex) {
            const auto& base = variants[variantIndex];
            const LC_HyperbolaData data{{base.center.x * scale, base.center.y * scale, base.center.z},
                                        {base.majorP.x * scale, base.majorP.y * scale, base.majorP.z},
                                        base.ratio,
                                        base.angle1,
                                        base.angle2,
                                        base.reversed};
            const LC_OffsetSourceGeometry source{data};
            REQUIRE(source.isWellFormed());
            auto sourceEntity = source.makeEntity();
            REQUIRE(sourceEntity);

            for (std::size_t distanceIndex = 0; distanceIndex < normalizedDistances.size(); ++distanceIndex) {
                const double normalizedDistance = normalizedDistances[distanceIndex];
                CAPTURE(data.angle1, data.angle2, data.reversed, normalizedDistance);
                const double signedDistance = normalizedDistance * scale;
                const LC_OffsetSourceDefinition definition{source, signedDistance, 1e-4, false};
                const auto options = LC_OffsetWorkerTolerance::directOptions(definition, *sourceEntity);
                REQUIRE(options);
                const double normalizedTolerance = options->tolerance.requestedGeometry / scale;
                if (scale == 1.0) {
                    unitScaleTolerances[variantIndex][distanceIndex] = normalizedTolerance;
                }
                else if (scale > 1.0) {
                    const double reference = unitScaleTolerances[variantIndex][distanceIndex];
                    REQUIRE(reference > 0.0);
                    CHECK(std::abs(normalizedTolerance / reference - 1.0) < 1e-10);
                }

                const auto generatedReply = LC_OffsetWorkerKernel::run({token, req::GenerateSource{definition, {}}});
                const auto& generated = native(generatedReply, 7);
                REQUIRE(generated.engineStatus == LC_CurveOffsetStatus::Ok);
                REQUIRE(generated.ordinaryStatus == result::OrdinaryStatus::Complete);
                REQUIRE_FALSE(generated.ordinary.empty());
                REQUIRE(std::holds_alternative<RS_SplineData>(generated.ordinary.front()));
                const auto& cache = std::get<RS_SplineData>(generated.ordinary.front());
                const auto cacheFingerprint = RS_Spline::geometryFingerprint(cache);

                const auto validationReply = LC_OffsetWorkerKernel::run({token, req::ValidateSourceCache{cache, definition, {}}});
                const auto& validated = native(validationReply, 8);
                if (generated.ordinary.size() == 1) {
                    CHECK(validated.refusal == LC_OffsetCurveRefusal::None);
                    CHECK(validated.changed);
                    ++acceptedCaches;
                }
                else {
                    CHECK(validated.refusal == LC_OffsetCurveRefusal::NotCacheRepresentable);
                    CHECK_FALSE(validated.changed);
                    ++refusedCaches;
                }
                CHECK(RS_Spline::geometryFingerprint(cache) == cacheFingerprint);
                CHECK(validated.engineRuns == 1);
            }
        }
    }
    CHECK(acceptedCaches + refusedCaches == scales.size() * variants.size() * normalizedDistances.size());
    CHECK(acceptedCaches > 0);
    CHECK(refusedCaches > 0);
}

TEST_CASE("bounded hyperbola cache validation preserves translated local detail",
          "[lc-offsetcurve][worker-kernel][source-cache][hyperbola][precision]") {
    const RS_Vector translation{1e9, -1e9};
    const std::array<LC_HyperbolaData, 8> variants{
        LC_HyperbolaData{{0, 0}, {3, 0}, .6, -1.2, 1.5, false}, LC_HyperbolaData{{0, 0}, {3, 0}, .6, 1.5, -1.2, false},
        LC_HyperbolaData{{0, 0}, {3, 0}, .6, -.4, .4, true},    LC_HyperbolaData{{0, 0}, {3, 0}, .6, -1.2, 1.5, true},
        LC_HyperbolaData{{0, 0}, {3, 0}, .6, .2, 1.2, false},   LC_HyperbolaData{{0, 0}, {3, 0}, .6, -1.2, -.2, false},
        LC_HyperbolaData{{0, 0}, {3, 0}, .6, -3.0, 3.0, false}, LC_HyperbolaData{{0, 0}, {3, 0}, .6, -3.0, 3.0, true}};
    const auto firstCache = [](const result::Native& output) {
        if (const auto* spline = std::get_if<RS_SplineData>(&output.ordinary.front()))
            return *spline;
        const auto& segment = std::get<req::Segment>(output.ordinary.front());
        RS_SplineData cache{1, false};
        cache.controlPoints = {segment.start, segment.end};
        cache.knotslist = {0, 0, 1, 1};
        cache.weights = {1, 1};
        return cache;
    };

    for (std::size_t variantIndex = 0; variantIndex < variants.size(); ++variantIndex) {
        const auto& base = variants[variantIndex];
        const LC_OffsetSourceGeometry centered{base};
        const LC_OffsetSourceGeometry translated{
            LC_HyperbolaData{{base.center.x + translation.x, base.center.y + translation.y, base.center.z},
                             base.majorP,
                             base.ratio,
                             base.angle1,
                             base.angle2,
                             base.reversed}};
        REQUIRE(centered.isWellFormed());
        REQUIRE(translated.isWellFormed());

        for (const double signedDistance : {-1.05, 1.05, -1.08, 1.08, -1.09, 1.09}) {
            CAPTURE(variantIndex, signedDistance);
            const LC_OffsetSourceDefinition centeredDefinition{centered, signedDistance, 1e-4, false};
            const LC_OffsetSourceDefinition translatedDefinition{translated, signedDistance, 1e-4, false};
            const auto centeredReply = LC_OffsetWorkerKernel::run({token, req::GenerateSource{centeredDefinition, {}}});
            const auto translatedReply = LC_OffsetWorkerKernel::run({token, req::GenerateSource{translatedDefinition, {}}});
            const auto& centeredOutput = native(centeredReply, 7);
            const auto& translatedOutput = native(translatedReply, 7);
            REQUIRE(centeredOutput.engineStatus == LC_CurveOffsetStatus::Ok);
            REQUIRE(translatedOutput.engineStatus == LC_CurveOffsetStatus::Ok);
            REQUIRE(centeredOutput.ordinaryStatus == result::OrdinaryStatus::Complete);
            REQUIRE(translatedOutput.ordinaryStatus == result::OrdinaryStatus::Complete);
            REQUIRE_FALSE(centeredOutput.ordinary.empty());
            REQUIRE_FALSE(translatedOutput.ordinary.empty());
            REQUIRE(centeredOutput.ordinary.size() == translatedOutput.ordinary.size());

            const RS_SplineData centeredCache = firstCache(centeredOutput);
            const RS_SplineData translatedCache = firstCache(translatedOutput);
            const RS_Spline centeredSpline{nullptr, centeredCache};
            const RS_Spline translatedSpline{nullptr, translatedCache};
            const auto centeredEntity = centered.makeEntity();
            const auto translatedEntity = translated.makeEntity();
            REQUIRE(centeredEntity);
            REQUIRE(translatedEntity);
            const auto centeredOptions = LC_OffsetWorkerTolerance::directOptions(centeredDefinition, *centeredEntity);
            const auto translatedOptions = LC_OffsetWorkerTolerance::directOptions(translatedDefinition, *translatedEntity);
            REQUIRE(centeredOptions);
            REQUIRE(translatedOptions);
            const double roundingAllowance =
                64.0 * std::numeric_limits<double>::epsilon() * std::max(std::abs(translation.x), std::abs(translation.y));
            const double deviationLimit =
                4.0 * std::max(centeredOptions->tolerance.requestedGeometry, translatedOptions->tolerance.requestedGeometry) +
                roundingAllowance;
            double maximumLocalDeviation = 0.0;
            for (int sample = 0; sample <= 128; ++sample) {
                const double parameter = static_cast<double>(sample) / 128.0;
                const RS_Vector local = centeredSpline.getPointAt(parameter);
                const RS_Vector world = translatedSpline.getPointAt(parameter);
                REQUIRE(local.valid);
                REQUIRE(world.valid);
                maximumLocalDeviation =
                    std::max(maximumLocalDeviation, std::hypot((world.x - translation.x) - local.x, (world.y - translation.y) - local.y));
            }
            CHECK(maximumLocalDeviation <= deviationLimit);

            for (const auto& [definition, cache, expectedOutput] : {std::tuple{centeredDefinition, centeredCache, &centeredOutput},
                                                                    std::tuple{translatedDefinition, translatedCache, &translatedOutput}}) {
                const auto validation = LC_OffsetWorkerKernel::run({token, req::ValidateSourceCache{cache, definition, {}}});
                const auto& validated = native(validation, 8);
                if (expectedOutput->ordinary.size() == 1) {
                    CHECK(validated.refusal == LC_OffsetCurveRefusal::None);
                    CHECK(validated.changed);
                }
                else {
                    CHECK(validated.refusal == LC_OffsetCurveRefusal::NotCacheRepresentable);
                    CHECK_FALSE(validated.changed);
                }
                CHECK(validated.engineRuns == 1);
            }
        }
    }
}

TEST_CASE("bounded hyperbola worker preserves detail across combined scale and translation",
          "[lc-offsetcurve][worker-kernel][source-cache][hyperbola][precision][combined]") {
    const std::array<LC_HyperbolaData, 8> variants{
        LC_HyperbolaData{{0, 0}, {3, 0}, .6, -1.2, 1.5, false}, LC_HyperbolaData{{0, 0}, {3, 0}, .6, 1.5, -1.2, false},
        LC_HyperbolaData{{0, 0}, {3, 0}, .6, -.4, .4, true},    LC_HyperbolaData{{0, 0}, {3, 0}, .6, -1.2, 1.5, true},
        LC_HyperbolaData{{0, 0}, {3, 0}, .6, .2, 1.2, false},   LC_HyperbolaData{{0, 0}, {3, 0}, .6, -1.2, -.2, false},
        LC_HyperbolaData{{0, 0}, {3, 0}, .6, -3.0, 3.0, false}, LC_HyperbolaData{{0, 0}, {3, 0}, .6, -3.0, 3.0, true}};
    constexpr std::array<double, 4> scales{1e-3, 1.0, 1e3, 1e12};
    constexpr std::array<double, 6> normalizedDistances{-1.05, 1.05, -1.08, 1.08, -1.09, 1.09};
    const auto firstCache = [](const result::Native& output) {
        if (const auto* spline = std::get_if<RS_SplineData>(&output.ordinary.front()))
            return *spline;
        const auto& segment = std::get<req::Segment>(output.ordinary.front());
        RS_SplineData cache{1, false};
        cache.controlPoints = {segment.start, segment.end};
        cache.knotslist = {0, 0, 1, 1};
        cache.weights = {1, 1};
        return cache;
    };

    std::size_t acceptedCaches = 0;
    std::size_t refusedCaches = 0;
    for (const double scale : scales) {
        CAPTURE(scale);
        const RS_Vector translation{1e9 * scale, -1e9 * scale};
        std::size_t acceptedAtScale = 0;
        std::size_t refusedAtScale = 0;
        for (std::size_t variantIndex = 0; variantIndex < variants.size(); ++variantIndex) {
            const auto& base = variants[variantIndex];
            const LC_HyperbolaData centeredData{{base.center.x * scale, base.center.y * scale, base.center.z},
                                                {base.majorP.x * scale, base.majorP.y * scale, base.majorP.z},
                                                base.ratio,
                                                base.angle1,
                                                base.angle2,
                                                base.reversed};
            const LC_HyperbolaData translatedData{
                {centeredData.center.x + translation.x, centeredData.center.y + translation.y, centeredData.center.z},
                centeredData.majorP,
                centeredData.ratio,
                centeredData.angle1,
                centeredData.angle2,
                centeredData.reversed};
            const LC_OffsetSourceGeometry centered{centeredData};
            const LC_OffsetSourceGeometry translated{translatedData};
            REQUIRE(centered.isWellFormed());
            REQUIRE(translated.isWellFormed());
            const auto centeredEntity = centered.makeEntity();
            const auto translatedEntity = translated.makeEntity();
            REQUIRE(centeredEntity);
            REQUIRE(translatedEntity);

            for (const double normalizedDistance : normalizedDistances) {
                CAPTURE(variantIndex, normalizedDistance);
                const double signedDistance = normalizedDistance * scale;
                const LC_OffsetSourceDefinition centeredDefinition{centered, signedDistance, 1e-4, false};
                const LC_OffsetSourceDefinition translatedDefinition{translated, signedDistance, 1e-4, false};
                const auto centeredOptions = LC_OffsetWorkerTolerance::directOptions(centeredDefinition, *centeredEntity);
                const auto translatedOptions = LC_OffsetWorkerTolerance::directOptions(translatedDefinition, *translatedEntity);
                REQUIRE(centeredOptions);
                REQUIRE(translatedOptions);
                const double toleranceRoundingAllowance = 64.0 * std::numeric_limits<double>::epsilon() *
                    std::max(std::abs(translation.x), std::abs(translation.y)) * centeredDefinition.relativeTolerance;
                CHECK(std::abs(centeredOptions->tolerance.requestedGeometry - translatedOptions->tolerance.requestedGeometry) <=
                      toleranceRoundingAllowance);

                const auto centeredReply = LC_OffsetWorkerKernel::run({token, req::GenerateSource{centeredDefinition, {}}});
                const auto translatedReply = LC_OffsetWorkerKernel::run({token, req::GenerateSource{translatedDefinition, {}}});
                const auto& centeredOutput = native(centeredReply, 7);
                const auto& translatedOutput = native(translatedReply, 7);
                REQUIRE(centeredOutput.engineStatus == LC_CurveOffsetStatus::Ok);
                REQUIRE(translatedOutput.engineStatus == LC_CurveOffsetStatus::Ok);
                REQUIRE(centeredOutput.ordinaryStatus == result::OrdinaryStatus::Complete);
                REQUIRE(translatedOutput.ordinaryStatus == result::OrdinaryStatus::Complete);
                REQUIRE_FALSE(centeredOutput.ordinary.empty());
                REQUIRE_FALSE(translatedOutput.ordinary.empty());
                REQUIRE(centeredOutput.ordinary.size() == translatedOutput.ordinary.size());

                const RS_SplineData centeredCache = firstCache(centeredOutput);
                const RS_SplineData translatedCache = firstCache(translatedOutput);
                const RS_Spline centeredSpline{nullptr, centeredCache};
                const RS_Spline translatedSpline{nullptr, translatedCache};
                const double roundingAllowance =
                    64.0 * std::numeric_limits<double>::epsilon() * std::max(std::abs(translation.x), std::abs(translation.y));
                const double deviationLimit =
                    4.0 * std::max(centeredOptions->tolerance.requestedGeometry, translatedOptions->tolerance.requestedGeometry) +
                    roundingAllowance;
                double maximumLocalDeviation = 0.0;
                for (int sample = 0; sample <= 128; ++sample) {
                    const double parameter = static_cast<double>(sample) / 128.0;
                    const RS_Vector local = centeredSpline.getPointAt(parameter);
                    const RS_Vector world = translatedSpline.getPointAt(parameter);
                    REQUIRE(local.valid);
                    REQUIRE(world.valid);
                    maximumLocalDeviation = std::max(maximumLocalDeviation,
                                                     std::hypot((world.x - translation.x) - local.x, (world.y - translation.y) - local.y));
                }
                CHECK(maximumLocalDeviation <= deviationLimit);

                for (const auto& [definition, cache, expectedOutput] :
                     {std::tuple{centeredDefinition, centeredCache, &centeredOutput},
                      std::tuple{translatedDefinition, translatedCache, &translatedOutput}}) {
                    const auto cacheFingerprint = RS_Spline::geometryFingerprint(cache);
                    const auto validation = LC_OffsetWorkerKernel::run({token, req::ValidateSourceCache{cache, definition, {}}});
                    const auto& validated = native(validation, 8);
                    if (expectedOutput->ordinary.size() == 1) {
                        CHECK(validated.refusal == LC_OffsetCurveRefusal::None);
                        CHECK(validated.changed);
                        ++acceptedAtScale;
                        ++acceptedCaches;
                    }
                    else {
                        CHECK(validated.refusal == LC_OffsetCurveRefusal::NotCacheRepresentable);
                        CHECK_FALSE(validated.changed);
                        CHECK(RS_Spline::geometryFingerprint(cache) == cacheFingerprint);
                        ++refusedAtScale;
                        ++refusedCaches;
                    }
                    CHECK(validated.engineRuns == 1);
                }
            }
        }
        CHECK(acceptedAtScale > 0);
        CHECK(refusedAtScale > 0);
    }
    CHECK(acceptedCaches + refusedCaches == scales.size() * variants.size() * normalizedDistances.size() * 2);
}

TEST_CASE("bounded hyperbola worker classifies relative translation boundaries",
          "[lc-offsetcurve][worker-kernel][source-cache][hyperbola][precision][translation-boundary]") {
    const std::array<LC_HyperbolaData, 8> variants{
        LC_HyperbolaData{{0, 0}, {3, 0}, .6, -1.2, 1.5, false}, LC_HyperbolaData{{0, 0}, {3, 0}, .6, 1.5, -1.2, false},
        LC_HyperbolaData{{0, 0}, {3, 0}, .6, -.4, .4, true},    LC_HyperbolaData{{0, 0}, {3, 0}, .6, -1.2, 1.5, true},
        LC_HyperbolaData{{0, 0}, {3, 0}, .6, .2, 1.2, false},   LC_HyperbolaData{{0, 0}, {3, 0}, .6, -1.2, -.2, false},
        LC_HyperbolaData{{0, 0}, {3, 0}, .6, -3.0, 3.0, false}, LC_HyperbolaData{{0, 0}, {3, 0}, .6, -3.0, 3.0, true}};
    constexpr std::array<double, 3> scales{1e-3, 1.0, 1e12};
    constexpr std::array<double, 3> translationRatios{1e6, 1e9, 1e12};
    constexpr std::array<double, 6> normalizedDistances{-1.05, 1.05, -1.08, 1.08, -1.09, 1.09};
    const auto firstCache = [](const result::Native& output) {
        if (const auto* spline = std::get_if<RS_SplineData>(&output.ordinary.front()))
            return *spline;
        const auto& segment = std::get<req::Segment>(output.ordinary.front());
        RS_SplineData cache{1, false};
        cache.controlPoints = {segment.start, segment.end};
        cache.knotslist = {0, 0, 1, 1};
        cache.weights = {1, 1};
        return cache;
    };

    std::size_t completedTranslated = 0;
    std::size_t refusedTranslated = 0;
    for (const double scale : scales) {
        CAPTURE(scale);
        for (const double translationRatio : translationRatios) {
            CAPTURE(translationRatio);
            const RS_Vector translation{translationRatio * scale, -translationRatio * scale};
            for (std::size_t variantIndex = 0; variantIndex < variants.size(); ++variantIndex) {
                const auto& base = variants[variantIndex];
                const LC_HyperbolaData centeredData{{base.center.x * scale, base.center.y * scale, base.center.z},
                                                    {base.majorP.x * scale, base.majorP.y * scale, base.majorP.z},
                                                    base.ratio,
                                                    base.angle1,
                                                    base.angle2,
                                                    base.reversed};
                const LC_HyperbolaData translatedData{
                    {centeredData.center.x + translation.x, centeredData.center.y + translation.y, centeredData.center.z},
                    centeredData.majorP,
                    centeredData.ratio,
                    centeredData.angle1,
                    centeredData.angle2,
                    centeredData.reversed};
                const LC_OffsetSourceGeometry centered{centeredData};
                const LC_OffsetSourceGeometry translated{translatedData};
                REQUIRE(centered.isWellFormed());
                REQUIRE(translated.isWellFormed());
                for (const double normalizedDistance : normalizedDistances) {
                    CAPTURE(variantIndex, normalizedDistance);
                    const double signedDistance = normalizedDistance * scale;
                    const LC_OffsetSourceDefinition centeredDefinition{centered, signedDistance, 1e-4, false};
                    const LC_OffsetSourceDefinition translatedDefinition{translated, signedDistance, 1e-4, false};
                    const auto centeredReply = LC_OffsetWorkerKernel::run({token, req::GenerateSource{centeredDefinition, {}}});
                    const auto translatedReply = LC_OffsetWorkerKernel::run({token, req::GenerateSource{translatedDefinition, {}}});
                    const auto& centeredOutput = native(centeredReply, 7);
                    const auto& translatedOutput = native(translatedReply, 7);
                    REQUIRE(centeredOutput.engineStatus == LC_CurveOffsetStatus::Ok);
                    REQUIRE(centeredOutput.ordinaryStatus == result::OrdinaryStatus::Complete);
                    REQUIRE_FALSE(centeredOutput.ordinary.empty());

                    if (translatedOutput.engineStatus != LC_CurveOffsetStatus::Ok ||
                        translatedOutput.ordinaryStatus != result::OrdinaryStatus::Complete || translatedOutput.ordinary.empty()) {
                        CHECK(translatedOutput.ordinary.empty());
                        CHECK_FALSE(translatedOutput.changed);
                        ++refusedTranslated;
                        continue;
                    }

                    ++completedTranslated;
                    CHECK(centeredOutput.ordinary.size() == translatedOutput.ordinary.size());
                    const RS_SplineData centeredCache = firstCache(centeredOutput);
                    const RS_SplineData translatedCache = firstCache(translatedOutput);
                    const RS_Spline centeredSpline{nullptr, centeredCache};
                    const RS_Spline translatedSpline{nullptr, translatedCache};
                    const double roundingAllowance =
                        64.0 * std::numeric_limits<double>::epsilon() * std::max(std::abs(translation.x), std::abs(translation.y));
                    const auto centeredEntity = centered.makeEntity();
                    const auto translatedEntity = translated.makeEntity();
                    REQUIRE(centeredEntity);
                    REQUIRE(translatedEntity);
                    const LC_OffsetSourceDefinition centeredWithDistance{centered, signedDistance, 1e-4, false};
                    const LC_OffsetSourceDefinition translatedWithDistance{translated, signedDistance, 1e-4, false};
                    const auto centeredOptions = LC_OffsetWorkerTolerance::directOptions(centeredWithDistance, *centeredEntity);
                    const auto translatedOptions = LC_OffsetWorkerTolerance::directOptions(translatedWithDistance, *translatedEntity);
                    REQUIRE(centeredOptions);
                    REQUIRE(translatedOptions);
                    const double deviationLimit =
                        4.0 * std::max(centeredOptions->tolerance.requestedGeometry, translatedOptions->tolerance.requestedGeometry) +
                        roundingAllowance;
                    double maximumLocalDeviation = 0.0;
                    for (int sample = 0; sample <= 128; ++sample) {
                        const double parameter = static_cast<double>(sample) / 128.0;
                        const RS_Vector local = centeredSpline.getPointAt(parameter);
                        const RS_Vector world = translatedSpline.getPointAt(parameter);
                        REQUIRE(local.valid);
                        REQUIRE(world.valid);
                        maximumLocalDeviation = std::max(
                            maximumLocalDeviation, std::hypot((world.x - translation.x) - local.x, (world.y - translation.y) - local.y));
                    }
                    CHECK(maximumLocalDeviation <= deviationLimit);

                    for (const auto& [definition, cache, expectedOutput] :
                         {std::tuple{centeredDefinition, centeredCache, &centeredOutput},
                          std::tuple{translatedDefinition, translatedCache, &translatedOutput}}) {
                        const auto fingerprint = RS_Spline::geometryFingerprint(cache);
                        const auto validation = LC_OffsetWorkerKernel::run({token, req::ValidateSourceCache{cache, definition, {}}});
                        const auto& validated = native(validation, 8);
                        if (expectedOutput->ordinary.size() == 1) {
                            CHECK(validated.refusal == LC_OffsetCurveRefusal::None);
                            CHECK(validated.changed);
                        }
                        else {
                            CHECK(validated.refusal == LC_OffsetCurveRefusal::NotCacheRepresentable);
                            CHECK_FALSE(validated.changed);
                            CHECK(RS_Spline::geometryFingerprint(cache) == fingerprint);
                        }
                        CHECK(validated.engineRuns == 1);
                    }
                }
            }
        }
    }
    CHECK(completedTranslated > 0);
    CHECK(refusedTranslated > 0);
}

TEST_CASE("ellipse worker preserves detail across combined scale and translation",
          "[lc-offsetcurve][worker-kernel][source-cache][ellipse][precision][combined]") {
    constexpr double tau = 2.0 * M_PI;
    const std::array<RS_EllipseData, 9> variants{
        RS_EllipseData{{2, -3}, {4, 0}, .6, .2, 2.1, false},
        RS_EllipseData{{-1, 2}, {3, 1}, .8, 1.8, .1, true},
        RS_EllipseData{{3, 1}, {2, -1}, 1.4, .3, 2.3, false},
        RS_EllipseData{{0, 0}, {2, 0}, .2, 0, tau, false},
        RS_EllipseData{{0, 0}, {2, 0}, .2, 0, tau, true},
        RS_EllipseData{{0, 0}, {std::sqrt(3.0), 1}, .2, 0, tau, false},
        RS_EllipseData{{0, 0}, {std::sqrt(3.0), 1}, .2, 0, tau, true},
        RS_EllipseData{{0, 0}, {1, std::sqrt(3.0)}, .2, 0, tau, false},
        RS_EllipseData{{0, 0}, {1, std::sqrt(3.0)}, .2, 0, tau, true}};
    constexpr std::array<double, 3> scales{1e-3, 1.0, 1e12};
    constexpr std::array<double, 3> translationRatios{1e6, 1e9, 1e12};
    constexpr std::array<double, 4> ordinaryNormalizedDistances{-.4, -.1, .1, .4};
    constexpr std::array<double, 8> thinNormalizedDistances{-.09, -.08, -.07, -.05, .05, .07, .08, .09};
    const auto asSpline = [](const req::Cutter& item) {
        if (const auto* spline = std::get_if<RS_SplineData>(&item)) return *spline;
        const auto& segment = std::get<req::Segment>(item);
        RS_SplineData cache{1, false};
        cache.controlPoints = {segment.start, segment.end};
        cache.knotslist = {0, 0, 1, 1};
        cache.weights = {1, 1};
        return cache;
    };

    std::size_t completedPairs = 0;
    std::size_t generationOrToleranceRefusals = 0;
    std::size_t acceptedCaches = 0;
    for (const double scale : scales) {
        CAPTURE(scale);
        for (const double translationRatio : translationRatios) {
          CAPTURE(translationRatio);
          const RS_Vector translation{translationRatio * scale, -translationRatio * scale};
          for (std::size_t variantIndex = 0; variantIndex < variants.size(); ++variantIndex) {
            CAPTURE(variantIndex);
            const auto& base = variants[variantIndex];
            const RS_EllipseData centeredData{{base.center.x * scale, base.center.y * scale, base.center.z},
                                              {base.majorP.x * scale, base.majorP.y * scale, base.majorP.z},
                                              base.ratio, base.angle1, base.angle2, base.reversed};
            const RS_EllipseData translatedData{{centeredData.center.x + translation.x,
                                                 centeredData.center.y + translation.y, centeredData.center.z},
                                                centeredData.majorP, centeredData.ratio, centeredData.angle1,
                                                centeredData.angle2, centeredData.reversed};
            const LC_OffsetSourceGeometry centered{centeredData};
            const LC_OffsetSourceGeometry translated{translatedData};
            REQUIRE(centered.isWellFormed());
            REQUIRE(translated.isWellFormed());
            const auto centeredEntity = centered.makeEntity();
            const auto translatedEntity = translated.makeEntity();
            REQUIRE(centeredEntity);
            REQUIRE(translatedEntity);
            const bool thin = base.ratio == .2;
            const std::vector<double> distances = thin
                ? std::vector<double>(thinNormalizedDistances.begin(), thinNormalizedDistances.end())
                : std::vector<double>(ordinaryNormalizedDistances.begin(), ordinaryNormalizedDistances.end());

            for (const double normalizedDistance : distances) {
                CAPTURE(normalizedDistance);
                const double signedDistance = normalizedDistance * scale;
                const LC_OffsetSourceDefinition centeredDefinition{centered, signedDistance, 1e-4, false};
                const LC_OffsetSourceDefinition translatedDefinition{translated, signedDistance, 1e-4, false};
                const auto centeredOptions = LC_OffsetWorkerTolerance::directOptions(centeredDefinition, *centeredEntity);
                const auto translatedOptions = LC_OffsetWorkerTolerance::directOptions(translatedDefinition, *translatedEntity);

                const auto centeredReply = LC_OffsetWorkerKernel::run(
                    {token, req::GenerateSource{centeredDefinition, {}}});
                const auto& centeredOutput = native(centeredReply, 7);
                const auto translatedReply = LC_OffsetWorkerKernel::run(
                    {token, req::GenerateSource{translatedDefinition, {}}});
                const auto& translatedOutput = native(translatedReply, 7);
                const auto generated = [](const result::Native& output) {
                    return output.engineStatus == LC_CurveOffsetStatus::Ok &&
                           output.ordinaryStatus == result::OrdinaryStatus::Complete &&
                           !output.ordinary.empty();
                };
                const bool centeredGenerated = generated(centeredOutput);
                const bool translatedGenerated = generated(translatedOutput);
                const auto validateOutput = [&](const LC_OffsetSourceDefinition& definition,
                                                const auto& options,
                                                const result::Native& output) {
                    if (!generated(output)) return;
                    const auto cache = asSpline(output.ordinary.front());
                    const auto fingerprint = RS_Spline::geometryFingerprint(cache);
                    const auto validation = LC_OffsetWorkerKernel::run(
                        {token, req::ValidateSourceCache{cache, definition, {}}});
                    const auto& validated = native(validation, 8);
                    if (options && output.ordinary.size() == 1) {
                        CHECK(validated.refusal == LC_OffsetCurveRefusal::None);
                        CHECK(validated.changed);
                        ++acceptedCaches;
                    } else {
                        CHECK(validated.refusal != LC_OffsetCurveRefusal::None);
                        CHECK_FALSE(validated.changed);
                        CHECK(RS_Spline::geometryFingerprint(cache) == fingerprint);
                    }
                    CHECK(validated.engineRuns == 1);
                };
                validateOutput(centeredDefinition, centeredOptions, centeredOutput);
                validateOutput(translatedDefinition, translatedOptions, translatedOutput);
                if (!centeredGenerated || !translatedGenerated) {
                    if (!centeredGenerated) {
                        CHECK(centeredOutput.ordinary.empty());
                        CHECK_FALSE(centeredOutput.changed);
                    }
                    if (!translatedGenerated) {
                        CHECK(translatedOutput.ordinary.empty());
                        CHECK_FALSE(translatedOutput.changed);
                    }
                    ++generationOrToleranceRefusals;
                    continue;
                }

                if (!centeredOptions || !translatedOptions ||
                    centeredOutput.ordinary.size() != translatedOutput.ordinary.size()) {
                    ++generationOrToleranceRefusals;
                    continue;
                }
                ++completedPairs;

                if (centeredOutput.ordinary.size() == translatedOutput.ordinary.size()) {
                    const double roundingAllowance = 64.0 * std::numeric_limits<double>::epsilon() *
                        std::max(std::abs(translation.x), std::abs(translation.y));
                    const double deviationLimit = 4.0 * std::max(centeredOptions->tolerance.requestedGeometry,
                                                                  translatedOptions->tolerance.requestedGeometry) + roundingAllowance;
                    for (std::size_t branch = 0; branch < centeredOutput.ordinary.size(); ++branch) {
                        const RS_Spline centeredSpline{nullptr, asSpline(centeredOutput.ordinary[branch])};
                        const RS_Spline translatedSpline{nullptr, asSpline(translatedOutput.ordinary[branch])};
                        double maximumLocalDeviation = 0.0;
                        for (int sample = 0; sample <= 128; ++sample) {
                            const double parameter = static_cast<double>(sample) / 128.0;
                            const RS_Vector local = centeredSpline.getPointAt(parameter);
                            const RS_Vector world = translatedSpline.getPointAt(parameter);
                            REQUIRE(local.valid);
                            REQUIRE(world.valid);
                            maximumLocalDeviation = std::max(maximumLocalDeviation,
                                std::hypot((world.x - translation.x) - local.x,
                                           (world.y - translation.y) - local.y));
                        }
                        CHECK(maximumLocalDeviation <= deviationLimit);
                    }
                }
            }
          }
        }
    }
    CHECK(completedPairs + generationOrToleranceRefusals == scales.size() *
          translationRatios.size() *
          (3 * ordinaryNormalizedDistances.size() + 6 * thinNormalizedDistances.size()));
    CHECK(completedPairs > 0);
    CHECK(acceptedCaches > 0);
}

TEST_CASE("polyline cache promotion preserves mixed segment orientation and offset side",
          "[lc-offsetcurve][worker-kernel][source-cache][polyline]") {
    const std::array<LC_OffsetPolylineData, 2> variants{
        LC_OffsetPolylineData{{0, 0}, {2, 2}, false, {RS_ArcData{{1, 0}, 1, M_PI, 0, false}, RS_LineData{{2, 0}, {2, 2}}}},
        LC_OffsetPolylineData{{2, 2}, {0, 0}, false, {RS_LineData{{2, 2}, {2, 0}}, RS_ArcData{{1, 0}, 1, 0, M_PI, true}}}};
    for (const auto& data : variants) {
        CAPTURE(data.startpoint.x, data.startpoint.y, data.endpoint.x, data.endpoint.y);
        const LC_OffsetSourceGeometry source{data};
        REQUIRE(source.isWellFormed());
        for (const double signedDistance : {-.25, .25}) {
            CAPTURE(signedDistance);
            const LC_OffsetSourceDefinition definition{source, signedDistance, 1e-4, false};
            const req::Request generationRequest{token, req::GenerateSource{definition, {}}};
            const auto generationReply = LC_OffsetWorkerKernel::run(generationRequest);
            const auto& generated = native(generationReply, 7);
            REQUIRE(generated.engineStatus == LC_CurveOffsetStatus::Ok);
            REQUIRE(generated.ordinaryStatus == result::OrdinaryStatus::Complete);
            REQUIRE(generated.ordinary.size() == 1);
            REQUIRE(std::holds_alternative<RS_SplineData>(generated.ordinary.front()));
            const auto& cache = std::get<RS_SplineData>(generated.ordinary.front());
            const req::Request validationRequest{token, req::ValidateSourceCache{cache, definition, {}}};
            const auto validationReply = LC_OffsetWorkerKernel::run(validationRequest);
            const auto& validated = native(validationReply, 8);
            CHECK(validated.refusal == LC_OffsetCurveRefusal::None);
            CHECK(validated.changed);
        }
    }
}

TEST_CASE("closed polyline cache promotion or refusal covers line and arc chains on both sides",
          "[lc-offsetcurve][worker-kernel][source-cache][polyline]") {
    const double quarterTurn = M_PI / 2.0;
    RS_Polyline mixedBulgePolyline;
    mixedBulgePolyline.addVertex({0, 0}, 0.2);
    mixedBulgePolyline.addVertex({8, 0}, -0.2);
    mixedBulgePolyline.addVertex({8, 6});
    mixedBulgePolyline.addVertex({0, 6});
    mixedBulgePolyline.setClosed(true);
    mixedBulgePolyline.endPolyline();
    const auto mixedBulgeSource = LC_OffsetSourceGeometry::capture(mixedBulgePolyline);
    REQUIRE(mixedBulgeSource);
    REQUIRE(mixedBulgeSource->kind() == LC_OffsetSourceGeometry::Kind::Polyline);
    const auto& mixedBulgeData = std::get<LC_OffsetPolylineData>(mixedBulgeSource->value);
    REQUIRE(mixedBulgeData.closed);
    REQUIRE(mixedBulgeData.segments.size() == 4);
    const std::array<LC_OffsetPolylineData, 3> variants{
        LC_OffsetPolylineData{
            {0, 0},
            {0, 0},
            true,
            {RS_LineData{{0, 0}, {2, 0}}, RS_LineData{{2, 0}, {2, 2}}, RS_LineData{{2, 2}, {0, 2}}, RS_LineData{{0, 2}, {0, 0}}}},
        LC_OffsetPolylineData{{2, 0},
                              {2, 0},
                              true,
                              {RS_ArcData{{0, 0}, 2, 0, quarterTurn, false}, RS_ArcData{{0, 0}, 2, quarterTurn, M_PI, false},
                               RS_ArcData{{0, 0}, 2, M_PI, 3 * quarterTurn, false},
                               RS_ArcData{{0, 0}, 2, 3 * quarterTurn, 2 * M_PI, false}}},
        mixedBulgeData};
    for (const auto& data : variants) {
        const LC_OffsetSourceGeometry source{data};
        REQUIRE(source.isWellFormed());
        for (const double signedDistance : {-.25, .25}) {
            CAPTURE(signedDistance, data.segments.size());
            const LC_OffsetSourceDefinition definition{source, signedDistance, 1e-4, false};
            const req::Request generationRequest{token, req::GenerateSource{definition, {}}};
            const auto generationReply = LC_OffsetWorkerKernel::run(generationRequest);
            const auto& generated = native(generationReply, 7);
            REQUIRE(generated.engineStatus == LC_CurveOffsetStatus::Ok);
            REQUIRE(generated.ordinaryStatus == result::OrdinaryStatus::Complete);
            REQUIRE_FALSE(generated.ordinary.empty());
            RS_SplineData cache;
            if (const auto* spline = std::get_if<RS_SplineData>(&generated.ordinary.front())) {
                cache = *spline;
            } else {
                const auto& segment = std::get<req::Segment>(generated.ordinary.front());
                cache.degree = 1;
                cache.controlPoints = {segment.start, segment.end};
                cache.knotslist = {0, 0, 1, 1};
                cache.weights = {1, 1};
            }
            const req::Request validationRequest{token,
                req::ValidateSourceCache{cache, definition, {}}};
            const auto validationReply = LC_OffsetWorkerKernel::run(validationRequest);
            const auto& validated = native(validationReply, 8);
            if (generated.ordinary.size() == 1 &&
                std::holds_alternative<RS_SplineData>(generated.ordinary.front())) {
                CHECK(validated.refusal == LC_OffsetCurveRefusal::None);
                CHECK(validated.changed);
            } else {
                CHECK(validated.refusal == LC_OffsetCurveRefusal::NotCacheRepresentable);
                CHECK_FALSE(validated.changed);
          }
        }
    }
}

TEST_CASE("closed mixed-bulge source promotes only the cache-representable side",
          "[lc-offsetcurve][worker-kernel][source-cache][polyline][mixed-bulge]") {
    RS_Polyline polyline;
    polyline.addVertex({0, 0}, 0.2);
    polyline.addVertex({8, 0}, -0.2);
    polyline.addVertex({8, 6});
    polyline.addVertex({0, 6});
    polyline.setClosed(true);
    polyline.endPolyline();

    const auto source = LC_OffsetSourceGeometry::capture(polyline);
    REQUIRE(source);
    REQUIRE(source->kind() == LC_OffsetSourceGeometry::Kind::Polyline);
    REQUIRE(source->isWellFormed());
    for (const double signedDistance : {-.25, .25}) {
        CAPTURE(signedDistance);
        const LC_OffsetSourceDefinition definition{*source, signedDistance, 1e-4, false};
        const req::Request generationRequest{token, req::GenerateSource{definition, {}}};
        const auto generationReply = LC_OffsetWorkerKernel::run(generationRequest);
        const auto& generated = native(generationReply, 7);
        REQUIRE(generated.engineStatus == LC_CurveOffsetStatus::Ok);
        REQUIRE(generated.ordinaryStatus == result::OrdinaryStatus::Complete);
        RS_SplineData cache;
        if (signedDistance < 0.0) {
            REQUIRE(generated.ordinary.size() == 1);
            REQUIRE(std::holds_alternative<RS_SplineData>(generated.ordinary.front()));
            cache = std::get<RS_SplineData>(generated.ordinary.front());
        } else {
            REQUIRE(generated.ordinary.size() > 1);
            if (const auto* spline =
                    std::get_if<RS_SplineData>(&generated.ordinary.front())) {
                cache = *spline;
            } else {
                const auto& segment = std::get<req::Segment>(generated.ordinary.front());
                cache.degree = 1;
                cache.controlPoints = {segment.start, segment.end};
                cache.knotslist = {0, 0, 1, 1};
                cache.weights = {1, 1};
            }
        }
        const req::Request validationRequest{token,
            req::ValidateSourceCache{cache, definition, {}}};
        const auto validationReply = LC_OffsetWorkerKernel::run(validationRequest);
        const auto& validated = native(validationReply, 8);
        if (signedDistance < 0.0) {
            CHECK(validated.refusal == LC_OffsetCurveRefusal::None);
            CHECK(validated.changed);
            CHECK(validated.engineRuns == 1);
        } else {
            CHECK(validated.refusal == LC_OffsetCurveRefusal::NotCacheRepresentable);
            CHECK_FALSE(validated.changed);
            CHECK(validated.engineRuns == 1);
        }
    }
}

TEST_CASE("polyline source validation rejects degenerate and inconsistent chains",
          "[lc-offsetcurve][worker-kernel][source-cache][polyline]") {
    const std::array<LC_OffsetPolylineData, 4> invalidVariants{
        LC_OffsetPolylineData{{0, 0}, {1, 0}, false,
            {RS_LineData{{0, 0}, {0, 0}}, RS_LineData{{0, 0}, {1, 0}}}},
        LC_OffsetPolylineData{{0, 0}, {2, 0}, false,
            {RS_LineData{{0, 0}, {1, 0}}, RS_LineData{{1.01, 0}, {2, 0}}}},
        LC_OffsetPolylineData{{0, 0}, {2, 0}, false,
            {RS_LineData{{0, 0}, {1, 0}}, RS_LineData{{1, 0}, {2, 0}},
             RS_LineData{{2, 0}, {2, 0}}}},
        LC_OffsetPolylineData{{0, 0}, {1, 0}, true,
            {RS_LineData{{0, 0}, {1, 0}}}}
    };

    for (const auto& data : invalidVariants) {
        const LC_OffsetSourceGeometry source{data};
        CHECK_FALSE(source.isWellFormed());
        CHECK_FALSE(source.makeEntity());
        const LC_OffsetSourceDefinition definition{source, .25, 1e-4, false};
        CHECK_FALSE(definition.isWellFormed());
    }
}

TEST_CASE("long mixed polyline cache promotion preserves segment order across offset sides",
          "[lc-offsetcurve][worker-kernel][source-cache][polyline]") {
    const double pi = std::acos(-1.0);
    const LC_OffsetPolylineData data{{0, 0}, {-1, 1}, false, {
        RS_LineData{{0, 0}, {2, 0}},
        RS_ArcData{{2, 1}, 1, -pi / 2, 0, false},
        RS_LineData{{3, 1}, {3, 3}},
        RS_ArcData{{2, 3}, 1, 0, pi / 2, false},
        RS_LineData{{2, 4}, {0, 4}},
        RS_ArcData{{0, 3}, 1, pi / 2, pi, false},
        RS_LineData{{-1, 3}, {-1, 1}}
    }};
    const LC_OffsetSourceGeometry source{data};
    REQUIRE(source.isWellFormed());
    for (const double signedDistance : {-.5, -.1, .1, .5}) {
        CAPTURE(signedDistance);
        const LC_OffsetSourceDefinition definition{source, signedDistance, 1e-4, false};
        const req::Request generationRequest{token, req::GenerateSource{definition, {}}};
        const auto generationReply = LC_OffsetWorkerKernel::run(generationRequest);
        const auto& generated = native(generationReply, 7);
        REQUIRE(generated.engineStatus == LC_CurveOffsetStatus::Ok);
        REQUIRE(generated.ordinaryStatus == result::OrdinaryStatus::Complete);
        REQUIRE_FALSE(generated.ordinary.empty());

        if (generated.ordinary.size() == 1 &&
            std::holds_alternative<RS_SplineData>(generated.ordinary.front())) {
            const auto& cache = std::get<RS_SplineData>(generated.ordinary.front());
            const req::Request validationRequest{token,
                req::ValidateSourceCache{cache, definition, {}}};
            const auto validationReply = LC_OffsetWorkerKernel::run(validationRequest);
            const auto& validated = native(validationReply, 8);
            CHECK(validated.refusal == LC_OffsetCurveRefusal::None);
            CHECK(validated.changed);
        } else {
            RS_SplineData cache;
            const auto& segment = std::get<req::Segment>(generated.ordinary.front());
            cache.degree = 1;
            cache.controlPoints = {segment.start, segment.end};
            cache.knotslist = {0, 0, 1, 1};
            cache.weights = {1, 1};
            const req::Request validationRequest{token,
                req::ValidateSourceCache{cache, definition, {}}};
            const auto validationReply = LC_OffsetWorkerKernel::run(validationRequest);
            const auto& validated = native(validationReply, 8);
            CHECK(validated.refusal == LC_OffsetCurveRefusal::NotCacheRepresentable);
            CHECK_FALSE(validated.changed);
        }
    }
}

TEST_CASE("spline and fit-point caches promote on both signed offset sides",
          "[lc-offsetcurve][worker-kernel][source-cache][spline-variant]") {
    RS_SplineData rationalQuadratic{2, false};
    rationalQuadratic.controlPoints = {{0, 0}, {1, 2}, {3, 0}};
    rationalQuadratic.knotslist = {0, 0, 0, 1, 1, 1};
    rationalQuadratic.weights = {1, .7, 1};
    RS_SplineData cubic{3, false};
    cubic.controlPoints = {{0, 0}, {1, 2}, {2, 2}, {3, 0}};
    cubic.knotslist = {0, 0, 0, 0, 1, 1, 1, 1};
    LC_SplinePointsData fitPoints{false, false};
    fitPoints.splinePoints = {{0, 0}, {1, 2}, {2, 0}};
    const std::array<LC_OffsetSourceGeometry, 3> sources{
        LC_OffsetSourceGeometry{rationalQuadratic},
        LC_OffsetSourceGeometry{cubic},
        LC_OffsetSourceGeometry{fitPoints}
    };
    for (const auto& source : sources) {
        CAPTURE(static_cast<int>(source.kind()));
        for (const double signedDistance : {-.25, .25}) {
            CAPTURE(signedDistance);
            const LC_OffsetSourceDefinition definition{source, signedDistance, 1e-4, false};
            const req::Request generationRequest{token, req::GenerateSource{definition, {}}};
            const auto generationReply = LC_OffsetWorkerKernel::run(generationRequest);
            const auto& generated = native(generationReply, 7);
            REQUIRE(generated.engineStatus == LC_CurveOffsetStatus::Ok);
            REQUIRE(generated.ordinaryStatus == result::OrdinaryStatus::Complete);
            REQUIRE(generated.ordinary.size() == 1);
            REQUIRE(std::holds_alternative<RS_SplineData>(generated.ordinary.front()));
            const auto& cache = std::get<RS_SplineData>(generated.ordinary.front());
            const req::Request validationRequest{token,
                req::ValidateSourceCache{cache, definition, {}}};
            const auto validationReply = LC_OffsetWorkerKernel::run(validationRequest);
            const auto& validated = native(validationReply, 8);
            CHECK(validated.refusal == LC_OffsetCurveRefusal::None);
            CHECK(validated.changed);
        }
    }
}

TEST_CASE("nonuniform rational spline knot layouts preserve exact cache validation",
          "[lc-offsetcurve][worker-kernel][source-cache][spline-variant]") {
    RS_SplineData quadratic{2, false};
    quadratic.controlPoints = {{0, 0}, {1, 2}, {2, -1}, {3, 2}, {4, 0}};
    quadratic.knotslist = {0, 0, 0, .3, .7, 1, 1, 1};
    quadratic.weights = {1, .6, 1.8, .8, 1};
    RS_SplineData cubic{3, false};
    cubic.controlPoints = {{0, 0}, {1, 2}, {2, 2}, {3, -1}, {4, -1}, {5, 1}};
    cubic.knotslist = {0, 0, 0, 0, .4, .4, 1, 1, 1, 1};
    cubic.weights = {1, .7, 1.4, .9, 1.6, 1};
    const std::array<LC_OffsetSourceGeometry, 2> sources{
        LC_OffsetSourceGeometry{quadratic}, LC_OffsetSourceGeometry{cubic}};

    for (const auto& source : sources) {
        CAPTURE(static_cast<int>(source.kind()));
        REQUIRE(source.isWellFormed());
        for (const double signedDistance : {-.2, .2}) {
            CAPTURE(signedDistance);
            const LC_OffsetSourceDefinition definition{source, signedDistance, 1e-4, false};
            const req::Request generationRequest{token, req::GenerateSource{definition, {}}};
            const auto generationReply = LC_OffsetWorkerKernel::run(generationRequest);
            const auto& generated = native(generationReply, 7);
            REQUIRE(generated.engineStatus == LC_CurveOffsetStatus::Ok);
            REQUIRE(generated.ordinaryStatus == result::OrdinaryStatus::Complete);
            REQUIRE_FALSE(generated.ordinary.empty());

            if (generated.ordinary.size() == 1 &&
                std::holds_alternative<RS_SplineData>(generated.ordinary.front())) {
                const auto& cache = std::get<RS_SplineData>(generated.ordinary.front());
                const req::Request validationRequest{token,
                    req::ValidateSourceCache{cache, definition, {}}};
                const auto validationReply = LC_OffsetWorkerKernel::run(validationRequest);
                const auto& validated = native(validationReply, 8);
                CHECK(validated.refusal == LC_OffsetCurveRefusal::None);
                CHECK(validated.changed);
            } else {
                const RS_SplineData cache = arcCache();
                const req::Request validationRequest{token,
                    req::ValidateSourceCache{cache, definition, {}}};
                const auto validationReply = LC_OffsetWorkerKernel::run(validationRequest);
                const auto& validated = native(validationReply, 8);
                CHECK(validated.refusal == LC_OffsetCurveRefusal::NotCacheRepresentable);
                CHECK_FALSE(validated.changed);
            }
        }
    }
}

TEST_CASE("smooth internal-knot rational spline produces an exact-promotable cache",
          "[lc-offsetcurve][worker-kernel][source-cache][spline-variant]") {
    RS_SplineData rational{2, false};
    rational.controlPoints = {{0, 0}, {1, .25}, {3, .25}, {4, 0}};
    rational.knotslist = {0, 0, 0, .5, 1, 1, 1};
    rational.weights = {1, .9, 1.1, 1};
    const LC_OffsetSourceDefinition definition{
        LC_OffsetSourceGeometry{rational}, .05, 1e-4, false};
    REQUIRE(definition.isWellFormed());

    const req::Request generationRequest{token, req::GenerateSource{definition, {}}};
    const auto generationReply = LC_OffsetWorkerKernel::run(generationRequest);
    const auto& generated = native(generationReply, 7);
    REQUIRE(generated.engineStatus == LC_CurveOffsetStatus::Ok);
    REQUIRE(generated.ordinaryStatus == result::OrdinaryStatus::Complete);
    REQUIRE(generated.ordinary.size() == 1);
    REQUIRE(std::holds_alternative<RS_SplineData>(generated.ordinary.front()));

    const auto& cache = std::get<RS_SplineData>(generated.ordinary.front());
    const req::Request validationRequest{token,
        req::ValidateSourceCache{cache, definition, {}}};
    const auto validationReply = LC_OffsetWorkerKernel::run(validationRequest);
    const auto& validated = native(validationReply, 8);
    CHECK(validated.refusal == LC_OffsetCurveRefusal::None);
    CHECK(validated.changed);
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


namespace {
req::Produce seriesInput(const RS_Entity& source, bool native = false) {
    req::Produce op;
    if (const auto* ellipse = dynamic_cast<const RS_Ellipse*>(&source)) op.source = ellipse->getData();
    else op.source = dynamic_cast<const RS_Spline&>(source).getData();
    if (source.getMin().valid) op.minimum = source.getMin();
    if (source.getMax().valid) op.maximum = source.getMax();
    op.context.topLevel = true; op.coordinate = {3, 2}; op.distance = .25;
    op.copies = 2; op.asOffsetCurve = native;
    return op;
}
void sameProducedWork(const result::Produced& value, const LC_OffsetSourceOutcome& expected) {
    const auto& s = value.source;
    CHECK(s.status == expected.status); CHECK(s.engineStatus == expected.engineStatus); CHECK(s.detail == expected.detail);
    CHECK(s.stoppedBy == expected.stoppedBy); CHECK(s.copiesMade == expected.copiesMade);
    CHECK(s.copiesRequested == expected.copiesRequested); CHECK(s.sourceRemoved == expected.sourceRemoved);
    CHECK(s.copiesKeptPlain == expected.copiesKeptPlain); CHECK(s.resolvedSide == expected.resolvedSide);
    CHECK(value.copyOutputCounts == expected.copyOutputCounts);
    CHECK(s.rebaseEngineRuns == expected.rebaseEngineRuns); CHECK(s.cacheEngineRuns == expected.cacheEngineRuns);
    CHECK(s.reusedCacheCopies == expected.reusedCacheCopies); CHECK(s.sampleUnits == expected.sampleUnits);
    CHECK(s.intersectionPairAllowance == expected.intersectionPairAllowance);
    CHECK(s.adoptionAttempts == expected.adoptionAttempts); work(s.adoptionCheckWork, expected.adoptionCheckWork);
    CHECK(s.usage.cubicPieces == expected.usage.cubicPieces); CHECK(s.usage.outputEntities == expected.usage.outputEntities);
    CHECK(s.usage.deepEntities == expected.usage.deepEntities);
    CHECK(s.attemptedOutputUsage.cubicPieces == expected.attemptedOutputUsage.cubicPieces);
    CHECK(s.attemptedOutputUsage.outputEntities == expected.attemptedOutputUsage.outputEntities);
    CHECK(s.attemptedOutputUsage.deepEntities == expected.attemptedOutputUsage.deepEntities);
    CHECK(s.attemptedDeepCountComplete == expected.attemptedDeepCountComplete);
    REQUIRE(value.keptPlain.size() == expected.keptPlain.size());
    for (std::size_t i = 0; i < value.keptPlain.size(); ++i) {
        CHECK(value.keptPlain[i].copy == expected.keptPlain[i].copy);
        CHECK(value.keptPlain[i].reason == expected.keptPlain[i].reason);
        CHECK(value.keptPlain[i].checkReason == expected.keptPlain[i].checkReason);
    }
    REQUIRE(value.rebases.size() == expected.rebases.size());
    for (std::size_t i = 0; i < value.rebases.size(); ++i) {
        const auto& a = value.rebases[i]; const auto& b = expected.rebases[i];
        CHECK(a.copy == b.copy); CHECK(a.proposalRefusal == b.proposalRefusal); CHECK(a.nativeRefusal == b.nativeRefusal);
        CHECK(a.engineStatus == b.engineStatus); CHECK(a.checkReason == b.checkReason); work(a.checkWork, b.checkWork);
        CHECK(a.engineRuns == b.engineRuns); CHECK(a.sampleUnits == b.sampleUnits);
        CHECK(a.intersectionPairAllowance == b.intersectionPairAllowance); CHECK(a.adopted == b.adopted);
    }
}
result::Produced seriesParity(RS_Entity& source, const req::Produce& op) {
    RS_OffsetData data; data.coord = op.coordinate; data.sideFallback = op.fallback.value_or(RS_Vector{false});
    data.distance = op.distance; data.number = op.copies; data.multipleCopies = true; data.keepOriginals = true;
    data.asOffsetCurve = op.asOffsetCurve;
    LC_OffsetSourceOutcome expected;
    std::vector<std::unique_ptr<RS_Entity>> roots;
    expected.status = RS_Modification::prepareOffsetSource(data, source, op.preview, op.limits,
        op.limits.maxDeepEntitiesPerRequest, op.context, roots, expected);
    const req::Request input{token, op}; const auto before = req::encode(input); REQUIRE(before);
    const auto reply = LC_OffsetWorkerKernel::run(input);
    REQUIRE(reply.failure == result::Failure::None); REQUIRE(std::holds_alternative<result::Produced>(reply.result));
    const auto& value = std::get<result::Produced>(reply.result);
    sameProducedWork(value, expected); CHECK(req::encode(input) == before);
    if (value.outputStatus == result::OrdinaryStatus::Complete) {
        REQUIRE(value.outputs.size() == roots.size());
        for (std::size_t i = 0; i < roots.size(); ++i) {
            const auto& output = value.outputs[i];
            if (const auto* curve = std::get_if<result::Curve>(&output)) {
                const auto* native = dynamic_cast<const LC_OffsetCurve*>(roots[i].get()); REQUIRE(native);
                CHECK(req::Values::encodeCache(curve->cache) == req::Values::encodeCache(native->getData()));
                CHECK(req::Values::encodeDefinition(curve->definition) == req::Values::encodeDefinition(native->definition()));
                CHECK(curve->worldTolerance == native->checkedWorldTolerance());
                CHECK(curve->closed == native->cacheFormsClosedChain());
                CHECK(curve->grips == native->cachedDefinitionGrips()); CHECK(curve->midpoint == native->getMiddlePoint());
            } else if (const auto* line = std::get_if<req::Segment>(&output)) {
                REQUIRE(dynamic_cast<const RS_Line*>(roots[i].get()));
                CHECK(line->start == roots[i]->getStartpoint()); CHECK(line->end == roots[i]->getEndpoint());
            } else {
                const auto* spline = dynamic_cast<const RS_Spline*>(roots[i].get()); REQUIRE(spline);
                CHECK_FALSE(dynamic_cast<const LC_OffsetCurve*>(roots[i].get()));
                CHECK(req::Values::encodeCache(std::get<RS_SplineData>(output)) == req::Values::encodeCache(spline->getData()));
            }
        }
    } else CHECK(value.outputs.empty());
    const auto encoded = result::encode(reply, input); REQUIRE(encoded);
    const auto decoded = result::decode(*encoded, input); REQUIRE(decoded);
    CHECK(result::encode(*decoded, input) == encoded);
    return value;
}
}

TEST_CASE("Produce kernel matches shared ellipse source series and records actual adoption C work",
          "[lc-offsetcurve][worker-kernel][produce]") {
    RS_Ellipse source{nullptr, RS_EllipseData{{0, 0}, {1, 0}, 1, 0, .1, false}};
    for (bool native : {false, true}) for (bool preview : {false, true}) {
        auto op = seriesInput(source, native); op.preview = preview;
        const auto value = seriesParity(source, op);
        REQUIRE(value.source.status == LC_OffsetSourceStatus::Succeeded);
        REQUIRE(value.outputStatus == result::OrdinaryStatus::Complete);
        CHECK(value.copyOutputCounts == std::vector<std::size_t>{1, 1});
        if (native && !preview) {
            CHECK(value.source.adoptionAttempts == 2);
            geo::CheckWork actual;
            for (const auto& output : value.outputs) {
                REQUIRE(std::holds_alternative<result::Curve>(output));
                const auto& c = std::get<result::Curve>(output);
                const auto checked = geo::checkCache(c.cache, c.definition, c.worldTolerance); REQUIRE(checked);
                actual.cacheSpans += checked.work.cacheSpans; actual.vertices += checked.work.vertices;
                actual.distancePairs += checked.work.distancePairs; actual.orderingComparisons += checked.work.orderingComparisons;
                actual.deepest = std::max(actual.deepest, checked.work.deepest);
            }
            work(value.source.adoptionCheckWork, actual); CHECK(actual.vertices > 0);
        } else {
            CHECK(value.source.adoptionAttempts == 0); work(value.source.adoptionCheckWork, {});
            for (const auto& output : value.outputs) CHECK_FALSE(std::holds_alternative<result::Curve>(output));
        }
    }
    auto op = seriesInput(source, true); op.context.topLevel = false;
    const auto plain = seriesParity(source, op);
    CHECK(plain.source.adoptionAttempts == 0); CHECK(plain.source.copiesKeptPlain == plain.source.copiesMade);
}

TEST_CASE("Produce kernel repeats actual stored caches using raw definition context without native input shells",
          "[lc-offsetcurve][worker-kernel][produce]") {
    RS_Ellipse original{nullptr, RS_EllipseData{{0, 0}, {1, 0}, 1, 0, .1, false}};
    auto made = LC_OffsetCurve::create(original, 1, false); REQUIRE(made);
    for (bool native : {false, true}) for (bool preview : {false, true}) {
        auto op = seriesInput(*made.entity, native); op.preview = preview;
        op.context.nativeSource = true; op.context.nativeDefinition = made.entity->definition();
        const auto value = seriesParity(*made.entity, op);
        REQUIRE(value.source.status == LC_OffsetSourceStatus::Succeeded);
        CHECK(value.source.adoptionAttempts == 0);
        if (!native || preview) {
            CHECK(value.rebases.empty()); CHECK(value.source.rebaseEngineRuns == 0);
            for (const auto& output : value.outputs) CHECK_FALSE(std::holds_alternative<result::Curve>(output));
        } else {
            REQUIRE(value.rebases.size() == 2);
            CHECK(value.source.rebaseEngineRuns == 2); CHECK(value.source.cacheEngineRuns == 0);
        }
    }
    auto op = seriesInput(*made.entity, true);
    op.context.nativeSource = true; op.context.nativeDefinition = made.entity->definition();
    op.coordinate = {0, 0}; op.distance = 1; op.copies = 1;
    const auto zero = seriesParity(*made.entity, op);
    REQUIRE(zero.rebases.size() == 1); CHECK(zero.rebases[0].proposalRefusal == LC_OffsetCurveAgain::Refusal::ZeroSum);
    CHECK(zero.source.rebaseEngineRuns == 0); CHECK(zero.source.cacheEngineRuns == 1);
    auto ineligible = seriesInput(*made.entity, true); ineligible.copies = 1;
    ineligible.context.nativeSource = true;
    const auto undefined = seriesParity(*made.entity, ineligible);
    REQUIRE(undefined.keptPlain.size() == 1);
    CHECK(undefined.keptPlain[0].reason == LC_OffsetPlainReason::UndefinedDefinition);
    ineligible.context.nativeDefinition = made.entity->definition(); ineligible.context.topLevel = false;
    const auto nested = seriesParity(*made.entity, ineligible);
    REQUIRE(nested.keptPlain.size() == 1); CHECK(nested.keptPlain[0].reason == LC_OffsetPlainReason::NotTopLevel);
    for (const auto& output : nested.outputs) CHECK_FALSE(std::holds_alternative<result::Curve>(output));
    op.coordinate = {3, 2}; op.distance = .25; op.limits.maxSamples = 1;
    const auto refused = seriesParity(*made.entity, op);
    CHECK(refused.source.status == LC_OffsetSourceStatus::LimitExceeded);
    CHECK(refused.copyOutputCounts.empty()); CHECK(refused.outputs.empty());
    CHECK(refused.outputStatus == result::OrdinaryStatus::None);
    CHECK(refused.source.rebaseEngineRuns == 1); CHECK(refused.source.sampleUnits > 1);
}

TEST_CASE("Produce kernel preserves real vanished prefixes and aggregate cap unsupported receipts",
          "[lc-offsetcurve][worker-kernel][produce]") {
    RS_Ellipse circle{nullptr, RS_EllipseData{{0, 0}, {5, 0}, 1}};
    auto op = seriesInput(circle); op.coordinate = {0, 0}; op.distance = 2; op.copies = 3;
    const auto prefix = seriesParity(circle, op);
    REQUIRE(prefix.source.status == LC_OffsetSourceStatus::Succeeded);
    CHECK(prefix.source.copiesMade == 2); CHECK(prefix.source.stoppedBy == LC_OffsetSourceStatus::Vanished);
    CHECK(prefix.copyOutputCounts == std::vector<std::size_t>{1, 1});
    op.distance = 1; op.copies = 1;
    const auto baseline = seriesParity(circle, op);
    REQUIRE(baseline.outputStatus == result::OrdinaryStatus::Complete);
    op.limits.perSource.maxCubicPieces = baseline.source.usage.cubicPieces;
    const auto seam = seriesParity(circle, op);
    REQUIRE(seam.source.status == LC_OffsetSourceStatus::Succeeded);
    CHECK(seam.source.usage.cubicPieces == baseline.source.usage.cubicPieces);
    CHECK(seam.outputStatus == result::OrdinaryStatus::Unsupported); CHECK(seam.outputs.empty());
    CHECK(seam.copyOutputCounts == baseline.copyOutputCounts);
}

TEST_CASE("Produce raw ellipse adapters preserve captured bounds and do not repair source angles",
          "[lc-offsetcurve][worker-kernel][produce][raw]") {
    class RawSource final : public RS_Ellipse {
    public:
        RawSource(const RS_EllipseData& d, const RS_Vector& lo, const RS_Vector& hi) {
            m_data = d; m_minV = lo; m_maxV = hi;
        }
    };
    RS_Ellipse normal{nullptr, RS_EllipseData{{0, 0}, {1, 0}, 1, 0, .1, false}};
    // Captured bounds are values, not a request to calculateBorders() and repair raw data.
    RawSource captured{normal.getData(), {-1e9, -1e9}, {1e9, 1e9}};
    auto op = seriesInput(captured); op.copies = 1; op.coordinate = {1.1, .05}; op.fallback = RS_Vector{0, 0};
    const auto fallback = seriesParity(captured, op);
    REQUIRE(fallback.source.resolvedSide);
    CHECK(*fallback.source.resolvedSide == LC_CurveOffsetSide::Left);
    op.distance = -.25; seriesParity(captured, op);
    auto tiny = normal.getData(); tiny.angle1 = 0; tiny.angle2 = 1e-12; tiny.isArc = true;
    RawSource unmodified{tiny, normal.getMin(), normal.getMax()};
    op = seriesInput(unmodified); op.copies = 1;
    seriesParity(unmodified, op); // Compare the raw source, not its constructor-normalized whole ellipse.
}

namespace {
result::Produced cachedSeries(const req::CachedProduce& op) {
    const req::Request input{token, op};
    const auto request = req::encode(input); REQUIRE(request);
    const auto reply = LC_OffsetWorkerKernel::run(input);
    INFO("cached Produce failure=" << int(reply.failure));
    REQUIRE(reply.failure == result::Failure::None);
    REQUIRE(std::holds_alternative<result::Produced>(reply.result));
    const auto wire = result::encode(reply, input); REQUIRE(wire);
    const auto decoded = result::decode(*wire, input); REQUIRE(decoded);
    CHECK(result::encode(*decoded, input) == wire);
    CHECK(req::encode(input) == request);
    return std::get<result::Produced>(decoded->result);
}
}

TEST_CASE("Cached Produce reuses per-copy geometry with zero producer effort and preserves side and eviction",
          "[lc-offsetcurve][worker-kernel][preview-portable]") {
    RS_Ellipse source{nullptr, RS_EllipseData{{0, 0}, {1, 0}, 1, 0, .1, false}};
    req::CachedProduce op{seriesInput(source), {}}; op.produce.preview = true;
    auto first = cachedSeries(op);
    REQUIRE(first.source.status == LC_OffsetSourceStatus::Succeeded);
    CHECK(first.source.cacheEngineRuns == 2); REQUIRE(first.previewCache);
    CHECK(first.previewCache->entries.size() == 2);
    op.cache = *first.previewCache;
    op.produce.limits.maxSamples = 1; op.produce.limits.maxIntersectionPairs = 1;
    const auto reused = cachedSeries(op);
    CHECK(reused.source.status == LC_OffsetSourceStatus::Succeeded);
    CHECK(reused.source.cacheEngineRuns == 0); CHECK(reused.source.sampleUnits == 0);
    CHECK(reused.source.intersectionPairAllowance == 0); CHECK(reused.source.reusedCacheCopies == 2);
    REQUIRE(reused.reusedCopies.size() == 2);
    CHECK(reused.reusedCopies[0].copy == 1); CHECK(reused.reusedCopies[1].copy == 2);
    CHECK(reused.previewCache->revision == op.cache.revision);
    op.produce.limits = {}; op.produce.coordinate = {0, 0};
    const auto opposite = cachedSeries(op);
    REQUIRE(opposite.source.resolvedSide); REQUIRE(first.source.resolvedSide);
    CHECK(opposite.source.resolvedSide != first.source.resolvedSide);
    CHECK(opposite.source.reusedCacheCopies == 0); CHECK(opposite.source.cacheEngineRuns == 2);
    op.produce.coordinate = {3, 2}; op.produce.distance = .5;
    op.cache.otherEntries = 254;
    const auto evicted = cachedSeries(op);
    CHECK(evicted.source.reusedCacheCopies == 1); CHECK(evicted.source.cacheEngineRuns == 1);
    REQUIRE(evicted.previewCache); CHECK(evicted.previewCache->cleared);
    CHECK(evicted.previewCache->otherEntries == 0); CHECK(evicted.previewCache->entries.size() == 1);
}

TEST_CASE("Cached Produce refusal budgets grow correctly and cached ellipse adoption still runs C",
          "[lc-offsetcurve][worker-kernel][preview-portable]") {
    RS_Ellipse source{nullptr, RS_EllipseData{{0, 0}, {1, 0}, 1, 0, .1, false}};
    req::CachedProduce op{seriesInput(source), {}}; op.produce.preview = true; op.produce.copies = 1;
    const auto initial = cachedSeries(op); REQUIRE(initial.previewCache);
    op.cache = *initial.previewCache;
    op.produce.preview = false; op.produce.asOffsetCurve = true;
    const auto native = cachedSeries(op);
    REQUIRE(native.source.status == LC_OffsetSourceStatus::Succeeded);
    CHECK(native.source.reusedCacheCopies == 1); CHECK(native.source.cacheEngineRuns == 0);
    CHECK(native.source.adoptionAttempts == 1); CHECK(native.source.adoptionCheckWork.vertices > 0);
    REQUIRE(native.outputs.size() == 1); REQUIRE(std::holds_alternative<result::Curve>(native.outputs[0]));
    auto& shape = op.cache.entries[0].geometry[0];
    if (auto* line = std::get_if<RS_LineData>(&shape)) line->endpoint.x += 100;
    else std::get<RS_SplineData>(shape).controlPoints[0].x += 100;
    const auto unchecked = cachedSeries(op);
    CHECK(unchecked.source.adoptionAttempts == 1);
    REQUIRE(unchecked.outputs.size() == 1); CHECK_FALSE(std::holds_alternative<result::Curve>(unchecked.outputs[0]));
    REQUIRE(unchecked.keptPlain.size() == 1);
    CHECK(unchecked.keptPlain[0].reason == LC_OffsetPlainReason::CheckFailed);
    auto& e = op.cache.entries[0]; e.geometry.clear(); e.usage = {};
    e.status = LC_CurveOffsetStatus::LimitExceeded; e.budget = {1, 1, 1};
    op.produce.asOffsetCurve = false; op.produce.preview = true; op.produce.limits.perSource = e.budget;
    const auto refused = cachedSeries(op);
    CHECK(refused.source.status == LC_OffsetSourceStatus::LimitExceeded);
    CHECK(refused.source.cacheEngineRuns == 0); CHECK(refused.source.reusedCacheCopies == 1);
    op.produce.limits = {};
    const auto grown = cachedSeries(op);
    CHECK(grown.source.status == LC_OffsetSourceStatus::Succeeded);
    CHECK(grown.source.cacheEngineRuns == 1); CHECK(grown.source.reusedCacheCopies == 0);
}

TEST_CASE("Cached Produce native zero-sum repeats use the distinct stored-cache fallback",
          "[lc-offsetcurve][worker-kernel][preview-portable]") {
    RS_Ellipse original{nullptr, RS_EllipseData{{0, 0}, {1, 0}, 1, 0, .1, false}};
    auto source = LC_OffsetCurve::create(original, 1, false); REQUIRE(source);
    auto input = seriesInput(*source.entity, true); input.context.nativeSource = true;
    input.context.nativeDefinition = source.entity->definition();
    input.coordinate = {0, 0}; input.distance = 1; input.copies = 1; input.preview = true;
    req::CachedProduce op{input, {}};
    const auto preview = cachedSeries(op); REQUIRE(preview.previewCache);
    op.cache = *preview.previewCache; op.produce.preview = false;
    const auto fallback = cachedSeries(op);
    REQUIRE(fallback.source.status == LC_OffsetSourceStatus::Succeeded);
    CHECK(fallback.source.reusedCacheCopies == 1); CHECK(fallback.source.cacheEngineRuns == 0);
    CHECK(fallback.source.rebaseEngineRuns == 0); CHECK(fallback.source.sampleUnits == 0);
    REQUIRE(fallback.rebases.size() == 1);
    CHECK(fallback.rebases[0].proposalRefusal == LC_OffsetCurveAgain::Refusal::ZeroSum);
    REQUIRE(fallback.keptPlain.size() == 1); CHECK(fallback.keptPlain[0].reason == LC_OffsetPlainReason::ZeroSum);
    for (const auto& output : fallback.outputs) CHECK_FALSE(std::holds_alternative<result::Curve>(output));
}

TEST_CASE("Cached Produce transports real cubic spline records without geometry repair",
          "[lc-offsetcurve][worker-kernel][preview-portable]") {
    RS_Ellipse source{nullptr, RS_EllipseData{{0, 0}, {3, 0}, .7, 0, 1.5, false}};
    req::CachedProduce op{seriesInput(source), {}}; op.produce.copies = 1; op.produce.preview = true;
    op.produce.coordinate = {8, 8};
    const auto first = cachedSeries(op); REQUIRE(first.source.status == LC_OffsetSourceStatus::Succeeded);
    REQUIRE(first.previewCache); REQUIRE(first.previewCache->entries.size() == 1);
    REQUIRE_FALSE(first.previewCache->entries[0].geometry.empty());
    bool spline = false;
    for (const auto& value : first.previewCache->entries[0].geometry) spline |= std::holds_alternative<RS_SplineData>(value);
    REQUIRE(spline);
    op.cache = *first.previewCache; op.produce.limits.maxSamples = 1;
    const auto reused = cachedSeries(op);
    CHECK(reused.source.status == LC_OffsetSourceStatus::Succeeded);
    CHECK(reused.source.cacheEngineRuns == 0); CHECK(reused.source.reusedCacheCopies == 1);
    REQUIRE(reused.outputs.size() == first.outputs.size());
    for (std::size_t i = 0; i < reused.outputs.size(); ++i) {
        const auto& a = std::get<RS_SplineData>(reused.outputs[i]);
        const auto& b = std::get<RS_SplineData>(first.outputs[i]);
        CHECK(req::Values::encodeCache(a) == req::Values::encodeCache(b));
    }
}
