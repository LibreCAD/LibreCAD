/*
 * This file is part of LibreCAD, a 2D CAD program.
 * Copyright (C) 2026 LibreCAD.org
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "lc_offsetworkerrequest.h"

#include <QCborValue>
#include <cmath>
#include <cstdint>
#include <limits>
#include <utility>

namespace LC_OffsetWorkerRequest {
namespace {
constexpr std::size_t ControlCap = 16384;
constexpr std::size_t KnotCap = 32768;
constexpr std::size_t FitCap = 4096;
constexpr std::size_t HintCap = 4095;

bool array(const QCborValue& value, qsizetype count, QCborArray& out) {
    if (!value.isArray()) return false;
    out = value.toArray();
    return out.size() == count;
}
bool number(const QCborValue& value, double& out) {
    if (!value.isDouble()) return false;
    out = value.toDouble();
    return std::isfinite(out);
}
bool boolean(const QCborValue& value, bool& out) {
    if (!value.isBool()) return false;
    out = value.toBool();
    return true;
}
bool integer(const QCborValue& value, qint64& out) {
    if (!value.isInteger()) return false;
    out = value.toInteger();
    return true;
}
bool count(const QCborValue& value, std::size_t cap, std::size_t& out) {
    qint64 raw;
    if (!integer(value, raw) || raw < 0 ||
        std::uintmax_t(raw) > std::uintmax_t(std::numeric_limits<std::size_t>::max()) ||
        std::uintmax_t(raw) > std::uintmax_t(cap)) return false;
    out = static_cast<std::size_t>(raw);
    return true;
}
bool depth(const QCborValue& value, unsigned cap, unsigned& out) {
    std::size_t raw;
    if (!count(value, cap, raw)) return false;
    out = static_cast<unsigned>(raw);
    return true;
}
bool signedSize(std::size_t value) {
    return std::uintmax_t(value) <= std::uintmax_t(std::numeric_limits<qint64>::max());
}
bool valid(const RS_Vector& value) {
    return value.valid && std::isfinite(value.x) && std::isfinite(value.y) &&
        std::isfinite(value.z) && value.z == 0;
}
bool read(const QCborValue& value, RS_Vector& out) {
    QCborArray a;
    return array(value, 4, a) && number(a.at(0), out.x) && number(a.at(1), out.y) &&
        number(a.at(2), out.z) && boolean(a.at(3), out.valid) && valid(out);
}
bool valid(const RS_ArcData& d) {
    return valid(d.center) && std::isfinite(d.radius) && std::isfinite(d.angle1) &&
        std::isfinite(d.angle2) && std::isfinite(d.startAngleDegrees) &&
        std::isfinite(d.otherAngleDegrees) && std::isfinite(d.angularLength);
}
bool read(const QCborValue& value, RS_ArcData& d) {
    QCborArray a;
    return array(value, 8, a) && read(a.at(0), d.center) && number(a.at(1), d.radius) &&
        number(a.at(2), d.angle1) && number(a.at(3), d.angle2) && boolean(a.at(4), d.reversed) &&
        number(a.at(5), d.startAngleDegrees) && number(a.at(6), d.otherAngleDegrees) &&
        number(a.at(7), d.angularLength);
}
bool valid(const RS_CircleData& d) { return valid(d.center) && std::isfinite(d.radius); }
bool read(const QCborValue& value, RS_CircleData& d) {
    QCborArray a;
    return array(value, 2, a) && read(a.at(0), d.center) && number(a.at(1), d.radius);
}
bool valid(const RS_EllipseData& d) {
    return valid(d.center) && valid(d.majorP) && std::isfinite(d.ratio) &&
        std::isfinite(d.angle1) && std::isfinite(d.angle2) && std::isfinite(d.angleDegrees) &&
        std::isfinite(d.startAngleDegrees) && std::isfinite(d.otherAngleDegrees) &&
        std::isfinite(d.angularLength);
}
bool read(const QCborValue& value, RS_EllipseData& d) {
    QCborArray a;
    return array(value, 11, a) && read(a.at(0), d.center) && read(a.at(1), d.majorP) &&
        number(a.at(2), d.ratio) && number(a.at(3), d.angle1) && number(a.at(4), d.angle2) &&
        boolean(a.at(5), d.reversed) && boolean(a.at(6), d.isArc) &&
        number(a.at(7), d.angleDegrees) && number(a.at(8), d.startAngleDegrees) &&
        number(a.at(9), d.otherAngleDegrees) && number(a.at(10), d.angularLength);
}
bool valid(RS_SplineData::SplineType type) {
    switch (type) {
    case RS_SplineData::SplineType::Standard:
    case RS_SplineData::SplineType::ClampedOpen:
    case RS_SplineData::SplineType::WrappedClosed: return true;
    }
    return false;
}
bool splineType(const QCborValue& value, RS_SplineData::SplineType& out) {
    qint64 raw;
    if (!integer(value, raw) || raw < 0 || raw > 2) return false;
    out = static_cast<RS_SplineData::SplineType>(raw);
    return valid(out);
}
bool valid(const std::vector<RS_Vector>& values, std::size_t cap) {
    if (values.size() > cap) return false;
    for (const auto& value : values) if (!valid(value)) return false;
    return true;
}
bool valid(const std::vector<double>& values, std::size_t cap) {
    if (values.size() > cap) return false;
    for (double value : values) if (!std::isfinite(value)) return false;
    return true;
}
bool valid(const RS_SplineData& d) {
    return signedSize(d.degree) && valid(d.type) && valid(d.savedOpenType) &&
        valid(d.controlPoints, ControlCap) && valid(d.knotslist, KnotCap) &&
        valid(d.weights, ControlCap) && valid(d.fitPoints, FitCap) && valid(d.savedOpenKnots, KnotCap);
}
bool boundedArray(const QCborValue& value, std::size_t cap, QCborArray& out) {
    if (!value.isArray()) return false;
    out = value.toArray();
    return std::uintmax_t(out.size()) <= std::uintmax_t(cap);
}
bool readPoints(const QCborArray& a, std::vector<RS_Vector>& out) {
    // Scan before allocating, including values that a kernel might not use.
    RS_Vector value;
    for (const auto& item : a) if (!read(item, value)) return false;
    out.reserve(static_cast<std::size_t>(a.size()));
    for (const auto& item : a) { read(item, value); out.push_back(value); }
    return true;
}
bool readNumbers(const QCborArray& a, std::vector<double>& out) {
    double value;
    for (const auto& item : a) if (!number(item, value)) return false;
    out.reserve(static_cast<std::size_t>(a.size()));
    for (const auto& item : a) { number(item, value); out.push_back(value); }
    return true;
}
bool read(const QCborValue& value, RS_SplineData& d) {
    QCborArray a, controls, knots, weights, fit, saved;
    // Admit ALL five counts before reserving any native array.
    return array(value, 10, a) && boundedArray(a.at(0), ControlCap, controls) &&
        boundedArray(a.at(1), KnotCap, knots) && boundedArray(a.at(2), ControlCap, weights) &&
        boundedArray(a.at(3), FitCap, fit) && boundedArray(a.at(7), KnotCap, saved) &&
        count(a.at(4), std::numeric_limits<std::size_t>::max(), d.degree) &&
        splineType(a.at(5), d.type) && splineType(a.at(6), d.savedOpenType) &&
        boolean(a.at(8), d.m_closedFlag) && boolean(a.at(9), d.m_periodicFlag) &&
        readPoints(controls, d.controlPoints) && readNumbers(knots, d.knotslist) &&
        readNumbers(weights, d.weights) && readPoints(fit, d.fitPoints) &&
        readNumbers(saved, d.savedOpenKnots);
}
bool valid(const LC_EllipseOffsetDefinition& d) {
    return valid(d.basis) && d.basis.ratio > 0 && std::isfinite(d.distance) && d.distance > 0 &&
        std::isfinite(d.relTolerance) && d.relTolerance > 0;
}
bool read(const QCborValue& value, LC_EllipseOffsetDefinition& d) {
    QCborArray a;
    return array(value, 5, a) && read(a.at(0), d.basis) && number(a.at(1), d.distance) &&
        boolean(a.at(2), d.towardCentre) && boolean(a.at(3), d.cacheReversed) &&
        number(a.at(4), d.relTolerance) && valid(d);
}
bool valid(const LC_OffsetCheckLimits& l) {
    const LC_OffsetCheckLimits hard;
    return l.maxCacheSpans <= hard.maxCacheSpans && l.maxVertices <= hard.maxVertices &&
        l.maxDistancePairs <= hard.maxDistancePairs && l.maxDepth <= hard.maxDepth &&
        l.maxOrderingComparisons <= hard.maxOrderingComparisons;
}
bool read(const QCborValue& value, LC_OffsetCheckLimits& l) {
    const LC_OffsetCheckLimits hard;
    QCborArray a;
    return array(value, 5, a) && count(a.at(0), hard.maxCacheSpans, l.maxCacheSpans) &&
        count(a.at(1), hard.maxVertices, l.maxVertices) &&
        count(a.at(2), hard.maxDistancePairs, l.maxDistancePairs) &&
        depth(a.at(3), hard.maxDepth, l.maxDepth) &&
        count(a.at(4), hard.maxOrderingComparisons, l.maxOrderingComparisons);
}
bool valid(const LC_OffsetRunLimits& l) {
    const auto hard = makeDefaultOffsetSourceBudget();
    return l.outputBudget.maxCubicPieces <= hard.maxCubicPieces &&
        l.outputBudget.maxOutputEntities <= hard.maxOutputEntities &&
        l.outputBudget.maxDeepEntities <= hard.maxDeepEntities &&
        l.maxSamples <= LC_CurveOffset::kDefaultMaxSamples &&
        l.maxIntersectionPairs <= LC_CurveOffset::kDefaultMaxIntersectionPairs && valid(l.checkLimits);
}
bool read(const QCborValue& value, LC_OffsetRunLimits& l) {
    const auto hard = makeDefaultOffsetSourceBudget();
    QCborArray a, budget;
    return array(value, 4, a) && array(a.at(0), 3, budget) &&
        count(budget.at(0), hard.maxCubicPieces, l.outputBudget.maxCubicPieces) &&
        count(budget.at(1), hard.maxOutputEntities, l.outputBudget.maxOutputEntities) &&
        count(budget.at(2), hard.maxDeepEntities, l.outputBudget.maxDeepEntities) &&
        count(a.at(1), LC_CurveOffset::kDefaultMaxSamples, l.maxSamples) &&
        count(a.at(2), LC_CurveOffset::kDefaultMaxIntersectionPairs, l.maxIntersectionPairs) &&
        read(a.at(3), l.checkLimits);
}
bool valid(const LC_OffsetCacheTrim::Limits& l) {
    const LC_OffsetCacheTrim::Limits hard;
    return l.maxSpansPerCurve <= hard.maxSpansPerCurve && l.maxInputSpans <= hard.maxInputSpans &&
        l.maxInputRecords <= hard.maxInputRecords && l.maxBoxPairs <= hard.maxBoxPairs &&
        l.maxLeaves <= hard.maxLeaves && l.maxGeneratedItems <= hard.maxGeneratedItems &&
        l.maxOrderingComparisons <= hard.maxOrderingComparisons && l.maxRoots <= hard.maxRoots &&
        l.maxDepth <= hard.maxDepth && l.maxLeafDepth <= hard.maxLeafDepth &&
        l.maxFragments <= hard.maxFragments && l.maxOutputSpans <= hard.maxOutputSpans &&
        l.maxOutputControls <= hard.maxOutputControls && l.maxOutputKnots <= hard.maxOutputKnots;
}
bool read(const QCborValue& value, LC_OffsetCacheTrim::Limits& l) {
    const LC_OffsetCacheTrim::Limits hard;
    QCborArray a;
    return array(value, 14, a) && count(a.at(0), hard.maxSpansPerCurve, l.maxSpansPerCurve) &&
        count(a.at(1), hard.maxInputSpans, l.maxInputSpans) &&
        count(a.at(2), hard.maxInputRecords, l.maxInputRecords) &&
        count(a.at(3), hard.maxBoxPairs, l.maxBoxPairs) && count(a.at(4), hard.maxLeaves, l.maxLeaves) &&
        count(a.at(5), hard.maxGeneratedItems, l.maxGeneratedItems) &&
        count(a.at(6), hard.maxOrderingComparisons, l.maxOrderingComparisons) &&
        count(a.at(7), hard.maxRoots, l.maxRoots) && depth(a.at(8), hard.maxDepth, l.maxDepth) &&
        depth(a.at(9), hard.maxLeafDepth, l.maxLeafDepth) &&
        count(a.at(10), hard.maxFragments, l.maxFragments) &&
        count(a.at(11), hard.maxOutputSpans, l.maxOutputSpans) &&
        count(a.at(12), hard.maxOutputControls, l.maxOutputControls) &&
        count(a.at(13), hard.maxOutputKnots, l.maxOutputKnots);
}
bool valid(const LC_OffsetCacheTrim::Pick& p) {
    return valid(p.worldPoint) && (!p.rawSpanHint || *p.rawSpanHint <= HintCap) &&
        (!p.localParameterSeed || (std::isfinite(*p.localParameterSeed) &&
            *p.localParameterSeed >= 0 && *p.localParameterSeed <= 1));
}
bool read(const QCborValue& value, LC_OffsetCacheTrim::Pick& p) {
    QCborArray a;
    if (!array(value, 3, a) || !read(a.at(0), p.worldPoint)) return false;
    if (!a.at(1).isNull()) {
        std::size_t hint;
        if (!count(a.at(1), HintCap, hint)) return false;
        p.rawSpanHint = hint;
    }
    if (!a.at(2).isNull()) {
        double seed;
        if (!number(a.at(2), seed) || seed < 0 || seed > 1) return false;
        p.localParameterSeed = seed;
    }
    return true;
}
bool valid(const Segment& s) { return valid(s.start) && valid(s.end); }
bool read(const QCborValue& value, Segment& s) {
    QCborArray a;
    return array(value, 2, a) && read(a.at(0), s.start) && read(a.at(1), s.end);
}
bool valid(const Cutter& c) {
    return !c.valueless_by_exception() && std::visit([](const auto& d) { return valid(d); }, c);
}
bool isConic(const Cutter& c) { return c.index() >= 1 && c.index() <= 3; }
bool readCutter(const QCborValue& value, Cutter& c, bool conicOnly = false) {
    QCborArray a;
    qint64 tag;
    if (!array(value, 2, a) || !integer(a.at(0), tag) || tag < 0 || tag > 4 ||
        (conicOnly && (tag < 1 || tag > 3))) return false;
    switch (tag) {
    case 0: { Segment d; if (!read(a.at(1), d)) return false; c = std::move(d); break; }
    case 1: { RS_ArcData d; if (!read(a.at(1), d)) return false; c = std::move(d); break; }
    case 2: { RS_CircleData d; if (!read(a.at(1), d)) return false; c = std::move(d); break; }
    case 3: { RS_EllipseData d; if (!read(a.at(1), d)) return false; c = std::move(d); break; }
    case 4: { RS_SplineData d; if (!read(a.at(1), d)) return false; c = std::move(d); break; }
    }
    return true;
}
bool valid(const Token& t) {
    return t.requestId > 0 && t.generation >= 0 && t.sourceRevision >= 0 && t.undoEpoch >= 0;
}
bool read(const QCborValue& value, Token& t) {
    QCborArray a;
    return array(value, 4, a) && integer(a.at(0), t.requestId) && integer(a.at(1), t.generation) &&
        integer(a.at(2), t.sourceRevision) && integer(a.at(3), t.undoEpoch) && valid(t);
}
bool valid(const Generate& op) {
    return valid(op.definition) && valid(op.limits) && (!op.throughPoint || valid(*op.throughPoint));
}
bool valid(const ValidateCache& op) {
    return valid(op.cache) && valid(op.definition) && std::isfinite(op.worldTolerance) && valid(op.limits);
}
bool valid(const TrimCache& op) {
    return valid(op.cache) && valid(op.cutter) && valid(op.pick) && valid(op.limits);
}
bool valid(const TrimConic& op) {
    return valid(op.cache) && isConic(op.conic) && valid(op.conic) && valid(op.conicPick) &&
        (!op.cachePick || valid(*op.cachePick)) && valid(op.limits);
}
bool valid(const CheckThrough& op) {
    return valid(op.cache) && valid(op.definition) && std::isfinite(op.worldTolerance) &&
        valid(op.point) && valid(op.limits);
}

QCborArray fields(const RS_Vector& p) { return {p.x, p.y, p.z, p.valid}; }
QCborArray fields(const RS_ArcData& d) {
    return {fields(d.center), d.radius, d.angle1, d.angle2, d.reversed,
        d.startAngleDegrees, d.otherAngleDegrees, d.angularLength};
}
QCborArray fields(const RS_CircleData& d) { return {fields(d.center), d.radius}; }
QCborArray fields(const RS_EllipseData& d) {
    return {fields(d.center), fields(d.majorP), d.ratio, d.angle1, d.angle2, d.reversed,
        d.isArc, d.angleDegrees, d.startAngleDegrees, d.otherAngleDegrees, d.angularLength};
}
QCborArray fields(const std::vector<RS_Vector>& values) {
    QCborArray a;
    for (const auto& value : values) a.append(fields(value));
    return a;
}
QCborArray fields(const std::vector<double>& values) {
    QCborArray a;
    for (double value : values) a.append(value);
    return a;
}
QCborArray fields(const RS_SplineData& d) {
    return {fields(d.controlPoints), fields(d.knotslist), fields(d.weights), fields(d.fitPoints),
        qint64(d.degree), qint64(d.type), qint64(d.savedOpenType), fields(d.savedOpenKnots),
        d.m_closedFlag, d.m_periodicFlag};
}
QCborArray fields(const LC_EllipseOffsetDefinition& d) {
    return {fields(d.basis), d.distance, d.towardCentre, d.cacheReversed, d.relTolerance};
}
QCborArray fields(const LC_OffsetCheckLimits& l) {
    return {qint64(l.maxCacheSpans), qint64(l.maxVertices), qint64(l.maxDistancePairs),
        qint64(l.maxDepth), qint64(l.maxOrderingComparisons)};
}
QCborArray fields(const LC_OffsetRunLimits& l) {
    return {QCborArray{qint64(l.outputBudget.maxCubicPieces), qint64(l.outputBudget.maxOutputEntities),
        qint64(l.outputBudget.maxDeepEntities)}, qint64(l.maxSamples), qint64(l.maxIntersectionPairs),
        fields(l.checkLimits)};
}
QCborArray fields(const LC_OffsetCacheTrim::Limits& l) {
    return {qint64(l.maxSpansPerCurve), qint64(l.maxInputSpans), qint64(l.maxInputRecords),
        qint64(l.maxBoxPairs), qint64(l.maxLeaves), qint64(l.maxGeneratedItems),
        qint64(l.maxOrderingComparisons), qint64(l.maxRoots), qint64(l.maxDepth),
        qint64(l.maxLeafDepth), qint64(l.maxFragments), qint64(l.maxOutputSpans),
        qint64(l.maxOutputControls), qint64(l.maxOutputKnots)};
}
QCborArray fields(const LC_OffsetCacheTrim::Pick& p) {
    return {fields(p.worldPoint), p.rawSpanHint ? QCborValue(qint64(*p.rawSpanHint)) : QCborValue(nullptr),
        p.localParameterSeed ? QCborValue(*p.localParameterSeed) : QCborValue(nullptr)};
}
QCborArray fields(const Segment& s) { return {fields(s.start), fields(s.end)}; }
QCborArray fields(const Cutter& c) {
    return {qint64(c.index()), std::visit([](const auto& d) { return fields(d); }, c)};
}
QCborArray fields(const Generate& op) {
    return {fields(op.definition), fields(op.limits),
        op.throughPoint ? QCborValue(fields(*op.throughPoint)) : QCborValue(nullptr)};
}
QCborArray fields(const ValidateCache& op) {
    return {fields(op.cache), fields(op.definition), op.worldTolerance, fields(op.limits)};
}
QCborArray fields(const TrimCache& op) {
    return {fields(op.cache), fields(op.cutter), fields(op.pick), fields(op.limits)};
}
QCborArray fields(const TrimConic& op) {
    return {fields(op.cache), fields(op.conic), fields(op.conicPick),
        op.cachePick ? QCborValue(fields(*op.cachePick)) : QCborValue(nullptr), fields(op.limits)};
}
QCborArray fields(const CheckThrough& op) {
    return {fields(op.cache), fields(op.definition), op.worldTolerance, fields(op.point), fields(op.limits)};
}
}

namespace Values {
std::optional<QCborArray> encodeCache(const RS_SplineData& data) {
    if (!valid(data)) return std::nullopt;
    return fields(data);
}
std::optional<RS_SplineData> decodeCache(const QCborArray& a) {
    RS_SplineData data;
    if (!read(a, data)) return std::nullopt;
    return data;
}
std::optional<QCborArray> encodeDefinition(const LC_EllipseOffsetDefinition& data) {
    if (!valid(data)) return std::nullopt;
    return fields(data);
}
std::optional<LC_EllipseOffsetDefinition> decodeDefinition(const QCborArray& a) {
    LC_EllipseOffsetDefinition data;
    if (!read(a, data)) return std::nullopt;
    return data;
}
std::optional<QCborArray> encodePick(const LC_OffsetCacheTrim::Pick& data) {
    if (!valid(data)) return std::nullopt;
    return fields(data);
}
std::optional<LC_OffsetCacheTrim::Pick> decodePick(const QCborArray& a) {
    LC_OffsetCacheTrim::Pick data;
    if (!read(a, data)) return std::nullopt;
    return data;
}
std::optional<QCborArray> encodeCutter(const Cutter& data) {
    if (!valid(data)) return std::nullopt;
    return fields(data);
}
std::optional<Cutter> decodeCutter(const QCborArray& a) {
    Cutter data;
    if (!readCutter(a, data)) return std::nullopt;
    return data;
}
}

bool admitted(const Request& request) {
    return valid(request.token) && !request.operation.valueless_by_exception() &&
        std::visit([](const auto& op) { return valid(op); }, request.operation);
}

std::optional<QCborArray> encode(const Request& request) {
    // No CBOR arrays are populated until the entire caller-owned value is admitted.
    if (!admitted(request)) return std::nullopt;
    const auto& t = request.token;
    return QCborArray{2, QCborArray{t.requestId, t.generation, t.sourceRevision, t.undoEpoch},
        qint64(request.operation.index()), std::visit([](const auto& op) { return fields(op); }, request.operation)};
}

std::optional<Request> decode(const QCborArray& a) {
    qint64 version, tag;
    Token token;
    if (a.size() != 4 || !integer(a.at(0), version) || version != 2 ||
        !read(a.at(1), token) || !integer(a.at(2), tag) || !a.at(3).isArray()) return std::nullopt;
    const auto body = a.at(3).toArray();
    switch (tag) {
    case 0: {
        Generate op;
        if (body.size() != 3 || !read(body.at(0), op.definition) || !read(body.at(1), op.limits)) break;
        if (!body.at(2).isNull()) {
            RS_Vector point;
            if (!read(body.at(2), point)) break;
            op.throughPoint = point;
        }
        return Request{token, std::move(op)};
        break;
    }
    case 1: {
        ValidateCache op;
        if (body.size() == 4 && read(body.at(0), op.cache) && read(body.at(1), op.definition) &&
            number(body.at(2), op.worldTolerance) && read(body.at(3), op.limits))
            return Request{token, std::move(op)};
        break;
    }
    case 2: {
        TrimCache op;
        if (body.size() == 4 && read(body.at(0), op.cache) && readCutter(body.at(1), op.cutter) &&
            read(body.at(2), op.pick) && read(body.at(3), op.limits))
            return Request{token, std::move(op)};
        break;
    }
    case 3: {
        TrimConic op;
        if (body.size() != 5 || !read(body.at(0), op.cache) || !readCutter(body.at(1), op.conic, true) ||
            !read(body.at(2), op.conicPick) || !read(body.at(4), op.limits)) break;
        if (!body.at(3).isNull()) {
            LC_OffsetCacheTrim::Pick pick;
            if (!read(body.at(3), pick)) break;
            op.cachePick = pick;
        }
        return Request{token, std::move(op)};
    }
    case 4: {
        CheckThrough op;
        if (body.size() == 5 && read(body.at(0), op.cache) && read(body.at(1), op.definition) &&
            number(body.at(2), op.worldTolerance) && read(body.at(3), op.point) && read(body.at(4), op.limits))
            return Request{token, std::move(op)};
        break;
    }
    }
    return std::nullopt;
}
}
