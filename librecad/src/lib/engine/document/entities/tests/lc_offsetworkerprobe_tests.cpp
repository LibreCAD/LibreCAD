/*
 * This file is part of LibreCAD, a 2D CAD program.
 * Copyright (C) 2026 LibreCAD.org
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "lc_offsetcurve.h"
#include "lc_offsetworkerrequest.h"
#include "lc_offsetworkerwire.h"
#include "rs_debug.h"

#include <catch2/catch_test_macros.hpp>
#include <QJsonDocument>
#include <QJsonObject>
#include <chrono>
#include <cmath>
#include <iostream>

namespace {
using Clock = std::chrono::steady_clock;
using Work = LC_EllipseOffsetGeometry::CheckWork;
namespace request = LC_OffsetWorkerRequest;
namespace wire = LC_OffsetWorkerWire;
struct Transported {
    request::Request request;
    qsizetype frameBytes;
    std::size_t decodedWireItems;
};
Transported transfer(const request::Request& value) {
    const auto fields = request::encode(value);
    REQUIRE(fields);
    const auto frame = wire::encode(*fields);
    REQUIRE_FALSE(frame.isEmpty());
    wire::Receiver receiver;
    REQUIRE(receiver.append(frame));
    const auto message = receiver.finish();
    REQUIRE(message);
    auto decoded = request::decode(message.fields);
    REQUIRE(decoded);
    return {std::move(*decoded), frame.size(), message.work.items};
}
struct DebugSilencer {
    RS_Debug::RS_DebugLevel previous = RS_DEBUG->getLevel();
    DebugSilencer() { RS_DEBUG->setLevel(RS_Debug::D_NOTHING); }
    ~DebugSilencer() { RS_DEBUG->setLevel(previous); }
};
void record(const QString& id, Clock::time_point start, bool admitted,
            int refusal, int checkReason, const Work& work, std::size_t controls,
            std::size_t engineRuns, const Transported& transported) {
    const auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(Clock::now() - start).count();
    REQUIRE(elapsed >= 0);
    const QJsonObject row{{"probe", id}, {"elapsed_us", qint64(elapsed)},
        {"admitted", admitted}, {"refusal", refusal}, {"check_reason", checkReason},
        {"cache_spans", qint64(work.cacheSpans)}, {"vertices", qint64(work.vertices)},
        {"pairs", qint64(work.distancePairs)}, {"ordering", qint64(work.orderingComparisons)},
        {"depth", int(work.deepest)}, {"controls", qint64(controls)},
        {"engine_runs", qint64(engineRuns)}, {"request_frame_bytes", transported.frameBytes},
        {"decoded_wire_items", qint64(transported.decodedWireItems)}};
    std::cout << QJsonDocument(row).toJson(QJsonDocument::Compact).constData() << '\n';
}
RS_SplineData circleCache(std::size_t degree, std::size_t spans) {
    RS_SplineData cache{int(degree), false};
    cache.knotslist.assign(degree + 1, 0.0);
    constexpr double radius = 101.0;
    const double step = .001 / double(spans);
    const auto point = [](double angle) { return RS_Vector{radius * std::cos(angle), radius * std::sin(angle)}; };
    const auto tangent = [](double angle) { return RS_Vector{-radius * std::sin(angle), radius * std::cos(angle)}; };
    cache.controlPoints.push_back(point(0));
    for (std::size_t i = 0; i < spans; ++i) {
        const double from = double(i) * step, to = double(i + 1) * step;
        if (degree == 3) {
            cache.controlPoints.push_back(point(from) + tangent(from) * (step / 3));
            cache.controlPoints.push_back(point(to) - tangent(to) * (step / 3));
        }
        cache.controlPoints.push_back(point(to));
        cache.knotslist.insert(cache.knotslist.end(), i + 1 == spans ? degree + 1 : degree, double(i + 1));
    }
    cache.weights.assign(cache.controlPoints.size(), 1.0);
    return cache;
}
void bounded(const Work& work) {
    const LC_OffsetCheckLimits hard;
    CHECK(work.cacheSpans <= hard.maxCacheSpans);
    CHECK(work.vertices <= hard.maxVertices);
    CHECK(work.distancePairs <= hard.maxDistancePairs);
    CHECK(work.orderingComparisons <= hard.maxOrderingComparisons);
    CHECK(work.deepest <= hard.maxDepth);
}
}

// Explicit opt-in probes report measurements and typed refusals. They never
// turn timing on a noisy host into geometric acceptance or raise a work cap.
TEST_CASE("Native offset worker probes actual cache admission through the hard span boundary",
          "[.][lc-offsetcurve][worker-probe]") {
    const DebugSilencer silence;
    LC_EllipseOffsetDefinition definition;
    definition.basis = {{0, 0}, {100, 0}, 1, 0, .001, false};
    definition.distance = 1; definition.relTolerance = 1e-4;
    for (const std::size_t degree : {1u, 3u}) for (const std::size_t spans : {1u, 64u, 256u, 1024u, 4096u}) {
        CAPTURE(degree, spans);
        const auto data = circleCache(degree, spans);
        const auto stamp = RS_Spline::geometryFingerprint(data);
        const auto start = Clock::now();
        const auto transported = transfer({{qint64(degree * 10000 + spans), 0, 0, 0},
            request::ValidateCache{data, definition, .01, {}}});
        const auto& operation = std::get<request::ValidateCache>(transported.request.operation);
        const RS_Spline stored{nullptr, operation.cache};
        LC_OffsetCheckResult check;
        LC_OffsetCurveRefusal refusal;
        const auto curve = LC_OffsetCurve::adopt(stored, operation.definition,
            operation.worldTolerance, &check, operation.limits, &refusal);
        record(QStringLiteral("adopt-degree-%1-spans-%2").arg(degree).arg(spans), start,
            bool(curve), int(refusal), int(check.reason), check.work, data.controlPoints.size(), 0, transported);
        bounded(check.work);
        CHECK(RS_Spline::geometryFingerprint(data) == stamp);
        CHECK(RS_Spline::geometryFingerprint(stored.getData()) == stamp);
        if (curve) {
            CHECK(curve->hasEditableDefinition());
            CHECK(RS_Spline::geometryFingerprint(curve->getData()) == stamp);
        } else {
            CHECK(refusal != LC_OffsetCurveRefusal::None);
            if (check.reason != LC_OffsetCheckResult::Reason::None) CHECK_FALSE(check.analysis);
            else {
                REQUIRE(check.analysis);
                CHECK(refusal == LC_OffsetCurveRefusal::NotCacheRepresentable);
            }
        }
        if (spans == 1) REQUIRE(curve);
    }
}

TEST_CASE("Native offset worker probes generation without substituting stored geometry",
          "[.][lc-offsetcurve][worker-probe]") {
    const DebugSilencer silence;
    for (const int family : {0, 1, 2}) {
        CAPTURE(family);
        RS_EllipseData basis{{0, 0}, {100, 0}, family == 0 ? 1.0 : .5, 0, 0, false};
        const RS_Ellipse source{nullptr, basis};
        const auto start = Clock::now();
        const LC_EllipseOffsetDefinition definition{source.getData(), family == 2 ? 49.0 : 1.0,
            family == 2, false, LC_CurveOffset::kEllipseRelativeOffsetTolerance};
        const auto transported = transfer({{qint64(family + 1), 0, 0, 0}, request::Generate{definition, {}}});
        const auto& operation = std::get<request::Generate>(transported.request.operation);
        const RS_Ellipse decodedSource{nullptr, operation.definition.basis};
        const auto result = LC_OffsetCurve::create(decodedSource, operation.definition.distance,
            operation.definition.towardCentre, operation.limits, operation.definition.relTolerance,
            operation.definition.cacheReversed);
        record(QStringLiteral("generate-family-%1").arg(family), start, bool(result),
            int(result.refusal), result.checkReason ? int(*result.checkReason) : -1,
            result.checkWork, result.entity ? result.entity->getData().controlPoints.size() : 0,
            result.engineRuns, transported);
        bounded(result.checkWork);
        REQUIRE(result);
        CHECK(result.engineRuns == 1);
        CHECK(result.entity->hasEditableDefinition());
    }
}
