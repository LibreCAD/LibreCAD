/*******************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 * Copyright (C) 2026 LibreCAD.org
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later version.
 * This program is distributed WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details. You should have received a copy along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 ******************************************************************************/

#include "lc_ellipseoffsetgeometry.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <utility>

#include <boost/math/tools/toms748_solve.hpp>
#include <boost/multiprecision/cpp_int.hpp>

#include "rs_math.h"
#include "rs_spline.h"

namespace {
constexpr double turn = 2.0 * M_PI;
constexpr double fullTurnAllowance = 1e-12 * turn;

double wrapped(const double angle) {
    if (angle >= 0.0 && angle < turn) {
        return angle;
    }
    // fmod by the rounded double 2*pi disagrees with libm's reduction at large arguments.
    double result = std::atan2(std::sin(angle), std::cos(angle));
    if (result < 0.0) {
        result += turn;
    }
    return result < turn ? result : 0.0;
}

double invalidNumber() {
    return std::numeric_limits<double>::quiet_NaN();
}

struct Frame {
    RS_Vector major;
    RS_Vector minor;
    RS_Vector unitMajor;
    RS_Vector unitMinor;
    double a = 0.0;
    double b = 0.0;
};

std::optional<Frame> frame(const RS_EllipseData& basis, const bool normalized = true) {
    using LC_EllipseOffsetGeometry::isFinite2D;
    if (!isFinite2D(basis.center) || !isFinite2D(basis.majorP) ||
        !std::isfinite(basis.ratio) || !(basis.ratio > 0.0) || (normalized && basis.ratio > 1.0)) {
        return std::nullopt;
    }
    Frame f;
    f.major = basis.majorP;
    f.a = std::hypot(f.major.x, f.major.y);
    f.b = f.a * basis.ratio;
    f.minor = RS_Vector{-f.major.y * basis.ratio, f.major.x * basis.ratio};
    if (!std::isfinite(f.a) || !(f.a > 0.0) || !std::isfinite(f.b) || !(f.b > 0.0) ||
        !isFinite2D(f.minor)) {
        return std::nullopt;
    }
    // RS_Vector's scalar division deliberately does nothing below RS_TOLERANCE.
    f.unitMajor = RS_Vector{f.major.x / f.a, f.major.y / f.a};
    f.unitMinor = RS_Vector{-f.unitMajor.y, f.unitMajor.x};
    return f;
}

RS_Vector checked(const RS_Vector& point) {
    return LC_EllipseOffsetGeometry::isFinite2D(point) ? point : RS_Vector{false};
}

bool finiteInterval(const LC_Interval& value) {
    return value.isValid() && std::isfinite(value.lo()) && std::isfinite(value.hi());
}

bool finiteBounds(const LC_CurveJetBounds& bounds) {
    return finiteInterval(bounds.x) && finiteInterval(bounds.y) && finiteInterval(bounds.dx) &&
           finiteInterval(bounds.dy) && finiteInterval(bounds.ddx) && finiteInterval(bounds.ddy);
}

/** Binary rescaling is exact unless the result is subnormal; enclose that rounding too. */
LC_Interval scaled(const LC_Interval& value, const int exponent) {
    if (!finiteInterval(value)) {
        return {};
    }
    const auto endpoint = [exponent](const double input, const double toward) {
        const double result = std::scalbn(input, exponent);
        // An inexact subnormal result may round up to the smallest normal double.
        const bool exactNormal = std::isnormal(result) && std::abs(result) > std::numeric_limits<double>::min();
        return input == 0.0 || exactNormal ? result : std::nextafter(result, toward);
    };
    const double lo = endpoint(value.lo(), -std::numeric_limits<double>::infinity());
    const double hi = endpoint(value.hi(), std::numeric_limits<double>::infinity());
    if (!std::isfinite(lo) || !std::isfinite(hi)) {
        return {};
    }
    return LC_Interval::hull(lo, hi);
}

LC_Interval unitRange(const LC_Interval& value) {
    if (!finiteInterval(value)) {
        return {};
    }
    const double lo = std::max(-1.0, value.lo());
    const double hi = std::min(1.0, value.hi());
    return lo <= hi ? LC_Interval::hull(lo, hi) : LC_Interval{};
}

/** R^order/order!, all coefficients and products enclosed without integer factorials. */
double taylorRemainder(const double radius, const int order) {
    LC_Interval error = LC_Interval::point(1.0);
    for (int k = 1; k <= order; ++k) {
        error = error * LC_Interval::point(radius) / LC_Interval::point(static_cast<double>(k));
    }
    return error.hi();
}

LC_Interval realPi() {
    // Adjacent binary64 values bracketing real pi, not an assumed libm rounding bound.
    return LC_Interval::hull(0x1.921fb54442d18p+1, 0x1.921fb54442d19p+1);
}

/**
 * Reduce by an enclosed real multiple of pi/2, not by a rounded period.
 * Taylor's theorem uses |sin^(n)|, |cos^(n)| <= 1 for the explicit remainder.
 */
bool referenceTrig(const LC_Interval& angle, LC_Interval& cosine, LC_Interval& sine) {
    cosine = {};
    sine = {};
    if (!finiteInterval(angle) || angle.lo() < -16.0 || angle.hi() > 16.0) {
        return false;
    }
    const LC_Interval halfPi = realPi() * LC_Interval::point(0.5);
    const double centre = 0.5 * angle.lo() + 0.5 * angle.hi();
    const int quadrant = static_cast<int>(std::round(centre / M_PI_2));
    const LC_Interval reduced = angle - LC_Interval::point(static_cast<double>(quadrant)) * halfPi;
    const double radius = std::max(std::abs(reduced.lo()), std::abs(reduced.hi()));
    if (radius > halfPi.lo()) {
        cosine = sine = LC_Interval::hull(-1.0, 1.0);
        return true;
    }
    const LC_Interval square = sqr(reduced);
    LC_Interval cosTerm = LC_Interval::point(1.0);
    LC_Interval sinTerm = reduced;
    LC_Interval c = cosTerm;
    LC_Interval s = sinTerm;
    for (int k = 1; k <= 10; ++k) {
        cosTerm = -(cosTerm * square) / LC_Interval::point(static_cast<double>((2 * k - 1) * (2 * k)));
        sinTerm = -(sinTerm * square) / LC_Interval::point(static_cast<double>((2 * k) * (2 * k + 1)));
        c = c + cosTerm;
        s = s + sinTerm;
    }
    const double cosError = taylorRemainder(radius, 22);
    const double sinError = taylorRemainder(radius, 23);
    c = unitRange(c + LC_Interval::hull(-cosError, cosError));
    s = unitRange(s + LC_Interval::hull(-sinError, sinError));
    if (!finiteInterval(c) || !finiteInterval(s)) {
        return false;
    }
    switch ((quadrant % 4 + 4) % 4) {
    case 0: cosine = c; sine = s; break;
    case 1: cosine = -s; sine = c; break;
    case 2: cosine = -c; sine = -s; break;
    case 3: cosine = s; sine = -c; break;
    }
    return true;
}

bool jetFromBounds(const LC_CurveJetBounds& bounds, LC_CurveJet& out) {
    out = {};
    if (!finiteBounds(bounds)) {
        return false;
    }
    const auto middle = [](const LC_Interval& value) { return 0.5 * value.lo() + 0.5 * value.hi(); };
    LC_CurveJet candidate;
    candidate.point = RS_Vector{middle(bounds.x), middle(bounds.y)};
    candidate.first = RS_Vector{middle(bounds.dx), middle(bounds.dy)};
    candidate.second = RS_Vector{middle(bounds.ddx), middle(bounds.ddy)};
    if (!LC_EllipseOffsetGeometry::isFinite2D(candidate.point) ||
        !LC_EllipseOffsetGeometry::isFinite2D(candidate.first) ||
        !LC_EllipseOffsetGeometry::isFinite2D(candidate.second)) {
        return false;
    }
    out = candidate;
    return true;
}

RS_Vector farEllipseFoot(const RS_Vector& query, const double ratio, const double axisScale) {
    const double absoluteX = std::abs(query.x);
    const double weightedY = std::abs(ratio * query.y);
    if (axisScale > 0.0 && absoluteX >= axisScale && weightedY == 0.0) {
        return RS_Vector{std::copysign(1.0, query.x), 0.0};
    }
    auto tolerance = boost::math::tools::eps_tolerance<double>(48);
    if (absoluteX < axisScale && weightedY < axisScale * 1e-8) {
        const double unitX = absoluteX / axisScale;
        const double unitY = weightedY / axisScale;
        // On an extremely thin ellipse the multiplier can underflow; the angular root is well scaled instead.
        if (unitX <= 64.0 * std::numeric_limits<double>::epsilon()) {
            return RS_Vector{0.0, std::copysign(ratio, query.y)};
        }
        const auto residual = [&](const double angle) {
            const double c = std::cos(angle);
            return std::sin(angle) * ((1.0 - ratio * ratio) * c - unitX) + unitY * c;
        };
        boost::uintmax_t iterations = 100;
        const auto bracket = boost::math::tools::toms748_solve(residual, 0.0, M_PI_2, tolerance, iterations);
        if (!tolerance(bracket.first, bracket.second)) {
            return RS_Vector{false};
        }
        const double angle = bracket.first + (bracket.second - bracket.first) * 0.5;
        return RS_Vector{std::copysign(std::cos(angle), query.x),
                         std::copysign(ratio * std::sin(angle), query.y)};
    }
    const double scale = std::hypot(query.x, ratio * query.y);
    if (!(scale > 0.0) || !std::isfinite(scale)) {
        return RS_Vector{false};
    }
    const double x = query.x / scale;
    const double y = (ratio * query.y) / scale;
    const double majorTerm = axisScale / scale;
    const double minorTerm = ratio * (ratio * majorTerm);
    const auto quotient = [](const double numerator, const double denominator) {
        if (numerator == 0.0) {
            return 0.0;
        }
        const double cap = std::numeric_limits<double>::max() / 4.0;
        return std::clamp(numerator / denominator, -cap, cap);
    };
    // The exterior closest-point multiplier is nonnegative. Scale it so its root stays in [0, 2].
    const auto residual = [&](const double u) {
        return std::hypot(quotient(x, majorTerm + u), quotient(y, minorTerm + u)) - 1.0;
    };
    const double low = residual(0.0);
    const double high = residual(2.0);
    if (!(low > 0.0) || !(high < 0.0)) {
        return RS_Vector{false};
    }
    boost::uintmax_t iterations = 100;
    const auto bracket = boost::math::tools::toms748_solve(residual, 0.0, 2.0, low, high,
        tolerance, iterations);
    if (!tolerance(bracket.first, bracket.second)) {
        return RS_Vector{false};
    }
    const double u = bracket.first + (bracket.second - bracket.first) * 0.5;
    const double px = quotient(x, majorTerm + u);
    const double py = quotient(y, minorTerm + u);
    if (std::abs(std::hypot(px, py) - 1.0) > 256.0 * std::numeric_limits<double>::epsilon()) {
        return RS_Vector{false};
    }
    const double parameter = std::atan2(py, px);
    return RS_Vector{std::cos(parameter), ratio * std::sin(parameter)};
}
} // namespace

namespace LC_EllipseOffsetGeometry {

WorkBudget::WorkBudget(const LC_OffsetCheckLimits& limits) : m_limits{limits} {
    if (limits.maxCacheSpans == 0 || limits.maxVertices == 0 || limits.maxDistancePairs == 0 || limits.maxDepth == 0) {
        m_status = BudgetStatus::InvalidInput;
    }
}

BudgetStatus WorkBudget::status() const {
    return m_status;
}

const CheckWork& WorkBudget::consumed() const {
    return m_work;
}

std::size_t WorkBudget::remainingPieces() const {
    return m_status == BudgetStatus::Ok ? m_limits.maxCacheSpans - m_work.cacheSpans : 0;
}

std::size_t WorkBudget::remainingVertices() const {
    return m_status == BudgetStatus::Ok ? m_limits.maxVertices - m_work.vertices : 0;
}

std::size_t WorkBudget::remainingPairs() const {
    return m_status == BudgetStatus::Ok ? m_limits.maxDistancePairs - m_work.distancePairs : 0;
}

bool WorkBudget::consume(const std::size_t count, const std::size_t limit, std::size_t& used) {
    if (m_status != BudgetStatus::Ok) {
        return false;
    }
    if (count > limit - used) {
        m_status = BudgetStatus::LimitExceeded;
        return false;
    }
    used += count;
    return true;
}

bool WorkBudget::consumePieces(const std::size_t count) {
    return consume(count, m_limits.maxCacheSpans, m_work.cacheSpans);
}

bool WorkBudget::consumeVertices(const std::size_t count) {
    return consume(count, m_limits.maxVertices, m_work.vertices);
}

bool WorkBudget::consumePairs(const std::size_t count) {
    return consume(count, m_limits.maxDistancePairs, m_work.distancePairs);
}

bool WorkBudget::checkDepth(const unsigned depth) {
    if (m_status != BudgetStatus::Ok) {
        return false;
    }
    if (depth > m_limits.maxDepth) {
        m_status = BudgetStatus::LimitExceeded;
        return false;
    }
    m_work.deepest = std::max(m_work.deepest, depth);
    return true;
}

unsigned WorkBudget::depthLimit() const {
    return m_limits.maxDepth;
}

void WorkBudget::latchLimitExceeded() {
    if (m_status == BudgetStatus::Ok) {
        m_status = BudgetStatus::LimitExceeded;
    }
}

std::optional<CacheLayoutCounts> cacheLayoutCounts(const std::size_t degree, const std::size_t spans) noexcept {
    if ((degree != 1 && degree != 3) || spans == 0 ||
        spans > (std::numeric_limits<std::size_t>::max() - degree - 2) / degree) { return {}; }
    return CacheLayoutCounts{spans, degree * spans + 1, degree * spans + degree + 2};
}

std::optional<CacheLayoutCounts> cacheLayoutCountsFromControls(const std::size_t degree,
                                                            const std::size_t controls) noexcept {
    if ((degree != 1 && degree != 3) || controls < degree + 1 || (controls - 1) % degree != 0) { return {}; }
    return cacheLayoutCounts(degree, (controls - 1) / degree);
}

CacheLayoutResult inspectCacheLayout(const RS_SplineData& cache, WorkBudget& budget) {
    CacheLayoutResult result;
    const auto finish = [&]() { result.work = budget.consumed(); return std::move(result); };
    if (budget.status() != BudgetStatus::Ok) {
        if (budget.status() == BudgetStatus::LimitExceeded) { result.status = CacheLayoutStatus::LimitExceeded; }
        return finish();
    }
    result.status = CacheLayoutStatus::InvalidLayout;
    const auto counts = cacheLayoutCountsFromControls(cache.degree, cache.controlPoints.size());
    if (!counts || cache.type != RS_SplineData::SplineType::ClampedOpen || !cache.fitPoints.empty() ||
        cache.savedOpenType != RS_SplineData::SplineType::ClampedOpen || !cache.savedOpenKnots.empty() ||
        cache.knotslist.size() != counts->knots || cache.weights.size() != counts->controls) { return finish(); }
    // Both counters are admitted before either is spent. Count/array checks above inspect no elements.
    if (counts->spans > budget.remainingPieces() || counts->controls > budget.remainingVertices() ||
        counts->spans > result.spans.max_size()) {
        budget.latchLimitExceeded(); result.status = CacheLayoutStatus::LimitExceeded;
        return finish();
    }
    if (!budget.consumePieces(counts->spans) || !budget.consumeVertices(counts->controls)) {
        result.status = CacheLayoutStatus::LimitExceeded;
        return finish();
    }
    for (std::size_t i = 0; i < counts->controls; ++i) {
        if (!isFinite2D(cache.controlPoints[i]) || !std::isfinite(cache.weights[i]) || cache.weights[i] != 1.0) {
            return finish();
        }
    }
    std::size_t knot = 0;
    double previous = 0.0;
    for (std::size_t group = 0; group <= counts->spans; ++group) {
        const double value = cache.knotslist[knot];
        if (!std::isfinite(value) || (group != 0 && !(previous < value))) { return finish(); }
        const std::size_t multiplicity = group == 0 || group == counts->spans ? cache.degree + 1 : cache.degree;
        for (std::size_t i = 0; i < multiplicity; ++i) {
            if (cache.knotslist[knot++] != value) { return finish(); }
        }
        previous = value;
    }
    // All groups are valid before any output allocation; each span keeps its actual affine domain.
    result.spans.reserve(counts->spans);
    for (std::size_t k = 0; k < counts->spans; ++k) {
        CacheSpan span;
        span.degree = cache.degree;
        for (std::size_t i = 0; i <= cache.degree; ++i) {
            span.controls[i] = cache.controlPoints[cache.degree * k + i];
        }
        const std::size_t end = cache.degree * (k + 1);
        span.parameter = LC_Interval::hull(cache.knotslist[end], cache.knotslist[end + 1]);
        result.spans.push_back(std::move(span));
    }
    result.status = CacheLayoutStatus::Ok;
    return finish();
}

bool isFinite2D(const RS_Vector& point) {
    return point.valid && std::isfinite(point.x) && std::isfinite(point.y) && point.z == 0.0;
}

std::optional<Sweep> sweep(const RS_EllipseData& basis) {
    if (!std::isfinite(basis.angle1) || !std::isfinite(basis.angle2)) {
        return std::nullopt;
    }
    const double delta = basis.angle2 - basis.angle1;
    if (!std::isfinite(delta) || std::abs(delta) > turn + fullTurnAllowance) {
        return std::nullopt;
    }
    const bool whole = (basis.angle1 == 0.0 && basis.angle2 == 0.0) ||
                       std::abs(std::abs(delta) - turn) <= fullTurnAllowance;
    if (whole) {
        return Sweep{M_PI_4, basis.reversed ? -turn : turn, true};
    }
    if (delta == 0.0) {
        return std::nullopt;
    }
    double directed = delta;
    if (basis.reversed && directed > 0.0) {
        directed -= turn;
    } else if (!basis.reversed && directed < 0.0) {
        directed += turn;
    }
    if (!(std::abs(directed) > RS_TOLERANCE_ANGLE)) {
        return std::nullopt;
    }
    return Sweep{wrapped(basis.angle1), directed, false};
}

bool Sweep::contains(const double parameter, const double angularAllowance) const {
    if (!std::isfinite(parameter) || !std::isfinite(start) || !std::isfinite(signedSweep) ||
        !std::isfinite(angularAllowance) || angularAllowance < 0.0 || signedSweep == 0.0) {
        return false;
    }
    if (whole) {
        return true;
    }
    const double delta = (wrapped(parameter) - wrapped(start)) * (signedSweep < 0.0 ? -1.0 : 1.0);
    if (!std::isfinite(delta)) {
        return false;
    }
    const double along = wrapped(delta);
    return along <= std::abs(signedSweep) + angularAllowance || turn - along <= angularAllowance;
}

bool normaliseBasis(RS_EllipseData& basis) {
    if (!frame(basis, false)) {
        return false;
    }
    const auto interval = sweep(basis);
    if (!interval) {
        return false;
    }
    RS_EllipseData candidate = basis;
    candidate.center = RS_Vector{0.0, 0.0};
    candidate.setAngle1(interval->whole ? 0.0 : wrapped(basis.angle1));
    candidate.setAngle2(interval->whole ? 0.0 : wrapped(basis.angle2));
    if (!interval->whole && std::abs(candidate.angle1) < RS_TOLERANCE_ANGLE &&
        std::abs(candidate.angle2) < RS_TOLERANCE_ANGLE) {
        return false; // the native ellipse API cannot keep this near-full arc distinct from whole
    }
    // Axis switching re-derives arc angles from endpoints. Do that about zero, not a large world centre.
    RS_Ellipse ellipse{nullptr, candidate};
    if (ellipse.getRatio() > 1.0 && !ellipse.switchMajorMinor()) {
        return false;
    }
    ellipse.setAngle1(interval->whole ? 0.0 : wrapped(ellipse.getAngle1()));
    ellipse.setAngle2(interval->whole ? 0.0 : wrapped(ellipse.getAngle2()));
    ellipse.setCenter(basis.center);
    candidate = ellipse.getData();
    const auto normalizedFrame = frame(candidate);
    const auto normalizedSweep = sweep(candidate);
    if (!normalizedFrame || !normalizedSweep || normalizedSweep->whole != interval->whole ||
        !isFinite2D(ellipse.getMin()) || !isFinite2D(ellipse.getMax())) {
        return false;
    }
    const double diagonal = std::hypot(ellipse.getMax().x - ellipse.getMin().x,
                                       ellipse.getMax().y - ellipse.getMin().y);
    if (!std::isfinite(diagonal) || !(diagonal > 0.0)) {
        return false;
    }
    for (const RS_Vector& axis : {normalizedFrame->major, normalizedFrame->minor}) {
        const RS_Vector positive = candidate.center + axis;
        const RS_Vector negative = candidate.center - axis;
        if (!isFinite2D(positive) || !isFinite2D(negative) ||
            (positive.x == negative.x && positive.y == negative.y)) {
            return false;
        }
    }
    basis = candidate;
    return true;
}

std::optional<ReferenceFrame> ReferenceFrame::create(const RS_EllipseData& basis, const double signedDistance) {
    const auto axes = frame(basis);
    const auto domain = sweep(basis);
    if (!axes || !domain || !std::isfinite(signedDistance) ||
        (!domain->whole && (!(basis.angle1 >= 0.0 && basis.angle1 < turn) ||
                            !(basis.angle2 >= 0.0 && basis.angle2 < turn)))) {
        return std::nullopt;
    }
    ReferenceFrame candidate;
    candidate.m_centre = basis.center;
    candidate.m_sweep = *domain;
    candidate.m_endAngle = domain->whole ? domain->start : basis.angle2;
    candidate.m_signedDistance = signedDistance;
    const LC_Interval realTurn = scaled(realPi(), 1);
    LC_Interval extent = realTurn;
    if (!domain->whole) {
        // Subtract exact endpoint doubles before adding a REAL-period winding.
        LC_Interval directed = LC_Interval::point(basis.angle2) - LC_Interval::point(basis.angle1);
        if (!basis.reversed && basis.angle2 < basis.angle1) {
            directed = directed + realTurn;
        } else if (basis.reversed && basis.angle2 > basis.angle1) {
            directed = directed - realTurn;
        }
        extent = basis.reversed ? -directed : directed;
    }
    candidate.m_rate = extent / LC_Interval::point(candidate.parameterEnd());
    candidate.m_rateSquared = sqr(candidate.m_rate);
    candidate.m_rateCubed = candidate.m_rateSquared * candidate.m_rate;
    if (!finiteInterval(candidate.m_rate) || !candidate.m_rate.isPositive() ||
        !finiteInterval(candidate.m_rateSquared) || !candidate.m_rateSquared.isPositive() ||
        !finiteInterval(candidate.m_rateCubed) || !candidate.m_rateCubed.isPositive()) {
        return std::nullopt;
    }
    int exponent = 0;
    std::frexp(std::max(axes->a, std::abs(signedDistance)), &exponent);
    candidate.m_scaleExponent = exponent - 1;
    candidate.m_scale = std::scalbn(1.0, candidate.m_scaleExponent);
    candidate.m_majorX = scaled(LC_Interval::point(basis.majorP.x), -candidate.m_scaleExponent);
    candidate.m_majorY = scaled(LC_Interval::point(basis.majorP.y), -candidate.m_scaleExponent);
    candidate.m_distance = scaled(LC_Interval::point(signedDistance), -candidate.m_scaleExponent);
    candidate.m_ratio = LC_Interval::point(basis.ratio);
    candidate.m_axisSquared = sqr(candidate.m_majorX) + sqr(candidate.m_majorY);
    candidate.m_axis = sqrt(candidate.m_axisSquared);
    if (!(candidate.m_scale > 0.0) || !std::isfinite(candidate.m_scale) ||
        !finiteInterval(candidate.m_majorX) || !finiteInterval(candidate.m_majorY) ||
        !finiteInterval(candidate.m_distance) || !finiteInterval(candidate.m_axisSquared) ||
        !candidate.m_axisSquared.isPositive() || !finiteInterval(candidate.m_axis) || !candidate.m_axis.isPositive()) {
        return std::nullopt;
    }
    return candidate;
}

double ReferenceFrame::scale() const {
    return m_scale;
}

double ReferenceFrame::parameterEnd() const {
    return std::abs(m_sweep.signedSweep);
}

const Sweep& ReferenceFrame::sourceSweep() const {
    return m_sweep;
}

const LC_Interval& ReferenceFrame::traversalRate() const {
    return m_rate;
}

bool ReferenceFrame::boundUnwrappedAngle(const double t0, const double t1, LC_Interval& out) const {
    out = {};
    if (!std::isfinite(t0) || !std::isfinite(t1) || !(0.0 <= t0 && t0 <= t1 && t1 <= parameterEnd())) {
        return false;
    }
    const LC_Interval start = LC_Interval::point(m_sweep.start);
    const LC_Interval direction = LC_Interval::point(m_sweep.signedSweep < 0.0 ? -1.0 : 1.0);
    const LC_Interval angle = t1 == 0.0 ? start :
        start + direction * m_rate * LC_Interval::hull(t0, t1);
    if (!finiteInterval(angle)) {
        return false;
    }
    out = angle;
    return true;
}

double ReferenceFrame::engineSignedDistance() const {
    return m_sweep.signedSweep < 0.0 ? m_signedDistance : -m_signedDistance;
}

bool ReferenceFrame::trig(const double t0, const double t1, LC_Interval& cosine, LC_Interval& sine) const {
    cosine = {};
    sine = {};
    LC_Interval angle;
    if (!boundUnwrappedAngle(t0, t1, angle)) {
        return false;
    }
    if (t0 == t1 && t1 == parameterEnd()) {
        // gamma*T is the exact directed endpoint difference plus integer REAL turns.
        // This is periodic evaluation of the same continuous curve, not an endpoint patch.
        angle = LC_Interval::point(m_endAngle);
    }
    return referenceTrig(angle, cosine, sine);
}

bool ReferenceFrame::boundSourceJet(const double t0, const double t1, LC_CurveJetBounds& out) const {
    out = {};
    LC_Interval cosine;
    LC_Interval sine;
    if (!trig(t0, t1, cosine, sine)) {
        return false;
    }
    const LC_Interval direction = LC_Interval::point(m_sweep.signedSweep < 0.0 ? -1.0 : 1.0);
    const LC_Interval signedRate = direction * m_rate;
    const LC_Interval minorX = -m_majorY * m_ratio;
    const LC_Interval minorY = m_majorX * m_ratio;
    LC_CurveJetBounds candidate;
    candidate.x = m_majorX * cosine + minorX * sine;
    candidate.y = m_majorY * cosine + minorY * sine;
    candidate.dx = signedRate * (-m_majorX * sine + minorX * cosine);
    candidate.dy = signedRate * (-m_majorY * sine + minorY * cosine);
    candidate.ddx = -m_rateSquared * candidate.x;
    candidate.ddy = -m_rateSquared * candidate.y;
    const LC_Interval ratioSquared = sqr(m_ratio);
    const LC_Interval speedFactor = m_ratio.lo() == 1.0 ? LC_Interval::point(1.0) :
        ratioSquared + (LC_Interval::point(1.0) - ratioSquared) * sqr(sine);
    candidate.speedSquaredProduct = m_axisSquared * speedFactor * m_rateSquared;
    candidate.crossProduct = direction * m_axisSquared * m_ratio * m_rateCubed;
    if (!finiteBounds(candidate) || !finiteInterval(candidate.speedSquaredProduct) ||
        !finiteInterval(candidate.crossProduct)) {
        return false;
    }
    out = candidate;
    return true;
}

bool ReferenceFrame::boundOffsetJet(const double t0, const double t1, LC_CurveJetBounds& out) const {
    out = {};
    LC_CurveJetBounds source;
    LC_Interval cosine;
    LC_Interval sine;
    if (!boundSourceJet(t0, t1, source)) {
        return false;
    }
    if (m_signedDistance == 0.0) {
        out = source;
        return true;
    }
    if (!trig(t0, t1, cosine, sine)) {
        return false;
    }
    const LC_Interval direction = LC_Interval::point(m_sweep.signedSweep < 0.0 ? -1.0 : 1.0);
    const LC_Interval signedRate = direction * m_rate;
    LC_CurveJetBounds candidate;
    if (m_ratio.lo() == 1.0) {
        // Keep the shared circle radius, rather than independently bounding E + delta*N.
        const LC_Interval radius = m_axis + m_distance;
        LC_Interval factor;
        LC_Interval derivative;
        LC_Interval speed;
        if (!offsetFactor(cosine, sine, factor, derivative, speed)) {
            return false;
        }
        candidate.x = factor * source.x;
        candidate.y = factor * source.y;
        candidate.dx = -signedRate * candidate.y;
        candidate.dy = signedRate * candidate.x;
        candidate.ddx = -m_rateSquared * candidate.x;
        candidate.ddy = -m_rateSquared * candidate.y;
        candidate.speedSquaredProduct = sqr(radius) * m_rateSquared;
        candidate.crossProduct = direction * sqr(radius) * m_rateCubed;
    } else {
        LC_Interval factor;
        LC_Interval factorDerivative;
        LC_Interval speed;
        if (!offsetFactor(cosine, sine, factor, factorDerivative, speed)) {
            return false;
        }
        const LC_Interval normalX = (m_ratio * cosine * m_majorX - sine * m_majorY) / speed;
        const LC_Interval normalY = (m_ratio * cosine * m_majorY + sine * m_majorX) / speed;
        candidate.x = source.x + m_distance * normalX;
        candidate.y = source.y + m_distance * normalY;
        candidate.dx = factor * source.dx;
        candidate.dy = factor * source.dy;
        candidate.ddx = factor * source.ddx + factorDerivative * source.dx;
        candidate.ddy = factor * source.ddy + factorDerivative * source.dy;
        const LC_Interval speedSquared = sqr(factor) * source.speedSquaredProduct;
        const LC_Interval cross = sqr(factor) * source.crossProduct;
        if (finiteInterval(speedSquared)) {
            candidate.speedSquaredProduct = speedSquared;
        }
        if (finiteInterval(cross)) {
            candidate.crossProduct = cross;
        }
    }
    if (!finiteBounds(candidate)) {
        return false;
    }
    out = candidate;
    return true;
}

bool ReferenceFrame::offsetFactor(const LC_Interval& cosine, const LC_Interval& sine,
                                  LC_Interval& factor, LC_Interval& derivative, LC_Interval& speed) const {
    factor = derivative = speed = {};
    if (m_signedDistance == 0.0) {
        factor = LC_Interval::point(1.0);
        derivative = LC_Interval::point(0.0);
        return true;
    }
    if (m_ratio.lo() == 1.0) {
        const LC_Interval value = (m_axis + m_distance) / m_axis;
        if (!finiteInterval(value)) {
            return false;
        }
        factor = value;
        derivative = LC_Interval::point(0.0);
        speed = m_axis;
        return true;
    }
    const LC_Interval ratioSquared = sqr(m_ratio);
    const LC_Interval difference = LC_Interval::point(1.0) - ratioSquared;
    const LC_Interval h2 = ratioSquared + difference * sqr(sine);
    if (!finiteInterval(h2) || !h2.isPositive()) {
        return false;
    }
    // Physical angular speed, not the rate-scaled adapter speed.
    const LC_Interval physicalSpeed = m_axis * sqrt(h2);
    const LC_Interval denominator = physicalSpeed * h2;
    const LC_Interval derivativeDenominator = denominator * h2;
    if (!finiteInterval(physicalSpeed) || !physicalSpeed.isPositive() || !finiteInterval(denominator) ||
        !denominator.isPositive() || !finiteInterval(derivativeDenominator) || !derivativeDenominator.isPositive()) {
        return false;
    }
    const LC_Interval direction = LC_Interval::point(m_sweep.signedSweep < 0.0 ? -1.0 : 1.0);
    const LC_Interval value = LC_Interval::point(1.0) + m_distance * m_ratio / denominator;
    const LC_Interval slope = LC_Interval::point(-3.0) * direction * m_rate * m_distance * m_ratio *
        difference * sine * cosine / derivativeDenominator;
    if (!finiteInterval(value) || !finiteInterval(slope)) {
        return false;
    }
    factor = value;
    derivative = slope;
    speed = physicalSpeed;
    return true;
}

bool ReferenceFrame::boundOffsetFactor(const double t0, const double t1,
                                      LC_Interval& factor, LC_Interval& derivative) const {
    factor = derivative = {};
    LC_Interval cosine;
    LC_Interval sine;
    LC_Interval speed;
    return trig(t0, t1, cosine, sine) && offsetFactor(cosine, sine, factor, derivative, speed);
}

bool ReferenceFrame::sourceJet(const double t, LC_CurveJet& out) const {
    out = {};
    LC_CurveJetBounds bounds;
    return boundSourceJet(t, t, bounds) && jetFromBounds(bounds, out);
}

bool ReferenceFrame::offsetJet(const double t, LC_CurveJet& out) const {
    out = {};
    LC_CurveJetBounds bounds;
    return boundOffsetJet(t, t, bounds) && jetFromBounds(bounds, out);
}

bool ReferenceFrame::pointToFrame(const RS_Vector& world, LC_Interval& x, LC_Interval& y) const {
    x = {};
    y = {};
    if (!isFinite2D(world)) {
        return false;
    }
    const auto coordinate = [this](const double value, const double centre) {
        const LC_Interval delta = LC_Interval::point(value) - LC_Interval::point(centre);
        return finiteInterval(delta) ? scaled(delta, -m_scaleExponent) :
            scaled(LC_Interval::point(value), -m_scaleExponent) -
            scaled(LC_Interval::point(centre), -m_scaleExponent);
    };
    const LC_Interval localX = coordinate(world.x, m_centre.x);
    const LC_Interval localY = coordinate(world.y, m_centre.y);
    if (!finiteInterval(localX) || !finiteInterval(localY)) {
        return false;
    }
    x = localX;
    y = localY;
    return true;
}

bool ReferenceFrame::pointToWorld(const RS_Vector& local, LC_Interval& x, LC_Interval& y) const {
    x = {};
    y = {};
    if (!isFinite2D(local)) {
        return false;
    }
    const LC_Interval worldX = LC_Interval::point(m_centre.x) + scaled(LC_Interval::point(local.x), m_scaleExponent);
    const LC_Interval worldY = LC_Interval::point(m_centre.y) + scaled(LC_Interval::point(local.y), m_scaleExponent);
    if (!finiteInterval(worldX) || !finiteInterval(worldY)) {
        return false;
    }
    x = worldX;
    y = worldY;
    return true;
}

CriticalAdapter::CriticalAdapter(const ReferenceFrame& frame, const CriticalRegime regime)
    : m_frame{frame}, m_regime{regime} {}

const ReferenceFrame& CriticalAdapter::frame() const { return m_frame; }
CriticalRegime CriticalAdapter::regime() const { return m_regime; }
const std::vector<CriticalOccurrence>& CriticalAdapter::criticalOccurrences() const { return m_occurrences; }
const std::vector<CriticalRegion>& CriticalAdapter::regions() const { return m_regions; }
const std::vector<LC_ParametricSegment>& CriticalAdapter::segments() const { return m_segments; }
std::size_t CriticalAdapter::branchCount() const { return 1; }
std::ptrdiff_t CriticalAdapter::next(const std::size_t branch) const {
    return branch == 0 && m_frame.sourceSweep().whole ? 0 : -1;
}

bool CriticalAdapter::contains(const std::size_t segment, const LC_Interval& parameter) const {
    return segment < m_segments.size() && finiteInterval(parameter) &&
        m_segments[segment].t0 <= parameter.lo() && parameter.hi() <= m_segments[segment].t1;
}

bool CriticalAdapter::evaluate(const std::size_t segment, const double t,
                               RS_Vector& point, RS_Vector& derivative) const {
    point = derivative = RS_Vector{false};
    LC_CurveJet jet;
    if (!contains(segment, LC_Interval::point(t)) || !m_frame.offsetJet(t, jet)) {
        return false;
    }
    point = jet.point;
    derivative = jet.first;
    return true;
}

bool CriticalAdapter::bound(const std::size_t segment, const double a, const double b,
                            LC_Interval& x, LC_Interval& y, LC_Interval& dx, LC_Interval& dy) const {
    x = y = dx = dy = {};
    LC_CurveJetBounds bounds;
    if (!(a <= b) || !contains(segment, LC_Interval::hull(a, b)) || !m_frame.boundOffsetJet(a, b, bounds)) {
        return false;
    }
    x = bounds.x;
    y = bounds.y;
    dx = bounds.dx;
    dy = bounds.dy;
    return true;
}

LC_AnalyticLeafCertificate CriticalAdapter::certifyInjective(
    const std::size_t segment, const LC_Interval& parameter) const {
    return contains(segment, parameter) ? LC_AnalyticLeafCertificate{true, segment, parameter} :
        LC_AnalyticLeafCertificate{};
}

LC_AnalyticIntersectionCertificate CriticalAdapter::certifyIntersection(const LC_IntersectionDomain& box) const {
    if (!contains(box.segmentA, box.parameterA) || !contains(box.segmentB, box.parameterB)) {
        return {};
    }
    std::size_t first = box.segmentA;
    std::size_t last = box.segmentB;
    LC_Interval a = box.parameterA;
    LC_Interval b = box.parameterB;
    if (first > last || (first == last && a.lo() > b.lo())) {
        std::swap(first, last);
        std::swap(a, b);
    }
    bool excluded = false;
    if (first == last) {
        // A positive-width overlap is a diagonal continuum, not an isolated construction join.
        excluded = a.hi() <= b.lo();
    } else if (last - first == 1) {
        const auto& proof = m_joins[first];
        excluded = proof.from <= a.lo() && b.hi() <= proof.to;
    }
    if (!excluded && m_seam && first == 0 && last == m_segments.size() - 1) {
        excluded = a.hi() <= m_seam->to && m_seam->from <= b.lo();
    }
    if (!excluded) {
        return {};
    }
    LC_AnalyticIntersectionCertificate certificate;
    certificate.outcome = LC_AnalyticIntersectionCertificate::Outcome::Excluded;
    certificate.proof = LC_IntersectionProof::AnalyticExclusion;
    certificate.proofDomain = box; // Never publish the lifted or cross-segment proof interval as a solver domain.
    return certificate;
}

namespace {
using Rational = boost::multiprecision::cpp_rational;
using Integer = boost::multiprecision::cpp_int;

Rational exactDouble(const double value) {
    if (value == 0.0) {
        return Rational{0};
    }
    int exponent = 0;
    const double fraction = std::frexp(value, &exponent);
    const auto significand = static_cast<std::int64_t>(std::ldexp(fraction, 53));
    exponent -= 53;
    Integer numerator{significand};
    Integer denominator{1};
    if (exponent >= 0) { numerator <<= exponent; }
    else { denominator <<= -exponent; }
    return Rational{numerator} / Rational{denominator};
}

bool rationalBounds(const Rational& low, const Rational& high, LC_Interval& out) {
    out = {};
    if (low > high) {
        return false;
    }
    double a = low.convert_to<double>();
    double b = high.convert_to<double>();
    if (!std::isfinite(a) || !std::isfinite(b)) {
        return false;
    }
    // Conversion is only a seed. The exact inequalities, not a rounding-mode assumption, certify the result.
    for (unsigned i = 0; i < 8 && exactDouble(a) > low; ++i) {
        a = std::nextafter(a, -std::numeric_limits<double>::infinity());
        if (!std::isfinite(a)) {
            return false;
        }
    }
    for (unsigned i = 0; i < 8 && exactDouble(b) < high; ++i) {
        b = std::nextafter(b, std::numeric_limits<double>::infinity());
        if (!std::isfinite(b)) {
            return false;
        }
    }
    if (exactDouble(a) > low || exactDouble(b) < high) {
        return false;
    }
    out = LC_Interval::hull(a, b);
    return true;
}

struct ExactAngle { Rational constant; Rational pi; };

Rational dyadicRootUpper(const Rational& value, const unsigned degree) {
    const Integer n = boost::multiprecision::numerator(value);
    const Integer d = boost::multiprecision::denominator(value);
    // Fixed-degree expressions in finite binary64 inputs have only a bounded number of bits.
    const int exponent = static_cast<int>(boost::multiprecision::msb(n)) -
        static_cast<int>(boost::multiprecision::msb(d)) + 1;
    const int divisor = static_cast<int>(degree);
    const int rootExponent = exponent <= 0 ? exponent / divisor : (exponent + divisor - 1) / divisor;
    Integer magnitude{1};
    magnitude <<= rootExponent >= 0 ? rootExponent : -rootExponent;
    const Rational upper = rootExponent >= 0 ? Rational{magnitude} : Rational{1} / Rational{magnitude};
    Rational power = 1;
    for (unsigned i = 0; i < degree; ++i) { power *= upper; }
    return power >= value ? upper : Rational{0};
}

struct CriticalKernel {
    Rational axisSquared;
    Rational r2;
    Rational difference;
    Rational distanceSquared;
    std::array<Rational, 4> coefficient;

    CriticalKernel(const RS_EllipseData& basis, const double distance) {
        const Rational x = exactDouble(basis.majorP.x);
        const Rational y = exactDouble(basis.majorP.y);
        const Rational r = exactDouble(basis.ratio);
        const Rational d = exactDouble(distance);
        axisSquared = x * x + y * y;
        r2 = r * r;
        difference = 1 - r2;
        distanceSquared = distance < 0.0 ? Rational{d * d} : Rational{0};
        coefficient[0] = r2 * (axisSquared * r2 * r2 - distanceSquared);
        coefficient[1] = 3 * axisSquared * r2 * r2 * difference;
        coefficient[2] = 3 * axisSquared * r2 * difference * difference;
        coefficient[3] = axisSquared * difference * difference * difference;
    }
    Rational polynomial(const Rational& z) const {
        return ((coefficient[3] * z + coefficient[2]) * z + coefficient[1]) * z + coefficient[0];
    }
    Rational derivative(const Rational& z) const {
        return (3 * coefficient[3] * z + 2 * coefficient[2]) * z + coefficient[1];
    }
};

bool exactAngleBounds(const ExactAngle& angle, const Rational& piLow, const Rational& piHigh, LC_Interval& out) {
    const Rational low = angle.constant + angle.pi * (angle.pi < 0 ? piHigh : piLow);
    const Rational high = angle.constant + angle.pi * (angle.pi < 0 ? piLow : piHigh);
    return rationalBounds(low, high, out);
}

/** Pure bounded predicate shared by critical membership and construction contacts. */
int exactFactorSign(const CriticalKernel& kernel, const Rational& piLow, const Rational& piHigh,
                    const ExactAngle& first, const ExactAngle& last) {
    LC_Interval a, b;
    if (!exactAngleBounds(first, piLow, piHigh, a) || !exactAngleBounds(last, piLow, piHigh, b)) { return 2; }
    const double middle = 0.25 * a.lo() + 0.25 * a.hi() + 0.25 * b.lo() + 0.25 * b.hi();
    if (!std::isfinite(middle) || std::abs(middle) > 20.0) { return 2; }
    const int axis = static_cast<int>(std::round(middle / M_PI_2));
    const bool complement = axis % 2 != 0;
    const ExactAngle reducedA{first.constant, first.pi - Rational{axis} / 2};
    const ExactAngle reducedB{last.constant, last.pi - Rational{axis} / 2};
    LC_Interval variable;
    if (reducedA.constant == 0 && reducedA.pi == 0 && reducedB.constant == 0 && reducedB.pi == 0) {
        variable = LC_Interval::point(0.0);
    } else {
        if (!exactAngleBounds(reducedA, piLow, piHigh, a) || !exactAngleBounds(reducedB, piLow, piHigh, b)) {
            return 2;
        }
        LC_Interval cosine, sine;
        if (!referenceTrig(LC_Interval::hull(a, b), cosine, sine)) { return 2; }
        const LC_Interval z = sqr(sine);
        variable = LC_Interval::hull(std::max(0.0, z.lo()), std::min(1.0, z.hi()));
        if (!finiteInterval(variable)) { return 2; }
    }
    // Preserve 1-sin^2(reduced) exactly near a minor axis, including tiny P(1).
    const Rational low = complement ? Rational{1 - exactDouble(variable.hi())} : exactDouble(variable.lo());
    const Rational high = complement ? Rational{1 - exactDouble(variable.lo())} : exactDouble(variable.hi());
    if (kernel.polynomial(low) > 0) { return 1; }
    if (kernel.polynomial(high) < 0) { return -1; }
    return 2;
}

struct RationalRange { Rational lo, hi; };

LC_Interval intersect(const LC_Interval& a, const LC_Interval& b) {
    if (!finiteInterval(a) || !finiteInterval(b)) { return {}; }
    const double lo = std::max(a.lo(), b.lo());
    const double hi = std::min(a.hi(), b.hi());
    return lo <= hi ? LC_Interval::hull(lo, hi) : LC_Interval{};
}

RationalRange product(const RationalRange& a, const RationalRange& b) {
    const std::array<Rational, 4> values{{a.lo * b.lo, a.lo * b.hi, a.hi * b.lo, a.hi * b.hi}};
    return {*std::min_element(values.begin(), values.end()), *std::max_element(values.begin(), values.end())};
}

RationalRange product(const RationalRange& a, const Rational& b) {
    return b < 0 ? RationalRange{a.hi * b, a.lo * b} : RationalRange{a.lo * b, a.hi * b};
}

bool powerThreeHalves(const RationalRange& value, RationalRange& out) {
    LC_Interval bounds;
    if (value.lo < 0 || !rationalBounds(value.lo, value.hi, bounds)) { return false; }
    const LC_Interval root = sqrt(bounds);
    if (!finiteInterval(root)) { return false; }
    out = product(value, RationalRange{exactDouble(std::max(0.0, root.lo())), exactDouble(root.hi())});
    return true;
}
} // namespace

bool ReferenceFrame::pointToWorld(const LC_Interval& localX, const LC_Interval& localY,
                                  LC_Interval& worldX, LC_Interval& worldY) const {
    const LC_Interval x = localX;
    const LC_Interval y = localY;
    worldX = {};
    worldY = {};
    if (!finiteInterval(x) || !finiteInterval(y)) { return false; }
    // Exact rational endpoint expressions avoid overflowing a product which centre addition cancels.
    const Rational scale = exactDouble(m_scale);
    LC_Interval candidateX, candidateY;
    if (!rationalBounds(exactDouble(m_centre.x) + scale * exactDouble(x.lo()),
                        exactDouble(m_centre.x) + scale * exactDouble(x.hi()), candidateX) ||
        !rationalBounds(exactDouble(m_centre.y) + scale * exactDouble(y.lo()),
                        exactDouble(m_centre.y) + scale * exactDouble(y.hi()), candidateY)) { return false; }
    worldX = candidateX;
    worldY = candidateY;
    return true;
}

/** Immutable factory provenance shared independently by the adapter and its refinement session. */
struct CriticalProvenance {
    RS_EllipseData basis;
    ReferenceFrame frame;
    CriticalKernel kernel;
    Rational piLow, piHigh, duration;
    ExactAngle start, extent;
    CriticalCanonicalBounds initial;
    unsigned initialDepth = 0;

    CriticalProvenance(const RS_EllipseData& raw, const ReferenceFrame& reference, const CriticalKernel& exact,
                       const Rational& lowPi, const Rational& highPi, const Rational& time,
                       const ExactAngle& first, const ExactAngle& span,
                       const CriticalCanonicalBounds& seed, const unsigned depth)
        : basis{raw}, frame{reference}, kernel{exact}, piLow{lowPi}, piHigh{highPi}, duration{time},
          start{first}, extent{span}, initial{seed}, initialDepth{depth} {}

    Rational residual(const Rational& u) const {
        return initial.complement ? Rational{-kernel.polynomial(1 - u)} : kernel.polynomial(u);
    }
    int factorSign(const double t) const {
        const Rational fraction = exactDouble(t) / duration;
        const ExactAngle angle{start.constant + extent.constant * fraction, start.pi + extent.pi * fraction};
        return exactFactorSign(kernel, piLow, piHigh, angle, angle);
    }
    bool map(const CriticalOccurrence& occurrence, const LC_Interval& alpha, LC_Interval& out) const {
        const int q = occurrence.quadrant + 4 * occurrence.winding;
        const bool even = q % 2 == 0;
        // Use alpha directly: in complement mode beta=real*pi/2-alpha, not rounded pi/2-beta.
        const int sign = (even != initial.complement) ? 1 : -1;
        const Rational axis = Rational{even ? q : q + 1} / 2 +
            (initial.complement ? Rational{even ? 1 : -1} / 2 : Rational{0});
        const ExactAngle low{exactDouble(sign > 0 ? alpha.lo() : -alpha.hi()), axis};
        const ExactAngle high{exactDouble(sign > 0 ? alpha.hi() : -alpha.lo()), axis};
        std::array<Rational, 4> values;
        std::size_t i = 0;
        for (const auto* angle : {&low, &high}) {
            for (const auto* pi : {&piLow, &piHigh}) {
                const Rational denominator = extent.constant + extent.pi * *pi;
                if (denominator == 0 || (denominator < 0) != basis.reversed) { return false; }
                values[i++] = duration * (angle->constant - start.constant + (angle->pi - start.pi) * *pi) /
                    denominator;
            }
        }
        return rationalBounds(*std::min_element(values.begin(), values.end()),
                              *std::max_element(values.begin(), values.end()), out);
    }
    bool image(const CriticalOccurrence& occurrence, const LC_Interval& u, LC_Interval& x, LC_Interval& y) const {
        x = {};
        y = {};
        const Rational r = exactDouble(basis.ratio);
        const Rational scale = exactDouble(frame.scale());
        const Rational mx = exactDouble(basis.majorP.x) / scale;
        const Rational my = exactDouble(basis.majorP.y) / scale;
        const Rational c = kernel.difference;
        if (occurrence.kind == CriticalKind::CriticalStationary) {
            const bool major = occurrence.quadrant % 2 == 0;
            const Rational factor = major ? c : Rational{r - 1 / r};
            const int sign = occurrence.quadrant < 2 ? 1 : -1;
            const Rational a = sign * factor * (major ? mx : Rational{-my});
            const Rational b = sign * factor * (major ? my : mx);
            return rationalBounds(a, a, x) && rationalBounds(b, b, y);
        }
        if (!finiteInterval(u) || u.lo() < 0.0 || u.hi() > 0.5) { return false; }
        const RationalRange small{exactDouble(u.lo()), exactDouble(u.hi())};
        const RationalRange large{1 - small.hi, 1 - small.lo};
        RationalRange zPower, cosPower;
        if (!powerThreeHalves(initial.complement ? large : small, zPower) ||
            !powerThreeHalves(initial.complement ? small : large, cosPower)) { return false; }
        const int cosSign = occurrence.quadrant == 0 || occurrence.quadrant == 3 ? 1 : -1;
        const int sinSign = occurrence.quadrant < 2 ? 1 : -1;
        // Keep coefficients rational until AFTER multiplication by raw axes/scale and tiny powers.
        const auto ax = product(cosPower, Rational{c * cosSign * mx});
        const auto bx = product(zPower, Rational{c * sinSign * my / r});
        const auto ay = product(cosPower, Rational{c * cosSign * my});
        const auto by = product(zPower, Rational{-c * sinSign * mx / r});
        return rationalBounds(ax.lo + bx.lo, ax.hi + bx.hi, x) &&
            rationalBounds(ay.lo + by.lo, ay.hi + by.hi, y);
    }
};

struct CriticalRefinementSession::Impl {
    enum class Step { Ok, Unavailable, Contradiction };
    struct Record { CriticalOccurrence original; CriticalOccurrenceSnapshot current; };
    std::shared_ptr<const CriticalProvenance> provenance;
    std::shared_ptr<const void> issuer;
    std::weak_ptr<const void> budgetIdentity;
    LC_OffsetCheckLimits ceilings;
    std::vector<Record> records;
    CriticalCanonicalBounds canonical;
    unsigned depth = 0;
    CheckWork lastWork{};
    bool terminal = false;
    bool limited = false;

    bool matches(const std::weak_ptr<const void>& identity, const std::shared_ptr<const void>& owner) const {
        return !identity.expired() && owner && !identity.owner_before(owner) && !owner.owner_before(identity);
    }
    bool handle(const CriticalOccurrenceHandle& handle) const {
        return matches(handle.m_issuer, issuer) && handle.m_index < records.size();
    }
    Step scalar(CriticalCanonicalBounds& next) const {
        const auto& p = *provenance;
        const Rational low = exactDouble(canonical.variable.lo());
        const Rational high = exactDouble(canonical.variable.hi());
        if (p.residual(low) > 0 || p.residual(high) < 0) { return Step::Contradiction; }
        const Rational mid = (low + high) / 2;
        const Rational value = p.residual(mid);
        const Rational zLow = canonical.complement ? Rational{1 - high} : low;
        const Rational zHigh = canonical.complement ? Rational{1 - low} : high;
        // g(u)=-P(1-u) still has positive derivative P'(1-u); P' increases in z.
        const Rational dLow = p.kernel.derivative(zLow);
        const Rational dHigh = p.kernel.derivative(zHigh);
        if (dLow <= 0 || dHigh < dLow) { return Step::Contradiction; }
        const Rational a = mid - value / dLow;
        const Rational b = mid - value / dHigh;
        const Rational newLow = std::max(low, std::min(a, b));
        const Rational newHigh = std::min(high, std::max(a, b));
        if (newLow > newHigh) { return Step::Contradiction; }
        LC_Interval enclosure;
        if (!rationalBounds(newLow, newHigh, enclosure)) { return Step::Unavailable; }
        next.variable = intersect(canonical.variable, enclosure);
        return finiteInterval(next.variable) && p.residual(exactDouble(next.variable.lo())) <= 0 &&
            p.residual(exactDouble(next.variable.hi())) >= 0 ? Step::Ok : Step::Contradiction;
    }
    Step inverse(CriticalCanonicalBounds& next) const {
        const LC_Interval prior = canonical.inverseAngle;
        LC_Interval c, s, cm, sm;
        const double mid = prior.lo() + 0.5 * (prior.hi() - prior.lo());
        if (!referenceTrig(prior, c, s) || !referenceTrig(LC_Interval::point(mid), cm, sm)) { return Step::Unavailable; }
        const LC_Interval residual = sqr(sm) - next.variable;
        const LC_Interval derivative = LC_Interval::point(2.0) * s * c;
        if (!finiteInterval(residual) || !finiteInterval(derivative)) { return Step::Unavailable; }
        next.inverseAngle = prior;
        const LC_Interval newton = derivative.isPositive() ? LC_Interval::point(mid) - residual / derivative :
            LC_Interval{};
        if (finiteInterval(newton)) {
            next.inverseAngle = intersect(prior, newton);
            if (!finiteInterval(next.inverseAngle)) { return Step::Contradiction; }
        } else if (residual.hi() < 0.0) {
            next.inverseAngle = LC_Interval::hull(mid, prior.hi());
        } else if (residual.lo() > 0.0) {
            next.inverseAngle = LC_Interval::hull(prior.lo(), mid);
        }
        // Mean-value interval Newton encloses the SAME retained alpha; U is never replaced by its midpoint.
        return finiteInterval(next.inverseAngle) ? Step::Ok : Step::Unavailable;
    }
    Step consistent(const CriticalCanonicalBounds& next) const {
        LC_Interval c, s;
        // Existence/identity come from the factory and the two mean-value contractions, NOT this overlap test.
        if (!finiteInterval(next.variable) || !finiteInterval(next.inverseAngle)) { return Step::Unavailable; }
        if (next.variable.lo() < 0.0 || next.variable.hi() > 0.5 || next.inverseAngle.lo() <= 0.0 ||
            next.inverseAngle.hi() >= 0.5 * realPi().lo() || next.complement != provenance->initial.complement ||
            provenance->residual(exactDouble(next.variable.lo())) > 0 ||
            provenance->residual(exactDouble(next.variable.hi())) < 0) { return Step::Contradiction; }
        if (!referenceTrig(next.inverseAngle, c, s)) { return Step::Unavailable; }
        const LC_Interval values = sqr(s);
        if (!finiteInterval(values)) { return Step::Unavailable; }
        return finiteInterval(intersect(values, next.variable)) ? Step::Ok : Step::Contradiction;
    }
};

class CriticalBuilder {
public:
    CriticalBuilder(const RS_EllipseData& basis, const double distance, const ReferenceFrame& frame,
                    WorkBudget& budget)
        : m_basis{basis}, m_distance{distance}, m_frame{frame}, m_budget{budget}, m_kernel{basis, distance},
          m_piLow{exactDouble(realPi().lo())}, m_piHigh{exactDouble(realPi().hi())},
          m_duration{exactDouble(frame.parameterEnd())} {
        m_start = {exactDouble(frame.sourceSweep().start), 0};
        m_end = {exactDouble(frame.sourceSweep().whole ? frame.sourceSweep().start : basis.angle2), 0};
        if (frame.sourceSweep().whole || (!basis.reversed && basis.angle2 < basis.angle1) ||
            (basis.reversed && basis.angle2 > basis.angle1)) {
            m_end.pi = basis.reversed ? -2 : 2;
        }
        m_extent = {m_end.constant - m_start.constant, m_end.pi - m_start.pi};
    }

    CriticalAnalysis run() {
        if (!prepare()) { return result(); }
        CriticalAnalysis out;
        out.status = LC_IntersectionStatus::Ok;
        out.work = m_budget.consumed();
        out.adapter = std::move(m_adapter);
        return out;
    }

    CriticalRetainedAnalysis runRetained() {
        CriticalRetainedAnalysis out;
        const auto finish = [&]() {
            out.work = m_budget.consumed();
            if (m_budget.status() == BudgetStatus::LimitExceeded) {
                out.status = LC_IntersectionStatus::LimitExceeded;
                out.reason = CriticalAnalysis::Reason::None;
            }
            return std::move(out);
        };
        if (!prepare()) {
            const auto failed = result();
            out.status = failed.status;
            out.reason = failed.reason;
            return finish();
        }
        out.status = LC_IntersectionStatus::Ok;
        const auto& occurrences = m_adapter->criticalOccurrences();
        const bool canonical = m_adapter->regime() == CriticalRegime::IntermediateThresholds;
        const bool token = !m_budget.m_identity;
        const std::size_t fixed = 3 + std::size_t(canonical) + std::size_t(token);
        if (occurrences.size() > std::numeric_limits<std::size_t>::max() - fixed) {
            m_budget.latchLimitExceeded();
            return finish();
        }
        // These are extra retained logical records. Legacy preparation has already finished unchanged.
        if (!m_budget.consumeVertices(fixed + occurrences.size()) || !m_budget.consumePairs(1)) {
            return finish();
        }
        out.status = LC_IntersectionStatus::AmbiguousTopology;
        out.reason = CriticalAnalysis::Reason::UnresolvedGeometry;
        auto provenance = std::make_shared<const CriticalProvenance>(m_basis, m_frame, m_kernel,
            m_piLow, m_piHigh, m_duration, m_start, m_extent, m_canonical, m_scalarDepth);
        auto impl = std::make_unique<CriticalRefinementSession::Impl>();
        impl->provenance = provenance;
        impl->canonical = m_canonical;
        impl->depth = m_scalarDepth;
        impl->ceilings = m_budget.m_limits;
        if (canonical) {
            const auto common = impl->consistent(impl->canonical);
            if (common != CriticalRefinementSession::Impl::Step::Ok) {
                if (common == CriticalRefinementSession::Impl::Step::Contradiction) {
                    out.status = LC_IntersectionStatus::InvalidInput;
                    out.reason = CriticalAnalysis::Reason::None;
                }
                return finish();
            }
        }
        if (occurrences.size() > impl->records.max_size()) {
            m_budget.latchLimitExceeded();
            return finish();
        }
        impl->records.reserve(occurrences.size());
        for (const auto& occurrence : occurrences) {
            if (!m_budget.consumePairs(1)) { return finish(); }
            CriticalOccurrenceSnapshot snapshot;
            snapshot.occurrence = occurrence;
            snapshot.admittedRounds = m_scalarDepth;
            if (canonical) {
                snapshot.canonical = m_canonical;
                LC_Interval parameter;
                if (!provenance->map(occurrence, m_canonical.inverseAngle, parameter)) { return finish(); }
                snapshot.occurrence.parameter = intersect(occurrence.parameter, parameter);
                if (!finiteInterval(snapshot.occurrence.parameter)) { return finish(); }
            }
            if (!provenance->image(occurrence, m_canonical.variable, snapshot.imageX, snapshot.imageY)) {
                return finish();
            }
            impl->records.push_back({occurrence, snapshot});
        }
        impl->issuer = std::make_shared<const unsigned char>(0);
        if (token) { m_budget.m_identity = std::make_shared<const unsigned char>(0); }
        impl->budgetIdentity = m_budget.m_identity;
        impl->lastWork = m_budget.consumed();
        // The adapter is immutable thereafter; the session owns independent mutable brackets/caches.
        m_adapter->m_provenance = std::move(provenance);
        out.refinement.reset(new CriticalRefinementSession{std::move(impl)});
        out.adapter = std::move(m_adapter);
        out.status = LC_IntersectionStatus::Ok;
        out.reason = CriticalAnalysis::Reason::None;
        return finish();
    }

private:
    struct Axis { int index; LC_Interval parameter; bool atStart; bool atEnd; };
    const RS_EllipseData& m_basis;
    double m_distance;
    ReferenceFrame m_frame;
    WorkBudget& m_budget;
    CriticalKernel m_kernel;
    Rational m_piLow;
    Rational m_piHigh;
    Rational m_duration;
    ExactAngle m_start;
    ExactAngle m_end;
    ExactAngle m_extent;
    std::unique_ptr<CriticalAdapter> m_adapter;
    std::vector<Axis> m_axes;
    LC_Interval m_beta;
    CriticalCanonicalBounds m_canonical;
    unsigned m_scalarDepth = 0;
    CriticalAnalysis::Reason m_reason = CriticalAnalysis::Reason::UnresolvedGeometry;

    bool prepare() {
        const auto regime = classify();
        if (!regime) { return false; }
        m_adapter.reset(new CriticalAdapter{m_frame, *regime});
        return axes() && occurrences() && regions() && joins() && preflight();
    }
    int direction() const { return m_basis.reversed ? -1 : 1; }
    bool unresolved() {
        m_reason = CriticalAnalysis::Reason::UnresolvedGeometry;
        return false;
    }
    CriticalAnalysis result() const {
        CriticalAnalysis out;
        out.work = m_budget.consumed();
        out.status = m_budget.status() == BudgetStatus::LimitExceeded ? LC_IntersectionStatus::LimitExceeded :
            m_budget.status() == BudgetStatus::InvalidInput ? LC_IntersectionStatus::InvalidInput :
            LC_IntersectionStatus::AmbiguousTopology;
        out.reason = out.status == LC_IntersectionStatus::AmbiguousTopology ? m_reason : CriticalAnalysis::Reason::None;
        return out;
    }
    bool generated(const std::size_t count, const unsigned depth) {
        if (depth > m_budget.depthLimit()) {
            m_budget.latchLimitExceeded();
            return false;
        }
        return m_budget.consumeVertices(count);
    }
    bool split(const unsigned depth) {
        if (depth >= m_budget.depthLimit()) {
            m_budget.latchLimitExceeded();
            return false;
        }
        return generated(2, depth + 1);
    }
    std::optional<CriticalRegime> classify() {
        if (m_distance >= 0.0) {
            return CriticalRegime::PositiveFactor;
        }
        if (m_basis.ratio == 1.0 && m_kernel.distanceSquared == m_kernel.axisSquared) {
            m_reason = CriticalAnalysis::Reason::CollapsedCircle;
            return {};
        }
        const Rational minimum = m_kernel.axisSquared * m_kernel.r2 * m_kernel.r2;
        if (m_kernel.distanceSquared < minimum) {
            return CriticalRegime::PositiveFactor;
        }
        if (m_kernel.distanceSquared == minimum) {
            return CriticalRegime::MinimumStationary;
        }
        const Rational maximum = m_kernel.distanceSquared * m_kernel.r2;
        if (maximum > m_kernel.axisSquared) {
            return CriticalRegime::NegativeFactor;
        }
        return maximum == m_kernel.axisSquared ? CriticalRegime::MaximumStationary :
            CriticalRegime::IntermediateThresholds;
    }
    ExactAngle angle(const double t) const {
        const Rational fraction = exactDouble(t) / m_duration;
        return {m_start.constant + m_extent.constant * fraction, m_start.pi + m_extent.pi * fraction};
    }
    bool angleBounds(const ExactAngle& a, LC_Interval& out) const {
        return exactAngleBounds(a, m_piLow, m_piHigh, out);
    }
    int compareAxis(const int index, const ExactAngle& endpoint) const {
        const Rational coefficient = Rational{index} / 2 - endpoint.pi;
        if (coefficient == 0) {
            return endpoint.constant == 0 ? 0 : endpoint.constant < 0 ? 1 : -1;
        }
        const Rational low = coefficient * (coefficient < 0 ? m_piHigh : m_piLow) - endpoint.constant;
        const Rational high = coefficient * (coefficient < 0 ? m_piLow : m_piHigh) - endpoint.constant;
        // Pi is STRICTLY between these rational constants; equality to a bracket edge is not axis membership.
        if (low >= 0) { return 1; }
        if (high <= 0) { return -1; }
        return 2;
    }
    bool mapAngle(const ExactAngle& low, const ExactAngle& high, LC_Interval& out) const {
        std::array<Rational, 4> values;
        std::size_t i = 0;
        for (const ExactAngle* endpoint : {&low, &high}) {
            for (const Rational* pi : {&m_piLow, &m_piHigh}) {
                const Rational denominator = m_extent.constant + m_extent.pi * *pi;
                if (denominator == 0 || (denominator < 0) != m_basis.reversed) {
                    return false;
                }
                values[i++] = m_duration * (endpoint->constant - m_start.constant +
                    (endpoint->pi - m_start.pi) * *pi) / denominator;
            }
        }
        // A linear-fractional function of pi has constant derivative sign on this nonzero denominator.
        return rationalBounds(*std::min_element(values.begin(), values.end()),
                              *std::max_element(values.begin(), values.end()), out);
    }
    bool axes() {
        for (int index = -8; index <= 12; ++index) {
            if (!m_budget.consumePairs(1)) { return false; }
            const int a = compareAxis(index, m_start);
            const int b = compareAxis(index, m_end);
            if ((a != 2 && direction() * a < 0) || (b != 2 && direction() * b > 0)) { continue; }
            if (a == 2 || b == 2) { return unresolved(); }
            if (!generated(1, 0)) { return false; }
            LC_Interval parameter;
            const ExactAngle axis{0, Rational{index} / 2};
            if (a == 0) { parameter = LC_Interval::point(0.0); }
            else if (b == 0) { parameter = LC_Interval::point(m_frame.parameterEnd()); }
            else if (!mapAngle(axis, axis, parameter)) { return unresolved(); }
            const double lo = std::max(0.0, parameter.lo());
            const double hi = std::min(m_frame.parameterEnd(), parameter.hi());
            if (!(lo <= hi)) { return unresolved(); }
            m_axes.push_back({index, LC_Interval::hull(lo, hi), a == 0, b == 0});
        }
        std::sort(m_axes.begin(), m_axes.end(), [](const Axis& a, const Axis& b) {
            return a.parameter.lo() < b.parameter.lo();
        });
        for (std::size_t i = 1; i < m_axes.size(); ++i) {
            if (m_axes[i - 1].parameter.hi() > m_axes[i].parameter.lo()) { return unresolved(); }
        }
        return true;
    }
    int sign(const ExactAngle& first, const ExactAngle& last) const {
        return exactFactorSign(m_kernel, m_piLow, m_piHigh, first, last);
    }
    bool cuspAngle() {
        if (!generated(1, 0) || !m_budget.consumePairs(1)) { return false; }
        const auto& k = m_kernel;
        const Rational positive = k.polynomial(Rational{1});
        Rational zHigh = std::min(Rational{1}, Rational{-k.coefficient[0] / k.coefficient[1]});
        const Rational squareUpper = dyadicRootUpper(-k.coefficient[0] / k.coefficient[2], 2);
        const Rational cubeUpper = dyadicRootUpper(-k.coefficient[0] / k.coefficient[3], 3);
        if (!(squareUpper > 0 && cubeUpper > 0)) { return unresolved(); }
        zHigh = std::min(zHigh, std::min(squareUpper, cubeUpper));
        const Rational zLow = -k.coefficient[0] /
            (k.coefficient[1] + k.coefficient[2] * zHigh + k.coefficient[3] * zHigh * zHigh);
        const Rational wLow = positive / (3 * k.axisSquared * k.difference);
        const Rational wHigh = std::min(Rational{1}, Rational{positive /
            (k.axisSquared * k.difference * (3 - 3 * k.difference + k.difference * k.difference))});
        // Select the SMALL variable by the exact root location, not by competing loose upper bounds.
        const Rational half = Rational{1} / 2;
        const bool complement = k.polynomial(half) < 0;
        const Rational selectedHigh = std::min(half, complement ? wHigh : zHigh);
        LC_Interval variable;
        if (!rationalBounds(complement ? wLow : zLow, selectedHigh, variable)) { return unresolved(); }
        double lo = std::max(0.0, variable.lo());
        double hi = std::min(1.0, variable.hi());
        const auto residual = [&k, complement](const double t) -> Rational {
            return complement ? Rational{-k.polynomial(1 - exactDouble(t))} : k.polynomial(exactDouble(t));
        };
        for (unsigned depth = 0;; ++depth) {
            if (depth != 0 && !m_budget.consumePairs(1)) { return false; }
            if (depth == 8 || lo == hi) { break; }
            const double mid = lo + 0.5 * (hi - lo);
            if (!(lo < mid && mid < hi)) {
                // Adjacent doubles can already enclose the unique monotone scalar root completely.
                if (residual(lo) <= 0 && residual(hi) >= 0) { break; }
                return unresolved();
            }
            if (!split(depth) || !m_budget.checkDepth(depth + 1)) { return false; }
            m_scalarDepth = depth + 1;
            const Rational value = residual(mid);
            if (value == 0) { lo = hi = mid; }
            else if (value < 0) { lo = mid; }
            else { hi = mid; }
        }
        variable = LC_Interval::hull(lo, hi);
        const LC_Interval root = sqrt(variable);
        const LC_Interval denominator = sqrt(LC_Interval::point(1.0) - variable);
        if (!finiteInterval(root) || !finiteInterval(denominator) || !denominator.isPositive()) { return unresolved(); }
        // x <= asin(x) <= x/sqrt(1-x^2), proved by integrating its increasing derivative.
        const LC_Interval upper = root / denominator;
        if (!finiteInterval(upper)) { return unresolved(); }
        const LC_Interval inverse = LC_Interval::hull(root.lo(), upper.hi());
        m_canonical = {variable, inverse, complement};
        m_beta = complement ? scaled(realPi(), -1) - inverse : inverse;
        return finiteInterval(m_beta) && m_beta.lo() > 0.0 && m_beta.hi() < realPi().lo() * 0.5;
    }
    int compareCusp(const int quadrant, const ExactAngle& endpoint) const {
        const int first = compareAxis(quadrant, endpoint);
        const int last = compareAxis(quadrant + 1, endpoint);
        if (first != 2 && first >= 0) { return 1; }
        if (last != 2 && last <= 0) { return -1; }
        if (first == 2 || last == 2) { return 2; }
        const int value = sign(endpoint, endpoint);
        return value == 2 ? 2 : (quadrant % 2 == 0 ? -value : value);
    }
    bool appendOccurrence(const LC_Interval& parameter, const CriticalKind kind, const int index,
                          const bool atStart, const bool atEnd) {
        if (!generated(1, 0)) { return false; }
        const int quadrant = (index % 4 + 4) % 4;
        m_adapter->m_occurrences.push_back({parameter, kind,
            kind == CriticalKind::SimpleCusp ? CriticalProof::MonotonePolynomial : CriticalProof::ExactAxisThreshold,
            quadrant, (index - quadrant) / 4, atStart, atEnd});
        return true;
    }
    bool occurrences() {
        const auto regime = m_adapter->m_regime;
        if (regime == CriticalRegime::MinimumStationary || regime == CriticalRegime::MaximumStationary) {
            for (const auto& axis : m_axes) {
                const bool major = axis.index % 2 == 0;
                if (major == (regime == CriticalRegime::MinimumStationary) &&
                    !appendOccurrence(axis.parameter, CriticalKind::CriticalStationary, axis.index,
                                      axis.atStart, axis.atEnd)) { return false; }
            }
        } else if (regime == CriticalRegime::IntermediateThresholds) {
            if (!cuspAngle()) { return false; }
            for (int quadrant = -8; quadrant < 12; ++quadrant) {
                if (!m_budget.consumePairs(1)) { return false; }
                const int a = compareCusp(quadrant, m_start);
                const int b = compareCusp(quadrant, m_end);
                if ((a != 2 && direction() * a < 0) || (b != 2 && direction() * b > 0)) { continue; }
                if (a == 2 || b == 2) { return unresolved(); }
                const bool even = quadrant % 2 == 0;
                const ExactAngle low{exactDouble(even ? m_beta.lo() : -m_beta.hi()),
                                     Rational{even ? quadrant : quadrant + 1} / 2};
                const ExactAngle high{exactDouble(even ? m_beta.hi() : -m_beta.lo()), low.pi};
                LC_Interval parameter;
                if (!mapAngle(low, high, parameter)) { return unresolved(); }
                const double lo = std::max(0.0, parameter.lo());
                const double hi = std::min(m_frame.parameterEnd(), parameter.hi());
                if (!(lo <= hi) || !appendOccurrence(LC_Interval::hull(lo, hi), CriticalKind::SimpleCusp,
                                                    quadrant, false, false)) { return false; }
            }
        }
        auto& occurrences = m_adapter->m_occurrences;
        std::sort(occurrences.begin(), occurrences.end(), [](const auto& a, const auto& b) {
            return a.parameter.lo() < b.parameter.lo();
        });
        for (std::size_t i = 1; i < occurrences.size(); ++i) {
            if (!(occurrences[i - 1].parameter.hi() < occurrences[i].parameter.lo())) { return unresolved(); }
        }
        return true;
    }
    std::optional<InjectivityProof> prove(const double from, const double to) const {
        const ExactAngle first = angle(m_basis.reversed ? to : from);
        const ExactAngle last = angle(m_basis.reversed ? from : to);
        LC_Interval a;
        LC_Interval b;
        if (!angleBounds(first, a) || !angleBounds(last, b)) { return {}; }
        const LC_Interval width = LC_Interval::point(b.hi()) - LC_Interval::point(a.lo());
        if (!finiteInterval(width) || !(width.hi() < realPi().lo())) { return {}; }
        const auto regime = m_adapter->m_regime;
        if (regime != CriticalRegime::IntermediateThresholds) {
            return regime == CriticalRegime::MinimumStationary || regime == CriticalRegime::MaximumStationary ?
                InjectivityProof::StationaryProjection : InjectivityProof::RegularProjection;
        }
        for (int q = -8; q < 12; ++q) {
            const int left = compareAxis(q, first);
            const int right = compareAxis(q + 1, last);
            if (left != 2 && right != 2 && left <= 0 && right >= 0) {
                // On one true quadrant F and sin(eta_c-eta) change sign together.
                // Their product has one sign, with only an isolated zero: its integral is strictly monotone.
                return InjectivityProof::SingleCuspProjection;
            }
        }
        if (sign(first, last) != 2) { return InjectivityProof::RegularProjection; }
        if (from < 0.0 || to > m_frame.parameterEnd()) { return {}; }
        std::size_t roots = 0;
        for (const auto& occurrence : m_adapter->m_occurrences) {
            if (occurrence.parameter.hi() < from || occurrence.parameter.lo() > to) { continue; }
            if (!(from < occurrence.parameter.lo() && occurrence.parameter.hi() < to)) { return {}; }
            ++roots;
        }
        return roots == 1 ? std::optional<InjectivityProof>{InjectivityProof::SingleCuspProjection} :
            std::optional<InjectivityProof>{};
    }
    bool addRegions(const double from, const double to, const bool band) {
        if (from == to) { return true; }
        struct Box { double from; double to; unsigned depth; };
        if (!generated(1, 0)) { return false; }
        std::vector<Box> pending{{from, to, 0}};
        while (!pending.empty()) {
            if (!m_budget.consumePairs(1)) { return false; }
            const Box box = pending.back();
            pending.pop_back();
            if (!m_budget.checkDepth(box.depth)) { return false; }
            const auto proof = prove(box.from, box.to);
            if (proof) {
                if (m_adapter->m_regions.size() == m_adapter->m_regions.max_size() ||
                    m_adapter->m_segments.size() == m_adapter->m_segments.max_size()) {
                    m_budget.latchLimitExceeded();
                    return false;
                }
                const LC_ParametricSegment segment{0, box.from, box.to};
                m_adapter->m_regions.push_back({segment, band, *proof});
                m_adapter->m_segments.push_back(segment);
                continue;
            }
            const double mid = box.from + 0.5 * (box.to - box.from);
            if (!(box.from < mid && mid < box.to)) { return unresolved(); }
            if (!split(box.depth)) { return false; }
            if (pending.size() > pending.max_size() - 2) {
                m_budget.latchLimitExceeded();
                return false;
            }
            pending.push_back({mid, box.to, box.depth + 1});
            pending.push_back({box.from, mid, box.depth + 1});
        }
        return true;
    }
    bool regions() {
        double cursor = 0.0;
        for (const auto& axis : m_axes) {
            if (!addRegions(cursor, axis.parameter.lo(), false) ||
                !addRegions(axis.parameter.lo(), axis.parameter.hi(), true)) { return false; }
            cursor = axis.parameter.hi();
        }
        return addRegions(cursor, m_frame.parameterEnd(), false) && !m_adapter->m_segments.empty();
    }
    bool joins() {
        const auto& segments = m_adapter->m_segments;
        for (std::size_t i = 1; i < segments.size(); ++i) {
            if (!generated(1, 0)) { return false; }
            const double join = segments[i].t0;
            double from = segments[i - 1].t0 + 0.5 * (join - segments[i - 1].t0);
            double to = join + 0.5 * (segments[i].t1 - join);
            if (from == join) { from = segments[i - 1].t0; }
            if (to == join) { to = segments[i].t1; }
            for (unsigned depth = 0;; ++depth) {
                if (!m_budget.consumePairs(1) || !m_budget.checkDepth(depth)) { return false; }
                if (prove(from, to)) { break; }
                const double left = from + 0.5 * (join - from);
                const double right = join + 0.5 * (to - join);
                const bool leftMoves = from < left && left < join;
                const bool rightMoves = join < right && right < to;
                if (!leftMoves && !rightMoves) { return unresolved(); }
                if (!split(depth)) { return false; }
                // An adjacent-double side stays intact; the next examination proves the WHOLE narrowed span.
                if (leftMoves) { from = left; }
                if (rightMoves) { to = right; }
            }
            m_adapter->m_joins.push_back({from, to});
        }
        if (m_frame.sourceSweep().whole) {
            if (!generated(1, 0) || !m_budget.consumePairs(1)) { return false; }
            const double extent = m_frame.parameterEnd();
            const double width = 0.125 * std::min(segments.front().t1, extent - segments.back().t0);
            const double last = extent - width;
            // last and extent are within a factor two, so this subtraction is exact (Sterbenz).
            if (!(width > 0.0 && last < extent) || !prove(last - extent, width)) { return unresolved(); }
            m_adapter->m_seam = CriticalAdapter::JoinProof{last, width};
        }
        return true;
    }
    bool preflight() {
        for (const auto& segment : m_adapter->m_segments) {
            if (!m_budget.consumePairs(1)) { return false; }
            LC_CurveJetBounds bounds;
            if (!m_frame.boundOffsetJet(segment.t0, segment.t1, bounds)) { return unresolved(); }
            for (const double t : {segment.t0, segment.t0 + 0.5 * (segment.t1 - segment.t0), segment.t1}) {
                LC_CurveJet seed;
                if (!m_frame.boundOffsetJet(t, t, bounds) || !m_frame.offsetJet(t, seed) ||
                    !isFinite2D(seed.point) || !isFinite2D(seed.first)) { return unresolved(); }
            }
        }
        return true;
    }
};

CriticalAnalysis analyseCritical(const RS_EllipseData& basis, const double signedDistance, WorkBudget& budget) {
    CriticalAnalysis out;
    const auto snapshot = [&]() {
        out.work = budget.consumed();
        if (budget.status() == BudgetStatus::LimitExceeded) { out.status = LC_IntersectionStatus::LimitExceeded; }
        return std::move(out);
    };
    if (budget.status() != BudgetStatus::Ok) { return snapshot(); }
    const auto domain = sweep(basis);
    if (!isFinite2D(basis.center) || !isFinite2D(basis.majorP) ||
        !(basis.majorP.x != 0.0 || basis.majorP.y != 0.0) || !std::isfinite(basis.ratio) ||
        !(basis.ratio > 0.0 && basis.ratio <= 1.0) || !std::isfinite(signedDistance) || !domain ||
        (!domain->whole && (!(basis.angle1 >= 0.0 && basis.angle1 < turn) ||
                            !(basis.angle2 >= 0.0 && basis.angle2 < turn)))) { return snapshot(); }
    if (!budget.consumeVertices(1) || !budget.consumePairs(1) || !budget.checkDepth(0)) { return snapshot(); }
    const auto reference = ReferenceFrame::create(basis, signedDistance);
    if (!reference) {
        out.status = LC_IntersectionStatus::AmbiguousTopology;
        out.reason = CriticalAnalysis::Reason::UnresolvedGeometry;
        return snapshot();
    }
    return CriticalBuilder{basis, signedDistance, *reference, budget}.run();
}

CriticalRetainedAnalysis analyseCriticalRetained(const RS_EllipseData& basis, const double signedDistance,
                                               WorkBudget& budget) {
    CriticalRetainedAnalysis out;
    const auto finish = [&]() {
        out.work = budget.consumed();
        if (budget.status() == BudgetStatus::LimitExceeded) { out.status = LC_IntersectionStatus::LimitExceeded; }
        return std::move(out);
    };
    if (budget.status() != BudgetStatus::Ok) { return finish(); }
    const auto domain = sweep(basis);
    if (!isFinite2D(basis.center) || !isFinite2D(basis.majorP) ||
        !(basis.majorP.x != 0.0 || basis.majorP.y != 0.0) || !std::isfinite(basis.ratio) ||
        !(basis.ratio > 0.0 && basis.ratio <= 1.0) || !std::isfinite(signedDistance) || !domain ||
        (!domain->whole && (!(basis.angle1 >= 0.0 && basis.angle1 < turn) ||
                            !(basis.angle2 >= 0.0 && basis.angle2 < turn)))) { return finish(); }
    if (!budget.consumeVertices(1) || !budget.consumePairs(1) || !budget.checkDepth(0)) { return finish(); }
    const auto reference = ReferenceFrame::create(basis, signedDistance);
    if (!reference) {
        out.status = LC_IntersectionStatus::AmbiguousTopology;
        out.reason = CriticalAnalysis::Reason::UnresolvedGeometry;
        return finish();
    }
    return CriticalBuilder{basis, signedDistance, *reference, budget}.runRetained();
}

namespace {
struct ReferenceCutter {
    LC_Interval x, y, nx, ny, tx, ty, distance;
    double orientation = 1.0;

    bool prepare(const ReferenceFrame& source, const double delta, const bool start) {
        const double t = start ? 0.0 : source.parameterEnd();
        LC_CurveJetBounds endpoint;
        if (!source.boundSourceJet(t, t, endpoint) || !finiteInterval(endpoint.speedSquaredProduct) ||
            !endpoint.speedSquaredProduct.isPositive()) { return false; }
        const LC_Interval speed = sqrt(endpoint.speedSquaredProduct);
        if (!finiteInterval(speed) || !speed.isPositive()) { return false; }
        const LC_Interval directedX = endpoint.dx / speed;
        const LC_Interval directedY = endpoint.dy / speed;
        const double direction = source.sourceSweep().signedSweep < 0.0 ? -1.0 : 1.0;
        const LC_Interval sign = LC_Interval::point(direction);
        const LC_Interval outward = LC_Interval::point(start ? -1.0 : 1.0);
        x = endpoint.x;
        y = endpoint.y;
        nx = sign * directedY;
        ny = -sign * directedX;
        tx = outward * directedX;
        ty = outward * directedY;
        distance = scaled(LC_Interval::point(std::abs(delta)), -std::ilogb(source.scale()));
        orientation = start ? -direction : direction;
        return finiteInterval(x) && finiteInterval(y) && finiteInterval(nx) && finiteInterval(ny) &&
            finiteInterval(tx) && finiteInterval(ty) && finiteInterval(distance) && distance.isPositive();
    }

    bool bound(const double a, const double b, LC_CurveJetBounds& out) const {
        out = {};
        if (!std::isfinite(a) || !std::isfinite(b) || !(0.0 <= a && a <= b && b <= 1.0)) {
            return false;
        }
        LC_Interval c, s;
        const LC_Interval zero = LC_Interval::point(0.0);
        const LC_Interval one = LC_Interval::point(1.0);
        if (a == b && a == 0.0) { c = one; s = zero; }
        else if (a == b && a == 0.5) { c = zero; s = one; }
        else if (a == b && a == 1.0) { c = -one; s = zero; }
        else if (!referenceTrig(realPi() * LC_Interval::hull(a, b), c, s)) { return false; }
        const LC_Interval radialX = nx * c + tx * s;
        const LC_Interval radialY = ny * c + ty * s;
        const LC_Interval pi = realPi();
        const LC_Interval piSquared = sqr(pi);
        LC_CurveJetBounds candidate;
        candidate.x = x + distance * radialX;
        candidate.y = y + distance * radialY;
        candidate.dx = distance * pi * (-nx * s + tx * c);
        candidate.dy = distance * pi * (-ny * s + ty * c);
        candidate.ddx = -distance * piSquared * radialX;
        candidate.ddy = -distance * piSquared * radialY;
        candidate.speedSquaredProduct = sqr(distance) * piSquared;
        candidate.crossProduct = LC_Interval::point(orientation) * candidate.speedSquaredProduct * pi;
        if (!finiteBounds(candidate) || !finiteInterval(candidate.speedSquaredProduct) ||
            !finiteInterval(candidate.crossProduct)) { return false; }
        out = candidate;
        return true;
    }
};

bool containsInterval(const LC_Interval& outer, const LC_Interval& inner) {
    return finiteInterval(outer) && finiteInterval(inner) && outer.lo() <= inner.lo() && inner.hi() <= outer.hi();
}
} // namespace

struct ReferenceProviders::Impl {
    struct Map { std::size_t part; std::size_t local; };
    struct Contact { LC_Interval parameter; double sourceEnd = 0.0; double capEnd = 0.0; };
    struct ContactLedger { std::array<std::array<std::optional<Contact>, 2>, 2> records; };
    std::array<std::unique_ptr<const CriticalAdapter>, 2> adapters;
    std::array<std::optional<ReferenceCutter>, 2> cutters;
    std::array<std::optional<std::size_t>, 4> branches;
    std::vector<LC_ParametricSegment> segments;
    std::vector<Map> maps;
    std::unique_ptr<const ContactLedger> contacts;
    // Only component preparation enables this exact major-radius contact family.
    std::shared_ptr<const CriticalProvenance> axisContacts;
    OppositeDisposition disposition;
    std::size_t count = 0;

    Impl(std::unique_ptr<const CriticalAdapter> own, std::unique_ptr<const CriticalAdapter> opposite,
         const OppositeDisposition value)
        : adapters{{std::move(own), std::move(opposite)}}, disposition{value} {}

    LC_ParametricSegment localSegment(const std::size_t part, const std::size_t local) const {
        return part < 2 ? adapters[part]->segments()[local] :
            LC_ParametricSegment{0, local == 0 ? 0.0 : 0.5, local == 0 ? 0.5 : 1.0};
    }
    bool contains(const std::size_t segment, const LC_Interval& parameter) const {
        return segment < segments.size() && finiteInterval(parameter) &&
            segments[segment].t0 <= parameter.lo() && parameter.hi() <= segments[segment].t1;
    }
    bool localContains(const std::size_t part, const std::size_t segment, const LC_Interval& parameter) const {
        const std::size_t size = part < 2 ? adapters[part]->segments().size() : 2;
        if (segment >= size || !finiteInterval(parameter)) { return false; }
        const auto piece = localSegment(part, segment);
        return piece.t0 <= parameter.lo() && parameter.hi() <= piece.t1;
    }
    bool axisParameter(const int q, LC_Interval& parameter) const {
        if (!axisContacts) { return false; }
        const auto& p = *axisContacts;
        const Rational low = p.extent.constant + p.extent.pi * p.piLow;
        const Rational high = p.extent.constant + p.extent.pi * p.piHigh;
        if (p.basis.reversed ? !(low < 0 && high < 0) : !(low > 0 && high > 0)) { return false; }
        std::array<Rational, 2> values;
        std::size_t index = 0;
        for (const auto* pi : {&p.piLow, &p.piHigh}) {
            const Rational denominator = p.extent.constant + p.extent.pi * *pi;
            if (denominator == 0) { return false; }
            values[index++] = p.duration * (-p.start.constant + (Rational{q} / 2 - p.start.pi) * *pi) /
                denominator;
        }
        return rationalBounds(std::min(values[0], values[1]), std::max(values[0], values[1]), parameter);
    }
    enum class AxisMembership { Unavailable, Absent, Present };
    struct AxisDomain {
        AxisMembership membership = AxisMembership::Unavailable;
        int index = 99;
        int cosSign = 0;
        LC_Interval parameter;
    };
    AxisDomain axisDomain(const LC_Interval& interval) const {
        if (!axisContacts) { return {}; }
        const auto& p = *axisContacts;
        const auto angle = [&](const double t) {
            const Rational fraction = exactDouble(t) / p.duration;
            return ExactAngle{p.start.constant + p.extent.constant * fraction,
                              p.start.pi + p.extent.pi * fraction};
        };
        LC_Interval first, last, c, s;
        if (!exactAngleBounds(angle(interval.lo()), p.piLow, p.piHigh, first) ||
            !exactAngleBounds(angle(interval.hi()), p.piLow, p.piHigh, last) ||
            !referenceTrig(LC_Interval::hull(std::min(first.lo(), last.lo()), std::max(first.hi(), last.hi())), c, s) ||
            !(c.lo() > 0 || c.hi() < 0)) { return {}; }
        AxisDomain out;
        out.cosSign = c.lo() > 0 ? 1 : -1;
        const auto sign = [&p](const ExactAngle& value) {
            const Rational low = value.constant + value.pi * (value.pi < 0 ? p.piHigh : p.piLow);
            const Rational high = value.constant + value.pi * (value.pi < 0 ? p.piLow : p.piHigh);
            if (value.constant == 0 && value.pi == 0) { return 0; }
            // A nonzero rational affine expression in irrational pi cannot equal zero.
            // This also resolves a dyadic boundary exactly equal to an enclosure endpoint.
            return low >= 0 ? 1 : high <= 0 ? -1 : 2;
        };
        bool uncertain = false;
        for (int q = -8; q <= 12; q += 2) {
            if ((q % 4 == 0 ? 1 : -1) != out.cosSign) { continue; }
            const auto lo = angle(p.basis.reversed ? interval.hi() : interval.lo());
            const auto hi = angle(p.basis.reversed ? interval.lo() : interval.hi());
            const int a = sign({lo.constant, lo.pi - Rational{q} / 2});
            const int b = sign({hi.constant, hi.pi - Rational{q} / 2});
            if (a == 1 || b == -1) { continue; }
            if (a == 2 || b == 2) { uncertain = true; continue; }
            LC_Interval candidate;
            if (!axisParameter(q, candidate)) { return {}; }
            out.parameter = intersect(interval, candidate);
            if (!finiteInterval(out.parameter)) { return {}; }
            out.index = q;
            out.membership = AxisMembership::Present;
            return out;
        }
        out.membership = uncertain ? AxisMembership::Unavailable : AxisMembership::Absent;
        return out;
    }
    int axisIn(const LC_Interval& interval, LC_Interval& parameter) const {
        const auto axis = axisDomain(interval);
        if (axis.membership != AxisMembership::Present) { return 99; }
        parameter = axis.parameter;
        return axis.index;
    }
    LC_AnalyticIntersectionCertificate axisContact(const LC_IntersectionDomain& box) const {
        if (!axisContacts) { return {}; }
        const auto a = maps[box.segmentA], b = maps[box.segmentB];
        const auto excluded = [&]() {
            return LC_AnalyticIntersectionCertificate{LC_AnalyticIntersectionCertificate::Outcome::Excluded,
                LC_IntersectionProof::AnalyticExclusion, box, {}, {}, LC_ParametricIntersection::Kind::Tangent};
        };
        if (a.part == 0 && b.part == 0) {
            const auto aa = axisDomain(box.parameterA), bb = axisDomain(box.parameterB);
            if (aa.cosSign == 0 || aa.cosSign == bb.cosSign || bb.cosSign == 0) { return {}; }
            // D=a: opposite strict major projections can agree only at the two major axes.
            // Absence is proved on the WHOLE box, even when numerical image boxes touch zero.
            if (aa.membership == AxisMembership::Absent || bb.membership == AxisMembership::Absent) {
                return excluded();
            }
            if (aa.membership != AxisMembership::Present || bb.membership != AxisMembership::Present) { return {}; }
            return {LC_AnalyticIntersectionCertificate::Outcome::Unique,
                LC_IntersectionProof::AnalyticTangency, box, aa.parameter, bb.parameter,
                LC_ParametricIntersection::Kind::Tangent};
        }
        const bool offsetFirst = a.part == 0 && b.part >= 2;
        if (!offsetFirst && !(b.part == 0 && a.part >= 2)) { return {}; }
        const auto cap = offsetFirst ? b.part : a.part;
        const auto& p = *axisContacts;
        const bool start = cap == 2;
        if ((start ? p.basis.angle1 : p.basis.angle2) != 0.0) { return {}; }
        const auto& interval = offsetFirst ? box.parameterA : box.parameterB;
        const auto& capInterval = offsetFirst ? box.parameterB : box.parameterA;
        const auto axis = axisDomain(interval);
        if (axis.cosSign != -1) { return {}; }
        // Enclose the unique opposite axis as well as the query before the radial proof.
        // It need not lie in the query: that case needs a certified exclusion, not endless bisection.
        LC_Interval anchor;
        int q = axis.index;
        if (q == 99) {
            for (int candidate = -6; candidate <= 10; candidate += 4) {
                LC_Interval value;
                if (axisParameter(candidate, value) && value.hi() >= 0 && value.lo() <= p.frame.parameterEnd()) {
                    const auto range = LC_Interval::hull(std::min(interval.lo(), value.lo()),
                                                         std::max(interval.hi(), value.hi()));
                    LC_Interval angleLo, angleHi;
                    const auto at = [&](const double t) {
                        const Rational fraction = exactDouble(t) / p.duration;
                        return ExactAngle{p.start.constant + p.extent.constant * fraction,
                                          p.start.pi + p.extent.pi * fraction};
                    };
                    if (!exactAngleBounds(at(range.lo()), p.piLow, p.piHigh, angleLo) ||
                        !exactAngleBounds(at(range.hi()), p.piLow, p.piHigh, angleHi) ||
                        std::max(angleLo.hi(), angleHi.hi()) - std::min(angleLo.lo(), angleHi.lo()) >= realPi().lo()) {
                        continue;
                    }
                    anchor = value; q = candidate; break;
                }
            }
        } else { anchor = axis.parameter; }
        if (q == 99 || !finiteInterval(anchor)) { return {}; }
        const auto proofRange = LC_Interval::hull(std::min(interval.lo(), anchor.lo()),
                                                  std::max(interval.hi(), anchor.hi()));
        LC_CurveJetBounds source, endpoint;
        LC_Interval factor, derivative;
        const double end = start ? 0.0 : p.frame.parameterEnd();
        if (!p.frame.boundSourceJet(proofRange.lo(), proofRange.hi(), source) ||
            !p.frame.boundSourceJet(end, end, endpoint) ||
            !p.frame.boundOffsetFactor(proofRange.lo(), proofRange.hi(), factor, derivative)) { return {}; }
        const auto radialDerivative = source.speedSquaredProduct + source.ddx * (source.x - endpoint.x) +
            source.ddy * (source.y - endpoint.y);
        if (!finiteInterval(radialDerivative) || radialDerivative.hi() >= 0 || factor.hi() >= 0) { return {}; }
        // The strict radial derivative proves a unique opposite-major-axis contact over this box.
        if (!capInterval.contains(1.0) || axis.membership == AxisMembership::Absent) { return excluded(); }
        if (axis.membership != AxisMembership::Present) { return {}; }
        return {LC_AnalyticIntersectionCertificate::Outcome::Unique, LC_IntersectionProof::AnalyticTangency,
            box, offsetFirst ? axis.parameter : LC_Interval::point(1),
            offsetFirst ? LC_Interval::point(1) : axis.parameter,
            LC_ParametricIntersection::Kind::Tangent};
    }
};

ReferenceProviders::ReferenceProviders(std::unique_ptr<Impl> impl) : m_impl{std::move(impl)} {}
ReferenceProviders::~ReferenceProviders() = default;
const ReferenceFrame& ReferenceProviders::frame() const noexcept { return m_impl->adapters[0]->frame(); }
std::optional<std::size_t> ReferenceProviders::branchFor(const ReferenceRole role) const noexcept {
    const auto index = static_cast<std::size_t>(role);
    return index < m_impl->branches.size() ? m_impl->branches[index] : std::nullopt;
}
OppositeDisposition ReferenceProviders::oppositeDisposition() const noexcept { return m_impl->disposition; }
std::optional<LC_Interval> ReferenceProviders::contactProofInterval(
    const ReferenceRole offset, const ReferenceRole cutter) const noexcept {
    const auto part = static_cast<std::size_t>(offset), cap = static_cast<std::size_t>(cutter);
    if (!m_impl->contacts || part >= 2 || cap < 2 || cap >= 4 ||
        !m_impl->branches[part] || !m_impl->branches[cap]) { return {}; }
    const auto& proof = m_impl->contacts->records[part][cap - 2];
    return proof ? std::optional<LC_Interval>{proof->parameter} : std::nullopt;
}
const std::vector<LC_ParametricSegment>& ReferenceProviders::segments() const { return m_impl->segments; }
std::size_t ReferenceProviders::branchCount() const { return m_impl->count; }
std::ptrdiff_t ReferenceProviders::next(const std::size_t branch) const {
    for (std::size_t part = 0; part < 2; ++part) {
        if (m_impl->branches[part] && *m_impl->branches[part] == branch) {
            return m_impl->adapters[part]->next(0) == 0 ? static_cast<std::ptrdiff_t>(branch) : -1;
        }
    }
    return -1;
}

bool ReferenceProviders::boundJet(const std::size_t segment, const double a, const double b,
                                 LC_CurveJetBounds& out) const {
    out = {};
    if (!std::isfinite(a) || !std::isfinite(b) || !(a <= b) ||
        !m_impl->contains(segment, LC_Interval::hull(a, b))) { return false; }
    const auto map = m_impl->maps[segment];
    return map.part < 2 ? m_impl->adapters[map.part]->frame().boundOffsetJet(a, b, out) :
        m_impl->cutters[map.part - 2]->bound(a, b, out);
}

bool ReferenceProviders::evaluate(const std::size_t segment, const double t,
                                  RS_Vector& point, RS_Vector& derivative) const {
    point = derivative = RS_Vector{false};
    LC_CurveJetBounds bounds;
    LC_CurveJet seed;
    if (!boundJet(segment, t, t, bounds) || !jetFromBounds(bounds, seed)) { return false; }
    point = seed.point;
    derivative = seed.first;
    return true;
}

bool ReferenceProviders::bound(const std::size_t segment, const double a, const double b,
                               LC_Interval& x, LC_Interval& y, LC_Interval& dx, LC_Interval& dy) const {
    x = y = dx = dy = {};
    LC_CurveJetBounds bounds;
    if (!boundJet(segment, a, b, bounds)) { return false; }
    x = bounds.x;
    y = bounds.y;
    dx = bounds.dx;
    dy = bounds.dy;
    return true;
}

LC_AnalyticLeafCertificate ReferenceProviders::certifyInjective(
    const std::size_t segment, const LC_Interval& parameter) const {
    if (!m_impl->contains(segment, parameter)) { return {}; }
    const auto map = m_impl->maps[segment];
    if (map.part >= 2) {
        // N.dot(Q-E) = D*cos(real_pi*u) is strictly decreasing on the whole cap.
        return {true, segment, parameter};
    }
    auto certificate = m_impl->adapters[map.part]->certifyInjective(map.local, parameter);
    if (!certificate.available || certificate.segment != map.local ||
        !m_impl->localContains(map.part, certificate.segment, certificate.parameter) ||
        !containsInterval(certificate.parameter, parameter)) { return {}; }
    certificate.segment = segment;
    return certificate;
}

LC_AnalyticIntersectionCertificate ReferenceProviders::certifyIntersection(const LC_IntersectionDomain& box) const {
    using Outcome = LC_AnalyticIntersectionCertificate::Outcome;
    if (!m_impl->contains(box.segmentA, box.parameterA) ||
        !m_impl->contains(box.segmentB, box.parameterB)) { return {}; }
    const auto a = m_impl->maps[box.segmentA];
    const auto b = m_impl->maps[box.segmentB];
    const auto excluded = [&]() {
        LC_AnalyticIntersectionCertificate out;
        out.outcome = Outcome::Excluded;
        out.proof = LC_IntersectionProof::AnalyticExclusion;
        out.proofDomain = box;
        return out;
    };
    if (a.part < 2 && b.part < 2 && a.part != b.part) {
        // Same exact strictly convex source and equal/opposite D: support equality
        // would require both the same source point and opposite outward normals.
        return excluded();
    }
    if (m_impl->axisContacts) {
        const auto certificate = m_impl->axisContact(box);
        if (certificate.outcome != Outcome::Unavailable) { return certificate; }
    }
    if (m_impl->contacts && ((a.part < 2 && b.part >= 2) || (b.part < 2 && a.part >= 2))) {
        const bool offsetFirst = a.part < 2;
        const auto offset = offsetFirst ? a : b, cap = offsetFirst ? b : a;
        const auto& record = m_impl->contacts->records[offset.part][cap.part - 2];
        const auto& offsetParameter = offsetFirst ? box.parameterA : box.parameterB;
        const auto& capParameter = offsetFirst ? box.parameterB : box.parameterA;
        if (!record || !containsInterval(record->parameter, offsetParameter)) { return {}; }
        if (!offsetParameter.contains(record->sourceEnd) || !capParameter.contains(record->capEnd)) {
            return excluded();
        }
        LC_AnalyticIntersectionCertificate out;
        out.outcome = Outcome::Unique;
        out.proof = LC_IntersectionProof::AnalyticEndpoint;
        out.proofDomain = box;
        out.isolationA = LC_Interval::point(offsetFirst ? record->sourceEnd : record->capEnd);
        out.isolationB = LC_Interval::point(offsetFirst ? record->capEnd : record->sourceEnd);
        out.kind = LC_ParametricIntersection::Kind::Endpoint;
        return out;
    }
    if (a.part != b.part) { return {}; } // Endpoint-cap contacts belong to event isolation.
    if (a.part >= 2) {
        // Injectivity excludes all reportable contacts, including the sole shared
        // construction endpoint of adjacent pieces, but not a diagonal continuum.
        return box.parameterA.hi() <= box.parameterB.lo() || box.parameterB.hi() <= box.parameterA.lo() ?
            excluded() : LC_AnalyticIntersectionCertificate{};
    }
    const LC_IntersectionDomain local{a.local, b.local, box.parameterA, box.parameterB};
    auto certificate = m_impl->adapters[a.part]->certifyIntersection(local);
    if (certificate.outcome == Outcome::Unavailable) { return {}; }
    const auto& proof = certificate.proofDomain;
    if (proof.segmentA != a.local || proof.segmentB != b.local ||
        !m_impl->localContains(a.part, proof.segmentA, proof.parameterA) ||
        !m_impl->localContains(a.part, proof.segmentB, proof.parameterB) ||
        !containsInterval(proof.parameterA, box.parameterA) || !containsInterval(proof.parameterB, box.parameterB)) {
        return {};
    }
    if (certificate.outcome == Outcome::Excluded) {
        if (certificate.proof != LC_IntersectionProof::AnalyticExclusion) { return {}; }
    } else if (certificate.outcome == Outcome::Unique) {
        if ((certificate.proof != LC_IntersectionProof::AnalyticEndpoint &&
             certificate.proof != LC_IntersectionProof::AnalyticCusp &&
             certificate.proof != LC_IntersectionProof::AnalyticTangency) ||
            !containsInterval(proof.parameterA, certificate.isolationA) ||
            !containsInterval(proof.parameterB, certificate.isolationB) ||
            (certificate.kind != LC_ParametricIntersection::Kind::Endpoint &&
             certificate.kind != LC_ParametricIntersection::Kind::Transverse &&
             certificate.kind != LC_ParametricIntersection::Kind::Tangent)) { return {}; }
    } else { return {}; }
    certificate.proofDomain.segmentA = box.segmentA;
    certificate.proofDomain.segmentB = box.segmentB;
    return certificate;
}

LC_AnalyticOccurrenceIdentityCertificate ReferenceProviders::certifyOccurrenceIdentity(
    const LC_ParametricIntersection& first, const bool firstA,
    const LC_ParametricIntersection& second, const bool secondA) const {
    if (!m_impl->axisContacts) { return {}; }
    const auto branch = m_impl->branches[0];
    if (!branch || (firstA ? first.branchA : first.branchB) != *branch ||
        (secondA ? second.branchA : second.branchB) != *branch) { return {}; }
    const auto validEvent = [this](const LC_ParametricIntersection& event) {
        return event.proofDomain.segmentA < m_impl->segments.size() &&
            event.proofDomain.segmentB < m_impl->segments.size() &&
            m_impl->segments[event.proofDomain.segmentA].branch == event.branchA &&
            m_impl->segments[event.proofDomain.segmentB].branch == event.branchB &&
            event.isolation.segmentA == event.proofDomain.segmentA &&
            event.isolation.segmentB == event.proofDomain.segmentB;
    };
    if (!validEvent(first) || !validEvent(second)) { return {}; }
    // Re-establish the named isolated roots from this provider's exact provenance.
    // A matching q labels a real ANGLE/parameter occurrence, not the common centre image.
    const auto a = certifyIntersection(first.proofDomain), b = certifyIntersection(second.proofDomain);
    if (a.outcome != LC_AnalyticIntersectionCertificate::Outcome::Unique ||
        b.outcome != LC_AnalyticIntersectionCertificate::Outcome::Unique ||
        a.proof != first.proof || b.proof != second.proof) { return {}; }
    const auto aa = m_impl->axisDomain(firstA ? a.isolationA : a.isolationB);
    const auto bb = m_impl->axisDomain(secondA ? b.isolationA : b.isolationB);
    if (aa.membership != Impl::AxisMembership::Present || bb.membership != Impl::AxisMembership::Present ||
        aa.index != bb.index) { return {}; }
    const auto common = intersect(aa.parameter, bb.parameter);
    if (!finiteInterval(common)) { return {}; }
    return {true, first.proofDomain, second.proofDomain, common};
}

class ReferenceProviderBuilder {
public:
    static ReferenceProviderPreparation run(const RS_EllipseData& basis, double signedDistance,
                                            WorkBudget& budget, bool withContacts, bool componentContacts = false);
private:
    static LC_IntersectionStatus prepareContacts(ReferenceProviders::Impl& provider,
        CriticalRefinementSession& own, CriticalRefinementSession* opposite, double delta, WorkBudget& budget);
};

ReferenceProviderPreparation ReferenceProviderBuilder::run(
    const RS_EllipseData& basis, const double signedDistance, WorkBudget& budget, const bool withContacts,
    const bool componentContacts) {
    ReferenceProviderPreparation out;
    const auto finish = [&]() {
        out.work = budget.consumed();
        if (budget.status() == BudgetStatus::LimitExceeded) {
            out.status = LC_IntersectionStatus::LimitExceeded;
            out.reason = ReferencePreparationReason::None;
        }
        return std::move(out);
    };
    if (budget.status() != BudgetStatus::Ok || !std::isfinite(signedDistance) || signedDistance == 0.0) {
        return finish();
    }
    const auto propagate = [&](const CriticalRetainedAnalysis& failed) {
        out.status = failed.status;
        out.reason = failed.reason == CriticalAnalysis::Reason::CollapsedCircle ?
            ReferencePreparationReason::CollapsedCircle :
            failed.reason == CriticalAnalysis::Reason::UnresolvedGeometry ?
                ReferencePreparationReason::UnresolvedGeometry : ReferencePreparationReason::None;
    };
    auto own = analyseCriticalRetained(basis, signedDistance, budget);
    if (own.status != LC_IntersectionStatus::Ok) { propagate(own); return finish(); }
    auto opposite = analyseCriticalRetained(basis, -signedDistance, budget);
    const bool collapseCandidate = signedDistance > 0.0 && budget.status() == BudgetStatus::Ok &&
        opposite.status == LC_IntersectionStatus::AmbiguousTopology &&
        opposite.reason == CriticalAnalysis::Reason::CollapsedCircle && !opposite.adapter && !opposite.refinement;
    if (opposite.status != LC_IntersectionStatus::Ok && !collapseCandidate) {
        propagate(opposite);
        return finish();
    }
    if (!budget.consumeVertices(3) || !budget.consumePairs(1)) { return finish(); }
    const Rational mx = exactDouble(basis.majorP.x);
    const Rational my = exactDouble(basis.majorP.y);
    const Rational d = exactDouble(signedDistance);
    if (!own.adapter || !own.refinement || (!collapseCandidate && (!opposite.adapter || !opposite.refinement)) ||
        (collapseCandidate && !(basis.ratio == 1.0 && mx * mx + my * my == d * d)) ||
        (!collapseCandidate && own.adapter->frame().scale() != opposite.adapter->frame().scale())) {
        out.status = LC_IntersectionStatus::InvalidInput;
        return finish();
    }
    const auto disposition = collapseCandidate ? OppositeDisposition::ExcludedCollapsedCircle :
        OppositeDisposition::Present;
    auto impl = std::make_unique<ReferenceProviders::Impl>(std::move(own.adapter), std::move(opposite.adapter), disposition);
    if (componentContacts) {
        if (!budget.consumePairs(1)) { return finish(); }
        if (signedDistance < 0 && basis.ratio < 1 && mx * mx + my * my == d * d) {
            impl->axisContacts = impl->adapters[0]->m_provenance;
        }
    }
    const std::size_t capCount = impl->adapters[0]->frame().sourceSweep().whole ? 0 : 2;
    std::size_t size = impl->adapters[0]->segments().size();
    const std::size_t oppositeSize = impl->adapters[1] ? impl->adapters[1]->segments().size() : 0;
    if (oppositeSize > std::numeric_limits<std::size_t>::max() - size ||
        capCount * 2 > std::numeric_limits<std::size_t>::max() - size - oppositeSize) {
        budget.latchLimitExceeded();
        return finish();
    }
    size += oppositeSize + capCount * 2;
    if (size > impl->segments.max_size() || size > impl->maps.max_size()) {
        budget.latchLimitExceeded();
        return finish();
    }
    out.status = LC_IntersectionStatus::AmbiguousTopology;
    out.reason = ReferencePreparationReason::UnresolvedGeometry;
    for (std::size_t cap = 0; cap < capCount; ++cap) {
        if (!budget.consumeVertices(1) || !budget.consumePairs(1)) { return finish(); }
        impl->cutters[cap].emplace();
        if (!impl->cutters[cap]->prepare(impl->adapters[0]->frame(), signedDistance, cap == 0)) { return finish(); }
        for (const double t : {0.0, 0.5, 1.0}) {
            if (!budget.consumePairs(1)) { return finish(); }
            LC_CurveJetBounds bounds;
            LC_CurveJet seed;
            if (!impl->cutters[cap]->bound(t, t, bounds) || !jetFromBounds(bounds, seed)) { return finish(); }
        }
    }
    if (!budget.consumeVertices(size)) { return finish(); }
    impl->segments.reserve(size);
    impl->maps.reserve(size);
    for (std::size_t part = 0; part < 4; ++part) {
        const bool present = part < 2 ? bool(impl->adapters[part]) : bool(impl->cutters[part - 2]);
        if (!present) { continue; }
        const std::size_t branch = impl->count++;
        impl->branches[part] = branch;
        const std::size_t count = part < 2 ? impl->adapters[part]->segments().size() : 2;
        double previous = 0.0;
        if (count == 0) { return finish(); }
        for (std::size_t local = 0; local < count; ++local) {
            if (!budget.consumePairs(1)) { return finish(); }
            auto segment = impl->localSegment(part, local);
            if (segment.branch != 0 || !std::isfinite(segment.t0) || !std::isfinite(segment.t1) ||
                segment.t0 != previous || !(segment.t0 < segment.t1) ||
                (part < 2 && impl->adapters[part]->branchCount() != 1)) { return finish(); }
            previous = segment.t1;
            segment.branch = branch;
            impl->segments.push_back(segment);
            impl->maps.push_back({part, local});
        }
        const double expectedEnd = part < 2 ? impl->adapters[part]->frame().parameterEnd() : 1.0;
        if (previous != expectedEnd) { return finish(); }
        if (part < 2) {
            const auto next = impl->adapters[part]->next(0);
            if (next != (impl->adapters[part]->frame().sourceSweep().whole ? 0 : -1)) { return finish(); }
        }
    }
    if (withContacts) {
        out.status = prepareContacts(*impl, *own.refinement, opposite.refinement.get(), signedDistance, budget);
        if (out.status != LC_IntersectionStatus::Ok) {
            if (out.status != LC_IntersectionStatus::AmbiguousTopology) { out.reason = ReferencePreparationReason::None; }
            return finish();
        }
    }
    out.provider.reset(new ReferenceProviders{std::move(impl)});
    out.ownCritical = std::move(own.refinement);
    out.oppositeCritical = std::move(opposite.refinement);
    out.status = LC_IntersectionStatus::Ok;
    out.reason = ReferencePreparationReason::None;
    return finish();
}

LC_IntersectionStatus ReferenceProviderBuilder::prepareContacts(
    ReferenceProviders::Impl& provider, CriticalRefinementSession& own, CriticalRefinementSession* opposite,
    const double delta, WorkBudget& budget) {
    if (!budget.consumeVertices(1) || !budget.consumePairs(1)) { return LC_IntersectionStatus::LimitExceeded; }
    const std::array<CriticalRefinementSession*, 2> sessions{{&own, opposite}};
    for (std::size_t part = 0; part < 2; ++part) {
        const auto& adapter = provider.adapters[part];
        if (adapter && (!sessions[part] || !adapter->m_provenance ||
            adapter->criticalOccurrences().size() > 4 ||
            adapter->criticalOccurrences().size() != sessions[part]->occurrenceCount())) {
            return LC_IntersectionStatus::InvalidInput;
        }
    }
    auto ledger = std::make_unique<ReferenceProviders::Impl::ContactLedger>();
    for (std::size_t part = 0; part < 2; ++part) {
        if (!provider.adapters[part]) { continue; }
        const auto& adapter = *provider.adapters[part];
        const auto& source = adapter.frame();
        const double duration = source.parameterEnd();
        const bool outer = part == 0 ? delta > 0.0 : delta < 0.0;
        for (std::size_t cap = 0; cap < 2; ++cap) {
            if (!provider.cutters[cap]) { continue; }
            if (!budget.consumeVertices(1)) { return LC_IntersectionStatus::LimitExceeded; }
            auto& record = ledger->records[part][cap];
            record.emplace();
            if (!budget.consumePairs(1)) { return LC_IntersectionStatus::LimitExceeded; }
            const bool start = cap == 0;
            record->sourceEnd = start ? 0.0 : duration;
            record->capEnd = outer ? 0.0 : 1.0;
            if (outer) {
                // Strict support equality forces the same source endpoint and its outward normal.
                record->parameter = LC_Interval::hull(0.0, duration);
                continue;
            }
            const bool intermediate = adapter.regime() == CriticalRegime::IntermediateThresholds;
            int sign = intermediate ? adapter.m_provenance->factorSign(record->sourceEnd) :
                adapter.regime() == CriticalRegime::PositiveFactor ||
                adapter.regime() == CriticalRegime::MinimumStationary ? 1 : -1;
            const std::size_t count = adapter.criticalOccurrences().size();
            double boundary = start ? duration : 0.0;
            if (intermediate && count != 0) {
                // Factory inventory is complete and ordered; refinement preserves its identities/order.
                auto& session = *sessions[part];
                const auto handle = session.occurrenceHandle(start ? 0 : count - 1);
                auto snapshot = session.snapshot(handle);
                if (!snapshot) { return LC_IntersectionStatus::InvalidInput; }
                while (true) {
                    const auto parameter = snapshot->occurrence.parameter;
                    if (!finiteInterval(parameter) || parameter.lo() < 0.0 || parameter.hi() > duration ||
                        snapshot->occurrence.kind != CriticalKind::SimpleCusp) {
                        return LC_IntersectionStatus::InvalidInput;
                    }
                    boundary = start ? parameter.lo() : parameter.hi();
                    if (start ? boundary > 0.0 : boundary < duration) { break; }
                    const auto depth = snapshot->admittedRounds;
                    const auto contracted = session.contract(handle, budget); // Directly spent, never re-debit.
                    if (contracted.workSource != CriticalWorkSource::CurrentBudget) {
                        return LC_IntersectionStatus::InvalidInput;
                    }
                    if (contracted.status != LC_IntersectionStatus::Ok) { return contracted.status; }
                    if (!contracted.snapshot || contracted.snapshot->admittedRounds <= depth) {
                        return LC_IntersectionStatus::InvalidInput;
                    }
                    if (!budget.consumePairs(1)) { return LC_IntersectionStatus::LimitExceeded; }
                    snapshot = session.snapshot(handle);
                    if (!snapshot) { return LC_IntersectionStatus::InvalidInput; }
                }
            }
            double from = 0.0, to = duration;
            if (intermediate) {
                if (start) { to = 0.5 * boundary; }
                else { from = 0.5 * boundary + 0.5 * duration; }
                if (!(0.0 <= from && from < to && to <= duration) ||
                    (start ? !(to < boundary) : !(boundary < from))) {
                    return LC_IntersectionStatus::AmbiguousTopology;
                }
                if (sign == 2) {
                    // A certified point in the SAME complete zero-free gap transfers its sign by continuity.
                    const double inside = 0.5 * from + 0.5 * to;
                    if (!(from < inside && inside < to)) { return LC_IntersectionStatus::AmbiguousTopology; }
                    sign = adapter.m_provenance->factorSign(inside);
                    if (sign == 2) { return LC_IntersectionStatus::AmbiguousTopology; }
                }
            }
            LC_CurveJetBounds endpoint;
            if (!source.boundSourceJet(record->sourceEnd, record->sourceEnd, endpoint)) {
                return LC_IntersectionStatus::AmbiguousTopology;
            }
            for (unsigned depth = 0;; ++depth) {
                if (!budget.consumeVertices(1) || !budget.consumePairs(1)) { return LC_IntersectionStatus::LimitExceeded; }
                if (!budget.checkDepth(depth)) { return LC_IntersectionStatus::LimitExceeded; }
                LC_CurveJetBounds span;
                if (!source.boundSourceJet(from, to, span)) { return LC_IntersectionStatus::AmbiguousTopology; }
                const LC_Interval hPrime = span.speedSquaredProduct + span.ddx * (span.x - endpoint.x) +
                    span.ddy * (span.y - endpoint.y);
                if (!finiteInterval(hPrime)) { return LC_IntersectionStatus::AmbiguousTopology; }
                if (hPrime.isPositive()) {
                    // h(endpoint)=G(endpoint)=0. h'>0 and constant F sign (isolated stationary zeros allowed)
                    // imply strictly one-sided G'=2*F*h, hence only the known full-circle/cap contact.
                    record->parameter = LC_Interval::hull(from, to);
                    break;
                }
                if (depth >= budget.depthLimit()) {
                    budget.latchLimitExceeded();
                    return LC_IntersectionStatus::LimitExceeded;
                }
                const double mid = 0.5 * from + 0.5 * to;
                if (!(from < mid && mid < to)) { return LC_IntersectionStatus::AmbiguousTopology; }
                if (start) { to = mid; }
                else { from = mid; }
            }
        }
    }
    provider.contacts = std::move(ledger);
    return LC_IntersectionStatus::Ok;
}

ReferenceProviderPreparation prepareReferenceProviders(const RS_EllipseData& basis, const double signedDistance,
                                                       WorkBudget& budget) {
    return ReferenceProviderBuilder::run(basis, signedDistance, budget, false);
}

ReferenceProviderPreparation prepareReferenceContactProviders(const RS_EllipseData& basis, const double signedDistance,
                                                              WorkBudget& budget) {
    return ReferenceProviderBuilder::run(basis, signedDistance, budget, true);
}

struct ReferenceEventAnalysis::Impl {
    // providerView is valid exactly as long as core, its sole owner. No caller state is borrowed.
    std::unique_ptr<LC_CertifiedIntersectionSession> core;
    const ReferenceProviders* providerView = nullptr;
    double localTolerance = 0.0;
    std::unique_ptr<CriticalRefinementSession> ownCritical, oppositeCritical;
    std::vector<ReferenceOccurrence> occurrences;
    std::vector<ReferenceOccurrenceId> ownOccurrences;
    std::vector<ReferenceNode> nodes;
    std::vector<ReferencePairEvent> pairs;
    std::vector<ReferenceCriticalEvent> criticals;
    std::vector<ReferenceFreeEnd> freeEnds;
    std::optional<ReferenceSeam> seam;
};

struct ReferenceComponentAnalysis::Impl {
    std::unique_ptr<const ReferenceEventAnalysis> events;
    std::vector<ReferenceFringe> fringes;
    std::vector<ReferenceSpan> spans;
    std::vector<ReferenceEdge> edges;
    std::vector<ReferenceIncidence> incidence;
    std::vector<ReferenceComponent> components;
};

class ReferenceEventBuilder {
public:
    static ReferenceComponentResult components(const RS_EllipseData& basis, const double distance,
                                                const double tolerance, WorkBudget& budget) {
        ReferenceComponentResult out;
        const auto events = run(basis, distance, tolerance, budget, &out);
        out.status = events.status;
        out.eventReason = events.reason;
        out.work = budget.consumed();
        if (!out.analysis && out.reason == ReferenceComponentReason::None &&
            out.status != LC_IntersectionStatus::LimitExceeded) {
            out.reason = events.reason == ReferenceEventReason::ProofContradiction
                ? ReferenceComponentReason::ProofContradiction : ReferenceComponentReason::EventRefusal;
        }
        return out;
    }
    static ReferenceEventResult run(const RS_EllipseData& basis, const double distance,
                                    const double tolerance, WorkBudget& budget,
                                    ReferenceComponentResult* components = nullptr) {
        ReferenceEventResult out;
        const auto finish = [&]() { out.work = budget.consumed(); return std::move(out); };
        if (budget.status() != BudgetStatus::Ok) {
            out.status = budget.status() == BudgetStatus::LimitExceeded ? LC_IntersectionStatus::LimitExceeded
                                                                       : LC_IntersectionStatus::InvalidInput;
            return finish();
        }
        if (!(tolerance > 0.0) || !std::isfinite(tolerance) || !std::isfinite(distance) || distance == 0.0) {
            return finish();
        }
        const auto sourceSweep = sweep(basis);
        if (components && distance < 0.0 && basis.ratio > 0.0 && basis.ratio < 1.0 &&
            frame(basis) && sourceSweep && sourceSweep->whole) {
            if (!budget.consumeVertices(1) || !budget.consumePairs(1)) {
                out.status = LC_IntersectionStatus::LimitExceeded;
                return finish();
            }
            const Rational x = exactDouble(basis.majorP.x), y = exactDouble(basis.majorP.y);
            const Rational ratio = exactDouble(basis.ratio), d = exactDouble(distance);
            if (d * d == (x * x + y * y) * ratio * ratio) {
                out.status = LC_IntersectionStatus::AmbiguousTopology;
                components->reason = ReferenceComponentReason::UnresolvedVisibility;
                return finish();
            }
        }
        auto prepared = components ? ReferenceProviderBuilder::run(basis, distance, budget, true, true) :
            prepareReferenceContactProviders(basis, distance, budget);
        if (prepared.status != LC_IntersectionStatus::Ok) {
            out.status = prepared.status;
            if (prepared.status != LC_IntersectionStatus::LimitExceeded) {
                out.reason = prepared.reason == ReferencePreparationReason::CollapsedCircle
                    ? ReferenceEventReason::CollapsedCircle : ReferenceEventReason::UnresolvedGeometry;
            }
            return finish();
        }
        if (budget.remainingPairs() == 0 || budget.remainingVertices() == 0) {
            budget.latchLimitExceeded();
            out.status = LC_IntersectionStatus::LimitExceeded;
            return finish();
        }
        const auto* view = prepared.provider.get();
        if (!budget.consumePairs(1)) {
            out.status = LC_IntersectionStatus::LimitExceeded;
            return finish();
        }
        const double localTolerance = (exactDouble(tolerance) / exactDouble(view->frame().scale())).convert_to<double>();
        if (!(localTolerance > 0.0) || !std::isfinite(localTolerance)) {
            out.status = LC_IntersectionStatus::AmbiguousTopology;
            out.reason = ReferenceEventReason::NodeAccuracy;
            return finish();
        }
        LC_IntersectionOptions options;
        options.tolerance = localTolerance;
        options.strict = true;
        options.anchoredBranches = 1;
        options.maxDepth = options.maxLeafDepth = budget.depthLimit();
        options.maxBoxPairs = budget.remainingPairs();
        options.maxLeaves = options.maxGeneratedItems = budget.remainingVertices();
        options.analyticOccurrenceIdentity = components && bool(view->m_impl->axisContacts);
        auto query = findCertifiedIntersections(std::move(prepared.provider), options);
        const bool items = budget.consumeVertices(query.work.generatedItems);
        const bool pairs = budget.consumePairs(query.work.totalBoxPairs);
        const bool depth = budget.checkDepth(query.work.deepest());
        if (!items || !pairs || !depth || query.status == LC_IntersectionStatus::LimitExceeded) {
            budget.latchLimitExceeded();
            out.status = LC_IntersectionStatus::LimitExceeded;
            return finish();
        }
        if (query.status != LC_IntersectionStatus::Ok || !query.session) {
            out.status = query.status;
            out.reason = query.status == LC_IntersectionStatus::InvalidInput
                ? ReferenceEventReason::ProofContradiction : ReferenceEventReason::UnresolvedGeometry;
            return finish();
        }
        if (!budget.consumeVertices(1) || !budget.consumePairs(1)) {
            out.status = LC_IntersectionStatus::LimitExceeded;
            return finish();
        }
        auto data = std::make_unique<ReferenceEventAnalysis::Impl>();
        data->core = std::move(query.session);
        data->providerView = view;
        data->localTolerance = options.tolerance;
        data->ownCritical = std::move(prepared.ownCritical);
        data->oppositeCritical = std::move(prepared.oppositeCritical);
        ReferenceEventBuilder builder{budget, tolerance, std::move(data)};
        if (builder.prepare() && builder.order() && builder.accuracy() &&
            (!components || builder.fringes()) && builder.publish()) {
            if (!components) {
                out.analysis.reset(new ReferenceEventAnalysis{std::move(builder.m_data)});
            } else if (builder.assemble(basis, distance)) {
                builder.m_components->events.reset(new ReferenceEventAnalysis{std::move(builder.m_data)});
                components->analysis.reset(new ReferenceComponentAnalysis{std::move(builder.m_components)});
            }
        }
        if (components) { components->reason = builder.m_componentReason; }
        out.status = builder.m_status;
        out.reason = builder.m_reason;
        return finish();
    }

private:
    static constexpr std::size_t none = std::size_t(-1);
    enum class Origin { Pair, Critical, Boundary };
    struct Witness {
        ReferenceRole role{};
        Origin origin{};
        std::size_t source = none;
        LC_Interval parameter, occurrenceParameter;
        std::size_t node = none, parent = none, head = none, tail = none;
        std::size_t nextOccurrence = none, nextNode = none, publicId = none;
        unsigned rank = 0;
        CriticalOccurrenceHandle criticalHandle;
        CriticalOccurrence critical{};
        int axis = 99;
    };
    struct Node {
        LC_Interval x, y;
        std::size_t parent = none, minimum = none, head = none, tail = none, publicId = none;
        unsigned rank = 0;
        ReferenceNode published;
    };
    struct Root {
        LC_CertifiedRootHandle handle;
        LC_CertifiedRootSnapshot snapshot;
        bool blocked = false;
    };
    ReferenceEventBuilder(WorkBudget& budget, const double tolerance,
                          std::unique_ptr<ReferenceEventAnalysis::Impl> data)
        : m_budget{budget}, m_errorSquared{exactDouble(tolerance) * exactDouble(tolerance) / 256},
          m_data{std::move(data)} {}

    bool fail(const LC_IntersectionStatus status, const ReferenceEventReason reason) {
        m_status = status;
        m_reason = reason;
        return false;
    }
    bool limit() {
        m_budget.latchLimitExceeded();
        return fail(LC_IntersectionStatus::LimitExceeded, ReferenceEventReason::None);
    }
    bool pair() { return m_budget.consumePairs(1) || limit(); }
    bool vertices(const std::size_t n) { return m_budget.consumeVertices(n) || limit(); }
    bool contradiction() { return fail(LC_IntersectionStatus::InvalidInput, ReferenceEventReason::ProofContradiction); }
    bool good() const { return m_status == LC_IntersectionStatus::Ok; }
    template<typename T> bool room(const std::vector<T>& v) { return v.size() < v.max_size() || limit(); }
    static double middle(const LC_Interval& value) {
        if (value.lo() == value.hi()) { return value.lo(); }
        const double difference = value.hi() - value.lo();
        const double result = std::isfinite(difference) ? value.lo() + difference / 2
                                                       : value.lo() / 2 + value.hi() / 2;
        return std::max(value.lo(), std::min(value.hi(), result));
    }
    const ReferenceProviders& provider() const { return *m_data->providerView; }
    std::optional<ReferenceRole> role(const std::size_t branch) const {
        for (const auto candidate : {ReferenceRole::Own, ReferenceRole::Opposite,
                                     ReferenceRole::StartCutter, ReferenceRole::EndCutter}) {
            if (provider().branchFor(candidate) == branch) { return candidate; }
        }
        return {};
    }
    std::size_t occurrence(std::size_t index) {
        while (m_witnesses[index].parent != index) {
            if (!pair()) { return none; }
            index = m_witnesses[index].parent;
        }
        return index;
    }
    std::size_t node(std::size_t index) {
        while (m_nodes[index].parent != index) {
            if (!pair()) { return none; }
            index = m_nodes[index].parent;
        }
        return index;
    }
    std::size_t addNode(const LC_Interval& x, const LC_Interval& y) {
        if (!room(m_nodes) || !vertices(1)) { return none; }
        if (!finiteInterval(x) || !finiteInterval(y)) { contradiction(); return none; }
        const std::size_t index = m_nodes.size();
        Node record;
        record.x = x; record.y = y;
        record.parent = record.minimum = index;
        m_nodes.push_back(record);
        return index;
    }
    bool addWitness(Witness witness, const std::size_t owner) {
        if (owner == none || !room(m_witnesses) || !vertices(1) || !pair()) { return false; }
        if (!finiteInterval(witness.parameter)) { return contradiction(); }
        const std::size_t index = m_witnesses.size();
        witness.parent = witness.head = witness.tail = index;
        witness.node = owner;
        witness.occurrenceParameter = witness.parameter;
        m_witnesses.push_back(witness);
        auto& n = m_nodes[owner];
        if (n.head == none) { n.head = index; }
        else { m_witnesses[n.tail].nextNode = index; }
        n.tail = index;
        return true;
    }
    bool prepare() {
        const std::size_t count = m_data->core->rootCount();
        if (!m_data->ownCritical) { return contradiction(); }
        if (count > (m_witnesses.max_size() - 6) / 2) { return limit(); }
        for (std::size_t i = 0; i < count; ++i) {
            if (!room(m_roots) || !vertices(1) || !pair()) { return false; }
            const auto handle = m_data->core->rootHandle(i);
            const auto snapshot = m_data->core->snapshot(handle);
            if (!snapshot) { return contradiction(); }
            m_roots.push_back({handle, *snapshot, false});
            const auto a = role(snapshot->event.branchA), b = role(snapshot->event.branchB);
            if (!a || !b || (*a != ReferenceRole::Own && *b != ReferenceRole::Own)) { return contradiction(); }
            const auto n = addNode(snapshot->imageX, snapshot->imageY);
            Witness wa, wb;
            wa.role = *a; wb.role = *b;
            wa.origin = wb.origin = Origin::Pair;
            wa.source = wb.source = i;
            wa.parameter = snapshot->event.isolation.parameterA;
            wb.parameter = snapshot->event.isolation.parameterB;
            if (provider().m_impl->axisContacts) {
                if (!pair()) { return false; }
                // Reissue the qualified provider proof on the original domain, never infer identity
                // merely from overlapping public isolations or a copied analytic proof flag.
                const auto certificate = provider().certifyIntersection(snapshot->event.proofDomain);
                if (certificate.outcome == LC_AnalyticIntersectionCertificate::Outcome::Unique) {
                    LC_Interval unused;
                    if (wa.role == ReferenceRole::Own) {
                        wa.axis = provider().m_impl->axisIn(certificate.isolationA, unused);
                    }
                    if (wb.role == ReferenceRole::Own) {
                        wb.axis = provider().m_impl->axisIn(certificate.isolationB, unused);
                    }
                }
            }
            if (!addWitness(wa, n) || !addWitness(wb, n)) { return false; }
        }
        m_criticalStart = m_witnesses.size();
        m_criticalCount = m_data->ownCritical->occurrenceCount();
        if (m_criticalCount > 4) { return contradiction(); }
        if (m_criticalCount && (!vertices(1) || !pair())) { return false; }
        for (std::size_t i = 0; i < m_criticalCount; ++i) {
            if (!pair()) { return false; }
            const auto handle = m_data->ownCritical->occurrenceHandle(i);
            const auto snapshot = m_data->ownCritical->snapshot(handle);
            if (!snapshot) { return contradiction(); }
            Witness w;
            w.role = ReferenceRole::Own; w.origin = Origin::Critical; w.source = i;
            w.criticalHandle = handle; w.critical = snapshot->occurrence;
            w.parameter = criticalParameter(*snapshot);
            if (!addWitness(w, addNode(snapshot->imageX, snapshot->imageY))) { return false; }
        }
        m_boundaryStart = m_witnesses.size();
        for (const double t : {0.0, provider().frame().parameterEnd()}) {
            if (!pair()) { return false; }
            LC_CurveJetBounds bounds;
            if (!provider().frame().boundOffsetJet(t, t, bounds)) {
                return fail(LC_IntersectionStatus::AmbiguousTopology, ReferenceEventReason::UnresolvedGeometry);
            }
            Witness w;
            w.role = ReferenceRole::Own; w.origin = Origin::Boundary;
            w.parameter = LC_Interval::point(t);
            if (provider().m_impl->axisContacts) {
                LC_Interval unused;
                w.axis = provider().m_impl->axisIn(w.parameter, unused);
            }
            if (!addWitness(w, addNode(bounds.x, bounds.y))) { return false; }
        }
        if (provider().frame().sourceSweep().whole &&
            !joinNodes(m_witnesses[m_boundaryStart].node, m_witnesses[m_boundaryStart + 1].node)) { return false; }
        if (m_witnesses.size() > m_order.max_size() || !vertices(m_witnesses.size())) { return limit(); }
        m_order.reserve(m_witnesses.size());
        return true;
    }
    LC_Interval criticalParameter(const CriticalOccurrenceSnapshot& snapshot) const {
        if (snapshot.occurrence.atStart) { return LC_Interval::point(0); }
        if (snapshot.occurrence.atEnd) { return LC_Interval::point(provider().frame().parameterEnd()); }
        return snapshot.occurrence.parameter;
    }
    bool joinNodes(std::size_t a, std::size_t b) {
        a = node(a); if (a == none) { return false; }
        b = node(b); if (b == none || !pair()) { return false; }
        if (a == b) { return true; }
        if (!pair()) { return false; }
        const auto x = intersect(m_nodes[a].x, m_nodes[b].x);
        const auto y = intersect(m_nodes[a].y, m_nodes[b].y);
        if (!finiteInterval(x) || !finiteInterval(y)) { return contradiction(); }
        if (m_nodes[a].rank < m_nodes[b].rank) { std::swap(a, b); }
        auto& into = m_nodes[a];
        auto& from = m_nodes[b];
        into.x = x; into.y = y;
        into.minimum = std::min(into.minimum, from.minimum);
        if (into.rank == from.rank) { ++into.rank; }
        from.parent = a;
        m_witnesses[into.tail].nextNode = from.head;
        into.tail = from.tail;
        return true;
    }
    bool joinOccurrences(std::size_t a, std::size_t b) {
        if (!pair()) { return false; }
        const auto parameter = intersect(m_witnesses[a].occurrenceParameter, m_witnesses[b].occurrenceParameter);
        if (!finiteInterval(parameter)) { return contradiction(); }
        if (!joinNodes(m_witnesses[a].node, m_witnesses[b].node)) { return false; }
        if (m_witnesses[a].rank < m_witnesses[b].rank) { std::swap(a, b); }
        auto& into = m_witnesses[a];
        auto& from = m_witnesses[b];
        into.occurrenceParameter = parameter;
        if (into.axis == 99) { into.axis = from.axis; }
        if (into.rank == from.rank) { ++into.rank; }
        from.parent = a;
        m_witnesses[into.tail].nextOccurrence = from.head;
        into.tail = from.tail;
        return true;
    }
    bool refresh(const std::size_t index, const LC_Interval& parameter,
                 const LC_Interval& x, const LC_Interval& y) {
        if (!pair()) { return false; }
        const auto o = occurrence(index); if (o == none) { return false; }
        const auto n = node(m_witnesses[index].node); if (n == none || !pair()) { return false; }
        const auto p = intersect(m_witnesses[o].occurrenceParameter, parameter);
        const auto ix = intersect(m_nodes[n].x, x), iy = intersect(m_nodes[n].y, y);
        if (!finiteInterval(p) || !finiteInterval(ix) || !finiteInterval(iy)) { return contradiction(); }
        m_witnesses[index].parameter = parameter;
        m_witnesses[o].occurrenceParameter = p;
        m_nodes[n].x = ix; m_nodes[n].y = iy;
        return true;
    }
    bool debit(const LC_IntersectionWorkStats& work) {
        const bool v = m_budget.consumeVertices(work.generatedItems);
        const bool p = m_budget.consumePairs(work.totalBoxPairs);
        const bool d = m_budget.checkDepth(work.deepest());
        return (v && p && d) || limit();
    }
    bool refineWitness(const std::size_t index) {
        auto& w = m_witnesses[index];
        if (w.origin == Origin::Boundary) { return false; }
        if (w.origin == Origin::Pair) {
            auto& root = m_roots[w.source];
            if (root.blocked) { return false; }
            LC_CertifiedContractionAllowance allowance;
            allowance.remainingItems = m_budget.remainingVertices();
            allowance.remainingPairs = m_budget.remainingPairs();
            allowance.absoluteRootDepth = m_budget.depthLimit();
            const auto result = m_data->core->contract(root.handle, allowance);
            if (!debit(result.work)) { return false; }
            if (result.status == LC_IntersectionStatus::LimitExceeded) { return limit(); }
            if (result.status == LC_IntersectionStatus::InvalidInput) { return contradiction(); }
            if (result.status != LC_IntersectionStatus::Ok) { root.blocked = true; return false; }
            if (!result.snapshot) { return contradiction(); }
            root.snapshot = *result.snapshot;
            const auto first = 2 * w.source;
            return refresh(first, root.snapshot.event.isolation.parameterA, root.snapshot.imageX, root.snapshot.imageY) &&
                refresh(first + 1, root.snapshot.event.isolation.parameterB, root.snapshot.imageX, root.snapshot.imageY);
        }
        if (m_criticalBlocked[w.source]) { return false; }
        const auto result = m_data->ownCritical->contract(w.criticalHandle, m_budget);
        // This session directly charged m_budget; neither result ledger is an additional debit.
        if (result.workSource != CriticalWorkSource::CurrentBudget) { return contradiction(); }
        if (result.status == LC_IntersectionStatus::LimitExceeded) { return limit(); }
        if (result.status == LC_IntersectionStatus::InvalidInput) { return contradiction(); }
        if (result.status != LC_IntersectionStatus::Ok) { m_criticalBlocked[w.source] = true; return false; }
        for (std::size_t i = 0; i < m_criticalCount; ++i) {
            if (!pair()) { return false; }
            const auto wi = m_criticalStart + i;
            const auto snapshot = m_data->ownCritical->snapshot(m_witnesses[wi].criticalHandle);
            if (!snapshot) { return contradiction(); }
            if (!refresh(wi, criticalParameter(*snapshot), snapshot->imageX, snapshot->imageY)) { return false; }
        }
        return true;
    }
    bool refineOccurrence(const std::size_t index) {
        for (auto i = m_witnesses[index].head; i != none; i = m_witnesses[i].nextOccurrence) {
            if (!pair()) { return false; }
            if (refineWitness(i)) { return true; }
            if (!good()) { return false; }
        }
        return false;
    }
    bool order() {
        // Insertion comparisons are proofs, not mutable-key sort predicates. Narrowing preserves proved order.
        for (std::size_t i = 0; i < m_witnesses.size(); ++i) {
            std::size_t position = 0;
            bool merged = false;
            while (position < m_order.size()) {
                const auto a = occurrence(i); if (a == none) { return false; }
                const auto b = occurrence(m_order[position].value); if (b == none || !pair()) { return false; }
                if (a == b) { merged = true; break; }
                const auto ar = m_witnesses[a].role, br = m_witnesses[b].role;
                if (ar != br) {
                    if (ar < br) { break; }
                    ++position;
                    continue;
                }
                const auto pa = m_witnesses[a].occurrenceParameter, pb = m_witnesses[b].occurrenceParameter;
                if (pa.hi() < pb.lo()) { break; }
                if (pb.hi() < pa.lo()) { ++position; continue; }
                if (!pair()) { return false; } // Identity examination is separate from the order comparison.
                if ((pa.lo() == pa.hi() && pb.lo() == pb.hi() && pa.lo() == pb.lo()) ||
                    (ar == ReferenceRole::Own && m_witnesses[a].axis != 99 &&
                     m_witnesses[a].axis == m_witnesses[b].axis)) {
                    if (!joinOccurrences(a, b)) { return false; }
                    merged = true;
                    break;
                }
                if (refineOccurrence(a)) { continue; }
                if (!good()) { return false; }
                if (refineOccurrence(b)) { continue; }
                if (!good()) { return false; }
                return fail(LC_IntersectionStatus::AmbiguousTopology, ReferenceEventReason::UnresolvedIdentity);
            }
            if (!merged) {
                m_order.push_back({i}); // Storage was admitted before reserve; moves are separately examined.
                for (std::size_t j = m_order.size() - 1; j > position; --j) {
                    if (!pair()) { return false; }
                    m_order[j] = m_order[j - 1];
                }
                m_order[position] = {i};
            }
        }
        return true;
    }
    bool refineNode(const std::size_t index) {
        for (auto i = m_nodes[index].head; i != none; i = m_witnesses[i].nextNode) {
            if (!pair()) { return false; }
            if (refineWitness(i)) { return true; }
            if (!good()) { return false; }
        }
        return false;
    }
    bool accurate(const std::size_t index) {
        auto& record = m_nodes[index];
        LC_Interval x, y;
        if (!provider().frame().pointToWorld(record.x, record.y, x, y)) { return false; }
        if (!finiteInterval(x) || !finiteInterval(y)) { return false; }
        const RS_Vector point{middle(x), middle(y)};
        if (!isFinite2D(point)) { return false; }
        // Finiteness precedes exactDouble. This includes rounding of the actual published double node.
        const auto maximumError = [](const LC_Interval& interval, const double value) -> Rational {
            const Rational p = exactDouble(value);
            const Rational lo = p - exactDouble(interval.lo()), hi = exactDouble(interval.hi()) - p;
            return std::max(lo, hi);
        };
        const Rational ex = maximumError(x, point.x), ey = maximumError(y, point.y);
        if (ex * ex + ey * ey > m_errorSquared) { return false; }
        record.published = {point, record.x, record.y, x, y};
        return true;
    }
    bool accuracy() {
        for (std::size_t i = 0; i < m_nodes.size(); ++i) {
            if (!pair()) { return false; }
            if (m_nodes[i].parent != i) { continue; }
            for (;;) {
                if (!pair()) { return false; }
                if (accurate(i)) { break; }
                if (refineNode(i)) { continue; }
                if (!good()) { return false; }
                return fail(LC_IntersectionStatus::AmbiguousTopology, ReferenceEventReason::NodeAccuracy);
            }
        }
        return true;
    }
    bool componentFailure(const ReferenceComponentReason reason) {
        m_componentReason = reason;
        return fail(LC_IntersectionStatus::AmbiguousTopology, ReferenceEventReason::None);
    }
    bool errorBound(const LC_Interval& x, const LC_Interval& y, const RS_Vector& point) const {
        if (!finiteInterval(x) || !finiteInterval(y) || !isFinite2D(point)) { return false; }
        const Rational px = exactDouble(point.x), py = exactDouble(point.y);
        const Rational dx = std::max(Rational{px - exactDouble(x.lo())}, Rational{exactDouble(x.hi()) - px});
        const Rational dy = std::max(Rational{py - exactDouble(y.lo())}, Rational{exactDouble(y.hi()) - py});
        return dx * dx + dy * dy <= m_errorSquared;
    }
    bool fringeBounds(const std::size_t witness, LC_CurveJetBounds& bounds) {
        if (!pair()) { return false; }
        const auto o = occurrence(witness); if (o == none) { return false; }
        const auto p = m_witnesses[o].occurrenceParameter;
        return provider().frame().boundOffsetJet(p.lo(), p.hi(), bounds) ||
            componentFailure(ReferenceComponentReason::UnresolvedFringe);
    }
    bool fringes() {
        if (!vertices(1) || !pair()) { return false; }
        m_components = std::make_unique<ReferenceComponentAnalysis::Impl>();
        // A root-image intersection is not a bound on a curve's full root enclosure.
        // Bound the UNION of all incident own fringes, then choose one rounded node for that union.
        for (std::size_t n = 0; n < m_nodes.size(); ++n) {
            if (!pair()) { return false; }
            if (m_nodes[n].parent != n) { continue; }
            for (;;) {
                LC_Interval x, y;
                for (auto i = m_nodes[n].head; i != none; i = m_witnesses[i].nextNode) {
                    if (!pair()) { return false; }
                    if (m_witnesses[i].role != ReferenceRole::Own) { continue; }
                    LC_CurveJetBounds b;
                    if (!fringeBounds(i, b) || !pair()) { return false; }
                    x = x.isValid() ? LC_Interval::hull(std::min(x.lo(), b.x.lo()), std::max(x.hi(), b.x.hi())) : b.x;
                    y = y.isValid() ? LC_Interval::hull(std::min(y.lo(), b.y.lo()), std::max(y.hi(), b.y.hi())) : b.y;
                }
                if (!pair()) { return false; }
                LC_Interval wx, wy;
                if (finiteInterval(x) && finiteInterval(y) && provider().frame().pointToWorld(x, y, wx, wy) &&
                    errorBound(wx, wy, RS_Vector{middle(wx), middle(wy)})) {
                    auto& target = m_nodes[n];
                    target.x = intersect(target.x, x); target.y = intersect(target.y, y);
                    if (!finiteInterval(target.x) || !finiteInterval(target.y)) { return contradiction(); }
                    target.published = {RS_Vector{middle(wx), middle(wy)}, target.x, target.y, wx, wy};
                    break;
                }
                if (refineNode(n)) { continue; }
                return good() ? componentFailure(ReferenceComponentReason::UnresolvedFringe) : false;
            }
            for (auto i = m_nodes[n].head; i != none; i = m_witnesses[i].nextNode) {
                if (!pair()) { return false; }
                if (m_witnesses[i].role != ReferenceRole::Own || m_witnesses[i].parent != i) { continue; }
                if (!room(m_components->fringes) || !vertices(1)) { return false; }
                LC_CurveJetBounds b;
                if (!fringeBounds(i, b)) { return false; }
                ReferenceFringe fringe;
                fringe.occurrence = {i}; // Private witness index until the publication map is established.
                fringe.parameter = m_witnesses[i].occurrenceParameter;
                fringe.localX = b.x; fringe.localY = b.y;
                if (!provider().frame().pointToWorld(b.x, b.y, fringe.worldX, fringe.worldY)) {
                    return componentFailure(ReferenceComponentReason::UnresolvedFringe);
                }
                m_components->fringes.push_back(fringe);
            }
        }
        return true;
    }
    struct VisibilityBasis {
        CriticalKernel kernel;
        Rational piLow, piHigh, duration;
        ExactAngle start, extent;
        bool reversed, whole;
        VisibilityBasis(const RS_EllipseData& basis, const double distance, const ReferenceFrame& frame)
            : kernel{basis, distance}, piLow{exactDouble(realPi().lo())}, piHigh{exactDouble(realPi().hi())},
              duration{exactDouble(frame.parameterEnd())}, start{exactDouble(frame.sourceSweep().start), 0},
              reversed{basis.reversed}, whole{frame.sourceSweep().whole} {
            extent = whole ? ExactAngle{0, reversed ? -2 : 2} :
                ExactAngle{exactDouble(basis.angle2) - start.constant, 0};
            if (!whole && reversed && extent.constant > 0) { extent.pi = -2; }
            if (!whole && !reversed && extent.constant < 0) { extent.pi = 2; }
        }
        ExactAngle angle(const Rational& t) const {
            return {start.constant + extent.constant * t / duration, start.pi + extent.pi * t / duration};
        }
        bool hSquared(const ExactAngle& angle, RationalRange& out) const {
            LC_Interval a, c, s;
            if (!exactAngleBounds(angle, piLow, piHigh, a) || !referenceTrig(a, c, s)) { return false; }
            const auto squared = sqr(s);
            if (!finiteInterval(squared)) { return false; }
            out = {kernel.r2 + kernel.difference * exactDouble(std::max(0.0, squared.lo())),
                   kernel.r2 + kernel.difference * exactDouble(std::min(1.0, squared.hi()))};
            return true;
        }
        int compare(const ExactAngle& a, const ExactAngle& b) const {
            const Rational constant = a.constant - b.constant, p = a.pi - b.pi;
            const Rational low = constant + p * (p < 0 ? piHigh : piLow);
            const Rational high = constant + p * (p < 0 ? piLow : piHigh);
            return low > 0 ? 1 : high < 0 ? -1 : low == 0 && high == 0 ? 0 : 2;
        }
    };
    int visibilitySign(const VisibilityBasis& basis, const Rational& t) {
        if (!pair()) { return 2; }
        const auto theta = basis.angle(t);
        RationalRange h;
        if (!basis.hSquared(theta, h)) { return 2; }
        RationalRange minimum{basis.kernel.r2, basis.kernel.r2};
        if (!basis.whole) {
            const auto last = basis.angle(basis.duration);
            const auto first = basis.reversed ? last : basis.start;
            const auto end = basis.reversed ? basis.start : last;
            const ExactAngle lo{(theta.constant + first.constant) / 2, (theta.pi + first.pi) / 2};
            const ExactAngle hi{(theta.constant + end.constant) / 2, (theta.pi + end.pi) / 2};
            RationalRange left, right;
            if (!basis.hSquared(lo, left) || !basis.hSquared(hi, right)) { return 2; }
            bool contained = false, uncertain = false;
            // Input phases and a single directed turn place every possible mirror in this fixed range.
            for (int k = -4; k <= 6; ++k) {
                if (!pair()) { return 2; }
                const ExactAngle mirror{0, k};
                const int a = basis.compare(lo, mirror), b = basis.compare(hi, mirror);
                if ((a == -1 || a == 0) && (b == 1 || b == 0)) { contained = true; break; }
                if (a == 2 || b == 2) { uncertain = true; }
            }
            if (!contained) {
                minimum = {uncertain ? basis.kernel.r2 : std::min(left.lo, right.lo),
                           std::min(left.hi, right.hi)};
            }
        }
        const Rational low = basis.kernel.axisSquared * minimum.lo * minimum.lo * h.lo -
            basis.kernel.distanceSquared * basis.kernel.r2;
        const Rational high = basis.kernel.axisSquared * minimum.hi * minimum.hi * h.hi -
            basis.kernel.distanceSquared * basis.kernel.r2;
        return low > 0 ? 1 : high < 0 ? -1 : low == 0 && high == 0 ? 0 : 2;
    }
    bool classify(const VisibilityBasis& basis, const LC_Interval& interval, ReferenceVisibility& visibility) {
        struct Box { double a, b; unsigned depth; };
        if (!vertices(1)) { return false; }
        std::vector<Box> pending{{interval.lo(), interval.hi(), 0}};
        while (!pending.empty()) {
            if (!pair()) { return false; }
            const Box box = std::move(pending.back()); pending.pop_back();
            if (!m_budget.checkDepth(box.depth)) { return limit(); }
            const auto sign = visibilitySign(basis, Rational{(exactDouble(box.a) + exactDouble(box.b)) / 2});
            if (!good()) { return false; }
            if (sign == 0) { return componentFailure(ReferenceComponentReason::UnresolvedVisibility); }
            if (sign != 2) {
                // Complete strict events exclude a change of nearest-foot visibility inside this gap:
                // another interior foot gives an own/opposite root, an endpoint a cap root, and a
                // loss at the generating foot F=0 is in the complete critical inventory.
                visibility = sign < 0 ? ReferenceVisibility::Hidden : ReferenceVisibility::Visible;
                return true;
            }
            if (box.depth >= m_budget.depthLimit()) { return limit(); }
            const double mid = middle(LC_Interval::hull(box.a, box.b));
            if (!(box.a < mid && mid < box.b)) {
                return componentFailure(ReferenceComponentReason::UnresolvedVisibility);
            }
            if (pending.size() > pending.max_size() - 2 || !vertices(2)) { return limit(); }
            pending.push_back({mid, box.b, box.depth + 1});
            pending.push_back({box.a, mid, box.depth + 1});
        }
        return componentFailure(ReferenceComponentReason::UnresolvedVisibility);
    }
    bool assemble(const RS_EllipseData& basis, const double distance) {
        for (auto& fringe : m_components->fringes) {
            if (!pair()) { return false; }
            const auto witness = fringe.occurrence.value;
            fringe.occurrence = publicOccurrence(witness);
            if (!good()) { return false; }
            fringe.parameter = m_data->occurrences[fringe.occurrence.value].parameter;
        }
        if (!vertices(1) || !pair()) { return false; }
        const VisibilityBasis exact{basis, distance, provider().frame()};
        if (distance < 0.0 && exact.whole && exact.kernel.r2 < 1 &&
            exact.kernel.distanceSquared == exact.kernel.axisSquared * exact.kernel.r2) {
            return componentFailure(ReferenceComponentReason::UnresolvedVisibility);
        }
        const auto& own = m_data->ownOccurrences;
        for (std::size_t i = 1; i < own.size(); ++i) {
            if (!room(m_components->spans) || !vertices(1) || !pair()) { return false; }
            const auto a = own[i - 1], b = own[i];
            const auto& from = m_data->occurrences[a.value];
            const auto& to = m_data->occurrences[b.value];
            if (!(from.parameter.hi() < to.parameter.lo())) { return contradiction(); }
            ReferenceSpan span;
            span.start = a; span.end = b;
            span.interior = LC_Interval::hull(from.parameter.hi(), to.parameter.lo());
            span.proof = distance > 0 ? ReferenceVisibilityProof::ConvexSupport :
                ReferenceVisibilityProof::CompleteEventClearance;
            span.visibility = ReferenceVisibility::Visible;
            if (distance < 0 && !classify(exact, span.interior, span.visibility)) { return false; }
            span.clearance = span.visibility == ReferenceVisibility::Visible ? LC_Interval::point(std::abs(distance)) :
                LC_Interval::hull(0.0, std::abs(distance));
            const auto index = m_components->spans.size();
            m_components->spans.push_back(span);
            if (span.visibility == ReferenceVisibility::Visible) {
                if (!room(m_components->edges) || !vertices(1) || !pair()) { return false; }
                m_components->edges.push_back({index, from.node, to.node});
            }
        }
        return graph(exact, distance);
    }
    bool hiddenEventPoint(const VisibilityBasis& basis, const double distance, const std::size_t index) {
        const auto& n = m_nodes[index];
        const Rational d = exactDouble(std::abs(distance)) / exactDouble(provider().frame().scale());
        const auto sourceWitness = [&](const LC_Interval& parameter) {
            if (!pair()) { return false; }
            if (!finiteInterval(parameter) || parameter.lo() < 0 || parameter.hi() > provider().frame().parameterEnd()) {
                return false;
            }
            LC_CurveJetBounds source;
            if (!provider().frame().boundSourceJet(parameter.lo(), parameter.hi(), source) ||
                !finiteInterval(source.x) || !finiteInterval(source.y)) { return false; }
            const auto separation = [](const LC_Interval& a, const LC_Interval& b) -> Rational {
                const Rational first = abs(Rational{exactDouble(a.lo()) - exactDouble(b.hi())});
                const Rational last = abs(Rational{exactDouble(a.hi()) - exactDouble(b.lo())});
                return std::max(first, last);
            };
            const Rational x = separation(n.x, source.x), y = separation(n.y, source.y);
            // One actual admitted source point suffices. Bounding every point in its enclosure
            // against the whole event enclosure also covers a root not representable as a double.
            return x * x + y * y < d * d;
        };
        for (auto i = n.head; i != none; i = m_witnesses[i].nextNode) {
            if (!pair()) { return false; }
            if (m_witnesses[i].role != ReferenceRole::Own) { continue; }
            const auto o = occurrence(i); if (o == none) { return false; }
            const auto& interval = m_witnesses[o].occurrenceParameter;
            const auto theta = basis.angle((exactDouble(interval.lo()) + exactDouble(interval.hi())) / 2);
            // Mirror-foot candidates are only witness selection, never an identity/nearest-foot
            // assumption. Their entire mapped enclosure must belong to the restricted source.
            for (int k = -4; k <= 6; ++k) {
                if (!pair()) { return false; }
                std::array<Rational, 2> values;
                std::size_t j = 0;
                const Rational low = basis.extent.constant + basis.extent.pi * basis.piLow;
                const Rational high = basis.extent.constant + basis.extent.pi * basis.piHigh;
                bool valid = basis.reversed ? low < 0 && high < 0 : low > 0 && high > 0;
                if (!valid) { continue; }
                for (const auto* pi : {&basis.piLow, &basis.piHigh}) {
                    const Rational denominator = basis.extent.constant + basis.extent.pi * *pi;
                    if (denominator == 0) { valid = false; break; }
                    values[j++] = basis.duration * (-theta.constant - basis.start.constant +
                        (2 * k - theta.pi - basis.start.pi) * *pi) / denominator;
                }
                LC_Interval parameter;
                if (valid && rationalBounds(std::min(values[0], values[1]), std::max(values[0], values[1]), parameter) &&
                    sourceWitness(parameter)) { return true; }
                if (!good()) { return false; }
            }
        }
        // A fixed finite set also covers hidden centre contacts whose mirror foot is the
        // generating point. Failure to find a witness is not a claim that the event is retained.
        for (unsigned i = 0; i <= 8; ++i) {
            if (!pair()) { return false; }
            LC_Interval parameter;
            const Rational t = basis.duration * i / 8;
            if (rationalBounds(t, t, parameter) && sourceWitness(parameter)) { return true; }
            if (!good()) { return false; }
        }
        return componentFailure(ReferenceComponentReason::UnresolvedEventPoint);
    }
    bool graph(const VisibilityBasis& basis, const double distance) {
        const auto count = m_data->nodes.size();
        struct Parent {
            std::size_t parent, component = none, privateNode = none;
            unsigned rank = 0;
            bool own = false;
        };
        std::vector<Parent> parents;
        if (count > parents.max_size() || count > m_components->incidence.max_size() ||
            !vertices(count) || !vertices(count)) { return limit(); }
        parents.reserve(count); m_components->incidence.reserve(count);
        for (std::size_t i = 0; i < count; ++i) {
            if (!pair()) { return false; }
            parents.push_back({i});
            ReferenceIncidence incidence; incidence.node = {i};
            m_components->incidence.push_back(std::move(incidence));
        }
        for (std::size_t i = 0; i < m_nodes.size(); ++i) {
            if (!pair()) { return false; }
            if (m_nodes[i].parent != i) { continue; }
            auto& target = parents[m_nodes[i].publicId];
            target.privateNode = i;
            for (auto w = m_nodes[i].head; w != none; w = m_witnesses[w].nextNode) {
                if (!pair()) { return false; }
                target.own = target.own || m_witnesses[w].role == ReferenceRole::Own;
            }
        }
        const auto find = [&](std::size_t i) {
            while (parents[i].parent != i) {
                if (!pair()) { return none; }
                i = parents[i].parent;
            }
            return i;
        };
        for (std::size_t e = 0; e < m_components->edges.size(); ++e) {
            if (!pair()) { return false; }
            const auto& edge = m_components->edges[e];
            auto a = find(edge.start.value); if (a == none) { return false; }
            auto b = find(edge.end.value); if (b == none) { return false; }
            if (a != b) {
                if (parents[a].rank < parents[b].rank) { std::swap(a, b); }
                if (parents[a].rank == parents[b].rank) { ++parents[a].rank; }
                parents[b].parent = a;
            }
            auto& outgoing = m_components->incidence[edge.start.value].outgoing;
            auto& incoming = m_components->incidence[edge.end.value].incoming;
            if (!room(outgoing) || !vertices(1) || !room(incoming) || !vertices(1)) { return false; }
            outgoing.push_back(e); incoming.push_back(e);
        }
        for (const auto& f : m_data->freeEnds) {
            if (!pair()) { return false; }
            auto& n = m_components->incidence[m_data->occurrences[f.occurrence.value].node.value];
            (f.atStart ? n.sourceStart : n.sourceEnd) = true;
        }
        for (const auto& c : m_data->criticals) {
            if (!pair()) { return false; }
            auto& n = m_components->incidence[m_data->occurrences[c.occurrence.value].node.value];
            (c.kind == CriticalKind::SimpleCusp ? n.cusp : n.stationary) = true;
        }
        for (const auto& p : m_data->pairs) {
            if (!pair()) { return false; }
            auto& n = m_components->incidence[m_data->occurrences[p.first.value].node.value];
            if (p.kind == LC_ParametricIntersection::Kind::Transverse) { n.crossing = true; }
            else if (p.kind == LC_ParametricIntersection::Kind::Tangent) { n.tangency = true; }
            else { n.construction = true; }
        }
        if (m_data->seam) {
            if (!pair()) { return false; }
            m_components->incidence[m_data->occurrences[m_data->seam->start.value].node.value].seam = true;
        }
        for (std::size_t e = 0; e < m_components->edges.size(); ++e) {
            if (!pair()) { return false; }
            const auto root = find(m_components->edges[e].start.value); if (root == none) { return false; }
            auto& index = parents[root].component;
            if (index == none) {
                if (!room(m_components->components) || !vertices(1)) { return false; }
                index = m_components->components.size();
                ReferenceComponent component; component.endpointFree = true;
                m_components->components.push_back(std::move(component));
            }
            auto& list = m_components->components[index].edges;
            if (!room(list) || !vertices(1)) { return false; }
            list.push_back(e);
        }
        for (auto& n : m_components->incidence) {
            if (!pair()) { return false; }
            if (n.incoming.empty() && n.outgoing.empty()) {
                // Other roles are cutters only; they do not belong to the retained own graph.
                if (!parents[n.node.value].own) { continue; }
                const auto privateNode = parents[n.node.value].privateNode;
                if (privateNode == none) { return contradiction(); }
                if (!hiddenEventPoint(basis, distance, privateNode)) { return false; }
                n.hiddenPoint = true;
                continue;
            }
            const auto root = find(n.node.value); if (root == none) { return false; }
            auto& c = m_components->components[parents[root].component];
            const bool end = n.incoming.empty() || n.outgoing.empty();
            c.endpointFree = c.endpointFree && !end;
            c.branching = c.branching || n.incoming.size() > 1 || n.outgoing.size() > 1;
            n.trimBoundary = end && !n.sourceStart && !n.sourceEnd;
        }
        return true;
    }
    ReferenceOccurrenceId publicOccurrence(const std::size_t witness) {
        const auto o = occurrence(witness);
        return {o == none ? none : m_witnesses[o].publicId};
    }
    bool publish() {
        std::size_t nodeCount = 0;
        for (std::size_t i = 0; i < m_nodes.size(); ++i) {
            if (!pair()) { return false; }
            if (m_nodes[i].parent == i) { ++nodeCount; }
        }
        if (nodeCount > m_data->nodes.max_size() || m_order.size() > m_data->occurrences.max_size() ||
            !vertices(nodeCount) || !vertices(m_order.size())) { return limit(); }
        m_data->nodes.reserve(nodeCount);
        m_data->occurrences.reserve(m_order.size());
        // Assign stable physical IDs by the first issued node, independent of union-tree rank.
        for (std::size_t i = 0; i < m_nodes.size(); ++i) {
            if (!pair()) { return false; }
            const auto n = node(i); if (n == none) { return false; }
            if (m_nodes[n].minimum != i) { continue; }
            m_nodes[n].publicId = m_data->nodes.size();
            m_nodes[n].published.localX = m_nodes[n].x;
            m_nodes[n].published.localY = m_nodes[n].y;
            m_data->nodes.push_back(m_nodes[n].published);
        }
        std::size_t ownCount = 0;
        for (std::size_t i = 0; i < m_order.size(); ++i) {
            if (!pair()) { return false; }
            const auto o = occurrence(m_order[i].value); if (o == none) { return false; }
            const auto n = node(m_witnesses[o].node); if (n == none) { return false; }
            auto& w = m_witnesses[o];
            w.publicId = m_data->occurrences.size();
            m_data->occurrences.push_back({w.role, w.occurrenceParameter, {m_nodes[n].publicId}});
            if (w.role == ReferenceRole::Own) { m_order[ownCount++] = {w.publicId}; }
        }
        m_order.resize(ownCount);
        m_data->ownOccurrences = std::move(m_order); // Reuse the already charged ordering slots.
        for (std::size_t i = 0; i < m_roots.size(); ++i) {
            if (!room(m_data->pairs) || !vertices(1) || !pair()) { return false; }
            const auto a = publicOccurrence(2 * i), b = publicOccurrence(2 * i + 1);
            if (!good()) { return false; }
            m_data->pairs.push_back({a, b, m_roots[i].snapshot.event.kind, m_roots[i].snapshot.event.proof});
        }
        for (std::size_t i = 0; i < m_criticalCount; ++i) {
            if (!room(m_data->criticals) || !vertices(1) || !pair()) { return false; }
            const auto wi = m_criticalStart + i;
            const auto id = publicOccurrence(wi); if (!good()) { return false; }
            m_data->criticals.push_back({id, m_witnesses[wi].critical.kind, m_witnesses[wi].critical.proof});
        }
        if (provider().frame().sourceSweep().whole) {
            if (!vertices(1) || !pair()) { return false; }
            const auto a = publicOccurrence(m_boundaryStart), b = publicOccurrence(m_boundaryStart + 1);
            if (!good()) { return false; }
            m_data->seam = ReferenceSeam{a, b};
        } else {
            for (std::size_t i = 0; i < 2; ++i) {
                if (!room(m_data->freeEnds) || !vertices(1) || !pair()) { return false; }
                const auto id = publicOccurrence(m_boundaryStart + i); if (!good()) { return false; }
                m_data->freeEnds.push_back({id, i == 0});
            }
        }
        return true;
    }

    WorkBudget& m_budget;
    Rational m_errorSquared;
    std::unique_ptr<ReferenceEventAnalysis::Impl> m_data;
    std::unique_ptr<ReferenceComponentAnalysis::Impl> m_components;
    std::vector<Root> m_roots;
    std::vector<Witness> m_witnesses;
    std::vector<Node> m_nodes;
    std::vector<ReferenceOccurrenceId> m_order;
    std::size_t m_criticalStart = 0, m_criticalCount = 0, m_boundaryStart = 0;
    std::array<bool, 4> m_criticalBlocked{};
    LC_IntersectionStatus m_status = LC_IntersectionStatus::Ok;
    ReferenceEventReason m_reason = ReferenceEventReason::None;
    ReferenceComponentReason m_componentReason = ReferenceComponentReason::None;
};

ReferenceEventAnalysis::ReferenceEventAnalysis(std::unique_ptr<Impl> impl) : m_impl{std::move(impl)} {}
ReferenceEventAnalysis::~ReferenceEventAnalysis() = default;
const ReferenceProviders& ReferenceEventAnalysis::providers() const noexcept { return *m_impl->providerView; }
double ReferenceEventAnalysis::localTolerance() const noexcept { return m_impl->localTolerance; }
const std::vector<ReferenceOccurrence>& ReferenceEventAnalysis::occurrences() const noexcept { return m_impl->occurrences; }
const std::vector<ReferenceOccurrenceId>& ReferenceEventAnalysis::ownOccurrences() const noexcept { return m_impl->ownOccurrences; }
const std::vector<ReferenceNode>& ReferenceEventAnalysis::nodes() const noexcept { return m_impl->nodes; }
const std::vector<ReferencePairEvent>& ReferenceEventAnalysis::pairs() const noexcept { return m_impl->pairs; }
const std::vector<ReferenceCriticalEvent>& ReferenceEventAnalysis::criticals() const noexcept { return m_impl->criticals; }
const std::vector<ReferenceFreeEnd>& ReferenceEventAnalysis::freeEnds() const noexcept { return m_impl->freeEnds; }
const std::optional<ReferenceSeam>& ReferenceEventAnalysis::seam() const noexcept { return m_impl->seam; }

ReferenceEventResult analyseReferenceEvents(const RS_EllipseData& basis, const double signedDistance,
                                          const double worldTolerance, WorkBudget& budget) {
    return ReferenceEventBuilder::run(basis, signedDistance, worldTolerance, budget);
}

ReferenceComponentAnalysis::ReferenceComponentAnalysis(std::unique_ptr<Impl> impl) : m_impl{std::move(impl)} {}
ReferenceComponentAnalysis::~ReferenceComponentAnalysis() = default;
const ReferenceEventAnalysis& ReferenceComponentAnalysis::events() const noexcept { return *m_impl->events; }
const std::vector<ReferenceFringe>& ReferenceComponentAnalysis::fringes() const noexcept { return m_impl->fringes; }
const std::vector<ReferenceSpan>& ReferenceComponentAnalysis::spans() const noexcept { return m_impl->spans; }
const std::vector<ReferenceEdge>& ReferenceComponentAnalysis::edges() const noexcept { return m_impl->edges; }
const std::vector<ReferenceIncidence>& ReferenceComponentAnalysis::incidence() const noexcept { return m_impl->incidence; }
const std::vector<ReferenceComponent>& ReferenceComponentAnalysis::components() const noexcept { return m_impl->components; }
ReferenceComponentResult analyseReferenceComponents(const RS_EllipseData& basis, const double signedDistance,
                                                  const double worldTolerance, WorkBudget& budget) {
    return ReferenceEventBuilder::components(basis, signedDistance, worldTolerance, budget);
}

CriticalRefinementSession::CriticalRefinementSession(std::unique_ptr<Impl> impl) : m_impl{std::move(impl)} {}
CriticalRefinementSession::~CriticalRefinementSession() = default;
CriticalRefinementSession::CriticalRefinementSession(CriticalRefinementSession&&) noexcept = default;
CriticalRefinementSession& CriticalRefinementSession::operator=(CriticalRefinementSession&&) noexcept = default;

std::size_t CriticalRefinementSession::occurrenceCount() const {
    return m_impl && !m_impl->terminal ? m_impl->records.size() : 0;
}

CriticalOccurrenceHandle CriticalRefinementSession::occurrenceHandle(const std::size_t index) const {
    CriticalOccurrenceHandle handle;
    if (index < occurrenceCount()) {
        handle.m_issuer = m_impl->issuer;
        handle.m_index = index;
    }
    return handle;
}

std::optional<CriticalOccurrenceSnapshot> CriticalRefinementSession::snapshot(const CriticalOccurrenceHandle& handle) const {
    if (!m_impl || m_impl->terminal || !m_impl->handle(handle)) { return {}; }
    auto out = m_impl->records[handle.m_index].current;
    // Geometry is atomically staged; admitted failed work still belongs to ALL symmetric occurrences.
    out.admittedRounds = m_impl->depth;
    return out;
}

CheckWork CriticalRefinementSession::lastMatchingWork() const { return m_impl ? m_impl->lastWork : CheckWork{}; }

CriticalContractionResult CriticalRefinementSession::contract(const CriticalOccurrenceHandle& handle,
                                                            WorkBudget& budget) {
    CriticalContractionResult out;
    if (!m_impl) { return out; }
    auto& state = *m_impl;
    out.work = state.lastWork;
    if (!state.matches(state.budgetIdentity, budget.m_identity)) { return out; }
    out.workSource = CriticalWorkSource::CurrentBudget;
    out.work = budget.consumed();
    if (!state.handle(handle)) { return out; }
    const auto finish = [&]() {
        state.lastWork = budget.consumed();
        out.work = state.lastWork;
        return out;
    };
    if (state.terminal) { return finish(); }
    if (state.limited || budget.status() == BudgetStatus::LimitExceeded) {
        state.limited = true;
        out.status = LC_IntersectionStatus::LimitExceeded;
        return finish();
    }
    if (budget.status() != BudgetStatus::Ok) {
        state.terminal = true;
        return finish();
    }
    if (state.records[handle.m_index].original.kind == CriticalKind::CriticalStationary) {
        out.status = LC_IntersectionStatus::AmbiguousTopology;
        return finish();
    }
    const auto limit = [&]() {
        budget.latchLimitExceeded();
        state.limited = true;
        out.status = LC_IntersectionStatus::LimitExceeded;
        return finish();
    };
    if (state.depth >= state.ceilings.maxDepth || state.depth >= budget.depthLimit()) { return limit(); }
    const auto vertices = [&](const std::size_t count) {
        if (!budget.consumeVertices(count)) { return false; }
        out.entered.vertices += count;
        return true;
    };
    const auto pair = [&]() {
        if (!budget.consumePairs(1)) { return false; }
        ++out.entered.distancePairs;
        return true;
    };
    try {
        if (!vertices(1) || !pair()) { return limit(); }
        ++state.depth; // Admitted level; failed later work must never reset this history.
        if (!budget.checkDepth(state.depth)) { return limit(); }
        out.entered.deepest = state.depth;
        CriticalCanonicalBounds next = state.canonical;
        const auto scalar = state.scalar(next);
        if (scalar != Impl::Step::Ok) {
            state.terminal = scalar == Impl::Step::Contradiction;
            out.status = state.terminal ? LC_IntersectionStatus::InvalidInput : LC_IntersectionStatus::AmbiguousTopology;
            return finish();
        }
        if (!pair()) { return limit(); }
        const auto inverse = state.inverse(next);
        if (inverse != Impl::Step::Ok) {
            state.terminal = inverse == Impl::Step::Contradiction;
            out.status = state.terminal ? LC_IntersectionStatus::InvalidInput : LC_IntersectionStatus::AmbiguousTopology;
            return finish();
        }
        if (!pair()) { return limit(); }
        const auto common = state.consistent(next);
        if (common != Impl::Step::Ok) {
            state.terminal = common == Impl::Step::Contradiction;
            out.status = state.terminal ? LC_IntersectionStatus::InvalidInput : LC_IntersectionStatus::AmbiguousTopology;
            return finish();
        }
        if (next.variable.lo() == state.canonical.variable.lo() &&
            next.variable.hi() == state.canonical.variable.hi() &&
            next.inverseAngle.lo() == state.canonical.inverseAngle.lo() &&
            next.inverseAngle.hi() == state.canonical.inverseAngle.hi()) {
            out.status = LC_IntersectionStatus::AmbiguousTopology;
            return finish();
        }
        // One logical staged record per occurrence, before allocating the publication vector.
        if (!vertices(state.records.size())) { return limit(); }
        std::vector<CriticalOccurrenceSnapshot> staged;
        if (state.records.size() > staged.max_size()) { return limit(); }
        staged.reserve(state.records.size());
        for (const auto& record : state.records) {
            if (!pair()) { return limit(); }
            CriticalOccurrenceSnapshot candidate = record.current;
            LC_Interval parameter, x, y;
            if (!state.provenance->map(record.original, next.inverseAngle, parameter) ||
                !state.provenance->image(record.original, next.variable, x, y)) {
                out.status = LC_IntersectionStatus::AmbiguousTopology;
                return finish();
            }
            candidate.occurrence.parameter = intersect(record.current.occurrence.parameter, parameter);
            candidate.imageX = intersect(record.current.imageX, x);
            candidate.imageY = intersect(record.current.imageY, y);
            if (!finiteInterval(candidate.occurrence.parameter) || !finiteInterval(candidate.imageX) ||
                !finiteInterval(candidate.imageY)) {
                state.terminal = true;
                return finish();
            }
            candidate.canonical = next;
            candidate.admittedRounds = state.depth;
            staged.push_back(candidate);
        }
        for (std::size_t i = 0; i < staged.size(); ++i) { state.records[i].current = staged[i]; }
        state.canonical = next;
        out.status = LC_IntersectionStatus::Ok;
        out.snapshot = snapshot(handle);
        return finish();
    } catch (...) {
        state.terminal = true;
        state.lastWork = budget.consumed();
        throw;
    }
}

RS_Vector ellipsePoint(const RS_EllipseData& basis, const double parameter) {
    const auto f = frame(basis);
    if (!f || !std::isfinite(parameter)) {
        return RS_Vector{false};
    }
    return checked(basis.center + f->major * std::cos(parameter) + f->minor * std::sin(parameter));
}

RS_Vector ellipseDerivative(const RS_EllipseData& basis, const double parameter) {
    const auto f = frame(basis);
    if (!f || !std::isfinite(parameter)) {
        return RS_Vector{false};
    }
    return checked(f->minor * std::cos(parameter) - f->major * std::sin(parameter));
}

RS_Vector outwardNormal(const RS_EllipseData& basis, const double parameter) {
    const auto f = frame(basis);
    if (!f || !std::isfinite(parameter)) {
        return RS_Vector{false};
    }
    const double x = basis.ratio * std::cos(parameter);
    const double y = std::sin(parameter);
    const double length = std::hypot(x, y);
    if (!(length > 0.0) || !std::isfinite(length)) {
        return RS_Vector{false};
    }
    return checked(f->unitMajor * (x / length) + f->unitMinor * (y / length));
}

RS_Vector exactOffsetPoint(const RS_EllipseData& basis, const double parameter, const double signedDistance) {
    const RS_Vector point = ellipsePoint(basis, parameter);
    const RS_Vector normal = outwardNormal(basis, parameter);
    if (!point.valid || !normal.valid || !std::isfinite(signedDistance)) {
        return RS_Vector{false};
    }
    return checked(point + normal * signedDistance);
}

double radiusOfCurvature(const RS_EllipseData& basis, const double parameter) {
    const auto f = frame(basis);
    if (!f || !std::isfinite(parameter)) {
        return invalidNumber();
    }
    const double q = std::hypot(std::sin(parameter), basis.ratio * std::cos(parameter));
    int axisExponent = 0;
    int speedExponent = 0;
    int ratioExponent = 0;
    const double axisMantissa = std::frexp(f->a, &axisExponent);
    const double speedMantissa = std::frexp(q, &speedExponent);
    const double ratioMantissa = std::frexp(basis.ratio, &ratioExponent);
    const double radius = std::scalbn(axisMantissa * speedMantissa * speedMantissa * speedMantissa / ratioMantissa,
                                      axisExponent + 3 * speedExponent - ratioExponent);
    return std::isfinite(radius) && radius > 0.0 ? radius : invalidNumber();
}

double minRadiusOfCurvature(const RS_EllipseData& basis) {
    const auto f = frame(basis);
    const auto interval = sweep(basis);
    if (!f || !interval) {
        return invalidNumber();
    }
    if (interval->whole || interval->contains(0.0) || interval->contains(M_PI)) {
        const double radius = f->b * basis.ratio;
        return radius > 0.0 && std::isfinite(radius) ? radius : invalidNumber();
    }
    return std::min(radiusOfCurvature(basis, interval->start),
                    radiusOfCurvature(basis, interval->start + interval->signedSweep));
}

bool isInside(const RS_EllipseData& basis, const RS_Vector& point) {
    const auto f = frame(basis);
    if (!f || !isFinite2D(point)) {
        return false;
    }
    const RS_Vector delta = point - basis.center;
    const double x = std::fma(delta.x, f->unitMajor.x, delta.y * f->unitMajor.y) / f->a;
    const double y = std::fma(delta.x, f->unitMinor.x, delta.y * f->unitMinor.y) / f->b;
    return std::isfinite(x) && std::isfinite(y) && std::hypot(x, y) <= 1.0;
}

std::optional<double> lensTrimAngle(const RS_EllipseData& basis, const double distance) {
    const auto f = frame(basis);
    if (!f || basis.ratio == 1.0 || !std::isfinite(distance) || !(distance > 0.0) ||
        distance < f->b * basis.ratio || !(distance < f->b)) {
        return std::nullopt;
    }
    const double ratio = basis.ratio;
    const double scaledDistance = distance / f->b;
    const double sineSquared = ((scaledDistance - ratio) * (scaledDistance + ratio)) /
                               ((1.0 - ratio) * (1.0 + ratio));
    return std::asin(std::sqrt(std::clamp(sineSquared, 0.0, 1.0)));
}

std::vector<RS_Vector> lensCorners(const RS_EllipseData& basis, const double distance) {
    const auto angle = lensTrimAngle(basis, distance);
    const auto f = frame(basis);
    if (!angle || !f) {
        return {};
    }
    const double extent = f->a * ((1.0 - basis.ratio) * (1.0 + basis.ratio)) * std::cos(*angle);
    const RS_Vector first = checked(basis.center + f->unitMajor * extent);
    const RS_Vector second = checked(basis.center - f->unitMajor * extent);
    return first.valid && second.valid ? std::vector<RS_Vector>{first, second} : std::vector<RS_Vector>{};
}

double distanceToOriginal(const RS_EllipseData& basis, const RS_Vector& point, double* parameter) {
    if (parameter != nullptr) {
        *parameter = invalidNumber();
    }
    const auto f = frame(basis);
    if (!f || !isFinite2D(point)) {
        return std::numeric_limits<double>::infinity();
    }
    const double dx = point.x - basis.center.x;
    const double dy = point.y - basis.center.y;
    const bool finiteDelta = std::isfinite(dx) && std::isfinite(dy);
    const double scale = finiteDelta ? std::max({f->a, std::abs(dx), std::abs(dy)})
        : std::max({f->a, std::abs(point.x), std::abs(point.y), std::abs(basis.center.x), std::abs(basis.center.y)});
    const double scaledX = std::isfinite(dx) ? dx / scale : point.x / scale - basis.center.x / scale;
    const double scaledY = std::isfinite(dy) ? dy / scale : point.y / scale - basis.center.y / scale;
    const RS_Vector local{std::fma(scaledX, f->unitMajor.x, scaledY * f->unitMajor.y),
                          std::fma(scaledX, f->unitMinor.x, scaledY * f->unitMinor.y)};
    if (!isFinite2D(local)) {
        return std::numeric_limits<double>::infinity();
    }
    const double axisScale = f->a / scale;
    RS_Vector nearest{false};
    bool thinProjection = false;
    if (std::hypot(local.x, local.y) > axisScale * 1e8) {
        nearest = farEllipseFoot(local, basis.ratio, axisScale);
    } else {
        const RS_Vector unitQuery{local.x / axisScale, local.y / axisScale};
        const bool thin = basis.ratio <= std::numeric_limits<double>::epsilon();
        if (thin && std::abs(unitQuery.x) < 1.0) {
            const double sine = std::sqrt((1.0 - std::abs(unitQuery.x)) * (1.0 + std::abs(unitQuery.x)));
            if (basis.ratio * std::abs(unitQuery.y) <= std::numeric_limits<double>::epsilon() * sine) {
                // Here the x correction is below an ulp; preserve tiny vertical distances without squaring.
                nearest = RS_Vector{unitQuery.x, std::copysign(basis.ratio * sine, unitQuery.y)};
                thinProjection = true;
            } else {
                nearest = farEllipseFoot(local, basis.ratio, axisScale);
            }
        } else if (thin && unitQuery.y != 0.0) {
            nearest = farEllipseFoot(local, basis.ratio, axisScale);
        } else {
            const RS_Ellipse unit{nullptr, RS_EllipseData{RS_Vector{0.0, 0.0}, RS_Vector{1.0, 0.0}, basis.ratio}};
            // Squared distances can tie by underflow; symmetry fixes the correct quadrant before comparison.
            nearest = unit.getNearestPointOnEntity(RS_Vector{std::abs(unitQuery.x), std::abs(unitQuery.y)}, true);
            if (nearest.valid) {
                nearest.x = std::copysign(nearest.x, unitQuery.x);
                nearest.y = std::copysign(nearest.y, unitQuery.y);
            }
        }
    }
    if (!isFinite2D(nearest) ||
        std::abs(std::hypot(nearest.x, nearest.y / basis.ratio) - 1.0) >
            256.0 * std::numeric_limits<double>::epsilon()) {
        return std::numeric_limits<double>::infinity();
    }
    double distance = std::hypot(local.x - axisScale * nearest.x, local.y - axisScale * nearest.y) * scale;
    if (thinProjection && finiteDelta) {
        const double physicalY = std::fma(dx, f->unitMinor.x, dy * f->unitMinor.y);
        if (std::isfinite(physicalY)) {
            distance = std::abs(physicalY - f->a * nearest.y);
        }
    }
    if (std::isfinite(distance) && parameter != nullptr) {
        *parameter = wrapped(std::atan2(nearest.y / basis.ratio, nearest.x));
    }
    return distance;
}

} // namespace LC_EllipseOffsetGeometry
