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
#include <memory>
#include <optional>
#include <random>
#include <type_traits>
#include <utility>
#include <vector>

#include <boost/math/constants/constants.hpp>
#include <boost/multiprecision/cpp_bin_float.hpp>
#include <boost/multiprecision/cpp_int.hpp>

#include "lc_curveoffset.h"
#include "lc_ellipseoffsetgeometry.h"
#include "rs_line.h"
#include "rs_spline.h"

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

using CriticalFloat = boost::multiprecision::number<boost::multiprecision::cpp_bin_float<256>>;

void checkCriticalEnclosure(const LC_Interval& interval, const CriticalFloat& value) {
    REQUIRE(interval.isValid());
    REQUIRE(std::isfinite(interval.lo()));
    REQUIRE(std::isfinite(interval.hi()));
    CHECK(CriticalFloat(interval.lo()) <= value);
    CHECK(value <= CriticalFloat(interval.hi()));
}

void checkNested(const LC_Interval& inner, const LC_Interval& outer) {
    REQUIRE(inner.isValid());
    REQUIRE(outer.isValid());
    CHECK(outer.lo() <= inner.lo());
    CHECK(inner.hi() <= outer.hi());
}

void checkWork(const geo::CheckWork& actual, const geo::CheckWork& expected) {
    CHECK(actual.cacheSpans == expected.cacheSpans);
    CHECK(actual.vertices == expected.vertices);
    CHECK(actual.distancePairs == expected.distancePairs);
    CHECK(actual.deepest == expected.deepest);
    CHECK(actual.orderingComparisons == expected.orderingComparisons);
}

void checkCriticalSnapshot(const RS_EllipseData& data, const double distance, const geo::ReferenceFrame& frame,
                           const geo::CriticalOccurrenceSnapshot& snapshot) {
    const CriticalFloat pi = boost::math::constants::pi<CriticalFloat>();
    const CriticalFloat r = data.ratio;
    const CriticalFloat mx = data.majorP.x, my = data.majorP.y;
    const CriticalFloat c = 1 - r * r;
    const auto& occurrence = snapshot.occurrence;
    const int q = occurrence.quadrant + 4 * occurrence.winding;
    CriticalFloat angle = CriticalFloat(q) * pi / 2;
    CriticalFloat x, y;
    if (occurrence.kind == geo::CriticalKind::SimpleCusp) {
        const CriticalFloat norm = sqrt(mx * mx + my * my);
        const CriticalFloat h2 = pow(CriticalFloat(-distance) * r / norm, CriticalFloat(2) / 3);
        const CriticalFloat z = (h2 - r * r) / c;
        const CriticalFloat beta = asin(sqrt(z));
        angle += q % 2 == 0 ? beta : pi / 2 - beta;
        const CriticalFloat cosPower = (1 - z) * sqrt(1 - z);
        const CriticalFloat sinPower = z * sqrt(z);
        const int cosSign = occurrence.quadrant == 0 || occurrence.quadrant == 3 ? 1 : -1;
        const int sinSign = occurrence.quadrant < 2 ? 1 : -1;
        x = c * (cosSign * cosPower * mx + sinSign * sinPower * my / r);
        y = c * (cosSign * cosPower * my - sinSign * sinPower * mx / r);
        // Also qualify the cancellation-free image against the independent E + distance*N formula.
        const CriticalFloat cosine = cosSign * sqrt(1 - z);
        const CriticalFloat sine = sinSign * sqrt(z);
        const CriticalFloat speed = norm * sqrt(r * r * cosine * cosine + sine * sine);
        const CriticalFloat directX = mx * cosine - my * r * sine +
            CriticalFloat(distance) * (r * cosine * mx - sine * my) / speed;
        const CriticalFloat directY = my * cosine + mx * r * sine +
            CriticalFloat(distance) * (r * cosine * my + sine * mx) / speed;
        checkCriticalEnclosure(snapshot.imageX, directX / CriticalFloat(frame.scale()));
        checkCriticalEnclosure(snapshot.imageY, directY / CriticalFloat(frame.scale()));
        REQUIRE(snapshot.canonical);
        const CriticalFloat u = snapshot.canonical->complement ? CriticalFloat(1 - z) : z;
        checkCriticalEnclosure(snapshot.canonical->variable, u);
        checkCriticalEnclosure(snapshot.canonical->inverseAngle, asin(sqrt(u)));
    } else {
        CHECK_FALSE(snapshot.canonical);
        const bool major = occurrence.quadrant % 2 == 0;
        const int sign = occurrence.quadrant < 2 ? 1 : -1;
        const CriticalFloat factor = major ? c : CriticalFloat(r - 1 / r);
        x = sign * factor * (major ? mx : CriticalFloat(-my));
        y = sign * factor * (major ? my : mx);
    }
    const CriticalFloat period = 2 * pi;
    const CriticalFloat start = frame.sourceSweep().whole ? CriticalFloat(M_PI_4) : CriticalFloat(data.angle1);
    CriticalFloat span = frame.sourceSweep().whole ? (data.reversed ? CriticalFloat(-period) : period) :
        CriticalFloat(data.angle2) - CriticalFloat(data.angle1);
    if (!frame.sourceSweep().whole) {
        if (!data.reversed && span < 0) { span += period; }
        if (data.reversed && span > 0) { span -= period; }
    }
    const CriticalFloat parameter = occurrence.atStart ? CriticalFloat(0) : occurrence.atEnd ?
        CriticalFloat(frame.parameterEnd()) : (angle - start) * CriticalFloat(frame.parameterEnd()) / span;
    checkCriticalEnclosure(occurrence.parameter, parameter);
    checkCriticalEnclosure(snapshot.imageX, x / CriticalFloat(frame.scale()));
    checkCriticalEnclosure(snapshot.imageY, y / CriticalFloat(frame.scale()));
    LC_Interval worldX, worldY;
    REQUIRE(frame.pointToWorld(snapshot.imageX, snapshot.imageY, worldX, worldY));
    checkCriticalEnclosure(worldX, CriticalFloat(data.center.x) + x);
    checkCriticalEnclosure(worldY, CriticalFloat(data.center.y) + y);
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
            CHECK(result.work.cacheSpans == 0);
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

TEST_CASE("Critical depth excludes a split refused before child examination",
          "[lc-offsetcurve][geometry][reference-critical-depth]") {
    LC_OffsetCheckLimits limits;
    limits.maxVertices = 6;
    limits.maxDepth = 20;
    geo::WorkBudget budget{limits};
    const auto result = geo::analyseCritical(basis(100.0, 0.5), -30.0, budget);

    REQUIRE(result.status == LC_IntersectionStatus::LimitExceeded);
    CHECK_FALSE(result.adapter);
    CHECK(budget.status() == geo::BudgetStatus::LimitExceeded);
    CHECK(result.work.vertices == 6);
    CHECK(result.work.distancePairs == 26); // Root + 21 axes + 3 axis adjacencies + scalar root.
    CHECK(result.work.cacheSpans == 0);
    CHECK(result.work.deepest == 0);
    CHECK(result.work.vertices == budget.consumed().vertices);
    CHECK(result.work.distancePairs == budget.consumed().distancePairs);
    CHECK(result.work.cacheSpans == budget.consumed().cacheSpans);
    CHECK(result.work.deepest == budget.consumed().deepest);
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
    CHECK(accepted.work.cacheSpans == 0);
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
        CHECK(budget.consumed().cacheSpans == 3);
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
    CHECK(oneLess.consumed().cacheSpans == 0);
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
        CHECK(shortBudget.consumed().cacheSpans == 0);
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
        CHECK(invalid.consumed().cacheSpans == 0);
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
        CHECK(budget.consumed().cacheSpans == maximum - 1);
        CHECK(budget.consumed().vertices == maximum - 1);
        CHECK(budget.consumed().distancePairs == maximum - 1);
        CHECK_FALSE(budget.consumeVertices(1));
    }
    geo::WorkBudget exact{LC_OffsetCheckLimits{maximum, maximum, maximum, 1}};
    REQUIRE(exact.consumePieces(maximum));
    REQUIRE(exact.consumeVertices(maximum));
    REQUIRE(exact.consumePairs(maximum));
    CHECK(exact.consumed().cacheSpans == maximum);
    CHECK(exact.consumed().vertices == maximum);
    CHECK(exact.consumed().distancePairs == maximum);
    CHECK_FALSE(exact.consumePairs(1));
    CHECK(exact.consumed().distancePairs == maximum);
}

TEST_CASE("shared work budget preserves sticky refusal",
          "[lc-offsetcurve][geometry][cache-budget-accounting][budget-core]") {
    const geo::CheckWork spent{2, 3, 5, 2, 7};
    const geo::CheckWork delta{11, 13, 17, 5, 19};
    const geo::CheckWork total{13, 16, 22, 5, 26};
    const LC_OffsetCheckLimits limits{total.cacheSpans, total.vertices, total.distancePairs,
                                    total.deepest, total.orderingComparisons};
    const auto preSpend = [&](geo::WorkBudget& budget) {
        REQUIRE(budget.consumePieces(spent.cacheSpans));
        REQUIRE(budget.consumeVertices(spent.vertices));
        REQUIRE(budget.consumePairs(spent.distancePairs));
        REQUIRE(budget.checkDepth(spent.deepest));
        REQUIRE(budget.consumeOrderingComparisons(spent.orderingComparisons));
        checkWork(budget.consumed(), spent);
    };

    SECTION("complete adoption reaches every exact cap from pre-spent state") {
        geo::WorkBudget budget{limits};
        preSpend(budget);
        CHECK(budget.remainingPieces() == delta.cacheSpans);
        CHECK(budget.remainingVertices() == delta.vertices);
        CHECK(budget.remainingPairs() == delta.distancePairs);
        CHECK(budget.remainingOrderingComparisons() == delta.orderingComparisons);
        REQUIRE(budget.adoptWork(delta));
        CHECK(budget.status() == geo::BudgetStatus::Ok);
        checkWork(budget.consumed(), total);
        CHECK(budget.remainingPieces() == 0);
        CHECK(budget.remainingVertices() == 0);
        CHECK(budget.remainingPairs() == 0);
        CHECK(budget.remainingOrderingComparisons() == 0);
        REQUIRE(budget.adoptWork({}));
        REQUIRE(budget.adoptWork({0, 0, 0, 1, 0}));
        REQUIRE(budget.checkDepth(limits.maxDepth));
        REQUIRE(budget.checkDepth(0));
        checkWork(budget.consumed(), total);
    }

    SECTION("each isolated one-short additive cap refuses the entire delta") {
        for (int resource = 0; resource < 4; ++resource) {
            CAPTURE(resource);
            auto shortLimits = limits;
            if (resource == 0) { --shortLimits.maxCacheSpans; }
            else if (resource == 1) { --shortLimits.maxVertices; }
            else if (resource == 2) { --shortLimits.maxDistancePairs; }
            else { --shortLimits.maxOrderingComparisons; }
            geo::WorkBudget budget{shortLimits};
            preSpend(budget);
            CHECK_FALSE(budget.adoptWork(delta));
            CHECK(budget.status() == geo::BudgetStatus::LimitExceeded);
            checkWork(budget.consumed(), spent);
            CHECK(budget.remainingPieces() == 0);
            CHECK(budget.remainingVertices() == 0);
            CHECK(budget.remainingPairs() == 0);
            CHECK(budget.remainingOrderingComparisons() == 0);
            CHECK_FALSE(budget.adoptWork({}));
            checkWork(budget.consumed(), spent);
            CHECK_FALSE(budget.adoptWork(delta));
            checkWork(budget.consumed(), spent);
        }
    }

    SECTION("over-depth adoption and direct depth refusal preserve every counter") {
        auto shortLimits = limits;
        shortLimits.maxDepth = delta.deepest - 1;
        for (const bool adopt : {false, true}) {
            CAPTURE(adopt);
            geo::WorkBudget budget{shortLimits};
            preSpend(budget);
            REQUIRE(budget.checkDepth(shortLimits.maxDepth - 1));
            REQUIRE(budget.checkDepth(shortLimits.maxDepth));
            const auto before = budget.consumed();
            const bool accepted = adopt ? budget.adoptWork(delta) : budget.checkDepth(delta.deepest);
            CHECK_FALSE(accepted);
            CHECK(budget.status() == geo::BudgetStatus::LimitExceeded);
            checkWork(budget.consumed(), before);
            CHECK_FALSE(budget.checkDepth(0));
            CHECK_FALSE(budget.adoptWork({}));
            checkWork(budget.consumed(), before);
        }
    }

    SECTION("each additive dimension rejects size_t overflow without partial debit") {
        const auto maximum = std::numeric_limits<std::size_t>::max();
        const auto maximumDepth = std::numeric_limits<unsigned>::max();
        const LC_OffsetCheckLimits large{maximum, maximum, maximum, maximumDepth, maximum};
        const geo::CheckWork nearLimit{maximum - 1, maximum - 1, maximum - 1, 2, maximum - 1};
        const std::array<std::size_t geo::CheckWork::*, 4> counters{{
            &geo::CheckWork::cacheSpans, &geo::CheckWork::vertices,
            &geo::CheckWork::distancePairs, &geo::CheckWork::orderingComparisons}};
        const std::array<bool (geo::WorkBudget::*)(std::size_t), 4> admissions{{
            &geo::WorkBudget::consumePieces, &geo::WorkBudget::consumeVertices,
            &geo::WorkBudget::consumePairs, &geo::WorkBudget::consumeOrderingComparisons}};
        for (std::size_t resource = 0; resource < counters.size(); ++resource) {
            for (const bool adopt : {false, true}) {
                CAPTURE(resource, adopt);
                geo::WorkBudget budget{large};
                REQUIRE(budget.adoptWork(nearLimit));
                auto overflow = geo::CheckWork{1, 1, 1, 3, 1};
                overflow.*counters[resource] = 2;
                const bool accepted = adopt ? budget.adoptWork(overflow) : (budget.*admissions[resource])(2);
                CHECK_FALSE(accepted);
                CHECK(budget.status() == geo::BudgetStatus::LimitExceeded);
                checkWork(budget.consumed(), nearLimit);
                CHECK_FALSE(budget.adoptWork({}));
                checkWork(budget.consumed(), nearLimit);
            }
        }
        geo::WorkBudget exact{large};
        REQUIRE(exact.adoptWork(nearLimit));
        REQUIRE(exact.adoptWork({1, 1, 1, maximumDepth, 1}));
        const geo::CheckWork atLimit{maximum, maximum, maximum, maximumDepth, maximum};
        checkWork(exact.consumed(), atLimit);
        CHECK(exact.remainingPieces() == 0);
        CHECK(exact.remainingVertices() == 0);
        CHECK(exact.remainingPairs() == 0);
        CHECK(exact.remainingOrderingComparisons() == 0);
        REQUIRE(exact.adoptWork({}));
        REQUIRE(exact.checkDepth(maximumDepth));
        REQUIRE(exact.checkDepth(0));
        checkWork(exact.consumed(), atLimit);
    }

    SECTION("ordering comparisons and geometric pairs have independent allowances") {
        for (const bool orderingFirst : {false, true}) {
            CAPTURE(orderingFirst);
            geo::WorkBudget budget{LC_OffsetCheckLimits{3, 5, 7, 2, 11}};
            if (orderingFirst) {
                REQUIRE(budget.consumeOrderingComparisons(11));
                CHECK(budget.remainingPairs() == 7);
                CHECK(budget.consumed().distancePairs == 0);
                REQUIRE(budget.consumePairs(7));
            } else {
                REQUIRE(budget.consumePairs(7));
                CHECK(budget.remainingOrderingComparisons() == 11);
                CHECK(budget.consumed().orderingComparisons == 0);
                REQUIRE(budget.consumeOrderingComparisons(11));
            }
            const geo::CheckWork expected{0, 0, 7, 0, 11};
            checkWork(budget.consumed(), expected);
            CHECK(budget.remainingPairs() == 0);
            CHECK(budget.remainingOrderingComparisons() == 0);
            CHECK(budget.remainingPieces() == 3);
            CHECK(budget.remainingVertices() == 5);
            const bool accepted = orderingFirst ? budget.consumePairs(1) : budget.consumeOrderingComparisons(1);
            CHECK_FALSE(accepted);
            CHECK(budget.status() == geo::BudgetStatus::LimitExceeded);
            checkWork(budget.consumed(), expected);
        }
    }

    SECTION("legacy four-field aggregates keep their meanings and ordering defaults") {
        static_assert(std::is_aggregate_v<LC_OffsetCheckLimits>);
        static_assert(std::is_aggregate_v<geo::CheckWork>);
        const LC_OffsetCheckLimits legacyLimits{3, 5, 7, 2};
        CHECK(legacyLimits.maxCacheSpans == 3);
        CHECK(legacyLimits.maxVertices == 5);
        CHECK(legacyLimits.maxDistancePairs == 7);
        CHECK(legacyLimits.maxDepth == 2);
        CHECK(legacyLimits.maxOrderingComparisons == (std::size_t{1} << 20));
        const geo::CheckWork legacyWork{1, 2, 3, 2};
        checkWork(legacyWork, geo::CheckWork{1, 2, 3, 2, 0});
        geo::WorkBudget budget{legacyLimits};
        REQUIRE(budget.adoptWork(legacyWork));
        CHECK(budget.depthLimit() == 2);
        CHECK(budget.remainingOrderingComparisons() == (std::size_t{1} << 20));
        REQUIRE(budget.consumeOrderingComparisons(std::size_t{1} << 20));
        checkWork(budget.consumed(), geo::CheckWork{1, 2, 3, 2, std::size_t{1} << 20});
    }

    SECTION("adoption can alias consumed work on both success and refusal") {
        geo::WorkBudget budget{LC_OffsetCheckLimits{8, 12, 20, 7, 43}};
        REQUIRE(budget.adoptWork({2, 3, 5, 7, 11}));
        const auto& alias = budget.consumed();
        REQUIRE(budget.adoptWork(alias));
        const geo::CheckWork doubled{4, 6, 10, 7, 22};
        checkWork(alias, doubled);
        CHECK_FALSE(budget.adoptWork(alias));
        CHECK(budget.status() == geo::BudgetStatus::LimitExceeded);
        checkWork(alias, doubled);
        CHECK_FALSE(budget.adoptWork(alias));
        checkWork(budget.consumed(), doubled);
    }
}

TEST_CASE("work budget terminal latches preserve counters",
          "[lc-offsetcurve][geometry][cache-budget-accounting][budget-core]") {
    const auto checkLatched = [](const geo::WorkBudget& budget, const geo::BudgetStatus status,
                                const geo::CheckWork& before) {
        CHECK(budget.status() == status);
        checkWork(budget.consumed(), before);
        CHECK(budget.remainingPieces() == 0);
        CHECK(budget.remainingVertices() == 0);
        CHECK(budget.remainingPairs() == 0);
        CHECK(budget.remainingOrderingComparisons() == 0);
    };
    const auto retryLatched = [&](geo::WorkBudget& budget, const geo::BudgetStatus status,
                                  const geo::CheckWork& before) {
        for (const auto count : {std::size_t{0}, std::size_t{1}, std::numeric_limits<std::size_t>::max()}) {
            CAPTURE(count);
            CHECK_FALSE(budget.consumePieces(count));
            checkLatched(budget, status, before);
            CHECK_FALSE(budget.consumeVertices(count));
            checkLatched(budget, status, before);
            CHECK_FALSE(budget.consumePairs(count));
            checkLatched(budget, status, before);
            CHECK_FALSE(budget.consumeOrderingComparisons(count));
            checkLatched(budget, status, before);
        }
        for (const unsigned depth : {0u, 1u, std::numeric_limits<unsigned>::max()}) {
            CHECK_FALSE(budget.checkDepth(depth));
            checkLatched(budget, status, before);
        }
        CHECK_FALSE(budget.adoptWork({}));
        checkLatched(budget, status, before);
        CHECK_FALSE(budget.adoptWork({1, 1, 1, 1, 1}));
        checkLatched(budget, status, before);
        CHECK_FALSE(budget.adoptWork(budget.consumed()));
        checkLatched(budget, status, before);
    };

    SECTION("the first explicit terminal reason wins in either latch order") {
        for (const bool invalidFirst : {false, true}) {
            for (const bool preSpent : {false, true}) {
                CAPTURE(invalidFirst, preSpent);
                geo::WorkBudget budget{LC_OffsetCheckLimits{13, 17, 19, 5, 23}};
                if (preSpent) { REQUIRE(budget.adoptWork({2, 3, 5, 2, 7})); }
                const auto before = budget.consumed();
                const auto status = invalidFirst ? geo::BudgetStatus::InvalidInput : geo::BudgetStatus::LimitExceeded;
                if (invalidFirst) { budget.latchInvalidInput(); }
                else { budget.latchLimitExceeded(); }
                checkLatched(budget, status, before);
                for (int repeat = 0; repeat < 2; ++repeat) {
                    budget.latchInvalidInput();
                    checkLatched(budget, status, before);
                    budget.latchLimitExceeded();
                    checkLatched(budget, status, before);
                }
                retryLatched(budget, status, before);
            }
        }
    }

    SECTION("every zero constructor limit starts with sticky invalid input and no work") {
        const std::array<LC_OffsetCheckLimits, 5> invalidLimits{{
            {0, 1, 1, 1, 1}, {1, 0, 1, 1, 1}, {1, 1, 0, 1, 1},
            {1, 1, 1, 0, 1}, {1, 1, 1, 1, 0}}};
        for (const auto& limits : invalidLimits) {
            CAPTURE(limits.maxCacheSpans, limits.maxVertices, limits.maxDistancePairs,
                    limits.maxDepth, limits.maxOrderingComparisons);
            geo::WorkBudget budget{limits};
            checkLatched(budget, geo::BudgetStatus::InvalidInput, {});
            budget.latchLimitExceeded();
            checkLatched(budget, geo::BudgetStatus::InvalidInput, {});
            budget.latchInvalidInput();
            checkLatched(budget, geo::BudgetStatus::InvalidInput, {});
            retryLatched(budget, geo::BudgetStatus::InvalidInput, {});
        }
    }

    SECTION("admission refusals cannot be replaced by invalid input or refund admitted work") {
        const LC_OffsetCheckLimits limits{3, 5, 7, 2, 11};
        const geo::CheckWork spent{3, 5, 7, 2, 11};
        for (int resource = 0; resource < 6; ++resource) {
            CAPTURE(resource);
            geo::WorkBudget budget{limits};
            REQUIRE(budget.adoptWork(spent));
            const bool accepted = resource == 0 ? budget.consumePieces(1) :
                resource == 1 ? budget.consumeVertices(1) : resource == 2 ? budget.consumePairs(1) :
                resource == 3 ? budget.consumeOrderingComparisons(1) : resource == 4 ? budget.checkDepth(3) :
                budget.adoptWork({1, 1, 1, 3, 1});
            CHECK_FALSE(accepted);
            checkLatched(budget, geo::BudgetStatus::LimitExceeded, spent);
            budget.latchInvalidInput();
            checkLatched(budget, geo::BudgetStatus::LimitExceeded, spent);
            budget.latchLimitExceeded();
            checkLatched(budget, geo::BudgetStatus::LimitExceeded, spent);
            retryLatched(budget, geo::BudgetStatus::LimitExceeded, spent);
        }
    }
}

TEST_CASE("axis inventory ordering comparisons are separately budgeted",
          "[lc-offsetcurve][geometry][cache-budget-accounting][reference-accounting]") {
    for (const bool reversed : {false, true}) {
        // Whole sources start at M_PI_4, not an axis: four maps (six comparisons
        // each), then three ascending or six descending insertion predicates.
        const std::size_t expected = 24 + (reversed ? 6 : 3);
        for (const bool prespent : {false, true}) {
            for (const bool shortCap : {false, true}) {
                CAPTURE(reversed, prespent, shortCap);
                LC_OffsetCheckLimits limits;
                limits.maxOrderingComparisons = expected + (prespent ? 7 : 0) - shortCap;
                geo::WorkBudget budget{limits};
                if (prespent) { REQUIRE(budget.consumeOrderingComparisons(7)); }
                const auto result = geo::analyseCritical(basis(100, .5, 0, 0, reversed), 1, budget);
                CHECK(result.status == (shortCap ? LC_IntersectionStatus::LimitExceeded : LC_IntersectionStatus::Ok));
                CHECK(bool(result.adapter) == !shortCap);
                CHECK(result.work.orderingComparisons == limits.maxOrderingComparisons);
                checkWork(result.work, budget.consumed());
                if (result.adapter) { checkCriticalCoverage(*result.adapter); }
                else {
                    const auto stopped = budget.consumed();
                    CHECK(geo::analyseCritical(basis(100, .5), 1, budget).status == LC_IntersectionStatus::LimitExceeded);
                    checkWork(budget.consumed(), stopped);
                }
            }
        }
    }
    geo::WorkBudget noOrdering{LC_OffsetCheckLimits{4096, 65536, 1u << 20, 20, 1}};
    const auto empty = geo::analyseCritical(basis(100, .5, .01, .05), 1, noOrdering);
    REQUIRE(empty.status == LC_IntersectionStatus::Ok);
    CHECK(empty.work.orderingComparisons == 0); // No axis or occurrence to order/map.
}

TEST_CASE("critical occurrence ordering comparisons are separately budgeted",
          "[lc-offsetcurve][geometry][cache-budget-accounting][reference-accounting]") {
    for (const bool reversed : {false, true}) {
        // Axis work as A05; four cusp maps cost 24, then three/six insertion predicates.
        const std::size_t expected = 24 + (reversed ? 6 : 3) + 24 + (reversed ? 6 : 3);
        for (const bool retained : {false, true}) {
            for (const bool shortCap : {false, true}) {
                CAPTURE(reversed, retained, shortCap);
                LC_OffsetCheckLimits limits;
                // Each retained occurrence adds its six-comparison map and two six-comparison image products.
                limits.maxOrderingComparisons = expected + (retained ? 72 : 0) - shortCap;
                geo::WorkBudget budget{limits};
                const auto source = basis(100, .5, 0, 0, reversed);
                if (retained) {
                    auto result = geo::analyseCriticalRetained(source, -30, budget);
                    CHECK(result.status == (shortCap ? LC_IntersectionStatus::LimitExceeded : LC_IntersectionStatus::Ok));
                    CHECK(bool(result.adapter) == !shortCap); CHECK(bool(result.refinement) == !shortCap);
                    if (result.adapter) { CHECK(result.adapter->criticalOccurrences().size() == 4); checkCriticalCoverage(*result.adapter); }
                } else {
                    const auto result = geo::analyseCritical(source, -30, budget);
                    CHECK(result.status == (shortCap ? LC_IntersectionStatus::LimitExceeded : LC_IntersectionStatus::Ok));
                    CHECK(bool(result.adapter) == !shortCap);
                    if (result.adapter) { CHECK(result.adapter->criticalOccurrences().size() == 4); checkCriticalCoverage(*result.adapter); }
                }
                CHECK(budget.consumed().orderingComparisons == limits.maxOrderingComparisons);
                if (shortCap) {
                    const auto stopped = budget.consumed();
                    CHECK(geo::analyseCriticalRetained(source, -30, budget).status == LC_IntersectionStatus::LimitExceeded);
                    checkWork(budget.consumed(), stopped);
                }
            }
        }
    }
    for (const bool prespent : {false, true}) {
        for (const bool shortCap : {false, true}) {
            LC_OffsetCheckLimits limits;
            // Forward issuance: 54 for the inventory + 4*(6 map + 12 image).
            // A compound contraction stages all four images with the same 18 predicates apiece.
            limits.maxOrderingComparisons = 54 + 72 + 72 + (prespent ? 7 : 0) - shortCap;
            geo::WorkBudget budget{limits};
            if (prespent) { REQUIRE(budget.consumeOrderingComparisons(7)); }
            auto query = geo::analyseCriticalRetained(basis(100, .5), -30, budget);
            REQUIRE(query.status == LC_IntersectionStatus::Ok);
            const auto handle = query.refinement->occurrenceHandle(0);
            const auto original = *query.refinement->snapshot(handle);
            const auto refined = query.refinement->contract(handle, budget);
            CHECK(refined.status == (shortCap ? LC_IntersectionStatus::LimitExceeded : LC_IntersectionStatus::Ok));
            CHECK(bool(refined.snapshot) == !shortCap);
            CHECK(refined.entered.orderingComparisons == 72 - shortCap);
            CHECK(refined.work.orderingComparisons == limits.maxOrderingComparisons);
            if (shortCap) {
                const auto unchanged = *query.refinement->snapshot(handle);
                CHECK(unchanged.occurrence.parameter.lo() == original.occurrence.parameter.lo());
                CHECK(unchanged.occurrence.parameter.hi() == original.occurrence.parameter.hi());
                const auto stopped = budget.consumed();
                CHECK(query.refinement->contract(handle, budget).status == LC_IntersectionStatus::LimitExceeded);
                checkWork(budget.consumed(), stopped);
            }
        }
    }
}

TEST_CASE("critical fixed scans charge before iteration",
          "[lc-offsetcurve][geometry][cache-budget-accounting][reference-accounting]") {
    const auto source = basis(100, .5);
    for (std::size_t visits = 0; visits < 21; ++visits) {
        CAPTURE(visits);
        LC_OffsetCheckLimits limits;
        limits.maxDistancePairs = 1 + visits; // Classify/frame root, then indices -8 through 12.
        geo::WorkBudget budget{limits};
        const auto result = geo::analyseCritical(source, 1, budget);
        CHECK(result.status == LC_IntersectionStatus::LimitExceeded); CHECK_FALSE(result.adapter);
        CHECK(result.work.distancePairs == limits.maxDistancePairs);
        const auto accepted = visits < 10 ? 0 : std::min<std::size_t>(4, visits - 9);
        CHECK(result.work.vertices == 1 + accepted);
        CHECK(result.work.orderingComparisons == 6 * accepted);
        const auto stopped = budget.consumed();
        CHECK(geo::analyseCritical(source, 1, budget).status == LC_IntersectionStatus::LimitExceeded);
        checkWork(budget.consumed(), stopped);
    }
    for (std::size_t adjacent = 0; adjacent <= 3; ++adjacent) {
        CAPTURE(adjacent);
        LC_OffsetCheckLimits limits;
        limits.maxDistancePairs = 1 + 21 + adjacent;
        geo::WorkBudget budget{limits};
        const auto result = geo::analyseCritical(source, 1, budget);
        CHECK(result.status == LC_IntersectionStatus::LimitExceeded); CHECK_FALSE(result.adapter);
        CHECK(result.work.distancePairs == limits.maxDistancePairs);
        CHECK(result.work.vertices == 5); CHECK(result.work.orderingComparisons == 27);
    }
    for (std::size_t visits = 0; visits < 4; ++visits) {
        CAPTURE(visits);
        LC_OffsetCheckLimits limits;
        limits.maxDistancePairs = 1 + 21 + 3 + visits;
        geo::WorkBudget budget{limits};
        const auto result = geo::analyseCritical(source, -25, budget); // Exact minimum stationary threshold.
        CHECK(result.status == LC_IntersectionStatus::LimitExceeded); CHECK_FALSE(result.adapter);
        CHECK(result.work.distancePairs == limits.maxDistancePairs);
        CHECK(result.work.vertices == 5 + visits / 2);
        CHECK(result.work.orderingComparisons == 27);
    }
}

TEST_CASE("geometry root and retained-record capacity checks precede debit",
          "[lc-offsetcurve][geometry][cache-budget-accounting][reference-accounting]") {
    const auto source = basis(100, .5);
    SECTION("either exhausted root dimension refuses both debits before classification") {
        for (const bool vertices : {false, true}) {
            for (const bool retained : {false, true}) {
                LC_OffsetCheckLimits limits{2, 3, 5, 20};
                geo::WorkBudget budget{limits};
                REQUIRE(budget.consumePieces(1)); REQUIRE(budget.checkDepth(7));
                REQUIRE(budget.consumeVertices(vertices ? 3 : 2));
                REQUIRE(budget.consumePairs(vertices ? 4 : 5));
                const auto before = budget.consumed();
                if (retained) {
                    const auto result = geo::analyseCriticalRetained(source, -30, budget);
                    CHECK(result.status == LC_IntersectionStatus::LimitExceeded);
                    CHECK_FALSE(result.adapter); CHECK_FALSE(result.refinement);
                } else {
                    const auto result = geo::analyseCritical(source, -30, budget);
                    CHECK(result.status == LC_IntersectionStatus::LimitExceeded); CHECK_FALSE(result.adapter);
                }
                checkWork(budget.consumed(), before);
            }
        }
    }
    SECTION("retained records are preflighted and published as one complete inventory") {
        geo::WorkBudget legacyBudget;
        const auto legacy = geo::analyseCritical(source, -30, legacyBudget);
        REQUIRE(legacy.status == LC_IntersectionStatus::Ok);
        const auto count = legacy.adapter->criticalOccurrences().size(); REQUIRE(count == 4);
        // Three metadata records, one canonical state, one issuer, and all occurrence records.
        const auto extra = 3 + 1 + 1 + count;
        for (const bool oneShort : {false, true}) {
            LC_OffsetCheckLimits limits;
            limits.maxVertices = legacy.work.vertices + extra - oneShort;
            geo::WorkBudget budget{limits};
            const auto result = geo::analyseCriticalRetained(source, -30, budget);
            CHECK(result.status == (oneShort ? LC_IntersectionStatus::LimitExceeded : LC_IntersectionStatus::Ok));
            CHECK(bool(result.adapter) == !oneShort); CHECK(bool(result.refinement) == !oneShort);
            CHECK(result.work.vertices == legacy.work.vertices + (oneShort ? 0 : extra));
            if (oneShort) { checkWork(result.work, legacy.work); }
        }
    }
    SECTION("staged contraction capacity is prepared before its complete record debit") {
        geo::WorkBudget measured;
        const auto issued = geo::analyseCriticalRetained(source, -30, measured);
        REQUIRE(issued.status == LC_IntersectionStatus::Ok);
        const auto count = issued.refinement->occurrenceCount();
        for (const bool oneShort : {false, true}) {
            LC_OffsetCheckLimits limits;
            limits.maxVertices = issued.work.vertices + 1 + count - oneShort;
            geo::WorkBudget budget{limits};
            auto query = geo::analyseCriticalRetained(source, -30, budget);
            REQUIRE(query.status == LC_IntersectionStatus::Ok);
            const auto handle = query.refinement->occurrenceHandle(0);
            const auto original = *query.refinement->snapshot(handle);
            const auto result = query.refinement->contract(handle, budget);
            CHECK(result.status == (oneShort ? LC_IntersectionStatus::LimitExceeded : LC_IntersectionStatus::Ok));
            CHECK(bool(result.snapshot) == !oneShort);
            CHECK(result.work.vertices == issued.work.vertices + 1 + (oneShort ? 0 : count));
            if (oneShort) {
                const auto unchanged = *query.refinement->snapshot(handle);
                CHECK(unchanged.occurrence.parameter.lo() == original.occurrence.parameter.lo());
                CHECK(unchanged.occurrence.parameter.hi() == original.occurrence.parameter.hi());
                const auto stopped = budget.consumed();
                CHECK(query.refinement->contract(handle, budget).status == LC_IntersectionStatus::LimitExceeded);
                checkWork(budget.consumed(), stopped);
            }
        }
    }
    SECTION("one-shot solver handoff adopts the actual complete delta exactly once on refusal") {
        const auto circle = basis(10, 1);
        const double tolerance = std::ldexp(1.0, -10);
        const auto queryCore = [&](geo::WorkBudget& ledger) {
            auto prepared = geo::prepareReferenceContactProviders(circle, 1, ledger);
            REQUIRE(prepared.status == LC_IntersectionStatus::Ok);
            REQUIRE(ledger.consumePairs(1)); // Local-tolerance conversion in the production handoff.
            LC_IntersectionOptions options;
            options.tolerance = tolerance / prepared.provider->frame().scale();
            options.strict = true; options.anchoredBranches = 1;
            options.maxDepth = options.maxLeafDepth = ledger.depthLimit();
            options.maxBoxPairs = ledger.remainingPairs();
            options.maxLeaves = options.maxGeneratedItems = ledger.remainingVertices();
            options.maxOrderingComparisons = ledger.remainingOrderingComparisons();
            const auto before = ledger.consumed();
            LC_IntersectionWorkStats entered;
            const auto result = findCertifiedIntersections(std::move(prepared.provider), options, &entered);
            CHECK(entered.generatedItems == result.work.generatedItems);
            CHECK(entered.totalBoxPairs == result.work.totalBoxPairs);
            CHECK(entered.orderingComparisons == result.work.orderingComparisons);
            return std::make_pair(before, entered);
        };
        geo::WorkBudget measurement;
        const auto baseline = queryCore(measurement);
        REQUIRE(baseline.second.generatedItems > 1); REQUIRE(baseline.second.orderingComparisons > 1);
        for (const unsigned shortDimension : {0u, 1u, 2u}) {
            CAPTURE(shortDimension);
            LC_OffsetCheckLimits limits;
            limits.maxVertices = baseline.first.vertices + baseline.second.generatedItems - (shortDimension == 1);
            limits.maxOrderingComparisons = baseline.first.orderingComparisons + baseline.second.orderingComparisons -
                (shortDimension == 2);
            geo::WorkBudget direct{limits};
            const auto expected = queryCore(direct);
            REQUIRE(direct.adoptWork({0, expected.second.generatedItems, expected.second.totalBoxPairs,
                expected.second.deepest(), expected.second.orderingComparisons}));
            geo::WorkBudget production{limits};
            const auto refused = geo::analyseReferenceEvents(circle, 1, tolerance, production);
            CHECK(refused.status == LC_IntersectionStatus::LimitExceeded); CHECK_FALSE(refused.analysis);
            // Even the exact core cap leaves no record for the enclosing event analysis.
            checkWork(refused.work, direct.consumed());
            const auto stopped = production.consumed();
            CHECK(geo::analyseReferenceEvents(circle, 1, tolerance, production).status == LC_IntersectionStatus::LimitExceeded);
            checkWork(production.consumed(), stopped);
        }
    }
}

TEST_CASE("Retained critical issuance adds only its explicit publication records",
          "[lc-offsetcurve][geometry][reference-critical-refine]") {
    static_assert(!std::is_copy_constructible_v<geo::CriticalRefinementSession>);
    static_assert(!std::is_default_constructible_v<geo::CriticalRefinementSession>);
    static_assert(std::is_nothrow_move_constructible_v<geo::CriticalRefinementSession>);
    static_assert(std::is_nothrow_move_assignable_v<geo::CriticalRefinementSession>);
    static_assert(!std::is_constructible_v<geo::CriticalOccurrenceHandle, geo::CriticalOccurrence>);
    static_assert(!std::is_constructible_v<geo::CriticalOccurrenceHandle, geo::CriticalOccurrenceSnapshot>);
    static_assert(!std::is_constructible_v<geo::CriticalRefinementSession, geo::CriticalOccurrenceSnapshot>);
    static_assert(!std::is_move_constructible_v<geo::WorkBudget>);
    for (const double distance : {-30.0, 1.0}) {
        const auto data = basis(100.0, 0.5);
        geo::WorkBudget legacyBudget;
        const auto legacy = geo::analyseCritical(data, distance, legacyBudget);
        REQUIRE(legacy.status == LC_IntersectionStatus::Ok);
        geo::WorkBudget retainedBudget;
        auto retained = geo::analyseCriticalRetained(data, distance, retainedBudget);
        REQUIRE(retained.status == LC_IntersectionStatus::Ok);
        REQUIRE(retained.adapter);
        REQUIRE(retained.refinement);
        const auto n = retained.refinement->occurrenceCount();
        const std::size_t canonical = distance < 0.0 ? 1 : 0;
        CHECK(n == legacy.adapter->criticalOccurrences().size());
        CHECK(retained.work.vertices == legacy.work.vertices + 3 + canonical + n + 1);
        CHECK(retained.work.distancePairs == legacy.work.distancePairs + 1 + n);
        CHECK(retained.work.deepest == legacy.work.deepest);
        checkWork(retained.work, retainedBudget.consumed());
        checkWork(retained.refinement->lastMatchingWork(), retained.work);
        checkCriticalCoverage(*retained.adapter);
        const auto beforeSecond = retainedBudget.consumed();
        auto second = geo::analyseCriticalRetained(data, distance, retainedBudget);
        REQUIRE(second.status == LC_IntersectionStatus::Ok);
        CHECK(second.work.vertices == beforeSecond.vertices + legacy.work.vertices + 3 + canonical + n);
        CHECK(second.work.distancePairs == beforeSecond.distancePairs + legacy.work.distancePairs + 1 + n);
        const auto beforeLegacy = retainedBudget.consumed();
        const auto againLegacy = geo::analyseCritical(data, distance, retainedBudget);
        REQUIRE(againLegacy.status == LC_IntersectionStatus::Ok);
        CHECK(againLegacy.work.vertices == beforeLegacy.vertices + legacy.work.vertices);
        CHECK(againLegacy.work.distancePairs == beforeLegacy.distancePairs + legacy.work.distancePairs);
    }
    const auto emptyArc = basis(100.0, 0.5, 0.01, 0.05);
    geo::WorkBudget emptyLegacyBudget;
    const auto emptyLegacy = geo::analyseCritical(emptyArc, -30.0, emptyLegacyBudget);
    REQUIRE(emptyLegacy.status == LC_IntersectionStatus::Ok);
    geo::WorkBudget budget;
    auto noOccurrence = geo::analyseCriticalRetained(emptyArc, -30.0, budget);
    REQUIRE(noOccurrence.status == LC_IntersectionStatus::Ok);
    CHECK(noOccurrence.adapter->regime() == geo::CriticalRegime::IntermediateThresholds);
    CHECK(noOccurrence.refinement->occurrenceCount() == 0);
    CHECK(noOccurrence.work.vertices == emptyLegacy.work.vertices + 5);
    CHECK(noOccurrence.work.distancePairs == emptyLegacy.work.distancePairs + 1);
    CHECK_FALSE(noOccurrence.refinement->snapshot(noOccurrence.refinement->occurrenceHandle(0)));
}

TEST_CASE("Retained critical publication has exact one less and prespent cumulative caps",
          "[lc-offsetcurve][geometry][reference-critical-refine]") {
    const auto data = basis(100.0, 0.5);
    geo::WorkBudget baseline;
    auto reference = geo::analyseCriticalRetained(data, -30.0, baseline);
    REQUIRE(reference.status == LC_IntersectionStatus::Ok);
    const auto work = reference.work;
    for (int cap = 0; cap < 3; ++cap) {
        LC_OffsetCheckLimits limits{1, work.vertices, work.distancePairs, 20};
        if (cap == 1) { --limits.maxVertices; }
        if (cap == 2) { --limits.maxDistancePairs; }
        geo::WorkBudget budget{limits};
        auto result = geo::analyseCriticalRetained(data, -30.0, budget);
        CHECK(result.status == (cap == 0 ? LC_IntersectionStatus::Ok : LC_IntersectionStatus::LimitExceeded));
        CHECK(bool(result.adapter) == (cap == 0));
        CHECK(bool(result.refinement) == (cap == 0));
        CHECK(result.work.vertices <= limits.maxVertices);
        CHECK(result.work.distancePairs <= limits.maxDistancePairs);
        checkWork(result.work, budget.consumed());
    }
    geo::WorkBudget prespent{LC_OffsetCheckLimits{1, work.vertices + 2, work.distancePairs + 3, 20}};
    REQUIRE(prespent.consumeVertices(2));
    REQUIRE(prespent.consumePairs(3));
    auto issued = geo::analyseCriticalRetained(data, -30.0, prespent);
    REQUIRE(issued.status == LC_IntersectionStatus::Ok);
    CHECK(issued.work.vertices == work.vertices + 2);
    CHECK(issued.work.distancePairs == work.distancePairs + 3);
}

TEST_CASE("Retained critical preparation preserves the original failed split boundary",
          "[lc-offsetcurve][geometry][reference-critical-depth]") {
    LC_OffsetCheckLimits limits;
    limits.maxVertices = 6;
    geo::WorkBudget budget{limits};
    const auto result = geo::analyseCriticalRetained(basis(100.0, 0.5), -30.0, budget);
    CHECK(result.status == LC_IntersectionStatus::LimitExceeded);
    CHECK_FALSE(result.adapter);
    CHECK_FALSE(result.refinement);
    CHECK(result.work.vertices == 6);
    CHECK(result.work.distancePairs == 26); // Includes the three admitted axis adjacency checks.
    CHECK(result.work.deepest == 0);
    checkWork(result.work, budget.consumed());

    geo::WorkBudget examined{LC_OffsetCheckLimits{1, 8, 26, 20}};
    const auto later = geo::analyseCritical(basis(100.0, 0.5), -30.0, examined);
    CHECK(later.status == LC_IntersectionStatus::LimitExceeded);
    CHECK(later.work.vertices == 8);
    CHECK(later.work.distancePairs == 26);
    CHECK(later.work.deepest == 1);
}

TEST_CASE("Retained critical snapshots contain independently computed parameters and local world images",
          "[lc-offsetcurve][geometry][reference-critical-refine]") {
    const std::array<RS_EllipseData, 5> sources{{basis(100.0, 0.5), basis(100.0, 0.5, 0.0, 0.0, true),
        basis(100.0, 0.5, 6.0, 0.5), basis(100.0, 0.5, 0.5, 6.0, true), basis(100.0, 0.5, 0.0, 2.0)}};
    for (auto data : sources) {
        for (const double scale : {1e-250, 1.0, 1e250}) {
            CAPTURE(data.angle1, data.angle2, data.reversed, scale);
            data.majorP = RS_Vector{80.0 * scale, 60.0 * scale};
            data.center = RS_Vector{123.0 * scale, -87.0 * scale};
            const double distance = -30.0 * scale;
            geo::WorkBudget budget;
            auto result = geo::analyseCriticalRetained(data, distance, budget);
            REQUIRE(result.status == LC_IntersectionStatus::Ok);
            REQUIRE(result.refinement->occurrenceCount() > 0);
            for (std::size_t i = 0; i < result.refinement->occurrenceCount(); ++i) {
                auto previous = result.refinement->snapshot(result.refinement->occurrenceHandle(i));
                REQUIRE(previous);
                checkCriticalSnapshot(data, distance, result.adapter->frame(), *previous);
            }
            const auto handle = result.refinement->occurrenceHandle(0);
            for (unsigned round = 0; round < 5; ++round) {
                const auto before = result.refinement->snapshot(handle);
                REQUIRE(before);
                const auto refined = result.refinement->contract(handle, budget);
                REQUIRE((refined.status == LC_IntersectionStatus::Ok ||
                         refined.status == LC_IntersectionStatus::AmbiguousTopology));
                if (refined.status != LC_IntersectionStatus::Ok) { break; }
                REQUIRE(refined.snapshot);
                checkNested(refined.snapshot->occurrence.parameter, before->occurrence.parameter);
                checkNested(refined.snapshot->imageX, before->imageX);
                checkNested(refined.snapshot->imageY, before->imageY);
                for (std::size_t i = 0; i < result.refinement->occurrenceCount(); ++i) {
                    const auto symmetric = result.refinement->snapshot(result.refinement->occurrenceHandle(i));
                    REQUIRE(symmetric);
                    checkCriticalSnapshot(data, distance, result.adapter->frame(), *symmetric);
                    CHECK(symmetric->admittedRounds == refined.snapshot->admittedRounds);
                }
            }
        }
    }
}

TEST_CASE("Critical inverse refinement progresses independently of an adjacent scalar bracket",
          "[lc-offsetcurve][geometry][reference-critical-refine]") {
    for (const double distance : {-30.0, -100.0}) {
        const auto data = basis(100.0, 0.5, 0.0, 2.0);
        geo::WorkBudget legacyBudget;
        const auto legacy = geo::analyseCritical(data, distance, legacyBudget);
        const auto legacyStatus = static_cast<int>(legacy.status);
        const auto legacyReason = static_cast<int>(legacy.reason);
        CAPTURE(distance, legacyStatus, legacyReason, legacy.work.vertices,
                legacy.work.distancePairs, legacy.work.deepest);
        REQUIRE(legacy.status == LC_IntersectionStatus::Ok);
        geo::WorkBudget budget;
        auto result = geo::analyseCriticalRetained(data, distance, budget);
        REQUIRE(result.status == LC_IntersectionStatus::Ok);
        REQUIRE(result.refinement->occurrenceCount() == 1);
        CHECK(result.work.vertices == legacy.work.vertices + 6);
        CHECK(result.work.distancePairs == legacy.work.distancePairs + 2);
        CHECK(result.work.deepest == legacy.work.deepest);
        const auto handle = result.refinement->occurrenceHandle(0);
        bool independentProgress = false;
        bool strongComplementProgress = false;
        for (unsigned round = 0; round < 8; ++round) {
            const auto before = result.refinement->snapshot(handle);
            REQUIRE(before);
            REQUIRE(before->canonical);
            const auto refined = result.refinement->contract(handle, budget);
            REQUIRE((refined.status == LC_IntersectionStatus::Ok ||
                     refined.status == LC_IntersectionStatus::AmbiguousTopology));
            const auto after = result.refinement->snapshot(handle);
            REQUIRE(after);
            checkCriticalSnapshot(data, distance, result.adapter->frame(), *after);
            if (refined.status != LC_IntersectionStatus::Ok) { break; }
            const auto& a = before->canonical->variable;
            const auto& b = after->canonical->variable;
            if (a.lo() == b.lo() && a.hi() == b.hi() &&
                after->canonical->inverseAngle.width() < before->canonical->inverseAngle.width()) {
                independentProgress = true;
                CHECK(std::nextafter(a.lo(), std::numeric_limits<double>::infinity()) >= a.hi());
                strongComplementProgress |= after->canonical->complement &&
                    after->canonical->inverseAngle.width() < 0.1 * before->canonical->inverseAngle.width();
            }
        }
        if (distance == -100.0) {
            CHECK(independentProgress);
            CHECK(strongComplementProgress);
        }
    }
    auto data = basis(100.0, 0.5, 0.0, 2.0);
    data.majorP = RS_Vector{120.0, 40.0};
    geo::WorkBudget budget;
    auto special = geo::analyseCriticalRetained(data, -125.0, budget);
    REQUIRE(special.status == LC_IntersectionStatus::Ok);
    const auto handle = special.refinement->occurrenceHandle(0);
    const auto issuedHalf = special.refinement->snapshot(handle);
    REQUIRE(issuedHalf);
    REQUIRE(issuedHalf->canonical);
    CHECK(issuedHalf->canonical->variable.hi() == 0.5);
    CHECK(issuedHalf->canonical->inverseAngle.hi() > 1.0);
    checkCriticalSnapshot(data, -125.0, special.adapter->frame(), *issuedHalf);
    for (unsigned round = 0; round < 5; ++round) {
        const auto snapshot = special.refinement->snapshot(handle);
        REQUIRE(snapshot);
        REQUIRE(snapshot->canonical);
        CHECK(snapshot->canonical->variable.contains(0.5));
        checkCriticalSnapshot(data, -125.0, special.adapter->frame(), *snapshot);
        const auto refined = special.refinement->contract(handle, budget);
        if (refined.status == LC_IntersectionStatus::AmbiguousTopology) { break; }
        REQUIRE(refined.status == LC_IntersectionStatus::Ok);
    }
}

TEST_CASE("Critical compound rounds admit only entered stages and publish symmetries atomically",
          "[lc-offsetcurve][geometry][reference-critical-refine]") {
    const auto data = basis(100.0, 0.5);
    geo::WorkBudget baseline;
    auto reference = geo::analyseCriticalRetained(data, -30.0, baseline);
    REQUIRE(reference.status == LC_IntersectionStatus::Ok);
    const auto initial = reference.work;
    const auto n = reference.refinement->occurrenceCount();
    REQUIRE(n == 4);
    const auto first = reference.refinement->contract(reference.refinement->occurrenceHandle(0), baseline);
    REQUIRE(first.status == LC_IntersectionStatus::Ok);
    CHECK(first.entered.vertices == 1 + n);
    CHECK(first.entered.distancePairs == 3 + n);
    REQUIRE(first.snapshot);
    CHECK(first.entered.deepest == first.snapshot->admittedRounds);
    const auto initialDepth = first.snapshot->admittedRounds - 1;
    for (int stop = 0; stop < 7; ++stop) {
        LC_OffsetCheckLimits limits{1, initial.vertices + 1 + n, initial.distancePairs + 3 + n, 20};
        if (stop == 1) { --limits.maxVertices; }
        if (stop == 2) { --limits.maxDistancePairs; }
        if (stop == 3) { limits.maxVertices = initial.vertices; }
        if (stop == 4) { limits.maxDistancePairs = initial.distancePairs; }
        if (stop == 5) { limits.maxDistancePairs = initial.distancePairs + 1; }
        if (stop == 6) { limits.maxDistancePairs = initial.distancePairs + 2; }
        geo::WorkBudget budget{limits};
        auto issued = geo::analyseCriticalRetained(data, -30.0, budget);
        REQUIRE(issued.status == LC_IntersectionStatus::Ok);
        const auto handle = issued.refinement->occurrenceHandle(0);
        std::vector<geo::CriticalOccurrenceSnapshot> original;
        for (std::size_t i = 0; i < n; ++i) {
            const auto snapshot = issued.refinement->snapshot(issued.refinement->occurrenceHandle(i));
            REQUIRE(snapshot);
            original.push_back(*snapshot);
        }
        const auto refined = issued.refinement->contract(handle, budget);
        CHECK(refined.status == (stop == 0 ? LC_IntersectionStatus::Ok : LC_IntersectionStatus::LimitExceeded));
        CHECK(bool(refined.snapshot) == (stop == 0));
        CHECK(refined.workSource == geo::CriticalWorkSource::CurrentBudget);
        CHECK(refined.entered.vertices == (stop == 3 ? 0 : stop == 1 || stop >= 4 ? 1 : 1 + n));
        const std::size_t expectedPairs = stop == 0 ? 3 + n : stop == 1 ? 3 : stop == 2 ? 2 + n :
            stop <= 4 ? 0 : stop == 5 ? 1 : 2;
        CHECK(refined.entered.distancePairs == expectedPairs);
        CHECK(refined.entered.deepest == (stop == 3 || stop == 4 ? 0 : initialDepth + 1));
        checkWork(refined.work, budget.consumed());
        for (std::size_t i = 0; i < n; ++i) {
            const auto current = issued.refinement->snapshot(issued.refinement->occurrenceHandle(i));
            REQUIRE(current);
            CHECK(current->admittedRounds == (stop == 3 || stop == 4 ? initialDepth : initialDepth + 1));
            if (stop == 0) {
                checkCriticalSnapshot(data, -30.0, issued.adapter->frame(), *current);
                checkNested(current->occurrence.parameter, original[i].occurrence.parameter);
            } else {
                CHECK(current->occurrence.parameter.lo() == original[i].occurrence.parameter.lo());
                CHECK(current->occurrence.parameter.hi() == original[i].occurrence.parameter.hi());
                CHECK(current->imageX.lo() == original[i].imageX.lo());
                CHECK(current->imageX.hi() == original[i].imageX.hi());
                CHECK(current->imageY.lo() == original[i].imageY.lo());
                CHECK(current->imageY.hi() == original[i].imageY.hi());
                CHECK(current->canonical->inverseAngle.lo() == original[i].canonical->inverseAngle.lo());
                CHECK(current->canonical->inverseAngle.hi() == original[i].canonical->inverseAngle.hi());
            }
        }
        if (stop != 0) {
            const auto cached = budget.consumed();
            const auto again = issued.refinement->contract(issued.refinement->occurrenceHandle(n - 1), budget);
            CHECK(again.status == LC_IntersectionStatus::LimitExceeded);
            checkWork(again.entered, {});
            checkWork(budget.consumed(), cached);
            const auto invalid = issued.refinement->contract({}, budget);
            CHECK(invalid.status == LC_IntersectionStatus::InvalidInput);
            checkWork(invalid.entered, {});
            CHECK(issued.refinement->contract(handle, budget).status == LC_IntersectionStatus::LimitExceeded);
        }
    }
}

TEST_CASE("Critical canonical history is not a global depth maximum or reset by another occurrence",
          "[lc-offsetcurve][geometry][reference-critical-depth][reference-critical-refine]") {
    geo::WorkBudget budget;
    REQUIRE(budget.checkDepth(19));
    auto issued = geo::analyseCriticalRetained(basis(100.0, 0.5), -30.0, budget);
    REQUIRE(issued.status == LC_IntersectionStatus::Ok);
    const auto a = issued.refinement->occurrenceHandle(0);
    const auto b = issued.refinement->occurrenceHandle(1);
    const auto before = issued.refinement->snapshot(a);
    REQUIRE(before);
    CHECK(before->admittedRounds < 19);
    CHECK(issued.work.deepest == 19);
    const auto first = issued.refinement->contract(a, budget);
    REQUIRE(first.status == LC_IntersectionStatus::Ok);
    CHECK(first.entered.deepest == before->admittedRounds + 1);
    CHECK(first.work.deepest == 19);
    const auto after = issued.refinement->snapshot(b);
    REQUIRE(after);
    CHECK(after->admittedRounds == first.entered.deepest);
    const auto second = issued.refinement->contract(b, budget);
    REQUIRE(second.status == LC_IntersectionStatus::Ok);
    CHECK(second.entered.deepest == first.entered.deepest + 1);
    const auto shared = issued.refinement->snapshot(a);
    REQUIRE(shared);
    CHECK(shared->admittedRounds == second.entered.deepest);

    geo::WorkBudget shortBudget{LC_OffsetCheckLimits{4096, 65536, std::size_t{1} << 20,
        before->admittedRounds}};
    auto shortSession = geo::analyseCriticalRetained(basis(100.0, 0.5), -30.0, shortBudget);
    REQUIRE(shortSession.status == LC_IntersectionStatus::Ok);
    const auto handle = shortSession.refinement->occurrenceHandle(0);
    const auto exhausted = shortSession.refinement->contract(handle, shortBudget);
    CHECK(exhausted.status == LC_IntersectionStatus::LimitExceeded);
    checkWork(exhausted.entered, {});
    const auto cached = shortSession.refinement->snapshot(handle);
    REQUIRE(cached);
    CHECK(cached->admittedRounds == before->admittedRounds);
}

TEST_CASE("Critical stationary occurrences cache exact images and do not invent contraction work",
          "[lc-offsetcurve][geometry][reference-critical-refine]") {
    for (const double distance : {-25.0, -200.0}) {
        auto data = basis(100.0, 0.5);
        data.majorP = RS_Vector{80.0, 60.0};
        data.center = RS_Vector{1e6, -2e6};
        geo::WorkBudget budget;
        auto issued = geo::analyseCriticalRetained(data, distance, budget);
        REQUIRE(issued.status == LC_IntersectionStatus::Ok);
        REQUIRE(issued.refinement->occurrenceCount() == 2);
        for (std::size_t i = 0; i < issued.refinement->occurrenceCount(); ++i) {
            const auto handle = issued.refinement->occurrenceHandle(i);
            const auto before = issued.refinement->snapshot(handle);
            REQUIRE(before);
            CHECK_FALSE(before->canonical);
            checkCriticalSnapshot(data, distance, issued.adapter->frame(), *before);
            const auto work = budget.consumed();
            const auto result = issued.refinement->contract(handle, budget);
            CHECK(result.status == LC_IntersectionStatus::AmbiguousTopology);
            CHECK_FALSE(result.snapshot);
            checkWork(result.entered, {});
            checkWork(result.work, work);
            const auto cached = issued.refinement->snapshot(handle);
            REQUIRE(cached);
            CHECK(cached->admittedRounds == before->admittedRounds);
        }
        budget.latchLimitExceeded();
        CHECK(issued.refinement->contract(issued.refinement->occurrenceHandle(0), budget).status ==
            LC_IntersectionStatus::LimitExceeded);
    }
}

TEST_CASE("Critical handles and diagnostic copies cannot manufacture or alter issued geometry",
          "[lc-offsetcurve][geometry][reference-critical-refine]") {
    geo::WorkBudget budget;
    auto issued = geo::analyseCriticalRetained(basis(100.0, 0.5), -30.0, budget);
    auto other = geo::analyseCriticalRetained(basis(100.0, 0.5), -30.0, budget);
    REQUIRE(issued.status == LC_IntersectionStatus::Ok);
    REQUIRE(other.status == LC_IntersectionStatus::Ok);
    const auto handle = issued.refinement->occurrenceHandle(0);
    const auto original = issued.refinement->snapshot(handle);
    REQUIRE(original);
    auto copy = *original;
    copy.occurrence.parameter = LC_Interval::point(-99.0);
    copy.imageX = LC_Interval::point(99.0);
    copy.canonical->variable = LC_Interval::point(0.0);
    copy.admittedRounds = std::numeric_limits<unsigned>::max();
    const auto current = issued.refinement->snapshot(handle);
    REQUIRE(current);
    CHECK(current->occurrence.parameter.lo() == original->occurrence.parameter.lo());
    CHECK(current->imageX.lo() == original->imageX.lo());
    CHECK(current->canonical->variable.lo() == original->canonical->variable.lo());
    CHECK(current->admittedRounds == original->admittedRounds);
    const auto work = budget.consumed();
    for (const auto bad : {geo::CriticalOccurrenceHandle{}, issued.refinement->occurrenceHandle(99),
                           other.refinement->occurrenceHandle(0)}) {
        const auto result = issued.refinement->contract(bad, budget);
        CHECK(result.status == LC_IntersectionStatus::InvalidInput);
        CHECK_FALSE(result.snapshot);
        checkWork(result.entered, {});
        checkWork(budget.consumed(), work);
        CHECK_FALSE(issued.refinement->snapshot(bad));
    }
    CHECK(issued.refinement->contract(handle, budget).status == LC_IntersectionStatus::Ok);
}

TEST_CASE("Critical refiners preserve independent adapter ownership and issuer identity through moves",
          "[lc-offsetcurve][geometry][reference-critical-refine]") {
    geo::WorkBudget budget;
    auto issued = geo::analyseCriticalRetained(basis(100.0, 0.5), -30.0, budget);
    REQUIRE(issued.status == LC_IntersectionStatus::Ok);
    const auto handle = issued.refinement->occurrenceHandle(0);
    std::unique_ptr<const LC_ParametricCurves> provider = std::move(issued.adapter);
    geo::CriticalRefinementSession moved{std::move(*issued.refinement)};
    CHECK(issued.refinement->occurrenceCount() == 0);
    CHECK_FALSE(issued.refinement->snapshot(handle));
    CHECK(issued.refinement->contract(handle, budget).status == LC_IntersectionStatus::InvalidInput);
    REQUIRE(moved.snapshot(handle));
    CHECK(moved.contract(handle, budget).status == LC_IntersectionStatus::Ok);
    const auto& segment = provider->segments().front();
    LC_Interval x, y, dx, dy;
    CHECK(provider->bound(0, segment.t0, segment.t1, x, y, dx, dy));
    provider.reset();
    CHECK(moved.contract(handle, budget).status == LC_IntersectionStatus::Ok);
    auto destination = geo::analyseCriticalRetained(basis(100.0, 0.5), -30.0, budget);
    REQUIRE(destination.status == LC_IntersectionStatus::Ok);
    const auto overwritten = destination.refinement->occurrenceHandle(0);
    *destination.refinement = std::move(moved);
    CHECK_FALSE(destination.refinement->snapshot(overwritten));
    CHECK(destination.refinement->snapshot(handle));
    CHECK(moved.occurrenceCount() == 0);
    CHECK_FALSE(moved.snapshot(handle));
    destination.refinement.reset();
    CHECK_FALSE(moved.snapshot(handle));

    auto adapterOnly = geo::analyseCriticalRetained(basis(100.0, 0.5), -30.0, budget);
    REQUIRE(adapterOnly.status == LC_IntersectionStatus::Ok);
    adapterOnly.refinement.reset();
    checkCriticalCoverage(*adapterOnly.adapter);
}

TEST_CASE("Critical budget identity rejects fresh expired and same address replacements without resetting work",
          "[lc-offsetcurve][geometry][reference-critical-refine]") {
    std::optional<geo::WorkBudget> owner{std::in_place};
    const auto* originalAddress = &*owner;
    auto issued = geo::analyseCriticalRetained(basis(100.0, 0.5), -30.0, *owner);
    REQUIRE(issued.status == LC_IntersectionStatus::Ok);
    const auto handle = issued.refinement->occurrenceHandle(0);
    const auto last = issued.refinement->lastMatchingWork();
    geo::WorkBudget fresh;
    REQUIRE(fresh.consumePairs(7));
    const auto rejected = issued.refinement->contract(handle, fresh);
    CHECK(rejected.status == LC_IntersectionStatus::InvalidInput);
    CHECK(rejected.workSource == geo::CriticalWorkSource::LastMatchingBudget);
    checkWork(rejected.work, last);
    checkWork(rejected.entered, {});
    CHECK(fresh.consumed().distancePairs == 7);
    CHECK(issued.refinement->contract(handle, *owner).status == LC_IntersectionStatus::Ok);
    const auto updated = issued.refinement->lastMatchingWork();
    owner.reset();
    REQUIRE(issued.refinement->snapshot(handle));
    owner.emplace();
    CHECK(&*owner == originalAddress);
    const auto recreated = issued.refinement->contract(handle, *owner);
    CHECK(recreated.status == LC_IntersectionStatus::InvalidInput);
    CHECK(recreated.workSource == geo::CriticalWorkSource::LastMatchingBudget);
    checkWork(recreated.work, updated);
    checkWork(recreated.entered, {});
    checkWork(owner->consumed(), {});
    checkWork(issued.refinement->lastMatchingWork(), updated);
}

TEST_CASE("Critical interval world conversion is outward cancellation safe and clears failures",
          "[lc-offsetcurve][geometry][reference-critical-refine]") {
    auto data = basis(1e308, 1.0);
    data.center = RS_Vector{-1e308, 1e308};
    const auto frame = geo::ReferenceFrame::create(data, 0.0);
    REQUIRE(frame);
    const LC_Interval local = LC_Interval::point(1e308 / frame->scale());
    LC_Interval x, y;
    REQUIRE(frame->pointToWorld(local, -local, x, y));
    const CriticalFloat world = CriticalFloat(-1e308) + CriticalFloat(local.lo()) * CriticalFloat(frame->scale());
    checkCriticalEnclosure(x, world);
    checkCriticalEnclosure(y, -world);
    const LC_Interval twice = LC_Interval::point(2.0 * (1e308 / frame->scale()));
    REQUIRE(frame->pointToWorld(twice, -twice, x, y));
    checkCriticalEnclosure(x, CriticalFloat(-1e308) + CriticalFloat(twice.lo()) * CriticalFloat(frame->scale()));
    const LC_Interval excessive = LC_Interval::point(std::numeric_limits<double>::max());
    CHECK_FALSE(frame->pointToWorld(excessive, excessive, x, y));
    CHECK_FALSE(x.isValid());
    CHECK_FALSE(y.isValid());
    x = y = LC_Interval::point(1.0);
    CHECK_FALSE(frame->pointToWorld({}, local, x, y));
    CHECK_FALSE(x.isValid());
    CHECK_FALSE(y.isValid());
    LC_Interval alias = local;
    REQUIRE(frame->pointToWorld(alias, -local, alias, y));
    checkCriticalEnclosure(alias, world);
}

TEST_CASE("Critical retained failures never publish a partial trusted session",
          "[lc-offsetcurve][geometry][reference-critical-refine]") {
    for (const auto& item : std::array<std::pair<RS_EllipseData, double>, 4>{{
        {basis(5.0, 1.0), -5.0}, {basis(5.0, 2.0), 1.0},
        {basis(1.0, 1e-250, 0.0, 1.0), -0.25}, {basis(1e-300, 0.5), -1e300}}}) {
        geo::WorkBudget budget;
        const auto failed = geo::analyseCriticalRetained(item.first, item.second, budget);
        CHECK(failed.status != LC_IntersectionStatus::Ok);
        CHECK_FALSE(failed.adapter);
        CHECK_FALSE(failed.refinement);
        checkWork(failed.work, budget.consumed());
    }
    geo::WorkBudget zero{LC_OffsetCheckLimits{1, 1, 1, 0}};
    const auto invalid = geo::analyseCriticalRetained(basis(100.0, 0.5), -30.0, zero);
    CHECK(invalid.status == LC_IntersectionStatus::InvalidInput);
    CHECK_FALSE(invalid.adapter);
    CHECK_FALSE(invalid.refinement);
    checkWork(invalid.work, {});
}

TEST_CASE("Critical no progress refuses without publishing or erasing admitted history",
          "[lc-offsetcurve][geometry][reference-critical-refine][reference-critical-depth]") {
    geo::WorkBudget budget;
    const auto data = basis(100.0, 0.5, 0.0, 2.0);
    geo::WorkBudget legacyBudget;
    const auto legacy = geo::analyseCritical(data, -100.0, legacyBudget);
    const auto legacyStatus = static_cast<int>(legacy.status);
    const auto legacyReason = static_cast<int>(legacy.reason);
    CAPTURE(legacyStatus, legacyReason, legacy.work.vertices, legacy.work.distancePairs, legacy.work.deepest);
    REQUIRE(legacy.status == LC_IntersectionStatus::Ok);
    auto issued = geo::analyseCriticalRetained(data, -100.0, budget);
    REQUIRE(issued.status == LC_IntersectionStatus::Ok);
    CHECK(issued.work.vertices == legacy.work.vertices + 6);
    CHECK(issued.work.distancePairs == legacy.work.distancePairs + 2);
    CHECK(issued.work.deepest == legacy.work.deepest);
    const auto handle = issued.refinement->occurrenceHandle(0);
    bool refused = false;
    for (unsigned round = 0; round < 10; ++round) {
        const auto before = issued.refinement->snapshot(handle);
        REQUIRE(before);
        const auto work = budget.consumed();
        const auto result = issued.refinement->contract(handle, budget);
        if (result.status == LC_IntersectionStatus::Ok) {
            REQUIRE(result.snapshot);
            checkCriticalSnapshot(data, -100.0, issued.adapter->frame(), *result.snapshot);
            continue;
        }
        REQUIRE(result.status == LC_IntersectionStatus::AmbiguousTopology);
        CHECK_FALSE(result.snapshot);
        CHECK(result.entered.vertices == 1);
        CHECK(result.entered.distancePairs == 3);
        CHECK(result.entered.deepest == before->admittedRounds + 1);
        CHECK(result.work.vertices == work.vertices + 1);
        CHECK(result.work.distancePairs == work.distancePairs + 3);
        const auto cached = issued.refinement->snapshot(handle);
        REQUIRE(cached);
        CHECK(cached->admittedRounds == before->admittedRounds + 1);
        CHECK(cached->occurrence.parameter.lo() == before->occurrence.parameter.lo());
        CHECK(cached->occurrence.parameter.hi() == before->occurrence.parameter.hi());
        CHECK(cached->canonical->inverseAngle.lo() == before->canonical->inverseAngle.lo());
        CHECK(cached->canonical->inverseAngle.hi() == before->canonical->inverseAngle.hi());
        checkCriticalSnapshot(data, -100.0, issued.adapter->frame(), *cached);
        refused = true;
        break;
    }
    CHECK(refused);
}

TEST_CASE("Critical close cusps retain distinct handles and common state without proximity merging",
          "[lc-offsetcurve][geometry][reference-critical-refine]") {
    for (const bool reversed : {false, true}) {
        const auto data = basis(100.0, 0.5, 0.0, 0.0, reversed);
        const double distance = -std::nextafter(25.0, std::numeric_limits<double>::infinity());
        geo::WorkBudget budget;
        auto issued = geo::analyseCriticalRetained(data, distance, budget);
        REQUIRE(issued.status == LC_IntersectionStatus::Ok);
        REQUIRE(issued.refinement->occurrenceCount() == 4);
        for (unsigned round = 0; round < 3; ++round) {
            double end = -1.0;
            for (std::size_t i = 0; i < 4; ++i) {
                const auto snapshot = issued.refinement->snapshot(issued.refinement->occurrenceHandle(i));
                REQUIRE(snapshot);
                CHECK(snapshot->occurrence.parameter.lo() > end);
                end = snapshot->occurrence.parameter.hi();
                checkCriticalSnapshot(data, distance, issued.adapter->frame(), *snapshot);
            }
            const auto refined = issued.refinement->contract(issued.refinement->occurrenceHandle(round), budget);
            REQUIRE((refined.status == LC_IntersectionStatus::Ok ||
                     refined.status == LC_IntersectionStatus::AmbiguousTopology));
            CHECK(issued.refinement->occurrenceCount() == 4);
        }
    }
}

TEST_CASE("Retained critical analysis respects real axis membership and thin general cusp geometry",
          "[lc-offsetcurve][geometry][reference-critical-refine]") {
    for (const bool reversed : {false, true}) {
        const auto axisArc = reversed ? basis(5.0, 0.5, M_PI_2, 0.0, true) : basis(5.0, 0.5, 0.0, M_PI_2);
        geo::WorkBudget axisBudget;
        const auto axis = geo::analyseCriticalRetained(axisArc, -10.0, axisBudget);
        REQUIRE(axis.status == LC_IntersectionStatus::Ok);
        CHECK(axis.adapter->regime() == geo::CriticalRegime::MaximumStationary);
        CHECK(axis.refinement->occurrenceCount() == 0);
        for (const auto& entry : std::array<std::pair<double, double>, 2>{{{0.02, -5.3}, {0.01, -10.0}}}) {
            const auto data = basis(100.0, entry.first, 0.0, 0.0, reversed);
            geo::WorkBudget budget;
            auto issued = geo::analyseCriticalRetained(data, entry.second, budget);
            REQUIRE(issued.status == LC_IntersectionStatus::Ok);
            REQUIRE(issued.refinement->occurrenceCount() == 4);
            const auto handle = issued.refinement->occurrenceHandle(0);
            const auto refined = issued.refinement->contract(handle, budget);
            REQUIRE(refined.status == LC_IntersectionStatus::Ok);
            for (std::size_t i = 0; i < 4; ++i) {
                const auto snapshot = issued.refinement->snapshot(issued.refinement->occurrenceHandle(i));
                REQUIRE(snapshot);
                checkCriticalSnapshot(data, entry.second, issued.adapter->frame(), *snapshot);
            }
        }
    }
}

TEST_CASE("Critical resource latches survive fresh budgets moves and saturated shared counters",
          "[lc-offsetcurve][geometry][reference-critical-refine]") {
    const auto maximum = std::numeric_limits<std::size_t>::max();
    geo::WorkBudget budget{LC_OffsetCheckLimits{1, maximum, maximum, 20}};
    auto issued = geo::analyseCriticalRetained(basis(100.0, 0.5), -30.0, budget);
    REQUIRE(issued.status == LC_IntersectionStatus::Ok);
    const auto handle = issued.refinement->occurrenceHandle(0);
    const auto before = issued.refinement->snapshot(handle);
    REQUIRE(before);
    REQUIRE(budget.consumeVertices(maximum - budget.consumed().vertices));
    const auto result = issued.refinement->contract(handle, budget);
    CHECK(result.status == LC_IntersectionStatus::LimitExceeded);
    CHECK(result.work.vertices == maximum);
    checkWork(result.entered, {});
    const auto cached = issued.refinement->snapshot(handle);
    REQUIRE(cached);
    CHECK(cached->admittedRounds == before->admittedRounds);
    geo::WorkBudget fresh;
    const auto wrong = issued.refinement->contract(handle, fresh);
    CHECK(wrong.status == LC_IntersectionStatus::InvalidInput);
    CHECK(wrong.workSource == geo::CriticalWorkSource::LastMatchingBudget);
    checkWork(wrong.work, result.work);
    checkWork(wrong.entered, {});
    checkWork(fresh.consumed(), {});
    geo::CriticalRefinementSession moved{std::move(*issued.refinement)};
    REQUIRE(moved.snapshot(handle));
    const auto again = moved.contract(handle, budget);
    CHECK(again.status == LC_IntersectionStatus::LimitExceeded);
    checkWork(again.entered, {});
    checkWork(again.work, result.work);
    CHECK(moved.occurrenceCount() == 4);
}

TEST_CASE("Critical cumulative publication counts remain exact after failed legacy preparation",
          "[lc-offsetcurve][geometry][reference-critical-refine]") {
    const auto data = basis(100.0, 0.5);
    geo::WorkBudget budget;
    const auto failed = geo::analyseCritical(basis(5.0, 1.0), -5.0, budget);
    REQUIRE(failed.status == LC_IntersectionStatus::AmbiguousTopology);
    const auto spent = budget.consumed();
    geo::WorkBudget referenceBudget;
    const auto reference = geo::analyseCriticalRetained(data, -30.0, referenceBudget);
    REQUIRE(reference.status == LC_IntersectionStatus::Ok);
    const auto issued = geo::analyseCriticalRetained(data, -30.0, budget);
    REQUIRE(issued.status == LC_IntersectionStatus::Ok);
    CHECK(issued.work.vertices == spent.vertices + reference.work.vertices);
    CHECK(issued.work.distancePairs == spent.distancePairs + reference.work.distancePairs);
    CHECK(issued.work.deepest == std::max(spent.deepest, reference.work.deepest));
}

TEST_CASE("Critical joins keep an adjacent side while certifying the whole one sided contraction",
          "[lc-offsetcurve][geometry][reference-critical][reference-critical-refine]") {
    for (const bool reversed : {false, true}) {
        const auto data = reversed ? basis(100.0, 0.5, 3.0, 0.0, true) : basis(100.0, 0.5, 0.0, 2.0);
        geo::WorkBudget budget;
        const auto legacy = geo::analyseCritical(data, -100.0, budget);
        CAPTURE(reversed, legacy.work.vertices, legacy.work.distancePairs, legacy.work.deepest);
        REQUIRE(legacy.status == LC_IntersectionStatus::Ok);
        REQUIRE(legacy.adapter);
        CHECK(legacy.adapter->criticalOccurrences().size() == (reversed ? 2 : 1));
        checkCriticalCoverage(*legacy.adapter);
        checkCriticalOccurrences(data, -100.0, *legacy.adapter);
        const auto& segments = legacy.adapter->segments();
        REQUIRE(segments.size() == 3);
        const std::size_t i = reversed ? 2 : 1;
        const double join = segments[i].t0;
        double from = segments[i - 1].t0 + 0.5 * (join - segments[i - 1].t0);
        double to = join + 0.5 * (segments[i].t1 - join);
        if (from == join) { from = segments[i - 1].t0; }
        if (to == join) { to = segments[i].t1; }
        const double left = from + 0.5 * (join - from);
        const double right = join + 0.5 * (to - join);
        const bool leftMoves = from < left && left < join;
        const bool rightMoves = join < right && right < to;
        CHECK(leftMoves == !reversed);
        CHECK(rightMoves == reversed);
        if (reversed) {
            CHECK(std::nextafter(from, std::numeric_limits<double>::infinity()) == join);
        } else {
            CHECK(std::nextafter(join, std::numeric_limits<double>::infinity()) == to);
        }
        const LC_IntersectionDomain original{i - 1, i, LC_Interval::hull(from, join), LC_Interval::hull(join, to)};
        CHECK(legacy.adapter->certifyIntersection(original).outcome ==
            LC_AnalyticIntersectionCertificate::Outcome::Unavailable);
        const double narrowedFrom = leftMoves ? left : from;
        const double narrowedTo = rightMoves ? right : to;
        REQUIRE(narrowedFrom < join);
        REQUIRE(join < narrowedTo);
        LC_Interval factor, derivative;
        REQUIRE(legacy.adapter->frame().boundOffsetFactor(narrowedFrom, narrowedTo, factor, derivative));
        CHECK(factor.isPositive()); // Whole-span enclosure, not a midpoint sign or an adjacency exemption.
        const LC_IntersectionDomain whole{i - 1, i, LC_Interval::hull(narrowedFrom, join),
                                         LC_Interval::hull(join, narrowedTo)};
        const auto proof = legacy.adapter->certifyIntersection(whole);
        REQUIRE(proof.outcome == LC_AnalyticIntersectionCertificate::Outcome::Excluded);
        CHECK(proof.proof == LC_IntersectionProof::AnalyticExclusion);
        CHECK(proof.proofDomain.segmentA == whole.segmentA);
        CHECK(proof.proofDomain.segmentB == whole.segmentB);
        CHECK(proof.proofDomain.parameterA.lo() == whole.parameterA.lo());
        CHECK(proof.proofDomain.parameterA.hi() == whole.parameterA.hi());
        CHECK(proof.proofDomain.parameterB.lo() == whole.parameterB.lo());
        CHECK(proof.proofDomain.parameterB.hi() == whole.parameterB.hi());
        const LC_IntersectionDomain swapped{i, i - 1, whole.parameterB, whole.parameterA};
        CHECK(legacy.adapter->certifyIntersection(swapped).outcome ==
            LC_AnalyticIntersectionCertificate::Outcome::Excluded);
        geo::WorkBudget retainedBudget;
        const auto retained = geo::analyseCriticalRetained(data, -100.0, retainedBudget);
        REQUIRE(retained.status == LC_IntersectionStatus::Ok);
        REQUIRE(retained.refinement);
        const std::size_t n = reversed ? 2 : 1;
        CHECK(retained.refinement->occurrenceCount() == n);
        CHECK(retained.work.vertices == legacy.work.vertices + 5 + n);
        CHECK(retained.work.distancePairs == legacy.work.distancePairs + 1 + n);
        CHECK(retained.work.deepest == legacy.work.deepest);
        for (std::size_t k = 0; k < n; ++k) {
            const auto snapshot = retained.refinement->snapshot(retained.refinement->occurrenceHandle(k));
            REQUIRE(snapshot);
            checkCriticalSnapshot(data, -100.0, retained.adapter->frame(), *snapshot);
        }
    }
}

TEST_CASE("Critical one sided join repair obeys exact and one less legacy retained work ceilings",
          "[lc-offsetcurve][geometry][reference-critical][reference-critical-refine][reference-critical-depth]") {
    for (const bool reversed : {false, true}) {
        const auto data = reversed ? basis(100.0, 0.5, 3.0, 0.0, true) : basis(100.0, 0.5, 0.0, 2.0);
        geo::WorkBudget legacyBudget;
        const auto legacy = geo::analyseCritical(data, -100.0, legacyBudget);
        REQUIRE(legacy.status == LC_IntersectionStatus::Ok);
        geo::WorkBudget retainedBudget;
        const auto retained = geo::analyseCriticalRetained(data, -100.0, retainedBudget);
        REQUIRE(retained.status == LC_IntersectionStatus::Ok);
        for (const bool retain : {false, true}) {
            const auto work = retain ? retained.work : legacy.work;
            CAPTURE(reversed, retain, work.vertices, work.distancePairs, work.deepest);
            REQUIRE(work.deepest > 0);
            for (int cap = 0; cap < 4; ++cap) {
                LC_OffsetCheckLimits limits{1, work.vertices, work.distancePairs, work.deepest};
                if (cap == 1) { --limits.maxVertices; }
                if (cap == 2) { --limits.maxDistancePairs; }
                if (cap == 3) { --limits.maxDepth; }
                geo::WorkBudget budget{limits};
                const auto expected = cap == 0 ? LC_IntersectionStatus::Ok : LC_IntersectionStatus::LimitExceeded;
                if (retain) {
                    const auto result = geo::analyseCriticalRetained(data, -100.0, budget);
                    CHECK(result.status == expected);
                    CHECK(bool(result.adapter) == (cap == 0));
                    CHECK(bool(result.refinement) == (cap == 0));
                    checkWork(result.work, budget.consumed());
                } else {
                    const auto result = geo::analyseCritical(data, -100.0, budget);
                    CHECK(result.status == expected);
                    CHECK(bool(result.adapter) == (cap == 0));
                    checkWork(result.work, budget.consumed());
                }
                CHECK(budget.consumed().vertices <= limits.maxVertices);
                CHECK(budget.consumed().distancePairs <= limits.maxDistancePairs);
                CHECK(budget.consumed().deepest <= limits.maxDepth);
                if (cap == 0) {
                    checkWork(budget.consumed(), work);
                } else {
                    const auto spent = budget.consumed();
                    const auto again = geo::analyseCritical(data, -100.0, budget);
                    CHECK(again.status == LC_IntersectionStatus::LimitExceeded);
                    CHECK_FALSE(again.adapter);
                    checkWork(again.work, spent);
                }
            }
        }
    }
}

namespace {
using CutterJet = std::array<CriticalFloat, 6>;

CutterJet cutterOracle(const RS_EllipseData& data, const double scale, const double delta,
                       const bool start, const double u) {
    const CriticalFloat angle = start ? data.angle1 : data.angle2;
    const CriticalFloat mx = CriticalFloat(data.majorP.x) / scale;
    const CriticalFloat my = CriticalFloat(data.majorP.y) / scale;
    const CriticalFloat r = data.ratio;
    const CriticalFloat c = cos(angle), s = sin(angle);
    const CriticalFloat ex = mx * c - r * my * s;
    const CriticalFloat ey = my * c + r * mx * s;
    const CriticalFloat vx = -mx * s - r * my * c;
    const CriticalFloat vy = -my * s + r * mx * c;
    const CriticalFloat speed = sqrt(vx * vx + vy * vy);
    const CriticalFloat nx = vy / speed, ny = -vx / speed;
    const CriticalFloat direction = data.reversed ? -1 : 1;
    const CriticalFloat outward = start ? -direction : direction;
    const CriticalFloat tx = outward * vx / speed, ty = outward * vy / speed;
    const CriticalFloat d = CriticalFloat(std::abs(delta)) / scale;
    const CriticalFloat pi = boost::math::constants::pi<CriticalFloat>();
    CriticalFloat ca, sa;
    if (u == 0.0) { ca = 1; sa = 0; }
    else if (u == 0.5) { ca = 0; sa = 1; }
    else if (u == 1.0) { ca = -1; sa = 0; }
    else if (u > 0.5) {
        const CriticalFloat remaining = pi * (CriticalFloat(1) - CriticalFloat(u));
        ca = -cos(remaining);
        sa = sin(remaining);
    } else {
        ca = cos(pi * CriticalFloat(u));
        sa = sin(pi * CriticalFloat(u));
    }
    const CriticalFloat rx = nx * ca + tx * sa, ry = ny * ca + ty * sa;
    return {{ex + d * rx, ey + d * ry,
             d * pi * (-nx * sa + tx * ca), d * pi * (-ny * sa + ty * ca),
             -d * pi * pi * rx, -d * pi * pi * ry}};
}

void checkCutterEnclosure(const LC_CurveJetBounds& bound, const CutterJet& oracle) {
    const std::array<LC_Interval, 6> intervals{{bound.x, bound.y, bound.dx, bound.dy, bound.ddx, bound.ddy}};
    for (std::size_t i = 0; i < intervals.size(); ++i) {
        CAPTURE(i);
        checkCriticalEnclosure(intervals[i], oracle[i]);
    }
    checkCriticalEnclosure(bound.speedSquaredProduct, oracle[2] * oracle[2] + oracle[3] * oracle[3]);
    checkCriticalEnclosure(bound.crossProduct, oracle[2] * oracle[5] - oracle[3] * oracle[4]);
}

std::vector<std::size_t> roleSegments(const geo::ReferenceProviders& provider, const geo::ReferenceRole role) {
    std::vector<std::size_t> out;
    const auto branch = provider.branchFor(role);
    if (!branch) { return out; }
    for (std::size_t i = 0; i < provider.segments().size(); ++i) {
        if (provider.segments()[i].branch == *branch) { out.push_back(i); }
    }
    return out;
}

void checkNoProviders(const geo::ReferenceProviderPreparation& result, const geo::WorkBudget& budget) {
    CHECK_FALSE(result.provider);
    CHECK_FALSE(result.ownCritical);
    CHECK_FALSE(result.oppositeCritical);
    checkWork(result.work, budget.consumed());
}

void checkUnavailableProviderProof(const LC_AnalyticIntersectionCertificate& certificate) {
    CHECK(certificate.outcome == LC_AnalyticIntersectionCertificate::Outcome::Unavailable);
    CHECK(certificate.proof == LC_IntersectionProof::Uncertified);
    CHECK_FALSE(certificate.proofDomain.parameterA.isValid());
    CHECK_FALSE(certificate.proofDomain.parameterB.isValid());
    CHECK_FALSE(certificate.isolationA.isValid());
    CHECK_FALSE(certificate.isolationB.isValid());
}

void checkProviderLayout(const geo::ReferenceProviders& provider, const bool whole, const bool omitted) {
    REQUIRE(provider.branchFor(geo::ReferenceRole::Own));
    CHECK(*provider.branchFor(geo::ReferenceRole::Own) == 0);
    CHECK(bool(provider.branchFor(geo::ReferenceRole::Opposite)) == !omitted);
    CHECK(provider.oppositeDisposition() == (omitted ? geo::OppositeDisposition::ExcludedCollapsedCircle :
        geo::OppositeDisposition::Present));
    const std::size_t offsetCount = omitted ? 1 : 2;
    CHECK(provider.branchCount() == offsetCount + (whole ? 0 : 2));
    for (std::size_t branch = 0; branch < provider.branchCount(); ++branch) {
        std::size_t count = 0;
        double previous = 0.0;
        for (const auto& segment : provider.segments()) {
            if (segment.branch != branch) { continue; }
            CHECK(segment.t0 == previous);
            CHECK(segment.t0 < segment.t1);
            previous = segment.t1;
            ++count;
        }
        REQUIRE(count > 0);
        CHECK(previous == (branch < offsetCount ? provider.frame().parameterEnd() : 1.0));
        CHECK(provider.next(branch) == (whole ? static_cast<std::ptrdiff_t>(branch) : -1));
    }
    CHECK(provider.next(provider.branchCount()) == -1);
    CHECK(provider.next(std::numeric_limits<std::size_t>::max()) == -1);
    for (const auto role : {geo::ReferenceRole::StartCutter, geo::ReferenceRole::EndCutter}) {
        CHECK(bool(provider.branchFor(role)) == !whole);
        if (!whole) {
            const auto indices = roleSegments(provider, role);
            REQUIRE(indices.size() == 2);
            CHECK(provider.segments()[indices[0]].t0 == 0.0);
            CHECK(provider.segments()[indices[0]].t1 == 0.5);
            CHECK(provider.segments()[indices[1]].t0 == 0.5);
            CHECK(provider.segments()[indices[1]].t1 == 1.0);
        }
    }
    if (!whole) {
        CHECK(*provider.branchFor(geo::ReferenceRole::StartCutter) == offsetCount);
        CHECK(*provider.branchFor(geo::ReferenceRole::EndCutter) == offsetCount + 1);
    }
    CHECK_FALSE(provider.branchFor(static_cast<geo::ReferenceRole>(999)));
}
} // namespace

TEST_CASE("Reference providers own contiguous role layouts without exposing construction credentials",
          "[lc-offsetcurve][geometry][reference-providers]") {
    static_assert(!std::is_default_constructible_v<geo::ReferenceProviders>);
    static_assert(!std::is_copy_constructible_v<geo::ReferenceProviders>);
    static_assert(!std::is_move_constructible_v<geo::ReferenceProviders>);
    static_assert(!std::is_constructible_v<geo::ReferenceProviders, geo::CriticalOccurrenceSnapshot>);
    static_assert(std::is_convertible_v<std::unique_ptr<const geo::ReferenceProviders>,
                                        std::unique_ptr<const LC_ParametricCurves>>);
    for (const bool reversed : {false, true}) {
        for (const bool whole : {false, true}) {
            for (const double delta : {-1.0, 1.0}) {
                const auto data = whole ? basis(10.0, 0.5, 0.0, 0.0, reversed) :
                    reversed ? basis(10.0, 0.5, 2.0, 0.2, true) : basis(10.0, 0.5, 0.2, 2.0);
                CAPTURE(reversed, whole, delta);
                geo::WorkBudget budget;
                const auto result = geo::prepareReferenceProviders(data, delta, budget);
                REQUIRE(result.status == LC_IntersectionStatus::Ok);
                REQUIRE(result.provider);
                REQUIRE(result.ownCritical);
                REQUIRE(result.oppositeCritical);
                CHECK(result.reason == geo::ReferencePreparationReason::None);
                checkProviderLayout(*result.provider, whole, false);
                checkWork(result.work, budget.consumed());
            }
        }
    }
}

TEST_CASE("Reference cutter jets enclose independent exact geometry over points and partial ranges",
          "[lc-offsetcurve][geometry][reference-providers]") {
    for (const double magnitude : {1e-280, 10.0, 1e280}) {
        for (const bool reversed : {false, true}) {
            for (const bool wrapped : {false, true}) {
                auto data = wrapped ? (reversed ? basis(magnitude, 0.5, 0.2, 5.8, true) :
                                                    basis(magnitude, 0.5, 5.8, 0.2)) :
                    reversed ? basis(magnitude, 0.5, 2.0, 0.2, true) : basis(magnitude, 0.5, 0.2, 2.0);
                data.majorP = {0.6 * magnitude, 0.8 * magnitude};
                data.center = {1e300, -1e300};
                const double delta = 0.1 * magnitude;
                CAPTURE(magnitude, reversed, wrapped);
                geo::WorkBudget budget;
                const auto prepared = geo::prepareReferenceProviders(data, delta, budget);
                REQUIRE(prepared.status == LC_IntersectionStatus::Ok);
                const auto& provider = *prepared.provider;
                for (const bool start : {false, true}) {
                    const auto role = start ? geo::ReferenceRole::StartCutter : geo::ReferenceRole::EndCutter;
                    const auto indices = roleSegments(provider, role);
                    REQUIRE(indices.size() == 2);
                    for (const std::size_t index : indices) {
                        const auto segment = provider.segments()[index];
                        for (unsigned i = 0; i <= 32; ++i) {
                            const double u = segment.t0 + (segment.t1 - segment.t0) * (double(i) / 32.0);
                            LC_CurveJetBounds point;
                            REQUIRE(provider.boundJet(index, u, u, point));
                            checkCutterEnclosure(point, cutterOracle(data, provider.frame().scale(), delta, start, u));
                            RS_Vector seed, derivative;
                            REQUIRE(provider.evaluate(index, u, seed, derivative));
                            REQUIRE(geo::isFinite2D(seed));
                            REQUIRE(geo::isFinite2D(derivative));
                            CHECK(point.x.contains(seed.x));
                            CHECK(point.y.contains(seed.y));
                            CHECK(point.dx.contains(derivative.x));
                            CHECK(point.dy.contains(derivative.y));
                            if (i == 32) { continue; }
                            const double end = segment.t0 + (segment.t1 - segment.t0) * (double(i + 1) / 32.0);
                            LC_CurveJetBounds range;
                            REQUIRE(provider.boundJet(index, u, end, range));
                            for (const double t : {u, 0.5 * u + 0.5 * end, end}) {
                                checkCutterEnclosure(range, cutterOracle(data, provider.frame().scale(), delta, start, t));
                            }
                        }
                        LC_CurveJetBounds wholePiece;
                        REQUIRE(provider.boundJet(index, segment.t0, segment.t1, wholePiece));
                        for (unsigned i = 0; i <= 16; ++i) {
                            const double u = segment.t0 + (segment.t1 - segment.t0) * (double(i) / 16.0);
                            checkCutterEnclosure(wholePiece, cutterOracle(data, provider.frame().scale(), delta, start, u));
                        }
                    }
                }
            }
        }
    }
}

TEST_CASE("Reference cutter true pi endpoints and half join have continuous two sided jet limits",
          "[lc-offsetcurve][geometry][reference-providers]") {
    for (const bool reversed : {false, true}) {
        const auto data = reversed ? basis(10.0, 0.5, 2.0, 0.0, true) : basis(10.0, 0.5, 0.0, 2.0);
        geo::WorkBudget budget;
        const auto result = geo::prepareReferenceProviders(data, -1.0, budget);
        REQUIRE(result.status == LC_IntersectionStatus::Ok);
        const auto& provider = *result.provider;
        for (const bool start : {false, true}) {
            const auto indices = roleSegments(provider, start ? geo::ReferenceRole::StartCutter :
                                                               geo::ReferenceRole::EndCutter);
            REQUIRE(indices.size() == 2);
            for (const double u : {0.0, 0.5, 1.0}) {
                const std::size_t index = indices[u <= 0.5 ? 0 : 1];
                LC_CurveJetBounds point, neighborhood;
                REQUIRE(provider.boundJet(index, u, u, point));
                const double lo = u == 0.0 ? u : std::nextafter(u, 0.0);
                const double hi = u == 0.5 ? u : u == 1.0 ? u : std::nextafter(u, 1.0);
                REQUIRE(provider.boundJet(index, lo, hi, neighborhood));
                const auto oracle = cutterOracle(data, provider.frame().scale(), -1.0, start, u);
                checkCutterEnclosure(point, oracle);
                checkCutterEnclosure(neighborhood, oracle);
            }
            LC_CurveJetBounds left, right;
            REQUIRE(provider.boundJet(indices[0], std::nextafter(0.5, 0.0), 0.5, left));
            REQUIRE(provider.boundJet(indices[1], 0.5, std::nextafter(0.5, 1.0), right));
            const auto join = cutterOracle(data, provider.frame().scale(), -1.0, start, 0.5);
            checkCutterEnclosure(left, join);
            checkCutterEnclosure(right, join);
            const double endpointT = start ? 0.0 : provider.frame().parameterEnd();
            for (const double u : {0.0, 1.0}) {
                const double physical = u == 0.0 ? 1.0 : -1.0;
                const auto endpoint = referenceOracle(data, provider.frame(), physical, endpointT, true);
                const auto cap = cutterOracle(data, provider.frame().scale(), -1.0, start, u);
                CHECK(abs(cap[0] - CriticalFloat(endpoint[0])) < CriticalFloat("1e-30"));
                CHECK(abs(cap[1] - CriticalFloat(endpoint[1])) < CriticalFloat("1e-30"));
            }
        }
    }
}

TEST_CASE("Reference provider strict hooks remap both directions without leaking local segment indices",
          "[lc-offsetcurve][geometry][reference-providers]") {
    using Outcome = LC_AnalyticIntersectionCertificate::Outcome;
    const auto data = basis(100.0, 0.5, 0.0, 2.0);
    geo::WorkBudget budget;
    const auto prepared = geo::prepareReferenceProviders(data, 30.0, budget);
    REQUIRE(prepared.status == LC_IntersectionStatus::Ok);
    const auto& provider = *prepared.provider;
    for (const bool own : {false, true}) {
        geo::WorkBudget separate;
        const auto child = geo::analyseCritical(data, own ? 30.0 : -30.0, separate);
        REQUIRE(child.status == LC_IntersectionStatus::Ok);
        const auto indices = roleSegments(provider, own ? geo::ReferenceRole::Own : geo::ReferenceRole::Opposite);
        REQUIRE(indices.size() == child.adapter->segments().size());
        std::size_t certified = 0;
        for (std::size_t i = 0; i < indices.size(); ++i) {
            const auto piece = child.adapter->segments()[i];
            const LC_Interval interval = LC_Interval::hull(piece.t0, piece.t1);
            const auto localLeaf = child.adapter->certifyInjective(i, interval);
            const auto globalLeaf = provider.certifyInjective(indices[i], interval);
            REQUIRE(localLeaf.available);
            REQUIRE(globalLeaf.available);
            CHECK(globalLeaf.segment == indices[i]);
            CHECK(globalLeaf.parameter.lo() == localLeaf.parameter.lo());
            CHECK(globalLeaf.parameter.hi() == localLeaf.parameter.hi());
            LC_CurveJetBounds bounds;
            const double middle = 0.5 * piece.t0 + 0.5 * piece.t1;
            REQUIRE(provider.boundJet(indices[i], middle, middle, bounds));
            checkEnclosure(bounds, referenceOracle(data, provider.frame(), own ? 30.0 : -30.0, middle, true));
            if (i + 1 == indices.size()) { continue; }
            const double end = piece.t1;
            LC_IntersectionDomain local{i, i + 1,
                LC_Interval::hull(std::nextafter(end, piece.t0), end),
                LC_Interval::hull(end, std::nextafter(end, child.adapter->segments()[i + 1].t1))};
            for (const bool swap : {false, true}) {
                if (swap) {
                    std::swap(local.segmentA, local.segmentB);
                    std::swap(local.parameterA, local.parameterB);
                }
                const auto expected = child.adapter->certifyIntersection(local);
                const LC_IntersectionDomain global{indices[local.segmentA], indices[local.segmentB],
                    local.parameterA, local.parameterB};
                const auto actual = provider.certifyIntersection(global);
                CHECK(actual.outcome == expected.outcome);
                if (expected.outcome == Outcome::Unavailable) { continue; }
                ++certified;
                CHECK(actual.proof == expected.proof);
                CHECK(actual.proofDomain.segmentA == global.segmentA);
                CHECK(actual.proofDomain.segmentB == global.segmentB);
                CHECK(actual.proofDomain.parameterA.lo() == expected.proofDomain.parameterA.lo());
                CHECK(actual.proofDomain.parameterA.hi() == expected.proofDomain.parameterA.hi());
                CHECK(actual.proofDomain.parameterB.lo() == expected.proofDomain.parameterB.lo());
                CHECK(actual.proofDomain.parameterB.hi() == expected.proofDomain.parameterB.hi());
            }
        }
        CHECK(certified > 0);
    }
    for (const auto role : {geo::ReferenceRole::StartCutter, geo::ReferenceRole::EndCutter}) {
        const auto indices = roleSegments(provider, role);
        REQUIRE(indices.size() == 2);
        const auto first = provider.certifyInjective(indices[0], LC_Interval::hull(0.0, 0.5));
        const auto second = provider.certifyInjective(indices[1], LC_Interval::hull(0.5, 1.0));
        CHECK(first.available);
        CHECK(second.available);
        for (const bool swap : {false, true}) {
            const LC_IntersectionDomain joint{indices[swap ? 1 : 0], indices[swap ? 0 : 1],
                LC_Interval::hull(swap ? 0.5 : 0.0, swap ? 1.0 : 0.5),
                LC_Interval::hull(swap ? 0.0 : 0.5, swap ? 0.5 : 1.0)};
            const auto proof = provider.certifyIntersection(joint);
            REQUIRE(proof.outcome == Outcome::Excluded);
            CHECK(proof.proof == LC_IntersectionProof::AnalyticExclusion);
            CHECK(proof.proofDomain.segmentA == joint.segmentA);
            CHECK(proof.proofDomain.segmentB == joint.segmentB);
        }
        const LC_IntersectionDomain diagonal{indices[0], indices[0], LC_Interval::hull(0.1, 0.3),
                                             LC_Interval::hull(0.2, 0.4)};
        CHECK(provider.certifyIntersection(diagonal).outcome == Outcome::Unavailable);
    }
}

TEST_CASE("Reference support exclusions cover only factory qualified opposite roles",
          "[lc-offsetcurve][geometry][reference-providers]") {
    using Outcome = LC_AnalyticIntersectionCertificate::Outcome;
    for (const bool reversed : {false, true}) {
        for (const double delta : {-1.0, 1.0}) {
            const auto data = reversed ? basis(10.0, 0.5, 0.2, 5.8, true) : basis(10.0, 0.5, 5.8, 0.2);
            geo::WorkBudget budget;
            const auto result = geo::prepareReferenceProviders(data, delta, budget);
            REQUIRE(result.status == LC_IntersectionStatus::Ok);
            const auto& provider = *result.provider;
            const auto own = roleSegments(provider, geo::ReferenceRole::Own);
            const auto opposite = roleSegments(provider, geo::ReferenceRole::Opposite);
            for (const auto i : own) {
                for (const auto j : opposite) {
                    for (const bool swap : {false, true}) {
                        const auto a = swap ? j : i, b = swap ? i : j;
                        const auto sa = provider.segments()[a], sb = provider.segments()[b];
                        const LC_IntersectionDomain domain{a, b, LC_Interval::hull(sa.t0, sa.t1),
                                                               LC_Interval::hull(sb.t0, sb.t1)};
                        const auto certificate = provider.certifyIntersection(domain);
                        REQUIRE(certificate.outcome == Outcome::Excluded);
                        CHECK(certificate.proof == LC_IntersectionProof::AnalyticExclusion);
                        CHECK(certificate.proofDomain.segmentA == a);
                        CHECK(certificate.proofDomain.segmentB == b);
                        CHECK(certificate.proofDomain.parameterA.lo() == sa.t0);
                        CHECK(certificate.proofDomain.parameterA.hi() == sa.t1);
                        CHECK(certificate.proofDomain.parameterB.lo() == sb.t0);
                        CHECK(certificate.proofDomain.parameterB.hi() == sb.t1);
                    }
                }
            }
            const auto cap = roleSegments(provider, geo::ReferenceRole::StartCutter);
            REQUIRE_FALSE(cap.empty());
            for (const auto offset : {own.front(), opposite.front()}) {
                const auto piece = provider.segments()[offset];
                const LC_IntersectionDomain box{offset, cap[0], LC_Interval::hull(piece.t0, piece.t1),
                                                LC_Interval::hull(0.0, 0.5)};
                CHECK(provider.certifyIntersection(box).outcome == Outcome::Unavailable);
            }
            const auto otherCap = roleSegments(provider, geo::ReferenceRole::EndCutter);
            CHECK(provider.certifyIntersection({cap[0], otherCap[0], LC_Interval::hull(0.0, 0.5),
                LC_Interval::hull(0.0, 0.5)}).outcome == Outcome::Unavailable);
        }
    }
}

TEST_CASE("Reference providers clear every output on invalid indices and parameter domains",
          "[lc-offsetcurve][geometry][reference-providers]") {
    const auto data = basis(10.0, 0.5, 0.0, 2.0);
    geo::WorkBudget budget;
    const auto result = geo::prepareReferenceProviders(data, 1.0, budget);
    REQUIRE(result.status == LC_IntersectionStatus::Ok);
    const auto& provider = *result.provider;
    const auto saved = budget.consumed();
    const auto cap = roleSegments(provider, geo::ReferenceRole::StartCutter);
    const double nan = std::numeric_limits<double>::quiet_NaN();
    const double inf = std::numeric_limits<double>::infinity();
    struct Request { std::size_t index; double a, b; };
    const std::array<Request, 8> invalid{{
        {provider.segments().size(), 0.0, 0.0}, {std::size_t(-1), 0.0, 0.0},
        {cap[0], nan, 0.1}, {cap[0], 0.0, inf}, {cap[0], -0.1, 0.1},
        {cap[0], 0.4, 0.6}, {cap[1], 0.4, 0.6}, {cap[0], 0.3, 0.1}}};
    for (const auto request : invalid) {
        CAPTURE(request.index, request.a, request.b);
        LC_CurveJetBounds bounds;
        REQUIRE(provider.boundJet(cap[0], 0.2, 0.3, bounds));
        CHECK_FALSE(provider.boundJet(request.index, request.a, request.b, bounds));
        checkCleared(bounds);
        LC_Interval x = LC_Interval::point(1.0), y = x, dx = x, dy = x;
        CHECK_FALSE(provider.bound(request.index, request.a, request.b, x, y, dx, dy));
        CHECK_FALSE(x.isValid());
        CHECK_FALSE(y.isValid());
        CHECK_FALSE(dx.isValid());
        CHECK_FALSE(dy.isValid());
    }
    for (const auto request : {Request{provider.segments().size(), 0.0, 0.0}, Request{cap[0], nan, nan},
                               Request{cap[0], -0.1, -0.1}, Request{cap[0], 0.6, 0.6}}) {
        RS_Vector point{1.0, 2.0}, derivative{3.0, 4.0};
        CHECK_FALSE(provider.evaluate(request.index, request.a, point, derivative));
        CHECK_FALSE(point.valid);
        CHECK_FALSE(derivative.valid);
    }
    for (const auto interval : {LC_Interval{}, LC_Interval::hull(-0.1, 0.1), LC_Interval::hull(0.4, 0.6),
                                LC_Interval::hull(0.0, inf)}) {
        const auto leaf = provider.certifyInjective(cap[0], interval);
        CHECK_FALSE(leaf.available);
        CHECK_FALSE(leaf.parameter.isValid());
        checkUnavailableProviderProof(provider.certifyIntersection({cap[0], cap[1], interval,
                                                                   LC_Interval::hull(0.5, 0.6)}));
    }
    CHECK_FALSE(provider.certifyInjective(std::size_t(-1), LC_Interval::point(0.0)).available);
    checkUnavailableProviderProof(provider.certifyIntersection({std::size_t(-1), cap[0], LC_Interval::point(0.0),
                                                                LC_Interval::point(0.0)}));
    checkWork(budget.consumed(), saved); // Prepared hooks/queries never spend a second hidden budget.
}

TEST_CASE("Reference providers omit only the exact trusted collapsed opposite circle",
          "[lc-offsetcurve][geometry][reference-providers]") {
    for (const bool reversed : {false, true}) {
        for (const bool whole : {false, true}) {
            auto data = whole ? basis(5.0, 1.0, 0.0, 0.0, reversed) :
                reversed ? basis(5.0, 1.0, 2.0, 0.2, true) : basis(5.0, 1.0, 0.2, 2.0);
            data.majorP = {3.0, 4.0};
            geo::WorkBudget budget;
            const auto result = geo::prepareReferenceProviders(data, 5.0, budget);
            REQUIRE(result.status == LC_IntersectionStatus::Ok);
            REQUIRE(result.provider);
            REQUIRE(result.ownCritical);
            CHECK_FALSE(result.oppositeCritical);
            checkProviderLayout(*result.provider, whole, true);
            geo::WorkBudget collapseBudget;
            const auto ownCollapse = geo::prepareReferenceProviders(data, -5.0, collapseBudget);
            CHECK(ownCollapse.status == LC_IntersectionStatus::AmbiguousTopology);
            CHECK(ownCollapse.reason == geo::ReferencePreparationReason::CollapsedCircle);
            checkNoProviders(ownCollapse, collapseBudget);
        }
    }
    for (const double delta : {std::nextafter(5.0, 0.0), std::nextafter(5.0, 10.0)}) {
        auto data = basis(5.0, 1.0);
        data.majorP = {3.0, 4.0};
        geo::WorkBudget budget;
        const auto result = geo::prepareReferenceProviders(data, delta, budget);
        REQUIRE(result.status == LC_IntersectionStatus::Ok);
        REQUIRE(result.oppositeCritical);
        checkProviderLayout(*result.provider, true, false);
    }
    geo::WorkBudget nonunitBudget;
    const auto nonunit = geo::prepareReferenceProviders(basis(5.0, 0.5), 5.0, nonunitBudget);
    REQUIRE(nonunit.status == LC_IntersectionStatus::Ok);
    REQUIRE(nonunit.oppositeCritical);
    CHECK(nonunit.provider->oppositeDisposition() == geo::OppositeDisposition::Present);
    CHECK(nonunit.provider->branchFor(geo::ReferenceRole::Opposite));
}

TEST_CASE("Reference provider failures never masquerade as qualified collapsed omission",
          "[lc-offsetcurve][geometry][reference-providers]") {
    auto data = basis(5.0, 1.0);
    data.majorP = {3.0, 4.0};
    geo::WorkBudget measure;
    const auto own = geo::analyseCriticalRetained(data, 5.0, measure);
    REQUIRE(own.status == LC_IntersectionStatus::Ok);
    LC_OffsetCheckLimits limits;
    limits.maxVertices = own.work.vertices;
    limits.maxDistancePairs = own.work.distancePairs;
    geo::WorkBudget exhausted{limits};
    const auto failed = geo::prepareReferenceProviders(data, 5.0, exhausted);
    CHECK(failed.status == LC_IntersectionStatus::LimitExceeded);
    CHECK(failed.reason == geo::ReferencePreparationReason::None);
    checkNoProviders(failed, exhausted);
    checkWork(failed.work, own.work);
    geo::WorkBudget invalidBudget;
    for (const double delta : {0.0, -0.0, std::numeric_limits<double>::infinity(),
                               std::numeric_limits<double>::quiet_NaN()}) {
        const auto invalid = geo::prepareReferenceProviders(data, delta, invalidBudget);
        CHECK(invalid.status == LC_IntersectionStatus::InvalidInput);
        checkNoProviders(invalid, invalidBudget);
        checkWork(invalid.work, {});
    }
    auto invalid = data;
    invalid.ratio = 2.0;
    const auto invalidBasis = geo::prepareReferenceProviders(invalid, 5.0, invalidBudget);
    CHECK(invalidBasis.status == LC_IntersectionStatus::InvalidInput);
    checkNoProviders(invalidBasis, invalidBudget);
    CHECK(invalidBasis.reason == geo::ReferencePreparationReason::None);
    geo::WorkBudget invalidLimits{LC_OffsetCheckLimits{1, 1, 1, 0}};
    const auto invalidAllowance = geo::prepareReferenceProviders(data, 5.0, invalidLimits);
    CHECK(invalidAllowance.status == LC_IntersectionStatus::InvalidInput);
    checkNoProviders(invalidAllowance, invalidLimits);
}

TEST_CASE("Reference provider metadata obeys exact one less and pre spent shared ceilings",
          "[lc-offsetcurve][geometry][reference-providers]") {
    for (const bool whole : {false, true}) {
        for (const bool collapsed : {false, true}) {
            const auto data = collapsed ? (whole ? basis(5.0, 1.0) : basis(5.0, 1.0, 0.0, 2.0)) :
                whole ? basis(10.0, 0.5) : basis(10.0, 0.5, 0.0, 2.0);
            const double distance = collapsed ? 5.0 : 1.0;
            geo::WorkBudget analyses;
            const auto own = geo::analyseCriticalRetained(data, distance, analyses);
            REQUIRE(own.status == LC_IntersectionStatus::Ok);
            const auto opposite = geo::analyseCriticalRetained(data, -distance, analyses);
            REQUIRE(opposite.status == (collapsed ? LC_IntersectionStatus::AmbiguousTopology : LC_IntersectionStatus::Ok));
            const auto before = analyses.consumed();
            geo::WorkBudget measured;
            const auto baseline = geo::prepareReferenceProviders(data, distance, measured);
            REQUIRE(baseline.status == LC_IntersectionStatus::Ok);
            const std::size_t c = whole ? 0 : 2;
            const std::size_t s = baseline.provider->segments().size();
            CHECK(baseline.work.vertices == before.vertices + 3 + c + s);
            CHECK(baseline.work.distancePairs == before.distancePairs + 1 + 4 * c + s);
            CHECK(baseline.work.deepest == before.deepest);
            CHECK(baseline.work.cacheSpans == before.cacheSpans);
            for (const bool prespent : {false, true}) {
                const std::size_t spentV = prespent ? 5 : 0, spentP = prespent ? 7 : 0;
                for (int cap = 0; cap < 3; ++cap) {
                    CAPTURE(whole, collapsed, prespent, cap, s);
                    LC_OffsetCheckLimits limits;
                    limits.maxCacheSpans = 2;
                    limits.maxVertices = baseline.work.vertices + spentV - (cap == 1 ? 1 : 0);
                    limits.maxDistancePairs = baseline.work.distancePairs + spentP - (cap == 2 ? 1 : 0);
                    geo::WorkBudget budget{limits};
                    if (prespent) {
                        REQUIRE(budget.consumePieces(1));
                        REQUIRE(budget.consumeVertices(spentV));
                        REQUIRE(budget.consumePairs(spentP));
                        REQUIRE(budget.checkDepth(15));
                    }
                    const auto result = geo::prepareReferenceProviders(data, distance, budget);
                    CHECK(result.status == (cap == 0 ? LC_IntersectionStatus::Ok : LC_IntersectionStatus::LimitExceeded));
                    CHECK(result.work.vertices <= limits.maxVertices);
                    CHECK(result.work.distancePairs <= limits.maxDistancePairs);
                    CHECK(result.work.deepest == (prespent ? 15 : before.deepest));
                    checkWork(result.work, budget.consumed());
                    if (cap == 0) {
                        REQUIRE(result.provider);
                        CHECK(result.work.vertices == baseline.work.vertices + spentV);
                        CHECK(result.work.distancePairs == baseline.work.distancePairs + spentP);
                        CHECK(result.work.cacheSpans == std::size_t(prespent));
                    } else {
                        checkNoProviders(result, budget);
                        const auto saved = budget.consumed();
                        const auto again = geo::prepareReferenceProviders(data, distance, budget);
                        CHECK(again.status == LC_IntersectionStatus::LimitExceeded);
                        checkNoProviders(again, budget);
                        checkWork(again.work, saved);
                    }
                }
            }
        }
    }
}

TEST_CASE("Reference provider failed entered stages retain only actual admitted pair work",
          "[lc-offsetcurve][geometry][reference-providers][budget-core]") {
    const auto data = basis(10.0, 0.5, 0.0, 2.0);
    geo::WorkBudget measure;
    REQUIRE(geo::analyseCriticalRetained(data, 1.0, measure).status == LC_IntersectionStatus::Ok);
    REQUIRE(geo::analyseCriticalRetained(data, -1.0, measure).status == LC_IntersectionStatus::Ok);
    const auto before = measure.consumed();
    geo::WorkBudget completeBudget;
    const auto complete = geo::prepareReferenceProviders(data, 1.0, completeBudget);
    REQUIRE(complete.status == LC_IntersectionStatus::Ok);
    const auto s = complete.provider->segments().size();
    // P1 common proof; two groups of normalization + three fixed cap checks; then S map validations.
    for (std::size_t allowed = 0; allowed < 9 + s; ++allowed) {
        CAPTURE(allowed, s);
        LC_OffsetCheckLimits limits;
        limits.maxDistancePairs = before.distancePairs + allowed;
        geo::WorkBudget budget{limits};
        const auto result = geo::prepareReferenceProviders(data, 1.0, budget);
        CHECK(result.status == LC_IntersectionStatus::LimitExceeded);
        checkNoProviders(result, budget);
        auto expected = before;
        auto remaining = allowed;
        const auto admit = [&](const std::size_t vertices, const std::size_t pairs) {
            if (pairs > remaining) { return false; }
            remaining -= pairs;
            expected.vertices += vertices;
            expected.distancePairs += pairs;
            return true;
        };
        bool completePrefix = admit(3, 1);
        for (std::size_t cap = 0; completePrefix && cap < 2; ++cap) {
            completePrefix = admit(1, 1);
            for (std::size_t check = 0; completePrefix && check < 3; ++check) {
                if (remaining == 0) { completePrefix = false; break; }
                --remaining;
                ++expected.distancePairs;
            }
        }
        if (completePrefix) {
            expected.vertices += s;
            for (std::size_t segment = 0; segment < s; ++segment) {
                if (remaining == 0) { break; }
                --remaining;
                ++expected.distancePairs;
            }
        }
        checkWork(result.work, expected);
    }
    for (const std::size_t allowed : {std::size_t{2}, std::size_t{3}, std::size_t{4}, std::size_t{4} + s}) {
        LC_OffsetCheckLimits limits;
        limits.maxVertices = before.vertices + allowed;
        geo::WorkBudget budget{limits};
        const auto result = geo::prepareReferenceProviders(data, 1.0, budget);
        CHECK(result.status == LC_IntersectionStatus::LimitExceeded);
        checkNoProviders(result, budget);
        const std::size_t admittedV = allowed == 2 ? 0 : allowed == 3 ? 3 : allowed == 4 ? 4 : 5;
        const std::size_t admittedP = allowed == 2 ? 0 : allowed == 3 ? 1 : allowed == 4 ? 5 : 9;
        CHECK(result.work.vertices == before.vertices + admittedV);
        CHECK(result.work.distancePairs == before.distancePairs + admittedP);
        CHECK(result.work.deepest == before.deepest);
    }
}

TEST_CASE("Reference collapsed omission verification is admitted after trusted critical analysis",
          "[lc-offsetcurve][geometry][reference-providers][budget-core]") {
    auto data = basis(5.0, 1.0);
    data.majorP = {3.0, 4.0};
    geo::WorkBudget base;
    REQUIRE(geo::analyseCriticalRetained(data, 5.0, base).status == LC_IntersectionStatus::Ok);
    const auto collapsed = geo::analyseCriticalRetained(data, -5.0, base);
    REQUIRE(collapsed.status == LC_IntersectionStatus::AmbiguousTopology);
    REQUIRE(collapsed.reason == geo::CriticalAnalysis::Reason::CollapsedCircle);
    LC_OffsetCheckLimits limits;
    limits.maxDistancePairs = base.consumed().distancePairs;
    geo::WorkBudget budget{limits};
    const auto result = geo::prepareReferenceProviders(data, 5.0, budget);
    CHECK(result.status == LC_IntersectionStatus::LimitExceeded);
    CHECK(result.reason == geo::ReferencePreparationReason::None);
    checkNoProviders(result, budget);
    checkWork(result.work, base.consumed());
    CHECK(result.work.distancePairs == base.consumed().distancePairs);
}

TEST_CASE("Reference provider ownership can enter core while critical refiners remain independently alive",
          "[lc-offsetcurve][geometry][reference-providers]") {
    const auto data = basis(100.0, 0.5);
    geo::WorkBudget budget;
    auto prepared = geo::prepareReferenceProviders(data, 30.0, budget);
    REQUIRE(prepared.status == LC_IntersectionStatus::Ok);
    REQUIRE(prepared.ownCritical);
    REQUIRE(prepared.oppositeCritical);
    REQUIRE(prepared.oppositeCritical->occurrenceCount() == 4);
    const auto handle = prepared.oppositeCritical->occurrenceHandle(0);
    const auto original = prepared.oppositeCritical->snapshot(handle);
    REQUIRE(original);
    const auto frame = prepared.provider->frame();
    const auto* view = prepared.provider.get();
    LC_IntersectionOptions options;
    options.tolerance = 1e-8;
    options.maxDepth = budget.depthLimit();
    options.maxLeafDepth = budget.depthLimit();
    options.maxBoxPairs = budget.remainingPairs();
    options.maxGeneratedItems = budget.remainingVertices();
    options.maxLeaves = budget.remainingVertices();
    options.anchoredBranches = 1;
    auto core = findCertifiedIntersections(std::move(prepared.provider), options);
    REQUIRE(budget.consumePairs(core.work.totalBoxPairs));
    REQUIRE(budget.consumeVertices(core.work.generatedItems));
    REQUIRE(budget.checkDepth(core.work.deepest()));
    REQUIRE(core.status == LC_IntersectionStatus::Ok);
    REQUIRE(core.session);
    CHECK(core.session->rootCount() == 0);
    LC_CurveJetBounds bounds;
    REQUIRE(view->boundJet(0, view->segments()[0].t0, view->segments()[0].t0, bounds));
    checkCriticalSnapshot(data, -30.0, frame, *original);
    core.session.reset(); // No borrowed composite/adapters survive in the critical session.
    const auto result = prepared.oppositeCritical->contract(handle, budget);
    REQUIRE(result.status == LC_IntersectionStatus::Ok);
    REQUIRE(result.snapshot);
    CHECK(result.workSource == geo::CriticalWorkSource::CurrentBudget);
    checkCriticalSnapshot(data, -30.0, frame, *result.snapshot);
    CHECK(prepared.ownCritical->occurrenceCount() == 0);
    checkWork(result.work, budget.consumed());
}

TEST_CASE("Reference providers do not borrow input fields or require their issuing budget to stay alive",
          "[lc-offsetcurve][geometry][reference-providers]") {
    std::unique_ptr<const geo::ReferenceProviders> provider;
    std::unique_ptr<geo::CriticalRefinementSession> retained;
    geo::CriticalOccurrenceHandle handle;
    {
        auto data = basis(100.0, 0.5, 0.0, 2.0);
        geo::WorkBudget budget;
        auto result = geo::prepareReferenceProviders(data, 30.0, budget);
        REQUIRE(result.status == LC_IntersectionStatus::Ok);
        REQUIRE(result.oppositeCritical->occurrenceCount() == 1);
        handle = result.oppositeCritical->occurrenceHandle(0);
        provider = std::move(result.provider);
        retained = std::move(result.oppositeCritical);
        data.majorP = {0.0, 0.0};
        data.ratio = -1.0;
        data.angle1 = std::numeric_limits<double>::quiet_NaN();
    }
    const auto cap = roleSegments(*provider, geo::ReferenceRole::EndCutter);
    LC_CurveJetBounds bounds;
    REQUIRE(provider->boundJet(cap[1], 0.75, 0.75, bounds));
    checkCutterEnclosure(bounds, cutterOracle(basis(100.0, 0.5, 0.0, 2.0), provider->frame().scale(), 30.0, false, 0.75));
    const auto snapshot = retained->snapshot(handle);
    REQUIRE(snapshot);
    geo::WorkBudget foreign;
    const auto rejected = retained->contract(handle, foreign);
    CHECK(rejected.status == LC_IntersectionStatus::InvalidInput);
    CHECK(rejected.workSource == geo::CriticalWorkSource::LastMatchingBudget);
    checkWork(rejected.entered, {});
    checkWork(foreign.consumed(), {});
    REQUIRE(retained->snapshot(handle));
    CHECK(retained->snapshot(handle)->occurrence.parameter.lo() == snapshot->occurrence.parameter.lo());
    CHECK(retained->snapshot(handle)->occurrence.parameter.hi() == snapshot->occurrence.parameter.hi());
}

TEST_CASE("Reference provider refusal preserves valid thin frame diagnostics without publishing geometry",
          "[lc-offsetcurve][geometry][reference-providers]") {
    const auto data = basis(1.0, 1e-300);
    REQUIRE(geo::ReferenceFrame::create(data, 1.0));
    geo::WorkBudget budget;
    const auto result = geo::prepareReferenceProviders(data, 1.0, budget);
    CHECK(result.status == LC_IntersectionStatus::AmbiguousTopology);
    CHECK(result.reason == geo::ReferencePreparationReason::UnresolvedGeometry);
    checkNoProviders(result, budget);
    CHECK(result.work.vertices > 0);
    CHECK(result.work.distancePairs > 0);
}

TEST_CASE("Reference provider preparation carries actual critical depth and refuses one less without owners",
          "[lc-offsetcurve][geometry][reference-providers]") {
    for (const bool reversed : {false, true}) {
        const auto data = reversed ? basis(100.0, 0.5, 3.0, 0.0, true) : basis(100.0, 0.5, 0.0, 2.0);
        geo::WorkBudget baselineBudget;
        const auto baseline = geo::prepareReferenceProviders(data, 100.0, baselineBudget);
        REQUIRE(baseline.status == LC_IntersectionStatus::Ok);
        REQUIRE(baseline.work.deepest > 0);
        for (const bool oneLess : {false, true}) {
            LC_OffsetCheckLimits limits;
            limits.maxVertices = baseline.work.vertices;
            limits.maxDistancePairs = baseline.work.distancePairs;
            limits.maxDepth = baseline.work.deepest - (oneLess ? 1 : 0);
            geo::WorkBudget budget{limits};
            const auto result = geo::prepareReferenceProviders(data, 100.0, budget);
            CHECK(result.status == (oneLess ? LC_IntersectionStatus::LimitExceeded : LC_IntersectionStatus::Ok));
            CHECK(result.work.deepest <= limits.maxDepth);
            checkWork(result.work, budget.consumed());
            if (oneLess) {
                checkNoProviders(result, budget);
                CHECK(result.reason == geo::ReferencePreparationReason::None);
            } else {
                REQUIRE(result.provider);
                checkWork(result.work, baseline.work);
            }
        }
    }
}

TEST_CASE("Reference provider raw validation never repairs malformed source definitions",
          "[lc-offsetcurve][geometry][reference-providers]") {
    const auto original = basis(10.0, 0.5, 0.0, 2.0);
    for (int field = 0; field < 8; ++field) {
        auto data = original;
        switch (field) {
        case 0: data.majorP = {0.0, 0.0}; break;
        case 1: data.majorP.z = 1.0; break;
        case 2: data.center.z = 1.0; break;
        case 3: data.majorP.x = std::numeric_limits<double>::infinity(); break;
        case 4: data.ratio = 0.0; break;
        case 5: data.ratio = 1.5; break;
        case 6: data.angle1 = std::numeric_limits<double>::quiet_NaN(); break;
        case 7: data.angle2 = 7.0; break;
        }
        CAPTURE(field);
        const auto before = data;
        geo::WorkBudget budget;
        const auto result = geo::prepareReferenceProviders(data, 1.0, budget);
        CHECK(result.status == LC_IntersectionStatus::InvalidInput);
        CHECK(result.reason == geo::ReferencePreparationReason::None);
        checkNoProviders(result, budget);
        checkWork(result.work, {});
        if (field != 6) { checkSameFields(data, before); }
        else { CHECK(std::isnan(data.angle1)); }
        CHECK(data.center.z == before.center.z);
        CHECK(data.majorP.z == before.majorP.z);
    }
}

namespace {
struct ConstructionBox {
    LC_IntersectionDomain box;
    double sourceEnd, capEnd;
};

ConstructionBox constructionBox(const geo::ReferenceProviders& provider, const geo::ReferenceRole offset,
                                const geo::ReferenceRole cutter, const double delta) {
    const bool start = cutter == geo::ReferenceRole::StartCutter;
    const bool outer = offset == geo::ReferenceRole::Own ? delta > 0.0 : delta < 0.0;
    const auto indices = roleSegments(provider, offset), caps = roleSegments(provider, cutter);
    REQUIRE_FALSE(indices.empty());
    REQUIRE(caps.size() == 2);
    const auto cached = provider.contactProofInterval(offset, cutter);
    REQUIRE(cached);
    const std::size_t index = start ? indices.front() : indices.back();
    const auto segment = provider.segments()[index];
    const auto cap = caps[outer ? 0 : 1];
    const auto capSegment = provider.segments()[cap];
    return {{index, cap,
             LC_Interval::hull(std::max(segment.t0, cached->lo()), std::min(segment.t1, cached->hi())),
             LC_Interval::hull(capSegment.t0, capSegment.t1)},
             start ? 0.0 : provider.frame().parameterEnd(), outer ? 0.0 : 1.0};
}

void checkConstructionProof(const geo::ReferenceProviders& provider, const ConstructionBox& target) {
    const auto& box = target.box;
    for (const bool swapped : {false, true}) {
        const LC_IntersectionDomain request{swapped ? box.segmentB : box.segmentA,
            swapped ? box.segmentA : box.segmentB, swapped ? box.parameterB : box.parameterA,
            swapped ? box.parameterA : box.parameterB};
        const auto proof = provider.certifyIntersection(request);
        REQUIRE(proof.outcome == LC_AnalyticIntersectionCertificate::Outcome::Unique);
        CHECK(proof.proof == LC_IntersectionProof::AnalyticEndpoint);
        CHECK(proof.kind == LC_ParametricIntersection::Kind::Endpoint);
        CHECK(proof.proofDomain.segmentA == request.segmentA);
        CHECK(proof.proofDomain.segmentB == request.segmentB);
        CHECK(proof.proofDomain.parameterA.lo() == request.parameterA.lo());
        CHECK(proof.proofDomain.parameterA.hi() == request.parameterA.hi());
        CHECK(proof.proofDomain.parameterB.lo() == request.parameterB.lo());
        CHECK(proof.proofDomain.parameterB.hi() == request.parameterB.hi());
        CHECK(proof.isolationA.lo() == (swapped ? target.capEnd : target.sourceEnd));
        CHECK(proof.isolationA.hi() == proof.isolationA.lo());
        CHECK(proof.isolationB.lo() == (swapped ? target.sourceEnd : target.capEnd));
        CHECK(proof.isolationB.hi() == proof.isolationB.lo());
    }
}

CriticalFloat radialDerivativeOracle(const RS_EllipseData& data, const geo::ReferenceFrame& frame,
                                     const bool start, const double t) {
    const auto current = referenceOracle(data, frame, 0.0, t, false);
    const auto endpoint = referenceOracle(data, frame, 0.0, start ? 0.0 : frame.parameterEnd(), false);
    return CriticalFloat(current[2]) * CriticalFloat(current[2]) +
        CriticalFloat(current[3]) * CriticalFloat(current[3]) +
        CriticalFloat(current[4]) * (CriticalFloat(current[0]) - CriticalFloat(endpoint[0])) +
        CriticalFloat(current[5]) * (CriticalFloat(current[1]) - CriticalFloat(endpoint[1]));
}

void checkWholeRadialProof(const geo::ReferenceProviders& provider, const geo::ReferenceRole offset,
                           const geo::ReferenceRole cutter) {
    const auto range = provider.contactProofInterval(offset, cutter);
    REQUIRE(range);
    const bool start = cutter == geo::ReferenceRole::StartCutter;
    const double endpointT = start ? 0.0 : provider.frame().parameterEnd();
    LC_CurveJetBounds endpoint, span;
    REQUIRE(provider.frame().boundSourceJet(endpointT, endpointT, endpoint));
    REQUIRE(provider.frame().boundSourceJet(range->lo(), range->hi(), span));
    const auto hPrime = span.speedSquaredProduct + span.ddx * (span.x - endpoint.x) +
        span.ddy * (span.y - endpoint.y);
    REQUIRE(hPrime.isValid());
    CHECK(hPrime.isPositive());
}
} // namespace

TEST_CASE("Contact prepared providers certify both offset roles and ends without changing legacy hooks",
          "[lc-offsetcurve][geometry][reference-contacts]") {
    for (const bool reversed : {false, true}) {
        for (const double delta : {-1.0, 1.0}) {
            const auto data = reversed ? basis(10.0, 0.5, 2.0, 0.0, true) : basis(10.0, 0.5, 0.0, 2.0);
            geo::WorkBudget oldBudget, newBudget;
            const auto legacy = geo::prepareReferenceProviders(data, delta, oldBudget);
            auto contact = geo::prepareReferenceContactProviders(data, delta, newBudget);
            REQUIRE(legacy.status == LC_IntersectionStatus::Ok);
            REQUIRE(contact.status == LC_IntersectionStatus::Ok);
            checkProviderLayout(*contact.provider, false, false);
            for (const auto offset : {geo::ReferenceRole::Own, geo::ReferenceRole::Opposite}) {
                for (const auto cutter : {geo::ReferenceRole::StartCutter, geo::ReferenceRole::EndCutter}) {
                    CHECK_FALSE(legacy.provider->contactProofInterval(offset, cutter));
                    const auto box = constructionBox(*contact.provider, offset, cutter, delta);
                    checkConstructionProof(*contact.provider, box);
                    checkUnavailableProviderProof(legacy.provider->certifyIntersection(box.box));
                    const bool outer = offset == geo::ReferenceRole::Own ? delta > 0.0 : delta < 0.0;
                    const auto range = contact.provider->contactProofInterval(offset, cutter);
                    if (outer) {
                        CHECK(range->lo() == 0.0);
                        CHECK(range->hi() == contact.provider->frame().parameterEnd());
                    } else {
                        checkWholeRadialProof(*contact.provider, offset, cutter);
                    }
                }
            }
            checkWork(contact.work, newBudget.consumed());
        }
    }
}

TEST_CASE("Contact certificates exclude only complete cached boxes without the known construction point",
          "[lc-offsetcurve][geometry][reference-contacts]") {
    geo::WorkBudget budget;
    const auto prepared = geo::prepareReferenceContactProviders(basis(10.0, 0.5, 0.0, 2.0), -1.0, budget);
    REQUIRE(prepared.status == LC_IntersectionStatus::Ok);
    const auto& provider = *prepared.provider;
    const auto work = budget.consumed();
    for (const auto offset : {geo::ReferenceRole::Own, geo::ReferenceRole::Opposite}) {
        for (const auto cutter : {geo::ReferenceRole::StartCutter, geo::ReferenceRole::EndCutter}) {
            const auto target = constructionBox(provider, offset, cutter, -1.0);
            for (const bool omitSource : {false, true}) {
                auto request = target.box;
                auto& interval = omitSource ? request.parameterA : request.parameterB;
                const double middle = 0.5 * interval.lo() + 0.5 * interval.hi();
                REQUIRE(interval.lo() < middle);
                REQUIRE(middle < interval.hi());
                const double endpoint = omitSource ? target.sourceEnd : target.capEnd;
                interval = endpoint == interval.lo() ? LC_Interval::hull(middle, interval.hi()) :
                    LC_Interval::hull(interval.lo(), middle);
                for (const bool swapped : {false, true}) {
                    const auto proof = provider.certifyIntersection(request);
                    REQUIRE(proof.outcome == LC_AnalyticIntersectionCertificate::Outcome::Excluded);
                    CHECK(proof.proof == LC_IntersectionProof::AnalyticExclusion);
                    CHECK(proof.proofDomain.segmentA == request.segmentA);
                    CHECK(proof.proofDomain.segmentB == request.segmentB);
                    CHECK(proof.proofDomain.parameterA.lo() == request.parameterA.lo());
                    CHECK(proof.proofDomain.parameterA.hi() == request.parameterA.hi());
                    CHECK(proof.proofDomain.parameterB.lo() == request.parameterB.lo());
                    CHECK(proof.proofDomain.parameterB.hi() == request.parameterB.hi());
                    if (!swapped) {
                        std::swap(request.segmentA, request.segmentB);
                        std::swap(request.parameterA, request.parameterB);
                    }
                }
            }
        }
    }
    checkWork(budget.consumed(), work);
}

TEST_CASE("Contact diagnostics are unavailable for invalid absent whole and legacy roles",
          "[lc-offsetcurve][geometry][reference-contacts]") {
    for (const bool collapsed : {false, true}) {
        for (const bool whole : {false, true}) {
            const auto data = collapsed ? (whole ? basis(5.0, 1.0) : basis(5.0, 1.0, 0.0, 2.0)) :
                whole ? basis() : basis(10.0, 0.5, 0.0, 2.0);
            geo::WorkBudget budget;
            const auto result = geo::prepareReferenceContactProviders(data, collapsed ? 5.0 : 1.0, budget);
            REQUIRE(result.status == LC_IntersectionStatus::Ok);
            checkProviderLayout(*result.provider, whole, collapsed);
            for (const auto cutter : {geo::ReferenceRole::StartCutter, geo::ReferenceRole::EndCutter}) {
                CHECK(bool(result.provider->contactProofInterval(geo::ReferenceRole::Own, cutter)) == !whole);
                CHECK(bool(result.provider->contactProofInterval(geo::ReferenceRole::Opposite, cutter)) ==
                      (!whole && !collapsed));
            }
            CHECK_FALSE(result.provider->contactProofInterval(geo::ReferenceRole::Own, geo::ReferenceRole::Own));
            CHECK_FALSE(result.provider->contactProofInterval(geo::ReferenceRole::StartCutter, geo::ReferenceRole::EndCutter));
            CHECK_FALSE(result.provider->contactProofInterval(static_cast<geo::ReferenceRole>(-1), geo::ReferenceRole::EndCutter));
            CHECK_FALSE(result.provider->contactProofInterval(geo::ReferenceRole::Own, static_cast<geo::ReferenceRole>(-1)));
            if (!whole) {
                auto copied = result.provider->contactProofInterval(geo::ReferenceRole::Own, geo::ReferenceRole::StartCutter);
                REQUIRE(copied);
                const auto original = *copied;
                copied = LC_Interval::hull(-100.0, 100.0);
                CHECK(result.provider->contactProofInterval(geo::ReferenceRole::Own, geo::ReferenceRole::StartCutter)->lo() == original.lo());
                CHECK(result.provider->contactProofInterval(geo::ReferenceRole::Own, geo::ReferenceRole::StartCutter)->hi() == original.hi());
            }
        }
    }
}

TEST_CASE("Contact radial neighborhoods qualify exact threshold and ULP adjacent minimum endpoints",
          "[lc-offsetcurve][geometry][reference-contacts]") {
    for (const double distance : {25.0, std::nextafter(25.0, 100.0)}) {
        for (const bool reversed : {false, true}) {
            const auto data = reversed ? basis(100.0, 0.5, 2.0, 0.0, true) : basis(100.0, 0.5, 0.0, 2.0);
            geo::WorkBudget budget;
            const auto result = geo::prepareReferenceContactProviders(data, -distance, budget);
            REQUIRE(result.status == LC_IntersectionStatus::Ok);
            const auto& provider = *result.provider;
            const auto cutter = reversed ? geo::ReferenceRole::EndCutter : geo::ReferenceRole::StartCutter;
            const double endpoint = reversed ? provider.frame().parameterEnd() : 0.0;
            LC_Interval f, df;
            REQUIRE(provider.frame().boundOffsetFactor(endpoint, endpoint, f, df));
            CHECK(f.contains(0.0));
            const auto cache = provider.contactProofInterval(geo::ReferenceRole::Own, cutter);
            REQUIRE(cache);
            REQUIRE(cache->lo() < cache->hi());
            checkWholeRadialProof(provider, geo::ReferenceRole::Own, cutter);
            checkConstructionProof(provider, constructionBox(provider, geo::ReferenceRole::Own, cutter, -distance));
            for (unsigned i = 0; i <= 16; ++i) {
                const double t = cache->lo() + (cache->hi() - cache->lo()) * double(i) / 16.0;
                CHECK(radialDerivativeOracle(data, provider.frame(), !reversed, t) > 0);
            }
            if (distance > 25.0) {
                REQUIRE(result.ownCritical->occurrenceCount() == 1);
                const auto root = result.ownCritical->snapshot(result.ownCritical->occurrenceHandle(0));
                REQUIRE(root);
                if (reversed) { CHECK(root->occurrence.parameter.hi() < cache->lo()); }
                else { CHECK(cache->hi() < root->occurrence.parameter.lo()); }
            } else {
                REQUIRE(result.ownCritical->occurrenceCount() == 1);
                const auto stationary = result.ownCritical->snapshot(result.ownCritical->occurrenceHandle(0));
                REQUIRE(stationary);
                CHECK(stationary->occurrence.kind == geo::CriticalKind::CriticalStationary);
                CHECK(stationary->occurrence.parameter.lo() == endpoint);
                CHECK(stationary->occurrence.parameter.hi() == endpoint);
            }
        }
    }
}

TEST_CASE("Contact preparation refines clipped critical gaps without resetting canonical history",
          "[lc-offsetcurve][geometry][reference-contacts]") {
    for (const bool reversed : {false, true}) {
        const auto data = reversed ? basis(100.0, 0.5, 2.0, 0.7, true) : basis(100.0, 0.5, 0.7, 2.0);
        geo::WorkBudget oldBudget;
        const auto old = geo::prepareReferenceProviders(data, -100.0, oldBudget);
        REQUIRE(old.status == LC_IntersectionStatus::Ok);
        REQUIRE(old.ownCritical->occurrenceCount() == 1);
        const auto before = old.ownCritical->snapshot(old.ownCritical->occurrenceHandle(0));
        REQUIRE(before);
        const double end = old.provider->frame().parameterEnd();
        if (reversed) { REQUIRE(before->occurrence.parameter.hi() == end); }
        else { REQUIRE(before->occurrence.parameter.lo() == 0.0); }
        geo::WorkBudget budget;
        REQUIRE(budget.checkDepth(15)); // An unrelated maximum is not the canonical starting history.
        const auto result = geo::prepareReferenceContactProviders(data, -100.0, budget);
        REQUIRE(result.status == LC_IntersectionStatus::Ok);
        const auto after = result.ownCritical->snapshot(result.ownCritical->occurrenceHandle(0));
        REQUIRE(after);
        CHECK(after->admittedRounds > before->admittedRounds);
        CHECK(after->admittedRounds < 15);
        CHECK(result.work.deepest == 15);
        checkNested(after->occurrence.parameter, before->occurrence.parameter);
        const auto cutter = reversed ? geo::ReferenceRole::EndCutter : geo::ReferenceRole::StartCutter;
        const auto cache = result.provider->contactProofInterval(geo::ReferenceRole::Own, cutter);
        REQUIRE(cache);
        if (reversed) { CHECK(after->occurrence.parameter.hi() < cache->lo()); }
        else { CHECK(cache->hi() < after->occurrence.parameter.lo()); }
        checkWholeRadialProof(*result.provider, geo::ReferenceRole::Own, cutter);
        checkConstructionProof(*result.provider, constructionBox(*result.provider, geo::ReferenceRole::Own, cutter, -100.0));
        checkCriticalSnapshot(data, -100.0, result.provider->frame(), *after);
        checkWork(result.work, budget.consumed());
    }
}

TEST_CASE("Contact hooks never extend an inner endpoint certificate across a nearby transverse recrossing",
          "[lc-offsetcurve][geometry][reference-contacts]") {
    const auto data = basis(10.0, 0.5, 0.0, 1.0);
    geo::WorkBudget budget;
    auto result = geo::prepareReferenceContactProviders(data, -3.0, budget);
    REQUIRE(result.status == LC_IntersectionStatus::Ok);
    const auto* provider = result.provider.get();
    const auto cache = provider->contactProofInterval(geo::ReferenceRole::Own, geo::ReferenceRole::StartCutter);
    REQUIRE(cache);
    const auto target = constructionBox(*provider, geo::ReferenceRole::Own, geo::ReferenceRole::StartCutter, -3.0);
    checkConstructionProof(*provider, target);
    auto broad = target.box;
    broad.parameterA = LC_Interval::hull(provider->segments()[broad.segmentA].t0,
                                        provider->segments()[broad.segmentA].t1);
    checkUnavailableProviderProof(provider->certifyIntersection(broad));
    std::swap(broad.segmentA, broad.segmentB);
    std::swap(broad.parameterA, broad.parameterB);
    checkUnavailableProviderProof(provider->certifyIntersection(broad));
    // Exact-positive-factor squaring gives this monotone quartic in x=cos(theta).
    const auto polynomial = [](const CriticalFloat& x) {
        const CriticalFloat a = CriticalFloat("1.25") - CriticalFloat(".75") * x;
        return CriticalFloat(25) * a * a * (1 - CriticalFloat(".75") * x * x) - CriticalFloat("2.25");
    };
    CriticalFloat lo(".95"), hi(".96");
    REQUIRE(polynomial(lo) > 0);
    REQUIRE(polynomial(hi) < 0);
    for (unsigned i = 0; i < 180; ++i) {
        const CriticalFloat middle = (lo + hi) / 2;
        if (polynomial(middle) > 0) { lo = middle; }
        else { hi = middle; }
    }
    const CriticalFloat x = (lo + hi) / 2, theta = acos(x), s = sqrt(1 - x * x);
    const CriticalFloat h = sqrt(CriticalFloat(".25") * x * x + s * s);
    const CriticalFloat qx = 10 * x - CriticalFloat("1.5") * x / h;
    const CriticalFloat qy = 5 * s - 3 * s / h;
    REQUIRE(qy < 0);
    REQUIRE(qx < 10);
    const CriticalFloat u = atan2(-qy, qx - 10) / boost::math::constants::pi<CriticalFloat>();
    REQUIRE(u > CriticalFloat(".5"));
    REQUIRE(u < 1);
    CHECK(CriticalFloat(cache->hi()) < theta);
    const auto ownBranch = provider->branchFor(geo::ReferenceRole::Own);
    const auto capBranch = provider->branchFor(geo::ReferenceRole::StartCutter);
    REQUIRE(ownBranch);
    REQUIRE(capBranch);
    const double scale = provider->frame().scale();
    LC_IntersectionOptions options;
    options.tolerance = 1e-6;
    options.maxDepth = budget.depthLimit();
    options.maxLeafDepth = budget.depthLimit();
    options.maxBoxPairs = budget.remainingPairs();
    options.maxGeneratedItems = budget.remainingVertices();
    options.maxLeaves = budget.remainingVertices();
    options.anchoredBranches = 1;
    const auto query = findCertifiedIntersections(std::move(result.provider), options);
    REQUIRE(budget.consumePairs(query.work.totalBoxPairs));
    REQUIRE(budget.consumeVertices(query.work.generatedItems));
    REQUIRE(budget.checkDepth(query.work.deepest()));
    REQUIRE(query.status == LC_IntersectionStatus::Ok);
    REQUIRE(query.session);
    std::size_t matched = 0, endpoints = 0, crossings = 0;
    for (std::size_t i = 0; i < query.session->rootCount(); ++i) {
        const auto root = query.session->snapshot(query.session->rootHandle(i));
        REQUIRE(root);
        const auto& event = root->event;
        if (event.branchA != *ownBranch || event.branchB != *capBranch) { continue; }
        ++matched;
        if (event.isolation.parameterA.lo() == 0.0 && event.isolation.parameterA.hi() == 0.0) {
            ++endpoints;
            CHECK(event.proof == LC_IntersectionProof::AnalyticEndpoint);
            CHECK(event.isolation.parameterB.lo() == 1.0);
            CHECK(event.isolation.parameterB.hi() == 1.0);
        } else {
            ++crossings;
            CHECK(event.proof == LC_IntersectionProof::IntervalUniqueTransverse);
            CHECK(event.kind == LC_ParametricIntersection::Kind::Transverse);
            checkCriticalEnclosure(event.isolation.parameterA, theta);
            checkCriticalEnclosure(event.isolation.parameterB, u);
            checkCriticalEnclosure(root->imageX, qx / scale);
            checkCriticalEnclosure(root->imageY, qy / scale);
        }
    }
    CHECK(matched == 2);
    CHECK(endpoints == 1);
    CHECK(crossings == 1);
}

TEST_CASE("Contact prepared geometry qualifies the six required source families without publishing events",
          "[lc-offsetcurve][geometry][reference-contacts]") {
    struct Fixture { RS_EllipseData source; double delta; };
    const std::array<Fixture, 6> fixtures{{
        {basis(10.0, 0.5), 3.0}, {basis(10.0, 0.5), -1.0},
        {basis(100.0, 0.6), -36.036}, {basis(10.0, 0.5, M_PI / 3, 2 * M_PI / 3), -2.0},
        {basis(100.0, 0.5, M_PI / 18, 35 * M_PI / 18), -49.5},
        {basis(100.0, 0.05, M_PI / 12, 7 * M_PI / 4), -4.0}}};
    for (std::size_t f = 0; f < fixtures.size(); ++f) {
        for (const bool reversed : {false, true}) {
            auto data = fixtures[f].source;
            if (reversed) {
                std::swap(data.angle1, data.angle2);
                data.reversed = true;
            }
            // Exact ninety-degree rotation does not introduce an unrelated norm perturbation at thresholds.
            data.majorP = {0.0, data.majorP.x};
            data.center = {1e200, -1e200};
            CAPTURE(f, reversed);
            geo::WorkBudget budget;
            const auto result = geo::prepareReferenceContactProviders(data, fixtures[f].delta, budget);
            REQUIRE(result.status == LC_IntersectionStatus::Ok);
            REQUIRE(result.provider);
            if (result.provider->frame().sourceSweep().whole) {
                CHECK_FALSE(result.provider->contactProofInterval(geo::ReferenceRole::Own, geo::ReferenceRole::StartCutter));
                continue;
            }
            for (const auto offset : {geo::ReferenceRole::Own, geo::ReferenceRole::Opposite}) {
                for (const auto cutter : {geo::ReferenceRole::StartCutter, geo::ReferenceRole::EndCutter}) {
                    const auto target = constructionBox(*result.provider, offset, cutter, fixtures[f].delta);
                    checkConstructionProof(*result.provider, target);
                    const bool inner = offset == geo::ReferenceRole::Own ? fixtures[f].delta < 0.0 : fixtures[f].delta > 0.0;
                    if (inner) {
                        checkWholeRadialProof(*result.provider, offset, cutter);
                        const auto interval = result.provider->contactProofInterval(offset, cutter);
                        for (unsigned i = 0; i <= 8; ++i) {
                            const double t = interval->lo() + (interval->hi() - interval->lo()) * double(i) / 8;
                            CHECK(radialDerivativeOracle(data, result.provider->frame(),
                                cutter == geo::ReferenceRole::StartCutter, t) > 0);
                        }
                    }
                }
            }
            checkWork(result.work, budget.consumed());
        }
    }
}

namespace {
unsigned radialDepth(const geo::ReferenceProviders& provider, const geo::ReferenceRole offset,
                     const geo::ReferenceRole cutter) {
    const auto range = provider.contactProofInterval(offset, cutter);
    REQUIRE(range);
    const double length = range->hi() - range->lo();
    double expected = provider.frame().parameterEnd();
    unsigned depth = 0;
    while (expected != length && depth < 20) { expected *= 0.5; ++depth; }
    REQUIRE(expected == length); // These accounting fixtures use exact dyadic T=2 and no critical gap.
    return depth;
}
} // namespace

TEST_CASE("Contact metadata costs match construction pairs and actual radial examinations",
          "[lc-offsetcurve][geometry][reference-contacts]") {
    for (const bool whole : {false, true}) {
        for (const bool collapsed : {false, true}) {
            const auto data = collapsed ? (whole ? basis(5.0, 1.0) : basis(5.0, 1.0, 0.0, 2.0)) :
                whole ? basis(10.0, 0.5) : basis(10.0, 0.5, 0.0, 2.0);
            const double delta = collapsed ? 5.0 : 1.0;
            geo::WorkBudget oldBudget, budget;
            const auto old = geo::prepareReferenceProviders(data, delta, oldBudget);
            const auto contact = geo::prepareReferenceContactProviders(data, delta, budget);
            REQUIRE(old.status == LC_IntersectionStatus::Ok);
            REQUIRE(contact.status == LC_IntersectionStatus::Ok);
            const std::size_t c = whole ? 0 : collapsed ? 2 : 4;
            std::size_t j = 0;
            unsigned deepest = old.work.deepest;
            if (!whole && !collapsed) {
                for (const auto cutter : {geo::ReferenceRole::StartCutter, geo::ReferenceRole::EndCutter}) {
                    const auto depth = radialDepth(*contact.provider, geo::ReferenceRole::Opposite, cutter);
                    j += depth + 1;
                    deepest = std::max(deepest, depth);
                }
            }
            CHECK(contact.work.vertices == old.work.vertices + 1 + c + j);
            CHECK(contact.work.distancePairs == old.work.distancePairs + 1 + c + j);
            CHECK(contact.work.deepest == deepest);
            CHECK(contact.work.cacheSpans == old.work.cacheSpans);
            checkWork(contact.work, budget.consumed());
            for (const bool prespent : {false, true}) {
                for (int cap = 0; cap < 3; ++cap) {
                    LC_OffsetCheckLimits limits;
                    limits.maxVertices = contact.work.vertices + (prespent ? 5 : 0) - (cap == 1 ? 1 : 0);
                    limits.maxDistancePairs = contact.work.distancePairs + (prespent ? 7 : 0) - (cap == 2 ? 1 : 0);
                    geo::WorkBudget bounded{limits};
                    if (prespent) {
                        REQUIRE(bounded.consumeVertices(5));
                        REQUIRE(bounded.consumePairs(7));
                        REQUIRE(bounded.checkDepth(15));
                    }
                    const auto result = geo::prepareReferenceContactProviders(data, delta, bounded);
                    CHECK(result.status == (cap == 0 ? LC_IntersectionStatus::Ok : LC_IntersectionStatus::LimitExceeded));
                    checkWork(result.work, bounded.consumed());
                    if (cap == 0) {
                        REQUIRE(result.provider);
                        CHECK(result.work.vertices == limits.maxVertices);
                        CHECK(result.work.distancePairs == limits.maxDistancePairs);
                        CHECK(result.work.deepest == (prespent ? 15 : deepest));
                    } else {
                        checkNoProviders(result, bounded);
                        const auto work = bounded.consumed();
                        const auto again = geo::prepareReferenceContactProviders(data, delta, bounded);
                        CHECK(again.status == LC_IntersectionStatus::LimitExceeded);
                        checkNoProviders(again, bounded);
                        checkWork(again.work, work);
                    }
                }
            }
        }
    }
}

TEST_CASE("Contact failed item pair and depth admission retains the exact entered stage prefix",
          "[lc-offsetcurve][geometry][reference-contacts][budget-core]") {
    const auto data = basis(10.0, 0.5, 0.0, 2.0);
    geo::WorkBudget oldBudget, completeBudget;
    const auto old = geo::prepareReferenceProviders(data, 1.0, oldBudget);
    const auto complete = geo::prepareReferenceContactProviders(data, 1.0, completeBudget);
    REQUIRE(old.status == LC_IntersectionStatus::Ok);
    REQUIRE(complete.status == LC_IntersectionStatus::Ok);
    struct Stage { std::optional<unsigned> depth; };
    std::vector<Stage> stages{Stage{std::nullopt}}; // Contact control; item precedes its pair.
    for (const auto offset : {geo::ReferenceRole::Own, geo::ReferenceRole::Opposite}) {
        for (const auto cutter : {geo::ReferenceRole::StartCutter, geo::ReferenceRole::EndCutter}) {
            stages.push_back({std::nullopt}); // Construction record/initial examination.
            if (offset == geo::ReferenceRole::Own) { continue; } // Positive own, negative opposite.
            const auto last = radialDepth(*complete.provider, offset, cutter);
            for (unsigned depth = 0; depth <= last; ++depth) { stages.push_back({depth}); }
        }
    }
    REQUIRE(complete.work.vertices == old.work.vertices + stages.size());
    REQUIRE(complete.work.distancePairs == old.work.distancePairs + stages.size());
    for (const bool pairs : {false, true}) {
        unsigned observed = old.work.deepest;
        for (std::size_t entered = 0; entered < stages.size(); ++entered) {
            CAPTURE(pairs, entered);
            LC_OffsetCheckLimits limits;
            if (pairs) { limits.maxDistancePairs = old.work.distancePairs + entered; }
            else { limits.maxVertices = old.work.vertices + entered; }
            geo::WorkBudget budget{limits};
            const auto failed = geo::prepareReferenceContactProviders(data, 1.0, budget);
            CHECK(failed.status == LC_IntersectionStatus::LimitExceeded);
            checkNoProviders(failed, budget);
            const bool itemAdmittedBeforePair = pairs && !stages[entered].depth;
            CHECK(failed.work.vertices == old.work.vertices + entered + (itemAdmittedBeforePair ? 1 : 0));
            CHECK(failed.work.distancePairs == old.work.distancePairs + entered);
            CHECK(failed.work.deepest == observed); // An admitted item with a failed pair is not examined.
            if (stages[entered].depth) { observed = std::max(observed, *stages[entered].depth); }
        }
    }
    REQUIRE(complete.work.deepest > old.work.deepest);
    for (const bool oneLess : {false, true}) {
        LC_OffsetCheckLimits limits;
        limits.maxDepth = complete.work.deepest - (oneLess ? 1 : 0);
        geo::WorkBudget budget{limits};
        const auto result = geo::prepareReferenceContactProviders(data, 1.0, budget);
        CHECK(result.status == (oneLess ? LC_IntersectionStatus::LimitExceeded : LC_IntersectionStatus::Ok));
        CHECK(result.work.deepest <= limits.maxDepth);
        checkWork(result.work, budget.consumed());
        if (oneLess) { checkNoProviders(result, budget); }
        else { checkWork(result.work, complete.work); }
    }
}

TEST_CASE("Contact clipped gap retries spend critical work once with persistent root depth",
          "[lc-offsetcurve][geometry][reference-contacts]") {
    const auto data = basis(100.0, 0.5, 0.7, 2.0);
    geo::WorkBudget oldBudget, budget;
    const auto old = geo::prepareReferenceProviders(data, -100.0, oldBudget);
    const auto result = geo::prepareReferenceContactProviders(data, -100.0, budget);
    REQUIRE(old.status == LC_IntersectionStatus::Ok);
    REQUIRE(result.status == LC_IntersectionStatus::Ok);
    const auto a = old.ownCritical->snapshot(old.ownCritical->occurrenceHandle(0));
    const auto b = result.ownCritical->snapshot(result.ownCritical->occurrenceHandle(0));
    REQUIRE(a);
    REQUIRE(b);
    REQUIRE(b->admittedRounds > a->admittedRounds);
    const std::size_t rounds = b->admittedRounds - a->admittedRounds;
    const std::size_t n = result.ownCritical->occurrenceCount();
    REQUIRE(n == 1);
    const std::size_t extraV = result.work.vertices - old.work.vertices;
    const std::size_t extraP = result.work.distancePairs - old.work.distancePairs;
    // One contact control + four pairs, J radial exams; each successful canonical round V1+N/P3+N,
    // followed by one pair-only gap retry. No metadata/leaf count is debited a second time.
    REQUIRE(extraV >= 5 + rounds * (1 + n));
    const std::size_t radial = extraV - 5 - rounds * (1 + n);
    CHECK(radial >= 2);
    CHECK(extraP == 5 + radial + rounds * (3 + n) + rounds);
    for (int cap = 0; cap < 3; ++cap) {
        LC_OffsetCheckLimits limits;
        limits.maxVertices = result.work.vertices - (cap == 1 ? 1 : 0);
        limits.maxDistancePairs = result.work.distancePairs - (cap == 2 ? 1 : 0);
        geo::WorkBudget bounded{limits};
        const auto next = geo::prepareReferenceContactProviders(data, -100.0, bounded);
        CHECK(next.status == (cap == 0 ? LC_IntersectionStatus::Ok : LC_IntersectionStatus::LimitExceeded));
        checkWork(next.work, bounded.consumed());
        if (cap != 0) { checkNoProviders(next, bounded); }
    }
}

TEST_CASE("Contact refusal inside a clipped canonical round retains actual scalar inverse and map charges",
          "[lc-offsetcurve][geometry][reference-contacts]") {
    const auto data = basis(100.0, 0.5, 0.7, 2.0);
    geo::WorkBudget baseBudget;
    const auto base = geo::prepareReferenceProviders(data, -100.0, baseBudget);
    REQUIRE(base.status == LC_IntersectionStatus::Ok);
    REQUIRE(base.ownCritical->occurrenceCount() == 1);
    const auto initial = base.ownCritical->snapshot(base.ownCritical->occurrenceHandle(0));
    REQUIRE(initial);
    REQUIRE(initial->occurrence.parameter.lo() == 0.0);
    for (std::size_t extraPairs = 2; extraPairs <= 6; ++extraPairs) {
        CAPTURE(extraPairs);
        LC_OffsetCheckLimits limits;
        limits.maxDistancePairs = base.work.distancePairs + extraPairs;
        geo::WorkBudget budget{limits};
        const auto result = geo::prepareReferenceContactProviders(data, -100.0, budget);
        CHECK(result.status == LC_IntersectionStatus::LimitExceeded);
        checkNoProviders(result, budget);
        // Control/record V2P2, scalar V1P1, inverse P1, common P1, occurrence V1P1, gap retry P1.
        CHECK(result.work.vertices == base.work.vertices + (extraPairs < 5 ? 3 : 4));
        CHECK(result.work.distancePairs == base.work.distancePairs + extraPairs);
        CHECK(result.work.deepest == std::max(base.work.deepest,
            initial->admittedRounds + (extraPairs >= 3 ? 1u : 0u)));
    }
}

TEST_CASE("Contact global weak factor signs allow isolated stationary zeros without scalar refinement",
          "[lc-offsetcurve][geometry][reference-contacts]") {
    for (const double distance : {1.0, 1.25, 10.0, 11.0}) {
        geo::WorkBudget baseBudget, budget;
        const auto data = basis(5.0, 0.5, 0.0, 2.0);
        const auto old = geo::prepareReferenceProviders(data, -distance, baseBudget);
        const auto result = geo::prepareReferenceContactProviders(data, -distance, budget);
        REQUIRE(old.status == LC_IntersectionStatus::Ok);
        REQUIRE(result.status == LC_IntersectionStatus::Ok);
        CHECK(result.work.vertices - old.work.vertices == result.work.distancePairs - old.work.distancePairs);
        for (const auto cutter : {geo::ReferenceRole::StartCutter, geo::ReferenceRole::EndCutter}) {
            checkWholeRadialProof(*result.provider, geo::ReferenceRole::Own, cutter);
            checkConstructionProof(*result.provider, constructionBox(*result.provider, geo::ReferenceRole::Own, cutter, -distance));
        }
        for (std::size_t i = 0; i < result.ownCritical->occurrenceCount(); ++i) {
            const auto root = result.ownCritical->snapshot(result.ownCritical->occurrenceHandle(i));
            REQUIRE(root);
            CHECK(root->admittedRounds == 0);
        }
    }
}

TEST_CASE("Contact unavailable near critical endpoint evidence cannot mint a construction certificate",
          "[lc-offsetcurve][geometry][reference-contacts]") {
    // A genuine raw endpoint within interval-predicate uncertainty, not a fabricated provider/certificate.
    const double endpoint = 0.7;
    const CriticalFloat angle = endpoint, r(".5");
    const CriticalFloat h2 = r * r * cos(angle) * cos(angle) + sin(angle) * sin(angle);
    const double distance = (CriticalFloat(100) * h2 * sqrt(h2) / r).convert_to<double>();
    const auto data = basis(100.0, 0.5, 0.0, endpoint);
    geo::WorkBudget oldBudget, budget;
    const auto old = geo::prepareReferenceProviders(data, -distance, oldBudget);
    const auto result = geo::prepareReferenceContactProviders(data, -distance, budget);
    REQUIRE(old.status == LC_IntersectionStatus::AmbiguousTopology);
    CHECK(result.status == LC_IntersectionStatus::AmbiguousTopology);
    CHECK(result.reason == geo::ReferencePreparationReason::UnresolvedGeometry);
    checkNoProviders(result, budget);
    checkWork(result.work, old.work); // Unavailable base evidence is not upgraded merely by opting into contacts.
}

TEST_CASE("Contact hook refusals clear proof outputs and cannot be enlarged by diagnostic copies",
          "[lc-offsetcurve][geometry][reference-contacts]") {
    geo::WorkBudget budget;
    const auto result = geo::prepareReferenceContactProviders(basis(10.0, 0.5, 0.0, 1.0), -3.0, budget);
    REQUIRE(result.status == LC_IntersectionStatus::Ok);
    const auto& provider = *result.provider;
    const auto work = budget.consumed();
    const auto target = constructionBox(provider, geo::ReferenceRole::Own, geo::ReferenceRole::StartCutter, -3.0);
    auto copied = *provider.contactProofInterval(geo::ReferenceRole::Own, geo::ReferenceRole::StartCutter);
    copied = LC_Interval::hull(0.0, 1.0);
    auto outside = target.box;
    outside.parameterA = copied;
    checkUnavailableProviderProof(provider.certifyIntersection(outside));
    auto invalid = target.box;
    invalid.segmentA = provider.segments().size();
    checkUnavailableProviderProof(provider.certifyIntersection(invalid));
    invalid = target.box;
    invalid.parameterB = LC_Interval{};
    checkUnavailableProviderProof(provider.certifyIntersection(invalid));
    invalid = target.box;
    invalid.parameterB = LC_Interval::hull(0.4, 1.0); // Named cap piece is [.5,1].
    checkUnavailableProviderProof(provider.certifyIntersection(invalid));
    const auto actual = provider.contactProofInterval(geo::ReferenceRole::Own, geo::ReferenceRole::StartCutter);
    REQUIRE(actual);
    CHECK(actual->hi() < copied.hi());
    checkWork(budget.consumed(), work);
}

TEST_CASE("Contact provider and issued critical state survive independent ownership and budget lifetimes",
          "[lc-offsetcurve][geometry][reference-contacts]") {
    std::unique_ptr<const geo::ReferenceProviders> provider;
    std::unique_ptr<geo::CriticalRefinementSession> critical;
    geo::CriticalOccurrenceHandle handle;
    std::optional<ConstructionBox> box;
    {
        auto data = basis(100.0, 0.5, 0.7, 2.0);
        geo::WorkBudget budget;
        auto result = geo::prepareReferenceContactProviders(data, -100.0, budget);
        REQUIRE(result.status == LC_IntersectionStatus::Ok);
        box = constructionBox(*result.provider, geo::ReferenceRole::Own, geo::ReferenceRole::StartCutter, -100.0);
        handle = result.ownCritical->occurrenceHandle(0);
        critical = std::move(result.ownCritical);
        provider = std::move(result.provider);
        data.majorP = {0.0, 0.0};
    }
    REQUIRE(box);
    checkConstructionProof(*provider, *box);
    REQUIRE(critical->snapshot(handle));
    const auto* borrowed = provider.get();
    const auto proof = provider->contactProofInterval(geo::ReferenceRole::Own, geo::ReferenceRole::StartCutter);
    REQUIRE(proof);
    std::unique_ptr<const LC_ParametricCurves> owned = std::move(provider);
    checkConstructionProof(*borrowed, *box);
    owned.reset();
    REQUIRE(critical->snapshot(handle));
    geo::WorkBudget foreign;
    const auto rejected = critical->contract(handle, foreign);
    CHECK(rejected.status == LC_IntersectionStatus::InvalidInput);
    CHECK(rejected.workSource == geo::CriticalWorkSource::LastMatchingBudget);
    checkWork(rejected.entered, {});
    checkWork(foreign.consumed(), {});
    REQUIRE(critical->snapshot(handle));
}

TEST_CASE("Contact radial proofs respect true wrapped traversal rates under rotation and reversal",
          "[lc-offsetcurve][geometry][reference-contacts]") {
    for (const bool reversed : {false, true}) {
        auto data = reversed ? basis(100.0, 0.5, 0.2, 5.8, true) : basis(100.0, 0.5, 5.8, 0.2);
        data.majorP = {60.0, 80.0};
        data.center = {-1e200, 1e200};
        geo::WorkBudget budget;
        const auto result = geo::prepareReferenceContactProviders(data, -1.0, budget);
        REQUIRE(result.status == LC_IntersectionStatus::Ok);
        for (const auto offset : {geo::ReferenceRole::Own, geo::ReferenceRole::Opposite}) {
            for (const auto cutter : {geo::ReferenceRole::StartCutter, geo::ReferenceRole::EndCutter}) {
                checkConstructionProof(*result.provider, constructionBox(*result.provider, offset, cutter, -1.0));
                if (offset != geo::ReferenceRole::Own) { continue; }
                checkWholeRadialProof(*result.provider, offset, cutter);
                const auto interval = result.provider->contactProofInterval(offset, cutter);
                for (unsigned i = 0; i <= 16; ++i) {
                    const double t = interval->lo() + (interval->hi() - interval->lo()) * double(i) / 16.0;
                    CHECK(radialDerivativeOracle(data, result.provider->frame(),
                        cutter == geo::ReferenceRole::StartCutter, t) > 0);
                }
            }
        }
    }
}

TEST_CASE("Contact request failures never publish partially prepared geometry or proof metadata",
          "[lc-offsetcurve][geometry][reference-contacts]") {
    for (const double delta : {0.0, std::numeric_limits<double>::quiet_NaN(),
                               std::numeric_limits<double>::infinity()}) {
        geo::WorkBudget budget;
        const auto result = geo::prepareReferenceContactProviders(basis(), delta, budget);
        CHECK(result.status == LC_IntersectionStatus::InvalidInput);
        checkNoProviders(result, budget);
        checkWork(result.work, {});
    }
    geo::WorkBudget collapsedBudget;
    const auto collapsed = geo::prepareReferenceContactProviders(basis(5.0, 1.0), -5.0, collapsedBudget);
    CHECK(collapsed.status == LC_IntersectionStatus::AmbiguousTopology);
    CHECK(collapsed.reason == geo::ReferencePreparationReason::CollapsedCircle);
    checkNoProviders(collapsed, collapsedBudget);
    geo::WorkBudget invalid{LC_OffsetCheckLimits{1, 1, 1, 0}};
    const auto bad = geo::prepareReferenceContactProviders(basis(), 1.0, invalid);
    CHECK(bad.status == LC_IntersectionStatus::InvalidInput);
    checkNoProviders(bad, invalid);
    geo::WorkBudget exhausted{LC_OffsetCheckLimits{1, 1, 1, 20}};
    REQUIRE(exhausted.consumeVertices(1));
    const auto limited = geo::prepareReferenceContactProviders(basis(), 1.0, exhausted);
    CHECK(limited.status == LC_IntersectionStatus::LimitExceeded);
    checkNoProviders(limited, exhausted);
    CHECK(limited.work.vertices == 1);
    CHECK(limited.work.distancePairs == 0);
}

namespace {
using EventRational = boost::multiprecision::cpp_rational;

void checkEventNodes(const geo::ReferenceEventAnalysis& analysis, const double tolerance) {
    REQUIRE_FALSE(analysis.nodes().empty());
    const EventRational maximum = EventRational(tolerance) * EventRational(tolerance) / 256;
    for (const auto& n : analysis.nodes()) {
        REQUIRE(geo::isFinite2D(n.world));
        for (const auto& v : {n.localX, n.localY, n.worldX, n.worldY}) {
            REQUIRE(v.isValid());
            REQUIRE(std::isfinite(v.lo()));
            REQUIRE(std::isfinite(v.hi()));
        }
        REQUIRE(n.worldX.contains(n.world.x));
        REQUIRE(n.worldY.contains(n.world.y));
        const auto error = [](const LC_Interval& v, const double point) -> EventRational {
            const EventRational p(point);
            const EventRational a = p - EventRational(v.lo()), b = EventRational(v.hi()) - p;
            return std::max(a, b);
        };
        const EventRational ex = error(n.worldX, n.world.x), ey = error(n.worldY, n.world.y);
        CHECK(ex * ex + ey * ey <= maximum);
        LC_Interval wx, wy;
        REQUIRE(analysis.providers().frame().pointToWorld(n.localX, n.localY, wx, wy));
        CHECK(n.worldX.lo() <= wx.lo()); CHECK(wx.hi() <= n.worldX.hi());
        CHECK(n.worldY.lo() <= wy.lo()); CHECK(wy.hi() <= n.worldY.hi());
    }
}

void checkEventRecords(const geo::ReferenceEventAnalysis& analysis, const double tolerance) {
    const auto& occurrences = analysis.occurrences();
    REQUIRE_FALSE(occurrences.empty());
    const auto validId = [&](const geo::ReferenceOccurrenceId id) {
        REQUIRE(id.value < occurrences.size());
        REQUIRE(occurrences[id.value].node.value < analysis.nodes().size());
    };
    for (const auto& o : occurrences) {
        REQUIRE(o.parameter.isValid());
        REQUIRE(o.node.value < analysis.nodes().size());
    }
    for (std::size_t i = 0; i < analysis.ownOccurrences().size(); ++i) {
        const auto id = analysis.ownOccurrences()[i];
        validId(id);
        CHECK(occurrences[id.value].role == geo::ReferenceRole::Own);
        if (i) {
            const auto prior = analysis.ownOccurrences()[i - 1];
            CHECK(occurrences[prior.value].parameter.hi() < occurrences[id.value].parameter.lo());
        }
    }
    for (std::size_t i = 1; i < occurrences.size(); ++i) {
        const auto& prior = occurrences[i - 1];
        const auto& current = occurrences[i];
        CHECK(static_cast<unsigned>(prior.role) <= static_cast<unsigned>(current.role));
        if (prior.role == current.role) {
            CHECK(prior.parameter.hi() < current.parameter.lo());
        }
    }
    for (const auto& p : analysis.pairs()) {
        validId(p.first); validId(p.second);
        CHECK(p.proof != LC_IntersectionProof::Uncertified);
        CHECK(occurrences[p.first.value].node.value == occurrences[p.second.value].node.value);
        CHECK((occurrences[p.first.value].role == geo::ReferenceRole::Own ||
               occurrences[p.second.value].role == geo::ReferenceRole::Own));
    }
    for (const auto& c : analysis.criticals()) {
        validId(c.occurrence);
        CHECK(occurrences[c.occurrence.value].role == geo::ReferenceRole::Own);
    }
    if (analysis.seam()) {
        REQUIRE(analysis.freeEnds().empty());
        const auto& s = *analysis.seam();
        validId(s.start); validId(s.end);
        CHECK(s.start.value != s.end.value);
        CHECK(occurrences[s.start.value].parameter.lo() == 0.0);
        CHECK(occurrences[s.end.value].parameter.hi() == analysis.providers().frame().parameterEnd());
        CHECK(occurrences[s.start.value].node.value == occurrences[s.end.value].node.value);
    } else {
        REQUIRE(analysis.freeEnds().size() == 2);
        for (const auto& f : analysis.freeEnds()) { validId(f.occurrence); }
    }
    checkEventNodes(analysis, tolerance);
}

struct EventFixture { RS_EllipseData source; double distance; std::size_t pairs, criticals; };
std::array<EventFixture, 6> eventFixtures() {
    return {{{basis(10.0, 0.5), 3.0, 0, 0}, {basis(10.0, 0.5), -1.0, 0, 0},
        {basis(100.0, 0.6), -36.036, 2, 4},
        {basis(10.0, 0.5, M_PI / 3, 2 * M_PI / 3), -2.0, 2, 0},
        {basis(100.0, 0.5, M_PI / 18, 35 * M_PI / 18), -49.5, 6, 4},
        {basis(100.0, 0.05, M_PI / 12, 7 * M_PI / 4), -4.0, 5, 2}}};
}
} // namespace

TEST_CASE("Reference event analysis is factory owned and diagnostic identities start invalid",
          "[lc-offsetcurve][geometry][reference-events]") {
    STATIC_REQUIRE_FALSE(std::is_default_constructible_v<geo::ReferenceEventAnalysis>);
    STATIC_REQUIRE_FALSE(std::is_copy_constructible_v<geo::ReferenceEventAnalysis>);
    CHECK(geo::ReferenceOccurrenceId{}.value == std::size_t(-1));
    CHECK(geo::ReferenceNodeId{}.value == std::size_t(-1));
    geo::ReferenceEventResult result;
    CHECK(result.status == LC_IntersectionStatus::InvalidInput);
    CHECK_FALSE(result.analysis);
}

TEST_CASE("Reference events enumerate and order all six required combined families with accurate world nodes",
          "[lc-offsetcurve][geometry][reference-events]") {
    const auto fixtures = eventFixtures();
    for (std::size_t i = 0; i < fixtures.size(); ++i) {
        CAPTURE(i);
        geo::WorkBudget budget;
        const double tolerance = 1e-5;
        const auto result = geo::analyseReferenceEvents(fixtures[i].source, fixtures[i].distance, tolerance, budget);
        CAPTURE(result.status, result.reason, result.work.vertices, result.work.distancePairs, result.work.deepest);
        REQUIRE(result.status == LC_IntersectionStatus::Ok);
        REQUIRE(result.analysis);
        CHECK(result.analysis->pairs().size() == fixtures[i].pairs);
        CHECK(result.analysis->criticals().size() == fixtures[i].criticals);
        checkEventRecords(*result.analysis, tolerance);
        checkWork(result.work, budget.consumed());
        CHECK(result.work.deepest <= budget.depthLimit());
    }
}

TEST_CASE("Reference combined records survive reversed rotated and representable translated families",
          "[lc-offsetcurve][geometry][reference-events]") {
    const auto fixtures = eventFixtures();
    for (std::size_t i = 0; i < fixtures.size(); ++i) {
        for (const bool reversed : {false, true}) {
            auto source = fixtures[i].source;
            source.majorP = {0.0, source.majorP.x};
            source.center = {1048576.0, -2097152.0};
            if (reversed) {
                std::swap(source.angle1, source.angle2);
                source.reversed = true;
            }
            CAPTURE(i, reversed);
            geo::WorkBudget budget;
            const auto result = geo::analyseReferenceEvents(source, fixtures[i].distance, 1e-5, budget);
            CAPTURE(result.status, result.reason);
            REQUIRE(result.status == LC_IntersectionStatus::Ok);
            REQUIRE(result.analysis);
            CHECK(result.analysis->pairs().size() == fixtures[i].pairs);
            CHECK(result.analysis->criticals().size() == fixtures[i].criticals);
            checkEventRecords(*result.analysis, 1e-5);
            checkWork(result.work, budget.consumed());
        }
    }
}

TEST_CASE("Reference seam shares a physical node without collapsing its two parameter occurrences",
          "[lc-offsetcurve][geometry][reference-events]") {
    const auto source = basis(5.0, 1.0);
    geo::WorkBudget budget;
    const auto result = geo::analyseReferenceEvents(source, 3.0, 1e-6, budget);
    REQUIRE(result.status == LC_IntersectionStatus::Ok);
    REQUIRE(result.analysis);
    CHECK(result.analysis->ownOccurrences().size() == 2);
    CHECK(result.analysis->nodes().size() == 1);
    CHECK(result.analysis->pairs().empty());
    checkEventRecords(*result.analysis, 1e-6);
    const auto& frame = result.analysis->providers().frame();
    const auto exact = referenceOracle(source, frame, 3.0, 0.0, true);
    const auto& seam = *result.analysis->seam();
    const auto& node = result.analysis->nodes()[result.analysis->occurrences()[seam.start.value].node.value];
    CHECK(abs(Quad(node.world.x) - (Quad(source.center.x) + Quad(frame.scale()) * exact[0])) <= Quad(1e-6) / 16);
    CHECK(abs(Quad(node.world.y) - (Quad(source.center.y) + Quad(frame.scale()) * exact[1])) <= Quad(1e-6) / 16);
}

TEST_CASE("Reference event analysis handles a wrapped source arc",
          "[lc-offsetcurve][geometry][reference-events]") {
    const auto source = basis(100.0, 0.5, 5.6, 0.7);
    const auto sourceSweep = geo::sweep(source);
    REQUIRE(sourceSweep);
    CHECK(sourceSweep->start == source.angle1);
    CHECK(sourceSweep->signedSweep > 0.0);
    CHECK(sourceSweep->signedSweep < 2.0 * M_PI);
    geo::WorkBudget budget;
    const auto result = geo::analyseReferenceEvents(source, -5.0, 1e-5, budget);
    CAPTURE(result.status, result.reason);
    REQUIRE(result.status == LC_IntersectionStatus::Ok);
    REQUIRE(result.analysis);
    REQUIRE_FALSE(result.analysis->seam());
    REQUIRE(result.analysis->freeEnds().size() == 2);
    checkEventRecords(*result.analysis, 1e-5);
}

TEST_CASE("Reference lens crossings retain two own occurrences per physical node independently of tolerance",
          "[lc-offsetcurve][geometry][reference-events]") {
    for (const double multiplier : {1.0, 1024.0}) {
        auto source = basis(100.0 * multiplier, 0.6);
        const double tolerance = 10.0 * multiplier;
        geo::WorkBudget budget;
        const auto result = geo::analyseReferenceEvents(source, -36.036 * multiplier, tolerance, budget);
        CAPTURE(multiplier, result.status, result.reason);
        REQUIRE(result.status == LC_IntersectionStatus::Ok);
        REQUIRE(result.analysis);
        const auto& a = *result.analysis;
        REQUIRE(a.pairs().size() == 2);
        REQUIRE(a.criticals().size() == 4);
        CHECK(a.localTolerance() == tolerance / a.providers().frame().scale());
        CHECK(a.localTolerance() != tolerance);
        const auto& p = a.pairs();
        CHECK(p[0].first.value != p[0].second.value);
        CHECK(p[1].first.value != p[1].second.value);
        CHECK(a.occurrences()[p[0].first.value].node.value != a.occurrences()[p[1].first.value].node.value);
        CHECK(a.ownOccurrences().size() == 10);
        for (const auto& critical : a.criticals()) {
            for (const auto& pair : a.pairs()) {
                CHECK(a.occurrences()[critical.occurrence.value].node.value !=
                      a.occurrences()[pair.first.value].node.value);
                CHECK(a.occurrences()[critical.occurrence.value].node.value !=
                      a.occurrences()[pair.second.value].node.value);
            }
        }
        checkEventRecords(a, tolerance);
    }
}

TEST_CASE("Reference tangent strict search stops at the shared depth limit without publishing partial events",
          "[lc-offsetcurve][geometry][reference-events]") {
    const auto source = basis(10.0, 0.5, 0.0, M_PI);
    geo::WorkBudget budget;
    const auto result = geo::analyseReferenceEvents(source, -10.0, 1e-5, budget);
    CAPTURE(result.status, result.reason, result.work.vertices, result.work.distancePairs, result.work.deepest);
    CHECK(result.status == LC_IntersectionStatus::LimitExceeded);
    CHECK(result.reason == geo::ReferenceEventReason::None);
    CHECK_FALSE(result.analysis);
    CHECK(result.work.deepest == budget.depthLimit());
    checkWork(result.work, budget.consumed());
}

TEST_CASE("Reference construction endpoints share occurrences while additional inner cap crossings remain distinct",
          "[lc-offsetcurve][geometry][reference-events]") {
    geo::WorkBudget budget;
    const auto result = geo::analyseReferenceEvents(basis(10.0, 0.5, 0.0, 1.0), -3.0, 1e-6, budget);
    CAPTURE(result.status, result.reason);
    REQUIRE(result.status == LC_IntersectionStatus::Ok);
    REQUIRE(result.analysis);
    const auto& a = *result.analysis;
    REQUIRE(a.freeEnds().size() == 2);
    std::size_t extra = 0, construction = 0;
    for (const auto& p : a.pairs()) {
        const auto& first = a.occurrences()[p.first.value];
        const auto& second = a.occurrences()[p.second.value];
        const auto own = first.role == geo::ReferenceRole::Own ? p.first : p.second;
        const auto cutter = first.role == geo::ReferenceRole::Own ? second.role : first.role;
        if (own.value == a.freeEnds()[0].occurrence.value || own.value == a.freeEnds()[1].occurrence.value) {
            ++construction;
            CHECK(p.proof == LC_IntersectionProof::AnalyticEndpoint);
        } else if (cutter == geo::ReferenceRole::StartCutter) {
            ++extra;
            CHECK(p.proof == LC_IntersectionProof::IntervalUniqueTransverse);
            const auto interval = a.occurrences()[own.value].parameter;
            CHECK(interval.lo() < std::acos(0.95));
            CHECK(interval.hi() > std::acos(0.96));
        }
    }
    CHECK(construction == 2);
    CHECK(extra == 1);
    checkEventRecords(a, 1e-6);
}

TEST_CASE("Reference critical stationary endpoints preserve both critical and free end records",
          "[lc-offsetcurve][geometry][reference-events]") {
    geo::WorkBudget budget;
    const auto result = geo::analyseReferenceEvents(basis(100.0, 0.5, 0.0, 1.0), -25.0, 1e-5, budget);
    CAPTURE(result.status, result.reason);
    REQUIRE(result.status == LC_IntersectionStatus::Ok);
    REQUIRE(result.analysis);
    const auto& a = *result.analysis;
    REQUIRE_FALSE(a.criticals().empty());
    REQUIRE(a.freeEnds().size() == 2);
    CHECK(a.criticals()[0].kind == geo::CriticalKind::CriticalStationary);
    CHECK(a.criticals()[0].occurrence.value == a.freeEnds()[0].occurrence.value);
    checkEventRecords(a, 1e-5);
}

TEST_CASE("Reference event limits are exact irreversible and never expose partial topology",
          "[lc-offsetcurve][geometry][reference-events]") {
    const auto source = basis(100.0, 0.6);
    geo::WorkBudget completeBudget;
    const auto complete = geo::analyseReferenceEvents(source, -36.036, 1e-5, completeBudget);
    REQUIRE(complete.status == LC_IntersectionStatus::Ok);
    const auto used = complete.work;
    REQUIRE(used.vertices > 1); REQUIRE(used.distancePairs > 1); REQUIRE(used.deepest > 0);
    for (const unsigned dimension : {0u, 1u, 2u}) {
        for (const bool oneLess : {false, true}) {
            LC_OffsetCheckLimits limits;
            if (dimension == 0) { limits.maxVertices = used.vertices - (oneLess ? 1 : 0); }
            if (dimension == 1) { limits.maxDistancePairs = used.distancePairs - (oneLess ? 1 : 0); }
            if (dimension == 2) { limits.maxDepth = used.deepest - (oneLess ? 1 : 0); }
            geo::WorkBudget budget{limits};
            const auto result = geo::analyseReferenceEvents(source, -36.036, 1e-5, budget);
            CAPTURE(dimension, oneLess, result.status, result.reason);
            checkWork(result.work, budget.consumed());
            if (!oneLess) {
                REQUIRE(result.status == LC_IntersectionStatus::Ok);
                REQUIRE(result.analysis);
                checkEventRecords(*result.analysis, 1e-5);
            } else {
                CHECK(result.status == LC_IntersectionStatus::LimitExceeded);
                CHECK_FALSE(result.analysis);
                const auto spent = budget.consumed();
                const auto again = geo::analyseReferenceEvents(source, -36.036, 1e-5, budget);
                CHECK(again.status == LC_IntersectionStatus::LimitExceeded);
                CHECK_FALSE(again.analysis);
                checkWork(again.work, spent);
            }
        }
    }
}

TEST_CASE("Reference publication charges honor pre spent and failed metadata boundaries",
          "[lc-offsetcurve][geometry][reference-events]") {
    geo::WorkBudget baseline;
    const auto complete = geo::analyseReferenceEvents(basis(), 3.0, 1e-6, baseline);
    REQUIRE(complete.status == LC_IntersectionStatus::Ok);
    for (const bool enough : {false, true}) {
        LC_OffsetCheckLimits limits;
        limits.maxVertices = complete.work.vertices + (enough ? 1 : 0);
        limits.maxDistancePairs = complete.work.distancePairs + 1;
        geo::WorkBudget budget{limits};
        REQUIRE(budget.consumeVertices(1)); REQUIRE(budget.consumePairs(1));
        const auto result = geo::analyseReferenceEvents(basis(), 3.0, 1e-6, budget);
        CHECK(result.status == (enough ? LC_IntersectionStatus::Ok : LC_IntersectionStatus::LimitExceeded));
        CHECK(bool(result.analysis) == enough);
        checkWork(result.work, budget.consumed());
        CHECK(result.work.vertices <= limits.maxVertices);
        CHECK(result.work.distancePairs <= limits.maxDistancePairs);
    }
}

TEST_CASE("Reference events retain their owned provider after input and budget lifetimes end",
          "[lc-offsetcurve][geometry][reference-events]") {
    geo::ReferenceEventResult retained;
    {
        auto source = basis(100.0, 0.6);
        geo::WorkBudget budget;
        retained = geo::analyseReferenceEvents(source, -36.036, 1e-5, budget);
        REQUIRE(retained.status == LC_IntersectionStatus::Ok);
        source.majorP = {0.0, 0.0};
        source.center = {std::numeric_limits<double>::infinity(), 0.0};
    }
    REQUIRE(retained.analysis);
    auto copy = retained.analysis->occurrences();
    copy[0].node.value = std::size_t(-1);
    copy[0].parameter = {};
    CHECK(retained.analysis->occurrences()[0].node.value != std::size_t(-1));
    const auto& segments = retained.analysis->providers().segments();
    REQUIRE_FALSE(segments.empty());
    LC_CurveJetBounds jet;
    REQUIRE(retained.analysis->providers().boundJet(0, segments[0].t0, segments[0].t1, jet));
    checkEventRecords(*retained.analysis, 1e-5);
}

TEST_CASE("Reference event world rounding and unavailable conversion refuse rather than accepting a coarse node",
          "[lc-offsetcurve][geometry][reference-events]") {
    auto source = basis();
    source.center = {1e200, -1e200};
    geo::WorkBudget budget;
    const auto result = geo::analyseReferenceEvents(source, 3.0, 1e-6, budget);
    CHECK(result.status == LC_IntersectionStatus::AmbiguousTopology);
    CHECK(result.reason == geo::ReferenceEventReason::NodeAccuracy);
    CHECK_FALSE(result.analysis);
    checkWork(result.work, budget.consumed());
    REQUIRE(result.work.vertices > 0);
}

TEST_CASE("Reference event subnormal world bounds use a finite in interval rounded node",
          "[lc-offsetcurve][geometry][reference-events]") {
    const double tiny = std::numeric_limits<double>::denorm_min();
    auto source = basis(tiny, 1.0);
    source.center = {tiny, tiny};
    geo::WorkBudget budget;
    const auto result = geo::analyseReferenceEvents(source, tiny, 1e-300, budget);
    CAPTURE(result.status, result.reason);
    REQUIRE(result.status == LC_IntersectionStatus::Ok);
    REQUIRE(result.analysis);
    checkEventRecords(*result.analysis, 1e-300);
    for (const auto& n : result.analysis->nodes()) {
        CHECK(std::isfinite(n.world.x)); CHECK(std::isfinite(n.world.y));
    }
}

TEST_CASE("Reference event invalid collapsed and sticky limit requests publish no analysis",
          "[lc-offsetcurve][geometry][reference-events]") {
    for (const double tolerance : {0.0, -1.0, std::numeric_limits<double>::quiet_NaN(),
                                   std::numeric_limits<double>::infinity()}) {
        geo::WorkBudget budget;
        const auto result = geo::analyseReferenceEvents(basis(), 1.0, tolerance, budget);
        CHECK(result.status == LC_IntersectionStatus::InvalidInput);
        CHECK_FALSE(result.analysis);
        checkWork(result.work, {});
    }
    geo::WorkBudget budget;
    const auto collapsed = geo::analyseReferenceEvents(basis(5.0, 1.0), -5.0, 1e-6, budget);
    CHECK(collapsed.status == LC_IntersectionStatus::AmbiguousTopology);
    CHECK(collapsed.reason == geo::ReferenceEventReason::CollapsedCircle);
    CHECK_FALSE(collapsed.analysis);
    geo::WorkBudget limited{LC_OffsetCheckLimits{1, 1, 1, 20}};
    REQUIRE(limited.consumeVertices(1));
    const auto refused = geo::analyseReferenceEvents(basis(), 1.0, 1e-6, limited);
    CHECK(refused.status == LC_IntersectionStatus::LimitExceeded);
    CHECK_FALSE(refused.analysis);
    checkWork(refused.work, limited.consumed());

    LC_OffsetCheckLimits preparationLimits;
    preparationLimits.maxVertices = 1;
    geo::WorkBudget preparationBudget{preparationLimits};
    const auto preparationLimited = geo::analyseReferenceEvents(basis(), 1.0, 1e-6, preparationBudget);
    CHECK(preparationLimited.status == LC_IntersectionStatus::LimitExceeded);
    CHECK(preparationLimited.reason == geo::ReferenceEventReason::None);
    CHECK_FALSE(preparationLimited.analysis);
    checkWork(preparationLimited.work, preparationBudget.consumed());
}

namespace {
bool componentVisibilityOracle(const RS_EllipseData& source, const double distance,
                               const geo::ReferenceFrame& frame, const double t) {
    if (distance > 0) { return true; }
    const CriticalFloat pi = boost::math::constants::pi<CriticalFloat>();
    const CriticalFloat period = 2 * pi;
    const bool whole = frame.sourceSweep().whole;
    const CriticalFloat start = whole ? CriticalFloat(M_PI_4) : CriticalFloat(source.angle1);
    CriticalFloat extent = whole ? (source.reversed ? -period : period) :
        CriticalFloat(source.angle2) - CriticalFloat(source.angle1);
    if (!whole && source.reversed && extent > 0) { extent -= period; }
    if (!whole && !source.reversed && extent < 0) { extent += period; }
    const CriticalFloat theta = start + extent * CriticalFloat(t) / CriticalFloat(frame.parameterEnd());
    const CriticalFloat finish = start + extent;
    const CriticalFloat a = std::min(start, finish), b = std::max(start, finish);
    const CriticalFloat scale = frame.scale();
    const CriticalFloat mx = CriticalFloat(source.majorP.x) / scale, my = CriticalFloat(source.majorP.y) / scale;
    const CriticalFloat r = source.ratio, d = CriticalFloat(-distance) / scale;
    const CriticalFloat c = cos(theta), s = sin(theta), norm = sqrt(mx * mx + my * my);
    const CriticalFloat h = sqrt(r * r * c * c + s * s);
    const CriticalFloat qx = mx * c - my * r * s - d * (r * mx * c - my * s) / (norm * h);
    const CriticalFloat qy = my * c + mx * r * s - d * (r * my * c + mx * s) / (norm * h);
    const auto nearer = [&](const CriticalFloat& phi) {
        const CriticalFloat dx = qx - (mx * cos(phi) - my * r * sin(phi));
        const CriticalFloat dy = qy - (my * cos(phi) + mx * r * sin(phi));
        return dx * dx + dy * dy < d * d - CriticalFloat("1e-100") * (1 + d * d);
    };
    // Independent vector distances, not the product predicate used in production.
    if (nearer(a) || nearer(b)) { return false; }
    for (int k = -4; k <= 6; ++k) {
        const CriticalFloat mirror = 2 * k * pi - theta;
        if (a <= mirror && mirror <= b && nearer(mirror)) { return false; }
    }
    for (unsigned i = 1; i < 64; ++i) {
        if (nearer(a + (b - a) * i / 64)) { return false; }
    }
    return true;
}

void checkComponentAnalysis(const geo::ReferenceComponentAnalysis& analysis, const RS_EllipseData& source,
                            const double distance, const double tolerance) {
    const auto& events = analysis.events();
    checkEventRecords(events, tolerance);
    const auto& own = events.ownOccurrences();
    const auto& occurrences = events.occurrences();
    REQUIRE(analysis.spans().size() + 1 == own.size());
    REQUIRE(analysis.fringes().size() == own.size());
    std::vector<unsigned> fringes(occurrences.size()), retained(analysis.spans().size());
    const EventRational maximum = EventRational(tolerance) * EventRational(tolerance) / 256;
    for (const auto& fringe : analysis.fringes()) {
        REQUIRE(fringe.occurrence.value < occurrences.size());
        const auto& o = occurrences[fringe.occurrence.value];
        CHECK(o.role == geo::ReferenceRole::Own);
        ++fringes[fringe.occurrence.value];
        CHECK(fringe.parameter.lo() == o.parameter.lo());
        CHECK(fringe.parameter.hi() == o.parameter.hi());
        const auto& node = events.nodes()[o.node.value];
        const auto error = [](const LC_Interval& v, const double point) -> EventRational {
            REQUIRE(v.isValid()); REQUIRE(std::isfinite(v.lo())); REQUIRE(std::isfinite(v.hi()));
            const EventRational p(point);
            const EventRational a = p - EventRational(v.lo()), b = EventRational(v.hi()) - p;
            return std::max(EventRational{abs(a)}, EventRational{abs(b)});
        };
        const EventRational x = error(fringe.worldX, node.world.x), y = error(fringe.worldY, node.world.y);
        CHECK(x * x + y * y <= maximum);
        for (unsigned i = 0; i <= 8; ++i) {
            const double t = fringe.parameter.lo() + (fringe.parameter.hi() - fringe.parameter.lo()) * double(i) / 8;
            const auto oracle = referenceOracle(source, events.providers().frame(), distance, t, true);
            checkEnclosure(fringe.localX, oracle[0]); checkEnclosure(fringe.localY, oracle[1]);
        }
    }
    for (const auto id : own) { CHECK(fringes[id.value] == 1); }
    for (std::size_t i = 0; i < analysis.spans().size(); ++i) {
        const auto& span = analysis.spans()[i];
        CHECK(span.start.value == own[i].value); CHECK(span.end.value == own[i + 1].value);
        CHECK(span.interior.lo() == occurrences[span.start.value].parameter.hi());
        CHECK(span.interior.hi() == occurrences[span.end.value].parameter.lo());
        REQUIRE(span.interior.lo() < span.interior.hi());
        for (const double part : {0.125, 0.5, 0.875}) {
            const double t = span.interior.lo() + part * (span.interior.hi() - span.interior.lo());
            CAPTURE(i, part, t);
            CHECK((span.visibility == geo::ReferenceVisibility::Visible) ==
                componentVisibilityOracle(source, distance, events.providers().frame(), t));
        }
        CHECK(span.clearance.isValid());
        CHECK(span.clearance.hi() == std::abs(distance));
        CHECK(span.clearance.lo() == (span.visibility == geo::ReferenceVisibility::Visible ? std::abs(distance) : 0.0));
    }
    std::vector<unsigned> incoming(analysis.edges().size()), outgoing(analysis.edges().size());
    for (std::size_t e = 0; e < analysis.edges().size(); ++e) {
        const auto& edge = analysis.edges()[e];
        REQUIRE(edge.span < analysis.spans().size());
        const auto& span = analysis.spans()[edge.span];
        CHECK(span.visibility == geo::ReferenceVisibility::Visible);
        CHECK(edge.start.value == occurrences[span.start.value].node.value);
        CHECK(edge.end.value == occurrences[span.end.value].node.value);
        ++retained[edge.span];
    }
    for (std::size_t s = 0; s < retained.size(); ++s) {
        CHECK(retained[s] == (analysis.spans()[s].visibility == geo::ReferenceVisibility::Visible ? 1u : 0u));
    }
    REQUIRE(analysis.incidence().size() == events.nodes().size());
    std::vector<std::size_t> expectedIncoming(events.nodes().size()), expectedOutgoing(events.nodes().size());
    std::vector<bool> sourceStart(events.nodes().size()), sourceEnd(events.nodes().size());
    std::vector<bool> cusp(events.nodes().size()), stationary(events.nodes().size());
    std::vector<bool> crossing(events.nodes().size()), tangency(events.nodes().size()), construction(events.nodes().size());
    std::vector<bool> seam(events.nodes().size());
    for (const auto& freeEnd : events.freeEnds()) {
        const auto node = occurrences[freeEnd.occurrence.value].node.value;
        (freeEnd.atStart ? sourceStart[node] : sourceEnd[node]) = true;
    }
    for (const auto& critical : events.criticals()) {
        const auto node = occurrences[critical.occurrence.value].node.value;
        (critical.kind == geo::CriticalKind::SimpleCusp ? cusp[node] : stationary[node]) = true;
    }
    for (const auto& pair : events.pairs()) {
        const auto node = occurrences[pair.first.value].node.value;
        if (pair.kind == LC_ParametricIntersection::Kind::Transverse) { crossing[node] = true; }
        else if (pair.kind == LC_ParametricIntersection::Kind::Tangent) { tangency[node] = true; }
        else { construction[node] = true; }
    }
    if (events.seam()) { seam[occurrences[events.seam()->start.value].node.value] = true; }
    for (const auto& edge : analysis.edges()) {
        ++expectedOutgoing[edge.start.value]; ++expectedIncoming[edge.end.value];
    }
    for (std::size_t n = 0; n < analysis.incidence().size(); ++n) {
        const auto& row = analysis.incidence()[n];
        CHECK(row.node.value == n);
        for (const auto e : row.incoming) {
            REQUIRE(e < incoming.size()); ++incoming[e]; CHECK(analysis.edges()[e].end.value == n);
        }
        for (const auto e : row.outgoing) {
            REQUIRE(e < outgoing.size()); ++outgoing[e]; CHECK(analysis.edges()[e].start.value == n);
        }
        CHECK(row.sourceStart == sourceStart[n]); CHECK(row.sourceEnd == sourceEnd[n]);
        CHECK(row.cusp == cusp[n]); CHECK(row.stationary == stationary[n]);
        CHECK(row.crossing == crossing[n]); CHECK(row.tangency == tangency[n]);
        CHECK(row.construction == construction[n]); CHECK(row.seam == seam[n]);
        const bool hasRetainedEdge = expectedIncoming[n] || expectedOutgoing[n];
        const bool hasOwn = std::any_of(own.begin(), own.end(), [&](const auto id) {
            return occurrences[id.value].node.value == n;
        });
        CHECK(row.hiddenPoint == (hasOwn && !hasRetainedEdge));
        if (hasOwn && !hasRetainedEdge) {
            for (const auto id : own) {
                const auto& o = occurrences[id.value];
                if (o.node.value != n) { continue; }
                const double t = o.parameter.lo() + (o.parameter.hi() - o.parameter.lo()) / 2;
                CHECK_FALSE(componentVisibilityOracle(source, distance, events.providers().frame(), t));
            }
        }
        const bool expectedTrim = hasRetainedEdge && (!expectedIncoming[n] || !expectedOutgoing[n]) &&
            !sourceStart[n] && !sourceEnd[n];
        CHECK(row.trimBoundary == expectedTrim);
    }
    std::vector<unsigned> membership(analysis.edges().size());
    std::vector<std::size_t> nodeComponent(events.nodes().size(), std::size_t(-1));
    std::size_t componentIndex = 0;
    for (const auto& component : analysis.components()) {
        REQUIRE_FALSE(component.edges.empty());
        std::size_t previous = std::size_t(-1);
        std::vector<bool> used(events.nodes().size());
        for (const auto e : component.edges) {
            REQUIRE(e < membership.size()); ++membership[e];
            if (previous != std::size_t(-1)) { CHECK(previous < e); }
            previous = e;
            used[analysis.edges()[e].start.value] = used[analysis.edges()[e].end.value] = true;
        }
        bool endpointFree = true, branching = false;
        std::vector<bool> reachable(events.nodes().size());
        reachable[analysis.edges()[component.edges[0]].start.value] = true;
        bool changed = true;
        while (changed) {
            changed = false;
            for (const auto e : component.edges) {
                const auto& edge = analysis.edges()[e];
                if (reachable[edge.start.value] != reachable[edge.end.value]) {
                    reachable[edge.start.value] = reachable[edge.end.value] = true;
                    changed = true;
                }
            }
        }
        for (std::size_t n = 0; n < used.size(); ++n) {
            if (!used[n]) { continue; }
            CHECK(reachable[n]);
            REQUIRE(nodeComponent[n] == std::size_t(-1));
            nodeComponent[n] = componentIndex;
            const auto& row = analysis.incidence()[n];
            endpointFree = endpointFree && !row.incoming.empty() && !row.outgoing.empty();
            branching = branching || row.incoming.size() > 1 || row.outgoing.size() > 1;
        }
        CHECK(component.endpointFree == endpointFree); CHECK(component.branching == branching);
        ++componentIndex;
    }
    for (std::size_t e = 0; e < membership.size(); ++e) {
        CHECK(membership[e] == 1); CHECK(incoming[e] == 1); CHECK(outgoing[e] == 1);
    }
}
} // namespace

TEST_CASE("Reference components expose an immutable exact graph rather than a fabricated crossing walk",
          "[lc-offsetcurve][geometry][reference-components]") {
    STATIC_REQUIRE_FALSE(std::is_default_constructible_v<geo::ReferenceComponentAnalysis>);
    STATIC_REQUIRE_FALSE(std::is_copy_constructible_v<geo::ReferenceComponentAnalysis>);
    geo::ReferenceComponentResult result;
    CHECK(result.status == LC_IntersectionStatus::InvalidInput);
    CHECK_FALSE(result.analysis);
    CHECK(geo::ReferenceEdge{}.span == std::size_t(-1));
}

TEST_CASE("Reference components retain the six exact mathematical fixture outcomes and every fringe",
          "[lc-offsetcurve][geometry][reference-components]") {
    const auto fixtures = eventFixtures();
    bool sourceStart = false, sourceEnd = false, cusp = false, stationary = false;
    bool crossing = false, tangency = false, construction = false, trimBoundary = false;
    std::size_t hiddenOwnNodes = 0;
    for (std::size_t i = 0; i < fixtures.size(); ++i) {
        CAPTURE(i);
        geo::WorkBudget budget;
        const auto result = geo::analyseReferenceComponents(fixtures[i].source, fixtures[i].distance, 1e-5, budget);
        CAPTURE(result.status, result.reason, result.eventReason, result.work.vertices,
                result.work.distancePairs, result.work.deepest);
        REQUIRE(result.status == LC_IntersectionStatus::Ok);
        REQUIRE(result.analysis);
        const auto& a = *result.analysis;
        REQUIRE(a.components().size() == (i == 5 ? 2 : 1));
        std::size_t open = 0;
        for (const auto& c : a.components()) { open += !c.endpointFree; CHECK_FALSE(c.branching); }
        CHECK(open == (i == 3 || i == 5 ? 1 : 0));
        CHECK(a.events().pairs().size() == fixtures[i].pairs);
        checkComponentAnalysis(a, fixtures[i].source, fixtures[i].distance, 1e-5);
        checkWork(result.work, budget.consumed());
        for (const auto& row : a.incidence()) {
            sourceStart = sourceStart || row.sourceStart; sourceEnd = sourceEnd || row.sourceEnd;
            cusp = cusp || row.cusp;
            stationary = stationary || row.stationary;
            crossing = crossing || row.crossing; tangency = tangency || row.tangency;
            construction = construction || row.construction; trimBoundary = trimBoundary || row.trimBoundary;
            hiddenOwnNodes += row.hiddenPoint;
        }
    }
    CHECK(sourceStart); CHECK(sourceEnd); CHECK(cusp);
    CHECK(crossing); CHECK(construction); CHECK(trimBoundary);
    // None of these six distances is an exact curvature threshold or a tangent contact.
    CHECK_FALSE(stationary); CHECK_FALSE(tangency);
    CHECK(hiddenOwnNodes > 0);
}

TEST_CASE("Reference exact graphs preserve rotated reversed and representably translated nontrivial families",
          "[lc-offsetcurve][geometry][reference-components]") {
    const auto fixtures = eventFixtures();
    for (std::size_t i = 2; i < fixtures.size(); ++i) {
        for (const bool reversed : {false, true}) {
            auto data = fixtures[i].source;
            data.majorP = {0.0, data.majorP.x}; data.center = {4096.0, -8192.0};
            if (reversed) { std::swap(data.angle1, data.angle2); data.reversed = true; }
            CAPTURE(i, reversed);
            geo::WorkBudget budget;
            const auto result = geo::analyseReferenceComponents(data, fixtures[i].distance, 1e-5, budget);
            CAPTURE(result.status, result.reason, result.eventReason);
            REQUIRE(result.status == LC_IntersectionStatus::Ok); REQUIRE(result.analysis);
            CHECK(result.analysis->components().size() == (i == 5 ? 2 : 1));
            checkComponentAnalysis(*result.analysis, data, fixtures[i].distance, 1e-5);
            checkWork(result.work, budget.consumed());
        }
    }
}

TEST_CASE("Reference whole seam is an explicit two occurrence incidence on one physical node",
          "[lc-offsetcurve][geometry][reference-components]") {
    geo::WorkBudget budget;
    const auto result = geo::analyseReferenceComponents(basis(), 3.0, 1e-6, budget);
    REQUIRE(result.status == LC_IntersectionStatus::Ok); REQUIRE(result.analysis);
    const auto& a = *result.analysis;
    REQUIRE(a.events().seam());
    CHECK(a.events().seam()->start.value != a.events().seam()->end.value);
    REQUIRE(a.edges().size() == 1);
    CHECK(a.edges()[0].start.value == a.edges()[0].end.value);
    const auto& n = a.incidence()[a.edges()[0].start.value];
    CHECK(n.seam); CHECK(n.incoming.size() == 1); CHECK(n.outgoing.size() == 1);
    CHECK_FALSE(n.trimBoundary);
    checkComponentAnalysis(a, basis(), 3.0, 1e-6);
}

TEST_CASE("Reference restricted source visibility does not use the supporting whole ellipse",
          "[lc-offsetcurve][geometry][reference-components]") {
    const auto data = basis(100.0, 0.05, M_PI / 12, 7 * M_PI / 4);
    geo::WorkBudget budget;
    const auto result = geo::analyseReferenceComponents(data, -4.0, 1e-5, budget);
    REQUIRE(result.status == LC_IntersectionStatus::Ok); REQUIRE(result.analysis);
    REQUIRE(result.analysis->components().size() == 2);
    bool restrictedOnly = false;
    const double r = data.ratio;
    for (const auto& span : result.analysis->spans()) {
        if (span.visibility != geo::ReferenceVisibility::Visible) { continue; }
        const double t = span.interior.lo() + (span.interior.hi() - span.interior.lo()) / 2;
        const Quad theta = Quad(data.angle1) + idealDirectedSweep(data, false) * Quad(t) /
            Quad(result.analysis->events().providers().frame().parameterEnd());
        const Quad h = sqrt(Quad(r * r) * cos(theta) * cos(theta) + sin(theta) * sin(theta));
        if (Quad(4) > Quad(100) * Quad(r) * h) { restrictedOnly = true; }
    }
    CHECK(restrictedOnly);
    checkComponentAnalysis(*result.analysis, data, -4.0, 1e-5);
}

TEST_CASE("Reference component analysis refuses an isolated whole-source equality point",
          "[lc-offsetcurve][geometry][reference-components]") {
    const auto data = basis(10.0, 0.5);
    geo::WorkBudget budget;
    const auto result = geo::analyseReferenceComponents(data, -5.0, 1e-5, budget);
    CAPTURE(result.status, result.reason, result.eventReason, result.work.deepest);
    CHECK(result.status == LC_IntersectionStatus::AmbiguousTopology);
    CHECK(result.reason == geo::ReferenceComponentReason::UnresolvedVisibility);
    CHECK_FALSE(result.analysis);
    checkWork(result.work, budget.consumed());
}

TEST_CASE("Reference exact threshold stationary incidence survives without epsilon visibility",
          "[lc-offsetcurve][geometry][reference-components]") {
    for (const double d : {std::nextafter(25.0, 0.0), 25.0}) {
        geo::WorkBudget budget;
        const auto result = geo::analyseReferenceComponents(basis(100.0, 0.5, 0.0, 1.0), -d, 1e-5, budget);
        CAPTURE(d, result.status, result.reason, result.eventReason);
        REQUIRE(result.status == LC_IntersectionStatus::Ok); REQUIRE(result.analysis);
        CHECK(result.analysis->components().size() == 1);
        if (d == 25.0) {
            REQUIRE_FALSE(result.analysis->events().criticals().empty());
            const auto id = result.analysis->events().criticals()[0].occurrence;
            const auto node = result.analysis->events().occurrences()[id.value].node;
            CHECK(result.analysis->incidence()[node.value].stationary);
            CHECK(result.analysis->incidence()[node.value].sourceStart);
        }
        checkComponentAnalysis(*result.analysis, basis(100.0, 0.5, 0.0, 1.0), -d, 1e-5);
        checkWork(result.work, budget.consumed());
    }

    geo::WorkBudget budget;
    const double justBeyond = std::nextafter(25.0, 30.0);
    const auto refused = geo::analyseReferenceComponents(basis(100.0, 0.5, 0.0, 1.0), -justBeyond, 1e-5, budget);
    CAPTURE(justBeyond, refused.status, refused.reason, refused.eventReason, refused.work.deepest);
    CHECK(refused.status == LC_IntersectionStatus::LimitExceeded);
    CHECK_FALSE(refused.analysis);
    CHECK(refused.work.deepest == budget.depthLimit());
    checkWork(refused.work, budget.consumed());
}

TEST_CASE("Reference component shared own occurrence retains every centre tangency pair",
          "[lc-offsetcurve][geometry][reference-components]") {
    const auto data = basis(10.0, 0.5, 0.0, 4.0);
    geo::WorkBudget budget;
    const auto result = geo::analyseReferenceComponents(data, -10.0, 1e-5, budget);
    CAPTURE(result.status, result.reason, result.eventReason, result.work.deepest);
    REQUIRE(result.status == LC_IntersectionStatus::Ok); REQUIRE(result.analysis);
    std::vector<unsigned> pairCount(result.analysis->events().occurrences().size());
    for (const auto& pair : result.analysis->events().pairs()) {
        ++pairCount[pair.first.value]; ++pairCount[pair.second.value];
    }
    bool shared = false;
    for (const auto id : result.analysis->events().ownOccurrences()) { shared = shared || pairCount[id.value] > 1; }
    CHECK(shared);
    const auto& events = result.analysis->events();
    const auto start = std::find_if(events.freeEnds().begin(), events.freeEnds().end(), [](const auto& end) {
        return end.atStart;
    });
    REQUIRE(start != events.freeEnds().end());
    const auto centre = events.occurrences()[start->occurrence.value].node;
    std::size_t centrePairs = 0, tangencies = 0, centreOwn = 0;
    for (const auto& pair : events.pairs()) {
        if (events.occurrences()[pair.first.value].node.value != centre.value) { continue; }
        ++centrePairs; tangencies += pair.kind == LC_ParametricIntersection::Kind::Tangent;
        CHECK(events.occurrences()[pair.second.value].node.value == centre.value);
    }
    for (const auto id : events.ownOccurrences()) {
        const auto& occurrence = events.occurrences()[id.value];
        if (occurrence.node.value != centre.value) { continue; }
        ++centreOwn;
        if (id.value != start->occurrence.value) {
            checkEnclosure(occurrence.parameter, boost::math::constants::pi<Quad>());
            CHECK(occurrence.parameter.lo() > 3.0); // Not the same parameter as the centre at t=0.
        }
    }
    CHECK(centrePairs == 3); CHECK(tangencies == 2); CHECK(centreOwn == 2);
    CHECK(result.analysis->incidence()[centre.value].hiddenPoint);
    checkComponentAnalysis(*result.analysis, data, -10.0, 1e-5);
}

TEST_CASE("Reference whole major radius contact preserves the seam apart from its centre pair",
          "[lc-offsetcurve][geometry][reference-components]") {
    const auto data = basis(10.0, 0.5);
    geo::WorkBudget budget;
    const auto result = geo::analyseReferenceComponents(data, -10.0, 1e-5, budget);
    CAPTURE(result.status, result.reason, result.eventReason, result.work.deepest);
    REQUIRE(result.status == LC_IntersectionStatus::Ok); REQUIRE(result.analysis);
    const auto& events = result.analysis->events();
    REQUIRE(events.seam()); REQUIRE(events.pairs().size() == 1);
    const auto& pair = events.pairs()[0];
    REQUIRE(pair.kind == LC_ParametricIntersection::Kind::Tangent);
    const auto& a = events.occurrences()[pair.first.value];
    const auto& b = events.occurrences()[pair.second.value];
    CHECK(pair.first.value != pair.second.value); CHECK(a.node.value == b.node.value);
    CHECK(a.parameter.hi() < b.parameter.lo());
    CHECK(events.occurrences()[events.seam()->start.value].node.value != a.node.value);
    CHECK(result.analysis->incidence()[a.node.value].hiddenPoint);
    checkComponentAnalysis(*result.analysis, data, -10.0, 1e-5);
}

TEST_CASE("Reference component caps are shared exact and irreversible across every stage",
          "[lc-offsetcurve][geometry][reference-components]") {
    const auto data = basis(100.0, 0.6);
    geo::WorkBudget baseline;
    const auto complete = geo::analyseReferenceComponents(data, -36.036, 1e-5, baseline);
    REQUIRE(complete.status == LC_IntersectionStatus::Ok); REQUIRE(complete.analysis);
    const auto used = complete.work;
    REQUIRE(used.vertices > 0); REQUIRE(used.distancePairs > 0); REQUIRE(used.deepest > 1);
    for (unsigned dimension = 0; dimension < 3; ++dimension) {
        for (const bool less : {false, true}) {
            LC_OffsetCheckLimits limits;
            if (dimension == 0) { limits.maxVertices = used.vertices - (less ? 1 : 0); }
            if (dimension == 1) { limits.maxDistancePairs = used.distancePairs - (less ? 1 : 0); }
            if (dimension == 2) { limits.maxDepth = used.deepest - (less ? 1 : 0); }
            geo::WorkBudget budget{limits};
            const auto result = geo::analyseReferenceComponents(data, -36.036, 1e-5, budget);
            CAPTURE(dimension, less, result.status, result.reason, result.eventReason);
            CHECK(result.status == (less ? LC_IntersectionStatus::LimitExceeded : LC_IntersectionStatus::Ok));
            CHECK(bool(result.analysis) == !less);
            checkWork(result.work, budget.consumed());
            CHECK(result.work.vertices <= limits.maxVertices); CHECK(result.work.distancePairs <= limits.maxDistancePairs);
            if (less) {
                const auto spent = budget.consumed();
                const auto again = geo::analyseReferenceComponents(data, -36.036, 1e-5, budget);
                CHECK(again.status == LC_IntersectionStatus::LimitExceeded); CHECK_FALSE(again.analysis);
                checkWork(again.work, spent);
            }
        }
    }
}

TEST_CASE("Reference components count pre spent work once and preserve owned geometry lifetime",
          "[lc-offsetcurve][geometry][reference-components]") {
    geo::WorkBudget baseline;
    const auto complete = geo::analyseReferenceComponents(basis(), 3.0, 1e-6, baseline);
    REQUIRE(complete.status == LC_IntersectionStatus::Ok);
    for (const bool enough : {false, true}) {
        LC_OffsetCheckLimits limits;
        limits.maxVertices = complete.work.vertices + (enough ? 1 : 0);
        limits.maxDistancePairs = complete.work.distancePairs + 1;
        geo::WorkBudget budget{limits};
        REQUIRE(budget.consumeVertices(1)); REQUIRE(budget.consumePairs(1));
        const auto result = geo::analyseReferenceComponents(basis(), 3.0, 1e-6, budget);
        CHECK(result.status == (enough ? LC_IntersectionStatus::Ok : LC_IntersectionStatus::LimitExceeded));
        CHECK(bool(result.analysis) == enough); checkWork(result.work, budget.consumed());
    }
    geo::ReferenceComponentResult retained;
    {
        auto data = basis(); geo::WorkBudget budget;
        retained = geo::analyseReferenceComponents(data, 3.0, 1e-6, budget);
        REQUIRE(retained.status == LC_IntersectionStatus::Ok);
        data.majorP = {0.0, 0.0};
    }
    REQUIRE(retained.analysis);
    auto diagnostic = retained.analysis->spans(); diagnostic[0].start.value = std::size_t(-1);
    CHECK(retained.analysis->spans()[0].start.value != std::size_t(-1));
    checkComponentAnalysis(*retained.analysis, basis(), 3.0, 1e-6);
}

TEST_CASE("Reference component invalid collapsed and unrepresentable requests expose no partial graph",
          "[lc-offsetcurve][geometry][reference-components]") {
    for (const double tolerance : {0.0, -1.0, std::numeric_limits<double>::quiet_NaN()}) {
        geo::WorkBudget budget;
        const auto result = geo::analyseReferenceComponents(basis(), 3.0, tolerance, budget);
        CHECK(result.status == LC_IntersectionStatus::InvalidInput); CHECK_FALSE(result.analysis);
        checkWork(result.work, {});
    }
    for (const double distance : {0.0, -0.0, std::numeric_limits<double>::infinity(),
                                  -std::numeric_limits<double>::infinity(),
                                  std::numeric_limits<double>::quiet_NaN()}) {
        geo::WorkBudget componentBudget;
        const auto components = geo::analyseReferenceComponents(basis(10, 0.5), distance, 1e-6, componentBudget);
        CHECK(components.status == LC_IntersectionStatus::InvalidInput); CHECK_FALSE(components.analysis);
        checkWork(components.work, {});

        geo::WorkBudget eventBudget;
        const auto events = geo::analyseReferenceEvents(basis(10, 0.5), distance, 1e-6, eventBudget);
        CHECK(events.status == LC_IntersectionStatus::InvalidInput); CHECK_FALSE(events.analysis);
        checkWork(events.work, {});
    }
    geo::WorkBudget collapsedBudget;
    const auto collapsed = geo::analyseReferenceComponents(basis(5, 1), -5, 1e-6, collapsedBudget);
    CHECK(collapsed.status == LC_IntersectionStatus::AmbiguousTopology); CHECK_FALSE(collapsed.analysis);
    CHECK(collapsed.eventReason == geo::ReferenceEventReason::CollapsedCircle);
    auto huge = basis(); huge.center = {1e200, -1e200};
    geo::WorkBudget budget;
    const auto refused = geo::analyseReferenceComponents(huge, 3.0, 1e-6, budget);
    CHECK(refused.status == LC_IntersectionStatus::AmbiguousTopology); CHECK_FALSE(refused.analysis);
    checkWork(refused.work, budget.consumed());
}

namespace {
// The two nonrepresentable roots share an exact algebraic parameter on branch zero.
// No spatial epsilon, floating seed equality, or overlap alone proves that identity.
class SharedAlgebraicOccurrence final : public LC_ParametricCurves {
public:
    enum class Evidence { Valid, Unavailable, WrongSegment, EmptyDomain, WrongParameter };
    explicit SharedAlgebraicOccurrence(Evidence evidence, std::shared_ptr<unsigned> calls)
        : m_evidence{evidence}, m_calls{std::move(calls)} {}
    const std::vector<LC_ParametricSegment>& segments() const override { return m_segments; }
    std::size_t branchCount() const override { return 3; }
    std::ptrdiff_t next(std::size_t) const override { return -1; }
    bool evaluate(std::size_t segment, double t, RS_Vector& point, RS_Vector& derivative) const override {
        if (segment >= m_segments.size() || !std::isfinite(t) ||
            t < m_segments[segment].t0 || t > m_segments[segment].t1) { return false; }
        const double root = std::sqrt(2.0);
        point = segment == 0 ? RS_Vector{t, 0} : RS_Vector{root + (segment == 2 ? t : 0), t};
        derivative = segment == 0 ? RS_Vector{1, 0} : RS_Vector{segment == 2 ? 1.0 : 0.0, 1};
        return true;
    }
    bool bound(std::size_t segment, double a, double b, LC_Interval& x, LC_Interval& y,
               LC_Interval& dx, LC_Interval& dy) const override {
        x = y = dx = dy = {};
        if (segment >= m_segments.size() || !std::isfinite(a) || !std::isfinite(b) || a > b ||
            a < m_segments[segment].t0 || b > m_segments[segment].t1) { return false; }
        const auto t = LC_Interval::hull(a, b), root = sqrt(LC_Interval::point(2));
        x = segment == 0 ? t : segment == 1 ? root : root + t;
        y = segment == 0 ? LC_Interval::point(0) : t;
        dx = LC_Interval::point(segment == 1 ? 0 : 1);
        dy = LC_Interval::point(segment == 0 ? 0 : 1);
        return true;
    }
    LC_AnalyticOccurrenceIdentityCertificate certifyOccurrenceIdentity(
        const LC_ParametricIntersection& a, bool firstA,
        const LC_ParametricIntersection& b, bool secondA) const override {
        ++*m_calls;
        if (m_evidence == Evidence::Unavailable || !firstA || !secondA || a.branchA != 0 || b.branchA != 0 ||
            a.branchB == b.branchB || a.branchB > 2 || b.branchB > 2) { return {}; }
        LC_AnalyticOccurrenceIdentityCertificate out{true, a.proofDomain, b.proofDomain,
                                                     sqrt(LC_Interval::point(2))};
        if (m_evidence == Evidence::WrongSegment) { out.firstDomain.segmentA = 3; }
        if (m_evidence == Evidence::EmptyDomain) { out.secondDomain.parameterA = {}; }
        if (m_evidence == Evidence::WrongParameter) { out.parameter = LC_Interval::point(0); }
        return out;
    }
private:
    Evidence m_evidence;
    std::shared_ptr<unsigned> m_calls;
    const std::vector<LC_ParametricSegment> m_segments{{0, 0, 2}, {1, -1, 1}, {2, -1, 1}};
};
}

TEST_CASE("Reference shared occurrence proof orders two roots without deleting either pair",
          "[lc-offsetcurve][geometry][reference-components][intersection-identity]") {
    LC_IntersectionOptions options;
    options.tolerance = 1e-6; options.anchoredBranches = 1; options.maxDepth = 20;
    options.analyticOccurrenceIdentity = true;
    auto calls = std::make_shared<unsigned>(0);
    const auto result = findCertifiedIntersections(std::make_unique<SharedAlgebraicOccurrence>(
        SharedAlgebraicOccurrence::Evidence::Valid, calls), options);
    REQUIRE(result.status == LC_IntersectionStatus::Ok); REQUIRE(result.session);
    REQUIRE(result.session->rootCount() == 2); REQUIRE(*calls > 0);
    const Quad root = sqrt(Quad(2));
    std::array<bool, 3> seen{};
    for (std::size_t i = 0; i < 2; ++i) {
        const auto snapshot = result.session->snapshot(result.session->rootHandle(i));
        REQUIRE(snapshot);
        CHECK(snapshot->event.branchA == 0); REQUIRE(snapshot->event.branchB > 0);
        REQUIRE(snapshot->event.branchB < seen.size()); seen[snapshot->event.branchB] = true;
        checkEnclosure(snapshot->event.isolation.parameterA, root);
        CHECK(snapshot->event.isolation.parameterB.contains(0));
    }
    CHECK(seen[1]); CHECK(seen[2]);
    auto legacyOptions = options; legacyOptions.strict = true;
    const SharedAlgebraicOccurrence legacyProvider{SharedAlgebraicOccurrence::Evidence::Valid,
                                                    std::make_shared<unsigned>(0)};
    const auto legacy = findIntersections(legacyProvider, legacyOptions);
    CHECK(legacy.status == LC_IntersectionStatus::Ok); CHECK(legacy.intersections.size() == 2);
    for (const bool itemCap : {false, true}) {
        for (const bool less : {false, true}) {
            auto limited = options;
            if (itemCap) { limited.maxGeneratedItems = result.work.generatedItems - unsigned(less); }
            else { limited.maxBoxPairs = result.work.totalBoxPairs - unsigned(less); }
            const auto replay = findCertifiedIntersections(std::make_unique<SharedAlgebraicOccurrence>(
                SharedAlgebraicOccurrence::Evidence::Valid, std::make_shared<unsigned>(0)), limited);
            CHECK(replay.status == (less ? LC_IntersectionStatus::LimitExceeded : LC_IntersectionStatus::Ok));
            CHECK(bool(replay.session) == !less);
            CHECK(replay.work.generatedItems <= limited.maxGeneratedItems);
            CHECK(replay.work.totalBoxPairs <= limited.maxBoxPairs);
        }
    }
}

TEST_CASE("Reference shared occurrence evidence is opt in and malformed domains fail closed",
          "[lc-offsetcurve][geometry][reference-components][intersection-identity]") {
    LC_IntersectionOptions options;
    options.tolerance = 1e-6; options.anchoredBranches = 1; options.maxDepth = 4;
    std::optional<LC_IntersectionWorkStats> legacyWork;
    for (const auto evidence : {SharedAlgebraicOccurrence::Evidence::Valid,
                                SharedAlgebraicOccurrence::Evidence::Unavailable}) {
        auto calls = std::make_shared<unsigned>(0);
        const auto legacy = findCertifiedIntersections(std::make_unique<SharedAlgebraicOccurrence>(evidence, calls), options);
        CHECK(legacy.status != LC_IntersectionStatus::Ok); CHECK_FALSE(legacy.session); CHECK(*calls == 0);
        if (legacyWork) {
            CHECK(legacy.work.totalBoxPairs == legacyWork->totalBoxPairs);
            CHECK(legacy.work.generatedItems == legacyWork->generatedItems);
            CHECK(legacy.work.deepest() == legacyWork->deepest());
        } else { legacyWork = legacy.work; }
    }
    options.analyticOccurrenceIdentity = true;
    const auto unavailable = findCertifiedIntersections(std::make_unique<SharedAlgebraicOccurrence>(
        SharedAlgebraicOccurrence::Evidence::Unavailable, std::make_shared<unsigned>(0)), options);
    CHECK(unavailable.status != LC_IntersectionStatus::Ok); CHECK_FALSE(unavailable.session);
    for (const auto evidence : {SharedAlgebraicOccurrence::Evidence::WrongSegment,
                                SharedAlgebraicOccurrence::Evidence::EmptyDomain,
                                SharedAlgebraicOccurrence::Evidence::WrongParameter}) {
        auto calls = std::make_shared<unsigned>(0);
        const auto invalid = findCertifiedIntersections(std::make_unique<SharedAlgebraicOccurrence>(evidence, calls), options);
        CAPTURE(evidence, invalid.status);
        CHECK(invalid.status == LC_IntersectionStatus::InvalidInput); CHECK_FALSE(invalid.session);
        CHECK(*calls > 0); CHECK(invalid.work.certificateBoxPairs > 0);
    }
}

TEST_CASE("Reference zero edge own nodes require strict hidden point evidence",
          "[lc-offsetcurve][geometry][reference-components]") {
    // F<0 makes every open span hidden, but a span decision does NOT dispose of
    // its endpoint. At this scale the true endpoint clearance deficit is below
    // the certified image arithmetic: the point must remain unresolved, not disappear.
    const auto data = basis(1.0, 1.0, 0.0, 2e-8);
    geo::WorkBudget budget;
    const auto result = geo::analyseReferenceComponents(data, -2.0, 1e-6, budget);
    CAPTURE(result.status, result.reason, result.eventReason, result.work.deepest);
    CHECK(result.status == LC_IntersectionStatus::AmbiguousTopology);
    CHECK(result.reason == geo::ReferenceComponentReason::UnresolvedEventPoint);
    CHECK_FALSE(result.analysis); checkWork(result.work, budget.consumed());
}

TEST_CASE("Reference true axis mapping covers admitted near minimum wrapped sweeps",
          "[lc-offsetcurve][geometry][reference-components]") {
    for (const bool reversed : {false, true}) {
        auto data = basis(10.0, 0.5, 2 * M_PI - 2e-8, 0.0);
        if (reversed) { std::swap(data.angle1, data.angle2); data.reversed = true; }
        const auto admitted = geo::sweep(data);
        REQUIRE(admitted); CHECK_FALSE(admitted->whole);
        CHECK(std::abs(admitted->signedSweep) > RS_TOLERANCE_ANGLE);
        geo::WorkBudget budget;
        const auto result = geo::analyseReferenceComponents(data, -10.0, 1e-5, budget);
        CAPTURE(reversed, result.status, result.reason, result.eventReason);
        // This is a valid near-minimum sweep, not an ambiguous pi bracket. The
        // denominator-sign guard is the safety proof; successful geometry is
        // independently checked here without requiring numerical acceptance.
        if (result.status == LC_IntersectionStatus::Ok) {
            REQUIRE(result.analysis);
            checkComponentAnalysis(*result.analysis, data, -10.0, 1e-5);
        } else { CHECK_FALSE(result.analysis); }
        checkWork(result.work, budget.consumed());
    }
}

namespace {
RS_SplineData cacheLayoutFixture(const std::size_t degree, const std::vector<double>& breaks) {
    RS_SplineData data;
    data.degree = degree;
    const std::size_t spans = breaks.size() - 1;
    for (std::size_t i = 0; i <= degree * spans; ++i) {
        data.controlPoints.emplace_back(double(i), double(i * i));
        data.weights.push_back(1.0);
    }
    for (std::size_t i = 0; i < breaks.size(); ++i) {
        data.knotslist.insert(data.knotslist.end(), i == 0 || i == spans ? degree + 1 : degree, breaks[i]);
    }
    return data;
}

void checkCacheSpans(const RS_SplineData& data, const geo::CacheLayoutResult& layout,
                     const std::vector<double>& breaks) {
    REQUIRE(layout.status == geo::CacheLayoutStatus::Ok);
    REQUIRE(layout.spanCount() == breaks.size() - 1);
    for (std::size_t k = 0; k < layout.spanCount(); ++k) {
        const auto& span = layout.spans[k];
        CHECK(span.degree == data.degree);
        CHECK(span.parameter.lo() == breaks[k]); CHECK(span.parameter.hi() == breaks[k + 1]);
        for (std::size_t i = 0; i <= data.degree; ++i) {
            const auto& actual = span.controls[i];
            const auto& expected = data.controlPoints[data.degree * k + i];
            CHECK(actual.valid == expected.valid);
            CHECK(actual.x == expected.x); CHECK(actual.y == expected.y); CHECK(actual.z == expected.z);
        }
    }
    checkWork(layout.work, {breaks.size() - 1, data.controlPoints.size(), 0, 0});
}

void checkLayoutRefusal(const RS_SplineData& data, const geo::CheckWork& spent = {}) {
    geo::WorkBudget budget;
    const auto layout = geo::inspectCacheLayout(data, budget);
    CHECK(layout.status == geo::CacheLayoutStatus::InvalidLayout);
    CHECK(layout.spanCount() == 0); CHECK(layout.spans.empty());
    CHECK(budget.status() == geo::BudgetStatus::Ok);
    checkWork(layout.work, spent); checkWork(layout.work, budget.consumed());
}
}

TEST_CASE("Cache layout checked formulas reject overflow without allocating arrays",
          "[lc-offsetcurve][geometry][cache-layout]") {
    const auto maximum = std::numeric_limits<std::size_t>::max();
    for (const std::size_t degree : {1u, 3u}) {
        for (const std::size_t n : {1u, 2u, 17u}) {
            const auto count = geo::cacheLayoutCounts(degree, n);
            REQUIRE(count); CHECK(count->spans == n);
            CHECK(count->controls == degree * n + 1); CHECK(count->knots == degree * n + degree + 2);
            const auto derived = geo::cacheLayoutCountsFromControls(degree, count->controls);
            REQUIRE(derived); CHECK(derived->spans == n); CHECK(derived->knots == count->knots);
        }
        const auto last = (maximum - degree - 2) / degree;
        const auto limit = geo::cacheLayoutCounts(degree, last);
        REQUIRE(limit);
        const auto recovered = geo::cacheLayoutCountsFromControls(degree, limit->controls);
        REQUIRE(recovered); CHECK(recovered->spans == last);
        CHECK_FALSE(geo::cacheLayoutCountsFromControls(degree, limit->controls + degree));
        using Big = boost::multiprecision::cpp_int;
        CHECK(Big(limit->controls) == Big(degree) * last + 1);
        CHECK(Big(limit->knots) == Big(degree) * last + degree + 2);
        CHECK(Big(degree) * (Big(last) + 1) + degree + 2 > Big(maximum));
        CHECK_FALSE(geo::cacheLayoutCounts(degree, last + 1));
        CHECK_FALSE(geo::cacheLayoutCounts(degree, maximum));
        CHECK_FALSE(geo::cacheLayoutCounts(degree, 0));
        CHECK_FALSE(geo::cacheLayoutCountsFromControls(degree, 0));
        CHECK_FALSE(geo::cacheLayoutCountsFromControls(degree, 1));
        CHECK_FALSE(geo::cacheLayoutCountsFromControls(degree, maximum));
    }
    for (const std::size_t degree : {std::size_t(0), std::size_t(2), std::size_t(4), maximum}) {
        CHECK_FALSE(geo::cacheLayoutCounts(degree, 1));
        CHECK_FALSE(geo::cacheLayoutCountsFromControls(degree, 4));
    }
    CHECK_FALSE(geo::cacheLayoutCountsFromControls(3, 3));
    CHECK_FALSE(geo::cacheLayoutCountsFromControls(3, 5));
}

TEST_CASE("Cache layout admits normalized line data without an entity conversion or geometry claim",
          "[lc-offsetcurve][geometry][cache-layout]") {
    geo::CacheLayoutResult uninspected;
    CHECK(uninspected.status == geo::CacheLayoutStatus::InvalidInput); CHECK(uninspected.spans.empty());
    auto data = cacheLayoutFixture(1, {0, 1});
    data.controlPoints = {{-2, 3}, {7, -5}};
    geo::WorkBudget budget;
    const auto layout = geo::inspectCacheLayout(data, budget);
    checkCacheSpans(data, layout, {0, 1});
    CHECK(layout.spans[0].degree == 1);
}

TEST_CASE("Cache layout extracts each linear and cubic span with its own shared endpoint",
          "[lc-offsetcurve][geometry][cache-layout]") {
    for (const std::size_t degree : {1u, 3u}) {
        for (const auto& breaks : {std::vector<double>{0, 1}, std::vector<double>{0, 1, 2, 3}}) {
            const auto data = cacheLayoutFixture(degree, breaks);
            geo::WorkBudget budget;
            const auto layout = geo::inspectCacheLayout(data, budget);
            checkCacheSpans(data, layout, breaks);
            for (std::size_t i = 1; i < layout.spanCount(); ++i) {
                CHECK(layout.spans[i - 1].controls[degree].x == layout.spans[i].controls[0].x);
                CHECK(layout.spans[i - 1].controls[degree].y == layout.spans[i].controls[0].y);
            }
        }
    }
}

TEST_CASE("Cache layout retains affine narrow subnormal and extreme finite knot domains exactly",
          "[lc-offsetcurve][geometry][cache-layout]") {
    const double tiny = std::numeric_limits<double>::denorm_min();
    const double maximum = std::numeric_limits<double>::max();
    const double a = 1.0, b = std::nextafter(a, 2.0), c = std::nextafter(b, 2.0);
    for (const std::size_t degree : {1u, 3u}) {
        for (const auto& breaks : {std::vector<double>{-7, -4, -1, 2}, std::vector<double>{a, b, c},
                                   std::vector<double>{0, tiny, 2 * tiny}, std::vector<double>{-maximum, maximum}}) {
            const auto data = cacheLayoutFixture(degree, breaks);
            geo::WorkBudget budget;
            checkCacheSpans(data, geo::inspectCacheLayout(data, budget), breaks);
        }
    }
}

TEST_CASE("Cache layout rejects unsupported degree representation and nondefault payload before admission",
          "[lc-offsetcurve][geometry][cache-layout]") {
    const auto valid = cacheLayoutFixture(3, {0, 1, 2});
    for (const std::size_t degree : {std::size_t(0), std::size_t(2), std::size_t(4),
                                    std::numeric_limits<std::size_t>::max()}) {
        auto bad = valid; bad.degree = degree; checkLayoutRefusal(bad);
    }
    for (const auto type : {RS_SplineData::SplineType::Standard, RS_SplineData::SplineType::WrappedClosed,
                            static_cast<RS_SplineData::SplineType>(99)}) {
        auto bad = valid; bad.type = type; checkLayoutRefusal(bad);
        bad = valid; bad.savedOpenType = type; checkLayoutRefusal(bad);
    }
    auto fit = valid; fit.fitPoints = {{0, 0}}; checkLayoutRefusal(fit);
    auto saved = valid; saved.savedOpenKnots = {0, 1}; checkLayoutRefusal(saved);
}

TEST_CASE("Cache layout checks short and inconsistent arrays before scanning or debiting",
          "[lc-offsetcurve][geometry][cache-layout]") {
    for (const std::size_t degree : {1u, 3u}) {
        const auto valid = cacheLayoutFixture(degree, {0, 1, 2});
        for (const std::size_t count : {std::size_t(0), std::size_t(1)}) {
            auto bad = valid; bad.controlPoints.resize(count); checkLayoutRefusal(bad);
        }
        auto bad = valid; bad.controlPoints.pop_back(); checkLayoutRefusal(bad);
        bad = valid; bad.knotslist.pop_back(); checkLayoutRefusal(bad);
        bad = valid; bad.knotslist.push_back(2); checkLayoutRefusal(bad);
        bad = valid; bad.weights.clear(); checkLayoutRefusal(bad);
        bad = valid; bad.weights.pop_back(); checkLayoutRefusal(bad);
        bad = valid; bad.weights.push_back(1); checkLayoutRefusal(bad);
    }
}

TEST_CASE("Cache layout rejects unequal knot groups and unordered representatives without epsilon repair",
          "[lc-offsetcurve][geometry][cache-layout]") {
    for (const std::size_t degree : {1u, 3u}) {
        const auto valid = cacheLayoutFixture(degree, {0, 1, 2});
        const geo::CheckWork spent{2, valid.controlPoints.size(), 0, 0};
        auto bad = valid; bad.knotslist[0] = std::nextafter(0.0, -1.0); checkLayoutRefusal(bad, spent);
        bad = valid; bad.knotslist.back() = std::nextafter(2.0, 3.0); checkLayoutRefusal(bad, spent);
        bad = valid;
        for (std::size_t i = degree + 1; i < 2 * degree + 1; ++i) { bad.knotslist[i] = 0; }
        checkLayoutRefusal(bad, spent);
        bad = valid;
        for (std::size_t i = degree + 1; i < 2 * degree + 1; ++i) { bad.knotslist[i] = -1; }
        checkLayoutRefusal(bad, spent);
        if (degree == 3) {
            bad = valid; bad.knotslist[5] = std::nextafter(1.0, 2.0); checkLayoutRefusal(bad, spent);
        }
    }
}

TEST_CASE("Cache layout rejects nonfinite knots after irreversible admission without exposing spans",
          "[lc-offsetcurve][geometry][cache-layout]") {
    for (const std::size_t degree : {1u, 3u}) {
        const auto valid = cacheLayoutFixture(degree, {0, 1, 2});
        for (const double value : {std::numeric_limits<double>::infinity(), -std::numeric_limits<double>::infinity(),
                                   std::numeric_limits<double>::quiet_NaN()}) {
            for (std::size_t i = 0; i < valid.knotslist.size(); ++i) {
                auto bad = valid; bad.knotslist[i] = value;
                checkLayoutRefusal(bad, {2, valid.controlPoints.size(), 0, 0});
            }
        }
    }
}

TEST_CASE("Cache layout requires valid finite planar controls and does not project nonzero Z",
          "[lc-offsetcurve][geometry][cache-layout]") {
    for (const std::size_t degree : {1u, 3u}) {
        const auto valid = cacheLayoutFixture(degree, {0, 1, 2});
        for (const RS_Vector point : {RS_Vector{false}, RS_Vector{0, 0, std::numeric_limits<double>::denorm_min()},
                                      RS_Vector{std::numeric_limits<double>::infinity(), 0},
                                      RS_Vector{0, std::numeric_limits<double>::quiet_NaN()}}) {
            auto bad = valid; bad.controlPoints.back() = point;
            checkLayoutRefusal(bad, {2, valid.controlPoints.size(), 0, 0});
        }
        auto negativeZero = valid; negativeZero.controlPoints[0].z = -0.0;
        geo::WorkBudget budget;
        checkCacheSpans(negativeZero, geo::inspectCacheLayout(negativeZero, budget), {0, 1, 2});
    }
}

TEST_CASE("Cache layout requires complete exactly unit finite weights without rational normalization",
          "[lc-offsetcurve][geometry][cache-layout]") {
    for (const std::size_t degree : {1u, 3u}) {
        const auto valid = cacheLayoutFixture(degree, {0, 1, 2});
        for (const double weight : {0.0, -1.0, 2.0, std::nextafter(1.0, 0.0), std::nextafter(1.0, 2.0),
                                    std::numeric_limits<double>::infinity(), std::numeric_limits<double>::quiet_NaN()}) {
            auto bad = valid; bad.weights.back() = weight;
            checkLayoutRefusal(bad, {2, valid.controlPoints.size(), 0, 0});
        }
    }
}

TEST_CASE("Cache layout leaves closure and periodicity decisions to later topology stages",
          "[lc-offsetcurve][geometry][cache-layout]") {
    for (const std::size_t degree : {1u, 3u}) {
        for (const bool closed : {false, true}) {
            for (const bool periodic : {false, true}) {
                auto data = cacheLayoutFixture(degree, {0, 1, 2});
                data.m_closedFlag = closed; data.m_periodicFlag = periodic;
                geo::WorkBudget budget;
                checkCacheSpans(data, geo::inspectCacheLayout(data, budget), {0, 1, 2});
                CHECK(data.m_closedFlag == closed); CHECK(data.m_periodicFlag == periodic);
            }
        }
    }
}

TEST_CASE("Cache layout admits exact span and vertex budgets and refuses one less atomically",
          "[lc-offsetcurve][geometry][cache-layout]") {
    for (const std::size_t degree : {1u, 3u}) {
        const auto data = cacheLayoutFixture(degree, {0, 1, 2, 3});
        for (unsigned shortDimension = 0; shortDimension < 3; ++shortDimension) {
            LC_OffsetCheckLimits limits{3, data.controlPoints.size(), 1, 1};
            if (shortDimension == 1) { --limits.maxCacheSpans; }
            if (shortDimension == 2) { --limits.maxVertices; }
            geo::WorkBudget budget{limits};
            const auto result = geo::inspectCacheLayout(data, budget);
            if (shortDimension == 0) { checkCacheSpans(data, result, {0, 1, 2, 3}); }
            else {
                CHECK(result.status == geo::CacheLayoutStatus::LimitExceeded); CHECK(result.spans.empty());
                checkWork(result.work, {});
                CHECK(budget.status() == geo::BudgetStatus::LimitExceeded);
                const auto again = geo::inspectCacheLayout(data, budget);
                CHECK(again.status == geo::CacheLayoutStatus::LimitExceeded); CHECK(again.spans.empty());
                checkWork(again.work, {});
            }
        }
    }
}

TEST_CASE("Cache layout pre spent and invalid budgets cannot partially debit either dimension",
          "[lc-offsetcurve][geometry][cache-layout]") {
    const auto data = cacheLayoutFixture(3, {0, 1, 2});
    for (unsigned resource = 0; resource < 2; ++resource) {
        LC_OffsetCheckLimits limits{3, data.controlPoints.size() + 1, 9, 3};
        geo::WorkBudget budget{limits};
        REQUIRE(budget.consumePieces(resource == 0 ? 2 : 1));
        REQUIRE(budget.consumeVertices(resource == 1 ? 2 : 1));
        REQUIRE(budget.consumePairs(3)); REQUIRE(budget.checkDepth(2));
        const auto before = budget.consumed();
        const auto result = geo::inspectCacheLayout(data, budget);
        CHECK(result.status == geo::CacheLayoutStatus::LimitExceeded); CHECK(result.spans.empty());
        checkWork(result.work, before); checkWork(budget.consumed(), before);
    }
    geo::WorkBudget exact{LC_OffsetCheckLimits{3, data.controlPoints.size() + 1, 9, 3}};
    REQUIRE(exact.consumePieces(1)); REQUIRE(exact.consumeVertices(1));
    REQUIRE(exact.consumePairs(3)); REQUIRE(exact.checkDepth(2));
    const auto admitted = geo::inspectCacheLayout(data, exact);
    REQUIRE(admitted.status == geo::CacheLayoutStatus::Ok); CHECK(admitted.spanCount() == 2);
    checkWork(admitted.work, {3, data.controlPoints.size() + 1, 3, 2});
    for (const LC_OffsetCheckLimits& limits : std::array<LC_OffsetCheckLimits, 4>{{{0, 1, 1, 1}, {1, 0, 1, 1},
                                                                                {1, 1, 0, 1}, {1, 1, 1, 0}}}) {
        geo::WorkBudget budget{limits};
        const auto result = geo::inspectCacheLayout(data, budget);
        CHECK(result.status == geo::CacheLayoutStatus::InvalidInput); CHECK(result.spans.empty());
        checkWork(result.work, {});
    }
}

TEST_CASE("Cache layout failures after admission spend once and successful output owns its copied spans",
          "[lc-offsetcurve][geometry][cache-layout]") {
    auto data = cacheLayoutFixture(3, {0, 1, 2});
    geo::WorkBudget budget{LC_OffsetCheckLimits{4, 2 * data.controlPoints.size(), 1, 1}};
    auto malformed = data; malformed.knotslist.back() = 3;
    const auto bad = geo::inspectCacheLayout(malformed, budget);
    CHECK(bad.status == geo::CacheLayoutStatus::InvalidLayout); CHECK(bad.spans.empty());
    checkWork(bad.work, {2, data.controlPoints.size(), 0, 0});
    const auto valid = geo::inspectCacheLayout(data, budget);
    REQUIRE(valid.status == geo::CacheLayoutStatus::Ok); REQUIRE(valid.spanCount() == 2);
    checkWork(valid.work, {4, 2 * data.controlPoints.size(), 0, 0});
    const auto exhausted = geo::inspectCacheLayout(data, budget);
    CHECK(exhausted.status == geo::CacheLayoutStatus::LimitExceeded); CHECK(exhausted.spans.empty());
    checkWork(exhausted.work, valid.work);
    const RS_Vector first = valid.spans[0].controls[0];
    data.controlPoints.clear(); data.knotslist.clear(); data.weights.clear();
    CHECK(valid.spans[0].controls[0].x == first.x); CHECK(valid.spans[0].controls[0].y == first.y);
    CHECK(valid.spans[0].parameter.lo() == 0); CHECK(valid.spans[1].parameter.hi() == 2);
}

TEST_CASE("Cache layout accepts the actual cubic materializer output without resampling or refitting",
          "[lc-offsetcurve][geometry][cache-layout]") {
    auto sourceData = cacheLayoutFixture(3, {0, 1});
    sourceData.controlPoints = {{0, 0}, {4, 6}, {8, -6}, {12, 0}};
    const RS_Spline source{nullptr, sourceData};
    const auto made = LC_CurveOffset::createEntities(source,
        LC_CurveOffset::makeSideRequest(LC_CurveOffsetSide::Left, 0.75),
        LC_CurveOffset::makeDirectOptions(source, 0.75), LC_CurveOffset::makeDirectSourceBudget());
    REQUIRE(made.status == LC_CurveOffsetStatus::Ok); REQUIRE(made.entities.size() == 1);
    const auto* spline = dynamic_cast<const RS_Spline*>(made.entities[0].get());
    REQUIRE(spline); REQUIRE(spline->getData().degree == 3);
    const auto& data = spline->getData();
    const std::size_t n = (data.controlPoints.size() - 1) / 3;
    REQUIRE(n > 1);
    std::vector<double> breaks;
    for (std::size_t i = 0; i <= n; ++i) { breaks.push_back(double(i)); }
    geo::WorkBudget budget;
    checkCacheSpans(data, geo::inspectCacheLayout(data, budget), breaks);
}

TEST_CASE("Cache layout accepts the actual degree one closed rectangle materializer output",
          "[lc-offsetcurve][geometry][cache-layout]") {
    RS_Spline source{nullptr, RS_SplineData(1, false)};
    for (const RS_Vector p : {RS_Vector{0, 0}, RS_Vector{10, 0}, RS_Vector{10, 4}, RS_Vector{0, 4}}) {
        source.addControlPoint(p);
    }
    source.setClosed(true);
    auto options = LC_CurveOffset::makeDirectOptions(source, 1.0); options.mode = LC_CurveOffsetMode::Trimmed;
    const auto made = LC_CurveOffset::createEntities(source,
        LC_CurveOffset::makeSideRequest(LC_CurveOffsetSide::Left, 1.0), options,
        LC_CurveOffset::makeDirectSourceBudget());
    REQUIRE(made.status == LC_CurveOffsetStatus::Ok); REQUIRE(made.entities.size() == 1);
    const auto* spline = dynamic_cast<const RS_Spline*>(made.entities[0].get());
    REQUIRE(spline); REQUIRE(spline->getData().degree == 1);
    const auto& data = spline->getData();
    REQUIRE(data.controlPoints.size() > 2);
    std::vector<double> breaks;
    for (std::size_t i = 0; i < data.controlPoints.size(); ++i) { breaks.push_back(double(i)); }
    geo::WorkBudget budget;
    checkCacheSpans(data, geo::inspectCacheLayout(data, budget), breaks);
}

namespace {
// Test-only directed dyadic arithmetic. Every rounding decision is an exact
// integer/rational comparison, independent of LC_Interval, libm and the fenv.
using OracleInteger = boost::multiprecision::cpp_int;
const EventRational& oracleGrid() {
    static const EventRational grid{OracleInteger{1} << 192};
    return grid;
}
OracleInteger oracleFloor(const EventRational& value) {
    const OracleInteger n = numerator(value), d = denominator(value);
    OracleInteger result = n / d;
    if (n < 0 && n % d != 0) { --result; }
    return result;
}
struct OutwardOracle {
    EventRational lo, hi;
    OutwardOracle(const EventRational& lower, const EventRational& upper)
        : lo{EventRational{oracleFloor(lower * oracleGrid())} / oracleGrid()},
          hi{-EventRational{oracleFloor(-upper * oracleGrid())} / oracleGrid()} {}
    explicit OutwardOracle(const EventRational& point) : OutwardOracle{point, point} {}
};
OutwardOracle operator+(const OutwardOracle& a, const OutwardOracle& b) { return {a.lo + b.lo, a.hi + b.hi}; }
OutwardOracle operator-(const OutwardOracle& a) { return {-a.hi, -a.lo}; }
OutwardOracle operator-(const OutwardOracle& a, const OutwardOracle& b) { return a + (-b); }
OutwardOracle operator*(const OutwardOracle& a, const OutwardOracle& b) {
    const std::array<EventRational, 4> products{{a.lo * b.lo, a.lo * b.hi, a.hi * b.lo, a.hi * b.hi}};
    return {*std::min_element(products.begin(), products.end()), *std::max_element(products.begin(), products.end())};
}
OutwardOracle operator/(const OutwardOracle& a, const OutwardOracle& b) {
    REQUIRE(b.lo > 0);
    return a * OutwardOracle{EventRational{1} / b.hi, EventRational{1} / b.lo};
}
OutwardOracle oracleSqrt(const OutwardOracle& a) {
    REQUIRE(a.lo >= 0);
    const EventRational scale = oracleGrid() * oracleGrid();
    const OracleInteger low = oracleFloor(a.lo * scale), high = -oracleFloor(-a.hi * scale);
    const OracleInteger lower = boost::multiprecision::sqrt(low);
    OracleInteger upper = boost::multiprecision::sqrt(high);
    if (upper * upper < high) { ++upper; }
    REQUIRE(lower * lower <= low); REQUIRE((lower + 1) * (lower + 1) > low);
    REQUIRE(upper * upper >= high);
    return {EventRational{lower} / oracleGrid(), EventRational{upper} / oracleGrid()};
}
const OutwardOracle& oraclePi() {
    static const OutwardOracle pi = [] {
        const auto atanBounds = [](const EventRational& x) -> std::array<EventRational, 2> {
            EventRational sum = 0, term = x;
            for (unsigned k = 0; k < 32; ++k) {
                EventRational signedTerm = term;
                if (k % 2) { signedTerm = -signedTerm; }
                sum += signedTerm / (2 * k + 1); term *= x * x;
            }
            return {sum, EventRational{sum + term / 65}};
        };
        // Exact alternating remainders and Machin's identity enclose REAL pi.
        const auto a = atanBounds(EventRational{1} / 5), b = atanBounds(EventRational{1} / 239);
        return OutwardOracle{16 * a[0] - 4 * b[1], 16 * a[1] - 4 * b[0]};
    }();
    return pi;
}
std::array<OutwardOracle, 2> oracleTrig(const OutwardOracle& angle) {
    const double estimate = EventRational{(angle.lo + angle.hi) / 2}.convert_to<double>();
    REQUIRE(std::isfinite(estimate));
    // This estimate only chooses an integer reduction. Its validity is checked
    // by the outward reduced interval, not assumed from rounded pi or libm.
    const int axis = int(std::lround(estimate / M_PI_2));
    const OutwardOracle x = angle - oraclePi() * OutwardOracle{EventRational{axis} / 2};
    REQUIRE(x.lo >= -1); REQUIRE(x.hi <= 1);
    std::array<EventRational, 34> inverseFactorial;
    inverseFactorial[0] = 1;
    for (unsigned n = 1; n <= 33; ++n) { inverseFactorial[n] = inverseFactorial[n - 1] / n; }
    const OutwardOracle square = x * x;
    OutwardOracle sine{-inverseFactorial[31]}, cosine{inverseFactorial[32]};
    for (int k = 14; k >= 0; --k) {
        const EventRational c = k % 2 ? -inverseFactorial[2 * k + 1] : inverseFactorial[2 * k + 1];
        sine = OutwardOracle{c} + square * sine;
    }
    for (int k = 15; k >= 0; --k) {
        const EventRational c = k % 2 ? -inverseFactorial[2 * k] : inverseFactorial[2 * k];
        cosine = OutwardOracle{c} + square * cosine;
    }
    // Degree-32 Taylor/Lagrange remainders: all trig derivatives have modulus
    // <=1 and |x|<=1, so BOTH errors are <=1/33! on the entire interval.
    const OutwardOracle remainder{-inverseFactorial[33], inverseFactorial[33]};
    sine = x * sine + remainder; cosine = cosine + remainder;
    switch ((axis % 4 + 4) % 4) {
    case 0: return {cosine, sine};
    case 1: return {-sine, cosine};
    case 2: return {-cosine, -sine};
    case 3: return {sine, -cosine};
    }
    FAIL("Invalid oracle quadrant");
    return {cosine, sine};
}
std::array<OutwardOracle, 2> oracleEllipseOffset(const RS_EllipseData& source, const double distance,
                                              const double scale, const double duration, const OutwardOracle& t) {
    const EventRational rawExtent = EventRational(source.angle2) - EventRational(source.angle1);
    OutwardOracle extent{rawExtent};
    const OutwardOracle two{EventRational{2}};
    if (!source.reversed && rawExtent < 0) { extent = extent + two * oraclePi(); }
    if (source.reversed && rawExtent > 0) { extent = extent - two * oraclePi(); }
    const auto trig = oracleTrig(OutwardOracle{EventRational(source.angle1)} + extent * t / OutwardOracle{EventRational(duration)});
    const auto& c = trig[0]; const auto& s = trig[1];
    // Normalize the exact raw doubles BEFORE fixed-grid rounding, preserving
    // subnormal/huge axes; no world-valued intermediate is rounded to zero.
    const OutwardOracle mx{EventRational(source.majorP.x) / EventRational(scale)};
    const OutwardOracle my{EventRational(source.majorP.y) / EventRational(scale)};
    const OutwardOracle r{EventRational(source.ratio)}, d{EventRational(distance) / EventRational(scale)};
    const auto norm = oracleSqrt(mx * mx + my * my);
    const auto h = oracleSqrt(r * r * c * c + s * s);
    const auto normalScale = d / (norm * h);
    return {mx * c - r * my * s + normalScale * (r * mx * c - my * s),
            my * c + r * mx * s + normalScale * (r * my * c + mx * s)};
}

std::array<Quad, 2> cachePointOracle(const geo::CacheSpan& span, const RS_EllipseData& source,
                                   const geo::ReferenceFrame& frame, const Quad& t) {
    std::array<std::array<Quad, 2>, 4> points;
    for (std::size_t i = 0; i <= span.degree; ++i) {
        points[i] = {(Quad(span.controls[i].x) - Quad(source.center.x)) / Quad(frame.scale()),
                     (Quad(span.controls[i].y) - Quad(source.center.y)) / Quad(frame.scale())};
    }
    for (std::size_t level = span.degree; level > 0; --level) {
        for (std::size_t i = 0; i < level; ++i) {
            for (unsigned k = 0; k < 2; ++k) { points[i][k] = (1 - t) * points[i][k] + t * points[i + 1][k]; }
        }
    }
    return points[0];
}
void checkChordCleared(const geo::CurveChordBounds& bounds) {
    for (const auto& interval : {bounds.startX, bounds.startY, bounds.endX, bounds.endY,
                                 bounds.x, bounds.y, bounds.deviation}) { CHECK_FALSE(interval.isValid()); }
}
RS_SplineData arcCache(const RS_EllipseData& source, const double distance, const std::size_t degree) {
    const auto frame = geo::ReferenceFrame::create(source, distance);
    REQUIRE(frame);
    LC_CurveJet a, b;
    REQUIRE(frame->offsetJet(0, a)); REQUIRE(frame->offsetJet(frame->parameterEnd(), b));
    const auto world = [&](const RS_Vector& point) {
        return RS_Vector{source.center.x + frame->scale() * point.x, source.center.y + frame->scale() * point.y};
    };
    auto data = cacheLayoutFixture(degree, {0, 1});
    data.controlPoints.front() = world(a.point); data.controlPoints.back() = world(b.point);
    if (degree == 3) {
        const double h = frame->parameterEnd() / 3;
        data.controlPoints[1] = world({a.point.x + h * a.first.x, a.point.y + h * a.first.y});
        data.controlPoints[2] = world({b.point.x - h * b.first.x, b.point.y - h * b.first.y});
    }
    return data;
}
void checkBoundsRelation(const geo::CacheBoundsResult& result, const double distance, const double tolerance) {
    REQUIRE(result.status == geo::CacheBoundsStatus::Ok); REQUIRE(result.analysis);
    const auto& a = *result.analysis;
    REQUIRE_FALSE(a.witnesses().empty());
    const auto& r = a.reference(); const auto& events = r.events();
    const auto scale = LC_Interval::point(events.providers().frame().scale());
    const auto threshold = LC_Interval::point(2) * LC_Interval::point(tolerance) / scale;
    const auto floor = (LC_Interval::point(std::abs(distance)) - LC_Interval::point(2.5) * LC_Interval::point(tolerance)) / scale;
    for (const auto& w : a.witnesses()) {
        REQUIRE(w.cache.span < a.layout().spanCount()); REQUIRE(w.edge < r.edges().size());
        CHECK(w.cache.parameter.lo() >= 0); CHECK(w.cache.parameter.hi() <= 1);
        CHECK(w.cache.parameter.lo() < w.cache.parameter.hi());
        CHECK(w.geometryError.isValid()); CHECK(w.geometryError.hi() <= threshold.lo());
        CHECK(w.clearanceLowerBound.lo() >= floor.hi());
        const auto& edge = r.edges()[w.edge]; const auto& span = r.spans()[edge.span];
        CHECK(span.visibility == geo::ReferenceVisibility::Visible);
        if (w.kind == geo::CacheWitnessKind::Interior) {
            CHECK(w.occurrence.value == std::size_t(-1)); CHECK(w.node.value == std::size_t(-1));
            CHECK(w.referenceParameter.lo() >= span.interior.lo()); CHECK(w.referenceParameter.hi() <= span.interior.hi());
            CHECK_FALSE(w.candidateNodeError.isValid());
            const auto expected = LC_Interval::point(std::abs(distance)) / scale - w.geometryError;
            CHECK(w.clearanceLowerBound.lo() == expected.lo()); CHECK(w.clearanceLowerBound.hi() == expected.hi());
        } else {
            const bool start = w.kind == geo::CacheWitnessKind::StartFringe;
            CHECK(w.occurrence.value == (start ? span.start.value : span.end.value));
            CHECK(w.node.value == (start ? edge.start.value : edge.end.value));
            CHECK(w.candidateNodeError.isValid()); CHECK(w.geometryError.hi() >= w.candidateNodeError.hi());
            const auto expected = LC_Interval::point(std::abs(distance)) / scale - w.candidateNodeError;
            CHECK(w.clearanceLowerBound.lo() == expected.lo()); CHECK(w.clearanceLowerBound.hi() == expected.hi());
        }
    }
    for (std::size_t span = 0; span < a.layout().spanCount(); ++span) {
        double covered = 0;
        while (covered < 1) {
            double next = covered;
            for (const auto& w : a.witnesses()) {
                if (w.cache.span == span && w.cache.parameter.lo() <= covered) { next = std::max(next, w.cache.parameter.hi()); }
            }
            REQUIRE(next > covered); covered = next;
        }
    }
    // Candidate refinement adds two spans per split, including discarded proof branches.
    CHECK(result.work.cacheSpans >= result.layoutWork.cacheSpans);
    CHECK((result.work.cacheSpans - result.layoutWork.cacheSpans) % 2 == 0);
    CHECK(result.work.vertices > result.referenceWork.vertices);
    CHECK(result.work.distancePairs > result.referenceWork.distancePairs);
}
}

TEST_CASE("Cache chord primitives enclose restricted linear endpoints and points with zero residual",
          "[lc-offsetcurve][geometry][cache-bounds]") {
    auto source = basis(); source.center = {100, -200};
    const auto frame = geo::ReferenceFrame::create(source, 3); REQUIRE(frame);
    geo::CacheSpan span; span.degree = 1; span.controls[0] = {97, -204}; span.controls[1] = {114, -185};
    for (const auto& range : {LC_Interval::hull(0, 1), LC_Interval::hull(.125, .875), LC_Interval::point(.5),
                              LC_Interval::hull(.5, std::nextafter(.5, 1.0))}) {
        geo::CurveChordBounds out; REQUIRE(geo::boundCacheChord(span, *frame, range.lo(), range.hi(), out));
        const auto a = cachePointOracle(span, source, *frame, Quad(range.lo()));
        const auto b = cachePointOracle(span, source, *frame, Quad(range.hi()));
        checkEnclosure(out.startX, a[0]); checkEnclosure(out.startY, a[1]);
        checkEnclosure(out.endX, b[0]); checkEnclosure(out.endY, b[1]);
        CHECK(out.deviation.lo() == 0); CHECK(out.deviation.hi() == 0);
        for (unsigned i = 0; i <= 16; ++i) {
            const Quad t = Quad(range.lo()) + (Quad(range.hi()) - Quad(range.lo())) * i / 16;
            const auto p = cachePointOracle(span, source, *frame, t);
            checkEnclosure(out.x, p[0]); checkEnclosure(out.y, p[1]);
        }
    }
}

TEST_CASE("Cache cubic chord residual bounds independent de Casteljau curves including narrow and point ranges",
          "[lc-offsetcurve][geometry][cache-bounds]") {
    const auto source = basis(); const auto frame = geo::ReferenceFrame::create(source, 3); REQUIRE(frame);
    geo::CacheSpan span; span.degree = 3; span.controls = {{{0, 0}, {3, 8}, {7, -5}, {10, 2}}};
    for (const auto& range : {LC_Interval::hull(0, 1), LC_Interval::hull(.125, .875), LC_Interval::point(.5),
                              LC_Interval::hull(.5, std::nextafter(.5, 1.0)), LC_Interval::hull(0, 1e-100)}) {
        geo::CurveChordBounds out; REQUIRE(geo::boundCacheChord(span, *frame, range.lo(), range.hi(), out));
        const auto a = cachePointOracle(span, source, *frame, Quad(range.lo()));
        const auto b = cachePointOracle(span, source, *frame, Quad(range.hi()));
        checkEnclosure(out.startX, a[0]); checkEnclosure(out.startY, a[1]);
        checkEnclosure(out.endX, b[0]); checkEnclosure(out.endY, b[1]);
        std::array<std::array<EventRational, 2>, 4> controls, restricted;
        for (unsigned j = 0; j <= 3; ++j) {
            controls[j] = {{(EventRational(span.controls[j].x) - EventRational(source.center.x)) / EventRational(frame->scale()),
                            (EventRational(span.controls[j].y) - EventRational(source.center.y)) / EventRational(frame->scale())}};
        }
        // Independent blossom restriction: Q_j = B(a,...,a,b,...,b), with j
        // copies of b. This does not use the production split-at-b then a/b.
        for (unsigned j = 0; j <= 3; ++j) {
            auto points = controls;
            for (unsigned stage = 0; stage < 3; ++stage) {
                const EventRational parameter(stage < 3 - j ? range.lo() : range.hi());
                for (unsigned k = 0; k < 3 - stage; ++k) {
                    for (unsigned coordinate = 0; coordinate < 2; ++coordinate) {
                        points[k][coordinate] = (1 - parameter) * points[k][coordinate] + parameter * points[k + 1][coordinate];
                    }
                }
            }
            restricted[j] = points[0];
        }
        REQUIRE(std::isfinite(out.deviation.hi())); REQUIRE(out.deviation.hi() >= 0);
        const EventRational squaredError = EventRational(out.deviation.hi()) * EventRational(out.deviation.hi());
        for (unsigned j = 0; j <= 3; ++j) {
            CHECK(EventRational(out.x.lo()) <= restricted[j][0]); CHECK(restricted[j][0] <= EventRational(out.x.hi()));
            CHECK(EventRational(out.y.lo()) <= restricted[j][1]); CHECK(restricted[j][1] <= EventRational(out.y.hi()));
            std::array<EventRational, 2> residual;
            for (unsigned coordinate = 0; coordinate < 2; ++coordinate) {
                const EventRational chordControl = ((3 - j) * restricted[0][coordinate] + j * restricted[3][coordinate]) / 3;
                residual[coordinate] = restricted[j][coordinate] - chordControl;
            }
            // Bernstein convexity encloses every curve point and every
            // curve-minus-chord value, not just the samples below.
            CHECK(residual[0] * residual[0] + residual[1] * residual[1] <= squaredError);
        }
        for (unsigned i = 0; i <= 32; ++i) {
            const Quad u = Quad(i) / 32, t = Quad(range.lo()) + (Quad(range.hi()) - Quad(range.lo())) * u;
            const auto p = cachePointOracle(span, source, *frame, t);
            checkEnclosure(out.x, p[0]); checkEnclosure(out.y, p[1]);
            const Quad x = p[0] - ((1 - u) * a[0] + u * b[0]);
            const Quad y = p[1] - ((1 - u) * a[1] + u * b[1]);
            CHECK(sqrt(x * x + y * y) <= Quad(out.deviation.hi()));
        }
    }
}

TEST_CASE("Chord primitive invalid queries clear every field and never project bad controls",
          "[lc-offsetcurve][geometry][cache-bounds]") {
    const auto frame = geo::ReferenceFrame::create(basis(), 3); REQUIRE(frame);
    geo::CacheSpan span; span.degree = 1; span.controls[0] = {0, 0}; span.controls[1] = {1, 1};
    for (const auto& range : {LC_Interval::hull(-1, 0), LC_Interval::hull(1, 2),
                              LC_Interval::point(std::numeric_limits<double>::infinity())}) {
        geo::CurveChordBounds out; REQUIRE(geo::boundCacheChord(span, *frame, 0, 1, out));
        CHECK_FALSE(geo::boundCacheChord(span, *frame, range.lo(), range.hi(), out)); checkChordCleared(out);
    }
    geo::CurveChordBounds out; REQUIRE(geo::boundCacheChord(span, *frame, 0, 1, out));
    CHECK_FALSE(geo::boundCacheChord(span, *frame, .75, .25, out)); checkChordCleared(out);
    span.controls[1].z = 1e-200;
    CHECK_FALSE(geo::boundCacheChord(span, *frame, 0, 1, out)); checkChordCleared(out);
    span.degree = 2; CHECK_FALSE(geo::boundCacheChord(span, *frame, 0, 1, out)); checkChordCleared(out);
    REQUIRE(geo::boundReferenceChord(*frame, 0, .1, out));
    CHECK_FALSE(geo::boundReferenceChord(*frame, .1, 0, out)); checkChordCleared(out);
    CHECK_FALSE(geo::boundReferenceChord(*frame, 0, frame->parameterEnd() + 1, out)); checkChordCleared(out);
}

TEST_CASE("Reference circle chord error encloses analytic sagitta and full interpolation error near the seam",
          "[lc-offsetcurve][geometry][cache-bounds]") {
    const auto source = basis(1, 1); const auto frame = geo::ReferenceFrame::create(source, 1); REQUIRE(frame);
    const double end = frame->parameterEnd();
    std::array<bool, 4> cardinalSeen{};
    for (const auto& range : {LC_Interval::hull(0, .25), LC_Interval::hull(1e-10, 2e-10),
                              LC_Interval::hull(end - .01, end), LC_Interval::point(end),
                              LC_Interval::hull(0, 1.5), LC_Interval::hull(.5, 4.5), LC_Interval::hull(0, end)}) {
        geo::CurveChordBounds out; REQUIRE(geo::boundReferenceChord(*frame, range.lo(), range.hi(), out));
        const auto a = referenceOracle(source, *frame, 1, range.lo(), true);
        const auto b = referenceOracle(source, *frame, 1, range.hi(), true);
        checkEnclosure(out.startX, a[0]); checkEnclosure(out.startY, a[1]);
        checkEnclosure(out.endX, b[0]); checkEnclosure(out.endY, b[1]);
        const Quad angle = idealDirectedSweep(source, true) * (Quad(range.hi()) - Quad(range.lo())) / Quad(end);
        const Quad sagitta = Quad(2) / Quad(frame->scale()) * 2 * pow(sin(angle / 4), 2);
        CHECK(sagitta <= Quad(out.deviation.hi()));
        const Quad pi = boost::math::constants::pi<Quad>(), radius = Quad(2) / Quad(frame->scale());
        const Quad firstAngle = Quad(M_PI_4) + 2 * pi * Quad(range.lo()) / Quad(end);
        const Quad lastAngle = Quad(M_PI_4) + 2 * pi * Quad(range.hi()) / Quad(end);
        std::array<Quad, 2> minimum{{std::min(a[0], b[0]), std::min(a[1], b[1])}};
        std::array<Quad, 2> maximum{{std::max(a[0], b[0]), std::max(a[1], b[1])}};
        const std::array<std::array<Quad, 2>, 4> cardinals{{{{radius, 0}}, {{0, radius}}, {{-radius, 0}}, {{0, -radius}}}};
        // Coordinate extrema over the WHOLE circular interval occur only at
        // endpoints or cardinal angles. Use symbolic cardinal coordinates, not libm zeros.
        for (int k = 0; k <= 8; ++k) {
            const Quad axis = Quad(k) * pi / 2;
            if (axis < firstAngle || lastAngle < axis) { continue; }
            const unsigned cardinal = unsigned(k) % 4;
            cardinalSeen[cardinal] = true;
            for (unsigned coordinate = 0; coordinate < 2; ++coordinate) {
                minimum[coordinate] = std::min(minimum[coordinate], cardinals[cardinal][coordinate]);
                maximum[coordinate] = std::max(maximum[coordinate], cardinals[cardinal][coordinate]);
            }
        }
        checkEnclosure(out.x, minimum[0]); checkEnclosure(out.x, maximum[0]);
        checkEnclosure(out.y, minimum[1]); checkEnclosure(out.y, maximum[1]);
        for (unsigned i = 0; i <= 32; ++i) {
            const double t = range.lo() + (range.hi() - range.lo()) * i / 32;
            const auto p = referenceOracle(source, *frame, 1, t, true);
            checkEnclosure(out.x, p[0]); checkEnclosure(out.y, p[1]);
            const Quad u = range.lo() == range.hi() ? Quad(0) : (Quad(t) - Quad(range.lo())) / (Quad(range.hi()) - Quad(range.lo()));
            const Quad x = p[0] - ((1 - u) * a[0] + u * b[0]);
            const Quad y = p[1] - ((1 - u) * a[1] + u * b[1]);
            CHECK(sqrt(x * x + y * y) <= Quad(out.deviation.hi()));
        }
    }
    for (const bool seen : cardinalSeen) { CHECK(seen); }
}

TEST_CASE("Reference ellipse chord bounds retain reversed wrapped near cusp and extreme scale geometry",
          "[lc-offsetcurve][geometry][cache-bounds]") {
    for (const double radius : {1e-280, 100.0, 1e280}) {
        for (const bool reversed : {false, true}) {
            auto source = basis(radius, .5, 6.0, .2, reversed);
            if (reversed) { std::swap(source.angle1, source.angle2); }
            const double distance = -.251 * radius;
            const auto frame = geo::ReferenceFrame::create(source, distance); REQUIRE(frame);
            for (const auto& range : {std::array<double, 2>{{.125 * frame->parameterEnd(), .875 * frame->parameterEnd()}},
                                       std::array<double, 2>{{0, frame->parameterEnd()}}}) {
            const double a = range[0], b = range[1];
            geo::CurveChordBounds out; REQUIRE(geo::boundReferenceChord(*frame, a, b, out));
            const auto startBounds = oracleEllipseOffset(source, distance, frame->scale(), frame->parameterEnd(), OutwardOracle{EventRational(a)});
            const auto endBounds = oracleEllipseOffset(source, distance, frame->scale(), frame->parameterEnd(), OutwardOracle{EventRational(b)});
            CHECK(EventRational(out.startX.lo()) <= startBounds[0].lo); CHECK(startBounds[0].hi <= EventRational(out.startX.hi()));
            CHECK(EventRational(out.startY.lo()) <= startBounds[1].lo); CHECK(startBounds[1].hi <= EventRational(out.startY.hi()));
            CHECK(EventRational(out.endX.lo()) <= endBounds[0].lo); CHECK(endBounds[0].hi <= EventRational(out.endX.hi()));
            CHECK(EventRational(out.endY.lo()) <= endBounds[1].lo); CHECK(endBounds[1].hi <= EventRational(out.endY.hi()));
            REQUIRE(std::isfinite(out.deviation.hi())); REQUIRE(out.deviation.hi() >= 0);
            const EventRational errorSquared = EventRational(out.deviation.hi()) * EventRational(out.deviation.hi());
            EventRational covered = a;
            for (unsigned cell = 0; cell < 128; ++cell) {
                const EventRational lo = EventRational(a) + (EventRational(b) - EventRational(a)) * cell / 128;
                const EventRational hi = EventRational(a) + (EventRational(b) - EventRational(a)) * (cell + 1) / 128;
                REQUIRE(lo == covered); REQUIRE(lo < hi); covered = hi;
                const auto point = oracleEllipseOffset(source, distance, frame->scale(), frame->parameterEnd(), OutwardOracle{lo, hi});
                CHECK(EventRational(out.x.lo()) <= point[0].lo); CHECK(point[0].hi <= EventRational(out.x.hi()));
                CHECK(EventRational(out.y.lo()) <= point[1].lo); CHECK(point[1].hi <= EventRational(out.y.hi()));
                const OutwardOracle u{EventRational{cell} / 128, EventRational{cell + 1} / 128}, one{EventRational{1}};
                std::array<EventRational, 2> error;
                for (unsigned coordinate = 0; coordinate < 2; ++coordinate) {
                    const auto residual = point[coordinate] - ((one - u) * startBounds[coordinate] + u * endBounds[coordinate]);
                    error[coordinate] = std::max(EventRational{abs(residual.lo)}, EventRational{abs(residual.hi)});
                }
                // The independent outward boxes enclose ALL Q(t)-L(t) values
                // on this cell; the adjacent rational cells cover [a,b] exactly.
                CHECK(error[0] * error[0] + error[1] * error[1] <= errorSquared);
            }
            CHECK(covered == EventRational(b));
            const auto first = referenceOracle(source, *frame, distance, a, true);
            const auto last = referenceOracle(source, *frame, distance, b, true);
            for (unsigned i = 0; i <= 32; ++i) {
                const double t = a + (b - a) * i / 32;
                const auto p = referenceOracle(source, *frame, distance, t, true);
                checkEnclosure(out.x, p[0]); checkEnclosure(out.y, p[1]);
                const Quad u = (Quad(t) - Quad(a)) / (Quad(b) - Quad(a));
                const Quad x = p[0] - ((1 - u) * first[0] + u * last[0]);
                const Quad y = p[1] - ((1 - u) * first[1] + u * last[1]);
                CHECK(sqrt(x * x + y * y) <= Quad(out.deviation.hi()));
            }
            }
        }
    }
}

TEST_CASE("Cache bounds certify normalized linear and cubic arc caches with both exact endpoint incidences",
          "[lc-offsetcurve][geometry][cache-bounds]") {
    const auto source = basis(1, 1, .25, .2501);
    for (const std::size_t degree : {1u, 3u}) {
        const auto data = arcCache(source, 1, degree); geo::WorkBudget budget;
        const auto result = geo::analyseCacheBounds(data, source, 1, .01, budget);
        CAPTURE(degree, result.status, result.referenceStatus, result.referenceReason);
        checkBoundsRelation(result, 1, .01); checkWork(result.work, budget.consumed());
        bool start = false, end = false, interior = false;
        for (const auto& w : result.analysis->witnesses()) {
            start = start || w.kind == geo::CacheWitnessKind::StartFringe;
            end = end || w.kind == geo::CacheWitnessKind::EndFringe;
            interior = interior || w.kind == geo::CacheWitnessKind::Interior;
        }
        CHECK(start); CHECK(end); CHECK(interior);
    }
}

TEST_CASE("Cache bounds are setwise only and preserve affine knots reversal and folded control nets for topology",
          "[lc-offsetcurve][geometry][cache-bounds]") {
    const auto source = basis(1, 1, .25, .2501);
    auto data = arcCache(source, 1, 3);
    for (const bool reverse : {false, true}) {
        auto cache = data;
        cache.knotslist = {-7, -7, -7, -7, std::nextafter(-7.0, 0.0), std::nextafter(-7.0, 0.0),
                           std::nextafter(-7.0, 0.0), std::nextafter(-7.0, 0.0)};
        if (reverse) { std::reverse(cache.controlPoints.begin(), cache.controlPoints.end()); }
        geo::WorkBudget budget; const auto result = geo::analyseCacheBounds(cache, source, 1, .01, budget);
        checkBoundsRelation(result, 1, .01);
        CHECK(result.analysis->layout().spans[0].parameter.lo() == -7);
    }
    const auto a = data.controlPoints[0], b = data.controlPoints[3];
    data.controlPoints[1] = {a.x + 3 * (b.x - a.x), a.y + 3 * (b.y - a.y)};
    data.controlPoints[2] = {a.x - 2 * (b.x - a.x), a.y - 2 * (b.y - a.y)};
    geo::WorkBudget budget; const auto folded = geo::analyseCacheBounds(data, source, 1, .01, budget);
    checkBoundsRelation(folded, 1, .01);
    const auto& controls = folded.analysis->layout().spans[0].controls;
    const double dx = b.x - a.x, dy = b.y - a.y;
    CHECK((controls[1].x - controls[0].x) * dx + (controls[1].y - controls[0].y) * dy > 0);
    CHECK((controls[2].x - controls[1].x) * dx + (controls[2].y - controls[1].y) * dy < 0);
}

TEST_CASE("Cache fringe bounds charge full fringe geometry separately from exact node source clearance",
          "[lc-offsetcurve][geometry][cache-bounds]") {
    const auto source = basis(1, 1, .25, .2501); auto data = arcCache(source, 1, 1);
    data.controlPoints[1] = data.controlPoints[0]; // Degenerate chord: endpoint-ball proof, not injectivity.
    geo::WorkBudget budget; const auto result = geo::analyseCacheBounds(data, source, 1, .01, budget);
    checkBoundsRelation(result, 1, .01);
    bool nonzeroFringe = false;
    for (const auto& w : result.analysis->witnesses()) {
        if (w.kind != geo::CacheWitnessKind::Interior) {
            nonzeroFringe = nonzeroFringe || w.geometryError.hi() > w.candidateNodeError.hi();
            CHECK(w.clearanceLowerBound.lo() > .97 / result.analysis->reference().events().providers().frame().scale());
        }
    }
    CHECK(nonzeroFringe);
}

TEST_CASE("Cache bounds preserve the two whole seam incidences without a jet query across the seam",
          "[lc-offsetcurve][geometry][cache-bounds]") {
    auto cache = cacheLayoutFixture(1, {0, 1, 2, 3, 4});
    cache.controlPoints = {{2, 0}, {0, 2}, {-2, 0}, {0, -2}, {2, 0}};
    geo::WorkBudget budget; const auto result = geo::analyseCacheBounds(cache, basis(1, 1), 1, 2.5, budget);
    CAPTURE(result.status, result.work.deepest); checkBoundsRelation(result, 1, 2.5);
    const auto& events = result.analysis->reference().events(); REQUIRE(events.seam());
    const auto& reference = result.analysis->reference();
    const auto node = events.occurrences()[events.seam()->start.value].node;
    REQUIRE(events.occurrences()[events.seam()->end.value].node.value == node.value);
    REQUIRE(events.seam()->start.value != events.seam()->end.value);
    const auto& incidence = reference.incidence()[node.value];
    REQUIRE(incidence.seam); REQUIRE_FALSE(incidence.incoming.empty()); REQUIRE_FALSE(incidence.outgoing.empty());
    bool start = false, end = false;
    for (const auto& w : result.analysis->witnesses()) {
        if (w.occurrence.value == events.seam()->start.value) {
            CHECK(w.kind == geo::CacheWitnessKind::StartFringe); CHECK(w.node.value == node.value);
            CHECK(std::find(incidence.outgoing.begin(), incidence.outgoing.end(), w.edge) != incidence.outgoing.end());
            CHECK(reference.spans()[reference.edges()[w.edge].span].start.value == events.seam()->start.value);
            start = true;
        }
        if (w.occurrence.value == events.seam()->end.value) {
            CHECK(w.kind == geo::CacheWitnessKind::EndFringe); CHECK(w.node.value == node.value);
            CHECK(std::find(incidence.incoming.begin(), incidence.incoming.end(), w.edge) != incidence.incoming.end());
            CHECK(reference.spans()[reference.edges()[w.edge].span].end.value == events.seam()->end.value);
            end = true;
        }
        CHECK(w.referenceParameter.lo() >= 0); CHECK(w.referenceParameter.hi() <= events.providers().frame().parameterEnd());
    }
    for (const auto edge : incidence.outgoing) {
        CHECK(std::any_of(result.analysis->witnesses().begin(), result.analysis->witnesses().end(), [&](const auto& w) {
            return w.edge == edge && w.kind == geo::CacheWitnessKind::StartFringe &&
                w.occurrence.value == events.seam()->start.value && w.node.value == node.value;
        }));
    }
    for (const auto edge : incidence.incoming) {
        CHECK(std::any_of(result.analysis->witnesses().begin(), result.analysis->witnesses().end(), [&](const auto& w) {
            return w.edge == edge && w.kind == geo::CacheWitnessKind::EndFringe &&
                w.occurrence.value == events.seam()->end.value && w.node.value == node.value;
        }));
    }
    CHECK(start); CHECK(end);
}

TEST_CASE("Cache bounds reject an endpoint midpoint impostor with a large between sample cubic excursion",
          "[lc-offsetcurve][geometry][cache-bounds]") {
    const auto source = basis(1, 1, .25, .2501); auto data = arcCache(source, 1, 3);
    const auto a = data.controlPoints[0], b = data.controlPoints[3];
    data.controlPoints[1] = {(2 * a.x + b.x) / 3, (2 * a.y + b.y) / 3 + 10};
    data.controlPoints[2] = {(a.x + 2 * b.x) / 3, (a.y + 2 * b.y) / 3 - 10};
    geo::WorkBudget budget; const auto result = geo::analyseCacheBounds(data, source, 1, .01, budget);
    CHECK(result.status != geo::CacheBoundsStatus::Ok); CHECK_FALSE(result.analysis);
    checkWork(result.work, budget.consumed());
}

TEST_CASE("Cache bounds distinguish separated one sided and exact threshold pairs with atomic refusal",
          "[lc-offsetcurve][geometry][cache-bounds]") {
    const auto source = basis(1, 1, 0, 1e-4);
    for (const double x : {2.125, 2.25, 2.375}) {
        auto data = cacheLayoutFixture(1, {0, 1}); data.controlPoints = {{x, 0}, {x, 0}};
        geo::WorkBudget budget{LC_OffsetCheckLimits{4096, 65536, 1u << 20, 1}};
        const auto result = geo::analyseCacheBounds(data, source, 1, .125, budget);
        CAPTURE(x, result.status, result.referenceStatus);
        REQUIRE(result.referenceStatus == LC_IntersectionStatus::Ok);
        if (x < 2.25) { checkBoundsRelation(result, 1, .125); }
        else {
            CHECK(result.status == (x == 2.25 ? geo::CacheBoundsStatus::LimitExceeded : geo::CacheBoundsStatus::OffCurve));
            CHECK_FALSE(result.analysis);
        }
        checkWork(result.work, budget.consumed());
    }
}

TEST_CASE("Cache bounds cannot use a hidden point or cutter when no visible reference witness exists",
          "[lc-offsetcurve][geometry][cache-bounds]") {
    auto cache = cacheLayoutFixture(1, {0, 1}); cache.controlPoints = {{0, 0}, {0, 0}};
    geo::WorkBudget budget; const auto result = geo::analyseCacheBounds(cache, basis(10, .5), -10, 1e-5, budget);
    REQUIRE(result.referenceStatus == LC_IntersectionStatus::Ok);
    CHECK(result.status == geo::CacheBoundsStatus::OffCurve); CHECK_FALSE(result.analysis);
    checkWork(result.work, result.referenceWork); checkWork(result.work, budget.consumed());
    cache.controlPoints = {{1e5, 1e5}, {1e5 + 1, 1e5 - 1}};
    geo::WorkBudget far; const auto separated = geo::analyseCacheBounds(cache, basis(1, 1, 0, .01), 1, .01, far);
    REQUIRE(separated.referenceStatus == LC_IntersectionStatus::Ok);
    CHECK(separated.status == geo::CacheBoundsStatus::OffCurve); CHECK_FALSE(separated.analysis);
    CHECK(separated.work.distancePairs > separated.referenceWork.distancePairs);
}

TEST_CASE("Cache bounds share one ledger across layout reference and bounds and stop at each failed stage",
          "[lc-offsetcurve][geometry][cache-bounds]") {
    const auto source = basis(1, 1, .25, .2501); auto data = arcCache(source, 1, 3);
    geo::WorkBudget budget; REQUIRE(budget.consumeVertices(2)); REQUIRE(budget.consumePairs(3));
    const auto result = geo::analyseCacheBounds(data, source, 1, .01, budget);
    checkBoundsRelation(result, 1, .01); checkWork(result.work, budget.consumed());
    checkWork(result.layoutWork, {1, 6, 3, 0});
    geo::WorkBudget replay; REQUIRE(replay.consumePieces(1)); REQUIRE(replay.consumeVertices(6)); REQUIRE(replay.consumePairs(3));
    const auto reference = geo::analyseReferenceComponents(source, 1, .01, replay);
    REQUIRE(reference.status == LC_IntersectionStatus::Ok); checkWork(result.referenceWork, replay.consumed());
    data.weights.clear(); geo::WorkBudget badLayout;
    const auto bad = geo::analyseCacheBounds(data, source, 1, .01, badLayout);
    CHECK(bad.status == geo::CacheBoundsStatus::InvalidLayout); CHECK_FALSE(bad.analysis); checkWork(bad.work, {});
    data = cacheLayoutFixture(1, {0, 1}); geo::WorkBudget badReference;
    const auto refused = geo::analyseCacheBounds(data, basis(10, .5), -5, 1e-5, badReference);
    CHECK(refused.status == geo::CacheBoundsStatus::ReferenceRefusal); CHECK_FALSE(refused.analysis);
    CHECK(refused.referenceStatus == LC_IntersectionStatus::AmbiguousTopology);
    checkWork(refused.work, refused.referenceWork);
}

TEST_CASE("Cache bounds reject hidden geometry even when the restricted source has visible alternatives",
          "[lc-offsetcurve][geometry][cache-bounds]") {
    const auto source = basis(100, .05, M_PI / 12, 7 * M_PI / 4);
    const double distance = -4, tolerance = 1e-5;
    geo::WorkBudget referenceBudget;
    const auto prepared = geo::analyseReferenceComponents(source, distance, tolerance, referenceBudget);
    REQUIRE(prepared.status == LC_IntersectionStatus::Ok); REQUIRE(prepared.analysis);
    REQUIRE_FALSE(prepared.analysis->edges().empty());
    REQUIRE(prepared.analysis->components().size() == 2);
    const auto hidden = std::find_if(prepared.analysis->spans().begin(), prepared.analysis->spans().end(), [](const auto& span) {
        return span.visibility == geo::ReferenceVisibility::Hidden && span.interior.lo() < span.interior.hi();
    });
    REQUIRE(hidden != prepared.analysis->spans().end());
    const double t = hidden->interior.lo() + .5 * (hidden->interior.hi() - hidden->interior.lo());
    const auto& frame = prepared.analysis->events().providers().frame();
    LC_CurveJet jet; REQUIRE(frame.offsetJet(t, jet));
    LC_Interval wx, wy; REQUIRE(frame.pointToWorld(jet.point, wx, wy));
    const RS_Vector point{wx.lo() + .5 * (wx.hi() - wx.lo()), wy.lo() + .5 * (wy.hi() - wy.lo())};
    const auto oracle = referenceOracle(source, frame, distance, t, true);
    const Quad dx = Quad(point.x) - Quad(source.center.x) - Quad(frame.scale()) * oracle[0];
    const Quad dy = Quad(point.y) - Quad(source.center.y) - Quad(frame.scale()) * oracle[1];
    CHECK(sqrt(dx * dx + dy * dy) < Quad(tolerance));
    CHECK_FALSE(componentVisibilityOracle(source, distance, frame, t));
    auto cache = cacheLayoutFixture(1, {0, 1}); cache.controlPoints = {point, point};
    geo::WorkBudget budget;
    const auto result = geo::analyseCacheBounds(cache, source, distance, tolerance, budget);
    CAPTURE(t, point.x, point.y, result.status, result.referenceStatus, result.referenceReason);
    REQUIRE(result.referenceStatus == LC_IntersectionStatus::Ok);
    CHECK(result.status == geo::CacheBoundsStatus::OffCurve); CHECK_FALSE(result.analysis);
    CHECK(result.work.distancePairs > result.referenceWork.distancePairs);
    checkWork(result.work, budget.consumed());
}

TEST_CASE("Cache bounds clearance uses admitted quarter arc geometry rather than the supporting circle",
          "[lc-offsetcurve][geometry][cache-bounds]") {
    const auto source = basis(1, 1, 0, M_PI_2);
    auto cache = cacheLayoutFixture(1, {0, 1}); cache.controlPoints = {{-.25, -.5}, {-.125, -.5}};
    geo::WorkBudget budget;
    const auto result = geo::analyseCacheBounds(cache, source, 1, 2, budget);
    CAPTURE(result.status, result.referenceStatus, result.referenceReason);
    checkBoundsRelation(result, 1, 2);
    const auto& frame = result.analysis->reference().events().providers().frame();
    std::size_t interiorSamples = 0;
    for (const auto& witness : result.analysis->witnesses()) {
        if (witness.kind != geo::CacheWitnessKind::Interior) { continue; }
        for (unsigned i = 0; i <= 8; ++i) {
            const Quad u = Quad(witness.cache.parameter.lo()) +
                (Quad(witness.cache.parameter.hi()) - Quad(witness.cache.parameter.lo())) * i / 8;
            const Quad x = Quad(-.25) + u * Quad(.125), y(-.5);
            // x,y are negative and x>y. Over this admitted arc, the nearest
            // source endpoint is (1,0); the radial whole-circle foot is not admitted.
            const Quad restricted = sqrt(pow(x - 1, 2) + y * y);
            const Quad whole = abs(sqrt(x * x + y * y) - 1);
            CHECK(restricted > 1); CHECK(whole < 1);
            CHECK(Quad(witness.clearanceLowerBound.lo()) * Quad(frame.scale()) <= restricted);
            for (unsigned j = 0; j <= 32; ++j) {
                const Quad theta = Quad(source.angle1) + (Quad(source.angle2) - Quad(source.angle1)) * j / 32;
                const Quad sampled = sqrt(pow(x - cos(theta), 2) + pow(y - sin(theta), 2));
                CHECK(sampled >= restricted);
            }
            ++interiorSamples;
        }
    }
    CHECK(interiorSamples > 0);
    checkWork(result.work, budget.consumed());
}

TEST_CASE("Cache bounds discard staged witnesses on later unrepresentable candidate geometry",
          "[lc-offsetcurve][geometry][cache-bounds]") {
    const auto source = basis(1e-280, 1, .25, .2501);
    const double distance = 1e-280, tolerance = 1e-282;
    const auto prefix = arcCache(source, distance, 1);
    geo::WorkBudget prefixBudget;
    const auto accepted = geo::analyseCacheBounds(prefix, source, distance, tolerance, prefixBudget);
    checkBoundsRelation(accepted, distance, tolerance);
    REQUIRE_FALSE(accepted.analysis->witnesses().empty());
    auto cache = cacheLayoutFixture(1, {0, 1, 2});
    cache.controlPoints = {prefix.controlPoints[0], prefix.controlPoints[1], {1e308, 1e308}};
    geo::WorkBudget budget;
    const auto result = geo::analyseCacheBounds(cache, source, distance, tolerance, budget);
    CAPTURE(result.status, result.referenceStatus);
    REQUIRE(result.layoutStatus == geo::CacheLayoutStatus::Ok);
    REQUIRE(result.referenceStatus == LC_IntersectionStatus::Ok);
    CHECK(result.status == geo::CacheBoundsStatus::Ambiguous); CHECK_FALSE(result.analysis);
    // The prefix enters the same comparisons and stages all its witnesses; the
    // next pending pair is charged before its world-to-local conversion refuses.
    CHECK(result.work.vertices - result.referenceWork.vertices ==
        accepted.work.vertices - accepted.referenceWork.vertices + 1);
    CHECK(result.work.distancePairs > result.referenceWork.distancePairs);
    CHECK(budget.status() == geo::BudgetStatus::Ok);
    checkWork(result.work, budget.consumed());
}

TEST_CASE("Cache bounds exact and one less vertex pair and depth allowances never publish partial witnesses",
          "[lc-offsetcurve][geometry][cache-bounds]") {
    const auto source = basis(1, 1, 0, 1e-4);
    const double tolerance = .125;
    const Quad angle(source.angle2), threshold = Quad(2) * Quad(tolerance);
    // Strictly inside the far-end threshold, but closer than the initial chord-error bound.
    const double x = (2 * cos(angle) + sqrt(threshold * threshold - 4 * pow(sin(angle), 2)) - Quad("1e-11")).convert_to<double>();
    auto cache = cacheLayoutFixture(1, {0, 1}); cache.controlPoints = {{x, 0}, {x, 0}};
    const Quad dx = Quad(x) - 2 * cos(angle), dy = 2 * sin(angle);
    REQUIRE(sqrt(dx * dx + dy * dy) < threshold);
    geo::WorkBudget baseline; const auto complete = geo::analyseCacheBounds(cache, source, 1, tolerance, baseline);
    REQUIRE(complete.status == geo::CacheBoundsStatus::Ok); REQUIRE(complete.analysis);
    CAPTURE(complete.work.deepest, complete.referenceWork.deepest);
    REQUIRE(complete.work.deepest > complete.referenceWork.deepest);
    for (unsigned dimension = 0; dimension < 3; ++dimension) {
        for (const bool less : {false, true}) {
            LC_OffsetCheckLimits limits{complete.work.cacheSpans, complete.work.vertices, complete.work.distancePairs, complete.work.deepest};
            if (dimension == 0) { limits.maxVertices -= unsigned(less); }
            if (dimension == 1) { limits.maxDistancePairs -= unsigned(less); }
            if (dimension == 2) { limits.maxDepth -= unsigned(less); }
            geo::WorkBudget budget{limits}; const auto result = geo::analyseCacheBounds(cache, source, 1, tolerance, budget);
            CAPTURE(dimension, less, result.status);
            REQUIRE(result.referenceStatus == LC_IntersectionStatus::Ok);
            CHECK(result.status == (less ? geo::CacheBoundsStatus::LimitExceeded : geo::CacheBoundsStatus::Ok));
            CHECK(bool(result.analysis) == !less); checkWork(result.work, budget.consumed());
            if (less) {
                CHECK(result.work.distancePairs > result.referenceWork.distancePairs);
                const auto spent = budget.consumed(); const auto again = geo::analyseCacheBounds(cache, source, 1, tolerance, budget);
                CHECK(again.status == geo::CacheBoundsStatus::LimitExceeded); CHECK_FALSE(again.analysis); checkWork(again.work, spent);
            }
        }
    }
    LC_OffsetCheckLimits limits{complete.work.cacheSpans, complete.work.vertices, complete.work.distancePairs, complete.work.deepest};
    geo::WorkBudget spent{limits}; REQUIRE(spent.consumeVertices(1));
    const auto refused = geo::analyseCacheBounds(cache, source, 1, tolerance, spent);
    CHECK(refused.status == geo::CacheBoundsStatus::LimitExceeded); CHECK_FALSE(refused.analysis);
}

TEST_CASE("Cache bounds preserve every distinct visible edge occurrence incidence at shared crossing nodes",
          "[lc-offsetcurve][geometry][cache-bounds]") {
    const auto source = basis(100, .5, M_PI / 18, 35 * M_PI / 18);
    auto cache = cacheLayoutFixture(1, {0, 1}); cache.controlPoints = {{0, 0}, {0, 0}};
    // A deliberately coarse setwise relation, not a cache-topology or traversal certificate.
    geo::WorkBudget budget;
    const auto result = geo::analyseCacheBounds(cache, source, -49.5, 500, budget);
    CAPTURE(result.status, result.referenceStatus, result.referenceReason, result.eventReason);
    checkBoundsRelation(result, -49.5, 500);
    const auto& reference = result.analysis->reference(); const auto& events = reference.events();
    std::size_t sharedCrossings = 0, distinctIncidences = 0;
    for (const auto& row : reference.incidence()) {
        if (!row.crossing || row.incoming.size() + row.outgoing.size() < 2) { continue; }
        std::vector<std::size_t> ownOccurrences;
        const auto checkIncidence = [&](const std::size_t edgeIndex, const bool start) {
            const auto& edge = reference.edges()[edgeIndex]; const auto& span = reference.spans()[edge.span];
            const auto occurrence = start ? span.start : span.end;
            REQUIRE(events.occurrences()[occurrence.value].node.value == row.node.value);
            const auto kind = start ? geo::CacheWitnessKind::StartFringe : geo::CacheWitnessKind::EndFringe;
            CHECK(std::any_of(result.analysis->witnesses().begin(), result.analysis->witnesses().end(), [&](const auto& witness) {
                return witness.edge == edgeIndex && witness.kind == kind &&
                    witness.occurrence.value == occurrence.value && witness.node.value == row.node.value;
            }));
            if (std::find(ownOccurrences.begin(), ownOccurrences.end(), occurrence.value) == ownOccurrences.end()) {
                ownOccurrences.push_back(occurrence.value);
            }
            ++distinctIncidences;
        };
        for (const auto edge : row.incoming) { checkIncidence(edge, false); }
        for (const auto edge : row.outgoing) { checkIncidence(edge, true); }
        REQUIRE(ownOccurrences.size() >= 2);
        ++sharedCrossings;
    }
    CHECK(sharedCrossings > 0); CHECK(distinctIncidences >= 2);
    checkWork(result.work, budget.consumed());
}

TEST_CASE("Cache bounds retain proof only witnesses across both restricted source components",
          "[lc-offsetcurve][geometry][cache-bounds]") {
    const auto source = basis(100, .05, M_PI / 12, 7 * M_PI / 4);
    auto cache = cacheLayoutFixture(1, {0, 1}); cache.controlPoints = {{0, 0}, {0, 0}};
    geo::WorkBudget budget;
    const auto result = geo::analyseCacheBounds(cache, source, -4, 500, budget);
    CAPTURE(result.status, result.referenceStatus, result.referenceReason, result.eventReason);
    checkBoundsRelation(result, -4, 500);
    const auto& reference = result.analysis->reference();
    REQUIRE(reference.components().size() == 2);
    for (const auto& component : reference.components()) {
        REQUIRE_FALSE(component.edges.empty());
        for (const auto edge : component.edges) {
            CHECK(std::any_of(result.analysis->witnesses().begin(), result.analysis->witnesses().end(), [&](const auto& witness) {
                return witness.edge == edge && witness.kind == geo::CacheWitnessKind::Interior;
            }));
        }
    }
    // One degenerate candidate can have a complete coarse relation to two components;
    // bounds deliberately neither chooses a component nor certifies a cache chain.
    CHECK(result.analysis->layout().spanCount() == 1);
    checkWork(result.work, budget.consumed());
}

TEST_CASE("Cache bounds preserve stationary endpoint incidences and exclude hidden simple cusp incidences",
          "[lc-offsetcurve][geometry][cache-bounds]") {
    auto cache = cacheLayoutFixture(1, {0, 1}); cache.controlPoints = {{0, 0}, {0, 0}};
    const auto source = basis(100, .5, M_PI / 18, 35 * M_PI / 18);
    geo::WorkBudget budget;
    const auto result = geo::analyseCacheBounds(cache, source, -49.5, 500, budget);
    checkBoundsRelation(result, -49.5, 500);
    const auto& reference = result.analysis->reference(); const auto& events = reference.events();
    std::size_t hiddenCusps = 0;
    for (const auto& critical : events.criticals()) {
        if (critical.kind != geo::CriticalKind::SimpleCusp) { continue; }
        const auto node = events.occurrences()[critical.occurrence.value].node;
        const auto& row = reference.incidence()[node.value];
        REQUIRE(row.cusp); REQUIRE(row.hiddenPoint);
        CHECK(row.incoming.empty()); CHECK(row.outgoing.empty());
        for (const auto& witness : result.analysis->witnesses()) {
            CHECK(witness.occurrence.value != critical.occurrence.value); CHECK(witness.node.value != node.value);
        }
        ++hiddenCusps;
    }
    CHECK(hiddenCusps == 4);
    for (const bool reversed : {false, true}) {
        const auto stationarySource = reversed ? basis(100, .5, 1, 0, true) : basis(100, .5, 0, 1);
        geo::WorkBudget stationaryBudget;
        const auto retained = geo::analyseCacheBounds(cache, stationarySource, -25, 500, stationaryBudget);
        CAPTURE(reversed, retained.status, retained.referenceStatus, retained.referenceReason, retained.eventReason);
        checkBoundsRelation(retained, -25, 500);
        const auto& graph = retained.analysis->reference(); const auto& records = graph.events();
        std::size_t stationaryIncidences = 0;
        for (const auto& critical : records.criticals()) {
            if (critical.kind != geo::CriticalKind::CriticalStationary) { continue; }
            const auto node = records.occurrences()[critical.occurrence.value].node;
            const auto& row = graph.incidence()[node.value];
            CHECK(row.stationary); CHECK_FALSE(row.hiddenPoint);
            CHECK(std::any_of(records.freeEnds().begin(), records.freeEnds().end(), [&](const auto& end) {
                return end.occurrence.value == critical.occurrence.value && end.atStart == !reversed;
            }));
            const auto kind = reversed ? geo::CacheWitnessKind::EndFringe : geo::CacheWitnessKind::StartFringe;
            const auto& incident = reversed ? row.incoming : row.outgoing;
            REQUIRE_FALSE(incident.empty());
            for (const auto edge : incident) {
                CHECK(std::any_of(retained.analysis->witnesses().begin(), retained.analysis->witnesses().end(), [&](const auto& w) {
                    return w.edge == edge && w.kind == kind && w.occurrence.value == critical.occurrence.value && w.node.value == node.value;
                }));
                ++stationaryIncidences;
            }
        }
        CHECK(stationaryIncidences > 0);
        checkWork(retained.work, stationaryBudget.consumed());
    }
}

TEST_CASE("Cache bounds respect pre spent pair allowances and pre latched depth refusal on sticky retries",
          "[lc-offsetcurve][geometry][cache-bounds]") {
    const auto source = basis(1, 1, .25, .2501); const auto cache = arcCache(source, 1, 3);
    geo::WorkBudget baseline;
    const auto complete = geo::analyseCacheBounds(cache, source, 1, .01, baseline);
    checkBoundsRelation(complete, 1, .01);
    for (const bool enough : {false, true}) {
        LC_OffsetCheckLimits limits;
        limits.maxDistancePairs = complete.work.distancePairs + unsigned(enough);
        geo::WorkBudget budget{limits}; REQUIRE(budget.consumePairs(1));
        const auto result = geo::analyseCacheBounds(cache, source, 1, .01, budget);
        CAPTURE(enough, result.status, result.referenceStatus);
        REQUIRE(result.referenceStatus == LC_IntersectionStatus::Ok);
        CHECK(result.status == (enough ? geo::CacheBoundsStatus::Ok : geo::CacheBoundsStatus::LimitExceeded));
        CHECK(bool(result.analysis) == enough); checkWork(result.work, budget.consumed());
        if (!enough) {
            REQUIRE(result.work.distancePairs > result.referenceWork.distancePairs);
            const auto before = budget.consumed();
            const auto retry = geo::analyseCacheBounds(cache, source, 1, .01, budget);
            CHECK(retry.status == geo::CacheBoundsStatus::LimitExceeded); CHECK_FALSE(retry.analysis);
            checkWork(retry.work, before); checkWork(budget.consumed(), before);
        } else { CHECK(result.work.distancePairs == complete.work.distancePairs + 1); }
    }
    // Depth is an observed maximum, not a pool of consumable depth units.
    geo::WorkBudget depth;
    REQUIRE(depth.checkDepth(depth.depthLimit()));
    CHECK_FALSE(depth.checkDepth(depth.depthLimit() + 1));
    const auto before = depth.consumed();
    for (unsigned retry = 0; retry < 2; ++retry) {
        const auto result = geo::analyseCacheBounds(cache, source, 1, .01, depth);
        CHECK(result.status == geo::CacheBoundsStatus::LimitExceeded); CHECK_FALSE(result.analysis);
        checkWork(result.work, before); checkWork(depth.consumed(), before);
    }
}

TEST_CASE("Cache bounds accept actual materialized degree one spline controls without refitting",
          "[lc-offsetcurve][geometry][cache-bounds]") {
    RS_Spline source{nullptr, RS_SplineData(1, false)};
    for (const RS_Vector p : {RS_Vector{0, 0}, RS_Vector{10, 0}, RS_Vector{10, 4}, RS_Vector{0, 4}}) {
        source.addControlPoint(p);
    }
    source.setClosed(true);
    auto options = LC_CurveOffset::makeDirectOptions(source, 1); options.mode = LC_CurveOffsetMode::Trimmed;
    const auto made = LC_CurveOffset::createEntities(source, LC_CurveOffset::makeSideRequest(LC_CurveOffsetSide::Left, 1),
        options, LC_CurveOffset::makeDirectSourceBudget());
    REQUIRE(made.status == LC_CurveOffsetStatus::Ok); REQUIRE(made.entities.size() == 1);
    const auto* spline = dynamic_cast<const RS_Spline*>(made.entities[0].get()); REQUIRE(spline);
    REQUIRE(spline->getData().degree == 1);
    auto target = basis(5, 1); target.center = {5, 2};
    geo::WorkBudget budget;
    const auto result = geo::analyseCacheBounds(spline->getData(), target, 1, 10, budget);
    // Producer compatibility only: this coarse ellipse relation is not a claim
    // that the rectangle is an ellipse cache, nor a public adoption decision.
    checkBoundsRelation(result, 1, 10);
    REQUIRE(result.analysis->layout().spanCount() + 1 == spline->getData().controlPoints.size());
    for (std::size_t i = 0; i < result.analysis->layout().spanCount(); ++i) {
        CHECK(result.analysis->layout().spans[i].degree == 1);
        for (unsigned end = 0; end < 2; ++end) {
            const auto& actual = result.analysis->layout().spans[i].controls[end];
            const auto& expected = spline->getData().controlPoints[i + end];
            CHECK(actual.x == expected.x); CHECK(actual.y == expected.y);
        }
    }
}

TEST_CASE("Cache bounds accept the actual cubic ellipse materializer output as compatibility evidence",
          "[lc-offsetcurve][geometry][cache-bounds]") {
    const auto data = basis(1, 1); const RS_Ellipse source{nullptr, data};
    const auto made = LC_CurveOffset::createEntities(source, LC_CurveOffset::makeSideRequest(LC_CurveOffsetSide::Right, 1),
        LC_CurveOffset::makeDirectOptions(source, 1), LC_CurveOffset::makeDirectSourceBudget());
    REQUIRE(made.status == LC_CurveOffsetStatus::Ok); REQUIRE(made.entities.size() == 1);
    const auto* spline = dynamic_cast<const RS_Spline*>(made.entities[0].get()); REQUIRE(spline);
    REQUIRE(spline->getData().degree == 3);
    geo::WorkBudget budget;
    const auto result = geo::analyseCacheBounds(spline->getData(), data, 1, 2.5, budget);
    CAPTURE(result.status, result.referenceStatus, result.work.vertices, result.work.distancePairs);
    checkBoundsRelation(result, 1, 2.5);
    REQUIRE(result.analysis->layout().spanCount() > 1);
    for (std::size_t i = 0; i < result.analysis->layout().spanCount(); ++i) {
        CHECK(result.analysis->layout().spans[i].degree == 3);
        for (unsigned control = 0; control <= 3; ++control) {
            const auto& actual = result.analysis->layout().spans[i].controls[control];
            const auto& expected = spline->getData().controlPoints[3 * i + control];
            CHECK(actual.x == expected.x); CHECK(actual.y == expected.y);
        }
    }
}

TEST_CASE("Cache bounds keep local scale conversions explicit and own their inputs after caller lifetime ends",
          "[lc-offsetcurve][geometry][cache-bounds]") {
    STATIC_REQUIRE_FALSE(std::is_copy_constructible_v<geo::CacheBoundsAnalysis>);
    STATIC_REQUIRE_FALSE(std::is_default_constructible_v<geo::CacheBoundsAnalysis>);
    STATIC_REQUIRE(std::is_move_constructible_v<geo::CacheBoundsResult>);
    for (const double radius : {1e-280, 1.0, 1e280}) {
        const auto source = basis(radius, 1, .25, .2501);
        const auto cache = arcCache(source, radius, 3); geo::WorkBudget budget;
        const auto result = geo::analyseCacheBounds(cache, source, radius, radius * .01, budget);
        CAPTURE(radius, result.status, result.referenceStatus);
        checkBoundsRelation(result, radius, radius * .01);
    }
    geo::CacheBoundsResult retained;
    { auto source = basis(1, 1, .25, .2501); auto cache = arcCache(source, 1, 1); geo::WorkBudget budget;
      retained = geo::analyseCacheBounds(cache, source, 1, .01, budget); cache.controlPoints.clear(); source.majorP = {0, 0}; }
    checkBoundsRelation(retained, 1, .01);
    for (const double value : {0.0, -1.0, std::numeric_limits<double>::infinity(), std::numeric_limits<double>::quiet_NaN()}) {
        geo::WorkBudget budget; const auto invalid = geo::analyseCacheBounds(cacheLayoutFixture(1, {0, 1}), basis(), 1, value, budget);
        CHECK(invalid.status == geo::CacheBoundsStatus::InvalidInput); CHECK_FALSE(invalid.analysis); checkWork(invalid.work, {});
    }
}

TEST_CASE("Cache bounds qualify adjacent binary64 cusp endpoints and exact trim boundary witness identities",
          "[lc-offsetcurve][geometry][cache-bounds][cache-bounds-boundary]") {
    // Binary64 radian endpoints and Mx,My,r,D are rational. For a simple cusp,
    // (Mx^2+My^2)*(r^2+(1-r^2)*z)^3-D^2*r^2=0, so z=sin^2(theta)
    // is algebraic (0<r<1 makes this a nonconstant polynomial). If a nonzero
    // binary64 theta satisfied it, exp(2*i*theta) would solve the algebraic
    // quadratic w^2-2*(1-2*z)*w+1=0, contradicting Hermite-Lindemann.
    // theta=0 is an axis/stationary case, not SimpleCusp; adding real periods,
    // rotating the major vector or reversing traversal does not change z.
    // See Theorem 1.2, doi:10.1017/S000497272200140X. No finite probe proves
    // that theorem: the exact rational construction below qualifies its boundary.
    const auto atanBounds = [](const EventRational& x) -> std::array<EventRational, 2> {
        EventRational sum = 0, term = x;
        for (unsigned k = 0; k < 32; ++k) {
            EventRational signedTerm = term;
            if (k % 2) { signedTerm = -signedTerm; }
            sum += signedTerm / (2 * k + 1);
            term *= x * x;
        }
        // The even alternating partial sum is below atan(x); the first
        // omitted positive term bounds the remainder, with exact arithmetic.
        return {sum, EventRational{sum + term / 65}};
    };
    const EventRational x = EventRational{1} / 5, y = EventRational{1} / 239;
    const auto a = atanBounds(x), b = atanBounds(y);
    const EventRational thetaLow = 4 * a[0] - b[1], thetaHigh = 4 * a[1] - b[0];
    const EventRational tan2 = 2 * x / (1 - x * x), tan4 = 2 * tan2 / (1 - tan2 * tan2);
    REQUIRE((tan4 - y) / (1 + tan4 * y) == 1);
    REQUIRE(thetaLow > 0); REQUIRE(thetaHigh < 1);
    // Machin's identity in the principal quadrant: 4*atan(1/5)-atan(1/239)=pi/4.
    const double low = M_PI_4, high = std::nextafter(low, std::numeric_limits<double>::infinity());
    REQUIRE(EventRational(low) < thetaLow); REQUIRE(thetaHigh < EventRational(high));
    REQUIRE(std::nextafter(high, -std::numeric_limits<double>::infinity()) == low);
    const EventRational axisSquared = EventRational{120} * 120 + EventRational{40} * 40;
    const EventRational r = EventRational{1} / 2, d = 125;
    const auto polynomial = [&](const EventRational& z) -> EventRational {
        const EventRational h2 = r * r + (1 - r * r) * z;
        return axisSquared * h2 * h2 * h2 - d * d * r * r;
    };
    REQUIRE(polynomial(EventRational{1} / 2) == 0);
    REQUIRE(polynomial(0) < 0); REQUIRE(polynomial(1) > 0);
    // P is strictly increasing on [0,1], and sin^2 is strictly increasing in
    // this quadrant. Thus the adjacent doubles strictly exclude/include the root.
    // Wider controls also exercise successful classification on both sides.
    const std::array<double, 4> endpoints{{low, high, low - .125, high + .125}};
    auto cache = cacheLayoutFixture(1, {0, 1}); cache.controlPoints = {{0, 0}, {0, 0}};
    for (std::size_t i = 0; i < endpoints.size(); ++i) {
        for (const bool reversed : {false, true}) {
            auto source = reversed ? basis(1, .5, endpoints[i], 0, true) : basis(1, .5, 0, endpoints[i]);
            source.majorP = {120, 40};
            geo::WorkBudget criticalBudget;
            const auto critical = geo::analyseCritical(source, -125, criticalBudget);
            CAPTURE(i, reversed, endpoints[i], critical.status, critical.reason);
            if (i >= 2) { REQUIRE(critical.status == LC_IntersectionStatus::Ok); }
            if (critical.status == LC_IntersectionStatus::Ok) {
                REQUIRE(critical.adapter);
                REQUIRE(critical.adapter->criticalOccurrences().size() == (i % 2 ? 1u : 0u));
                for (const auto& occurrence : critical.adapter->criticalOccurrences()) {
                    CHECK(occurrence.kind == geo::CriticalKind::SimpleCusp);
                    CHECK_FALSE(occurrence.atStart); CHECK_FALSE(occurrence.atEnd);
                    CHECK_FALSE((occurrence.parameter.lo() == 0 && occurrence.parameter.hi() == 0));
                    const double end = critical.adapter->frame().parameterEnd();
                    CHECK_FALSE((occurrence.parameter.lo() == end && occurrence.parameter.hi() == end));
                    const EventRational expectedLow = reversed ? EventRational{EventRational(endpoints[i]) - thetaHigh} : thetaLow;
                    const EventRational expectedHigh = reversed ? EventRational{EventRational(endpoints[i]) - thetaLow} : thetaHigh;
                    CHECK(EventRational(occurrence.parameter.lo()) <= expectedLow);
                    CHECK(expectedHigh <= EventRational(occurrence.parameter.hi()));
                }
            } else {
                CHECK((critical.status == LC_IntersectionStatus::AmbiguousTopology || critical.status == LC_IntersectionStatus::LimitExceeded));
                CHECK_FALSE(critical.adapter);
            }
            checkWork(critical.work, criticalBudget.consumed());
            if (i >= 2) { continue; }
            geo::WorkBudget budget;
            const auto result = geo::analyseCacheBounds(cache, source, -125, 500, budget);
            CAPTURE(result.status, result.referenceStatus, result.referenceReason, result.eventReason);
            if (critical.status != LC_IntersectionStatus::Ok) {
                CHECK(result.status == (critical.status == LC_IntersectionStatus::LimitExceeded ?
                    geo::CacheBoundsStatus::LimitExceeded : geo::CacheBoundsStatus::ReferenceRefusal));
                CHECK_FALSE(result.analysis);
            } else if (result.status == geo::CacheBoundsStatus::Ok) {
                REQUIRE(result.analysis);
                const auto& graph = result.analysis->reference(); const auto& events = graph.events();
                REQUIRE(events.criticals().size() == critical.adapter->criticalOccurrences().size());
                if (i == 0) { REQUIRE(events.criticals().empty()); }
                else {
                    REQUIRE(i == 1); REQUIRE(events.criticals().size() == 1);
                    const auto& cusp = events.criticals().front();
                    REQUIRE(cusp.kind == geo::CriticalKind::SimpleCusp);
                    CHECK(cusp.proof == critical.adapter->criticalOccurrences().front().proof);
                    REQUIRE(cusp.occurrence.value < events.occurrences().size());
                    const auto& occurrence = events.occurrences()[cusp.occurrence.value];
                    CHECK(occurrence.role == geo::ReferenceRole::Own);
                    const EventRational expectedLow = reversed ? EventRational{EventRational(endpoints[i]) - thetaHigh} : thetaLow;
                    const EventRational expectedHigh = reversed ? EventRational{EventRational(endpoints[i]) - thetaLow} : thetaHigh;
                    double bracketLow = expectedLow.convert_to<double>(), bracketHigh = expectedHigh.convert_to<double>();
                    if (EventRational(bracketLow) > expectedLow) { bracketLow = std::nextafter(bracketLow, -std::numeric_limits<double>::infinity()); }
                    if (EventRational(bracketHigh) < expectedHigh) { bracketHigh = std::nextafter(bracketHigh, std::numeric_limits<double>::infinity()); }
                    REQUIRE(EventRational(bracketLow) <= expectedLow); REQUIRE(expectedHigh <= EventRational(bracketHigh));
                    CHECK(occurrence.parameter.lo() >= bracketLow); CHECK(occurrence.parameter.hi() <= bracketHigh);
                    CHECK(EventRational(occurrence.parameter.lo()) <= expectedLow);
                    CHECK(expectedHigh <= EventRational(occurrence.parameter.hi()));
                    REQUIRE(occurrence.node.value < graph.incidence().size());
                    const auto& row = graph.incidence()[occurrence.node.value];
                    CHECK(row.cusp); CHECK(row.hiddenPoint); CHECK(row.incoming.empty()); CHECK(row.outgoing.empty());
                    for (const auto& witness : result.analysis->witnesses()) {
                        CHECK(witness.occurrence.value != cusp.occurrence.value); CHECK(witness.node.value != occurrence.node.value);
                    }
                    for (const auto& end : events.freeEnds()) {
                        CHECK(cusp.occurrence.value != end.occurrence.value);
                    }
                }
            } else {
                CHECK((result.status == geo::CacheBoundsStatus::ReferenceRefusal || result.status == geo::CacheBoundsStatus::OffCurve ||
                    result.status == geo::CacheBoundsStatus::Ambiguous || result.status == geo::CacheBoundsStatus::LimitExceeded));
                CHECK_FALSE(result.analysis);
            }
            checkWork(result.work, budget.consumed());
        }
    }

    const auto source = basis(100, .05, M_PI / 12, 7 * M_PI / 4);
    geo::WorkBudget budget;
    const auto result = geo::analyseCacheBounds(cache, source, -4, 500, budget);
    checkBoundsRelation(result, -4, 500);
    const auto& reference = result.analysis->reference(); const auto& events = reference.events();
    std::size_t trimNodes = 0, trimIncidences = 0;
    for (const auto& row : reference.incidence()) {
        if (!row.trimBoundary) { continue; }
        ++trimNodes;
        CHECK_FALSE(row.sourceStart); CHECK_FALSE(row.sourceEnd);
        REQUIRE(row.incoming.empty() != row.outgoing.empty());
        const auto checkIncidence = [&](const std::size_t edgeIndex, const bool start) {
            const auto& edge = reference.edges()[edgeIndex]; const auto& span = reference.spans()[edge.span];
            const auto occurrence = start ? span.start : span.end;
            const auto kind = start ? geo::CacheWitnessKind::StartFringe : geo::CacheWitnessKind::EndFringe;
            CHECK((start ? edge.start.value : edge.end.value) == row.node.value);
            CHECK(events.occurrences()[occurrence.value].role == geo::ReferenceRole::Own);
            CHECK(events.occurrences()[occurrence.value].node.value == row.node.value);
            const auto& parameter = events.occurrences()[occurrence.value].parameter;
            CHECK(std::any_of(result.analysis->witnesses().begin(), result.analysis->witnesses().end(), [&](const auto& w) {
                return w.edge == edgeIndex && w.kind == kind && w.occurrence.value == occurrence.value && w.node.value == row.node.value &&
                    w.referenceParameter.lo() == parameter.lo() && w.referenceParameter.hi() == parameter.hi();
            }));
            ++trimIncidences;
        };
        for (const auto edge : row.incoming) { checkIncidence(edge, false); }
        for (const auto edge : row.outgoing) { checkIncidence(edge, true); }
    }
    CHECK(trimNodes > 0); CHECK(trimIncidences >= trimNodes);
    checkWork(result.work, budget.consumed());
}

namespace {
LC_EllipseOffsetDefinition localTopologyDefinition(const RS_EllipseData& source, const double distance = 1.0,
                                                  const bool reversed = false) {
    LC_EllipseOffsetDefinition definition;
    definition.basis = source; definition.distance = std::abs(distance);
    definition.towardCentre = distance < 0; definition.cacheReversed = reversed;
    definition.relTolerance = .01;
    return definition;
}

RS_SplineData stationaryKnotArcCache(const RS_EllipseData& source, const double distance,
                                     const double from, const double to) {
    const auto frame = geo::ReferenceFrame::create(source, distance);
    REQUIRE(frame);
    LC_CurveJet first, middle, last;
    const double startParameter = frame->parameterEnd() * from;
    const double middleParameter = startParameter + frame->parameterEnd() * (to - from) / 2;
    const double endParameter = frame->parameterEnd() * to;
    REQUIRE(frame->offsetJet(startParameter, first)); REQUIRE(frame->offsetJet(middleParameter, middle));
    REQUIRE(frame->offsetJet(endParameter, last));
    const auto world = [&](const RS_Vector& point) {
        return RS_Vector{source.center.x + frame->scale() * point.x,
                         source.center.y + frame->scale() * point.y};
    };
    auto data = cacheLayoutFixture(3, {0, .5, 1});
    const auto a = world(first.point), b = world(middle.point), c = world(last.point);
    data.controlPoints = {a, a + (b - a) / 3, b, b, b, b + (c - b) * (2.0 / 3.0), c};
    return data;
}

void checkLocalTopologyAccepted(const geo::CacheLocalTopologyResult& result, const geo::WorkBudget& budget) {
    CAPTURE(static_cast<int>(result.status), static_cast<int>(result.boundsStatus),
            result.work.cacheSpans, result.work.vertices,
            result.work.distancePairs, result.work.deepest);
    REQUIRE(result.status == geo::CacheLocalTopologyStatus::Ok);
    REQUIRE(result.analysis);
    REQUIRE_FALSE(result.analysis->witnesses().empty());
    checkWork(result.work, budget.consumed());
    CHECK(result.analysis->bounds().status == geo::CacheBoundsStatus::Ok);
}

void checkLocalTopologyRefused(const geo::CacheLocalTopologyResult& result, const geo::WorkBudget& budget) {
    CHECK(result.status != geo::CacheLocalTopologyStatus::Ok);
    CHECK_FALSE(result.analysis);
    checkWork(result.work, budget.consumed());
}

void checkLocalWitnessIdentities(const geo::CacheLocalTopologyAnalysis& analysis, const double tolerance = .01) {
    const auto& bounds = analysis.bounds();
    REQUIRE(bounds.analysis);
    const auto& layout = bounds.analysis->layout();
    const auto& reference = bounds.analysis->reference();
    const auto& events = reference.events();
    bool interior = false, start = false, end = false;
    const double threshold = 2 * tolerance /
        reference.events().providers().frame().scale();
    for (const auto& witness : analysis.witnesses()) {
        REQUIRE(witness.cache.span < layout.spanCount());
        REQUIRE(witness.normalizedSpan < layout.spanCount());
        REQUIRE(witness.edge < reference.edges().size());
        const auto& edge = reference.edges()[witness.edge];
        REQUIRE(edge.span < reference.spans().size());
        const auto& span = reference.spans()[edge.span];
        if (witness.kind == geo::CacheWitnessKind::Interior) {
            interior = true;
            CHECK(witness.occurrence.value == std::size_t(-1));
            CHECK(witness.node.value == std::size_t(-1));
            CHECK(witness.referenceParameter.lo() < witness.referenceParameter.hi());
            CHECK(witness.startEndpointError.hi() <= threshold);
            CHECK(witness.endEndpointError.hi() <= threshold);
            const auto degree = layout.spans[witness.cache.span].degree;
            if (degree == 1) { CHECK(witness.candidateDerivativeProjection.lo() > 0); }
            else {
                CHECK(witness.candidateDerivativeProjection.lo() >= 0);
                CHECK(witness.candidateDerivativeProjection.hi() > 0);
            }
            CHECK(witness.referenceDerivativeProjection.lo() >= 0);
        } else {
            const bool isStart = witness.kind == geo::CacheWitnessKind::StartFringe;
            const auto occurrence = isStart ? span.start : span.end;
            const auto node = isStart ? edge.start : edge.end;
            CHECK(witness.occurrence.value == occurrence.value);
            CHECK(witness.node.value == node.value);
            REQUIRE(occurrence.value < events.occurrences().size());
            CHECK(events.occurrences()[occurrence.value].role == geo::ReferenceRole::Own);
            CHECK(events.occurrences()[occurrence.value].node.value == node.value);
            CHECK(witness.referenceParameter.lo() == events.occurrences()[occurrence.value].parameter.lo());
            CHECK(witness.referenceParameter.hi() == events.occurrences()[occurrence.value].parameter.hi());
            CHECK(witness.candidateAtStart == isStart);
            CHECK((isStart ? witness.cache.parameter.lo() == 0.0 : witness.cache.parameter.hi() == 1.0));
            CHECK(witness.candidateDerivativeProjection.lo() > 0);
            CHECK(witness.referenceDerivativeProjection.lo() > 0);
            start = start || isStart; end = end || !isStart;
        }
    }
    CHECK(interior); CHECK(start); CHECK(end);
}
} // namespace

TEST_CASE("Local topology certifies linear arc-cache forward alternatives", "[lc-offsetcurve][geometry][cache-topology-local]") {
    STATIC_REQUIRE_FALSE(std::is_copy_constructible_v<geo::CacheLocalTopologyResult>);
    STATIC_REQUIRE(std::is_move_constructible_v<geo::CacheLocalTopologyResult>);
    const auto source = basis(1, 1, .25, .2501); const auto cache = arcCache(source, 1, 1);
    geo::WorkBudget budget;
    const auto result = geo::analyseCacheTopologyLocally(cache, localTopologyDefinition(source), .05, budget);
    checkLocalTopologyAccepted(result, budget); checkLocalWitnessIdentities(*result.analysis);
}

TEST_CASE("Local topology certifies generated cubic arc-cache forward alternatives", "[lc-offsetcurve][geometry][cache-topology-local]") {
    const auto source = basis(1, 1, .25, .2501); const auto cache = arcCache(source, 1, 3);
    geo::WorkBudget budget;
    const auto result = geo::analyseCacheTopologyLocally(cache, localTopologyDefinition(source), .01, budget);
    checkLocalTopologyAccepted(result, budget); checkLocalWitnessIdentities(*result.analysis);
}

TEST_CASE("Local topology permits zero derivative at an internal knot without changing forward orientation",
          "[lc-offsetcurve][geometry][cache-topology-local]") {
    const auto source = basis(1, 1, .25, .26); const auto cache = stationaryKnotArcCache(source, 1, 0, 1);
    geo::WorkBudget budget;
    const auto result = geo::analyseCacheTopologyLocally(cache, localTopologyDefinition(source), .05, budget);
    checkLocalTopologyAccepted(result, budget);
    CHECK(result.analysis->bounds().analysis->layout().spanCount() == 2);
}

TEST_CASE("Local topology preserves forward orientation when the cache is already normalized",
          "[lc-offsetcurve][geometry][cache-topology-local]") {
    const auto source = basis(1, 1, .25, .2501); const auto cache = arcCache(source, 1, 3);
    geo::WorkBudget budget;
    const auto result = geo::analyseCacheTopologyLocally(cache, localTopologyDefinition(source, 1, false), .01, budget);
    checkLocalTopologyAccepted(result, budget);
}

TEST_CASE("Cache dyadic reflection maps refined cells and rejects malformed domains",
          "[lc-offsetcurve][geometry][cache-topology-local]") {
    geo::CacheDyadicDomain whole;
    whole.span = 3; whole.parameter = LC_Interval::hull(0, 1);
    geo::CacheDyadicDomain wholeReflected;
    REQUIRE(geo::reflectCacheDyadicDomain(whole, wholeReflected));
    CHECK(wholeReflected.span == whole.span);
    CHECK(wholeReflected.depth == 0);
    CHECK(wholeReflected.index == 0);
    CHECK(wholeReflected.parameter.lo() == 0);
    CHECK(wholeReflected.parameter.hi() == 1);

    for (unsigned depth = 1; depth <= 8; ++depth) {
        const std::size_t denominator = std::size_t{1} << depth;
        for (std::size_t index = 0; index < denominator; ++index) {
            geo::CacheDyadicDomain input;
            input.span = 3; input.depth = depth; input.index = index;
            input.parameter = LC_Interval::hull(static_cast<double>(index) / denominator,
                                                 static_cast<double>(index + 1) / denominator);
            geo::CacheDyadicDomain reflected;
            REQUIRE(geo::reflectCacheDyadicDomain(input, reflected));
            CHECK(reflected.span == input.span);
            CHECK(reflected.depth == depth);
            CHECK(reflected.index == denominator - 1 - index);
            CHECK(reflected.parameter.lo() == static_cast<double>(denominator - 1 - index) / denominator);
            CHECK(reflected.parameter.hi() == static_cast<double>(denominator - index) / denominator);
            geo::CacheDyadicDomain restored;
            REQUIRE(geo::reflectCacheDyadicDomain(reflected, restored));
            CHECK(restored.span == input.span);
            CHECK(restored.depth == input.depth);
            CHECK(restored.index == input.index);
            CHECK(restored.parameter.lo() == input.parameter.lo());
            CHECK(restored.parameter.hi() == input.parameter.hi());
        }
    }
    geo::CacheDyadicDomain output;
    geo::CacheDyadicDomain invalid;
    invalid.span = 2; invalid.depth = 2; invalid.index = 4;
    invalid.parameter = LC_Interval::hull(0, 1);
    CHECK_FALSE(geo::reflectCacheDyadicDomain(invalid, output));
    CHECK(output.span == std::size_t(-1));
    CHECK_FALSE(output.parameter.isValid());
    invalid.index = 0; invalid.parameter = LC_Interval::hull(.125, .375);
    CHECK_FALSE(geo::reflectCacheDyadicDomain(invalid, output));
    CHECK(output.span == std::size_t(-1));
    CHECK_FALSE(output.parameter.isValid());

    invalid.depth = 2; invalid.parameter = LC_Interval::hull(-.25, 0);
    CHECK_FALSE(geo::reflectCacheDyadicDomain(invalid, output));
    CHECK(output.span == std::size_t(-1));
    CHECK_FALSE(output.parameter.isValid());

    invalid.depth = std::numeric_limits<double>::digits;
    invalid.index = 0;
    invalid.parameter = LC_Interval::hull(0, std::ldexp(1.0, -std::numeric_limits<double>::digits));
    CHECK_FALSE(geo::reflectCacheDyadicDomain(invalid, output));
    CHECK(output.span == std::size_t(-1));
    CHECK_FALSE(output.parameter.isValid());

    invalid.depth = std::numeric_limits<std::size_t>::digits;
    invalid.parameter = LC_Interval::hull(0, 1);
    CHECK_FALSE(geo::reflectCacheDyadicDomain(invalid, output));
    CHECK(output.span == std::size_t(-1));
    CHECK_FALSE(output.parameter.isValid());
}

TEST_CASE("Whole-circle seam uses a quarter-span chord to certify its one-sided tangent",
          "[lc-offsetcurve][geometry][cache-topology-local]") {
    const auto source = basis(1, 1);
    const auto frame = geo::ReferenceFrame::create(source, 1);
    REQUIRE(frame);
    geo::WorkBudget budget;
    const auto reference = geo::analyseReferenceComponents(source, 1, .01, budget);
    REQUIRE(reference.status == LC_IntersectionStatus::Ok);
    REQUIRE(reference.analysis);
    REQUIRE(reference.analysis->events().seam());
    REQUIRE(reference.analysis->spans().size() == 1);
    const auto& span = reference.analysis->spans().front();
    const double width = span.interior.hi() - span.interior.lo();
    const double quarter = width / 4.0;
    geo::CurveChordBounds half, firstQuarter, lastQuarter;
    REQUIRE(geo::boundReferenceChord(*frame, span.interior.lo(), span.interior.lo() + 2 * quarter, half));
    REQUIRE(geo::boundReferenceChord(*frame, span.interior.lo(), span.interior.lo() + quarter, firstQuarter));
    REQUIRE(geo::boundReferenceChord(*frame, span.interior.hi() - quarter, span.interior.hi(), lastQuarter));
    LC_CurveJetBounds startJet, endJet;
    REQUIRE(frame->boundOffsetJet(span.interior.lo(), span.interior.lo(), startJet));
    REQUIRE(frame->boundOffsetJet(span.interior.hi(), span.interior.hi(), endJet));
    const auto project = [](const LC_CurveJetBounds& jet, const geo::CurveChordBounds& chord) {
        return jet.dx * (chord.endX - chord.startX) + jet.dy * (chord.endY - chord.startY);
    };
    const auto oldStartProjection = project(startJet, half);
    const auto newStartProjection = project(startJet, firstQuarter);
    const auto newEndProjection = project(endJet, lastQuarter);
    CHECK_FALSE(oldStartProjection.lo() > 0);
    CHECK_FALSE(oldStartProjection.hi() < 0);
    CHECK(newStartProjection.lo() > 0);
    CHECK(newEndProjection.lo() > 0);
}

TEST_CASE("Local topology applies cacheReversed once to whole span order and cell endpoints",
          "[lc-offsetcurve][geometry][cache-topology-local]") {
    const auto source = basis(1, 1, .25, .2501);
    const std::vector<double> breaks{0, .1, .3, .6, 1};
    const auto original = arcCache(source, 1, 3);
    std::array<RS_Vector, 4> remaining;
    std::copy_n(original.controlPoints.begin(), remaining.size(), remaining.begin());
    const auto split = [](const std::array<RS_Vector, 4>& points, const double t) {
        const auto lerp = [t](const RS_Vector& a, const RS_Vector& b) { return a + (b - a) * t; };
        const RS_Vector a = lerp(points[0], points[1]);
        const RS_Vector b = lerp(points[1], points[2]);
        const RS_Vector c = lerp(points[2], points[3]);
        const RS_Vector d = lerp(a, b), e = lerp(b, c);
        const RS_Vector middle = lerp(d, e);
        return std::pair<std::array<RS_Vector, 4>, std::array<RS_Vector, 4>>{
            {{points[0], a, d, middle}}, {{middle, e, c, points[3]}}};
    };
    auto cache = cacheLayoutFixture(3, breaks);
    for (std::size_t span = 0; span + 1 < breaks.size(); ++span) {
        const double relativeEnd = (breaks[span + 1] - breaks[span]) / (1.0 - breaks[span]);
        auto [piece, rest] = split(remaining, relativeEnd);
        const std::size_t first = 3 * span;
        std::copy(piece.begin(), piece.end(), cache.controlPoints.begin() + static_cast<std::ptrdiff_t>(first));
        remaining = rest;
    }
    geo::WorkBudget forwardBoundsBudget;
    const auto forwardBounds = geo::analyseCacheBounds(cache, source, 1, .0001, forwardBoundsBudget);
    CAPTURE(static_cast<int>(forwardBounds.status), forwardBounds.work.cacheSpans,
            forwardBounds.work.vertices, forwardBounds.work.distancePairs, forwardBounds.work.deepest);
    REQUIRE(forwardBounds.status == geo::CacheBoundsStatus::Ok);
    std::reverse(cache.controlPoints.begin(), cache.controlPoints.end());
    std::reverse(cache.weights.begin(), cache.weights.end());
    std::reverse(cache.knotslist.begin(), cache.knotslist.end());
    for (auto& knot : cache.knotslist) { knot = 1.0 - knot; }
    geo::WorkBudget budget;
    const auto result = geo::analyseCacheTopologyLocally(cache, localTopologyDefinition(source, 1, true), .0001, budget);
    checkLocalTopologyAccepted(result, budget); checkLocalWitnessIdentities(*result.analysis, .0001);
    const auto& layout = result.analysis->bounds().analysis->layout();
    REQUIRE(layout.spanCount() == breaks.size() - 1);
    std::vector<bool> sawNormalizedSpan(layout.spanCount(), false);
    bool sawRefinedCell = false;
    for (const auto& witness : result.analysis->witnesses()) {
        REQUIRE(witness.cache.span < layout.spanCount());
        CHECK(witness.normalizedSpan == layout.spanCount() - 1 - witness.cache.span);
        sawNormalizedSpan[witness.normalizedSpan] = true;
        const double denominator = std::ldexp(1.0, static_cast<int>(witness.cache.depth));
        CHECK(witness.cache.parameter.lo() == static_cast<double>(witness.cache.index) / denominator);
        CHECK(witness.cache.parameter.hi() == static_cast<double>(witness.cache.index + 1) / denominator);
        sawRefinedCell = sawRefinedCell || witness.cache.depth > 0;
    }
    CHECK(std::all_of(sawNormalizedSpan.begin(), sawNormalizedSpan.end(), [](const bool value) { return value; }));
    CHECK(sawRefinedCell);
}

TEST_CASE("Local topology refuses reversed controls when cacheReversed is absent",
          "[lc-offsetcurve][geometry][cache-topology-local]") {
    const auto source = basis(1, 1, .25, .2501); auto cache = arcCache(source, 1, 3);
    std::reverse(cache.controlPoints.begin(), cache.controlPoints.end());
    geo::WorkBudget budget;
    const auto result = geo::analyseCacheTopologyLocally(cache, localTopologyDefinition(source), .01, budget);
    REQUIRE(result.boundsStatus == geo::CacheBoundsStatus::Ok);
    checkLocalTopologyRefused(result, budget);
}

TEST_CASE("Local topology rejects an exact-binary folded cubic whose set image matches a short circular offset",
          "[lc-offsetcurve][geometry][cache-topology-local]") {
    auto source = basis(1000, 1, 2 * M_PI - .000004, .000004); auto cache = arcCache(source, 1, 3);
    source.center.y = -cache.controlPoints.front().y;
    cache = arcCache(source, 1, 3);
    const double x = cache.controlPoints.front().x, y0 = cache.controlPoints.front().y;
    const double dy = cache.controlPoints.back().y - y0;
    REQUIRE(y0 == 0); REQUIRE(dy > 0);
    cache.controlPoints = {{x, y0}, {x, y0 + dy}, {x, y0 - dy / 4}, {x, y0 + dy}};

    const std::array<EventRational, 4> control{{0, 1, EventRational(cache.controlPoints[2].y - y0) / EventRational(dy), 1}};
    const EventRational a = 3 * control[1];
    const EventRational b = -6 * control[1] + 3 * control[2];
    const EventRational c = 3 * control[1] - 3 * control[2] + control[3];
    const auto derivative = [&](const EventRational& t) {
        EventRational value = a;
        EventRational term = b;
        term *= t; term *= 2; value += term;
        term = c; term *= t; term *= t; term *= 3; value += term;
        return value;
    };
    const auto polynomial = [&](const EventRational& t) {
        EventRational value = a, term = b;
        value *= t;
        term *= t; term *= t; value += term;
        term = c; term *= t; term *= t; term *= t; value += term;
        return value;
    };
    const auto valueRange = [&](const EventRational& lo, const EventRational& hi) {
        EventRational lower = a, upper = a, term = b;
        lower *= lo; upper *= hi;
        term *= hi; term *= hi; lower += term;
        term = b; term *= lo; term *= lo; upper += term;
        term = c; term *= lo; term *= lo; term *= lo; lower += term;
        term = c; term *= hi; term *= hi; term *= hi; upper += term;
        return std::pair<EventRational, EventRational>{lower, upper};
    };
    CHECK(control[1] - control[0] > 0); CHECK(control[2] - control[1] < 0); CHECK(control[3] - control[2] > 0);
    const EventRational quarterNegative = EventRational{-1} / 4;
    CHECK(control[2] == quarterNegative);
    const EventRational halfSweep = EventRational{1} / 250000;
    EventRational sagittaUpper = EventRational{1001};
    sagittaUpper *= halfSweep; sagittaUpper *= halfSweep; sagittaUpper /= 2;
    const EventRational sagittaLimit = EventRational{1} / 100000000;
    CHECK(sagittaUpper < sagittaLimit);
    EventRational controlDifferenceBound = EventRational{11};
    controlDifferenceBound *= EventRational(dy); controlDifferenceBound /= 12;
    const EventRational geometryThreshold = EventRational{1} / 100;
    EventRational totalErrorBound = controlDifferenceBound;
    totalErrorBound += sagittaUpper;
    CHECK(controlDifferenceBound < geometryThreshold);
    CHECK(totalErrorBound < geometryThreshold);
    EventRational clearanceLowerBound = EventRational{1};
    clearanceLowerBound -= sagittaUpper;
    const EventRational clearanceFloor = EventRational{79} / 80;
    CHECK(clearanceLowerBound > clearanceFloor);
    CHECK(a > 0); CHECK(b < 0); CHECK(c > 0);
    const EventRational r0lo = EventRational{355} / 1000;
    const EventRational r0hi = EventRational{356} / 1000;
    const EventRational r1lo = EventRational{591} / 1000;
    const EventRational r1hi = EventRational{592} / 1000;
    CHECK(derivative(r0lo) > 0); CHECK(derivative(r0hi) < 0);
    CHECK(derivative(r1lo) < 0); CHECK(derivative(r1hi) > 0);
    const EventRational atZero = polynomial(EventRational{0});
    const EventRational atOne = polynomial(EventRational{1});
    CHECK(atZero == EventRational{0}); CHECK(atOne == EventRational{1});
    const auto bernstein = [&](const EventRational& t) {
        const EventRational u = 1 - t;
        EventRational first = u; first *= u; first *= t; first *= control[1]; first *= 3;
        EventRational second = u; second *= t; second *= t; second *= control[2]; second *= 3;
        EventRational third = t; third *= t; third *= t; third *= control[3];
        first += second; first += third;
        return first;
    };
    for (const EventRational t : {EventRational{1} / 4, EventRational{1} / 2, EventRational{3} / 4}) {
        CHECK(bernstein(t) == polynomial(t));
    }
    const auto maximumRange = valueRange(r0lo, r0hi), minimumRange = valueRange(r1lo, r1hi);
    CHECK(minimumRange.first > 0); CHECK(minimumRange.second < 1);
    CHECK(maximumRange.first > 0); CHECK(maximumRange.second < 1);

    const auto diagnosticFrame = geo::ReferenceFrame::create(source, 1);
    REQUIRE(diagnosticFrame);
    geo::WorkBudget diagnosticBudget;
    const auto diagnosticLayout = geo::inspectCacheLayout(cache, diagnosticBudget);
    REQUIRE(diagnosticLayout.status == geo::CacheLayoutStatus::Ok);
    geo::CurveChordBounds diagnosticChord;
    REQUIRE(geo::boundCacheChord(diagnosticLayout.spans.front(), *diagnosticFrame, 0, 1, diagnosticChord));
    CAPTURE(dy, diagnosticFrame->scale(), diagnosticChord.deviation.lo(), diagnosticChord.deviation.hi(),
            diagnosticChord.startY.lo(), diagnosticChord.startY.hi(), diagnosticChord.endY.lo(),
            diagnosticChord.endY.hi(), diagnosticChord.y.lo(), diagnosticChord.y.hi());

    geo::WorkBudget referenceBudget;
    const auto reference = geo::analyseReferenceComponents(source, 1, .01, referenceBudget);
    CAPTURE(static_cast<int>(reference.status), static_cast<int>(reference.reason),
            static_cast<int>(reference.eventReason), reference.work.deepest);
    REQUIRE(reference.status == LC_IntersectionStatus::Ok);
    geo::WorkBudget boundsBudget;
    const auto bounds = geo::analyseCacheBounds(cache, source, 1, .005, boundsBudget);
    CAPTURE(static_cast<int>(bounds.status), static_cast<int>(bounds.referenceStatus),
            static_cast<int>(bounds.referenceReason), bounds.referenceWork.deepest,
            bounds.work.cacheSpans, bounds.work.vertices, bounds.work.distancePairs, bounds.work.deepest);
    REQUIRE(bounds.status == geo::CacheBoundsStatus::Ok);
    geo::WorkBudget budget;
    const auto result = geo::analyseCacheTopologyLocally(cache, localTopologyDefinition(source), .005, budget);
    REQUIRE(result.boundsStatus == geo::CacheBoundsStatus::Ok);
    checkLocalTopologyRefused(result, budget);
}

TEST_CASE("Local topology rejects the rounded legacy folded-cubic discriminator",
          "[lc-offsetcurve][geometry][cache-topology-local]") {
    const auto source = basis(1000, 1, 2 * M_PI - .000005, .000005); auto cache = arcCache(source, 1, 3);
    const double x = cache.controlPoints.front().x, y0 = cache.controlPoints.front().y;
    const double dy = cache.controlPoints.back().y - y0;
    cache.controlPoints = {{x, y0}, {x, y0 + (4.0 / 3.0) * dy},
                           {x, y0 - dy / 3.0}, {x, y0 + dy}};
    geo::WorkBudget boundsBudget;
    const auto bounds = geo::analyseCacheBounds(cache, source, 1, .01, boundsBudget);
    CAPTURE(static_cast<int>(bounds.status), static_cast<int>(bounds.referenceStatus),
            static_cast<int>(bounds.referenceReason), bounds.referenceWork.deepest,
            bounds.work.cacheSpans, bounds.work.vertices, bounds.work.distancePairs, bounds.work.deepest);
    REQUIRE(bounds.status == geo::CacheBoundsStatus::Ok);
    geo::WorkBudget budget;
    const auto result = geo::analyseCacheTopologyLocally(cache, localTopologyDefinition(source), .01, budget);
    checkLocalTopologyRefused(result, budget);
}

TEST_CASE("Local topology refuses a constant positive-length cache span", "[lc-offsetcurve][geometry][cache-topology-local]") {
    const auto source = basis(1, 1, .25, .2501); auto cache = cacheLayoutFixture(1, {0, 1});
    const auto endpoint = arcCache(source, 1, 1).controlPoints.front();
    cache.controlPoints = {endpoint, endpoint};
    geo::WorkBudget budget;
    const auto result = geo::analyseCacheTopologyLocally(cache, localTopologyDefinition(source), 2.5, budget);
    checkLocalTopologyRefused(result, budget);
    CHECK(result.status == geo::CacheLocalTopologyStatus::Topology);
}

TEST_CASE("Local topology pairs candidate start only with reference lower endpoint",
          "[lc-offsetcurve][geometry][cache-topology-local]") {
    const auto source = basis(1, 1, .25, .2501); auto cache = arcCache(source, 1, 3);
    const auto start = cache.controlPoints.front(), end = cache.controlPoints.back();
    cache.controlPoints = {end, start, start, end};
    geo::WorkBudget boundsBudget;
    const auto bounds = geo::analyseCacheBounds(cache, source, 1, .01, boundsBudget);
    REQUIRE(bounds.status == geo::CacheBoundsStatus::Ok);
    geo::WorkBudget budget;
    const auto result = geo::analyseCacheTopologyLocally(cache, localTopologyDefinition(source), .01, budget);
    checkLocalTopologyRefused(result, budget);
}

TEST_CASE("Local topology pairs candidate end only with reference upper endpoint",
          "[lc-offsetcurve][geometry][cache-topology-local]") {
    const auto source = basis(1, 1, .25, .2501); auto cache = arcCache(source, 1, 3);
    const auto start = cache.controlPoints.front(), end = cache.controlPoints.back();
    cache.controlPoints = {start, end, end, start};
    geo::WorkBudget boundsBudget;
    const auto bounds = geo::analyseCacheBounds(cache, source, 1, .01, boundsBudget);
    REQUIRE(bounds.status == geo::CacheBoundsStatus::Ok);
    geo::WorkBudget budget;
    const auto result = geo::analyseCacheTopologyLocally(cache, localTopologyDefinition(source), .01, budget);
    checkLocalTopologyRefused(result, budget);
}

TEST_CASE("Local topology preserves exact event-root edge occurrence node and fringe roles",
          "[lc-offsetcurve][geometry][cache-topology-local]") {
    const auto source = basis(100, .05, M_PI / 12, 7 * M_PI / 4);
    const double distance = -4, tolerance = 500;
    geo::WorkBudget referenceBudget;
    const auto prepared = geo::analyseReferenceComponents(source, distance, tolerance, referenceBudget);
    REQUIRE(prepared.status == LC_IntersectionStatus::Ok);
    REQUIRE(prepared.analysis);
    const auto& reference = *prepared.analysis;
    const auto& events = reference.events();
    const auto validEdgeEnd = [&](const std::size_t edgeIndex, const bool start, const geo::ReferenceNodeId node) {
        if (edgeIndex >= reference.edges().size()) { return false; }
        const auto& edge = reference.edges()[edgeIndex];
        if (edge.span >= reference.spans().size()) { return false; }
        const auto& span = reference.spans()[edge.span];
        const auto occurrence = start ? span.start : span.end;
        return (start ? edge.start.value : edge.end.value) == node.value &&
            occurrence.value < events.occurrences().size() &&
            events.occurrences()[occurrence.value].role == geo::ReferenceRole::Own &&
            events.occurrences()[occurrence.value].node.value == node.value;
    };
    const auto root = std::find_if(reference.incidence().begin(), reference.incidence().end(), [&](const auto& row) {
        if (!row.trimBoundary || row.sourceStart || row.sourceEnd ||
            row.incoming.empty() == row.outgoing.empty()) { return false; }
        return std::all_of(row.incoming.begin(), row.incoming.end(), [&](const auto edge) {
                   return validEdgeEnd(edge, false, row.node);
               }) &&
            std::all_of(row.outgoing.begin(), row.outgoing.end(), [&](const auto edge) {
                   return validEdgeEnd(edge, true, row.node);
               });
    });
    REQUIRE(root != reference.incidence().end());
    const bool rootAtStart = !root->outgoing.empty();
    const auto selectedEdgeIndex = rootAtStart ? root->outgoing.front() : root->incoming.front();
    const auto& selectedEdge = reference.edges()[selectedEdgeIndex];
    const auto& selectedSpan = reference.spans()[selectedEdge.span];
    const auto& startOccurrence = events.occurrences()[selectedSpan.start.value];
    const auto& endOccurrence = events.occurrences()[selectedSpan.end.value];
    const double startParameter = startOccurrence.parameter.lo() +
        (startOccurrence.parameter.hi() - startOccurrence.parameter.lo()) / 2;
    const double endParameter = endOccurrence.parameter.lo() +
        (endOccurrence.parameter.hi() - endOccurrence.parameter.lo()) / 2;
    const auto& frame = events.providers().frame();
    LC_CurveJet startJet, endJet;
    REQUIRE(frame.offsetJet(startParameter, startJet));
    REQUIRE(frame.offsetJet(endParameter, endJet));
    const auto worldPoint = [&](const RS_Vector& local) -> std::optional<RS_Vector> {
        LC_Interval x, y;
        if (!frame.pointToWorld(local, x, y)) { return std::nullopt; }
        return RS_Vector{x.lo() + (x.hi() - x.lo()) / 2, y.lo() + (y.hi() - y.lo()) / 2};
    };
    const auto worldStart = worldPoint(startJet.point);
    const auto worldControl1 = worldPoint({startJet.point.x + (endParameter - startParameter) * startJet.first.x / 3,
                                           startJet.point.y + (endParameter - startParameter) * startJet.first.y / 3});
    const auto worldControl2 = worldPoint({endJet.point.x - (endParameter - startParameter) * endJet.first.x / 3,
                                           endJet.point.y - (endParameter - startParameter) * endJet.first.y / 3});
    const auto worldEnd = worldPoint(endJet.point);
    REQUIRE(worldStart); REQUIRE(worldControl1); REQUIRE(worldControl2); REQUIRE(worldEnd);
    auto cache = cacheLayoutFixture(3, {0, 1});
    cache.controlPoints = {*worldStart, *worldControl1, *worldControl2, *worldEnd};
    geo::WorkBudget boundsBudget;
    const auto bounds = geo::analyseCacheBounds(cache, source, distance, tolerance, boundsBudget);
    CAPTURE(bounds.status, bounds.referenceStatus, bounds.referenceReason, bounds.eventReason,
            bounds.work.vertices, bounds.work.distancePairs, bounds.work.deepest);
    REQUIRE(bounds.status == geo::CacheBoundsStatus::Ok);
    const auto selectedOccurrence = rootAtStart ? selectedSpan.start : selectedSpan.end;
    const auto selectedKind = rootAtStart ? geo::CacheWitnessKind::StartFringe : geo::CacheWitnessKind::EndFringe;
    REQUIRE(selectedOccurrence.value < events.occurrences().size());
    CHECK(events.occurrences()[selectedOccurrence.value].role == geo::ReferenceRole::Own);
    CHECK(events.occurrences()[selectedOccurrence.value].node.value == root->node.value);
    CHECK(std::any_of(bounds.analysis->witnesses().begin(), bounds.analysis->witnesses().end(), [&](const auto& witness) {
        return witness.kind == selectedKind && witness.edge == selectedEdgeIndex &&
            witness.occurrence.value == selectedOccurrence.value && witness.node.value == root->node.value &&
            witness.referenceParameter.lo() == events.occurrences()[selectedOccurrence.value].parameter.lo() &&
            witness.referenceParameter.hi() == events.occurrences()[selectedOccurrence.value].parameter.hi();
    }));
    CHECK(std::any_of(events.pairs().begin(), events.pairs().end(), [&](const auto& pair) {
        const auto nodeOf = [&](const geo::ReferenceOccurrenceId id) {
            return id.value < events.occurrences().size() ? events.occurrences()[id.value].node.value :
                std::size_t(-1);
        };
        return nodeOf(pair.first) == root->node.value || nodeOf(pair.second) == root->node.value;
    }));

    geo::WorkBudget budget;
    const auto result = geo::analyseCacheTopologyLocally(cache, localTopologyDefinition(source, distance), tolerance, budget);
    CHECK(result.boundsStatus == geo::CacheBoundsStatus::Ok);
    CHECK(result.work.distancePairs > bounds.work.distancePairs);
    if (result.analysis) {
        CHECK(std::any_of(result.analysis->witnesses().begin(), result.analysis->witnesses().end(), [&](const auto& witness) {
            return witness.kind == selectedKind && witness.edge == selectedEdgeIndex &&
                witness.occurrence.value == selectedOccurrence.value && witness.node.value == root->node.value &&
                (rootAtStart ? witness.cache.parameter.lo() == 0.0 : witness.cache.parameter.hi() == 1.0);
        }));
    } else {
        CHECK((result.status == geo::CacheLocalTopologyStatus::Topology ||
               result.status == geo::CacheLocalTopologyStatus::Ambiguous ||
               result.status == geo::CacheLocalTopologyStatus::LimitExceeded));
    }
}

TEST_CASE("Local topology resolves a zero endpoint derivative from the first inward control",
          "[lc-offsetcurve][geometry][cache-topology-local]") {
    const auto source = basis(1, 1, .25, .2501); auto cache = arcCache(source, 1, 3);
    cache.controlPoints[1] = cache.controlPoints.front();
    geo::WorkBudget budget;
    const auto result = geo::analyseCacheTopologyLocally(cache, localTopologyDefinition(source), .01, budget);
    checkLocalTopologyAccepted(result, budget); checkLocalWitnessIdentities(*result.analysis);
}

TEST_CASE("Local topology does not promote hidden cusp roots into reference alternatives",
          "[lc-offsetcurve][geometry][cache-topology-local]") {
    const auto source = basis(10, .5); const auto cache = cacheLayoutFixture(1, {0, 1});
    geo::WorkBudget budget;
    const auto result = geo::analyseCacheTopologyLocally(cache, localTopologyDefinition(source, -5), 1e-5, budget);
    checkLocalTopologyRefused(result, budget);
    CHECK((result.status == geo::CacheLocalTopologyStatus::BoundsRefusal ||
           result.status == geo::CacheLocalTopologyStatus::Topology));
}

TEST_CASE("Local topology is invariant under affine and narrow normalized knot domains",
          "[lc-offsetcurve][geometry][cache-topology-local]") {
    const auto source = basis(1, 1, .25, .2501); const auto cache = arcCache(source, 1, 3);
    const std::array<std::array<double, 8>, 2> knotSets{{
        {{-7, -7, -7, -7, -6, -6, -6, -6}},
        {{-7, -7, -7, -7, std::nextafter(-7.0, 0.0), std::nextafter(-7.0, 0.0),
          std::nextafter(-7.0, 0.0), std::nextafter(-7.0, 0.0)}}}};
    for (const auto& knots : knotSets) {
        auto affine = cache; affine.knotslist.assign(knots.begin(), knots.end());
        geo::WorkBudget budget;
        const auto result = geo::analyseCacheTopologyLocally(affine, localTopologyDefinition(source), .01, budget);
        checkLocalTopologyAccepted(result, budget);
    }
}

TEST_CASE("Local topology stays atomic for extreme finite coordinates", "[lc-offsetcurve][geometry][cache-topology-local]") {
    const auto source = basis(1e300, 1, .25, .2501); const auto cache = arcCache(source, 1, 3);
    geo::WorkBudget budget;
    const auto result = geo::analyseCacheTopologyLocally(cache, localTopologyDefinition(source), .01, budget);
    checkWork(result.work, budget.consumed());
    if (result.status == geo::CacheLocalTopologyStatus::Ok) {
        REQUIRE(result.analysis);
        for (const auto& witness : result.analysis->witnesses()) {
            CHECK(witness.clearanceLowerBound.isValid());
            CHECK(witness.startEndpointError.isValid()); CHECK(witness.endEndpointError.isValid());
        }
    } else {
        CHECK_FALSE(result.analysis);
    }
}

TEST_CASE("Local topology shares its exact work ledger across bounds and refinement",
          "[lc-offsetcurve][geometry][cache-topology-local]") {
    const auto source = basis(1, 1, .25, .2501); const auto cache = arcCache(source, 1, 1);
    geo::WorkBudget budget;
    const auto result = geo::analyseCacheTopologyLocally(cache, localTopologyDefinition(source), .01, budget);
    checkLocalTopologyAccepted(result, budget);
    CHECK(result.work.distancePairs > result.analysis->bounds().work.distancePairs);
    CHECK(result.work.vertices >= result.analysis->bounds().work.vertices);
}

TEST_CASE("Local topology exact budgets succeed and one-less pair vertex and depth budgets refuse atomically",
          "[lc-offsetcurve][geometry][cache-topology-local]") {
    const auto source = basis(1, 1, .25, .2501); auto cache = arcCache(source, 1, 3);
    const auto start = cache.controlPoints.front(), end = cache.controlPoints.back();
    const auto chord = end - start;
    cache.controlPoints = {start, start + chord * (7.0 / 6.0), start + chord * (23.0 / 24.0), end};
    geo::WorkBudget baseline;
    const auto complete = geo::analyseCacheTopologyLocally(cache, localTopologyDefinition(source), .01, baseline);
    checkLocalTopologyAccepted(complete, baseline);
    const auto used = complete.work;
    REQUIRE(used.deepest > 1);
    LC_OffsetCheckLimits exact{used.cacheSpans, used.vertices, used.distancePairs, used.deepest};
    geo::WorkBudget exactBudget{exact};
    const auto exactResult = geo::analyseCacheTopologyLocally(cache, localTopologyDefinition(source), .01, exactBudget);
    checkLocalTopologyAccepted(exactResult, exactBudget);
    for (const int resource : {1, 2, 3}) {
        CAPTURE(resource, used.cacheSpans, used.vertices, used.distancePairs, used.deepest);
        auto oneLess = exact;
        if (resource == 1 && oneLess.maxVertices > 0) { --oneLess.maxVertices; }
        if (resource == 2 && oneLess.maxDistancePairs > 0) { --oneLess.maxDistancePairs; }
        if (resource == 3 && oneLess.maxDepth > 0) { --oneLess.maxDepth; }
        geo::WorkBudget limited{oneLess};
        const auto refused = geo::analyseCacheTopologyLocally(cache, localTopologyDefinition(source), .01, limited);
        checkLocalTopologyRefused(refused, limited);
        CHECK(refused.status == geo::CacheLocalTopologyStatus::LimitExceeded);
    }
    for (const int invalidLimit : {0, 1}) {
        auto limits = exact;
        if (invalidLimit == 0) { limits.maxCacheSpans = 0; }
        else { limits.maxDepth = 0; }
        geo::WorkBudget invalid{limits};
        const auto rejected = geo::analyseCacheTopologyLocally(cache, localTopologyDefinition(source), .01, invalid);
        checkLocalTopologyRefused(rejected, invalid);
        CHECK(rejected.status == geo::CacheLocalTopologyStatus::InvalidInput);
    }
    for (const bool spendPair : {false, true}) {
        geo::WorkBudget spent{exact};
        const bool consumed = spendPair ? spent.consumePairs(1) : spent.consumeVertices(1);
        REQUIRE(consumed);
        const auto refused = geo::analyseCacheTopologyLocally(cache, localTopologyDefinition(source), .01, spent);
        checkLocalTopologyRefused(refused, spent);
        CHECK(refused.status == geo::CacheLocalTopologyStatus::LimitExceeded);
    }
    geo::WorkBudget sticky{exact}; sticky.latchLimitExceeded();
    const auto refused = geo::analyseCacheTopologyLocally(cache, localTopologyDefinition(source), .01, sticky);
    checkLocalTopologyRefused(refused, sticky);
    CHECK(refused.status == geo::CacheLocalTopologyStatus::LimitExceeded);
}

namespace {
RS_SplineData accountingBulgeCache(const RS_EllipseData& source) {
    auto cache = arcCache(source, 1, 3);
    const auto start = cache.controlPoints.front(), chord = cache.controlPoints.back() - start;
    const double length = std::hypot(chord.x, chord.y);
    const RS_Vector normal{-chord.y * .03 / length, chord.x * .03 / length};
    // The cubic's actual perpendicular excursion is .03/(2*sqrt(3)) < .01,
    // but the unsplit control hull has .03 deviation and cannot pass a .02 error bound.
    cache.controlPoints[1] = start + chord / 3 + normal;
    cache.controlPoints[2] = start + chord * (2.0 / 3.0) - normal;
    return cache;
}
}

TEST_CASE("cache split atomically charges two children",
          "[lc-offsetcurve][geometry][cache-budget-accounting][cache-accounting]") {
    SECTION("the production admission operation never recreates the popped parent") {
        geo::WorkBudget candidate{LC_OffsetCheckLimits{3, 3, 7, 20}};
        REQUIRE(candidate.consumePieces(1)); REQUIRE(candidate.consumeVertices(1));
        REQUIRE(candidate.admitSubdivision(geo::SubdivisionKind::Candidate));
        checkWork(candidate.consumed(), {3, 3});
        geo::WorkBudget proof{LC_OffsetCheckLimits{1, 3, 7, 20}};
        REQUIRE(proof.consumePieces(1)); REQUIRE(proof.consumeVertices(1));
        REQUIRE(proof.admitSubdivision(geo::SubdivisionKind::Proof));
        checkWork(proof.consumed(), {1, 3});
    }
    SECTION("the bounds call site charges candidate child spans") {
        const auto source = basis(1, 1, .25, .2501);
        const auto cache = accountingBulgeCache(source);
        geo::WorkBudget budget;
        const auto result = geo::analyseCacheBounds(cache, source, 1, .01, budget);
        checkBoundsRelation(result, 1, .01);
        CHECK(result.work.cacheSpans > result.layoutWork.cacheSpans);
        CHECK(std::any_of(result.analysis->witnesses().begin(), result.analysis->witnesses().end(),
            [](const auto& witness) { return witness.cache.depth > 0; }));
    }
    SECTION("the local-topology call site charges its own candidate children") {
        const auto source = basis(1, 1, .25, .2501);
        auto cache = arcCache(source, 1, 3);
        const auto start = cache.controlPoints.front(), chord = cache.controlPoints.back() - start;
        cache.controlPoints = {start, start + chord * (7.0 / 6.0), start + chord * (23.0 / 24.0), start + chord};
        geo::WorkBudget budget;
        const auto result = geo::analyseCacheTopologyLocally(cache, localTopologyDefinition(source), .01, budget);
        checkLocalTopologyAccepted(result, budget);
        const auto added = result.work.cacheSpans - result.analysis->bounds().work.cacheSpans;
        CHECK(added >= 2); CHECK(added % 2 == 0);
    }
    SECTION("critical proof-only callers cannot spend candidate spans") {
        geo::WorkBudget budget{LC_OffsetCheckLimits{1, 65536, 1u << 20, 20}};
        const auto result = geo::analyseCritical(basis(100, .5), -30, budget);
        REQUIRE(result.status == LC_IntersectionStatus::Ok);
        CHECK(result.work.deepest > 0); CHECK(result.work.cacheSpans == 0);
    }
}

TEST_CASE("one-less split budget refuses without partial debit",
          "[lc-offsetcurve][geometry][cache-budget-accounting][cache-accounting]") {
    for (const bool shortSpans : {false, true}) {
        LC_OffsetCheckLimits limits{3, 3, 7, 20};
        if (shortSpans) { --limits.maxCacheSpans; } else { --limits.maxVertices; }
        geo::WorkBudget budget{limits};
        REQUIRE(budget.consumePieces(1)); REQUIRE(budget.consumeVertices(1));
        const auto before = budget.consumed();
        CHECK_FALSE(budget.admitSubdivision(geo::SubdivisionKind::Candidate));
        CHECK(budget.status() == geo::BudgetStatus::LimitExceeded);
        checkWork(budget.consumed(), before);
        CHECK_FALSE(budget.admitSubdivision(geo::SubdivisionKind::Proof));
        checkWork(budget.consumed(), before);
    }
    const auto maximum = std::numeric_limits<std::size_t>::max();
    geo::WorkBudget overflow{LC_OffsetCheckLimits{maximum, maximum, 7, 20}};
    REQUIRE(overflow.consumePieces(maximum - 1)); REQUIRE(overflow.consumeVertices(maximum - 2));
    const auto before = overflow.consumed();
    CHECK_FALSE(overflow.admitSubdivision(geo::SubdivisionKind::Candidate));
    checkWork(overflow.consumed(), before);
    geo::WorkBudget invalid;
    CHECK_FALSE(invalid.admitSubdivision(static_cast<geo::SubdivisionKind>(99)));
    CHECK(invalid.status() == geo::BudgetStatus::InvalidInput); checkWork(invalid.consumed(), {});

    // A real bounds refinement must stop before the last two-span admission, not after its first child.
    const auto source = basis(1, 1, .25, .2501); const auto cache = accountingBulgeCache(source);
    geo::WorkBudget baseline;
    const auto complete = geo::analyseCacheBounds(cache, source, 1, .01, baseline);
    REQUIRE(complete.status == geo::CacheBoundsStatus::Ok);
    REQUIRE(complete.work.cacheSpans > complete.layoutWork.cacheSpans);
    for (const bool shortCap : {false, true}) {
        LC_OffsetCheckLimits limits; limits.maxCacheSpans = complete.work.cacheSpans - shortCap;
        geo::WorkBudget budget{limits};
        const auto result = geo::analyseCacheBounds(cache, source, 1, .01, budget);
        CHECK(result.status == (shortCap ? geo::CacheBoundsStatus::LimitExceeded : geo::CacheBoundsStatus::Ok));
        CHECK(bool(result.analysis) == !shortCap);
        CHECK(result.work.cacheSpans == complete.work.cacheSpans - (shortCap ? 2 : 0));
        if (shortCap) {
            const auto stopped = budget.consumed();
            CHECK(geo::analyseCacheBounds(cache, source, 1, .01, budget).status == geo::CacheBoundsStatus::LimitExceeded);
            checkWork(budget.consumed(), stopped);
        }
    }
}

TEST_CASE("reference depth survives subdivision and enforces combined depth",
          "[lc-offsetcurve][geometry][cache-budget-accounting][cache-accounting]") {
    SECTION("combined admission is exact overflow-safe and sticky without observing an unentered child") {
        geo::WorkBudget budget{LC_OffsetCheckLimits{4096, 65536, 1u << 20, 7}};
        REQUIRE(budget.checkDepth(2, 5)); CHECK(budget.consumed().deepest == 7);
        const auto before = budget.consumed();
        CHECK_FALSE(budget.checkDepth(2, 6)); checkWork(budget.consumed(), before);
        CHECK_FALSE(budget.checkDepth(0, 0)); checkWork(budget.consumed(), before);
        LC_OffsetCheckLimits limits; limits.maxDepth = std::numeric_limits<unsigned>::max();
        geo::WorkBudget large{limits};
        REQUIRE(large.checkDepth(limits.maxDepth - 1, 1));
        const auto stopped = large.consumed();
        CHECK_FALSE(large.checkDepth(1, limits.maxDepth)); checkWork(large.consumed(), stopped);
    }
    SECTION("published local witnesses retain the bounds reference history in both cache orders") {
        const auto source = basis(100, .01, 0, .1);
        for (const bool reversed : {false, true}) {
            // A straight chord has a strictly forward derivative; the high-curvature
            // reference needs genuine refinement even under this coarse internal-layer tolerance.
            auto cache = arcCache(source, 1, 1);
            if (reversed) { std::reverse(cache.controlPoints.begin(), cache.controlPoints.end()); }
            geo::WorkBudget budget;
            const auto result = geo::analyseCacheTopologyLocally(cache, localTopologyDefinition(source, 1, reversed), 2, budget);
            checkLocalTopologyAccepted(result, budget);
            const auto& bounds = result.analysis->bounds();
            bool inherited = false;
            for (const auto& local : result.analysis->witnesses()) {
                CHECK(local.cache.depth <= budget.depthLimit());
                CHECK(local.referenceDepth <= budget.depthLimit() - local.cache.depth);
                CHECK(local.cache.depth + local.referenceDepth <= result.work.deepest);
                for (const auto& input : bounds.analysis->witnesses()) {
                    geo::CacheDyadicDomain domain = input.cache;
                    if (reversed) { REQUIRE(geo::reflectCacheDyadicDomain(input.cache, domain)); }
                    if (local.kind == geo::CacheWitnessKind::Interior && input.kind == local.kind && input.edge == local.edge &&
                        local.cache.span == domain.span && local.cache.parameter.lo() >= domain.parameter.lo() &&
                        local.cache.parameter.hi() <= domain.parameter.hi() &&
                        local.referenceParameter.lo() >= input.referenceParameter.lo() &&
                        local.referenceParameter.hi() <= input.referenceParameter.hi() && input.referenceDepth != 0) {
                        CHECK(local.referenceDepth >= input.referenceDepth);
                        inherited = true;
                    }
                }
            }
            CHECK(inherited);
            LC_OffsetCheckLimits limits{result.work.cacheSpans, result.work.vertices, result.work.distancePairs,
                result.work.deepest - 1, result.work.orderingComparisons};
            geo::WorkBudget limited{limits};
            const auto refused = geo::analyseCacheTopologyLocally(cache, localTopologyDefinition(source, 1, reversed), 2, limited);
            CHECK(refused.status == geo::CacheLocalTopologyStatus::LimitExceeded); CHECK_FALSE(refused.analysis);
            CHECK(refused.work.deepest <= limits.maxDepth);
        }
    }
    SECTION("a one-short combined depth refuses inside local topology after bounds succeeded") {
        const auto source = basis(100, .01, 0, .1);
        auto original = arcCache(source, 1, 3);
        const auto start = original.controlPoints.front(), chord = original.controlPoints.back() - start;
        const double epsilon = std::ldexp(1.0, -18);
        const double scale = 1.0 / 9.0 + epsilon;
        original.controlPoints[1] = start + chord / 3;
        original.controlPoints[2] = start + chord * (1.0 / 3.0 + (-2.0 / 9.0 + epsilon) / (3 * scale));
        // Its ideal scalar derivative is proportional to (t-1/3)^2+epsilon.
        // Prove positivity for the actual rounded world controls, not only the ideal polynomial.
        std::array<EventRational, 3> projected;
        const EventRational cx(chord.x), cy(chord.y);
        for (std::size_t i = 0; i < projected.size(); ++i) {
            projected[i] = 3 * ((EventRational(original.controlPoints[i + 1].x) - EventRational(original.controlPoints[i].x)) * cx +
                (EventRational(original.controlPoints[i + 1].y) - EventRational(original.controlPoints[i].y)) * cy);
        }
        const EventRational a = projected[0] - 2 * projected[1] + projected[2];
        const EventRational b = 2 * (projected[1] - projected[0]);
        REQUIRE(a > 0); REQUIRE(4 * a * projected[0] - b * b > 0);
        for (const bool reversed : {false, true}) {
            auto cache = original;
            if (reversed) { std::reverse(cache.controlPoints.begin(), cache.controlPoints.end()); }
            geo::WorkBudget budget;
            const auto definition = localTopologyDefinition(source, 1, reversed);
            const auto complete = geo::analyseCacheTopologyLocally(cache, definition, 2, budget);
            checkLocalTopologyAccepted(complete, budget);
            const auto& bounds = complete.analysis->bounds();
            REQUIRE(complete.work.deepest > bounds.work.deepest);
            LC_OffsetCheckLimits limits{complete.work.cacheSpans, complete.work.vertices, complete.work.distancePairs,
                complete.work.deepest - 1, complete.work.orderingComparisons};
            REQUIRE(limits.maxDepth >= bounds.work.deepest);
            geo::WorkBudget limited{limits};
            const auto refused = geo::analyseCacheTopologyLocally(cache, definition, 2, limited);
            REQUIRE(refused.boundsStatus == geo::CacheBoundsStatus::Ok);
            CHECK(refused.status == geo::CacheLocalTopologyStatus::LimitExceeded); CHECK_FALSE(refused.analysis);
            CHECK(refused.work.deepest == limits.maxDepth);
        }
    }
}

TEST_CASE("bounds dispatch and coverage advancement charge before work",
          "[lc-offsetcurve][geometry][cache-budget-accounting][cache-accounting]") {
    const auto source = basis(1, 1, .25, .2501); const auto cache = arcCache(source, 1, 3);
    geo::WorkBudget baseline;
    const auto complete = geo::analyseCacheBounds(cache, source, 1, .01, baseline);
    REQUIRE(complete.status == geo::CacheBoundsStatus::Ok); REQUIRE(complete.analysis);
    const auto& reference = complete.analysis->reference();
    REQUIRE(reference.edges().size() == 1); REQUIRE(reference.fringes().size() == 2);
    REQUIRE(reference.events().occurrences().size() == 4); REQUIRE(complete.analysis->witnesses().size() == 3);
    for (const auto& witness : complete.analysis->witnesses()) {
        REQUIRE(witness.cache.depth == 0); REQUIRE(witness.referenceDepth == 0);
    }
    // Prepare: two fringe-index visits, one edge, two fringe fragments = five pairs.
    // Compare: one span + three (dispatch, examination) pairs. Coverage: span, advance, three witnesses.
    CHECK(complete.work.distancePairs == complete.referenceWork.distancePairs + 5 + 1 + 6 + 5);
    CHECK(complete.work.vertices == complete.referenceWork.vertices + 1 + 4 + 3 + 6);
    const std::array<std::size_t, 18> generated{{5, 5, 5, 6, 7, 8, 8, 9, 10, 11, 12, 13, 14, 14, 14, 14, 14, 14}};
    for (const bool prespent : {false, true}) {
        for (std::size_t pairs = 0; pairs <= 17; ++pairs) {
            CAPTURE(prespent, pairs);
            LC_OffsetCheckLimits limits;
            limits.maxDistancePairs = complete.referenceWork.distancePairs + pairs + (prespent ? 3 : 0);
            geo::WorkBudget budget{limits};
            if (prespent) { REQUIRE(budget.consumeVertices(2)); REQUIRE(budget.consumePairs(3)); }
            const auto result = geo::analyseCacheBounds(cache, source, 1, .01, budget);
            REQUIRE(result.referenceStatus == LC_IntersectionStatus::Ok);
            CHECK(result.status == (pairs == 17 ? geo::CacheBoundsStatus::Ok : geo::CacheBoundsStatus::LimitExceeded));
            CHECK(bool(result.analysis) == (pairs == 17)); CHECK(result.work.distancePairs == limits.maxDistancePairs);
            CHECK(result.work.vertices == complete.referenceWork.vertices + generated[pairs] + (prespent ? 2 : 0));
            checkWork(result.work, budget.consumed());
            if (pairs != 17) {
                const auto stopped = budget.consumed();
                CHECK(geo::analyseCacheBounds(cache, source, 1, .01, budget).status == geo::CacheBoundsStatus::LimitExceeded);
                checkWork(budget.consumed(), stopped);
            }
        }
    }
}

namespace {
constexpr std::size_t noTerminal = std::size_t(-1);
RS_EllipseData terminalSource(RS_EllipseData source, const bool reversed) {
    if (reversed) { std::swap(source.angle1, source.angle2); source.reversed = true; }
    return source;
}
geo::ReferenceComponentResult terminalGraph(const RS_EllipseData& source, const double distance) {
    geo::WorkBudget budget;
    auto result = geo::analyseReferenceComponents(source, distance, 1e-5, budget);
    CAPTURE(result.status, result.reason, result.eventReason);
    REQUIRE(result.status == LC_IntersectionStatus::Ok); REQUIRE(result.analysis);
    checkWork(result.work, budget.consumed());
    return result;
}
void checkTerminalMetadata(const geo::ReferenceComponentAnalysis& analysis) {
    const auto& occurrences = analysis.events().occurrences();
    REQUIRE(analysis.occurrenceIncidence().size() == occurrences.size());
    std::vector<std::size_t> incoming(occurrences.size(), noTerminal), outgoing = incoming;
    for (std::size_t e = 0; e < analysis.edges().size(); ++e) {
        const auto& edge = analysis.edges()[e]; const auto& span = analysis.spans()[edge.span];
        REQUIRE(outgoing[span.start.value] == noTerminal); REQUIRE(incoming[span.end.value] == noTerminal);
        outgoing[span.start.value] = e; incoming[span.end.value] = e;
        for (const auto side : {geo::ReferenceEdgeSide::Start, geo::ReferenceEdgeSide::End}) {
            const auto* terminal = analysis.terminal(e, side);
            if (!terminal) { continue; }
            const bool start = side == geo::ReferenceEdgeSide::Start;
            CHECK(terminal->role == geo::ReferenceRole::Own); CHECK(terminal->edge == e); CHECK(terminal->side == side);
            CHECK(terminal->occurrence.value == (start ? span.start.value : span.end.value));
            CHECK(terminal->node.value == (start ? edge.start.value : edge.end.value));
            CHECK(occurrences[terminal->occurrence.value].node.value == terminal->node.value);
            if (terminal->kind == geo::ReferenceTerminalKind::PairTrim) {
                CHECK(terminal->freeEnd == noTerminal);
                REQUIRE(terminal->pairEvent < analysis.events().pairs().size());
                const auto& pair = analysis.events().pairs()[terminal->pairEvent];
                CHECK(((pair.first.value == terminal->occurrence.value && pair.second.value == terminal->partner.value) ||
                       (pair.second.value == terminal->occurrence.value && pair.first.value == terminal->partner.value)));
                CHECK(occurrences[terminal->partner.value].node.value == terminal->node.value);
            } else {
                CHECK(terminal->pairEvent == noTerminal); CHECK(terminal->partner.value == noTerminal);
                REQUIRE(terminal->freeEnd < analysis.events().freeEnds().size());
                const auto& freeEnd = analysis.events().freeEnds()[terminal->freeEnd];
                CHECK(freeEnd.occurrence.value == terminal->occurrence.value); CHECK(freeEnd.atStart == start);
                CHECK(terminal->kind == (start ? geo::ReferenceTerminalKind::SourceStart : geo::ReferenceTerminalKind::SourceEnd));
                const double parameter = start ? 0.0 : analysis.events().providers().frame().parameterEnd();
                CHECK(occurrences[terminal->occurrence.value].parameter.lo() == parameter);
                CHECK(occurrences[terminal->occurrence.value].parameter.hi() == parameter);
            }
        }
        CHECK_FALSE(analysis.terminal(e, static_cast<geo::ReferenceEdgeSide>(99)));
    }
    for (std::size_t i = 0; i < occurrences.size(); ++i) {
        CHECK(analysis.occurrenceIncidence()[i].incoming == incoming[i]);
        CHECK(analysis.occurrenceIncidence()[i].outgoing == outgoing[i]);
    }
    CHECK_FALSE(analysis.terminal(noTerminal, geo::ReferenceEdgeSide::Start));
}
}

TEST_CASE("source start maps to exact terminal", "[lc-offsetcurve][geometry][reference-terminals]") {
    for (const bool reversed : {false, true}) {
        const auto source = terminalSource(basis(1, 1, .25, .2501), reversed);
        const auto result = terminalGraph(source, 1);
        const auto& analysis = *result.analysis; REQUIRE(analysis.edges().size() == 1);
        const auto* terminal = analysis.terminal(0, geo::ReferenceEdgeSide::Start); REQUIRE(terminal);
        CHECK(terminal->kind == geo::ReferenceTerminalKind::SourceStart);
        checkTerminalMetadata(analysis);
    }
    SECTION("terminal tables and all scan prefixes have exact cumulative caps") {
        const auto source = basis(1, 1, .25, .2501);
        const auto measured = terminalGraph(source, 1);
        REQUIRE(measured.analysis->events().occurrences().size() == 4);
        REQUIRE(measured.analysis->edges().size() == 1);
        // Four occurrence slots + two edge-side slots, admitted together after the graph.
        LC_OffsetCheckLimits limits;
        limits.maxVertices = measured.work.vertices - 1;
        geo::WorkBudget shortRecords{limits};
        const auto refused = geo::analyseReferenceComponents(source, 1, 1e-5, shortRecords);
        CHECK(refused.status == LC_IntersectionStatus::LimitExceeded); CHECK_FALSE(refused.analysis);
        CHECK(refused.work.vertices == measured.work.vertices - 6);
        // Incidence: edge + two ends = 3 pairs. Classification: edge + two ends + 2*two free ends = 7.
        for (std::size_t pairs = 0; pairs <= 10; ++pairs) {
            limits = {}; limits.maxVertices = measured.work.vertices;
            limits.maxDistancePairs = measured.work.distancePairs - 10 + pairs;
            geo::WorkBudget budget{limits};
            const auto result = geo::analyseReferenceComponents(source, 1, 1e-5, budget);
            CHECK(result.status == (pairs == 10 ? LC_IntersectionStatus::Ok : LC_IntersectionStatus::LimitExceeded));
            CHECK(bool(result.analysis) == (pairs == 10));
            CHECK(result.work.distancePairs == limits.maxDistancePairs); CHECK(result.work.vertices == measured.work.vertices);
            checkWork(result.work, budget.consumed());
            if (pairs != 10) {
                const auto stopped = budget.consumed();
                CHECK(geo::analyseReferenceComponents(source, 1, 1e-5, budget).status == LC_IntersectionStatus::LimitExceeded);
                checkWork(budget.consumed(), stopped);
            }
        }
    }
}

TEST_CASE("source end maps to exact terminal", "[lc-offsetcurve][geometry][reference-terminals]") {
    for (const bool reversed : {false, true}) {
        const auto source = terminalSource(basis(1, 1, .25, .2501), reversed);
        auto result = terminalGraph(source, 1); // The helper's source budget has already died.
        const auto* terminal = result.analysis->terminal(0, geo::ReferenceEdgeSide::End); REQUIRE(terminal);
        CHECK(terminal->kind == geo::ReferenceTerminalKind::SourceEnd);
        checkTerminalMetadata(*result.analysis);
        LC_OffsetCheckLimits limits{1, result.work.vertices + 3, result.work.distancePairs + 7, 20, result.work.orderingComparisons + 5};
        geo::WorkBudget prespent{limits};
        REQUIRE(prespent.consumePieces(1)); REQUIRE(prespent.consumeVertices(3)); REQUIRE(prespent.consumePairs(7));
        REQUIRE(prespent.consumeOrderingComparisons(5)); REQUIRE(prespent.checkDepth(15));
        const auto again = geo::analyseReferenceComponents(source, 1, 1e-5, prespent);
        REQUIRE(again.status == LC_IntersectionStatus::Ok);
        checkWork(again.work, {1, result.work.vertices + 3, result.work.distancePairs + 7,
            std::max(15u, result.work.deepest), result.work.orderingComparisons + 5});
        CHECK(again.analysis->terminal(0, geo::ReferenceEdgeSide::End)->kind == geo::ReferenceTerminalKind::SourceEnd);
    }
}

TEST_CASE("pair trim terminates without paired continuation", "[lc-offsetcurve][geometry][reference-terminals]") {
    const auto original = basis(100, .05, M_PI / 12, 7 * M_PI / 4);
    const Quad cx = Quad(100) * cos(Quad(original.angle2)), cy = Quad(5) * sin(Quad(original.angle2));
    const auto point = [](const Quad& theta) -> std::array<Quad, 2> {
        const Quad c = cos(theta), s = sin(theta), speed = sqrt(Quad(25) * c * c + Quad(10000) * s * s);
        return {Quad(100) * c - Quad(20) * c / speed, Quad(5) * s - Quad(400) * s / speed};
    };
    const auto residual = [&](const Quad& theta) -> Quad {
        const auto p = point(theta); const Quad x = p[0] - cx, y = p[1] - cy;
        return x * x + y * y - 16;
    };
    Quad lo(".7"), hi(".75"); REQUIRE(residual(lo) > 0); REQUIRE(residual(hi) < 0);
    // F>0 and h<0 make this first endpoint-circle crossing unique on [.7,.75].
    CHECK(sin(lo) > Quad(".64")); CHECK(cos(hi) > Quad(".73")); CHECK(cx < 71); CHECK(abs(cy) < 5);
    CHECK(Quad(".64") * (7100 - Quad(9975) * Quad(".73")) + 25 < 0);
    CHECK(Quad(64) * 64 * 64 / 500 > 4);
    for (unsigned i = 0; i < 80; ++i) {
        const Quad mid = (lo + hi) / 2;
        if (residual(mid) > 0) { lo = mid; } else { hi = mid; }
    }
    const auto p = point((lo + hi) / 2);
    CHECK((p[0] - cx) * (-Quad(100) * sin(Quad(original.angle2))) +
          (p[1] - cy) * (Quad(5) * cos(Quad(original.angle2))) > 0); // Actual end-cutter half-space.
    for (const bool reversed : {false, true}) {
        const auto source = terminalSource(original, reversed); const auto result = terminalGraph(source, -4);
        const auto& analysis = *result.analysis; checkTerminalMetadata(analysis);
        std::size_t found = 0;
        for (std::size_t e = 0; e < analysis.edges().size(); ++e) {
            for (const auto side : {geo::ReferenceEdgeSide::Start, geo::ReferenceEdgeSide::End}) {
                const auto* terminal = analysis.terminal(e, side);
                if (!terminal || terminal->kind != geo::ReferenceTerminalKind::PairTrim) { continue; }
                ++found; CHECK(side == (reversed ? geo::ReferenceEdgeSide::Start : geo::ReferenceEdgeSide::End));
                const auto& occurrences = analysis.events().occurrences();
                CHECK(occurrences[terminal->partner.value].role ==
                    (reversed ? geo::ReferenceRole::StartCutter : geo::ReferenceRole::EndCutter));
                const Quad duration(analysis.events().providers().frame().parameterEnd());
                const Quad sweep = idealDirectedSweep(source, false);
                const Quad a = (lo - Quad(source.angle1)) * duration / sweep;
                const Quad b = (hi - Quad(source.angle1)) * duration / sweep;
                checkEnclosure(occurrences[terminal->occurrence.value].parameter, a);
                checkEnclosure(occurrences[terminal->occurrence.value].parameter, b);
                const auto& own = analysis.occurrenceIncidence()[terminal->occurrence.value];
                CHECK((side == geo::ReferenceEdgeSide::Start ? own.incoming : own.outgoing) == noTerminal);
            }
        }
        CHECK(found == 1);
    }
}

TEST_CASE("paired retained continuation blocks trim terminal", "[lc-offsetcurve][geometry][reference-terminals]") {
    const Quad pi = boost::math::constants::pi<Quad>();
    const Quad alpha = asin(sqrt(Quad(7104) / 7500)); // s=98 for E(100,50), D=-49.
    for (const bool reversed : {false, true}) {
        const auto source = terminalSource(basis(100, .5), reversed); const auto result = terminalGraph(source, -49);
        const auto& analysis = *result.analysis; checkTerminalMetadata(analysis);
        REQUIRE(analysis.edges().size() == 2); REQUIRE(analysis.events().pairs().size() == 2);
        const auto& frame = analysis.events().providers().frame();
        const Quad phase(frame.sourceSweep().start), duration(frame.parameterEnd());
        std::array<Quad, 4> parameters{{alpha, pi - alpha, pi + alpha, 2 * pi - alpha}};
        for (auto& theta : parameters) {
            if (reversed && theta > phase) { theta -= 2 * pi; }
            if (!reversed && theta < phase) { theta += 2 * pi; }
            theta = (theta - phase) * duration / (reversed ? -2 * pi : 2 * pi);
        }
        std::sort(parameters.begin(), parameters.end());
        for (std::size_t e = 0; e < 2; ++e) {
            const auto& span = analysis.spans()[analysis.edges()[e].span];
            for (const auto side : {geo::ReferenceEdgeSide::Start, geo::ReferenceEdgeSide::End}) {
                const bool start = side == geo::ReferenceEdgeSide::Start;
                const auto id = start ? span.start : span.end;
                checkEnclosure(analysis.events().occurrences()[id.value].parameter, parameters[2 * e + (start ? 0 : 1)]);
                CHECK_FALSE(analysis.terminal(e, side)); // In particular, E0 alone has no authorized trim ends.
                bool paired = false;
                for (const auto& event : analysis.events().pairs()) {
                    if (event.first.value != id.value && event.second.value != id.value) { continue; }
                    const auto partner = event.first.value == id.value ? event.second : event.first;
                    REQUIRE(analysis.events().occurrences()[partner.value].role == geo::ReferenceRole::Own);
                    const auto& incidence = analysis.occurrenceIncidence()[partner.value];
                    CHECK((start ? incidence.incoming : incidence.outgoing) != noTerminal); paired = true;
                }
                CHECK(paired);
            }
        }
        REQUIRE(analysis.events().seam());
        const auto node = analysis.events().occurrences()[analysis.events().seam()->start.value].node;
        CHECK(analysis.incidence()[node.value].hiddenPoint);
    }
}

TEST_CASE("through event is not a terminal", "[lc-offsetcurve][geometry][reference-terminals]") {
    for (const double ratio : {1.0, .5}) {
        for (const bool reversed : {false, true}) {
            const auto source = terminalSource(basis(5, ratio), reversed);
            const auto result = terminalGraph(source, 3);
            const auto& analysis = *result.analysis; checkTerminalMetadata(analysis);
            REQUIRE(analysis.events().seam());
            REQUIRE(analysis.events().freeEnds().empty());
            REQUIRE(analysis.edges().size() == 1);
            const auto& seam = *analysis.events().seam();
            const auto& first = analysis.occurrenceIncidence()[seam.start.value];
            const auto& last = analysis.occurrenceIncidence()[seam.end.value];
            REQUIRE(first.outgoing == 0); REQUIRE(last.incoming == 0);
            CHECK(first.incoming == noTerminal); CHECK(last.outgoing == noTerminal);
            const auto& start = analysis.events().occurrences()[seam.start.value];
            const auto& end = analysis.events().occurrences()[seam.end.value];
            REQUIRE(start.node.value == end.node.value);
            const auto& physical = analysis.incidence()[start.node.value];
            REQUIRE(physical.incoming.size() == 1); REQUIRE(physical.outgoing.size() == 1);
            CHECK_FALSE(physical.hiddenPoint); CHECK_FALSE(physical.trimBoundary);
            CHECK_FALSE(analysis.terminal(0, geo::ReferenceEdgeSide::Start));
            CHECK_FALSE(analysis.terminal(0, geo::ReferenceEdgeSide::End));
        }
    }
}

TEST_CASE("wrong paired occurrence orientation is not a terminal", "[lc-offsetcurve][geometry][reference-terminals]") {
    const auto result = terminalGraph(basis(100, .05, M_PI / 12, 7 * M_PI / 4), -4);
    const auto& analysis = *result.analysis;
    std::size_t checked = 0;
    for (std::size_t e = 0; e < analysis.edges().size(); ++e) {
        for (const auto side : {geo::ReferenceEdgeSide::Start, geo::ReferenceEdgeSide::End}) {
            const auto* terminal = analysis.terminal(e, side);
            if (!terminal || terminal->kind != geo::ReferenceTerminalKind::PairTrim) { continue; }
            const auto wrongSide = side == geo::ReferenceEdgeSide::Start ? geo::ReferenceEdgeSide::End : geo::ReferenceEdgeSide::Start;
            const auto* wrong = analysis.terminal(e, wrongSide);
            CHECK((!wrong || wrong->occurrence.value != terminal->occurrence.value));
            const auto& partner = analysis.occurrenceIncidence()[terminal->partner.value];
            CHECK(partner.incoming == noTerminal); CHECK(partner.outgoing == noTerminal);
            CHECK_FALSE(analysis.terminal(partner.incoming, wrongSide)); ++checked;
        }
    }
    CHECK(checked == 1);
}

TEST_CASE("hidden simple cusp is not a terminal", "[lc-offsetcurve][geometry][reference-terminals]") {
    const auto source = basis(100, .05, M_PI / 12, 7 * M_PI / 4);
    const auto result = terminalGraph(source, -4); const auto& analysis = *result.analysis;
    std::size_t hidden = 0;
    for (const auto& critical : analysis.events().criticals()) {
        if (critical.kind != geo::CriticalKind::SimpleCusp) { continue; }
        const auto& occurrence = analysis.events().occurrences()[critical.occurrence.value];
        REQUIRE(analysis.incidence()[occurrence.node.value].hiddenPoint);
        const auto& incidence = analysis.occurrenceIncidence()[critical.occurrence.value];
        CHECK(incidence.incoming == noTerminal); CHECK(incidence.outgoing == noTerminal);
        const double t = occurrence.parameter.lo() + (occurrence.parameter.hi() - occurrence.parameter.lo()) / 2;
        CHECK_FALSE(componentVisibilityOracle(source, -4, analysis.events().providers().frame(), t));
        for (std::size_t e = 0; e < analysis.edges().size(); ++e) {
            for (const auto side : {geo::ReferenceEdgeSide::Start, geo::ReferenceEdgeSide::End}) {
                const auto* terminal = analysis.terminal(e, side);
                CHECK((!terminal || terminal->occurrence.value != critical.occurrence.value));
            }
        }
        ++hidden;
    }
    CHECK(hidden > 0);
}

TEST_CASE("arbitrary interior point is not a terminal", "[lc-offsetcurve][geometry][reference-terminals]") {
    const auto result = terminalGraph(basis(1, 1, .25, .2501), 1); const auto& analysis = *result.analysis;
    const double interior = analysis.events().providers().frame().parameterEnd() / 2;
    for (const auto& occurrence : analysis.events().occurrences()) {
        if (occurrence.role == geo::ReferenceRole::Own) { CHECK_FALSE(occurrence.parameter.contains(interior)); }
    }
    for (std::size_t e = 0; e < analysis.edges().size(); ++e) {
        for (const auto side : {geo::ReferenceEdgeSide::Start, geo::ReferenceEdgeSide::End}) {
            const auto* terminal = analysis.terminal(e, side); REQUIRE(terminal);
            CHECK_FALSE(analysis.events().occurrences()[terminal->occurrence.value].parameter.contains(interior));
        }
    }
    // Only issued edge/side identities can be queried; no coordinate-based promotion exists.
    CHECK_FALSE(analysis.terminal(analysis.edges().size(), geo::ReferenceEdgeSide::Start));
    CHECK_FALSE(analysis.terminal(0, static_cast<geo::ReferenceEdgeSide>(99)));
}

namespace {
void checkPublicPath(const LC_OffsetCheckResult& result, const geo::WorkBudget& budget, const double distance,
                     const double tolerance) {
    CAPTURE(result.reason, result.span, result.work.cacheSpans, result.work.vertices,
            result.work.distancePairs, result.work.orderingComparisons, result.work.deepest);
    REQUIRE(result.reason == LC_OffsetCheckResult::Reason::None); REQUIRE(bool(result)); REQUIRE(result.analysis);
    checkWork(result.work, budget.consumed());
    const auto& proof = *result.analysis;
    REQUIRE_FALSE(proof.pieces().empty()); REQUIRE_FALSE(proof.edges().empty());
    const double scale = proof.reference().events().providers().frame().scale();
    const auto threshold = LC_Interval::point(2) * LC_Interval::point(tolerance) / LC_Interval::point(scale);
    const auto floor = (LC_Interval::point(distance) - LC_Interval::point(2.5) * LC_Interval::point(tolerance)) /
        LC_Interval::point(scale);
    std::size_t span = 0;
    double candidate = 0;
    for (const auto& piece : proof.pieces()) {
        if (piece.normalizedSpan != span) {
            REQUIRE(candidate == 1); REQUIRE(piece.normalizedSpan == span + 1);
            ++span; candidate = 0;
        }
        CHECK(piece.candidateParameter.lo() == candidate);
        REQUIRE(piece.candidateParameter.lo() < piece.candidateParameter.hi());
        candidate = piece.candidateParameter.hi();
        REQUIRE(piece.referenceParameter.lo() < piece.referenceParameter.hi());
        CHECK(piece.geometryError.hi() <= threshold.lo()); CHECK(piece.clearanceLowerBound.lo() >= floor.hi());
        CHECK(piece.candidateDepth + piece.referenceDepth <= budget.depthLimit());
    }
    CHECK(candidate == 1); CHECK(span + 1 == proof.layout().spanCount());
    for (const auto edge : proof.edges()) {
        const auto& reference = proof.reference().spans()[proof.reference().edges()[edge].span];
        std::vector<const geo::CachePathPiece*> intervals;
        for (const auto& piece : proof.pieces()) { if (piece.edge == edge) { intervals.push_back(&piece); } }
        REQUIRE_FALSE(intervals.empty());
        std::sort(intervals.begin(), intervals.end(), [](const auto* a, const auto* b) {
            return a->referenceParameter.lo() < b->referenceParameter.lo();
        });
        double covered = reference.interior.lo();
        bool symbolicJoin = false;
        const geo::CachePathPiece* previous = nullptr;
        unsigned startsAtCut = 0, endsAtCut = 0;
        for (const auto* piece : intervals) {
            const auto& interval = piece->referenceParameter;
            startsAtCut += piece->startsAtCut; endsAtCut += piece->endsAtCut;
            if (piece->startsAtCut || piece->endsAtCut) {
                REQUIRE(proof.cut()); REQUIRE_FALSE(proof.cut()->transition); CHECK(proof.cut()->edge == edge);
                if (piece->startsAtCut) { CHECK(interval.lo() == proof.cut()->referenceParameter.lo()); }
                if (piece->endsAtCut) { CHECK(interval.hi() == proof.cut()->referenceParameter.hi()); }
            }
            if (previous && previous->endsAtCut && piece->startsAtCut) {
                // Enclosing boxes overlap, but actual domains meet at ONE shared certified root.
                REQUIRE_FALSE(symbolicJoin); symbolicJoin = true;
                CHECK(covered == proof.cut()->referenceParameter.hi());
                CHECK(interval.lo() == proof.cut()->referenceParameter.lo());
            } else { CHECK(interval.lo() == covered); }
            covered = interval.hi(); previous = piece;
        }
        CHECK(covered == reference.interior.hi());
        const bool cutEdge = proof.cut() && !proof.cut()->transition && proof.cut()->edge == edge;
        CHECK(symbolicJoin == cutEdge); CHECK(startsAtCut == unsigned(cutEdge)); CHECK(endsAtCut == unsigned(cutEdge));
        for (const auto side : {geo::ReferenceEdgeSide::Start, geo::ReferenceEdgeSide::End}) {
            CHECK(std::count_if(proof.endpoints().begin(), proof.endpoints().end(), [&](const auto& endpoint) {
                return endpoint.edge == edge && endpoint.side == side;
            }) == 1);
        }
    }
    if (proof.closed()) {
        REQUIRE(proof.cut()); CHECK_FALSE(proof.startTerminal()); CHECK_FALSE(proof.endTerminal());
        const auto& cut = *proof.cut();
        CHECK(cut.candidateParameter.lo() == 0); CHECK(cut.candidateParameter.hi() == 0);
        CHECK(cut.closingCandidateParameter.lo() == 1); CHECK(cut.closingCandidateParameter.hi() == 1);
        if (!cut.transition) {
            CHECK(cut.proofParameter.lo() <= cut.referenceParameter.lo());
            CHECK(cut.referenceParameter.hi() <= cut.proofParameter.hi());
            CHECK(cut.firstSign.hi() < 0); CHECK(cut.lastSign.lo() > 0); CHECK(cut.derivativeProjection.lo() > 0);
            CHECK(cut.geometryError.hi() <= threshold.lo()); CHECK(cut.clearanceLowerBound.lo() >= floor.hi());
            CHECK(cut.referenceImageDiameter.hi() <= threshold.lo()/32);
            CHECK(proof.pieces().front().startsAtCut); CHECK(proof.pieces().back().endsAtCut);
            CHECK(cut.referenceParameter.lo() == cut.closingReferenceParameter.lo());
            CHECK(cut.referenceParameter.hi() == cut.closingReferenceParameter.hi());
        }
    }
    else { REQUIRE(proof.startTerminal()); REQUIRE(proof.endTerminal()); CHECK_FALSE(proof.cut()); }
}
LC_OffsetCheckResult publicArc(const std::size_t degree, const bool reversed, geo::WorkBudget& budget) {
    const auto source = basis(1, 1, .25, .2501);
    auto cache = arcCache(source, 1, degree);
    if (reversed) { std::reverse(cache.controlPoints.begin(), cache.controlPoints.end()); }
    return geo::checkCache(cache, localTopologyDefinition(source, 1, reversed), .01, budget);
}
RS_SplineData phaseRing(const bool split, const bool sourceReversed = false, const bool cacheReversed = false,
                        const bool rotateKnot = false, const double splitParameter = .25) {
    using Net = std::array<RS_Vector, 4>;
    std::vector<Net> pieces;
    Net net{{{2, 0}, {2, 9.0 / 8}, {9.0 / 8, 2}, {0, 2}}};
    for (unsigned quadrant = 0; quadrant < 4; ++quadrant) {
        pieces.push_back(net);
        for (auto& p : net) { p = {-p.y, p.x}; }
    }
    if (split) {
        net = pieces.front(); Net left, right;
        left[0] = net[0]; right[3] = net[3];
        for (unsigned level = 1; level <= 3; ++level) {
            for (unsigned i = 0; i <= 3 - level; ++i) {
                net[i] = net[i] * (1-splitParameter) + net[i + 1] * splitParameter;
            }
            left[level] = net[0]; right[3 - level] = net[3 - level];
        }
        pieces.front() = left; pieces.insert(pieces.begin() + 1, right);
        std::rotate(pieces.begin(), pieces.begin() + 1, pieces.end());
    } else if (rotateKnot) { std::rotate(pieces.begin(), pieces.begin() + 1, pieces.end()); }
    std::vector<double> knots;
    for (std::size_t i = 0; i <= pieces.size(); ++i) { knots.push_back(double(i)); }
    auto cache = cacheLayoutFixture(3, knots);
    for (std::size_t i = 0; i < pieces.size(); ++i) {
        for (std::size_t j = 0; j <= 3; ++j) { cache.controlPoints[3 * i + j] = pieces[i][j]; }
    }
    if (sourceReversed != cacheReversed) { std::reverse(cache.controlPoints.begin(), cache.controlPoints.end()); }
    return cache;
}
void checkPhaseOracle(const LC_OffsetCheckResult& result, const bool sourceReversed, const bool split,
                      const bool north = false) {
    REQUIRE(result.analysis); REQUIRE(result.analysis->cut());
    const auto& cut = *result.analysis->cut(); REQUIRE_FALSE(cut.transition);
    const auto& frame = result.analysis->reference().events().providers().frame();
    const Quad pi = boost::math::constants::pi<Quad>();
    const Quad angle = split ? atan2(Quad(403), Quad(945)) : north ? pi / 2 : Quad(0);
    Quad difference = angle - Quad(frame.sourceSweep().start);
    if (sourceReversed && difference > 0) { difference -= 2 * pi; }
    if (!sourceReversed && difference < 0) { difference += 2 * pi; }
    const Quad parameter = difference * Quad(frame.parameterEnd()) / (sourceReversed ? -2 * pi : 2 * pi);
    checkEnclosure(cut.referenceParameter, parameter);
    CHECK(2 * (sqrt(Quad(1) + Quad(89) / 8192) - 1) < Quad(".011"));
    CHECK(Quad(114) - 150 * Quad(".25") > 0); // radial error polynomial increases for v in [0,1/4]
    // Bernstein product coefficients of x*y'-y*x' are positive on the entire quarter.
    const std::array<std::array<Quad, 2>, 4> p{{{{2,0}},{{2,Quad(9)/8}},{{Quad(9)/8,2}},{{0,2}}}};
    constexpr unsigned c3[] = {1,3,3,1}, c2[] = {1,2,1}, c5[] = {1,5,10,10,5,1};
    for (unsigned k = 0; k <= 5; ++k) {
        Quad coefficient = 0;
        for (unsigned i = 0; i <= 3; ++i) {
            if (k < i || k - i > 2) { continue; }
            const auto j = k - i;
            const Quad dx = 3 * (p[j+1][0] - p[j][0]), dy = 3 * (p[j+1][1] - p[j][1]);
            coefficient += (p[i][0] * dy - p[i][1] * dx) * c3[i] * c2[j] / c5[k];
        }
        CHECK(coefficient > 0);
    }
}
RS_SplineData materializedCache(const RS_EllipseData& source, const double distance, const double tolerance) {
    const RS_Ellipse ellipse{nullptr, source};
    const auto side = ((distance < 0) != source.reversed) ? LC_CurveOffsetSide::Left : LC_CurveOffsetSide::Right;
    auto options = LC_CurveOffset::makeDirectOptions(ellipse, std::abs(distance), tolerance);
    if (distance < 0) { options.mode = LC_CurveOffsetMode::Trimmed; }
    const auto made = LC_CurveOffset::createEntities(ellipse, LC_CurveOffset::makeSideRequest(side, std::abs(distance)),
        options, LC_CurveOffset::makeDirectSourceBudget());
    REQUIRE(made.status == LC_CurveOffsetStatus::Ok); REQUIRE(made.entities.size() == 1);
    const auto* spline = dynamic_cast<const RS_Spline*>(made.entities[0].get()); REQUIRE(spline);
    return spline->getData();
}
RS_SplineData cachePieces(const RS_SplineData& source, const std::size_t begin, const std::size_t end) {
    REQUIRE(begin < end);
    const auto degree = std::size_t(source.degree);
    REQUIRE(degree * end < source.controlPoints.size());
    std::vector<double> knots;
    for (std::size_t i = begin; i <= end; ++i) { knots.push_back(double(i)); }
    auto result = cacheLayoutFixture(degree, knots);
    std::copy(source.controlPoints.begin() + degree * begin, source.controlPoints.begin() + degree * end + 1,
        result.controlPoints.begin());
    return result;
}
void checkPublicRefusal(const LC_OffsetCheckResult& result, const LC_OffsetCheckResult::Reason reason,
                        const geo::WorkBudget& budget) {
    CAPTURE(result.reason, result.span, result.work.cacheSpans, result.work.vertices,
        result.work.distancePairs, result.work.deepest, result.work.orderingComparisons);
    CHECK(result.reason == reason); CHECK_FALSE(result.analysis); CHECK_FALSE(bool(result));
    checkWork(result.work, budget.consumed());
}
std::array<RS_SplineData, 2> twoComponentCaches() {
    const auto source = basis(100, .05, M_PI / 12, 7 * M_PI / 4);
    const RS_Ellipse ellipse{nullptr, source};
    auto options = LC_CurveOffset::makeDirectOptions(ellipse, 4, 1e-4); options.mode = LC_CurveOffsetMode::Trimmed;
    const auto made = LC_CurveOffset::createEntities(ellipse,
        LC_CurveOffset::makeSideRequest(LC_CurveOffsetSide::Left, 4), options, LC_CurveOffset::makeDirectSourceBudget());
    CAPTURE(made.status, made.entities.size());
    REQUIRE(made.status == LC_CurveOffsetStatus::Ok); REQUIRE(made.entities.size() == 2);
    std::array<RS_SplineData, 2> caches;
    for (unsigned i = 0; i < 2; ++i) {
        const auto* spline = dynamic_cast<const RS_Spline*>(made.entities[i].get()); REQUIRE(spline);
        caches[i] = spline->getData();
    }
    if (caches[0].controlPoints.front() == caches[0].controlPoints.back()) { std::swap(caches[0], caches[1]); }
    REQUIRE(caches[0].controlPoints.front() != caches[0].controlPoints.back());
    REQUIRE(caches[1].controlPoints.front() == caches[1].controlPoints.back());
    return caches;
}
RS_SplineData upperLensCache() {
    // Independent s=98 crossing: sin(alpha)^2=7104/7500 for E(100,50), D=-49.
    const Quad pi = boost::math::constants::pi<Quad>();
    const Quad alpha = asin(sqrt(Quad(7104) / 7500));
    const Quad width = pi - 2 * alpha;
    const auto jet = [](const Quad& angle) -> std::array<Quad, 4> {
        const Quad c = cos(angle), s = sin(angle), h = sqrt(Quad(25) * c*c + 100*s*s);
        const Quad factor = 1 - Quad(24500) / (100 * h*h*h);
        return {100*c - 245*c/h, 50*s - 490*s/h, -100*s*factor, 50*c*factor};
    };
    std::vector<double> knots; for (unsigned i = 0; i <= 16; ++i) { knots.push_back(double(i)); }
    auto cache = cacheLayoutFixture(3, knots);
    const Quad step = width / 16;
    for (unsigned i = 0; i < 16; ++i) {
        const auto a = jet(alpha + step*i), b = jet(alpha + step*(i+1));
        cache.controlPoints[3*i] = {double(a[0]), double(a[1])};
        cache.controlPoints[3*i+1] = {double(a[0] + step*a[2]/3), double(a[1] + step*a[3]/3)};
        cache.controlPoints[3*i+2] = {double(b[0] - step*b[2]/3), double(b[1] - step*b[3]/3)};
        cache.controlPoints[3*i+3] = {double(b[0]), double(b[1])};
    }
    // The algebraic crossing image has exactly zero Y, not an arbitrary coordinate join.
    cache.controlPoints.front().y = cache.controlPoints.back().y = 0;
    REQUIRE(cache.controlPoints.front().x > 17); REQUIRE(cache.controlPoints.back().x < -17);
    return cache;
}
}

TEST_CASE("cache topology accepts degree one complete path", "[lc-offsetcurve][geometry][cache-topology-chain]") {
    geo::WorkBudget budget;
    const auto result = publicArc(1, false, budget);
    checkPublicPath(result, budget, 1, .01);
    CHECK(result.analysis->layout().spans[0].degree == 1);
}

TEST_CASE("cache topology accepts generated cubic complete path", "[lc-offsetcurve][geometry][cache-topology-chain]") {
    const auto source = basis(1, 1, .25, .5); const RS_Ellipse ellipse{nullptr, source};
    const auto made = LC_CurveOffset::createEntities(ellipse,
        LC_CurveOffset::makeSideRequest(LC_CurveOffsetSide::Right, 1),
        LC_CurveOffset::makeDirectOptions(ellipse, 1, 1e-4), LC_CurveOffset::makeDirectSourceBudget());
    REQUIRE(made.status == LC_CurveOffsetStatus::Ok); REQUIRE(made.entities.size() == 1);
    const auto* spline = dynamic_cast<const RS_Spline*>(made.entities[0].get()); REQUIRE(spline);
    REQUIRE(spline->getData().degree == 3);
    geo::WorkBudget budget;
    const auto result = geo::checkCache(spline->getData(), localTopologyDefinition(source), 1e-4, budget);
    checkPublicPath(result, budget, 1, 1e-4);
}

TEST_CASE("cache topology accepts monotone cubic with stationary endpoint", "[lc-offsetcurve][geometry][cache-topology-chain]") {
    const auto source = basis(1, 1, .25, .2501);
    auto cache = arcCache(source, 1, 3); cache.controlPoints[1] = cache.controlPoints[0];
    geo::WorkBudget budget;
    const auto result = geo::checkCache(cache, localTopologyDefinition(source), .01, budget);
    checkPublicPath(result, budget, 1, .01);
}

TEST_CASE("cache topology accepts forward cache orientation", "[lc-offsetcurve][geometry][cache-topology-chain]") {
    geo::WorkBudget budget; const auto result = publicArc(3, false, budget);
    checkPublicPath(result, budget, 1, .01);
}

TEST_CASE("cache topology accepts reversed cache orientation", "[lc-offsetcurve][geometry][cache-topology-chain]") {
    geo::WorkBudget budget; const auto result = publicArc(3, true, budget);
    checkPublicPath(result, budget, 1, .01);
    CHECK(result.analysis->pieces().front().span == 0);
}

TEST_CASE("cache topology directly asserts None reason", "[lc-offsetcurve][geometry][cache-topology-chain]") {
    geo::WorkBudget budget; const auto result = publicArc(1, false, budget);
    REQUIRE(result.reason == LC_OffsetCheckResult::Reason::None); REQUIRE(result.analysis); REQUIRE(bool(result));
    CHECK(result.span == std::size_t(-1)); checkWork(result.work, budget.consumed());
}

TEST_CASE("cache topology directly asserts InvalidInput reason", "[lc-offsetcurve][geometry][cache-topology-chain]") {
    const auto source = basis(1, 1, .25, .2501); const auto cache = arcCache(source, 1, 1);
    for (unsigned kind = 0; kind < 5; ++kind) {
        auto definition = localTopologyDefinition(source); double tolerance = .01;
        if (kind == 0) { tolerance = 0; }
        if (kind == 1) { tolerance = std::numeric_limits<double>::quiet_NaN(); }
        if (kind == 2) { definition.distance = -1; }
        if (kind == 3) { definition.relTolerance = 0; }
        if (kind == 4) { definition.basis.center.z = 1; }
        geo::WorkBudget budget;
        const auto result = geo::checkCache(cache, definition, tolerance, budget);
        CHECK(result.reason == LC_OffsetCheckResult::Reason::InvalidInput); CHECK_FALSE(result.analysis); CHECK_FALSE(bool(result));
        checkWork(result.work, {}); checkWork(result.work, budget.consumed());
    }
}

TEST_CASE("cache topology directly asserts Layout reason", "[lc-offsetcurve][geometry][cache-topology-chain]") {
    const auto source = basis(1, 1, .25, .2501); auto cache = arcCache(source, 1, 1); cache.weights[0] = .5;
    geo::WorkBudget budget; const auto result = geo::checkCache(cache, localTopologyDefinition(source), .01, budget);
    CHECK(result.reason == LC_OffsetCheckResult::Reason::Layout); CHECK_FALSE(result.analysis); checkWork(result.work, budget.consumed());
}

TEST_CASE("cache topology directly asserts TooSmall reason", "[lc-offsetcurve][geometry][cache-topology-chain]") {
    const auto source = basis(1, 1, .25, .2501); const auto cache = arcCache(source, .01, 1);
    for (const double tolerance : {.01, .02}) {
        geo::WorkBudget budget; const auto result = geo::checkCache(cache, localTopologyDefinition(source, .01), tolerance, budget);
        CHECK(result.reason == LC_OffsetCheckResult::Reason::TooSmall); CHECK_FALSE(result.analysis);
        CHECK(result.work.cacheSpans == 1); checkWork(result.work, budget.consumed());
    }
}

TEST_CASE("cache topology directly asserts OffCurve reason", "[lc-offsetcurve][geometry][cache-topology-chain]") {
    const auto source = basis(1, 1, .25, .2501); auto cache = arcCache(source, 1, 1);
    cache.controlPoints = {{100, 100}, {101, 100}};
    geo::WorkBudget budget; const auto result = geo::checkCache(cache, localTopologyDefinition(source), .01, budget);
    CHECK(result.reason == LC_OffsetCheckResult::Reason::OffCurve); CHECK_FALSE(result.analysis);
    CHECK(result.span == 0); checkWork(result.work, budget.consumed());
}

TEST_CASE("cache topology directly asserts LimitExceeded reason", "[lc-offsetcurve][geometry][cache-topology-chain]") {
    const auto source = basis(1, 1, .25, .2501); const auto cache = arcCache(source, 1, 1);
    LC_OffsetCheckLimits limits; limits.maxVertices = 1;
    geo::WorkBudget budget{limits}; const auto result = geo::checkCache(cache, localTopologyDefinition(source), .01, budget);
    CHECK(result.reason == LC_OffsetCheckResult::Reason::LimitExceeded); CHECK_FALSE(result.analysis);
    CHECK(budget.status() == geo::BudgetStatus::LimitExceeded); checkWork(result.work, {});
}

TEST_CASE("cache topology closed metadata requires exact closure", "[lc-offsetcurve][geometry][cache-topology-chain]") {
    const auto source = basis(1, 1, .25, .2501); auto cache = arcCache(source, 1, 1); cache.m_closedFlag = true;
    geo::WorkBudget budget; const auto result = geo::checkCache(cache, localTopologyDefinition(source), .01, budget);
    CHECK(result.reason == LC_OffsetCheckResult::Reason::Topology); CHECK_FALSE(result.analysis); checkWork(result.work, budget.consumed());
}

TEST_CASE("cache topology periodic metadata requires closed metadata", "[lc-offsetcurve][geometry][cache-topology-chain]") {
    const auto source = basis(1, 1, .25, .2501); auto cache = arcCache(source, 1, 1); cache.m_periodicFlag = true;
    geo::WorkBudget budget; const auto result = geo::checkCache(cache, localTopologyDefinition(source), .01, budget);
    CHECK(result.reason == LC_OffsetCheckResult::Reason::Topology); CHECK_FALSE(result.analysis); checkWork(result.work, budget.consumed());
}

TEST_CASE("cache topology accepts retained seam whole source cycle", "[lc-offsetcurve][geometry][cache-topology-chain]") {
    const auto source = basis(10, .5);
    const auto cache = materializedCache(source, 3, 1e-3);
    geo::WorkBudget budget; const auto result = geo::checkCache(cache, localTopologyDefinition(source, 3), .01, budget);
    checkPublicPath(result, budget, 3, .01);
    REQUIRE(result.analysis->closed()); REQUIRE(result.analysis->reference().events().seam());
    CHECK(std::count_if(result.analysis->joins().begin(), result.analysis->joins().end(), [](const auto& join) {
        return join.kind == geo::CachePathJoinKind::Seam;
    }) == 1);
}

TEST_CASE("cache topology aligns phase at existing knot", "[lc-offsetcurve][geometry][cache-topology-chain]") {
    for (const bool reversed : {false, true}) {
        geo::WorkBudget budget;
        const auto result = geo::checkCache(phaseRing(false, false, reversed, true),
            localTopologyDefinition(basis(1, 1), 1, reversed), .02, budget);
        checkPublicPath(result, budget, 1, .02); checkPhaseOracle(result, false, false, true);
    }
}

TEST_CASE("cache topology aligns forward dyadic interior phase", "[lc-offsetcurve][geometry][cache-topology-chain]") {
    for (const bool sourceReversed : {false, true}) for (const bool cacheReversed : {false, true}) {
        auto source = basis(1, 1); source.reversed = sourceReversed;
        const auto cache = phaseRing(true, sourceReversed, cacheReversed);
        REQUIRE(cache.controlPoints.front().x == 945.0 / 512); REQUIRE(cache.controlPoints.front().y == 403.0 / 512);
        geo::WorkBudget budget;
        const auto result = geo::checkCache(cache, localTopologyDefinition(source, 1, cacheReversed), .02, budget);
        checkPublicPath(result, budget, 1, .02); checkPhaseOracle(result, sourceReversed, true);
    }
}

TEST_CASE("cache topology aligns reversed dyadic interior phase", "[lc-offsetcurve][geometry][cache-topology-chain]") {
    for (const bool sourceReversed : {false, true}) for (const bool cacheReversed : {false, true}) {
        auto source = basis(1, 1); source.reversed = sourceReversed;
        auto cache = phaseRing(true, sourceReversed, cacheReversed);
        std::reverse(cache.controlPoints.begin(), cache.controlPoints.end());
        geo::WorkBudget budget;
        const auto result = geo::checkCache(cache, localTopologyDefinition(source, 1, !cacheReversed), .02, budget);
        checkPublicPath(result, budget, 1, .02); checkPhaseOracle(result, sourceReversed, true);
    }
}

TEST_CASE("cache topology rejects missing reference tail", "[lc-offsetcurve][geometry][cache-topology-chain]") {
    const auto full = phaseRing(false);
    const auto prefix = cachePieces(full, 0, 3);
    geo::WorkBudget budget;
    checkPublicRefusal(geo::checkCache(prefix, localTopologyDefinition(basis(1, 1)), .02, budget),
        LC_OffsetCheckResult::Reason::Topology, budget);
}

TEST_CASE("cache topology rejects repeated reference tail", "[lc-offsetcurve][geometry][cache-topology-chain]") {
    const auto full = phaseRing(false);
    auto cache = cacheLayoutFixture(3, {0, 1, 2, 3, 4, 5, 6, 7, 8});
    std::copy(full.controlPoints.begin(), full.controlPoints.end(), cache.controlPoints.begin());
    std::copy(full.controlPoints.begin() + 1, full.controlPoints.end(), cache.controlPoints.begin() + 13);
    geo::WorkBudget budget;
    const auto result = geo::checkCache(cache, localTopologyDefinition(basis(1, 1)), .02, budget);
    checkPublicRefusal(result, LC_OffsetCheckResult::Reason::Topology, budget); CHECK(result.span == 4);
}

TEST_CASE("cache topology rejects candidate loop and retrace", "[lc-offsetcurve][geometry][cache-topology-chain]") {
    SECTION("exact retrace is independent of tolerance") {
        auto cache = cacheLayoutFixture(1, {0, 1, 2});
        cache.controlPoints = {{2, 0}, {2, .001}, {2, 0}};
        geo::WorkBudget budget;
        const auto result = geo::checkCache(cache, localTopologyDefinition(basis(1, 1)), .02, budget);
        checkPublicRefusal(result, LC_OffsetCheckResult::Reason::Topology, budget); CHECK(result.span == 1);
    }
    SECTION("adjacent monotone quadratic pieces still have a transverse interior contact") {
        const auto source = basis(1, 1, .25, .2501);
        const auto baseline = arcCache(source, 1, 1);
        const auto origin = baseline.controlPoints.front();
        auto cache = cacheLayoutFixture(3, {0, 1, 2});
        // Exact degree elevations of (3t-2t^2,2t^2-t), then (1-s+2s^2,1-3s+2s^2).
        const std::array<RS_Vector, 7> net{{{0,0},{1,-1.0/3},{4.0/3,0},{1,1},
            {2.0/3,0},{1,-1.0/3},{2,0}}};
        for (std::size_t i = 0; i < net.size(); ++i) { cache.controlPoints[i] = origin + net[i] * 1e-5; }
        geo::WorkBudget budget;
        checkPublicRefusal(geo::checkCache(cache, localTopologyDefinition(source), .01, budget),
            LC_OffsetCheckResult::Reason::Topology, budget);
    }
}

TEST_CASE("cache topology accepts exact event fringe join", "[lc-offsetcurve][geometry][cache-topology-chain]") {
    const auto source = basis(100, .5);
    const auto full = materializedCache(source, -49, 1e-3);
    for (const bool reversed : {false, true}) {
        auto cache = full;
        if (reversed) { std::reverse(cache.controlPoints.begin(), cache.controlPoints.end()); }
        geo::WorkBudget budget;
        const auto result = geo::checkCache(cache, localTopologyDefinition(source, -49, reversed), .01, budget);
        checkPublicPath(result, budget, 49, .01);
        REQUIRE(result.analysis->joins().size() == 2);
        for (const auto& join : result.analysis->joins()) {
            CHECK(join.kind == geo::CachePathJoinKind::PairEvent);
            CHECK(join.from.value != join.to.value);
        }
    }
}

TEST_CASE("cache topology accepts closed trimmed cycle", "[lc-offsetcurve][geometry][cache-topology-chain]") {
    const auto source = basis(100, .5, 10 * M_PI / 180, 350 * M_PI / 180);
    const auto cache = materializedCache(source, -49.5, 1e-3);
    geo::WorkBudget budget;
    const auto result = geo::checkCache(cache, localTopologyDefinition(source, -49.5), .01, budget);
    checkPublicPath(result, budget, 49.5, .01);
    CHECK(result.analysis->closed()); CHECK_FALSE(result.analysis->reference().events().seam());
    CHECK(result.analysis->edges().size() == 2);
}

TEST_CASE("cache topology rejects open endpoint mismatch", "[lc-offsetcurve][geometry][cache-topology-chain]") {
    const auto source = basis(1, 1, .25, 1.25);
    const auto cache = arcCache(basis(1, 1, .5, 1), 1, 3);
    geo::WorkBudget budget;
    checkPublicRefusal(geo::checkCache(cache, localTopologyDefinition(source), .001, budget),
        LC_OffsetCheckResult::Reason::Topology, budget);
}

TEST_CASE("cache topology closure metadata false false permits open and closed geometry", "[lc-offsetcurve][geometry][cache-topology-chain]") {
    for (const bool closed : {false, true}) {
        const auto source = closed ? basis(1, 1) : basis(1, 1, .25, .2501);
        auto cache = closed ? phaseRing(false) : arcCache(source, 1, 1);
        cache.m_closedFlag = cache.m_periodicFlag = false;
        geo::WorkBudget budget;
        const auto result = geo::checkCache(cache, localTopologyDefinition(source), .02, budget);
        checkPublicPath(result, budget, 1, .02); CHECK(result.analysis->closed() == closed);
    }
}

TEST_CASE("cache topology periodic metadata does not infer derivative continuity", "[lc-offsetcurve][geometry][cache-topology-chain]") {
    auto cache = phaseRing(true);
    cache.m_closedFlag = cache.m_periodicFlag = true;
    // One-sided speeds at this exact split are unequal; periodic metadata is not a C1 proof.
    const auto first = cache.controlPoints[1] - cache.controlPoints[0];
    const auto last = cache.controlPoints.back() - cache.controlPoints[cache.controlPoints.size() - 2];
    REQUIRE(first.squared() != last.squared());
    geo::WorkBudget budget;
    checkPublicPath(geo::checkCache(cache, localTopologyDefinition(basis(1, 1)), .02, budget), budget, 1, .02);
}

TEST_CASE("cache topology directly asserts Topology reason", "[lc-offsetcurve][geometry][cache-topology-chain]") {
    const auto source = basis(1, 1, .25, 1.25);
    const auto cache = arcCache(basis(1, 1, .5, 1), 1, 3);
    geo::WorkBudget budget;
    checkPublicRefusal(geo::checkCache(cache, localTopologyDefinition(source), .001, budget),
        LC_OffsetCheckResult::Reason::Topology, budget);
}

TEST_CASE("cache topology accepts source start terminal", "[lc-offsetcurve][geometry][cache-topology-chain]") {
    geo::WorkBudget budget; const auto result = publicArc(1, false, budget);
    checkPublicPath(result, budget, 1, .01);
    REQUIRE(result.analysis->startTerminal());
    CHECK(result.analysis->startTerminal()->kind == geo::ReferenceTerminalKind::SourceStart);
    CHECK(result.analysis->startTerminal()->occurrence.value == result.analysis->endpoints().front().occurrence.value);
}

TEST_CASE("cache topology accepts source end terminal", "[lc-offsetcurve][geometry][cache-topology-chain]") {
    geo::WorkBudget budget; const auto result = publicArc(3, true, budget);
    checkPublicPath(result, budget, 1, .01);
    REQUIRE(result.analysis->endTerminal());
    CHECK(result.analysis->endTerminal()->kind == geo::ReferenceTerminalKind::SourceEnd);
    CHECK(result.analysis->endTerminal()->occurrence.value == result.analysis->endpoints().back().occurrence.value);
}

TEST_CASE("cache topology rejects through event endpoint", "[lc-offsetcurve][geometry][cache-topology-chain]") {
    const auto source = basis(1, 1);
    const auto cache = arcCache(basis(1, 1, .25, .2501), 1, 1);
    geo::WorkBudget budget;
    checkPublicRefusal(geo::checkCache(cache, localTopologyDefinition(source), .01, budget),
        LC_OffsetCheckResult::Reason::Topology, budget);
}

TEST_CASE("cache topology rejects arbitrary interior endpoint", "[lc-offsetcurve][geometry][cache-topology-chain]") {
    const auto source = basis(1, 1, .25, 1.25);
    auto cache = arcCache(basis(1, 1, .5, 1.25), 1, 3);
    geo::WorkBudget budget;
    checkPublicRefusal(geo::checkCache(cache, localTopologyDefinition(source), .001, budget),
        LC_OffsetCheckResult::Reason::Topology, budget);
}

TEST_CASE("cache topology rejects hidden cusp endpoint", "[lc-offsetcurve][geometry][cache-topology-chain]") {
    auto cache = cacheLayoutFixture(1, {0, 1});
    cache.controlPoints = {{-0x1.8700a72e08b38p+6, -0x1.8c704c47d1742p+1},
        {-0x1.8700a72e08b38p+6, 0x1.8c704c47d1742p+1}};
    geo::WorkBudget budget;
    const auto result = geo::checkCache(cache, localTopologyDefinition(basis(100, .05, M_PI / 12, 7 * M_PI / 4), -4),
        1e-5, budget);
    checkPublicRefusal(result, LC_OffsetCheckResult::Reason::OffCurve, budget); CHECK(result.span == 0);
}

TEST_CASE("cache topology directly asserts ReferenceUnresolved reason", "[lc-offsetcurve][geometry][cache-topology-chain]") {
    auto cache = cacheLayoutFixture(1, {0, 1}); cache.controlPoints = {{1, 0}, {2, 0}};
    geo::WorkBudget budget;
    checkPublicRefusal(geo::checkCache(cache, localTopologyDefinition(basis(1, 1), -1), .01, budget),
        LC_OffsetCheckResult::Reason::ReferenceUnresolved, budget);
    CHECK(budget.status() == geo::BudgetStatus::Ok);
}

TEST_CASE("cache topology refuses unresolved cut correspondence", "[lc-offsetcurve][geometry][cache-topology-chain]") {
    auto cache = cacheLayoutFixture(1, {0, 1, 2, 3, 4});
    cache.controlPoints = {{0,0},{.1,-.1},{.2,0},{.1,.1},{0,0}};
    geo::WorkBudget budget;
    checkPublicRefusal(geo::checkCache(cache, localTopologyDefinition(basis(1, 1), -.5), .49, budget),
        LC_OffsetCheckResult::Reason::Ambiguous, budget);
    CHECK(budget.status() == geo::BudgetStatus::Ok);
}

TEST_CASE("cache topology directly asserts Ambiguous reason", "[lc-offsetcurve][geometry][cache-topology-chain]") {
    auto cache = cacheLayoutFixture(1, {0, 1, 2, 3, 4});
    cache.controlPoints = {{0,0},{.1,-.1},{.2,0},{.1,.1},{0,0}};
    geo::WorkBudget budget;
    checkPublicRefusal(geo::checkCache(cache, localTopologyDefinition(basis(1, 1), -.5), .49, budget),
        LC_OffsetCheckResult::Reason::Ambiguous, budget);
}

TEST_CASE("cache topology attributes original spans deterministically", "[lc-offsetcurve][geometry][cache-topology-chain]") {
    const auto source = basis(1, 1, .25, .2501);
    const auto full = arcCache(source, 1, 1);
    auto cache = cacheLayoutFixture(1, {0, 1, 2});
    cache.controlPoints = {full.controlPoints[0], full.controlPoints[1], full.controlPoints[0]};
    for (const bool reversed : {false, true}) {
        geo::WorkBudget budget;
        const auto result = geo::checkCache(cache, localTopologyDefinition(source, 1, reversed), .01, budget);
        checkPublicRefusal(result, LC_OffsetCheckResult::Reason::Topology, budget);
        CHECK(result.span == 1); // original raw order, not normalized traversal order
    }
}

TEST_CASE("cache topology canonicalizes nearby same edge cut frontiers", "[lc-offsetcurve][geometry][cache-topology-chain]") {
    for (const bool reversed : {false, true}) {
        geo::WorkBudget aBudget, bBudget;
        auto split = phaseRing(true, false, reversed);
        auto nearby = phaseRing(true, false, reversed, false, .25 + 0x1p-12);
        const auto definition = localTopologyDefinition(basis(1, 1), 1, reversed);
        const auto a = geo::checkCache(split, definition, .02, aBudget);
        const auto b = geo::checkCache(nearby, definition, .02, bBudget);
        checkPublicPath(a, aBudget, 1, .02); checkPublicPath(b, bBudget, 1, .02);
        CHECK(a.analysis->edges() == b.analysis->edges());
        REQUIRE(a.analysis->joins().size() == 1); REQUIRE(b.analysis->joins().size() == 1);
        CHECK(a.analysis->joins()[0].from.value == b.analysis->joins()[0].from.value);
        CHECK(a.analysis->joins()[0].to.value == b.analysis->joins()[0].to.value);
        REQUIRE(a.analysis->cut()); REQUIRE(b.analysis->cut());
        REQUIRE_FALSE(a.analysis->cut()->transition); REQUIRE_FALSE(b.analysis->cut()->transition);
        const auto& p = a.analysis->cut()->referenceParameter;
        const auto& q = b.analysis->cut()->referenceParameter;
        CHECK(p.hi() < q.lo()); CHECK(q.hi() - p.lo() < .01);
    }
}

TEST_CASE("cache topology admits deterministic 64 span cycle", "[lc-offsetcurve][geometry][cache-topology-chain]") {
    const auto base = phaseRing(false);
    geo::WorkBudget layoutBudget;
    const auto layout = geo::inspectCacheLayout(base, layoutBudget);
    REQUIRE(layout.status == geo::CacheLayoutStatus::Ok);
    std::vector<std::array<RS_Vector, 4>> nets;
    for (const auto& span : layout.spans) { nets.push_back(span.controls); }
    for (unsigned level = 0; level < 4; ++level) {
        std::vector<std::array<RS_Vector, 4>> children;
        for (auto net : nets) {
            std::array<RS_Vector, 4> left, right; left[0] = net[0]; right[3] = net[3];
            for (unsigned row = 1; row <= 3; ++row) {
                for (unsigned i = 0; i <= 3 - row; ++i) { net[i] = (net[i] + net[i+1]) * .5; }
                left[row] = net[0]; right[3-row] = net[3-row];
            }
            children.push_back(left); children.push_back(right);
        }
        nets = std::move(children);
    }
    REQUIRE(nets.size() == 64);
    std::vector<double> knots; for (unsigned i = 0; i <= 64; ++i) { knots.push_back(double(i)); }
    auto cache = cacheLayoutFixture(3, knots);
    for (std::size_t i = 0; i < nets.size(); ++i) {
        for (unsigned j = 0; j <= 3; ++j) { cache.controlPoints[3*i+j] = nets[i][j]; }
    }
    geo::WorkBudget aBudget, bBudget;
    const auto a = geo::checkCache(cache, localTopologyDefinition(basis(1, 1)), .02, aBudget);
    const auto b = geo::checkCache(cache, localTopologyDefinition(basis(1, 1)), .02, bBudget);
    checkPublicPath(a, aBudget, 1, .02); checkPublicPath(b, bBudget, 1, .02);
    CHECK(a.work.cacheSpans >= 64); checkWork(a.work, b.work);
    CHECK(a.analysis->pieces().size() == b.analysis->pieces().size());
}

TEST_CASE("cache topology follows source continuation without hidden interval", "[lc-offsetcurve][geometry][cache-topology-chain]") {
    const auto source = basis(100, .5, 2.6, 3.6);
    const auto cache = materializedCache(source, -25, 1e-3);
    geo::WorkBudget diagnosticBudget;
    const auto diagnostic = geo::analyseReferenceComponents(source, -25, .01, diagnosticBudget);
    CAPTURE(diagnostic.status, diagnostic.reason, diagnostic.eventReason, diagnostic.work.distancePairs,
        diagnostic.work.vertices, diagnostic.work.deepest);
    REQUIRE(diagnostic.status == LC_IntersectionStatus::Ok); REQUIRE(diagnostic.analysis);
    geo::WorkBudget budget;
    const auto result = geo::checkCache(cache, localTopologyDefinition(source, -25), .01, budget);
    checkPublicPath(result, budget, 25, .01);
    REQUIRE(std::any_of(result.analysis->joins().begin(), result.analysis->joins().end(), [](const auto& join) {
        return join.kind == geo::CachePathJoinKind::SourceContinuation && join.from.value == join.to.value;
    }));
    for (const auto& join : result.analysis->joins()) {
        if (join.kind != geo::CachePathJoinKind::SourceContinuation) { continue; }
        CHECK_FALSE(result.analysis->reference().terminal(join.incoming, geo::ReferenceEdgeSide::End));
        CHECK_FALSE(result.analysis->reference().terminal(join.outgoing, geo::ReferenceEdgeSide::Start));
    }
}

TEST_CASE("cache topology selects separate paths from two component producer", "[lc-offsetcurve][geometry][cache-topology-chain]") {
    const auto caches = twoComponentCaches();
    const auto definition = localTopologyDefinition(basis(100, .05, M_PI / 12, 7 * M_PI / 4), -4);
    for (unsigned i = 0; i < 2; ++i) {
        geo::WorkBudget budget;
        const auto result = geo::checkCache(caches[i], definition, .001, budget);
        checkPublicPath(result, budget, 4, .001);
        CHECK(result.analysis->closed() == (i == 1));
        CHECK(result.analysis->edges().size() == (i == 0 ? 1 : 2));
        CHECK(result.analysis->reference().edges().size() == 3);
    }
}

TEST_CASE("cache topology accepts pair trim without retained continuation", "[lc-offsetcurve][geometry][cache-topology-chain]") {
    const auto cache = twoComponentCaches()[0];
    const auto source = basis(100, .05, M_PI / 12, 7 * M_PI / 4);
    geo::WorkBudget budget;
    const auto result = geo::checkCache(cache, localTopologyDefinition(source, -4), .001, budget);
    checkPublicPath(result, budget, 4, .001);
    REQUIRE(result.analysis->endTerminal());
    const auto& terminal = *result.analysis->endTerminal();
    CHECK(terminal.kind == geo::ReferenceTerminalKind::PairTrim);
    CHECK(result.analysis->reference().events().occurrences()[terminal.partner.value].role == geo::ReferenceRole::EndCutter);
    CHECK(result.analysis->reference().occurrenceIncidence()[terminal.occurrence.value].outgoing == noTerminal);
}

TEST_CASE("cache topology rejects wrong paired terminal orientation", "[lc-offsetcurve][geometry][cache-topology-chain]") {
    auto cache = twoComponentCaches()[0];
    std::reverse(cache.controlPoints.begin(), cache.controlPoints.end());
    geo::WorkBudget budget;
    checkPublicRefusal(geo::checkCache(cache, localTopologyDefinition(basis(100, .05, M_PI / 12, 7 * M_PI / 4), -4), .001, budget),
        LC_OffsetCheckResult::Reason::Topology, budget);
}

TEST_CASE("cache topology rejects G2 upper lens E0 only path", "[lc-offsetcurve][geometry][cache-topology-chain]") {
    const auto cache = upperLensCache();
    geo::WorkBudget budget;
    checkPublicRefusal(geo::checkCache(cache, localTopologyDefinition(basis(100, .5), -49), .01, budget),
        LC_OffsetCheckResult::Reason::Topology, budget);
    const auto reference = terminalGraph(basis(100, .5), -49);
    REQUIRE(reference.analysis->edges().size() == 2);
    for (std::size_t i = 0; i < 2; ++i) {
        CHECK_FALSE(reference.analysis->terminal(i, geo::ReferenceEdgeSide::Start));
        CHECK_FALSE(reference.analysis->terminal(i, geo::ReferenceEdgeSide::End));
    }
}

namespace {
RS_SplineData nonNearestCutCache(const bool sourceReversed = false, const bool cacheReversed = false) {
    // An independently specified CCW triangle, with its horizontal top split at
    // an interior point. No producer, private certificate or fitted inverse.
    auto cache = cacheLayoutFixture(1, {0,1,2,3,4});
    cache.controlPoints = {{.25,0},{-4,0},{0,-4},{4,0},{.25,0}};
    if (sourceReversed != cacheReversed) { std::reverse(cache.controlPoints.begin(), cache.controlPoints.end()); }
    return cache;
}

void checkStartCutInventory(const geo::CacheTopologyAnalysis& proof, const double tolerance) {
    REQUIRE_FALSE(proof.startCutAxes().empty()); REQUIRE_FALSE(proof.startCuts().empty());
    const auto& stats = proof.searchStats();
    REQUIRE(stats.selectedCut < proof.startCuts().size()); REQUIRE(stats.nearestCut < proof.startCuts().size());
    CHECK(stats.acceptedRoutes == 1); CHECK(stats.memoStates > 0);
    REQUIRE_FALSE(proof.startCutPieces(stats.selectedCut).empty());
    const auto& frame = proof.reference().events().providers().frame();
    const auto threshold = LC_Interval::point(2) * LC_Interval::point(tolerance) / LC_Interval::point(frame.scale());
    for (std::size_t axis = 0; axis < proof.startCutAxes().size(); ++axis) {
        for (const auto edge : proof.edges()) {
            const auto& span = proof.reference().spans()[proof.reference().edges()[edge].span];
            std::vector<const geo::CacheStartCutRegion*> cells;
            for (const auto& region : proof.startCutRegions()) {
                if (region.edge == edge && region.axis == axis) { cells.push_back(&region); }
            }
            REQUIRE_FALSE(cells.empty());
            std::sort(cells.begin(), cells.end(), [](const auto* a, const auto* b) {
                return a->parameter.lo() < b->parameter.lo();
            });
            double covered = span.interior.lo();
            for (const auto* cell : cells) {
                CHECK(cell->parameter.lo() == covered); REQUIRE(cell->parameter.lo() < cell->parameter.hi());
                covered = cell->parameter.hi();
                switch (cell->kind) {
                case geo::CacheStartCutRegionKind::Separated:
                    CHECK((cell->value.lo() > 0 || cell->value.hi() < 0)); break;
                case geo::CacheStartCutRegionKind::Far:
                    CHECK(cell->distance.lo() > threshold.hi()); break;
                case geo::CacheStartCutRegionKind::Reverse:
                    CHECK(cell->derivative.hi() < 0); break;
                case geo::CacheStartCutRegionKind::UniqueRoot: {
                    REQUIRE(cell->cut < proof.startCuts().size()); const auto& cut = proof.startCuts()[cell->cut];
                    CHECK_FALSE(cut.transition); CHECK(cut.edge == edge);
                    CHECK(cut.proofParameter.lo() <= cell->parameter.lo());
                    CHECK(cell->parameter.hi() <= cut.proofParameter.hi());
                    CHECK(cut.firstSign.hi() < 0); CHECK(cut.lastSign.lo() > 0);
                    CHECK(cut.derivativeProjection.lo() > 0);
                    REQUIRE_FALSE(proof.startCutPieces(cell->cut).empty());
                    CHECK(proof.startCutLapAdmission(cell->cut).has_value());
                    CHECK(proof.startCutExclusion(cell->cut).kind == geo::CacheStartCutExclusionKind::None);
                    break;
                }
                case geo::CacheStartCutRegionKind::ExcludedRoot: {
                    REQUIRE(cell->cut < proof.startCuts().size()); const auto& cut = proof.startCuts()[cell->cut];
                    CHECK_FALSE(cut.transition); CHECK(cut.edge == edge);
                    CHECK(cut.proofParameter.lo() <= cell->parameter.lo());
                    CHECK(cell->parameter.hi() <= cut.proofParameter.hi());
                    CHECK(cut.firstSign.hi() < 0); CHECK(cut.lastSign.lo() > 0);
                    CHECK(cut.derivativeProjection.lo() > 0);
                    CHECK(proof.startCutPieces(cell->cut).empty());
                    CHECK_FALSE(proof.startCutLapAdmission(cell->cut));
                    const auto evidence = proof.startCutExclusion(cell->cut);
                    REQUIRE(evidence.kind != geo::CacheStartCutExclusionKind::None);
                    if (evidence.kind == geo::CacheStartCutExclusionKind::Distance) {
                        CHECK(evidence.bound.lo() > threshold.hi());
                    } else if (evidence.kind == geo::CacheStartCutExclusionKind::BackwardEndpoint) {
                        CHECK(evidence.bound.hi() < 0);
                        const auto& net = proof.layout().spans[evidence.start ? cut.span : cut.closingSpan];
                        bool foundDirection = false;
                        for (std::size_t k = 1; k <= net.degree; ++k) {
                            const Quad x = Quad(net.controls[k].x) - Quad(net.controls[0].x);
                            const Quad y = Quad(net.controls[k].y) - Quad(net.controls[0].y);
                            if (x == 0 && y == 0) { continue; }
                            foundDirection = true;
                            for (std::size_t j = 0; j <= net.degree; ++j) {
                                CHECK((Quad(net.controls[j].x) - Quad(net.controls[0].x))*y ==
                                      (Quad(net.controls[j].y) - Quad(net.controls[0].y))*x);
                            }
                            break;
                        }
                        CHECK(foundDirection);
                    } else {
                        REQUIRE(evidence.kind == geo::CacheStartCutExclusionKind::CircleSupport);
                        CHECK(evidence.bound.lo() > 0);
                        CHECK((Quad(evidence.point.x)-Quad(evidence.centre.x))*Quad(cut.axisX.lo()) +
                              (Quad(evidence.point.y)-Quad(evidence.centre.y))*Quad(cut.axisY.lo()) == 0);
                    }
                    break;
                }
                }
            }
            CHECK(covered == span.interior.hi()); // no omitted cut-search fringe
        }
    }
}
}

TEST_CASE("Public cache search proves a non-nearest certified start cut in every directed cache order",
          "[lc-offsetcurve][geometry][cache-topology-chain][cache-search-completion]") {
    for (const bool sourceReversed : {false,true}) for (const bool cacheReversed : {false,true}) {
        auto source = basis(1,1); source.reversed = sourceReversed;
        const auto cache = nonNearestCutCache(sourceReversed, cacheReversed);
        geo::WorkBudget budget;
        const auto result = geo::checkCache(cache, localTopologyDefinition(source, 3, cacheReversed), 2.5, budget);
        checkPublicPath(result, budget, 3, 2.5); checkStartCutInventory(*result.analysis, 2.5);
        const auto& proof = *result.analysis; const auto& stats = proof.searchStats();
        REQUIRE(stats.selectedCut != stats.nearestCut);
        const auto& nearest = proof.startCuts()[stats.nearestCut];
        REQUIRE_FALSE(nearest.transition); CHECK(proof.startCutPieces(stats.nearestCut).empty());
        // Reference circle radius 4: nearest radial foot is (4,0), distance
        // 15/4 from P=(1/4,0). Its north/south phase axis is perpendicular to
        // BOTH nonzero horizontal cache seam derivatives, so that cut refuses.
        // A farther exact seam or upper/lower transverse cut still proves one lap.
        const auto& frame = proof.reference().events().providers().frame();
        const Quad pi = boost::math::constants::pi<Quad>();
        const Quad angle = sourceReversed ? Quad(0) : 2*pi;
        const Quad parameter = (angle - Quad(frame.sourceSweep().start)) * Quad(frame.parameterEnd()) /
            (sourceReversed ? -2*pi : 2*pi);
        checkEnclosure(nearest.referenceParameter, parameter);
        CHECK(nearest.axisX.lo() == 0); CHECK(nearest.axisX.hi() == 0);
        CHECK(proof.startCuts()[stats.selectedCut].geometryError.hi() > nearest.geometryError.lo());
        CHECK(stats.acceptedRoutes == 1); // never count equivalent cuts as another cycle
    }
}

TEST_CASE("Public cache search retains exhaustive cut covers and independent equivalent full laps",
          "[lc-offsetcurve][geometry][cache-topology-chain][cache-search-completion]") {
    for (const bool reversed : {false,true}) {
        geo::WorkBudget budget;
        const auto result = geo::checkCache(phaseRing(true, false, reversed),
            localTopologyDefinition(basis(1,1), 1, reversed), .02, budget);
        checkPublicPath(result, budget, 1, .02); checkPhaseOracle(result, false, true);
        checkStartCutInventory(*result.analysis, .02);
        REQUIRE(result.analysis->searchStats().cutsProved >= 2);
        std::size_t proved = 0;
        for (std::size_t i = 0; i < result.analysis->startCuts().size(); ++i) {
            if (!result.analysis->startCutPieces(i).empty()) { ++proved; }
        }
        CHECK(proved == result.analysis->searchStats().cutsProved);
        CHECK(result.analysis->searchStats().acceptedRoutes == 1);
    }
}

TEST_CASE("Public root-straddling inventory cannot borrow an earlier node lap",
          "[lc-offsetcurve][geometry][cache-topology-chain][cache-search-completion]") {
    // The exact band is 1. This distance enclosure contains 1 but lies above
    // the outward threshold's lower endpoint: that is NOT negative evidence.
    const auto band = LC_Interval::point(2)*LC_Interval::point(.5)/LC_Interval::point(1);
    const auto gray = LC_Interval::hull(std::nextafter(1.,0.),std::nextafter(1.,2.));
    CHECK(Quad(gray.lo()) < Quad(1)); CHECK(Quad(gray.hi()) > Quad(1));
    CHECK(gray.lo() > band.lo()); CHECK_FALSE(gray.lo() > band.hi());
    const auto source = basis(1,1); const auto cache = nonNearestCutCache();
    const auto definition = localTopologyDefinition(source,3);
    geo::WorkBudget budget;
    const auto result = geo::checkCache(cache, definition, 2.5, budget);
    checkPublicPath(result, budget, 3, 2.5); checkStartCutInventory(*result.analysis, 2.5);
    const auto& proof = *result.analysis;
    const auto& frame = proof.reference().events().providers().frame();
    const auto selected = proof.searchStats().selectedCut;
    REQUIRE(proof.startCuts()[selected].transition);
    const auto earlier = proof.startCutLapAdmission(selected); REQUIRE(earlier);

    // On the FIRST quarter of the stored whole chart, west projection has
    // opposite endpoint signs and strictly positive derivative. Its actual
    // interior root is theta=acos(1/16), not the retained theta=pi/4 node.
    const double a = 0, b = frame.parameterEnd()/4;
    LC_CurveJetBounds first, last;
    REQUIRE(frame.boundOffsetJet(a,a,first)); REQUIRE(frame.boundOffsetJet(b,b,last));
    LC_Interval px, py; REQUIRE(frame.pointToFrame(cache.controlPoints.front(),px,py));
    CHECK((px-first.x).hi() < 0); CHECK((px-last.x).lo() > 0);
    LC_Interval derivative;
    REQUIRE(proof.reference().events().providers().boundOwnProjectedDerivative(
        a,b,LC_Interval::point(-1),LC_Interval::point(0),derivative));
    REQUIRE(derivative.lo() > 0);
    const Quad pi = boost::math::constants::pi<Quad>();
    const Quad expected = (acos(Quad(1)/16)-Quad(frame.sourceSweep().start))*
        Quad(frame.parameterEnd())/(2*pi);
    std::size_t target = std::size_t(-1), matches = 0;
    for (std::size_t i = 0; i < proof.startCuts().size(); ++i) {
        const auto& cut = proof.startCuts()[i];
        if (!cut.transition && cut.axisX.hi() < 0 && cut.axisY.lo() == 0 && cut.axisY.hi() == 0 &&
            Quad(cut.referenceParameter.lo()) <= expected && expected <= Quad(cut.referenceParameter.hi())) {
            target = i; ++matches;
        }
    }
    REQUIRE(matches == 1); REQUIRE(target != selected);
    REQUIRE_FALSE(proof.startCutPieces(target).empty());
    CHECK(proof.startCutExclusion(target).kind == geo::CacheStartCutExclusionKind::None);
    const auto admission = proof.startCutLapAdmission(target); REQUIRE(admission);
    REQUIRE(admission->vertices > earlier->vertices);

    // Leave exactly the actually entered prefix up to this root's own lap.
    // A successful earlier node packet MUST NOT make this incomplete run None.
    LC_OffsetCheckLimits limits; limits.maxVertices = admission->vertices;
    geo::WorkBudget limited{limits};
    const auto refused = geo::checkCache(cache,definition,2.5,limited);
    CHECK(refused.reason == LC_OffsetCheckResult::Reason::LimitExceeded); CHECK_FALSE(refused.analysis);
    CHECK(refused.work.vertices <= admission->vertices); checkWork(refused.work,limited.consumed());
    const auto retry = geo::checkCache(cache,definition,2.5,limited);
    CHECK(retry.reason == LC_OffsetCheckResult::Reason::LimitExceeded); CHECK_FALSE(retry.analysis);
    checkWork(retry.work,refused.work);
}

TEST_CASE("Public curved endpoint cut receives an own lap despite negative leading tangent dot",
          "[lc-offsetcurve][geometry][cache-topology-chain][cache-search-completion]") {
    using Net = std::array<RS_Vector,4>;
    const RS_Vector p{3,2.5};
    // eps=1/4 times (1,0),(1,3),(1,3) derivative controls. The
    // finite first chord is parallel to A=(1,2), NOT the leading (1,0).
    const std::array<Net,6> nets{{
        Net{{p,{3.25,2.5},{3.5,3.25},{3.75,4}}},
        Net{{{3.75,4},{3.75,4},{-4,0},{-4,0}}},
        Net{{{-4,0},{-4,0},{0,-4},{0,-4}}},
        Net{{{0,-4},{0,-4},{4,0},{4,0}}},
        Net{{{4,0},{4,0},{3.75,1.5},{3.75,1.5}}},
        Net{{{3.75,1.5},{3.75,1.5},p,p}}
    }};
    auto cache = cacheLayoutFixture(3,{0,1,2,3,4,5,6});
    for (std::size_t k = 0; k < nets.size(); ++k) for (std::size_t j = 0; j <= 3; ++j) {
        cache.controlPoints[3*k+j] = nets[k][j];
    }
    geo::WorkBudget budget;
    const auto result = geo::checkCache(cache,localTopologyDefinition(basis(1,1),3),2.5,budget);
    checkPublicPath(result,budget,3,2.5); checkStartCutInventory(*result.analysis,2.5);
    const auto& proof = *result.analysis;
    const auto nearest = proof.searchStats().nearestCut;
    const auto& cut = proof.startCuts()[nearest]; REQUIRE_FALSE(cut.transition);
    const auto& frame = proof.reference().events().providers().frame();
    std::array<geo::CacheBezierPoint,4> derivatives; std::size_t degree = 0;
    REQUIRE(geo::boundCacheDerivativeControls(proof.layout().spans.front(),frame,0,1,derivatives,degree));
    for (std::size_t j = 0; j <= degree; ++j) {
        CHECK((derivatives[j].x+LC_Interval::point(2)*derivatives[j].y).lo() > 0);
    }
    LC_CurveJetBounds root;
    REQUIRE(frame.boundOffsetJet(cut.referenceParameter.lo(),cut.referenceParameter.hi(),root));
    CHECK((root.dx*derivatives[0].x+root.dy*derivatives[0].y).hi() < 0);
    CHECK((root.dx+LC_Interval::point(2)*root.dy).lo() > 0);
    REQUIRE_FALSE(proof.startCutPieces(nearest).empty());
    CHECK(proof.startCutLapAdmission(nearest).has_value());
    CHECK(proof.startCutExclusion(nearest).kind == geo::CacheStartCutExclusionKind::None);
    // The issued root precedes the retained seam. Both sides of that seam
    // must use the SAME nonempty interior candidate frontier, not cache u=0.
    const auto& referenceSpan = proof.reference().spans()[proof.reference().edges()[cut.edge].span];
    std::optional<double> incomingSeam, outgoingSeam;
    for (const auto& piece : proof.startCutPieces(nearest)) {
        if (piece.edge != cut.edge || piece.normalizedSpan != 0) { continue; }
        if (piece.referenceParameter.hi() == referenceSpan.interior.hi()) {
            REQUIRE_FALSE(incomingSeam); incomingSeam = piece.candidateParameter.hi();
        }
        if (piece.referenceParameter.lo() == referenceSpan.interior.lo()) {
            REQUIRE_FALSE(outgoingSeam); outgoingSeam = piece.candidateParameter.lo();
        }
    }
    REQUIRE(incomingSeam); REQUIRE(outgoingSeam);
    CHECK(*incomingSeam > 0); CHECK(*incomingSeam < 1);
    CHECK(*incomingSeam == *outgoingSeam);
}

TEST_CASE("Public circle support exclusion keeps world axes under rotated translated source bases",
          "[lc-offsetcurve][geometry][cache-topology-chain][cache-search-completion]") {
    for (const bool sourceReversed : {false,true}) for (const bool cacheReversed : {false,true}) {
        RS_EllipseData source{{16,-32},{3,4},1,0,0,sourceReversed}; // exact radius 5, rotated basis
        auto cache = nonNearestCutCache(sourceReversed,cacheReversed);
        for (auto& point : cache.controlPoints) { point = {16+5*point.x,-32+5*point.y}; }
        geo::WorkBudget budget;
        const auto result = geo::checkCache(cache,localTopologyDefinition(source,15,cacheReversed),12.5,budget);
        checkPublicPath(result,budget,15,12.5); checkStartCutInventory(*result.analysis,12.5);
        const auto& proof = *result.analysis;
        const auto& frame = proof.reference().events().providers().frame();
        LC_Interval x,y; REQUIRE(frame.pointToFrame({21,-32},x,y));
        checkEnclosure(x,Quad(5)/Quad(frame.scale())); checkEnclosure(y,Quad(0));
        CHECK(x.lo() > 0); CHECK(y.width() < std::numeric_limits<double>::epsilon());
        const auto nearest = proof.searchStats().nearestCut;
        const auto& cut = proof.startCuts()[nearest]; REQUIRE_FALSE(cut.transition);
        const auto evidence = proof.startCutExclusion(nearest);
        REQUIRE(evidence.kind == geo::CacheStartCutExclusionKind::CircleSupport);
        CHECK(evidence.centre.x == 16); CHECK(evidence.centre.y == -32);
        CHECK(evidence.point.x == 17.25); CHECK(evidence.point.y == -32);
        CHECK(cut.axisX.lo() == 0); CHECK(cut.axisX.hi() == 0);
        CHECK(evidence.bound.lo() > 0); CHECK(proof.startCutPieces(nearest).empty());
        CHECK_FALSE(proof.startCutLapAdmission(nearest));
        CHECK(proof.searchStats().acceptedRoutes == 1);
    }
}

TEST_CASE("Public nonaxis radial support retains an exactly orthogonal unnormalized circle axis",
          "[lc-offsetcurve][geometry][cache-topology-chain][cache-search-completion]") {
    // Independently show why the old blanket normalization cannot confer an
    // exact support-plane credential, even for a dyadic 3-4-5 direction.
    const double length = std::hypot(-1,.75);
    const Quad normalizedDot = Quad(.75)*Quad(-1/length)+Quad(.75/length);
    CHECK(normalizedDot == -Quad(0x1p-54));
    for (const bool sourceReversed : {false,true}) for (const bool cacheReversed : {false,true}) {
        RS_EllipseData source{{16,-32},{3,4},1,0,0,sourceReversed};
        auto cache = nonNearestCutCache(sourceReversed,cacheReversed);
        // Exact dyadic similarity [[3,-4],[4,3]], not a rounded trig rotation.
        // Rotate BOTH geometry and cache: P-centre=(.75,1), not a world axis.
        for (auto& point : cache.controlPoints) {
            const double x = point.x, y = point.y;
            point = {16+3*x-4*y,-32+4*x+3*y};
        }
        geo::WorkBudget budget;
        const auto result = geo::checkCache(cache,localTopologyDefinition(source,15,cacheReversed),12.5,budget);
        checkPublicPath(result,budget,15,12.5); checkStartCutInventory(*result.analysis,12.5);
        const auto& proof = *result.analysis;
        const auto& frame = proof.reference().events().providers().frame();
        const auto nearest = proof.searchStats().nearestCut;
        const auto& cut = proof.startCuts()[nearest]; REQUIRE_FALSE(cut.transition);
        const auto evidence = proof.startCutExclusion(nearest);
        REQUIRE(evidence.kind == geo::CacheStartCutExclusionKind::CircleSupport);
        CHECK(evidence.point.x == 16.75); CHECK(evidence.point.y == -31);
        const double direction = sourceReversed ? -1 : 1;
        CHECK(cut.axisX.lo() == -direction/frame.scale());
        CHECK(cut.axisX.hi() == cut.axisX.lo());
        CHECK(cut.axisY.lo() == direction*.75/frame.scale());
        CHECK(cut.axisY.hi() == cut.axisY.lo());
        CHECK(Quad(.75)*Quad(cut.axisX.lo())+Quad(cut.axisY.lo()) == 0);
        CHECK(evidence.bound.lo() > 0);
        CHECK(proof.startCutPieces(nearest).empty()); CHECK_FALSE(proof.startCutLapAdmission(nearest));
        CHECK(proof.searchStats().acceptedRoutes == 1);
    }
}

TEST_CASE("Source projection bounds tighten rotated cancellation only with a monotone sinusoid proof",
          "[lc-offsetcurve][geometry][source-projection-tightening]") {
    RS_EllipseData source{{16,-32},{3,4},1,0,0,false};
    auto frame = geo::ReferenceFrame::create(source,15); REQUIRE(frame);
    LC_CurveJetBounds jet; LC_Interval tight;
    REQUIRE(frame->boundSourceJet(2.125,2.35,jet));
    const auto independentXY = LC_Interval::point(-3)*jet.dx+LC_Interval::point(-4)*jet.dy;
    REQUIRE(independentXY.lo() < 0);
    REQUIRE(frame->boundSourceProjectedDerivative(2.125,2.35,LC_Interval::point(-3),LC_Interval::point(-4),tight));
    CHECK(tight.lo() > 0); // same true projection, not a clipped negative bound
    for (unsigned k = 0; k <= 16; ++k) {
        const double t = 2.125+(2.35-2.125)*double(k)/16;
        const auto oracle = referenceOracle(source,*frame,15,t,false);
        checkEnclosure(tight,-3*oracle[2]-4*oracle[3]);
    }
    source.reversed = true; frame = geo::ReferenceFrame::create(source,15); REQUIRE(frame);
    REQUIRE(frame->boundSourceJet(.3125,.625,jet));
    const auto naive = -jet.dx; // world-X projection: no X/Y dependency to remove
    REQUIRE(frame->boundSourceProjectedDerivative(.3125,.625,LC_Interval::point(-1),LC_Interval::point(0),tight));
    CHECK(tight.hi() < 0); CHECK(tight.width() < naive.width()); // endpoint tightening itself is exercised
    for (unsigned k = 0; k <= 16; ++k) {
        const double t = .3125+(.625-.3125)*double(k)/16;
        checkEnclosure(tight,-referenceOracle(source,*frame,15,t,false)[2]);
    }
    const auto ax = LC_Interval::hull(-1,-.5), ay = LC_Interval::hull(0,.25);
    REQUIRE(frame->boundSourceProjectedDerivative(.1,.2,ax,ay,tight));
    for (unsigned k = 0; k <= 8; ++k) for (const double x : {-1.,-.5}) for (const double y : {0.,.25}) {
        const auto oracle = referenceOracle(source,*frame,15,.1+.1*double(k)/8,false);
        checkEnclosure(tight,Quad(x)*oracle[2]+Quad(y)*oracle[3]);
    }
    REQUIRE(frame->boundSourceProjectedDerivative(0,frame->parameterEnd(),LC_Interval::point(-1),LC_Interval::point(0),tight));
    CHECK(tight.lo() < 0); CHECK(tight.hi() > 0); // equal whole-turn endpoints MUST NOT collapse its range
    CHECK_FALSE(frame->boundSourceProjectedDerivative(.2,.1,ax,ay,tight)); CHECK_FALSE(tight.isValid());
}

TEST_CASE("Public complete cut search accepts exact caps and refuses every one-less ledger without a certificate",
          "[lc-offsetcurve][geometry][cache-topology-chain][cache-search-completion]") {
    const auto source = basis(1,1); const auto cache = nonNearestCutCache();
    const auto definition = localTopologyDefinition(source, 3);
    const geo::CheckWork before{2,3,5,1,7};
    geo::WorkBudget measured; REQUIRE(measured.adoptWork(before));
    const auto baseline = geo::checkCache(cache, definition, 2.5, measured);
    checkPublicPath(baseline, measured, 3, 2.5); checkStartCutInventory(*baseline.analysis, 2.5);
    const auto work = baseline.work;
    const LC_OffsetCheckLimits exact{work.cacheSpans, work.vertices, work.distancePairs, work.deepest, work.orderingComparisons};
    geo::WorkBudget exactBudget{exact}; REQUIRE(exactBudget.adoptWork(before));
    const auto accepted = geo::checkCache(cache, definition, 2.5, exactBudget);
    checkPublicPath(accepted, exactBudget, 3, 2.5); checkWork(accepted.work, work);
    for (unsigned dimension = 0; dimension < 5; ++dimension) {
        CAPTURE(dimension); auto limits = exact;
        if (dimension == 0) { REQUIRE(work.cacheSpans > before.cacheSpans); --limits.maxCacheSpans; }
        if (dimension == 1) { REQUIRE(work.vertices > before.vertices); --limits.maxVertices; }
        if (dimension == 2) { REQUIRE(work.distancePairs > before.distancePairs); --limits.maxDistancePairs; }
        if (dimension == 3) { REQUIRE(work.deepest > before.deepest); --limits.maxDepth; }
        if (dimension == 4) { REQUIRE(work.orderingComparisons > before.orderingComparisons); --limits.maxOrderingComparisons; }
        geo::WorkBudget budget{limits}; REQUIRE(budget.adoptWork(before));
        const auto refused = geo::checkCache(cache, definition, 2.5, budget);
        CHECK(refused.reason == LC_OffsetCheckResult::Reason::LimitExceeded); CHECK_FALSE(refused.analysis);
        checkWork(refused.work, budget.consumed()); const auto stopped = refused.work;
        const auto retry = geo::checkCache(cache, definition, 2.5, budget);
        CHECK(retry.reason == LC_OffsetCheckResult::Reason::LimitExceeded); CHECK_FALSE(retry.analysis);
        checkWork(retry.work, stopped);
        CHECK(cache.controlPoints.front().x == .25); CHECK(cache.controlPoints.front().y == 0);
        CHECK(cache.knotslist == nonNearestCutCache().knotslist);
    }
}

TEST_CASE("Public cut inventory and memo diagnostics own no caller graph frame or budget",
          "[lc-offsetcurve][geometry][cache-topology-chain][cache-search-completion]") {
    std::unique_ptr<const geo::CacheTopologyAnalysis> proof;
    {
        auto source = basis(1,1); auto cache = nonNearestCutCache(); geo::WorkBudget budget;
        auto result = geo::checkCache(cache, localTopologyDefinition(source,3), 2.5, budget);
        REQUIRE(result); proof = std::move(result.analysis);
    }
    REQUIRE(proof); checkStartCutInventory(*proof, 2.5);
    CHECK(proof->searchStats().acceptedRoutes == 1); CHECK_FALSE(proof->pieces().empty());
}

TEST_CASE("cache topology rejects crossing branch switch", "[lc-offsetcurve][geometry][cache-topology-chain]") {
    const auto upper = upperLensCache();
    std::vector<double> knots; for (unsigned i = 0; i <= 32; ++i) { knots.push_back(double(i)); }
    auto cache = cacheLayoutFixture(3, knots);
    std::copy(upper.controlPoints.begin(), upper.controlPoints.end(), cache.controlPoints.begin());
    // At the left crossing, the incoming upper occurrence must continue through its paired
    // lower outgoing occurrence, not reverse along the same incoming upper edge.
    std::reverse_copy(upper.controlPoints.begin(), upper.controlPoints.end() - 1, cache.controlPoints.begin() + 49);
    geo::WorkBudget budget;
    const auto result = geo::checkCache(cache, localTopologyDefinition(basis(100, .5), -49), .01, budget);
    checkPublicRefusal(result, LC_OffsetCheckResult::Reason::Topology, budget); CHECK(result.span == 16);
}

TEST_CASE("cache topology rejects wrong event occurrence identity", "[lc-offsetcurve][geometry][cache-topology-chain]") {
    const auto upper = upperLensCache();
    const auto reference = terminalGraph(basis(100, .5), -49);
    const auto& edge = reference.analysis->edges()[0];
    const auto& span = reference.analysis->spans()[edge.span];
    REQUIRE(reference.analysis->occurrenceIncidence()[span.end.value].outgoing == noTerminal);
    bool pairedOutgoing = false;
    for (const auto& event : reference.analysis->events().pairs()) {
        if (event.first.value != span.end.value && event.second.value != span.end.value) { continue; }
        const auto partner = event.first.value == span.end.value ? event.second : event.first;
        pairedOutgoing = reference.analysis->occurrenceIncidence()[partner.value].outgoing != noTerminal;
        CHECK(partner.value != span.end.value);
    }
    REQUIRE(pairedOutgoing);
    geo::WorkBudget budget;
    // Its endpoints have correct physical node images but not a terminal return credential.
    checkPublicRefusal(geo::checkCache(upper, localTopologyDefinition(basis(100, .5), -49), .01, budget),
        LC_OffsetCheckResult::Reason::Topology, budget);
}

TEST_CASE("cache topology rejects continuation across hidden interval", "[lc-offsetcurve][geometry][cache-topology-chain]") {
    // The untrimmed major-axis neighbourhood is hidden for this inward whole source.
    const auto cache = arcCache(basis(100, .5, 0, .1), -49, 3);
    geo::WorkBudget budget;
    checkPublicRefusal(geo::checkCache(cache, localTopologyDefinition(basis(100, .5), -49), .01, budget),
        LC_OffsetCheckResult::Reason::OffCurve, budget);
}

TEST_CASE("cache topology rejects hidden seam inward lens shortcut", "[lc-offsetcurve][geometry][cache-topology-chain]") {
    // A source-wrap shortcut through theta=0 cannot supply G2's hidden seam.
    const auto cache = arcCache(basis(100, .5, 6.2, .1), -49, 3);
    geo::WorkBudget budget;
    checkPublicRefusal(geo::checkCache(cache, localTopologyDefinition(basis(100, .5), -49), .01, budget),
        LC_OffsetCheckResult::Reason::OffCurve, budget);
}

TEST_CASE("cache topology rejects unauthorized disconnected component join", "[lc-offsetcurve][geometry][cache-topology-chain]") {
    const auto caches = twoComponentCaches();
    const auto openSpans = (caches[0].controlPoints.size()-1)/3;
    const auto closedSpans = (caches[1].controlPoints.size()-1)/3;
    std::vector<double> knots;
    for (std::size_t i = 0; i <= openSpans + 1 + closedSpans; ++i) { knots.push_back(double(i)); }
    auto cache = cacheLayoutFixture(3, knots);
    std::copy(caches[0].controlPoints.begin(), caches[0].controlPoints.end(), cache.controlPoints.begin());
    const auto a = caches[0].controlPoints.back(), b = caches[1].controlPoints.front();
    REQUIRE(a != b);
    cache.controlPoints[3*openSpans+1] = a + (b-a)/3;
    cache.controlPoints[3*openSpans+2] = b - (b-a)/3;
    std::copy(caches[1].controlPoints.begin(), caches[1].controlPoints.end(), cache.controlPoints.begin() + 3*(openSpans+1));
    geo::WorkBudget budget;
    checkPublicRefusal(geo::checkCache(cache, localTopologyDefinition(basis(100, .05, M_PI/12, 7*M_PI/4), -4), .001, budget),
        LC_OffsetCheckResult::Reason::Topology, budget);
}

TEST_CASE("Reference stationary root bands retain a complete weak-monotone projection proof",
          "[lc-offsetcurve][geometry][reference-projection]") {
    const auto source = basis(100, .5, 2.6, 3.6);
    geo::WorkBudget budget;
    const auto prepared = geo::prepareReferenceProviders(source, -25, budget);
    REQUIRE(prepared.status == LC_IntersectionStatus::Ok); REQUIRE(prepared.provider);
    const auto& provider = *prepared.provider;
    LC_Interval bound;
    REQUIRE(provider.boundOwnProjectedDerivative(0, 1, LC_Interval::point(0), LC_Interval::point(-1), bound));
    CHECK(bound.lo() == 0); CHECK(bound.hi() > 0);
    for (unsigned i = 0; i <= 128; ++i) {
        checkEnclosure(bound, -referenceOracle(source, provider.frame(), -25, double(i)/128, true)[3]);
    }
    for (const auto& bad : {LC_Interval{}, LC_Interval::point(std::numeric_limits<double>::infinity())}) {
        bound = LC_Interval::point(123);
        CHECK_FALSE(provider.boundOwnProjectedDerivative(0, 1, bad, LC_Interval::point(1), bound));
        CHECK_FALSE(bound.isValid());
    }
    CHECK_FALSE(provider.boundOwnProjectedDerivative(-1, 1, LC_Interval::point(0), LC_Interval::point(-1), bound));
    CHECK_FALSE(bound.isValid());
}

TEST_CASE("Critical constant-factor exclusion spans nonadjacent stationary regions but no diagonal continuum",
          "[lc-offsetcurve][geometry][reference-projection]") {
    const auto source = basis(100, .5, 2.6, 3.6);
    geo::WorkBudget budget;
    const auto critical = geo::analyseCritical(source, -25, budget);
    REQUIRE(critical.status == LC_IntersectionStatus::Ok); REQUIRE(critical.adapter);
    REQUIRE(critical.adapter->regime() == geo::CriticalRegime::MinimumStationary);
    const auto& segments = critical.adapter->segments(); REQUIRE(segments.size() == 3);
    const auto domain = LC_IntersectionDomain{0, 2, LC_Interval::hull(segments[0].t0, segments[0].t1),
        LC_Interval::hull(segments[2].t0, segments[2].t1)};
    const auto certificate = critical.adapter->certifyIntersection(domain);
    CHECK(certificate.outcome == LC_AnalyticIntersectionCertificate::Outcome::Excluded);
    CHECK(certificate.proof == LC_IntersectionProof::AnalyticExclusion);
    const auto continuum = critical.adapter->certifyIntersection({0, 0, domain.parameterA, domain.parameterA});
    CHECK(continuum.outcome == LC_AnalyticIntersectionCertificate::Outcome::Unavailable);
    const Quad pi = boost::math::constants::pi<Quad>();
    CHECK(Quad(source.angle2)-Quad(source.angle1) < pi);
    // F=1-25*5000/s^3 >=0 because s>=50, with only the isolated major-axis zero.
    CHECK(Quad(25)*5000 == Quad(50)*50*50);
    CHECK(-sin(Quad(source.angle1)) < 0); CHECK(-sin(Quad(source.angle2)) > 0);
    CHECK(cos(Quad(source.angle1)) < 0); CHECK(cos(Quad(source.angle2)) < 0);
}
