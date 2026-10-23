/*
 * This file is part of LibreCAD, a 2D CAD program.
 * Copyright (C) 2026 LibreCAD.org
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "lc_offsetcurve.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <type_traits>

#include <boost/multiprecision/cpp_int.hpp>

#include "rs_line.h"

namespace geo = LC_EllipseOffsetGeometry;
namespace {
using Exact = boost::multiprecision::cpp_rational;
using Matrix = std::array<double, 6>;

bool exactValue(const Exact &value, double &out, const bool requireExact) {
    out = value.convert_to<double>();
    return std::isfinite(out) && (!requireExact || Exact(out) == value);
}

bool mapPoint(const Matrix &m, const RS_Vector &point, RS_Vector &out, const bool requireExact,
              const bool displacement = false) {
    if (!geo::isFinite2D(point)) return false;
    double x, y;
    if (!exactValue(Exact(m[0])*Exact(point.x) + Exact(m[1])*Exact(point.y) + (displacement ? Exact(0) : Exact(m[4])), x, requireExact) ||
        !exactValue(Exact(m[2])*Exact(point.x) + Exact(m[3])*Exact(point.y) + (displacement ? Exact(0) : Exact(m[5])), y, requireExact)) return false;
    out = RS_Vector{x, y};
    return true;
}

bool samePoint(const RS_Vector &a, const RS_Vector &b) {
    return a.x == b.x && a.y == b.y && a.z == b.z && a.valid == b.valid;
}

bool sameBasisGeometry(const RS_EllipseData &a, const RS_EllipseData &b) {
    return samePoint(a.center, b.center) && samePoint(a.majorP, b.majorP) && a.ratio == b.ratio &&
        a.angle1 == b.angle1 && a.angle2 == b.angle2 && a.reversed == b.reversed;
}

bool sameDefinition(const LC_EllipseOffsetDefinition &a, const LC_EllipseOffsetDefinition &b) {
    return sameBasisGeometry(a.basis, b.basis) && a.distance == b.distance &&
        a.towardCentre == b.towardCentre && a.cacheReversed == b.cacheReversed && a.relTolerance == b.relTolerance;
}

bool normalizedDefinition(LC_EllipseOffsetDefinition &definition) {
    return std::isfinite(definition.distance) && definition.distance > 0 &&
        std::isfinite(definition.relTolerance) && definition.relTolerance > 0 && geo::normaliseBasis(definition.basis);
}

LC_OffsetCheckLimits clampedLimits(const LC_OffsetCheckLimits &limits) {
    const LC_OffsetCheckLimits hard;
    auto result = limits;
    result.maxCacheSpans = std::min(limits.maxCacheSpans, hard.maxCacheSpans);
    result.maxVertices = std::min(limits.maxVertices, hard.maxVertices);
    result.maxDistancePairs = std::min(limits.maxDistancePairs, hard.maxDistancePairs);
    result.maxDepth = std::min(limits.maxDepth, hard.maxDepth);
    result.maxOrderingComparisons = std::min(limits.maxOrderingComparisons, hard.maxOrderingComparisons);
    return result;
}

LC_OffsetSourceBudget clampedOutputBudget(const LC_OffsetSourceBudget &budget) {
    const auto hard = makeDefaultOffsetSourceBudget();
    return {std::min(budget.maxCubicPieces, hard.maxCubicPieces),
            std::min(budget.maxOutputEntities, hard.maxOutputEntities),
            std::min(budget.maxDeepEntities, hard.maxDeepEntities)};
}

bool fitsOutputBudget(const RS_Spline &cache, const LC_OffsetSourceBudget &budget) {
    if (!isValidOffsetBudget(budget) || budget.maxDeepEntities < kOffsetDeepEntitiesPerOutputSpline - 1) return false;
    const auto count = geo::cacheLayoutCountsFromControls(cache.getData().degree, cache.getData().controlPoints.size());
    return count && count->spans <= budget.maxCubicPieces &&
        measureOffsetOutput({&cache}, budget.maxDeepEntities).status == LC_OffsetTreeStatus::Ok;
}

const RS_SplineData *cacheData(const RS_Entity &entity, RS_SplineData &lineData) {
    if (const auto *spline = dynamic_cast<const RS_Spline *>(&entity)) return &spline->getData();
    if (const auto *line = dynamic_cast<const RS_Line *>(&entity)) {
        lineData.degree = 1;
        lineData.controlPoints = {line->getStartpoint(), line->getEndpoint()};
        lineData.knotslist = {0, 0, 1, 1}; lineData.weights = {1, 1};
        return &lineData;
    }
    return nullptr;
}

bool containsThroughPoint(const RS_Spline &cache, const RS_Vector &point, const double tolerance,
                          const std::size_t maxVertices, const std::size_t maxPairs, const unsigned maxDepth) {
    if (!geo::isFinite2D(point) || !std::isfinite(tolerance) || !(tolerance > 0) || maxVertices == 0 || maxPairs == 0)
        return false;
    const Exact px{point.x}, py{point.y}, squaredTolerance = Exact(tolerance)*Exact(tolerance);
    std::size_t vertices = 0, pairs = 0;
    const auto debit = [](std::size_t &used, const std::size_t cap, const std::size_t count = 1) {
        if (count > cap - used) return false;
        used += count; return true;
    };
    // C admitted a clamped line/Bezier chain with unit weights and degree-fold
    // interior knots. Its span endpoints are these exact stored control points.
    const auto &data = cache.getData();
    for (std::size_t i = 0; i < data.controlPoints.size(); i += data.degree) {
        if (!debit(vertices, maxVertices) || !debit(pairs, maxPairs)) return false;
        const auto &p = data.controlPoints[i];
        const Exact dx = px - Exact(p.x), dy = py - Exact(p.y);
        if (dx*dx + dy*dy <= squaredTolerance) return true;
    }
    const auto breaks = cache.getBreakParameters();
    struct Domain { double a, b; unsigned depth; };
    std::vector<Domain> pending;
    pending.reserve(std::min(maxVertices, static_cast<std::size_t>(maxDepth) + 2));
    const auto distanceBounds = [&](const LC_CurveJetBounds &bounds, const bool maximum) -> std::optional<Exact> {
        if (!bounds.x.isValid() || !bounds.y.isValid() || !std::isfinite(bounds.x.lo()) ||
            !std::isfinite(bounds.x.hi()) || !std::isfinite(bounds.y.lo()) || !std::isfinite(bounds.y.hi())) return std::nullopt;
        const auto delta = [maximum](const Exact &p, const LC_Interval &interval) -> Exact {
            const Exact lo{interval.lo()}, hi{interval.hi()};
            if (!maximum) return p < lo ? Exact(lo - p) : p > hi ? Exact(p - hi) : Exact(0);
            const Exact a = abs(p - lo), b = abs(p - hi);
            return std::max(a, b);
        };
        const Exact x = delta(px, bounds.x), y = delta(py, bounds.y);
        return Exact(x*x + y*y);
    };
    for (std::size_t span = 1; span < breaks.size(); ++span) {
        if (!debit(vertices, maxVertices)) return false;
        pending.assign(1, {breaks[span - 1], breaks[span], 0});
        while (!pending.empty()) {
            const auto cell = pending.back(); pending.pop_back();
            if (!debit(pairs, maxPairs)) return false;
            LC_CurveJetBounds bounds;
            if (!cache.tryBoundJet(cell.a, cell.b, bounds)) continue;
            const auto lower = distanceBounds(bounds, false);
            if (lower && *lower > squaredTolerance) continue;
            if (!debit(pairs, maxPairs)) return false;
            // This nonempty cell contains actual curve points. Its entire
            // outward position box being inside the target disk is a witness;
            // neither a rounded display vertex nor a zero-width query is used.
            const auto upper = distanceBounds(bounds, true);
            if (upper && *upper <= squaredTolerance) return true;
            const double mid = .5*cell.a + .5*cell.b;
            if (cell.depth >= maxDepth || !(mid > cell.a && mid < cell.b)) continue;
            if (!debit(vertices, maxVertices, 2)) return false;
            pending.push_back({mid, cell.b, cell.depth + 1});
            pending.push_back({cell.a, mid, cell.depth + 1});
        }
    }
    return false;
}

bool boundedCache(const RS_SplineData &data, const LC_OffsetCheckLimits &limits) {
    const auto counts = geo::cacheLayoutCountsFromControls(data.degree, data.controlPoints.size());
    return counts && counts->spans <= limits.maxCacheSpans && counts->controls <= limits.maxVertices &&
        data.knotslist.size() == counts->knots && data.weights.size() == counts->controls &&
        data.fitPoints.empty() && data.savedOpenKnots.empty();
}

bool boundedRepresentation(const RS_SplineData &data) noexcept {
    const LC_OffsetCheckLimits limits;
    return data.degree >= 1 && data.degree <= 3 && data.controlPoints.size() <= limits.maxVertices &&
        data.fitPoints.size() <= limits.maxVertices && data.weights.size() <= limits.maxVertices &&
        data.knotslist.size() <= 4*limits.maxVertices && data.savedOpenKnots.size() <= 4*limits.maxVertices;
}

bool isSignedPermutation(const Matrix &m, const double scale) {
    if (!(scale > 0) || !std::isfinite(scale)) return false;
    const auto entry = [scale](const double x) { return x == 0 || std::abs(x) == scale; };
    return std::all_of(m.begin(), m.begin() + 4, entry) &&
        ((m[0] != 0 && m[3] != 0 && m[1] == 0 && m[2] == 0) ||
         (m[1] != 0 && m[2] != 0 && m[0] == 0 && m[3] == 0));
}

bool setTranslation(Matrix &matrix, const RS_Vector &center, const bool exact) {
    return geo::isFinite2D(center) &&
        exactValue(Exact(center.x) - Exact(matrix[0])*Exact(center.x) - Exact(matrix[1])*Exact(center.y), matrix[4], exact) &&
        exactValue(Exact(center.y) - Exact(matrix[2])*Exact(center.x) - Exact(matrix[3])*Exact(center.y), matrix[5], exact);
}
}

std::unique_ptr<LC_OffsetCurve> LC_OffsetCurve::adopt(const RS_Entity &cache,
    const LC_EllipseOffsetDefinition &definition, const double tolerance,
    LC_OffsetCheckResult *outcome, const LC_OffsetCheckLimits &limits, LC_OffsetCurveRefusal *refusal) {
    LC_OffsetCheckResult checked;
    auto nativeRefusal = LC_OffsetCurveRefusal::InvalidInput;
    auto normalized = definition;
    std::unique_ptr<LC_OffsetCurve> result;
    const auto effective = clampedLimits(limits);
    if (geo::normaliseBasis(normalized.basis)) {
        RS_SplineData lineData;
        const RS_SplineData *data = cacheData(cache, lineData);
        if (data) {
            // C admits raw counts before allocating or copying any cache array.
            checked = geo::checkCache(*data, normalized, tolerance, effective);
            nativeRefusal = LC_OffsetCurveRefusal::CheckFailed;
            if (checked) {
                // Exact Bezier domains may be narrower than the ordinary
                // display validator's knot resolution. Never certify a native
                // entity whose unchanged cache cannot be displayed as a spline.
                const auto *spline = dynamic_cast<const RS_Spline *>(&cache);
                if (spline && !spline->validate()) {
                    nativeRefusal = LC_OffsetCurveRefusal::NotCacheRepresentable;
                } else {
                    result.reset(new LC_OffsetCurve(cache, *data, normalized, tolerance,
                        checked.analysis->closed(), ValidatedCache{}));
                    nativeRefusal = LC_OffsetCurveRefusal::None;
                }
            }
        } else nativeRefusal = LC_OffsetCurveRefusal::NotCacheRepresentable;
    }
    if (outcome) *outcome = std::move(checked);
    if (refusal) *refusal = nativeRefusal;
    return result;
}

LC_OffsetCurveCreateResult LC_OffsetCurve::runEngine(const RS_Entity &attributes,
    const LC_EllipseOffsetDefinition &definition, const LC_OffsetRunLimits &limits) {
    using Refusal = LC_OffsetCurveRefusal;
    LC_OffsetCurveCreateResult result;
    auto normalized = definition;
    const auto checkLimits = clampedLimits(limits.checkLimits);
    geo::WorkBudget admission{checkLimits};
    if (!normalizedDefinition(normalized) || !isValidOffsetBudget(limits.outputBudget) ||
        admission.status() != geo::BudgetStatus::Ok) return result;
    const RS_Ellipse source{nullptr, normalized.basis};
    const double tolerance = LC_CurveOffset::toleranceFor(source, normalized.distance, normalized.relTolerance);
    if (!std::isfinite(tolerance) || !(tolerance > 0)) return result;
    if (normalized.distance <= tolerance) { result.refusal = Refusal::TooSmall; return result; }
    auto options = LC_CurveOffset::makeOffsetOptions(source, normalized.distance, tolerance);
    if (!std::isfinite(options.tolerance.requestedGeometry) || !(options.tolerance.requestedGeometry > 0) ||
        options.maxSamples == 0 || options.maxIntersectionPairs == 0) return result;
    if (limits.maxSamples) options.maxSamples = std::min(limits.maxSamples, options.maxSamples);
    if (limits.maxIntersectionPairs) options.maxIntersectionPairs = std::min(limits.maxIntersectionPairs, options.maxIntersectionPairs);
    const auto output = clampedOutputBudget(limits.outputBudget);
    const auto side = (normalized.towardCentre != normalized.basis.reversed) ? LC_CurveOffsetSide::Left : LC_CurveOffsetSide::Right;
    ++result.engineRuns;
    auto made = LC_CurveOffset::createEntities(source, LC_CurveOffset::makeSideRequest(side, normalized.distance), options, output);
    result.engineStatus = made.status;
    if (made.status != LC_CurveOffsetStatus::Ok) { result.refusal = Refusal::Engine; return result; }
    if (made.entities.empty()) { result.refusal = Refusal::NothingLeft; return result; }
    if (made.entities.size() != 1) { result.refusal = Refusal::SeveralPieces; return result; }
    if (!made.entities.front()) { result.refusal = Refusal::NotCacheRepresentable; return result; }
    auto &entity = *made.entities.front();
    if (normalized.cacheReversed) entity.revertDirection();
    RS_SplineData line;
    const auto *data = cacheData(entity, line);
    if (!data || (data->degree != 1 && data->degree != 3)) {
        result.refusal = Refusal::NotCacheRepresentable; return result;
    }
    auto checked = geo::checkCache(*data, normalized, tolerance, checkLimits);
    result.checkReason = checked.reason; result.checkWork = checked.work;
    if (!checked) { result.refusal = Refusal::CheckFailed; return result; }
    // A native line is still drawn as a spline, not the producer's one-leaf
    // RS_Line. Admit its minimum display expansion before constructing it.
    if (output.maxDeepEntities < kOffsetDeepEntitiesPerOutputSpline - 1) {
        result.refusal = Refusal::NotCacheRepresentable; return result;
    }
    result.entity.reset(new LC_OffsetCurve(attributes, *data, normalized, tolerance,
        checked.analysis->closed(), ValidatedCache{}));
    if (!result.entity->validate() || !fitsOutputBudget(*result.entity, output)) {
        result.entity.reset(); result.refusal = Refusal::NotCacheRepresentable; return result;
    }
    result.refusal = Refusal::None; result.changed = true;
    return result;
}

LC_OffsetCurveCreateResult LC_OffsetCurve::create(const RS_Ellipse &source, const double distance,
    const bool towardCentre, const LC_OffsetRunLimits &limits, const double relativeTolerance, const bool cacheReversed) {
    LC_EllipseOffsetDefinition definition;
    definition.basis = source.getData(); definition.distance = distance; definition.towardCentre = towardCentre;
    definition.relTolerance = relativeTolerance == 0 ? LC_CurveOffset::kEllipseRelativeOffsetTolerance : relativeTolerance;
    definition.cacheReversed = cacheReversed;
    auto result = runEngine(source, definition, limits);
    if (result) {
        // Creation is a new object, not a replacement of the source's DWG graph identity.
        result.entity->setSourceHandle(0); result.entity->setXDictHandle(0); result.entity->setReactorHandles({});
        result.entity->setLineage(RS_Entity::newLineage()); result.entity->setOwnerLineage(0);
    }
    return result;
}

LC_OffsetCurveEditResult LC_OffsetCurve::eligibility() const noexcept {
    LC_OffsetCurveEditResult result;
    result.refusal = !isDefined() ? LC_OffsetCurveRefusal::Undefined :
        !isConsistent() ? LC_OffsetCurveRefusal::Inconsistent : LC_OffsetCurveRefusal::None;
    return result;
}

LC_OffsetCurveEditResult LC_OffsetCurve::commitPrepared(LC_OffsetCurveCreateResult &prepared) {
    auto result = static_cast<const LC_OffsetCurveEditResult &>(prepared);
    if (!prepared) return result;
    auto &next = *prepared.entity;
    const auto definition = next.m_definition;
    auto grips = std::move(next.m_grips);
    publish(next, definition, grips, next.m_worldTolerance, true, next.m_closed);
    return result;
}

LC_OffsetCurveEditResult LC_OffsetCurve::edit(const LC_EllipseOffsetDefinition &definition,
    const LC_OffsetRunLimits &limits, const RS_Vector *through) {
    using Refusal = LC_OffsetCurveRefusal;
    auto result = eligibility();
    if (!result) return result;
    auto normalized = definition;
    if (!normalizedDefinition(normalized)) { result.refusal = Refusal::InvalidInput; return result; }
    if (sameDefinition(normalized, m_definition)) {
        if (through) {
            const auto cap = clampedLimits(limits.checkLimits);
            if (!containsThroughPoint(*this, *through, m_worldTolerance, cap.maxVertices, cap.maxDistancePairs, cap.maxDepth))
                result.refusal = Refusal::ThroughNotOnCache;
        }
        return result;
    }
    // A rigid fast path must reproduce every normalized semantic field. Never
    // infer one from axis lengths or a broad angular/coordinate tolerance.
    auto translated = m_definition;
    translated.basis.center = normalized.basis.center;
    const auto output = clampedOutputBudget(limits.outputBudget);
    if (!through && sameDefinition(translated, normalized) && fitsOutputBudget(*this, output)) {
        double dx, dy;
        if (exactValue(Exact(normalized.basis.center.x) - Exact(m_definition.basis.center.x), dx, true) &&
            exactValue(Exact(normalized.basis.center.y) - Exact(m_definition.basis.center.y), dy, true)) {
            auto staged = std::unique_ptr<LC_OffsetCurve>{static_cast<LC_OffsetCurve *>(clone())};
            staged->move({dx, dy});
            if (staged->hasEditableDefinition() && sameDefinition(staged->definition(), normalized) &&
                fitsOutputBudget(*staged, output)) {
                LC_OffsetCurveCreateResult prepared;
                prepared.refusal = Refusal::None; prepared.changed = true; prepared.entity = std::move(staged);
                return commitPrepared(prepared);
            }
        }
    }
    auto prepared = runEngine(*this, normalized, limits);
    if (!prepared) return static_cast<const LC_OffsetCurveEditResult &>(prepared);
    if (through) {
        const auto cap = clampedLimits(limits.checkLimits);
        const auto vertices = cap.maxVertices - prepared.checkWork.vertices;
        const auto pairs = cap.maxDistancePairs - prepared.checkWork.distancePairs;
        if (!containsThroughPoint(*prepared.entity, *through, prepared.entity->m_worldTolerance, vertices, pairs, cap.maxDepth)) {
            auto refused = static_cast<const LC_OffsetCurveEditResult &>(prepared);
            refused.refusal = Refusal::ThroughNotOnCache; refused.changed = false;
            return refused;
        }
    }
    return commitPrepared(prepared);
}

LC_OffsetCurveEditResult LC_OffsetCurve::setDefinition(const LC_EllipseOffsetDefinition &definition,
    const LC_OffsetRunLimits &limits) { return edit(definition, limits); }

LC_OffsetCurveEditResult LC_OffsetCurve::setDistance(const double distance, const LC_OffsetRunLimits &limits) {
    auto next = m_definition; next.distance = distance; return edit(next, limits);
}

LC_OffsetCurveEditResult LC_OffsetCurve::setTowardCentre(const bool towardCentre, const LC_OffsetRunLimits &limits) {
    auto next = m_definition; next.towardCentre = towardCentre; return edit(next, limits);
}

LC_OffsetCurveEditResult LC_OffsetCurve::setBasis(const RS_EllipseData &basis, const LC_OffsetRunLimits &limits) {
    auto next = m_definition; next.basis = basis; return edit(next, limits);
}

std::optional<LC_OffsetThroughProposal> LC_OffsetCurve::proposeThrough(const RS_Vector &point) const {
    if (!hasEditableDefinition() || !geo::isFinite2D(point)) return std::nullopt;
    double parameter;
    const double distance = geo::distanceToOriginal(m_definition.basis, point, &parameter);
    const auto sweep = geo::sweep(m_definition.basis);
    if (!std::isfinite(distance) || !(distance > 0) || !std::isfinite(parameter) || !sweep || !sweep->contains(parameter))
        return std::nullopt;
    return LC_OffsetThroughProposal{distance, geo::isInside(m_definition.basis, point), parameter};
}

LC_OffsetCurveEditResult LC_OffsetCurve::setThrough(const RS_Vector &point, const LC_OffsetRunLimits &limits) {
    auto result = eligibility();
    if (!result) return result;
    const auto proposal = proposeThrough(point);
    if (!proposal) { result.refusal = LC_OffsetCurveRefusal::InvalidInput; return result; }
    const double localScale = std::max({std::hypot(m_definition.basis.majorP.x, m_definition.basis.majorP.y),
                                       m_definition.distance, proposal->distance});
    const double projectionRoundoff = 32*std::numeric_limits<double>::epsilon()*localScale;
    if (proposal->towardCentre == m_definition.towardCentre &&
        std::abs(proposal->distance - m_definition.distance) <= projectionRoundoff) {
        // Projection is a numerical proposal. An exact stored Bezier endpoint
        // is already an actual curve point, so its few local projection ulps
        // must not manufacture a meaningful edit or clear preserved metadata.
        const auto &data = getData();
        for (std::size_t i = 0; i < data.controlPoints.size(); i += data.degree)
            if (samePoint(data.controlPoints[i], point)) return result;
    }
    auto next = m_definition; next.distance = proposal->distance; next.towardCentre = proposal->towardCentre;
    return edit(next, limits, &point);
}

LC_OffsetCurve::LC_OffsetCurve(const RS_Entity &source, const RS_SplineData &cache,
    const LC_EllipseOffsetDefinition &definition, const double tolerance, const bool closed, ValidatedCache)
    : RS_Spline(source.getParent(), cache), m_definition(definition), m_worldTolerance(tolerance), m_closed(closed) {
    RS_Entity::operator=(source);
    calculateBorders();
    m_stamp = geometryFingerprint(getData());
    // D-grips adds visible analytic grips. These cache-owned endpoints are a
    // bounded initial fallback, never borrowed from the source entity.
    m_grips.push_back(getData().controlPoints.front());
    if (!samePoint(getData().controlPoints.front(), getData().controlPoints.back()))
        m_grips.push_back(getData().controlPoints.back());
}

RS_Entity *LC_OffsetCurve::clone() const { return new LC_OffsetCurve(*this); }

bool LC_OffsetCurve::isConsistent() const noexcept {
    return m_defined && boundedRepresentation(getData()) && m_stamp == geometryFingerprint(getData());
}

bool LC_OffsetCurve::sourceIsWhole() const noexcept {
    return m_definition.basis.angle1 == 0 && m_definition.basis.angle2 == 0;
}

std::unique_ptr<RS_Spline> LC_OffsetCurve::toPlainSpline() const {
    return std::make_unique<RS_Spline>(static_cast<const RS_Spline &>(*this));
}

std::unique_ptr<RS_Ellipse> LC_OffsetCurve::originalEllipse() const {
    return std::make_unique<RS_Ellipse>(nullptr, m_definition.basis);
}

bool LC_OffsetCurve::setOwnedExtensions(const unsigned flags, const OwnedTail &tail) {
    if ((flags & ~0x7ff0u) || tail.size() > 64) return false;
    std::size_t bytes = 0;
    unsigned depth = 0;
    for (const auto &item : tail) {
        std::size_t size = 0;
        if (item.code == 1000 || item.code == 1002) {
            const auto *text = std::get_if<QString>(&item.value);
            if (!text || text->size() > 1023) return false;
            size = 3 + 4*static_cast<std::size_t>(text->size());
            if (item.code == 1002) {
                if (*text == "{") { if (++depth > 8) return false; }
                else if (*text == "}") { if (!depth) return false; --depth; }
                else return false;
            }
        } else if (item.code >= 1010 && item.code <= 1013) {
            const auto *point = std::get_if<RS_Vector>(&item.value);
            if (!point || !point->valid || !std::isfinite(point->x) || !std::isfinite(point->y) || !std::isfinite(point->z)) return false;
            size = 25;
        } else if (item.code >= 1040 && item.code <= 1042) {
            const auto *number = std::get_if<double>(&item.value);
            if (!number || !std::isfinite(*number)) return false;
            size = 9;
        } else if (item.code == 1070 && std::holds_alternative<std::int16_t>(item.value)) size = 3;
        else if (item.code == 1071 && std::holds_alternative<std::int32_t>(item.value)) size = 5;
        else return false;
        if (size > 4096 - bytes) return false;
        bytes += size;
    }
    if (depth) return false;
    auto owned = tail;
    m_unknownTail.swap(owned); m_unknownFlags = flags;
    return true;
}

void LC_OffsetCurve::publish(RS_Spline &prepared, const LC_EllipseOffsetDefinition &definition,
    std::vector<RS_Vector> &grips, const double tolerance, const bool defined, const bool closed) {
    static_assert(std::is_nothrow_copy_assignable_v<LC_EllipseOffsetDefinition>);
    const auto stamp = geometryFingerprint(prepared.getData());
    auto notification = replacePreparedSpline(prepared);
    m_definition = definition; m_grips.swap(grips); m_worldTolerance = tolerance;
    m_defined = defined; m_stamp = stamp; m_closed = closed;
    m_unknownFlags = 0; m_unknownTail.clear();
    notification.notify();
}

void LC_OffsetCurve::update() {
    if (!boundedRepresentation(getData())) return;
    RS_Spline prepared{nullptr, getData()};
    auto notification = replacePreparedSpline(prepared);
    // In particular, do not restamp an externally edited getData() payload.
    notification.notify();
}

bool LC_OffsetCurve::transform(const Matrix &matrix, const double scale, const bool reflected) {
    if (!std::all_of(matrix.begin(), matrix.end(), [](double x) { return std::isfinite(x); })) return false;
    if (matrix == Matrix{1, 0, 0, 1, 0, 0}) return true;
    const bool defined = hasEditableDefinition();
    const bool similarity = scale > 0;
    if (defined && similarity && (!isSignedPermutation(matrix, scale) || (reflected && !sourceIsWhole()))) return false;
    // Bound even a stale, externally replaced payload before copying it.
    const auto &current = getData();
    if (!boundedRepresentation(current)) return false;
    auto data = current;
    const bool preserve = defined && similarity;
    for (auto *points : {&data.controlPoints, &data.fitPoints})
        for (auto &point : *points) if (!mapPoint(matrix, point, point, preserve)) return false;
    auto definition = m_definition;
    double tolerance = m_worldTolerance;
    auto grips = m_grips;
    if (preserve) {
        if (!mapPoint(matrix, definition.basis.center, definition.basis.center, true) ||
            !mapPoint(matrix, definition.basis.majorP, definition.basis.majorP, true, true) ||
            !exactValue(Exact(definition.distance)*Exact(scale), definition.distance, true) ||
            !exactValue(Exact(tolerance)*Exact(scale), tolerance, true) ||
            !(definition.distance > 0) || !(tolerance > 0)) return false;
        if (reflected) definition.basis.reversed = !definition.basis.reversed;
        const auto transported = definition.basis;
        if (!geo::normaliseBasis(definition.basis) || !sameBasisGeometry(transported, definition.basis)) return false;
        for (auto &point : grips) if (!mapPoint(matrix, point, point, true)) return false;
    } else grips.clear();
    RS_Spline prepared{nullptr, data};
    publish(prepared, definition, grips, tolerance, preserve, m_closed);
    return true;
}

void LC_OffsetCurve::move(const RS_Vector &offset) {
    if (!geo::isFinite2D(offset)) return;
    transform({1, 0, 0, 1, offset.x, offset.y}, 1, false);
}

void LC_OffsetCurve::rotate(const RS_Vector &center, const double angle) {
    if (!std::isfinite(angle)) return;
    rotate(center, RS_Vector{std::cos(angle), std::sin(angle)});
}

void LC_OffsetCurve::rotate(const RS_Vector &center, const RS_Vector &angle) {
    if (!geo::isFinite2D(angle) || !geo::isFinite2D(center) || (angle.x == 0 && angle.y == 0)) return;
    // A defined curve needs a true unit vector to transport its certificate.
    // Undefined/stale instances retain ordinary spline-shaped transform behavior.
    if (hasEditableDefinition() && Exact(angle.x)*Exact(angle.x) + Exact(angle.y)*Exact(angle.y) != 1) return;
    Matrix matrix{angle.x, -angle.y, angle.y, angle.x, 0, 0};
    if (setTranslation(matrix, center, hasEditableDefinition())) transform(matrix, 1, false);
}

void LC_OffsetCurve::scale(const RS_Vector &center, const RS_Vector &factor) {
    if (!geo::isFinite2D(center) || !geo::isFinite2D(factor)) return;
    const double scale = std::abs(factor.x) == std::abs(factor.y) ? std::abs(factor.x) : 0;
    Matrix matrix{factor.x, 0, 0, factor.y, 0, 0};
    if (setTranslation(matrix, center, scale > 0 && hasEditableDefinition()))
        transform(matrix, scale, (factor.x < 0) != (factor.y < 0));
}

RS_Entity &LC_OffsetCurve::shear(const double k) {
    if (std::isfinite(k)) transform({1, k, 0, 1, 0, 0}, k == 0 ? 1 : 0, false);
    return *this;
}

void LC_OffsetCurve::mirror(const RS_Vector &a, const RS_Vector &b) {
    if (!geo::isFinite2D(a) || !geo::isFinite2D(b)) return;
    const Exact dx = Exact(b.x) - Exact(a.x), dy = Exact(b.y) - Exact(a.y);
    const Exact length = dx*dx + dy*dy;
    if (length == 0) return;
    Matrix matrix{};
    const bool exact = hasEditableDefinition();
    if (!exactValue((dx*dx - dy*dy)/length, matrix[0], exact) ||
        !exactValue(2*dx*dy/length, matrix[1], exact)) return;
    matrix[2] = matrix[1]; matrix[3] = -matrix[0];
    if (setTranslation(matrix, a, exact)) transform(matrix, 1, true);
}

void LC_OffsetCurve::revertDirection() {
    const bool defined = hasEditableDefinition();
    const LC_OffsetCheckLimits limits;
    const auto &current = getData();
    if (!boundedCache(current, limits)) return;
    if (!std::all_of(current.knotslist.begin(), current.knotslist.end(),
        [](double knot) { return std::isfinite(knot); })) return;
    auto data = current;
    const Exact sum = Exact(data.knotslist.front()) + Exact(data.knotslist.back());
    for (auto &knot : data.knotslist) if (!exactValue(sum - Exact(knot), knot, defined)) return;
    std::reverse(data.knotslist.begin(), data.knotslist.end());
    std::reverse(data.controlPoints.begin(), data.controlPoints.end());
    std::reverse(data.weights.begin(), data.weights.end());
    auto definition = m_definition;
    if (defined) definition.cacheReversed = !definition.cacheReversed;
    auto grips = m_grips; std::reverse(grips.begin(), grips.end());
    RS_Spline prepared{nullptr, data};
    publish(prepared, definition, grips, m_worldTolerance, defined, m_closed);
}
