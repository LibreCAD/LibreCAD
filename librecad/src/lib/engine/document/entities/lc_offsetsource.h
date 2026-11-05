/*
 * This file is part of LibreCAD.
 * Copyright (C) 2026 LibreCAD.org
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef LC_OFFSETSOURCE_H
#define LC_OFFSETSOURCE_H

#include <algorithm>
#include <cmath>
#include <memory>
#include <cstddef>
#include <limits>
#include <optional>
#include <type_traits>
#include <utility>
#include <variant>

#include "lc_hyperbola.h"
#include "lc_parabola.h"
#include "lc_splinepoints.h"
#include "rs_arc.h"
#include "rs_circle.h"
#include "rs_ellipse.h"
#include "rs_line.h"
#include "rs_polyline.h"
#include "rs_spline.h"

class RS_EntityContainer;

using LC_OffsetPolylineSegment = std::variant<RS_LineData, RS_ArcData>;
struct LC_OffsetPolylineData {
    RS_Vector startpoint;
    RS_Vector endpoint;
    bool closed = false;
    std::vector<LC_OffsetPolylineSegment> segments;
};

/** Geometry-only, value-owned source data for editable offset definitions. */
struct LC_OffsetSourceGeometry {
    using Value = std::variant<RS_EllipseData, RS_SplineData, LC_SplinePointsData,
                               LC_ParabolaData, LC_HyperbolaData, RS_LineData,
                               RS_ArcData, RS_CircleData, LC_OffsetPolylineData>;

    enum class Kind { Ellipse, Spline, SplinePoints, Parabola, BoundedHyperbola,
                      Line, Arc, Circle, Polyline, Invalid };

    Value value;

    Kind kind() const noexcept {
        if (value.valueless_by_exception()) return Kind::Invalid;
        return std::visit([](const auto& data) noexcept {
            using T = std::decay_t<decltype(data)>;
            if constexpr (std::is_same_v<T, RS_EllipseData>) return Kind::Ellipse;
            if constexpr (std::is_same_v<T, RS_SplineData>) return Kind::Spline;
            if constexpr (std::is_same_v<T, LC_SplinePointsData>) return Kind::SplinePoints;
            if constexpr (std::is_same_v<T, LC_ParabolaData>) return Kind::Parabola;
            if constexpr (std::is_same_v<T, LC_HyperbolaData>) return Kind::BoundedHyperbola;
            if constexpr (std::is_same_v<T, RS_LineData>) return Kind::Line;
            if constexpr (std::is_same_v<T, RS_ArcData>) return Kind::Arc;
            if constexpr (std::is_same_v<T, RS_CircleData>) return Kind::Circle;
            return Kind::Polyline;
        }, value);
    }

    static std::optional<LC_OffsetSourceGeometry> capture(const RS_Entity& entity);
    std::unique_ptr<RS_Entity> makeEntity(RS_EntityContainer* parent = nullptr) const;
    bool isWellFormed() const noexcept;
};

/** Source plus the common signed left-normal distance and native cache options. */
struct LC_OffsetSourceDefinition {
    LC_OffsetSourceGeometry source;
    double signedDistance = 0.0;
    double relativeTolerance = 0.0;
    bool cacheReversed = false;

    bool isWellFormed() const noexcept {
        return source.isWellFormed() && std::isfinite(signedDistance) &&
               std::isfinite(relativeTolerance) && relativeTolerance >= 0.0;
    }
};

namespace LC_OffsetSourceDetail {
inline constexpr std::size_t MaximumSourceItems = 65536;

inline bool finite(const RS_Vector& point) noexcept {
    return point.valid && std::isfinite(point.x) && std::isfinite(point.y) &&
           std::isfinite(point.z) && point.z == 0.0;
}

inline bool finitePoints(const std::vector<RS_Vector>& points) noexcept {
    return points.size() <= MaximumSourceItems &&
           std::all_of(points.begin(), points.end(), finite);
}

inline bool sameEndpoint(const RS_Vector& lhs, const RS_Vector& rhs) noexcept {
    const double scale = std::max({1.0, std::abs(lhs.x), std::abs(lhs.y),
                                   std::abs(rhs.x), std::abs(rhs.y)});
    return std::hypot(lhs.x - rhs.x, lhs.y - rhs.y) <=
           128.0 * std::numeric_limits<double>::epsilon() * scale;
}

inline bool endpoints(const LC_OffsetPolylineSegment& segment, RS_Vector& start,
                      RS_Vector& end) noexcept {
    if (segment.valueless_by_exception()) return false;
    return std::visit([&](const auto& data) noexcept {
        using T = std::decay_t<decltype(data)>;
        if constexpr (std::is_same_v<T, RS_LineData>) {
            start = data.startpoint;
            end = data.endpoint;
        } else {
            start = {data.center.x + data.radius * std::cos(data.angle1),
                     data.center.y + data.radius * std::sin(data.angle1)};
            end = {data.center.x + data.radius * std::cos(data.angle2),
                   data.center.y + data.radius * std::sin(data.angle2)};
        }
        return finite(start) && finite(end);
    }, segment);
}
}

inline std::optional<LC_OffsetSourceGeometry> LC_OffsetSourceGeometry::capture(const RS_Entity& entity) {
    if (const auto* ellipse = dynamic_cast<const RS_Ellipse*>(&entity)) {
        LC_OffsetSourceGeometry result{ellipse->getData()};
        return result.isWellFormed() ? std::optional{std::move(result)} : std::nullopt;
    }
    if (const auto* spline = dynamic_cast<const RS_Spline*>(&entity)) {
        LC_OffsetSourceGeometry result{spline->getData()};
        return result.isWellFormed() ? std::optional{std::move(result)} : std::nullopt;
    }
    // LC_Parabola derives from LC_SplinePoints, so preserve its exact identity first.
    if (const auto* parabola = dynamic_cast<const LC_Parabola*>(&entity)) {
        LC_OffsetSourceGeometry result{parabola->getData()};
        return result.isWellFormed() ? std::optional{std::move(result)} : std::nullopt;
    }
    if (const auto* points = dynamic_cast<const LC_SplinePoints*>(&entity)) {
        LC_OffsetSourceGeometry result{points->getData()};
        return result.isWellFormed() ? std::optional{std::move(result)} : std::nullopt;
    }
    if (const auto* hyperbola = dynamic_cast<const LC_Hyperbola*>(&entity)) {
        if (hyperbola->isInfinite()) {
            return std::nullopt;
        }
        LC_OffsetSourceGeometry result{hyperbola->getData()};
        return result.isWellFormed() ? std::optional{std::move(result)} : std::nullopt;
    }
    if (const auto* line = dynamic_cast<const RS_Line*>(&entity)) {
        LC_OffsetSourceGeometry result{RS_LineData{line->getStartpoint(), line->getEndpoint()}};
        return result.isWellFormed() ? std::optional{std::move(result)} : std::nullopt;
    }
    if (const auto* arc = dynamic_cast<const RS_Arc*>(&entity)) {
        LC_OffsetSourceGeometry result{arc->getData()};
        return result.isWellFormed() ? std::optional{std::move(result)} : std::nullopt;
    }
    if (const auto* circle = dynamic_cast<const RS_Circle*>(&entity)) {
        LC_OffsetSourceGeometry result{circle->getData()};
        return result.isWellFormed() ? std::optional{std::move(result)} : std::nullopt;
    }
    if (const auto* polyline = dynamic_cast<const RS_Polyline*>(&entity)) {
        LC_OffsetPolylineData data;
        data.startpoint = polyline->getStartpoint();
        data.endpoint = polyline->getEndpoint();
        data.closed = polyline->isClosed();
        data.segments.reserve(polyline->size());
        for (const RS_Entity* child : *polyline) {
            if (const auto* line = dynamic_cast<const RS_Line*>(child)) data.segments.emplace_back(line->getData());
            else if (const auto* arc = dynamic_cast<const RS_Arc*>(child)) data.segments.emplace_back(arc->getData());
            else return std::nullopt;
        }
        LC_OffsetSourceGeometry result{std::move(data)};
        return result.isWellFormed() ? std::optional{std::move(result)} : std::nullopt;
    }
    return std::nullopt;
}

inline std::unique_ptr<RS_Entity> LC_OffsetSourceGeometry::makeEntity(RS_EntityContainer* parent) const {
    if (!isWellFormed()) return nullptr;
    return std::visit([parent](const auto& data) -> std::unique_ptr<RS_Entity> {
        using T = std::decay_t<decltype(data)>;
        if constexpr (std::is_same_v<T, RS_EllipseData>) return std::make_unique<RS_Ellipse>(parent, data);
        else if constexpr (std::is_same_v<T, RS_SplineData>) return std::make_unique<RS_Spline>(parent, data);
        else if constexpr (std::is_same_v<T, LC_SplinePointsData>) return std::make_unique<LC_SplinePoints>(parent, data);
        else if constexpr (std::is_same_v<T, LC_ParabolaData>) return std::make_unique<LC_Parabola>(parent, data);
        else if constexpr (std::is_same_v<T, LC_HyperbolaData>) return std::make_unique<LC_Hyperbola>(parent, data);
        else if constexpr (std::is_same_v<T, RS_LineData>) return std::make_unique<RS_Line>(parent, data);
        else if constexpr (std::is_same_v<T, RS_ArcData>) return std::make_unique<RS_Arc>(parent, data);
        else if constexpr (std::is_same_v<T, RS_CircleData>) return std::make_unique<RS_Circle>(parent, data);
        else {
            auto result = std::make_unique<RS_Polyline>(parent,
                RS_PolylineData{data.startpoint, data.endpoint, data.closed});
            result->setClosed(data.closed);
            for (const auto& segment : data.segments) {
                std::unique_ptr<RS_Entity> child = std::visit([&](const auto& value) -> std::unique_ptr<RS_Entity> {
                    using S = std::decay_t<decltype(value)>;
                    if constexpr (std::is_same_v<S, RS_LineData>) return std::make_unique<RS_Line>(result.get(), value);
                    else return std::make_unique<RS_Arc>(result.get(), value);
                }, segment);
                result->RS_EntityContainer::addEntity(child.get());
                child.release();
            }
            result->calculateBorders();
            return result;
        }
    }, value);
}

inline bool LC_OffsetSourceGeometry::isWellFormed() const noexcept {
    using namespace LC_OffsetSourceDetail;
    if (value.valueless_by_exception()) return false;
    return std::visit([](const auto& data) noexcept {
        using T = std::decay_t<decltype(data)>;
        if constexpr (std::is_same_v<T, RS_EllipseData>) {
            const double majorLength = std::hypot(data.majorP.x, data.majorP.y);
            return finite(data.center) && finite(data.majorP) && std::isfinite(majorLength) && majorLength > 0.0 &&
                   std::isfinite(data.ratio) && data.ratio > 0.0 &&
                   std::isfinite(data.angle1) && std::isfinite(data.angle2);
        } else if constexpr (std::is_same_v<T, RS_SplineData>) {
            const auto degree = data.degree;
            const auto validSplineType = [](RS_SplineData::SplineType type) {
                return type == RS_SplineData::SplineType::Standard ||
                       type == RS_SplineData::SplineType::ClampedOpen ||
                       type == RS_SplineData::SplineType::WrappedClosed;
            };
            const auto validKnots = [](const std::vector<double>& knots) {
                return std::all_of(knots.begin(), knots.end(), [](double value) { return std::isfinite(value); }) &&
                       std::is_sorted(knots.begin(), knots.end());
            };
            return degree >= 1 && degree <= 3 && data.controlPoints.size() >= degree + 1 &&
                   data.controlPoints.size() <= MaximumSourceItems &&
                   data.knotslist.size() == data.controlPoints.size() + degree + 1 &&
                   data.knotslist.size() <= 2 * MaximumSourceItems &&
                   data.weights.size() <= MaximumSourceItems && data.fitPoints.size() <= MaximumSourceItems &&
                   data.savedOpenKnots.size() <= 2 * MaximumSourceItems && finitePoints(data.controlPoints) &&
                   finitePoints(data.fitPoints) &&
                   (data.weights.empty() || data.weights.size() == data.controlPoints.size()) &&
                   validSplineType(data.type) && validSplineType(data.savedOpenType) &&
                   validKnots(data.knotslist) && validKnots(data.savedOpenKnots) &&
                   std::all_of(data.weights.begin(), data.weights.end(), [](double value) { return std::isfinite(value) && value > 0.0; });
        } else if constexpr (std::is_same_v<T, LC_SplinePointsData>) {
            const auto& activePoints = data.useControlPoints ? data.controlPoints : data.splinePoints;
            return activePoints.size() >= 2 && data.splinePoints.size() <= MaximumSourceItems &&
                   data.controlPoints.size() <= MaximumSourceItems &&
                   finitePoints(data.splinePoints) && finitePoints(data.controlPoints);
        } else if constexpr (std::is_same_v<T, LC_ParabolaData>) {
            return data.isValid() &&
                   std::all_of(data.m_controlPoints.begin(), data.m_controlPoints.end(), finite) &&
                   finite(data.m_focus) && finite(data.m_axis) && finite(data.m_vertex);
        } else if constexpr (std::is_same_v<T, LC_HyperbolaData>) {
            const double majorLength = std::hypot(data.majorP.x, data.majorP.y);
            return data.isValid() && finite(data.center) && finite(data.majorP) && std::isfinite(data.ratio) &&
                   data.ratio > 0.0 && std::isfinite(majorLength) && majorLength > 0.0 && std::isfinite(data.angle1) &&
                   std::isfinite(data.angle2) && data.angle1 != data.angle2 &&
                   std::max(std::abs(data.angle1), std::abs(data.angle2)) < 700.0;
        } else if constexpr (std::is_same_v<T, RS_LineData>) {
            return finite(data.startpoint) && finite(data.endpoint) &&
                   (data.startpoint.x != data.endpoint.x || data.startpoint.y != data.endpoint.y);
        } else if constexpr (std::is_same_v<T, RS_ArcData>) {
            return data.isValid() && finite(data.center) && std::isfinite(data.radius) && data.radius > 0.0 &&
                   std::isfinite(data.angle1) && std::isfinite(data.angle2);
        } else if constexpr (std::is_same_v<T, RS_CircleData>) {
            return data.isValid() && finite(data.center) && std::isfinite(data.radius) && data.radius > 0.0;
        } else {
            if (!finite(data.startpoint) || !finite(data.endpoint) || data.segments.empty() ||
                data.segments.size() > MaximumSourceItems) return false;
            const bool validSegments = std::all_of(data.segments.begin(), data.segments.end(), [](const auto& segment) {
                if (segment.valueless_by_exception()) return false;
                return std::visit([](const auto& value) {
                    using S = std::decay_t<decltype(value)>;
                    if constexpr (std::is_same_v<S, RS_LineData>)
                        return finite(value.startpoint) && finite(value.endpoint) &&
                               (value.startpoint.x != value.endpoint.x || value.startpoint.y != value.endpoint.y);
                    else
                        return value.isValid() && finite(value.center) && std::isfinite(value.radius) &&
                               value.radius > 0.0 && std::isfinite(value.angle1) && std::isfinite(value.angle2);
                }, segment);
            });
            if (!validSegments) return false;

            RS_Vector firstStart;
            RS_Vector previousEnd;
            RS_Vector lastStart;
            RS_Vector lastEnd;
            for (std::size_t i = 0; i < data.segments.size(); ++i) {
                RS_Vector start;
                RS_Vector end;
                if (!endpoints(data.segments[i], start, end)) return false;
                if (i == 0) firstStart = start;
                else if (!sameEndpoint(previousEnd, start)) return false;
                previousEnd = end;
                lastStart = start;
                lastEnd = end;
            }
            if (!sameEndpoint(firstStart, data.startpoint)) return false;
            if (data.closed) {
                return sameEndpoint(lastEnd, data.startpoint) &&
                       (sameEndpoint(data.endpoint, lastStart) || sameEndpoint(data.endpoint, lastEnd));
            }
            return sameEndpoint(lastEnd, data.endpoint);
        }
    }, value);
}

#endif
