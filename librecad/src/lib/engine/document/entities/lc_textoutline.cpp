/****************************************************************************
** This file is part of LibreCAD, a 2D CAD program.
** Copyright (C) 2026 LibreCAD.org
** Copyright (C) 2026 Dongxu Li
** SPDX-License-Identifier: GPL-2.0-or-later
****************************************************************************/
#include "lc_textoutline.h"
#include "rs.h"
#include "rs_entitycontainer.h"
#include "rs_pen.h"
#include "rs_polyline.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

namespace {
using Converter = LC_TextOutline;
using Command = LC_OpenTypeFont::Command;
constexpr std::size_t hardMaximumSegments = 1000000;
constexpr std::size_t maximumCommands = 100000;
constexpr unsigned maximumDepth = 32;
// Native containers initialize their bounds with +/-RS_MAXDOUBLE. Allowing
// coordinates beyond those sentinels would silently clamp otherwise valid ink.
constexpr double maximumCoordinate = RS_MAXDOUBLE;

bool finite(const QPointF &point) {
    return std::isfinite(point.x()) && std::isfinite(point.y());
}
bool samePoint(const QPointF &a, const QPointF &b) {
    // QPointF equality is fuzzy. At large document origins it can discard
    // short but representable edges by more than the requested tolerance.
    return a.x() == b.x() && a.y() == b.y();
}
QPointF midpoint(const QPointF &a, const QPointF &b) {
    // Unlike (a+b)/2, this remains finite near the coordinate limit.
    return {a.x() * 0.5 + b.x() * 0.5, a.y() * 0.5 + b.y() * 0.5};
}
double segmentDistance(const QPointF &point, const QPointF &start, const QPointF &end) {
    const long double dx = static_cast<long double>(end.x()) - start.x();
    const long double dy = static_cast<long double>(end.y()) - start.y();
    const long double px = static_cast<long double>(point.x()) - start.x();
    const long double py = static_cast<long double>(point.y()) - start.y();
    // Squaring a nonzero chord can underflow on platforms where long double
    // has double's range. Normalize by hypot instead, keeping the projection
    // in physical units rather than dividing by the squared length.
    const long double length = std::hypot(dx, dy);
    if (length == 0) return static_cast<double>(std::hypot(px, py));
    const long double ux = dx / length, uy = dy / length;
    const long double projection = std::clamp(px * ux + py * uy, 0.0L, length);
    return static_cast<double>(std::hypot(px - projection * ux, py - projection * uy));
}

struct Builder {
    const Converter::Affine &affine;
    const Converter::Options &options;
    std::vector<std::vector<QPointF>> contours;
    std::vector<QPointF> points;
    std::size_t segments = 0;
    bool active = false;
    Converter::Error error;

    Builder(const Converter::Affine &transform, const Converter::Options &settings)
        : affine(transform), options(settings) {}

    bool fail(Converter::ErrorCode code, const QString &message) {
        error = {code, message};
        return false;
    }
    bool map(const QPointF &source, QPointF &destination) {
        if (!finite(source))
            return fail(Converter::ErrorCode::InvalidOutline, QStringLiteral("The font contour has a non-finite coordinate"));
        const double x1 = affine.xx * source.x(), x2 = affine.xy * source.y();
        const double y1 = affine.yx * source.x(), y2 = affine.yy * source.y();
        destination = {std::fma(affine.xx, source.x(), std::fma(affine.xy, source.y(), affine.dx)),
                       std::fma(affine.yx, source.x(), std::fma(affine.yy, source.y(), affine.dy))};
        if (!std::isfinite(x1) || !std::isfinite(x2) || !std::isfinite(y1) || !std::isfinite(y2) ||
            !finite(destination) || std::abs(destination.x()) > maximumCoordinate ||
            std::abs(destination.y()) > maximumCoordinate)
            return fail(Converter::ErrorCode::NumericRange,
                        QStringLiteral("The transformed contour exceeds LibreCAD's native coordinate range (+/-%1 document units)")
                            .arg(maximumCoordinate, 0, 'g', 15));
        const long double magnitude = std::max(
            std::abs(static_cast<long double>(x1)) + std::abs(static_cast<long double>(x2)) + std::abs(static_cast<long double>(affine.dx)),
            std::abs(static_cast<long double>(y1)) + std::abs(static_cast<long double>(y2)) + std::abs(static_cast<long double>(affine.dy)));
        // Reserve a conservative floating-point allowance for the affine,
        // distance calculation and <=32 levels of degree-three subdivision.
        // Include absolute subnormal rounding: epsilon*magnitude can itself
        // underflow to zero even when an affine loses a representable unit.
        // Half the requested tolerance is used for geometric flatness below.
        const long double numericAllowance = 256 * std::numeric_limits<double>::epsilon() * magnitude +
            256 * static_cast<long double>(std::numeric_limits<double>::denorm_min());
        if (numericAllowance > static_cast<long double>(options.tolerance) * 0.25L)
            return fail(Converter::ErrorCode::PrecisionLoss, QStringLiteral("The requested document tolerance is below reliable coordinate precision"));
        return true;
    }
    bool append(const QPointF &point) {
        if (!points.empty() && samePoint(points.back(), point)) return true;
        if (!points.empty()) {
            if (segments >= options.maximumSegments)
                return fail(Converter::ErrorCode::ComplexityLimit, QStringLiteral("The native contour segment budget was exceeded"));
            ++segments;
        }
        points.push_back(point);
        return true;
    }
    bool quadratic(const QPointF &a, const QPointF &b, const QPointF &c, unsigned depth) {
        // A nonconstant closed curve must retain an actual contour even when
        // it is smaller than the tolerance; its zero-length chord has no ink.
        if ((!samePoint(a, c) || samePoint(a, b)) && segmentDistance(b, a, c) <= options.tolerance * 0.5)
            return append(c);
        if (depth >= maximumDepth)
            return fail(Converter::ErrorCode::ComplexityLimit, QStringLiteral("The contour subdivision depth limit was exceeded"));
        const auto ab = midpoint(a, b), bc = midpoint(b, c), middle = midpoint(ab, bc);
        return quadratic(a, ab, middle, depth + 1) && quadratic(middle, bc, c, depth + 1);
    }
    bool cubic(const QPointF &a, const QPointF &b, const QPointF &c, const QPointF &d, unsigned depth,
               unsigned minimumDepth = 0) {
        // Retain quarter points of a closed cubic loop: one split alone could
        // turn a genuine hole into an out-and-back line with zero winding.
        if (samePoint(a, d) && (!samePoint(a, b) || !samePoint(a, c)))
            minimumDepth = std::max(minimumDepth, depth + 2);
        if (depth >= minimumDepth &&
            std::max(segmentDistance(b, a, d), segmentDistance(c, a, d)) <= options.tolerance * 0.5)
            return append(d);
        if (depth >= maximumDepth)
            return fail(Converter::ErrorCode::ComplexityLimit, QStringLiteral("The contour subdivision depth limit was exceeded"));
        const auto ab = midpoint(a, b), bc = midpoint(b, c), cd = midpoint(c, d);
        const auto abc = midpoint(ab, bc), bcd = midpoint(bc, cd), middle = midpoint(abc, bcd);
        return cubic(a, ab, abc, middle, depth + 1, minimumDepth) &&
               cubic(middle, bcd, cd, d, depth + 1, minimumDepth);
    }
    bool close() {
        if (!active || points.empty())
            return fail(Converter::ErrorCode::InvalidOutline, QStringLiteral("A contour close has no active contour"));
        if (!append(points.front())) return false;
        // Completely collapsed contours have no stroked extent or segments.
        if (points.size() > 1) contours.push_back(std::move(points));
        points.clear();
        active = false;
        return true;
    }
    bool consume(const Command &command) {
        QPointF a, b, c;
        switch (command.type) {
        case Command::Type::MoveTo:
            if (active)
                return fail(Converter::ErrorCode::InvalidOutline, QStringLiteral("A new contour starts before the previous contour closes"));
            if (!map(command.first, a)) return false;
            active = true;
            return append(a);
        case Command::Type::LineTo:
            if (!active)
                return fail(Converter::ErrorCode::InvalidOutline, QStringLiteral("A contour line has no starting point"));
            return map(command.first, a) && append(a);
        case Command::Type::QuadraticTo:
            if (!active)
                return fail(Converter::ErrorCode::InvalidOutline, QStringLiteral("A quadratic contour has no starting point"));
            return map(command.first, a) && map(command.second, b) && quadratic(points.back(), a, b, 0);
        case Command::Type::CubicTo:
            if (!active)
                return fail(Converter::ErrorCode::InvalidOutline, QStringLiteral("A cubic contour has no starting point"));
            return map(command.first, a) && map(command.second, b) && map(command.third, c) && cubic(points.back(), a, b, c, 0);
        case Command::Type::Close:
            return close();
        }
        return fail(Converter::ErrorCode::InvalidOutline, QStringLiteral("An unknown contour command was encountered"));
    }
};
} // namespace

LC_TextOutline::Result::Result() = default;
LC_TextOutline::Result::~Result() = default;
LC_TextOutline::Result::Result(Result &&) noexcept = default;
LC_TextOutline::Result &LC_TextOutline::Result::operator=(Result &&) noexcept = default;

LC_TextOutline::Result LC_TextOutline::build(const LC_OpenTypeFont::Outline &outline,
                                            const Affine &transform, const Options &options) {
    Result result;
    if (!std::isfinite(options.tolerance) || options.tolerance <= 0 || !options.maximumSegments ||
        options.maximumSegments > hardMaximumSegments) {
        result.error = {ErrorCode::InvalidOptions, QStringLiteral("Invalid native contour tolerance or segment budget")};
        return result;
    }
    for (double value : {transform.xx, transform.xy, transform.yx, transform.yy, transform.dx, transform.dy}) {
        if (!std::isfinite(value)) {
            result.error = {ErrorCode::InvalidTransform, QStringLiteral("The complete entity transform must be finite")};
            return result;
        }
    }
    if (outline.commands.size() > maximumCommands) {
        result.error = {ErrorCode::ComplexityLimit, QStringLiteral("The font contour command budget was exceeded")};
        return result;
    }
    Builder builder{transform, options};
    for (const auto &command : outline.commands) {
        if (!builder.consume(command)) {
            result.error = std::move(builder.error);
            return result;
        }
    }
    if (builder.active) {
        result.error = {ErrorCode::InvalidOutline, QStringLiteral("The final font contour is not closed")};
        return result;
    }
    result.geometry = std::make_unique<RS_EntityContainer>(nullptr, true);
    result.geometry->setPen(RS_Pen(RS2::FlagInvalid));
    for (const auto &points : builder.contours) {
        auto polyline = std::make_unique<RS_Polyline>(result.geometry.get());
        polyline->setPen(RS_Pen(RS2::FlagInvalid));
        polyline->setLayer(nullptr);
        for (const auto &point : points) polyline->addVertex(RS_Vector(point));
        for (const auto *segment : *polyline) segment->setPen(RS_Pen(RS2::FlagInvalid));
        // The closing edge was emitted explicitly, avoiding endPolyline's
        // legacy 1e-4 closing-edge threshold even for tiny glyph contours.
        polyline->setClosed(true);
        result.geometry->addEntity(polyline.release());
    }
    result.segmentCount = builder.segments;
    result.hasInk = !builder.contours.empty();
    // In particular, an empty container's constructor leaves valid but
    // inverted reset bounds. Finalization converts that into the engine's
    // invalid-vector no-extent sentinel; nonempty contours are recomputed once.
    result.geometry->calculateBorders();
    if (result.hasInk) {
        if (!result.geometry->hasValidBorders()) {
            result.geometry.reset();
            result.hasInk = false;
            result.segmentCount = 0;
            result.error = {ErrorCode::NumericRange, QStringLiteral("The final native text bounds cannot be represented")};
            return result;
        }
        const auto minimum = result.geometry->getMin(), maximum = result.geometry->getMax();
        result.inkBounds = QRectF(minimum.x, minimum.y, maximum.x - minimum.x, maximum.y - minimum.y);
    }
    return result;
}

LC_TextOutline::Result LC_TextOutline::build(const LC_OpenTypeFont::Outline &outline, const Affine &transform) {
    return build(outline, transform, Options{});
}
