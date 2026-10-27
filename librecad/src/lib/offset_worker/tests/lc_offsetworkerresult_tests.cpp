/*
 * This file is part of LibreCAD, a 2D CAD program.
 * Copyright (C) 2026 LibreCAD.org
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include <catch2/catch_test_macros.hpp>

#include "../lc_offsetworkerresult.h"
#include "lc_offsetworkerwire.h"
#include "rs_arc.h"
#include "rs_circle.h"
#include "rs_ellipse.h"
#include "rs_line.h"

#include <QCborMap>
#include <QCborValue>
#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <initializer_list>
#include <limits>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

namespace {
namespace Req = LC_OffsetWorkerRequest;
namespace Reply = LC_OffsetWorkerResult;
namespace Trim = LC_OffsetCacheTrim;
namespace Geo = LC_EllipseOffsetGeometry;
namespace Wire = LC_OffsetWorkerWire;
using Refusal = LC_OffsetCurveRefusal;
using Reason = LC_OffsetCheckResult::Reason;
using Status = LC_CurveOffsetStatus;
constexpr const char* tags = "[lc-offsetcurve][worker-protocol][reply]";
constexpr double pi = 3.14159265358979323846;

void same(double a, double b) { CHECK(std::memcmp(&a, &b, sizeof(double)) == 0); }
void same(const RS_Vector& a, const RS_Vector& b) {
    same(a.x, b.x); same(a.y, b.y); same(a.z, b.z); CHECK(a.valid == b.valid);
}
void same(const Trim::Section&, const Trim::Section&);
void same(const Trim::Fragment&, const Trim::Fragment&);
void same(const Trim::RootProvenance&, const Trim::RootProvenance&);
void same(const Trim::Result::ConicFragment&, const Trim::Result::ConicFragment&);
template<class T> void sameValues(const std::vector<T>& a, const std::vector<T>& b) {
    REQUIRE(a.size() == b.size());
    for (std::size_t i = 0; i < a.size(); ++i) same(a[i], b[i]);
}
void same(const RS_EllipseData& a, const RS_EllipseData& b) {
    same(a.center, b.center); same(a.majorP, b.majorP); same(a.ratio, b.ratio);
    same(a.angle1, b.angle1); same(a.angle2, b.angle2); CHECK(a.reversed == b.reversed);
    CHECK(a.isArc == b.isArc); same(a.angleDegrees, b.angleDegrees);
    same(a.startAngleDegrees, b.startAngleDegrees); same(a.otherAngleDegrees, b.otherAngleDegrees);
    same(a.angularLength, b.angularLength);
}
void same(const RS_SplineData& a, const RS_SplineData& b) {
    sameValues(a.controlPoints, b.controlPoints); sameValues(a.knotslist, b.knotslist);
    sameValues(a.weights, b.weights); sameValues(a.fitPoints, b.fitPoints);
    CHECK(a.degree == b.degree); CHECK(a.type == b.type); CHECK(a.savedOpenType == b.savedOpenType);
    sameValues(a.savedOpenKnots, b.savedOpenKnots);
    CHECK(a.m_closedFlag == b.m_closedFlag); CHECK(a.m_periodicFlag == b.m_periodicFlag);
}
void same(const LC_EllipseOffsetDefinition& a, const LC_EllipseOffsetDefinition& b) {
    same(a.basis, b.basis); same(a.distance, b.distance); same(a.relTolerance, b.relTolerance);
    CHECK(a.towardCentre == b.towardCentre); CHECK(a.cacheReversed == b.cacheReversed);
}
void same(const Geo::CheckWork& a, const Geo::CheckWork& b) {
    CHECK(a.cacheSpans == b.cacheSpans); CHECK(a.vertices == b.vertices);
    CHECK(a.distancePairs == b.distancePairs); CHECK(a.deepest == b.deepest);
    CHECK(a.orderingComparisons == b.orderingComparisons);
}
void same(const LC_OffsetOutputUsage& a, const LC_OffsetOutputUsage& b) {
    CHECK(a.cubicPieces == b.cubicPieces); CHECK(a.outputEntities == b.outputEntities);
    CHECK(a.deepEntities == b.deepEntities);
}
void same(const Reply::Curve& a, const Reply::Curve& b) {
    same(a.cache, b.cache); same(a.definition, b.definition); same(a.worldTolerance, b.worldTolerance);
    CHECK(a.closed == b.closed); sameValues(a.grips, b.grips); same(a.midpoint, b.midpoint);
}
void same(const Reply::Native& a, const Reply::Native& b) {
    CHECK(a.refusal == b.refusal); CHECK(a.engineStatus == b.engineStatus); CHECK(a.checkReason == b.checkReason);
    same(a.checkWork, b.checkWork); same(a.throughWork, b.throughWork);
    CHECK(a.engineRuns == b.engineRuns); CHECK(a.changed == b.changed);
    CHECK(a.curve.has_value() == b.curve.has_value()); if (a.curve && b.curve) same(*a.curve, *b.curve);
    same(a.attemptedOutputUsage, b.attemptedOutputUsage);
    CHECK(a.attemptedDeepCountComplete == b.attemptedDeepCountComplete);
    same(a.outputUsage, b.outputUsage); CHECK(a.sampleUnits == b.sampleUnits);
}
void same(const LC_Interval& a, const LC_Interval& b) { same(a.lo(), b.lo()); same(a.hi(), b.hi()); }
void same(const Trim::Location& a, const Trim::Location& b) {
    CHECK(a.rawSpan == b.rawSpan); same(a.localParameter, b.localParameter);
}
void same(const Trim::Endpoint& a, const Trim::Endpoint& b) {
    CHECK(a.labels == b.labels); same(a.source, b.source); CHECK(a.rootId == b.rootId);
    same(a.imageX, b.imageX); same(a.imageY, b.imageY);
}
void same(const Trim::Section& a, const Trim::Section& b) {
    CHECK(a.rawSpan == b.rawSpan); same(a.from, b.from); same(a.to, b.to); same(a.originalDomain, b.originalDomain);
}
void same(const LC_IntersectionDomain& a, const LC_IntersectionDomain& b) {
    CHECK(a.segmentA == b.segmentA); CHECK(a.segmentB == b.segmentB);
    same(a.parameterA, b.parameterA); same(a.parameterB, b.parameterB);
}
void same(const Trim::RootProvenance& a, const Trim::RootProvenance& b) {
    CHECK(a.id == b.id); same(a.target, b.target); same(a.isolation, b.isolation);
    same(a.proofDomain, b.proofDomain); CHECK(a.proof == b.proof); same(a.worldX, b.worldX); same(a.worldY, b.worldY);
}
void same(const Trim::Result::ConicFragment& a, const Trim::Result::ConicFragment& b) {
    same(a.data, b.data); same(a.from, b.from); same(a.to, b.to);
    CHECK(a.startRoot == b.startRoot); CHECK(a.endRoot == b.endRoot);
    CHECK(a.crossesStorageSeam == b.crossesStorageSeam);
}
void same(const Trim::Fragment& a, const Trim::Fragment& b) {
    same(a.data, b.data); sameValues(a.sourceSections, b.sourceSections); same(a.start, b.start); same(a.end, b.end);
}
void same(const Trim::RemovedInterval& a, const Trim::RemovedInterval& b) {
    same(a.from, b.from); same(a.to, b.to); CHECK(a.crossesStorageSeam == b.crossesStorageSeam);
}
void same(const LC_IntersectionWorkStats& a, const LC_IntersectionWorkStats& b) {
    CHECK(a.candidateBoxPairs == b.candidateBoxPairs); CHECK(a.refinementBoxPairs == b.refinementBoxPairs);
    CHECK(a.certificateBoxPairs == b.certificateBoxPairs); CHECK(a.totalBoxPairs == b.totalBoxPairs);
    CHECK(a.generatedLeaves == b.generatedLeaves); CHECK(a.generatedItems == b.generatedItems);
    CHECK(a.deepestPreparation == b.deepestPreparation); CHECK(a.deepestPairRefinement == b.deepestPairRefinement);
    CHECK(a.deepestOrderingRefinement == b.deepestOrderingRefinement); CHECK(a.deepestRootRefinement == b.deepestRootRefinement);
    CHECK(a.orderingComparisons == b.orderingComparisons); CHECK(a.validationSegmentVisits == b.validationSegmentVisits);
    CHECK(a.validationNextCalls == b.validationNextCalls);
}
void same(const Trim::Work& a, const Trim::Work& b) {
    CHECK(a.inputSpans == b.inputSpans); CHECK(a.inputRecords == b.inputRecords); CHECK(a.rootRecords == b.rootRecords);
    CHECK(a.kernelItems == b.kernelItems); CHECK(a.kernelComparisons == b.kernelComparisons);
    CHECK(a.outputSpans == b.outputSpans); CHECK(a.outputControls == b.outputControls); CHECK(a.outputKnots == b.outputKnots);
    same(a.intersections, b.intersections);
}
void same(const Trim::Result& a, const Trim::Result& b) {
    CHECK(a.refusal == b.refusal); sameValues(a.fragments, b.fragments); sameValues(a.conicFragments, b.conicFragments);
    sameValues(a.roots, b.roots); CHECK(a.removed.has_value() == b.removed.has_value());
    if (a.removed && b.removed) same(*a.removed, *b.removed);
    CHECK(a.rootCount == b.rootCount); CHECK(a.fragmentCount == b.fragmentCount); same(a.work, b.work);
}
void same(const Reply::Reply& a, const Reply::Reply& b) {
    CHECK(a.token.requestId == b.token.requestId); CHECK(a.token.generation == b.token.generation);
    CHECK(a.token.sourceRevision == b.token.sourceRevision); CHECK(a.token.undoEpoch == b.token.undoEpoch);
    CHECK(a.operation == b.operation); CHECK(a.failure == b.failure); REQUIRE(a.result.index() == b.result.index());
    if (const auto* n = std::get_if<Reply::Native>(&a.result)) same(*n, std::get<Reply::Native>(b.result));
    if (const auto* t = std::get_if<Trim::Result>(&a.result)) same(*t, std::get<Trim::Result>(b.result));
}

RS_SplineData cache(std::vector<RS_Vector> points = {{-0.0, -0.0}, {10, 0}}, std::size_t degree = 1,
                    double from = 0, double step = 1) {
    RS_SplineData d;
    d.degree = degree; d.controlPoints = std::move(points); d.weights.assign(d.controlPoints.size(), 1);
    const auto spans = (d.controlPoints.size() - 1) / degree;
    for (std::size_t k = 0; k <= spans; ++k)
        for (std::size_t j = 0; j < (k == 0 || k == spans ? degree + 1 : degree); ++j)
            d.knotslist.push_back(from + step * double(k));
    return d;
}
LC_EllipseOffsetDefinition definition() {
    return {RS_EllipseData{{-0.0, 0}, {2, -0.0}, .5, .25, 2.75, true}, .25, false, true, .0001};
}
Req::Token token() { return {std::numeric_limits<qint64>::max(), 17, 23, 29}; }
Req::Request nativeRequest(bool validate = false) {
    if (validate) return {token(), Req::ValidateCache{cache(), definition(), .001, {}}};
    return {token(), Req::Generate{definition(), {}, {}}};
}
Reply::Native nativeValue(bool validate = false) {
    // Protocol value fixtures only. No certificate or editable native entity
    // is made by constructing or successfully decoding these values.
    Reply::Native n;
    n.refusal = Refusal::None; n.checkReason = Reason::None; n.changed = true;
    n.checkWork = {1, 13, 17, 2, 19};
    n.curve = Reply::Curve{cache(), definition(), .001, false, {{-0.0, 0}, {2, 3}}, {5, -0.0}};
    if (!validate) {
        n.engineRuns = 1; n.engineStatus = Status::Ok;
        n.attemptedOutputUsage = {1, 1, 32}; n.outputUsage = {1, 1, 33}; n.sampleUnits = 101;
    }
    return n;
}
Reply::Reply reply(const Req::Request& r, Reply::Native n) {
    return {r.token, r.operation.index(), Reply::Failure::None, std::move(n)};
}
Reply::Reply reply(const Req::Request& r, Trim::Result t) {
    return {r.token, r.operation.index(), Reply::Failure::None, std::move(t)};
}
Reply::Reply roundtrip(const Req::Request& r, const Reply::Reply& before) {
    auto encoded = Reply::encode(before, r); REQUIRE(encoded);
    Wire::Error error;
    const auto frame = Wire::encode(*encoded, &error); REQUIRE(error == Wire::Error::None); REQUIRE_FALSE(frame.isEmpty());
    const auto wire = Wire::decode(frame.mid(4)); REQUIRE(wire);
    auto decoded = Reply::decode(wire.fields, r); REQUIRE(decoded);
    same(before, *decoded);
    auto again = Reply::encode(*decoded, r); REQUIRE(again); CHECK(*again == *encoded);
    return std::move(*decoded);
}
QCborArray patched(QCborArray a, std::initializer_list<qsizetype> path, const QCborValue& value) {
    const auto patch = [&](auto&& self, QCborArray& current, auto i) -> void {
        const auto at = *i++;
        if (i == path.end()) { current[at] = value; return; }
        auto child = current.at(at).toArray(); self(self, child, i); current[at] = child;
    };
    patch(patch, a, path.begin());
    return a;
}
Req::Cutter cutterValue(const RS_Entity& e) {
    if (const auto* p = dynamic_cast<const RS_Line*>(&e)) return Req::Segment{p->getStartpoint(), p->getEndpoint()};
    if (const auto* p = dynamic_cast<const RS_Arc*>(&e)) return p->getData();
    if (const auto* p = dynamic_cast<const RS_Circle*>(&e)) return p->getData();
    if (const auto* p = dynamic_cast<const RS_Ellipse*>(&e)) return p->getData();
    return dynamic_cast<const RS_Spline&>(e).getData();
}
std::pair<Req::Request, Trim::Result> trimLine() {
    RS_Line cutter{nullptr, {4, -2}, {4, 2}};
    const auto data = cache(); const Trim::Pick pick{{1, .25}};
    Req::Request r{token(), Req::TrimCache{data, cutterValue(cutter), pick, {}}};
    auto t = Trim::prepare(data, cutter, pick);
    REQUIRE(t.refusal == Trim::Refusal::None);
    return {std::move(r), std::move(t)};
}
void trimRoundtrip(const RS_SplineData& data, const RS_Entity& cutter, const Trim::Pick& pick,
                   const Trim::Limits& limits = {}, Trim::Cancellation cancel = {}) {
    Req::Request r{token(), Req::TrimCache{data, cutterValue(cutter), pick, limits}};
    Trim::Work observed;
    const auto t = Trim::prepare(data, cutter, pick, limits, std::move(cancel), &observed);
    same(t.work, observed); roundtrip(r, reply(r, t));
}
void conicRoundtrip(const RS_SplineData& data, const RS_Entity& cutter, const Trim::Pick& pick,
                    const std::optional<Trim::Pick>& cachePick, const Trim::Limits& limits = {},
                    Trim::Cancellation cancel = {}) {
    Req::Request r{token(), Req::TrimConic{data, cutterValue(cutter), pick, cachePick, limits}};
    Trim::Work observed;
    const auto t = Trim::prepareConic(data, cutter, pick, cachePick, limits, std::move(cancel), &observed);
    same(t.work, observed); roundtrip(r, reply(r, t));
}
}

TEST_CASE("Worker native values retain every field without issuing native authority", tags) {
    for (bool validate : {false, true}) for (std::size_t degree : {1u, 3u}) {
        auto r = nativeRequest(validate); auto n = nativeValue(validate);
        if (degree == 3) n.curve->cache = cache({{-0.0, 0}, {1, 2}, {3, 2}, {4, 0}}, 3, -2.5, .125);
        n.curve->definition.basis.angleDegrees = -0.0;
        n.curve->definition.basis.startAngleDegrees = 123.25;
        n.curve->definition.basis.otherAngleDegrees = -17.5;
        n.curve->definition.basis.angularLength = 2.25;
        roundtrip(r, reply(r, n));
    }
    auto r = nativeRequest(); auto n = nativeValue();
    std::get<Req::Generate>(r.operation).throughPoint = RS_Vector{5, 0};
    n.throughWork = {0, 3, 5, 2, 0};
    roundtrip(r, reply(r, n));
}

TEST_CASE("Worker native refusals retain missing stages and failed attempted prefixes", tags) {
    auto r = nativeRequest();
    Reply::Native early; early.refusal = Refusal::TooSmall;
    roundtrip(r, reply(r, early));
    early.refusal = Refusal::InvalidInput; roundtrip(r, reply(r, early));
    for (qint64 status = qint64(Status::InvalidRequest); status <= qint64(Status::LimitExceeded); ++status) {
        Reply::Native n; n.refusal = Refusal::Engine; n.engineRuns = 1; n.engineStatus = Status(status);
        roundtrip(r, reply(r, n));
    }
    Reply::Native partial; partial.refusal = Refusal::Engine; partial.engineRuns = 1;
    partial.engineStatus = Status::LimitExceeded; partial.attemptedOutputUsage = {7, 2, 11};
    partial.attemptedDeepCountComplete = false; partial.sampleUnits = 23;
    roundtrip(r, reply(r, partial));
    partial.attemptedOutputUsage = {7, 0, 0}; partial.attemptedDeepCountComplete = true;
    partial.sampleUnits = LC_CurveOffset::kDefaultMaxSamples + 1; roundtrip(r, reply(r, partial));
    for (bool validate : {false, true}) for (qint64 reason = 1; reason <= qint64(Reason::LimitExceeded); ++reason) {
        auto request = nativeRequest(validate); auto n = nativeValue(validate);
        n.curve.reset(); n.changed = false; n.refusal = Refusal::CheckFailed; n.checkReason = Reason(reason);
        n.outputUsage = {}; roundtrip(request, reply(request, n));
    }
    for (auto refusal : {Refusal::NothingLeft, Refusal::SeveralPieces, Refusal::NotCacheRepresentable}) {
        Reply::Native n; n.refusal = refusal; n.engineRuns = 1; n.engineStatus = Status::Ok;
        n.attemptedOutputUsage = {3, 2, 11}; roundtrip(r, reply(r, n));
    }
    auto n = nativeValue(); n.curve.reset(); n.changed = false; n.refusal = Refusal::ThroughNotOnCache;
    n.throughWork = {0, 5, 7, 2, 0}; std::get<Req::Generate>(r.operation).throughPoint = RS_Vector{5, 7};
    roundtrip(r, reply(r, n));
    n.refusal = Refusal::NotCacheRepresentable; n.throughWork = {};
    roundtrip(r, reply(r, n));
    auto validate = nativeRequest(true); auto notDisplayable = nativeValue(true);
    notDisplayable.curve.reset(); notDisplayable.changed = false; notDisplayable.refusal = Refusal::NotCacheRepresentable;
    roundtrip(validate, reply(validate, notDisplayable));
}

TEST_CASE("Worker fatal replies have unknown work and a null payload only", tags) {
    auto [trimRequest, result] = trimLine();
    std::vector<Req::Request> requests{nativeRequest(), nativeRequest(true), trimRequest};
    requests.push_back({token(), Req::TrimConic{cache(), RS_CircleData{{0, 0}, 2}, {{0, 2}}, {}, {}}});
    requests.push_back({token(), Req::CheckThrough{cache(), definition(), .001, {5, 0}, {}}});
    for (const auto& r : requests) for (auto failure : {Reply::Failure::AllocationFailure, Reply::Failure::InternalFailure}) {
        Reply::Reply d{r.token, r.operation.index(), failure, std::monostate{}};
        roundtrip(r, d); auto encoded = Reply::encode(d, r); REQUIRE(encoded); CHECK(encoded->at(4).isNull());
        CHECK_FALSE(Reply::decode(patched(*encoded, {4}, QCborArray{}), r));
        CHECK_FALSE(Reply::decode(patched(*encoded, {4}, qint64(0)), r));
        d.result = nativeValue(); CHECK_FALSE(Reply::encode(d, r));
        d.result = result; CHECK_FALSE(Reply::encode(d, r));
        d.result = std::monostate{}; d.failure = Reply::Failure::None; CHECK_FALSE(Reply::encode(d, r));
    }
}

TEST_CASE("Worker replies match all four token fields and the exact request operation", tags) {
    const auto r = nativeRequest(); const auto d = reply(r, nativeValue());
    const auto encoded = Reply::encode(d, r); REQUIRE(encoded);
    for (qsizetype i = 0; i < 4; ++i) {
        CHECK_FALSE(Reply::decode(patched(*encoded, {1, i}, qint64(1)), r));
        auto other = d;
        if (i == 0) other.token.requestId = 1;
        if (i == 1) other.token.generation = 1;
        if (i == 2) other.token.sourceRevision = 1;
        if (i == 3) other.token.undoEpoch = 1;
        CHECK_FALSE(Reply::encode(other, r));
    }
    for (qint64 i : {-1, 1, 2, 3, 4}) CHECK_FALSE(Reply::decode(patched(*encoded, {2}, i), r));
    for (qint64 i : {-1, 0, 1, 3}) CHECK_FALSE(Reply::decode(patched(*encoded, {0}, i), r));
    for (qint64 i : {-1, 3, 99}) CHECK_FALSE(Reply::decode(patched(*encoded, {3}, i), r));
    auto other = d; other.operation = 1; CHECK_FALSE(Reply::encode(other, r));
    other = d; other.operation = std::numeric_limits<std::size_t>::max(); CHECK_FALSE(Reply::encode(other, r));
    other = d; other.failure = static_cast<Reply::Failure>(99); CHECK_FALSE(Reply::encode(other, r));
    other = d; other.result = Trim::Result{}; CHECK_FALSE(Reply::encode(other, r));
    auto zeroRequest = r; zeroRequest.token.requestId = 0; other = d; other.token = zeroRequest.token;
    CHECK_FALSE(Reply::encode(other, zeroRequest));
}

TEST_CASE("Worker native status contradictions never fabricate absent stages", tags) {
    const auto r = nativeRequest(); const auto d = reply(r, nativeValue());
    const auto encoded = Reply::encode(d, r); REQUIRE(encoded);
    for (const auto& bad : {
            patched(*encoded, {4, 0}, qint64(Refusal::CheckFailed)),
            patched(*encoded, {4, 1}, QCborValue(nullptr)),
            patched(*encoded, {4, 1}, qint64(Status::FitFailed)),
            patched(*encoded, {4, 2}, QCborValue(nullptr)),
            patched(*encoded, {4, 2}, qint64(Reason::Layout)),
            patched(*encoded, {4, 5}, qint64(0)), patched(*encoded, {4, 5}, qint64(2)),
            patched(*encoded, {4, 6}, false), patched(*encoded, {4, 7}, QCborValue(nullptr)),
            patched(*encoded, {4, 9}, false)}) CHECK_FALSE(Reply::decode(bad, r));
    for (auto refusal : {Refusal::Undefined, Refusal::Inconsistent, Refusal::ThroughNotOnCache}) {
        auto n = nativeValue(); n.curve.reset(); n.changed = false; n.refusal = refusal;
        CHECK_FALSE(Reply::encode(reply(r, n), r));
    }
    auto validate = nativeRequest(true); auto n = nativeValue(true);
    n.engineRuns = 1; CHECK_FALSE(Reply::encode(reply(validate, n), validate));
    n = nativeValue(true); n.engineStatus = Status::Ok; CHECK_FALSE(Reply::encode(reply(validate, n), validate));
    n = nativeValue(true); n.sampleUnits = 1; CHECK_FALSE(Reply::encode(reply(validate, n), validate));
    n = nativeValue(true); n.attemptedDeepCountComplete = false; CHECK_FALSE(Reply::encode(reply(validate, n), validate));
    Reply::Native missing; missing.checkWork.vertices = 1; CHECK_FALSE(Reply::encode(reply(r, missing), r));
}

TEST_CASE("Worker native work adds vertices and pairs but independently caps query depth", tags) {
    auto r = nativeRequest(); auto& op = std::get<Req::Generate>(r.operation);
    op.throughPoint = RS_Vector{5, 0}; op.limits.checkLimits = {1, 20, 30, 3, 40};
    auto n = nativeValue(); n.checkWork = {1, 7, 11, 3, 40}; n.throughWork = {0, 13, 19, 3, 0};
    roundtrip(r, reply(r, n)); // depth is max(3,3), not 3+3
    const auto check = [&](auto change) {
        auto value = n; change(value); CHECK_FALSE(Reply::encode(reply(r, value), r));
    };
    check([](auto& x) { ++x.checkWork.cacheSpans; }); check([](auto& x) { ++x.checkWork.orderingComparisons; });
    check([](auto& x) { ++x.throughWork.vertices; }); check([](auto& x) { ++x.throughWork.distancePairs; });
    check([](auto& x) { ++x.checkWork.deepest; }); check([](auto& x) { ++x.throughWork.deepest; });
    check([](auto& x) { x.throughWork.cacheSpans = 1; }); check([](auto& x) { x.throughWork.orderingComparisons = 1; });
    check([](auto& x) { x.throughWork.vertices = std::numeric_limits<std::size_t>::max(); });
    check([](auto& x) { x.checkWork.distancePairs = std::numeric_limits<std::size_t>::max(); });
    op.limits.checkLimits.maxVertices = std::numeric_limits<std::size_t>::max();
    n.checkWork.vertices = LC_OffsetCheckLimits{}.maxVertices; n.throughWork.vertices = 1;
    CHECK_FALSE(Reply::encode(reply(r, n), r));
}

TEST_CASE("Worker CheckThrough receipts contain only bounded membership work and no authority", tags) {
    Req::Request r{token(), Req::CheckThrough{cache(), definition(), .001, {5, 0}, {2, 11, 13, 3, 17}}};
    Reply::Native n;
    n.refusal = Refusal::None; n.throughWork = {2, 11, 13, 3, 17};
    roundtrip(r, reply(r, n));
    n.refusal = Refusal::ThroughNotOnCache; roundtrip(r, reply(r, n));
    const auto encoded = Reply::encode(reply(r, n), r); REQUIRE(encoded);
    CHECK(encoded->at(0).toInteger() == 2); CHECK(encoded->at(2).toInteger() == 4);
    REQUIRE(encoded->at(4).toArray().size() == 12);
    CHECK_FALSE(Reply::decode(patched(*encoded, {0}, 1), r));
    CHECK_FALSE(Reply::decode(patched(*encoded, {2}, 5), r));
    for (qint64 raw = 0; raw <= qint64(Refusal::ThroughNotOnCache); ++raw) {
        auto value = n; value.refusal = Refusal(raw);
        const bool allowed = value.refusal == Refusal::None || value.refusal == Refusal::ThroughNotOnCache;
        CHECK(bool(Reply::encode(reply(r, value), r)) == allowed);
        CHECK(bool(Reply::decode(patched(*encoded, {4, 0}, raw), r)) == allowed);
    }
    Reply::Native invalid; invalid.refusal = Refusal::InvalidInput;
    roundtrip(r, reply(r, invalid));
    invalid.throughWork.cacheSpans = 1; CHECK_FALSE(Reply::encode(reply(r, invalid), r));
    const auto reject = [&](auto mutate) {
        auto value = n; mutate(value); CHECK_FALSE(Reply::encode(reply(r, value), r));
    };
    reject([](auto& v) { v.engineRuns = 1; }); reject([](auto& v) { v.engineStatus = Status::Ok; });
    reject([](auto& v) { v.checkReason = Reason::None; }); reject([](auto& v) { v.checkWork.vertices = 1; });
    reject([](auto& v) { v.changed = true; }); reject([](auto& v) { v.curve = nativeValue().curve; });
    reject([](auto& v) { v.attemptedOutputUsage.cubicPieces = 1; });
    reject([](auto& v) { v.attemptedDeepCountComplete = false; });
    reject([](auto& v) { v.outputUsage.outputEntities = 1; }); reject([](auto& v) { v.sampleUnits = 1; });
    reject([](auto& v) { ++v.throughWork.cacheSpans; }); reject([](auto& v) { ++v.throughWork.vertices; });
    reject([](auto& v) { ++v.throughWork.distancePairs; }); reject([](auto& v) { ++v.throughWork.deepest; });
    reject([](auto& v) { ++v.throughWork.orderingComparisons; });
    reject([](auto& v) { v.throughWork.vertices = std::numeric_limits<std::size_t>::max(); });
    const std::array<qint64, 5> caps{2, 11, 13, 3, 17};
    for (qsizetype field = 0; field < 5; ++field) {
        CHECK_FALSE(Reply::decode(patched(*encoded, {4, 4, field}, caps[std::size_t(field)] + 1), r));
        CHECK_FALSE(Reply::decode(patched(*encoded, {4, 3, field}, 1), r));
        CHECK_FALSE(Reply::decode(patched(*encoded, {4, 4, field}, -1), r));
        CHECK_FALSE(Reply::decode(patched(*encoded, {4, 4, field}, 0.0), r));
    }
    const auto generated = Reply::encode(reply(nativeRequest(), nativeValue()), nativeRequest()); REQUIRE(generated);
    for (const auto& bad : {
            patched(*encoded, {4, 1}, qint64(Status::Ok)), patched(*encoded, {4, 2}, qint64(Reason::None)),
            patched(*encoded, {4, 5}, 1), patched(*encoded, {4, 6}, true),
            patched(*encoded, {4, 7}, generated->at(4).toArray().at(7)),
            patched(*encoded, {4, 8, 0}, 1), patched(*encoded, {4, 9}, false),
            patched(*encoded, {4, 10, 0}, 1), patched(*encoded, {4, 11}, 1)})
        CHECK_FALSE(Reply::decode(bad, r));
    auto body = encoded->at(4).toArray(); body.removeLast();
    CHECK_FALSE(Reply::decode(patched(*encoded, {4}, body), r));
    body = encoded->at(4).toArray(); body.append(nullptr);
    CHECK_FALSE(Reply::decode(patched(*encoded, {4}, body), r));
    CHECK_FALSE(Reply::encode(reply(r, Trim::Result{}), r));

    auto& limits = std::get<Req::CheckThrough>(r.operation).limits;
    limits = {}; limits.maxVertices = std::numeric_limits<std::size_t>::max();
    n.throughWork = {0, LC_OffsetCheckLimits{}.maxVertices, 0, 0, 0}; roundtrip(r, reply(r, n));
    ++n.throughWork.vertices; CHECK_FALSE(Reply::encode(reply(r, n), r));
}

TEST_CASE("Worker native array and producer usage ceilings admit exact boundaries", tags) {
    auto r = nativeRequest(); auto n = nativeValue();
    std::vector<RS_Vector> points;
    for (std::size_t i = 0; i <= 3 * 4096; ++i) points.emplace_back(double(i), 0);
    n.curve->cache = cache(std::move(points), 3); n.curve->grips.assign(256, RS_Vector{2, 3});
    n.checkWork.cacheSpans = 4096; roundtrip(r, reply(r, n));
    n.curve->grips.push_back({2, 3}); CHECK_FALSE(Reply::encode(reply(r, n), r)); n.curve->grips.pop_back();
    auto badCache = n; std::vector<RS_Vector> over;
    for (std::size_t i = 0; i <= 3 * 4097; ++i) over.emplace_back(double(i), 0);
    badCache.curve->cache = cache(std::move(over), 3);
    CHECK_FALSE(Reply::encode(reply(r, badCache), r));
    for (std::size_t field = 0; field < 3; ++field) {
        auto limited = nativeRequest(); auto& budget = std::get<Req::Generate>(limited.operation).limits.outputBudget;
        budget = {3, 2, 35}; auto value = nativeValue(); value.attemptedOutputUsage = {3, 2, 35}; value.outputUsage = {3, 1, 35};
        roundtrip(limited, reply(limited, value));
        if (field == 0) ++value.attemptedOutputUsage.cubicPieces;
        if (field == 1) ++value.attemptedOutputUsage.outputEntities;
        if (field == 2) ++value.attemptedOutputUsage.deepEntities;
        CHECK_FALSE(Reply::encode(reply(limited, value), limited));
    }
    auto limited = nativeRequest(); std::get<Req::Generate>(limited.operation).limits.maxSamples = 3;
    Reply::Native failed; failed.refusal = Refusal::Engine; failed.engineRuns = 1; failed.engineStatus = Status::LimitExceeded;
    failed.sampleUnits = 4; roundtrip(limited, reply(limited, failed));
    failed.sampleUnits = 5; CHECK_FALSE(Reply::encode(reply(limited, failed), limited));
    failed.sampleUnits = 4; failed.engineStatus = Status::FitFailed; CHECK_FALSE(Reply::encode(reply(limited, failed), limited));
}

TEST_CASE("Worker native cache syntax is strict and does not repair latent fields", tags) {
    const auto r = nativeRequest(); const auto good = nativeValue();
    const auto check = [&](auto change) { auto n = good; change(*n.curve); CHECK_FALSE(Reply::encode(reply(r, n), r)); };
    check([](auto& c) { c.cache.degree = 2; }); check([](auto& c) { c.cache.degree = std::numeric_limits<std::size_t>::max(); });
    check([](auto& c) { c.cache.type = RS_SplineData::SplineType::Standard; });
    check([](auto& c) { c.cache.savedOpenType = RS_SplineData::SplineType::WrappedClosed; });
    check([](auto& c) { c.cache.fitPoints = {{1, 2}}; }); check([](auto& c) { c.cache.savedOpenKnots = {0}; });
    check([](auto& c) { c.cache.weights[0] = 2; }); check([](auto& c) { c.cache.weights.pop_back(); });
    check([](auto& c) { c.cache.knotslist[1] = .5; }); check([](auto& c) { c.cache.controlPoints[0].valid = false; });
    check([](auto& c) { c.cache.controlPoints[0].z = 1; }); check([](auto& c) { c.grips[0].valid = false; });
    check([](auto& c) { c.midpoint.valid = false; }); check([](auto& c) { c.worldTolerance = 0; });
    check([](auto& c) { c.definition.basis.ratio = 2; }); check([](auto& c) { c.definition.distance = 0; });
    check([](auto& c) { c.definition.basis.angle1 = -.25; }); check([](auto& c) { c.definition.relTolerance = 0; });
    for (double bad : std::array<double, 3>{INFINITY, -INFINITY, std::numeric_limits<double>::quiet_NaN()}) {
        check([&](auto& c) { c.midpoint.x = bad; }); check([&](auto& c) { c.cache.controlPoints[0].x = bad; });
        check([&](auto& c) { c.definition.basis.angularLength = bad; }); check([&](auto& c) { c.worldTolerance = bad; });
    }
}

TEST_CASE("Worker Trim prepare preserves polynomial sections endpoints roots removed intervals and work", tags) {
    for (bool reverse : {false, true}) for (std::size_t degree : {1u, 3u}) {
        auto data = degree == 1 ? cache() : cache({{0, 0}, {2, 1}, {4, 1}, {6, 0}}, 3, -1e200, 2e200);
        if (reverse) std::reverse(data.controlPoints.begin(), data.controlPoints.end());
        RS_Line line{nullptr, {3, -2}, {3, 2}}; trimRoundtrip(data, line, {{1.5, .5625}});
    }
    for (bool reverse : {false, true}) for (bool cutterReverse : {false, true}) {
        auto data = cache(); if (reverse) std::reverse(data.controlPoints.begin(), data.controlPoints.end());
        auto cutting = cache({{3, -2}, {3, 2}, {7, 2}, {7, -2}});
        if (cutterReverse) std::reverse(cutting.controlPoints.begin(), cutting.controlPoints.end());
        RS_Spline cutter{nullptr, cutting};
        const Trim::Pick pick{{5, .5}}; const auto t = Trim::prepare(data, cutter, pick);
        REQUIRE(t.refusal == Trim::Refusal::None); REQUIRE(t.fragments.size() == 2);
        trimRoundtrip(data, cutter, pick);
    }
    for (bool reverse : {false, true}) for (double side : {-1., 1.}) {
        auto data = cache({{-2, -2}, {2, -2}, {2, 2}, {-2, 2}, {-2, -2}});
        if (reverse) std::reverse(data.controlPoints.begin(), data.controlPoints.end());
        RS_Line line{nullptr, {0, -3}, {0, 3}}; trimRoundtrip(data, line, {{2 * side, 0}});
        RS_Circle circle{nullptr, {{0, 0}, 2.25}}; trimRoundtrip(data, circle, {{2, 0}});
    }
}

TEST_CASE("Worker Trim conic and paired results preserve original conics and all diagnostics", tags) {
    for (bool reverse : {false, true}) for (unsigned kind = 0; kind < 4; ++kind) {
        const auto data = cache({{-3, .5}, {3, .5}});
        const bool open = kind == 1 || kind == 3;
        const double ratio = kind >= 2 ? .75 : 1.;
        std::unique_ptr<RS_Entity> cutter;
        if (kind == 0) cutter = std::make_unique<RS_Circle>(nullptr, RS_CircleData{{0, 0}, 2});
        else if (kind == 1) cutter = std::make_unique<RS_Arc>(nullptr,
            RS_ArcData{{0, 0}, 2, reverse ? pi : 0, reverse ? 0 : pi, reverse});
        else cutter = std::make_unique<RS_Ellipse>(nullptr,
            RS_EllipseData{{0, 0}, {2, 0}, ratio, open && reverse ? pi : 0, open && !reverse ? pi : 0, reverse});
        const Trim::Pick pick{{0, 2 * ratio}};
        for (bool paired : {false, true}) {
            const std::optional<Trim::Pick> cachePick = paired ? std::optional<Trim::Pick>{Trim::Pick{{0, .5}}} : std::nullopt;
            const auto t = Trim::prepareConic(data, *cutter, pick, cachePick);
            REQUIRE(t.refusal == Trim::Refusal::None); REQUIRE_FALSE(t.conicFragments.empty());
            CHECK(t.fragmentCount == t.fragments.size()); CHECK(t.fragments.empty() == !paired);
            conicRoundtrip(data, *cutter, pick, cachePick);
        }
        trimRoundtrip(data, *cutter, {{0, .5}});
    }
    for (double ratio : {.5, 2.}) {
        RS_Ellipse ellipse{nullptr, {{0, 0}, {2, 1}, ratio}};
        conicRoundtrip(cache({{-6, 0}, {6, 0}}), ellipse, {{-ratio, 2 * ratio}}, Trim::Pick{{0, 0}});
    }
}

TEST_CASE("Worker actual Trim refusals retain consumed prefixes and diagnostic root counts", tags) {
    auto [r, baseline] = trimLine(); const auto& op = std::get<Req::TrimCache>(r.operation);
    RS_Line line{nullptr, {4, -2}, {4, 2}};
    RS_Line miss{nullptr, {20, -2}, {20, 2}}; trimRoundtrip(op.cache, miss, op.pick);
    RS_Line overlap{nullptr, {0, 0}, {10, 0}}; trimRoundtrip(op.cache, overlap, op.pick);
    trimRoundtrip(op.cache, line, {{4, 0}});
    auto invalid = op.cache; invalid.weights[0] = 2; trimRoundtrip(invalid, line, op.pick);
    invalid = op.cache; invalid.controlPoints[0].valid = false; trimRoundtrip(invalid, line, op.pick);
    Trim::Cancellation cancel{std::make_shared<std::atomic_bool>(true)};
    trimRoundtrip(op.cache, line, op.pick, {}, cancel);
    for (unsigned field = 0; field < 14; ++field) {
        Trim::Limits l;
        switch (field) {
        case 0: l.maxSpansPerCurve = 0; break; case 1: l.maxInputSpans = 0; break;
        case 2: l.maxInputRecords = 0; break; case 3: l.maxBoxPairs = 0; break;
        case 4: l.maxLeaves = 0; break; case 5: l.maxGeneratedItems = 0; break;
        case 6: l.maxOrderingComparisons = 0; break; case 7: l.maxRoots = 0; break;
        case 8: l.maxDepth = 0; break; case 9: l.maxLeafDepth = 0; break;
        case 10: l.maxFragments = 0; break; case 11: l.maxOutputSpans = 0; break;
        case 12: l.maxOutputControls = 0; break; case 13: l.maxOutputKnots = 0; break;
        }
        const auto t = Trim::prepare(op.cache, line, op.pick, l);
        REQUIRE(t.refusal != Trim::Refusal::None); trimRoundtrip(op.cache, line, op.pick, l);
    }
    const auto data = cache({{-3, .5}, {3, .5}}); RS_Circle circle{nullptr, {{0, 0}, 2}};
    const auto paired = Trim::prepareConic(data, circle, {{0, 2}}, Trim::Pick{{0, .5}});
    REQUIRE(paired.refusal == Trim::Refusal::None);
    Trim::Limits limited; limited.maxOutputSpans = paired.work.outputSpans - 1;
    const auto refused = Trim::prepareConic(data, circle, {{0, 2}}, Trim::Pick{{0, .5}}, limited);
    REQUIRE(refused.refusal == Trim::Refusal::LimitExceeded); REQUIRE(refused.rootCount > 0);
    REQUIRE(refused.work.outputControls > 0); REQUIRE(refused.fragments.empty());
    conicRoundtrip(data, circle, {{0, 2}}, Trim::Pick{{0, .5}}, limited);
    conicRoundtrip(data, circle, {{0, 0}}, Trim::Pick{{0, .5}});
    conicRoundtrip(data, circle, {{0, 2}}, {}, {}, cancel);
    conicRoundtrip(cache({{-3, 2}, {3, 2}}), circle, {{0, 2}}, {});
}

TEST_CASE("Worker Trim malformed records cannot drop fields repair intervals or forge references", tags) {
    auto [r, t] = trimLine(); auto d = reply(r, t); const auto encoded = Reply::encode(d, r); REQUIRE(encoded);
    const std::vector<QCborArray> bad{
        patched(*encoded, {4, 0}, qint64(99)), patched(*encoded, {4, 0}, qint64(Trim::Refusal::Arithmetic)),
        patched(*encoded, {4, 5}, qint64(t.rootCount + 1)), patched(*encoded, {4, 6}, qint64(t.fragmentCount + 1)),
        patched(*encoded, {4, 4}, QCborValue(nullptr)), patched(*encoded, {4, 1, 0, 1}, QCborArray{}),
        patched(*encoded, {4, 1, 0, 2, 0}, qint64(8)), patched(*encoded, {4, 1, 0, 2, 2}, qint64(t.roots.size())),
        patched(*encoded, {4, 1, 0, 2, 1, 0}, qint64(4096)),
        patched(*encoded, {4, 1, 0, 2, 1, 1}, QCborArray{.75, .25}),
        patched(*encoded, {4, 1, 0, 1, 0, 0}, qint64(4096)),
        patched(*encoded, {4, 1, 0, 1, 0, 1}, 1.1), patched(*encoded, {4, 1, 0, 1, 0, 2}, .1),
        patched(*encoded, {4, 1, 0, 1, 0, 3}, QCborArray{1., -1.}),
        patched(*encoded, {4, 3, 0, 0}, qint64(1)), patched(*encoded, {4, 3, 0, 4}, qint64(99)),
        patched(*encoded, {4, 3, 0, 2, 0}, qint64(4096)),
        patched(*encoded, {4, 3, 0, 2, 2}, QCborArray{.8, .2}),
        patched(*encoded, {4, 3, 0, 3, 3}, QCborArray{2., -2.}),
        patched(*encoded, {4, 3, 0, 5}, QCborArray{INFINITY, INFINITY}),
        patched(*encoded, {4, 7, 5}, qint64(t.work.outputSpans + 1))
    };
    for (const auto& a : bad) CHECK_FALSE(Reply::decode(a, r));
    auto changed = t; changed.roots[0].id = 1; CHECK_FALSE(Reply::encode(reply(r, changed), r));
    changed = t; changed.fragments[0].start.labels = 8; CHECK_FALSE(Reply::encode(reply(r, changed), r));
    changed = t; changed.fragments[0].sourceSections[0].to = INFINITY; CHECK_FALSE(Reply::encode(reply(r, changed), r));
    changed = t; changed.removed->from.localParameter = LC_Interval{}; CHECK_FALSE(Reply::encode(reply(r, changed), r));
    changed = t; changed.work.intersections.totalBoxPairs = std::numeric_limits<std::size_t>::max();
    CHECK_FALSE(Reply::encode(reply(r, changed), r));
}

TEST_CASE("Worker Trim shared work ceilings and every intersection statistic are checked", tags) {
    auto [r, success] = trimLine(); (void)success;
    auto& op = std::get<Req::TrimCache>(r.operation);
    op.limits = {2, 5, 31, 37, 41, 43, 47, 7, 3, 2, 1, 11, 13, 17};
    Trim::Result t; t.refusal = Trim::Refusal::LimitExceeded; t.rootCount = 3;
    t.work = {5, 31, 7, 11, 13, 11, 13, 17, {7, 11, 19, 37, 41, 32, 2, 3, 3, 3, 34, 29, 23}};
    roundtrip(r, reply(r, t));
    for (unsigned field = 0; field < 8; ++field) {
        auto v = t;
        switch (field) {
        case 0: ++v.work.inputSpans; break; case 1: ++v.work.inputRecords; break;
        case 2: ++v.work.rootRecords; break; case 3: ++v.work.kernelItems; break;
        case 4: ++v.work.kernelComparisons; break; case 5: ++v.work.outputSpans; break;
        case 6: ++v.work.outputControls; break; case 7: ++v.work.outputKnots; break;
        }
        CHECK_FALSE(Reply::encode(reply(r, v), r));
    }
    for (unsigned field = 0; field < 13; ++field) {
        auto v = t; auto& s = v.work.intersections;
        switch (field) {
        case 0: ++s.candidateBoxPairs; break; case 1: ++s.refinementBoxPairs; break;
        case 2: ++s.certificateBoxPairs; break; case 3: ++s.totalBoxPairs; break;
        case 4: ++s.generatedLeaves; break; case 5: ++s.generatedItems; break;
        case 6: ++s.deepestPreparation; break; case 7: ++s.deepestPairRefinement; break;
        case 8: ++s.deepestOrderingRefinement; break; case 9: ++s.deepestRootRefinement; break;
        case 10: ++s.orderingComparisons; break; case 11: s.validationSegmentVisits = 44; break;
        case 12: s.validationNextCalls = 30; break;
        }
        CHECK_FALSE(Reply::encode(reply(r, v), r));
    }
    auto v = t; v.work.intersections.candidateBoxPairs = std::numeric_limits<std::size_t>::max();
    CHECK_FALSE(Reply::encode(reply(r, v), r));
    v = t; v.work.kernelComparisons = std::numeric_limits<std::size_t>::max(); CHECK_FALSE(Reply::encode(reply(r, v), r));
    // A failed preparation can stop after validation but before generating all
    // of its segment records. These independent diagnostics must survive.
    v = {}; v.refusal = Trim::Refusal::Cancelled;
    v.work.intersections.validationSegmentVisits = 5; v.work.intersections.validationNextCalls = 2;
    roundtrip(r, reply(r, v));
}

TEST_CASE("Worker Trim output and nested conic records obey lowered caps and exact layouts", tags) {
    auto [r, t] = trimLine();
    auto& l = std::get<Req::TrimCache>(r.operation).limits;
    l.maxOutputSpans = t.work.outputSpans; l.maxOutputControls = t.work.outputControls;
    l.maxOutputKnots = t.work.outputKnots; l.maxFragments = t.fragmentCount; l.maxRoots = t.work.rootRecords;
    const auto encoded = Reply::encode(reply(r, t), r); REQUIRE(encoded); roundtrip(r, reply(r, t));
    for (unsigned field = 0; field < 5; ++field) {
        auto lowered = r; auto& cap = std::get<Req::TrimCache>(lowered.operation).limits;
        if (field == 0) --cap.maxOutputSpans;
        if (field == 1) --cap.maxOutputControls;
        if (field == 2) --cap.maxOutputKnots;
        if (field == 3) --cap.maxFragments;
        if (field == 4) --cap.maxRoots;
        CHECK_FALSE(Reply::encode(reply(lowered, t), lowered)); CHECK_FALSE(Reply::decode(*encoded, lowered));
    }
    for (qsizetype field = 0; field < 8; ++field)
        CHECK_FALSE(Reply::decode(patched(*encoded, {4, 7, field}, std::numeric_limits<qint64>::max()), r));
    for (qsizetype field = 0; field < 13; ++field)
        CHECK_FALSE(Reply::decode(patched(*encoded, {4, 7, 8, field}, std::numeric_limits<qint64>::max()), r));
    CHECK_FALSE(Reply::decode(patched(*encoded, {4, 1, 0, 1, 0, 3}, QCborArray{-2., 3.}), r));
    auto fragment = encoded->at(4).toArray().at(1).toArray().at(0).toArray(); fragment.append(fragment.at(2));
    CHECK_FALSE(Reply::decode(patched(*encoded, {4, 1, 0}, fragment), r));

    const auto data = cache({{-3, .5}, {3, .5}}); RS_Circle circle{nullptr, {{0, 0}, 2}};
    Req::Request cr{token(), Req::TrimConic{data, circle.getData(), {{0, 2}}, {}, {}}};
    const auto ct = Trim::prepareConic(data, circle, {{0, 2}});
    REQUIRE(ct.refusal == Trim::Refusal::None); REQUIRE_FALSE(ct.conicFragments.empty());
    const auto wire = Reply::encode(reply(cr, ct), cr); REQUIRE(wire);
    for (const auto& bad : {
            patched(*wire, {4, 2, 0, 0, 2}, INFINITY),
            patched(*wire, {4, 2, 0, 0, 7}, INFINITY),
            patched(*wire, {4, 2, 0, 1}, QCborArray{1., 0.}),
            patched(*wire, {4, 2, 0, 2}, QCborArray{0., INFINITY}),
            patched(*wire, {4, 2, 0, 3}, qint64(ct.roots.size())),
            patched(*wire, {4, 2, 0, 4}, qint64(ct.roots.size())),
            patched(*wire, {4, 2, 0, 5}, qint64(0)),
            patched(*wire, {4, 6}, qint64(ct.conicFragments.size()))}) CHECK_FALSE(Reply::decode(bad, cr));
    auto conic = wire->at(4).toArray().at(2).toArray().at(0).toArray(); conic.append(conic.at(0));
    CHECK_FALSE(Reply::decode(patched(*wire, {4, 2, 0}, conic), cr));
    auto refused = ct; refused.refusal = Trim::Refusal::Arithmetic; CHECK_FALSE(Reply::encode(reply(cr, refused), cr));
}

TEST_CASE("Worker worst structural Trim shape fits the unchanged whole wire ceilings", tags) {
    // SYNTHETIC VALUES, NOT PROOF: exercise the protocol's maximal record
    // shape. No geometric query, certificate or native authority is produced.
    std::vector<RS_Vector> points;
    for (std::size_t i = 0; i <= 3 * 4096; ++i) points.emplace_back(double(i), 0);
    const auto source = cache(std::move(points), 3);
    Req::Request r{token(), Req::TrimCache{source, Req::Cutter{source}, {{6142.5, 0}}, {}}};
    Trim::Result t; t.refusal = Trim::Refusal::None;
    t.rootCount = 4096; t.fragmentCount = 2; t.roots.reserve(t.rootCount);
    const auto interval = [](double lo, double hi) { return LC_Interval::hull(lo, hi); };
    for (std::size_t i = 0; i < 4096; ++i) {
        const std::size_t raw = i / 2;
        const double local = i % 2 ? .75 : .25;
        const double a = double(raw) + local, b = double(i) + .5;
        // A indexes the original cache; B indexes the concatenated cutter
        // segments, whose own parameters are still [i,i+1], not [B,B+1].
        const LC_IntersectionDomain isolation{raw, 4096 + i, interval(a, a), interval(b, b)};
        const LC_IntersectionDomain domain{raw, 4096 + i, interval(a - .125, a + .125), interval(b - .125, b + .125)};
        t.roots.push_back({i, {raw, interval(local, local)}, isolation, domain,
            LC_IntersectionProof::IntervalUniqueTransverse, interval(3 * a, 3 * a), interval(0, 0)});
    }
    const auto endpoint = [&](std::size_t raw, double u, unsigned labels, std::optional<std::size_t> root) {
        const double x = 3 * (double(raw) + u);
        return Trim::Endpoint{labels, {raw, interval(u, u)}, root, interval(x, x), interval(0, 0)};
    };
    const auto fragment = [&](bool first) {
        Trim::Fragment f; f.data.degree = 3;
        const std::size_t begin = first ? 0 : 2047, end = first ? 2048 : 4096;
        std::vector<RS_Vector> controls;
        for (std::size_t raw = begin; raw < end; ++raw) {
            const double from = !first && raw == 2047 ? .75 : 0.;
            const double to = first && raw == 2047 ? .25 : 1.;
            f.sourceSections.push_back({raw, from, to, interval(double(raw), double(raw + 1))});
            for (std::size_t j = controls.empty() ? 0 : 1; j <= 3; ++j)
                controls.emplace_back(3 * double(raw) + 3 * from + (to - from) * double(j), 0);
        }
        f.data = cache(std::move(controls), 3);
        f.start = first ? endpoint(0, 0, unsigned(Trim::EndLabel::SourceStart), {}) :
            endpoint(2047, .75, unsigned(Trim::EndLabel::CutterRoot), 4095);
        f.end = first ? endpoint(2047, .25, unsigned(Trim::EndLabel::CutterRoot), 4094) :
            endpoint(4095, 1, unsigned(Trim::EndLabel::SourceEnd), {});
        return f;
    };
    t.fragments.push_back(fragment(true)); t.fragments.push_back(fragment(false));
    t.removed = Trim::RemovedInterval{{2047, interval(.25, .25)}, {2047, interval(.75, .75)}, false};
    t.work.inputSpans = 8192;
    t.work.inputRecords = 2 * (source.controlPoints.size() + source.weights.size() + source.knotslist.size());
    t.work.rootRecords = 4096; t.work.kernelItems = 20000; t.work.kernelComparisons = 30000;
    for (const auto& f : t.fragments) {
        t.work.outputSpans += f.sourceSections.size();
        t.work.outputControls += f.data.controlPoints.size(); t.work.outputKnots += f.data.knotslist.size();
    }
    t.work.intersections = {4096, 4096, 8192, 16384, 8192, 40000, 1, 2, 3, 4, 31000, 8192, 2};
    REQUIRE(t.work.outputSpans == 4097); REQUIRE(t.work.outputControls == 12293); REQUIRE(t.work.outputKnots == 12301);
    const auto d = reply(r, t); const auto encoded = Reply::encode(d, r); REQUIRE(encoded);
    const auto count = [](auto&& self, const QCborValue& value) -> std::size_t {
        std::size_t result = 1;
        if (value.isArray()) for (const auto& child : value.toArray()) result += self(self, child);
        return result;
    };
    const auto values = count(count, *encoded);
    CAPTURE(values);
    CHECK(values == 245943); CHECK(values <= Wire::Limits{}.items);
    Wire::Error error; const auto frame = Wire::encode(*encoded, &error);
    REQUIRE(error == Wire::Error::None); REQUIRE(frame.size() > 4);
    CAPTURE(frame.size()); CHECK(frame.size() - 4 <= Wire::Limits{}.frameBytes);
    const auto admitted = Wire::decode(frame.mid(4)); REQUIRE(admitted);
    CHECK(admitted.work.items == values); CHECK(admitted.work.payloadBytes <= Wire::Limits{}.frameBytes);
    const auto decoded = Reply::decode(admitted.fields, r); REQUIRE(decoded); same(*decoded, d);
    // No geometry caps are raised to accommodate the maximal structural case.
    auto tooMany = t; ++tooMany.rootCount; CHECK_FALSE(Reply::encode(reply(r, tooMany), r));
    tooMany = t; tooMany.roots.push_back(tooMany.roots.back()); CHECK_FALSE(Reply::encode(reply(r, tooMany), r));
    tooMany = t; ++tooMany.work.outputSpans; CHECK_FALSE(Reply::encode(reply(r, tooMany), r));
}

TEST_CASE("Worker reply arrays reject extra duplicate and unknown metadata and scalar coercion", tags) {
    const auto r = nativeRequest(); const auto encoded = Reply::encode(reply(r, nativeValue()), r); REQUIRE(encoded);
    auto extra = *encoded; extra.append(QCborArray{QStringLiteral("work"), 0}); CHECK_FALSE(Reply::decode(extra, r));
    extra = encoded->at(4).toArray(); extra.append(extra.at(3));
    CHECK_FALSE(Reply::decode(patched(*encoded, {4}, extra), r));
    extra = encoded->at(4).toArray(); extra.removeLast(); CHECK_FALSE(Reply::decode(patched(*encoded, {4}, extra), r));
    QCborMap metadata; metadata.insert(QStringLiteral("work"), QCborArray{});
    for (const QCborValue& bad : std::vector<QCborValue>{metadata, QStringLiteral("unknown"), QByteArray("x"), QCborValue(), true, QCborArray{}})
        CHECK_FALSE(Reply::decode(patched(*encoded, {4, 3}, bad), r));
    for (const auto& bad : std::vector<QCborValue>{qint64(0), true, QCborValue(nullptr), QStringLiteral("1"), INFINITY,
            -INFINITY, std::numeric_limits<double>::quiet_NaN()}) {
        CHECK_FALSE(Reply::decode(patched(*encoded, {4, 7, 2}, bad), r));
        CHECK_FALSE(Reply::decode(patched(*encoded, {4, 7, 0, 0, 0, 0}, bad), r));
    }
    CHECK_FALSE(Reply::decode(patched(*encoded, {4, 3, 1}, 1.), r));
    CHECK_FALSE(Reply::decode(patched(*encoded, {4, 3, 1}, qint64(-1)), r));
    CHECK_FALSE(Reply::decode(patched(*encoded, {4, 3, 1}, std::numeric_limits<qint64>::max()), r));
    CHECK_FALSE(Reply::decode(patched(*encoded, {4, 7, 0, 4}, std::numeric_limits<qint64>::max()), r));
    // Typed QCborArray cannot retain an out-of-int64 CBOR integer or its float
    // width. The raw wire admission must reject these before typed decoding.
    for (const auto& raw : {QByteArray::fromHex("811b8000000000000000"), QByteArray::fromHex("813b8000000000000000"),
            QByteArray::fromHex("81fa3f800000"), QByteArray::fromHex("81f93c00")}) CHECK_FALSE(Wire::decode(raw));
}

TEST_CASE("Worker reply storage is owned after caller and CBOR buffers change or die", tags) {
    auto r = nativeRequest(); auto d = reply(r, nativeValue()); const auto original = d;
    auto encoded = Reply::encode(d, r); REQUIRE(encoded); auto decoded = Reply::decode(*encoded, r); REQUIRE(decoded);
    auto& source = *std::get<Reply::Native>(d.result).curve;
    source.cache.controlPoints[0].x = 123; source.cache.knotslist[0] = 7; source.grips[0].y = 456;
    *encoded = {}; same(*decoded, original);
    auto& owned = *std::get<Reply::Native>(decoded->result).curve;
    CHECK(owned.cache.controlPoints.data() != source.cache.controlPoints.data());
    CHECK(owned.cache.knotslist.data() != source.cache.knotslist.data()); CHECK(owned.grips.data() != source.grips.data());
    owned.cache.controlPoints[0].x = 999; CHECK(source.cache.controlPoints[0].x == 123);
    auto [tr, t] = trimLine(); const auto before = t;
    auto wire = Reply::encode(reply(tr, t), tr); REQUIRE(wire);
    auto retained = Reply::decode(*wire, tr); REQUIRE(retained);
    t.fragments[0].sourceSections[0].from = .75; t.fragments[0].data.controlPoints[0].x = 888;
    t.roots.clear(); wire.reset(); same(std::get<Trim::Result>(retained->result), before);
    auto temporary = [&] { return Reply::decode(*Reply::encode(reply(tr, before), tr), tr); }();
    REQUIRE(temporary); same(std::get<Trim::Result>(temporary->result), before);
}
