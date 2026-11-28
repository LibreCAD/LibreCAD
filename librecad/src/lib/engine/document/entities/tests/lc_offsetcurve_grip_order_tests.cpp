/*
 * This file is part of LibreCAD, a 2D CAD program.
 * Copyright (C) 2026 LibreCAD.org
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <memory>

#include <boost/multiprecision/cpp_int.hpp>

#include "lc_offsetcurve.h"

namespace {
using Exact = boost::multiprecision::cpp_rational;
constexpr double tolerance = 0.01;
constexpr double radius = 1.25;

Exact squaredDistance(const RS_Vector &a, const RS_Vector &b) {
    const Exact dx = Exact(a.x) - Exact(b.x), dy = Exact(a.y) - Exact(b.y);
    return Exact(dx*dx + dy*dy);
}

Exact projectedParameter(const RS_SplineData &data, const RS_Vector &point) {
    const auto &a = data.controlPoints.front(), &b = data.controlPoints.back();
    const Exact dx = Exact(b.x) - Exact(a.x), dy = Exact(b.y) - Exact(a.y);
    const Exact denominator = dx*dx + dy*dy;
    REQUIRE(denominator > 0);
    const Exact parameter = ((Exact(point.x) - Exact(a.x))*dx + (Exact(point.y) - Exact(a.y))*dy)/denominator;
    return std::max(Exact(0), std::min(Exact(1), parameter));
}

std::unique_ptr<LC_OffsetCurve> shortArc(const unsigned degree, const double end, const bool cacheReversed) {
    LC_EllipseOffsetDefinition definition;
    definition.basis = {{0, 0}, {1, 0}, 1, 0, end, false};
    definition.distance = 0.25; definition.relTolerance = 0.01;
    definition.cacheReversed = cacheReversed;
    RS_SplineData data{static_cast<int>(degree), false};
    const RS_Vector first{radius, 0}, last{radius*std::cos(end), radius*std::sin(end)};
    data.controlPoints.push_back(first);
    if (degree == 3) {
        data.controlPoints.push_back(first + RS_Vector{0, radius*end/3});
        data.controlPoints.push_back(last - RS_Vector{-radius*std::sin(end), radius*std::cos(end)}*(end/3));
    }
    data.controlPoints.push_back(last);
    data.weights.assign(degree + 1, 1);
    for (unsigned i = 0; i <= degree; ++i) data.knotslist.push_back(0);
    for (unsigned i = 0; i <= degree; ++i) data.knotslist.push_back(1);
    if (cacheReversed) std::reverse(data.controlPoints.begin(), data.controlPoints.end());
    const auto fingerprint = RS_Spline::geometryFingerprint(data);
    RS_Spline cache{nullptr, data};
    LC_OffsetCheckResult checked;
    auto curve = LC_OffsetCurve::adopt(cache, definition, tolerance, &checked);
    CAPTURE(degree, end, cacheReversed, checked.reason, checked.span);
    REQUIRE(checked); REQUIRE(checked.analysis); REQUIRE(curve); REQUIRE(curve->hasEditableDefinition());
    CHECK(RS_Spline::geometryFingerprint(curve->getData()) == fingerprint);
    return curve;
}

void checkShortArcGrips(const LC_OffsetCurve &curve, const double end, const bool cacheReversed) {
    const auto &grips = curve.cachedDefinitionGrips();
    REQUIRE(grips.size() == 2);
    const RS_Vector quarter{radius, 0}, midpoint{radius*std::cos(end/2), radius*std::sin(end/2)};
    const auto &first = curve.getData().controlPoints.front();
    const Exact squaredTolerance = Exact(tolerance)*Exact(tolerance);
    // Both seeds have the same first within-T witness in either cache order.
    CHECK(squaredDistance(first, quarter) < squaredTolerance);
    CHECK(squaredDistance(first, midpoint) < squaredTolerance);
    const std::array<RS_Vector, 2> expected = cacheReversed ?
        std::array<RS_Vector, 2>{{midpoint, quarter}} : std::array<RS_Vector, 2>{{quarter, midpoint}};
    for (std::size_t i = 0; i < grips.size(); ++i)
        CHECK(std::hypot(grips[i].x - expected[i].x, grips[i].y - expected[i].y) < 1e-12);
}

std::unique_ptr<LC_OffsetCurve> ring(const bool cacheReversed) {
    RS_SplineData data{3, false};
    std::array<RS_Vector, 4> net{{{2, 0}, {2, 9.0/8}, {9.0/8, 2}, {0, 2}}};
    for (unsigned quadrant = 0; quadrant < 4; ++quadrant) {
        for (unsigned i = quadrant ? 1 : 0; i < 4; ++i) data.controlPoints.push_back(net[i]);
        for (auto &point : net) point = {-point.y, point.x};
    }
    for (unsigned i = 0; i <= 4; ++i)
        for (unsigned j = 0; j < (i == 0 || i == 4 ? 4u : 3u); ++j) data.knotslist.push_back(i);
    data.weights.assign(data.controlPoints.size(), 1); data.m_closedFlag = true;
    if (cacheReversed) std::reverse(data.controlPoints.begin(), data.controlPoints.end());
    LC_EllipseOffsetDefinition definition;
    definition.basis = {{0, 0}, {1, 0}, 1}; definition.distance = 1;
    definition.relTolerance = 0.01; definition.cacheReversed = cacheReversed;
    RS_Spline cache{nullptr, data};
    LC_OffsetCheckResult checked;
    auto curve = LC_OffsetCurve::adopt(cache, definition, 0.02, &checked);
    CAPTURE(cacheReversed, checked.reason, checked.span);
    REQUIRE(checked); REQUIRE(checked.analysis); REQUIRE(curve); REQUIRE(curve->cacheFormsClosedChain());
    return curve;
}
}

TEST_CASE("Short line offset grips follow exact segment projections despite overlapping visibility neighborhoods",
          "[lc-offsetcurve][grips][grip-order]") {
    for (const double end : {0.001, 0.005}) for (const bool cacheReversed : {false, true}) {
        auto curve = shortArc(1, end, cacheReversed);
        checkShortArcGrips(*curve, end, cacheReversed);
        const auto &grips = curve->cachedDefinitionGrips();
        const Exact first = projectedParameter(curve->getData(), grips[0]);
        const Exact second = projectedParameter(curve->getData(), grips[1]);
        CHECK(first < second);
        CHECK(first >= 0); CHECK(second <= 1);
    }
}

TEST_CASE("Short cubic offset grips use curve correspondence instead of the first visibility contact",
          "[lc-offsetcurve][grips][grip-order]") {
    for (const double end : {0.001, 0.005}) for (const bool cacheReversed : {false, true}) {
        auto curve = shortArc(3, end, cacheReversed);
        checkShortArcGrips(*curve, end, cacheReversed);
        const auto &controls = curve->getData().controlPoints;
        // Strictly ordered Bernstein y controls give an independent traversal
        // oracle for this concrete short Hermite arc, without the new helper.
        for (std::size_t i = 1; i < controls.size(); ++i)
            CHECK((cacheReversed ? controls[i].y < controls[i - 1].y : controls[i - 1].y < controls[i].y));
        const auto &grips = curve->cachedDefinitionGrips();
        CHECK((cacheReversed ? grips[1].y < grips[0].y : grips[0].y < grips[1].y));
    }
}

TEST_CASE("Cubic closest correspondence retains all four quarter grips and canonical closed seam order",
          "[lc-offsetcurve][grips][grip-order]") {
    for (const bool cacheReversed : {false, true}) {
        auto curve = ring(cacheReversed);
        const auto &grips = curve->cachedDefinitionGrips();
        REQUIRE(grips.size() == 4);
        const double direction = cacheReversed ? -1 : 1;
        const std::array<RS_Vector, 4> expected{{{2, 0}, {0, 2*direction}, {-2, 0}, {0, -2*direction}}};
        for (std::size_t i = 0; i < grips.size(); ++i) {
            CHECK(grips[i].x == expected[i].x); CHECK(grips[i].y == expected[i].y);
        }
        CHECK(curve->getMiddlePoint().valid);
    }
}

TEST_CASE("Admitted tolerance-band carriers retain visible cache quarter grips",
          "[lc-offsetcurve][grips][grip-coverage]") {
    for (const bool cacheReversed : {false, true}) {
        auto original = ring(cacheReversed);
        auto cache = original->toPlainSpline();
        constexpr double factor = 1.0125;
        cache->scale({0, 0}, {factor, factor});
        LC_OffsetCheckResult checked;
        auto curve = LC_OffsetCurve::adopt(*cache, original->definition(), .02, &checked);
        CAPTURE(cacheReversed, checked.reason, checked.span);
        REQUIRE(checked); REQUIRE(curve);
        // This actual C admission allows geometry between T and 2T away from
        // the exact profile. Unproved ideal seeds must not become snap points;
        // the visible cache quadrants still need usable grips.
        const auto &grips = curve->cachedDefinitionGrips();
        REQUIRE(grips.size() == 4);
        for (const auto &expected : std::array<RS_Vector, 4>{{{2*factor, 0}, {0, 2*factor}, {-2*factor, 0}, {0, -2*factor}}}) {
            double nearest = std::numeric_limits<double>::infinity();
            for (const auto &point : grips)
                nearest = std::min(nearest, std::hypot(point.x - expected.x, point.y - expected.y));
            CHECK(nearest < .002);
        }
    }
}
