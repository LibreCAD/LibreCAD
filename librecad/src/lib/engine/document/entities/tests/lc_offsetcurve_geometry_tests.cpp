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
#include <cmath>
#include <limits>
#include <random>

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
