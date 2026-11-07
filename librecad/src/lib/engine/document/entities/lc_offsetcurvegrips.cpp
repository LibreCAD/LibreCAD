/*
 * This file is part of LibreCAD, a 2D CAD program.
 * Copyright (C) 2026 LibreCAD.org
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "lc_offsetcurvegrips.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <optional>
#include <set>
#include <utility>

#include <boost/multiprecision/cpp_int.hpp>

#include "lc_ellipseoffsetgeometry.h"
#include "rs_spline.h"

namespace geo = LC_EllipseOffsetGeometry;
namespace {
using Exact = boost::multiprecision::cpp_rational;
constexpr std::size_t maxSpans = 4096;
constexpr std::size_t maxSegments = 8192;
constexpr std::size_t maxWork = 65536;
constexpr std::size_t maxGrips = 256;
constexpr unsigned maxDepth = 20;

struct Point { Exact x, y; };
using Net = std::array<Point, 4>;
struct Location { std::size_t span; double u; };
struct Order { Exact lo, hi; };
struct Grip { RS_Vector point; Order order; bool optional = false; };
struct Sample { Point point; Location location; long double length; };
struct Work {
    std::size_t used = 0;
    bool take() { if (used == maxWork) return false; ++used; return true; }
};

Point exactPoint(const RS_Vector &p) { return {Exact(p.x), Exact(p.y)}; }
Exact distanceSquared(const Point &a, const Point &b) {
    const Exact x = a.x - b.x, y = a.y - b.y;
    return Exact(x*x + y*y);
}
Point blend(const Point &a, const Point &b, const Exact &u) {
    return {Exact(a.x + u*(b.x - a.x)), Exact(a.y + u*(b.y - a.y))};
}
Net spanNet(const RS_SplineData &data, const std::size_t span) {
    Net net{};
    for (std::size_t i = 0; i <= data.degree; ++i)
        net[i] = exactPoint(data.controlPoints[data.degree*span + i]);
    return net;
}
Point evaluate(Net net, const std::size_t degree, const double u) {
    const Exact t{u};
    for (std::size_t n = degree; n; --n)
        for (std::size_t i = 0; i < n; ++i) net[i] = blend(net[i], net[i + 1], t);
    return net[0];
}
std::pair<Net, Net> split(Net net, const std::size_t degree) {
    Net left{}, right{};
    left[0] = net[0]; right[degree] = net[degree];
    const Exact half = Exact(1)/2;
    for (std::size_t n = degree; n; --n) {
        for (std::size_t i = 0; i < n; ++i) net[i] = blend(net[i], net[i + 1], half);
        left[degree - n + 1] = net[0]; right[n - 1] = net[n - 1];
    }
    return {std::move(left), std::move(right)};
}
RS_Vector rounded(const Point &point, const Exact &squaredTolerance) {
    const RS_Vector out{point.x.convert_to<double>(), point.y.convert_to<double>()};
    if (!geo::isFinite2D(out) || distanceSquared(point, exactPoint(out)) > squaredTolerance)
        return RS_Vector{false};
    return out;
}

std::optional<std::size_t> layout(const RS_SplineData &data) {
    const auto count = geo::cacheLayoutCountsFromControls(data.degree, data.controlPoints.size());
    const LC_OffsetCheckLimits limits;
    if (!count || count->spans > maxSpans || count->spans > limits.maxCacheSpans || count->controls > limits.maxVertices ||
        data.type != RS_SplineData::SplineType::ClampedOpen ||
        data.savedOpenType != RS_SplineData::SplineType::ClampedOpen ||
        !data.fitPoints.empty() || !data.savedOpenKnots.empty() ||
        data.knotslist.size() != count->knots || data.weights.size() != count->controls) return {};
    for (std::size_t i = 0; i < count->controls; ++i)
        if (!geo::isFinite2D(data.controlPoints[i]) || data.weights[i] != 1) return {};
    std::size_t index = 0;
    double previous = 0;
    for (std::size_t group = 0; group <= count->spans; ++group) {
        const double knot = data.knotslist[index];
        if (!std::isfinite(knot) || (group && !(previous < knot))) return {};
        const auto multiplicity = group == 0 || group == count->spans ? data.degree + 1 : data.degree;
        for (std::size_t i = 0; i < multiplicity; ++i)
            if (data.knotslist[index++] != knot) return {};
        previous = knot;
    }
    return count->spans;
}

Exact boxDistanceSquared(const Net &net, const std::size_t degree, const Point &target) {
    Exact xmin = net[0].x, xmax = xmin, ymin = net[0].y, ymax = ymin;
    for (std::size_t i = 1; i <= degree; ++i) {
        xmin = std::min(xmin, net[i].x); xmax = std::max(xmax, net[i].x);
        ymin = std::min(ymin, net[i].y); ymax = std::max(ymax, net[i].y);
    }
    const Exact dx = target.x < xmin ? Exact(xmin - target.x) : target.x > xmax ? Exact(target.x - xmax) : Exact(0);
    const Exact dy = target.y < ymin ? Exact(ymin - target.y) : target.y > ymax ? Exact(target.y - ymax) : Exact(0);
    return Exact(dx*dx + dy*dy);
}

std::optional<Location> witness(const RS_SplineData &data, const std::size_t spans,
                                const RS_Vector &candidate, const Exact &squaredTolerance, Work &work) {
    if (!geo::isFinite2D(candidate)) return {};
    const auto target = exactPoint(candidate);
    struct Cell { Net net; double a, b; unsigned depth; };
    std::vector<Cell> pending;
    pending.reserve(maxDepth + 2);
    for (std::size_t span = 0; span < spans; ++span) {
        if (!work.take()) return {};
        pending.push_back({spanNet(data, span), 0, 1, 0});
        while (!pending.empty()) {
            auto cell = std::move(pending.back()); pending.pop_back();
            if (!work.take()) return {};
            if (boxDistanceSquared(cell.net, data.degree, target) > squaredTolerance) continue;
            if (distanceSquared(cell.net[0], target) <= squaredTolerance) return Location{span, cell.a};
            if (distanceSquared(cell.net[data.degree], target) <= squaredTolerance) return Location{span, cell.b};
            if (!work.take()) return {};
            auto halves = split(std::move(cell.net), data.degree);
            const double mid = .5*cell.a + .5*cell.b;
            if (distanceSquared(halves.first[data.degree], target) <= squaredTolerance) return Location{span, mid};
            if (cell.depth == maxDepth) continue;
            pending.push_back({std::move(halves.second), mid, cell.b, cell.depth + 1});
            pending.push_back({std::move(halves.first), cell.a, mid, cell.depth + 1});
        }
    }
    return {};
}

Order at(const Location &location) {
    const Exact value = Exact(location.span) + Exact(location.u);
    return {value, value};
}

void include(std::optional<Order> &order, const Order &part) {
    if (!order) order = part;
    else {
        order->lo = std::min(order->lo, part.lo);
        order->hi = std::max(order->hi, part.hi);
    }
}

Order endpointOrder(const std::size_t span, const double u, const std::size_t spans, const bool closed) {
    return closed && span + 1 == spans && u == 1 ? Order{0, 0} : at({span, u});
}

std::optional<Order> lineCorrespondence(const RS_SplineData &data, const std::size_t spans,
                                      const Point &target, const bool closed, Work &work) {
    std::optional<Exact> best;
    std::optional<Order> order;
    for (std::size_t span = 0; span < spans; ++span) {
        if (!work.take()) return {};
        const auto net = spanNet(data, span);
        const Exact dx = net[1].x - net[0].x, dy = net[1].y - net[0].y;
        const Exact denominator = dx*dx + dy*dy;
        if (denominator == 0) return {};
        const Exact projection = ((target.x - net[0].x)*dx + (target.y - net[0].y)*dy)/denominator;
        const Exact u = std::max(Exact(0), std::min(Exact(1), projection));
        const auto squared = distanceSquared(blend(net[0], net[1], u), target);
        const Exact position = closed && span + 1 == spans && u == 1 ? Exact(0) : Exact(Exact(span) + u);
        if (!best || squared < *best) { best = squared; order = Order{position, position}; }
        else if (squared == *best) include(order, {position, position});
    }
    return order && order->lo == order->hi ? order : std::nullopt;
}

enum class DistanceSlope { Increasing, Decreasing, Constant, Unresolved };

DistanceSlope cubicDistanceSlope(const Net &net, const Point &target) {
    constexpr std::array<unsigned, 4> choose3{{1, 3, 3, 1}};
    constexpr std::array<unsigned, 3> choose2{{1, 2, 1}};
    std::array<Exact, 6> coefficients{};
    // Positive Bernstein denominators and the derivative's common factor do
    // not affect signs of (Bezier - target) dot Bezier'.
    for (std::size_t i = 0; i < 4; ++i)
        for (std::size_t j = 0; j < 3; ++j)
            coefficients[i + j] += Exact(choose3[i]*choose2[j]) *
                ((net[i].x - target.x)*(net[j + 1].x - net[j].x) +
                 (net[i].y - target.y)*(net[j + 1].y - net[j].y));
    bool nonnegative = true, nonpositive = true;
    for (const auto &value : coefficients) {
        nonnegative = nonnegative && value >= 0;
        nonpositive = nonpositive && value <= 0;
    }
    if (nonnegative && nonpositive) return DistanceSlope::Constant;
    if (nonnegative) return DistanceSlope::Increasing;
    if (nonpositive) return DistanceSlope::Decreasing;
    return DistanceSlope::Unresolved;
}

bool cubicCorrespondencePass(const RS_SplineData &data, const std::size_t spans, const Point &target,
                             const bool closed, Exact &best, std::optional<Order> *order, Work &work) {
    struct Cell { Net net; double a, b; unsigned depth; };
    std::vector<Cell> pending;
    pending.reserve(maxDepth + 2);
    for (std::size_t span = 0; span < spans; ++span) {
        if (!work.take()) return false;
        pending.push_back({spanNet(data, span), 0, 1, 0});
        while (!pending.empty()) {
            auto cell = std::move(pending.back()); pending.pop_back();
            if (!work.take()) return false;
            if (boxDistanceSquared(cell.net, 3, target) > best) continue;
            if (!work.take()) return false;
            const auto slope = cubicDistanceSlope(cell.net, target);
            if (slope != DistanceSlope::Unresolved) {
                const bool end = slope == DistanceSlope::Decreasing;
                const auto squared = distanceSquared(cell.net[end ? 3 : 0], target);
                if (!order) best = std::min(best, squared);
                else if (squared <= best) {
                    if (slope == DistanceSlope::Constant) include(*order, {at({span, cell.a}).lo, at({span, cell.b}).hi});
                    else include(*order, endpointOrder(span, end ? cell.b : cell.a, spans, closed));
                }
                continue;
            }
            if (!work.take()) return false;
            auto halves = split(std::move(cell.net), 3);
            const double mid = .5*cell.a + .5*cell.b;
            if (!order) {
                best = std::min(best, distanceSquared(halves.first[0], target));
                best = std::min(best, distanceSquared(halves.first[3], target));
                best = std::min(best, distanceSquared(halves.second[3], target));
            }
            if (cell.depth == maxDepth) {
                if (order) include(*order, {at({span, cell.a}).lo, at({span, cell.b}).hi});
                continue;
            }
            if (pending.size() > maxDepth) return false;
            pending.push_back({std::move(halves.second), mid, cell.b, cell.depth + 1});
            pending.push_back({std::move(halves.first), cell.a, mid, cell.depth + 1});
        }
    }
    return true;
}

std::optional<Order> correspondence(const RS_SplineData &data, const std::size_t spans,
                                   const RS_Vector &candidate, const Location &visible,
                                   const bool closed, Work &work) {
    const auto target = exactPoint(candidate);
    if (data.degree == 1) return lineCorrespondence(data, spans, target, closed, work);
    if (!work.take()) return {};
    Exact best = distanceSquared(evaluate(spanNet(data, visible.span), 3, visible.u), target);
    if (!cubicCorrespondencePass(data, spans, target, closed, best, nullptr, work)) return {};
    std::optional<Order> order;
    // Freeze the upper bound before collecting: no previously retained domain
    // can become stale when a later actual curve evaluation improves the bound.
    if (!cubicCorrespondencePass(data, spans, target, closed, best, &order, work)) return {};
    const Exact width = Exact(2)/Exact(std::size_t{1} << maxDepth);
    return order && order->hi - order->lo <= width ? order : std::nullopt;
}

bool separated(const Order &a, const Order &b) { return a.hi < b.lo || b.hi < a.lo; }

bool append(std::vector<Grip> &grips, Grip grip, const Exact &duplicateSquared) {
    for (std::size_t i = 0; i < grips.size(); ++i)
        if (distanceSquared(exactPoint(grip.point), exactPoint(grips[i].point)) <= duplicateSquared) {
            if (grip.optional || !grips[i].optional) return true;
            grips.erase(grips.begin() + i);
            break;
        }
    for (const auto &other : grips)
        if (!separated(grip.order, other.order) && (grip.optional || !other.optional)) return false;
    // Known cache parameters, including C corners, outrank uncertain optional
    // analytic hints. Only provably disjoint order enclosures survive together.
    if (!grip.optional)
        grips.erase(std::remove_if(grips.begin(), grips.end(), [&](const Grip &other) {
            return !separated(grip.order, other.order);
        }), grips.end());
    if (grips.size() == maxGrips) return false;
    grips.push_back(std::move(grip));
    return true;
}

std::optional<std::vector<Sample>> samples(const RS_SplineData &data, const std::size_t spans) {
    const auto origin = exactPoint(data.controlPoints.front());
    Exact divisor = 0;
    for (const auto &control : data.controlPoints) {
        const Exact dx = abs(Exact(control.x) - origin.x), dy = abs(Exact(control.y) - origin.y);
        divisor = std::max(divisor, std::max(dx, dy));
    }
    if (divisor == 0) return {};
    const std::size_t steps = data.degree == 1 ? 1 : std::min<std::size_t>(32, maxSegments/spans);
    std::vector<Sample> out;
    out.reserve(spans*steps + 1);
    out.push_back({exactPoint(data.controlPoints.front()), {0, 0}, 0});
    for (std::size_t span = 0; span < spans; ++span) {
        const auto net = spanNet(data, span);
        for (std::size_t i = 1; i <= steps; ++i) {
            const double u = static_cast<double>(i)/static_cast<double>(steps);
            auto point = evaluate(net, data.degree, u);
            const Exact dx = (point.x - out.back().point.x)/divisor;
            const Exact dy = (point.y - out.back().point.y)/divisor;
            const auto length = out.back().length + std::hypot(dx.convert_to<long double>(), dy.convert_to<long double>());
            if (!std::isfinite(length)) return {};
            out.push_back({std::move(point), {span, u}, length});
        }
    }
    return out;
}

Grip middle(const RS_SplineData &data, const std::vector<Sample> &points,
            const Exact &squaredTolerance, const bool closed) {
    const long double half = points.back().length/2;
    const auto found = std::lower_bound(points.begin(), points.end(), half,
        [](const Sample &sample, const long double value) { return sample.length < value; });
    if (found != points.begin() && found != points.end()) {
        const auto &previous = *(found - 1);
        const auto width = found->length - previous.length;
        const auto fraction = width > 0 ? (half - previous.length)/width : 0;
        const double start = previous.location.span == found->location.span ? previous.location.u : 0;
        const double u = std::clamp(start + (found->location.u - start)*static_cast<double>(fraction), start, found->location.u);
        const auto point = rounded(evaluate(spanNet(data, found->location.span), data.degree, u), squaredTolerance);
        if (point.valid) return {point, at({found->location.span, u})};
    }
    Grip best{RS_Vector{false}, {0, 0}};
    long double gap = std::numeric_limits<long double>::infinity();
    for (std::size_t i = 0; i < points.size(); ++i) {
        if (closed && (i == 0 || i + 1 == points.size())) continue;
        const auto point = rounded(points[i].point, squaredTolerance);
        const auto distance = std::abs(points[i].length - half);
        if (point.valid && distance < gap) { best = {point, at(points[i].location)}; gap = distance; }
    }
    return best;
}

Grip nearestSample(const std::vector<Sample> &points, const RS_Vector &target,
                   const Exact &squaredTolerance) {
    Grip best{RS_Vector{false}, {0, 0}};
    std::optional<Exact> distance;
    const auto query = exactPoint(target);
    for (const auto &sample : points) {
        const auto point = rounded(sample.point, squaredTolerance);
        if (!point.valid) continue;
        const auto squared = distanceSquared(sample.point, query);
        if (!distance || squared < *distance) { best = {point, at(sample.location)}; distance = squared; }
    }
    return best;
}

bool corners(const RS_SplineData &data, const std::size_t spans, const bool reversed, const bool closed,
             const geo::CacheTopologyAnalysis &topology, const Exact &squaredTolerance,
             const Exact &duplicateSquared, std::vector<Grip> &out) {
    if (topology.closed() != closed || topology.layout().spanCount() != spans ||
        topology.joins().size() > maxWork || topology.endpoints().size() > maxWork) return false;
    for (std::size_t span = 0; span < spans; ++span) {
        const auto &saved = topology.layout().spans[span];
        if (saved.degree != data.degree) return false;
        for (std::size_t i = 0; i <= data.degree; ++i) {
            const auto &a = saved.controls[i], &b = data.controlPoints[data.degree*span + i];
            if (a.x != b.x || a.y != b.y || a.z != b.z || a.valid != b.valid) return false;
        }
    }
    std::set<std::pair<std::size_t, std::size_t>> incoming;
    for (const auto &join : topology.joins())
        if (join.kind == geo::CachePathJoinKind::PairEvent) incoming.emplace(join.incoming, join.from.value);
    for (const auto &endpoint : topology.endpoints()) {
        if (endpoint.side != geo::ReferenceEdgeSide::End || !incoming.count({endpoint.edge, endpoint.occurrence.value})) continue;
        if (endpoint.span >= spans || !std::isfinite(endpoint.candidateParameter) ||
            endpoint.candidateParameter < 0 || endpoint.candidateParameter > 1 || out.size() == maxGrips) return false;
        const double u = reversed ? 1 - endpoint.candidateParameter : endpoint.candidateParameter;
        const auto point = rounded(evaluate(spanNet(data, endpoint.span), data.degree, u), squaredTolerance);
        if (!point.valid || !append(out, {point, endpointOrder(endpoint.span, u, spans, closed)}, duplicateSquared)) return false;
    }
    return true;
}
} // namespace

LC_OffsetCurveGrips::Prepared LC_OffsetCurveGrips::prepare(const RS_SplineData &cache,
    const LC_EllipseOffsetDefinition &definition, const double tolerance, const bool closed,
    const geo::CacheTopologyAnalysis *topology) {
    Prepared out;
    const auto spans = layout(cache);
    if (!spans || !std::isfinite(tolerance) || !(tolerance > 0) ||
        !std::isfinite(definition.distance) || !(definition.distance > 0) ||
        !geo::isFinite2D(definition.basis.center) || !geo::isFinite2D(definition.basis.majorP) ||
        !std::isfinite(definition.basis.ratio) || !(definition.basis.ratio > 0) || definition.basis.ratio > 1) return out;
    const auto sweep = geo::sweep(definition.basis);
    const double axis = std::hypot(definition.basis.majorP.x, definition.basis.majorP.y);
    const double scale = std::max(axis, definition.distance);
    if (!sweep || !(axis > 0) || !std::isfinite(axis) || !std::isfinite(scale)) return out;
    const Exact squaredTolerance = Exact(tolerance)*Exact(tolerance);
    const auto sampled = samples(cache, *spans);
    if (!sampled) return out;
    const auto midpoint = middle(cache, *sampled, squaredTolerance, closed);
    const double ulp = scale - std::nextafter(scale, 0.0);
    const Exact localAllowance = Exact(ulp)*4, toleranceAllowance = Exact(tolerance)/16;
    const Exact duplicateDistance = std::min(localAllowance, toleranceAllowance);
    const Exact duplicateSquared = duplicateDistance*duplicateDistance;
    std::vector<Grip> grips;
    grips.reserve(maxGrips);
    if (topology && !corners(cache, *spans, definition.cacheReversed, closed, *topology, squaredTolerance, duplicateSquared, grips)) return out;
    Work work;
    const Exact admittedBandSquared = 4*squaredTolerance;
    const auto analyticGrip = [&](const RS_Vector &candidate, const bool projectQuarter = false) {
        auto visible = witness(cache, *spans, candidate, squaredTolerance, work);
        const bool needsProjection = !visible && projectQuarter;
        if (needsProjection) visible = witness(cache, *spans, candidate, admittedBandSquared, work);
        if (!visible) return false;
        auto order = correspondence(cache, *spans, candidate, *visible, closed, work);
        if (!order) return false;
        auto point = candidate;
        if (needsProjection) {
            if (!work.take()) return false;
            const Exact parameter = (order->lo + order->hi)/2;
            if (parameter < 0 || parameter > Exact(*spans)) return false;
            const auto span = std::min(parameter.convert_to<std::size_t>(), *spans - 1);
            const double u = Exact(parameter - Exact(span)).convert_to<double>();
            if (!std::isfinite(u) || u < 0 || u > 1) return false;
            const auto actual = evaluate(spanNet(cache, span), cache.degree, u);
            if (distanceSquared(actual, exactPoint(candidate)) > admittedBandSquared) return false;
            point = rounded(actual, squaredTolerance);
            if (!geo::isFinite2D(point)) return false;
            // This is a rounded, checked actual cache point with a known
            // parameter, not the unproved ideal quarter seed.
            order = endpointOrder(span, u, *spans, closed);
        }
        return append(grips, {point, *order, true}, duplicateSquared);
    };
    const double signedDistance = definition.towardCentre ? -definition.distance : definition.distance;
    const Point center = exactPoint(definition.basis.center);
    const Point major = exactPoint(definition.basis.majorP);
    const Exact ratio{definition.basis.ratio}, distance{signedDistance}, length{axis};
    const Point minor{Exact(-major.y*ratio), Exact(major.x*ratio)};
    const Point unitMajor{Exact(major.x/length), Exact(major.y/length)};
    const Point unitMinor{Exact(-unitMajor.y), unitMajor.x};
    constexpr double pi = 3.141592653589793238462643383279502884;
    for (unsigned quarter = 0; quarter < 4; ++quarter) {
        if (!sweep->contains(quarter*(pi/2))) continue;
        const Point &direction = quarter % 2 ? minor : major;
        const Point &normal = quarter % 2 ? unitMinor : unitMajor;
        const Exact sign = quarter < 2 ? 1 : -1;
        const Point analytic{Exact(center.x + sign*(direction.x + distance*normal.x)),
                             Exact(center.y + sign*(direction.y + distance*normal.y))};
        const RS_Vector candidate{analytic.x.convert_to<double>(), analytic.y.convert_to<double>()};
        analyticGrip(candidate, true);
    }
    if (!sweep->whole) {
        const auto frame = geo::ReferenceFrame::create(definition.basis, signedDistance);
        LC_CurveJet jet;
        if (frame && frame->offsetJet(.5*frame->parameterEnd(), jet)) {
            const Point analytic{Exact(center.x + Exact(jet.point.x)*Exact(frame->scale())),
                                 Exact(center.y + Exact(jet.point.y)*Exact(frame->scale()))};
            const RS_Vector candidate{analytic.x.convert_to<double>(), analytic.y.convert_to<double>()};
            if (geo::isFinite2D(candidate)) {
                if (!analyticGrip(candidate)) {
                    const auto fallback = nearestSample(*sampled, candidate, squaredTolerance);
                    if (fallback.point.valid) append(grips, fallback, duplicateSquared);
                }
            }
        }
    }
    if (grips.empty() && midpoint.point.valid) append(grips, midpoint, duplicateSquared);
    std::stable_sort(grips.begin(), grips.end(), [](const Grip &a, const Grip &b) { return a.order.lo < b.order.lo; });
    out.grips.reserve(grips.size());
    for (const auto &grip : grips) out.grips.push_back(grip.point);
    out.midpoint = midpoint.point;
    return out;
}

RS_Vector LC_OffsetCurveGrips::prepareMiddle(const RS_SplineData &cache, const double tolerance, const bool closed) {
    const auto spans = layout(cache);
    if (!spans || !std::isfinite(tolerance) || !(tolerance > 0)) return RS_Vector{false};
    const auto sampled = samples(cache, *spans);
    if (!sampled) return RS_Vector{false};
    const Exact squaredTolerance = Exact(tolerance)*Exact(tolerance);
    return middle(cache, *sampled, squaredTolerance, closed).point;
}
