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
#include <cmath>
#include <limits>

#include <boost/math/tools/toms748_solve.hpp>

#include "rs_math.h"

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
