/*
 * This file is part of LibreCAD, a 2D CAD program.
 * Copyright (C) 2026 LibreCAD.org
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef LC_OFFSETWORKERTOLERANCE_H
#define LC_OFFSETWORKERTOLERANCE_H

#include "../engine/document/entities/lc_curveoffset.h"
#include "../engine/document/entities/lc_offsetsource.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <optional>
#include <type_traits>
#include <vector>

namespace LC_OffsetWorkerTolerance {

inline std::optional<double> strictConvexLinearPolylineOrientation(
    const LC_OffsetSourceDefinition& definition, const RS_Entity& source) {
    const auto* data = std::get_if<LC_OffsetPolylineData>(&definition.source.value);
    const auto* polyline = dynamic_cast<const RS_Polyline*>(&source);
    if (data == nullptr || !data->closed || data->segments.size() < 3 || polyline == nullptr ||
        !polyline->isClosed()) {
        return std::nullopt;
    }
    if (!std::all_of(data->segments.begin(), data->segments.end(), [](const auto& segment) {
            return std::holds_alternative<RS_LineData>(segment);
        })) {
        return std::nullopt;
    }
    if (static_cast<std::size_t>(polyline->size()) != data->segments.size() ||
        !LC_OffsetSourceDetail::hasNoNonAdjacentSegmentIntersections(*data)) {
        return std::nullopt;
    }

    std::vector<RS_Vector> vertices;
    std::vector<RS_Vector> directions;
    std::vector<double> lengths;
    vertices.reserve(data->segments.size());
    directions.reserve(data->segments.size());
    lengths.reserve(data->segments.size());
    RS_Vector previousEnd;
    RS_Vector definitionTranslation;
    std::size_t index = 0;
    std::size_t segmentIndex = 0;
    for (const RS_Entity* child : *polyline) {
        const auto* line = dynamic_cast<const RS_Line*>(child);
        if (line == nullptr) return std::nullopt;
        const RS_Vector start = line->getStartpoint();
        const RS_Vector end = line->getEndpoint();
        if (index == 0) {
            definitionTranslation = data->startpoint - start;
        }
        const auto& definitionLine = std::get<RS_LineData>(data->segments[segmentIndex]);
        const double scale = std::max({1.0, std::abs(start.x), std::abs(start.y), std::abs(end.x),
                                       std::abs(end.y), std::abs(previousEnd.x), std::abs(previousEnd.y),
                                       std::abs(definitionLine.startpoint.x), std::abs(definitionLine.startpoint.y),
                                       std::abs(definitionLine.endpoint.x), std::abs(definitionLine.endpoint.y)});
        const double slack = 64.0 * std::numeric_limits<double>::epsilon() * scale;
        if ((start + definitionTranslation).distanceTo(definitionLine.startpoint) > slack ||
            (end + definitionTranslation).distanceTo(definitionLine.endpoint) > slack) return std::nullopt;
        if (index > 0 && start.distanceTo(previousEnd) > slack) return std::nullopt;
        vertices.push_back(start);
        directions.push_back(end - start);
        lengths.push_back(directions.back().magnitude());
        if (!(lengths.back() > 0.0) || !std::isfinite(lengths.back())) return std::nullopt;
        previousEnd = end;
        ++index;
        ++segmentIndex;
    }
    const double closureScale = std::max({1.0, std::abs(previousEnd.x), std::abs(previousEnd.y),
                                          std::abs(vertices.front().x), std::abs(vertices.front().y)});
    if (index != vertices.size() || previousEnd.distanceTo(vertices.front()) >
            64.0 * std::numeric_limits<double>::epsilon() * closureScale) {
        return std::nullopt;
    }

    long double twiceArea = 0.0L;
    for (std::size_t i = 0; i < vertices.size(); ++i) {
        const RS_Vector a = vertices[i] - vertices.front();
        const RS_Vector b = vertices[(i + 1) % vertices.size()] - vertices.front();
        twiceArea += static_cast<long double>(a.x) * b.y - static_cast<long double>(a.y) * b.x;
    }
    if (!std::isfinite(static_cast<double>(twiceArea)) || twiceArea == 0.0L) return std::nullopt;
    const double orientation = twiceArea > 0.0L ? 1.0 : -1.0;
    int turnSign = 0;
    for (std::size_t i = 0; i < directions.size(); ++i) {
        const RS_Vector& current = directions[i];
        const RS_Vector& next = directions[(i + 1) % directions.size()];
        const long double turn = static_cast<long double>(current.x) * next.y -
                                 static_cast<long double>(current.y) * next.x;
        const long double threshold = 128.0L * std::numeric_limits<double>::epsilon() *
                                      static_cast<long double>(lengths[i]) * lengths[(i + 1) % lengths.size()];
        if (std::abs(turn) <= threshold) return std::nullopt;
        const int sign = turn > 0.0L ? 1 : -1;
        if (turnSign != 0 && turnSign != sign) return std::nullopt;
        turnSign = sign;
    }
    return turnSign == (orientation > 0.0 ? 1 : -1)
        ? std::optional<double>{orientation}
        : std::nullopt;
}

inline std::optional<LC_CurveOffsetOptions> directOptions(
    const RS_Entity& source, double distance, double relativeFactor) {
    if (!std::isfinite(relativeFactor) || relativeFactor < 0.0 || !std::isfinite(distance)) {
        return std::nullopt;
    }

    distance = std::abs(distance);
    relativeFactor = relativeFactor == 0.0
        ? LC_CurveOffset::relativeOffsetTolerance(source)
        : relativeFactor;
    const double worldTolerance = LC_CurveOffset::toleranceFor(source, distance, relativeFactor);
    if (!std::isfinite(worldTolerance) || !(worldTolerance > 0.0)) return std::nullopt;

    LC_CurveOffsetOptions options =
        LC_CurveOffset::makeDirectOptions(source, distance, worldTolerance);
    if (!std::isfinite(options.tolerance.requestedGeometry) ||
        !(options.tolerance.requestedGeometry > 0.0) ||
        options.tolerance.requestedGeometry != worldTolerance) {
        return std::nullopt;
    }
    return options;
}

inline std::optional<LC_CurveOffsetOptions> directOptions(
    const LC_OffsetSourceDefinition& definition, const RS_Entity& source) {
    auto options = directOptions(source, definition.signedDistance, definition.relativeTolerance);
    const auto orientation = strictConvexLinearPolylineOrientation(definition, source);
    if (options && orientation && definition.signedDistance * *orientation > 0.0) {
        options->mode = LC_CurveOffsetMode::RegionBoundary;
    }
    return options;
}

} // namespace LC_OffsetWorkerTolerance

#endif
