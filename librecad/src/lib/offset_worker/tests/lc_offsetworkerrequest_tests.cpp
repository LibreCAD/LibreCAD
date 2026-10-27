/*
 * This file is part of LibreCAD, a 2D CAD program.
 * Copyright (C) 2026 LibreCAD.org
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "../lc_offsetworkerrequest.h"

#include <catch2/catch_test_macros.hpp>
#include <QCborMap>
#include <QCborValue>
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <limits>
#include <type_traits>
#include <utility>
#include <vector>

namespace request = LC_OffsetWorkerRequest;
namespace trim = LC_OffsetCacheTrim;
namespace {
RS_Vector point(double x = -0.0, double y = 2.5) {
    RS_Vector p;
    p.x = x; p.y = y; p.z = -0.0; p.valid = true;
    return p;
}
RS_EllipseData ellipse() {
    RS_EllipseData d;
    d.center = point(); d.majorP = point(3.5, -7.25);
    d.ratio = 2.5; d.angle1 = -11.25; d.angle2 = 25.5; d.reversed = true;
    d.isArc = true; d.angleDegrees = -0.0; d.startAngleDegrees = -93.25;
    d.otherAngleDegrees = 721.5; d.angularLength = -1.25;
    return d;
}
RS_ArcData arc() {
    RS_ArcData d;
    d.center = point(); d.radius = 7.5; d.angle1 = -9.25; d.angle2 = 21.75;
    d.reversed = true; d.startAngleDegrees = -0.0; d.otherAngleDegrees = 381.25;
    d.angularLength = -17.5;
    return d;
}
RS_CircleData circle() {
    RS_CircleData d;
    d.center = point(); d.radius = 8.75;
    return d;
}
RS_SplineData spline() {
    RS_SplineData d;
    // Intentionally raw/inconsistent: the codec must not repair or fit these records.
    d.controlPoints = {point(), point(4.25, -0.0), point(-7.5, 9.25)};
    d.knotslist = {-0.0, 2.75, -3.5, 11.25};
    d.weights = {2.5, -0.0}; d.fitPoints = {point(17.5, -0.0), point(-19.25, 23.5)};
    d.degree = 2; d.type = RS_SplineData::SplineType::WrappedClosed;
    d.savedOpenType = RS_SplineData::SplineType::Standard;
    d.savedOpenKnots = {31.25, -0.0, -37.5}; d.m_closedFlag = true; d.m_periodicFlag = true;
    return d;
}
LC_EllipseOffsetDefinition definition() {
    return {ellipse(), 0.75, true, true, 0.00025};
}
LC_OffsetCheckLimits checkLimits() { return {31, 37, 41, 7, 43}; }
LC_OffsetRunLimits runLimits() { return {{11, 13, 17}, 23, 29, checkLimits()}; }
trim::Limits trimLimits() { return {11, 13, 17, 19, 23, 29, 31, 37, 11, 13, 1, 41, 43, 47}; }
trim::Pick pick() { return {point(41.25, -0.0), 4095, -0.0}; }
request::Token token() { return {std::numeric_limits<qint64>::max(), 17, 23, 29}; }
std::vector<request::Request> fixtures() {
    std::vector<request::Request> values{
        {token(), request::Generate{definition(), runLimits()}},
        {token(), request::ValidateCache{spline(), definition(), 0.125, checkLimits()}}
    };
    const std::array<request::Cutter, 5> cutters{
        request::Segment{point(), point(3.25, -0.0)}, arc(), circle(), ellipse(), spline()
    };
    for (const auto& cutter : cutters)
        values.push_back({token(), request::TrimCache{spline(), cutter, pick(), trimLimits()}});
    for (unsigned i = 1; i <= 3; ++i) {
        values.push_back({token(), request::TrimConic{spline(), cutters[i], pick(), pick(), trimLimits()}});
        auto& op = std::get<request::TrimConic>(values.back().operation);
        op.cachePick->rawSpanHint = 0;
        op.cachePick->localParameterSeed = 1.0;
        values.push_back({token(), request::TrimConic{spline(), cutters[i],
            {point(), {}, {}}, {}, trimLimits()}});
    }
    values.push_back({token(), request::Generate{definition(), runLimits(), point(13.25, -0.0)}});
    values.push_back({token(), request::CheckThrough{spline(), definition(), 0.125,
        point(13.25, -0.0), checkLimits()}});
    return values;
}
void same(double a, double b) { CHECK(std::memcmp(&a, &b, sizeof(double)) == 0); }
void same(const RS_Vector& a, const RS_Vector& b) {
    same(a.x, b.x); same(a.y, b.y); same(a.z, b.z); CHECK(a.valid == b.valid);
}
void same(const std::vector<double>& a, const std::vector<double>& b) {
    REQUIRE(a.size() == b.size());
    for (std::size_t i = 0; i < a.size(); ++i) same(a[i], b[i]);
}
void same(const std::vector<RS_Vector>& a, const std::vector<RS_Vector>& b) {
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
void same(const RS_ArcData& a, const RS_ArcData& b) {
    same(a.center, b.center); same(a.radius, b.radius); same(a.angle1, b.angle1); same(a.angle2, b.angle2);
    CHECK(a.reversed == b.reversed); same(a.startAngleDegrees, b.startAngleDegrees);
    same(a.otherAngleDegrees, b.otherAngleDegrees); same(a.angularLength, b.angularLength);
}
void same(const RS_CircleData& a, const RS_CircleData& b) {
    same(a.center, b.center); same(a.radius, b.radius);
}
void same(const RS_SplineData& a, const RS_SplineData& b) {
    same(a.controlPoints, b.controlPoints); same(a.knotslist, b.knotslist); same(a.weights, b.weights);
    same(a.fitPoints, b.fitPoints); CHECK(a.degree == b.degree); CHECK(a.type == b.type);
    CHECK(a.savedOpenType == b.savedOpenType); same(a.savedOpenKnots, b.savedOpenKnots);
    CHECK(a.m_closedFlag == b.m_closedFlag); CHECK(a.m_periodicFlag == b.m_periodicFlag);
}
void same(const LC_EllipseOffsetDefinition& a, const LC_EllipseOffsetDefinition& b) {
    same(a.basis, b.basis); same(a.distance, b.distance); same(a.relTolerance, b.relTolerance);
    CHECK(a.towardCentre == b.towardCentre); CHECK(a.cacheReversed == b.cacheReversed);
}
void same(const LC_OffsetCheckLimits& a, const LC_OffsetCheckLimits& b) {
    CHECK(a.maxCacheSpans == b.maxCacheSpans); CHECK(a.maxVertices == b.maxVertices);
    CHECK(a.maxDistancePairs == b.maxDistancePairs); CHECK(a.maxDepth == b.maxDepth);
    CHECK(a.maxOrderingComparisons == b.maxOrderingComparisons);
}
void same(const LC_OffsetRunLimits& a, const LC_OffsetRunLimits& b) {
    CHECK(a.outputBudget.maxCubicPieces == b.outputBudget.maxCubicPieces);
    CHECK(a.outputBudget.maxOutputEntities == b.outputBudget.maxOutputEntities);
    CHECK(a.outputBudget.maxDeepEntities == b.outputBudget.maxDeepEntities);
    CHECK(a.maxSamples == b.maxSamples); CHECK(a.maxIntersectionPairs == b.maxIntersectionPairs);
    same(a.checkLimits, b.checkLimits);
}
void same(const trim::Limits& a, const trim::Limits& b) {
    CHECK(a.maxSpansPerCurve == b.maxSpansPerCurve); CHECK(a.maxInputSpans == b.maxInputSpans);
    CHECK(a.maxInputRecords == b.maxInputRecords); CHECK(a.maxBoxPairs == b.maxBoxPairs);
    CHECK(a.maxLeaves == b.maxLeaves); CHECK(a.maxGeneratedItems == b.maxGeneratedItems);
    CHECK(a.maxOrderingComparisons == b.maxOrderingComparisons); CHECK(a.maxRoots == b.maxRoots);
    CHECK(a.maxDepth == b.maxDepth); CHECK(a.maxLeafDepth == b.maxLeafDepth);
    CHECK(a.maxFragments == b.maxFragments); CHECK(a.maxOutputSpans == b.maxOutputSpans);
    CHECK(a.maxOutputControls == b.maxOutputControls); CHECK(a.maxOutputKnots == b.maxOutputKnots);
}
void same(const trim::Pick& a, const trim::Pick& b) {
    same(a.worldPoint, b.worldPoint); CHECK(a.rawSpanHint == b.rawSpanHint);
    REQUIRE(a.localParameterSeed.has_value() == b.localParameterSeed.has_value());
    if (a.localParameterSeed) same(*a.localParameterSeed, *b.localParameterSeed);
}
void same(const request::Segment& a, const request::Segment& b) {
    same(a.start, b.start); same(a.end, b.end);
}
void same(const request::Cutter& a, const request::Cutter& b) {
    REQUIRE(a.index() == b.index());
    std::visit([&b](const auto& data) { same(data, std::get<std::decay_t<decltype(data)>>(b)); }, a);
}
void same(const request::Generate& a, const request::Generate& b) {
    same(a.definition, b.definition); same(a.limits, b.limits);
    REQUIRE(a.throughPoint.has_value() == b.throughPoint.has_value());
    if (a.throughPoint) same(*a.throughPoint, *b.throughPoint);
}
void same(const request::ValidateCache& a, const request::ValidateCache& b) {
    same(a.cache, b.cache); same(a.definition, b.definition);
    same(a.worldTolerance, b.worldTolerance); same(a.limits, b.limits);
}
void same(const request::TrimCache& a, const request::TrimCache& b) {
    same(a.cache, b.cache); same(a.cutter, b.cutter); same(a.pick, b.pick); same(a.limits, b.limits);
}
void same(const request::TrimConic& a, const request::TrimConic& b) {
    same(a.cache, b.cache); same(a.conic, b.conic); same(a.conicPick, b.conicPick); same(a.limits, b.limits);
    REQUIRE(a.cachePick.has_value() == b.cachePick.has_value());
    if (a.cachePick) same(*a.cachePick, *b.cachePick);
}
void same(const request::CheckThrough& a, const request::CheckThrough& b) {
    same(a.cache, b.cache); same(a.definition, b.definition); same(a.worldTolerance, b.worldTolerance);
    same(a.point, b.point); same(a.limits, b.limits);
}
void same(const request::Request& a, const request::Request& b) {
    CHECK(a.token.requestId == b.token.requestId); CHECK(a.token.generation == b.token.generation);
    CHECK(a.token.sourceRevision == b.token.sourceRevision); CHECK(a.token.undoEpoch == b.token.undoEpoch);
    REQUIRE(a.operation.index() == b.operation.index());
    std::visit([&b](const auto& op) { same(op, std::get<std::decay_t<decltype(op)>>(b.operation)); }, a.operation);
}
void sameTree(const QCborValue& a, const QCborValue& b) {
    REQUIRE(a.type() == b.type());
    if (a.isArray()) {
        const auto aa = a.toArray(), bb = b.toArray();
        REQUIRE(aa.size() == bb.size());
        for (qsizetype i = 0; i < aa.size(); ++i) sameTree(aa.at(i), bb.at(i));
    } else if (a.isDouble()) same(a.toDouble(), b.toDouble());
    else CHECK(a == b);
}
QCborArray encoded(const request::Request& value) {
    const auto fields = request::encode(value);
    REQUIRE(fields);
    return *fields;
}
void roundTrip(const request::Request& value) {
    const auto fields = encoded(value);
    const auto decoded = request::decode(fields);
    REQUIRE(decoded);
    same(value, *decoded);
    sameTree(fields, encoded(*decoded));
}
using Path = std::vector<qsizetype>;
QCborValue at(QCborValue node, const Path& path) {
    for (auto index : path) { REQUIRE(node.isArray()); node = node.toArray().at(index); }
    return node;
}
QCborValue replace(QCborValue node, const Path& path, const QCborValue& value, std::size_t depth = 0) {
    if (depth == path.size()) return value;
    REQUIRE(node.isArray());
    auto a = node.toArray();
    a[path[depth]] = replace(a.at(path[depth]), path, value, depth + 1);
    return a;
}
QCborArray changed(const QCborArray& root, const Path& path, const QCborValue& value) {
    return replace(root, path, value).toArray();
}
bool variableArray(const Path& path, const std::vector<Path>& splines) {
    for (const auto& base : splines) {
        if (path.size() != base.size() + 1 || !std::equal(base.begin(), base.end(), path.begin())) continue;
        if (path.back() == 0 || path.back() == 1 || path.back() == 2 || path.back() == 3 || path.back() == 7)
            return true;
    }
    return false;
}
void malformedNodes(const QCborArray& root, const QCborValue& node, Path path,
                    const std::vector<Path>& splines) {
    CAPTURE(path);
    CHECK_FALSE(request::decode(changed(root, path, QStringLiteral("wrong type"))));
    CHECK_FALSE(request::decode(changed(root, path, QCborValue(QCborValue::Undefined))));
    CHECK_FALSE(request::decode(changed(root, path, QCborMap{})));
    if (node.isArray()) {
        const auto a = node.toArray();
        if (!variableArray(path, splines)) {
            auto extra = a; extra.append(nullptr);
            CHECK_FALSE(request::decode(changed(root, path, extra)));
            if (!a.empty()) {
                auto missing = a; missing.removeLast();
                CHECK_FALSE(request::decode(changed(root, path, missing)));
            }
        }
        for (qsizetype i = 0; i < a.size(); ++i) {
            auto child = path; child.push_back(i);
            malformedNodes(root, a.at(i), std::move(child), splines);
        }
    } else if (node.isDouble()) {
        CHECK_FALSE(request::decode(changed(root, path, qint64{0})));
        for (double bad : {std::numeric_limits<double>::infinity(),
                           -std::numeric_limits<double>::infinity(), std::numeric_limits<double>::quiet_NaN()})
            CHECK_FALSE(request::decode(changed(root, path, bad)));
    } else if (node.isInteger()) {
        CHECK_FALSE(request::decode(changed(root, path, double(node.toInteger()))));
        CHECK_FALSE(request::decode(changed(root, path, true)));
    } else if (node.isBool()) CHECK_FALSE(request::decode(changed(root, path, 1)));
}
void limitNodes(const QCborArray& root, const QCborValue& node, Path path) {
    if (node.isArray()) {
        const auto a = node.toArray();
        for (qsizetype i = 0; i < a.size(); ++i) {
            auto child = path; child.push_back(i);
            limitNodes(root, a.at(i), std::move(child));
        }
    } else {
        CAPTURE(path);
        REQUIRE(node.isInteger());
        REQUIRE(request::decode(changed(root, path, 0)));
        CHECK_FALSE(request::decode(changed(root, path, node.toInteger() + qint64{1})));
        CHECK_FALSE(request::decode(changed(root, path, qint64{-1})));
        CHECK_FALSE(request::decode(changed(root, path, double(node.toInteger()))));
    }
}
const std::array<std::size_t LC_OffsetCheckLimits::*, 4> checkMembers{
    &LC_OffsetCheckLimits::maxCacheSpans, &LC_OffsetCheckLimits::maxVertices,
    &LC_OffsetCheckLimits::maxDistancePairs, &LC_OffsetCheckLimits::maxOrderingComparisons
};
const std::array<std::size_t trim::Limits::*, 12> trimMembers{
    &trim::Limits::maxSpansPerCurve, &trim::Limits::maxInputSpans, &trim::Limits::maxInputRecords,
    &trim::Limits::maxBoxPairs, &trim::Limits::maxLeaves, &trim::Limits::maxGeneratedItems,
    &trim::Limits::maxOrderingComparisons, &trim::Limits::maxRoots, &trim::Limits::maxFragments,
    &trim::Limits::maxOutputSpans, &trim::Limits::maxOutputControls, &trim::Limits::maxOutputKnots
};
}

TEST_CASE("offset worker request owns and bitwise round trips every operation and cutter",
          "[lc-offsetcurve][worker-protocol][request]") {
    for (const auto& value : fixtures()) {
        CAPTURE(value.operation.index());
        roundTrip(value);
    }
    const auto expected = fixtures().at(6); // TrimCache with a spline cutter.
    std::optional<request::Request> decoded;
    {
        auto input = expected;
        auto fields = encoded(input);
        decoded = request::decode(fields);
        REQUIRE(decoded);
        std::get<request::TrimCache>(input.operation).cache.controlPoints.clear();
        std::get<RS_SplineData>(std::get<request::TrimCache>(input.operation).cutter).savedOpenKnots.clear();
        fields.clear();
    }
    same(expected, *decoded);
    auto moved = std::move(*decoded);
    decoded.reset();
    same(expected, moved);
    roundTrip(moved);
    QCborArray retained;
    {
        auto input = expected;
        retained = encoded(input);
    }
    const auto later = request::decode(retained);
    REQUIRE(later);
    same(expected, *later);
}

TEST_CASE("offset worker request has exact version two positional shapes",
          "[lc-offsetcurve][worker-protocol][request]") {
    for (const auto& value : fixtures()) {
        const auto root = encoded(value);
        REQUIRE(root.size() == 4); CHECK(root.at(0).isInteger()); CHECK(root.at(0).toInteger() == 2);
        const auto t = root.at(1).toArray();
        REQUIRE(t.size() == 4); CHECK(t.at(0).toInteger() == value.token.requestId);
        CHECK(t.at(1).toInteger() == value.token.generation);
        CHECK(t.at(2).toInteger() == value.token.sourceRevision); CHECK(t.at(3).toInteger() == value.token.undoEpoch);
        CHECK(root.at(2).isInteger()); CHECK(root.at(2).toInteger() == qint64(value.operation.index()));
        const auto body = root.at(3).toArray();
        if (value.operation.index() == 0) {
            REQUIRE(body.size() == 3);
            const auto& generated = std::get<request::Generate>(value.operation);
            if (generated.throughPoint) REQUIRE(body.at(2).toArray().size() == 4);
            else CHECK(body.at(2).isNull());
            REQUIRE(body.at(0).toArray().size() == 5);
            CHECK(at(root, {3, 0, 0}).toArray().size() == 11);
            CHECK(at(root, {3, 1}).toArray().size() == 4);
            CHECK(at(root, {3, 1, 0}).toArray().size() == 3);
            CHECK(at(root, {3, 1, 3}).toArray().size() == 5);
        } else {
            REQUIRE(body.size() == (value.operation.index() >= 3 ? 5 : 4));
            REQUIRE(body.at(0).toArray().size() == 10);
            const auto& source = value.operation;
            std::visit([&](const auto& op) {
                if constexpr (!std::is_same_v<std::decay_t<decltype(op)>, request::Generate>) {
                    CHECK(at(root, {3, 0, 0}).toArray().size() == qsizetype(op.cache.controlPoints.size()));
                    CHECK(at(root, {3, 0, 1}).toArray().size() == qsizetype(op.cache.knotslist.size()));
                    CHECK(at(root, {3, 0, 2}).toArray().size() == qsizetype(op.cache.weights.size()));
                    CHECK(at(root, {3, 0, 3}).toArray().size() == qsizetype(op.cache.fitPoints.size()));
                    CHECK(at(root, {3, 0, 7}).toArray().size() == qsizetype(op.cache.savedOpenKnots.size()));
                }
            }, source);
            if (value.operation.index() == 4) {
                CHECK(body.at(1).toArray().size() == 5);
                CHECK(body.at(2).isDouble());
                CHECK(body.at(3).toArray().size() == 4);
                CHECK(body.at(4).toArray().size() == 5);
                const auto& op = std::get<request::CheckThrough>(value.operation);
                sameTree(body.at(1), *request::Values::encodeDefinition(op.definition));
                same(body.at(2).toDouble(), op.worldTolerance);
                sameTree(body.at(3), QCborArray{op.point.x, op.point.y, op.point.z, op.point.valid});
            } else if (value.operation.index() >= 2) {
                CHECK(at(root, {3, 1}).toArray().size() == 2);
                const auto tag = at(root, {3, 1, 0}).toInteger();
                const std::array<qsizetype, 5> arities{2, 8, 2, 11, 10};
                REQUIRE(tag >= 0); REQUIRE(tag < 5);
                CHECK(at(root, {3, 1, 1}).toArray().size() == arities[std::size_t(tag)]);
                CHECK(at(root, {3, 2}).toArray().size() == 3);
                CHECK(body.last().toArray().size() == 14);
            }
        }
    }
}

TEST_CASE("offset worker request retains raw finite extremes and all spline metadata alternatives",
          "[lc-offsetcurve][worker-protocol][request]") {
    auto data = spline();
    data.controlPoints.front().x = std::numeric_limits<double>::max();
    data.controlPoints.front().y = std::numeric_limits<double>::lowest();
    data.weights.front() = std::numeric_limits<double>::denorm_min();
    data.knotslist.front() = -std::numeric_limits<double>::denorm_min();
    data.fitPoints.front().x = std::numeric_limits<double>::min();
    data.savedOpenKnots.front() = -std::numeric_limits<double>::min();
    for (int type = 0; type < 3; ++type) {
        for (int saved = 0; saved < 3; ++saved) {
            for (unsigned flags = 0; flags < 4; ++flags) {
                data.type = static_cast<RS_SplineData::SplineType>(type);
                data.savedOpenType = static_cast<RS_SplineData::SplineType>(saved);
                data.m_closedFlag = flags & 1;
                data.m_periodicFlag = flags & 2;
                roundTrip({{1, 0, 0, 0}, request::ValidateCache{data, definition(), -0.0, {}}});
            }
        }
    }
    // Degree is a representable nonnegative count, not a claim of kernel support.
    data.degree = 0;
    roundTrip({token(), request::ValidateCache{data, definition(), 0.125, {}}});
    const auto maximum = std::min(std::uintmax_t(std::numeric_limits<std::size_t>::max()),
                                  std::uintmax_t(std::numeric_limits<qint64>::max()));
    data.degree = static_cast<std::size_t>(maximum);
    roundTrip({token(), request::ValidateCache{data, definition(), 0.125, {}}});
}

TEST_CASE("offset worker request rejects malformed types arities and nonfinite values at every node",
          "[lc-offsetcurve][worker-protocol][request]") {
    for (const auto& value : fixtures()) {
        const auto root = encoded(value);
        std::vector<Path> splines;
        if (value.operation.index() != 0) splines.push_back({3, 0});
        if (value.operation.index() == 2 && std::get<request::TrimCache>(value.operation).cutter.index() == 4)
            splines.push_back({3, 1, 1});
        malformedNodes(root, root, {}, splines);
    }
}

TEST_CASE("offset worker request rejects invalid tokens versions operations enums and overflow",
          "[lc-offsetcurve][worker-protocol][request]") {
    auto value = fixtures().front();
    const auto root = encoded(value);
    for (qint64 bad : std::array<qint64, 5>{-1, 0, 1, 3, std::numeric_limits<qint64>::max()})
        CHECK_FALSE(request::decode(changed(root, {0}, bad)));
    for (qint64 bad : std::array<qint64, 3>{-1, 5, std::numeric_limits<qint64>::max()})
        CHECK_FALSE(request::decode(changed(root, {2}, bad)));
    for (qsizetype i = 0; i < 4; ++i) {
        CAPTURE(i);
        CHECK_FALSE(request::decode(changed(root, {1, i}, qint64{-1})));
        auto maximum = changed(root, {1, i}, std::numeric_limits<qint64>::max());
        REQUIRE(request::decode(maximum));
        if (i == 0) {
            value.token.requestId = 0; CHECK_FALSE(request::encode(value));
            CHECK_FALSE(request::decode(changed(root, {1, i}, 0)));
        } else REQUIRE(request::decode(changed(root, {1, i}, 0)));
        value = fixtures().front();
        if (i == 0) value.token.requestId = -1;
        if (i == 1) value.token.generation = -1;
        if (i == 2) value.token.sourceRevision = -1;
        if (i == 3) value.token.undoEpoch = -1;
        CHECK_FALSE(request::encode(value));
    }
    auto cache = fixtures().at(1);
    const auto cacheRoot = encoded(cache);
    for (qsizetype field : {5, 6}) {
        for (qint64 bad : std::array<qint64, 3>{-1, 3, std::numeric_limits<qint64>::max()})
            CHECK_FALSE(request::decode(changed(cacheRoot, {3, 0, field}, bad)));
        for (qint64 admitted : {0, 1, 2}) REQUIRE(request::decode(changed(cacheRoot, {3, 0, field}, admitted)));
        auto& d = std::get<request::ValidateCache>(cache.operation).cache;
        if (field == 5) d.type = static_cast<RS_SplineData::SplineType>(3);
        else d.savedOpenType = static_cast<RS_SplineData::SplineType>(-1);
        CHECK_FALSE(request::encode(cache));
        cache = fixtures().at(1);
    }
    const auto trimRoot = encoded(fixtures().at(2));
    for (qint64 bad : std::array<qint64, 3>{-1, 5, std::numeric_limits<qint64>::max()})
        CHECK_FALSE(request::decode(changed(trimRoot, {3, 1, 0}, bad)));
    for (const auto& raw : {QByteArray::fromHex("1b8000000000000000"),
                            QByteArray::fromHex("3b8000000000000000"),
                            QByteArray::fromHex("1bffffffffffffffff")}) {
        const auto overflow = QCborValue::fromCbor(raw);
        CHECK_FALSE(request::decode(changed(root, {1, 0}, overflow)));
        CHECK_FALSE(request::decode(changed(cacheRoot, {3, 0, 4}, overflow)));
        CHECK_FALSE(request::decode(changed(root, {3, 1, 1}, overflow)));
    }
    CHECK_FALSE(request::decode(changed(cacheRoot, {3, 0, 4}, qint64{-1})));
    if (std::uintmax_t(std::numeric_limits<std::size_t>::max()) >
        std::uintmax_t(std::numeric_limits<qint64>::max())) {
        auto& d = std::get<request::ValidateCache>(cache.operation).cache;
        d.degree = std::numeric_limits<std::size_t>::max();
        CHECK_FALSE(request::encode(cache));
    } else if (sizeof(std::size_t) < sizeof(qint64)) {
        const auto largest = std::uintmax_t(std::numeric_limits<std::size_t>::max());
        const auto overflow = qint64(largest + std::uintmax_t{1});
        CHECK_FALSE(request::decode(changed(cacheRoot, {3, 0, 4}, overflow)));
    }
}

TEST_CASE("offset worker request admits only lowered run check and trim budgets including zero",
          "[lc-offsetcurve][worker-protocol][request]") {
    request::Generate generate{definition(), {}};
    generate.throughPoint = point(17.25, -0.0);
    roundTrip({token(), generate});
    for (const int malformed : {0, 1, 2}) {
        auto bad = generate;
        if (malformed == 0) bad.throughPoint->valid = false;
        if (malformed == 1) bad.throughPoint->z = 1;
        if (malformed == 2) bad.throughPoint->x = std::numeric_limits<double>::infinity();
        CHECK_FALSE(request::encode({token(), bad}));
        const auto root = encoded({token(), generate});
        if (malformed == 0) CHECK_FALSE(request::decode(changed(root, {3, 2, 3}, false)));
        if (malformed == 1) CHECK_FALSE(request::decode(changed(root, {3, 2, 2}, 1.0)));
        if (malformed == 2) CHECK_FALSE(request::decode(changed(root, {3, 2, 0}, bad.throughPoint->x)));
    }
    generate.limits.maxSamples = LC_CurveOffset::kDefaultMaxSamples;
    generate.limits.maxIntersectionPairs = LC_CurveOffset::kDefaultMaxIntersectionPairs;
    const auto root = encoded({token(), generate});
    limitNodes(root, at(root, {3, 1}), {3, 1});
    for (auto member : {&LC_OffsetSourceBudget::maxCubicPieces, &LC_OffsetSourceBudget::maxOutputEntities,
                        &LC_OffsetSourceBudget::maxDeepEntities}) {
        auto op = generate; op.limits.outputBudget.*member = 0; roundTrip({token(), op});
        op = generate; ++(op.limits.outputBudget.*member); CHECK_FALSE(request::encode({token(), op}));
    }
    for (auto member : {&LC_OffsetRunLimits::maxSamples, &LC_OffsetRunLimits::maxIntersectionPairs}) {
        auto op = generate; op.limits.*member = 0; roundTrip({token(), op});
        op = generate; ++(op.limits.*member); CHECK_FALSE(request::encode({token(), op}));
    }
    for (auto member : checkMembers) {
        auto op = generate; op.limits.checkLimits.*member = 0; roundTrip({token(), op});
        op = generate; ++(op.limits.checkLimits.*member); CHECK_FALSE(request::encode({token(), op}));
        request::ValidateCache validate{spline(), definition(), 0.125, {}};
        validate.limits.*member = 0; roundTrip({token(), validate});
        validate.limits = {}; ++(validate.limits.*member); CHECK_FALSE(request::encode({token(), validate}));
    }
    auto op = generate; op.limits.checkLimits.maxDepth = 0; roundTrip({token(), op});
    op = generate; ++op.limits.checkLimits.maxDepth; CHECK_FALSE(request::encode({token(), op}));
    request::ValidateCache validate{spline(), definition(), 0.125, {}};
    const auto validateRoot = encoded({token(), validate});
    limitNodes(validateRoot, at(validateRoot, {3, 3}), {3, 3});
    validate.limits.maxDepth = 0; roundTrip({token(), validate});
    validate.limits = {}; ++validate.limits.maxDepth; CHECK_FALSE(request::encode({token(), validate}));

    request::TrimCache trimmer{spline(), circle(), pick(), {}};
    const auto trimRoot = encoded({token(), trimmer});
    limitNodes(trimRoot, at(trimRoot, {3, 3}), {3, 3});
    for (auto member : trimMembers) {
        auto lowered = trimmer; lowered.limits.*member = 0; roundTrip({token(), lowered});
        auto excess = trimmer; ++(excess.limits.*member); CHECK_FALSE(request::encode({token(), excess}));
    }
    for (auto member : {&trim::Limits::maxDepth, &trim::Limits::maxLeafDepth}) {
        auto lowered = trimmer; lowered.limits.*member = 0; roundTrip({token(), lowered});
        auto excess = trimmer; ++(excess.limits.*member); CHECK_FALSE(request::encode({token(), excess}));
    }
    const request::TrimConic conic{spline(), ellipse(), pick(), pick(), {}};
    const auto conicRoot = encoded({token(), conic});
    limitNodes(conicRoot, at(conicRoot, {3, 4}), {3, 4});
    for (auto member : trimMembers) {
        auto excess = conic; ++(excess.limits.*member); CHECK_FALSE(request::encode({token(), excess}));
    }
    for (auto member : {&trim::Limits::maxDepth, &trim::Limits::maxLeafDepth}) {
        auto excess = conic; ++(excess.limits.*member); CHECK_FALSE(request::encode({token(), excess}));
    }
}

TEST_CASE("offset worker request bounds every owned spline array at the exact input cap",
          "[lc-offsetcurve][worker-protocol][request]") {
    for (unsigned field = 0; field < 5; ++field) {
        CAPTURE(field);
        auto d = spline();
        qsizetype index = 0;
        std::size_t cap = 0;
        if (field == 0) { index = 0; cap = 16384; d.controlPoints.assign(cap, point()); }
        if (field == 1) { index = 1; cap = 32768; d.knotslist.assign(cap, -0.0); }
        if (field == 2) { index = 2; cap = 16384; d.weights.assign(cap, -0.0); }
        if (field == 3) { index = 3; cap = 4096; d.fitPoints.assign(cap, point()); }
        if (field == 4) { index = 7; cap = 32768; d.savedOpenKnots.assign(cap, -0.0); }
        request::Request value{token(), request::ValidateCache{d, definition(), 0.125, {}}};
        roundTrip(value);
        const auto root = encoded(value);
        auto array = at(root, {3, 0, index}).toArray();
        REQUIRE(array.size() == qsizetype(cap));
        array.append(array.last());
        CHECK_FALSE(request::decode(changed(root, {3, 0, index}, array)));
        auto& data = std::get<request::ValidateCache>(value.operation).cache;
        if (field == 0) data.controlPoints.push_back(point());
        if (field == 1) data.knotslist.push_back(-0.0);
        if (field == 2) data.weights.push_back(-0.0);
        if (field == 3) data.fitPoints.push_back(point());
        if (field == 4) data.savedOpenKnots.push_back(-0.0);
        CHECK_FALSE(request::encode(value));
        const request::Request cutter{token(), request::TrimCache{spline(), data, pick(), {}}};
        CHECK_FALSE(request::encode(cutter));
        auto cutterRoot = encoded({token(), request::TrimCache{spline(), d, pick(), {}}});
        CHECK_FALSE(request::decode(changed(cutterRoot, {3, 1, 1, index}, array)));
    }
}

TEST_CASE("offset worker request checks latent caller fields and never drops malformed data",
          "[lc-offsetcurve][worker-protocol][request]") {
    const auto inf = std::numeric_limits<double>::infinity();
    for (unsigned field = 0; field < 10; ++field) {
        CAPTURE(field);
        auto d = spline();
        if (field == 0) d.controlPoints.front().x = inf;
        if (field == 1) d.knotslist.front() = inf;
        if (field == 2) d.weights.front() = inf;
        if (field == 3) d.fitPoints.front().valid = false;
        if (field == 4) d.savedOpenKnots.front() = inf;
        if (field == 5) d.controlPoints.back().z = 1;
        if (field == 6) d.fitPoints.back().z = inf;
        if (field == 7) d.savedOpenType = static_cast<RS_SplineData::SplineType>(42);
        if (field == 8) d.type = static_cast<RS_SplineData::SplineType>(-1);
        if (field == 9) d.fitPoints.back().y = std::numeric_limits<double>::quiet_NaN();
        CHECK_FALSE(request::encode({token(), request::ValidateCache{d, definition(), 0.125, {}}}));
        CHECK_FALSE(request::encode({token(), request::TrimCache{spline(), d, pick(), {}}}));
    }
    for (auto member : {&RS_EllipseData::ratio, &RS_EllipseData::angle1, &RS_EllipseData::angle2,
                        &RS_EllipseData::angleDegrees, &RS_EllipseData::startAngleDegrees,
                        &RS_EllipseData::otherAngleDegrees, &RS_EllipseData::angularLength}) {
        auto d = definition(); d.basis.*member = inf;
        CHECK_FALSE(request::encode({token(), request::Generate{d, {}}}));
        CHECK_FALSE(request::encode({token(), request::TrimCache{spline(), d.basis, pick(), {}}}));
    }
    for (auto member : {&RS_ArcData::radius, &RS_ArcData::angle1, &RS_ArcData::angle2,
                        &RS_ArcData::startAngleDegrees, &RS_ArcData::otherAngleDegrees, &RS_ArcData::angularLength}) {
        auto d = arc(); d.*member = inf;
        CHECK_FALSE(request::encode({token(), request::TrimCache{spline(), d, pick(), {}}}));
    }
    auto c = circle(); c.radius = inf;
    CHECK_FALSE(request::encode({token(), request::TrimCache{spline(), c, pick(), {}}}));
    for (auto& value : fixtures()) {
        const auto root = encoded(value);
        if (value.operation.index() == 0) continue;
        CHECK_FALSE(request::decode(changed(root, {3, 0, 0, 0, 2}, 1.0)));
        CHECK_FALSE(request::decode(changed(root, {3, 0, 0, 0, 3}, false)));
    }
}

TEST_CASE("offset worker request bounds definition scalars picks and conic variants without normalization",
          "[lc-offsetcurve][worker-protocol][request]") {
    request::Generate generate{definition(), {}};
    for (double bad : {-1.0, -0.0, 0.0}) {
        auto op = generate; op.definition.distance = bad; CHECK_FALSE(request::encode({token(), op}));
        op = generate; op.definition.relTolerance = bad; CHECK_FALSE(request::encode({token(), op}));
        op = generate; op.definition.basis.ratio = bad; CHECK_FALSE(request::encode({token(), op}));
        const auto root = encoded({token(), generate});
        CHECK_FALSE(request::decode(changed(root, {3, 0, 1}, bad)));
        CHECK_FALSE(request::decode(changed(root, {3, 0, 4}, bad)));
        CHECK_FALSE(request::decode(changed(root, {3, 0, 0, 2}, bad)));
    }
    // Noncanonical axes, sweep and stale draw caches remain exactly as supplied.
    generate.definition.basis.majorP = point(0, -0.0);
    roundTrip({token(), generate});
    request::TrimCache trimmer{spline(), circle(), pick(), {}};
    for (double seed : {-0.0, 0.0, 0.625, 1.0}) {
        trimmer.pick.localParameterSeed = seed; roundTrip({token(), trimmer});
    }
    const auto root = encoded({token(), trimmer});
    for (double seed : {-0.001, 1.001, std::numeric_limits<double>::infinity()}) {
        trimmer.pick.localParameterSeed = seed; CHECK_FALSE(request::encode({token(), trimmer}));
        CHECK_FALSE(request::decode(changed(root, {3, 2, 2}, seed)));
    }
    trimmer.pick = pick(); trimmer.pick.rawSpanHint = 4096;
    CHECK_FALSE(request::encode({token(), trimmer}));
    for (const QCborValue& bad : {QCborValue(4096), QCborValue(-1), QCborValue(4095.0), QCborValue(true)})
        CHECK_FALSE(request::decode(changed(root, {3, 2, 1}, bad)));
    trimmer.pick = {point(), {}, {}}; roundTrip({token(), trimmer});
    trimmer.pick.worldPoint.valid = false; CHECK_FALSE(request::encode({token(), trimmer}));
    auto conic = std::get<request::TrimConic>(fixtures().at(7).operation);
    for (const request::Cutter& bad : {request::Cutter{request::Segment{point(), point()}}, request::Cutter{spline()}}) {
        conic.conic = bad; CHECK_FALSE(request::encode({token(), conic}));
        const auto badCutter = at(encoded({token(), request::TrimCache{spline(), bad, pick(), {}}}), {3, 1});
        const auto goodRoot = encoded(fixtures().at(7));
        CHECK_FALSE(request::decode(changed(goodRoot, {3, 1}, badCutter)));
    }
    conic = std::get<request::TrimConic>(fixtures().at(7).operation);
    conic.cachePick->localParameterSeed = 1.01; CHECK_FALSE(request::encode({token(), conic}));
    conic = std::get<request::TrimConic>(fixtures().at(7).operation);
    conic.conicPick.rawSpanHint = 4096; CHECK_FALSE(request::encode({token(), conic}));
    request::ValidateCache validate{spline(), definition(), 0.125, {}};
    for (double raw : {-0.0, 0.0, -1.0}) {
        validate.worldTolerance = raw; roundTrip({token(), validate});
    }
    for (double bad : {std::numeric_limits<double>::infinity(), std::numeric_limits<double>::quiet_NaN()}) {
        validate.worldTolerance = bad; CHECK_FALSE(request::encode({token(), validate}));
    }
}

TEST_CASE("offset worker shared value codecs use the request layouts and strict admission",
          "[lc-offsetcurve][worker-protocol][request]") {
    namespace values = request::Values;
    const auto cache = values::encodeCache(spline());
    REQUIRE(cache);
    const auto decodedCache = values::decodeCache(*cache);
    REQUIRE(decodedCache); same(spline(), *decodedCache);
    sameTree(*cache, at(encoded(fixtures().at(1)), {3, 0}));
    auto extraCache = *cache; extraCache.append(nullptr);
    CHECK_FALSE(values::decodeCache(extraCache));
    CHECK_FALSE(values::decodeCache({}));
    CHECK_FALSE(values::decodeCache(changed(*cache, {7, 1}, 0)));
    CHECK_FALSE(values::decodeCache(changed(*cache, {6}, 3)));
    auto invalidCache = spline(); invalidCache.savedOpenKnots.back() = std::numeric_limits<double>::infinity();
    CHECK_FALSE(values::encodeCache(invalidCache));

    const auto def = values::encodeDefinition(definition());
    REQUIRE(def);
    const auto decodedDefinition = values::decodeDefinition(*def);
    REQUIRE(decodedDefinition); same(definition(), *decodedDefinition);
    sameTree(*def, at(encoded(fixtures().front()), {3, 0}));
    auto extraDefinition = *def; extraDefinition.append(nullptr);
    CHECK_FALSE(values::decodeDefinition(extraDefinition));
    CHECK_FALSE(values::decodeDefinition({}));
    CHECK_FALSE(values::decodeDefinition(changed(*def, {0, 2}, 0.0)));
    CHECK_FALSE(values::decodeDefinition(changed(*def, {0, 10}, 0)));
    auto invalidDefinition = definition(); invalidDefinition.basis.angularLength = std::numeric_limits<double>::infinity();
    CHECK_FALSE(values::encodeDefinition(invalidDefinition));

    const auto selection = values::encodePick(pick());
    REQUIRE(selection);
    const auto decodedPick = values::decodePick(*selection);
    REQUIRE(decodedPick); same(pick(), *decodedPick);
    sameTree(*selection, at(encoded(fixtures().at(2)), {3, 2}));
    auto extraPick = *selection; extraPick.append(nullptr);
    CHECK_FALSE(values::decodePick(extraPick));
    CHECK_FALSE(values::decodePick({}));
    CHECK_FALSE(values::decodePick(changed(*selection, {1}, 4096)));
    CHECK_FALSE(values::decodePick(changed(*selection, {2}, 0)));
    auto invalidPick = pick(); invalidPick.rawSpanHint = 4096;
    CHECK_FALSE(values::encodePick(invalidPick));

    for (unsigned i = 2; i <= 6; ++i) {
        const auto input = std::get<request::TrimCache>(fixtures().at(i).operation);
        const auto cutter = values::encodeCutter(input.cutter);
        REQUIRE(cutter);
        const auto decodedCutter = values::decodeCutter(*cutter);
        REQUIRE(decodedCutter); same(input.cutter, *decodedCutter);
        sameTree(*cutter, at(encoded({token(), input}), {3, 1}));
        auto extraCutter = *cutter; extraCutter.append(nullptr);
        CHECK_FALSE(values::decodeCutter(extraCutter));
        CHECK_FALSE(values::decodeCutter(changed(*cutter, {0}, 5)));
        CHECK_FALSE(values::decodeCutter(changed(*cutter, {1}, QCborArray{})));
    }
    CHECK_FALSE(values::decodeCutter({}));
    CHECK_FALSE(values::encodeCutter(request::Cutter{invalidCache}));
}

TEST_CASE("offset worker CheckThrough owns raw values and bounds every membership allowance",
          "[lc-offsetcurve][worker-protocol][request][through]") {
    request::CheckThrough op{spline(), definition(), .125, point(), {}};
    for (double tolerance : {-1.0, -0.0, 0.0, .125}) {
        op.worldTolerance = tolerance;
        roundTrip({token(), op}); // Semantic refusals remain the kernel's responsibility.
    }
    op.worldTolerance = .125;
    const auto root = encoded({token(), op});
    limitNodes(root, at(root, {3, 4}), {3, 4});
    for (auto member : checkMembers) {
        auto bad = op; ++(bad.limits.*member);
        CHECK_FALSE(request::encode({token(), bad}));
        auto zero = op; zero.limits.*member = 0; roundTrip({token(), zero});
    }
    auto bad = op; ++bad.limits.maxDepth; CHECK_FALSE(request::encode({token(), bad}));
    bad = op; bad.limits.maxDepth = 0; roundTrip({token(), bad});
    for (int malformed : {0, 1, 2, 3, 4}) {
        bad = op;
        if (malformed == 0) bad.point.valid = false;
        if (malformed == 1) bad.point.z = 1;
        if (malformed == 2) bad.point.x = std::numeric_limits<double>::infinity();
        if (malformed == 3) bad.worldTolerance = std::numeric_limits<double>::quiet_NaN();
        if (malformed == 4) bad.cache.savedOpenKnots.back() = std::numeric_limits<double>::infinity();
        CHECK_FALSE(request::admitted({token(), bad}));
        CHECK_FALSE(request::encode({token(), bad}));
    }
    auto decoded = request::decode(root); REQUIRE(decoded);
    op.cache.controlPoints.clear(); op.point.x = 99;
    const auto& owned = std::get<request::CheckThrough>(decoded->operation);
    same(owned.cache, spline()); same(owned.point, point());
    CHECK_FALSE(request::decode(changed(root, {0}, 1)));
    CHECK_FALSE(request::decode(changed(root, {3, 2}, 1)));
    CHECK_FALSE(request::decode(changed(root, {3, 3, 2}, 1.0)));
    CHECK_FALSE(request::decode(changed(root, {3, 3, 3}, false)));
    auto body = root.at(3).toArray();
    const auto pointFields = body.at(3); body[3] = body.at(4); body[4] = pointFields;
    CHECK_FALSE(request::decode(changed(root, {3}, body)));
}
