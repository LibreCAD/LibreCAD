/*
 * This file is part of LibreCAD, a 2D CAD program.
 * Copyright (C) 2026 LibreCAD.org
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "lc_offsetcurve.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

namespace {
double coordinateUlp(const double value) {
    if (!std::isfinite(value)) return 0;
    const double up = std::nextafter(value, std::numeric_limits<double>::infinity());
    const double down = std::nextafter(value, -std::numeric_limits<double>::infinity());
    return std::max(std::isfinite(up) ? std::abs(up - value) : 0,
                    std::isfinite(down) ? std::abs(value - down) : 0);
}

template<class Range>
RS_Vector nearestCached(const Range &points, const RS_Vector &coord, double *dist) {
    if (dist) *dist = RS_MAXDOUBLE;
    RS_Vector nearest{false};
    if (!LC_EllipseOffsetGeometry::isFinite2D(coord)) return nearest;
    double distance = std::numeric_limits<double>::infinity();
    for (const auto &point : points) {
        if (!LC_EllipseOffsetGeometry::isFinite2D(point)) continue;
        const double candidate = std::hypot(point.x - coord.x, point.y - coord.y);
        if (candidate < distance) { distance = candidate; nearest = point; }
    }
    if (nearest.valid && dist) *dist = distance;
    return nearest;
}
}

RS_VectorSolutions LC_OffsetCurve::getRefPoints() const {
    return hasEditableDefinition() ? RS_VectorSolutions{m_grips} : RS_Spline::getRefPoints();
}

void LC_OffsetCurve::moveRef(const RS_Vector &ref, const RS_Vector &offset) {
    if (!hasEditableDefinition()) RS_Spline::moveRef(ref, offset);
}

std::optional<std::size_t> LC_OffsetCurve::definitionGripIndex(const RS_Vector &ref) const {
    if (!hasEditableDefinition() || !LC_EllipseOffsetGeometry::isFinite2D(ref)) return std::nullopt;
    const double scale = std::max({std::hypot(m_definition.basis.majorP.x, m_definition.basis.majorP.y),
        m_definition.distance, m_worldTolerance, std::numeric_limits<double>::min()});
    const double localRoundoff = 64*std::numeric_limits<double>::epsilon()*scale;
    std::optional<std::size_t> found;
    for (std::size_t i = 0; i < m_grips.size(); ++i) {
        const auto &point = m_grips[i];
        const double xAllowance = localRoundoff + 4*std::max(coordinateUlp(point.x), coordinateUlp(ref.x));
        const double yAllowance = localRoundoff + 4*std::max(coordinateUlp(point.y), coordinateUlp(ref.y));
        if (std::abs(point.x - ref.x) <= xAllowance && std::abs(point.y - ref.y) <= yAllowance) {
            if (found) return std::nullopt;
            found = i;
        }
    }
    return found;
}

RS_Vector LC_OffsetCurve::doGetNearestRef(const RS_Vector &coord, double *dist) const {
    if (dist) *dist = RS_MAXDOUBLE;
    if (!LC_EllipseOffsetGeometry::isFinite2D(coord)) return RS_Vector{false};
    return hasEditableDefinition() ? nearestCached(m_grips, coord, dist) : RS_Spline::doGetNearestRef(coord, dist);
}

RS_Vector LC_OffsetCurve::doGetNearestSelectedRef(const RS_Vector &coord, double *dist) const {
    if (dist) *dist = RS_MAXDOUBLE;
    if (!LC_EllipseOffsetGeometry::isFinite2D(coord)) return RS_Vector{false};
    if (!hasEditableDefinition()) return RS_Spline::doGetNearestSelectedRef(coord, dist);
    return isSelected() ? nearestCached(m_grips, coord, dist) : RS_Vector{false};
}

RS_Vector LC_OffsetCurve::doGetNearestEndpoint(const RS_Vector &coord, double *dist, RS_Entity **entity) const {
    if (dist) *dist = RS_MAXDOUBLE;
    if (entity) *entity = nullptr;
    if (!LC_EllipseOffsetGeometry::isFinite2D(coord)) return RS_Vector{false};
    if (!hasEditableDefinition()) return RS_Spline::doGetNearestEndpoint(coord, dist, entity);
    RS_Vector point{false};
    if (sourceIsWhole() || m_closed) point = nearestCached(m_grips, coord, dist);
    else point = nearestCached(std::array<RS_Vector, 2>{getData().controlPoints.front(), getData().controlPoints.back()}, coord, dist);
    if (point.valid && entity) *entity = const_cast<LC_OffsetCurve *>(this);
    return point;
}

RS_Vector LC_OffsetCurve::doGetNearestCenter(const RS_Vector &coord, double *dist, RS_Entity **entity) const {
    if (dist) *dist = RS_MAXDOUBLE;
    if (entity) *entity = nullptr;
    if (!LC_EllipseOffsetGeometry::isFinite2D(coord)) return RS_Vector{false};
    if (!hasEditableDefinition()) return RS_Spline::doGetNearestCenter(coord, dist, entity);
    const auto point = nearestCached(std::array<RS_Vector, 1>{m_definition.basis.center}, coord, dist);
    if (point.valid && entity) *entity = const_cast<LC_OffsetCurve *>(this);
    return point;
}

RS_Vector LC_OffsetCurve::getMiddlePoint() const {
    return hasEditableDefinition() ? m_cachedMiddlePoint : RS_Spline::getMiddlePoint();
}

RS_Vector LC_OffsetCurve::doGetNearestMiddle(const RS_Vector &coord, double *dist, const int middlePoints) const {
    if (dist) *dist = RS_MAXDOUBLE;
    if (!LC_EllipseOffsetGeometry::isFinite2D(coord)) return RS_Vector{false};
    if (!hasEditableDefinition()) return RS_Spline::doGetNearestMiddle(coord, dist, middlePoints);
    if (middlePoints < 1 || middlePoints > 64) return RS_Vector{false};
    if (middlePoints == 1 || sourceIsWhole() || m_closed)
        return nearestCached(std::array<RS_Vector, 1>{m_cachedMiddlePoint}, coord, dist);
    return RS_Spline::doGetNearestMiddle(coord, dist, middlePoints);
}

RS_Vector LC_OffsetCurve::doGetNearestDist(const double distance, const RS_Vector &coord, double *dist) const {
    if (dist) *dist = RS_MAXDOUBLE;
    if (!LC_EllipseOffsetGeometry::isFinite2D(coord)) return RS_Vector{false};
    if (hasEditableDefinition() && (sourceIsWhole() || m_closed)) return RS_Vector{false};
    return RS_Spline::doGetNearestDist(distance, coord, dist);
}
