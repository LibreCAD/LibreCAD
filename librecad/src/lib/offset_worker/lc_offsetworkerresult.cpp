/*
 * This file is part of LibreCAD, a 2D CAD program.
 * Copyright (C) 2026 LibreCAD.org
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "lc_offsetworkerresult.h"
#include "lc_offsetworkerwire.h"

#include <QCborValue>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <tuple>
#include <type_traits>
#include <utility>

namespace LC_OffsetWorkerResult {
namespace {
namespace Req = LC_OffsetWorkerRequest;
namespace Trim = LC_OffsetCacheTrim;
namespace Geo = LC_EllipseOffsetGeometry;
using Refusal = LC_OffsetCurveRefusal;
using Reason = LC_OffsetCheckResult::Reason;
using Status = LC_CurveOffsetStatus;
constexpr double Tau = 6.28318530717958647692;

// Fixed records have one layout for admission, reading and writing. In
// particular, intersection diagnostics are values, never proof capabilities.
auto members(RS_Vector& d) { return std::tie(d.x, d.y, d.z, d.valid); }
auto members(RS_EllipseData& d) {
    return std::tie(d.center, d.majorP, d.ratio, d.angle1, d.angle2, d.reversed,
        d.isArc, d.angleDegrees, d.startAngleDegrees, d.otherAngleDegrees, d.angularLength);
}
auto members(LC_EllipseOffsetDefinition& d) {
    return std::tie(d.basis, d.distance, d.towardCentre, d.cacheReversed, d.relTolerance);
}
auto members(Geo::CheckWork& d) {
    return std::tie(d.cacheSpans, d.vertices, d.distancePairs, d.deepest, d.orderingComparisons);
}
auto members(LC_OffsetOutputUsage& d) { return std::tie(d.cubicPieces, d.outputEntities, d.deepEntities); }
auto members(Trim::Location& d) { return std::tie(d.rawSpan, d.localParameter); }
auto members(Trim::Endpoint& d) { return std::tie(d.labels, d.source, d.rootId, d.imageX, d.imageY); }
auto members(Trim::Section& d) { return std::tie(d.rawSpan, d.from, d.to, d.originalDomain); }
auto members(LC_IntersectionDomain& d) { return std::tie(d.segmentA, d.segmentB, d.parameterA, d.parameterB); }
auto members(Trim::RootProvenance& d) {
    return std::tie(d.id, d.target, d.isolation, d.proofDomain, d.proof, d.worldX, d.worldY);
}
auto members(Trim::RemovedInterval& d) { return std::tie(d.from, d.to, d.crossesStorageSeam); }
auto members(Trim::Result::ConicFragment& d) {
    return std::tie(d.data, d.from, d.to, d.startRoot, d.endRoot, d.crossesStorageSeam);
}
auto members(LC_IntersectionWorkStats& d) {
    return std::tie(d.candidateBoxPairs, d.refinementBoxPairs, d.certificateBoxPairs,
        d.totalBoxPairs, d.generatedLeaves, d.generatedItems, d.deepestPreparation,
        d.deepestPairRefinement, d.deepestOrderingRefinement, d.deepestRootRefinement,
        d.orderingComparisons, d.validationSegmentVisits, d.validationNextCalls);
}
auto members(Trim::Work& d) {
    return std::tie(d.inputSpans, d.inputRecords, d.rootRecords, d.kernelItems,
        d.kernelComparisons, d.outputSpans, d.outputControls, d.outputKnots, d.intersections);
}
auto members(SourceDiagnostics& d) {
    return std::tie(d.status, d.engineStatus, d.detail, d.stoppedBy, d.usage,
        d.copiesMade, d.copiesRequested, d.sourceRemoved, d.copiesKeptPlain,
        d.rebaseEngineRuns, d.cacheEngineRuns, d.reusedCacheCopies, d.sampleUnits,
        d.intersectionPairAllowance, d.attemptedOutputUsage, d.attemptedDeepCountComplete,
        d.adoptionAttempts, d.adoptionCheckWork, d.resolvedSide);
}
auto members(LC_OffsetPlainOutcome& d) { return std::tie(d.copy, d.reason, d.checkReason); }
auto members(LC_OffsetRebaseOutcome& d) {
    return std::tie(d.copy, d.proposalRefusal, d.nativeRefusal, d.engineStatus, d.checkReason,
        d.checkWork, d.engineRuns, d.sampleUnits, d.intersectionPairAllowance, d.adopted);
}
template<class T> struct IsOptional : std::false_type {};
template<class T> struct IsOptional<std::optional<T>> : std::true_type {};
template<class T> constexpr qint64 enumMax() {
    if constexpr (std::is_same_v<T, Refusal>) return qint64(Refusal::ThroughNotOnCache);
    else if constexpr (std::is_same_v<T, Reason>) return qint64(Reason::LimitExceeded);
    else if constexpr (std::is_same_v<T, Status>) return qint64(Status::LimitExceeded);
    else if constexpr (std::is_same_v<T, LC_OffsetSourceStatus>) return qint64(LC_OffsetSourceStatus::LimitExceeded);
    else if constexpr (std::is_same_v<T, LC_OffsetFailureDetail>) return qint64(LC_OffsetFailureDetail::AmbiguousSide);
    else if constexpr (std::is_same_v<T, LC_OffsetPlainReason>) return qint64(LC_OffsetPlainReason::UndefinedDefinition);
    else if constexpr (std::is_same_v<T, LC_OffsetCurveAgain::Refusal>) return qint64(LC_OffsetCurveAgain::Refusal::FinalNotProvedUntrimmed);
    else if constexpr (std::is_same_v<T, LC_CurveOffsetSide>) return qint64(LC_CurveOffsetSide::Right);
    else if constexpr (std::is_same_v<T, Failure>) return qint64(Failure::InternalFailure);
    else if constexpr (std::is_same_v<T, OrdinaryStatus>) return qint64(OrdinaryStatus::Unsupported);
    else if constexpr (std::is_same_v<T, Trim::Refusal>) return qint64(Trim::Refusal::AllocationFailure);
    else if constexpr (std::is_same_v<T, LC_IntersectionProof>) return qint64(LC_IntersectionProof::AnalyticExclusion);
    else { static_assert(std::is_same_v<T, void>, "Missing protocol enum range"); }
}
bool add(std::size_t& used, std::size_t n, std::size_t cap) {
    if (used > cap || n > cap - used) return false;
    used += n;
    return true;
}
bool array(const QCborValue& v, qsizetype n, QCborArray& a) {
    if (!v.isArray()) return false;
    a = v.toArray();
    return a.size() == n;
}
bool bounded(const QCborValue& v, std::size_t cap, QCborArray& a) {
    if (!v.isArray()) return false;
    a = v.toArray();
    return std::uintmax_t(a.size()) <= cap;
}

template<class T> bool admitted(T d) {
    if constexpr (std::is_same_v<T, double>) return std::isfinite(d);
    else if constexpr (std::is_same_v<T, bool>) return true;
    else if constexpr (std::is_integral_v<T>) return d >= 0 && std::uintmax_t(d) <= std::uintmax_t(std::numeric_limits<qint64>::max());
    else if constexpr (std::is_enum_v<T>) return qint64(d) >= 0 && qint64(d) <= enumMax<T>();
    else if constexpr (IsOptional<T>::value) return !d || admitted(*d);
    else if constexpr (std::is_same_v<T, LC_Interval>) return d.isValid() && std::isfinite(d.lo()) && std::isfinite(d.hi());
    else {
        if constexpr (std::is_same_v<T, RS_Vector>) if (!d.valid || d.z != 0) return false;
        return std::apply([](const auto&... f) { return (admitted(f) && ...); }, members(d));
    }
}
template<class T> bool read(const QCborValue& v, T& d) {
    if constexpr (std::is_same_v<T, double>) {
        if (!v.isDouble()) return false;
        d = v.toDouble();
    } else if constexpr (std::is_same_v<T, bool>) {
        if (!v.isBool()) return false;
        d = v.toBool();
    } else if constexpr (std::is_integral_v<T>) {
        if (!v.isInteger() || v.toInteger() < 0 ||
            std::uintmax_t(v.toInteger()) > std::uintmax_t(std::numeric_limits<T>::max())) return false;
        d = static_cast<T>(v.toInteger());
    } else if constexpr (std::is_enum_v<T>) {
        if (!v.isInteger() || v.toInteger() < 0 || v.toInteger() > enumMax<T>()) return false;
        d = static_cast<T>(v.toInteger());
    } else if constexpr (IsOptional<T>::value) {
        d.reset();
        if (!v.isNull()) {
            typename T::value_type item{};
            if (!read(v, item)) return false;
            d = std::move(item);
        }
    } else if constexpr (std::is_same_v<T, LC_Interval>) {
        QCborArray a;
        double lo, hi;
        // hull sorts its arguments: reject reversed bounds BEFORE calling it.
        if (!array(v, 2, a) || !read(a.at(0), lo) || !read(a.at(1), hi) || lo > hi) return false;
        d = LC_Interval::hull(lo, hi);
    } else {
        auto fields = members(d);
        QCborArray a;
        if (!array(v, std::tuple_size_v<decltype(fields)>, a)) return false;
        qsizetype i = 0;
        if (!std::apply([&](auto&... f) { return (read(a.at(i++), f) && ...); }, fields)) return false;
    }
    return admitted(d);
}
template<class T> QCborValue fields(T d) {
    if constexpr (std::is_same_v<T, double> || std::is_same_v<T, bool>) return QCborValue(d);
    else if constexpr (std::is_integral_v<T> || std::is_enum_v<T>) return QCborValue(qint64(d));
    else if constexpr (IsOptional<T>::value) return d ? fields(*d) : QCborValue(nullptr);
    else if constexpr (std::is_same_v<T, LC_Interval>) return QCborArray{d.lo(), d.hi()};
    else {
        QCborArray a;
        std::apply([&](const auto&... f) { (a.append(fields(f)), ...); }, members(d));
        return a;
    }
}

struct Counts { std::size_t spans = 0, controls = 0, knots = 0; };
bool shape(std::size_t degree, std::size_t controls, std::size_t knots,
           std::size_t weights, Counts& c, std::size_t cap) {
    if ((degree != 1 && degree != 3) || controls < degree + 1 || (controls - 1) % degree) return false;
    c = {(controls - 1) / degree, controls, knots};
    return c.spans <= cap && knots == controls + degree + 1 && weights == controls;
}
std::size_t sourceSpans(const RS_SplineData& d) {
    Counts c;
    return shape(d.degree, d.controlPoints.size(), d.knotslist.size(), d.weights.size(), c, 4096) ? c.spans : 0;
}
template<class Get> bool knots(std::size_t degree, std::size_t spans, Get get, bool integerBreaks) {
    std::size_t index = 0;
    double previous = 0;
    for (std::size_t k = 0; k <= spans; ++k) {
        const double value = get(index);
        if (!std::isfinite(value) || (k && !(previous < value)) || (integerBreaks && value != double(k))) return false;
        const auto multiplicity = k == 0 || k == spans ? degree + 1 : degree;
        for (std::size_t j = 0; j < multiplicity; ++j) if (get(index++) != value) return false;
        previous = value;
    }
    return true;
}
bool cache(const RS_SplineData& d, Counts& c, std::size_t cap, bool fragment = false) {
    if (!shape(d.degree, d.controlPoints.size(), d.knotslist.size(), d.weights.size(), c, cap) ||
        d.type != RS_SplineData::SplineType::ClampedOpen || d.savedOpenType != d.type ||
        !d.fitPoints.empty() || !d.savedOpenKnots.empty() ||
        (fragment && (d.m_closedFlag || d.m_periodicFlag))) return false;
    for (const auto& p : d.controlPoints) if (!admitted(p)) return false;
    for (double w : d.weights) if (w != 1) return false;
    return knots(d.degree, c.spans, [&](std::size_t i) { return d.knotslist[i]; }, fragment);
}
bool scanCache(const QCborValue& v, Counts& c, std::size_t cap, bool fragment = false) {
    QCborArray a, points, breaks, weights, fit, saved;
    std::size_t degree, type, savedType;
    bool closed, periodic;
    if (!array(v, 10, a) || !bounded(a.at(0), 16384, points) || !bounded(a.at(1), 32768, breaks) ||
        !bounded(a.at(2), 16384, weights) || !bounded(a.at(3), 0, fit) || !bounded(a.at(7), 0, saved) ||
        !read(a.at(4), degree) || !read(a.at(5), type) || !read(a.at(6), savedType) ||
        !read(a.at(8), closed) || !read(a.at(9), periodic) || type != 1 || savedType != 1 ||
        (fragment && (closed || periodic)) ||
        !shape(degree, std::size_t(points.size()), std::size_t(breaks.size()), std::size_t(weights.size()), c, cap)) return false;
    RS_Vector point;
    double number;
    for (const auto& p : points) if (!read(p, point)) return false;
    for (const auto& w : weights) if (!read(w, number) || number != 1) return false;
    for (const auto& k : breaks) if (!read(k, number)) return false;
    return knots(degree, c.spans, [&](std::size_t i) { return breaks.at(qsizetype(i)).toDouble(); }, fragment);
}
bool definition(const LC_EllipseOffsetDefinition& d) {
    const auto& b = d.basis;
    return admitted(b) && b.ratio > 0 && b.ratio <= 1 &&
        (b.majorP.x != 0 || b.majorP.y != 0) && b.angle1 >= 0 && b.angle1 < Tau &&
        b.angle2 >= 0 && b.angle2 < Tau && std::isfinite(d.distance) && d.distance > 0 &&
        std::isfinite(d.relTolerance) && d.relTolerance > 0;
}
bool curve(const Curve& d, Counts& c, std::size_t cap) {
    if (d.grips.empty() || d.grips.size() > 256 || !admitted(d.midpoint) ||
        !std::isfinite(d.worldTolerance) || d.worldTolerance <= 0 ||
        !definition(d.definition) || !cache(d.cache, c, cap)) return false;
    for (const auto& p : d.grips) if (!admitted(p)) return false;
    return true;
}
bool readCurve(const QCborValue& v, Curve& d, Counts& c, std::size_t cap, bool materialize) {
    QCborArray a, grips;
    if (!array(v, 6, a) || !bounded(a.at(4), 256, grips) || grips.isEmpty() ||
        !scanCache(a.at(0), c, cap) || !a.at(1).isArray() ||
        !read(a.at(2), d.worldTolerance) || d.worldTolerance <= 0 ||
        !read(a.at(3), d.closed) || !read(a.at(5), d.midpoint)) return false;
    const auto def = Req::Values::decodeDefinition(a.at(1).toArray());
    if (!def || !definition(*def)) return false;
    d.definition = *def;
    RS_Vector point;
    for (const auto& p : grips) if (!read(p, point)) return false;
    if (materialize) {
        auto data = Req::Values::decodeCache(a.at(0).toArray());
        if (!data) return false;
        d.cache = std::move(*data);
        d.grips.reserve(std::size_t(grips.size()));
        for (const auto& p : grips) { read(p, point); d.grips.push_back(point); }
    }
    return true;
}

LC_OffsetCheckLimits checkLimits(const Req::Request& r) {
    auto l = r.operation.index() == 0 ? std::get<Req::Generate>(r.operation).limits.checkLimits :
        r.operation.index() == 4 ? std::get<Req::CheckThrough>(r.operation).limits :
            std::get<Req::ValidateCache>(r.operation).limits;
    const LC_OffsetCheckLimits hard;
    l.maxCacheSpans = std::min(l.maxCacheSpans, hard.maxCacheSpans);
    l.maxVertices = std::min(l.maxVertices, hard.maxVertices);
    l.maxDistancePairs = std::min(l.maxDistancePairs, hard.maxDistancePairs);
    l.maxDepth = std::min(l.maxDepth, hard.maxDepth);
    l.maxOrderingComparisons = std::min(l.maxOrderingComparisons, hard.maxOrderingComparisons);
    return l;
}
bool zero(const Geo::CheckWork& w) {
    return !w.cacheSpans && !w.vertices && !w.distancePairs && !w.deepest && !w.orderingComparisons;
}
bool zero(const LC_OffsetOutputUsage& u) { return !u.cubicPieces && !u.outputEntities && !u.deepEntities; }
bool usage(const LC_OffsetOutputUsage& u, const LC_OffsetSourceBudget& l) {
    return admitted(u) && u.cubicPieces <= l.maxCubicPieces && u.outputEntities <= l.maxOutputEntities &&
        u.deepEntities <= l.maxDeepEntities;
}
LC_OffsetSourceBudget outputBudget(const Req::Generate& op) {
    auto budget = op.limits.outputBudget;
    const auto hard = makeDefaultOffsetSourceBudget();
    budget.maxCubicPieces = std::min(budget.maxCubicPieces, hard.maxCubicPieces);
    budget.maxOutputEntities = std::min(budget.maxOutputEntities, hard.maxOutputEntities);
    budget.maxDeepEntities = std::min(budget.maxDeepEntities, hard.maxDeepEntities);
    return budget;
}
bool ordinaryHeader(const Native& n, const Req::Request& r, std::size_t count) {
    if (!admitted(n.ordinaryStatus) || !admitted(n.ordinaryUsage)) return false;
    if (n.ordinaryStatus == OrdinaryStatus::None) return !count && zero(n.ordinaryUsage);
    if (r.operation.index() != 0 || n.engineStatus != Status::Ok || n.engineRuns != 1 ||
        n.curve || n.changed || !zero(n.throughWork) ||
        (n.refusal != Refusal::SeveralPieces && n.refusal != Refusal::NotCacheRepresentable &&
            n.refusal != Refusal::CheckFailed && n.refusal != Refusal::NothingLeft)) return false;
    const auto cap = outputBudget(std::get<Req::Generate>(r.operation));
    if (count > cap.maxOutputEntities || !usage(n.ordinaryUsage, cap) ||
        n.ordinaryUsage.cubicPieces != n.attemptedOutputUsage.cubicPieces ||
        n.ordinaryUsage.outputEntities != n.attemptedOutputUsage.outputEntities ||
        n.ordinaryUsage.deepEntities != n.attemptedOutputUsage.deepEntities) return false;
    if (n.ordinaryStatus == OrdinaryStatus::Unsupported) return !count;
    if (!count) return n.refusal == Refusal::NothingLeft && zero(n.ordinaryUsage);
    return n.refusal != Refusal::NothingLeft && n.ordinaryUsage.outputEntities == count &&
        n.ordinaryUsage.deepEntities >= count && n.ordinaryUsage.cubicPieces >= count;
}
bool ordinaryCount(Counts& total, const Counts& c, std::size_t spans) {
    return add(total.spans, c.spans, spans) && add(total.controls, c.controls, 16384) &&
        add(total.knots, c.knots, 32768);
}
bool ordinary(const Native& n, const Req::Request& r) {
    if (!ordinaryHeader(n, r, n.ordinary.size())) return false;
    Counts total;
    if (!n.ordinary.empty()) {
        const auto cap = outputBudget(std::get<Req::Generate>(r.operation));
        for (const auto& item : n.ordinary) {
            Counts c;
            if (const auto* line = std::get_if<Req::Segment>(&item)) {
                if (!admitted(line->start) || !admitted(line->end)) return false;
                c = {1, 2, 0};
            } else if (const auto* spline = std::get_if<RS_SplineData>(&item)) {
                if (!cache(*spline, c, cap.maxCubicPieces)) return false;
            } else return false;
            if (!ordinaryCount(total, c, cap.maxCubicPieces)) return false;
        }
    }
    // Producer work is measured before chainEntity's seam subdivision and
    // collinear compaction. Stored spans have their own independent cap above.
    return true;
}
bool readOrdinary(const QCborValue& v, Native& n, const Req::Request& r, bool materialize) {
    QCborArray values;
    if (!bounded(v, makeDefaultOffsetSourceBudget().maxOutputEntities, values) ||
        !ordinaryHeader(n, r, std::size_t(values.size()))) return false;
    Counts total;
    if (!values.isEmpty()) {
        const auto cap = outputBudget(std::get<Req::Generate>(r.operation));
        if (materialize) n.ordinary.reserve(std::size_t(values.size()));
        for (const auto& item : values) {
            QCborArray a, segment;
            std::size_t type;
            Counts c;
            if (!array(item, 2, a) || !read(a.at(0), type)) return false;
            if (type == 0) {
                Req::Segment line;
                if (!array(a.at(1), 2, segment) || !read(segment.at(0), line.start) ||
                    !read(segment.at(1), line.end)) return false;
                c = {1, 2, 0};
            } else if (type == 4) {
                if (!scanCache(a.at(1), c, cap.maxCubicPieces)) return false;
            } else return false;
            if (!ordinaryCount(total, c, cap.maxCubicPieces)) return false;
            if (materialize) {
                auto value = Req::Values::decodeCutter(a);
                if (!value) return false;
                n.ordinary.push_back(std::move(*value));
            }
        }
    }
    return true;
}

LC_OffsetSourceBudget produceBudget(const Req::Produce& op) {
    auto cap = op.limits.perSource;
    const auto hard = makeDefaultOffsetSourceBudget();
    cap.maxCubicPieces = std::min(cap.maxCubicPieces, hard.maxCubicPieces);
    cap.maxOutputEntities = std::min(cap.maxOutputEntities, hard.maxOutputEntities);
    cap.maxDeepEntities = std::min({cap.maxDeepEntities, hard.maxDeepEntities, op.limits.maxDeepEntitiesPerRequest});
    return cap;
}
std::size_t produceSamples(const Req::Produce& op) {
    return op.limits.maxSamples ? std::min(op.limits.maxSamples, LC_CurveOffset::kDefaultMaxSamples) :
        LC_CurveOffset::kDefaultMaxSamples;
}
std::size_t producePairs(const Req::Produce& op) {
    return op.limits.maxIntersectionPairs ? std::min(op.limits.maxIntersectionPairs, LC_CurveOffset::kDefaultMaxIntersectionPairs) :
        LC_CurveOffset::kDefaultMaxIntersectionPairs;
}
bool checkWorkBound(const Geo::CheckWork& w, std::size_t checks) {
    const LC_OffsetCheckLimits cap;
    return admitted(w) && w.cacheSpans <= checks * cap.maxCacheSpans &&
        w.vertices <= checks * cap.maxVertices && w.distancePairs <= checks * cap.maxDistancePairs &&
        w.orderingComparisons <= checks * cap.maxOrderingComparisons &&
        w.deepest <= (checks ? cap.maxDepth : 0);
}
struct ProductionScan {
    std::array<std::size_t, RS_OffsetData::kMaximumOffsetCopies> copies{};
    std::array<bool, RS_OffsetData::kMaximumOffsetCopies> plain{}, adopted{};
    std::array<LC_OffsetPlainOutcome, RS_OffsetData::kMaximumOffsetCopies> plainOutcomes{};
    std::size_t requiredAdoptions = 0, optionalAdoptions = 0;
    std::size_t roots = 0, rebaseRuns = 0, rebaseSamples = 0, rebasePairs = 0;
    std::size_t cacheRuns = 0, pairAllowance = 0;
    std::size_t previousPlain = 0, previousRebase = 0;
    Counts geometry;
    std::size_t minimumDeep = 0;
};
bool productionHeader(const Produced& d, const Req::Produce& op, std::size_t groups,
    std::size_t plain, std::size_t rebases, std::size_t outputs) {
    const auto& s = d.source;
    const auto cap = produceBudget(op);
    const std::size_t copies = std::size_t(op.copies);
    if (!admitted(s) || !admitted(d.outputStatus) || s.copiesRequested != op.copies ||
        s.copiesMade > op.copies || s.sourceRemoved || groups != std::size_t(s.copiesMade) ||
        s.copiesKeptPlain != int(plain) || plain > groups || rebases > copies ||
        s.rebaseEngineRuns > rebases || s.cacheEngineRuns > copies || s.reusedCacheCopies ||
        s.sampleUnits > copies * (produceSamples(op) + 1) ||
        s.intersectionPairAllowance > copies * producePairs(op) ||
        s.adoptionAttempts > copies || !checkWorkBound(s.adoptionCheckWork, s.adoptionAttempts) ||
        !usage(s.usage, cap)) return false;
    if (s.resolvedSide && *s.resolvedSide != LC_CurveOffsetSide::Left &&
        *s.resolvedSide != LC_CurveOffsetSide::Right) return false;
    const bool wrap = op.asOffsetCurve && !op.preview;
    if (plain && op.source.index() == 1 && !op.context.nativeSource) return false;
    if (s.status == LC_OffsetSourceStatus::Succeeded && s.adoptionAttempts > std::size_t(s.copiesMade)) return false;
    if ((!wrap && (plain || rebases || s.adoptionAttempts)) ||
        (s.adoptionAttempts && (op.source.index() != 0 || !op.context.topLevel)) ||
        (rebases && (op.source.index() != 1 || !op.context.nativeSource))) return false;
    const auto runs = s.rebaseEngineRuns + s.cacheEngineRuns;
    LC_OffsetSourceBudget attempted{runs * cap.maxCubicPieces, runs * cap.maxOutputEntities, runs * cap.maxDeepEntities};
    if (!usage(s.attemptedOutputUsage, attempted) ||
        (!s.attemptedDeepCountComplete && !s.attemptedOutputUsage.outputEntities)) return false;
    const bool success = s.status == LC_OffsetSourceStatus::Succeeded;
    if (success) {
        const auto completedOrStopped = std::size_t(s.copiesMade) + (s.copiesMade < op.copies ? 1 : 0);
        if (wrap && op.source.index() == 1 && op.context.nativeSource) {
            if (rebases != completedOrStopped) return false;
        } else if (s.cacheEngineRuns != completedOrStopped) return false;
        if (!s.copiesMade || !s.resolvedSide || !s.usage.outputEntities ||
            s.usage.outputEntities < std::size_t(s.copiesMade) || s.usage.cubicPieces < s.usage.outputEntities ||
            s.usage.deepEntities < s.usage.outputEntities ||
            s.usage.cubicPieces > s.attemptedOutputUsage.cubicPieces ||
            s.usage.outputEntities > s.attemptedOutputUsage.outputEntities) return false;
        if (s.copiesMade == op.copies) {
            if (s.stoppedBy != LC_OffsetSourceStatus::Succeeded || s.engineStatus != Status::Ok) return false;
        } else if (s.stoppedBy == LC_OffsetSourceStatus::Vanished) {
            if (s.engineStatus != Status::Ok) return false;
        } else if (s.stoppedBy != LC_OffsetSourceStatus::OffsetFailed ||
            s.engineStatus == Status::Ok || s.engineStatus == Status::LimitExceeded) return false;
        if (d.outputStatus == OrdinaryStatus::None) return false;
        if (d.outputStatus == OrdinaryStatus::Complete) {
            if (outputs != s.usage.outputEntities) return false;
        } else if (outputs) return false;
    } else {
        if (s.copiesMade || groups || plain || !zero(s.usage) ||
            d.outputStatus != OrdinaryStatus::None || outputs) return false;
        if (s.status == LC_OffsetSourceStatus::Vanished &&
            (s.stoppedBy != LC_OffsetSourceStatus::Vanished || s.engineStatus != Status::Ok || !s.resolvedSide)) return false;
        if (s.status == LC_OffsetSourceStatus::LimitExceeded &&
            s.engineStatus != Status::Ok && s.engineStatus != Status::LimitExceeded) return false;
        if (s.status == LC_OffsetSourceStatus::OffsetFailed && s.engineStatus == Status::LimitExceeded) return false;
    }
    return (!runs && !rebases && !s.adoptionAttempts) || s.resolvedSide.has_value();
}
bool productionCopy(std::size_t n, std::size_t index, ProductionScan& scan, const Req::Produce& op) {
    if (!n || index >= scan.copies.size() || !add(scan.roots, n, produceBudget(op).maxOutputEntities)) return false;
    scan.copies[index] = n; return true;
}
bool productionPlain(const LC_OffsetPlainOutcome& p, ProductionScan& scan, const Produced& d, const Req::Produce& op) {
    if (!admitted(p) || p.copy <= int(scan.previousPlain) || p.copy > d.source.copiesMade ||
        (p.reason == LC_OffsetPlainReason::CheckFailed && (!p.checkReason || *p.checkReason == Reason::None))) return false;
    const auto ordinal = std::size_t(p.copy - 1);
    if (!op.context.topLevel) {
        if (p.reason != LC_OffsetPlainReason::NotTopLevel || p.checkReason) return false;
    } else if (scan.copies[ordinal] != 1) {
        if (p.reason != LC_OffsetPlainReason::SeveralPieces || p.checkReason) return false;
    } else if (op.source.index() == 0) {
        if (p.reason == LC_OffsetPlainReason::CheckFailed || p.reason == LC_OffsetPlainReason::NotCacheRepresentable)
            ++scan.requiredAdoptions;
        else if (p.reason == LC_OffsetPlainReason::OutputBudget) ++scan.optionalAdoptions;
        else return false;
    }
    scan.plainOutcomes[ordinal] = p;
    scan.previousPlain = std::size_t(p.copy); scan.plain[ordinal] = true;
    return true;
}
bool productionRebase(const LC_OffsetRebaseOutcome& a, ProductionScan& scan,
    const Produced& d, const Req::Produce& op) {
    if (!admitted(a) || a.copy != int(scan.previousRebase + 1) || a.copy > op.copies ||
        a.engineRuns > 1 || a.sampleUnits > produceSamples(op) + 1 || !checkWorkBound(a.checkWork, 1)) return false;
    scan.previousRebase = std::size_t(a.copy);
    const bool eligible = op.context.topLevel && op.context.nativeDefinition.has_value();
    if (eligible) {
        if (!d.source.resolvedSide) return false;
        const auto proposal = LC_OffsetCurveAgain::propose(*op.context.nativeDefinition,
            *d.source.resolvedSide, std::abs(double(a.copy) * op.distance));
        if (a.proposalRefusal != proposal.refusal) return false;
    } else if (a.proposalRefusal) return false;
    const bool called = a.nativeRefusal.has_value();
    if (called != (eligible && a.proposalRefusal == LC_OffsetCurveAgain::Refusal::None && producePairs(op) >= 2)) return false;
    if (called && (!eligible || a.proposalRefusal != LC_OffsetCurveAgain::Refusal::None)) return false;
    if (!called) {
        if (a.engineStatus || a.checkReason || a.engineRuns || a.sampleUnits ||
            a.intersectionPairAllowance || !zero(a.checkWork) || a.adopted) return false;
    } else {
        if (bool(a.engineStatus) != bool(a.engineRuns) ||
            a.intersectionPairAllowance != (a.engineRuns ? producePairs(op) / 2 : 0)) return false;
        if (!a.engineStatus) {
            if ((*a.nativeRefusal != Refusal::InvalidInput && *a.nativeRefusal != Refusal::TooSmall) ||
                a.checkReason || !zero(a.checkWork) || a.sampleUnits || a.adopted) return false;
        } else if (*a.engineStatus != Status::Ok) {
            if (*a.nativeRefusal != Refusal::Engine || a.checkReason || !zero(a.checkWork) || a.adopted) return false;
        } else {
            if (*a.nativeRefusal != Refusal::None && *a.nativeRefusal != Refusal::NothingLeft &&
                *a.nativeRefusal != Refusal::SeveralPieces && *a.nativeRefusal != Refusal::NotCacheRepresentable &&
                *a.nativeRefusal != Refusal::CheckFailed) return false;
            if (a.sampleUnits > produceSamples(op) || a.adopted != (*a.nativeRefusal == Refusal::None) ||
                (a.adopted && a.checkReason != Reason::None) ||
                (*a.nativeRefusal == Refusal::CheckFailed && (!a.checkReason || *a.checkReason == Reason::None)) ||
                (a.checkReason && *a.checkReason != Reason::None && *a.nativeRefusal != Refusal::CheckFailed) ||
                (!a.checkReason && !zero(a.checkWork)) ||
                ((*a.nativeRefusal == Refusal::NothingLeft || *a.nativeRefusal == Refusal::SeveralPieces) && a.checkReason)) return false;
        }
    }
    if (d.source.status == LC_OffsetSourceStatus::Succeeded && a.copy <= d.source.copiesMade && !a.adopted) {
        const auto ordinal = std::size_t(a.copy - 1);
        if (!scan.plain[ordinal]) return false;
        const auto expected = !op.context.topLevel ? LC_OffsetPlainReason::NotTopLevel :
            scan.copies[ordinal] != 1 ? LC_OffsetPlainReason::SeveralPieces :
            !op.context.nativeDefinition ? LC_OffsetPlainReason::UndefinedDefinition :
            a.proposalRefusal == LC_OffsetCurveAgain::Refusal::ZeroSum ? LC_OffsetPlainReason::ZeroSum :
            LC_OffsetPlainReason::RebaseRefused;
        const auto checked = op.context.topLevel && scan.copies[ordinal] == 1 ? a.checkReason : std::nullopt;
        if (scan.plainOutcomes[ordinal].reason != expected || scan.plainOutcomes[ordinal].checkReason != checked) return false;
    }
    if (!add(scan.rebaseRuns, a.engineRuns, std::size_t(op.copies)) ||
        !add(scan.rebaseSamples, a.sampleUnits, d.source.sampleUnits) ||
        !add(scan.rebasePairs, a.intersectionPairAllowance, d.source.intersectionPairAllowance)) return false;
    if (!add(scan.pairAllowance, a.intersectionPairAllowance, d.source.intersectionPairAllowance)) return false;
    if (!a.adopted && a.sampleUnits < produceSamples(op)) {
        ++scan.cacheRuns;
        if (!add(scan.pairAllowance, producePairs(op) - a.intersectionPairAllowance,
            d.source.intersectionPairAllowance)) return false;
    }
    if (a.adopted) {
        if (d.source.status == LC_OffsetSourceStatus::Succeeded && a.copy > d.source.copiesMade) return false;
        scan.adopted[std::size_t(a.copy - 1)] = true;
    }
    return true;
}
bool nativeForCopy(const ProductionScan& scan, const Produced& d, const Req::Produce& op, std::size_t copy) {
    if (!op.asOffsetCurve || op.preview || !op.context.topLevel) return false;
    if (op.source.index() == 0) return !scan.plain[copy];
    return op.context.nativeSource && scan.adopted[copy];
}
bool productionDefinition(const Curve& curve, std::size_t ordinal, const Produced& d, const Req::Produce& op) {
    if (!d.source.resolvedSide) return false;
    LC_EllipseOffsetDefinition expected;
    const double magnitude = std::abs(double(ordinal + 1) * op.distance);
    if (op.source.index() == 0) {
        expected.basis = std::get<RS_EllipseData>(op.source); expected.distance = magnitude;
        expected.towardCentre = (*d.source.resolvedSide == LC_CurveOffsetSide::Left) != expected.basis.reversed;
        expected.relTolerance = LC_CurveOffset::kEllipseRelativeOffsetTolerance;
    } else {
        if (!op.context.nativeSource || !op.context.nativeDefinition) return false;
        const auto proposal = LC_OffsetCurveAgain::propose(*op.context.nativeDefinition, *d.source.resolvedSide, magnitude);
        if (!proposal) return false;
        expected = proposal.proposal->definition;
    }
    if (!Geo::normaliseBasis(expected.basis)) return false;
    return Req::Values::encodeDefinition(expected) == Req::Values::encodeDefinition(curve.definition);
}
bool productionTail(const ProductionScan& scan, const Produced& d, const Req::Produce& op) {
    if (scan.roots != d.source.usage.outputEntities || scan.rebaseRuns != d.source.rebaseEngineRuns) return false;
    if (op.source.index() == 1 && op.context.nativeSource && op.asOffsetCurve && !op.preview) {
        if (scan.cacheRuns != d.source.cacheEngineRuns || scan.pairAllowance != d.source.intersectionPairAllowance) return false;
    } else if (d.source.intersectionPairAllowance != d.source.cacheEngineRuns * producePairs(op)) return false;
    std::size_t adopted = 0;
    for (std::size_t i = 0; i < std::size_t(d.source.copiesMade); ++i) {
        if (scan.plain[i] && scan.adopted[i]) return false;
        adopted += scan.adopted[i] ? 1 : 0;
    }
    if (d.source.status == LC_OffsetSourceStatus::Succeeded && op.asOffsetCurve && !op.preview &&
        op.source.index() == 1 && op.context.nativeSource &&
        adopted + std::size_t(d.source.copiesKeptPlain) != std::size_t(d.source.copiesMade)) return false;
    if (d.source.status == LC_OffsetSourceStatus::Succeeded && op.asOffsetCurve && !op.preview &&
        op.source.index() == 0 && op.context.topLevel) {
        const auto required = std::size_t(d.source.copiesMade - d.source.copiesKeptPlain) + scan.requiredAdoptions;
        if (d.source.adoptionAttempts < required || d.source.adoptionAttempts > required + scan.optionalAdoptions) return false;
    }
    return true;
}
bool produced(const Produced& d, const Req::Produce& op) {
    if (!productionHeader(d, op, d.copyOutputCounts.size(), d.keptPlain.size(), d.rebases.size(), d.outputs.size())) return false;
    ProductionScan scan;
    for (std::size_t i = 0; i < d.copyOutputCounts.size(); ++i)
        if (!productionCopy(d.copyOutputCounts[i], i, scan, op)) return false;
    for (const auto& p : d.keptPlain) if (!productionPlain(p, scan, d, op)) return false;
    for (const auto& a : d.rebases) if (!productionRebase(a, scan, d, op)) return false;
    std::size_t ordinal = 0, end = scan.copies[0];
    const auto cap = produceBudget(op);
    for (std::size_t i = 0; i < d.outputs.size(); ++i) {
        while (i >= end && ordinal + 1 < d.copyOutputCounts.size()) end += scan.copies[++ordinal];
        const auto& item = d.outputs[i];
        const bool nativeExpected = nativeForCopy(scan, d, op, ordinal);
        Counts c;
        if (const auto* line = std::get_if<Req::Segment>(&item)) {
            if (nativeExpected || !admitted(line->start) || !admitted(line->end)) return false;
            c = {1, 2, 0};
        } else if (const auto* data = std::get_if<RS_SplineData>(&item)) {
            if (nativeExpected || !cache(*data, c, cap.maxCubicPieces)) return false;
        } else if (const auto* value = std::get_if<Curve>(&item)) {
            if (!nativeExpected || scan.copies[ordinal] != 1 ||
                !curve(*value, c, cap.maxCubicPieces) || !productionDefinition(*value, ordinal, d, op)) return false;
        } else return false;
        if (!ordinaryCount(scan.geometry, c, cap.maxCubicPieces) ||
            !add(scan.minimumDeep, item.index() == 0 ? 1 : kOffsetDeepEntitiesPerOutputSpline - 1,
                d.source.usage.deepEntities)) return false;
    }
    return productionTail(scan, d, op);
}
bool readProduced(const QCborValue& v, Produced& d, const Req::Produce& op, bool materialize) {
    QCborArray a, groups, plain, rebases, outputs;
    if (!array(v, 6, a) || !read(a.at(0), d.source) ||
        !bounded(a.at(1), RS_OffsetData::kMaximumOffsetCopies, groups) ||
        !bounded(a.at(2), RS_OffsetData::kMaximumOffsetCopies, plain) ||
        !bounded(a.at(3), RS_OffsetData::kMaximumOffsetCopies, rebases) ||
        !read(a.at(4), d.outputStatus) || !bounded(a.at(5), produceBudget(op).maxOutputEntities, outputs) ||
        !productionHeader(d, op, std::size_t(groups.size()), std::size_t(plain.size()),
            std::size_t(rebases.size()), std::size_t(outputs.size()))) return false;
    ProductionScan scan;
    for (qsizetype i = 0; i < groups.size(); ++i) {
        std::size_t count;
        if (!read(groups.at(i), count) || !productionCopy(count, std::size_t(i), scan, op)) return false;
    }
    if (materialize) {
        d.copyOutputCounts.reserve(std::size_t(groups.size())); d.keptPlain.reserve(std::size_t(plain.size()));
        d.rebases.reserve(std::size_t(rebases.size())); d.outputs.reserve(std::size_t(outputs.size()));
        for (qsizetype i = 0; i < groups.size(); ++i) d.copyOutputCounts.push_back(scan.copies[std::size_t(i)]);
    }
    for (const auto& item : plain) {
        LC_OffsetPlainOutcome p;
        if (!read(item, p) || !productionPlain(p, scan, d, op)) return false;
        if (materialize) d.keptPlain.push_back(p);
    }
    for (const auto& item : rebases) {
        LC_OffsetRebaseOutcome r;
        if (!read(item, r) || !productionRebase(r, scan, d, op)) return false;
        if (materialize) d.rebases.push_back(r);
    }
    std::size_t ordinal = 0, end = scan.copies[0], index = 0;
    const auto cap = produceBudget(op);
    for (const auto& item : outputs) {
        while (index >= end && ordinal + 1 < std::size_t(groups.size())) end += scan.copies[++ordinal];
        QCborArray output, segment;
        std::size_t type;
        Counts c;
        const bool nativeExpected = nativeForCopy(scan, d, op, ordinal);
        if (!array(item, 2, output) || !read(output.at(0), type)) return false;
        if (type == 0) {
            Req::Segment line;
            if (nativeExpected || !array(output.at(1), 2, segment) ||
                !read(segment.at(0), line.start) || !read(segment.at(1), line.end)) return false;
            c = {1, 2, 0};
            if (materialize) d.outputs.emplace_back(line);
        } else if (type == 1) {
            if (nativeExpected || !scanCache(output.at(1), c, cap.maxCubicPieces)) return false;
            if (materialize) {
                auto data = Req::Values::decodeCache(output.at(1).toArray());
                if (!data) return false;
                d.outputs.emplace_back(std::move(*data));
            }
        } else if (type == 2) {
            Curve value;
            if (!nativeExpected || scan.copies[ordinal] != 1 ||
                !readCurve(output.at(1), value, c, cap.maxCubicPieces, materialize) ||
                !productionDefinition(value, ordinal, d, op)) return false;
            if (materialize) d.outputs.emplace_back(std::move(value));
        } else return false;
        if (!ordinaryCount(scan.geometry, c, cap.maxCubicPieces) ||
            !add(scan.minimumDeep, type == 0 ? 1 : kOffsetDeepEntitiesPerOutputSpline - 1,
                d.source.usage.deepEntities)) return false;
        ++index;
    }
    return productionTail(scan, d, op);
}

bool native(const Native& n, const Req::Request& r) {
    if ((r.operation.index() > 1 && r.operation.index() != 4) || !admitted(n.refusal) || !admitted(n.engineStatus) ||
        !admitted(n.checkReason) || !admitted(n.checkWork) || !admitted(n.throughWork)) return false;
    const auto cap = checkLimits(r);
    const auto& c = n.checkWork;
    const auto& t = n.throughWork;
    if (r.operation.index() == 4) {
        return !n.engineStatus && !n.engineRuns && !n.checkReason && zero(c) && !n.curve && !n.changed &&
            zero(n.attemptedOutputUsage) && n.attemptedDeepCountComplete && zero(n.outputUsage) && !n.sampleUnits &&
            t.cacheSpans <= cap.maxCacheSpans && t.vertices <= cap.maxVertices &&
            t.distancePairs <= cap.maxDistancePairs && t.deepest <= cap.maxDepth &&
            t.orderingComparisons <= cap.maxOrderingComparisons &&
            (n.refusal == Refusal::None || n.refusal == Refusal::ThroughNotOnCache ||
                (n.refusal == Refusal::InvalidInput && zero(t)));
    }
    std::size_t vertices = c.vertices, pairs = c.distancePairs;
    if (c.cacheSpans > cap.maxCacheSpans || c.orderingComparisons > cap.maxOrderingComparisons ||
        t.cacheSpans || t.orderingComparisons || !add(vertices, t.vertices, cap.maxVertices) ||
        !add(pairs, t.distancePairs, cap.maxDistancePairs) || c.deepest > cap.maxDepth || t.deepest > cap.maxDepth ||
        (!n.checkReason && !zero(c))) return false;
    const bool success = n.refusal == Refusal::None;
    if (success != bool(n.curve) || success != n.changed || (success && n.checkReason != Reason::None)) return false;
    if (n.refusal == Refusal::CheckFailed && (!n.checkReason || n.checkReason == Reason::None)) return false;
    if (n.checkReason && n.checkReason != Reason::None && n.refusal != Refusal::CheckFailed) return false;
    if (r.operation.index() == 1) {
        return !n.engineStatus && !n.engineRuns && zero(t) && zero(n.attemptedOutputUsage) &&
            n.attemptedDeepCountComplete && zero(n.outputUsage) && !n.sampleUnits &&
            (success || n.refusal == Refusal::InvalidInput || n.refusal == Refusal::CheckFailed ||
                n.refusal == Refusal::NotCacheRepresentable);
    }
    const auto& op = std::get<Req::Generate>(r.operation);
    const auto budget = outputBudget(op);
    const auto samples = op.limits.maxSamples ? std::min(op.limits.maxSamples, LC_CurveOffset::kDefaultMaxSamples) :
        LC_CurveOffset::kDefaultMaxSamples;
    if (n.engineRuns > 1 || bool(n.engineStatus) != (n.engineRuns == 1) ||
        !usage(n.attemptedOutputUsage, budget) || !usage(n.outputUsage, budget) ||
        n.sampleUnits > samples + 1 || (!op.throughPoint && !zero(t)) ||
        (!zero(t) && n.checkReason != Reason::None)) return false;
    if ((!n.attemptedDeepCountComplete && (!n.attemptedOutputUsage.outputEntities ||
        (n.engineStatus != Status::LimitExceeded && n.engineStatus != Status::FitFailed))) ||
        (n.sampleUnits > samples && n.engineStatus != Status::LimitExceeded)) return false;
    if (!n.engineStatus) return !success && !n.checkReason && zero(t) && zero(n.attemptedOutputUsage) &&
        n.attemptedDeepCountComplete && zero(n.outputUsage) && !n.sampleUnits &&
        (n.refusal == Refusal::InvalidInput || n.refusal == Refusal::TooSmall);
    if (*n.engineStatus != Status::Ok) return n.refusal == Refusal::Engine && !n.checkReason &&
        zero(t) && zero(n.outputUsage);
    if (n.refusal == Refusal::Engine || !n.attemptedDeepCountComplete || n.sampleUnits > samples) return false;
    if (n.refusal == Refusal::InvalidInput || n.refusal == Refusal::TooSmall ||
        n.refusal == Refusal::Undefined || n.refusal == Refusal::Inconsistent) return false;
    if ((n.refusal == Refusal::NothingLeft || n.refusal == Refusal::SeveralPieces) && n.checkReason) return false;
    if (success && (n.engineRuns != 1 || n.outputUsage.outputEntities != 1)) return false;
    if (n.refusal == Refusal::ThroughNotOnCache && (!op.throughPoint || n.checkReason != Reason::None)) return false;
    return true;
}
bool nativeCounts(const Counts& c, const Req::Request& r) {
    return c.controls <= checkLimits(r).maxVertices && (r.operation.index() != 0 ||
        c.spans <= std::min(std::get<Req::Generate>(r.operation).limits.outputBudget.maxCubicPieces,
            makeDefaultOffsetSourceBudget().maxCubicPieces));
}
bool readNative(const QCborValue& v, Native& n, const Req::Request& r, bool materialize) {
    QCborArray a;
    if (!array(v, 15, a) || !read(a.at(0), n.refusal) || !read(a.at(1), n.engineStatus) ||
        !read(a.at(2), n.checkReason) || !read(a.at(3), n.checkWork) || !read(a.at(4), n.throughWork) ||
        !read(a.at(5), n.engineRuns) || !read(a.at(6), n.changed) ||
        !read(a.at(8), n.attemptedOutputUsage) || !read(a.at(9), n.attemptedDeepCountComplete) ||
        !read(a.at(10), n.outputUsage) || !read(a.at(11), n.sampleUnits) ||
        !read(a.at(12), n.ordinaryStatus) || !read(a.at(14), n.ordinaryUsage)) return false;
    if (!a.at(7).isNull()) {
        if (r.operation.index() == 4) return false;
        n.curve.emplace();
        Counts c;
        if (!readCurve(a.at(7), *n.curve, c, checkLimits(r).maxCacheSpans, materialize) || !nativeCounts(c, r)) return false;
    }
    return native(n, r) && readOrdinary(a.at(13), n, r, materialize);
}

Trim::Limits trimLimits(const Req::Request& r) {
    auto l = r.operation.index() == 2 ? std::get<Req::TrimCache>(r.operation).limits : std::get<Req::TrimConic>(r.operation).limits;
    const Trim::Limits hard;
#define LC_CAP(f) l.f = std::min(l.f, hard.f)
    LC_CAP(maxSpansPerCurve); LC_CAP(maxInputSpans); LC_CAP(maxInputRecords);
    LC_CAP(maxBoxPairs); LC_CAP(maxLeaves); LC_CAP(maxGeneratedItems); LC_CAP(maxOrderingComparisons);
    LC_CAP(maxRoots); LC_CAP(maxDepth); LC_CAP(maxLeafDepth); LC_CAP(maxFragments);
    LC_CAP(maxOutputSpans); LC_CAP(maxOutputControls); LC_CAP(maxOutputKnots);
#undef LC_CAP
    return l;
}
struct TrimContext {
    Trim::Limits cap;
    std::size_t source = 0, cutter = 0, rootCount = 0;
    bool conic = false, emitCache = false;
    Counts output;
    const RS_SplineData* sourceData = nullptr;
};
TrimContext trimContext(const Req::Request& r, std::size_t roots) {
    TrimContext c{trimLimits(r)};
    c.rootCount = roots;
    const RS_SplineData* data;
    const Req::Cutter* cutter;
    if (r.operation.index() == 2) {
        const auto& op = std::get<Req::TrimCache>(r.operation);
        data = &op.cache; cutter = &op.cutter; c.emitCache = true;
    } else {
        const auto& op = std::get<Req::TrimConic>(r.operation);
        data = &op.cache; cutter = &op.conic; c.emitCache = op.cachePick.has_value();
    }
    c.sourceData = data;
    c.source = sourceSpans(*data);
    c.conic = cutter->index() >= 1 && cutter->index() <= 3;
    c.cutter = c.conic ? 8 : cutter->index() == 0 ? 1 : cutter->index() == 4 ? sourceSpans(std::get<RS_SplineData>(*cutter)) : 0;
    return c;
}
bool work(const Trim::Work& w, const Trim::Limits& l) {
    if (!admitted(w)) return false;
    const auto& s = w.intersections;
    std::size_t pairs = s.candidateBoxPairs, items = w.kernelItems, comparisons = w.kernelComparisons;
    return w.inputSpans <= l.maxInputSpans && w.inputRecords <= l.maxInputRecords && w.rootRecords <= l.maxRoots &&
        w.outputSpans <= l.maxOutputSpans && w.outputControls <= l.maxOutputControls && w.outputKnots <= l.maxOutputKnots &&
        add(pairs, s.refinementBoxPairs, l.maxBoxPairs) && add(pairs, s.certificateBoxPairs, l.maxBoxPairs) && pairs == s.totalBoxPairs &&
        s.generatedLeaves <= l.maxLeaves && add(items, s.generatedItems, l.maxGeneratedItems) &&
        add(comparisons, s.orderingComparisons, l.maxOrderingComparisons) &&
        s.deepestPreparation <= l.maxLeafDepth && s.deepestPairRefinement <= l.maxDepth &&
        s.deepestOrderingRefinement <= l.maxDepth && s.deepestRootRefinement <= l.maxDepth &&
        s.validationNextCalls <= s.validationSegmentVisits && s.validationSegmentVisits <= l.maxGeneratedItems;
}
bool location(const Trim::Location& d, const TrimContext& c) {
    return admitted(d) && d.rawSpan < c.source && d.localParameter.lo() >= 0 && d.localParameter.hi() <= 1;
}
bool rootId(const std::optional<std::size_t>& id, const TrimContext& c) { return !id || *id < c.rootCount; }
bool endpoint(const Trim::Endpoint& d, const TrimContext& c) {
    return admitted(d) && d.labels > 0 && d.labels <= 7 && location(d.source, c) && rootId(d.rootId, c) &&
        bool(d.labels & unsigned(Trim::EndLabel::CutterRoot)) == bool(d.rootId) &&
        (!(d.labels & unsigned(Trim::EndLabel::SourceStart)) || (d.source.rawSpan == 0 &&
            d.source.localParameter.lo() == 0 && d.source.localParameter.hi() == 0)) &&
        (!(d.labels & unsigned(Trim::EndLabel::SourceEnd)) || (d.source.rawSpan == c.source - 1 &&
            d.source.localParameter.lo() == 1 && d.source.localParameter.hi() == 1));
}
bool section(const Trim::Section& d, const TrimContext& c) {
    if (!admitted(d) || d.rawSpan >= c.source || d.from < 0 || d.from >= d.to || d.to > 1 ||
        d.originalDomain.lo() >= d.originalDomain.hi()) return false;
    const auto index = c.sourceData->degree * (d.rawSpan + 1);
    return d.originalDomain.lo() == c.sourceData->knotslist[index] &&
        d.originalDomain.hi() == c.sourceData->knotslist[index + 1];
}
bool root(const Trim::RootProvenance& d, std::size_t index, const TrimContext& c) {
    if (!admitted(d) || d.id != index || !location(d.target, c)) return false;
    const auto& a = d.isolation;
    const auto& b = d.proofDomain;
    return a.segmentA < c.source && a.segmentB >= c.source && a.segmentB - c.source < c.cutter &&
        b.segmentA == a.segmentA && b.segmentB == a.segmentB &&
        a.parameterA.lo() >= double(a.segmentA) && a.parameterA.hi() <= double(a.segmentA + 1) &&
        a.parameterB.lo() >= 0 && a.parameterB.hi() <= (c.conic ? Tau : double(c.cutter)) &&
        (c.conic || (a.parameterB.lo() >= double(a.segmentB - c.source) &&
            a.parameterB.hi() <= double(a.segmentB - c.source + 1))) &&
        (d.proof == LC_IntersectionProof::IntervalUniqueTransverse || d.proof == LC_IntersectionProof::AnalyticEndpoint);
}
bool conic(const Trim::Result::ConicFragment& d, const TrimContext& c) {
    // ORIGINAL axes/ratio and rendering caches are retained, not normalized.
    return admitted(d) && c.conic && d.data.ratio > 0 &&
        (d.data.majorP.x != 0 || d.data.majorP.y != 0) &&
        d.from.lo() >= 0 && d.from.hi() <= Tau && d.to.lo() >= 0 && d.to.hi() <= Tau &&
        rootId(d.startRoot, c) && rootId(d.endRoot, c);
}
bool output(TrimContext& c, const Counts& n) {
    return add(c.output.spans, n.spans, c.cap.maxOutputSpans) &&
        add(c.output.controls, n.controls, c.cap.maxOutputControls) &&
        add(c.output.knots, n.knots, c.cap.maxOutputKnots);
}
bool trimHeader(const Trim::Result& d, std::size_t fragments, std::size_t conics,
                std::size_t roots, const Req::Request& r, TrimContext& c) {
    if (!admitted(d.refusal) || !work(d.work, c.cap) || d.rootCount > c.cap.maxRoots ||
        d.rootCount > d.work.rootRecords || roots > d.rootCount || fragments != d.fragmentCount ||
        fragments > c.cap.maxFragments || conics > c.cap.maxFragments ||
        (r.operation.index() == 2 && conics) || (!c.emitCache && fragments)) return false;
    if (d.refusal != Trim::Refusal::None) return !fragments && !conics && !d.removed;
    if (!c.cap.maxDepth || !c.cap.maxLeafDepth || !c.source || c.source > c.cap.maxSpansPerCurve ||
        !c.cutter || c.cutter > c.cap.maxSpansPerCurve ||
        !roots || roots != d.rootCount || bool(d.removed) != c.emitCache ||
        (c.emitCache && !fragments) || (r.operation.index() == 3 &&
            (!conics || !c.cap.maxOutputControls || !c.cap.maxOutputKnots))) return false;
    return !d.removed || (location(d.removed->from, c) && location(d.removed->to, c));
}
bool trimTail(const Trim::Result& d, const TrimContext& c) {
    if (d.refusal != Trim::Refusal::None) return true;
    return c.output.spans == d.work.outputSpans && c.output.controls == d.work.outputControls &&
        c.output.knots == d.work.outputKnots;
}
bool trim(const Trim::Result& d, const Req::Request& r) {
    auto c = trimContext(r, d.roots.size());
    if (!trimHeader(d, d.fragments.size(), d.conicFragments.size(), d.roots.size(), r, c)) return false;
    for (const auto& f : d.fragments) {
        Counts n;
        if (!cache(f.data, n, c.cap.maxOutputSpans, true) || f.sourceSections.size() != n.spans ||
            !output(c, n) || !endpoint(f.start, c) || !endpoint(f.end, c)) return false;
        for (const auto& s : f.sourceSections) if (!section(s, c)) return false;
    }
    for (const auto& f : d.conicFragments) if (!conic(f, c) || !output(c, {1, 0, 0})) return false;
    for (std::size_t i = 0; i < d.roots.size(); ++i) if (!root(d.roots[i], i, c)) return false;
    return trimTail(d, c);
}
bool readTrim(const QCborValue& v, Trim::Result& d, const Req::Request& r, bool materialize) {
    QCborArray a, fragments, conics, roots;
    const auto cap = trimLimits(r);
    // Admit the outer collections together, before any reserve or cache copy.
    if (!array(v, 8, a) || !bounded(a.at(1), cap.maxFragments, fragments) ||
        !bounded(a.at(2), cap.maxFragments, conics) || !bounded(a.at(3), cap.maxRoots, roots) ||
        !read(a.at(0), d.refusal) || !read(a.at(4), d.removed) || !read(a.at(5), d.rootCount) ||
        !read(a.at(6), d.fragmentCount) || !read(a.at(7), d.work)) return false;
    auto c = trimContext(r, std::size_t(roots.size()));
    if (!trimHeader(d, std::size_t(fragments.size()), std::size_t(conics.size()), std::size_t(roots.size()), r, c)) return false;
    if (materialize) {
        d.fragments.reserve(std::size_t(fragments.size()));
        d.conicFragments.reserve(std::size_t(conics.size()));
        d.roots.reserve(std::size_t(roots.size()));
    }
    for (const auto& item : fragments) {
        QCborArray f, sections;
        Trim::Fragment fragment;
        Counts n;
        if (!array(item, 4, f) || !bounded(f.at(1), cap.maxOutputSpans, sections) ||
            !scanCache(f.at(0), n, cap.maxOutputSpans, true) || std::size_t(sections.size()) != n.spans ||
            !output(c, n) || !read(f.at(2), fragment.start) || !read(f.at(3), fragment.end) ||
            !endpoint(fragment.start, c) || !endpoint(fragment.end, c)) return false;
        if (materialize) {
            auto data = Req::Values::decodeCache(f.at(0).toArray());
            if (!data) return false;
            fragment.data = std::move(*data);
            fragment.sourceSections.reserve(n.spans);
        }
        for (const auto& itemSection : sections) {
            Trim::Section s;
            if (!read(itemSection, s) || !section(s, c)) return false;
            if (materialize) fragment.sourceSections.push_back(s);
        }
        if (materialize) d.fragments.push_back(std::move(fragment));
    }
    for (const auto& item : conics) {
        Trim::Result::ConicFragment f;
        if (!read(item, f) || !conic(f, c) || !output(c, {1, 0, 0})) return false;
        if (materialize) {
            auto data = Req::Values::decodeCutter(QCborArray{3, item.toArray().at(0)});
            if (!data || !std::holds_alternative<RS_EllipseData>(*data)) return false;
            f.data = std::get<RS_EllipseData>(*data);
            d.conicFragments.push_back(std::move(f));
        }
    }
    std::size_t i = 0;
    for (const auto& item : roots) {
        Trim::RootProvenance p;
        if (!read(item, p) || !root(p, i++, c)) return false;
        if (materialize) d.roots.push_back(std::move(p));
    }
    return trimTail(d, c);
}

QCborArray fields(const Curve& d) {
    QCborArray grips;
    for (const auto& p : d.grips) grips.append(fields(p));
    return {*Req::Values::encodeCache(d.cache), *Req::Values::encodeDefinition(d.definition),
        d.worldTolerance, d.closed, grips, fields(d.midpoint)};
}
QCborArray fields(const Native& n) {
    QCborArray ordinary;
    for (const auto& item : n.ordinary) ordinary.append(*Req::Values::encodeCutter(item));
    return {fields(n.refusal), fields(n.engineStatus), fields(n.checkReason), fields(n.checkWork),
        fields(n.throughWork), fields(n.engineRuns), n.changed,
        n.curve ? QCborValue(fields(*n.curve)) : QCborValue(nullptr), fields(n.attemptedOutputUsage),
        n.attemptedDeepCountComplete, fields(n.outputUsage), fields(n.sampleUnits),
        fields(n.ordinaryStatus), ordinary, fields(n.ordinaryUsage)};
}
QCborArray fields(const Produced& d) {
    QCborArray copies, plain, rebases, outputs;
    for (auto n : d.copyOutputCounts) copies.append(fields(n));
    for (const auto& p : d.keptPlain) plain.append(fields(p));
    for (const auto& r : d.rebases) rebases.append(fields(r));
    for (const auto& item : d.outputs) {
        QCborValue value;
        if (const auto* line = std::get_if<Req::Segment>(&item))
            value = QCborArray{fields(line->start), fields(line->end)};
        else if (const auto* data = std::get_if<RS_SplineData>(&item)) value = *Req::Values::encodeCache(*data);
        else value = fields(std::get<Curve>(item));
        outputs.append(QCborArray{qint64(item.index()), value});
    }
    return {fields(d.source), copies, plain, rebases, fields(d.outputStatus), outputs};
}
QCborArray fields(const Trim::Result& d) {
    QCborArray fragments, conics, roots;
    for (const auto& f : d.fragments) {
        QCborArray sections;
        for (const auto& s : f.sourceSections) sections.append(fields(s));
        fragments.append(QCborArray{*Req::Values::encodeCache(f.data), sections, fields(f.start), fields(f.end)});
    }
    for (const auto& f : d.conicFragments) {
        // Reuse the request's ellipse value writer, including rendering caches.
        conics.append(QCborArray{Req::Values::encodeCutter(Req::Cutter{f.data})->at(1),
            fields(f.from), fields(f.to), fields(f.startRoot), fields(f.endRoot), f.crossesStorageSeam});
    }
    for (const auto& p : d.roots) roots.append(fields(p));
    return {fields(d.refusal), fragments, conics, roots, fields(d.removed), fields(d.rootCount),
        fields(d.fragmentCount), fields(d.work)};
}
bool token(const Req::Token& a, const Req::Token& b) {
    return a.requestId > 0 && a.generation >= 0 && a.sourceRevision >= 0 && a.undoEpoch >= 0 &&
        a.requestId == b.requestId && a.generation == b.generation &&
        a.sourceRevision == b.sourceRevision && a.undoEpoch == b.undoEpoch;
}
bool envelope(const Reply& d, const Req::Request& r) {
    return !r.operation.valueless_by_exception() && d.operation <= 5 && d.operation == r.operation.index() &&
        token(d.token, r.token) && admitted(d.failure) && !d.result.valueless_by_exception();
}

// All variable counts have already been admitted when this is used. With only
// numeric/bool/null records, the item ceiling also keeps bytes below 16 MiB.
template<class T> std::size_t itemCount(const T& d) {
    if constexpr (std::is_arithmetic_v<T> || std::is_enum_v<T>) return 1;
    else if constexpr (IsOptional<T>::value) return d ? itemCount(*d) : 1;
    else if constexpr (std::is_same_v<T, LC_Interval>) return 3;
    else if constexpr (std::is_same_v<T, RS_SplineData>)
        return 11 + 5 * d.controlPoints.size() + d.knotslist.size() + d.weights.size();
    else if constexpr (std::is_same_v<T, Curve>)
        return 1 + itemCount(d.cache) + itemCount(d.definition) + 2 + 1 + 5 * d.grips.size() + itemCount(d.midpoint);
    else if constexpr (std::is_same_v<T, Produced>) {
        std::size_t n = 1 + itemCount(d.source) + 1 + d.copyOutputCounts.size() + 1 + 1 + 1 + 1;
        for (const auto& p : d.keptPlain) n += itemCount(p);
        for (const auto& r : d.rebases) n += itemCount(r);
        for (const auto& item : d.outputs) n += 2 + std::visit([](const auto& value) {
            using V = std::decay_t<decltype(value)>;
            if constexpr (std::is_same_v<V, Req::Segment>) return std::size_t{11};
            else return itemCount(value);
        }, item);
        return n;
    }
    else if constexpr (std::is_same_v<T, Native>) {
        std::size_t ordinary = 1;
        for (const auto& item : d.ordinary) ordinary += 2 + std::visit([](const auto& value) {
            using V = std::decay_t<decltype(value)>;
            if constexpr (std::is_same_v<V, Req::Segment>) return std::size_t{11};
            else if constexpr (std::is_same_v<V, RS_SplineData>) return itemCount(value);
            else return std::size_t{0}; // Unsupported variants were already refused.
        }, item);
        return 1 + itemCount(d.refusal) + itemCount(d.engineStatus) + itemCount(d.checkReason) +
            itemCount(d.checkWork) + itemCount(d.throughWork) + 2 + itemCount(d.curve) +
            itemCount(d.attemptedOutputUsage) + 1 + itemCount(d.outputUsage) + 2 + ordinary + itemCount(d.ordinaryUsage);
    }
    else if constexpr (std::is_same_v<T, Trim::Result>) {
        std::size_t n = 1 + 1 + 3 + itemCount(d.removed) + 2 + itemCount(d.work);
        for (const auto& f : d.fragments) {
            n += 1 + itemCount(f.data) + 1 + itemCount(f.start) + itemCount(f.end);
            for (const auto& s : f.sourceSections) n += itemCount(s);
        }
        for (const auto& f : d.conicFragments) n += itemCount(f);
        for (const auto& p : d.roots) n += itemCount(p);
        return n;
    } else {
        auto copy = d;
        return std::apply([](const auto&... f) { return std::size_t{1} + (itemCount(f) + ...); }, members(copy));
    }
}
bool wireValue(const QCborValue& v, std::size_t& count, unsigned depth = 1) {
    const LC_OffsetWorkerWire::Limits hard;
    if (depth > hard.depth || !add(count, 1, hard.items)) return false;
    if (v.isArray()) {
        const auto a = v.toArray();
        if (quint64(a.size()) > hard.arrayItems || std::size_t(a.size()) > hard.items - count) return false;
        for (const auto& child : a) if (!wireValue(child, count, depth + 1)) return false;
        return true;
    }
    return (v.isDouble() && std::isfinite(v.toDouble())) || v.isInteger() || v.isBool() || v.isNull();
}
}

bool admittedProduced(const Produced& d, const Req::Produce& op) {
    return op.copies >= 1 && op.copies <= RS_OffsetData::kMaximumOffsetCopies &&
        produced(d, op) && itemCount(d) <= LC_OffsetWorkerWire::Limits{}.items - 9;
}

std::optional<QCborArray> encode(const Reply& d, const Req::Request& r) {
    if (!envelope(d, r)) return std::nullopt;
    // The complete caller-owned payload is checked before constructing CBOR.
    if (d.failure != Failure::None) {
        if (!std::holds_alternative<std::monostate>(d.result)) return std::nullopt;
    } else if (d.operation == 5) {
        const auto* value = std::get_if<Produced>(&d.result);
        if (!Req::admitted(r) || !value || !admittedProduced(*value, std::get<Req::Produce>(r.operation))) return std::nullopt;
    } else if (d.operation <= 1 || d.operation == 4) {
        const auto* n = std::get_if<Native>(&d.result);
        Counts c;
        if (!n || !native(*n, r) || !ordinary(*n, r) || (n->curve && (!curve(*n->curve, c, checkLimits(r).maxCacheSpans) ||
            !nativeCounts(c, r))) || itemCount(*n) > LC_OffsetWorkerWire::Limits{}.items - 9) return std::nullopt;
    } else {
        const auto* t = std::get_if<Trim::Result>(&d.result);
        if (!t || !trim(*t, r) || itemCount(*t) > LC_OffsetWorkerWire::Limits{}.items - 9) return std::nullopt;
    }
    QCborValue payload(nullptr);
    if (d.failure == Failure::None) {
        if (d.operation == 5) payload = fields(std::get<Produced>(d.result));
        else payload = d.operation <= 1 || d.operation == 4 ?
            fields(std::get<Native>(d.result)) : fields(std::get<Trim::Result>(d.result));
    }
    const auto& t = d.token;
    return QCborArray{4, QCborArray{t.requestId, t.generation, t.sourceRevision, t.undoEpoch},
        qint64(d.operation), fields(d.failure), payload};
}

std::optional<Reply> decode(const QCborArray& a, const Req::Request& r) {
    Reply d;
    QCborArray t;
    std::size_t version;
    std::size_t items = 0;
    if (a.size() != 5 || !wireValue(a, items) || !read(a.at(0), version) || version != 4 || !array(a.at(1), 4, t) ||
        !read(t.at(0), d.token.requestId) || !read(t.at(1), d.token.generation) ||
        !read(t.at(2), d.token.sourceRevision) || !read(t.at(3), d.token.undoEpoch) ||
        !read(a.at(2), d.operation) || !read(a.at(3), d.failure) || !envelope(d, r)) return std::nullopt;
    if (d.failure != Failure::None) return a.at(4).isNull() ? std::optional<Reply>(std::move(d)) : std::nullopt;
    // First pass admits every nested collection, scalar and aggregate without
    // allocating native arrays. Only then may shared value readers copy them.
    if (d.operation == 5) {
        if (!Req::admitted(r)) return std::nullopt;
        Produced scan, value;
        const auto& op = std::get<Req::Produce>(r.operation);
        if (!readProduced(a.at(4), scan, op, false) || !readProduced(a.at(4), value, op, true)) return std::nullopt;
        d.result = std::move(value);
    } else if (d.operation <= 1 || d.operation == 4) {
        Native scan, value;
        if (!readNative(a.at(4), scan, r, false) || !readNative(a.at(4), value, r, true)) return std::nullopt;
        d.result = std::move(value);
    } else {
        Trim::Result scan, value;
        if (!readTrim(a.at(4), scan, r, false) || !readTrim(a.at(4), value, r, true)) return std::nullopt;
        d.result = std::move(value);
    }
    return d;
}
}
