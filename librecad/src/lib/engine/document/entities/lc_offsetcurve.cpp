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
    LC_OffsetCheckResult *outcome, const LC_OffsetCheckLimits &limits) {
    LC_OffsetCheckResult checked;
    auto normalized = definition;
    std::unique_ptr<LC_OffsetCurve> result;
    const LC_OffsetCheckLimits hard;
    auto effective = limits;
    effective.maxCacheSpans = std::min(limits.maxCacheSpans, hard.maxCacheSpans);
    effective.maxVertices = std::min(limits.maxVertices, hard.maxVertices);
    effective.maxDistancePairs = std::min(limits.maxDistancePairs, hard.maxDistancePairs);
    effective.maxDepth = std::min(limits.maxDepth, hard.maxDepth);
    effective.maxOrderingComparisons = std::min(limits.maxOrderingComparisons, hard.maxOrderingComparisons);
    if (geo::normaliseBasis(normalized.basis)) {
        RS_SplineData lineData;
        const RS_SplineData *data = nullptr;
        if (const auto *spline = dynamic_cast<const RS_Spline *>(&cache)) {
            data = &spline->getData();
        } else if (const auto *line = dynamic_cast<const RS_Line *>(&cache)) {
            lineData.degree = 1;
            lineData.controlPoints = {line->getStartpoint(), line->getEndpoint()};
            lineData.knotslist = {0, 0, 1, 1}; lineData.weights = {1, 1};
            data = &lineData;
        }
        if (data) {
            // C admits raw counts before allocating or copying any cache array.
            checked = geo::checkCache(*data, normalized, tolerance, effective);
            if (checked) result.reset(new LC_OffsetCurve(cache, *data, normalized, tolerance,
                checked.analysis->closed(), ValidatedCache{}));
        }
    }
    if (outcome) *outcome = std::move(checked);
    return result;
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
    std::vector<RS_Vector> &grips, const double tolerance, const bool defined) {
    static_assert(std::is_nothrow_copy_assignable_v<LC_EllipseOffsetDefinition>);
    const auto stamp = geometryFingerprint(prepared.getData());
    auto notification = replacePreparedSpline(prepared);
    m_definition = definition; m_grips.swap(grips); m_worldTolerance = tolerance;
    m_defined = defined; m_stamp = stamp;
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
    publish(prepared, definition, grips, tolerance, preserve);
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
    publish(prepared, definition, grips, m_worldTolerance, defined);
}
