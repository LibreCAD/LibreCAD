/*
 * This file is part of LibreCAD.
 * Copyright (C) 2026 LibreCAD.org
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "lc_offsetsource.h"

#include <algorithm>
#include <cmath>
#include <iterator>
#include <limits>
#include <type_traits>
#include <vector>

#include <boost/geometry.hpp>
#include <boost/geometry/index/rtree.hpp>

#include "rs_information.h"

namespace {
namespace bg = boost::geometry;
namespace bgi = boost::geometry::index;

using IndexPoint = bg::model::point<double, 2, bg::cs::cartesian>;
using IndexBox = bg::model::box<IndexPoint>;
using IndexValue = std::pair<IndexBox, std::size_t>;
using SegmentIndex = bgi::rtree<IndexValue, bgi::quadratic<16>>;

IndexBox segmentBounds(const LC_OffsetPolylineSegment& segment) {
    return std::visit([](const auto& data) {
        using T = std::decay_t<decltype(data)>;
        RS_Vector min;
        RS_Vector max;
        if constexpr (std::is_same_v<T, RS_LineData>) {
            const RS_Line line{nullptr, data};
            min = line.getMin();
            max = line.getMax();
        } else {
            const RS_Arc arc{nullptr, data};
            min = arc.getMin();
            max = arc.getMax();
        }
        return IndexBox{IndexPoint{min.x, min.y}, IndexPoint{max.x, max.y}};
    }, segment);
}

RS_VectorSolutions segmentIntersections(const LC_OffsetPolylineSegment& lhs,
                                        const LC_OffsetPolylineSegment& rhs) {
    return std::visit([](const auto& first, const auto& second) {
        using A = std::decay_t<decltype(first)>;
        using B = std::decay_t<decltype(second)>;
        if constexpr (std::is_same_v<A, RS_LineData> && std::is_same_v<B, RS_LineData>) {
            const RS_Line a{nullptr, first};
            const RS_Line b{nullptr, second};
            return RS_Information::getIntersectionLineLine(&a, &b);
        } else if constexpr (std::is_same_v<A, RS_LineData>) {
            const RS_Line a{nullptr, first};
            const RS_Arc b{nullptr, second};
            return RS_Information::getIntersectionLineArc(&a, &b);
        } else if constexpr (std::is_same_v<B, RS_LineData>) {
            const RS_Arc a{nullptr, first};
            const RS_Line b{nullptr, second};
            return RS_Information::getIntersectionLineArc(&b, &a);
        } else {
            const RS_Arc a{nullptr, first};
            const RS_Arc b{nullptr, second};
            return RS_Information::getIntersectionArcArc(&a, &b);
        }
    }, lhs, rhs);
}

RS_VectorSolutions intersectionsOnSegments(const LC_OffsetPolylineSegment& lhs,
                                           const LC_OffsetPolylineSegment& rhs) {
    auto points = segmentIntersections(lhs, rhs);
    RS_VectorSolutions onSegments;
    const double scale = std::visit([](const auto& first, const auto& second) {
        const auto pointScale = [](const auto& item) {
            using T = std::decay_t<decltype(item)>;
            if constexpr (std::is_same_v<T, RS_LineData>)
                return std::max({1.0, std::abs(item.startpoint.x), std::abs(item.startpoint.y),
                                 std::abs(item.endpoint.x), std::abs(item.endpoint.y)});
            else
                return std::max({1.0, std::abs(item.center.x), std::abs(item.center.y), item.radius});
        };
        return std::max(pointScale(first), pointScale(second));
    }, lhs, rhs);
    const double tolerance = 128.0 * std::numeric_limits<double>::epsilon() * scale;
    std::visit([&](const auto& first, const auto& second) {
        using A = std::decay_t<decltype(first)>;
        using B = std::decay_t<decltype(second)>;
        if constexpr (std::is_same_v<A, RS_LineData> && std::is_same_v<B, RS_LineData>) {
            const RS_Line a{nullptr, first};
            const RS_Line b{nullptr, second};
            for (const RS_Vector& point : points)
                if (a.isPointOnEntity(point, tolerance) && b.isPointOnEntity(point, tolerance))
                    onSegments.push_back(point);
        } else if constexpr (std::is_same_v<A, RS_LineData>) {
            const RS_Line a{nullptr, first};
            const RS_Arc b{nullptr, second};
            for (const RS_Vector& point : points)
                if (a.isPointOnEntity(point, tolerance) && b.isPointOnEntity(point, tolerance))
                    onSegments.push_back(point);
        } else if constexpr (std::is_same_v<B, RS_LineData>) {
            const RS_Arc a{nullptr, first};
            const RS_Line b{nullptr, second};
            for (const RS_Vector& point : points)
                if (a.isPointOnEntity(point, tolerance) && b.isPointOnEntity(point, tolerance))
                    onSegments.push_back(point);
        } else {
            const RS_Arc a{nullptr, first};
            const RS_Arc b{nullptr, second};
            for (const RS_Vector& point : points)
                if (a.isPointOnEntity(point, tolerance) && b.isPointOnEntity(point, tolerance))
                    onSegments.push_back(point);
        }
    }, lhs, rhs);
    return onSegments;
}

bool collinearLinesOverlap(const RS_LineData& lhs, const RS_LineData& rhs) noexcept {
    const double ax = lhs.endpoint.x - lhs.startpoint.x;
    const double ay = lhs.endpoint.y - lhs.startpoint.y;
    const double bx = rhs.endpoint.x - rhs.startpoint.x;
    const double by = rhs.endpoint.y - rhs.startpoint.y;
    const double lengthA = std::hypot(ax, ay);
    const double lengthB = std::hypot(bx, by);
    const double scale = std::max({1.0, lengthA, lengthB,
                                   std::abs(lhs.startpoint.x), std::abs(lhs.startpoint.y),
                                   std::abs(rhs.startpoint.x), std::abs(rhs.startpoint.y)});
    const double tolerance = 128.0 * std::numeric_limits<double>::epsilon() * scale;
    const double crossDirections = ax * by - ay * bx;
    const double crossOffset = (rhs.startpoint.x - lhs.startpoint.x) * ay -
                               (rhs.startpoint.y - lhs.startpoint.y) * ax;
    if (!std::isfinite(tolerance) || !std::isfinite(crossDirections) ||
        !std::isfinite(crossOffset)) return true;
    if (std::abs(crossDirections) > tolerance * std::max(lengthA, lengthB) ||
        std::abs(crossOffset) > tolerance * lengthA) return false;

    if (std::abs(ax) >= std::abs(ay)) {
        const double low = std::max(std::min(lhs.startpoint.x, lhs.endpoint.x),
                                    std::min(rhs.startpoint.x, rhs.endpoint.x));
        const double high = std::min(std::max(lhs.startpoint.x, lhs.endpoint.x),
                                     std::max(rhs.startpoint.x, rhs.endpoint.x));
        return high - low > tolerance;
    }
    const double low = std::max(std::min(lhs.startpoint.y, lhs.endpoint.y),
                                std::min(rhs.startpoint.y, rhs.endpoint.y));
    const double high = std::min(std::max(lhs.startpoint.y, lhs.endpoint.y),
                                 std::max(rhs.startpoint.y, rhs.endpoint.y));
    return high - low > tolerance;
}

bool coincidentArcSpansOverlap(const RS_ArcData& lhs, const RS_ArcData& rhs) noexcept {
    constexpr double TwoPi = 2.0 * M_PI;
    const double scale = std::max({1.0, std::abs(lhs.center.x), std::abs(lhs.center.y),
        std::abs(rhs.center.x), std::abs(rhs.center.y), lhs.radius, rhs.radius});
    const double geometryTolerance = 128.0 * std::numeric_limits<double>::epsilon() * scale;
    const double centerDistance = std::hypot(lhs.center.x - rhs.center.x,
                                             lhs.center.y - rhs.center.y);
    if (!std::isfinite(geometryTolerance) || !std::isfinite(centerDistance)) return true;
    if (centerDistance > geometryTolerance ||
        std::abs(lhs.radius - rhs.radius) > geometryTolerance) return false;

    struct AngularIntervals {
        std::array<std::pair<double, double>, 2> values{};
        std::size_t count = 0;
    };
    const auto intervals = [=](const RS_ArcData& arc) noexcept {
        AngularIntervals result;
        const double start = arc.reversed ? arc.angle2 : arc.angle1;
        const double end = arc.reversed ? arc.angle1 : arc.angle2;
        const double rawSweep = end - start;
        if (!std::isfinite(rawSweep)) {
            result.count = 2;
            result.values = {{{0.0, TwoPi}, {0.0, 0.0}}};
            return result;
        }
        double normalizedStart = std::fmod(start, TwoPi);
        if (normalizedStart < 0.0) normalizedStart += TwoPi;
        double sweep = std::fmod(rawSweep, TwoPi);
        if (sweep < 0.0) sweep += TwoPi;
        const double finish = normalizedStart + sweep;
        if (finish <= TwoPi) {
            result.values[0] = {normalizedStart, finish};
            result.count = 1;
        } else {
            result.values[0] = {normalizedStart, TwoPi};
            result.values[1] = {0.0, finish - TwoPi};
            result.count = 2;
        }
        return result;
    };
    const AngularIntervals first = intervals(lhs);
    const AngularIntervals second = intervals(rhs);
    const double angularTolerance = std::max(
        128.0 * std::numeric_limits<double>::epsilon(),
        geometryTolerance / std::max(std::numeric_limits<double>::min(),
                                     std::min(lhs.radius, rhs.radius)));
    if (!std::isfinite(angularTolerance) || angularTolerance >= TwoPi) return true;
    for (std::size_t i = 0; i < first.count; ++i) {
        for (std::size_t j = 0; j < second.count; ++j) {
            const double overlap = std::min(first.values[i].second, second.values[j].second) -
                                   std::max(first.values[i].first, second.values[j].first);
            if (overlap > angularTolerance) return true;
        }
    }
    return false;
}

bool segmentsIntersectOnlyAtJoin(const LC_OffsetPolylineData& data,
                                 const std::size_t first,
                                 const std::size_t second) {
    const bool sequential = second == first + 1;
    const bool closedSeam = data.closed && first == 0 && second + 1 == data.segments.size();
    if (const auto* lineA = std::get_if<RS_LineData>(&data.segments[first])) {
        if (const auto* lineB = std::get_if<RS_LineData>(&data.segments[second]);
            lineB && collinearLinesOverlap(*lineA, *lineB)) return false;
    }
    if (const auto* arcA = std::get_if<RS_ArcData>(&data.segments[first])) {
        if (const auto* arcB = std::get_if<RS_ArcData>(&data.segments[second]);
            arcB && coincidentArcSpansOverlap(*arcA, *arcB)) return false;
    }
    if ((sequential || closedSeam) &&
        std::holds_alternative<RS_LineData>(data.segments[first]) &&
        std::holds_alternative<RS_LineData>(data.segments[second])) return true;
    const auto intersections = intersectionsOnSegments(data.segments[first], data.segments[second]);
    if (intersections.empty()) return true;
    if (!sequential && !closedSeam) return false;

    RS_Vector firstStart;
    RS_Vector firstEnd;
    RS_Vector otherStart;
    RS_Vector otherEnd;
    if (!LC_OffsetSourceDetail::endpoints(data.segments[first], firstStart, firstEnd) ||
        !LC_OffsetSourceDetail::endpoints(data.segments[second], otherStart, otherEnd)) return false;
    const RS_Vector& shared = sequential ? firstEnd : firstStart;
    const double geometryScale = std::max({1.0, std::abs(shared.x), std::abs(shared.y),
        LC_OffsetSourceDetail::segmentScale(data.segments[first]),
        LC_OffsetSourceDetail::segmentScale(data.segments[second])});
    const double joinTolerance = 8192.0 * std::numeric_limits<double>::epsilon() * geometryScale;
    return std::isfinite(joinTolerance) &&
        std::all_of(intersections.begin(), intersections.end(), [&](const RS_Vector& point) {
            return std::hypot(point.x - shared.x, point.y - shared.y) <= joinTolerance;
        });
}
}

namespace LC_OffsetSourceDetail {
bool hasNoNonAdjacentSegmentIntersections(const LC_OffsetPolylineData& data) noexcept {
    constexpr std::size_t MaximumCandidatePairsPerSegment = 64;
    try {
        SegmentIndex index;
        std::vector<IndexValue> candidates;
        std::size_t candidateCount = 0;
        const std::size_t candidateLimit = MaximumCandidatePairsPerSegment * data.segments.size();
        for (std::size_t current = 0; current < data.segments.size(); ++current) {
            const IndexBox bounds = segmentBounds(data.segments[current]);
            if (!std::isfinite(bg::get<bg::min_corner, 0>(bounds)) ||
                !std::isfinite(bg::get<bg::min_corner, 1>(bounds)) ||
                !std::isfinite(bg::get<bg::max_corner, 0>(bounds)) ||
                !std::isfinite(bg::get<bg::max_corner, 1>(bounds))) return false;
            candidates.clear();
            index.query(bgi::intersects(bounds), std::back_inserter(candidates));
            for (const auto& candidate : candidates) {
                if (++candidateCount > candidateLimit ||
                    !segmentsIntersectOnlyAtJoin(data, candidate.second, current)) return false;
            }
            index.insert({bounds, current});
        }
    } catch (...) {
        return false;
    }
    return true;
}
}
