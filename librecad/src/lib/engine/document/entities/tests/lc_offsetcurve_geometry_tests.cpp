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

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <random>
#include <type_traits>
#include <utility>

#include <boost/math/constants/constants.hpp>
#include <boost/multiprecision/cpp_bin_float.hpp>

#include "lc_curveoffset.h"
#include "lc_ellipseoffsetgeometry.h"
#include "rs_line.h"

namespace geo = LC_EllipseOffsetGeometry;

namespace {
RS_EllipseData basis(const double a = 10.0, const double ratio = 0.5,
                     const double from = 0.0, const double to = 0.0, const bool reversed = false) {
    return {RS_Vector{0.0, 0.0}, RS_Vector{a, 0.0}, ratio, from, to, reversed};
}

double dot(const RS_Vector& a, const RS_Vector& b) {
    return a.x * b.x + a.y * b.y;
}

void checkSameFields(const RS_EllipseData& a, const RS_EllipseData& b) {
    CHECK(a.center.x == b.center.x);
    CHECK(a.center.y == b.center.y);
    CHECK(a.majorP.x == b.majorP.x);
    CHECK(a.majorP.y == b.majorP.y);
    CHECK(a.ratio == b.ratio);
    CHECK(a.angle1 == b.angle1);
    CHECK(a.angle2 == b.angle2);
    CHECK(a.reversed == b.reversed);
    CHECK(a.angleDegrees == b.angleDegrees);
}

using Quad = boost::multiprecision::cpp_bin_float_quad;
using QuadJet = std::array<Quad, 6>;

Quad idealDirectedSweep(const RS_EllipseData& data, const bool whole) {
    const Quad period = 2 * boost::math::constants::pi<Quad>();
    if (whole) {
        return data.reversed ? -period : period;
    }
    Quad directed = Quad(data.angle2) - Quad(data.angle1);
    if (data.reversed && directed > 0) {
        directed -= period;
    } else if (!data.reversed && directed < 0) {
        directed += period;
    }
    return directed;
}

/** Independent 113-bit evaluation of exact input doubles, including axis normalization. */
QuadJet referenceOracle(const RS_EllipseData& data, const geo::ReferenceFrame& frame, const double delta,
                        const double t, const bool offset, const bool canonicalEndpoint = true,
                        const bool unitRate = false) {
    const bool whole = frame.sourceSweep().whole;
    const Quad directed = idealDirectedSweep(data, whole);
    const Quad sign = data.reversed ? -1 : 1;
    const Quad rate = unitRate ? Quad(1) : abs(directed) / Quad(frame.parameterEnd());
    const Quad signedRate = sign * rate;
    const Quad start = whole ? Quad(M_PI_4) : Quad(data.angle1);
    Quad angle = start + signedRate * Quad(t);
    if (!unitRate) {
        const Quad endpoint = whole ? start : Quad(data.angle2);
        if (canonicalEndpoint && t == frame.parameterEnd()) {
            angle = endpoint;
        } else if (t > 0.5 * frame.parameterEnd()) {
            // Periodically equivalent over the entire last half, not an endpoint patch.
            // This avoids treating finite-precision sin(2*pi) residue as exact zero's oracle.
            angle = endpoint - signedRate * (Quad(frame.parameterEnd()) - Quad(t));
        }
    }
    const Quad c = cos(angle);
    const Quad s = sin(angle);
    const Quad mx = Quad(data.majorP.x) / Quad(frame.scale());
    const Quad my = Quad(data.majorP.y) / Quad(frame.scale());
    const Quad r = data.ratio;
    const Quad a = sqrt(mx * mx + my * my);
    const Quad h2 = s * s + r * r * c * c;
    const Quad h = sqrt(h2);
    QuadJet jet{{mx * c - my * r * s, my * c + mx * r * s,
                 signedRate * (-mx * s - my * r * c), signedRate * (-my * s + mx * r * c), 0, 0}};
    jet[4] = -rate * rate * jet[0];
    jet[5] = -rate * rate * jet[1];
    if (offset) {
        const Quad d = Quad(delta) / Quad(frame.scale());
        const Quad factor = 1 + d * r / (a * h2 * h);
        const Quad factorDerivative = -3 * signedRate * d * r * (1 - r * r) * s * c / (a * h2 * h2 * h);
        jet[0] += d * (r * c * mx - s * my) / (a * h);
        jet[1] += d * (r * c * my + s * mx) / (a * h);
        jet[4] = factor * jet[4] + factorDerivative * jet[2];
        jet[5] = factor * jet[5] + factorDerivative * jet[3];
        jet[2] *= factor;
        jet[3] *= factor;
    }
    return jet;
}

void checkEnclosure(const LC_Interval& interval, const Quad& value) {
    REQUIRE(interval.isValid());
    REQUIRE(std::isfinite(interval.lo()));
    REQUIRE(std::isfinite(interval.hi()));
    CHECK(Quad(interval.lo()) <= value);
    CHECK(value <= Quad(interval.hi()));
}

void checkEnclosure(const LC_CurveJetBounds& bounds, const QuadJet& jet) {
    const std::array<LC_Interval, 6> intervals{{bounds.x, bounds.y, bounds.dx, bounds.dy, bounds.ddx, bounds.ddy}};
    for (std::size_t k = 0; k < intervals.size(); ++k) {
        CAPTURE(k);
        checkEnclosure(intervals[k], jet[k]);
    }
    checkEnclosure(bounds.speedSquaredProduct, jet[2] * jet[2] + jet[3] * jet[3]);
    checkEnclosure(bounds.crossProduct, jet[2] * jet[5] - jet[3] * jet[4]);
}

void checkCleared(const LC_CurveJetBounds& bounds) {
    CHECK_FALSE(bounds.x.isValid());
    CHECK_FALSE(bounds.y.isValid());
    CHECK_FALSE(bounds.dx.isValid());
    CHECK_FALSE(bounds.dy.isValid());
    CHECK_FALSE(bounds.ddx.isValid());
    CHECK_FALSE(bounds.ddy.isValid());
    CHECK_FALSE(bounds.speedSquaredProduct.isValid());
    CHECK_FALSE(bounds.crossProduct.isValid());
}

void checkSameJet(const LC_CurveJet& first, const LC_CurveJet& last) {
    CHECK(first.point.x == last.point.x);
    CHECK(first.point.y == last.point.y);
    CHECK(first.first.x == last.first.x);
    CHECK(first.first.y == last.first.y);
    CHECK(first.second.x == last.second.x);
    CHECK(first.second.y == last.second.y);
}

std::array<RS_EllipseData, 18> rateSources() {
    const double period = 2.0 * M_PI;
    const double tiny = 2.0 * RS_TOLERANCE_ANGLE;
    return {{basis(100.0, 0.5), basis(100.0, 0.5, 0.0, 0.0, true),
             basis(100.0, 0.5, 0.7, 0.7 + period), basis(100.0, 0.5, 0.7, 0.7 - period, true),
             basis(100.0, 0.5, 0.2, 6.0), basis(100.0, 0.5, 6.0, 0.2, true),
             basis(100.0, 0.5, 6.0, 0.2), basis(100.0, 0.5, 0.2, 6.0, true),
             basis(100.0, 0.5, 0.7, 0.7 + tiny), basis(100.0, 0.5, 0.7 + tiny, 0.7, true),
             basis(100.0, 0.5, period - tiny, tiny), basis(100.0, 0.5, tiny, period - tiny, true),
             basis(100.0, 0.5, tiny, period - tiny), basis(100.0, 0.5, period - tiny, tiny, true),
             basis(100.0, 0.5, 0.1, 0.8), basis(100.0, 0.5, 0.8, 0.1, true),
             basis(100.0, 0.5, period - tiny, 0.0), basis(100.0, 0.5, 0.0, period - tiny, true)}};
}

std::array<Quad, 2> factorOracle(const RS_EllipseData& data, const geo::ReferenceFrame& frame,
                                const double delta, const double t) {
    const bool whole = frame.sourceSweep().whole;
    const Quad directed = idealDirectedSweep(data, whole);
    const Quad rate = directed / Quad(frame.parameterEnd());
    const Quad start = whole ? Quad(M_PI_4) : Quad(data.angle1);
    const Quad end = whole ? start : Quad(data.angle2);
    const Quad angle = t > 0.5 * frame.parameterEnd() ?
        end - rate * (Quad(frame.parameterEnd()) - Quad(t)) : start + rate * Quad(t);
    const Quad s = sin(angle);
    const Quad c = cos(angle);
    const Quad r = data.ratio;
    const Quad a = sqrt(Quad(data.majorP.x) * Quad(data.majorP.x) + Quad(data.majorP.y) * Quad(data.majorP.y));
    const Quad h2 = r * r * c * c + s * s;
    return {{1 + Quad(delta) * r / (a * h2 * sqrt(h2)),
        -3 * rate * Quad(delta) * r * (1 - r * r) * s * c / (a * h2 * h2 * sqrt(h2))}};
}

void checkCriticalCoverage(const geo::CriticalAdapter& adapter) {
    const auto& segments = adapter.segments();
    REQUIRE_FALSE(segments.empty());
    REQUIRE(adapter.regions().size() == segments.size());
    CHECK(segments.front().t0 == 0.0);
    CHECK(segments.back().t1 == adapter.frame().parameterEnd());
    CHECK(adapter.branchCount() == 1);
    CHECK(adapter.next(0) == (adapter.frame().sourceSweep().whole ? 0 : -1));
    for (std::size_t i = 0; i < segments.size(); ++i) {
        CAPTURE(i);
        const auto& segment = segments[i];
        CHECK(segment.branch == 0);
        REQUIRE(segment.t0 < segment.t1);
        if (i != 0) { CHECK(segments[i - 1].t1 == segment.t0); }
        const auto proof = adapter.certifyInjective(i, LC_Interval::hull(segment.t0, segment.t1));
        REQUIRE(proof.available);
        CHECK(proof.segment == i);
        CHECK(proof.parameter.lo() == segment.t0);
        CHECK(proof.parameter.hi() == segment.t1);
        for (const double t : {segment.t0, segment.t0 + 0.5 * (segment.t1 - segment.t0), segment.t1}) {
            RS_Vector p;
            RS_Vector d;
            REQUIRE(adapter.evaluate(i, t, p, d));
            CHECK(geo::isFinite2D(p));
            CHECK(geo::isFinite2D(d));
            LC_Interval x, y, dx, dy;
            REQUIRE(adapter.bound(i, t, t, x, y, dx, dy));
            CHECK(x.contains(p.x));
            CHECK(y.contains(p.y));
            CHECK(dx.contains(d.x));
            CHECK(dy.contains(d.y));
        }
    }
}

void checkCriticalOccurrences(const RS_EllipseData& data, const double delta,
                              const geo::CriticalAdapter& adapter) {
    const Quad pi = boost::math::constants::pi<Quad>();
    const Quad directed = idealDirectedSweep(data, adapter.frame().sourceSweep().whole);
    const Quad start = adapter.frame().sourceSweep().whole ? Quad(M_PI_4) : Quad(data.angle1);
    const Quad a = sqrt(Quad(data.majorP.x) * Quad(data.majorP.x) + Quad(data.majorP.y) * Quad(data.majorP.y));
    const Quad r = data.ratio;
    Quad beta = 0;
    if (adapter.regime() == geo::CriticalRegime::IntermediateThresholds) {
        const Quad h2 = pow(Quad(-delta) * r / a, Quad(2) / 3);
        beta = asin(sqrt((h2 - r * r) / (1 - r * r)));
    }
    const auto& occurrences = adapter.criticalOccurrences();
    for (std::size_t i = 0; i < occurrences.size(); ++i) {
        const auto& occurrence = occurrences[i];
        CAPTURE(i, occurrence.quadrant, occurrence.winding);
        const int index = occurrence.quadrant + 4 * occurrence.winding;
        Quad angle = Quad(index) * pi / 2;
        if (occurrence.kind == geo::CriticalKind::SimpleCusp) {
            angle = index % 2 == 0 ? angle + beta : angle + pi / 2 - beta;
        }
        const Quad parameter = occurrence.atStart ? Quad(0) : occurrence.atEnd ? Quad(adapter.frame().parameterEnd()) :
            (angle - start) * Quad(adapter.frame().parameterEnd()) / directed;
        checkEnclosure(occurrence.parameter, parameter);
        CHECK(occurrence.parameter.lo() >= 0.0);
        CHECK(occurrence.parameter.hi() <= adapter.frame().parameterEnd());
        if (i != 0) { CHECK(occurrences[i - 1].parameter.hi() < occurrence.parameter.lo()); }
        CHECK(occurrence.proof == (occurrence.kind == geo::CriticalKind::SimpleCusp ?
            geo::CriticalProof::MonotonePolynomial : geo::CriticalProof::ExactAxisThreshold));
    }
}
} // namespace

TEST_CASE("Offset basis normalization distinguishes raw whole, full and tiny sweeps", "[lc-offsetcurve][geometry]") {
    auto whole = basis();
    REQUIRE(geo::normaliseBasis(whole));
    CHECK(whole.angle1 == 0.0);
    CHECK(whole.angle2 == 0.0);
    CHECK_FALSE(whole.isArc);

    for (const bool reverse : {false, true}) {
        for (const double direction : {-1.0, 1.0}) {
            auto full = basis(5.0, 2.0, 0.7, 0.7 + direction * 2.0 * M_PI, reverse);
            REQUIRE(geo::normaliseBasis(full));
            CHECK(full.ratio == 0.5);
            CHECK(full.angle1 == 0.0);
            CHECK(full.angle2 == 0.0);
            CHECK_FALSE(full.isArc);
            CHECK(full.reversed == reverse);
            CHECK(full.majorP.magnitude() == Catch::Approx(10.0));
        }
    }
    for (const double delta : {0.0, 0.5 * RS_TOLERANCE_ANGLE, 4.0 * M_PI}) {
        auto invalid = basis(10.0, 0.5, 0.7, 0.7 + delta);
        invalid.angleDegrees = 42.0;
        const auto original = invalid;
        CHECK_FALSE(geo::normaliseBasis(invalid));
        checkSameFields(invalid, original);
    }
    auto tiny = basis(10.0, 0.5, RS_TOLERANCE_ANGLE * 0.1, RS_TOLERANCE_ANGLE * 0.2);
    CHECK_FALSE(geo::normaliseBasis(tiny));
    tiny.reversed = true;
    CHECK_FALSE(geo::normaliseBasis(tiny)); // near-full, but the native API would collapse both angles to zero

    auto wrappedAlmostFull = basis(10.0, 0.5, 0.0, -0.5 * RS_TOLERANCE_ANGLE);
    REQUIRE(geo::normaliseBasis(wrappedAlmostFull));
    CHECK(wrappedAlmostFull.isArc);
    REQUIRE(geo::sweep(wrappedAlmostFull));
    CHECK(geo::sweep(wrappedAlmostFull)->signedSweep > 6.0);
}

TEST_CASE("Offset full-turn allowance is distinct from tiny-arc allowance", "[lc-offsetcurve][geometry]") {
    const double allowance = 1e-12 * 2.0 * M_PI;
    auto accepted = basis(10.0, 0.5, 0.0, 2.0 * M_PI + allowance * 0.5);
    REQUIRE(geo::normaliseBasis(accepted));
    CHECK_FALSE(accepted.isArc);
    auto tooManyTurns = basis(10.0, 0.5, 0.0, 2.0 * M_PI + allowance * 2.0);
    CHECK_FALSE(geo::normaliseBasis(tooManyTurns));
    auto almostFull = basis(10.0, 0.5, 0.0, 2.0 * M_PI - allowance * 2.0);
    REQUIRE(geo::normaliseBasis(almostFull));
    CHECK(almostFull.isArc);
    auto smallUsable = basis(10.0, 0.5, 0.7, 0.7 + RS_TOLERANCE_ANGLE * 2.0);
    REQUIRE(geo::normaliseBasis(smallUsable));
    CHECK(smallUsable.isArc);
}

TEST_CASE("Offset source sweep matches wrapped and reversed engine conventions", "[lc-offsetcurve][geometry]") {
    for (const bool reverse : {false, true}) {
        const auto data = reverse ? basis(10.0, 0.5, 0.2, 6.0, true) : basis(10.0, 0.5, 6.0, 0.2);
        const auto interval = geo::sweep(data);
        REQUIRE(interval);
        CHECK_FALSE(interval->whole);
        CHECK(interval->contains(0.0));
        CHECK(interval->contains(data.angle1));
        CHECK(interval->contains(data.angle2, 1e-14));
        CHECK_FALSE(interval->contains(M_PI));
        CHECK((interval->signedSweep < 0.0) == reverse);
    }
    REQUIRE(geo::sweep(basis()));
    CHECK(geo::sweep(basis())->start == M_PI_4);
    CHECK(geo::sweep(basis())->whole);
    CHECK_FALSE(geo::sweep(basis())->contains(std::numeric_limits<double>::infinity()));
    CHECK_FALSE(geo::sweep(basis())->contains(0.0, -1.0));
}

TEST_CASE("Offset axis normalization preserves arc endpoints at large world centres", "[lc-offsetcurve][geometry]") {
    auto data = basis(3.0, 2.0, 0.3, 4.2, true);
    data.center = RS_Vector{1e12, -1e12};
    const RS_Vector oldMinor{-data.majorP.y * data.ratio, data.majorP.x * data.ratio};
    const RS_Vector first = data.center + data.majorP * std::cos(data.angle1) + oldMinor * std::sin(data.angle1);
    const RS_Vector last = data.center + data.majorP * std::cos(data.angle2) + oldMinor * std::sin(data.angle2);
    REQUIRE(geo::normaliseBasis(data));
    CHECK(data.ratio == 0.5);
    CHECK(data.reversed);
    CHECK(geo::ellipsePoint(data, data.angle1).distanceTo(first) <= 0.0003);
    CHECK(geo::ellipsePoint(data, data.angle2).distanceTo(last) <= 0.0003);
    CHECK(data.angle1 >= 0.0);
    CHECK(data.angle1 < 2.0 * M_PI);
    CHECK(data.angle2 >= 0.0);
    CHECK(data.angle2 < 2.0 * M_PI);
}

TEST_CASE("Offset basis refuses invalid and unrepresentable two-dimensional input", "[lc-offsetcurve][geometry]") {
    for (const double ratio : {0.0, -1.0, std::numeric_limits<double>::infinity()}) {
        auto data = basis(10.0, ratio);
        CHECK_FALSE(geo::normaliseBasis(data));
    }
    auto data = basis();
    data.center.z = 1.0;
    CHECK_FALSE(geo::normaliseBasis(data));
    data = basis();
    data.majorP = RS_Vector{false};
    CHECK_FALSE(geo::normaliseBasis(data));
    data = basis();
    data.angle2 = std::numeric_limits<double>::quiet_NaN();
    CHECK_FALSE(geo::normaliseBasis(data));
    data = basis(1.2e308);
    CHECK_FALSE(geo::normaliseBasis(data));
    data = basis(1e-10);
    data.center = RS_Vector{1e12, 1e12};
    CHECK_FALSE(geo::normaliseBasis(data));
    data = basis();
    data.majorP.z = std::numeric_limits<double>::quiet_NaN();
    CHECK_FALSE(geo::normaliseBasis(data));
}

TEST_CASE("Offset normalization preserves libm endpoints for huge raw angles", "[lc-offsetcurve][geometry]") {
    for (const double ratio : {0.5, 2.0}) {
        for (const double start : {-1e16, 1e16}) {
            for (const bool reverse : {false, true}) {
                auto data = basis(10.0, ratio, start, start + (reverse ? -2.0 : 2.0), reverse);
                data.center = RS_Vector{17.0, -23.0};
                const RS_Vector first = data.center + RS_Vector{10.0 * std::cos(data.angle1),
                                                                10.0 * ratio * std::sin(data.angle1)};
                const RS_Vector last = data.center + RS_Vector{10.0 * std::cos(data.angle2),
                                                               10.0 * ratio * std::sin(data.angle2)};
                REQUIRE(geo::normaliseBasis(data));
                CHECK(data.isArc);
                CHECK(data.reversed == reverse);
                CHECK(geo::ellipsePoint(data, data.angle1).distanceTo(first) <= 1e-11);
                CHECK(geo::ellipsePoint(data, data.angle2).distanceTo(last) <= 1e-11);
                REQUIRE(geo::sweep(data));
                CHECK(std::abs(geo::sweep(data)->signedSweep) == Catch::Approx(2.0).margin(1e-13));
            }
        }
    }
}

TEST_CASE("Offset closed forms preserve small and large finite local scales", "[lc-offsetcurve][geometry]") {
    for (const double scale : {1e-100, 1.0, 1e100}) {
        auto data = basis(scale);
        REQUIRE(geo::normaliseBasis(data));
        const auto n = geo::outwardNormal(data, 0.0);
        REQUIRE(n.valid);
        CHECK(n.x == Catch::Approx(1.0));
        CHECK(n.y == 0.0);
        const auto point = geo::exactOffsetPoint(data, 0.0, scale * 0.1);
        REQUIRE(point.valid);
        CHECK(point.x / scale == Catch::Approx(1.1));
        CHECK(geo::minRadiusOfCurvature(data) / scale == Catch::Approx(0.25));
        CHECK(geo::radiusOfCurvature(data, M_PI_2) / scale == Catch::Approx(2.0));
    }
}

TEST_CASE("Offset point derivative and normal satisfy independent ellipse identities", "[lc-offsetcurve][geometry]") {
    std::mt19937_64 generator{2961};
    std::uniform_real_distribution<double> parameter(0.0, 2.0 * M_PI);
    for (const double ratio : {0.05, 0.2, 0.5, 0.99, 1.0}) {
        auto data = basis(13.0, ratio);
        data.majorP.rotate(0.37);
        data.center = RS_Vector{17.0, -23.0};
        REQUIRE(geo::normaliseBasis(data));
        for (int i = 0; i < 30; ++i) {
            const double eta = parameter(generator);
            const auto p = geo::ellipsePoint(data, eta);
            const auto derivative = geo::ellipseDerivative(data, eta);
            const auto normal = geo::outwardNormal(data, eta);
            REQUIRE(p.valid);
            REQUIRE(derivative.valid);
            REQUIRE(normal.valid);
            CHECK(normal.magnitude() == Catch::Approx(1.0).epsilon(1e-12));
            CHECK(std::abs(dot(normal, derivative)) <= 1e-12 * 13.0);
            CHECK(dot(normal, p - data.center) > 0.0);
            const double expectedRadius = std::pow(std::hypot(13.0 * std::sin(eta), 13.0 * ratio * std::cos(eta)), 3.0) /
                                          (13.0 * 13.0 * ratio);
            CHECK(geo::radiusOfCurvature(data, eta) == Catch::Approx(expectedRadius).epsilon(1e-12));
            for (const double distance : {-2.0, 0.0, 3.0}) {
                const auto offset = geo::exactOffsetPoint(data, eta, distance);
                REQUIRE(offset.valid);
                CHECK(offset.distanceTo(p) == Catch::Approx(std::abs(distance)).margin(1e-12));
            }
        }
        CHECK(geo::isInside(data, data.center));
        CHECK_FALSE(geo::isInside(data, data.center + data.majorP * 2.0));
    }
}

TEST_CASE("Offset curvature avoids underflow before a representable result", "[lc-offsetcurve][geometry]") {
    const auto data = basis(1.0, 1e-250);
    const double radius = geo::radiusOfCurvature(data, 1e-165);
    REQUIRE(std::isfinite(radius));
    CHECK(radius / 1e-245 == Catch::Approx(1.0).epsilon(1e-12));
    CHECK_FALSE(geo::lensTrimAngle(data, 0.0));
    CHECK_FALSE(geo::lensTrimAngle(data, -1.0));
}

TEST_CASE("Offset curvature minimum follows the actual arc sweep", "[lc-offsetcurve][geometry]") {
    for (const auto data : {basis(10.0, 0.5, 0.4, 2.2), basis(10.0, 0.5, 2.2, 0.4, true),
                            basis(10.0, 0.5, 6.0, 0.2), basis(10.0, 1.0)}) {
        const auto interval = geo::sweep(data);
        REQUIRE(interval);
        double sampled = std::numeric_limits<double>::infinity();
        for (int i = 0; i <= 4096; ++i) {
            const double eta = interval->start + interval->signedSweep * i / 4096.0;
            const double speed = std::hypot(10.0 * std::sin(eta), 10.0 * data.ratio * std::cos(eta));
            sampled = std::min(sampled, speed * speed * speed / (100.0 * data.ratio));
        }
        const double minimum = geo::minRadiusOfCurvature(data);
        CHECK(minimum <= sampled + 1e-12);
        CHECK(sampled - minimum <= 1e-4);
    }
}

TEST_CASE("Offset inward lens corners agree with both exact offset occurrences", "[lc-offsetcurve][geometry]") {
    for (const double ratio : {0.05, 0.5, 0.95}) {
        auto data = basis(100.0, ratio);
        const double b = 100.0 * ratio;
        const double minimum = b * ratio;
        for (const double d : {minimum, (minimum + b) * 0.5, b * (1.0 - 1e-8)}) {
            const auto angle = geo::lensTrimAngle(data, d);
            const auto corners = geo::lensCorners(data, d);
            REQUIRE(angle);
            REQUIRE(corners.size() == 2);
            CHECK(corners[0].distanceTo(geo::exactOffsetPoint(data, *angle, -d)) <= 1e-10);
            CHECK(corners[0].distanceTo(geo::exactOffsetPoint(data, -*angle, -d)) <= 1e-10);
            CHECK(corners[1].distanceTo(geo::exactOffsetPoint(data, M_PI - *angle, -d)) <= 1e-10);
        }
        CHECK_FALSE(geo::lensTrimAngle(data, minimum * 0.9));
        CHECK_FALSE(geo::lensTrimAngle(data, b));
    }
    CHECK(geo::lensCorners(basis(10.0, 1.0), 5.0).empty());
}

TEST_CASE("Offset original distance reuses analytic ellipse query in a local frame", "[lc-offsetcurve][geometry]") {
    std::mt19937_64 generator{3112};
    std::uniform_real_distribution<double> coordinate(-30.0, 30.0);
    for (const double ratio : {0.05, 0.5, 1.0}) {
        const auto data = basis(10.0, ratio);
        for (int sample = 0; sample < 8; ++sample) {
            const RS_Vector point{coordinate(generator), coordinate(generator)};
            double dense = std::numeric_limits<double>::infinity();
            for (int i = 0; i < 20000; ++i) {
                const double eta = 2.0 * M_PI * i / 20000.0;
                dense = std::min(dense, std::hypot(point.x - 10.0 * std::cos(eta),
                                                  point.y - 10.0 * ratio * std::sin(eta)));
            }
            double foot = -1.0;
            const double distance = geo::distanceToOriginal(data, point, &foot);
            CHECK(distance <= dense + 1e-10);
            CHECK(dense - distance <= 10.0 * M_PI / 20000.0);
            CHECK(point.distanceTo(geo::ellipsePoint(data, foot)) == Catch::Approx(distance).margin(1e-10));
        }
    }
    double foot = 0.0;
    CHECK(std::isinf(geo::distanceToOriginal(basis(), RS_Vector{false}, &foot)));
    CHECK(std::isnan(foot));
}

TEST_CASE("Offset tolerance helper agrees with existing engine options", "[lc-offsetcurve][geometry]") {
    for (const double scale : {0.001, 10.0, 1e6}) {
        for (const bool arc : {false, true}) {
            const RS_Ellipse source{nullptr, arc ? basis(scale, 0.5, 0.3, 4.0) : basis(scale)};
            const double factor = LC_CurveOffset::relativeOffsetTolerance(source);
            CHECK(factor == LC_CurveOffset::kEllipseRelativeOffsetTolerance);
            for (const double distance : {scale * 0.01, scale * 3.0}) {
                const double tolerance = LC_CurveOffset::toleranceFor(source, distance, factor);
                CHECK(tolerance == LC_CurveOffset::makeOffsetOptions(source, distance).tolerance.requestedGeometry);
                CHECK(LC_CurveOffset::makeOffsetOptions(source, distance, tolerance).tolerance.requestedGeometry == tolerance);
            }
        }
    }
    const RS_Ellipse source{nullptr, basis()};
    CHECK(std::isnan(LC_CurveOffset::toleranceFor(source, 0.0, 1e-4)));
    CHECK(std::isnan(LC_CurveOffset::toleranceFor(source, 1.0, -1.0)));
    CHECK(std::isnan(LC_CurveOffset::toleranceFor(source, 1.0, std::numeric_limits<double>::infinity())));
    const RS_Line line{nullptr, RS_LineData{RS_Vector{0.0, 0.0}, RS_Vector{1.0, 0.0}}};
    CHECK(LC_CurveOffset::relativeOffsetTolerance(line) == LC_CurveOffset::kDefaultRelativeOffsetTolerance);
    CHECK(std::isnan(LC_CurveOffset::toleranceFor(line, 1.0, 1e-6)));

    auto distantData = basis();
    distantData.center = RS_Vector{1e12, -1e12};
    const RS_Ellipse distant{nullptr, distantData};
    const double floor = LC_CurveOffset::toleranceFor(distant, 1.0, 1e-4);
    CHECK(std::isfinite(floor));
    CHECK(floor > 0.01);
    CHECK(std::isnan(LC_CurveOffset::makeOffsetOptions(distant, 1.0).tolerance.requestedGeometry));
    CHECK(LC_CurveOffset::toleranceFor(distant, 1.0, 1e-4) == floor); // validation can still assess a carried cache
}

TEST_CASE("Offset original distance cannot accept a far query as its own foot", "[lc-offsetcurve][geometry]") {
    for (const double scale : {1e-100, 10.0, 1e100}) {
        for (const double ratio : {0.05, 0.5, 1.0}) {
            const auto data = basis(scale, ratio);
            for (const auto direction : {RS_Vector{1.0, 0.0}, RS_Vector{0.0, 1.0}, RS_Vector{-0.6, 0.8}}) {
                const RS_Vector query = direction * (scale * 1e11);
                double parameter = -1.0;
                const double distance = geo::distanceToOriginal(data, query, &parameter);
                REQUIRE(std::isfinite(distance));
                CHECK(distance > scale * (1e11 - 2.0));
                CHECK(distance <= scale * 1e11);
                CHECK(query.distanceTo(geo::ellipsePoint(data, parameter)) == Catch::Approx(distance).epsilon(1e-12));
                const auto foot = geo::ellipsePoint(data, parameter);
                CHECK(std::hypot(foot.x / scale, foot.y / (scale * ratio)) == Catch::Approx(1.0).epsilon(1e-12));
            }
        }
    }
    const auto thin = basis(1.0, 1e-250);
    double parameter = 0.0;
    const double distance = geo::distanceToOriginal(thin, RS_Vector{0.5, 1e12}, &parameter);
    REQUIRE(std::isfinite(distance));
    CHECK(distance == Catch::Approx(1e12));
    CHECK(std::cos(parameter) == Catch::Approx(0.5).margin(1e-12));
}

TEST_CASE("Offset closed forms reject nonfinite results", "[lc-offsetcurve][geometry]") {
    const double infinity = std::numeric_limits<double>::infinity();
    CHECK_FALSE(geo::ellipsePoint(basis(), infinity).valid);
    CHECK_FALSE(geo::ellipseDerivative(basis(), infinity).valid);
    CHECK_FALSE(geo::outwardNormal(basis(), infinity).valid);
    CHECK_FALSE(geo::exactOffsetPoint(basis(), 0.0, infinity).valid);
    CHECK(std::isnan(geo::radiusOfCurvature(basis(), infinity)));
    CHECK_FALSE(geo::isInside(basis(), RS_Vector{infinity, 0.0}));
    CHECK_FALSE(geo::lensTrimAngle(basis(), infinity));
    auto invalid = basis();
    invalid.ratio = 0.0;
    CHECK_FALSE(geo::outwardNormal(invalid, 0.0).valid);
    CHECK(std::isnan(geo::minRadiusOfCurvature(invalid)));
}

TEST_CASE("Offset original distance preserves thin-query quadrant and tiny distances", "[lc-offsetcurve][geometry]") {
    for (const double ratio : {1e-20, 1e-160, 1e-250}) {
        const auto data = basis(1.0, ratio);
        const double expected = (1.0 - std::sqrt(0.75)) * ratio;
        for (const double signX : {-1.0, 1.0}) {
            double plusFoot = 0.0;
            double minusFoot = 0.0;
            const double plus = geo::distanceToOriginal(data, RS_Vector{signX * 0.5, ratio}, &plusFoot);
            const double minus = geo::distanceToOriginal(data, RS_Vector{signX * 0.5, -ratio}, &minusFoot);
            CHECK(plus / expected == Catch::Approx(1.0).epsilon(1e-12));
            CHECK(minus / expected == Catch::Approx(1.0).epsilon(1e-12));
            CHECK(std::sin(plusFoot) > 0.0);
            CHECK(std::sin(minusFoot) < 0.0);
            for (const double signY : {-1.0, 1.0}) {
                const double atVertex = geo::distanceToOriginal(data, RS_Vector{signX, signY * ratio});
                REQUIRE(std::isfinite(atVertex));
                CHECK(atVertex / ratio == Catch::Approx(1.0).epsilon(1e-12));
            }
        }
    }
}

TEST_CASE("Offset original distance scales before overflowing intermediate coordinates", "[lc-offsetcurve][geometry]") {
    double parameter = 0.0;
    CHECK(geo::distanceToOriginal(basis(1e-300, 1.0), RS_Vector{1e10, 0.0}, &parameter) == Catch::Approx(1e10));
    CHECK(parameter == 0.0);
    auto data = basis(6e307, 1.0);
    data.center = RS_Vector{-1e308, 0.0};
    const double distance = geo::distanceToOriginal(data, RS_Vector{1e308, 0.0}, &parameter);
    REQUIRE(std::isfinite(distance));
    CHECK(distance / 1e308 == Catch::Approx(1.4).epsilon(1e-12));
    CHECK(parameter == 0.0);
}

TEST_CASE("Reference frames enclose independent dense source and offset jets at every scale",
          "[lc-offsetcurve][geometry][reference-frame]") {
    for (const double a : {1e-280, 10.0, 1e280}) {
        for (const double ratio : {0.02, 0.5, 1.0}) {
            for (const bool reversed : {false, true}) {
                for (const double distanceFactor : {-0.25, 0.2}) {
                    CAPTURE(a, ratio, reversed, distanceFactor);
                    auto data = basis(a, ratio, 0.0, 0.0, reversed);
                    data.majorP = RS_Vector{0.6 * a, 0.8 * a};
                    const double distance = distanceFactor * a;
                    const auto frame = geo::ReferenceFrame::create(data, distance);
                    REQUIRE(frame);
                    REQUIRE(frame->scale() > 0.0);
                    REQUIRE(std::isfinite(frame->scale()));
                    for (int window = 0; window < 12; ++window) {
                        const double from = frame->parameterEnd() * (static_cast<double>(window) / 12.0);
                        const double to = frame->parameterEnd() * (static_cast<double>(window + 1) / 12.0);
                        LC_CurveJetBounds source;
                        LC_CurveJetBounds offset;
                        REQUIRE(frame->boundSourceJet(from, to, source));
                        REQUIRE(frame->boundOffsetJet(from, to, offset));
                        for (int sample = 0; sample <= 8; ++sample) {
                            const double t = sample == 0 ? from : sample == 8 ? to :
                                from + (to - from) * (static_cast<double>(sample) / 8.0);
                            CAPTURE(window, sample, t);
                            checkEnclosure(source, referenceOracle(data, *frame, distance, t, false));
                            checkEnclosure(offset, referenceOracle(data, *frame, distance, t, true));
                        }
                        const double middle = from + 0.5 * (to - from);
                        REQUIRE(frame->boundSourceJet(middle, middle, source));
                        REQUIRE(frame->boundOffsetJet(middle, middle, offset));
                        checkEnclosure(source, referenceOracle(data, *frame, distance, middle, false));
                        checkEnclosure(offset, referenceOracle(data, *frame, distance, middle, true));
                    }
                }
            }
        }
    }
}

TEST_CASE("Reference frames preserve traversal signs wrapped arcs and the canonical whole seam",
          "[lc-offsetcurve][geometry][reference-frame]") {
    for (const bool reversed : {false, true}) {
        for (const bool whole : {false, true}) {
            auto data = whole ? basis(10.0, 0.5, 0.7, 0.7 + (reversed ? -2.0 : 2.0) * M_PI, reversed) :
                reversed ? basis(10.0, 0.5, 0.2, 6.0, true) : basis(10.0, 0.5, 6.0, 0.2);
            const auto frame = geo::ReferenceFrame::create(data, -1.0);
            REQUIRE(frame);
            CHECK(frame->sourceSweep().whole == whole);
            CHECK((frame->sourceSweep().signedSweep < 0.0) == reversed);
            CHECK(frame->engineSignedDistance() == (reversed ? -1.0 : 1.0));
            if (whole) {
                CHECK(frame->sourceSweep().start == M_PI_4);
                CHECK(frame->parameterEnd() == 2.0 * M_PI);
                LC_CurveJet first;
                LC_CurveJet last;
                REQUIRE(frame->sourceJet(0.0, first));
                REQUIRE(frame->sourceJet(frame->parameterEnd(), last));
                checkSameJet(first, last);
                REQUIRE(frame->offsetJet(0.0, first));
                REQUIRE(frame->offsetJet(frame->parameterEnd(), last));
                checkSameJet(first, last);
                LC_CurveJetBounds source;
                LC_CurveJetBounds offset;
                REQUIRE(frame->boundSourceJet(frame->parameterEnd() - 0.01, frame->parameterEnd(), source));
                REQUIRE(frame->boundOffsetJet(frame->parameterEnd() - 0.01, frame->parameterEnd(), offset));
                for (const bool canonical : {false, true}) {
                    checkEnclosure(source, referenceOracle(data, *frame, -1.0, frame->parameterEnd(), false, canonical));
                    checkEnclosure(offset, referenceOracle(data, *frame, -1.0, frame->parameterEnd(), true, canonical));
                }
            } else {
                CHECK(frame->parameterEnd() < 0.5);
                for (int k = 0; k <= 16; ++k) {
                    const double t = frame->parameterEnd() * (static_cast<double>(k) / 16.0);
                    LC_CurveJetBounds source;
                    LC_CurveJetBounds offset;
                    REQUIRE(frame->boundSourceJet(t, t, source));
                    REQUIRE(frame->boundOffsetJet(t, t, offset));
                    checkEnclosure(source, referenceOracle(data, *frame, -1.0, t, false));
                    checkEnclosure(offset, referenceOracle(data, *frame, -1.0, t, true));
                }
            }
        }
    }
}

TEST_CASE("Reference offset bounds preserve circle cancellation near circles and cusp zeros",
          "[lc-offsetcurve][geometry][reference-frame]") {
    for (const double ratio : {1.0, std::nextafter(1.0, 0.0), 1e-8}) {
        const auto data = basis(100.0, ratio);
        const auto frame = geo::ReferenceFrame::create(data, -99.99);
        REQUIRE(frame);
        for (int k = 0; k <= 64; ++k) {
            const double t = frame->parameterEnd() * (static_cast<double>(k) / 64.0);
            LC_CurveJetBounds bounds;
            REQUIRE(frame->boundOffsetJet(t, t, bounds));
            checkEnclosure(bounds, referenceOracle(data, *frame, -99.99, t, true));
        }
        if (ratio == 1.0) {
            LC_CurveJetBounds bounds;
            REQUIRE(frame->boundOffsetJet(0.0, frame->parameterEnd(), bounds));
            CHECK(std::max(std::abs(bounds.x.lo()), std::abs(bounds.x.hi())) < 0.001);
            CHECK(std::max(std::abs(bounds.y.lo()), std::abs(bounds.y.hi())) < 0.001);
            REQUIRE(frame->boundOffsetJet(0.0, 0.0, bounds));
            LC_CurveJet estimate;
            REQUIRE(frame->offsetJet(0.0, estimate));
            // Evaluation uncertainty is governed by the original radius, not only the tiny offset result.
            CHECK(bounds.x.width() > 64.0 * std::numeric_limits<double>::epsilon() * std::abs(estimate.point.x));
        }
    }
    const auto cusp = geo::ReferenceFrame::create(basis(100.0, 0.5, 0.0, 1.0), -25.0);
    REQUIRE(cusp);
    LC_CurveJetBounds bounds;
    REQUIRE(cusp->boundOffsetJet(0.0, 0.0, bounds));
    CHECK(bounds.dx.containsZero());
    CHECK(bounds.dy.containsZero());
    CHECK(bounds.ddx.containsZero());
    CHECK(bounds.ddy.containsZero());
    LC_CurveJet jet;
    REQUIRE(cusp->offsetJet(0.0, jet));
    CHECK(jet.first.magnitude() < 1e-12);
    const auto collapsed = geo::ReferenceFrame::create(basis(100.0, 1.0), -100.0);
    REQUIRE(collapsed);
    REQUIRE(collapsed->boundOffsetJet(0.0, collapsed->parameterEnd(), bounds));
    for (const LC_Interval interval : {bounds.x, bounds.y, bounds.dx, bounds.dy, bounds.ddx, bounds.ddy,
                                       bounds.speedSquaredProduct, bounds.crossProduct}) {
        CHECK(interval.containsZero());
    }
    REQUIRE(collapsed->offsetJet(0.0, jet));
    CHECK(jet.point.magnitude() < 1e-12);
    CHECK(jet.first.magnitude() < 1e-12);
    CHECK(jet.second.magnitude() < 1e-12);
}

TEST_CASE("Reference frame conversions enclose exact input differences and refuse unbounded outputs",
          "[lc-offsetcurve][geometry][reference-frame]") {
    auto data = basis(10.0, 0.5);
    data.center = RS_Vector{1e12, -1e12};
    data.majorP = RS_Vector{6.0, 8.0};
    const auto frame = geo::ReferenceFrame::create(data, 1.0);
    REQUIRE(frame);
    LC_Interval x;
    LC_Interval y;
    const RS_Vector world{data.center.x + 0.125, data.center.y - 0.25};
    REQUIRE(frame->pointToFrame(world, x, y));
    checkEnclosure(x, (Quad(world.x) - Quad(data.center.x)) / Quad(frame->scale()));
    checkEnclosure(y, (Quad(world.y) - Quad(data.center.y)) / Quad(frame->scale()));
    const RS_Vector local{0.375, -0.625};
    REQUIRE(frame->pointToWorld(local, x, y));
    checkEnclosure(x, Quad(data.center.x) + Quad(local.x) * Quad(frame->scale()));
    checkEnclosure(y, Quad(data.center.y) + Quad(local.y) * Quad(frame->scale()));

    data = basis(6e307, 0.5);
    data.center = RS_Vector{-1e308, 0.0};
    const auto distant = geo::ReferenceFrame::create(data, 0.0);
    REQUIRE(distant);
    const RS_Vector opposite{1e308, 0.0};
    REQUIRE_FALSE(std::isfinite(opposite.x - data.center.x));
    REQUIRE(distant->pointToFrame(opposite, x, y));
    checkEnclosure(x, (Quad(opposite.x) - Quad(data.center.x)) / Quad(distant->scale()));
    CHECK(y.containsZero());
    for (const double localX : {-8.0, -2.0}) {
        REQUIRE_FALSE(distant->pointToWorld({localX, 0.0}, x, y));
        CHECK_FALSE(x.isValid());
        CHECK_FALSE(y.isValid());
    }
    REQUIRE_FALSE(frame->pointToFrame({std::numeric_limits<double>::infinity(), 0.0}, x, y));
    CHECK_FALSE(x.isValid());
    CHECK_FALSE(y.isValid());
    REQUIRE_FALSE(frame->pointToWorld({0.0, 0.0, 1.0}, x, y));
    CHECK_FALSE(x.isValid());
    CHECK_FALSE(y.isValid());

    const auto tinyData = basis(1e-310, 1.0);
    const auto tiny = geo::ReferenceFrame::create(tinyData, -5e-311);
    REQUIRE(tiny);
    REQUIRE(std::fpclassify(tiny->scale()) == FP_SUBNORMAL);
    LC_CurveJetBounds bounds;
    REQUIRE(tiny->boundOffsetJet(0.0, 0.0, bounds));
    checkEnclosure(bounds, referenceOracle(tinyData, *tiny, -5e-311, 0.0, true));
    const double least = std::numeric_limits<double>::denorm_min();
    REQUIRE(least * tiny->scale() == 0.0);
    REQUIRE(tiny->pointToWorld({least, 0.0}, x, y));
    checkEnclosure(x, Quad(least) * Quad(tiny->scale()));
    CHECK(x.width() > 0.0); // underflow is enclosed, not silently rounded into an exact zero
    REQUIRE(tiny->pointToFrame({least, 0.0}, x, y));
    checkEnclosure(x, Quad(least) / Quad(tiny->scale()));
    int scaleExponent = 0;
    std::frexp(tiny->scale(), &scaleExponent);
    const double normalBoundary = std::scalbn(1.0, std::numeric_limits<double>::min_exponent - scaleExponent);
    const double justBelow = std::nextafter(normalBoundary, 0.0);
    REQUIRE(tiny->pointToWorld({justBelow, 0.0}, x, y));
    checkEnclosure(x, Quad(justBelow) * Quad(tiny->scale()));
}

TEST_CASE("Reference factories and failed jet queries never publish usable partial geometry",
          "[lc-offsetcurve][geometry][reference-frame]") {
    CHECK_FALSE(geo::ReferenceFrame::create(basis(10.0, 2.0), 1.0));
    CHECK_FALSE(geo::ReferenceFrame::create(basis(10.0, 0.5, -0.1, 0.2), 1.0));
    CHECK_FALSE(geo::ReferenceFrame::create(basis(10.0, 0.5, 0.7, 0.7), 1.0));
    CHECK_FALSE(geo::ReferenceFrame::create(basis(), std::numeric_limits<double>::quiet_NaN()));
    auto invalid = basis();
    invalid.majorP.z = 1.0;
    CHECK_FALSE(geo::ReferenceFrame::create(invalid, 1.0));
    CHECK_FALSE(geo::ReferenceFrame::create(basis(1e-300), 1e300)); // source lost at this scale
    const auto frame = geo::ReferenceFrame::create(basis(), 1.0);
    REQUIRE(frame);
    for (const double t : {-1.0, frame->parameterEnd() + 1.0, std::numeric_limits<double>::infinity(),
                           std::numeric_limits<double>::quiet_NaN()}) {
        LC_CurveJetBounds bounds;
        REQUIRE(frame->boundSourceJet(0.0, 0.1, bounds));
        CHECK_FALSE(frame->boundSourceJet(t, t, bounds));
        checkCleared(bounds);
        REQUIRE(frame->boundOffsetJet(0.0, 0.1, bounds));
        CHECK_FALSE(frame->boundOffsetJet(t, t, bounds));
        checkCleared(bounds);
        LC_CurveJet jet;
        REQUIRE(frame->sourceJet(0.0, jet));
        CHECK_FALSE(frame->sourceJet(t, jet));
        CHECK_FALSE(jet.point.valid);
        CHECK_FALSE(jet.first.valid);
        CHECK_FALSE(jet.second.valid);
        REQUIRE(frame->offsetJet(0.0, jet));
        CHECK_FALSE(frame->offsetJet(t, jet));
        CHECK_FALSE(jet.point.valid);
        CHECK_FALSE(jet.first.valid);
        CHECK_FALSE(jet.second.valid);
    }
    LC_CurveJetBounds bounds;
    CHECK_FALSE(frame->boundOffsetJet(0.2, 0.1, bounds));
    checkCleared(bounds);
    const auto thin = geo::ReferenceFrame::create(basis(1.0, 1e-250, 0.0, 1.0), -0.25);
    REQUIRE(thin);
    REQUIRE(thin->boundSourceJet(0.0, 0.0, bounds));
    CHECK_FALSE(thin->boundOffsetJet(0.0, 0.0, bounds)); // cannot prove a positive representable denominator
    checkCleared(bounds);
    LC_CurveJet jet;
    CHECK_FALSE(thin->offsetJet(0.0, jet));
    CHECK_FALSE(jet.point.valid);
    CHECK_FALSE(jet.first.valid);
    CHECK_FALSE(jet.second.valid);
}

TEST_CASE("Reference rates enclose raw endpoint sweeps without reducing unwrapped angles",
          "[lc-offsetcurve][geometry][reference-rate]") {
    for (const auto& data : rateSources()) {
        CAPTURE(data.angle1, data.angle2, data.reversed);
        const auto domain = geo::sweep(data);
        const auto frame = geo::ReferenceFrame::create(data, -25.0);
        REQUIRE(domain);
        REQUIRE(frame);
        CHECK(frame->parameterEnd() == std::abs(domain->signedSweep));
        CHECK(frame->sourceSweep().start == domain->start);
        CHECK(frame->sourceSweep().signedSweep == domain->signedSweep);
        CHECK(frame->sourceSweep().whole == domain->whole);
        CHECK(frame->engineSignedDistance() == (data.reversed ? -25.0 : 25.0));
        const Quad directed = idealDirectedSweep(data, domain->whole);
        const Quad rate = abs(directed) / Quad(frame->parameterEnd());
        checkEnclosure(frame->traversalRate(), rate);
        REQUIRE(frame->traversalRate().isPositive());
        const Quad start = domain->whole ? Quad(M_PI_4) : Quad(data.angle1);
        for (int window = 0; window < 8; ++window) {
            const double from = frame->parameterEnd() * (static_cast<double>(window) / 8.0);
            const double to = frame->parameterEnd() * (static_cast<double>(window + 1) / 8.0);
            LC_Interval angle;
            REQUIRE(frame->boundUnwrappedAngle(from, to, angle));
            for (const double t : {from, from + 0.5 * (to - from), to}) {
                const Quad expected = start + directed * Quad(t) / Quad(frame->parameterEnd());
                checkEnclosure(angle, expected);
                LC_Interval pointAngle;
                REQUIRE(frame->boundUnwrappedAngle(t, t, pointAngle));
                checkEnclosure(pointAngle, expected);
            }
        }
        LC_Interval end;
        REQUIRE(frame->boundUnwrappedAngle(frame->parameterEnd(), frame->parameterEnd(), end));
        checkEnclosure(end, start + directed);
        if (domain->whole) {
            CHECK_FALSE(end.contains(domain->start));
        } else if (!data.reversed && data.angle2 < data.angle1) {
            CHECK(end.lo() > data.angle2);
        } else if (data.reversed && data.angle2 > data.angle1) {
            CHECK(end.hi() < data.angle2);
        } else {
            CHECK(end.contains(data.angle2));
        }
    }
}

TEST_CASE("Exact sweep jets and products enclose dense independent arc and whole formulas",
          "[lc-offsetcurve][geometry][reference-rate]") {
    for (auto data : rateSources()) {
        for (const double ratio : {0.02, 0.5, 1.0}) {
            data.ratio = ratio;
            data.majorP = RS_Vector{60.0, 80.0};
            data.center = RS_Vector{1e200, -1e200};
            for (const double delta : {0.0, -25.0, 20.0}) {
                CAPTURE(data.angle1, data.angle2, data.reversed, ratio, delta);
                const auto frame = geo::ReferenceFrame::create(data, delta);
                REQUIRE(frame);
                for (int window = 0; window < 8; ++window) {
                    const double from = frame->parameterEnd() * (static_cast<double>(window) / 8.0);
                    const double to = frame->parameterEnd() * (static_cast<double>(window + 1) / 8.0);
                    LC_CurveJetBounds source;
                    LC_CurveJetBounds offset;
                    REQUIRE(frame->boundSourceJet(from, to, source));
                    REQUIRE(frame->boundOffsetJet(from, to, offset));
                    for (const double t : {from, from + 0.5 * (to - from), to}) {
                        checkEnclosure(source, referenceOracle(data, *frame, delta, t, false, false));
                        checkEnclosure(offset, referenceOracle(data, *frame, delta, t, true, false));
                    }
                    const double middle = from + 0.5 * (to - from);
                    REQUIRE(frame->boundSourceJet(middle, middle, source));
                    REQUIRE(frame->boundOffsetJet(middle, middle, offset));
                    checkEnclosure(source, referenceOracle(data, *frame, delta, middle, false));
                    checkEnclosure(offset, referenceOracle(data, *frame, delta, middle, true));
                    if (delta == 0.0) {
                        const std::array<LC_Interval, 8> a{{source.x, source.y, source.dx, source.dy,
                            source.ddx, source.ddy, source.speedSquaredProduct, source.crossProduct}};
                        const std::array<LC_Interval, 8> b{{offset.x, offset.y, offset.dx, offset.dy,
                            offset.ddx, offset.ddy, offset.speedSquaredProduct, offset.crossProduct}};
                        for (std::size_t k = 0; k < a.size(); ++k) {
                            CHECK(a[k].lo() == b[k].lo());
                            CHECK(a[k].hi() == b[k].hi());
                        }
                    }
                }
            }
        }
    }
}

TEST_CASE("Tiny real-period wraps distinguish every rate factor from unit-rate jets",
          "[lc-offsetcurve][geometry][reference-rate]") {
    for (const bool reversed : {false, true}) {
        const double nearPeriod = 2.0 * M_PI - 2.0 * RS_TOLERANCE_ANGLE;
        for (const double a : {1e-280, 100.0, 1e280}) {
            const auto data = reversed ? basis(a, 0.5, 0.0, nearPeriod, true) :
                basis(a, 0.5, nearPeriod, 0.0);
            for (const double delta : {0.0, 0.2 * a}) {
                CAPTURE(reversed, a, delta);
                const auto frame = geo::ReferenceFrame::create(data, delta);
                REQUIRE(frame);
                REQUIRE_FALSE(frame->sourceSweep().whole);
                REQUIRE(frame->parameterEnd() > RS_TOLERANCE_ANGLE);
                const Quad rate = abs(idealDirectedSweep(data, false)) / Quad(frame->parameterEnd());
                CHECK(rate - 1 > Quad("1e-9"));
                checkEnclosure(frame->traversalRate(), rate);
                const double t = 0.5 * frame->parameterEnd();
                LC_CurveJetBounds bounds;
                REQUIRE(frame->boundOffsetJet(t, t, bounds));
                const QuadJet exact = referenceOracle(data, *frame, delta, t, true);
                checkEnclosure(bounds, exact);
                const QuadJet unit = referenceOracle(data, *frame, delta, t, true, false, true);
                // Missing rate factors cannot hide inside ordinary component-rounding padding.
                // Correct outward bounds need not exclude the unit-rate values as well.
                CHECK(abs(exact[3] - unit[3]) > abs(unit[3]) * Quad("1e-9"));
                CHECK(abs(exact[4] - unit[4]) > abs(unit[4]) * Quad("1e-9"));
                const Quad unitSpeedSquared = unit[2] * unit[2] + unit[3] * unit[3];
                const Quad unitCross = unit[2] * unit[5] - unit[3] * unit[4];
                CHECK(abs(exact[2] * exact[2] + exact[3] * exact[3] - unitSpeedSquared) >
                      abs(unitSpeedSquared) * Quad("1e-9"));
                CHECK(abs(exact[2] * exact[5] - exact[3] * exact[4] - unitCross) >
                      abs(unitCross) * Quad("1e-9"));
                for (const double endpoint : {0.0, frame->parameterEnd()}) {
                    REQUIRE(frame->boundOffsetJet(endpoint, endpoint, bounds));
                    checkEnclosure(bounds, referenceOracle(data, *frame, delta, endpoint, true));
                    checkEnclosure(bounds, referenceOracle(data, *frame, delta, endpoint, true, false));
                }
            }
        }
    }
}

TEST_CASE("Exact whole and arc endpoints agree with their continuous one-sided jet limits",
          "[lc-offsetcurve][geometry][reference-rate]") {
    for (auto data : rateSources()) {
        for (const double ratio : {0.5, 1.0}) {
            data.ratio = ratio;
            data.majorP = RS_Vector{60.0, 80.0};
            for (const double delta : {0.0, -99.99, 20.0}) {
                CAPTURE(data.angle1, data.angle2, data.reversed, ratio, delta);
                const auto frame = geo::ReferenceFrame::create(data, delta);
                REQUIRE(frame);
                const double end = frame->parameterEnd();
                const double before = std::nextafter(end, 0.0);
                const double after = std::nextafter(0.0, end);
                for (const bool offset : {false, true}) {
                    LC_CurveJetBounds atStart;
                    LC_CurveJetBounds startLimit;
                    LC_CurveJetBounds atEnd;
                    LC_CurveJetBounds limit;
                    REQUIRE((offset ? frame->boundOffsetJet(0.0, 0.0, atStart) :
                                      frame->boundSourceJet(0.0, 0.0, atStart)));
                    REQUIRE((offset ? frame->boundOffsetJet(0.0, after, startLimit) :
                                      frame->boundSourceJet(0.0, after, startLimit)));
                    REQUIRE((offset ? frame->boundOffsetJet(end, end, atEnd) :
                                      frame->boundSourceJet(end, end, atEnd)));
                    REQUIRE((offset ? frame->boundOffsetJet(before, end, limit) :
                                      frame->boundSourceJet(before, end, limit)));
                    const QuadJet canonical = referenceOracle(data, *frame, delta, end, offset);
                    const QuadJet continuous = referenceOracle(data, *frame, delta, end, offset, false);
                    const QuadJet first = referenceOracle(data, *frame, delta, 0.0, offset);
                    checkEnclosure(atStart, first);
                    checkEnclosure(startLimit, first);
                    checkEnclosure(startLimit, referenceOracle(data, *frame, delta, after, offset, false));
                    checkEnclosure(atEnd, canonical);
                    checkEnclosure(atEnd, continuous);
                    checkEnclosure(limit, continuous);
                    checkEnclosure(limit, referenceOracle(data, *frame, delta, before, offset, false));
                    for (std::size_t k = 0; k < canonical.size(); ++k) {
                        CHECK(abs(canonical[k] - continuous[k]) < Quad("1e-29"));
                    }
                    if (frame->sourceSweep().whole) {
                        for (std::size_t k = 0; k < first.size(); ++k) {
                            CHECK(abs(first[k] - continuous[k]) < Quad("1e-29"));
                        }
                        LC_CurveJet startJet;
                        LC_CurveJet endJet;
                        REQUIRE((offset ? frame->offsetJet(0.0, startJet) : frame->sourceJet(0.0, startJet)));
                        REQUIRE((offset ? frame->offsetJet(end, endJet) : frame->sourceJet(end, endJet)));
                        checkSameJet(startJet, endJet);
                    }
                }
            }
        }
    }
}

TEST_CASE("Exact reference versus unit-rate geometry has an outward Lipschitz discrepancy bound",
          "[lc-offsetcurve][geometry][reference-rate]") {
    for (const auto& data : rateSources()) {
        for (const double delta : {-25.0, 0.0, 20.0}) {
            CAPTURE(data.angle1, data.angle2, data.reversed, delta);
            const auto frame = geo::ReferenceFrame::create(data, delta);
            REQUIRE(frame);
            const LC_Interval mx = LC_Interval::point(data.majorP.x) / LC_Interval::point(frame->scale());
            const LC_Interval my = LC_Interval::point(data.majorP.y) / LC_Interval::point(frame->scale());
            const LC_Interval axis = sqrt(sqr(mx) + sqr(my));
            const LC_Interval d = LC_Interval::point(std::abs(delta)) / LC_Interval::point(frame->scale());
            const LC_Interval lipschitz = axis + d / LC_Interval::point(data.ratio);
            const LC_Interval difference = frame->traversalRate() - LC_Interval::point(1.0);
            const double rateError = std::max(std::abs(difference.lo()), std::abs(difference.hi()));
            // |Q'_eta| <= a + |delta|/ratio; integrate over at most T*|gamma-1|.
            const LC_Interval discrepancy = lipschitz * LC_Interval::point(frame->parameterEnd()) *
                LC_Interval::point(rateError);
            REQUIRE(std::isfinite(discrepancy.hi()));
            CHECK(discrepancy.hi() < 1e-12);
            auto localData = data;
            localData.center = RS_Vector{0.0, 0.0};
            localData.majorP = RS_Vector{data.majorP.x / frame->scale(), data.majorP.y / frame->scale()};
            for (int k = 0; k <= 32; ++k) {
                const double t = frame->parameterEnd() * (static_cast<double>(k) / 32.0);
                const QuadJet exact = referenceOracle(data, *frame, delta, t, true, false);
                const QuadJet unit = referenceOracle(data, *frame, delta, t, true, false, true);
                const Quad dx = exact[0] - unit[0];
                const Quad dy = exact[1] - unit[1];
                CHECK(sqrt(dx * dx + dy * dy) <= Quad(discrepancy.hi()));
                const double angle = frame->sourceSweep().start + (data.reversed ? -t : t);
                const RS_Vector old = geo::exactOffsetPoint(localData, angle, delta / frame->scale());
                REQUIRE(geo::isFinite2D(old));
                const Quad errorX = Quad(old.x) - unit[0];
                const Quad errorY = Quad(old.y) - unit[1];
                // Empirical libm regression only: this is NOT part of the certified bound above.
                const Quad floatingError = sqrt(errorX * errorX + errorY * errorY);
                CHECK(floatingError < Quad(64.0 * std::numeric_limits<double>::epsilon() * lipschitz.hi()));
                const Quad actualX = Quad(old.x) - exact[0];
                const Quad actualY = Quad(old.y) - exact[1];
                CHECK(sqrt(actualX * actualX + actualY * actualY) <= Quad(discrepancy.hi()) + floatingError);
            }
        }
    }
}

TEST_CASE("Invalid exact-sweep queries clear angle and jet outputs",
          "[lc-offsetcurve][geometry][reference-rate]") {
    const auto frame = geo::ReferenceFrame::create(basis(10.0, 0.5, 6.0, 0.2), 1.0);
    REQUIRE(frame);
    const double end = frame->parameterEnd();
    for (const auto range : std::array<std::array<double, 2>, 7>{{
             {{-1.0, 0.0}}, {{0.0, std::nextafter(end, std::numeric_limits<double>::infinity())}},
             {{end, 0.0}}, {{0.0, std::numeric_limits<double>::infinity()}},
             {{std::numeric_limits<double>::quiet_NaN(), 0.0}},
             {{0.0, std::numeric_limits<double>::quiet_NaN()}},
             {{-std::numeric_limits<double>::infinity(), 0.0}}}}) {
        LC_Interval angle = LC_Interval::point(123.0);
        CHECK_FALSE(frame->boundUnwrappedAngle(range[0], range[1], angle));
        CHECK_FALSE(angle.isValid());
        LC_CurveJetBounds bounds;
        REQUIRE(frame->boundSourceJet(0.0, 0.0, bounds));
        CHECK_FALSE(frame->boundSourceJet(range[0], range[1], bounds));
        checkCleared(bounds);
        REQUIRE(frame->boundOffsetJet(0.0, 0.0, bounds));
        CHECK_FALSE(frame->boundOffsetJet(range[0], range[1], bounds));
        checkCleared(bounds);
    }
    CHECK_FALSE(geo::ReferenceFrame::create(basis(10.0, 0.5, 0.0, 0.5 * RS_TOLERANCE_ANGLE), 0.0));
    CHECK_FALSE(geo::ReferenceFrame::create(basis(10.0, 0.5, 0.0, 2.0 * M_PI + 1e-6), 0.0));
    const auto collapsed = geo::ReferenceFrame::create(basis(100.0, 1.0), -100.0);
    REQUIRE(collapsed);
    LC_CurveJetBounds bounds;
    REQUIRE(collapsed->boundOffsetJet(0.0, collapsed->parameterEnd(), bounds));
    for (const LC_Interval value : {bounds.x, bounds.y, bounds.dx, bounds.dy, bounds.ddx, bounds.ddy,
                                    bounds.speedSquaredProduct, bounds.crossProduct}) {
        CHECK(value.containsZero());
    }
}

TEST_CASE("Critical factors enclose independent physical curvature and traversal derivatives",
          "[lc-offsetcurve][geometry][reference-critical]") {
    for (auto data : rateSources()) {
        for (const double ratio : {0.02, 0.5, 1.0}) {
            data.ratio = ratio;
            data.majorP = RS_Vector{60.0, 80.0};
            for (const double delta : {-99.99, 0.0, 20.0}) {
                CAPTURE(data.angle1, data.angle2, data.reversed, ratio, delta);
                const auto frame = geo::ReferenceFrame::create(data, delta);
                REQUIRE(frame);
                for (int k = 0; k <= 32; ++k) {
                    const double t = frame->parameterEnd() * (static_cast<double>(k) / 32.0);
                    LC_Interval f, ft;
                    REQUIRE(frame->boundOffsetFactor(t, t, f, ft));
                    const auto oracle = factorOracle(data, *frame, delta, t);
                    checkEnclosure(f, oracle[0]);
                    checkEnclosure(ft, oracle[1]);
                }
                LC_Interval f, ft;
                REQUIRE(frame->boundOffsetFactor(0.0, frame->parameterEnd(), f, ft));
                for (int k = 0; k <= 16; ++k) {
                    const auto oracle = factorOracle(data, *frame, delta,
                        frame->parameterEnd() * (static_cast<double>(k) / 16.0));
                    checkEnclosure(f, oracle[0]);
                    checkEnclosure(ft, oracle[1]);
                }
                CHECK_FALSE(frame->boundOffsetFactor(-1.0, 0.0, f, ft));
                CHECK_FALSE(f.isValid());
                CHECK_FALSE(ft.isValid());
            }
        }
    }
}

TEST_CASE("Critical thresholds are exact for rotated axes rather than rounded radii",
          "[lc-offsetcurve][geometry][reference-critical]") {
    const std::array<std::pair<double, geo::CriticalRegime>, 9> distances{{
        {1.0, geo::CriticalRegime::PositiveFactor}, {1.25, geo::CriticalRegime::MinimumStationary},
        {2.0, geo::CriticalRegime::IntermediateThresholds}, {10.0, geo::CriticalRegime::MaximumStationary},
        {11.0, geo::CriticalRegime::NegativeFactor},
        {std::nextafter(1.25, 0.0), geo::CriticalRegime::PositiveFactor},
        {std::nextafter(1.25, 2.0), geo::CriticalRegime::IntermediateThresholds},
        {std::nextafter(10.0, 0.0), geo::CriticalRegime::IntermediateThresholds},
        {std::nextafter(10.0, 11.0), geo::CriticalRegime::NegativeFactor}}};
    for (const bool reversed : {false, true}) {
        auto data = basis(5.0, 0.5, 0.0, 0.0, reversed);
        data.majorP = RS_Vector{3.0, 4.0};
        data.center = RS_Vector{1e12, -1e12};
        for (const auto& entry : distances) {
            CAPTURE(reversed, entry.first);
            geo::WorkBudget budget;
            auto result = geo::analyseCritical(data, -entry.first, budget);
            REQUIRE(result.status == LC_IntersectionStatus::Ok);
            REQUIRE(result.adapter);
            CHECK(result.reason == geo::CriticalAnalysis::Reason::None);
            CHECK(result.adapter->regime() == entry.second);
            const std::size_t expected = entry.second == geo::CriticalRegime::IntermediateThresholds ? 4 :
                entry.second == geo::CriticalRegime::MinimumStationary ||
                entry.second == geo::CriticalRegime::MaximumStationary ? 2 : 0;
            CHECK(result.adapter->criticalOccurrences().size() == expected);
            checkCriticalCoverage(*result.adapter);
            checkCriticalOccurrences(data, -entry.first, *result.adapter);
            CHECK(result.work.cubicPieces == 0);
        }
    }
    for (const double distance : {std::nextafter(36.0, 0.0), 36.0, std::nextafter(36.0, 100.0)}) {
        CAPTURE(distance);
        const auto data = basis(100.0, 0.6);
        geo::WorkBudget budget;
        auto result = geo::analyseCritical(data, -distance, budget);
        REQUIRE(result.status == LC_IntersectionStatus::Ok);
        REQUIRE(result.adapter);
        CHECK(result.adapter->regime() == (distance < 36.0 ? geo::CriticalRegime::PositiveFactor :
            geo::CriticalRegime::IntermediateThresholds));
        CHECK(result.adapter->criticalOccurrences().size() == (distance < 36.0 ? 0 : 4));
        checkCriticalOccurrences(data, -distance, *result.adapter);
    }
    auto irrational = basis(1.0, 0.5);
    irrational.majorP = RS_Vector{1.0, 1.0};
    const double roundedMinimum = std::hypot(1.0, 1.0) * 0.25;
    geo::WorkBudget budget;
    auto result = geo::analyseCritical(irrational, -roundedMinimum, budget);
    REQUIRE(result.status == LC_IntersectionStatus::Ok);
    REQUIRE(result.adapter);
    CHECK(result.adapter->regime() != geo::CriticalRegime::MinimumStationary);
    checkCriticalOccurrences(irrational, -roundedMinimum, *result.adapter);
    auto circle = basis(5.0, 1.0);
    circle.majorP = RS_Vector{3.0, 4.0};
    geo::WorkBudget collapseBudget;
    auto collapse = geo::analyseCritical(circle, -5.0, collapseBudget);
    CHECK(collapse.status == LC_IntersectionStatus::AmbiguousTopology);
    CHECK(collapse.reason == geo::CriticalAnalysis::Reason::CollapsedCircle);
    CHECK_FALSE(collapse.adapter);
    circle.majorP = RS_Vector{1.0, 1.0};
    geo::WorkBudget regularBudget;
    auto regular = geo::analyseCritical(circle, -std::hypot(1.0, 1.0), regularBudget);
    REQUIRE(regular.status == LC_IntersectionStatus::Ok);
    REQUIRE(regular.adapter); // a rounded irrational radius is not an exact collapse
}

TEST_CASE("Critical occurrence membership uses true axes and independently qualified free ends",
          "[lc-offsetcurve][geometry][reference-critical]") {
    for (const bool reversed : {false, true}) {
        const auto data = reversed ? basis(5.0, 0.5, M_PI_2, 0.0, true) : basis(5.0, 0.5, 0.0, M_PI_2);
        geo::WorkBudget maxBudget;
        auto maximum = geo::analyseCritical(data, -10.0, maxBudget);
        REQUIRE(maximum.status == LC_IntersectionStatus::Ok);
        REQUIRE(maximum.adapter);
        CHECK(maximum.adapter->criticalOccurrences().empty());
        checkCriticalCoverage(*maximum.adapter);
        geo::WorkBudget minBudget;
        auto minimum = geo::analyseCritical(data, -1.25, minBudget);
        REQUIRE(minimum.status == LC_IntersectionStatus::Ok);
        REQUIRE(minimum.adapter);
        REQUIRE(minimum.adapter->criticalOccurrences().size() == 1);
        const auto& occurrence = minimum.adapter->criticalOccurrences().front();
        CHECK(occurrence.kind == geo::CriticalKind::CriticalStationary);
        CHECK(occurrence.atStart == !reversed);
        CHECK(occurrence.atEnd == reversed);
        CHECK(occurrence.parameter.lo() == (reversed ? minimum.adapter->frame().parameterEnd() : 0.0));
        CHECK(occurrence.parameter.lo() == occurrence.parameter.hi());
    }
    const double distance = 2.0;
    const Quad r = Quad(0.5);
    const Quad h2 = pow(Quad(distance) * r / 5, Quad(2) / 3);
    const double cusp = asin(sqrt((h2 - r * r) / (1 - r * r))).convert_to<double>();
    for (const double end : {cusp - 1e-6, cusp + 1e-6}) {
        const auto data = basis(5.0, 0.5, 0.0, end);
        geo::WorkBudget budget;
        auto result = geo::analyseCritical(data, -distance, budget);
        REQUIRE(result.status == LC_IntersectionStatus::Ok);
        REQUIRE(result.adapter);
        CHECK(result.adapter->criticalOccurrences().size() == (end < cusp ? 0 : 1));
        checkCriticalOccurrences(data, -distance, *result.adapter);
    }
}

TEST_CASE("Critical adapters cover wrapped reversed and extreme-scale sources without gaps",
          "[lc-offsetcurve][geometry][reference-critical]") {
    for (auto data : rateSources()) {
        for (const double scale : {1e-200, 1.0, 1e200}) {
            data.majorP = RS_Vector{3.0 * scale, 4.0 * scale};
            data.ratio = 0.5;
            for (const double distance : {0.0, 0.5 * scale, -2.0 * scale}) {
                CAPTURE(data.angle1, data.angle2, data.reversed, scale, distance);
                geo::WorkBudget budget;
                auto result = geo::analyseCritical(data, distance, budget);
                REQUIRE(result.status == LC_IntersectionStatus::Ok);
                REQUIRE(result.adapter);
                checkCriticalCoverage(*result.adapter);
                checkCriticalOccurrences(data, distance, *result.adapter);
                for (std::size_t i = 0; i < result.adapter->segments().size(); ++i) {
                    const auto& segment = result.adapter->segments()[i];
                    LC_CurveJetBounds bounds;
                    REQUIRE(result.adapter->frame().boundOffsetJet(segment.t0, segment.t1, bounds));
                    for (int k = 0; k <= 4; ++k) {
                        const double t = k == 4 ? segment.t1 :
                            segment.t0 + (segment.t1 - segment.t0) * (static_cast<double>(k) / 4.0);
                        checkEnclosure(bounds, referenceOracle(data, result.adapter->frame(), distance, t, true));
                    }
                }
            }
        }
    }
}

TEST_CASE("Critical cached join and seam hooks obey complete boxes and original segment domains",
          "[lc-offsetcurve][geometry][reference-critical]") {
    const auto data = basis(100.0, 0.5);
    geo::WorkBudget budget;
    auto result = geo::analyseCritical(data, -30.0, budget);
    REQUIRE(result.status == LC_IntersectionStatus::Ok);
    REQUIRE(result.adapter);
    const auto& adapter = *result.adapter;
    const auto& segments = adapter.segments();
    REQUIRE(segments.size() > 2);
    for (std::size_t i = 1; i < segments.size(); ++i) {
        const double join = segments[i].t0;
        const LC_IntersectionDomain box{i - 1, i, LC_Interval::point(join), LC_Interval::point(join)};
        const auto certificate = adapter.certifyIntersection(box);
        CHECK(certificate.outcome == LC_AnalyticIntersectionCertificate::Outcome::Excluded);
        CHECK(certificate.proof == LC_IntersectionProof::AnalyticExclusion);
        CHECK(certificate.proofDomain.segmentA == box.segmentA);
        CHECK(certificate.proofDomain.segmentB == box.segmentB);
        CHECK(certificate.proofDomain.parameterA.lo() == join);
        CHECK(certificate.proofDomain.parameterB.hi() == join);
        const LC_IntersectionDomain neighborhood{i - 1, i,
            LC_Interval::hull(std::nextafter(join, segments[i - 1].t0), join),
            LC_Interval::hull(join, std::nextafter(join, segments[i].t1))};
        CHECK(adapter.certifyIntersection(neighborhood).outcome ==
              LC_AnalyticIntersectionCertificate::Outcome::Excluded);
    }
    const double end = adapter.frame().parameterEnd();
    const LC_IntersectionDomain seam{0, segments.size() - 1, LC_Interval::point(0.0), LC_Interval::point(end)};
    const auto certificate = adapter.certifyIntersection(seam);
    REQUIRE(certificate.outcome == LC_AnalyticIntersectionCertificate::Outcome::Excluded);
    CHECK(certificate.proofDomain.parameterA.lo() == 0.0);
    CHECK(certificate.proofDomain.parameterB.hi() == end);
    CHECK(adapter.certifyIntersection({0, segments.size() - 1,
        LC_Interval::hull(0.0, std::nextafter(0.0, end)),
        LC_Interval::hull(std::nextafter(end, 0.0), end)}).outcome ==
        LC_AnalyticIntersectionCertificate::Outcome::Excluded);
    const LC_IntersectionDomain unproved{0, segments.size() - 1,
        LC_Interval::hull(segments.front().t0, segments.front().t1),
        LC_Interval::hull(segments.back().t0, segments.back().t1)};
    CHECK(adapter.certifyIntersection(unproved).outcome == LC_AnalyticIntersectionCertificate::Outcome::Unavailable);
    LC_IntersectionOptions options;
    options.strict = true;
    options.tolerance = 1e-8;
    geo::WorkBudget outwardBudget;
    auto outward = geo::analyseCritical(data, 1.0, outwardBudget);
    REQUIRE(outward.adapter);
    const auto intersections = findIntersections(*outward.adapter, options);
    CHECK(intersections.status == LC_IntersectionStatus::Ok);
    CHECK(intersections.intersections.empty());
    const auto& first = segments.front();
    const double mid = first.t0 + 0.5 * (first.t1 - first.t0);
    CHECK(adapter.certifyIntersection({0, 0, LC_Interval::hull(first.t0, mid),
        LC_Interval::hull(first.t0, mid)}).outcome == LC_AnalyticIntersectionCertificate::Outcome::Unavailable);
}

TEST_CASE("Critical failures and invalid requests never publish partial adapters or geometry",
          "[lc-offsetcurve][geometry][reference-critical]") {
    for (const auto& entry : std::array<std::pair<RS_EllipseData, double>, 3>{{
            {basis(5.0, 1.0), -5.0}, {basis(1.0, 1e-250, 0.0, 1.0), -0.25},
            {basis(1e-300, 0.5), -1e300}}}) {
        geo::WorkBudget budget;
        auto result = geo::analyseCritical(entry.first, entry.second, budget);
        CHECK(result.status == LC_IntersectionStatus::AmbiguousTopology);
        CHECK_FALSE(result.adapter);
        CHECK(result.reason != geo::CriticalAnalysis::Reason::None);
        CHECK(result.work.vertices > 0);
        CHECK(result.work.distancePairs > 0);
    }
    const auto unrepresentable = basis(1.0, 1e-80);
    const auto frame = geo::ReferenceFrame::create(unrepresentable, -1e81);
    REQUIRE(frame);
    LC_CurveJetBounds failedBounds;
    REQUIRE_FALSE(frame->boundOffsetJet(0.0, frame->parameterEnd(), failedBounds));
    geo::WorkBudget publicationBudget;
    auto publication = geo::analyseCritical(unrepresentable, -1e81, publicationBudget);
    CHECK(publication.status == LC_IntersectionStatus::AmbiguousTopology);
    CHECK(publication.reason == geo::CriticalAnalysis::Reason::UnresolvedGeometry);
    CHECK_FALSE(publication.adapter);
    CHECK(publication.work.vertices > 1);
    geo::WorkBudget invalidBudget;
    auto invalid = geo::analyseCritical(basis(5.0, 2.0), 1.0, invalidBudget);
    CHECK(invalid.status == LC_IntersectionStatus::InvalidInput);
    CHECK_FALSE(invalid.adapter);
    CHECK(invalid.work.vertices == 0);
    geo::WorkBudget budget;
    auto valid = geo::analyseCritical(basis(5.0, 0.5, 0.0, 1.0), 0.5, budget);
    REQUIRE(valid.adapter);
    const auto& adapter = *valid.adapter;
    for (const double t : {-1.0, std::numeric_limits<double>::quiet_NaN(),
                           std::numeric_limits<double>::infinity(), adapter.frame().parameterEnd() + 1.0}) {
        RS_Vector p{1.0, 1.0};
        RS_Vector d{1.0, 1.0};
        CHECK_FALSE(adapter.evaluate(0, t, p, d));
        CHECK_FALSE(p.valid);
        CHECK_FALSE(d.valid);
        LC_Interval x = LC_Interval::point(1.0), y = x, dx = x, dy = x;
        CHECK_FALSE(adapter.bound(0, t, t, x, y, dx, dy));
        CHECK_FALSE(x.isValid()); CHECK_FALSE(y.isValid());
        CHECK_FALSE(dx.isValid()); CHECK_FALSE(dy.isValid());
        CHECK_FALSE(adapter.certifyInjective(0, LC_Interval::point(t)).available);
    }
    CHECK_FALSE(adapter.certifyInjective(adapter.segments().size(), LC_Interval::point(0.0)).available);
}

TEST_CASE("Critical work caps are exact cumulative irreversible and checked before generation",
          "[lc-offsetcurve][geometry][reference-critical]") {
    const auto data = basis(100.0, 0.5);
    geo::WorkBudget baseline;
    auto reference = geo::analyseCritical(data, -30.0, baseline);
    REQUIRE(reference.status == LC_IntersectionStatus::Ok);
    const auto work = reference.work;
    REQUIRE(work.vertices > 1);
    REQUIRE(work.distancePairs > 1);
    REQUIRE(work.deepest > 1);
    geo::WorkBudget exact{LC_OffsetCheckLimits{1, work.vertices, work.distancePairs, work.deepest}};
    CHECK(exact.depthLimit() == work.deepest);
    auto accepted = geo::analyseCritical(data, -30.0, exact);
    REQUIRE(accepted.status == LC_IntersectionStatus::Ok);
    CHECK(accepted.work.vertices == work.vertices);
    CHECK(accepted.work.distancePairs == work.distancePairs);
    CHECK(accepted.work.cubicPieces == 0);
    for (int limit = 0; limit < 3; ++limit) {
        LC_OffsetCheckLimits limits{1, work.vertices, work.distancePairs, work.deepest};
        if (limit == 0) { --limits.maxVertices; }
        if (limit == 1) { --limits.maxDistancePairs; }
        if (limit == 2) { --limits.maxDepth; }
        geo::WorkBudget budget{limits};
        auto result = geo::analyseCritical(data, -30.0, budget);
        CHECK(result.status == LC_IntersectionStatus::LimitExceeded);
        CHECK_FALSE(result.adapter);
        CHECK(budget.status() == geo::BudgetStatus::LimitExceeded);
        CHECK(result.work.vertices <= limits.maxVertices);
        CHECK(result.work.distancePairs <= limits.maxDistancePairs);
        const auto consumed = budget.consumed();
        auto again = geo::analyseCritical(data, -30.0, budget);
        CHECK(again.status == LC_IntersectionStatus::LimitExceeded);
        CHECK(again.work.vertices == consumed.vertices);
        CHECK(again.work.distancePairs == consumed.distancePairs);
    }
    geo::WorkBudget spent{LC_OffsetCheckLimits{1, work.vertices + 2, work.distancePairs + 3, work.deepest}};
    REQUIRE(spent.consumeVertices(2));
    REQUIRE(spent.consumePairs(3));
    auto result = geo::analyseCritical(data, -30.0, spent);
    REQUIRE(result.status == LC_IntersectionStatus::Ok);
    CHECK(result.work.vertices == work.vertices + 2);
    CHECK(result.work.distancePairs == work.distancePairs + 3);
    spent.latchLimitExceeded();
    CHECK(spent.status() == geo::BudgetStatus::LimitExceeded);
    CHECK(spent.consumed().vertices == work.vertices + 2);
    geo::WorkBudget zero{LC_OffsetCheckLimits{1, 1, 1, 0}};
    zero.latchLimitExceeded();
    CHECK(zero.status() == geo::BudgetStatus::InvalidInput);
    auto refused = geo::analyseCritical(data, -30.0, zero);
    CHECK(refused.status == LC_IntersectionStatus::InvalidInput);
    CHECK(refused.work.vertices == 0);
}

TEST_CASE("Critical thin cusps use stable polynomial bounds and unrepresentable isolation refuses",
          "[lc-offsetcurve][geometry][reference-critical]") {
    for (const auto& entry : std::array<std::pair<double, double>, 2>{{{0.02, 5.3}, {0.01, 10.0}}}) {
        for (const bool reversed : {false, true}) {
            CAPTURE(entry.first, entry.second, reversed);
            const auto data = basis(100.0, entry.first, 0.0, 0.0, reversed);
            geo::WorkBudget regressionBudget;
            auto regression = geo::analyseCritical(data, -entry.second, regressionBudget);
            REQUIRE(regression.status == LC_IntersectionStatus::Ok);
            REQUIRE(regression.adapter);
            REQUIRE(regression.adapter->criticalOccurrences().size() == 4);
            checkCriticalCoverage(*regression.adapter);
            checkCriticalOccurrences(data, -entry.second, *regression.adapter);
        }
    }
    const auto thin = basis(1.0, 1e-7);
    geo::WorkBudget budget;
    auto result = geo::analyseCritical(thin, -1e-8, budget);
    REQUIRE(result.status == LC_IntersectionStatus::Ok);
    REQUIRE(result.adapter);
    REQUIRE(result.adapter->criticalOccurrences().size() == 4);
    checkCriticalCoverage(*result.adapter);
    checkCriticalOccurrences(thin, -1e-8, *result.adapter);
    const double ratio = 1e-155;
    const double distance = std::nextafter(ratio * ratio, std::numeric_limits<double>::infinity());
    REQUIRE(distance > 0.0);
    const auto tiny = basis(1.0, ratio);
    REQUIRE(geo::ReferenceFrame::create(tiny, -distance));
    geo::WorkBudget tinyBudget;
    auto unresolved = geo::analyseCritical(tiny, -distance, tinyBudget);
    CHECK(unresolved.status == LC_IntersectionStatus::AmbiguousTopology);
    CHECK(unresolved.reason == geo::CriticalAnalysis::Reason::UnresolvedGeometry);
    CHECK_FALSE(unresolved.adapter);
    CHECK(unresolved.work.vertices > 0);
    CHECK(unresolved.work.distancePairs > 0);
    CHECK(tinyBudget.status() == geo::BudgetStatus::Ok);
}

TEST_CASE("Critical region projection signs agree with independent cusp and tangent normals",
          "[lc-offsetcurve][geometry][reference-critical]") {
    for (const bool reversed : {false, true}) {
        const auto data = basis(5.0, 0.5, 0.0, 0.0, reversed);
        geo::WorkBudget budget;
        auto result = geo::analyseCritical(data, -2.0, budget);
        REQUIRE(result.status == LC_IntersectionStatus::Ok);
        REQUIRE(result.adapter);
        const auto& adapter = *result.adapter;
        const Quad pi = boost::math::constants::pi<Quad>();
        const Quad sweep = idealDirectedSweep(data, true);
        const Quad beta = asin(sqrt((pow(Quad(1) / 5, Quad(2) / 3) - Quad(0.25)) / Quad(0.75)));
        for (const auto& region : adapter.regions()) {
            const auto& segment = region.segment;
            const double middle = segment.t0 + 0.5 * (segment.t1 - segment.t0);
            const Quad eta = Quad(M_PI_4) + sweep * Quad(middle) / Quad(adapter.frame().parameterEnd());
            Quad nx;
            Quad ny;
            if (region.proof == geo::InjectivityProof::SingleCuspProjection) {
                const int q = floor(eta / (pi / 2)).convert_to<int>();
                const Quad cusp = q % 2 == 0 ? Quad(q) * pi / 2 + beta : Quad(q + 1) * pi / 2 - beta;
                const Quad h = sqrt(Quad(0.25) * cos(cusp) * cos(cusp) + sin(cusp) * sin(cusp));
                nx = Quad(0.5) * cos(cusp) / h;
                ny = sin(cusp) / h;
            } else {
                nx = -sin(eta) / 5;
                ny = cos(eta) / Quad(2.5);
            }
            const auto midJet = referenceOracle(data, adapter.frame(), -2.0, middle, true);
            const Quad expected = midJet[2] * nx + midJet[3] * ny;
            REQUIRE(expected != 0);
            for (int k = 0; k <= 16; ++k) {
                const double t = k == 16 ? segment.t1 :
                    segment.t0 + (segment.t1 - segment.t0) * (static_cast<double>(k) / 16.0);
                const auto jet = referenceOracle(data, adapter.frame(), -2.0, t, true);
                const Quad projected = jet[2] * nx + jet[3] * ny;
                CHECK(projected * expected >= 0);
            }
        }
    }
}

TEST_CASE("Shared reference budgets consume exact allowances once and latch every refusal",
          "[lc-offsetcurve][geometry][reference-frame]") {
    static_assert(!std::is_copy_constructible_v<geo::WorkBudget>);
    static_assert(!std::is_copy_assignable_v<geo::WorkBudget>);
    const LC_OffsetCheckLimits limits{3, 5, 7, 2};
    for (int resource = 0; resource < 4; ++resource) {
        CAPTURE(resource);
        geo::WorkBudget budget{limits};
        CHECK(budget.status() == geo::BudgetStatus::Ok);
        REQUIRE(budget.checkDepth(0));
        REQUIRE(budget.consumePieces(1));
        REQUIRE(budget.consumeVertices(2));
        REQUIRE(budget.consumePairs(3));
        REQUIRE(budget.checkDepth(1));
        CHECK(budget.remainingPieces() == 2);
        CHECK(budget.remainingVertices() == 3);
        CHECK(budget.remainingPairs() == 4);
        REQUIRE(budget.consumePieces(2));
        REQUIRE(budget.consumeVertices(3));
        REQUIRE(budget.consumePairs(4));
        REQUIRE(budget.checkDepth(2));
        REQUIRE(budget.consumePairs(0));
        CHECK(budget.remainingPieces() == 0);
        CHECK(budget.remainingVertices() == 0);
        CHECK(budget.remainingPairs() == 0);
        const bool accepted = resource == 0 ? budget.consumePieces(1) : resource == 1 ? budget.consumeVertices(1) :
            resource == 2 ? budget.consumePairs(1) : budget.checkDepth(3);
        CHECK_FALSE(accepted);
        CHECK(budget.status() == geo::BudgetStatus::LimitExceeded);
        CHECK(budget.consumed().cubicPieces == 3);
        CHECK(budget.consumed().vertices == 5);
        CHECK(budget.consumed().distancePairs == 7);
        CHECK(budget.consumed().deepest == 2);
        CHECK_FALSE(budget.consumePieces(0));
        CHECK_FALSE(budget.consumeVertices(0));
        CHECK_FALSE(budget.consumePairs(0));
        CHECK_FALSE(budget.checkDepth(0));
    }
    geo::WorkBudget oneLess{LC_OffsetCheckLimits{2, 4, 6, 1}};
    REQUIRE(oneLess.consumeVertices(3));
    CHECK_FALSE(oneLess.consumeVertices(2));
    CHECK(oneLess.consumed().vertices == 3);
    CHECK(oneLess.consumed().cubicPieces == 0);
    CHECK(oneLess.consumed().distancePairs == 0);
    CHECK(oneLess.remainingPieces() == 0);
    CHECK(oneLess.remainingVertices() == 0);
    CHECK(oneLess.remainingPairs() == 0);
    CHECK_FALSE(oneLess.consumePairs(1));
    CHECK(oneLess.consumed().distancePairs == 0);
    for (int resource = 0; resource < 3; ++resource) {
        geo::WorkBudget shortBudget{LC_OffsetCheckLimits{2, 4, 6, 1}};
        const bool accepted = resource == 0 ? shortBudget.consumePieces(3) : resource == 1 ?
            shortBudget.consumeVertices(5) : shortBudget.consumePairs(7);
        CHECK_FALSE(accepted);
        CHECK(shortBudget.status() == geo::BudgetStatus::LimitExceeded);
        CHECK(shortBudget.consumed().cubicPieces == 0);
        CHECK(shortBudget.consumed().vertices == 0);
        CHECK(shortBudget.consumed().distancePairs == 0);
    }
}

TEST_CASE("Shared reference budgets reject zero limits and cannot overflow counters",
          "[lc-offsetcurve][geometry][reference-frame]") {
    const std::array<LC_OffsetCheckLimits, 4> zeroLimits{{{0, 1, 1, 1}, {1, 0, 1, 1}, {1, 1, 0, 1}, {1, 1, 1, 0}}};
    for (const LC_OffsetCheckLimits& limits : zeroLimits) {
        geo::WorkBudget invalid{limits};
        CHECK(invalid.status() == geo::BudgetStatus::InvalidInput);
        CHECK_FALSE(invalid.consumePieces(0));
        CHECK_FALSE(invalid.consumeVertices(0));
        CHECK_FALSE(invalid.consumePairs(0));
        CHECK_FALSE(invalid.checkDepth(0));
        CHECK(invalid.consumed().cubicPieces == 0);
        CHECK(invalid.consumed().vertices == 0);
        CHECK(invalid.consumed().distancePairs == 0);
        CHECK(invalid.consumed().deepest == 0);
        CHECK(invalid.remainingPieces() == 0);
        CHECK(invalid.remainingVertices() == 0);
        CHECK(invalid.remainingPairs() == 0);
    }
    const auto maximum = std::numeric_limits<std::size_t>::max();
    for (int resource = 0; resource < 3; ++resource) {
        geo::WorkBudget budget{LC_OffsetCheckLimits{maximum, maximum, maximum, std::numeric_limits<unsigned>::max()}};
        REQUIRE(budget.consumePieces(maximum - 1));
        REQUIRE(budget.consumeVertices(maximum - 1));
        REQUIRE(budget.consumePairs(maximum - 1));
        REQUIRE(budget.checkDepth(std::numeric_limits<unsigned>::max()));
        const bool accepted = resource == 0 ? budget.consumePieces(2) : resource == 1 ? budget.consumeVertices(2) :
            budget.consumePairs(2);
        CHECK_FALSE(accepted);
        CHECK(budget.status() == geo::BudgetStatus::LimitExceeded);
        CHECK(budget.consumed().cubicPieces == maximum - 1);
        CHECK(budget.consumed().vertices == maximum - 1);
        CHECK(budget.consumed().distancePairs == maximum - 1);
        CHECK_FALSE(budget.consumeVertices(1));
    }
    geo::WorkBudget exact{LC_OffsetCheckLimits{maximum, maximum, maximum, 1}};
    REQUIRE(exact.consumePieces(maximum));
    REQUIRE(exact.consumeVertices(maximum));
    REQUIRE(exact.consumePairs(maximum));
    CHECK(exact.consumed().cubicPieces == maximum);
    CHECK(exact.consumed().vertices == maximum);
    CHECK(exact.consumed().distancePairs == maximum);
    CHECK_FALSE(exact.consumePairs(1));
    CHECK(exact.consumed().distancePairs == maximum);
}
