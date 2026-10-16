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
    CHECK(actual.cubicPieces == expected.cubicPieces);
    CHECK(actual.vertices == expected.vertices);
    CHECK(actual.distancePairs == expected.distancePairs);
    CHECK(actual.deepest == expected.deepest);
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
    CHECK(result.work.distancePairs == 23);
    CHECK(result.work.cubicPieces == 0);
    CHECK(result.work.deepest == 0);
    CHECK(result.work.vertices == budget.consumed().vertices);
    CHECK(result.work.distancePairs == budget.consumed().distancePairs);
    CHECK(result.work.cubicPieces == budget.consumed().cubicPieces);
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
    CHECK(result.work.distancePairs == 23);
    CHECK(result.work.deepest == 0);
    checkWork(result.work, budget.consumed());

    geo::WorkBudget examined{LC_OffsetCheckLimits{1, 8, 23, 20}};
    const auto later = geo::analyseCritical(basis(100.0, 0.5), -30.0, examined);
    CHECK(later.status == LC_IntersectionStatus::LimitExceeded);
    CHECK(later.work.vertices == 8);
    CHECK(later.work.distancePairs == 23);
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
            CHECK(baseline.work.cubicPieces == before.cubicPieces);
            for (const bool prespent : {false, true}) {
                const std::size_t spentV = prespent ? 5 : 0, spentP = prespent ? 7 : 0;
                for (int cap = 0; cap < 3; ++cap) {
                    CAPTURE(whole, collapsed, prespent, cap, s);
                    LC_OffsetCheckLimits limits;
                    limits.maxCubicPieces = 2;
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
                        CHECK(result.work.cubicPieces == std::size_t(prespent));
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
          "[lc-offsetcurve][geometry][reference-providers]") {
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
        const std::size_t admittedV = allowed == 0 ? 3 : allowed <= 4 ? 4 : allowed <= 8 ? 5 : 5 + s;
        CHECK(result.work.vertices == before.vertices + admittedV);
        CHECK(result.work.distancePairs == before.distancePairs + allowed);
        CHECK(result.work.deepest == before.deepest);
        CHECK(result.work.cubicPieces == before.cubicPieces);
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
          "[lc-offsetcurve][geometry][reference-providers]") {
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
    CHECK(result.work.vertices == base.consumed().vertices + 3);
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
