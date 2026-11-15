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

struct MatchedPolylineSnapshot {
    LC_OffsetPolylineData live;
    RS_Vector translation;
    double pointSlack = 0.0;
};

inline std::optional<MatchedPolylineSnapshot> matchLivePolyline(
    const LC_OffsetPolylineData& expected, const RS_Entity& source) {
    const auto* polyline = dynamic_cast<const RS_Polyline*>(&source);
    if (polyline == nullptr || !expected.closed || !polyline->isClosed() ||
        expected.segments.empty() || expected.segments.size() > LC_OffsetSourceDetail::MaximumSourceItems ||
        polyline->size() != expected.segments.size()) return std::nullopt;

    MatchedPolylineSnapshot snapshot;
    snapshot.live.startpoint = polyline->getStartpoint();
    snapshot.live.endpoint = polyline->getEndpoint();
    snapshot.live.closed = polyline->isClosed();
    snapshot.live.segments.reserve(polyline->size());
    for (const RS_Entity* child : *polyline) {
        if (const auto* line = dynamic_cast<const RS_Line*>(child))
            snapshot.live.segments.emplace_back(line->getData());
        else if (const auto* arc = dynamic_cast<const RS_Arc*>(child))
            snapshot.live.segments.emplace_back(arc->getData());
        else
            return std::nullopt;
    }
    if (snapshot.live.segments.size() != expected.segments.size()) return std::nullopt;

    snapshot.translation = expected.startpoint - snapshot.live.startpoint;
    if (!LC_OffsetSourceDetail::finite(snapshot.translation)) return std::nullopt;
    double coordinateScale = 1.0;
    for (const auto& segment : expected.segments) {
        std::visit([&](const auto& item) {
            using T = std::decay_t<decltype(item)>;
            if constexpr (std::is_same_v<T, RS_LineData>) {
                coordinateScale = std::max({coordinateScale, std::abs(item.startpoint.x),
                    std::abs(item.startpoint.y), std::abs(item.endpoint.x), std::abs(item.endpoint.y)});
            } else {
                coordinateScale = std::max({coordinateScale, std::abs(item.center.x),
                    std::abs(item.center.y), item.radius});
            }
        }, segment);
    }
    snapshot.pointSlack = 128.0 * std::numeric_limits<double>::epsilon() * coordinateScale;
    if (!std::isfinite(snapshot.pointSlack) ||
        (snapshot.live.startpoint + snapshot.translation).distanceTo(expected.startpoint) > snapshot.pointSlack ||
        (snapshot.live.endpoint + snapshot.translation).distanceTo(expected.endpoint) > snapshot.pointSlack) {
        return std::nullopt;
    }

    const double angleSlack = 256.0 * std::numeric_limits<double>::epsilon() * M_PI;
    const auto sameAngle = [angleSlack](const double lhs, const double rhs) {
        return std::abs(std::remainder(lhs - rhs, 2.0 * M_PI)) <= angleSlack;
    };
    for (std::size_t i = 0; i < expected.segments.size(); ++i) {
        const auto& wanted = expected.segments[i];
        const auto& actual = snapshot.live.segments[i];
        const bool matches = std::visit([&](const auto& item) {
            using T = std::decay_t<decltype(item)>;
            const auto* found = std::get_if<T>(&actual);
            if (found == nullptr) return false;
            if constexpr (std::is_same_v<T, RS_LineData>) {
                return (found->startpoint + snapshot.translation).distanceTo(item.startpoint) <= snapshot.pointSlack &&
                       (found->endpoint + snapshot.translation).distanceTo(item.endpoint) <= snapshot.pointSlack;
            } else {
                return (found->center + snapshot.translation).distanceTo(item.center) <= snapshot.pointSlack &&
                       std::abs(found->radius - item.radius) <= snapshot.pointSlack &&
                       sameAngle(found->angle1, item.angle1) && sameAngle(found->angle2, item.angle2) &&
                       found->reversed == item.reversed;
            }
        }, wanted);
        if (!matches) return std::nullopt;
    }
    return snapshot;
}

inline std::optional<double> simpleClosedPolylineOrientation(
    const LC_OffsetSourceDefinition& definition, const RS_Entity& source) {
    const auto* data = std::get_if<LC_OffsetPolylineData>(&definition.source.value);
    if (data == nullptr || !data->closed || data->segments.size() < 3) {
        return std::nullopt;
    }
    if (!LC_OffsetSourceDetail::hasNoNonAdjacentSegmentIntersections(*data)) {
        return std::nullopt;
    }

    const auto snapshot = matchLivePolyline(*data, source);
    if (!snapshot) return std::nullopt;
    const double pointSlack = snapshot->pointSlack;
    struct SegmentFrame {
        RS_Vector start;
        RS_Vector end;
        double signedSweep = 0.0;
    };
    std::vector<SegmentFrame> frames;
    frames.reserve(data->segments.size());
    long double twiceArea = 0.0L;
    const RS_Vector origin = data->startpoint;
    for (std::size_t i = 0; i < data->segments.size(); ++i) {
        const auto& definitionSegment = data->segments[i];
        SegmentFrame frame;
        if (const auto* line = std::get_if<RS_LineData>(&definitionSegment)) {
            frame.start = line->startpoint;
            frame.end = line->endpoint;
            const RS_Vector a = frame.start - origin;
            const RS_Vector b = frame.end - origin;
            twiceArea += static_cast<long double>(a.x) * b.y - static_cast<long double>(a.y) * b.x;
        } else {
            const auto& arc = std::get<RS_ArcData>(definitionSegment);
            frame.signedSweep = arc.reversed ? -arc.getAngleLength() : arc.getAngleLength();
            if (!std::isfinite(frame.signedSweep) || frame.signedSweep == 0.0) return std::nullopt;
            const double startAngle = arc.angle1;
            const double endAngle = startAngle + frame.signedSweep;
            frame.start = {arc.center.x + arc.radius * std::cos(startAngle),
                           arc.center.y + arc.radius * std::sin(startAngle)};
            frame.end = {arc.center.x + arc.radius * std::cos(endAngle),
                         arc.center.y + arc.radius * std::sin(endAngle)};
            const long double cx = static_cast<long double>(arc.center.x - origin.x);
            const long double cy = static_cast<long double>(arc.center.y - origin.y);
            const long double radius = arc.radius;
            twiceArea += cx * radius * (std::sin(endAngle) - std::sin(startAngle)) +
                         cy * radius * (std::cos(startAngle) - std::cos(endAngle)) +
                         radius * radius * frame.signedSweep;
        }
        if (frame.start.distanceTo(frame.end) <= 0.0) return std::nullopt;
        frames.push_back(frame);
    }

    if (!std::isfinite(static_cast<double>(twiceArea)) || twiceArea == 0.0L) return std::nullopt;
    const double orientation = twiceArea > 0.0L ? 1.0 : -1.0;
    for (std::size_t i = 0; i < frames.size(); ++i) {
        const auto& current = frames[i];
        const auto& next = frames[(i + 1) % frames.size()];
        const RS_Vector joinDelta = next.start - current.end;
        if (joinDelta.magnitude() > pointSlack) return std::nullopt;
        if (joinDelta.magnitude() > pointSlack) return std::nullopt;
    }
    return orientation;
}

inline bool hasIntersectingPolyline(const LC_OffsetSourceDefinition& definition,
                                    const RS_Entity& source) {
    const auto* data = std::get_if<LC_OffsetPolylineData>(&definition.source.value);
    if (data == nullptr || data->segments.size() < 4 || !matchLivePolyline(*data, source)) return false;
    return !LC_OffsetSourceDetail::hasNoNonAdjacentSegmentIntersections(*data);
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
    const auto orientation = simpleClosedPolylineOrientation(definition, source);
    if (options && ((orientation && definition.signedDistance * *orientation > 0.0) ||
                    hasIntersectingPolyline(definition, source))) {
        options->mode = LC_CurveOffsetMode::RegionBoundary;
    }
    return options;
}

} // namespace LC_OffsetWorkerTolerance

#endif
