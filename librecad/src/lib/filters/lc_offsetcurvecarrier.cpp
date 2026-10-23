/*
 * This file is part of LibreCAD, a 2D CAD program.
 * Copyright (C) 2026 LibreCAD.org
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "lc_offsetcurvecarrier.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <utility>

#include "drw_entities.h"

namespace LC_OffsetCurveCarrier {
namespace {
Result refused(const Refusal reason, const std::optional<std::size_t> item = {}) {
    Result result;
    result.refusal = reason;
    result.item = item;
    return result;
}

bool finite(const DRW_Coord &point) {
    return std::isfinite(point.x) && std::isfinite(point.y) && std::isfinite(point.z);
}

bool zero(const DRW_Coord &point) {
    return point.x == 0.0 && point.y == 0.0 && point.z == 0.0;
}

bool declaredCountMatches(const std::int32_t declared, const std::size_t actual) {
    return declared >= 0 && static_cast<std::size_t>(declared) == actual;
}
} // namespace

Result fromRawSpline(const DRW_Spline &raw, const LC_OffsetCheckLimits &limits) {
    if (raw.eType != DRW::SPLINE) return refused(Refusal::NotSpline);
    if (raw.degree != 1 && raw.degree != 3) return refused(Refusal::UnsupportedDegree);
    if (!declaredCountMatches(raw.ncontrol, raw.controllist.size()) ||
        !declaredCountMatches(raw.nknots, raw.knotslist.size()) ||
        !declaredCountMatches(raw.nfit, raw.fitlist.size())) return refused(Refusal::DeclaredCountMismatch);

    constexpr LC_OffsetCheckLimits hardLimits{};
    const auto maximumControls = std::min(limits.maxVertices, hardLimits.maxVertices);
    const auto maximumSpans = std::min(limits.maxCacheSpans, hardLimits.maxCacheSpans);
    const auto degree = static_cast<std::size_t>(raw.degree);
    // C's hard vertex cap bounds this addition, including SIZE_MAX caller limits.
    const auto maximumKnots = maximumControls + degree + 1;
    const auto count = raw.controllist.size();
    if (count > maximumControls || raw.fitlist.size() > maximumControls ||
        raw.weightlist.size() > maximumControls || raw.knotslist.size() > maximumKnots)
        return refused(Refusal::ResourceLimit);
    if (raw.nfit != 0 || !raw.fitlist.empty() || raw.m_scenario == 2) return refused(Refusal::FitData);

    // DXF flags 1/2/4/8/16 and the known control-scenario DWG flags only.
    // DXF 16 also shares its bit with DWG's explicit-weight marker; unit weights
    // remain polynomial in either codec. Unknown/fit-driven modes are not repaired.
    constexpr std::int32_t closed = 4, useKnotParameter = 8, customKnots = 15;
    const bool knownKnotParameter = raw.m_knotParam == 0 || raw.m_knotParam == 1 ||
        raw.m_knotParam == 2 || raw.m_knotParam == customKnots;
    if ((raw.m_scenario != 0 && raw.m_scenario != 1) || raw.flags < 0 || (raw.flags & ~0x1f) != 0 ||
        raw.m_splineFlags1 < 0 || (raw.m_splineFlags1 & ~(closed | useKnotParameter)) != 0 || !knownKnotParameter ||
        ((raw.m_splineFlags1 & closed) != 0 && (raw.flags & 1) == 0))
        return refused(Refusal::UnsupportedMode);
    // Known parameter hints do not override the explicit knots of a control
    // scenario. The fit-method bit and scenario 2 were refused, never refitted.
    if (!finite(raw.tgStart) || !finite(raw.tgEnd) || !std::isfinite(raw.tolknot) ||
        !std::isfinite(raw.tolcontrol) || !std::isfinite(raw.tolfit) ||
        raw.tolknot < 0 || raw.tolcontrol < 0 || raw.tolfit < 0)
        return refused(Refusal::InvalidAuxiliaryData);
    if (!zero(raw.tgStart) || !zero(raw.tgEnd)) return refused(Refusal::FitData);

    const auto counts = LC_EllipseOffsetGeometry::cacheLayoutCountsFromControls(degree, count);
    if (!counts) return refused(Refusal::InvalidLayout);
    if (counts->spans > maximumSpans) return refused(Refusal::ResourceLimit);
    if (raw.knotslist.size() != counts->knots) return refused(Refusal::InvalidKnots);
    if (!raw.weightlist.empty() && raw.weightlist.size() != count) return refused(Refusal::InvalidWeights);
    RS_SplineData data;
    if (count > data.controlPoints.max_size() || count > data.weights.max_size() ||
        counts->knots > data.knotslist.max_size()) return refused(Refusal::ResourceLimit);

    // Validate every original coordinate before accepting an unspecified plane.
    for (std::size_t i = 0; i < count; ++i) {
        const auto &point = raw.controllist[i];
        if (!point) return refused(Refusal::NullControl, i);
        if (!finite(*point)) return refused(Refusal::NonFiniteControl, i);
        if (point->z != 0.0) return refused(Refusal::Non2DControl, i);
    }
    if (!finite(raw.normalVec) || raw.normalVec.x != 0.0 || raw.normalVec.y != 0.0)
        return refused(Refusal::IncompatibleNormal);

    std::size_t knot = 0;
    double previous = 0;
    for (std::size_t group = 0; group <= counts->spans; ++group) {
        const auto value = raw.knotslist[knot];
        if (!std::isfinite(value) || (group != 0 && !(previous < value))) return refused(Refusal::InvalidKnots, knot);
        const auto multiplicity = group == 0 || group == counts->spans ? degree + 1 : degree;
        for (std::size_t i = 0; i < multiplicity; ++i, ++knot)
            if (raw.knotslist[knot] != value) return refused(Refusal::InvalidKnots, knot);
        previous = value;
    }
    for (std::size_t i = 0; i < raw.weightlist.size(); ++i)
        if (!std::isfinite(raw.weightlist[i]) || raw.weightlist[i] != 1.0) return refused(Refusal::InvalidWeights, i);

    const bool closedFlag = (raw.flags & 1) != 0, periodicFlag = (raw.flags & 2) != 0;
    const auto &first = *raw.controllist.front(), &last = *raw.controllist.back();
    if ((periodicFlag && !closedFlag) || ((closedFlag || periodicFlag) &&
        (first.x != last.x || first.y != last.y))) return refused(Refusal::RepairDependentClosure);

    // All scans/counts/storage limits passed before the first reserve or copy.
    data.degree = degree;
    data.type = data.savedOpenType = RS_SplineData::SplineType::ClampedOpen;
    data.m_closedFlag = closedFlag;
    data.m_periodicFlag = periodicFlag;
    data.controlPoints.reserve(count);
    data.knotslist.reserve(counts->knots);
    data.weights.reserve(count);
    for (const auto &point : raw.controllist) data.controlPoints.emplace_back(point->x, point->y, point->z);
    data.knotslist.assign(raw.knotslist.begin(), raw.knotslist.end());
    data.weights.assign(count, 1.0);
    Result result;
    result.data = std::move(data);
    result.refusal = Refusal::None;
    return result;
}

} // namespace LC_OffsetCurveCarrier
