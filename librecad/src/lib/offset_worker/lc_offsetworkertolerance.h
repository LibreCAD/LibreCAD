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

inline std::optional<double> strictConvexPolylineOrientation(
    const LC_OffsetSourceDefinition& definition, const RS_Entity& source) {
    const auto* data = std::get_if<LC_OffsetPolylineData>(&definition.source.value);
    const auto* polyline = dynamic_cast<const RS_Polyline*>(&source);
    if (data == nullptr || !data->closed || data->segments.size() < 3 || polyline == nullptr ||
        !polyline->isClosed()) {
        return std::nullopt;
    }
    if (!LC_OffsetSourceDetail::hasNoNonAdjacentSegmentIntersections(*data)) {
        return std::nullopt;
    }

    const auto liveGeometry = LC_OffsetSourceGeometry::capture(source);
    const auto* liveData = liveGeometry
        ? std::get_if<LC_OffsetPolylineData>(&liveGeometry->value) : nullptr;
    if (liveData == nullptr || liveData->segments.size() != data->segments.size()) {
        return std::nullopt;
    }
    const RS_Vector translation = data->startpoint - liveData->startpoint;
    const double coordinateScale = [&] {
        double scale = 1.0;
        for (const auto& segment : data->segments) {
            std::visit([&](const auto& item) {
                using T = std::decay_t<decltype(item)>;
                if constexpr (std::is_same_v<T, RS_LineData>) {
                    scale = std::max({scale, std::abs(item.startpoint.x), std::abs(item.startpoint.y),
                                      std::abs(item.endpoint.x), std::abs(item.endpoint.y)});
                } else {
                    scale = std::max({scale, std::abs(item.center.x), std::abs(item.center.y), item.radius});
                }
            }, segment);
        }
        return scale;
    }();
    const double pointSlack = 128.0 * std::numeric_limits<double>::epsilon() * coordinateScale;
    const double angleSlack = 128.0 * std::numeric_limits<double>::epsilon();
    const auto sameAngle = [](const double a, const double b, const double tolerance) {
        return std::abs(std::remainder(a - b, 2.0 * std::acos(-1.0))) <= tolerance;
    };

    struct SegmentFrame {
        RS_Vector start;
        RS_Vector end;
        RS_Vector startTangent;
        RS_Vector endTangent;
        bool curved = false;
        double signedSweep = 0.0;
    };
    std::vector<SegmentFrame> frames;
    frames.reserve(data->segments.size());
    long double twiceArea = 0.0L;
    const RS_Vector origin = data->startpoint;
    bool hasStrictTurn = false;
    for (std::size_t i = 0; i < data->segments.size(); ++i) {
        const auto& definitionSegment = data->segments[i];
        const auto& actualSegment = liveData->segments[i];
        SegmentFrame frame;
        if (const auto* line = std::get_if<RS_LineData>(&definitionSegment)) {
            const auto* actualLine = std::get_if<RS_LineData>(&actualSegment);
            if (actualLine == nullptr ||
                (actualLine->startpoint + translation).distanceTo(line->startpoint) > pointSlack ||
                (actualLine->endpoint + translation).distanceTo(line->endpoint) > pointSlack) {
                return std::nullopt;
            }
            frame.start = line->startpoint;
            frame.end = line->endpoint;
            frame.startTangent = frame.endTangent = frame.end - frame.start;
            const RS_Vector a = frame.start - origin;
            const RS_Vector b = frame.end - origin;
            twiceArea += static_cast<long double>(a.x) * b.y - static_cast<long double>(a.y) * b.x;
        } else {
            const auto& arc = std::get<RS_ArcData>(definitionSegment);
            const auto* actualArc = std::get_if<RS_ArcData>(&actualSegment);
            // Endpoint reconstruction at large origins loses angle precision proportional to 1/radius.
            const double arcAngleSlack = actualArc == nullptr ? angleSlack : std::max(
                angleSlack, pointSlack / std::min(arc.radius, actualArc->radius));
            if (actualArc == nullptr || actualArc->reversed != arc.reversed ||
                (actualArc->center + translation).distanceTo(arc.center) > pointSlack ||
                std::abs(actualArc->radius - arc.radius) > pointSlack ||
                !std::isfinite(arcAngleSlack) || arcAngleSlack > 1e-3 ||
                !sameAngle(actualArc->angle1, arc.angle1, arcAngleSlack) ||
                !sameAngle(actualArc->angle2, arc.angle2, arcAngleSlack)) {
                return std::nullopt;
            }
            frame.curved = true;
            frame.signedSweep = arc.reversed ? -arc.getAngleLength() : arc.getAngleLength();
            if (!std::isfinite(frame.signedSweep) || frame.signedSweep == 0.0) return std::nullopt;
            const double startAngle = arc.angle1;
            const double endAngle = startAngle + frame.signedSweep;
            frame.start = {arc.center.x + arc.radius * std::cos(startAngle),
                           arc.center.y + arc.radius * std::sin(startAngle)};
            frame.end = {arc.center.x + arc.radius * std::cos(endAngle),
                         arc.center.y + arc.radius * std::sin(endAngle)};
            const double direction = frame.signedSweep > 0.0 ? 1.0 : -1.0;
            frame.startTangent = {-direction * std::sin(startAngle), direction * std::cos(startAngle)};
            frame.endTangent = {-direction * std::sin(endAngle), direction * std::cos(endAngle)};
            const long double cx = static_cast<long double>(arc.center.x - origin.x);
            const long double cy = static_cast<long double>(arc.center.y - origin.y);
            const long double radius = arc.radius;
            twiceArea += cx * radius * (std::sin(endAngle) - std::sin(startAngle)) +
                         cy * radius * (std::cos(startAngle) - std::cos(endAngle)) +
                         radius * radius * frame.signedSweep;
        }
        if (frame.start.distanceTo(frame.end) <= 0.0 ||
            !frame.startTangent.valid || !frame.endTangent.valid) return std::nullopt;
        frames.push_back(frame);
    }

    if (!std::isfinite(static_cast<double>(twiceArea)) || twiceArea == 0.0L) return std::nullopt;
    const double orientation = twiceArea > 0.0L ? 1.0 : -1.0;
    for (std::size_t i = 0; i < frames.size(); ++i) {
        const auto& current = frames[i];
        const auto& next = frames[(i + 1) % frames.size()];
        const RS_Vector joinDelta = next.start - current.end;
        if (joinDelta.magnitude() > pointSlack) return std::nullopt;
        const long double turn = static_cast<long double>(current.endTangent.x) * next.startTangent.y -
                                 static_cast<long double>(current.endTangent.y) * next.startTangent.x;
        const double firstTangentLength = current.endTangent.magnitude();
        const double nextTangentLength = next.startTangent.magnitude();
        // Convert coordinate uncertainty to an angular tangent bound at the shorter adjacent segment.
        const double tangentAngleSlack = std::max(128.0 * std::numeric_limits<double>::epsilon(),
            pointSlack / std::min(firstTangentLength, nextTangentLength));
        if (!std::isfinite(tangentAngleSlack) || tangentAngleSlack > 1e-3) return std::nullopt;
        const long double threshold = static_cast<long double>(tangentAngleSlack) *
            static_cast<long double>(firstTangentLength) * nextTangentLength;
        if (orientation * turn < -threshold) return std::nullopt;
        if (orientation * turn > threshold) hasStrictTurn = true;
        if (current.curved) {
            if (orientation * current.signedSweep <= angleSlack) return std::nullopt;
            hasStrictTurn = true;
        }
    }
    return hasStrictTurn ? std::optional<double>{orientation} : std::nullopt;
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
    const auto orientation = strictConvexPolylineOrientation(definition, source);
    if (options && orientation && definition.signedDistance * *orientation > 0.0) {
        options->mode = LC_CurveOffsetMode::RegionBoundary;
    }
    return options;
}

} // namespace LC_OffsetWorkerTolerance

#endif
