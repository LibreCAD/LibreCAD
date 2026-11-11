/*
 * This file is part of LibreCAD, a 2D CAD program.
 * Copyright (C) 2026 LibreCAD.org
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "../lc_offsetworkerkernel.h"
#include "../lc_offsetworkersourcecache.h"
#include "../lc_offsetworkertolerance.h"
#include "rs_ellipse.h"
#include "rs_arc.h"
#include "rs_line.h"
#include "rs_spline.h"
#include <catch2/catch_test_macros.hpp>
#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <numeric>
#include <optional>
#include <tuple>
#include <vector>
#include <boost/multiprecision/cpp_bin_float.hpp>

namespace req = LC_OffsetWorkerRequest;
namespace result = LC_OffsetWorkerResult;
namespace trim = LC_OffsetCacheTrim;
namespace geo = LC_EllipseOffsetGeometry;
namespace {
using SplineOracle = boost::multiprecision::number<
    boost::multiprecision::cpp_bin_float<256>>;

// Independent homogeneous de Boor evaluation for conditioning diagnostics.
struct HomogeneousPoint {
    SplineOracle x;
    SplineOracle y;
    SplineOracle w;
};

std::pair<SplineOracle, SplineOracle> evaluateSplineOracle(
    const RS_SplineData& data, const SplineOracle& parameter) {
    const std::size_t degree = data.degree;
    const std::size_t lastControl = data.controlPoints.size() - 1;
    std::size_t low = degree;
    std::size_t high = lastControl + 1;
    while (high - low > 1) {
        const std::size_t middle = (low + high) / 2;
        if (parameter < SplineOracle{data.knotslist[middle]}) high = middle;
        else low = middle;
    }
    const std::size_t span = low;
    std::vector<HomogeneousPoint> work(degree + 1);
    for (std::size_t j = 0; j <= degree; ++j) {
        const std::size_t index = span - degree + j;
        const SplineOracle weight{data.weights[index]};
        work[j] = {SplineOracle{data.controlPoints[index].x} * weight,
                   SplineOracle{data.controlPoints[index].y} * weight,
                   weight};
    }
    for (std::size_t r = 1; r <= degree; ++r) {
        for (std::size_t j = degree; j >= r; --j) {
            const std::size_t index = span - degree + j;
            const SplineOracle denominator{
                data.knotslist[index + degree - r + 1] - data.knotslist[index]};
            const SplineOracle alpha =
                (parameter - SplineOracle{data.knotslist[index]}) / denominator;
            work[j] = {work[j - 1].x * (1 - alpha) + work[j].x * alpha,
                       work[j - 1].y * (1 - alpha) + work[j].y * alpha,
                       work[j - 1].w * (1 - alpha) + work[j].w * alpha};
            if (j == r) break;
        }
    }
    return {work[degree].x / work[degree].w,
            work[degree].y / work[degree].w};
}

std::optional<HomogeneousPoint> evaluateHomogeneousSplineOracle(
    const std::vector<HomogeneousPoint>& controls,
    const std::vector<SplineOracle>& knots, const std::size_t degree,
    const SplineOracle& parameter, const bool leftLimit) {
    if (controls.empty() || degree >= controls.size() ||
        knots.size() != controls.size() + degree + 1) {
        return std::nullopt;
    }
    std::size_t span = controls.size();
    for (std::size_t candidate = degree; candidate < controls.size(); ++candidate) {
        const bool contains = leftLimit
                                  ? knots[candidate] < parameter &&
                                        parameter <= knots[candidate + 1]
                                  : knots[candidate] <= parameter &&
                                        parameter < knots[candidate + 1];
        if (contains && knots[candidate] < knots[candidate + 1]) {
            span = candidate;
            break;
        }
    }
    if (span == controls.size()) return std::nullopt;

    std::vector<HomogeneousPoint> work(degree + 1);
    for (std::size_t j = 0; j <= degree; ++j)
        work[j] = controls[span - degree + j];
    for (std::size_t order = 1; order <= degree; ++order) {
        for (std::size_t j = degree; j >= order; --j) {
            const std::size_t index = span - degree + j;
            const SplineOracle denominator =
                knots[index + degree - order + 1] - knots[index];
            const SplineOracle alpha = denominator == 0
                                           ? SplineOracle{0}
                                           : (parameter - knots[index]) / denominator;
            work[j] = {
                work[j - 1].x * (1 - alpha) + work[j].x * alpha,
                work[j - 1].y * (1 - alpha) + work[j].y * alpha,
                work[j - 1].w * (1 - alpha) + work[j].w * alpha};
            if (j == order) break;
        }
    }
    return work[degree];
}

std::optional<SplineOracle> oracleSplineCurvatureLimit(
    const RS_SplineData& data, const SplineOracle& parameter,
    const bool leftLimit) {
    if (data.degree < 2 || data.controlPoints.size() < data.degree + 1 ||
        data.knotslist.size() != data.controlPoints.size() + data.degree + 1 ||
        (!data.weights.empty() && data.weights.size() != data.controlPoints.size())) {
        return std::nullopt;
    }

    std::vector<HomogeneousPoint> controls;
    controls.reserve(data.controlPoints.size());
    for (std::size_t i = 0; i < data.controlPoints.size(); ++i) {
        const SplineOracle weight{data.weights.empty() ? 1.0 : data.weights[i]};
        controls.push_back({SplineOracle{data.controlPoints[i].x} * weight,
                            SplineOracle{data.controlPoints[i].y} * weight,
                            weight});
    }
    std::vector<SplineOracle> knots;
    knots.reserve(data.knotslist.size());
    for (const double knot : data.knotslist) knots.emplace_back(knot);
    std::size_t degree = data.degree;
    std::array<std::vector<HomogeneousPoint>, 3> derivatives;
    std::array<std::vector<SplineOracle>, 3> derivativeKnots;
    derivatives[0] = controls;
    derivativeKnots[0] = knots;
    for (std::size_t order = 1; order <= 2; ++order) {
        const auto& previous = derivatives[order - 1];
        const auto& previousKnots = derivativeKnots[order - 1];
        if (previous.size() < 2 || degree == 0) return std::nullopt;
        auto& current = derivatives[order];
        current.reserve(previous.size() - 1);
        for (std::size_t i = 0; i + 1 < previous.size(); ++i) {
            const SplineOracle denominator =
                previousKnots[i + degree + 1] - previousKnots[i + 1];
            const SplineOracle scale = denominator == 0
                                           ? SplineOracle{0}
                                           : SplineOracle{degree} / denominator;
            current.push_back({(previous[i + 1].x - previous[i].x) * scale,
                               (previous[i + 1].y - previous[i].y) * scale,
                               (previous[i + 1].w - previous[i].w) * scale});
        }
        auto& currentKnots = derivativeKnots[order];
        currentKnots.assign(previousKnots.begin() + 1, previousKnots.end() - 1);
        --degree;
    }

    const auto position = evaluateHomogeneousSplineOracle(
        derivatives[0], derivativeKnots[0], data.degree, parameter, leftLimit);
    const auto first = evaluateHomogeneousSplineOracle(
        derivatives[1], derivativeKnots[1], data.degree - 1, parameter, leftLimit);
    const auto second = evaluateHomogeneousSplineOracle(
        derivatives[2], derivativeKnots[2], data.degree - 2, parameter, leftLimit);
    if (!position || !first || !second || position->w == 0) return std::nullopt;

    const auto rationalDerivatives = [](const SplineOracle& coordinate,
                                        const SplineOracle& firstCoordinate,
                                        const SplineOracle& secondCoordinate,
                                        const SplineOracle& weight,
                                        const SplineOracle& firstWeight,
                                        const SplineOracle& secondWeight) {
        const SplineOracle firstValue =
            (firstCoordinate * weight - coordinate * firstWeight) / (weight * weight);
        const SplineOracle secondValue =
            secondCoordinate / weight - coordinate * secondWeight / (weight * weight) -
            2 * firstCoordinate * firstWeight / (weight * weight) +
            2 * coordinate * firstWeight * firstWeight / (weight * weight * weight);
        return std::pair<SplineOracle, SplineOracle>{firstValue, secondValue};
    };
    const auto x = rationalDerivatives(position->x, first->x, second->x,
                                       position->w, first->w, second->w);
    const auto y = rationalDerivatives(position->y, first->y, second->y,
                                       position->w, first->w, second->w);
    const SplineOracle speedSquared = x.first * x.first + y.first * y.first;
    if (speedSquared <= 0) return std::nullopt;
    return (x.first * y.second - y.first * x.second) /
           (speedSquared * sqrt(speedSquared));
}

SplineOracle oracleStencilStep(const RS_SplineData& data,
                               const SplineOracle& parameter,
                               const SplineOracle& requested,
                               const bool left,
                               const bool right,
                               const unsigned pointsOnSide) {
    SplineOracle step = requested;
    for (double knotValue : data.knotslist) {
        const SplineOracle knot{knotValue};
        const SplineOracle gap = parameter - knot;
        if (left && gap > 0)
            step = std::min(step, gap / pointsOnSide);
        const SplineOracle rightGap = knot - parameter;
        if (right && rightGap > 0)
            step = std::min(step, rightGap / pointsOnSide);
    }
    return step;
}

SplineOracle oracleSpeedSquared(const RS_SplineData& data,
                                const SplineOracle& parameter,
                                const SplineOracle& step) {
    // High-precision finite differences avoid sharing the production jet path.
    const std::size_t lastControl = data.controlPoints.size() - 1;
    const SplineOracle domainEnd{data.knotslist[lastControl + 1]};
    const bool atKnot = std::any_of(data.knotslist.begin(), data.knotslist.end(),
                                    [&](double knot) { return parameter == SplineOracle{knot}; });
    const bool useLeft = parameter == domainEnd;
    const SplineOracle h = oracleStencilStep(data, parameter, step,
                                              atKnot ? useLeft : true,
                                              atKnot ? !useLeft : true, 2);
    if (useLeft) {
        const auto point = evaluateSplineOracle(data, parameter);
        const auto one = evaluateSplineOracle(data, parameter - h);
        const auto two = evaluateSplineOracle(data, parameter - 2 * h);
        const SplineOracle dx = (3 * point.first - 4 * one.first + two.first) / (2 * h);
        const SplineOracle dy = (3 * point.second - 4 * one.second + two.second) / (2 * h);
        return dx * dx + dy * dy;
    }
    if (atKnot) {
        const auto point = evaluateSplineOracle(data, parameter);
        const auto one = evaluateSplineOracle(data, parameter + h);
        const auto two = evaluateSplineOracle(data, parameter + 2 * h);
        const SplineOracle dx = (-3 * point.first + 4 * one.first - two.first) / (2 * h);
        const SplineOracle dy = (-3 * point.second + 4 * one.second - two.second) / (2 * h);
        return dx * dx + dy * dy;
    }
    const auto before = evaluateSplineOracle(data, parameter - h);
    const auto after = evaluateSplineOracle(data, parameter + h);
    const SplineOracle dx = (after.first - before.first) / (2 * h);
    const SplineOracle dy = (after.second - before.second) / (2 * h);
    return dx * dx + dy * dy;
}

SplineOracle oracleOffsetFactor(const RS_SplineData& data,
                                const SplineOracle& parameter,
                                const SplineOracle& step,
                                const SplineOracle& domainStart,
                                const SplineOracle& domainEnd,
                                const SplineOracle& signedDistance,
                                const bool leftLimit = false) {
    const auto point = evaluateSplineOracle(data, parameter);
    std::pair<SplineOracle, SplineOracle> first;
    std::pair<SplineOracle, SplineOracle> second;
    const bool atKnot = std::any_of(data.knotslist.begin(), data.knotslist.end(),
                                    [&](double knot) { return parameter == SplineOracle{knot}; });
    const bool useLeft = parameter == domainEnd || leftLimit;
    const bool oneSided = parameter == domainStart || parameter == domainEnd || atKnot || leftLimit;
    const SplineOracle h = oracleStencilStep(data, parameter, step,
                                              useLeft || !oneSided,
                                              !useLeft || !oneSided, 3);
    if (oneSided && !useLeft) {
        const auto one = evaluateSplineOracle(data, parameter + h);
        const auto two = evaluateSplineOracle(data, parameter + 2 * h);
        const auto three = evaluateSplineOracle(data, parameter + 3 * h);
        first = {(-3 * point.first + 4 * one.first - two.first) / (2 * h),
                 (-3 * point.second + 4 * one.second - two.second) / (2 * h)};
        second = {(2 * point.first - 5 * one.first + 4 * two.first - three.first) / (h * h),
                  (2 * point.second - 5 * one.second + 4 * two.second - three.second) / (h * h)};
    } else if (oneSided) {
        const auto one = evaluateSplineOracle(data, parameter - h);
        const auto two = evaluateSplineOracle(data, parameter - 2 * h);
        const auto three = evaluateSplineOracle(data, parameter - 3 * h);
        first = {(3 * point.first - 4 * one.first + two.first) / (2 * h),
                 (3 * point.second - 4 * one.second + two.second) / (2 * h)};
        second = {(2 * point.first - 5 * one.first + 4 * two.first - three.first) / (h * h),
                  (2 * point.second - 5 * one.second + 4 * two.second - three.second) / (h * h)};
    } else {
        const auto before = evaluateSplineOracle(data, parameter - h);
        const auto after = evaluateSplineOracle(data, parameter + h);
        first = {(after.first - before.first) / (2 * h),
                 (after.second - before.second) / (2 * h)};
        second = {(after.first - 2 * point.first + before.first) / (h * h),
                  (after.second - 2 * point.second + before.second) / (h * h)};
    }
    const SplineOracle speedSquared = first.first * first.first + first.second * first.second;
    const SplineOracle cross = first.first * second.second - first.second * second.first;
    return 1 - signedDistance * cross / (speedSquared * sqrt(speedSquared));
}

SplineOracle oracleOffsetFactorNumerator(const RS_SplineData& data,
                                         const SplineOracle& parameter,
                                         const SplineOracle& derivativeStep,
                                         const SplineOracle& domainStart,
                                         const SplineOracle& domainEnd,
                                         const SplineOracle& signedDistance) {
    const SplineOracle speedSquared = oracleSpeedSquared(data, parameter, derivativeStep);
    const SplineOracle factor = oracleOffsetFactor(
        data, parameter, derivativeStep, domainStart, domainEnd, signedDistance);
    return factor * speedSquared * sqrt(speedSquared);
}

std::pair<SplineOracle, SplineOracle> oracleOffsetPoint(
    const RS_SplineData& data, const SplineOracle& parameter,
    const SplineOracle& step, const SplineOracle& domainStart,
    const SplineOracle& domainEnd, const SplineOracle& signedDistance,
    const bool leftLimit = false) {
    const auto point = evaluateSplineOracle(data, parameter);
    std::pair<SplineOracle, SplineOracle> first;
    const bool atKnot = std::any_of(data.knotslist.begin(), data.knotslist.end(),
                                    [&](double knot) { return parameter == SplineOracle{knot}; });
    const bool useLeft = parameter == domainEnd || leftLimit;
    const bool oneSided = parameter == domainStart || parameter == domainEnd || atKnot || leftLimit;
    const SplineOracle h = oracleStencilStep(data, parameter, step,
                                              useLeft || !oneSided,
                                              !useLeft || !oneSided, 2);
    if (oneSided && !useLeft) {
        const auto one = evaluateSplineOracle(data, parameter + h);
        const auto two = evaluateSplineOracle(data, parameter + 2 * h);
        first = {(-3 * point.first + 4 * one.first - two.first) / (2 * h),
                 (-3 * point.second + 4 * one.second - two.second) / (2 * h)};
    } else if (oneSided) {
        const auto one = evaluateSplineOracle(data, parameter - h);
        const auto two = evaluateSplineOracle(data, parameter - 2 * h);
        first = {(3 * point.first - 4 * one.first + two.first) / (2 * h),
                 (3 * point.second - 4 * one.second + two.second) / (2 * h)};
    } else {
        const auto before = evaluateSplineOracle(data, parameter - h);
        const auto after = evaluateSplineOracle(data, parameter + h);
        first = {(after.first - before.first) / (2 * h),
                 (after.second - before.second) / (2 * h)};
    }
    const SplineOracle speed = sqrt(first.first * first.first + first.second * first.second);
    return {point.first - signedDistance * first.second / speed,
            point.second + signedDistance * first.first / speed};
}

double sampledSplineDistance(const RS_SplineData& data, const RS_Vector& point,
                             const std::size_t sampleCount) {
    const RS_Spline spline{nullptr, data};
    double start = 0.0, end = 0.0;
    if (!spline.getParameterDomain(start, end) || sampleCount == 0)
        return std::numeric_limits<double>::infinity();
    double nearest = std::numeric_limits<double>::infinity();
    for (std::size_t i = 0; i <= sampleCount; ++i) {
        const double parameter = start + (end - start) *
            static_cast<double>(i) / static_cast<double>(sampleCount);
        LC_CurveJet jet;
        if (!spline.tryEvaluateJet(parameter, LC_CurveEvaluationSide::Interior, jet))
            return std::numeric_limits<double>::infinity();
        nearest = std::min(nearest, point.distanceTo(jet.point));
    }
    return nearest;
}

std::optional<double> maximumOracleOffsetDistance(
    const RS_SplineData& sourceData, const std::vector<RS_SplineData>& cacheData,
    const double signedDistance, const std::size_t cacheSampleCount = 16384) {
    if (cacheData.empty()) return std::nullopt;
    const RS_Spline source{nullptr, sourceData};
    if (!source.validate()) return std::nullopt;
    double domainStartValue = 0.0, domainEndValue = 0.0;
    if (!source.getParameterDomain(domainStartValue, domainEndValue)) return std::nullopt;
    const SplineOracle domainStart{domainStartValue};
    const SplineOracle domainEnd{domainEndValue};

    std::vector<double> breaks{domainStartValue};
    for (double knot : sourceData.knotslist) {
        if (knot > domainStartValue && knot < domainEndValue &&
            (breaks.empty() || breaks.back() != knot))
            breaks.push_back(knot);
    }
    breaks.push_back(domainEndValue);
    std::sort(breaks.begin(), breaks.end());
    breaks.erase(std::unique(breaks.begin(), breaks.end()), breaks.end());

    double maximum = 0.0;
    constexpr std::array<double, 5> fractions{.1, .3, .5, .7, .9};
    for (std::size_t span = 1; span < breaks.size(); ++span) {
        const double start = breaks[span - 1], end = breaks[span];
        if (!(start < end)) continue;
        const SplineOracle step{(end - start) * 1e-20};
        for (double fraction : fractions) {
            const double parameter = start + (end - start) * fraction;
            const auto expected = oracleOffsetPoint(
                sourceData, SplineOracle{parameter}, step, domainStart,
                domainEnd, SplineOracle{signedDistance});
            const RS_Vector oraclePoint{expected.first.convert_to<double>(),
                                        expected.second.convert_to<double>()};
            double distance = std::numeric_limits<double>::infinity();
            for (const RS_SplineData& cache : cacheData)
                distance = std::min(distance,
                    sampledSplineDistance(cache, oraclePoint, cacheSampleCount));
            if (!std::isfinite(distance)) return std::nullopt;
            maximum = std::max(maximum, distance);
        }
    }
    return maximum;
}

std::optional<double> maximumOracleOffsetDistance(
    const RS_SplineData& sourceData, const RS_SplineData& cacheData,
    const double signedDistance, const std::size_t cacheSampleCount = 16384) {
    return maximumOracleOffsetDistance(sourceData, std::vector<RS_SplineData>{cacheData},
                                       signedDistance, cacheSampleCount);
}

struct OracleSplineCusp {
    std::size_t span{0};
    SplineOracle parameter{};
    RS_Vector point;
};

std::vector<OracleSplineCusp> oracleOffsetCusps(const RS_SplineData& data,
                                                const SplineOracle& domainStart,
                                                const SplineOracle& domainEnd,
                                                const double signedDistance,
                                                const SplineOracle derivativeStepScale =
                                                    SplineOracle{"1e-20"}) {
    const std::size_t lastControl = data.controlPoints.size() - 1;
    std::vector<OracleSplineCusp> cusps;
    for (std::size_t span = data.degree; span <= lastControl; ++span) {
        const SplineOracle start{data.knotslist[span]};
        const SplineOracle end{data.knotslist[span + 1]};
        if (!(start < end)) continue;
        const SplineOracle width = end - start;
        std::vector<SplineOracle> parameters;
        parameters.reserve(1100);
        for (int sample = 0; sample <= 1024; ++sample)
            parameters.push_back(start + width * sample / 1024);
        for (int exponent = 4; exponent <= 48; ++exponent) {
            const SplineOracle fraction{std::ldexp(1.0, -exponent)};
            parameters.push_back(start + width * fraction);
            parameters.push_back(end - width * fraction);
        }
        std::sort(parameters.begin(), parameters.end());
        parameters.erase(std::unique(parameters.begin(), parameters.end()), parameters.end());
        const SplineOracle step = width * derivativeStepScale;
        SplineOracle previousParameter = parameters.front();
        SplineOracle previousFactor = oracleOffsetFactor(
            data, previousParameter, step, domainStart, domainEnd,
            SplineOracle{signedDistance}, false);
        const auto addCusp = [&](const SplineOracle& root, const bool leftLimit) {
            const auto point = oracleOffsetPoint(
                data, root, width * SplineOracle{"1e-30"},
                domainStart, domainEnd, SplineOracle{signedDistance}, leftLimit);
            const RS_Vector cusp{point.first.convert_to<double>(),
                                 point.second.convert_to<double>()};
            if (std::none_of(cusps.begin(), cusps.end(), [&](const OracleSplineCusp& existing) {
                    return existing.parameter == root &&
                           existing.point.distanceTo(cusp) < 1e-8;
                }))
                cusps.push_back({span, root, cusp});
        };
        for (std::size_t i = 1; i < parameters.size(); ++i) {
            const SplineOracle parameter = parameters[i];
            const SplineOracle factor = oracleOffsetFactor(
                data, parameter, step, domainStart, domainEnd,
                SplineOracle{signedDistance}, parameter == end);
            if (factor == 0) {
                addCusp(parameter, parameter == end);
            } else if (previousFactor != 0 &&
                       ((previousFactor < 0) != (factor < 0))) {
                SplineOracle lo = previousParameter;
                SplineOracle hi = parameter;
                SplineOracle loFactor = previousFactor;
                for (int iteration = 0; iteration < 48; ++iteration) {
                    const SplineOracle middle = (lo + hi) / 2;
                    const SplineOracle middleFactor = oracleOffsetFactor(
                        data, middle, step, domainStart, domainEnd,
                        SplineOracle{signedDistance});
                    if ((loFactor < 0) == (middleFactor < 0)) {
                        lo = middle;
                        loFactor = middleFactor;
                    } else {
                        hi = middle;
                    }
                }
                addCusp((lo + hi) / 2, false);
            }
            previousParameter = parameter;
            previousFactor = factor;
        }
    }
    return cusps;
}

req::Token token{7, 11, 13, 17};
LC_EllipseOffsetDefinition definition() {
    return {{{0, 0}, {1, 0}, 1, 0, .001, false}, 1, false, false, 1e-4};
}
RS_SplineData lineCache() {
    RS_SplineData data{1, false};
    data.controlPoints = {{-3, 0}, {3, 0}};
    data.knotslist = {0, 0, 1, 1}; data.weights = {1, 1};
    return data;
}
RS_SplineData arcCache() {
    auto data = lineCache();
    data.controlPoints = {{2, 0}, {2 * std::cos(.001), 2 * std::sin(.001)}};
    return data;
}
const result::Native& native(const result::Reply& reply, std::size_t operation) {
    REQUIRE(reply.failure == result::Failure::None);
    CHECK(reply.operation == operation);
    CHECK(reply.token.requestId == token.requestId); CHECK(reply.token.generation == token.generation);
    CHECK(reply.token.sourceRevision == token.sourceRevision); CHECK(reply.token.undoEpoch == token.undoEpoch);
    REQUIRE(std::holds_alternative<result::Native>(reply.result));
    const auto& output = std::get<result::Native>(reply.result);
    if ((operation != 0 && operation != 7) || output.refusal == LC_OffsetCurveRefusal::None ||
        output.refusal == LC_OffsetCurveRefusal::ThroughNotOnCache) {
        CHECK(output.ordinaryStatus == result::OrdinaryStatus::None); CHECK(output.ordinary.empty());
        CHECK(output.ordinaryUsage.cubicPieces == 0); CHECK(output.ordinaryUsage.outputEntities == 0);
        CHECK(output.ordinaryUsage.deepEntities == 0);
    }
    return output;
}
const trim::Result& trimmed(const result::Reply& reply, std::size_t operation) {
    REQUIRE(reply.failure == result::Failure::None); CHECK(reply.operation == operation);
    REQUIRE(std::holds_alternative<trim::Result>(reply.result));
    return std::get<trim::Result>(reply.result);
}
void work(const geo::CheckWork& a, const geo::CheckWork& b) {
    CHECK(a.cacheSpans == b.cacheSpans); CHECK(a.vertices == b.vertices);
    CHECK(a.distancePairs == b.distancePairs); CHECK(a.deepest == b.deepest);
    CHECK(a.orderingComparisons == b.orderingComparisons);
}
void certified(const result::Curve& curve) {
    const auto checked = geo::checkCache(curve.cache, curve.definition, curve.worldTolerance);
    REQUIRE(checked); REQUIRE(checked.analysis);
    CHECK(curve.closed == checked.analysis->closed());
    REQUIRE_FALSE(curve.grips.empty()); CHECK(curve.grips.size() <= 256);
    for (const auto& point : curve.grips) CHECK(geo::isFinite2D(point));
    CHECK(geo::isFinite2D(curve.midpoint));
}
}

TEST_CASE("high precision oracle samples conditioned periodic spline tangent minima",
          "[lc-offsetcurve][worker-kernel][spline-variant][periodic][rational][oracle]") {
    const std::array<std::vector<double>, 3> profiles{{
        {1e-6, 1e6, 1e-4, 1e4, 1e-2, 1e2, 1.0},
        {1e-6, 1e-4, 1e-2, 1.0, 1e2, 1e4, 1e6},
        {1e6, 1e4, 1e2, 1.0, 1e-2, 1e-4, 1e-6}
    }};
    for (std::size_t profile = 0; profile < profiles.size(); ++profile) {
        CAPTURE(profile);
        RS_Spline source{nullptr, RS_SplineData{2, false}};
        for (const RS_Vector& point : {RS_Vector{0, 0}, RS_Vector{25, -15},
                                       RS_Vector{55, -5}, RS_Vector{70, 20},
                                       RS_Vector{50, 45}, RS_Vector{15, 50},
                                       RS_Vector{-15, 25}})
            source.addControlPoint({point.x + 1000.0, point.y - 2000.0});
        source.setKnotVector({0, 0, 0, .5, 1.5, 3, 4.5, 5, 5, 5});
        REQUIRE(source.validate());
        source.setClosed(true);
        RS_SplineData data = source.getData();
        data.weights = profiles[profile];
        data.weights.insert(data.weights.end(), profiles[profile].begin(),
                            profiles[profile].begin() + data.degree);
        RS_Spline checked{nullptr, data};
        REQUIRE(checked.validate());

        SplineOracle minimumSquared = std::numeric_limits<SplineOracle>::infinity();
        SplineOracle minimumParameter{};
        const std::size_t lastControl = data.controlPoints.size() - 1;
        const SplineOracle domainStart{data.knotslist[data.degree]};
        const SplineOracle domainEnd{data.knotslist[lastControl + 1]};
        RS_Vector hullLow = data.controlPoints.front();
        RS_Vector hullHigh = hullLow;
        for (const RS_Vector& point : data.controlPoints) {
            hullLow = RS_Vector::minimum(hullLow, point);
            hullHigh = RS_Vector::maximum(hullHigh, point);
        }
        const double feature = std::hypot(hullHigh.x - hullLow.x,
                                          hullHigh.y - hullLow.y);
        const double tangentFloor = 64.0 * std::numeric_limits<double>::epsilon() * feature;
        double certifiedSpeedSquared = std::numeric_limits<double>::infinity();
        std::size_t certifiedBoxes = 0;
        bool regularityCertified = true;
        struct SpeedBox { double lo; double hi; unsigned depth; };
        for (std::size_t span = data.degree; span <= lastControl; ++span) {
            const double start = data.knotslist[span];
            const double end = data.knotslist[span + 1];
            if (!(start < end)) continue;
            LC_CurveJetBounds productHull;
            REQUIRE(checked.tryBoundJet(start, end, productHull, true));
            for (int sample = 1; sample < 32; ++sample) {
                LC_CurveJet jet;
                REQUIRE(checked.tryEvaluateJet(
                    start + (end - start) * sample / 32.0,
                    LC_CurveEvaluationSide::Interior, jet));
                const double speedSquared = jet.first.x * jet.first.x +
                                            jet.first.y * jet.first.y;
                const double cross = jet.first.x * jet.second.y -
                                     jet.first.y * jet.second.x;
                const double speedSlack = 1e-12 * std::max(1.0, std::abs(speedSquared));
                const double crossSlack = 1e-12 * std::max(1.0, std::abs(cross));
                CHECK(productHull.speedSquared().lo() <= speedSquared + speedSlack);
                CHECK(productHull.speedSquared().hi() >= speedSquared - speedSlack);
                CHECK(productHull.cross().lo() <= cross + crossSlack);
                CHECK(productHull.cross().hi() >= cross - crossSlack);
            }
            std::vector<SpeedBox> pending{{start, end, 0}};
            while (!pending.empty()) {
                const SpeedBox box = pending.back();
                pending.pop_back();
                LC_CurveJetBounds bounds;
                if (!checked.tryBoundJet(box.lo, box.hi, bounds, true)) {
                    regularityCertified = false;
                    break;
                }
                const LC_Interval speedSquared = bounds.speedSquared();
                if (speedSquared.isValid() && speedSquared.lo() >
                        std::pow(16.0 * tangentFloor / (end - start), 2)) {
                    certifiedSpeedSquared = std::min(certifiedSpeedSquared, speedSquared.lo());
                    ++certifiedBoxes;
                    continue;
                }
                const double middle = box.lo + 0.5 * (box.hi - box.lo);
                if (box.depth >= 48 || !(box.lo < middle && middle < box.hi) ||
                    certifiedBoxes + pending.size() >= 100000) {
                    regularityCertified = false;
                    break;
                }
                pending.push_back({middle, box.hi, box.depth + 1});
                pending.push_back({box.lo, middle, box.depth + 1});
            }
            if (!regularityCertified) break;
        }
        INFO("certified minimum speed squared lower bound=" << certifiedSpeedSquared
             << ", boxes=" << certifiedBoxes);
        REQUIRE(regularityCertified);
        CHECK(certifiedSpeedSquared > 0.0);
        const SplineOracle seamStep = (domainEnd - domainStart) * SplineOracle{"1e-20"};
        const auto startPoint = evaluateSplineOracle(data, domainStart);
        const auto afterStart = evaluateSplineOracle(data, domainStart + seamStep);
        const SplineOracle startDx = (afterStart.first - startPoint.first) / seamStep;
        const SplineOracle startDy = (afterStart.second - startPoint.second) / seamStep;
        const SplineOracle startSpeedSquared = startDx * startDx + startDy * startDy;
        const auto beforeEnd = evaluateSplineOracle(data, domainEnd - seamStep);
        const auto endPoint = evaluateSplineOracle(data, domainEnd);
        const SplineOracle endDx = (endPoint.first - beforeEnd.first) / seamStep;
        const SplineOracle endDy = (endPoint.second - beforeEnd.second) / seamStep;
        const SplineOracle endSpeedSquared = endDx * endDx + endDy * endDy;
        if (startSpeedSquared < minimumSquared) {
            minimumSquared = startSpeedSquared;
            minimumParameter = domainStart;
        }
        if (endSpeedSquared < minimumSquared) {
            minimumSquared = endSpeedSquared;
            minimumParameter = domainEnd;
        }
        for (std::size_t span = data.degree; span <= lastControl; ++span) {
            const SplineOracle start{data.knotslist[span]};
            const SplineOracle end{data.knotslist[span + 1]};
            if (!(start < end)) continue;
            const SplineOracle width = end - start;
            const SplineOracle step = width * SplineOracle{"1e-12"};
            for (int sample = 1; sample < 1024; ++sample) {
                const SplineOracle t = start + width * sample / 1024;
                const SplineOracle speedSquared = oracleSpeedSquared(data, t, step);
                if (speedSquared < minimumSquared) {
                    minimumSquared = speedSquared;
                    minimumParameter = t;
                }
            }
        }
        const std::array<double, 8> signedDistances{{-.25, -.125, -.0625, -.005,
                                                       .005, .0625, .125, .25}};
        for (const double signedDistance : signedDistances) {
            const SplineOracle distance{signedDistance};
            unsigned signChanges = 0;
            double minimumFactor = std::numeric_limits<double>::infinity();
            double maximumFactor = -std::numeric_limits<double>::infinity();
            for (std::size_t span = data.degree; span <= lastControl; ++span) {
                const SplineOracle start{data.knotslist[span]};
                const SplineOracle end{data.knotslist[span + 1]};
                if (!(start < end)) continue;
                const SplineOracle width = end - start;
                std::vector<SplineOracle> parameters;
                parameters.reserve(1200);
                for (int sample = 1; sample < 1024; ++sample)
                    parameters.push_back(start + width * sample / 1024);
                for (int exponent = 4; exponent <= 48; ++exponent) {
                    const SplineOracle fraction{std::ldexp(1.0, -exponent)};
                    parameters.push_back(start + width * fraction);
                    parameters.push_back(end - width * fraction);
                }
                if (start == domainStart) parameters.push_back(start);
                if (end == domainEnd) parameters.push_back(end);
                std::sort(parameters.begin(), parameters.end());
                parameters.erase(std::unique(parameters.begin(), parameters.end()),
                                 parameters.end());
                bool havePrevious = false;
                SplineOracle previous{};
                for (const SplineOracle& t : parameters) {
                    const SplineOracle step = width * SplineOracle{"1e-20"};
                    const SplineOracle factor = oracleOffsetFactor(
                        data, t, step, domainStart, domainEnd, distance,
                        t == end && end != domainEnd);
                    const double factorValue = factor.convert_to<double>();
                    minimumFactor = std::min(minimumFactor, factorValue);
                    maximumFactor = std::max(maximumFactor, factorValue);
                    if (havePrevious && factor != 0 && previous != 0 &&
                        ((factor < 0) != (previous < 0)))
                        ++signChanges;
                    previous = factor;
                    havePrevious = true;
                }
            }
            CAPTURE(profile, signedDistance, signChanges,
                    minimumFactor, maximumFactor);
            if (signedDistance < 0.0) {
                CHECK(signChanges == 0);
                CHECK(minimumFactor > 0.0);
            } else {
                CHECK(signChanges >= 2);
                CHECK(minimumFactor < 0.0);
                CHECK(maximumFactor > 0.0);
            }
        }
        REQUIRE(minimumSquared > 0);
        const double parameter = minimumParameter.convert_to<double>();
        LC_CurveJet nativeJet;
        const auto side = parameter == domainStart.convert_to<double>()
            ? LC_CurveEvaluationSide::Right
            : parameter == domainEnd.convert_to<double>()
                ? LC_CurveEvaluationSide::Left
                : LC_CurveEvaluationSide::Interior;
        REQUIRE(checked.tryEvaluateJet(parameter, side, nativeJet));
        const double oracleSpeed = sqrt(minimumSquared).convert_to<double>();
        const double nativeSpeed = nativeJet.first.magnitude();
        INFO("oracle sampled min speed=" << oracleSpeed << " at t=" << parameter);
        CHECK(oracleSpeed > 2.2e-10);
        CHECK(oracleSpeed < 2.7e-10);
        CHECK(std::abs(oracleSpeed - nativeSpeed) <=
              std::max(1e-12, oracleSpeed * 1e-5));
    }
}

TEST_CASE("one-sided spline oracle jets converge at repeated knots and periodic seams",
          "[lc-offsetcurve][worker-kernel][spline-variant][periodic][rational][oracle][knot][seam]") {
    using OracleJet = std::array<SplineOracle, 4>;
    const auto jet = [](const RS_SplineData& data, const SplineOracle& parameter,
                        const SplineOracle& step, const int side) {
        const auto point = evaluateSplineOracle(data, parameter);
        if (side > 0) {
            const auto one = evaluateSplineOracle(data, parameter + step);
            const auto two = evaluateSplineOracle(data, parameter + 2 * step);
            const auto three = evaluateSplineOracle(data, parameter + 3 * step);
            return OracleJet{(-3 * point.first + 4 * one.first - two.first) / (2 * step),
                             (-3 * point.second + 4 * one.second - two.second) / (2 * step),
                             (2 * point.first - 5 * one.first + 4 * two.first - three.first) /
                                 (step * step),
                             (2 * point.second - 5 * one.second + 4 * two.second - three.second) /
                                 (step * step)};
        }
        if (side < 0) {
            const auto one = evaluateSplineOracle(data, parameter - step);
            const auto two = evaluateSplineOracle(data, parameter - 2 * step);
            const auto three = evaluateSplineOracle(data, parameter - 3 * step);
            return OracleJet{(3 * point.first - 4 * one.first + two.first) / (2 * step),
                             (3 * point.second - 4 * one.second + two.second) / (2 * step),
                             (2 * point.first - 5 * one.first + 4 * two.first - three.first) /
                                 (step * step),
                             (2 * point.second - 5 * one.second + 4 * two.second - three.second) /
                                 (step * step)};
        }
        const auto before = evaluateSplineOracle(data, parameter - step);
        const auto after = evaluateSplineOracle(data, parameter + step);
        return OracleJet{(after.first - before.first) / (2 * step),
                         (after.second - before.second) / (2 * step),
                         (after.first - 2 * point.first + before.first) / (step * step),
                         (after.second - 2 * point.second + before.second) / (step * step)};
    };
    const auto maxDifference = [](const OracleJet& a, const OracleJet& b,
                                  const std::size_t first, const std::size_t last) {
        SplineOracle difference{};
        for (std::size_t i = first; i < last; ++i)
            difference = std::max(difference, abs(a[i] - b[i]));
        return difference;
    };

    RS_SplineData repeated{3, false};
    repeated.type = RS_SplineData::SplineType::ClampedOpen;
    repeated.controlPoints = {{-3, 0}, {-18.0 / 7.0, 6.0 / 7.0},
                              {-18.0 / 13.0, 18.0 / 13.0},
                              {18.0 / 13.0, 18.0 / 13.0},
                              {18.0 / 7.0, 6.0 / 7.0}, {3, 0}};
    repeated.knotslist = {0, 0, 0, 0, .5, .5, 1, 1, 1, 1};
    repeated.weights = {1, .875, .8125, .8125, .875, 1};
    REQUIRE(RS_Spline{nullptr, repeated}.validate());
    const SplineOracle repeatedKnot{"0.5"};
    const auto repeatedLeft = jet(repeated, repeatedKnot, SplineOracle{"1e-10"}, -1);
    const auto repeatedRight = jet(repeated, repeatedKnot, SplineOracle{"1e-10"}, 1);
    CHECK(maxDifference(repeatedLeft, repeatedRight, 0, 2) < SplineOracle{"1e-7"});
    CHECK(maxDifference(repeatedLeft, repeatedRight, 2, 4) < SplineOracle{"1e-5"});
    for (const int side : {-1, 1}) {
        const auto atKnot = side < 0 ? repeatedLeft : repeatedRight;
        SplineOracle priorFirstError = std::numeric_limits<SplineOracle>::infinity();
        SplineOracle priorSecondError = std::numeric_limits<SplineOracle>::infinity();
        for (const SplineOracle delta : {SplineOracle{"1e-4"}, SplineOracle{"5e-5"}}) {
            const SplineOracle interiorParameter = repeatedKnot + side * delta;
            const SplineOracle interiorStep = delta * SplineOracle{"1e-3"};
            const auto interior = jet(repeated, interiorParameter, interiorStep, 0);
            const SplineOracle firstError = maxDifference(atKnot, interior, 0, 2);
            const SplineOracle secondError = maxDifference(atKnot, interior, 2, 4);
            CAPTURE(side, delta, firstError, secondError);
            if (delta == SplineOracle{"1e-4"}) {
                priorFirstError = firstError;
                priorSecondError = secondError;
            } else {
                CHECK(firstError < priorFirstError * SplineOracle{"0.7"});
                if (priorSecondError > SplineOracle{"1e-20"})
                    CHECK(secondError < priorSecondError * SplineOracle{"0.7"});
                else
                    CHECK(secondError < SplineOracle{"1e-20"});
            }
        }
    }

    RS_Spline periodic{nullptr, RS_SplineData{2, false}};
    for (const RS_Vector& point : {RS_Vector{0, 0}, RS_Vector{25, -15},
                                   RS_Vector{55, -5}, RS_Vector{70, 20},
                                   RS_Vector{50, 45}, RS_Vector{15, 50},
                                   RS_Vector{-15, 25}})
        periodic.addControlPoint({point.x + 1000.0, point.y - 2000.0});
    periodic.setKnotVector({0, 0, 0, .5, 1.5, 3, 4.5, 5, 5, 5});
    REQUIRE(periodic.validate());
    periodic.setClosed(true);
    const std::array<std::vector<double>, 5> seamProfiles{{
        {0.01, 100.0, 0.1, 10.0, 0.5, 2.0, 1.0},
        {1e-6, 1e-4, 1e-2, 1.0, 1e2, 1e4, 1e6},
        {1e6, 1e4, 1e2, 1.0, 1e-2, 1e-4, 1e-6},
        {1e-6, 1e6, 1e-4, 1e4, 1e-2, 1e2, 1.0},
        {1e6, 1e-6, 1e4, 1e-4, 1e2, 1e-2, 1.0}
    }};
    for (const auto& seamWeights : seamProfiles) {
        CAPTURE(seamWeights);
        RS_SplineData wrapped = periodic.getData();
        wrapped.weights = seamWeights;
        wrapped.weights.insert(wrapped.weights.end(), seamWeights.begin(),
                               seamWeights.begin() + wrapped.degree);
        REQUIRE(RS_Spline{nullptr, wrapped}.validate());
        const std::size_t lastControl = wrapped.controlPoints.size() - 1;
        const SplineOracle start{wrapped.knotslist[wrapped.degree]};
        const SplineOracle end{wrapped.knotslist[lastControl + 1]};
        const SplineOracle seamStep{"1e-10"};
        const auto startPoint = evaluateSplineOracle(wrapped, start);
        const auto endPoint = evaluateSplineOracle(wrapped, end);
        const auto rightAtStart = jet(wrapped, start, seamStep, 1);
        const auto leftAtEnd = jet(wrapped, end, seamStep, -1);
        CAPTURE(startPoint.first, startPoint.second, endPoint.first, endPoint.second,
                maxDifference(rightAtStart, leftAtEnd, 0, 2),
                maxDifference(rightAtStart, leftAtEnd, 2, 4));
        CHECK(abs(startPoint.first - endPoint.first) < SplineOracle{"1e-10"});
        CHECK(abs(startPoint.second - endPoint.second) < SplineOracle{"1e-10"});
        CHECK(maxDifference(rightAtStart, leftAtEnd, 0, 2) < SplineOracle{"1e-3"});
        CHECK(std::isfinite(maxDifference(rightAtStart, leftAtEnd, 2, 4)
                                .convert_to<double>()));
        for (const int side : {-1, 1}) {
            const auto atSeamSide = side < 0 ? leftAtEnd : rightAtStart;
            SplineOracle priorFirstError = std::numeric_limits<SplineOracle>::infinity();
            SplineOracle priorSecondError = std::numeric_limits<SplineOracle>::infinity();
            for (const SplineOracle delta : {SplineOracle{"1e-6"}, SplineOracle{"5e-7"}}) {
                const SplineOracle interiorParameter = side < 0 ? end - delta : start + delta;
                const SplineOracle interiorStep = delta * SplineOracle{"1e-3"};
                const auto interior = jet(wrapped, interiorParameter, interiorStep, 0);
                const SplineOracle firstError = maxDifference(atSeamSide, interior, 0, 2);
                const SplineOracle secondError = maxDifference(atSeamSide, interior, 2, 4);
                CAPTURE(side, delta, firstError, secondError);
                if (delta == SplineOracle{"1e-6"}) {
                    priorFirstError = firstError;
                    priorSecondError = secondError;
                } else {
                    CHECK(firstError < priorFirstError * SplineOracle{"0.7"});
                    if (priorSecondError > SplineOracle{"1e-20"})
                        CHECK(secondError < priorSecondError * SplineOracle{"0.7"});
                    else
                        CHECK(secondError < SplineOracle{"1e-20"});
                }
            }
        }
    }
}

TEST_CASE("rational periodic derivative sign bounds agree with the high precision oracle",
          "[lc-offsetcurve][worker-kernel][periodic][rational][oracle][cusp]") {
    RS_Spline source{nullptr, RS_SplineData{2, false}};
    for (const RS_Vector& point : {RS_Vector{0, 0}, RS_Vector{25, -15},
                                   RS_Vector{55, -5}, RS_Vector{70, 20},
                                   RS_Vector{50, 45}, RS_Vector{15, 50},
                                   RS_Vector{-15, 25}})
        source.addControlPoint({point.x + 1000.0, point.y - 2000.0});
    source.setKnotVector({0, 0, 0, .5, 1.5, 3, 4.5, 5, 5, 5});
    REQUIRE(source.validate());
    source.setClosed(true);
    RS_SplineData data = source.getData();
    const std::vector<double> weights{1e-6, 1e-4, 1e-2, 1.0, 1e2, 1e4, 1e6};
    data.weights = weights;
    data.weights.insert(data.weights.end(), weights.begin(), weights.begin() + data.degree);
    RS_Spline checked{nullptr, data};
    REQUIRE(checked.validate());

    double domainStartDouble = 0.0;
    double domainEndDouble = 0.0;
    REQUIRE(checked.getParameterDomain(domainStartDouble, domainEndDouble));
    const SplineOracle domainStart{domainStartDouble};
    const SplineOracle domainEnd{domainEndDouble};
    constexpr double signedDistance = 0.005;
    std::size_t certifiedBoxes = 0;
    const std::vector<double> breaks = checked.getBreakParameters();
    REQUIRE(breaks.size() > 1);
    for (std::size_t span = 0; span + 1 < breaks.size(); ++span) {
        const double spanWidth = breaks[span + 1] - breaks[span];
        if (!(spanWidth > 0.0)) continue;
        for (int piece = 0; piece < 32; ++piece) {
            const double a = breaks[span] + spanWidth * piece / 32.0;
            const double b = breaks[span] + spanWidth * (piece + 1) / 32.0;
            LC_CurveJetBounds bounds;
            REQUIRE(checked.tryBoundJet(a, b, bounds, true));
            CHECK_FALSE(bounds.rationalCuspNoRootCertified);
            const LC_Interval derivativeSign =
                bounds.offsetFactorDerivativeSignNumerator(signedDistance);
            if (!derivativeSign.excludesZero()) continue;
            ++certifiedBoxes;

            const SplineOracle finiteDifferenceStep{(b - a) * SplineOracle{"1e-5"}};
            const SplineOracle splineDerivativeStep{spanWidth * SplineOracle{"1e-20"}};
            for (const int sampleNumerator : {1, 3, 5, 7}) {
                const SplineOracle t{a + (b - a) * sampleNumerator / 8.0};
                const SplineOracle before = oracleOffsetFactorNumerator(
                    data, t - finiteDifferenceStep, splineDerivativeStep, domainStart,
                    domainEnd, SplineOracle{signedDistance});
                const SplineOracle after = oracleOffsetFactorNumerator(
                    data, t + finiteDifferenceStep, splineDerivativeStep, domainStart,
                    domainEnd, SplineOracle{signedDistance});
                const SplineOracle oracleDerivative =
                    (after - before) / (2 * finiteDifferenceStep);
                CAPTURE(span, piece, sampleNumerator, derivativeSign.lo(), derivativeSign.hi());
                CHECK((derivativeSign.lo() > 0.0) == (oracleDerivative > 0));
                CHECK(oracleDerivative != 0);
            }
        }
    }
    CHECK(certifiedBoxes > 0);
}

TEST_CASE("rational cusp polynomial no-root certificates agree with oracle samples",
          "[lc-offsetcurve][worker-kernel][periodic][rational][oracle][cusp]") {
    RS_Spline source{nullptr, RS_SplineData{2, false}};
    for (const RS_Vector& point : {RS_Vector{0, 0}, RS_Vector{25, -15},
                                   RS_Vector{55, -5}, RS_Vector{70, 20},
                                   RS_Vector{50, 45}, RS_Vector{15, 50},
                                   RS_Vector{-15, 25}})
        source.addControlPoint({point.x + 1000.0, point.y - 2000.0});
    source.setKnotVector({0, 0, 0, .5, 1.5, 3, 4.5, 5, 5, 5});
    REQUIRE(source.validate());
    source.setClosed(true);
    RS_SplineData data = source.getData();
    const std::vector<double> weights{1e-6, 1e-4, 1e-2, 1.0, 1e2, 1e4, 1e6};
    data.weights = weights;
    data.weights.insert(data.weights.end(), weights.begin(), weights.begin() + data.degree);
    RS_Spline checked{nullptr, data};
    REQUIRE(checked.validate());

    double domainStartValue = 0.0;
    double domainEndValue = 0.0;
    REQUIRE(checked.getParameterDomain(domainStartValue, domainEndValue));
    const SplineOracle domainStart{domainStartValue};
    const SplineOracle domainEnd{domainEndValue};
    const std::vector<double> breaks = checked.getBreakParameters();
    constexpr std::array<double, 8> distances{{-0.25, -0.125, -0.0625, -0.005,
                                               0.005, 0.0625, 0.125, 0.25}};
    for (const double signedDistance : distances) {
        std::size_t excludedBoxes = 0;
        std::size_t tightenedBoxes = 0;
        std::size_t signedTurnVariationOneBoxes = 0;
        for (std::size_t span = 0; span + 1 < breaks.size(); ++span) {
            const double spanWidth = breaks[span + 1] - breaks[span];
            if (!(spanWidth > 0.0)) continue;
            for (int piece = 0; piece < 32; ++piece) {
                const double a = breaks[span] + spanWidth * piece / 32.0;
                const double b = breaks[span] + spanWidth * (piece + 1) / 32.0;
                LC_CurveJetBounds bounds;
                REQUIRE(checked.tryBoundJetForCusp(a, b, bounds, signedDistance));
                const bool signedTurnPositive =
                    (LC_Interval::point(signedDistance) * bounds.cross()).isPositive();
                if (signedTurnPositive && bounds.rationalCuspRootVariationUpperBound == 1) {
                    ++signedTurnVariationOneBoxes;
                    const double t0 = a + (b - a) * 1e-8;
                    const double t1 = b - (b - a) * 1e-8;
                    const SplineOracle derivativeStep{spanWidth * SplineOracle{"1e-20"}};
                    const SplineOracle distance{signedDistance};
                    const SplineOracle g0 = oracleOffsetFactorNumerator(
                        data, SplineOracle{t0}, derivativeStep, domainStart, domainEnd, distance);
                    const SplineOracle g1 = oracleOffsetFactorNumerator(
                        data, SplineOracle{t1}, derivativeStep, domainStart, domainEnd, distance);
                    CAPTURE(span, piece, signedDistance);
                    CHECK((g0 < 0) != (g1 < 0));
                }
                if (!bounds.rationalCuspNoRootCertified) continue;
                ++excludedBoxes;
                if (bounds.offsetFactorNumerator(signedDistance).containsZero()) {
                    ++tightenedBoxes;
                }

                const SplineOracle derivativeStep{spanWidth * SplineOracle{"1e-20"}};
                const SplineOracle distance{signedDistance};
                for (const int sampleNumerator : {0, 1, 3, 5, 7, 8}) {
                    const SplineOracle t{a + (b - a) * sampleNumerator / 8.0};
                    const SplineOracle g = oracleOffsetFactorNumerator(
                        data, t, derivativeStep, domainStart, domainEnd, distance);
                    CAPTURE(span, piece, sampleNumerator, signedDistance);
                    CHECK(g != 0);
                }
            }
        }
        INFO("signed distance=" << signedDistance << ", excluded boxes=" << excludedBoxes);
        CHECK(excludedBoxes > 0);
        if (signedDistance > 0.0) {
            CHECK(tightenedBoxes > 0);
        }
        if (signedDistance > 0.0) CHECK(signedTurnVariationOneBoxes > 0);
    }
}

TEST_CASE("cubic rational cusp no-root certificate uses degree-aligned products",
          "[lc-offsetcurve][worker-kernel][rational][oracle][cusp][cubic]") {
    RS_Spline source{nullptr, RS_SplineData{3, false}};
    for (const RS_Vector& point : {RS_Vector{0, 0}, RS_Vector{10, 0},
                                   RS_Vector{20, 10}, RS_Vector{30, 10}}) {
        source.addControlPoint({point.x, point.y});
    }
    source.setKnotVector({0, 0, 0, 0, 1, 1, 1, 1});
    REQUIRE(source.validate());
    RS_SplineData data = source.getData();
    data.weights = {1.0, 0.75, 1.25, 1.0};
    RS_Spline checked{nullptr, data};
    REQUIRE(checked.validate());

    double domainStartValue = 0.0;
    double domainEndValue = 0.0;
    REQUIRE(checked.getParameterDomain(domainStartValue, domainEndValue));
    const SplineOracle domainStart{domainStartValue};
    const SplineOracle domainEnd{domainEndValue};
    std::size_t certifiedBoxes = 0;
    for (const double signedDistance : {-0.25, 0.25}) {
        for (int piece = 0; piece < 32; ++piece) {
            const double a = domainStartValue +
                             (domainEndValue - domainStartValue) * piece / 32.0;
            const double b = domainStartValue +
                             (domainEndValue - domainStartValue) * (piece + 1) / 32.0;
            LC_CurveJetBounds bounds;
            REQUIRE(checked.tryBoundJetForCusp(a, b, bounds, signedDistance));
            if (!bounds.rationalCuspNoRootCertified) continue;
            ++certifiedBoxes;
            const SplineOracle derivativeStep{SplineOracle{"1e-20"}};
            const SplineOracle distance{signedDistance};
            for (const int sampleNumerator : {0, 1, 4, 7, 8}) {
                const SplineOracle t{a + (b - a) * sampleNumerator / 8.0};
                const SplineOracle g = oracleOffsetFactorNumerator(
                    data, t, derivativeStep, domainStart, domainEnd, distance);
                CAPTURE(piece, sampleNumerator, signedDistance);
                CHECK(g != 0);
            }
        }
    }
    CHECK(certifiedBoxes > 0);
}

TEST_CASE("offset child kernel generates owned checked caches with actual stage counters",
          "[lc-offsetcurve][worker-kernel]") {
    for (const bool inward : {false, true}) {
        auto def = definition(); def.distance = inward ? .25 : 1.; def.towardCentre = inward;
        req::Request input{token, req::Generate{def, {}, {}}};
        LC_OffsetWorkerKernel::Trace trace;
        const auto reply = LC_OffsetWorkerKernel::run(input, &trace);
        const auto& output = native(reply, 0);
        REQUIRE(output); REQUIRE(output.curve); CHECK(output.changed);
        CHECK(output.engineRuns == 1); REQUIRE(output.engineStatus);
        CHECK(*output.engineStatus == LC_CurveOffsetStatus::Ok);
        REQUIRE(output.checkReason); CHECK(*output.checkReason == LC_OffsetCheckResult::Reason::None);
        CHECK(trace.nativePrepareEntered); CHECK(trace.checkEntered); CHECK_FALSE(trace.trimEntered);
        CHECK(trace.computationCompleted); work(trace.checkWork, output.checkWork);
        work(trace.throughWork, {}); certified(*output.curve);
        CHECK(output.curve->definition.distance == def.distance);
        CHECK(output.curve->definition.towardCentre == inward);
        CHECK(input.token.requestId == token.requestId);
    }
}

TEST_CASE("offset child Through checks use the remaining C allowance and retain refused work",
          "[lc-offsetcurve][worker-kernel][through]") {
    req::Generate op{definition(), {}, RS_Vector{2, 0}};
    const auto good = LC_OffsetWorkerKernel::run({token, op});
    const auto& accepted = native(good, 0);
    REQUIRE(accepted.curve); CHECK(accepted.refusal == LC_OffsetCurveRefusal::None);
    work(accepted.throughWork, {0, 1, 1});
    const auto c = accepted.checkWork;
    op.limits.checkLimits.maxVertices = c.vertices;
    LC_OffsetWorkerKernel::Trace trace;
    const auto refused = LC_OffsetWorkerKernel::run({token, op}, &trace);
    const auto& limited = native(refused, 0);
    CHECK(limited.refusal == LC_OffsetCurveRefusal::ThroughNotOnCache);
    CHECK_FALSE(limited.changed); CHECK_FALSE(limited.curve);
    work(limited.checkWork, c); work(limited.throughWork, {});
    CHECK(trace.computationCompleted);
    op.limits = {}; op.throughPoint = RS_Vector{50, 50};
    const auto far = LC_OffsetWorkerKernel::run({token, op}, &trace);
    const auto& missed = native(far, 0);
    CHECK(missed.refusal == LC_OffsetCurveRefusal::ThroughNotOnCache); CHECK_FALSE(missed.curve);
    CHECK(missed.throughWork.vertices > 0); CHECK(missed.throughWork.distancePairs > 0);
    work(trace.throughWork, missed.throughWork);
}

TEST_CASE("offset child transports checked ordinary lines from the same refused engine invocation",
          "[lc-offsetcurve][worker-kernel][ordinary]") {
    for (bool reversed : {false, true}) {
        auto def = definition(); def.cacheReversed = reversed; def.basis.center = {1e7, 0};
        const auto baseline = LC_OffsetWorkerKernel::run({token, req::Generate{def, {}, {}}});
        const auto& accepted = native(baseline, 0); REQUIRE(accepted.curve);
        for (bool display : {false, true}) {
            req::Generate op{def, {}, {}};
            if (display) op.limits.outputBudget.maxDeepEntities = 31;
            else op.limits.checkLimits.maxVertices = 1;
            // A native C refusal occurs before any Through check, even if requested.
            if (!display) op.throughPoint = RS_Vector{50, 50};
            const req::Request input{token, op};
            LC_OffsetWorkerKernel::Trace trace;
            const auto reply = LC_OffsetWorkerKernel::run(input, &trace);
            const auto& output = native(reply, 0);
            CHECK(output.refusal == (display ? LC_OffsetCurveRefusal::NotCacheRepresentable : LC_OffsetCurveRefusal::CheckFailed));
            CHECK(output.engineStatus == LC_CurveOffsetStatus::Ok); CHECK(output.engineRuns == 1);
            CHECK_FALSE(output.changed); CHECK_FALSE(output.curve);
            REQUIRE(output.ordinaryStatus == result::OrdinaryStatus::Complete);
            REQUIRE(output.ordinary.size() == 1); REQUIRE(std::holds_alternative<req::Segment>(output.ordinary[0]));
            const auto& line = std::get<req::Segment>(output.ordinary[0]);
            auto data = accepted.curve->cache; data.controlPoints = {line.start, line.end};
            CHECK(req::Values::encodeCache(data) == req::Values::encodeCache(accepted.curve->cache));
            REQUIRE(geo::checkCache(data, accepted.curve->definition, accepted.curve->worldTolerance));
            CHECK(output.ordinaryUsage.cubicPieces == 1); CHECK(output.ordinaryUsage.outputEntities == 1);
            CHECK(output.ordinaryUsage.deepEntities == 1);
            CHECK(output.sampleUnits == accepted.sampleUnits);
            CHECK(output.attemptedOutputUsage.cubicPieces == accepted.attemptedOutputUsage.cubicPieces);
            CHECK(trace.nativePrepareEntered); CHECK(trace.checkEntered); CHECK(trace.computationCompleted);
            CHECK_FALSE(trace.throughEntered); work(output.throughWork, {}); work(trace.checkWork, output.checkWork);
            if (display) work(output.checkWork, accepted.checkWork);
            else { CHECK(output.checkWork.vertices <= 1); CHECK(output.checkReason == LC_OffsetCheckResult::Reason::LimitExceeded); }
            const auto encoded = result::encode(reply, input); REQUIRE(encoded);
            const auto decoded = result::decode(*encoded, input); REQUIRE(decoded);
            const auto& transported = native(*decoded, 0);
            CHECK(transported.ordinaryStatus == result::OrdinaryStatus::Complete);
            REQUIRE(transported.ordinary.size() == 1);
            CHECK(req::Values::encodeCutter(transported.ordinary[0]) == req::Values::encodeCutter(output.ordinary[0]));
        }
    }
}

TEST_CASE("offset child transports real empty and several-piece ordinary producer results",
          "[lc-offsetcurve][worker-kernel][ordinary]") {
    const double pi = std::acos(-1.0);
    for (bool several : {false, true}) {
        auto def = definition(); def.towardCentre = true;
        def.basis = several ? RS_EllipseData{{0, 0}, {100, 0}, .05, pi / 12, 7 * pi / 4, false} :
            RS_EllipseData{{0, 0}, {5, 0}, 1};
        def.distance = several ? 4 : 5; def.relTolerance = several ? 1e-5 : 1e-4;
        const req::Request input{token, req::Generate{def, {}, {}}};
        const auto reply = LC_OffsetWorkerKernel::run(input);
        const auto& output = native(reply, 0);
        CHECK(output.refusal == (several ? LC_OffsetCurveRefusal::SeveralPieces : LC_OffsetCurveRefusal::NothingLeft));
        CHECK(output.engineStatus == LC_CurveOffsetStatus::Ok); CHECK(output.engineRuns == 1);
        CHECK_FALSE(output.curve); CHECK_FALSE(output.changed); CHECK_FALSE(output.checkReason);
        REQUIRE(output.ordinaryStatus == result::OrdinaryStatus::Complete);
        CHECK(output.ordinary.empty() == !several);
        CHECK(output.ordinaryUsage.outputEntities == output.ordinary.size());
        std::size_t spans = 0;
        for (const auto& value : output.ordinary) {
            if (std::holds_alternative<req::Segment>(value)) ++spans;
            else {
                REQUIRE(std::holds_alternative<RS_SplineData>(value));
                const auto& data = std::get<RS_SplineData>(value);
                REQUIRE((data.degree == 1 || data.degree == 3));
                spans += (data.controlPoints.size() - 1) / data.degree;
            }
        }
        CHECK(output.ordinaryUsage.cubicPieces == output.attemptedOutputUsage.cubicPieces);
        CHECK(spans <= makeDefaultOffsetSourceBudget().maxCubicPieces);
        CHECK(spans == output.ordinaryUsage.cubicPieces + (several ? 1 : 0));
        if (several) CHECK(output.ordinary.size() > 1);
        else CHECK(output.ordinaryUsage.deepEntities == 0);
        const auto encoded = result::encode(reply, input); REQUIRE(encoded);
        REQUIRE(result::decode(*encoded, input));
    }
}

TEST_CASE("offset worker op7 generates and transports a typed source without cache authority",
          "[lc-offsetcurve][worker-kernel][source-generation]") {
    RS_SplineData spline{2, false};
    spline.controlPoints = {{0, 0}, {1, 2}, {3, 0}};
    spline.knotslist = {0, 0, 0, 1, 1, 1};
    spline.weights = {1, .7, 1};
    LC_SplinePointsData points{false, false};
    points.useControlPoints = true;
    points.controlPoints = {{0, 0}, {1, 1}, {2, 0}};
    LC_SplinePointsData fitPoints{false, false};
    fitPoints.splinePoints = {{0, 0}, {1, 2}, {2, 0}};
    RS_SplineData degreeOne{1, false};
    degreeOne.controlPoints = {{0, 0}, {1, 1}, {2, 0}};
    degreeOne.knotslist = {0, 0, .5, 1, 1};
    RS_SplineData cubic{3, false};
    cubic.controlPoints = {{0, 0}, {1, 2}, {2, 2}, {3, 0}};
    cubic.knotslist = {0, 0, 0, 0, 1, 1, 1, 1};
    RS_Spline periodic{nullptr, RS_SplineData{3, false}};
    for (const RS_Vector& point : {RS_Vector{0, 0}, RS_Vector{40, -10}, RS_Vector{60, 30},
                                   RS_Vector{20, 50}, RS_Vector{-15, 25}})
        periodic.addControlPoint(point);
    periodic.setClosed(true);
    auto wrapped = periodic.getData();
    wrapped.weights.clear();
    const double pi = std::acos(-1.0);
    LC_OffsetPolylineData closedArcPolyline{{2, 0}, {2, 0}, true, {
        RS_ArcData{{0, 0}, 2, 0, pi / 2, false},
        RS_ArcData{{0, 0}, 2, pi / 2, pi, false},
        RS_ArcData{{0, 0}, 2, pi, 3 * pi / 2, false},
        RS_ArcData{{0, 0}, 2, 3 * pi / 2, 2 * pi, false}}};
    const std::array<LC_OffsetSourceGeometry, 15> sources{
        LC_OffsetSourceGeometry{RS_EllipseData{{0, 0}, {5, 0}, .5, 0, 1.5, false}},
        LC_OffsetSourceGeometry{spline},
        LC_OffsetSourceGeometry{points},
        LC_OffsetSourceGeometry{fitPoints},
        LC_OffsetSourceGeometry{degreeOne},
        LC_OffsetSourceGeometry{cubic},
        LC_OffsetSourceGeometry{LC_ParabolaData{{RS_Vector{0, 1}, RS_Vector{1, 0}, RS_Vector{2, 1}}}},
        LC_OffsetSourceGeometry{LC_HyperbolaData{{0, 0}, {2, 0}, .5, -.5, .5}},
        LC_OffsetSourceGeometry{RS_LineData{{0, 0}, {2, 0}}},
        LC_OffsetSourceGeometry{RS_ArcData{{0, 0}, 2, .25, 1.5, false}},
        LC_OffsetSourceGeometry{RS_CircleData{{0, 0}, 2}},
        LC_OffsetSourceGeometry{LC_OffsetPolylineData{{0, 0}, {2, 2}, false,
            {RS_ArcData{{1, 0}, 1, M_PI, 0, false}, RS_LineData{{2, 0}, {2, 2}}}}},
        LC_OffsetSourceGeometry{LC_OffsetPolylineData{{0, 0}, {0, 0}, true,
            {RS_LineData{{0, 0}, {2, 0}}, RS_LineData{{2, 0}, {2, 2}},
             RS_LineData{{2, 2}, {0, 2}}, RS_LineData{{0, 2}, {0, 0}}}}},
        LC_OffsetSourceGeometry{wrapped},
        LC_OffsetSourceGeometry{closedArcPolyline}
    };
    for (std::size_t i = 0; i < sources.size(); ++i) {
        CAPTURE(i);
        REQUIRE(sources[i].isWellFormed());
        if (i == 4) {
            CHECK(std::get<RS_SplineData>(sources[i].value).weights.empty());
            auto materialized = sources[i].makeEntity();
            const auto* splineEntity = dynamic_cast<const RS_Spline*>(materialized.get());
            REQUIRE(splineEntity);
            CHECK(splineEntity->getData().weights == std::vector<double>{1.0, 1.0, 1.0});
        }
        const LC_OffsetSourceDefinition definition{sources[i], .25, 1e-4, false};
        const req::Request input{token, req::GenerateSource{definition, {}}};
        LC_OffsetWorkerKernel::Trace trace;
        const auto reply = LC_OffsetWorkerKernel::run(input, &trace);
        const auto& output = native(reply, 7);
        CHECK(output.engineRuns == 1);
        REQUIRE(output.engineStatus);
        CHECK(*output.engineStatus == LC_CurveOffsetStatus::Ok);
        CHECK_FALSE(output.curve);
        CHECK_FALSE(output.changed);
        CHECK(trace.nativePrepareEntered);
        CHECK(trace.computationCompleted);
        REQUIRE(output.ordinaryStatus == result::OrdinaryStatus::Complete);
        REQUIRE_FALSE(output.ordinary.empty());
        CHECK(output.ordinaryUsage.outputEntities == output.ordinary.size());
        CHECK(output.refusal == LC_OffsetCurveRefusal::NotCacheRepresentable);

        const auto wire = result::encode(reply, input);
        REQUIRE(wire);
        const auto decoded = result::decode(*wire, input);
        REQUIRE(decoded);
        const auto& transported = native(*decoded, 7);
        CHECK_FALSE(transported.curve);
        CHECK(transported.ordinary.size() == output.ordinary.size());
        CHECK(transported.ordinaryUsage.outputEntities == output.ordinaryUsage.outputEntities);

        RS_SplineData candidate;
        bool cacheCandidateRepresentable = output.ordinary.size() == 1;
        if (cacheCandidateRepresentable) {
            if (const auto* splineOutput = std::get_if<RS_SplineData>(&output.ordinary.front())) {
                candidate = *splineOutput;
            } else if (const auto* segment = std::get_if<req::Segment>(&output.ordinary.front())) {
                candidate.degree = 1;
                candidate.controlPoints = {segment->start, segment->end};
                candidate.knotslist = {0, 0, 1, 1};
                candidate.weights = {1, 1};
            } else {
                cacheCandidateRepresentable = false;
            }
        }
        const req::Request validationInput{token,
            req::ValidateSourceCache{candidate, definition, {}}};
        const auto validationReply = LC_OffsetWorkerKernel::run(validationInput);
        const auto& validation = native(validationReply, 8);
        if (cacheCandidateRepresentable) {
            CHECK(validation.refusal == LC_OffsetCurveRefusal::None);
            CHECK(validation.changed);
        } else {
            CHECK(validation.refusal == LC_OffsetCurveRefusal::NotCacheRepresentable);
            CHECK_FALSE(validation.changed);
        }
        CHECK_FALSE(validation.curve);
        CHECK(validation.ordinary.empty());
        const auto validationWire = result::encode(validationReply, validationInput);
        REQUIRE(validationWire);
        const auto validationRoundTrip = result::decode(*validationWire, validationInput);
        REQUIRE(validationRoundTrip);
        const auto& transportedValidation = native(*validationRoundTrip, 8);
        CHECK(transportedValidation.refusal == validation.refusal);
        CHECK(transportedValidation.changed == validation.changed);
        if (i == 4) {
            auto equivalent = std::get<RS_SplineData>(sources[i].value);
            equivalent.weights.assign(equivalent.controlPoints.size(), 1.0);
            const req::Request explicitWeightsInput{token, req::GenerateSource{
                LC_OffsetSourceDefinition{LC_OffsetSourceGeometry{equivalent}, .25, 1e-4, false}, {}}};
            const auto explicitWeightsReply = LC_OffsetWorkerKernel::run(explicitWeightsInput);
            const auto explicitWeightsWire = result::encode(explicitWeightsReply, explicitWeightsInput);
            REQUIRE(explicitWeightsWire);
            CHECK(*wire == *explicitWeightsWire);
        }
    }
}

TEST_CASE("offset worker refuses tangent-constrained fit interpolation for generation and cache validation",
          "[lc-offsetcurve][worker-kernel][source-generation][fit-points]") {
    LC_SplinePointsData constrained{false, false};
    constrained.splinePoints = {{0, 0}, {1, 2}, {3, 1.5}, {4, 0}};
    constrained.fitTangentsConstrained = true;
    constrained.fitTolerance = 1e-10;
    const LC_OffsetSourceDefinition definition{
        LC_OffsetSourceGeometry{constrained}, 0.5, 1e-4, false};

    const req::Request generate{token, req::GenerateSource{definition, {}}};
    LC_OffsetWorkerKernel::Trace generateTrace;
    const auto generatedReply = LC_OffsetWorkerKernel::run(generate, &generateTrace);
    const auto& generated = native(generatedReply, 7);
    CHECK(generated.refusal == LC_OffsetCurveRefusal::Engine);
    CHECK(generated.engineRuns == 1);
    CHECK(generated.engineStatus == LC_CurveOffsetStatus::InvalidSource);
    CHECK(generated.ordinaryStatus == result::OrdinaryStatus::None);
    CHECK(generated.ordinary.empty());
    CHECK_FALSE(generated.changed);
    CHECK_FALSE(generated.curve);
    CHECK(generateTrace.nativePrepareEntered);
    CHECK(generateTrace.computationCompleted);

    // This is the cache the unconstrained display approximation would produce.
    // It must not certify the source whose exact interpolation uses tangents.
    LC_SplinePointsData displayApproximation = constrained;
    displayApproximation.fitTangentsConstrained = false;
    auto displaySource = LC_OffsetSourceGeometry{displayApproximation}.makeEntity();
    REQUIRE(displaySource);
    const auto options = LC_CurveOffset::makeDirectOptions(*displaySource, 0.5, 1e-4);
    const auto branches = LC_CurveOffset::buildDirectBranches(*displaySource,
        LC_CurveOffset::makeSideRequest(LC_CurveOffsetSide::Left, 0.5), options,
        LC_CurveOffset::makeDirectSourceBudget());
    REQUIRE(branches.status == LC_CurveOffsetStatus::Ok);
    auto materialized = LC_CurveOffset::materializeBranches(*displaySource, branches, options,
        LC_CurveOffset::makeDirectSourceBudget());
    REQUIRE(materialized.status == LC_CurveOffsetStatus::Ok);
    REQUIRE(materialized.entities.size() == 1);
    const auto* displayCache = dynamic_cast<const RS_Spline*>(materialized.entities.front().get());
    REQUIRE(displayCache);

    const req::Request validate{token, req::ValidateSourceCache{
        displayCache->getData(), definition, {}}};
    LC_OffsetWorkerKernel::Trace validateTrace;
    const auto validatedReply = LC_OffsetWorkerKernel::run(validate, &validateTrace);
    const auto& validated = native(validatedReply, 8);
    CHECK(validated.refusal == LC_OffsetCurveRefusal::NotCacheRepresentable);
    CHECK(validated.engineRuns == 1);
    CHECK(validated.engineStatus == LC_CurveOffsetStatus::InvalidSource);
    CHECK_FALSE(validated.changed);
    CHECK_FALSE(validated.curve);
    CHECK(validateTrace.nativePrepareEntered);
    CHECK(validateTrace.computationCompleted);
}

TEST_CASE("offset worker admits cubic tangent sources only when metadata matches the curve",
          "[lc-offsetcurve][worker-kernel][source-generation][fit-points][tangents]") {
    RS_Spline spline{nullptr, RS_SplineData{3, false}};
    REQUIRE(spline.setFitPointsWithTangents(
        {{0, 0}, {1, 0.4}, {3, 0.4}, {4, 0}}, {1, 1}, {1, -1}));
    REQUIRE(spline.fitTangentsMatchCurve());
    const LC_OffsetSourceDefinition definition{
        LC_OffsetSourceGeometry{spline.getData()}, .05, 1e-4, false};
    const req::Request request{token, req::GenerateSource{definition, {}}};
    const auto reply = LC_OffsetWorkerKernel::run(request);
    const auto& generated = native(reply, 7);
    REQUIRE(generated.engineStatus == LC_CurveOffsetStatus::Ok);
    REQUIRE(generated.ordinaryStatus == result::OrdinaryStatus::Complete);
    REQUIRE(generated.ordinary.size() == 1);
    const auto wire = result::encode(reply, request);
    REQUIRE(wire);
    CHECK(result::decode(*wire, request));

    const auto& cache = std::get<RS_SplineData>(generated.ordinary.front());
    const req::Request validation{token,
        req::ValidateSourceCache{cache, definition, {}}};
    const auto validationReply = LC_OffsetWorkerKernel::run(validation);
    const auto& checked = native(validationReply, 8);
    CHECK(checked.refusal == LC_OffsetCurveRefusal::None);
    CHECK(checked.changed);
    CHECK(checked.engineStatus == LC_CurveOffsetStatus::Ok);

    auto inconsistent = spline.getData();
    inconsistent.fitPoints[1].y += 1.0;
    const LC_OffsetSourceDefinition refusedDefinition{
        LC_OffsetSourceGeometry{inconsistent}, .05, 1e-4, false};
    const req::Request refusedRequest{token,
        req::GenerateSource{refusedDefinition, {}}};
    const auto refusedReply = LC_OffsetWorkerKernel::run(refusedRequest);
    const auto& refused = native(refusedReply, 7);
    CHECK(refused.engineStatus == LC_CurveOffsetStatus::InvalidSource);
    CHECK(refused.ordinaryStatus == result::OrdinaryStatus::None);
    CHECK(refused.ordinary.empty());
    CHECK_FALSE(refused.changed);
}

TEST_CASE("offset worker produces closed stationary spline-point profiles",
          "[lc-offsetcurve][worker-kernel][source-generation][spline-points][closed][stationary]") {
    const std::array<std::vector<RS_Vector>, 2> profiles{{
        {{0, 0}, {1, 1}, {1, 1}, {1, 1}, {2, 0}, {-1, -1}},
        {{0, 0}, {0, 0}, {10, 0}, {13, 8}, {4, 13}, {0, 0}},
    }};
    for (std::size_t profile = 0; profile < profiles.size(); ++profile) {
        LC_SplinePointsData data{true, false};
        data.useControlPoints = true;
        data.controlPoints = profiles[profile];
        LC_SplinePoints source{nullptr, data};
        REQUIRE(source.isClosed());

        LC_CurveJet jet;
        const double pickParameter = profile == 0 ? 0.5 : 2.5;
        REQUIRE(source.tryEvaluateJet(pickParameter, LC_CurveEvaluationSide::Interior, jet));
        REQUIRE(jet.first.magnitude() > 0.0);
        const RS_Vector leftNormal{-jet.first.y / jet.first.magnitude(),
                                   jet.first.x / jet.first.magnitude()};

        for (const double sign : {1.0, -1.0}) {
            CAPTURE(profile, sign);
            const auto trimmed = LC_CurveOffset::createEntities(
                source,
                LC_CurveOffset::makeSideRequest(sign > 0.0 ? LC_CurveOffsetSide::Left
                                                             : LC_CurveOffsetSide::Right,
                                                0.25),
                LC_CurveOffset::makeOffsetOptions(source, 0.25),
                LC_CurveOffset::makeDirectSourceBudget());
            CAPTURE(trimmed.status, trimmed.entities.size());
            req::Produce operation;
            operation.source = LC_OffsetSourceGeometry{data};
            operation.coordinate = jet.point + leftNormal * (sign * 0.5);
            operation.distance = 0.25;
            operation.context.topLevel = true;
            const auto reply = LC_OffsetWorkerKernel::run({token, operation});
            const auto& produced = std::get<result::Produced>(reply.result);
            CAPTURE(produced.source.status, produced.source.engineStatus,
                    produced.source.detail, produced.copyOutputCounts.size());
            CHECK(produced.source.status == LC_OffsetSourceStatus::Succeeded);
            CHECK(produced.source.engineStatus == LC_CurveOffsetStatus::Ok);
            CHECK(produced.outputs.size() == 1);
        }
    }
}

TEST_CASE("offset worker handles a rotated 100-to-1 elliptic arc past its cusp radius",
          "[lc-offsetcurve][worker-kernel][source-generation][ellipse][cusp]") {
    const double angle = M_PI / 6.0;
    const RS_EllipseData ellipse{{100.0, -100.0}, {100.0 * std::cos(angle), 100.0 * std::sin(angle)}, 1e-2, -0.2, 0.2, false};
    const LC_OffsetSourceDefinition definition{LC_OffsetSourceGeometry{ellipse}, 1.1e-2, 1e-4, false};
    const req::Request request{token, req::GenerateSource{definition, {}}};
    const auto encodedRequest = req::encode(request);
    REQUIRE(encodedRequest);
    const auto decodedRequest = req::decode(*encodedRequest);
    REQUIRE(decodedRequest);
    LC_OffsetWorkerKernel::Trace trace;
    const auto reply = LC_OffsetWorkerKernel::run(*decodedRequest, &trace);
    const auto& generated = native(reply, 7);
    CAPTURE(generated.engineStatus, generated.sampleUnits, generated.attemptedOutputUsage.cubicPieces,
            generated.attemptedOutputUsage.outputEntities, generated.attemptedOutputUsage.deepEntities,
            static_cast<int>(trace.nativePrepareEntered), static_cast<int>(trace.computationCompleted));
    REQUIRE(generated.engineStatus);
    CHECK(*generated.engineStatus == LC_CurveOffsetStatus::Ok);
    CHECK(generated.ordinaryStatus == result::OrdinaryStatus::Complete);
}

TEST_CASE("offset worker handles a rotated 10-to-1 elliptic arc past its cusp radius",
          "[lc-offsetcurve][worker-kernel][source-generation][ellipse][cusp]") {
    const double angle = M_PI / 6.0;
    const RS_EllipseData ellipse{{100.0, -100.0}, {100.0 * std::cos(angle), 100.0 * std::sin(angle)}, 1e-1, -0.2, 0.2, false};
    const LC_OffsetSourceDefinition definition{LC_OffsetSourceGeometry{ellipse}, 1.1, 1e-4, false};
    const req::Request request{token, req::GenerateSource{definition, {}}};
    const auto encodedRequest = req::encode(request);
    REQUIRE(encodedRequest);
    const auto decodedRequest = req::decode(*encodedRequest);
    REQUIRE(decodedRequest);
    const auto reply = LC_OffsetWorkerKernel::run(*decodedRequest);
    const auto& generated = native(reply, 7);
    REQUIRE(generated.engineStatus);
    CHECK(*generated.engineStatus == LC_CurveOffsetStatus::Ok);
    CHECK(generated.ordinaryStatus == result::OrdinaryStatus::Complete);
}

TEST_CASE("offset worker admits consistent rational tangent cubic sources and validates their cache",
          "[lc-offsetcurve][worker-kernel][source-generation][fit-points][tangents][rational]") {
    RS_SplineData data{3, false};
    data.controlPoints = {{0, 0}, {2, 3}, {6, 4}, {9, 0}};
    data.knotslist = {0, 0, 0, 0, 1, 1, 1, 1};
    data.weights = {1.0, 0.75, 0.8, 1.0};
    data.fitTangentsConstrained = true;
    data.fitTolerance = 1e-2;
    data.fitStartTangent = {2, 3};
    data.fitEndTangent = {3, -4};
    RS_Spline evaluator{nullptr, data};
    for (const double parameter : {0.0, 0.2, 0.5, 0.8, 1.0}) {
        LC_CurveJet jet;
        REQUIRE(evaluator.tryEvaluateJet(parameter,
            parameter == 1.0 ? LC_CurveEvaluationSide::Left
                             : LC_CurveEvaluationSide::Right, jet));
        data.fitPoints.push_back(jet.point);
    }
    RS_Spline spline{nullptr, data};
    REQUIRE(spline.fitTangentsMatchCurve());

    const LC_OffsetSourceDefinition definition{
        LC_OffsetSourceGeometry{data}, .05, 1e-4, false};
    const req::Request request{token, req::GenerateSource{definition, {}}};
    const auto reply = LC_OffsetWorkerKernel::run(request);
    const auto& generated = native(reply, 7);
    REQUIRE(generated.engineStatus == LC_CurveOffsetStatus::Ok);
    REQUIRE(generated.ordinaryStatus == result::OrdinaryStatus::Complete);
    REQUIRE(generated.ordinary.size() == 1);
    const auto wire = result::encode(reply, request);
    REQUIRE(wire);
    CHECK(result::decode(*wire, request));

    const auto& cache = std::get<RS_SplineData>(generated.ordinary.front());
    const req::Request validation{token,
        req::ValidateSourceCache{cache, definition, {}}};
    const auto validationReply = LC_OffsetWorkerKernel::run(validation);
    const auto& checked = native(validationReply, 8);
    CHECK(checked.refusal == LC_OffsetCurveRefusal::None);
    CHECK(checked.changed);
    CHECK(checked.engineStatus == LC_CurveOffsetStatus::Ok);

    data.fitEndTangent *= -1.0;
    const LC_OffsetSourceDefinition inconsistent{
        LC_OffsetSourceGeometry{data}, .05, 1e-4, false};
    const req::Request refusedRequest{token,
        req::GenerateSource{inconsistent, {}}};
    const auto refusedReply = LC_OffsetWorkerKernel::run(refusedRequest);
    const auto& refused = native(refusedReply, 7);
    CHECK(refused.engineStatus == LC_CurveOffsetStatus::InvalidSource);
    CHECK(refused.ordinary.empty());
    CHECK_FALSE(refused.changed);
}

TEST_CASE("offset worker admits rational quadratic tangent sources and validates their cache",
          "[lc-offsetcurve][worker-kernel][source-generation][fit-points][tangents][rational][quadratic]") {
    RS_SplineData data{2, false};
    data.controlPoints = {{0, 0}, {2, 3}, {5, 0}};
    data.knotslist = {0, 0, 0, 1, 1, 1};
    data.weights = {1.0, 0.7, 1.0};
    data.fitTangentsConstrained = true;
    data.fitTolerance = 1e-2;
    data.fitStartTangent = {2, 3};
    data.fitEndTangent = {3, -3};
    RS_Spline evaluator{nullptr, data};
    for (const double parameter : {0.0, 0.2, 0.5, 0.8, 1.0}) {
        LC_CurveJet jet;
        REQUIRE(evaluator.tryEvaluateJet(parameter,
            parameter == 1.0 ? LC_CurveEvaluationSide::Left
                             : LC_CurveEvaluationSide::Right, jet));
        data.fitPoints.push_back(jet.point);
    }
    RS_Spline source{nullptr, data};
    REQUIRE(source.fitTangentsMatchCurve());
    const LC_OffsetSourceDefinition definition{
        LC_OffsetSourceGeometry{data}, .05, 1e-4, false};
    const req::Request request{token, req::GenerateSource{definition, {}}};
    const auto reply = LC_OffsetWorkerKernel::run(request);
    const auto& generated = native(reply, 7);
    REQUIRE(generated.engineStatus == LC_CurveOffsetStatus::Ok);
    REQUIRE(generated.ordinaryStatus == result::OrdinaryStatus::Complete);
    REQUIRE(generated.ordinary.size() == 1);
    const auto wire = result::encode(reply, request);
    REQUIRE(wire);
    CHECK(result::decode(*wire, request));

    const auto& cache = std::get<RS_SplineData>(generated.ordinary.front());
    const req::Request validation{token,
        req::ValidateSourceCache{cache, definition, {}}};
    const auto validatedReply = LC_OffsetWorkerKernel::run(validation);
    const auto& validated = native(validatedReply, 8);
    CHECK(validated.refusal == LC_OffsetCurveRefusal::None);
    CHECK(validated.changed);
    CHECK(validated.engineStatus == LC_CurveOffsetStatus::Ok);
}

TEST_CASE("offset worker admits consistent degree-one tangent sources and validates their cache",
          "[lc-offsetcurve][worker-kernel][source-generation][fit-points][tangents][linear]") {
    RS_SplineData data{1, false};
    data.controlPoints = {{0, 0}, {4, 0}};
    data.knotslist = {0, 0, 1, 1};
    data.weights = {1.0, 0.5};
    data.fitPoints = {{0, 0}, {2, 0}, {4, 0}};
    data.fitTangentsConstrained = true;
    data.fitTolerance = 1e-8;
    data.fitStartTangent = {1, 0};
    data.fitEndTangent = {3, 0};
    RS_Spline spline{nullptr, data};
    REQUIRE(spline.fitTangentsMatchCurve());

    const LC_OffsetSourceDefinition definition{
        LC_OffsetSourceGeometry{data}, .05, 1e-4, false};
    const req::Request request{token, req::GenerateSource{definition, {}}};
    const auto reply = LC_OffsetWorkerKernel::run(request);
    const auto& generated = native(reply, 7);
    REQUIRE(generated.engineStatus == LC_CurveOffsetStatus::Ok);
    REQUIRE(generated.ordinaryStatus == result::OrdinaryStatus::Complete);
    REQUIRE(generated.ordinary.size() == 1);
    const auto wire = result::encode(reply, request);
    REQUIRE(wire);
    CHECK(result::decode(*wire, request));

    const auto& cache = std::get<RS_SplineData>(generated.ordinary.front());
    const req::Request validation{token,
        req::ValidateSourceCache{cache, definition, {}}};
    const auto validationReply = LC_OffsetWorkerKernel::run(validation);
    const auto& validated = native(validationReply, 8);
    CHECK(validated.refusal == LC_OffsetCurveRefusal::None);
    CHECK(validated.changed);
    CHECK(validated.engineStatus == LC_CurveOffsetStatus::Ok);

    data.fitEndTangent = {0, 0};
    const LC_OffsetSourceDefinition oneEndedDefinition{
        LC_OffsetSourceGeometry{data}, .05, 1e-4, false};
    const req::Request oneEndedRequest{token,
        req::GenerateSource{oneEndedDefinition, {}}};
    const auto oneEndedReply = LC_OffsetWorkerKernel::run(oneEndedRequest);
    const auto& oneEndedGenerated = native(oneEndedReply, 7);
    REQUIRE(oneEndedGenerated.engineStatus == LC_CurveOffsetStatus::Ok);
    REQUIRE(oneEndedGenerated.ordinary.size() == 1);
    const req::Request oneEndedValidation{token,
        req::ValidateSourceCache{
            std::get<RS_SplineData>(oneEndedGenerated.ordinary.front()),
            oneEndedDefinition, {}}};
    const auto oneEndedValidationReply =
        LC_OffsetWorkerKernel::run(oneEndedValidation);
    const auto& oneEndedValidated = native(oneEndedValidationReply, 8);
    CHECK(oneEndedValidated.refusal == LC_OffsetCurveRefusal::None);
    CHECK(oneEndedValidated.engineStatus == LC_CurveOffsetStatus::Ok);

    data.fitEndTangent = {-1, 0};
    const LC_OffsetSourceDefinition inconsistent{
        LC_OffsetSourceGeometry{data}, .05, 1e-4, false};
    const req::Request refusedRequest{token,
        req::GenerateSource{inconsistent, {}}};
    const auto refusedReply = LC_OffsetWorkerKernel::run(refusedRequest);
    const auto& refused = native(refusedReply, 7);
    CHECK(refused.engineStatus == LC_CurveOffsetStatus::InvalidSource);
    CHECK(refused.ordinary.empty());
    CHECK_FALSE(refused.changed);
}

TEST_CASE("offset worker admits consistent tangent-constrained spline-points and validates their cache",
          "[lc-offsetcurve][worker-kernel][source-generation][fit-points][tangents][spline-points]") {
    LC_SplinePointsData data{false, false};
    data.splinePoints = {{0, 0}, {1, 2}, {3, 1.5}, {4, 0}};
    LC_SplinePoints source{nullptr, data};
    LC_CurveJet startJet;
    LC_CurveJet endJet;
    REQUIRE(source.tryEvaluateJet(0.0, LC_CurveEvaluationSide::Right, startJet));
    REQUIRE(source.tryEvaluateJet(static_cast<double>(source.getSegmentCount()),
        LC_CurveEvaluationSide::Left, endJet));
    data = source.getData();
    data.fitTangentsConstrained = true;
    data.fitTolerance = 1e-7;
    data.fitStartTangent = startJet.first;
    data.fitEndTangent = endJet.first;
    source.getData() = data;
    REQUIRE(source.fitTangentsMatchCurve());

    const LC_OffsetSourceDefinition definition{
        LC_OffsetSourceGeometry{data}, .05, 1e-4, false};
    const req::Request request{token, req::GenerateSource{definition, {}}};
    const auto reply = LC_OffsetWorkerKernel::run(request);
    const auto& generated = native(reply, 7);
    REQUIRE(generated.engineStatus == LC_CurveOffsetStatus::Ok);
    REQUIRE(generated.ordinaryStatus == result::OrdinaryStatus::Complete);
    REQUIRE(generated.ordinary.size() == 1);
    const auto wire = result::encode(reply, request);
    REQUIRE(wire);
    CHECK(result::decode(*wire, request));

    const req::Request validation{token,
        req::ValidateSourceCache{
            std::get<RS_SplineData>(generated.ordinary.front()), definition, {}}};
    const auto validationReply = LC_OffsetWorkerKernel::run(validation);
    const auto& validated = native(validationReply, 8);
    CHECK(validated.refusal == LC_OffsetCurveRefusal::None);
    CHECK(validated.changed);
    CHECK(validated.engineStatus == LC_CurveOffsetStatus::Ok);

    data.fitStartTangent *= -1.0;
    const LC_OffsetSourceDefinition inconsistent{
        LC_OffsetSourceGeometry{data}, .05, 1e-4, false};
    const req::Request refusedRequest{token,
        req::GenerateSource{inconsistent, {}}};
    const auto refusedReply = LC_OffsetWorkerKernel::run(refusedRequest);
    const auto& refused = native(refusedReply, 7);
    CHECK(refused.engineStatus == LC_CurveOffsetStatus::InvalidSource);
    CHECK(refused.ordinary.empty());
    CHECK_FALSE(refused.changed);
}

TEST_CASE("offset worker generates and validates caches for degree four through six spline sources",
          "[lc-offsetcurve][worker-kernel][source-generation][spline][high-degree]") {
    for (std::size_t degree : {4u, 5u, 6u}) {
        CAPTURE(degree);
        RS_SplineData data;
        data.degree = degree;
        data.type = RS_SplineData::SplineType::ClampedOpen;
        data.savedOpenType = data.type;
        for (std::size_t i = 0; i <= degree; ++i) {
            const double t = static_cast<double>(i) / static_cast<double>(degree);
            data.controlPoints.emplace_back(4.0 * t, 1.2 * t * (1.0 - t) + 0.1 * t);
            data.weights.push_back(0.75 + 0.5 * t);
        }
        data.knotslist.insert(data.knotslist.end(), degree + 1, 0.0);
        data.knotslist.insert(data.knotslist.end(), degree + 1, 1.0);
        const LC_OffsetSourceDefinition definition{
            LC_OffsetSourceGeometry{data}, .02, 1e-3, false};
        REQUIRE(definition.isWellFormed());
        const RS_Spline source{nullptr, data};
        REQUIRE(source.validate());

        const req::Request generation{token,
            req::GenerateSource{definition, {}}};
        const auto generatedReply = LC_OffsetWorkerKernel::run(generation);
        const auto& generated = native(generatedReply, 7);
        INFO("degree=" << degree << " engine="
             << (generated.engineStatus ? int(*generated.engineStatus) : -1)
             << " ordinary=" << int(generated.ordinaryStatus)
             << " refusal=" << int(generated.refusal));
        REQUIRE(generated.engineStatus == LC_CurveOffsetStatus::Ok);
        REQUIRE(generated.ordinaryStatus == result::OrdinaryStatus::Complete);
        REQUIRE(generated.ordinary.size() == 1);
        REQUIRE(std::holds_alternative<RS_SplineData>(generated.ordinary.front()));
        const auto& cache = std::get<RS_SplineData>(generated.ordinary.front());
        CHECK((cache.degree == 1 || cache.degree == 3));

        double domainStart = 0.0, domainEnd = 0.0;
        REQUIRE(source.getParameterDomain(domainStart, domainEnd));
        const auto options = LC_OffsetWorkerTolerance::directOptions(definition, source);
        REQUIRE(options);
        double maximumOracleDistance = 0.0;
        for (int sample = 1; sample < 16; ++sample) {
            const double parameter = domainStart + (domainEnd - domainStart) *
                static_cast<double>(sample) / 16.0;
            const auto expected = oracleOffsetPoint(
                data, SplineOracle{parameter}, SplineOracle{"1e-20"},
                SplineOracle{domainStart}, SplineOracle{domainEnd},
                SplineOracle{definition.signedDistance});
            const RS_Vector oraclePoint{expected.first.convert_to<double>(),
                                        expected.second.convert_to<double>()};
            maximumOracleDistance = std::max(maximumOracleDistance,
                sampledSplineDistance(cache, oraclePoint, 8192));
        }
        CAPTURE(degree, maximumOracleDistance,
                options->tolerance.requestedGeometry);
        CHECK(maximumOracleDistance <=
              2.0 * options->tolerance.requestedGeometry);
        const auto encoded = result::encode(generatedReply, generation);
        REQUIRE(encoded);
        CHECK(result::decode(*encoded, generation));

        const req::Request validation{token,
            req::ValidateSourceCache{cache, definition, {}}};
        const auto validationReply = LC_OffsetWorkerKernel::run(validation);
        const auto& validated = native(validationReply, 8);
        CHECK(validated.refusal == LC_OffsetCurveRefusal::None);
        CHECK(validated.changed);
        CHECK(validated.engineStatus == LC_CurveOffsetStatus::Ok);
        const auto validatedWire = result::encode(validationReply, validation);
        REQUIRE(validatedWire);
        CHECK(result::decode(*validatedWire, validation));
    }
}

TEST_CASE("offset worker generates degree-four and degree-six splines with a distinct interior knot",
          "[lc-offsetcurve][worker-kernel][source-generation][spline][high-degree][knots]") {
    for (std::size_t degree : {4u, 6u}) {
        CAPTURE(degree);
        RS_SplineData data;
        data.degree = degree;
        data.type = RS_SplineData::SplineType::ClampedOpen;
        data.savedOpenType = data.type;
        for (std::size_t i = 0; i <= degree + 1; ++i) {
            const double t = static_cast<double>(i) / static_cast<double>(degree + 1);
            data.controlPoints.emplace_back(4.0 * t, 1.2 * t * (1.0 - t) + 0.1 * t);
            data.weights.push_back(0.75 + 0.5 * t);
        }
        data.knotslist.insert(data.knotslist.end(), degree + 1, 0.0);
        data.knotslist.push_back(.5);
        data.knotslist.insert(data.knotslist.end(), degree + 1, 1.0);
        const LC_OffsetSourceDefinition definition{
            LC_OffsetSourceGeometry{data}, .01, 2e-3, false};
        REQUIRE(definition.isWellFormed());

        RS_Spline source{nullptr, data};
        REQUIRE(source.validate());
        const auto breaks = source.getBreakParameters();
        REQUIRE(breaks.size() == 3);
        CHECK(breaks[1] == .5);

        const req::Request generation{token, req::GenerateSource{definition, {}}};
        const auto generatedReply = LC_OffsetWorkerKernel::run(generation);
        const auto& generated = native(generatedReply, 7);
        INFO("degree=" << degree << " engine="
             << (generated.engineStatus ? int(*generated.engineStatus) : -1)
             << " ordinary=" << int(generated.ordinaryStatus)
             << " refusal=" << int(generated.refusal));
        REQUIRE(generated.engineStatus == LC_CurveOffsetStatus::Ok);
        REQUIRE(generated.ordinaryStatus == result::OrdinaryStatus::Complete);
        REQUIRE(generated.ordinary.size() == 1);
        REQUIRE(std::holds_alternative<RS_SplineData>(generated.ordinary.front()));
        const auto& cache = std::get<RS_SplineData>(generated.ordinary.front());
        CHECK((cache.degree == 1 || cache.degree == 3));

        const auto oracleDistance = maximumOracleOffsetDistance(data, cache, definition.signedDistance);
        REQUIRE(oracleDistance);
        const auto tolerance = LC_OffsetWorkerTolerance::directOptions(definition, source);
        REQUIRE(tolerance);
        CAPTURE(*oracleDistance, tolerance->tolerance.requestedGeometry);
        CHECK(*oracleDistance <= 2.0 * tolerance->tolerance.requestedGeometry);

        const req::Request validation{token,
            req::ValidateSourceCache{cache, definition, {}}};
        const auto validationReply = LC_OffsetWorkerKernel::run(validation);
        const auto& validated = native(validationReply, 8);
        CHECK(validated.refusal == LC_OffsetCurveRefusal::None);
        CHECK(validated.changed);
        CHECK(validated.engineStatus == LC_CurveOffsetStatus::Ok);
    }
}

TEST_CASE("offset worker generates high-degree splines with asymmetric nonuniform knots",
          "[lc-offsetcurve][worker-kernel][source-generation][spline][high-degree][knots]") {
    for (std::size_t degree : {4u, 6u}) {
        CAPTURE(degree);
        RS_SplineData data;
        data.degree = degree;
        data.type = RS_SplineData::SplineType::ClampedOpen;
        data.savedOpenType = data.type;
        for (std::size_t i = 0; i <= degree + 2; ++i) {
            const double t = static_cast<double>(i) / static_cast<double>(degree + 2);
            data.controlPoints.emplace_back(4.0 * t, 1.2 * t * (1.0 - t) + 0.1 * t);
            data.weights.push_back(0.75 + 0.5 * t);
        }
        data.knotslist.insert(data.knotslist.end(), degree + 1, 0.0);
        data.knotslist.insert(data.knotslist.end(), {.2, .7});
        data.knotslist.insert(data.knotslist.end(), degree + 1, 1.0);
        const LC_OffsetSourceDefinition definition{
            LC_OffsetSourceGeometry{data}, .01, 2e-3, false};
        REQUIRE(definition.isWellFormed());

        RS_Spline source{nullptr, data};
        REQUIRE(source.validate());
        const auto breaks = source.getBreakParameters();
        REQUIRE(breaks.size() == 4);
        CHECK(breaks[1] == .2);
        CHECK(breaks[2] == .7);

        const req::Request generation{token, req::GenerateSource{definition, {}}};
        const auto generatedReply = LC_OffsetWorkerKernel::run(generation);
        const auto& generated = native(generatedReply, 7);
        INFO("degree=" << degree << " engine="
             << (generated.engineStatus ? int(*generated.engineStatus) : -1)
             << " ordinary=" << int(generated.ordinaryStatus)
             << " refusal=" << int(generated.refusal));
        REQUIRE(generated.engineStatus == LC_CurveOffsetStatus::Ok);
        REQUIRE(generated.ordinaryStatus == result::OrdinaryStatus::Complete);
        REQUIRE(generated.ordinary.size() == 1);
        REQUIRE(std::holds_alternative<RS_SplineData>(generated.ordinary.front()));
        const auto& cache = std::get<RS_SplineData>(generated.ordinary.front());
        CHECK((cache.degree == 1 || cache.degree == 3));

        const auto oracleDistance = maximumOracleOffsetDistance(
            data, cache, definition.signedDistance);
        REQUIRE(oracleDistance);
        const auto tolerance = LC_OffsetWorkerTolerance::directOptions(definition, source);
        REQUIRE(tolerance);
        CAPTURE(degree, *oracleDistance, tolerance->tolerance.requestedGeometry);
        CHECK(*oracleDistance <= 2.0 * tolerance->tolerance.requestedGeometry);

        const req::Request validation{token,
            req::ValidateSourceCache{cache, definition, {}}};
        const auto validationReply = LC_OffsetWorkerKernel::run(validation);
        const auto& validated = native(validationReply, 8);
        CHECK(validated.refusal == LC_OffsetCurveRefusal::None);
        CHECK(validated.changed);
        CHECK(validated.engineStatus == LC_CurveOffsetStatus::Ok);
    }
}

TEST_CASE("offset worker generates degree-four and degree-six splines with a repeated knot",
          "[lc-offsetcurve][worker-kernel][source-generation][spline][high-degree][knots]") {
    for (std::size_t degree : {4u, 6u}) {
        CAPTURE(degree);
        RS_SplineData data;
        data.degree = degree;
        data.type = RS_SplineData::SplineType::ClampedOpen;
        data.savedOpenType = data.type;
        for (std::size_t i = 0; i <= degree + 2; ++i) {
            const double t = static_cast<double>(i) / static_cast<double>(degree + 2);
            data.controlPoints.emplace_back(4.0 * t, 1.2 * t * (1.0 - t) + 0.1 * t);
            data.weights.push_back(0.75 + 0.5 * t);
        }
        data.knotslist.insert(data.knotslist.end(), degree + 1, 0.0);
        data.knotslist.insert(data.knotslist.end(), 2, .4);
        data.knotslist.insert(data.knotslist.end(), degree + 1, 1.0);
        const LC_OffsetSourceDefinition definition{
            LC_OffsetSourceGeometry{data}, .01, 2e-3, false};
        REQUIRE(definition.isWellFormed());

        RS_Spline source{nullptr, data};
        REQUIRE(source.validate());
        const auto breaks = source.getBreakParameters();
        REQUIRE(breaks.size() == 3);
        CHECK(breaks[1] == .4);

        const req::Request generation{token, req::GenerateSource{definition, {}}};
        const auto generatedReply = LC_OffsetWorkerKernel::run(generation);
        const auto& generated = native(generatedReply, 7);
        INFO("degree=" << degree << " engine="
             << (generated.engineStatus ? int(*generated.engineStatus) : -1)
             << " ordinary=" << int(generated.ordinaryStatus)
             << " refusal=" << int(generated.refusal));
        REQUIRE(generated.engineStatus == LC_CurveOffsetStatus::Ok);
        REQUIRE(generated.ordinaryStatus == result::OrdinaryStatus::Complete);
        REQUIRE(generated.ordinary.size() == 1);
        REQUIRE(std::holds_alternative<RS_SplineData>(generated.ordinary.front()));
        const auto& cache = std::get<RS_SplineData>(generated.ordinary.front());
        CHECK((cache.degree == 1 || cache.degree == 3));

        const auto oracleDistance = maximumOracleOffsetDistance(
            data, cache, definition.signedDistance);
        REQUIRE(oracleDistance);
        const auto tolerance = LC_OffsetWorkerTolerance::directOptions(definition, source);
        REQUIRE(tolerance);
        CAPTURE(degree, *oracleDistance, tolerance->tolerance.requestedGeometry);
        CHECK(*oracleDistance <= 2.0 * tolerance->tolerance.requestedGeometry);

        const req::Request validation{token,
            req::ValidateSourceCache{cache, definition, {}}};
        const auto validationReply = LC_OffsetWorkerKernel::run(validation);
        const auto& validated = native(validationReply, 8);
        CHECK(validated.refusal == LC_OffsetCurveRefusal::None);
        CHECK(validated.changed);
        CHECK(validated.engineStatus == LC_CurveOffsetStatus::Ok);
    }
}

TEST_CASE("offset worker oracle-checks multi-span rational degrees four through six with a repeated knot",
          "[lc-offsetcurve][worker-kernel][source-generation][spline][high-degree][repeated-knot][oracle]") {
    for (const std::size_t degree : {4u, 5u, 6u}) {
        CAPTURE(degree);
        RS_SplineData data;
        data.degree = degree;
        data.type = RS_SplineData::SplineType::ClampedOpen;
        data.savedOpenType = data.type;
        const std::size_t count = degree + 4;
        for (std::size_t i = 0; i < count; ++i) {
            const double t = static_cast<double>(i) / static_cast<double>(count - 1);
            data.controlPoints.emplace_back(8.0 * t,
                2.5 * t * (1.0 - t) * (1.0 + 0.15 * (2.0 * t - 1.0)));
            data.weights.push_back(0.6 + 0.9 * t);
        }
        data.knotslist.insert(data.knotslist.end(), degree + 1, 0.0);
        data.knotslist.insert(data.knotslist.end(), {0.35, 0.65, 0.65});
        data.knotslist.insert(data.knotslist.end(), degree + 1, 1.0);
        const LC_OffsetSourceDefinition definition{
            LC_OffsetSourceGeometry{data}, .15, 2e-3, false};
        REQUIRE(definition.isWellFormed());
        const RS_Spline source{nullptr, data};
        REQUIRE(source.validate());

        const req::Request generation{token,
            req::GenerateSource{definition, {}}};
        const auto generatedReply = LC_OffsetWorkerKernel::run(generation);
        const auto& generated = native(generatedReply, 7);
        INFO("engine=" << (generated.engineStatus ? int(*generated.engineStatus) : -1)
             << " ordinary=" << int(generated.ordinaryStatus)
             << " refusal=" << int(generated.refusal));
        REQUIRE(generated.engineStatus == LC_CurveOffsetStatus::Ok);
        REQUIRE(generated.ordinaryStatus == result::OrdinaryStatus::Complete);
        REQUIRE(generated.ordinary.size() == 1);
        REQUIRE(std::holds_alternative<RS_SplineData>(generated.ordinary.front()));
        const auto& cache = std::get<RS_SplineData>(generated.ordinary.front());
        CHECK((cache.degree == 1 || cache.degree == 3));

        const auto oracleDistance = maximumOracleOffsetDistance(
            data, cache, definition.signedDistance);
        REQUIRE(oracleDistance);
        const auto tolerance = LC_OffsetWorkerTolerance::directOptions(definition, source);
        REQUIRE(tolerance);
        CAPTURE(*oracleDistance, tolerance->tolerance.requestedGeometry);
        CHECK(*oracleDistance <= 2.0 * tolerance->tolerance.requestedGeometry);
    }
}

TEST_CASE("offset worker oracle-checks degree-multiplicity rational spline corners",
          "[lc-offsetcurve][worker-kernel][source-generation][spline][high-degree][corner][oracle]") {
    for (const std::size_t degree : {4u, 5u, 6u}) {
        CAPTURE(degree);
        RS_SplineData data;
        data.degree = degree;
        data.type = RS_SplineData::SplineType::ClampedOpen;
        data.savedOpenType = data.type;
        for (std::size_t i = 0; i <= degree; ++i) {
            const double t = static_cast<double>(i) / static_cast<double>(degree);
            data.controlPoints.emplace_back(4.0 * t, 2.0 * std::sin(M_PI * t));
            data.weights.push_back(0.75 + 0.25 * t);
        }
        for (std::size_t i = 1; i <= degree; ++i) {
            const double t = static_cast<double>(i) / static_cast<double>(degree);
            data.controlPoints.emplace_back(4.0 + 4.0 * t, 2.0 * std::sin(M_PI * t));
            data.weights.push_back(1.0 + 0.25 * t);
        }
        data.knotslist.insert(data.knotslist.end(), degree + 1, 0.0);
        data.knotslist.insert(data.knotslist.end(), degree, 0.5);
        data.knotslist.insert(data.knotslist.end(), degree + 1, 1.0);
        const LC_OffsetSourceDefinition definition{
            LC_OffsetSourceGeometry{data}, .1, 2e-3, false};
        REQUIRE(definition.isWellFormed());
        const RS_Spline source{nullptr, data};
        REQUIRE(source.validate());

        const req::Request generation{token,
            req::GenerateSource{definition, {}}};
        const auto generatedReply = LC_OffsetWorkerKernel::run(generation);
        const auto& generated = native(generatedReply, 7);
        INFO("engine=" << (generated.engineStatus ? int(*generated.engineStatus) : -1)
             << " ordinary=" << int(generated.ordinaryStatus)
             << " refusal=" << int(generated.refusal));
        REQUIRE(generated.engineStatus == LC_CurveOffsetStatus::Ok);
        REQUIRE(generated.ordinaryStatus == result::OrdinaryStatus::Complete);
        REQUIRE(generated.ordinary.size() == 2);
        std::vector<RS_SplineData> outputSplines;
        for (const auto& output : generated.ordinary) {
            if (const auto* spline = std::get_if<RS_SplineData>(&output)) {
                outputSplines.push_back(*spline);
            } else if (const auto* segment = std::get_if<req::Segment>(&output)) {
                RS_SplineData lineCache;
                lineCache.degree = 1;
                lineCache.controlPoints = {segment->start, segment->end};
                lineCache.knotslist = {0.0, 0.0, 1.0, 1.0};
                lineCache.weights = {1.0, 1.0};
                outputSplines.push_back(std::move(lineCache));
            } else {
                FAIL("unsupported ordinary output for a C0 spline corner");
            }
        }

        const auto oracleDistance = maximumOracleOffsetDistance(
            data, outputSplines, definition.signedDistance);
        REQUIRE(oracleDistance);
        const auto tolerance = LC_OffsetWorkerTolerance::directOptions(definition, source);
        REQUIRE(tolerance);
        CAPTURE(*oracleDistance, tolerance->tolerance.requestedGeometry);
        CHECK(*oracleDistance <= 2.0 * tolerance->tolerance.requestedGeometry);

        RS_SplineData cache;
        REQUIRE(LC_OffsetWorkerSourceCache::combine(generated.ordinary, cache));
        CHECK(generated.refusal == LC_OffsetCurveRefusal::NotCacheRepresentable);
        const req::Request validation{token,
            req::ValidateSourceCache{cache, definition, {}}};
        const auto validationReply = LC_OffsetWorkerKernel::run(validation);
        const auto& validated = native(validationReply, 8);
        CHECK(validated.refusal == LC_OffsetCurveRefusal::None);
        CHECK(validated.changed);
        CHECK(validated.engineStatus == LC_CurveOffsetStatus::Ok);
    }
}

TEST_CASE("offset worker oracle-checks acute and obtuse rational spline corners on both sides",
          "[lc-offsetcurve][worker-kernel][source-generation][spline][high-degree][corner][signed][oracle]") {
    for (const std::size_t degree : {4u, 5u, 6u}) {
        const double alpha = std::atan2(2.0 * std::sin(M_PI / static_cast<double>(degree)), 4.0 / static_cast<double>(degree));
        for (const double joinAngle : {M_PI / 6.0, M_PI / 2.0, 5.0 * M_PI / 6.0}) {
            for (const RS_Vector translation : {RS_Vector{0.0, 0.0}, RS_Vector{1.0e6, -2.0e6}}) {
                for (const double rotation : {0.0, M_PI / 180.0 * 37.0}) {
                    for (const double signedDistance : {-0.1, 0.1}) {
                        CAPTURE(degree, joinAngle, signedDistance, translation, rotation);
                        RS_SplineData data;
                        data.degree = degree;
                        data.type = RS_SplineData::SplineType::ClampedOpen;
                        data.savedOpenType = data.type;
                        for (std::size_t i = 0; i <= degree; ++i) {
                            const double t = static_cast<double>(i) / static_cast<double>(degree);
                            data.controlPoints.emplace_back(4.0 * t, 2.0 * std::sin(M_PI * t));
                            data.weights.push_back(0.75 + 0.25 * t);
                        }
                        const double cornerHeading = joinAngle - 2.0 * alpha;
                        const double cosine = std::cos(cornerHeading);
                        const double sine = std::sin(cornerHeading);
                        for (std::size_t i = 1; i <= degree; ++i) {
                            const double t = static_cast<double>(i) / static_cast<double>(degree);
                            const double x = 4.0 * t;
                            const double y = 2.0 * std::sin(M_PI * t);
                            data.controlPoints.emplace_back(4.0 + cosine * x - sine * y, sine * x + cosine * y);
                            data.weights.push_back(1.0 + 0.25 * t);
                        }
                        data.knotslist.insert(data.knotslist.end(), degree + 1, 0.0);
                        data.knotslist.insert(data.knotslist.end(), degree, 0.5);
                        data.knotslist.insert(data.knotslist.end(), degree + 1, 1.0);
                        const double cosineRotation = std::cos(rotation);
                        const double sineRotation = std::sin(rotation);
                        for (RS_Vector& point : data.controlPoints) {
                            point = {cosineRotation * point.x - sineRotation * point.y + translation.x,
                                     sineRotation * point.x + cosineRotation * point.y + translation.y};
                        }
                        const LC_OffsetSourceDefinition definition{LC_OffsetSourceGeometry{data}, signedDistance, 2e-3, false};
                        REQUIRE(definition.isWellFormed());
                        const RS_Spline source{nullptr, data};
                        REQUIRE(source.validate());

                        const req::Request generation{token, req::GenerateSource{definition, {}}};
                        const auto generatedReply = LC_OffsetWorkerKernel::run(generation);
                        const auto& generated = native(generatedReply, 7);
                        REQUIRE(generated.engineStatus == LC_CurveOffsetStatus::Ok);
                        REQUIRE(generated.ordinaryStatus == result::OrdinaryStatus::Complete);
                        REQUIRE(generated.ordinary.size() >= 1);
                        REQUIRE(generated.ordinary.size() <= 2);
                        std::vector<RS_SplineData> outputSplines;
                        for (const auto& output : generated.ordinary) {
                            if (const auto* spline = std::get_if<RS_SplineData>(&output)) {
                                outputSplines.push_back(*spline);
                            }
                            else if (const auto* segment = std::get_if<req::Segment>(&output)) {
                                RS_SplineData lineCache;
                                lineCache.degree = 1;
                                lineCache.controlPoints = {segment->start, segment->end};
                                lineCache.knotslist = {0.0, 0.0, 1.0, 1.0};
                                lineCache.weights = {1.0, 1.0};
                                outputSplines.push_back(std::move(lineCache));
                            }
                            else {
                                FAIL("unsupported ordinary output for a rational spline corner");
                            }
                        }
                        const auto oracleDistance = maximumOracleOffsetDistance(data, outputSplines, signedDistance);
                        REQUIRE(oracleDistance);
                        const auto tolerance = LC_OffsetWorkerTolerance::directOptions(definition, source);
                        REQUIRE(tolerance);
                        CAPTURE(*oracleDistance, tolerance->tolerance.requestedGeometry);
                        CHECK(*oracleDistance <= 2.0 * tolerance->tolerance.requestedGeometry);

                        RS_SplineData cache;
                        REQUIRE(LC_OffsetWorkerSourceCache::combine(generated.ordinary, cache));
                        const req::Request validation{token, req::ValidateSourceCache{cache, definition, {}}};
                        const auto validationReply = LC_OffsetWorkerKernel::run(validation);
                        const auto& validated = native(validationReply, 8);
                        CHECK(validated.refusal == LC_OffsetCurveRefusal::None);
                        CHECK(validated.changed);
                        CHECK(validated.engineStatus == LC_CurveOffsetStatus::Ok);
                    }
                }
            }
        }
    }
}

TEST_CASE("offset worker oracle-checks degree-minus-one repeated knots on both sides", "[lc-offsetcurve][worker-kernel][source-generation][spline][high-degree]["
                                                                                       "c1][oracle]") {
    constexpr double interiorKnot = 0.37;
    for (const std::size_t degree : {4u, 5u, 6u}) {
        for (const RS_Vector translation : {RS_Vector{0.0, 0.0}, RS_Vector{1.0e6, -2.0e6}}) {
            for (const double rotation : {0.0, M_PI / 180.0 * 37.0}) {
                for (const double signedDistance : {-0.1, 0.1}) {
                    CAPTURE(degree, signedDistance, interiorKnot, translation, rotation);
                    RS_SplineData data;
                    data.degree = degree;
                    data.type = RS_SplineData::SplineType::ClampedOpen;
                    data.savedOpenType = data.type;
                    std::vector<RS_Vector> firstSpan;
                    for (std::size_t i = 0; i <= degree; ++i) {
                        const double t = static_cast<double>(i) / static_cast<double>(degree);
                        firstSpan.emplace_back(4.0 * t, 2.0 * std::sin(M_PI * t));
                    }
                    data.controlPoints = firstSpan;
                    const RS_Vector joint = firstSpan.back();
                    const RS_Vector derivativeControl = joint - firstSpan[firstSpan.size() - 2];
                    const RS_Vector secondHandle = joint + derivativeControl * ((1.0 - interiorKnot) / interiorKnot);
                    const RS_Vector endpoint{8.0, 1.0};
                    for (std::size_t i = 2; i <= degree; ++i) {
                        const double t = static_cast<double>(i - 1) / static_cast<double>(degree - 1);
                        data.controlPoints.push_back(secondHandle + (endpoint - secondHandle) * t);
                    }
                    data.weights.resize(data.controlPoints.size());
                    for (std::size_t i = 0; i < data.weights.size(); ++i)
                        data.weights[i] = 0.65 + 0.17 * static_cast<double>(i % 5);
                    const double cosine = std::cos(rotation);
                    const double sine = std::sin(rotation);
                    for (RS_Vector &point : data.controlPoints) {
                        point = {cosine * point.x - sine * point.y + translation.x, sine * point.x + cosine * point.y + translation.y};
                    }
                    data.knotslist.insert(data.knotslist.end(), degree + 1, 0.0);
                    data.knotslist.insert(data.knotslist.end(), degree - 1, interiorKnot);
                    data.knotslist.insert(data.knotslist.end(), degree + 1, 1.0);
                    const LC_OffsetSourceDefinition definition{LC_OffsetSourceGeometry{data}, signedDistance, 2e-3, false};
                    REQUIRE(definition.isWellFormed());
                    const RS_Spline source{nullptr, data};
                    REQUIRE(source.validate());

                    LC_CurveJet leftJet;
                    LC_CurveJet rightJet;
                    REQUIRE(source.tryEvaluateJet(interiorKnot, LC_CurveEvaluationSide::Left, leftJet));
                    REQUIRE(source.tryEvaluateJet(interiorKnot, LC_CurveEvaluationSide::Right, rightJet));
                    CHECK(leftJet.point.distanceTo(rightJet.point) < 1e-10);
                    CHECK(leftJet.first.normalized().distanceTo(rightJet.first.normalized()) < 1e-10);

                    const req::Request generation{token, req::GenerateSource{definition, {}}};
                    const auto generatedReply = LC_OffsetWorkerKernel::run(generation);
                    const auto &generated = native(generatedReply, 7);
                    REQUIRE(generated.engineStatus == LC_CurveOffsetStatus::Ok);
                    REQUIRE(generated.ordinaryStatus == result::OrdinaryStatus::Complete);
                    REQUIRE_FALSE(generated.ordinary.empty());
                    std::vector<RS_SplineData> outputSplines;
                    for (const auto &output : generated.ordinary) {
                        if (const auto *spline = std::get_if<RS_SplineData>(&output)) {
                            outputSplines.push_back(*spline);
                        } else if (const auto *segment = std::get_if<req::Segment>(&output)) {
                            RS_SplineData lineCache;
                            lineCache.degree = 1;
                            lineCache.controlPoints = {segment->start, segment->end};
                            lineCache.knotslist = {0.0, 0.0, 1.0, 1.0};
                            lineCache.weights = {1.0, 1.0};
                            outputSplines.push_back(std::move(lineCache));
                        } else {
                            FAIL("unsupported ordinary output for a C1 spline join");
                        }
                    }
                    const auto oracleDistance = maximumOracleOffsetDistance(data, outputSplines, signedDistance);
                    REQUIRE(oracleDistance);
                    const auto tolerance = LC_OffsetWorkerTolerance::directOptions(definition, source);
                    REQUIRE(tolerance);
                    CAPTURE(*oracleDistance, tolerance->tolerance.requestedGeometry);
                    CHECK(*oracleDistance <= 2.0 * tolerance->tolerance.requestedGeometry);

                    RS_SplineData cache;
                    REQUIRE(LC_OffsetWorkerSourceCache::combine(generated.ordinary, cache));
                    const req::Request validation{token, req::ValidateSourceCache{cache, definition, {}}};
                    const auto validationReply = LC_OffsetWorkerKernel::run(validation);
                    const auto &validated = native(validationReply, 8);
                    CHECK(validated.refusal == LC_OffsetCurveRefusal::None);
                    CHECK(validated.changed);
                    CHECK(validated.engineStatus == LC_CurveOffsetStatus::Ok);
                }
            }
        }
    }
}

TEST_CASE("offset worker generates asymmetric rational degree-four and degree-six sources",
          "[lc-offsetcurve][worker-kernel][source-generation][spline][high-degree][asymmetric]") {
    constexpr std::array<double, 8> yValues{0.0, 1.5, -.4, 2.2, .3, 1.1, -1.0, .5};
    constexpr std::array<double, 8> weights{.7, 1.5, .6, 1.8, .85, 1.3, 2.0, .9};
    for (std::size_t degree : {4u, 6u}) {
        CAPTURE(degree);
        RS_SplineData data;
        data.degree = degree;
        data.type = RS_SplineData::SplineType::ClampedOpen;
        data.savedOpenType = data.type;
        for (std::size_t i = 0; i <= degree + 1; ++i) {
            data.controlPoints.emplace_back(.8 * static_cast<double>(i) + .04 * (i % 2),
                                            yValues[i]);
            data.weights.push_back(weights[i]);
        }
        data.knotslist.insert(data.knotslist.end(), degree + 1, 0.0);
        data.knotslist.push_back(degree == 4 ? .3 : .62);
        data.knotslist.insert(data.knotslist.end(), degree + 1, 1.0);
        const LC_OffsetSourceDefinition definition{
            LC_OffsetSourceGeometry{data}, .01, 2e-3, false};
        REQUIRE(definition.isWellFormed());
        RS_Spline source{nullptr, data};
        REQUIRE(source.validate());

        const req::Request generation{token, req::GenerateSource{definition, {}}};
        const auto generatedReply = LC_OffsetWorkerKernel::run(generation);
        const auto& generated = native(generatedReply, 7);
        INFO("degree=" << degree << " engine="
             << (generated.engineStatus ? int(*generated.engineStatus) : -1)
             << " ordinary=" << int(generated.ordinaryStatus)
             << " refusal=" << int(generated.refusal));
        REQUIRE(generated.engineStatus == LC_CurveOffsetStatus::Ok);
        REQUIRE(generated.ordinaryStatus == result::OrdinaryStatus::Complete);
        REQUIRE(generated.ordinary.size() == 1);
        REQUIRE(std::holds_alternative<RS_SplineData>(generated.ordinary.front()));
        const auto& cache = std::get<RS_SplineData>(generated.ordinary.front());
        CHECK((cache.degree == 1 || cache.degree == 3));

        const auto oracleDistance = maximumOracleOffsetDistance(
            data, cache, definition.signedDistance);
        REQUIRE(oracleDistance);
        const auto tolerance = LC_OffsetWorkerTolerance::directOptions(definition, source);
        REQUIRE(tolerance);
        CAPTURE(degree, *oracleDistance, tolerance->tolerance.requestedGeometry);
        CHECK(*oracleDistance <= 2.0 * tolerance->tolerance.requestedGeometry);

        const req::Request validation{token,
            req::ValidateSourceCache{cache, definition, {}}};
        const auto validationReply = LC_OffsetWorkerKernel::run(validation);
        const auto& validated = native(validationReply, 8);
        CHECK(validated.refusal == LC_OffsetCurveRefusal::None);
        CHECK(validated.changed);
        CHECK(validated.engineStatus == LC_CurveOffsetStatus::Ok);
    }
}

TEST_CASE("offset worker generates periodic degree-four and degree-six spline sources",
          "[lc-offsetcurve][worker-kernel][source-generation][spline][high-degree][periodic]") {
    for (const std::size_t degree : {4u, 6u}) {
        CAPTURE(degree);
        RS_SplineData data;
        data.degree = degree;
        data.type = RS_SplineData::SplineType::ClampedOpen;
        data.savedOpenType = data.type;
        const std::size_t controls = degree + 3;
        for (std::size_t i = 0; i < controls; ++i) {
            const double angle = 2.0 * M_PI * static_cast<double>(i) / static_cast<double>(controls);
            data.controlPoints.emplace_back(10.0 * std::cos(angle), 7.0 * std::sin(angle));
        }
        data.weights.assign(controls, 1.0);
        data.knotslist.insert(data.knotslist.end(), degree + 1, 0.0);
        for (std::size_t i = 1; i <= controls - degree - 1; ++i)
            data.knotslist.push_back(static_cast<double>(i) /
                                     static_cast<double>(controls - degree));
        data.knotslist.insert(data.knotslist.end(), degree + 1, 1.0);

        RS_Spline source{nullptr, data};
        REQUIRE(source.validate());
        source.setClosed(true);
        REQUIRE(source.validate());
        data = source.getData();
        REQUIRE(data.type == RS_SplineData::SplineType::WrappedClosed);
        const LC_OffsetSourceDefinition definition{
            LC_OffsetSourceGeometry{data}, .01, 2e-3, false};
        REQUIRE(definition.isWellFormed());

        const req::Request generation{token, req::GenerateSource{definition, {}}};
        const auto generatedReply = LC_OffsetWorkerKernel::run(generation);
        const auto& generated = native(generatedReply, 7);
        INFO("degree=" << degree << " engine="
             << (generated.engineStatus ? int(*generated.engineStatus) : -1)
             << " ordinary=" << int(generated.ordinaryStatus)
             << " refusal=" << int(generated.refusal));
        REQUIRE(generated.engineStatus == LC_CurveOffsetStatus::Ok);
        REQUIRE(generated.ordinaryStatus == result::OrdinaryStatus::Complete);
        REQUIRE(generated.ordinary.size() == 1);
        REQUIRE(std::holds_alternative<RS_SplineData>(generated.ordinary.front()));
        const auto& cache = std::get<RS_SplineData>(generated.ordinary.front());

        const auto oracleDistance = maximumOracleOffsetDistance(
            data, cache, definition.signedDistance, 262144);
        REQUIRE(oracleDistance);
        const auto tolerance = LC_OffsetWorkerTolerance::directOptions(definition, source);
        REQUIRE(tolerance);
        CAPTURE(degree, *oracleDistance, tolerance->tolerance.requestedGeometry);
        CHECK(*oracleDistance <= 2.0 * tolerance->tolerance.requestedGeometry);

        const req::Request validation{token,
            req::ValidateSourceCache{cache, definition, {}}};
        const auto validationReply = LC_OffsetWorkerKernel::run(validation);
        const auto& validated = native(validationReply, 8);
        CHECK(validated.refusal == LC_OffsetCurveRefusal::None);
        CHECK(validated.changed);
        CHECK(validated.engineStatus == LC_CurveOffsetStatus::Ok);
    }
}

TEST_CASE("offset worker op8 accepts only the exact canonical source cache",
          "[lc-offsetcurve][worker-kernel][source-cache]") {
    const LC_OffsetSourceDefinition definition{
        LC_OffsetSourceGeometry{RS_LineData{{0, 0}, {4, 0}}}, .5, 1e-4, false};
    const auto generatedRequest = req::Request{token, req::GenerateSource{definition, {}}};
    const auto generatedReply = LC_OffsetWorkerKernel::run(generatedRequest);
    const auto& generated = native(generatedReply, 7);
    REQUIRE(generated.engineStatus == LC_CurveOffsetStatus::Ok);
    REQUIRE(generated.ordinaryStatus == result::OrdinaryStatus::Complete);
    REQUIRE(generated.ordinary.size() == 1);

    RS_SplineData cache;
    if (const auto* spline = std::get_if<RS_SplineData>(&generated.ordinary.front())) cache = *spline;
    else {
        const auto& line = std::get<req::Segment>(generated.ordinary.front());
        cache.degree = 1; cache.controlPoints = {line.start, line.end};
        cache.knotslist = {0, 0, 1, 1}; cache.weights = {1, 1};
    }
    const req::Request input{token, req::ValidateSourceCache{cache, definition, {}}};
    LC_OffsetWorkerKernel::Trace trace;
    const auto reply = LC_OffsetWorkerKernel::run(input, &trace);
    const auto& accepted = native(reply, 8);
    INFO("refusal=" << int(accepted.refusal) << " changed=" << accepted.changed << " runs=" << accepted.engineRuns
        << " status=" << (accepted.engineStatus ? int(*accepted.engineStatus) : -1)
        << " samples=" << accepted.sampleUnits << " attempted=" << accepted.attemptedOutputUsage.cubicPieces
        << "," << accepted.attemptedOutputUsage.outputEntities << "," << accepted.attemptedOutputUsage.deepEntities);
    CHECK(accepted.refusal == LC_OffsetCurveRefusal::None);
    CHECK(accepted.changed);
    CHECK(accepted.engineRuns == 1);
    CHECK(accepted.engineStatus == LC_CurveOffsetStatus::Ok);
    CHECK_FALSE(accepted.curve);
    CHECK(trace.nativePrepareEntered);
    CHECK(trace.computationCompleted);
    const auto wire = result::encode(reply, input);
    REQUIRE(wire);
    CHECK(result::decode(*wire, input));

    auto altered = std::get<req::ValidateSourceCache>(input.operation);
    altered.cache.controlPoints.back().y += 1;
    const auto rejectedRequest = req::Request{token, altered};
    const auto rejectedReply = LC_OffsetWorkerKernel::run(rejectedRequest);
    const auto& rejected = native(rejectedReply, 8);
    CHECK(rejected.refusal == LC_OffsetCurveRefusal::NotCacheRepresentable);
    CHECK_FALSE(rejected.changed);
    CHECK_FALSE(rejected.curve);
    const auto rejectedWire = result::encode(rejectedReply, rejectedRequest);
    REQUIRE(rejectedWire);
    CHECK(result::decode(*rejectedWire, rejectedRequest));
}

TEST_CASE("offset worker op8 promotes a wrapped periodic spline source when its cache is canonical",
          "[lc-offsetcurve][worker-kernel][source-cache][spline-variant]") {
    RS_Spline periodic{nullptr, RS_SplineData{3, false}};
    for (const RS_Vector& point : {RS_Vector{0, 0}, RS_Vector{40, -10}, RS_Vector{60, 30},
                                   RS_Vector{20, 50}, RS_Vector{-15, 25}})
        periodic.addControlPoint(point);
    periodic.setClosed(true);
    auto wrapped = periodic.getData();
    wrapped.weights.clear();
    const LC_OffsetSourceDefinition definition{
        LC_OffsetSourceGeometry{wrapped}, .25, 1e-4, false};

    const req::Request generatedRequest{token, req::GenerateSource{definition, {}}};
    const auto generatedReply = LC_OffsetWorkerKernel::run(generatedRequest);
    const auto& generated = native(generatedReply, 7);
    REQUIRE(generated.engineStatus == LC_CurveOffsetStatus::Ok);
    REQUIRE(generated.ordinaryStatus == result::OrdinaryStatus::Complete);
    REQUIRE(generated.ordinary.size() == 1);
    REQUIRE(std::holds_alternative<RS_SplineData>(generated.ordinary.front()));
    const auto cache = std::get<RS_SplineData>(generated.ordinary.front());

    const req::Request validationRequest{token, req::ValidateSourceCache{cache, definition, {}}};
    LC_OffsetWorkerKernel::Trace trace;
    const auto validationReply = LC_OffsetWorkerKernel::run(validationRequest, &trace);
    const auto& validated = native(validationReply, 8);
    CHECK(validated.refusal == LC_OffsetCurveRefusal::None);
    CHECK(validated.changed);
    CHECK(validated.engineRuns == 1);
    CHECK(trace.computationCompleted);
    const auto encoded = result::encode(validationReply, validationRequest);
    REQUIRE(encoded);
    CHECK(result::decode(*encoded, validationRequest));

    for (const double signedDistance : {-.5, -.25, .25, .5}) {
        CAPTURE(signedDistance);
        auto variantDefinition = definition;
        variantDefinition.signedDistance = signedDistance;
        const req::Request variantGeneration{token,
            req::GenerateSource{variantDefinition, {}}};
        const auto variantGeneratedReply = LC_OffsetWorkerKernel::run(variantGeneration);
        const auto& variantGenerated = native(variantGeneratedReply, 7);
        REQUIRE(variantGenerated.engineStatus == LC_CurveOffsetStatus::Ok);
        REQUIRE(variantGenerated.ordinaryStatus == result::OrdinaryStatus::Complete);
        REQUIRE(variantGenerated.ordinary.size() == 1);
        REQUIRE(std::holds_alternative<RS_SplineData>(variantGenerated.ordinary.front()));
        const auto& variantCache = std::get<RS_SplineData>(variantGenerated.ordinary.front());
        const req::Request variantValidation{token,
            req::ValidateSourceCache{variantCache, variantDefinition, {}}};
        const auto variantValidatedReply = LC_OffsetWorkerKernel::run(variantValidation);
        const auto& variantValidated = native(variantValidatedReply, 8);
        CHECK(variantValidated.refusal == LC_OffsetCurveRefusal::None);
        CHECK(variantValidated.changed);
    }

    auto altered = cache;
    altered.controlPoints.front().x += .125;
    const req::Request alteredRequest{token, req::ValidateSourceCache{altered, definition, {}}};
    const auto alteredReply = LC_OffsetWorkerKernel::run(alteredRequest);
    const auto& refused = native(alteredReply, 8);
    CHECK(refused.refusal == LC_OffsetCurveRefusal::NotCacheRepresentable);
    CHECK_FALSE(refused.changed);
}

TEST_CASE("Trimmed high-ratio periodic spline reports bounded self-noding refusal",
          "[lc-offsetcurve][worker-kernel][produce][spline-variant][periodic][rational]") {
    RS_Spline spline{nullptr, RS_SplineData{2, false}};
    for (const RS_Vector& point : {RS_Vector{0, 0}, RS_Vector{25, -15}, RS_Vector{55, -5}, RS_Vector{70, 20}, RS_Vector{50, 45},
                                   RS_Vector{15, 50}, RS_Vector{-15, 25}})
        spline.addControlPoint(point);
    spline.setKnotVector({0, 0, 0, .5, 1.5, 3, 4.5, 5, 5, 5});
    spline.setClosed(true);
    auto data = spline.getData();
    const std::vector<double> independentWeights{1e6, 1e4, 1e2, 1.0, 1e-2, 1e-4, 1e-6};
    data.weights = independentWeights;
    data.weights.insert(data.weights.end(), independentWeights.begin(), independentWeights.begin() + data.degree);
    RS_Spline source{nullptr, data};
    REQUIRE(source.validate());

    auto directOptions = LC_CurveOffset::makeDirectOptions(source, .005);
    auto direct = LC_CurveOffset::buildDirectBranches(source, LC_CurveOffset::makeSideRequest(LC_CurveOffsetSide::Left, .005),
                                                      directOptions, LC_CurveOffset::makeDirectSourceBudget());
    std::size_t directPieces = 0;
    for (const auto& branch : direct.branches)
        directPieces += branch.cubicPieces.size();
    auto options = LC_CurveOffset::makeOffsetOptions(source, .005);
    LC_CurveOffsetGeometryResult geometry = LC_CurveOffset::buildDirectBranches(
        source, LC_CurveOffset::makeSideRequest(LC_CurveOffsetSide::Left, .005), options, LC_CurveOffset::makeDirectSourceBudget());
    CAPTURE(direct.status, direct.exactSamples, direct.branches.size(), directPieces, geometry.status, geometry.exactSamples,
            geometry.intersectionDiagnostics.stage, geometry.intersectionDiagnostics.status,
            geometry.intersectionDiagnostics.work.candidateBoxPairs, geometry.intersectionDiagnostics.work.refinementBoxPairs,
            geometry.intersectionDiagnostics.work.totalBoxPairs, geometry.intersectionDiagnostics.work.generatedLeaves,
            geometry.intersectionDiagnostics.work.bvhUnanchoredPairs, geometry.intersectionDiagnostics.work.bvhDisjointPairs,
            geometry.intersectionDiagnostics.work.bvhMonotonePathPrunes, geometry.intersectionDiagnostics.work.bvhLeafCandidates,
            geometry.intersectionDiagnostics.work.bvhExpandedPairs, geometry.intersectionDiagnostics.work.bvhInterruptedPairs,
            geometry.intersectionDiagnostics.work.generatedItems, geometry.intersectionDiagnostics.failureDepth, geometry.removedIntervals,
            geometry.branches.size());
    CHECK(direct.status == LC_CurveOffsetStatus::Ok);
    CHECK_FALSE(direct.branches.empty());
    CHECK(geometry.status == LC_CurveOffsetStatus::LimitExceeded);
    CHECK(geometry.intersectionDiagnostics.stage == LC_CurveOffsetGeometryResult::IntersectionDiagnostics::Stage::OffsetSelfNoding);
    CHECK(geometry.intersectionDiagnostics.status == LC_IntersectionStatus::LimitExceeded);
    CHECK(geometry.intersectionDiagnostics.work.totalBoxPairs == options.maxIntersectionPairs);
    CHECK(geometry.intersectionDiagnostics.work.bvhUnanchoredPairs + geometry.intersectionDiagnostics.work.bvhDisjointPairs +
              geometry.intersectionDiagnostics.work.bvhMonotonePathPrunes +
              geometry.intersectionDiagnostics.work.bvhLeafCandidates +
              geometry.intersectionDiagnostics.work.bvhExpandedPairs +
              geometry.intersectionDiagnostics.work.bvhInterruptedPairs ==
          geometry.intersectionDiagnostics.work.candidateBoxPairs);
    CHECK(geometry.intersectionDiagnostics.work.generatedLeaves <= LC_IntersectionOptions{}.maxLeaves);
    CHECK(geometry.branches.empty());
}

TEST_CASE("worker validates nonuniform periodic spline caches on both offset sides",
          "[lc-offsetcurve][worker-kernel][source-cache][spline-variant][periodic][nonuniform]") {
    enum class WeightProfile {
        Mild,
        Wide,
        Alternating,
        ShiftedAlternating,
        Gradual,
        ReversedGradual
    };
    struct Variant {
        int degree;
        double scale;
        bool sevenControlQuadratic;
        WeightProfile weights = WeightProfile::Mild;
    };
    const std::vector<Variant> variants{
        {1, 1e-3, false}, {1, 1.0, false}, {1, 1e3, false},
        {2, 1.0, false}, {2, 1e-3, true}, {2, 1.0, true},
        {2, 1e3, true}, {2, 1.0, true, WeightProfile::Wide},
        {2, 1.0, true, WeightProfile::Alternating},
        {2, 1.0, true, WeightProfile::ShiftedAlternating},
        {2, 1.0, true, WeightProfile::Gradual},
        {2, 1.0, true, WeightProfile::ReversedGradual},
        {3, 1.0, false}};
    for (const auto& variant : variants) {
        const int degree = variant.degree;
        const double scale = variant.scale;
        const bool sevenControlQuadratic = variant.sevenControlQuadratic;
        const WeightProfile weightProfile = variant.weights;
        const bool wideWeights = weightProfile == WeightProfile::Wide;
        const bool conditionedWeights =
            weightProfile == WeightProfile::Alternating ||
            weightProfile == WeightProfile::ShiftedAlternating ||
            weightProfile == WeightProfile::Gradual ||
            weightProfile == WeightProfile::ReversedGradual;
        const bool gradualWeights = weightProfile == WeightProfile::Gradual ||
                                    weightProfile == WeightProfile::ReversedGradual;
        CAPTURE(degree, scale, sevenControlQuadratic, weightProfile);
        RS_Spline periodic{nullptr, RS_SplineData{degree, false}};
        const std::vector<RS_Vector> controls = sevenControlQuadratic
            ? std::vector<RS_Vector>{{0, 0}, {25, -15}, {55, -5}, {70, 20},
                                     {50, 45}, {15, 50}, {-15, 25}}
            : std::vector<RS_Vector>{{0, 0}, {30, -15}, {60, 0}, {55, 35},
                                     {20, 50}, {-15, 25}};
        for (const RS_Vector& point : controls)
            periodic.addControlPoint({point.x * scale + 1000.0 * scale,
                                      point.y * scale - 2000.0 * scale});
        if (degree == 1)
            periodic.setKnotVector({0, 0, .5, 1.5, 2.5, 4, 5, 5});
        else if (degree == 2 && sevenControlQuadratic)
            periodic.setKnotVector({0, 0, 0, .5, 1.5, 3, 4.5, 5, 5, 5});
        else if (degree == 2)
            periodic.setKnotVector({0, 0, 0, .5, 1.75, 2.5, 3, 3, 3});
        else
            periodic.setKnotVector({0, 0, 0, 0, .5, 2, 3, 3, 3, 3});
        REQUIRE(periodic.validate());
        periodic.setClosed(true);
        auto sourceData = periodic.getData();
        const std::vector<double> independentWeights = [&] {
            switch (weightProfile) {
            case WeightProfile::Wide:
                return std::vector<double>{0.01, 100.0, 0.1, 10.0, 0.5, 2.0, 1.0};
            case WeightProfile::Alternating:
                return std::vector<double>{1e-6, 1e6, 1e-4, 1e4, 1e-2, 1e2, 1.0};
            case WeightProfile::ShiftedAlternating:
                return std::vector<double>{1e6, 1e-6, 1e4, 1e-4, 1e2, 1e-2, 1.0};
            case WeightProfile::Gradual:
                return std::vector<double>{1e-6, 1e-4, 1e-2, 1.0, 1e2, 1e4, 1e6};
            case WeightProfile::ReversedGradual:
                return std::vector<double>{1e6, 1e4, 1e2, 1.0, 1e-2, 1e-4, 1e-6};
            case WeightProfile::Mild:
                return sevenControlQuadratic
                    ? std::vector<double>{1.0, 0.9, 1.1, 0.95, 1.05, 0.85, 1.15}
                    : std::vector<double>{1.0, 0.9, 1.1, 0.95, 1.05, 0.85};
            }
            return std::vector<double>{};
        }();
        sourceData.weights = independentWeights;
        sourceData.weights.insert(sourceData.weights.end(), independentWeights.begin(),
                                  independentWeights.begin() + sourceData.degree);
        RS_Spline sourceCheck{nullptr, sourceData};
        REQUIRE(sourceCheck.validate());
        REQUIRE(sourceCheck.hasWrappedControlPoints());

        double startParameter = 0.0;
        double endParameter = 0.0;
        REQUIRE(sourceCheck.getParameterDomain(startParameter, endParameter));
        LC_CurveJet startJet;
        LC_CurveJet endJet;
        REQUIRE(sourceCheck.tryEvaluateJet(startParameter, LC_CurveEvaluationSide::Right,
                                           startJet));
        REQUIRE(sourceCheck.tryEvaluateJet(endParameter, LC_CurveEvaluationSide::Left,
                                           endJet));
        CHECK(startJet.point.distanceTo(endJet.point) < 1e-8);
        if (degree > 1)
            CHECK(startJet.first.distanceTo(endJet.first) < 1e-7);

        const std::vector<double> signedDistances = gradualWeights
            ? std::vector<double>{.005 * scale, -.005 * scale}
            : degree == 1 || sevenControlQuadratic
            ? std::vector<double>{.0625 * scale, -.0625 * scale, .125 * scale,
                                  -.125 * scale, .25 * scale, -.25 * scale}
            : std::vector<double>{.125, -.125};
        for (const double signedDistance : signedDistances) {
            CAPTURE(signedDistance);
            const std::size_t lastControl = sourceData.controlPoints.size() - 1;
            const SplineOracle domainStart{startParameter};
            const SplineOracle domainEnd{endParameter};
            const std::vector<OracleSplineCusp> expectedCusps =
                conditionedWeights
                    ? oracleOffsetCusps(sourceData, domainStart, domainEnd, signedDistance)
                    : std::vector<OracleSplineCusp>{};
            if (conditionedWeights) {
                CAPTURE(weightProfile, signedDistance, expectedCusps.size());
                std::size_t expectedSampledCandidates = 0;
                if (signedDistance > 0.0) {
                    switch (weightProfile) {
                    case WeightProfile::Alternating:
                        expectedSampledCandidates = 8;
                        break;
                    case WeightProfile::ShiftedAlternating:
                        expectedSampledCandidates = 10;
                        break;
                    case WeightProfile::Gradual:
                        expectedSampledCandidates = 8;
                        break;
                    case WeightProfile::ReversedGradual:
                        expectedSampledCandidates = 7;
                        break;
                    case WeightProfile::Mild:
                    case WeightProfile::Wide:
                        break;
                    }
                }
                CHECK(expectedCusps.size() == expectedSampledCandidates);
                for (const OracleSplineCusp& cusp : expectedCusps) {
                    REQUIRE(cusp.span + 1 < sourceData.knotslist.size());
                    const SplineOracle spanStart{sourceData.knotslist[cusp.span]};
                    const SplineOracle spanEnd{sourceData.knotslist[cusp.span + 1]};
                    const SplineOracle spanWidth = spanEnd - spanStart;
                    REQUIRE(spanWidth > 0);
                    const SplineOracle probe = spanWidth * SplineOracle{"1e-7"};
                    const SplineOracle derivativeStep = spanWidth * SplineOracle{"1e-20"};
                    const SplineOracle leftParameter = std::max(
                        spanStart, cusp.parameter - probe);
                    const SplineOracle rightParameter = std::min(
                        spanEnd, cusp.parameter + probe);
                    int leftSign = 0;
                    int rightSign = 0;
                    if (leftParameter < cusp.parameter) {
                        const SplineOracle factor = oracleOffsetFactor(
                            sourceData, leftParameter, derivativeStep, domainStart,
                            domainEnd, SplineOracle{signedDistance});
                        leftSign = factor < 0 ? -1 : factor > 0 ? 1 : 0;
                        CHECK(leftSign != 0);
                    }
                    if (cusp.parameter < rightParameter) {
                        const SplineOracle factor = oracleOffsetFactor(
                            sourceData, rightParameter, derivativeStep, domainStart,
                            domainEnd, SplineOracle{signedDistance});
                        rightSign = factor < 0 ? -1 : factor > 0 ? 1 : 0;
                        CHECK(rightSign != 0);
                    }
                    CAPTURE(cusp.span, cusp.parameter, leftSign, rightSign,
                            leftParameter, rightParameter);
                    CHECK(((leftSign != 0) || (rightSign != 0)));
                }
            }
            if (degree > 1) {
                const SplineOracle seamStep{"1e-20"};
                const SplineOracle seamRight = oracleOffsetFactor(
                    sourceData, domainStart, seamStep, domainStart, domainEnd,
                    SplineOracle{signedDistance}, false);
                const SplineOracle seamLeft = oracleOffsetFactor(
                    sourceData, domainEnd, seamStep, domainStart, domainEnd,
                    SplineOracle{signedDistance}, true);
                CAPTURE(weightProfile, seamRight, seamLeft);
                CHECK((seamRight == 0) == (seamLeft == 0));
                CHECK((seamRight < 0) == (seamLeft < 0));
            }
            if (weightProfile == WeightProfile::Gradual && signedDistance > 0.0) {
                // This is a sampled sign-change inventory, not a completeness proof.
                CHECK(expectedCusps.size() == 8);
                CHECK(std::any_of(expectedCusps.begin(), expectedCusps.end(),
                                  [](const OracleSplineCusp& cusp) {
                                      return abs(cusp.parameter - SplineOracle{"0.500122"}) <
                                             SplineOracle{"1e-5"};
                                  }));
                for (const auto& cusp : expectedCusps) {
                    const bool atKnot = std::any_of(
                        sourceData.knotslist.begin(), sourceData.knotslist.end(),
                        [&](double knot) { return cusp.parameter == SplineOracle{knot}; });
                    CHECK_FALSE(atKnot);
                }

                double previousKnot = -std::numeric_limits<double>::infinity();
                for (std::size_t index = sourceData.degree + 1;
                     index <= lastControl; ++index) {
                    const double knotValue = sourceData.knotslist[index];
                    if (knotValue == previousKnot) continue;
                    previousKnot = knotValue;
                    const SplineOracle knot{knotValue};
                    SplineOracle leftWidth{};
                    SplineOracle rightWidth{};
                    for (double candidateValue : sourceData.knotslist) {
                        const SplineOracle candidate{candidateValue};
                        if (candidate < knot &&
                            (leftWidth == 0 || knot - candidate < leftWidth))
                            leftWidth = knot - candidate;
                        if (candidate > knot &&
                            (rightWidth == 0 || candidate - knot < rightWidth))
                            rightWidth = candidate - knot;
                    }
                    if (leftWidth == 0 || rightWidth == 0) continue;
                    const SplineOracle step = std::min(leftWidth, rightWidth) *
                                              SplineOracle{"1e-20"};
                    const SplineOracle leftStencil = oracleStencilStep(
                        sourceData, knot, step, true, false, 3);
                    const SplineOracle rightStencil = oracleStencilStep(
                        sourceData, knot, step, false, true, 3);
                    CHECK(3 * leftStencil < leftWidth);
                    CHECK(3 * rightStencil < rightWidth);
                    const SplineOracle left = oracleOffsetFactor(
                        sourceData, knot, step, domainStart, domainEnd,
                        SplineOracle{signedDistance}, true);
                    const SplineOracle right = oracleOffsetFactor(
                        sourceData, knot, step, domainStart, domainEnd,
                        SplineOracle{signedDistance}, false);
                    const SplineOracle leftRefined = oracleOffsetFactor(
                        sourceData, knot, step / 2, domainStart, domainEnd,
                        SplineOracle{signedDistance}, true);
                    const SplineOracle rightRefined = oracleOffsetFactor(
                        sourceData, knot, step / 2, domainStart, domainEnd,
                        SplineOracle{signedDistance}, false);
                    CHECK(abs(left - leftRefined) < SplineOracle{"1e-12"});
                    CHECK(abs(right - rightRefined) < SplineOracle{"1e-12"});
                }
            }
            const LC_OffsetSourceDefinition definition{
                LC_OffsetSourceGeometry{sourceData}, signedDistance, 1e-4, false};
            REQUIRE(definition.isWellFormed());
            const req::Request generationRequest{token,
                req::GenerateSource{definition, {}}};
            const auto generationReply = LC_OffsetWorkerKernel::run(generationRequest);
            const auto& generated = native(generationReply, 7);
            LC_CurveOffsetCuspIsolationDiagnostics cuspDiagnostics;
            if (conditionedWeights &&
                !(weightProfile == WeightProfile::Gradual && signedDistance > 0.0)) {
                const auto diagnosticOptions =
                    LC_OffsetWorkerTolerance::directOptions(definition, sourceCheck);
                REQUIRE(diagnosticOptions);
                LC_CurveOffsetCuspIsolationDiagnostics profileDiagnostics;
                const auto diagnostic = LC_CurveOffset::buildDirectBranches(
                    sourceCheck,
                    LC_CurveOffset::makeSideRequest(
                        signedDistance > 0.0 ? LC_CurveOffsetSide::Left
                                             : LC_CurveOffsetSide::Right,
                        std::abs(signedDistance)),
                    *diagnosticOptions, LC_CurveOffset::makeDirectSourceBudget(),
                    &profileDiagnostics);
                CAPTURE(weightProfile, signedDistance, generated.engineStatus,
                        diagnostic.status, expectedCusps.size(),
                        profileDiagnostics.candidateParameters,
                        profileDiagnostics.status,
                        profileDiagnostics.failureSpan, profileDiagnostics.failureT0,
                        profileDiagnostics.failureT1);
                REQUIRE(generated.engineStatus.has_value());
                CHECK(diagnostic.status == *generated.engineStatus);
                CHECK(profileDiagnostics.complete ==
                      (profileDiagnostics.status == LC_CurveOffsetStatus::Ok));
                LC_CurveOffsetStatus expectedStatus = LC_CurveOffsetStatus::ToleranceNotMet;
                switch (weightProfile) {
                case WeightProfile::Alternating:
                case WeightProfile::ShiftedAlternating:
                case WeightProfile::Gradual:
                case WeightProfile::ReversedGradual:
                    expectedStatus = LC_CurveOffsetStatus::Ok;
                    break;
                case WeightProfile::Mild:
                case WeightProfile::Wide:
                    break;
                }
                CHECK(diagnostic.status == expectedStatus);
                if (expectedStatus != LC_CurveOffsetStatus::Ok)
                    CHECK(diagnostic.branches.empty());

                const std::vector<OracleSplineCusp> refinedOracleCusps =
                    oracleOffsetCusps(sourceData, domainStart, domainEnd, signedDistance,
                                      SplineOracle{"1e-25"});
                CHECK(refinedOracleCusps.size() == expectedCusps.size());
                for (const OracleSplineCusp& oracleCusp : expectedCusps) {
                    double nearestRefinedParameter = std::numeric_limits<double>::infinity();
                    for (const OracleSplineCusp& refinedCusp : refinedOracleCusps) {
                        nearestRefinedParameter = std::min(
                            nearestRefinedParameter,
                            std::abs(oracleCusp.parameter.convert_to<double>() -
                                     refinedCusp.parameter.convert_to<double>()));
                    }
                    CHECK(nearestRefinedParameter < 1e-8);
                }
                std::vector<bool> matchedOracleCusps(refinedOracleCusps.size(), false);
                for (const double productionParameter : profileDiagnostics.candidateParameters) {
                    double nearestParameterError = std::numeric_limits<double>::infinity();
                    double nearestPointError = std::numeric_limits<double>::infinity();
                    std::size_t nearestOracle = 0;
                    for (std::size_t i = 0; i < refinedOracleCusps.size(); ++i) {
                        const double error = std::abs(
                            productionParameter -
                            refinedOracleCusps[i].parameter.convert_to<double>());
                        if (error < nearestParameterError) {
                            nearestParameterError = error;
                        }
                    }
                    const auto productionPoint = oracleOffsetPoint(
                        sourceData, SplineOracle{productionParameter}, SplineOracle{"1e-25"},
                        domainStart, domainEnd, SplineOracle{signedDistance});
                    const RS_Vector productionOffsetPoint{
                        productionPoint.first.convert_to<double>(),
                        productionPoint.second.convert_to<double>()};
                    for (std::size_t i = 0; i < refinedOracleCusps.size(); ++i) {
                        const double error = productionOffsetPoint.distanceTo(
                            refinedOracleCusps[i].point);
                        if (error < nearestPointError) {
                            nearestPointError = error;
                            nearestOracle = i;
                        }
                    }
                    CAPTURE(weightProfile, signedDistance, productionParameter,
                            nearestParameterError, nearestPointError);
                    CHECK(nearestParameterError < 1e-5);
                    CHECK(nearestPointError < diagnosticOptions->tolerance.requestedGeometry);
                    if (nearestPointError < diagnosticOptions->tolerance.requestedGeometry) {
                        CHECK_FALSE(matchedOracleCusps[nearestOracle]);
                        matchedOracleCusps[nearestOracle] = true;
                        std::size_t productionSpan =
                            std::numeric_limits<std::size_t>::max();
                        const std::vector<double> sourceBreaks =
                            sourceCheck.getBreakParameters();
                        for (std::size_t span = 0; span + 1 < sourceBreaks.size(); ++span) {
                            const double spanStart = sourceBreaks[span];
                            const double spanEnd = sourceBreaks[span + 1];
                            if (spanStart < productionParameter &&
                                productionParameter < spanEnd) {
                                productionSpan = span;
                                break;
                            }
                        }
                        CAPTURE(productionSpan, refinedOracleCusps[nearestOracle].span);
                        CHECK(productionSpan != std::numeric_limits<std::size_t>::max());
                        const double oracleSpanStart = sourceData.knotslist[
                            refinedOracleCusps[nearestOracle].span];
                        const double oracleSpanEnd = sourceData.knotslist[
                            refinedOracleCusps[nearestOracle].span + 1];
                        CHECK(sourceBreaks[productionSpan] == oracleSpanStart);
                        CHECK(sourceBreaks[productionSpan + 1] == oracleSpanEnd);
                        std::vector<const LC_CurveOffsetCuspCandidateBox*> boxes;
                        for (const auto& box : profileDiagnostics.candidateBoxes) {
                            if (box.parameter == productionParameter)
                                boxes.push_back(&box);
                        }
                        REQUIRE(boxes.size() == 1);
                        const LC_CurveOffsetCuspCandidateBox& box = *boxes.front();
                        CHECK(box.t0 <= productionParameter);
                        CHECK(productionParameter <= box.t1);
                        CHECK(box.t0 <= box.t1);
                        CHECK(box.span == productionSpan);
                        CHECK(box.t0 >= sourceBreaks[box.span]);
                        CHECK(box.t1 <= sourceBreaks[box.span + 1]);
                        if (diagnostic.status == LC_CurveOffsetStatus::Ok &&
                            profileDiagnostics.complete) {
                            double nearestCuspEndpoint = std::numeric_limits<double>::infinity();
                            for (const LC_OffsetBranch& branch : diagnostic.branches) {
                                if (branch.cubicPieces.empty()) continue;
                                if (branch.startEnd == LC_OffsetBranchEnd::Cusp) {
                                    nearestCuspEndpoint = std::min(
                                        nearestCuspEndpoint,
                                        branch.cubicPieces.front().bezier.front().distanceTo(
                                            refinedOracleCusps[nearestOracle].point));
                                }
                                if (branch.endEnd == LC_OffsetBranchEnd::Cusp) {
                                    nearestCuspEndpoint = std::min(
                                        nearestCuspEndpoint,
                                        branch.cubicPieces.back().bezier.back().distanceTo(
                                            refinedOracleCusps[nearestOracle].point));
                                }
                            }
                            const bool mappedToCollapse = std::any_of(
                                profileDiagnostics.collapsedTinyReversals.begin(),
                                profileDiagnostics.collapsedTinyReversals.end(),
                                [&](const LC_CurveOffsetTinyReversalCollapse& collapse) {
                                    return collapse.sourceT0 <= productionParameter &&
                                           productionParameter <= collapse.sourceT1 &&
                                           collapse.extent <= diagnosticOptions->tolerance.nodeMerge &&
                                           collapse.replacementPoint.distanceTo(
                                               refinedOracleCusps[nearestOracle].point) <=
                                               diagnosticOptions->tolerance.requestedGeometry;
                                });
                            CAPTURE(weightProfile, signedDistance, productionParameter,
                                    nearestCuspEndpoint, mappedToCollapse);
                            CHECK((nearestCuspEndpoint <=
                                       diagnosticOptions->tolerance.requestedGeometry ||
                                   mappedToCollapse));
                        }
                    }
                }
                if (diagnostic.status == LC_CurveOffsetStatus::Ok &&
                    profileDiagnostics.complete) {
                    const double parameterStart = domainStart.convert_to<double>();
                    const double parameterEnd = domainEnd.convert_to<double>();
                    const double requestedGeometry =
                        diagnosticOptions->tolerance.requestedGeometry;
                    for (const auto& collapse : profileDiagnostics.collapsedTinyReversals) {
                        CAPTURE(weightProfile, signedDistance, collapse.sourceT0,
                                collapse.sourceT1, collapse.extent,
                                collapse.boundedByCusps,
                                collapse.replacementPoint.x,
                                collapse.replacementPoint.y);
                        CHECK(std::isfinite(collapse.sourceT0));
                        CHECK(std::isfinite(collapse.sourceT1));
                        CHECK(std::isfinite(collapse.extent));
                        CHECK(parameterStart <= collapse.sourceT0);
                        CHECK(collapse.sourceT0 <= parameterEnd);
                        CHECK(parameterStart <= collapse.sourceT1);
                        CHECK(collapse.sourceT1 <= parameterEnd);
                        const double maximumExtent =
                            (collapse.boundedByCusps ? 2.0 : 1.0) *
                            diagnosticOptions->tolerance.nodeMerge;
                        CHECK(collapse.extent <= maximumExtent);

                        double nearestOutputEndpoint =
                            std::numeric_limits<double>::infinity();
                        for (const LC_OffsetBranch& branch : diagnostic.branches) {
                            for (const LC_OffsetCubicPiece& piece : branch.cubicPieces) {
                                nearestOutputEndpoint = std::min(
                                    nearestOutputEndpoint,
                                    std::min(
                                        piece.bezier.front().distanceTo(
                                            collapse.replacementPoint),
                                        piece.bezier.back().distanceTo(
                                            collapse.replacementPoint)));
                            }
                        }
                        CHECK(nearestOutputEndpoint <= requestedGeometry);
                        if (collapse.boundedByCusps) {
                            const bool ownedByOracleCusp = std::any_of(
                                expectedCusps.begin(), expectedCusps.end(),
                                [&](const OracleSplineCusp& cusp) {
                                    const double parameter =
                                        cusp.parameter.convert_to<double>();
                                    return collapse.sourceT0 <= parameter &&
                                           parameter <= collapse.sourceT1 &&
                                           collapse.replacementPoint.distanceTo(
                                               cusp.point) <= requestedGeometry;
                                });
                            CHECK(ownedByOracleCusp);
                        }
                    }
                }
                if (profileDiagnostics.complete) {
                    CHECK(profileDiagnostics.candidateParameters.size() ==
                          refinedOracleCusps.size());
                    CHECK(profileDiagnostics.candidateBoxes.size() ==
                          profileDiagnostics.candidateParameters.size());
                    CHECK(std::all_of(matchedOracleCusps.begin(), matchedOracleCusps.end(),
                                      [](bool matched) { return matched; }));
                }
            }
            if (weightProfile == WeightProfile::Gradual && signedDistance > 0.0) {
                const auto diagnosticOptions =
                    LC_OffsetWorkerTolerance::directOptions(definition, sourceCheck);
                REQUIRE(diagnosticOptions);
                const auto diagnostic = LC_CurveOffset::buildDirectBranches(
                    sourceCheck,
                    LC_CurveOffset::makeSideRequest(LC_CurveOffsetSide::Left,
                                                    signedDistance),
                    *diagnosticOptions, LC_CurveOffset::makeDirectSourceBudget(),
                    &cuspDiagnostics);
                CHECK(generated.engineStatus.has_value());
                if (generated.engineStatus)
                    CHECK(diagnostic.status == *generated.engineStatus);
                CHECK(diagnostic.status == LC_CurveOffsetStatus::Ok);
                CHECK_FALSE(diagnostic.branches.empty());
                CHECK_FALSE(cuspDiagnostics.candidateParameters.empty());
                CHECK(cuspDiagnostics.complete);
                CHECK(cuspDiagnostics.candidateBoxes.size() ==
                      cuspDiagnostics.candidateParameters.size());
                const std::vector<double> sourceBreaks = sourceCheck.getBreakParameters();
                for (const double candidate : cuspDiagnostics.candidateParameters) {
                    std::vector<const LC_CurveOffsetCuspCandidateBox*> boxes;
                    for (const auto& box : cuspDiagnostics.candidateBoxes)
                        if (box.parameter == candidate) boxes.push_back(&box);
                    REQUIRE(boxes.size() == 1);
                    const LC_CurveOffsetCuspCandidateBox& box = *boxes.front();
                    CHECK(box.span + 1 < sourceBreaks.size());
                    CHECK(box.t0 <= candidate);
                    CHECK(candidate <= box.t1);
                    CHECK(sourceBreaks[box.span] <= box.t0);
                    CHECK(box.t1 <= sourceBreaks[box.span + 1]);
                }
                std::vector<bool> matchedOracleCusps(expectedCusps.size(), false);
                for (const double productionParameter : cuspDiagnostics.candidateParameters) {
                    double nearestParameterError = std::numeric_limits<double>::infinity();
                    std::size_t nearestOracle = 0;
                    for (std::size_t i = 0; i < expectedCusps.size(); ++i) {
                        const double error = std::abs(
                            productionParameter - expectedCusps[i].parameter.convert_to<double>());
                        if (error < nearestParameterError) {
                            nearestParameterError = error;
                            nearestOracle = i;
                        }
                    }
                    CAPTURE(productionParameter, nearestParameterError);
                    CHECK(nearestParameterError < 1e-6);
                    if (nearestParameterError < 1e-6) {
                        CHECK_FALSE(matchedOracleCusps[nearestOracle]);
                        matchedOracleCusps[nearestOracle] = true;
                    }
                }
                const std::size_t matchedCount = static_cast<std::size_t>(
                    std::count(matchedOracleCusps.begin(), matchedOracleCusps.end(), true));
                REQUIRE(expectedCusps.size() == 8);
                CHECK(cuspDiagnostics.candidateParameters.size() == expectedCusps.size());
                CHECK(matchedCount == expectedCusps.size());
                CHECK(std::all_of(matchedOracleCusps.begin(), matchedOracleCusps.end(),
                                  [](bool matched) { return matched; }));

                std::vector<RS_Vector> cuspEndpoints;
                for (const LC_OffsetBranch& branch : diagnostic.branches) {
                    if (branch.cubicPieces.empty()) continue;
                    if (branch.startEnd == LC_OffsetBranchEnd::Cusp)
                        cuspEndpoints.push_back(branch.cubicPieces.front().bezier.front());
                    if (branch.endEnd == LC_OffsetBranchEnd::Cusp)
                        cuspEndpoints.push_back(branch.cubicPieces.back().bezier.back());
                }
                for (std::size_t cusp = 0; cusp < expectedCusps.size(); ++cusp) {
                    double nearestEndpointError = std::numeric_limits<double>::infinity();
                    for (const RS_Vector& endpoint : cuspEndpoints)
                        nearestEndpointError = std::min(
                            nearestEndpointError, endpoint.distanceTo(expectedCusps[cusp].point));
                    CAPTURE(cusp, nearestEndpointError, cuspEndpoints.size());
                    bool representedByTinyCollapse = false;
                    for (const auto& collapse : cuspDiagnostics.collapsedTinyReversals) {
                        if (!collapse.boundedByCusps ||
                            std::min(std::abs(collapse.sourceT0 - expectedCusps[cusp].parameter.convert_to<double>()),
                                     std::abs(collapse.sourceT1 - expectedCusps[cusp].parameter.convert_to<double>())) >
                                1e-6 ||
                            collapse.replacementPoint.distanceTo(expectedCusps[cusp].point) >
                                diagnosticOptions->tolerance.requestedGeometry) {
                            continue;
                        }
                        double nearestBranchPoint = std::numeric_limits<double>::infinity();
                        for (const LC_OffsetBranch& branch : diagnostic.branches) {
                            for (const LC_OffsetCubicPiece& piece : branch.cubicPieces) {
                                for (const RS_Vector& point : piece.bezier) {
                                    nearestBranchPoint = std::min(
                                        nearestBranchPoint, point.distanceTo(collapse.replacementPoint));
                                }
                            }
                        }
                        representedByTinyCollapse =
                            nearestBranchPoint <= diagnosticOptions->tolerance.requestedGeometry;
                        CAPTURE(collapse.sourceT0, collapse.sourceT1, collapse.extent,
                                collapse.replacementPoint.x, collapse.replacementPoint.y,
                                nearestBranchPoint);
                    }
                    CHECK((nearestEndpointError <= diagnosticOptions->tolerance.requestedGeometry ||
                           representedByTinyCollapse));
                }
            }
            CAPTURE(generated.engineStatus.has_value(),
                    generated.engineStatus
                        ? static_cast<int>(*generated.engineStatus)
                        : -1);
            if (gradualWeights) {
                CHECK((signedDistance < 0.0
                          ? generated.engineStatus == LC_CurveOffsetStatus::Ok
                          : generated.engineStatus == LC_CurveOffsetStatus::Ok));
            }
            if (generated.engineStatus != LC_CurveOffsetStatus::Ok) {
                REQUIRE((wideWeights || conditionedWeights));
                if (conditionedWeights) {
                    CHECK((generated.engineStatus == LC_CurveOffsetStatus::SingularOffset ||
                           generated.engineStatus == LC_CurveOffsetStatus::AmbiguousRegularity ||
                           generated.engineStatus == LC_CurveOffsetStatus::ToleranceNotMet));
                } else {
                    CHECK((generated.engineStatus == LC_CurveOffsetStatus::ToleranceNotMet ||
                           generated.engineStatus == LC_CurveOffsetStatus::SingularOffset));
                }
                CHECK(generated.refusal == LC_OffsetCurveRefusal::Engine);
                CHECK(generated.ordinaryStatus == result::OrdinaryStatus::None);
                CHECK(generated.ordinary.empty());
                CHECK_FALSE(generated.changed);
                const auto generatedWire = result::encode(generationReply, generationRequest);
                REQUIRE(generatedWire);
                CHECK(result::decode(*generatedWire, generationRequest));
                continue;
            }
            REQUIRE(generated.engineStatus == LC_CurveOffsetStatus::Ok);
            REQUIRE(generated.ordinaryStatus == result::OrdinaryStatus::Complete);
            const auto generatedWire = result::encode(generationReply, generationRequest);
            REQUIRE(generatedWire);
            CHECK(result::decode(*generatedWire, generationRequest));

            if (conditionedWeights) {
                CHECK(generated.ordinary.size() <= 256);
                const auto offsetOptions =
                    LC_OffsetWorkerTolerance::directOptions(definition, sourceCheck);
                REQUIRE(offsetOptions);
                const double geometryTolerance =
                    offsetOptions->tolerance.requestedGeometry;
                std::vector<RS_Vector> expectedPoints;
                std::vector<std::array<RS_Vector, 2>> expectedSegments;
                for (std::size_t span = sourceData.degree; span <= lastControl; ++span) {
                    const SplineOracle start{sourceData.knotslist[span]};
                    const SplineOracle end{sourceData.knotslist[span + 1]};
                    if (!(start < end)) continue;
                    const SplineOracle width = end - start;
                    std::vector<SplineOracle> parameters;
                    parameters.reserve(1100);
                    for (int sample = 0; sample <= 1024; ++sample)
                        parameters.push_back(start + width * sample / 1024);
                    for (int exponent = 4; exponent <= 48; ++exponent) {
                        const SplineOracle fraction{std::ldexp(1.0, -exponent)};
                        parameters.push_back(start + width * fraction);
                        parameters.push_back(end - width * fraction);
                    }
                    std::sort(parameters.begin(), parameters.end());
                    parameters.erase(std::unique(parameters.begin(), parameters.end()),
                                     parameters.end());
                    std::vector<RS_Vector> spanPoints;
                    for (const SplineOracle& t : parameters) {
                        const auto point = oracleOffsetPoint(
                            sourceData, t, width * SplineOracle{"1e-20"},
                            domainStart, domainEnd, SplineOracle{signedDistance});
                        spanPoints.emplace_back(point.first.convert_to<double>(),
                                                point.second.convert_to<double>());
                    }
                    for (std::size_t i = 1; i < spanPoints.size(); ++i)
                        expectedSegments.push_back({spanPoints[i - 1], spanPoints[i]});
                    expectedPoints.insert(expectedPoints.end(), spanPoints.begin(), spanPoints.end());
                }
                std::vector<RS_Vector> generatedPoints;
                std::vector<RS_Vector> generatedEndpoints;
                std::vector<std::array<RS_Vector, 2>> generatedSegments;
                for (const auto& item : generated.ordinary) {
                    REQUIRE(std::holds_alternative<RS_SplineData>(item));
                    RS_Spline piece{nullptr, std::get<RS_SplineData>(item)};
                    REQUIRE(piece.validate());
                    double pieceStart = 0.0;
                    double pieceEnd = 0.0;
                    REQUIRE(piece.getParameterDomain(pieceStart, pieceEnd));
                    LC_CurveJet endpoint;
                    REQUIRE(piece.tryEvaluateJet(pieceStart, LC_CurveEvaluationSide::Right,
                                                 endpoint));
                    generatedEndpoints.push_back(endpoint.point);
                    REQUIRE(piece.tryEvaluateJet(pieceEnd, LC_CurveEvaluationSide::Left,
                                                 endpoint));
                    generatedEndpoints.push_back(endpoint.point);
                    const std::vector<double> breaks = piece.getBreakParameters();
                    REQUIRE(breaks.size() >= 2);
                    for (std::size_t span = 0; span + 1 < breaks.size(); ++span) {
                        RS_Vector previousPoint;
                        for (int sample = 0; sample <= 32; ++sample) {
                            const double t = breaks[span] +
                                (breaks[span + 1] - breaks[span]) * sample / 32.0;
                            LC_CurveJet jet;
                            const auto side = sample == 32 ? LC_CurveEvaluationSide::Left
                                                           : LC_CurveEvaluationSide::Interior;
                            REQUIRE(piece.tryEvaluateJet(t, side, jet));
                            generatedPoints.push_back(jet.point);
                            if (sample > 0)
                                generatedSegments.push_back({previousPoint, jet.point});
                            previousPoint = jet.point;
                        }
                    }
                }
                REQUIRE_FALSE(expectedPoints.empty());
                REQUIRE_FALSE(generatedPoints.empty());
                struct DistanceWitness {
                    double distance = 0.0;
                    RS_Vector point;
                    RS_Vector nearest;
                };
                const auto pointToSegment = [](const RS_Vector& point,
                                               const std::array<RS_Vector, 2>& segment) {
                    const double dx = segment[1].x - segment[0].x;
                    const double dy = segment[1].y - segment[0].y;
                    const double lengthSquared = dx * dx + dy * dy;
                    const double fraction = lengthSquared > 0.0
                        ? std::clamp(((point.x - segment[0].x) * dx +
                                      (point.y - segment[0].y) * dy) / lengthSquared,
                                     0.0, 1.0)
                        : 0.0;
                    const RS_Vector nearest{segment[0].x + fraction * dx,
                                            segment[0].y + fraction * dy};
                    return std::pair{point.distanceTo(nearest), nearest};
                };
                const auto directedDistance = [&](const std::vector<RS_Vector>& from,
                                                  const std::vector<std::array<RS_Vector, 2>>& to) {
                    DistanceWitness witness;
                    for (const RS_Vector& point : from) {
                        double nearest = std::numeric_limits<double>::infinity();
                        RS_Vector nearestPoint;
                        for (const auto& segment : to)
                            if (const auto [distance, candidate] = pointToSegment(point, segment);
                                distance < nearest) {
                                nearest = distance;
                                nearestPoint = candidate;
                            }
                        if (nearest > witness.distance)
                            witness = {nearest, point, nearestPoint};
                    }
                    return witness;
                };
                const DistanceWitness forward = directedDistance(generatedPoints, expectedSegments);
                const DistanceWitness reverse = directedDistance(expectedPoints, generatedSegments);
                INFO("sampled offset Hausdorff bounds " << forward.distance << " at "
                     << forward.point.x << ", " << forward.point.y << " vs "
                     << forward.nearest.x << ", " << forward.nearest.y << "; reverse "
                     << reverse.distance << " at " << reverse.point.x << ", "
                     << reverse.point.y << " vs " << reverse.nearest.x << ", "
                     << reverse.nearest.y << "; points " << generatedPoints.size() << ", "
                     << expectedPoints.size());
                CHECK(forward.distance < geometryTolerance);
                CHECK(reverse.distance < geometryTolerance);
                double minimumCuspSeparation = std::numeric_limits<double>::infinity();
                for (std::size_t i = 0; i < expectedCusps.size(); ++i)
                    for (std::size_t j = i + 1; j < expectedCusps.size(); ++j)
                        minimumCuspSeparation = std::min(
                            minimumCuspSeparation,
                            expectedCusps[i].point.distanceTo(expectedCusps[j].point));
                INFO("oracle cusps=" << expectedCusps.size()
                     << ", minimum cusp separation=" << minimumCuspSeparation
                     << ", generated spline endpoints=" << generatedEndpoints.size());
                if (expectedCusps.size() > 1)
                    CHECK(minimumCuspSeparation > 1e-8);
                for (const OracleSplineCusp& cusp : expectedCusps) {
                    CAPTURE(cusp.parameter, weightProfile, signedDistance);
                    REQUIRE(cusp.span + 1 < sourceData.knotslist.size());
                    const SplineOracle ownerStart{sourceData.knotslist[cusp.span]};
                    const SplineOracle ownerEnd{sourceData.knotslist[cusp.span + 1]};
                    REQUIRE(ownerStart < ownerEnd);
                    CHECK(cusp.parameter >= ownerStart);
                    CHECK(cusp.parameter <= ownerEnd);
                    double nearestEndpoint = std::numeric_limits<double>::infinity();
                    RS_Vector nearestEndpointPoint;
                    for (const RS_Vector& endpoint : generatedEndpoints) {
                        const double distance = cusp.point.distanceTo(endpoint);
                        if (distance < nearestEndpoint) {
                            nearestEndpoint = distance;
                            nearestEndpointPoint = endpoint;
                        }
                    }
                    INFO("oracle cusp point=" << cusp.point.x << "," << cusp.point.y
                         << " nearest generated endpoint=" << nearestEndpointPoint.x << ","
                         << nearestEndpointPoint.y);
                    INFO("oracle cusp nearest branch endpoint=" << nearestEndpoint);
                    bool representedByTinyCollapse = false;
                    for (const auto& collapse : cuspDiagnostics.collapsedTinyReversals) {
                        const double parameter = cusp.parameter.convert_to<double>();
                        const double parameterError = std::min(std::abs(collapse.sourceT0 - parameter),
                                                               std::abs(collapse.sourceT1 - parameter));
                        const double sourceGeometryError =
                            collapse.replacementPoint.distanceTo(cusp.point);
                        if (!collapse.boundedByCusps || parameterError > 1e-6 ||
                            collapse.extent > 2.0 * offsetOptions->tolerance.nodeMerge ||
                            sourceGeometryError > geometryTolerance) {
                            continue;
                        }
                        double outputGeometryError = std::numeric_limits<double>::infinity();
                        for (const auto& segment : generatedSegments) {
                            const auto [distance, point] = pointToSegment(collapse.replacementPoint, segment);
                            (void)point;
                            outputGeometryError = std::min(outputGeometryError, distance);
                        }
                        CAPTURE(parameterError, collapse.extent, sourceGeometryError,
                                outputGeometryError);
                        representedByTinyCollapse = outputGeometryError < geometryTolerance;
                    }
                    CHECK((nearestEndpoint < geometryTolerance || representedByTinyCollapse));
                }
            }

            if ((degree == 1 || wideWeights) && generated.ordinary.size() > 1) {
                REQUIRE(generated.ordinary.size() == (degree == 1 ? 6 : 8));
                for (const auto& item : generated.ordinary) {
                    CAPTURE(item.index());
                    CHECK((std::holds_alternative<req::Segment>(item) ||
                           std::holds_alternative<RS_SplineData>(item)));
                }
            }

            RS_SplineData cache;
            REQUIRE(LC_OffsetWorkerSourceCache::combine(generated.ordinary, cache));
            RS_Spline checkedCache{nullptr, cache};
            REQUIRE(checkedCache.validate());
            const req::Request validationRequest{token,
                req::ValidateSourceCache{cache, definition, {}}};
            const auto validationReply = LC_OffsetWorkerKernel::run(validationRequest);
            const auto& validated = native(validationReply, 8);
            CHECK(validated.refusal == LC_OffsetCurveRefusal::None);
            CHECK(validated.changed);
            CHECK(validated.engineRuns == 1);
            const auto validationWire = result::encode(validationReply, validationRequest);
            REQUIRE(validationWire);
            CHECK(result::decode(*validationWire, validationRequest));
        }
    }
}

TEST_CASE("span-local spline oracle does not infer a root across a repeated knot",
          "[lc-offsetcurve][worker-kernel][spline-variant][oracle][repeated-knot]") {
    RS_SplineData data{3, false};
    data.type = RS_SplineData::SplineType::ClampedOpen;
    data.controlPoints = {{0, 0}, {1, 3}, {2, -2}, {3, 1}, {4, 4}, {5, -3}, {6, 0}};
    data.knotslist = {0, 0, 0, 0, 1, 1, 2, 3, 3, 3, 3};
    data.weights = {1.0, .8, 1.3, .9, 1.2, .7, 1.0};
    const RS_Spline source{nullptr, data};
    REQUIRE(source.validate());

    const SplineOracle domainStart{0};
    const SplineOracle domainEnd{3};
    const SplineOracle repeatedKnot{1};
    bool observedOneSidedSignJump = false;
    double signJumpDistance = 0.0;
    for (const double signedDistance : {.1, .25, .5, 1.0, 2.0, 5.0,
                                        -.1, -.25, -.5, -1.0, -2.0, -5.0}) {
        CAPTURE(signedDistance);
        const SplineOracle step{"1e-24"};
        const SplineOracle leftFactor = oracleOffsetFactor(
            data, repeatedKnot, step, domainStart, domainEnd,
            SplineOracle{signedDistance}, true);
        const SplineOracle rightFactor = oracleOffsetFactor(
            data, repeatedKnot, step, domainStart, domainEnd,
            SplineOracle{signedDistance}, false);
        const SplineOracle leftRefined = oracleOffsetFactor(
            data, repeatedKnot, step / 2, domainStart, domainEnd,
            SplineOracle{signedDistance}, true);
        const SplineOracle rightRefined = oracleOffsetFactor(
            data, repeatedKnot, step / 2, domainStart, domainEnd,
            SplineOracle{signedDistance}, false);
        CHECK(abs(leftFactor - leftRefined) < SplineOracle{"1e-20"});
        CHECK(abs(rightFactor - rightRefined) < SplineOracle{"1e-20"});
        if (leftFactor == 0 || rightFactor == 0 ||
            ((leftFactor < 0) == (rightFactor < 0))) {
            continue;
        }

        observedOneSidedSignJump = true;
        signJumpDistance = signedDistance;
        const std::vector<OracleSplineCusp> candidates =
            oracleOffsetCusps(data, domainStart, domainEnd, signedDistance);
        CHECK(std::none_of(candidates.begin(), candidates.end(), [&](const OracleSplineCusp& cusp) {
            return cusp.parameter == repeatedKnot;
        }));
        for (const OracleSplineCusp& cusp : candidates) {
            const SplineOracle spanStart{data.knotslist[cusp.span]};
            const SplineOracle spanEnd{data.knotslist[cusp.span + 1]};
            CHECK(spanStart < spanEnd);
            CHECK(cusp.parameter >= spanStart);
            CHECK(cusp.parameter <= spanEnd);
        }
    }
    CHECK(observedOneSidedSignJump);

    REQUIRE(observedOneSidedSignJump);
    const SplineOracle jetStep{"1e-24"};
    const auto leftOffsetPoint = oracleOffsetPoint(
        data, repeatedKnot, jetStep, domainStart, domainEnd,
        SplineOracle{signJumpDistance}, true);
    const auto rightOffsetPoint = oracleOffsetPoint(
        data, repeatedKnot, jetStep, domainStart, domainEnd,
        SplineOracle{signJumpDistance}, false);
    const RS_Vector expectedJoinPoint{
        ((leftOffsetPoint.first + rightOffsetPoint.first) / 2).convert_to<double>(),
        ((leftOffsetPoint.second + rightOffsetPoint.second) / 2).convert_to<double>()};
    CHECK(RS_Vector{leftOffsetPoint.first.convert_to<double>(),
                    leftOffsetPoint.second.convert_to<double>()}.distanceTo(
              RS_Vector{rightOffsetPoint.first.convert_to<double>(),
                        rightOffsetPoint.second.convert_to<double>()}) < 1e-8);

    const LC_OffsetSourceDefinition definition{
        LC_OffsetSourceGeometry{data}, std::abs(signJumpDistance), 1e-4, false};
    const auto options = LC_OffsetWorkerTolerance::directOptions(definition, source);
    REQUIRE(options);
    LC_CurveOffsetCuspIsolationDiagnostics diagnostics;
    const auto built = LC_CurveOffset::buildDirectBranches(
        source,
        LC_CurveOffset::makeSideRequest(
            signJumpDistance > 0.0 ? LC_CurveOffsetSide::Left : LC_CurveOffsetSide::Right,
            std::abs(signJumpDistance)),
        *options, LC_CurveOffset::makeDirectSourceBudget(), &diagnostics);
    CAPTURE(signJumpDistance, built.status, diagnostics.candidateParameters);
    REQUIRE(built.status == LC_CurveOffsetStatus::Ok);
    CHECK(std::none_of(diagnostics.candidateParameters.begin(),
                       diagnostics.candidateParameters.end(), [&](const double parameter) {
        return std::abs(parameter - repeatedKnot.convert_to<double>()) < 1e-12;
    }));
    CHECK(diagnostics.candidateBoxes.size() == diagnostics.candidateParameters.size());
    const std::vector<double> sourceBreaks = source.getBreakParameters();
    for (const double candidate : diagnostics.candidateParameters) {
        std::vector<const LC_CurveOffsetCuspCandidateBox*> boxes;
        for (const auto& box : diagnostics.candidateBoxes)
            if (box.parameter == candidate) boxes.push_back(&box);
        REQUIRE(boxes.size() == 1);
        const LC_CurveOffsetCuspCandidateBox& box = *boxes.front();
        CHECK(box.span + 1 < sourceBreaks.size());
        CHECK(box.t0 <= candidate);
        CHECK(candidate <= box.t1);
        CHECK(sourceBreaks[box.span] <= box.t0);
        CHECK(box.t1 <= sourceBreaks[box.span + 1]);
    }
    bool productionOwnsRepeatedKnotCusp = false;
    for (const LC_OffsetBranch& branch : built.branches) {
        if (branch.cubicPieces.empty()) continue;
        if (branch.startEnd == LC_OffsetBranchEnd::Cusp) {
            productionOwnsRepeatedKnotCusp |=
                branch.cubicPieces.front().bezier.front().distanceTo(expectedJoinPoint) <=
                options->tolerance.requestedGeometry;
        }
        if (branch.endEnd == LC_OffsetBranchEnd::Cusp) {
            productionOwnsRepeatedKnotCusp |=
                branch.cubicPieces.back().bezier.back().distanceTo(expectedJoinPoint) <=
                options->tolerance.requestedGeometry;
        }
    }
    CHECK(productionOwnsRepeatedKnotCusp);
}

TEST_CASE("quadratic offset-factor tangency is retained as a bounded stall",
          "[lc-offsetcurve][worker-kernel][spline-variant][oracle][tangent-root]") {
    RS_SplineData data{2, false};
    data.type = RS_SplineData::SplineType::ClampedOpen;
    data.controlPoints = {{-1, 1}, {0, -1}, {1, 1}};
    data.knotslist = {0, 0, 0, 1, 1, 1};
    data.weights = {1, 1, 1};
    const RS_Spline source{nullptr, data};
    REQUIRE(source.validate());

    const SplineOracle domainStart{0};
    const SplineOracle domainEnd{1};
    const SplineOracle tangentParameter{"0.5"};
    const SplineOracle derivativeStep{"1e-30"};
    const SplineOracle signedDistance{"0.5"};
    const SplineOracle tangentFactor = oracleOffsetFactor(
        data, tangentParameter, derivativeStep, domainStart, domainEnd, signedDistance);
    const SplineOracle leftFactor = oracleOffsetFactor(
        data, tangentParameter - SplineOracle{"1e-10"}, derivativeStep,
        domainStart, domainEnd, signedDistance);
    const SplineOracle rightFactor = oracleOffsetFactor(
        data, tangentParameter + SplineOracle{"1e-10"}, derivativeStep,
        domainStart, domainEnd, signedDistance);
    CHECK(abs(tangentFactor) < SplineOracle{"1e-200"});
    CHECK(leftFactor > 0);
    CHECK(rightFactor > 0);
    CHECK(leftFactor < SplineOracle{"1e-18"});
    CHECK(rightFactor < SplineOracle{"1e-18"});

    const LC_OffsetSourceDefinition definition{
        LC_OffsetSourceGeometry{data}, .5, 1e-4, false};
    const auto options = LC_OffsetWorkerTolerance::directOptions(definition, source);
    REQUIRE(options);
    LC_CurveOffsetCuspIsolationDiagnostics diagnostics;
    const auto built = LC_CurveOffset::buildDirectBranches(
        source,
        LC_CurveOffset::makeSideRequest(LC_CurveOffsetSide::Left, .5),
        *options, LC_CurveOffset::makeDirectSourceBudget(), &diagnostics);
    CAPTURE(built.status, diagnostics.stallIntervals, diagnostics.candidateParameters);
    REQUIRE(built.status == LC_CurveOffsetStatus::Ok);
    CHECK_FALSE(diagnostics.stallIntervals.empty());
    CHECK(diagnostics.candidateBoxes.empty());
    CHECK(std::any_of(diagnostics.stallIntervals.begin(), diagnostics.stallIntervals.end(),
                      [](const std::pair<double, double>& interval) {
        return interval.first <= .5 && interval.second >= .5;
    }));

    const auto oraclePoint = oracleOffsetPoint(
        data, tangentParameter, derivativeStep, domainStart, domainEnd, signedDistance);
    const RS_Vector expectedPoint{oraclePoint.first.convert_to<double>(),
                                  oraclePoint.second.convert_to<double>()};
    double nearestOutput = std::numeric_limits<double>::infinity();
    for (const LC_OffsetBranch& branch : built.branches) {
        for (const LC_OffsetCubicPiece& piece : branch.cubicPieces) {
            const auto& p = piece.bezier;
            for (int sample = 0; sample <= 100; ++sample) {
                const double t = sample / 100.0;
                const double u = 1.0 - t;
                const RS_Vector point = p[0] * (u * u * u) +
                                        p[1] * (3.0 * u * u * t) +
                                        p[2] * (3.0 * u * t * t) +
                                        p[3] * (t * t * t);
                nearestOutput = std::min(nearestOutput, point.distanceTo(expectedPoint));
            }
        }
    }
    CAPTURE(expectedPoint.x, expectedPoint.y, nearestOutput,
            options->tolerance.requestedGeometry);
    CHECK(nearestOutput <= options->tolerance.requestedGeometry);
}

TEST_CASE("simple spline cusp at an interior knot has explicit boundary ownership",
          "[lc-offsetcurve][worker-kernel][spline-variant][oracle][knot-root]") {
    RS_SplineData data{3, false};
    data.type = RS_SplineData::SplineType::ClampedOpen;
    data.controlPoints = {{-1, .25}, {-2.0 / 3.0, 1.0 / 6.0}, {0, -1.0 / 6.0},
                          {2.0 / 3.0, 1.0 / 6.0}, {1, .75}};
    data.knotslist = {0, 0, 0, 0, .5, 1, 1, 1, 1};
    data.weights = {1, 1, 1, 1, 1};
    const RS_Spline source{nullptr, data};
    REQUIRE(source.validate());

    const SplineOracle domainStart{0};
    const SplineOracle domainEnd{1};
    const SplineOracle knot{"0.5"};
    const SplineOracle step{"1e-24"};
    const SplineOracle distance{1};
    const SplineOracle leftFactor = oracleOffsetFactor(
        data, knot, step, domainStart, domainEnd, distance, true);
    const SplineOracle rightFactor = oracleOffsetFactor(
        data, knot, step, domainStart, domainEnd, distance, false);
    const SplineOracle leftNeighbor = oracleOffsetFactor(
        data, knot - SplineOracle{"1e-8"}, step, domainStart, domainEnd, distance);
    const SplineOracle rightNeighbor = oracleOffsetFactor(
        data, knot + SplineOracle{"1e-8"}, step, domainStart, domainEnd, distance);
    CAPTURE(leftFactor, rightFactor, leftNeighbor, rightNeighbor);
    CHECK(abs(leftFactor) < SplineOracle{"1e-16"});
    CHECK(abs(rightFactor) < SplineOracle{"1e-16"});
    CHECK(leftNeighbor > 0);
    CHECK(rightNeighbor < 0);

    const LC_OffsetSourceDefinition definition{
        LC_OffsetSourceGeometry{data}, 1.0, 1e-4, false};
    const auto options = LC_OffsetWorkerTolerance::directOptions(definition, source);
    REQUIRE(options);
    LC_CurveOffsetCuspIsolationDiagnostics diagnostics;
    const auto built = LC_CurveOffset::buildDirectBranches(
        source,
        LC_CurveOffset::makeSideRequest(LC_CurveOffsetSide::Left, 1.0),
        *options, LC_CurveOffset::makeDirectSourceBudget(), &diagnostics);
    CAPTURE(built.status, diagnostics.status, diagnostics.candidateParameters,
            diagnostics.candidateBoxes, diagnostics.failureSpan,
            diagnostics.failureT0, diagnostics.failureT1);
    REQUIRE(built.status == LC_CurveOffsetStatus::Ok);
    REQUIRE(diagnostics.complete);
    const std::vector<double> sourceBreaks = source.getBreakParameters();
    std::vector<const LC_CurveOffsetCuspCandidateBox*> knotBoxes;
    for (const auto& box : diagnostics.candidateBoxes) {
        if (std::abs(box.parameter - .5) < 1e-8)
            knotBoxes.push_back(&box);
    }
    REQUIRE_FALSE(knotBoxes.empty());
    for (const auto* box : knotBoxes) {
        CHECK(box->t0 <= box->parameter);
        CHECK(box->parameter <= box->t1);
        CHECK(box->span + 1 < sourceBreaks.size());
        CHECK(sourceBreaks[box->span] <= box->t0);
        CHECK(box->t1 <= sourceBreaks[box->span + 1]);
        CHECK((box->span == 0 || box->span == 1));
    }

    const auto leftPoint = oracleOffsetPoint(
        data, knot, step, domainStart, domainEnd, distance, true);
    const auto rightPoint = oracleOffsetPoint(
        data, knot, step, domainStart, domainEnd, distance, false);
    const RS_Vector expectedPoint{
        ((leftPoint.first + rightPoint.first) / 2).convert_to<double>(),
        ((leftPoint.second + rightPoint.second) / 2).convert_to<double>()};
    bool representedByEndpoint = false;
    for (const LC_OffsetBranch& branch : built.branches) {
        if (branch.cubicPieces.empty()) continue;
        if (branch.startEnd == LC_OffsetBranchEnd::Cusp)
            representedByEndpoint |= branch.cubicPieces.front().bezier.front().distanceTo(
                expectedPoint) <= options->tolerance.requestedGeometry;
        if (branch.endEnd == LC_OffsetBranchEnd::Cusp)
            representedByEndpoint |= branch.cubicPieces.back().bezier.back().distanceTo(
                expectedPoint) <= options->tolerance.requestedGeometry;
    }
    CHECK(representedByEndpoint);
}

TEST_CASE("periodic spline seam curvature is checked with both one-sided owners",
          "[lc-offsetcurve][worker-kernel][spline-variant][oracle][seam-root]") {
    RS_Spline periodic{nullptr, RS_SplineData{3, false}};
    const std::vector<RS_Vector> controls{
        {0, 0}, {30, -15}, {60, 0}, {55, 35}, {20, 50}, {-15, 25}};
    for (const RS_Vector& point : controls)
        periodic.addControlPoint(point);
    periodic.setKnotVector({0, 0, 0, 0, .5, 2, 3, 3, 3, 3});
    REQUIRE(periodic.validate());
    periodic.setClosed(true);
    RS_Spline source{nullptr, periodic.getData()};
    REQUIRE(source.validate());
    const RS_SplineData data = source.getData();

    double domainStartValue = 0.0;
    double domainEndValue = 0.0;
    REQUIRE(source.getParameterDomain(domainStartValue, domainEndValue));
    const SplineOracle domainStart{domainStartValue};
    const SplineOracle domainEnd{domainEndValue};
    const SplineOracle step{"1e-24"};
    const SplineOracle unitLeft = oracleOffsetFactor(
        data, domainEnd, step, domainStart, domainEnd, SplineOracle{1}, true);
    const SplineOracle unitRight = oracleOffsetFactor(
        data, domainStart, step, domainStart, domainEnd, SplineOracle{1}, false);
    const SplineOracle curvatureLeft = 1 - unitLeft;
    const SplineOracle curvatureRight = 1 - unitRight;
    CAPTURE(domainStartValue, domainEndValue, curvatureLeft, curvatureRight);
    REQUIRE(curvatureLeft > 0);
    REQUIRE(curvatureRight > 0);
    CHECK(abs(curvatureLeft - curvatureRight) < SplineOracle{"1e-15"});
    const double signedDistance =
        (2 / (curvatureLeft + curvatureRight)).convert_to<double>();
    REQUIRE(std::isfinite(signedDistance));
    const SplineOracle distance{signedDistance};
    const SplineOracle seamLeft = oracleOffsetFactor(
        data, domainEnd, step, domainStart, domainEnd, distance, true);
    const SplineOracle seamRight = oracleOffsetFactor(
        data, domainStart, step, domainStart, domainEnd, distance, false);
    CHECK(abs(seamLeft) < SplineOracle{"1e-12"});
    CHECK(abs(seamRight) < SplineOracle{"1e-12"});
    const SplineOracle beforeSeam = oracleOffsetFactor(
        data, domainEnd - SplineOracle{"1e-8"}, step,
        domainStart, domainEnd, distance);
    const SplineOracle afterSeam = oracleOffsetFactor(
        data, domainStart + SplineOracle{"1e-8"}, step,
        domainStart, domainEnd, distance);
    CHECK(beforeSeam != 0);
    CHECK(afterSeam != 0);

    const LC_OffsetSourceDefinition definition{
        LC_OffsetSourceGeometry{data}, signedDistance, 1e-4, false};
    const auto options = LC_OffsetWorkerTolerance::directOptions(definition, source);
    REQUIRE(options);
    LC_CurveOffsetCuspIsolationDiagnostics diagnostics;
    const auto built = LC_CurveOffset::buildDirectBranches(
        source,
        LC_CurveOffset::makeSideRequest(LC_CurveOffsetSide::Left, signedDistance),
        *options, LC_CurveOffset::makeDirectSourceBudget(), &diagnostics);
    CAPTURE(built.status, diagnostics.status, diagnostics.candidateParameters,
            diagnostics.candidateBoxes, diagnostics.stallIntervals,
            diagnostics.failureSpan, diagnostics.failureT0, diagnostics.failureT1);
    CHECK(built.status == LC_CurveOffsetStatus::Ok);
    CHECK(diagnostics.complete == (diagnostics.status == LC_CurveOffsetStatus::Ok));

    const std::vector<double> sourceBreaks = source.getBreakParameters();
    bool seamCandidate = false;
    bool seamOwnedByStart = false;
    bool seamOwnedByEnd = false;
    for (const auto& box : diagnostics.candidateBoxes) {
        CHECK(box.t0 <= box.parameter);
        CHECK(box.parameter <= box.t1);
        if (box.span != std::numeric_limits<std::size_t>::max()) {
            CHECK(box.span + 1 < sourceBreaks.size());
            CHECK(sourceBreaks[box.span] <= box.t0);
            CHECK(box.t1 <= sourceBreaks[box.span + 1]);
            const bool ownsStart = box.span == 0 &&
                box.t0 <= domainStartValue && box.t1 >= domainStartValue;
            const bool ownsEnd = box.span + 2 == sourceBreaks.size() &&
                box.t0 <= domainEndValue && box.t1 >= domainEndValue;
            CHECK((ownsStart || ownsEnd ||
                   (box.t0 > domainStartValue && box.t1 < domainEndValue)));
            if (std::abs(box.parameter - domainStartValue) < 1e-6 ||
                std::abs(box.parameter - domainEndValue) < 1e-6) {
                seamCandidate = true;
                seamOwnedByStart |= ownsStart;
                seamOwnedByEnd |= ownsEnd;
            }
        }
    }
    const bool seamStall = std::any_of(
        diagnostics.stallIntervals.begin(), diagnostics.stallIntervals.end(),
        [&](const std::pair<double, double>& interval) {
            return (interval.first <= domainStartValue && interval.second >= domainStartValue) ||
                   (interval.first <= domainEndValue && interval.second >= domainEndValue);
        });
    CHECK((seamCandidate || seamStall));
    if (seamCandidate) CHECK((seamOwnedByStart || seamOwnedByEnd));

    const auto startPoint = oracleOffsetPoint(
        data, domainStart, step, domainStart, domainEnd, distance, false);
    const auto endPoint = oracleOffsetPoint(
        data, domainEnd, step, domainStart, domainEnd, distance, true);
    const RS_Vector expectedPoint{
        ((startPoint.first + endPoint.first) / 2).convert_to<double>(),
        ((startPoint.second + endPoint.second) / 2).convert_to<double>()};
    double nearestOutput = std::numeric_limits<double>::infinity();
    double nearestBranchEndpoint = std::numeric_limits<double>::infinity();
    double nearestCuspEndpoint = std::numeric_limits<double>::infinity();
    for (const LC_OffsetBranch& branch : built.branches) {
        if (!branch.cubicPieces.empty()) {
            const RS_Vector& start = branch.cubicPieces.front().bezier.front();
            const RS_Vector& end = branch.cubicPieces.back().bezier.back();
            nearestBranchEndpoint = std::min(
                nearestBranchEndpoint,
                std::min(start.distanceTo(expectedPoint), end.distanceTo(expectedPoint)));
            if (branch.startEnd == LC_OffsetBranchEnd::Cusp) {
                nearestCuspEndpoint = std::min(nearestCuspEndpoint,
                                               start.distanceTo(expectedPoint));
            }
            if (branch.endEnd == LC_OffsetBranchEnd::Cusp) {
                nearestCuspEndpoint = std::min(nearestCuspEndpoint,
                                               end.distanceTo(expectedPoint));
            }
        }
        for (const LC_OffsetCubicPiece& piece : branch.cubicPieces) {
            const auto& p = piece.bezier;
            for (int sample = 0; sample <= 100; ++sample) {
                const double t = sample / 100.0;
                const double u = 1.0 - t;
                const RS_Vector point = p[0] * (u * u * u) +
                                        p[1] * (3.0 * u * u * t) +
                                        p[2] * (3.0 * u * t * t) +
                                        p[3] * (t * t * t);
                nearestOutput = std::min(nearestOutput, point.distanceTo(expectedPoint));
            }
        }
    }
    CAPTURE(expectedPoint.x, expectedPoint.y, nearestOutput,
            nearestBranchEndpoint, nearestCuspEndpoint,
            options->tolerance.requestedGeometry, beforeSeam, afterSeam);
    CHECK(nearestOutput <= options->tolerance.requestedGeometry);
    if (seamCandidate) {
        CHECK(nearestCuspEndpoint <= options->tolerance.requestedGeometry);
    }
    if (seamStall) {
        CHECK(nearestBranchEndpoint <= options->tolerance.requestedGeometry);
    }
}

TEST_CASE("periodic rational seam roots map across weight profiles",
          "[lc-offsetcurve][worker-kernel][spline-variant][oracle][periodic]["
          "rational][seam-root]") {
  const std::array<std::vector<double>, 2> weightProfiles{
      {{1, .8, 1.2, 1.5}, {1, .7, 1.4, 1.1}}};
  for (const auto &weights : weightProfiles) {
    CAPTURE(weights);
    RS_Spline base{nullptr, RS_SplineData{3, false}};
    for (const RS_Vector &point : {RS_Vector{0, 0}, RS_Vector{30, -15},
                                   RS_Vector{55, 15}, RS_Vector{15, 35}})
      base.addControlPoint(point);
    base.setKnotVector({0, 0, 0, 0, 1, 1, 1, 1});
    REQUIRE(base.validate());
    base.setClosed(true);
    RS_SplineData data = base.getData();
    data.weights = weights;
    data.weights.insert(data.weights.end(), weights.begin(),
                        weights.begin() + data.degree);
    const RS_Spline source{nullptr, data};
    REQUIRE(source.validate());
    REQUIRE(source.isClosed());

    double startValue = 0.0;
    double endValue = 0.0;
    REQUIRE(source.getParameterDomain(startValue, endValue));
    const SplineOracle start{startValue};
    const SplineOracle end{endValue};
    const SplineOracle step{"1e-24"};
    const SplineOracle unitLeft =
        oracleOffsetFactor(data, end, step, start, end, SplineOracle{1}, true);
    const SplineOracle unitRight = oracleOffsetFactor(
        data, start, step, start, end, SplineOracle{1}, false);
    const SplineOracle curvatureLeft = 1 - unitLeft;
    const SplineOracle curvatureRight = 1 - unitRight;
    REQUIRE(abs(curvatureLeft - curvatureRight) < SplineOracle{"1e-8"});
    REQUIRE(abs(curvatureLeft) > SplineOracle{"1e-12"});
    const double signedDistance =
        (2 / (curvatureLeft + curvatureRight)).convert_to<double>();
    const SplineOracle distance{signedDistance};
    const SplineOracle seamLeft =
        oracleOffsetFactor(data, end, step, start, end, distance, true);
    const SplineOracle seamRight =
        oracleOffsetFactor(data, start, step, start, end, distance, false);
    const SplineOracle before = oracleOffsetFactor(
        data, end - SplineOracle{"1e-7"}, step, start, end, distance);
    const SplineOracle after = oracleOffsetFactor(
        data, start + SplineOracle{"1e-7"}, step, start, end, distance);
    CAPTURE(startValue, endValue, signedDistance, seamLeft, seamRight, before,
            after);
    CHECK(abs(seamLeft) < SplineOracle{"1e-8"});
    CHECK(abs(seamRight) < SplineOracle{"1e-8"});
    CHECK(before != 0);
    CHECK(after != 0);
    CHECK((before < 0) != (after < 0));

    const LC_OffsetSourceDefinition definition{LC_OffsetSourceGeometry{data},
                                               signedDistance, 1e-4, false};
    const auto options =
        LC_OffsetWorkerTolerance::directOptions(definition, source);
    REQUIRE(options);
    LC_CurveOffsetCuspIsolationDiagnostics diagnostics;
    const auto built = LC_CurveOffset::buildDirectBranches(
        source,
        LC_CurveOffset::makeSideRequest(signedDistance > 0
                                            ? LC_CurveOffsetSide::Left
                                            : LC_CurveOffsetSide::Right,
                                        std::abs(signedDistance)),
        *options, LC_CurveOffset::makeDirectSourceBudget(), &diagnostics);
    CAPTURE(built.status, diagnostics.failureStage,
            diagnostics.candidateParameters, diagnostics.candidateBoxes,
            diagnostics.stallIntervals, diagnostics.failingRunExtent,
            options->tolerance.nodeMerge);
    REQUIRE(built.status == LC_CurveOffsetStatus::Ok);
    REQUIRE(diagnostics.complete);

    const std::vector<double> sourceBreaks = source.getBreakParameters();
    REQUIRE(sourceBreaks.size() >= 2);
    bool seamCandidate = false;
    bool seamOwnedByStart = false;
    bool seamOwnedByEnd = false;
    for (const LC_CurveOffsetCuspCandidateBox &box :
         diagnostics.candidateBoxes) {
      if (box.span == std::numeric_limits<std::size_t>::max())
        continue;
      CHECK(box.span + 1 < sourceBreaks.size());
      CHECK(sourceBreaks[box.span] <= box.t0);
      CHECK(box.t1 <= sourceBreaks[box.span + 1]);
      CHECK(box.t0 <= box.parameter);
      CHECK(box.parameter <= box.t1);
      const bool atStart = std::abs(box.parameter - startValue) < 1e-6;
      const bool atEnd = std::abs(box.parameter - endValue) < 1e-6;
      if (!atStart && !atEnd)
        continue;
      seamCandidate = true;
      seamOwnedByStart |= atStart && box.span == 0 && box.t0 <= startValue &&
                          box.t1 >= startValue;
      seamOwnedByEnd |= atEnd && box.span + 2 == sourceBreaks.size() &&
                        box.t0 <= endValue && box.t1 >= endValue;
    }
    const bool seamStall = std::any_of(
        diagnostics.stallIntervals.begin(), diagnostics.stallIntervals.end(),
        [&](const std::pair<double, double> &interval) {
          return (interval.first <= startValue &&
                  interval.second >= startValue) ||
                 (interval.first <= endValue && interval.second >= endValue);
        });
    CHECK(seamCandidate);
    CHECK_FALSE(seamStall);
    CHECK((seamOwnedByStart || seamOwnedByEnd));

    const auto startPoint =
        oracleOffsetPoint(data, start, step, start, end, distance);
    const auto endPoint =
        oracleOffsetPoint(data, end, step, start, end, distance, true);
    const RS_Vector expected{
        ((startPoint.first + endPoint.first) / 2).convert_to<double>(),
        ((startPoint.second + endPoint.second) / 2).convert_to<double>()};
    const auto nearestEndpoint = [&](const RS_Vector &point) {
      double nearest = std::numeric_limits<double>::infinity();
      for (const LC_OffsetBranch &branch : built.branches) {
        for (const LC_OffsetCubicPiece &piece : branch.cubicPieces) {
          nearest = std::min(nearest,
                             std::min(piece.bezier.front().distanceTo(point),
                                      piece.bezier.back().distanceTo(point)));
        }
      }
      return nearest;
    };
    const double seamEndpointError = nearestEndpoint(expected);
    CAPTURE(expected.x, expected.y, seamEndpointError,
            options->tolerance.requestedGeometry);
    CHECK(seamEndpointError <= options->tolerance.requestedGeometry);
    for (const auto &stall : diagnostics.stallIntervals) {
      for (const double parameter : {stall.first, stall.second}) {
        const auto point =
            oracleOffsetPoint(data, SplineOracle{parameter}, step, start, end,
                              distance, parameter == endValue);
        const RS_Vector stallEndpoint{point.first.convert_to<double>(),
                                      point.second.convert_to<double>()};
        const double error = nearestEndpoint(stallEndpoint);
        CAPTURE(parameter, stallEndpoint.x, stallEndpoint.y, error,
                options->tolerance.requestedGeometry);
        CHECK(error <= options->tolerance.requestedGeometry);
      }
    }
  }
}

TEST_CASE("periodic rational seam tangent root maps to a bounded stall",
          "[lc-offsetcurve][worker-kernel][spline-variant][oracle][periodic]["
          "rational][seam-root][even-root]") {
    RS_Spline base{nullptr, RS_SplineData{3, false}};
    for (const RS_Vector& point : {RS_Vector{0, 0}, RS_Vector{10, 0},
                                   RS_Vector{10, 10}, RS_Vector{0, 10}})
        base.addControlPoint(point);
    base.setKnotVector({0, 0, 0, 0, 1, 1, 1, 1});
    REQUIRE(base.validate());
    base.setClosed(true);
    RS_SplineData data = base.getData();
    data.weights.assign(data.controlPoints.size(), 1.0);
    const RS_Spline source{nullptr, data};
    REQUIRE(source.validate());
    REQUIRE(source.isClosed());

    double startValue = 0.0;
    double endValue = 0.0;
    REQUIRE(source.getParameterDomain(startValue, endValue));
    const SplineOracle start{startValue};
    const SplineOracle end{endValue};
    const SplineOracle step{"1e-24"};
    const auto startPoint = evaluateSplineOracle(data, start);
    const auto endPoint = evaluateSplineOracle(data, end);
    CHECK(sqrt((startPoint.first - endPoint.first) *
                   (startPoint.first - endPoint.first) +
               (startPoint.second - endPoint.second) *
                   (startPoint.second - endPoint.second)) <
          SplineOracle{"1e-12"});
    const SplineOracle startStep =
        oracleStencilStep(data, start, step, false, true, 2);
    const SplineOracle endStep =
        oracleStencilStep(data, end, step, true, false, 2);
    const auto startOne = evaluateSplineOracle(data, start + startStep);
    const auto startTwo = evaluateSplineOracle(data, start + 2 * startStep);
    const auto endOne = evaluateSplineOracle(data, end - endStep);
    const auto endTwo = evaluateSplineOracle(data, end - 2 * endStep);
    const SplineOracle startDx =
        (-3 * startPoint.first + 4 * startOne.first - startTwo.first) /
        (2 * startStep);
    const SplineOracle startDy =
        (-3 * startPoint.second + 4 * startOne.second - startTwo.second) /
        (2 * startStep);
    const SplineOracle endDx =
        (3 * endPoint.first - 4 * endOne.first + endTwo.first) /
        (2 * endStep);
    const SplineOracle endDy =
        (3 * endPoint.second - 4 * endOne.second + endTwo.second) /
        (2 * endStep);
    CHECK(sqrt((startDx - endDx) * (startDx - endDx) +
               (startDy - endDy) * (startDy - endDy)) < SplineOracle{"1e-8"});

    const SplineOracle unitLeft = oracleOffsetFactor(
        data, end, step, start, end, SplineOracle{1}, true);
    const SplineOracle unitRight = oracleOffsetFactor(
        data, start, step, start, end, SplineOracle{1}, false);
    const SplineOracle curvatureLeft = 1 - unitLeft;
    const SplineOracle curvatureRight = 1 - unitRight;
    CAPTURE(curvatureLeft, curvatureRight);
    REQUIRE(abs(curvatureLeft - curvatureRight) < SplineOracle{"1e-8"});
    REQUIRE(abs(curvatureLeft) > SplineOracle{"1e-12"});
    const double signedDistance =
        (2 / (curvatureLeft + curvatureRight)).convert_to<double>();
    const SplineOracle distance{signedDistance};
    const SplineOracle seamLeft =
        oracleOffsetFactor(data, end, step, start, end, distance, true);
    const SplineOracle seamRight =
        oracleOffsetFactor(data, start, step, start, end, distance, false);
    const SplineOracle before = oracleOffsetFactor(
        data, end - SplineOracle{"1e-6"}, step, start, end, distance);
    const SplineOracle after = oracleOffsetFactor(
        data, start + SplineOracle{"1e-6"}, step, start, end, distance);
    CAPTURE(signedDistance, seamLeft, seamRight, before, after);
    CHECK(abs(seamLeft) < SplineOracle{"1e-8"});
    CHECK(abs(seamRight) < SplineOracle{"1e-8"});
    CHECK(before != 0);
    CHECK(after != 0);
    CHECK((before < 0) == (after < 0));

    const LC_OffsetSourceDefinition definition{LC_OffsetSourceGeometry{data},
                                               signedDistance, 1e-4, false};
    const auto options =
        LC_OffsetWorkerTolerance::directOptions(definition, source);
    REQUIRE(options);
    LC_CurveOffsetCuspIsolationDiagnostics diagnostics;
    const auto built = LC_CurveOffset::buildDirectBranches(
        source,
        LC_CurveOffset::makeSideRequest(signedDistance > 0
                                            ? LC_CurveOffsetSide::Left
                                            : LC_CurveOffsetSide::Right,
                                        std::abs(signedDistance)),
        *options, LC_CurveOffset::makeDirectSourceBudget(), &diagnostics);
    CAPTURE(built.status, diagnostics.status, diagnostics.stallIntervals,
            diagnostics.candidateParameters, diagnostics.candidateBoxes,
            diagnostics.failureStage, diagnostics.failureSpan,
            diagnostics.failureT0, diagnostics.failureT1);
    REQUIRE(built.status == LC_CurveOffsetStatus::Ok);
    REQUIRE(diagnostics.complete);
    const bool seamCandidate = std::any_of(
        diagnostics.candidateParameters.begin(),
        diagnostics.candidateParameters.end(), [&](double parameter) {
          return std::abs(parameter - startValue) < 1e-8 ||
                 std::abs(parameter - endValue) < 1e-8;
        });
    const bool seamStall = std::any_of(
        diagnostics.stallIntervals.begin(), diagnostics.stallIntervals.end(),
        [&](const std::pair<double, double>& interval) {
          return (interval.first <= startValue && interval.second >= startValue) ||
                 (interval.first <= endValue && interval.second >= endValue);
        });
    CHECK((seamStall != seamCandidate));
    const std::vector<double> sourceBreaks = source.getBreakParameters();
    if (seamCandidate) {
        bool ownedByFirstSpan = false;
        bool ownedByLastSpan = false;
        for (const LC_CurveOffsetCuspCandidateBox& box : diagnostics.candidateBoxes) {
            if (std::abs(box.parameter - startValue) < 1e-8) {
                REQUIRE(box.span != std::numeric_limits<std::size_t>::max());
                REQUIRE(box.span + 1 < sourceBreaks.size());
                CHECK(box.span == 0);
                CHECK(box.t0 <= startValue);
                CHECK(box.t1 >= startValue);
                ownedByFirstSpan = true;
            }
            if (std::abs(box.parameter - endValue) < 1e-8) {
                REQUIRE(box.span != std::numeric_limits<std::size_t>::max());
                REQUIRE(box.span + 1 < sourceBreaks.size());
                CHECK(box.span + 2 == sourceBreaks.size());
                CHECK(box.t0 <= endValue);
                CHECK(box.t1 >= endValue);
                ownedByLastSpan = true;
            }
        }
        CHECK((ownedByFirstSpan || ownedByLastSpan));
    }

    const auto seamOffsetStart =
        oracleOffsetPoint(data, start, step, start, end, distance);
    const auto seamOffsetEnd =
        oracleOffsetPoint(data, end, step, start, end, distance, true);
    const RS_Vector expected{
        ((seamOffsetStart.first + seamOffsetEnd.first) / 2).convert_to<double>(),
        ((seamOffsetStart.second + seamOffsetEnd.second) / 2).convert_to<double>()};
    double nearestEndpoint = std::numeric_limits<double>::infinity();
    double nearestCuspEndpoint = std::numeric_limits<double>::infinity();
    for (const LC_OffsetBranch& branch : built.branches) {
        for (const LC_OffsetCubicPiece& piece : branch.cubicPieces) {
            nearestEndpoint = std::min(
                nearestEndpoint,
                std::min(piece.bezier.front().distanceTo(expected),
                         piece.bezier.back().distanceTo(expected)));
            if (branch.startEnd == LC_OffsetBranchEnd::Cusp &&
                !branch.cubicPieces.empty())
                nearestCuspEndpoint = std::min(
                    nearestCuspEndpoint,
                    branch.cubicPieces.front().bezier.front().distanceTo(expected));
            if (branch.endEnd == LC_OffsetBranchEnd::Cusp &&
                !branch.cubicPieces.empty())
                nearestCuspEndpoint = std::min(
                    nearestCuspEndpoint,
                    branch.cubicPieces.back().bezier.back().distanceTo(expected));
        }
    }
    CAPTURE(expected.x, expected.y, nearestEndpoint,
            options->tolerance.requestedGeometry, seamCandidate, seamStall);
    CHECK(nearestEndpoint <= options->tolerance.requestedGeometry);
    if (seamCandidate)
        CHECK(nearestCuspEndpoint <= options->tolerance.requestedGeometry);
    if (seamStall) {
        for (const auto& interval : diagnostics.stallIntervals) {
            const bool ownsStart = interval.first <= startValue &&
                                   interval.second >= startValue;
            const bool ownsEnd = interval.first <= endValue &&
                                 interval.second >= endValue;
            if (!ownsStart && !ownsEnd)
                continue;
            for (const double parameter : {interval.first, interval.second}) {
                const auto stallPoint = oracleOffsetPoint(
                    data, SplineOracle{parameter}, step, start, end, distance,
                    parameter == endValue);
                const RS_Vector expectedStall{
                    stallPoint.first.convert_to<double>(),
                    stallPoint.second.convert_to<double>()};
                double stallEndpointError =
                    std::numeric_limits<double>::infinity();
                for (const LC_OffsetBranch& branch : built.branches) {
                    for (const LC_OffsetCubicPiece& piece : branch.cubicPieces) {
                        stallEndpointError = std::min(
                            stallEndpointError,
                            std::min(piece.bezier.front().distanceTo(expectedStall),
                                     piece.bezier.back().distanceTo(expectedStall)));
                    }
                }
                CAPTURE(parameter, stallEndpointError);
                CHECK(stallEndpointError <= options->tolerance.requestedGeometry);
            }
        }
    }
}

TEST_CASE("higher-degree periodic spline seam roots map to output",
          "[lc-offsetcurve][worker-kernel][spline-variant][oracle][periodic]["
          "rational][seam-root][even-root][degree-limit]") {
    std::size_t unclassifiedWeightedSeams = 0;
    for (const auto [degree, weightProfile] : {
             std::pair{4, 0}, std::pair{4, 1}, std::pair{5, 0},
             std::pair{4, 2}, std::pair{5, 1}, std::pair{5, 2},
             std::pair{6, 0}, std::pair{6, 1}, std::pair{6, 2}}) {
        RS_Spline base{nullptr, RS_SplineData{degree, false}};
        const int controlCount = degree + 1;
        std::vector<RS_Vector> basePoints;
        for (int i = 0; i < controlCount; ++i) {
            const double angle = 2.0 * M_PI * i / controlCount;
            const RS_Vector point{10.0 * std::cos(angle), 10.0 * std::sin(angle)};
            basePoints.push_back(point);
            base.addControlPoint(point);
        }
        std::vector<double> knots(static_cast<std::size_t>(degree + 1), 0.0);
        knots.insert(knots.end(), static_cast<std::size_t>(degree + 1), 1.0);
        base.setKnotVector(knots);
        REQUIRE(base.validate());
        base.setClosed(true);
        RS_SplineData data = base.getData();
        data.weights.assign(data.controlPoints.size(), 1.0);
        if (weightProfile == 1) {
            const std::vector<double> weights = degree == 4
                ? std::vector<double>{1.0, 1.3, .8, .8, 1.3}
                : degree == 5
                    ? std::vector<double>{1.0, 1.3, .8, 1.5, .8, 1.3}
                    : std::vector<double>{1.0, 1.3, .8, 1.5, 1.5, .8, 1.3};
            REQUIRE(weights.size() == basePoints.size());
            for (std::size_t i = 0; i < data.controlPoints.size(); ++i) {
                const auto original = std::find_if(
                    basePoints.begin(), basePoints.end(), [&](const RS_Vector& point) {
                        return point.distanceTo(data.controlPoints[i]) < 1e-12;
                    });
                REQUIRE(original != basePoints.end());
                data.weights[i] = weights[static_cast<std::size_t>(
                    std::distance(basePoints.begin(), original))];
            }
        }
        if (weightProfile == 2) {
            const std::vector<double> weights = degree == 4
                ? std::vector<double>{1.0, 2.5, .35, .35, 2.5}
                : degree == 5
                    ? std::vector<double>{1.0, 2.5, .35, 1.8, .35, 2.5}
                    : std::vector<double>{1.0, 2.5, .35, 1.8, 1.8, .35, 2.5};
            REQUIRE(weights.size() == basePoints.size());
            for (std::size_t i = 0; i < data.controlPoints.size(); ++i) {
                const auto original = std::find_if(
                    basePoints.begin(), basePoints.end(), [&](const RS_Vector& point) {
                        return point.distanceTo(data.controlPoints[i]) < 1e-12;
                    });
                REQUIRE(original != basePoints.end());
                data.weights[i] = weights[static_cast<std::size_t>(
                    std::distance(basePoints.begin(), original))];
            }
        }
        const RS_Spline source{nullptr, data};
        REQUIRE(source.validate());
        REQUIRE(source.isClosed());

        double startValue = 0.0;
        double endValue = 0.0;
        REQUIRE(source.getParameterDomain(startValue, endValue));
        const SplineOracle start{startValue};
        const SplineOracle end{endValue};
        const SplineOracle step{"1e-24"};
        const SplineOracle unitLeft = oracleOffsetFactor(
            data, end, step, start, end, SplineOracle{1}, true);
        const SplineOracle unitRight = oracleOffsetFactor(
            data, start, step, start, end, SplineOracle{1}, false);
        const SplineOracle curvatureLeft = 1 - unitLeft;
        const SplineOracle curvatureRight = 1 - unitRight;
        CAPTURE(degree, weightProfile, curvatureLeft, curvatureRight);
        REQUIRE(abs(curvatureLeft - curvatureRight) < SplineOracle{"1e-8"});
        REQUIRE(abs(curvatureLeft) > SplineOracle{"1e-12"});
        const double signedDistance =
            (2 / (curvatureLeft + curvatureRight)).convert_to<double>();
        const SplineOracle distance{signedDistance};
        const auto oracleCandidates =
            oracleOffsetCusps(data, start, end, signedDistance);
        std::vector<double> oracleCandidateParameters;
        oracleCandidateParameters.reserve(oracleCandidates.size());
        for (const auto& candidate : oracleCandidates) {
            oracleCandidateParameters.push_back(candidate.parameter.convert_to<double>());
        }
        const SplineOracle seamLeft = oracleOffsetFactor(
            data, end, step, start, end, distance, true);
        const SplineOracle seamRight = oracleOffsetFactor(
            data, start, step, start, end, distance, false);
        const SplineOracle before = oracleOffsetFactor(
            data, end - SplineOracle{"1e-7"}, step, start, end, distance);
        const SplineOracle after = oracleOffsetFactor(
            data, start + SplineOracle{"1e-7"}, step, start, end, distance);
        CAPTURE(startValue, endValue, weightProfile, signedDistance, seamLeft,
                seamRight, before, after);
        CHECK(abs(seamLeft) < SplineOracle{"1e-8"});
        CHECK(abs(seamRight) < SplineOracle{"1e-8"});
        REQUIRE(before != 0);
        REQUIRE(after != 0);
        if (weightProfile == 0)
            CHECK((before < 0) == (after < 0));
        else
            CHECK((before < 0) != (after < 0));

        const LC_OffsetSourceDefinition definition{LC_OffsetSourceGeometry{data},
                                                   signedDistance, 1e-4, false};
        const auto options =
            LC_OffsetWorkerTolerance::directOptions(definition, source);
        REQUIRE(options);
        LC_CurveOffsetCuspIsolationDiagnostics diagnostics;
        const auto built = LC_CurveOffset::buildDirectBranches(
            source,
            LC_CurveOffset::makeSideRequest(signedDistance > 0
                                                ? LC_CurveOffsetSide::Left
                                                : LC_CurveOffsetSide::Right,
                                            std::abs(signedDistance)),
            *options, LC_CurveOffset::makeDirectSourceBudget(), &diagnostics);
        CAPTURE(built.status, diagnostics.status, diagnostics.stallIntervals,
                diagnostics.candidateParameters, diagnostics.candidateBoxes,
                oracleCandidateParameters,
                diagnostics.unclassifiedBoundaries,
                diagnostics.failureStage, diagnostics.failureSpan,
                diagnostics.failureT0, diagnostics.failureT1,
                diagnostics.isolationBoxesVisited, diagnostics.failingRunBoxes,
                diagnostics.failingRunSignChanges, diagnostics.failingRunExtent,
                diagnostics.failingRunObserved, options->tolerance.nodeMerge);
        if (degree == 6 && weightProfile == 2) {
            LC_CurveJetBounds leftKnotBounds;
            LC_CurveJetBounds rightKnotBounds;
            REQUIRE(source.tryBoundJetForCusp(2.0 - 1e-6, 2.0,
                                               leftKnotBounds, signedDistance));
            REQUIRE(source.tryBoundJetForCusp(2.0, 2.0 + 1e-6,
                                               rightKnotBounds, signedDistance));
            CAPTURE(leftKnotBounds.rationalCuspRootVariationUpperBound,
                    rightKnotBounds.rationalCuspRootVariationUpperBound,
                    leftKnotBounds.rationalCuspIntervalVariationUpperBound,
                    rightKnotBounds.rationalCuspIntervalVariationUpperBound);
            CHECK(leftKnotBounds.rationalCuspIntervalVariationUpperBound >= 0);
            CHECK(rightKnotBounds.rationalCuspIntervalVariationUpperBound >= 0);
            CHECK(leftKnotBounds.rationalCuspIntervalVariationUpperBound +
                      rightKnotBounds.rationalCuspIntervalVariationUpperBound ==
                  2);
            LC_CurveJetBounds leftInteriorBounds;
            LC_CurveJetBounds rightInteriorBounds;
            REQUIRE(source.tryBoundJetForCusp(2.0 - 1e-6, 2.0 - 5e-7,
                                               leftInteriorBounds, signedDistance));
            REQUIRE(source.tryBoundJetForCusp(2.0 + 5e-7, 2.0 + 1e-6,
                                               rightInteriorBounds, signedDistance));
            const LC_Interval leftInteriorFactor =
                leftInteriorBounds.offsetFactorNumerator(signedDistance);
            const LC_Interval rightInteriorFactor =
                rightInteriorBounds.offsetFactorNumerator(signedDistance);
            CAPTURE(leftInteriorFactor, rightInteriorFactor);
            REQUIRE(leftInteriorFactor.excludesZero());
            REQUIRE(rightInteriorFactor.excludesZero());
            CHECK((leftInteriorFactor.hi() < 0.0) !=
                  (rightInteriorFactor.hi() < 0.0));
            CHECK(built.status == LC_CurveOffsetStatus::Ok);
            CHECK(diagnostics.status == built.status);
            CHECK(diagnostics.complete);
            CHECK_FALSE(built.branches.empty());
            CHECK(std::any_of(oracleCandidateParameters.begin(),
                              oracleCandidateParameters.end(), [](double parameter) {
                                  return std::abs(parameter - 2.0) < 1e-12;
                              }));
            CHECK(std::any_of(diagnostics.candidateParameters.begin(),
                              diagnostics.candidateParameters.end(), [](double parameter) {
                                  return std::abs(parameter - 2.0) < 1e-12;
                              }));
            const req::Request generationRequest{
                token, req::GenerateSource{definition, {}}};
            const auto generationReply =
                LC_OffsetWorkerKernel::run(generationRequest);
            const auto& generated = native(generationReply, 7);
            CHECK(generated.engineRuns == 1);
            REQUIRE(generated.engineStatus);
            CHECK(*generated.engineStatus == built.status);
            CHECK(generated.refusal != LC_OffsetCurveRefusal::Engine);
            CHECK_FALSE(generated.curve);
            CHECK_FALSE(generated.changed);
            CHECK(generated.ordinaryStatus == result::OrdinaryStatus::Complete);
            CHECK_FALSE(generated.ordinary.empty());
            const auto wire = result::encode(generationReply, generationRequest);
            REQUIRE(wire);
            const auto decoded = result::decode(*wire, generationRequest);
            REQUIRE(decoded);
            const auto& decodedGenerated = native(*decoded, 7);
            CHECK(decodedGenerated.engineStatus == generated.engineStatus);
            CHECK(decodedGenerated.refusal == generated.refusal);
            CHECK_FALSE(decodedGenerated.curve);
            CHECK_FALSE(decodedGenerated.changed);
            CHECK(decodedGenerated.ordinaryStatus == generated.ordinaryStatus);
            CHECK(decodedGenerated.ordinary.size() == generated.ordinary.size());
        }
        REQUIRE(built.status == LC_CurveOffsetStatus::Ok);
        REQUIRE(diagnostics.complete);
        const auto builtWithoutDiagnostics = LC_CurveOffset::buildDirectBranches(
            source,
            LC_CurveOffset::makeSideRequest(signedDistance > 0
                                                ? LC_CurveOffsetSide::Left
                                                : LC_CurveOffsetSide::Right,
                                            std::abs(signedDistance)),
            *options, LC_CurveOffset::makeDirectSourceBudget());
        REQUIRE(builtWithoutDiagnostics.status == built.status);
        REQUIRE(builtWithoutDiagnostics.branches.size() == built.branches.size());
        for (std::size_t i = 0; i < built.branches.size(); ++i) {
            const LC_OffsetBranch& withDiagnostics = built.branches[i];
            const LC_OffsetBranch& withoutDiagnostics = builtWithoutDiagnostics.branches[i];
            CHECK(withoutDiagnostics.closed == withDiagnostics.closed);
            CHECK(withoutDiagnostics.reversed == withDiagnostics.reversed);
            CHECK(withoutDiagnostics.startEnd == withDiagnostics.startEnd);
            CHECK(withoutDiagnostics.endEnd == withDiagnostics.endEnd);
            REQUIRE(withoutDiagnostics.cubicPieces.size() ==
                    withDiagnostics.cubicPieces.size());
            if (!withDiagnostics.cubicPieces.empty()) {
                CHECK(withoutDiagnostics.cubicPieces.front().bezier.front() ==
                      withDiagnostics.cubicPieces.front().bezier.front());
                CHECK(withoutDiagnostics.cubicPieces.back().bezier.back() ==
                      withDiagnostics.cubicPieces.back().bezier.back());
            }
        }
        const auto seamCandidate = std::find_if(
            diagnostics.candidateParameters.begin(),
            diagnostics.candidateParameters.end(), [&](double parameter) {
                return std::abs(parameter - startValue) < 1e-7 ||
                       std::abs(parameter - endValue) < 1e-7;
            });
        const auto seamStall = std::find_if(
            diagnostics.stallIntervals.begin(), diagnostics.stallIntervals.end(),
            [&](const std::pair<double, double>& interval) {
                return (interval.first <= startValue && interval.second >= startValue) ||
                       (interval.first <= endValue && interval.second >= endValue);
            });
        const auto unclassifiedSeam = std::find_if(
            diagnostics.unclassifiedBoundaries.begin(),
            diagnostics.unclassifiedBoundaries.end(), [&](const auto& boundary) {
                return std::abs(boundary.parameter - startValue) < 1e-7 ||
                       std::abs(boundary.parameter - endValue) < 1e-7;
            });
        if (weightProfile == 0) {
            CHECK((seamStall != diagnostics.stallIntervals.end()) !=
                  (seamCandidate != diagnostics.candidateParameters.end()));
        } else {
            const bool seamHasOwner = seamStall != diagnostics.stallIntervals.end() ||
                                      seamCandidate != diagnostics.candidateParameters.end();
            const bool hasUnclassifiedSeam =
                unclassifiedSeam != diagnostics.unclassifiedBoundaries.end();
            const bool seamHasDiagnostic = seamHasOwner || hasUnclassifiedSeam;
            const bool conflictingSeamDiagnostics = hasUnclassifiedSeam && seamHasOwner;
            CHECK(seamHasDiagnostic);
            CHECK_FALSE(conflictingSeamDiagnostics);
            if (hasUnclassifiedSeam) {
                CHECK(std::abs(unclassifiedSeam->leftFactor) <=
                      unclassifiedSeam->leftRoundoffBound);
                CHECK(std::abs(unclassifiedSeam->rightFactor) <=
                      unclassifiedSeam->rightRoundoffBound);
                ++unclassifiedWeightedSeams;
            }
        }
        if (seamCandidate != diagnostics.candidateParameters.end()) {
            const auto seamBox = std::find_if(
                diagnostics.candidateBoxes.begin(), diagnostics.candidateBoxes.end(),
                [&](const LC_CurveOffsetCuspCandidateBox& box) {
                    return std::abs(box.parameter - *seamCandidate) < 1e-12;
                });
            REQUIRE(seamBox != diagnostics.candidateBoxes.end());
            REQUIRE(seamBox->span != std::numeric_limits<std::size_t>::max());
            const std::vector<double> breaks = source.getBreakParameters();
            REQUIRE(seamBox->span + 1 < breaks.size());
            if (std::abs(*seamCandidate - startValue) < 1e-7) {
                CHECK(seamBox->span == 0);
                CHECK(seamBox->t0 <= startValue + 1e-7);
                CHECK(seamBox->t1 >= startValue);
            } else {
                CHECK(seamBox->span + 2 == breaks.size());
                CHECK(seamBox->t0 <= endValue);
                CHECK(seamBox->t1 >= endValue - 1e-7);
            }
        }

        for (const double candidate : diagnostics.candidateParameters) {
            const SplineOracle candidateParameter{candidate};
            const SplineOracle candidateNeighborhood{"1e-6"};
            const auto oracleCandidateFactor = oracleOffsetFactor(
                data, candidateParameter, step, start, end, distance);
            const auto oracleCandidateBefore = oracleOffsetFactor(
                data, candidateParameter - candidateNeighborhood, step,
                start, end, distance);
            const auto oracleCandidateAfter = oracleOffsetFactor(
                data, candidateParameter + candidateNeighborhood, step,
                start, end, distance);
            const auto productionFactor = [&](const double parameter) {
                double wrappedParameter = parameter;
                const double domain = endValue - startValue;
                if (wrappedParameter < startValue) wrappedParameter += domain;
                if (wrappedParameter > endValue) wrappedParameter -= domain;
                LC_CurveJet jet;
                if (!source.tryEvaluateJet(wrappedParameter,
                                           LC_CurveEvaluationSide::Interior, jet))
                    return std::numeric_limits<double>::quiet_NaN();
                const double speedSquared = jet.first.x * jet.first.x +
                                            jet.first.y * jet.first.y;
                return speedSquared * std::sqrt(speedSquared) - signedDistance *
                    (jet.first.x * jet.second.y - jet.first.y * jet.second.x);
            };
            const double productionBefore = productionFactor(candidate - 1e-6);
            const double productionAfter = productionFactor(candidate + 1e-6);
            const auto candidatePoint = oracleOffsetPoint(
                data, candidateParameter, step, start, end, distance,
                candidate == endValue);
            const RS_Vector expectedCandidate{
                candidatePoint.first.convert_to<double>(),
                candidatePoint.second.convert_to<double>()};
            double nearestCuspEndpoint = std::numeric_limits<double>::infinity();
            for (const LC_OffsetBranch& branch : built.branches) {
                if (branch.cubicPieces.empty()) continue;
                if (branch.startEnd == LC_OffsetBranchEnd::Cusp) {
                    nearestCuspEndpoint = std::min(
                        nearestCuspEndpoint,
                        branch.cubicPieces.front().bezier.front().distanceTo(
                            expectedCandidate));
                }
                if (branch.endEnd == LC_OffsetBranchEnd::Cusp) {
                    nearestCuspEndpoint = std::min(
                        nearestCuspEndpoint,
                        branch.cubicPieces.back().bezier.back().distanceTo(
                            expectedCandidate));
                }
            }
            const auto candidateBox = std::find_if(
                diagnostics.candidateBoxes.begin(), diagnostics.candidateBoxes.end(),
                [&](const LC_CurveOffsetCuspCandidateBox& box) {
                    return box.parameter == candidate;
                });
            REQUIRE(candidateBox != diagnostics.candidateBoxes.end());
            const bool mappedToTinyReversal = std::any_of(
                diagnostics.collapsedTinyReversals.begin(),
                diagnostics.collapsedTinyReversals.end(),
                [&](const LC_CurveOffsetTinyReversalCollapse& collapse) {
                    return collapse.boundedByCusps &&
                           collapse.sourceT0 <= candidate &&
                           candidate <= collapse.sourceT1 &&
                           collapse.extent <= 2.0 * options->tolerance.nodeMerge &&
                           collapse.replacementPoint.distanceTo(expectedCandidate) <=
                               options->tolerance.requestedGeometry;
                });
            CAPTURE(degree, weightProfile, candidate, candidateBox->span,
                    nearestCuspEndpoint, mappedToTinyReversal,
                    options->tolerance.requestedGeometry, oracleCandidateFactor,
                    oracleCandidateBefore, oracleCandidateAfter, productionBefore,
                    productionAfter);
            CHECK(abs(oracleCandidateFactor) < SplineOracle{"1e-8"});
            CHECK((oracleCandidateBefore < 0) != (oracleCandidateAfter < 0));
            CHECK((productionBefore < 0) != (productionAfter < 0));
            CHECK((nearestCuspEndpoint <= options->tolerance.requestedGeometry ||
                   mappedToTinyReversal));
        }

        const auto startPoint = oracleOffsetPoint(
            data, start, step, start, end, distance);
        const auto endPoint = oracleOffsetPoint(
            data, end, step, start, end, distance, true);
        const RS_Vector expected{
            ((startPoint.first + endPoint.first) / 2).convert_to<double>(),
            ((startPoint.second + endPoint.second) / 2).convert_to<double>()};
        double nearest = std::numeric_limits<double>::infinity();
        for (const LC_OffsetBranch& branch : built.branches) {
            for (const LC_OffsetCubicPiece& piece : branch.cubicPieces) {
                nearest = std::min(
                    nearest,
                    std::min(piece.bezier.front().distanceTo(expected),
                             piece.bezier.back().distanceTo(expected)));
            }
        }
        CAPTURE(degree, weightProfile, expected.x, expected.y, nearest,
                options->tolerance.requestedGeometry);
        CHECK(nearest <= options->tolerance.requestedGeometry);
        for (const auto& interval : diagnostics.stallIntervals) {
            const bool ownsStart = interval.first <= startValue &&
                                   interval.second >= startValue;
            const bool ownsEnd = interval.first <= endValue &&
                                 interval.second >= endValue;
            if (!ownsStart && !ownsEnd) {
                continue;
            }
            for (const double parameter : {interval.first, interval.second}) {
                const auto offsetPoint = oracleOffsetPoint(
                    data, SplineOracle{parameter}, step, start, end, distance,
                    parameter == endValue);
                const RS_Vector expectedEndpoint{
                    offsetPoint.first.convert_to<double>(),
                    offsetPoint.second.convert_to<double>()};
                double endpointError = std::numeric_limits<double>::infinity();
                for (const LC_OffsetBranch& branch : built.branches) {
                    for (const LC_OffsetCubicPiece& piece : branch.cubicPieces) {
                        endpointError = std::min(
                            endpointError,
                            std::min(piece.bezier.front().distanceTo(expectedEndpoint),
                                     piece.bezier.back().distanceTo(expectedEndpoint)));
                    }
                }
                CAPTURE(degree, weightProfile, parameter, endpointError,
                        options->tolerance.requestedGeometry);
                CHECK(endpointError <= options->tolerance.requestedGeometry);
            }
        }
    }
    CHECK(unclassifiedWeightedSeams > 0);
}

TEST_CASE("periodic rational seam tangent and interior roots map to output",
          "[lc-offsetcurve][worker-kernel][spline-variant][oracle][periodic]["
          "rational][seam-root][even-root][candidate-mapping]") {
    RS_Spline base{nullptr, RS_SplineData{3, false}};
    for (const RS_Vector& point : {RS_Vector{0, 0}, RS_Vector{10, 0},
                                   RS_Vector{10, 10}, RS_Vector{0, 10}})
        base.addControlPoint(point);
    base.setKnotVector({0, 0, 0, 0, 1, 1, 1, 1});
    REQUIRE(base.validate());
    base.setClosed(true);
    RS_SplineData data = base.getData();
    const std::vector<double> baseWeights{1, .7, 1.3, .7};
    data.weights = baseWeights;
    data.weights.insert(data.weights.end(), baseWeights.begin(),
                        baseWeights.begin() + data.degree);
    const RS_Spline source{nullptr, data};
    REQUIRE(source.validate());
    REQUIRE(source.isClosed());

    double startValue = 0.0;
    double endValue = 0.0;
    REQUIRE(source.getParameterDomain(startValue, endValue));
    const SplineOracle start{startValue};
    const SplineOracle end{endValue};
    const SplineOracle step{"1e-24"};
    const SplineOracle unitLeft = oracleOffsetFactor(
        data, end, step, start, end, SplineOracle{1}, true);
    const SplineOracle unitRight = oracleOffsetFactor(
        data, start, step, start, end, SplineOracle{1}, false);
    const SplineOracle curvatureLeft = 1 - unitLeft;
    const SplineOracle curvatureRight = 1 - unitRight;
    REQUIRE(abs(curvatureLeft - curvatureRight) < SplineOracle{"1e-8"});
    REQUIRE(abs(curvatureLeft) > SplineOracle{"1e-12"});
    const double signedDistance =
        (2 / (curvatureLeft + curvatureRight)).convert_to<double>();
    const SplineOracle distance{signedDistance};
    const SplineOracle seamLeft =
        oracleOffsetFactor(data, end, step, start, end, distance, true);
    const SplineOracle seamRight =
        oracleOffsetFactor(data, start, step, start, end, distance, false);
    const SplineOracle before = oracleOffsetFactor(
        data, end - SplineOracle{"1e-7"}, step, start, end, distance);
    const SplineOracle after = oracleOffsetFactor(
        data, start + SplineOracle{"1e-7"}, step, start, end, distance);
    CAPTURE(baseWeights, signedDistance, seamLeft, seamRight, before, after);
    CHECK(abs(seamLeft) < SplineOracle{"1e-8"});
    CHECK(abs(seamRight) < SplineOracle{"1e-8"});
    CHECK(before != 0);
    CHECK(after != 0);
    CHECK((before < 0) == (after < 0));

    const LC_OffsetSourceDefinition definition{LC_OffsetSourceGeometry{data},
                                               signedDistance, 1e-4, false};
    const auto options =
        LC_OffsetWorkerTolerance::directOptions(definition, source);
    REQUIRE(options);
    LC_CurveOffsetCuspIsolationDiagnostics diagnostics;
    const auto built = LC_CurveOffset::buildDirectBranches(
        source,
        LC_CurveOffset::makeSideRequest(signedDistance > 0
                                            ? LC_CurveOffsetSide::Left
                                            : LC_CurveOffsetSide::Right,
                                        std::abs(signedDistance)),
        *options, LC_CurveOffset::makeDirectSourceBudget(), &diagnostics);
    const std::vector<OracleSplineCusp> expectedCusps =
        oracleOffsetCusps(data, start, end, signedDistance, SplineOracle{"1e-25"});
    std::vector<double> expectedParameters;
    for (const OracleSplineCusp& cusp : expectedCusps)
        expectedParameters.push_back(cusp.parameter.convert_to<double>());
    CAPTURE(built.status, diagnostics.status, diagnostics.candidateParameters,
            diagnostics.candidateBoxes, diagnostics.stallIntervals, expectedParameters,
            diagnostics.failureStage, diagnostics.failureSpan,
            diagnostics.failureT0, diagnostics.failureT1);
    REQUIRE(built.status == LC_CurveOffsetStatus::Ok);
    CHECK(diagnostics.status == built.status);
    REQUIRE(diagnostics.complete);
    CHECK(diagnostics.candidateBoxes.size() >= diagnostics.candidateParameters.size());

    const std::vector<double> sourceBreaks = source.getBreakParameters();
    std::vector<bool> matchedOracleCusps(expectedCusps.size(), false);
    for (const double candidate : diagnostics.candidateParameters) {
        double nearestParameterError = std::numeric_limits<double>::infinity();
        double nearestPointError = std::numeric_limits<double>::infinity();
        std::size_t nearestOracle = 0;
        const auto candidatePoint = oracleOffsetPoint(
            data, SplineOracle{candidate}, step, start, end, distance,
            std::abs(candidate - endValue) < 1e-8);
        const RS_Vector productionPoint{candidatePoint.first.convert_to<double>(),
                                        candidatePoint.second.convert_to<double>()};
        for (std::size_t i = 0; i < expectedCusps.size(); ++i) {
            const double parameterError = std::abs(
                candidate - expectedCusps[i].parameter.convert_to<double>());
            if (parameterError < nearestParameterError) {
                nearestParameterError = parameterError;
                nearestPointError = productionPoint.distanceTo(expectedCusps[i].point);
                nearestOracle = i;
            }
        }
        CAPTURE(candidate, nearestParameterError, nearestPointError);
        CHECK(nearestParameterError < 1e-5);
        CHECK(nearestPointError < options->tolerance.requestedGeometry);
        if (nearestPointError < options->tolerance.requestedGeometry) {
            CHECK_FALSE(matchedOracleCusps[nearestOracle]);
            matchedOracleCusps[nearestOracle] = true;
        }

        std::vector<const LC_CurveOffsetCuspCandidateBox*> boxes;
        for (const LC_CurveOffsetCuspCandidateBox& box : diagnostics.candidateBoxes)
            if (box.parameter == candidate) boxes.push_back(&box);
        REQUIRE(boxes.size() == 1);
        const LC_CurveOffsetCuspCandidateBox& box = *boxes.front();
        REQUIRE(box.span != std::numeric_limits<std::size_t>::max());
        REQUIRE(box.span + 1 < sourceBreaks.size());
        CHECK(box.t0 <= candidate);
        CHECK(candidate <= box.t1);
        CHECK(sourceBreaks[box.span] <= box.t0);
        CHECK(box.t1 <= sourceBreaks[box.span + 1]);
    }
    CHECK(static_cast<std::size_t>(std::count(matchedOracleCusps.begin(),
                                              matchedOracleCusps.end(), true)) ==
          diagnostics.candidateParameters.size());

    std::vector<RS_Vector> outputCuspEndpoints;
    std::vector<RS_Vector> outputEndpoints;
    for (const LC_OffsetBranch& branch : built.branches) {
        if (branch.cubicPieces.empty()) continue;
        for (const LC_OffsetCubicPiece& piece : branch.cubicPieces) {
            outputEndpoints.push_back(piece.bezier.front());
            outputEndpoints.push_back(piece.bezier.back());
        }
        if (branch.startEnd == LC_OffsetBranchEnd::Cusp) {
            outputCuspEndpoints.push_back(branch.cubicPieces.front().bezier.front());
        }
        if (branch.endEnd == LC_OffsetBranchEnd::Cusp) {
            outputCuspEndpoints.push_back(branch.cubicPieces.back().bezier.back());
        }
    }
    std::vector<RS_Vector> simpleCuspPoints;
    for (const OracleSplineCusp& cusp : expectedCusps) {
        const double parameter = cusp.parameter.convert_to<double>();
        const double nearestOutputError = std::accumulate(
            outputEndpoints.begin(), outputEndpoints.end(),
            std::numeric_limits<double>::infinity(), [&](const double best, const RS_Vector& endpoint) {
                return std::min(best, endpoint.distanceTo(cusp.point));
            });
        const auto stall = std::find_if(
            diagnostics.stallIntervals.begin(), diagnostics.stallIntervals.end(),
            [&](const std::pair<double, double>& interval) {
                return interval.first <= parameter && parameter <= interval.second;
            });
        CAPTURE(parameter, cusp.span, nearestOutputError);
        CHECK(nearestOutputError <= options->tolerance.requestedGeometry);

        if (stall != diagnostics.stallIntervals.end()) {
            for (const double stallParameter : {stall->first, stall->second}) {
                const auto point = oracleOffsetPoint(
                    data, SplineOracle{stallParameter}, step, start, end, distance,
                    stallParameter == endValue);
                const RS_Vector expectedStallPoint{point.first.convert_to<double>(),
                                                   point.second.convert_to<double>()};
                const double endpointError = std::accumulate(
                    outputEndpoints.begin(), outputEndpoints.end(),
                    std::numeric_limits<double>::infinity(), [&](const double best, const RS_Vector& endpoint) {
                        return std::min(best, endpoint.distanceTo(expectedStallPoint));
                    });
                CAPTURE(stallParameter, endpointError);
                CHECK(endpointError <= options->tolerance.requestedGeometry);
            }
            continue;
        }

        const double seamEpsilon = 1e-7;
        const SplineOracle beforeParameter =
            std::abs(parameter - startValue) < 1e-8
                ? end - SplineOracle{seamEpsilon}
                : SplineOracle{parameter - seamEpsilon};
        const SplineOracle afterParameter =
            std::abs(parameter - endValue) < 1e-8
                ? start + SplineOracle{seamEpsilon}
                : SplineOracle{parameter + seamEpsilon};
        const SplineOracle beforeFactor = oracleOffsetFactor(
            data, beforeParameter, step, start, end, distance);
        const SplineOracle afterFactor = oracleOffsetFactor(
            data, afterParameter, step, start, end, distance);
        if ((beforeFactor < 0) != (afterFactor < 0)) {
            const double nearestCuspError = std::accumulate(
                outputCuspEndpoints.begin(), outputCuspEndpoints.end(),
                std::numeric_limits<double>::infinity(), [&](const double best, const RS_Vector& endpoint) {
                    return std::min(best, endpoint.distanceTo(cusp.point));
                });
            CAPTURE(parameter, beforeFactor, afterFactor, nearestCuspError);
            CHECK(nearestCuspError <= options->tolerance.requestedGeometry);
            if (std::none_of(simpleCuspPoints.begin(), simpleCuspPoints.end(),
                             [&](const RS_Vector& point) {
                                 return point.distanceTo(cusp.point) <=
                                        options->tolerance.requestedGeometry;
                             })) {
                simpleCuspPoints.push_back(cusp.point);
            }
        }
    }
    CHECK(outputCuspEndpoints.size() == 2 * simpleCuspPoints.size());
    for (const RS_Vector& point : simpleCuspPoints) {
        const std::size_t matchingEnds = static_cast<std::size_t>(std::count_if(
            outputCuspEndpoints.begin(), outputCuspEndpoints.end(),
            [&](const RS_Vector& endpoint) {
                return endpoint.distanceTo(point) <= options->tolerance.requestedGeometry;
            }));
        CHECK(matchingEnds == 2);
    }
    for (const RS_Vector& endpoint : outputCuspEndpoints) {
        const std::size_t matchingEvents = static_cast<std::size_t>(std::count_if(
            simpleCuspPoints.begin(), simpleCuspPoints.end(),
            [&](const RS_Vector& point) {
                return endpoint.distanceTo(point) <= options->tolerance.requestedGeometry;
            }));
        CHECK(matchingEvents == 1);
    }
}

TEST_CASE("repeated C2 rational spline knot cusp maps to output",
          "[lc-offsetcurve][worker-kernel][spline-variant][rational][knot][oracle]") {
    RS_SplineData data{6, false};
    data.controlPoints = {{0, 0}, {1, .001}, {2, .008}, {3, -5.0}, {4, .064},
                          {5, .125}, {6, .216}, {7, .343}, {8, .512},
                          {9, .729}, {10, 1.0}};
    data.weights = {1.0, 1.02, .98, 1.03, .97, 1.01, .99, 1.04, .96, 1.02, .98};
    data.knotslist = {0, 0, 0, 0, 0, 0, 0, .5, .5, .5, .5,
                      1, 1, 1, 1, 1, 1, 1};
    const RS_Spline source{nullptr, data};
    REQUIRE(source.validate());
    LC_CurveJet leftJet;
    LC_CurveJet rightJet;
    REQUIRE(source.tryEvaluateJet(.5, LC_CurveEvaluationSide::Left, leftJet));
    REQUIRE(source.tryEvaluateJet(.5, LC_CurveEvaluationSide::Right, rightJet));
    const auto curvature = [](const LC_CurveJet& jet) {
        const double speedSquared = jet.first.x * jet.first.x + jet.first.y * jet.first.y;
        return (jet.first.x * jet.second.y - jet.first.y * jet.second.x) /
               (speedSquared * std::sqrt(speedSquared));
    };
    const double leftCurvature = curvature(leftJet);
    const double rightCurvature = curvature(rightJet);
    CAPTURE(leftCurvature, rightCurvature);
    REQUIRE(std::isfinite(leftCurvature));
    REQUIRE(std::isfinite(rightCurvature));
    REQUIRE(std::abs(leftCurvature) > 1e-9);
    CHECK(std::abs(leftCurvature - rightCurvature) < 1e-8);
    const SplineOracle start{0};
    const SplineOracle end{1};
    const SplineOracle step{"1e-24"};
    const SplineOracle leftKnotCurvature = 1 - oracleOffsetFactor(
        data, SplineOracle{"0.5"}, step, start, end, SplineOracle{1}, true);
    const SplineOracle rightKnotCurvature = 1 - oracleOffsetFactor(
        data, SplineOracle{"0.5"}, step, start, end, SplineOracle{1});
    REQUIRE(abs(leftKnotCurvature - rightKnotCurvature) < SplineOracle{"1e-30"});
    const double signedDistance =
        (2 / (leftKnotCurvature + rightKnotCurvature)).convert_to<double>();
    LC_CurveJetBounds leftBounds;
    LC_CurveJetBounds rightBounds;
    REQUIRE(source.tryBoundJetForCusp(.5 - 1e-6, .5, leftBounds, signedDistance));
    REQUIRE(source.tryBoundJetForCusp(.5, .5 + 1e-6, rightBounds, signedDistance));
    CAPTURE(leftBounds.rationalCuspRootVariationUpperBound,
            rightBounds.rationalCuspRootVariationUpperBound,
            leftBounds.rationalCuspIntervalVariationUpperBound,
            rightBounds.rationalCuspIntervalVariationUpperBound);
    CHECK(leftBounds.rationalCuspRootVariationUpperBound < 0);
    CHECK(rightBounds.rationalCuspRootVariationUpperBound < 0);
    REQUIRE(leftBounds.rationalCuspIntervalVariationUpperBound >= 0);
    REQUIRE(rightBounds.rationalCuspIntervalVariationUpperBound >= 0);
    CHECK(leftBounds.rationalCuspIntervalVariationUpperBound +
              rightBounds.rationalCuspIntervalVariationUpperBound <=
          2);
    const SplineOracle before = oracleOffsetFactor(
        data, SplineOracle{"0.5"} - SplineOracle{"1e-7"}, step,
        start, end, SplineOracle{signedDistance});
    const SplineOracle after = oracleOffsetFactor(
        data, SplineOracle{"0.5"} + SplineOracle{"1e-7"}, step,
        start, end, SplineOracle{signedDistance});
    CAPTURE(signedDistance, before, after);
    REQUIRE(abs(before) > SplineOracle{"1e-12"});
    REQUIRE(abs(after) > SplineOracle{"1e-12"});
    REQUIRE((before < 0) != (after < 0));

    const LC_OffsetSourceDefinition definition{
        LC_OffsetSourceGeometry{data}, signedDistance, 1e-5, false};
    const auto options = LC_OffsetWorkerTolerance::directOptions(definition, source);
    REQUIRE(options);
    LC_CurveOffsetCuspIsolationDiagnostics diagnostics;
    const auto built = LC_CurveOffset::buildDirectBranches(
        source,
        LC_CurveOffset::makeSideRequest(
            signedDistance > 0.0 ? LC_CurveOffsetSide::Left
                                 : LC_CurveOffsetSide::Right,
            std::abs(signedDistance)),
        *options, LC_CurveOffset::makeDirectSourceBudget(), &diagnostics);
    CAPTURE(built.status, diagnostics.failureStage, diagnostics.candidateParameters,
            diagnostics.stallIntervals);
    REQUIRE(built.status == LC_CurveOffsetStatus::Ok);
    CHECK(std::any_of(diagnostics.candidateParameters.begin(),
                      diagnostics.candidateParameters.end(), [](const double parameter) {
                          return std::abs(parameter - .5) < 1e-8;
                      }));
    const auto knotCandidate = std::find_if(
        diagnostics.candidateBoxes.begin(), diagnostics.candidateBoxes.end(),
        [](const LC_CurveOffsetCuspCandidateBox& candidate) {
            return std::abs(candidate.parameter - .5) < 1e-8;
        });
    REQUIRE(knotCandidate != diagnostics.candidateBoxes.end());
    CHECK(knotCandidate->kind ==
          LC_CurveOffsetCuspCandidateKind::CertifiedUniqueBracket);
    const auto expectedPoint = oracleOffsetPoint(
        data, SplineOracle{"0.5"}, step, start, end, SplineOracle{signedDistance});
    const RS_Vector expected{expectedPoint.first.convert_to<double>(),
                             expectedPoint.second.convert_to<double>()};
    double endpointError = std::numeric_limits<double>::infinity();
    for (const auto& branch : built.branches) {
        if (branch.cubicPieces.empty()) continue;
        if (branch.startEnd == LC_OffsetBranchEnd::Cusp) {
            endpointError = std::min(endpointError,
                                     branch.cubicPieces.front().bezier.front().distanceTo(expected));
        }
        if (branch.endEnd == LC_OffsetBranchEnd::Cusp) {
            endpointError = std::min(endpointError,
                                     branch.cubicPieces.back().bezier.back().distanceTo(expected));
        }
    }
    CAPTURE(endpointError, options->tolerance.requestedGeometry);
    CHECK(endpointError <= options->tolerance.requestedGeometry);
}

TEST_CASE("C2 rational knot crossings map for degrees three through six and seven knot positions",
          "[lc-offsetcurve][worker-kernel][spline-variant][rational][knot][degree-limit]") {
    const std::array<double, 7> weightPattern{1.0, 1.02, .98, 1.03, .97, 1.01, .99};
    const std::array<std::pair<double, double>, 14> orientationAndKnotCases{
        {{-1.0, .125}, {-1.0, .23},  {-1.0, .25},  {-1.0, .37},
         {-1.0, .5},   {-1.0, .625}, {-1.0, .75},  {1.0, .125},
         {1.0, .23},   {1.0, .25},   {1.0, .37},   {1.0, .5},
         {1.0, .625},  {1.0, .75}}};
    for (const int degree : {3, 4, 5, 6}) {
      for (const auto& [curvatureSign, knot] : orientationAndKnotCases) {
        RS_SplineData data{degree, false};
        const int controlCount = 2 * degree - 1;
        for (int i = 0; i < controlCount; ++i) {
            const double x = static_cast<double>(i);
            const double y = curvatureSign * (i == degree - 3 ? -5.0 : .001 * x * x * x);
            data.controlPoints.emplace_back(x, y);
            data.weights.push_back(weightPattern[static_cast<std::size_t>(i) %
                                                 weightPattern.size()]);
        }
        data.knotslist.assign(static_cast<std::size_t>(degree + 1), 0.0);
        data.knotslist.insert(data.knotslist.end(),
                              static_cast<std::size_t>(degree - 2), knot);
        data.knotslist.insert(data.knotslist.end(),
                              static_cast<std::size_t>(degree + 1), 1.0);
        const RS_Spline source{nullptr, data};
        REQUIRE(source.validate());
        LC_CurveJet leftJet;
        LC_CurveJet rightJet;
        REQUIRE(source.tryEvaluateJet(knot, LC_CurveEvaluationSide::Left, leftJet));
        REQUIRE(source.tryEvaluateJet(knot, LC_CurveEvaluationSide::Right, rightJet));
        const auto curvature = [](const LC_CurveJet& jet) {
            const double speedSquared = jet.first.x * jet.first.x + jet.first.y * jet.first.y;
            return (jet.first.x * jet.second.y - jet.first.y * jet.second.x) /
                   (speedSquared * std::sqrt(speedSquared));
        };
        const double leftCurvature = curvature(leftJet);
        const double rightCurvature = curvature(rightJet);
        CAPTURE(degree, curvatureSign, leftCurvature, rightCurvature);
        REQUIRE(std::isfinite(leftCurvature));
        REQUIRE(std::isfinite(rightCurvature));

        const SplineOracle start{0};
        const SplineOracle end{1};
        const SplineOracle step{"1e-24"};
        const SplineOracle knotParameter{knot};
        const auto leftKnotCurvature =
            oracleSplineCurvatureLimit(data, knotParameter, true);
        const auto rightKnotCurvature =
            oracleSplineCurvatureLimit(data, knotParameter, false);
        REQUIRE(leftKnotCurvature);
        REQUIRE(rightKnotCurvature);
        REQUIRE(abs(*leftKnotCurvature - *rightKnotCurvature) < SplineOracle{"1e-30"});
        const double oracleLeftCurvature = leftKnotCurvature->convert_to<double>();
        const double oracleRightCurvature = rightKnotCurvature->convert_to<double>();
        CHECK(std::abs(leftCurvature - oracleLeftCurvature) <=
              1e-12 * std::max(1.0, std::abs(oracleLeftCurvature)));
        CHECK(std::abs(rightCurvature - oracleRightCurvature) <=
              1e-12 * std::max(1.0, std::abs(oracleRightCurvature)));
        const double signedDistance =
            (2 / (*leftKnotCurvature + *rightKnotCurvature)).convert_to<double>();
        const SplineOracle before = oracleOffsetFactor(
            data, knotParameter - SplineOracle{"1e-7"}, step,
            start, end, SplineOracle{signedDistance});
        const SplineOracle after = oracleOffsetFactor(
            data, knotParameter + SplineOracle{"1e-7"}, step,
            start, end, SplineOracle{signedDistance});
        CAPTURE(degree, curvatureSign, signedDistance, before, after);
        REQUIRE((before < 0) != (after < 0));

        LC_CurveJetBounds leftBounds;
        LC_CurveJetBounds rightBounds;
        REQUIRE(source.tryBoundJetForCusp(knot - 1e-6, knot, leftBounds, signedDistance));
        REQUIRE(source.tryBoundJetForCusp(knot, knot + 1e-6, rightBounds, signedDistance));
        CHECK(leftBounds.rationalCuspIntervalVariationUpperBound >= 0);
        CHECK(rightBounds.rationalCuspIntervalVariationUpperBound >= 0);
        CHECK(leftBounds.rationalCuspIntervalVariationUpperBound +
                  rightBounds.rationalCuspIntervalVariationUpperBound <=
              2);

        const LC_OffsetSourceDefinition definition{
            LC_OffsetSourceGeometry{data}, signedDistance, 1e-4, false};
        const auto options = LC_OffsetWorkerTolerance::directOptions(definition, source);
        REQUIRE(options);
        LC_CurveOffsetCuspIsolationDiagnostics diagnostics;
        const auto built = LC_CurveOffset::buildDirectBranches(
            source,
            LC_CurveOffset::makeSideRequest(
                signedDistance > 0.0 ? LC_CurveOffsetSide::Left
                                     : LC_CurveOffsetSide::Right,
                std::abs(signedDistance)),
            *options, LC_CurveOffset::makeDirectSourceBudget(), &diagnostics);
        CAPTURE(degree, curvatureSign, built.status, diagnostics.failureStage,
                diagnostics.candidateParameters, diagnostics.stallIntervals);
        REQUIRE(built.status == LC_CurveOffsetStatus::Ok);
        const auto cubicPoint = [](const std::array<RS_Vector, 4>& bezier,
                                   const double parameter) {
            const double inverse = 1.0 - parameter;
            return bezier[0] * (inverse * inverse * inverse) +
                   bezier[1] * (3.0 * inverse * inverse * parameter) +
                   bezier[2] * (3.0 * inverse * parameter * parameter) +
                   bezier[3] * (parameter * parameter * parameter);
        };
        std::size_t checkedOffsetSamples = 0;
        double maximumOffsetError = 0.0;
        for (const auto& branch : built.branches) {
            for (const auto& piece : branch.cubicPieces) {
                const double localWidth = piece.localU1 - piece.localU0;
                REQUIRE(localWidth > 0.0);
                for (const double fraction : {.2, .5, .8}) {
                    const double localParameter = piece.localU0 + localWidth * fraction;
                    const double sourceParameter = piece.provenance.sourceT0 +
                        (piece.provenance.sourceT1 - piece.provenance.sourceT0) * fraction;
                    const auto expectedPoint = oracleOffsetPoint(
                        data, SplineOracle{sourceParameter}, step, start, end,
                        SplineOracle{signedDistance});
                    const RS_Vector expected{
                        expectedPoint.first.convert_to<double>(),
                        expectedPoint.second.convert_to<double>()};
                    const double error = cubicPoint(piece.bezier, localParameter)
                                             .distanceTo(expected);
                    maximumOffsetError = std::max(maximumOffsetError, error);
                    ++checkedOffsetSamples;
                }
            }
        }
        CAPTURE(degree, curvatureSign, checkedOffsetSamples, maximumOffsetError,
                options->tolerance.requestedGeometry);
        REQUIRE(checkedOffsetSamples > 0);
        CHECK(maximumOffsetError <= options->tolerance.requestedGeometry);
        const double knotPosition = knot;
        const auto candidate = std::find_if(
            diagnostics.candidateBoxes.begin(), diagnostics.candidateBoxes.end(),
            [knotPosition](const LC_CurveOffsetCuspCandidateBox& box) {
                return std::abs(box.parameter - knotPosition) < 1e-8;
            });
        REQUIRE(candidate != diagnostics.candidateBoxes.end());
        CHECK(candidate->kind == LC_CurveOffsetCuspCandidateKind::CertifiedUniqueBracket);
        const auto oraclePoint = oracleOffsetPoint(
            data, knotParameter, step, start, end, SplineOracle{signedDistance});
        const RS_Vector expected{oraclePoint.first.convert_to<double>(),
                                 oraclePoint.second.convert_to<double>()};
        double endpointError = std::numeric_limits<double>::infinity();
        for (const auto& branch : built.branches) {
            if (branch.cubicPieces.empty()) continue;
            if (branch.startEnd == LC_OffsetBranchEnd::Cusp) {
                endpointError = std::min(
                    endpointError,
                    branch.cubicPieces.front().bezier.front().distanceTo(expected));
            }
            if (branch.endEnd == LC_OffsetBranchEnd::Cusp) {
                endpointError = std::min(
                    endpointError,
                    branch.cubicPieces.back().bezier.back().distanceTo(expected));
            }
        }
        CAPTURE(degree, curvatureSign, endpointError, options->tolerance.requestedGeometry);
        CHECK(endpointError <= options->tolerance.requestedGeometry);

        const req::Request generationRequest{
            token, req::GenerateSource{definition, {}}};
        const auto generationReply = LC_OffsetWorkerKernel::run(generationRequest);
        const auto& generated = native(generationReply, 7);
        CAPTURE(degree, curvatureSign, generated.engineStatus,
                generated.refusal, generated.ordinaryStatus,
                generated.ordinary.size());
        REQUIRE(generated.engineStatus == LC_CurveOffsetStatus::Ok);
        REQUIRE(generated.ordinaryStatus == result::OrdinaryStatus::Complete);
        REQUIRE_FALSE(generated.ordinary.empty());
        const auto wire = result::encode(generationReply, generationRequest);
        REQUIRE(wire);
        const auto decoded = result::decode(*wire, generationRequest);
        REQUIRE(decoded);
        const auto& decodedGenerated = native(*decoded, 7);
        CHECK(decodedGenerated.engineStatus == generated.engineStatus);
        CHECK(decodedGenerated.refusal == generated.refusal);
        CHECK(decodedGenerated.ordinaryStatus == generated.ordinaryStatus);
        CHECK(decodedGenerated.ordinary.size() == generated.ordinary.size());

        RS_SplineData cache;
        REQUIRE(LC_OffsetWorkerSourceCache::combine(generated.ordinary, cache));
        const req::Request validationRequest{
            token, req::ValidateSourceCache{cache, definition, {}}};
        const auto validationReply = LC_OffsetWorkerKernel::run(validationRequest);
        const auto& validated = native(validationReply, 8);
        CAPTURE(degree, curvatureSign, validated.refusal,
                validated.engineStatus, validated.changed);
        CHECK(validated.refusal == LC_OffsetCurveRefusal::None);
        CHECK(validated.changed);
        CHECK(validated.engineStatus == LC_CurveOffsetStatus::Ok);
        const auto validationWire = result::encode(validationReply, validationRequest);
        REQUIRE(validationWire);
        const auto decodedValidation = result::decode(*validationWire, validationRequest);
        REQUIRE(decodedValidation);
        const auto& transportedValidation = native(*decodedValidation, 8);
        CHECK(transportedValidation.refusal == validated.refusal);
        CHECK(transportedValidation.changed == validated.changed);
        CHECK(transportedValidation.engineStatus == validated.engineStatus);
      }
    }
}

TEST_CASE("C2 rational knot crossings map across continuous multiplicities",
          "[lc-offsetcurve][worker-kernel][spline-variant][rational][knot][multiplicity]") {
    const std::array<double, 11> weightPattern{
        1.0, 1.02, .98, 1.03, .97, 1.01, .99, 1.04, .96, 1.02, .98};
    const SplineOracle start{0};
    const SplineOracle end{1};
    const SplineOracle derivativeStep{"1e-24"};
    constexpr double knot = .5;

    for (const int degree : {3, 4, 5, 6}) {
        for (int multiplicity = 1; multiplicity <= degree - 2; ++multiplicity) {
            for (const double curvatureSign : {-1.0, 1.0}) {
                RS_SplineData data{degree, false};
                const int controlCount = degree + 1 + multiplicity;
                for (int i = 0; i < controlCount; ++i) {
                    const double x = static_cast<double>(i);
                    const double y = curvatureSign *
                        (i == degree - 3 ? -5.0 : .001 * x * x * x);
                    data.controlPoints.emplace_back(x, y);
                    data.weights.push_back(weightPattern[
                        static_cast<std::size_t>(i) % weightPattern.size()]);
                }
                data.knotslist.assign(static_cast<std::size_t>(degree + 1), 0.0);
                data.knotslist.insert(data.knotslist.end(),
                                      static_cast<std::size_t>(multiplicity), knot);
                data.knotslist.insert(data.knotslist.end(),
                                      static_cast<std::size_t>(degree + 1), 1.0);
                const RS_Spline source{nullptr, data};
                CAPTURE(degree, multiplicity, curvatureSign);
                REQUIRE(source.validate());

                const auto leftCurvature = oracleSplineCurvatureLimit(
                    data, SplineOracle{knot}, true);
                const auto rightCurvature = oracleSplineCurvatureLimit(
                    data, SplineOracle{knot}, false);
                REQUIRE(leftCurvature);
                REQUIRE(rightCurvature);
                REQUIRE(abs(*leftCurvature - *rightCurvature) < SplineOracle{"1e-30"});
                const double signedDistance =
                    (2 / (*leftCurvature + *rightCurvature)).convert_to<double>();
                REQUIRE(std::isfinite(signedDistance));

                const SplineOracle before = oracleOffsetFactor(
                    data, SplineOracle{knot} - SplineOracle{"1e-7"}, derivativeStep,
                    start, end, SplineOracle{signedDistance});
                const SplineOracle after = oracleOffsetFactor(
                    data, SplineOracle{knot} + SplineOracle{"1e-7"}, derivativeStep,
                    start, end, SplineOracle{signedDistance});
                REQUIRE((before < 0) != (after < 0));

                const LC_OffsetSourceDefinition definition{
                    LC_OffsetSourceGeometry{data}, signedDistance, 1e-4, false};
                const auto options = LC_OffsetWorkerTolerance::directOptions(definition, source);
                REQUIRE(options);
                LC_CurveOffsetCuspIsolationDiagnostics diagnostics;
                const auto built = LC_CurveOffset::buildDirectBranches(
                    source,
                    LC_CurveOffset::makeSideRequest(
                        signedDistance > 0 ? LC_CurveOffsetSide::Left
                                           : LC_CurveOffsetSide::Right,
                        std::abs(signedDistance)),
                    *options, LC_CurveOffset::makeDirectSourceBudget(), &diagnostics);
                CAPTURE(built.status, diagnostics.failureStage,
                        diagnostics.candidateParameters);
                REQUIRE(built.status == LC_CurveOffsetStatus::Ok);
                const auto candidate = std::find_if(
                    diagnostics.candidateBoxes.begin(), diagnostics.candidateBoxes.end(),
                    [](const LC_CurveOffsetCuspCandidateBox& box) {
                        return std::abs(box.parameter - knot) < 1e-8;
                    });
                REQUIRE(candidate != diagnostics.candidateBoxes.end());
                CHECK(candidate->kind ==
                      LC_CurveOffsetCuspCandidateKind::CertifiedUniqueBracket);
                const auto oraclePoint = oracleOffsetPoint(
                    data, SplineOracle{knot}, derivativeStep, start, end,
                    SplineOracle{signedDistance});
                const RS_Vector expected{oraclePoint.first.convert_to<double>(),
                                         oraclePoint.second.convert_to<double>()};
                double endpointError = std::numeric_limits<double>::infinity();
                for (const auto& branch : built.branches) {
                    if (branch.cubicPieces.empty()) continue;
                    if (branch.startEnd == LC_OffsetBranchEnd::Cusp)
                        endpointError = std::min(endpointError,
                            branch.cubicPieces.front().bezier.front().distanceTo(expected));
                    if (branch.endEnd == LC_OffsetBranchEnd::Cusp)
                        endpointError = std::min(endpointError,
                            branch.cubicPieces.back().bezier.back().distanceTo(expected));
                }
                CHECK(endpointError <= options->tolerance.requestedGeometry);

                const req::Request generationRequest{
                    token, req::GenerateSource{definition, {}}};
                const auto generatedReply =
                    LC_OffsetWorkerKernel::run(generationRequest);
                const auto& generated = native(generatedReply, 7);
                REQUIRE(generated.engineStatus == LC_CurveOffsetStatus::Ok);
                REQUIRE(generated.ordinaryStatus == result::OrdinaryStatus::Complete);
                const auto generatedWire = result::encode(
                    generatedReply, generationRequest);
                REQUIRE(generatedWire);
                REQUIRE(result::decode(*generatedWire, generationRequest));

                RS_SplineData cache;
                REQUIRE(LC_OffsetWorkerSourceCache::combine(generated.ordinary, cache));
                const req::Request validationRequest{
                    token, req::ValidateSourceCache{cache, definition, {}}};
                const auto validationReply =
                    LC_OffsetWorkerKernel::run(validationRequest);
                const auto& validated = native(validationReply, 8);
                CHECK(validated.refusal == LC_OffsetCurveRefusal::None);
                CHECK(validated.changed);
                CHECK(validated.engineStatus == LC_CurveOffsetStatus::Ok);
                const auto validationWire = result::encode(
                    validationReply, validationRequest);
                REQUIRE(validationWire);
                REQUIRE(result::decode(*validationWire, validationRequest));
            }
        }
    }
}

TEST_CASE("rational C2 knot crossing maps in a two-knot layout",
          "[lc-offsetcurve][worker-kernel][spline-variant][rational][knot][layout]") {
    const std::array<double, 11> weightPattern{1.0, 1.02, .98, 1.03, .97, 1.01, .99, 1.04, .96, 1.02, .98};
    const SplineOracle domainStart{0};
    const SplineOracle domainEnd{1};
    const SplineOracle derivativeStep{"1e-24"};
    constexpr double targetKnot = .5;

    for (const int degree : {3, 4, 5, 6}) {
        const int multiplicity = degree - 2;
        const int controlCount = degree + 1 + 2 * multiplicity;
        for (const double curvatureSign : {-1.0, 1.0}) {
            RS_SplineData data{degree, false};
            const int curvatureControl = multiplicity + degree - 3;
            for (int i = 0; i < controlCount; ++i) {
                const double x = static_cast<double>(i);
                const double y = curvatureSign * (i == curvatureControl ? -5.0 : .001 * x * x * x);
                data.controlPoints.emplace_back(x, y);
                data.weights.push_back(weightPattern[static_cast<std::size_t>(i) % weightPattern.size()]);
            }
            data.knotslist.assign(static_cast<std::size_t>(degree + 1), 0.0);
            for (const double knot : {.25, .5})
                data.knotslist.insert(data.knotslist.end(), static_cast<std::size_t>(multiplicity), knot);
            data.knotslist.insert(data.knotslist.end(), static_cast<std::size_t>(degree + 1), 1.0);
            const RS_Spline source{nullptr, data};
            CAPTURE(degree, multiplicity, curvatureSign, curvatureControl);
            REQUIRE(source.validate());

            const auto leftCurvature = oracleSplineCurvatureLimit(data, SplineOracle{targetKnot}, true);
            const auto rightCurvature = oracleSplineCurvatureLimit(data, SplineOracle{targetKnot}, false);
            REQUIRE(leftCurvature);
            REQUIRE(rightCurvature);
            REQUIRE(abs(*leftCurvature - *rightCurvature) < SplineOracle{"1e-30"});
            const double signedDistance = (2 / (*leftCurvature + *rightCurvature)).convert_to<double>();
            REQUIRE(std::isfinite(signedDistance));

            const SplineOracle before = oracleOffsetFactor(data, SplineOracle{targetKnot} - SplineOracle{"1e-7"}, derivativeStep,
                                                           domainStart, domainEnd, SplineOracle{signedDistance});
            const SplineOracle after = oracleOffsetFactor(data, SplineOracle{targetKnot} + SplineOracle{"1e-7"}, derivativeStep,
                                                          domainStart, domainEnd, SplineOracle{signedDistance});
            REQUIRE((before < 0) != (after < 0));

            const LC_OffsetSourceDefinition definition{LC_OffsetSourceGeometry{data}, signedDistance, 1e-4, false};
            const auto options = LC_OffsetWorkerTolerance::directOptions(definition, source);
            REQUIRE(options);
            LC_CurveOffsetCuspIsolationDiagnostics diagnostics;
            const auto built = LC_CurveOffset::buildDirectBranches(
                source,
                LC_CurveOffset::makeSideRequest(signedDistance > 0 ? LC_CurveOffsetSide::Left : LC_CurveOffsetSide::Right,
                                                std::abs(signedDistance)),
                *options, LC_CurveOffset::makeDirectSourceBudget(), &diagnostics);
            CAPTURE(built.status, diagnostics.failureStage, diagnostics.candidateParameters);
            REQUIRE(built.status == LC_CurveOffsetStatus::Ok);
            const auto candidate =
                std::find_if(diagnostics.candidateBoxes.begin(), diagnostics.candidateBoxes.end(),
                             [](const LC_CurveOffsetCuspCandidateBox& box) { return std::abs(box.parameter - targetKnot) < 1e-8; });
            REQUIRE(candidate != diagnostics.candidateBoxes.end());
            CHECK(candidate->kind == LC_CurveOffsetCuspCandidateKind::CertifiedUniqueBracket);
            const auto expectedPoint =
                oracleOffsetPoint(data, SplineOracle{targetKnot}, derivativeStep, domainStart, domainEnd, SplineOracle{signedDistance});
            const RS_Vector expected{expectedPoint.first.convert_to<double>(), expectedPoint.second.convert_to<double>()};
            double endpointError = std::numeric_limits<double>::infinity();
            for (const auto& branch : built.branches) {
                if (branch.cubicPieces.empty())
                    continue;
                if (branch.startEnd == LC_OffsetBranchEnd::Cusp)
                    endpointError = std::min(endpointError, branch.cubicPieces.front().bezier.front().distanceTo(expected));
                if (branch.endEnd == LC_OffsetBranchEnd::Cusp)
                    endpointError = std::min(endpointError, branch.cubicPieces.back().bezier.back().distanceTo(expected));
            }
            CHECK(endpointError <= options->tolerance.requestedGeometry);

            const req::Request generationRequest{token, req::GenerateSource{definition, {}}};
            const auto generationReply = LC_OffsetWorkerKernel::run(generationRequest);
            const auto& generated = native(generationReply, 7);
            REQUIRE(generated.engineStatus == LC_CurveOffsetStatus::Ok);
            REQUIRE(generated.ordinaryStatus == result::OrdinaryStatus::Complete);
            const auto generationWire = result::encode(generationReply, generationRequest);
            REQUIRE(generationWire);
            REQUIRE(result::decode(*generationWire, generationRequest));
            RS_SplineData cache;
            REQUIRE(LC_OffsetWorkerSourceCache::combine(generated.ordinary, cache));
            const req::Request validationRequest{token, req::ValidateSourceCache{cache, definition, {}}};
            const auto validationReply = LC_OffsetWorkerKernel::run(validationRequest);
            const auto& validated = native(validationReply, 8);
            CHECK(validated.refusal == LC_OffsetCurveRefusal::None);
            CHECK(validated.changed);
            CHECK(validated.engineStatus == LC_CurveOffsetStatus::Ok);
        }
    }
}

TEST_CASE("rational C2 knot crossing maps in an asymmetric two-knot layout",
          "[lc-offsetcurve][worker-kernel][spline-variant][rational][knot][layout]") {
    const std::array<double, 11> weightPattern{1.0, 1.02, .98, 1.03, .97, 1.01, .99, 1.04, .96, 1.02, .98};
    const SplineOracle domainStart{0};
    const SplineOracle domainEnd{1};
    const SplineOracle derivativeStep{"1e-24"};
    constexpr double firstKnot = .25;
    constexpr double targetKnot = .65;

    for (const int degree : {3, 4, 5, 6}) {
        const int multiplicity = degree - 2;
        const int controlCount = degree + 1 + 2 * multiplicity;
        for (const double curvatureSign : {-1.0, 1.0}) {
            RS_SplineData data{degree, false};
            const int curvatureControl = multiplicity + degree - 3;
            for (int i = 0; i < controlCount; ++i) {
                const double x = static_cast<double>(i);
                const double y = curvatureSign * (i == curvatureControl ? -5.0 : .001 * x * x * x);
                data.controlPoints.emplace_back(x, y);
                data.weights.push_back(weightPattern[static_cast<std::size_t>(i) % weightPattern.size()]);
            }
            data.knotslist.assign(static_cast<std::size_t>(degree + 1), 0.0);
            for (const double knot : {firstKnot, targetKnot})
                data.knotslist.insert(data.knotslist.end(), static_cast<std::size_t>(multiplicity), knot);
            data.knotslist.insert(data.knotslist.end(), static_cast<std::size_t>(degree + 1), 1.0);
            const RS_Spline source{nullptr, data};
            CAPTURE(degree, multiplicity, curvatureSign, curvatureControl, targetKnot);
            REQUIRE(source.validate());

            const auto leftCurvature = oracleSplineCurvatureLimit(data, SplineOracle{targetKnot}, true);
            const auto rightCurvature = oracleSplineCurvatureLimit(data, SplineOracle{targetKnot}, false);
            REQUIRE(leftCurvature);
            REQUIRE(rightCurvature);
            REQUIRE(abs(*leftCurvature - *rightCurvature) < SplineOracle{"1e-30"});
            const double signedDistance = (2 / (*leftCurvature + *rightCurvature)).convert_to<double>();
            REQUIRE(std::isfinite(signedDistance));

            const SplineOracle before = oracleOffsetFactor(data, SplineOracle{targetKnot} - SplineOracle{"1e-7"}, derivativeStep,
                                                           domainStart, domainEnd, SplineOracle{signedDistance});
            const SplineOracle after = oracleOffsetFactor(data, SplineOracle{targetKnot} + SplineOracle{"1e-7"}, derivativeStep,
                                                          domainStart, domainEnd, SplineOracle{signedDistance});
            REQUIRE((before < 0) != (after < 0));

            const LC_OffsetSourceDefinition definition{LC_OffsetSourceGeometry{data}, signedDistance, 1e-4, false};
            const auto options = LC_OffsetWorkerTolerance::directOptions(definition, source);
            REQUIRE(options);
            LC_CurveOffsetCuspIsolationDiagnostics diagnostics;
            const auto built = LC_CurveOffset::buildDirectBranches(
                source,
                LC_CurveOffset::makeSideRequest(signedDistance > 0 ? LC_CurveOffsetSide::Left : LC_CurveOffsetSide::Right,
                                                std::abs(signedDistance)),
                *options, LC_CurveOffset::makeDirectSourceBudget(), &diagnostics);
            CAPTURE(built.status, diagnostics.failureStage, diagnostics.candidateParameters);
            REQUIRE(built.status == LC_CurveOffsetStatus::Ok);
            const auto candidate =
                std::find_if(diagnostics.candidateBoxes.begin(), diagnostics.candidateBoxes.end(),
                             [](const LC_CurveOffsetCuspCandidateBox& box) { return std::abs(box.parameter - targetKnot) < 1e-8; });
            REQUIRE(candidate != diagnostics.candidateBoxes.end());
            CHECK(candidate->kind == LC_CurveOffsetCuspCandidateKind::CertifiedUniqueBracket);
            const auto expectedPoint =
                oracleOffsetPoint(data, SplineOracle{targetKnot}, derivativeStep, domainStart, domainEnd, SplineOracle{signedDistance});
            const RS_Vector expected{expectedPoint.first.convert_to<double>(), expectedPoint.second.convert_to<double>()};
            double endpointError = std::numeric_limits<double>::infinity();
            for (const auto& branch : built.branches) {
                if (branch.cubicPieces.empty())
                    continue;
                if (branch.startEnd == LC_OffsetBranchEnd::Cusp)
                    endpointError = std::min(endpointError, branch.cubicPieces.front().bezier.front().distanceTo(expected));
                if (branch.endEnd == LC_OffsetBranchEnd::Cusp)
                    endpointError = std::min(endpointError, branch.cubicPieces.back().bezier.back().distanceTo(expected));
            }
            CHECK(endpointError <= options->tolerance.requestedGeometry);

            const req::Request generationRequest{token, req::GenerateSource{definition, {}}};
            const auto generationReply = LC_OffsetWorkerKernel::run(generationRequest);
            const auto& generated = native(generationReply, 7);
            REQUIRE(generated.engineStatus == LC_CurveOffsetStatus::Ok);
            REQUIRE(generated.ordinaryStatus == result::OrdinaryStatus::Complete);
            const auto generationWire = result::encode(generationReply, generationRequest);
            REQUIRE(generationWire);
            REQUIRE(result::decode(*generationWire, generationRequest));
            RS_SplineData cache;
            REQUIRE(LC_OffsetWorkerSourceCache::combine(generated.ordinary, cache));
            const req::Request validationRequest{token, req::ValidateSourceCache{cache, definition, {}}};
            const auto validationReply = LC_OffsetWorkerKernel::run(validationRequest);
            const auto& validated = native(validationReply, 8);
            CHECK(validated.refusal == LC_OffsetCurveRefusal::None);
            CHECK(validated.changed);
            CHECK(validated.engineStatus == LC_CurveOffsetStatus::Ok);
        }
    }
}

TEST_CASE("rational C2 knot crossing maps in a three-knot layout",
          "[lc-offsetcurve][worker-kernel][spline-variant][rational][knot][layout]") {
    const std::array<double, 11> weightPattern{1.0, 1.02, .98, 1.03, .97, 1.01, .99, 1.04, .96, 1.02, .98};
    const SplineOracle domainStart{0};
    const SplineOracle domainEnd{1};
    const SplineOracle derivativeStep{"1e-24"};
    const std::array<double, 3> interiorKnots{.2, .45, .7};
    constexpr double targetKnot = .7;

    for (const int degree : {3, 4, 5, 6}) {
        const int multiplicity = degree - 2;
        const int controlCount = degree + 1 + 3 * multiplicity;
        for (const double curvatureSign : {-1.0, 1.0}) {
            RS_SplineData data{degree, false};
            const int curvatureControl = 2 * multiplicity + degree - 3;
            for (int i = 0; i < controlCount; ++i) {
                const double x = static_cast<double>(i);
                const double y = curvatureSign * (i == curvatureControl ? -5.0 : .001 * x * x * x);
                data.controlPoints.emplace_back(x, y);
                data.weights.push_back(weightPattern[static_cast<std::size_t>(i) % weightPattern.size()]);
            }
            data.knotslist.assign(static_cast<std::size_t>(degree + 1), 0.0);
            for (const double knot : interiorKnots)
                data.knotslist.insert(data.knotslist.end(), static_cast<std::size_t>(multiplicity), knot);
            data.knotslist.insert(data.knotslist.end(), static_cast<std::size_t>(degree + 1), 1.0);
            const RS_Spline source{nullptr, data};
            CAPTURE(degree, multiplicity, curvatureSign, curvatureControl, targetKnot, controlCount);
            REQUIRE(source.validate());

            const auto leftCurvature = oracleSplineCurvatureLimit(data, SplineOracle{targetKnot}, true);
            const auto rightCurvature = oracleSplineCurvatureLimit(data, SplineOracle{targetKnot}, false);
            REQUIRE(leftCurvature);
            REQUIRE(rightCurvature);
            REQUIRE(abs(*leftCurvature - *rightCurvature) < SplineOracle{"1e-30"});
            const double signedDistance = (2 / (*leftCurvature + *rightCurvature)).convert_to<double>();
            REQUIRE(std::isfinite(signedDistance));

            const SplineOracle before = oracleOffsetFactor(data, SplineOracle{targetKnot} - SplineOracle{"1e-7"}, derivativeStep,
                                                           domainStart, domainEnd, SplineOracle{signedDistance});
            const SplineOracle after = oracleOffsetFactor(data, SplineOracle{targetKnot} + SplineOracle{"1e-7"}, derivativeStep,
                                                          domainStart, domainEnd, SplineOracle{signedDistance});
            REQUIRE((before < 0) != (after < 0));

            const LC_OffsetSourceDefinition definition{LC_OffsetSourceGeometry{data}, signedDistance, 1e-4, false};
            const auto options = LC_OffsetWorkerTolerance::directOptions(definition, source);
            REQUIRE(options);
            LC_CurveOffsetCuspIsolationDiagnostics diagnostics;
            const auto built = LC_CurveOffset::buildDirectBranches(
                source,
                LC_CurveOffset::makeSideRequest(signedDistance > 0 ? LC_CurveOffsetSide::Left : LC_CurveOffsetSide::Right,
                                                std::abs(signedDistance)),
                *options, LC_CurveOffset::makeDirectSourceBudget(), &diagnostics);
            CAPTURE(built.status, diagnostics.failureStage, diagnostics.candidateParameters);
            REQUIRE(built.status == LC_CurveOffsetStatus::Ok);
            const auto candidate =
                std::find_if(diagnostics.candidateBoxes.begin(), diagnostics.candidateBoxes.end(),
                             [](const LC_CurveOffsetCuspCandidateBox& box) { return std::abs(box.parameter - targetKnot) < 1e-8; });
            REQUIRE(candidate != diagnostics.candidateBoxes.end());
            CHECK(candidate->kind == LC_CurveOffsetCuspCandidateKind::CertifiedUniqueBracket);
            const auto expectedPoint =
                oracleOffsetPoint(data, SplineOracle{targetKnot}, derivativeStep, domainStart, domainEnd, SplineOracle{signedDistance});
            const RS_Vector expected{expectedPoint.first.convert_to<double>(), expectedPoint.second.convert_to<double>()};
            double endpointError = std::numeric_limits<double>::infinity();
            for (const auto& branch : built.branches) {
                if (branch.cubicPieces.empty())
                    continue;
                if (branch.startEnd == LC_OffsetBranchEnd::Cusp)
                    endpointError = std::min(endpointError, branch.cubicPieces.front().bezier.front().distanceTo(expected));
                if (branch.endEnd == LC_OffsetBranchEnd::Cusp)
                    endpointError = std::min(endpointError, branch.cubicPieces.back().bezier.back().distanceTo(expected));
            }
            CHECK(endpointError <= options->tolerance.requestedGeometry);

            const req::Request generationRequest{token, req::GenerateSource{definition, {}}};
            const auto generationReply = LC_OffsetWorkerKernel::run(generationRequest);
            const auto& generated = native(generationReply, 7);
            REQUIRE(generated.engineStatus == LC_CurveOffsetStatus::Ok);
            REQUIRE(generated.ordinaryStatus == result::OrdinaryStatus::Complete);
            const auto generationWire = result::encode(generationReply, generationRequest);
            REQUIRE(generationWire);
            REQUIRE(result::decode(*generationWire, generationRequest));
            RS_SplineData cache;
            REQUIRE(LC_OffsetWorkerSourceCache::combine(generated.ordinary, cache));
            const req::Request validationRequest{token, req::ValidateSourceCache{cache, definition, {}}};
            const auto validationReply = LC_OffsetWorkerKernel::run(validationRequest);
            const auto& validated = native(validationReply, 8);
            CHECK(validated.refusal == LC_OffsetCurveRefusal::None);
            CHECK(validated.changed);
            CHECK(validated.engineStatus == LC_CurveOffsetStatus::Ok);
        }
    }
}

TEST_CASE("rational C2 knot crossing maps in a four-knot layout",
          "[lc-offsetcurve][worker-kernel][spline-variant][rational][knot][layout]") {
    const std::array<double, 11> weightPattern{1.0, 1.02, .98, 1.03, .97, 1.01, .99, 1.04, .96, 1.02, .98};
    const SplineOracle domainStart{0};
    const SplineOracle domainEnd{1};
    const SplineOracle derivativeStep{"1e-24"};
    const std::array<double, 4> interiorKnots{.12, .31, .58, .82};
    constexpr double targetKnot = .82;

    for (const int degree : {3, 4, 5, 6}) {
        const int multiplicity = degree - 2;
        const int controlCount = degree + 1 + 4 * multiplicity;
        for (const double curvatureSign : {-1.0, 1.0}) {
            RS_SplineData data{degree, false};
            const int curvatureControl = 3 * multiplicity + degree - 3;
            for (int i = 0; i < controlCount; ++i) {
                const double x = static_cast<double>(i);
                const double y = curvatureSign * (i == curvatureControl ? -5.0 : .001 * x * x * x);
                data.controlPoints.emplace_back(x, y);
                data.weights.push_back(weightPattern[static_cast<std::size_t>(i) % weightPattern.size()]);
            }
            data.knotslist.assign(static_cast<std::size_t>(degree + 1), 0.0);
            for (const double knot : interiorKnots)
                data.knotslist.insert(data.knotslist.end(), static_cast<std::size_t>(multiplicity), knot);
            data.knotslist.insert(data.knotslist.end(), static_cast<std::size_t>(degree + 1), 1.0);
            const RS_Spline source{nullptr, data};
            CAPTURE(degree, multiplicity, curvatureSign, curvatureControl, targetKnot, controlCount);
            REQUIRE(source.validate());

            const auto leftCurvature = oracleSplineCurvatureLimit(data, SplineOracle{targetKnot}, true);
            const auto rightCurvature = oracleSplineCurvatureLimit(data, SplineOracle{targetKnot}, false);
            REQUIRE(leftCurvature);
            REQUIRE(rightCurvature);
            REQUIRE(abs(*leftCurvature - *rightCurvature) < SplineOracle{"1e-30"});
            const double signedDistance = (2 / (*leftCurvature + *rightCurvature)).convert_to<double>();
            REQUIRE(std::isfinite(signedDistance));

            const SplineOracle before = oracleOffsetFactor(data, SplineOracle{targetKnot} - SplineOracle{"1e-7"}, derivativeStep,
                                                           domainStart, domainEnd, SplineOracle{signedDistance});
            const SplineOracle after = oracleOffsetFactor(data, SplineOracle{targetKnot} + SplineOracle{"1e-7"}, derivativeStep,
                                                          domainStart, domainEnd, SplineOracle{signedDistance});
            REQUIRE((before < 0) != (after < 0));

            const LC_OffsetSourceDefinition definition{LC_OffsetSourceGeometry{data}, signedDistance, 1e-4, false};
            const auto options = LC_OffsetWorkerTolerance::directOptions(definition, source);
            REQUIRE(options);
            LC_CurveOffsetCuspIsolationDiagnostics diagnostics;
            const auto built = LC_CurveOffset::buildDirectBranches(
                source,
                LC_CurveOffset::makeSideRequest(signedDistance > 0 ? LC_CurveOffsetSide::Left : LC_CurveOffsetSide::Right,
                                                std::abs(signedDistance)),
                *options, LC_CurveOffset::makeDirectSourceBudget(), &diagnostics);
            CAPTURE(built.status, diagnostics.failureStage, diagnostics.candidateParameters);
            REQUIRE(built.status == LC_CurveOffsetStatus::Ok);
            const auto candidate =
                std::find_if(diagnostics.candidateBoxes.begin(), diagnostics.candidateBoxes.end(),
                             [](const LC_CurveOffsetCuspCandidateBox& box) { return std::abs(box.parameter - targetKnot) < 1e-8; });
            REQUIRE(candidate != diagnostics.candidateBoxes.end());
            CHECK(candidate->kind == LC_CurveOffsetCuspCandidateKind::CertifiedUniqueBracket);
            const auto expectedPoint =
                oracleOffsetPoint(data, SplineOracle{targetKnot}, derivativeStep, domainStart, domainEnd, SplineOracle{signedDistance});
            const RS_Vector expected{expectedPoint.first.convert_to<double>(), expectedPoint.second.convert_to<double>()};
            double endpointError = std::numeric_limits<double>::infinity();
            for (const auto& branch : built.branches) {
                if (branch.cubicPieces.empty())
                    continue;
                if (branch.startEnd == LC_OffsetBranchEnd::Cusp)
                    endpointError = std::min(endpointError, branch.cubicPieces.front().bezier.front().distanceTo(expected));
                if (branch.endEnd == LC_OffsetBranchEnd::Cusp)
                    endpointError = std::min(endpointError, branch.cubicPieces.back().bezier.back().distanceTo(expected));
            }
            CHECK(endpointError <= options->tolerance.requestedGeometry);

            const req::Request generationRequest{token, req::GenerateSource{definition, {}}};
            const auto generationReply = LC_OffsetWorkerKernel::run(generationRequest);
            const auto& generated = native(generationReply, 7);
            REQUIRE(generated.engineStatus == LC_CurveOffsetStatus::Ok);
            REQUIRE(generated.ordinaryStatus == result::OrdinaryStatus::Complete);
            const auto generationWire = result::encode(generationReply, generationRequest);
            REQUIRE(generationWire);
            REQUIRE(result::decode(*generationWire, generationRequest));
            RS_SplineData cache;
            REQUIRE(LC_OffsetWorkerSourceCache::combine(generated.ordinary, cache));
            const req::Request validationRequest{token, req::ValidateSourceCache{cache, definition, {}}};
            const auto validationReply = LC_OffsetWorkerKernel::run(validationRequest);
            const auto& validated = native(validationReply, 8);
            CHECK(validated.refusal == LC_OffsetCurveRefusal::None);
            CHECK(validated.changed);
            CHECK(validated.engineStatus == LC_CurveOffsetStatus::Ok);
        }
    }
}

TEST_CASE("rational four-knot crossing maps across positive weight profiles",
          "[lc-offsetcurve][worker-kernel][spline-variant][rational][knot][weights]") {
    const std::array<std::array<double, 11>, 6> weightProfiles{{
        {{1.0, 1.02, .98, 1.03, .97, 1.01, .99, 1.04, .96, 1.02, .98}},
        {{.72, 1.38, .81, 1.27, .93, 1.46, .76, 1.19, 1.08, .84, 1.31}},
        {{.25, 2.0, .4, 2.5, .35, 2.2, .3, 2.8, 1.7, .45, 2.4}},
        {{.05, 20.0, .1, 10.0, .2, 15.0, .08, 18.0, 5.0, .12, 12.0}},
        {{1e-3, 1e3, 1e-2, 1e2, 1e-1, 10.0, 1.0, 500.0, 2e-3, 200.0, 2.0}},
        {{1e-6, 1e6, 1e-4, 1e4, 1e-2, 1e2, 1.0, 5e5, 2e-6, 2e5, 2.0}},
    }};
    const SplineOracle domainStart{0};
    const SplineOracle domainEnd{1};
    const SplineOracle derivativeStep{"1e-24"};
    const std::array<double, 4> interiorKnots{.12, .31, .58, .82};
    std::size_t oracleCrossingCases = 0;
    std::size_t extremeWeightDirectOutputs = 0;
    std::size_t extremeWeightRefusals = 0;
    std::size_t extremeWeightCertifiedCandidates = 0;
    std::size_t extremeWeightSampledCandidates = 0;
    std::size_t millionWeightCertifiedCandidates = 0;
    std::size_t millionWeightSampledCandidates = 0;
    std::size_t millionWeightRefusals = 0;
    std::array<std::size_t, 3> millionWeightCertifiedByKnot{};
    std::array<std::size_t, 3> millionWeightSampledByKnot{};
    std::array<std::size_t, 3> millionWeightRefusalsByKnot{};

    for (std::size_t profile = 0; profile < weightProfiles.size(); ++profile) {
        const auto& weightPattern = weightProfiles[profile];
        for (std::size_t targetKnotIndex = 0; targetKnotIndex < interiorKnots.size(); ++targetKnotIndex) {
            if (profile == 5 && targetKnotIndex != interiorKnots.size() - 1)
                continue;
            const double targetKnot = interiorKnots[targetKnotIndex];
            for (const int degree : {3, 4, 5, 6}) {
                const int multiplicity = degree - 2;
                const int controlCount = degree + 1 + 4 * multiplicity;
                for (const double curvatureSign : {-1.0, 1.0}) {
                    RS_SplineData data{degree, false};
                    const int curvatureControl = static_cast<int>(targetKnotIndex) * multiplicity + degree - 3;
                    for (int i = 0; i < controlCount; ++i) {
                        const double x = static_cast<double>(i);
                        const double y = curvatureSign * (i == curvatureControl ? -5.0 : .001 * x * x * x);
                        data.controlPoints.emplace_back(x, y);
                        data.weights.push_back(weightPattern[static_cast<std::size_t>(i) % weightPattern.size()]);
                    }
                    data.knotslist.assign(static_cast<std::size_t>(degree + 1), 0.0);
                    for (const double knot : interiorKnots)
                        data.knotslist.insert(data.knotslist.end(), static_cast<std::size_t>(multiplicity), knot);
                    data.knotslist.insert(data.knotslist.end(), static_cast<std::size_t>(degree + 1), 1.0);
                    const RS_Spline source{nullptr, data};
                    CAPTURE(profile, targetKnotIndex, degree, multiplicity, curvatureSign, curvatureControl, targetKnot, controlCount);
                    REQUIRE(source.validate());

                    const auto leftCurvature = oracleSplineCurvatureLimit(data, SplineOracle{targetKnot}, true);
                    const auto rightCurvature = oracleSplineCurvatureLimit(data, SplineOracle{targetKnot}, false);
                    REQUIRE(leftCurvature);
                    REQUIRE(rightCurvature);
                    REQUIRE(abs(*leftCurvature - *rightCurvature) < SplineOracle{"1e-30"});
                    const double signedDistance = (2 / (*leftCurvature + *rightCurvature)).convert_to<double>();
                    REQUIRE(std::isfinite(signedDistance));

                    const SplineOracle before = oracleOffsetFactor(data, SplineOracle{targetKnot} - SplineOracle{"1e-7"}, derivativeStep,
                                                                   domainStart, domainEnd, SplineOracle{signedDistance});
                    const SplineOracle after = oracleOffsetFactor(data, SplineOracle{targetKnot} + SplineOracle{"1e-7"}, derivativeStep,
                                                                  domainStart, domainEnd, SplineOracle{signedDistance});
                    if ((before < 0) == (after < 0))
                        continue;
                    ++oracleCrossingCases;

                    const LC_OffsetSourceDefinition definition{LC_OffsetSourceGeometry{data}, signedDistance, 1e-4, false};
                    const auto options = LC_OffsetWorkerTolerance::directOptions(definition, source);
                    REQUIRE(options);
                    LC_CurveOffsetCuspIsolationDiagnostics diagnostics;
                    const auto built = LC_CurveOffset::buildDirectBranches(
                        source,
                        LC_CurveOffset::makeSideRequest(signedDistance > 0 ? LC_CurveOffsetSide::Left : LC_CurveOffsetSide::Right,
                                                        std::abs(signedDistance)),
                        *options, LC_CurveOffset::makeDirectSourceBudget(), &diagnostics);
                    CAPTURE(built.status, diagnostics.failureStage, diagnostics.candidateParameters);
                    if (profile == 5 && built.status != LC_CurveOffsetStatus::Ok) {
                        CHECK(built.status == LC_CurveOffsetStatus::SingularOffset);
                        CHECK(diagnostics.failureStage == LC_CurveOffsetCuspFailureStage::UnresolvedRunBudget);
                        CHECK(built.branches.empty());
                        ++extremeWeightRefusals;
                        continue;
                    }
                    if (profile == 4 && targetKnotIndex != interiorKnots.size() - 1 && built.status != LC_CurveOffsetStatus::Ok) {
                        CHECK(built.status == LC_CurveOffsetStatus::ToleranceNotMet);
                        CHECK(built.branches.empty());
                        ++millionWeightRefusals;
                        ++millionWeightRefusalsByKnot[targetKnotIndex];
                        continue;
                    }
                    REQUIRE(built.status == LC_CurveOffsetStatus::Ok);
                    if (profile == 5)
                        ++extremeWeightDirectOutputs;
                    const auto candidate = std::find_if(
                        diagnostics.candidateBoxes.begin(), diagnostics.candidateBoxes.end(),
                        [targetKnot](const LC_CurveOffsetCuspCandidateBox& box) { return std::abs(box.parameter - targetKnot) < 1e-8; });
                    REQUIRE(candidate != diagnostics.candidateBoxes.end());
                    if (profile == 5) {
                        if (candidate->kind == LC_CurveOffsetCuspCandidateKind::CertifiedUniqueBracket) {
                            ++extremeWeightCertifiedCandidates;
                        }
                        else {
                            CHECK(candidate->kind == LC_CurveOffsetCuspCandidateKind::SampledSignChange);
                            ++extremeWeightSampledCandidates;
                        }
                    }
                    else if (profile == 4 && targetKnotIndex != interiorKnots.size() - 1) {
                        if (candidate->kind == LC_CurveOffsetCuspCandidateKind::CertifiedUniqueBracket) {
                            ++millionWeightCertifiedCandidates;
                            ++millionWeightCertifiedByKnot[targetKnotIndex];
                        }
                        else {
                            CHECK(candidate->kind == LC_CurveOffsetCuspCandidateKind::SampledSignChange);
                            ++millionWeightSampledCandidates;
                            ++millionWeightSampledByKnot[targetKnotIndex];
                        }
                    }
                    else {
                        CHECK(candidate->kind == LC_CurveOffsetCuspCandidateKind::CertifiedUniqueBracket);
                    }
                    const auto expectedPoint = oracleOffsetPoint(data, SplineOracle{targetKnot}, derivativeStep, domainStart, domainEnd,
                                                                 SplineOracle{signedDistance});
                    const RS_Vector expected{expectedPoint.first.convert_to<double>(), expectedPoint.second.convert_to<double>()};
                    double endpointError = std::numeric_limits<double>::infinity();
                    for (const auto& branch : built.branches) {
                        if (branch.cubicPieces.empty())
                            continue;
                        if (branch.startEnd == LC_OffsetBranchEnd::Cusp)
                            endpointError = std::min(endpointError, branch.cubicPieces.front().bezier.front().distanceTo(expected));
                        if (branch.endEnd == LC_OffsetBranchEnd::Cusp)
                            endpointError = std::min(endpointError, branch.cubicPieces.back().bezier.back().distanceTo(expected));
                    }
                    CHECK(endpointError <= options->tolerance.requestedGeometry);
                    if (profile == 5 ||
                        (profile == 4 && targetKnotIndex != interiorKnots.size() - 1 &&
                         candidate->kind != LC_CurveOffsetCuspCandidateKind::CertifiedUniqueBracket))
                        continue;

                    const req::Request generationRequest{token, req::GenerateSource{definition, {}}};
                    const auto generationReply = LC_OffsetWorkerKernel::run(generationRequest);
                    const auto& generated = native(generationReply, 7);
                    REQUIRE(generated.engineStatus == LC_CurveOffsetStatus::Ok);
                    REQUIRE(generated.ordinaryStatus == result::OrdinaryStatus::Complete);
                    const auto generationWire = result::encode(generationReply, generationRequest);
                    REQUIRE(generationWire);
                    REQUIRE(result::decode(*generationWire, generationRequest));
                    RS_SplineData cache;
                    REQUIRE(LC_OffsetWorkerSourceCache::combine(generated.ordinary, cache));
                    const req::Request validationRequest{token, req::ValidateSourceCache{cache, definition, {}}};
                    const auto validationReply = LC_OffsetWorkerKernel::run(validationRequest);
                    const auto& validated = native(validationReply, 8);
                    CHECK(validated.refusal == LC_OffsetCurveRefusal::None);
                    CHECK(validated.changed);
                    CHECK(validated.engineStatus == LC_CurveOffsetStatus::Ok);
                }
            }
        }
    }
    CAPTURE(oracleCrossingCases);
    CHECK(oracleCrossingCases == 136);
    CAPTURE(extremeWeightDirectOutputs, extremeWeightRefusals, extremeWeightCertifiedCandidates,
            extremeWeightSampledCandidates, millionWeightCertifiedCandidates, millionWeightSampledCandidates,
            millionWeightRefusals);
    CHECK(extremeWeightDirectOutputs == 4);
    CHECK(extremeWeightRefusals == 2);
    CHECK(extremeWeightCertifiedCandidates == 2);
    CHECK(extremeWeightSampledCandidates == 2);
    CAPTURE(millionWeightCertifiedByKnot, millionWeightSampledByKnot, millionWeightRefusalsByKnot);
    CHECK(millionWeightCertifiedCandidates == 14);
    CHECK(millionWeightSampledCandidates == 4);
    CHECK(millionWeightRefusals == 2);
    CHECK(millionWeightCertifiedByKnot[0] == 4);
    CHECK(millionWeightCertifiedByKnot[1] == 4);
    CHECK(millionWeightCertifiedByKnot[2] == 6);
    CHECK(millionWeightSampledByKnot[0] == 2);
    CHECK(millionWeightSampledByKnot[1] == 2);
    CHECK(millionWeightSampledByKnot[2] == 0);
    CHECK(millionWeightRefusalsByKnot[0] == 0);
    CHECK(millionWeightRefusalsByKnot[1] == 0);
    CHECK(millionWeightRefusalsByKnot[2] == 2);
}

TEST_CASE("rational spline knot sign jump is not certified as a cusp",
          "[lc-offsetcurve][worker-kernel][spline-variant][rational][knot][refusal]") {
    RS_SplineData data{6, false};
    data.controlPoints = {{0, 0}, {1, -1}, {2, 1}, {3, 2}, {4, 1}, {5, 0}, {6, 0}, {7, -2}, {8, -1}, {9, 1}, {10, 2}, {11, 0}};
    data.weights = {1.0, 1.2, .9, 1.1, .8, 1.3, 1.0, .7, 1.4, .9, 1.2, .8};
    data.knotslist = {0, 0, 0, 0, 0, 0, 0, .5, .5, .5, .5, .5, 1, 1, 1, 1, 1, 1, 1};
    const RS_Spline source{nullptr, data};
    REQUIRE(source.validate());
    LC_CurveJet leftJet;
    LC_CurveJet rightJet;
    REQUIRE(source.tryEvaluateJet(.5, LC_CurveEvaluationSide::Left, leftJet));
    REQUIRE(source.tryEvaluateJet(.5, LC_CurveEvaluationSide::Right, rightJet));
    const auto curvature = [](const LC_CurveJet& jet) {
        const double speedSquared = jet.first.x * jet.first.x + jet.first.y * jet.first.y;
        const double cross = jet.first.x * jet.second.y - jet.first.y * jet.second.x;
        return cross / (speedSquared * std::sqrt(speedSquared));
    };
    const double leftCurvature = curvature(leftJet);
    const double rightCurvature = curvature(rightJet);
    CAPTURE(leftCurvature, rightCurvature, leftJet, rightJet);
    REQUIRE(std::isfinite(leftCurvature));
    REQUIRE(std::isfinite(rightCurvature));
    REQUIRE(leftCurvature * rightCurvature < 0.0);
    const double signedDistance = leftCurvature > 0.0
                                      ? 2.0 / leftCurvature
                                      : 2.0 / rightCurvature;
    const LC_OffsetSourceDefinition definition{
        LC_OffsetSourceGeometry{data}, signedDistance, 1e-4, false};
    const auto options = LC_OffsetWorkerTolerance::directOptions(definition, source);
    REQUIRE(options);

    LC_CurveJetBounds leftBounds;
    LC_CurveJetBounds rightBounds;
    REQUIRE(source.tryBoundJetForCusp(.5 - 1e-6, .5, leftBounds, signedDistance));
    REQUIRE(source.tryBoundJetForCusp(.5, .5 + 1e-6, rightBounds, signedDistance));
    const LC_Interval leftFactor = leftBounds.offsetFactorNumerator(signedDistance);
    const LC_Interval rightFactor = rightBounds.offsetFactorNumerator(signedDistance);
    CAPTURE(leftFactor, rightFactor,
            leftBounds.rationalCuspIntervalVariationUpperBound,
            rightBounds.rationalCuspIntervalVariationUpperBound);
    REQUIRE(leftFactor.excludesZero());
    REQUIRE(rightFactor.excludesZero());
    CHECK((leftFactor.hi() < 0.0) != (rightFactor.hi() < 0.0));

    LC_CurveOffsetCuspIsolationDiagnostics diagnostics;
    const auto built = LC_CurveOffset::buildDirectBranches(
        source,
        LC_CurveOffset::makeSideRequest(
            signedDistance > 0.0 ? LC_CurveOffsetSide::Left
                                 : LC_CurveOffsetSide::Right,
            std::abs(signedDistance)),
        *options, LC_CurveOffset::makeDirectSourceBudget(), &diagnostics);
    CAPTURE(built.status, diagnostics.failureStage, diagnostics.candidateParameters,
            diagnostics.failureT0, diagnostics.failureT1);
    CHECK(std::none_of(diagnostics.candidateParameters.begin(),
                       diagnostics.candidateParameters.end(), [](const double parameter) {
                           return std::abs(parameter - .5) < 1e-8;
                       }));
    if (built.status != LC_CurveOffsetStatus::Ok) {
        CHECK(built.branches.empty());
    }
}

TEST_CASE("nonuniform periodic rational seam tangent maps to retained output",
          "[lc-offsetcurve][worker-kernel][spline-variant][oracle][periodic]["
          "rational][seam-root][nonuniform][even-root]") {
    RS_Spline base{nullptr, RS_SplineData{3, false}};
    for (const RS_Vector& point : {RS_Vector{5, -8.660254037844386}, RS_Vector{10, 0},
                                   RS_Vector{5, 8.660254037844386}, RS_Vector{-5, 8.660254037844386},
                                   RS_Vector{-10, 0}, RS_Vector{-5, -8.660254037844386}})
        base.addControlPoint(point);
    base.setKnotVector({0, 0, 0, 0, .5, 2.5, 3, 3, 3, 3});
    REQUIRE(base.validate());
    base.setClosed(true);

    RS_SplineData data = base.getData();
    data.knotslist = {-3, -2.5, -0.5, 0, 0.5, 2.5, 3,
                      3.5, 5.5, 6, 6.5, 8.5, 9};
    const std::vector<RS_Vector> controls{
        {5, -8.660254037844386}, {10, 0},
        {5, 8.660254037844386}, {-5, 8.660254037844386},
        {-10, 0}, {-5, -8.660254037844386}};
    data.controlPoints = controls;
    data.controlPoints.insert(data.controlPoints.end(), controls.begin(),
                              controls.begin() + data.degree);
    const std::vector<double> baseWeights{1.2, 1, 1.2, .8, 1.5, .8};
    data.weights = baseWeights;
    data.weights.insert(data.weights.end(), baseWeights.begin(),
                        baseWeights.begin() + data.degree);
    const RS_Spline source{nullptr, data};
    REQUIRE(source.validate());
    REQUIRE(source.isClosed());

    double startValue = 0.0;
    double endValue = 0.0;
    REQUIRE(source.getParameterDomain(startValue, endValue));
    const SplineOracle start{startValue};
    const SplineOracle end{endValue};
    const SplineOracle step{"1e-24"};
    const SplineOracle unitLeft = oracleOffsetFactor(
        data, end, step, start, end, SplineOracle{1}, true);
    const SplineOracle unitRight = oracleOffsetFactor(
        data, start, step, start, end, SplineOracle{1}, false);
    const SplineOracle curvatureLeft = 1 - unitLeft;
    const SplineOracle curvatureRight = 1 - unitRight;
    CAPTURE(curvatureLeft, curvatureRight);
    REQUIRE(abs(curvatureLeft - curvatureRight) < SplineOracle{"1e-8"});
    REQUIRE(abs(curvatureLeft) > SplineOracle{"1e-12"});
    const double signedDistance =
        (2 / (curvatureLeft + curvatureRight)).convert_to<double>();
    const SplineOracle distance{signedDistance};
    const SplineOracle seamLeft = oracleOffsetFactor(
        data, end, step, start, end, distance, true);
    const SplineOracle seamRight = oracleOffsetFactor(
        data, start, step, start, end, distance, false);
    const SplineOracle before = oracleOffsetFactor(
        data, end - SplineOracle{"1e-7"}, step, start, end, distance);
    const SplineOracle after = oracleOffsetFactor(
        data, start + SplineOracle{"1e-7"}, step, start, end, distance);
    CAPTURE(baseWeights, signedDistance, seamLeft, seamRight, before, after);
    CHECK(abs(seamLeft) < SplineOracle{"1e-8"});
    CHECK(abs(seamRight) < SplineOracle{"1e-8"});
    REQUIRE(before != 0);
    REQUIRE(after != 0);
    REQUIRE((before < 0) == (after < 0));

    const LC_OffsetSourceDefinition definition{LC_OffsetSourceGeometry{data},
                                               signedDistance, 1e-4, false};
    const auto options =
        LC_OffsetWorkerTolerance::directOptions(definition, source);
    REQUIRE(options);
    LC_CurveOffsetCuspIsolationDiagnostics diagnostics;
    const auto built = LC_CurveOffset::buildDirectBranches(
        source,
        LC_CurveOffset::makeSideRequest(signedDistance > 0
                                            ? LC_CurveOffsetSide::Left
                                            : LC_CurveOffsetSide::Right,
                                        std::abs(signedDistance)),
        *options, LC_CurveOffset::makeDirectSourceBudget(), &diagnostics);
    CAPTURE(built.status, diagnostics.status, diagnostics.candidateParameters,
            diagnostics.candidateBoxes, diagnostics.stallIntervals,
            diagnostics.failureStage, diagnostics.failureSpan,
            diagnostics.failureT0, diagnostics.failureT1);
    REQUIRE(built.status == LC_CurveOffsetStatus::Ok);
    REQUIRE(diagnostics.complete);

    const auto seamCandidate = std::find_if(
        diagnostics.candidateParameters.begin(), diagnostics.candidateParameters.end(),
        [&](double parameter) {
            return std::abs(parameter - startValue) < 1e-8 ||
                   std::abs(parameter - endValue) < 1e-8;
        });
    REQUIRE(seamCandidate == diagnostics.candidateParameters.end());
    const auto seamStartStall = std::find_if(
        diagnostics.stallIntervals.begin(), diagnostics.stallIntervals.end(),
        [&](const std::pair<double, double>& interval) {
            return interval.first <= startValue && interval.second > startValue;
        });
    const auto seamEndStall = std::find_if(
        diagnostics.stallIntervals.begin(), diagnostics.stallIntervals.end(),
        [&](const std::pair<double, double>& interval) {
            return interval.first < endValue && interval.second >= endValue;
        });
    REQUIRE(seamStartStall != diagnostics.stallIntervals.end());
    REQUIRE(seamEndStall != diagnostics.stallIntervals.end());
    REQUIRE(seamStartStall != seamEndStall);

    const auto startPoint = oracleOffsetPoint(
        data, start, step, start, end, distance);
    const auto endPoint = oracleOffsetPoint(
        data, end, step, start, end, distance, true);
    const RS_Vector expectedSeam{
        ((startPoint.first + endPoint.first) / 2).convert_to<double>(),
        ((startPoint.second + endPoint.second) / 2).convert_to<double>()};
    std::vector<RS_Vector> outputEndpoints;
    for (const LC_OffsetBranch& branch : built.branches) {
        for (const LC_OffsetCubicPiece& piece : branch.cubicPieces) {
            outputEndpoints.push_back(piece.bezier.front());
            outputEndpoints.push_back(piece.bezier.back());
        }
    }
    const double nearestSeamEndpoint = std::accumulate(
        outputEndpoints.begin(), outputEndpoints.end(),
        std::numeric_limits<double>::infinity(), [&](const double best, const RS_Vector& point) {
            return std::min(best, point.distanceTo(expectedSeam));
        });
    CAPTURE(expectedSeam.x, expectedSeam.y, nearestSeamEndpoint,
            options->tolerance.requestedGeometry);
    CHECK(nearestSeamEndpoint <= options->tolerance.requestedGeometry);

    for (const auto stall : {seamStartStall, seamEndStall}) {
        for (const double parameter : {stall->first, stall->second}) {
            const auto point = oracleOffsetPoint(
                data, SplineOracle{parameter}, step, start, end, distance,
                parameter == endValue);
            const RS_Vector expected{point.first.convert_to<double>(),
                                     point.second.convert_to<double>()};
            const double nearest = std::accumulate(
                outputEndpoints.begin(), outputEndpoints.end(),
                std::numeric_limits<double>::infinity(), [&](const double best, const RS_Vector& output) {
                    return std::min(best, output.distanceTo(expected));
                });
            CAPTURE(parameter, nearest);
            CHECK(nearest <= options->tolerance.requestedGeometry);
        }
    }
}

TEST_CASE("periodic rational tangent root is isolated as a bounded stall",
          "[lc-offsetcurve][worker-kernel][spline-variant][oracle][periodic]["
          "rational][tangent-root]") {
  struct Fixture {
    int degree;
    std::vector<RS_Vector> controls;
    std::vector<double> knots;
    std::vector<double> weights;
  };
  const std::vector<Fixture> fixtures{
      {3,
       {{0, 0}, {30, -15}, {55, 15}, {15, 35}},
       {0, 0, 0, 0, 1, 1, 1, 1},
       {1, .8, 1.2, 1.5}},
      {3,
       {{0, 0}, {30, -15}, {55, 15}, {15, 35}},
       {0, 0, 0, 0, 1, 1, 1, 1},
       {1, .7, 1.4, 1.1}},
      {3,
       {{0, 0}, {30, -15}, {60, 0}, {55, 35}, {20, 50}, {-15, 25}},
       {0, 0, 0, 0, .5, 2, 3, 3, 3, 3},
       {1, .8, 1.2, 1.5, 1.2, .8}},
      {3,
       {{0, 0}, {30, -15}, {60, 0}, {55, 35}, {20, 50}, {-15, 25}},
       {0, 0, 0, 0, .5, 2, 3, 3, 3, 3},
       {1, .7, 1.4, 1.7, 1.4, .7}},
      {2,
       {{0, 0}, {25, -10}, {55, 5}, {50, 35}, {15, 40}},
       {0, 0, 0, .33, .66, 1, 1, 1},
       {1, .8, 1.2, 1.5, 1.1}},
      {2,
       {{0, 0}, {25, -10}, {55, 5}, {60, 25}, {35, 45}, {5, 35}},
       {0, 0, 0, .5, 2, 2.5, 3, 3, 3},
       {1, .7, 1.3, 1.6, 1.2, .9}},
      {2,
       {{0, 0}, {25, -10}, {55, 5}, {60, 25}, {35, 45}, {5, 35}},
       {0, 0, 0, .5, 2, 2.5, 3, 3, 3},
       {1, .9, 1.4, 1.1, .8, 1.3}},
      {4,
       {{0, 0}, {20, -15}, {45, -5}, {55, 20}, {40, 45}, {10, 50}, {-15, 25}},
       {0, 0, 0, 0, 0, .25, .75, 1, 1, 1, 1, 1},
       {1, .8, 1.2, 1.5, 1.1, .9, 1.3}},
      {5,
       {{0, 0}, {15, -18}, {38, -12}, {55, 4}, {58, 27}, {38, 46}, {10, 52}, {-14, 28}},
       {0, 0, 0, 0, 0, 0, .25, .75, 1, 1, 1, 1, 1, 1},
       {1, .8, 1.2, 1.5, 1.1, .9, 1.3, 1.05}},
      {6,
       {{0, 0}, {12, -18}, {32, -20}, {50, -8}, {60, 10}, {55, 32}, {38, 48}, {13, 53}, {-12, 35}},
       {0, 0, 0, 0, 0, 0, 0, .25, .75, 1, 1, 1, 1, 1, 1, 1},
       {1, .8, 1.2, 1.5, 1.1, .9, 1.3, 1.05, .95}}};
  for (const Fixture &fixture : fixtures) {
    const std::vector<double> &baseWeights = fixture.weights;
    RS_Spline source{nullptr, RS_SplineData{fixture.degree, false}};
    for (const RS_Vector &point : fixture.controls)
      source.addControlPoint(point);
    source.setKnotVector(fixture.knots);
    REQUIRE(source.validate());
    source.setClosed(true);
    REQUIRE(source.isClosed());
    RS_SplineData original = source.getData();
    original.weights = baseWeights;
    original.weights.insert(original.weights.end(), baseWeights.begin(),
                            baseWeights.begin() + original.degree);
    RS_Spline rationalSource{nullptr, original};
    REQUIRE(rationalSource.validate());
    REQUIRE(rationalSource.isClosed());

    double domainStartValue = 0.0;
    double domainEndValue = 0.0;
    REQUIRE(
        rationalSource.getParameterDomain(domainStartValue, domainEndValue));
    const SplineOracle domainStart{domainStartValue};
    const SplineOracle domainEnd{domainEndValue};
    const std::vector<double> originalBreaks =
        rationalSource.getBreakParameters();
    SplineOracle bestParameter{};
    SplineOracle bestMagnitude{0};
    for (std::size_t span = 0; span + 1 < originalBreaks.size(); ++span) {
      const double aValue = originalBreaks[span];
      const double bValue = originalBreaks[span + 1];
      if (!(aValue < bValue))
        continue;
      const SplineOracle a{aValue};
      const SplineOracle width{bValue - aValue};
      const SplineOracle derivativeStep = width * SplineOracle{"1e-20"};
      const auto curvatureMagnitude = [&](const SplineOracle &t) {
        return abs(1 - oracleOffsetFactor(original, t, derivativeStep,
                                          domainStart, domainEnd,
                                          SplineOracle{1}));
      };
      constexpr int samples = 128;
      int bestSample = 0;
      SplineOracle spanBest{0};
      for (int sample = 2; sample < samples - 1; ++sample) {
        const SplineOracle t = a + width * sample / samples;
        const SplineOracle magnitude = curvatureMagnitude(t);
        if (magnitude > spanBest) {
          spanBest = magnitude;
          bestSample = sample;
        }
      }
      if (bestSample == 0 || spanBest <= bestMagnitude)
        continue;
      SplineOracle lo = a + width * (bestSample - 1) / samples;
      SplineOracle hi = a + width * (bestSample + 1) / samples;
      const SplineOracle golden{"0.61803398874989484820458683436563811772"};
      SplineOracle x1 = hi - golden * (hi - lo);
      SplineOracle x2 = lo + golden * (hi - lo);
      SplineOracle f1 = curvatureMagnitude(x1);
      SplineOracle f2 = curvatureMagnitude(x2);
      for (int iteration = 0; iteration < 120; ++iteration) {
        if (f1 < f2) {
          lo = x1;
          x1 = x2;
          f1 = f2;
          x2 = lo + golden * (hi - lo);
          f2 = curvatureMagnitude(x2);
        } else {
          hi = x2;
          x2 = x1;
          f2 = f1;
          x1 = hi - golden * (hi - lo);
          f1 = curvatureMagnitude(x1);
        }
      }
      const SplineOracle parameter = (lo + hi) / 2;
      const SplineOracle magnitude = curvatureMagnitude(parameter);
      if (magnitude > bestMagnitude) {
        bestMagnitude = magnitude;
        bestParameter = parameter;
      }
    }
    REQUIRE(bestMagnitude > 0);
    const RS_SplineData data = rationalSource.getData();
    const SplineOracle root = bestParameter;
    const SplineOracle step{"1e-24"};
    const auto seamStart = evaluateSplineOracle(data, domainStart);
    const auto seamEnd = evaluateSplineOracle(data, domainEnd);
    CHECK(sqrt((seamStart.first - seamEnd.first) *
                   (seamStart.first - seamEnd.first) +
               (seamStart.second - seamEnd.second) *
                   (seamStart.second - seamEnd.second)) <
          SplineOracle{"1e-12"});
    const SplineOracle startStep =
        oracleStencilStep(data, domainStart, step, false, true, 2);
    const SplineOracle endStep =
        oracleStencilStep(data, domainEnd, step, true, false, 2);
    const auto startOne = evaluateSplineOracle(data, domainStart + startStep);
    const auto startTwo =
        evaluateSplineOracle(data, domainStart + 2 * startStep);
    const auto endOne = evaluateSplineOracle(data, domainEnd - endStep);
    const auto endTwo = evaluateSplineOracle(data, domainEnd - 2 * endStep);
    const SplineOracle startDx =
        (-3 * seamStart.first + 4 * startOne.first - startTwo.first) /
        (2 * startStep);
    const SplineOracle startDy =
        (-3 * seamStart.second + 4 * startOne.second - startTwo.second) /
        (2 * startStep);
    const SplineOracle endDx =
        (3 * seamEnd.first - 4 * endOne.first + endTwo.first) / (2 * endStep);
    const SplineOracle endDy =
        (3 * seamEnd.second - 4 * endOne.second + endTwo.second) /
        (2 * endStep);
    CHECK(sqrt((startDx - endDx) * (startDx - endDx) +
               (startDy - endDy) * (startDy - endDy)) < SplineOracle{"1e-8"});
    const SplineOracle seamLeft = oracleOffsetFactor(
        data, domainEnd, step, domainStart, domainEnd, SplineOracle{1}, true);
    const SplineOracle seamRight =
        oracleOffsetFactor(data, domainStart, step, domainStart, domainEnd,
                           SplineOracle{1}, false);
    CAPTURE(fixture.degree, seamLeft, seamRight);
    if (fixture.degree >= 3)
      CHECK(abs(seamLeft - seamRight) < SplineOracle{"1e-8"});
    else
      CHECK(abs(seamLeft - seamRight) > SplineOracle{"1e-3"});
    const SplineOracle unitLeft = oracleOffsetFactor(
        data, root, step, domainStart, domainEnd, SplineOracle{1}, true);
    const SplineOracle unitRight = oracleOffsetFactor(
        data, root, step, domainStart, domainEnd, SplineOracle{1}, false);
    const SplineOracle curvatureLeft = 1 - unitLeft;
    const SplineOracle curvatureRight = 1 - unitRight;
    REQUIRE(curvatureLeft != 0);
    REQUIRE(curvatureRight != 0);
    CHECK(abs(curvatureLeft - curvatureRight) < SplineOracle{"1e-8"});
    const double signedDistance =
        (2 / (curvatureLeft + curvatureRight)).convert_to<double>();
    const SplineOracle distance{signedDistance};
    const SplineOracle tangentLeft = oracleOffsetFactor(
        data, root, step, domainStart, domainEnd, distance, true);
    const SplineOracle tangentRight = oracleOffsetFactor(
        data, root, step, domainStart, domainEnd, distance, false);
    const SplineOracle leftNeighbor =
        oracleOffsetFactor(data, root - SplineOracle{"1e-7"}, step, domainStart,
                           domainEnd, distance);
    const SplineOracle rightNeighbor =
        oracleOffsetFactor(data, root + SplineOracle{"1e-7"}, step, domainStart,
                           domainEnd, distance);
    const double rootValue = root.convert_to<double>();
    CAPTURE(baseWeights, rootValue, signedDistance, tangentLeft, tangentRight,
            leftNeighbor, rightNeighbor);
    CHECK(abs(tangentLeft) < SplineOracle{"1e-8"});
    CHECK(abs(tangentRight) < SplineOracle{"1e-8"});
    CHECK(leftNeighbor != 0);
    CHECK(rightNeighbor != 0);
    CHECK((leftNeighbor < 0) == (rightNeighbor < 0));
    const auto rootBreak = std::upper_bound(originalBreaks.begin(),
                                            originalBreaks.end(), rootValue);
    REQUIRE(rootBreak != originalBreaks.begin());
    REQUIRE(rootBreak != originalBreaks.end());
    const double spanStart = *(rootBreak - 1);
    const double spanEnd = *rootBreak;
    const double rootRadius =
        0.25 * std::min(rootValue - spanStart, spanEnd - rootValue);
    LC_CurveJetBounds rootBounds;
    REQUIRE(rationalSource.tryBoundJetForCusp(rootValue - rootRadius,
                                              rootValue + rootRadius,
                                              rootBounds, signedDistance));
    CAPTURE(rootBounds.rationalCuspRootVariationUpperBound,
            rootBounds.offsetFactorNumerator(signedDistance).lo(),
            rootBounds.offsetFactorNumerator(signedDistance).hi());
    CHECK(rootBounds.rationalCuspRootVariationUpperBound >= 2);
    CHECK(rootBounds.offsetFactorNumerator(signedDistance).lo() <= 0.0);
    CHECK(rootBounds.offsetFactorNumerator(signedDistance).hi() >= 0.0);
    const double rootFreeA = rootValue + 0.001 * (spanEnd - rootValue);
    const double rootFreeB = rootValue + 0.002 * (spanEnd - rootValue);
    LC_CurveJetBounds rootFreeBounds;
    REQUIRE(rationalSource.tryBoundJetForCusp(rootFreeA, rootFreeB,
                                              rootFreeBounds, signedDistance));
    CHECK(rootFreeBounds.rationalCuspNoRootCertified);

    const LC_OffsetSourceDefinition definition{LC_OffsetSourceGeometry{data},
                                               signedDistance, 1e-4, false};
    const auto options =
        LC_OffsetWorkerTolerance::directOptions(definition, rationalSource);
    REQUIRE(options);
    LC_CurveOffsetCuspIsolationDiagnostics diagnostics;
    const auto built = LC_CurveOffset::buildDirectBranches(
        rationalSource,
        LC_CurveOffset::makeSideRequest(signedDistance > 0
                                            ? LC_CurveOffsetSide::Left
                                            : LC_CurveOffsetSide::Right,
                                        std::abs(signedDistance)),
        *options, LC_CurveOffset::makeDirectSourceBudget(), &diagnostics);
    CAPTURE(built.status, diagnostics.status, diagnostics.stallIntervals,
            diagnostics.candidateBoxes, diagnostics.failureSpan,
            diagnostics.failureT0, diagnostics.failureT1,
            diagnostics.failureStage, diagnostics.isolationBoxesVisited,
            diagnostics.failingRunBoxes, diagnostics.failingRunExtent,
            diagnostics.failingRunObserved);
    CHECK(built.status == LC_CurveOffsetStatus::Ok);
    CHECK(diagnostics.complete);
    const auto owningStall = std::find_if(
        diagnostics.stallIntervals.begin(), diagnostics.stallIntervals.end(),
        [&](const std::pair<double, double> &interval) {
          return interval.first <= rootValue && rootValue <= interval.second;
        });
    REQUIRE(owningStall != diagnostics.stallIntervals.end());
    CHECK(spanStart <= owningStall->first);
    CHECK(owningStall->second <= spanEnd);
    const auto nearestPieceEndpoint = [&](const RS_Vector &point) {
      double nearest = std::numeric_limits<double>::infinity();
      for (const LC_OffsetBranch &branch : built.branches) {
        for (const LC_OffsetCubicPiece &piece : branch.cubicPieces) {
          nearest = std::min(nearest,
                             std::min(piece.bezier.front().distanceTo(point),
                                      piece.bezier.back().distanceTo(point)));
        }
      }
      return nearest;
    };
    for (const auto &stall : diagnostics.stallIntervals) {
      const auto stallStart =
          oracleOffsetPoint(data, SplineOracle{stall.first}, step, domainStart,
                            domainEnd, distance);
      const auto stallEnd =
          oracleOffsetPoint(data, SplineOracle{stall.second}, step, domainStart,
                            domainEnd, distance, true);
      const RS_Vector expectedStart{stallStart.first.convert_to<double>(),
                                    stallStart.second.convert_to<double>()};
      const RS_Vector expectedEnd{stallEnd.first.convert_to<double>(),
                                  stallEnd.second.convert_to<double>()};
      CHECK(nearestPieceEndpoint(expectedStart) <=
            options->tolerance.requestedGeometry);
      CHECK(nearestPieceEndpoint(expectedEnd) <=
            options->tolerance.requestedGeometry);
    }
    const auto oraclePoint =
        oracleOffsetPoint(data, root, step, domainStart, domainEnd, distance);
    const RS_Vector expectedPoint{oraclePoint.first.convert_to<double>(),
                                  oraclePoint.second.convert_to<double>()};
    const double nearestRootEndpoint = nearestPieceEndpoint(expectedPoint);
    CAPTURE(expectedPoint.x, expectedPoint.y, nearestRootEndpoint,
            options->tolerance.requestedGeometry,
            diagnostics.isolationBoxesVisited);
    CHECK(nearestRootEndpoint <= options->tolerance.requestedGeometry);
  }
}

TEST_CASE("rational quadratic interior tangent root obeys bounded tolerance",
          "[lc-offsetcurve][worker-kernel][spline-variant][oracle][rational][tangent-root]") {
    RS_SplineData data{2, false};
    data.type = RS_SplineData::SplineType::ClampedOpen;
    data.controlPoints = {{-2, 0}, {0, 2}, {2, 0}};
    data.knotslist = {0, 0, 0, 1, 1, 1};
    data.weights = {1, .5, 1};
    const RS_Spline source{nullptr, data};
    REQUIRE(source.validate());

    const SplineOracle domainStart{0};
    const SplineOracle domainEnd{1};
    const SplineOracle root{"0.5"};
    const SplineOracle step{"1e-24"};
    const SplineOracle unitFactor = oracleOffsetFactor(
        data, root, step, domainStart, domainEnd, SplineOracle{1});
    const SplineOracle curvature = 1 - unitFactor;
    REQUIRE(abs(curvature) > SplineOracle{"1e-12"});
    const double signedDistance = (1 / curvature).convert_to<double>();
    CHECK(signedDistance == -4.0);
    const SplineOracle distance{signedDistance};
    const SplineOracle tangent = oracleOffsetFactor(
        data, root, step, domainStart, domainEnd, distance);
    const SplineOracle leftNeighbor = oracleOffsetFactor(
        data, root - SplineOracle{"1e-7"}, step, domainStart, domainEnd, distance);
    const SplineOracle rightNeighbor = oracleOffsetFactor(
        data, root + SplineOracle{"1e-7"}, step, domainStart, domainEnd, distance);
    CAPTURE(signedDistance, tangent, leftNeighbor, rightNeighbor);
    CHECK(abs(tangent) < SplineOracle{"1e-10"});
    CHECK(leftNeighbor != 0);
    CHECK(rightNeighbor != 0);
    CHECK((leftNeighbor < 0) == (rightNeighbor < 0));

    const LC_OffsetSourceDefinition definition{
        LC_OffsetSourceGeometry{data}, signedDistance, 1e-4, false};
    const auto options = LC_OffsetWorkerTolerance::directOptions(definition, source);
    REQUIRE(options);
    LC_CurveJetBounds localBounds;
    REQUIRE(source.tryBoundJetForCusp(.499, .501, localBounds, signedDistance));
    REQUIRE(localBounds.rationalCuspRootVariationUpperBound >= 2);
    const LC_Interval ordinaryFactor =
        localBounds.speedSquared() * sqrt(localBounds.speedSquared()) -
        LC_Interval::point(signedDistance) * localBounds.cross();
    const LC_Interval rationalFactor =
        localBounds.offsetFactorNumeratorForMotionBound(signedDistance);
    CAPTURE(ordinaryFactor.lo(), ordinaryFactor.hi(),
            rationalFactor.lo(), rationalFactor.hi(),
            localBounds.rationalCuspFactorNumerator.isValid(),
            localBounds.rationalCuspFactorNumerator.lo(),
            localBounds.rationalCuspFactorNumerator.hi());
    CHECK(rationalFactor.width() < ordinaryFactor.width());
    for (const double parameter : {.499, .5, .501}) {
        const SplineOracle oracleFactor = oracleOffsetFactor(
            data, SplineOracle{parameter}, step, domainStart, domainEnd,
            SplineOracle{signedDistance}, parameter == 1.0);
        CHECK(rationalFactor.contains(oracleFactor.convert_to<double>()));
    }
    CAPTURE(localBounds.rationalCuspRootVariationUpperBound,
            rationalFactor.lo(), rationalFactor.hi());
    LC_CurveOffsetCuspIsolationDiagnostics diagnostics;
    const auto built = LC_CurveOffset::buildDirectBranches(
        source,
        LC_CurveOffset::makeSideRequest(
            signedDistance > 0 ? LC_CurveOffsetSide::Left : LC_CurveOffsetSide::Right,
            std::abs(signedDistance)),
        *options, LC_CurveOffset::makeDirectSourceBudget(), &diagnostics);
    CAPTURE(built.status, diagnostics.status, diagnostics.candidateParameters,
            diagnostics.candidateBoxes, diagnostics.stallIntervals,
            diagnostics.failureSpan, diagnostics.failureT0, diagnostics.failureT1,
            diagnostics.failureStage, diagnostics.isolationBoxesVisited,
            diagnostics.failingRunBoxes, diagnostics.failingRunExtent,
            diagnostics.failingRunObserved);
    REQUIRE(built.status == LC_CurveOffsetStatus::Ok);
    REQUIRE(diagnostics.complete);
    CHECK(diagnostics.status == built.status);
    CHECK(diagnostics.candidateParameters.empty());
    const auto strictStall = std::find_if(
        diagnostics.stallIntervals.begin(), diagnostics.stallIntervals.end(),
        [](const std::pair<double, double>& interval) {
            return interval.first <= .5 && .5 <= interval.second;
        });
    REQUIRE(strictStall != diagnostics.stallIntervals.end());
    LC_CurveJetBounds strictStallBounds;
    REQUIRE(source.tryBoundJetForCusp(strictStall->first, strictStall->second,
                                      strictStallBounds, signedDistance));
    const LC_Interval strictStallFactor =
        strictStallBounds.offsetFactorNumeratorForMotionBound(signedDistance);
    REQUIRE(strictStallFactor.isValid());
    const double strictStallMotion =
        std::max(std::abs(strictStallFactor.lo()),
                 std::abs(strictStallFactor.hi())) /
        strictStallBounds.speedSquared().lo() *
        (strictStall->second - strictStall->first);
    CAPTURE(strictStallMotion, options->tolerance.nodeMerge);
    CHECK(strictStallMotion <= options->tolerance.nodeMerge);

    const LC_OffsetSourceDefinition boundedDefinition{
        LC_OffsetSourceGeometry{data}, signedDistance, 1e-3, false};
    const auto boundedOptions =
        LC_OffsetWorkerTolerance::directOptions(boundedDefinition, source);
    REQUIRE(boundedOptions);
    LC_CurveOffsetCuspIsolationDiagnostics boundedDiagnostics;
    const auto bounded = LC_CurveOffset::buildDirectBranches(
        source,
        LC_CurveOffset::makeSideRequest(LC_CurveOffsetSide::Right,
                                        std::abs(signedDistance)),
        *boundedOptions, LC_CurveOffset::makeDirectSourceBudget(),
        &boundedDiagnostics);
    CAPTURE(bounded.status, boundedDiagnostics.status,
            boundedDiagnostics.stallIntervals,
            boundedDiagnostics.candidateParameters,
            boundedDiagnostics.failureStage);
    REQUIRE(bounded.status == LC_CurveOffsetStatus::Ok);
    REQUIRE(boundedDiagnostics.complete);
    const auto tangentStall = std::find_if(
        boundedDiagnostics.stallIntervals.begin(),
        boundedDiagnostics.stallIntervals.end(),
        [](const std::pair<double, double>& interval) {
            return interval.first <= .5 && .5 <= interval.second;
        });
    REQUIRE(tangentStall != boundedDiagnostics.stallIntervals.end());

    const auto nearestPieceEndpoint = [&](const RS_Vector& point) {
        double nearest = std::numeric_limits<double>::infinity();
        for (const LC_OffsetBranch& branch : bounded.branches) {
            for (const LC_OffsetCubicPiece& piece : branch.cubicPieces) {
                nearest = std::min(
                    nearest,
                    std::min(piece.bezier.front().distanceTo(point),
                             piece.bezier.back().distanceTo(point)));
            }
        }
        return nearest;
    };
    for (const auto& stall : boundedDiagnostics.stallIntervals) {
        for (const double parameter : {stall.first, stall.second}) {
            const auto point = oracleOffsetPoint(
                data, SplineOracle{parameter}, step, domainStart, domainEnd, distance,
                parameter == domainEnd);
            const RS_Vector expected{point.first.convert_to<double>(),
                                     point.second.convert_to<double>()};
            const double endpointError = nearestPieceEndpoint(expected);
            CAPTURE(parameter, expected.x, expected.y, endpointError,
                    boundedOptions->tolerance.requestedGeometry);
            CHECK(endpointError <= boundedOptions->tolerance.requestedGeometry);
        }
    }
    const auto tangentPoint = oracleOffsetPoint(
        data, root, step, domainStart, domainEnd, distance);
    const RS_Vector expectedTangentPoint{tangentPoint.first.convert_to<double>(),
                                         tangentPoint.second.convert_to<double>()};
    CHECK(nearestPieceEndpoint(expectedTangentPoint) <=
          boundedOptions->tolerance.requestedGeometry);

    const LC_OffsetSourceDefinition regularDefinition{
        LC_OffsetSourceGeometry{data}, -signedDistance, 1e-4, false};
    const auto regularOptions =
        LC_OffsetWorkerTolerance::directOptions(regularDefinition, source);
    REQUIRE(regularOptions);
    LC_CurveOffsetCuspIsolationDiagnostics regularDiagnostics;
    const auto regular = LC_CurveOffset::buildDirectBranches(
        source,
        LC_CurveOffset::makeSideRequest(LC_CurveOffsetSide::Left,
                                        -signedDistance),
        *regularOptions, LC_CurveOffset::makeDirectSourceBudget(),
        &regularDiagnostics);
    CAPTURE(regular.status, regularDiagnostics.status,
            regularDiagnostics.failureStage, regularDiagnostics.failureT0,
            regularDiagnostics.failureT1);
    CHECK(regular.status == LC_CurveOffsetStatus::Ok);
    CHECK(regularDiagnostics.complete);
    CHECK_FALSE(regular.branches.empty());
    CHECK(regularDiagnostics.candidateParameters.empty());
    CHECK(regularDiagnostics.stallIntervals.empty());
}

TEST_CASE("binary rational quadratic tangent stalls are bounded across scales",
          "[lc-offsetcurve][worker-kernel][spline-variant][oracle][rational]["
          "tangent-root][scale][refusal]") {
    for (const double scale : {0.5, 1.0, 2.0}) {
        RS_SplineData data{2, false};
        data.type = RS_SplineData::SplineType::ClampedOpen;
        data.controlPoints = {{-2 * scale, 0}, {0, 2 * scale},
                              {2 * scale, 0}};
        data.knotslist = {0, 0, 0, 1, 1, 1};
        data.weights = {1, 0.5, 1};
        const RS_Spline source{nullptr, data};
        REQUIRE(source.validate());

        const SplineOracle start{0};
        const SplineOracle end{1};
        const SplineOracle root{"0.5"};
        const SplineOracle step{"1e-24"};
        const double signedDistance = -4 * scale;
        const SplineOracle distance{signedDistance};
        const SplineOracle tangent = oracleOffsetFactor(
            data, root, step, start, end, distance);
        const SplineOracle before = oracleOffsetFactor(
            data, root - SplineOracle{"1e-7"}, step, start, end, distance);
        const SplineOracle after = oracleOffsetFactor(
            data, root + SplineOracle{"1e-7"}, step, start, end, distance);
        CAPTURE(scale, signedDistance, tangent, before, after);
        CHECK(abs(tangent) < SplineOracle{"1e-10"});
        CHECK(before != 0);
        CHECK(after != 0);
        CHECK((before < 0) == (after < 0));

        const double requestedGeometry = 1e-4 * scale;
        const LC_OffsetSourceDefinition definition{
            LC_OffsetSourceGeometry{data}, signedDistance,
            requestedGeometry, false};
        const auto options =
            LC_OffsetWorkerTolerance::directOptions(definition, source);
        REQUIRE(options);
        LC_CurveOffsetCuspIsolationDiagnostics diagnostics;
        const auto refused = LC_CurveOffset::buildDirectBranches(
            source,
            LC_CurveOffset::makeSideRequest(LC_CurveOffsetSide::Right,
                                            std::abs(signedDistance)),
            *options, LC_CurveOffset::makeDirectSourceBudget(), &diagnostics);
        CAPTURE(refused.status, diagnostics.failureStage,
                diagnostics.failingRunExtent, options->tolerance.nodeMerge,
                diagnostics.candidateParameters, diagnostics.stallIntervals);
        REQUIRE(refused.status == LC_CurveOffsetStatus::Ok);
        REQUIRE(diagnostics.complete);
        CHECK(diagnostics.candidateParameters.empty());
        const auto owningStall = std::find_if(
            diagnostics.stallIntervals.begin(),
            diagnostics.stallIntervals.end(),
            [](const std::pair<double, double>& interval) {
                return interval.first <= 0.5 && 0.5 <= interval.second;
            });
        REQUIRE(owningStall != diagnostics.stallIntervals.end());
        double aggregateMotion = 0.0;
        for (const auto& stall : diagnostics.stallIntervals) {
            LC_CurveJetBounds bounds;
            REQUIRE(source.tryBoundJetForCusp(stall.first, stall.second,
                                              bounds, signedDistance));
            REQUIRE(bounds.speedSquared().isPositive());
            const LC_Interval factor =
                bounds.offsetFactorNumeratorForMotionBound(signedDistance);
            REQUIRE(factor.isValid());
            for (const double parameter :
                 {stall.first, 0.5 * (stall.first + stall.second), stall.second}) {
                const SplineOracle oracleFactor = oracleOffsetFactor(
                    data, SplineOracle{parameter}, step, start, end, distance,
                    parameter == end.convert_to<double>());
                CHECK(factor.contains(oracleFactor.convert_to<double>()));
            }
            aggregateMotion +=
                std::max(std::abs(factor.lo()), std::abs(factor.hi())) /
                bounds.speedSquared().lo() * (stall.second - stall.first);
        }
        CAPTURE(aggregateMotion, options->tolerance.nodeMerge);
        CHECK(aggregateMotion <= options->tolerance.nodeMerge);
        const auto rootPoint = oracleOffsetPoint(
            data, root, step, start, end, distance);
        const RS_Vector expectedRoot{rootPoint.first.convert_to<double>(),
                                     rootPoint.second.convert_to<double>()};
        std::vector<RS_Vector> endpoints;
        for (const LC_OffsetBranch& branch : refused.branches) {
            for (const LC_OffsetCubicPiece& piece : branch.cubicPieces) {
                endpoints.push_back(piece.bezier.front());
                endpoints.push_back(piece.bezier.back());
            }
        }
        double nearestRootEndpoint = std::numeric_limits<double>::infinity();
        for (const RS_Vector& endpoint : endpoints)
            nearestRootEndpoint = std::min(
                nearestRootEndpoint, endpoint.distanceTo(expectedRoot));
        CAPTURE(nearestRootEndpoint, options->tolerance.requestedGeometry);
        CHECK(nearestRootEndpoint <= options->tolerance.requestedGeometry);
        for (const auto& stall : diagnostics.stallIntervals) {
            for (const double parameter : {stall.first, stall.second}) {
                const auto point = oracleOffsetPoint(
                    data, SplineOracle{parameter}, step, start, end, distance,
                    parameter == 1.0);
                const RS_Vector expected{point.first.convert_to<double>(),
                                         point.second.convert_to<double>()};
                double nearest = std::numeric_limits<double>::infinity();
                for (const RS_Vector& endpoint : endpoints)
                    nearest = std::min(nearest,
                                       endpoint.distanceTo(expected));
                CAPTURE(scale, parameter, nearest);
                CHECK(nearest <= options->tolerance.requestedGeometry);
            }
        }

        const LC_OffsetSourceDefinition regularDefinition{
            LC_OffsetSourceGeometry{data}, -signedDistance,
            requestedGeometry, false};
        const auto regularOptions =
            LC_OffsetWorkerTolerance::directOptions(regularDefinition, source);
        REQUIRE(regularOptions);
        LC_CurveOffsetCuspIsolationDiagnostics regularDiagnostics;
        const auto regular = LC_CurveOffset::buildDirectBranches(
            source,
            LC_CurveOffset::makeSideRequest(LC_CurveOffsetSide::Left,
                                            -signedDistance),
            *regularOptions, LC_CurveOffset::makeDirectSourceBudget(),
            &regularDiagnostics);
        CAPTURE(scale, regular.status, regularDiagnostics.failureStage);
        CHECK(regular.status == LC_CurveOffsetStatus::Ok);
        CHECK(regularDiagnostics.complete);
        CHECK_FALSE(regular.branches.empty());
        CHECK(regularDiagnostics.candidateParameters.empty());
        CHECK(regularDiagnostics.stallIntervals.empty());
    }
}

TEST_CASE("rational quadratic tangent stalls map across weight profiles and binary scales",
          "[lc-offsetcurve][worker-kernel][spline-variant][oracle][rational][tangent-root][scale]") {
    const std::vector<std::pair<double, double>> profiles{
        {0.5, 0.5}, {2.0, 0.5}, {1.0, 0.25}, {1.0, 2.0},
        {1.0, 0.125}, {1.0, 4.0}, {1.0, 0.0625}, {1.0, 8.0},
        {0.5, 0.125}, {2.0, 0.125}, {0.5, 4.0}, {2.0, 4.0},
        {0.5, 0.0625}, {2.0, 0.0625}, {0.5, 8.0}, {2.0, 8.0}};
    for (const auto& [scale, middleWeight] : profiles) {
        RS_SplineData data{2, false};
        data.type = RS_SplineData::SplineType::ClampedOpen;
        data.controlPoints = {{-2 * scale, 0}, {0, 2 * scale}, {2 * scale, 0}};
        data.knotslist = {0, 0, 0, 1, 1, 1};
        data.weights = {1, middleWeight, 1};
        const RS_Spline source{nullptr, data};
        REQUIRE(source.validate());

        const SplineOracle domainStart{0};
        const SplineOracle domainEnd{1};
        const SplineOracle root{"0.5"};
        const SplineOracle step{"1e-24"};
        const double signedDistance = -2.0 * scale / middleWeight;
        const SplineOracle distance{signedDistance};
        const SplineOracle tangent = oracleOffsetFactor(
            data, root, step, domainStart, domainEnd, distance);
        const SplineOracle leftNeighbor = oracleOffsetFactor(
            data, root - SplineOracle{"1e-7"}, step, domainStart, domainEnd,
            distance);
        const SplineOracle rightNeighbor = oracleOffsetFactor(
            data, root + SplineOracle{"1e-7"}, step, domainStart, domainEnd,
            distance);
        CAPTURE(scale, middleWeight, signedDistance, tangent, leftNeighbor,
                rightNeighbor);
        CHECK(abs(tangent) < SplineOracle{"1e-10"});
        CHECK(leftNeighbor != 0);
        CHECK(rightNeighbor != 0);
        CHECK((leftNeighbor < 0) == (rightNeighbor < 0));

        LC_CurveJetBounds rootBoundaryBounds;
        REQUIRE(source.tryBoundJetForCusp(.5, .5078125,
                                          rootBoundaryBounds, signedDistance));
        const LC_Interval rootBoundaryFactor =
            rootBoundaryBounds.offsetFactorNumeratorForMotionBound(signedDistance);
        CAPTURE(scale, middleWeight, rootBoundaryFactor.lo(),
                rootBoundaryFactor.hi());
        CHECK(rootBoundaryFactor.contains(tangent.convert_to<double>()));

        const double requestedGeometry = 1e-4 * scale;
        const LC_OffsetSourceDefinition definition{
            LC_OffsetSourceGeometry{data}, signedDistance,
            requestedGeometry, false};
        const auto options = LC_OffsetWorkerTolerance::directOptions(definition, source);
        REQUIRE(options);
        LC_CurveOffsetCuspIsolationDiagnostics diagnostics;
        const auto built = LC_CurveOffset::buildDirectBranches(
            source,
            LC_CurveOffset::makeSideRequest(LC_CurveOffsetSide::Right,
                                            std::abs(signedDistance)),
            *options, LC_CurveOffset::makeDirectSourceBudget(), &diagnostics);
        CAPTURE(built.status, diagnostics.status, diagnostics.stallIntervals,
                diagnostics.failureStage, diagnostics.failureT0,
                diagnostics.failureT1, diagnostics.failingRunBoxes,
                diagnostics.failingRunExtent, options->tolerance.nodeMerge);
        REQUIRE(built.status == LC_CurveOffsetStatus::Ok);
        REQUIRE(diagnostics.complete);

        const auto nearestPieceDistance = [](const LC_CurveOffsetGeometryResult& geometry,
                                             const RS_Vector& point) {
            double nearest = std::numeric_limits<double>::infinity();
            for (const LC_OffsetBranch& branch : geometry.branches) {
                for (const LC_OffsetCubicPiece& piece : branch.cubicPieces) {
                    constexpr int sampleCount = 128;
                    int bestSample = 0;
                    double pieceNearest = std::numeric_limits<double>::infinity();
                    const auto distanceAt = [&](double t) {
                        const double u = 1.0 - t;
                        const RS_Vector sample = piece.bezier[0] * (u * u * u) +
                            piece.bezier[1] * (3.0 * u * u * t) +
                            piece.bezier[2] * (3.0 * u * t * t) +
                            piece.bezier[3] * (t * t * t);
                        return sample.distanceTo(point);
                    };
                    for (int i = 0; i <= sampleCount; ++i) {
                        const double t = double(i) / sampleCount;
                        const double error = distanceAt(t);
                        if (error < pieceNearest) {
                            pieceNearest = error;
                            bestSample = i;
                        }
                    }
                    double left = std::max(0.0, double(bestSample - 1) / sampleCount);
                    double right = std::min(1.0, double(bestSample + 1) / sampleCount);
                    for (int i = 0; i < 40; ++i) {
                        const double t1 = left + (right - left) / 3.0;
                        const double t2 = right - (right - left) / 3.0;
                        if (distanceAt(t1) < distanceAt(t2)) right = t2;
                        else left = t1;
                    }
                    nearest = std::min(nearest, distanceAt((left + right) / 2.0));
                }
            }
            return nearest;
        };
        for (const auto& stall : diagnostics.stallIntervals) {
            for (const double parameter : {stall.first, stall.second}) {
                const auto point = oracleOffsetPoint(
                    data, SplineOracle{parameter}, step, domainStart, domainEnd,
                    distance, parameter == domainEnd);
                const RS_Vector expected{point.first.convert_to<double>(),
                                         point.second.convert_to<double>()};
                CHECK(nearestPieceDistance(built, expected) <= requestedGeometry);
            }
        }
        const auto tangentPoint = oracleOffsetPoint(
            data, root, step, domainStart, domainEnd, distance);
        const RS_Vector expectedTangentPoint{
            tangentPoint.first.convert_to<double>(),
            tangentPoint.second.convert_to<double>()};
        CHECK(nearestPieceDistance(built, expectedTangentPoint) <= requestedGeometry);

        const LC_OffsetSourceDefinition regularDefinition{
            LC_OffsetSourceGeometry{data}, -signedDistance,
            requestedGeometry, false};
        const auto regularOptions =
            LC_OffsetWorkerTolerance::directOptions(regularDefinition, source);
        REQUIRE(regularOptions);
        LC_CurveOffsetCuspIsolationDiagnostics regularDiagnostics;
        const auto regular = LC_CurveOffset::buildDirectBranches(
            source,
            LC_CurveOffset::makeSideRequest(LC_CurveOffsetSide::Left,
                                            std::abs(signedDistance)),
            *regularOptions, LC_CurveOffset::makeDirectSourceBudget(),
            &regularDiagnostics);
        CAPTURE(scale, middleWeight, regular.status,
                regularDiagnostics.failureStage);
        REQUIRE(regular.status == LC_CurveOffsetStatus::Ok);
        REQUIRE(regularDiagnostics.complete);
        CHECK(regularDiagnostics.candidateParameters.empty());
        CHECK(regularDiagnostics.stallIntervals.empty());
        for (const double parameter : {0.0, 0.25, 0.5, 0.75, 1.0}) {
            const auto point = oracleOffsetPoint(
                data, SplineOracle{parameter}, step, domainStart, domainEnd,
                SplineOracle{-signedDistance}, parameter == domainEnd);
            const RS_Vector expected{point.first.convert_to<double>(),
                                     point.second.convert_to<double>()};
            CAPTURE(scale, middleWeight, parameter);
            CHECK(nearestPieceDistance(regular, expected) <= requestedGeometry);
        }
    }
}

TEST_CASE("knot-inserted rational quadratic tangent maps across nonuniform spans",
          "[lc-offsetcurve][worker-kernel][spline-variant][oracle][rational][tangent-"
          "root][knot]") {
    const std::vector<std::pair<double, double>> profiles{{0.5, 0.5}, {1.0, 0.5}, {2.0, 0.5},
                                                          {0.5, 2.0}, {1.0, 2.0}, {2.0, 2.0}};
    const SplineOracle domainStart{0};
    const SplineOracle domainEnd{1};
    const SplineOracle root{"0.5"};
    const SplineOracle step{"1e-24"};

    for (const double insertionParameter : {0.25, 0.5}) {
        for (const auto &[scale, middleWeight] : profiles) {
            RS_SplineData original{2, false};
            original.type = RS_SplineData::SplineType::ClampedOpen;
            original.controlPoints = {{-2 * scale, 0}, {0, 2 * scale}, {2 * scale, 0}};
            original.knotslist = {0, 0, 0, 1, 1, 1};
            original.weights = {1, middleWeight, 1};

            RS_SplineData inserted{2, false};
            inserted.type = RS_SplineData::SplineType::ClampedOpen;
            const double firstWeight = 1.0 - insertionParameter + insertionParameter * middleWeight;
            const double secondWeight = (1.0 - insertionParameter) * middleWeight + insertionParameter;
            inserted.controlPoints = {{-2 * scale, 0},
                                      {-2 * scale * (1.0 - insertionParameter) / firstWeight,
                                       2 * scale * middleWeight * insertionParameter / firstWeight},
                                      {2 * scale * insertionParameter / secondWeight,
                                       2 * scale * middleWeight * (1.0 - insertionParameter) / secondWeight},
                                      {2 * scale, 0}};
            inserted.knotslist = {0, 0, 0, insertionParameter, 1, 1, 1};
            inserted.weights = {1, firstWeight, secondWeight, 1};
            const RS_Spline source{nullptr, inserted};
            REQUIRE(source.validate());

            for (const double parameter : {0.1, insertionParameter, 0.49, 0.5, 0.51, 0.75, 0.9}) {
                const auto originalPoint = evaluateSplineOracle(original, SplineOracle{parameter});
                const auto insertedPoint = evaluateSplineOracle(inserted, SplineOracle{parameter});
                const SplineOracle dx = originalPoint.first - insertedPoint.first;
                const SplineOracle dy = originalPoint.second - insertedPoint.second;
                CHECK(sqrt(dx * dx + dy * dy) < SplineOracle{"1e-12"});
            }

            const double signedDistance = -2.0 * scale / middleWeight;
            const SplineOracle distance{signedDistance};
            const SplineOracle tangentLeft =
                oracleOffsetFactor(inserted, root, step, domainStart, domainEnd, distance, true);
            const SplineOracle tangentRight =
                oracleOffsetFactor(inserted, root, step, domainStart, domainEnd, distance, false);
            const SplineOracle before =
                oracleOffsetFactor(inserted, root - SplineOracle{"1e-7"}, step, domainStart, domainEnd, distance);
            const SplineOracle after =
                oracleOffsetFactor(inserted, root + SplineOracle{"1e-7"}, step, domainStart, domainEnd, distance);
            CAPTURE(insertionParameter, scale, middleWeight, signedDistance, tangentLeft, tangentRight, before, after);
            CHECK(abs(tangentLeft) < SplineOracle{"1e-10"});
            CHECK(abs(tangentRight) < SplineOracle{"1e-10"});
            CHECK(before != 0);
            CHECK(after != 0);
            CHECK((before < 0) == (after < 0));

            const double requestedGeometry = 1e-4 * scale;
            const LC_OffsetSourceDefinition definition{LC_OffsetSourceGeometry{inserted}, signedDistance,
                                                       requestedGeometry, false};
            const auto options = LC_OffsetWorkerTolerance::directOptions(definition, source);
            REQUIRE(options);
            LC_CurveOffsetCuspIsolationDiagnostics diagnostics;
            const auto built = LC_CurveOffset::buildDirectBranches(
                source, LC_CurveOffset::makeSideRequest(LC_CurveOffsetSide::Right, std::abs(signedDistance)), *options,
                LC_CurveOffset::makeDirectSourceBudget(), &diagnostics);
            CAPTURE(insertionParameter, scale, middleWeight, built.status, diagnostics.failureStage,
                    diagnostics.failureT0, diagnostics.failureT1, diagnostics.stallIntervals);
            REQUIRE(built.status == LC_CurveOffsetStatus::Ok);
            REQUIRE(diagnostics.complete);
            std::vector<std::pair<double, double>> rootStalls;
            std::copy_if(diagnostics.stallIntervals.begin(), diagnostics.stallIntervals.end(),
                         std::back_inserter(rootStalls), [](const std::pair<double, double> &interval) {
                             return interval.first <= 0.5 && 0.5 <= interval.second;
                         });
            REQUIRE_FALSE(rootStalls.empty());

            std::vector<RS_Vector> endpoints;
            for (const LC_OffsetBranch &branch : built.branches) {
                for (const LC_OffsetCubicPiece &piece : branch.cubicPieces) {
                    endpoints.push_back(piece.bezier.front());
                    endpoints.push_back(piece.bezier.back());
                }
            }
            std::vector<double> mappedParameters{0.5};
            for (const auto &stall : rootStalls) {
                mappedParameters.push_back(stall.first);
                mappedParameters.push_back(stall.second);
            }
            for (const double parameter : mappedParameters) {
                const auto point =
                    oracleOffsetPoint(inserted, SplineOracle{parameter}, step, domainStart, domainEnd, distance);
                const RS_Vector expected{point.first.convert_to<double>(), point.second.convert_to<double>()};
                double nearest = std::numeric_limits<double>::infinity();
                for (const RS_Vector &endpoint : endpoints)
                    nearest = std::min(nearest, endpoint.distanceTo(expected));
                CAPTURE(insertionParameter, scale, middleWeight, parameter, nearest);
                CHECK(nearest <= requestedGeometry);
            }
        }
    }
}

TEST_CASE("degree-elevated rational cubic tangent maps to retained output",
          "[lc-offsetcurve][worker-kernel][spline-variant][oracle][rational]["
          "tangent-root][degree]") {
    RS_SplineData data{3, false};
    data.type = RS_SplineData::SplineType::ClampedOpen;
    data.controlPoints = {{-2, 0}, {-1, 1}, {1, 1}, {2, 0}};
    data.knotslist = {0, 0, 0, 0, 1, 1, 1, 1};
    data.weights = {1, 2.0 / 3.0, 2.0 / 3.0, 1};
    const RS_Spline source{nullptr, data};
    REQUIRE(source.validate());

    const SplineOracle domainStart{0};
    const SplineOracle domainEnd{1};
    const SplineOracle root{"0.5"};
    const SplineOracle step{"1e-24"};
    const SplineOracle unitFactor = oracleOffsetFactor(data, root, step, domainStart, domainEnd, SplineOracle{1});
    const SplineOracle curvature = 1 - unitFactor;
    REQUIRE(abs(curvature) > SplineOracle{"1e-12"});
    const double signedDistance = (1 / curvature).convert_to<double>();
    const SplineOracle distance{signedDistance};
    const SplineOracle tangent = oracleOffsetFactor(data, root, step, domainStart, domainEnd, distance);
    const SplineOracle leftNeighbor =
        oracleOffsetFactor(data, root - SplineOracle{"1e-7"}, step, domainStart, domainEnd, distance);
    const SplineOracle rightNeighbor =
        oracleOffsetFactor(data, root + SplineOracle{"1e-7"}, step, domainStart, domainEnd, distance);
    CAPTURE(signedDistance, tangent, leftNeighbor, rightNeighbor);
    CHECK(abs(tangent) < SplineOracle{"1e-10"});
    CHECK(leftNeighbor != 0);
    CHECK(rightNeighbor != 0);
    CHECK((leftNeighbor < 0) == (rightNeighbor < 0));

    const LC_OffsetSourceDefinition strictDefinition{LC_OffsetSourceGeometry{data}, signedDistance, 1e-3, false};
    const auto strictOptions = LC_OffsetWorkerTolerance::directOptions(strictDefinition, source);
    REQUIRE(strictOptions);
    LC_CurveOffsetCuspIsolationDiagnostics strictDiagnostics;
    const auto strict = LC_CurveOffset::buildDirectBranches(
        source,
        LC_CurveOffset::makeSideRequest(signedDistance > 0 ? LC_CurveOffsetSide::Left : LC_CurveOffsetSide::Right,
                                        std::abs(signedDistance)),
        *strictOptions, LC_CurveOffset::makeDirectSourceBudget(), &strictDiagnostics);
    CAPTURE(strict.status, strictDiagnostics.failureStage, strictDiagnostics.failingRunBoxes,
            strictDiagnostics.failingRunExtent, strictOptions->tolerance.nodeMerge);
    REQUIRE(strict.status == LC_CurveOffsetStatus::Ok);
    REQUIRE(strictDiagnostics.complete);
    const auto strictTangentStall = std::find_if(
        strictDiagnostics.stallIntervals.begin(), strictDiagnostics.stallIntervals.end(),
        [](const std::pair<double, double> &interval) { return interval.first <= .5 && .5 <= interval.second; });
    REQUIRE(strictTangentStall != strictDiagnostics.stallIntervals.end());
    LC_CurveJetBounds strictTangentBounds;
    REQUIRE(source.tryBoundJetForCusp(strictTangentStall->first, strictTangentStall->second, strictTangentBounds,
                                      signedDistance));
    const LC_Interval strictTangentFactor = strictTangentBounds.offsetFactorNumeratorForMotionBound(signedDistance);
    REQUIRE(strictTangentFactor.isValid());
    const double strictTangentMotion =
        std::max(std::abs(strictTangentFactor.lo()), std::abs(strictTangentFactor.hi())) /
        strictTangentBounds.speedSquared().lo() * (strictTangentStall->second - strictTangentStall->first);
    CAPTURE(strictTangentMotion, strictOptions->tolerance.nodeMerge);
    CHECK(strictTangentMotion <= strictOptions->tolerance.nodeMerge);
    const auto nearestStrictEndpoint = [&](const RS_Vector &point) {
        double nearest = std::numeric_limits<double>::infinity();
        for (const LC_OffsetBranch &branch : strict.branches) {
            for (const LC_OffsetCubicPiece &piece : branch.cubicPieces) {
                nearest = std::min(
                    nearest, std::min(piece.bezier.front().distanceTo(point), piece.bezier.back().distanceTo(point)));
            }
        }
        return nearest;
    };
    for (const double parameter : {strictTangentStall->first, strictTangentStall->second}) {
        const auto point = oracleOffsetPoint(data, SplineOracle{parameter}, step, domainStart, domainEnd, distance,
                                             parameter == domainEnd);
        const RS_Vector expected{point.first.convert_to<double>(), point.second.convert_to<double>()};
        CHECK(nearestStrictEndpoint(expected) <= strictOptions->tolerance.requestedGeometry);
    }
    const auto strictTangentPoint = oracleOffsetPoint(data, root, step, domainStart, domainEnd, distance);
    const RS_Vector expectedStrictTangent{strictTangentPoint.first.convert_to<double>(),
                                          strictTangentPoint.second.convert_to<double>()};
    CHECK(nearestStrictEndpoint(expectedStrictTangent) <= strictOptions->tolerance.requestedGeometry);

    const LC_OffsetSourceDefinition definition{LC_OffsetSourceGeometry{data}, signedDistance, 1e-2, false};
    const auto options = LC_OffsetWorkerTolerance::directOptions(definition, source);
    REQUIRE(options);
    LC_CurveOffsetCuspIsolationDiagnostics diagnostics;
    const auto built = LC_CurveOffset::buildDirectBranches(
        source,
        LC_CurveOffset::makeSideRequest(signedDistance > 0 ? LC_CurveOffsetSide::Left : LC_CurveOffsetSide::Right,
                                        std::abs(signedDistance)),
        *options, LC_CurveOffset::makeDirectSourceBudget(), &diagnostics);
    CAPTURE(built.status, diagnostics.status, diagnostics.stallIntervals, diagnostics.candidateParameters,
            diagnostics.failureStage, diagnostics.failingRunBoxes, diagnostics.failingRunExtent,
            diagnostics.failingRunSignChanges, diagnostics.failingRunObserved, diagnostics.isolationBoxesVisited);
    REQUIRE(built.status == LC_CurveOffsetStatus::Ok);
    REQUIRE(diagnostics.complete);
    const auto owningStall = std::find_if(
        diagnostics.stallIntervals.begin(), diagnostics.stallIntervals.end(),
        [](const std::pair<double, double> &interval) { return interval.first <= .5 && .5 <= interval.second; });
    REQUIRE(owningStall != diagnostics.stallIntervals.end());

    const auto nearestEndpoint = [&](const RS_Vector &point) {
        double nearest = std::numeric_limits<double>::infinity();
        for (const LC_OffsetBranch &branch : built.branches) {
            for (const LC_OffsetCubicPiece &piece : branch.cubicPieces) {
                nearest = std::min(
                    nearest, std::min(piece.bezier.front().distanceTo(point), piece.bezier.back().distanceTo(point)));
            }
        }
        return nearest;
    };
    for (const double parameter : {owningStall->first, owningStall->second}) {
        const auto oraclePoint = oracleOffsetPoint(data, SplineOracle{parameter}, step, domainStart, domainEnd,
                                                   distance, parameter == domainEnd);
        const RS_Vector expected{oraclePoint.first.convert_to<double>(), oraclePoint.second.convert_to<double>()};
        CHECK(nearestEndpoint(expected) <= options->tolerance.requestedGeometry);
    }
    const auto tangentPoint = oracleOffsetPoint(data, root, step, domainStart, domainEnd, distance);
    const RS_Vector expectedTangent{tangentPoint.first.convert_to<double>(), tangentPoint.second.convert_to<double>()};
    CHECK(nearestEndpoint(expectedTangent) <= options->tolerance.requestedGeometry);
}

TEST_CASE("exact binary rational cubic tangent has bounded output mapping",
          "[lc-offsetcurve][worker-kernel][spline-variant][oracle][rational][tangent-root][degree][binary]") {
    RS_SplineData data{3, false};
    data.type = RS_SplineData::SplineType::ClampedOpen;
    data.controlPoints = {{-6, 0}, {-4, 2}, {4, 2}, {6, 0}};
    data.knotslist = {0, 0, 0, 0, 1, 1, 1, 1};
    data.weights = {1, .5, .5, 1};
    const RS_Spline source{nullptr, data};
    REQUIRE(source.validate());

    const SplineOracle start{0};
    const SplineOracle end{1};
    const SplineOracle root{"0.5"};
    const SplineOracle step{"1e-24"};
    const SplineOracle unitFactor = oracleOffsetFactor(
        data, root, step, start, end, SplineOracle{1});
    const SplineOracle curvature = 1 - unitFactor;
    REQUIRE(abs(curvature) > SplineOracle{"1e-12"});
    const double signedDistance = (1 / curvature).convert_to<double>();
    const SplineOracle distance{signedDistance};
    const SplineOracle tangent = oracleOffsetFactor(
        data, root, step, start, end, distance);
    const SplineOracle left = oracleOffsetFactor(
        data, root - SplineOracle{"1e-7"}, step, start, end, distance);
    const SplineOracle right = oracleOffsetFactor(
        data, root + SplineOracle{"1e-7"}, step, start, end, distance);
    CAPTURE(signedDistance, tangent, left, right);
    CHECK(abs(tangent) < SplineOracle{"1e-10"});
    CHECK(left != 0);
    CHECK(right != 0);
    CHECK((left < 0) == (right < 0));

    const LC_OffsetSourceDefinition definition{
        LC_OffsetSourceGeometry{data}, signedDistance, 1e-3, false};
    const auto options = LC_OffsetWorkerTolerance::directOptions(definition, source);
    REQUIRE(options);
    LC_CurveOffsetCuspIsolationDiagnostics diagnostics;
    const auto built = LC_CurveOffset::buildDirectBranches(
        source,
        LC_CurveOffset::makeSideRequest(signedDistance > 0
                                            ? LC_CurveOffsetSide::Left
                                            : LC_CurveOffsetSide::Right,
                                        std::abs(signedDistance)),
        *options, LC_CurveOffset::makeDirectSourceBudget(), &diagnostics);
    CAPTURE(built.status, diagnostics.failureStage, diagnostics.stallIntervals,
            diagnostics.failingRunExtent, options->tolerance.nodeMerge);
    REQUIRE(built.status == LC_CurveOffsetStatus::Ok);
    REQUIRE(diagnostics.complete);
    const auto oraclePoint = oracleOffsetPoint(data, root, step, start, end, distance);
    const RS_Vector expected{oraclePoint.first.convert_to<double>(),
                             oraclePoint.second.convert_to<double>()};
    double nearest = std::numeric_limits<double>::infinity();
    for (const LC_OffsetBranch& branch : built.branches) {
        for (const LC_OffsetCubicPiece& piece : branch.cubicPieces) {
            const auto& p = piece.bezier;
            for (int i = 0; i <= 512; ++i) {
                const double t = i / 512.0;
                const double u = 1.0 - t;
                const RS_Vector sample = p[0] * (u * u * u) +
                    p[1] * (3.0 * u * u * t) + p[2] * (3.0 * u * t * t) +
                    p[3] * (t * t * t);
                nearest = std::min(nearest, sample.distanceTo(expected));
            }
        }
    }
    CAPTURE(expected.x, expected.y, nearest,
            options->tolerance.requestedGeometry);
    CHECK(nearest <= options->tolerance.requestedGeometry);
}

TEST_CASE("rational cubic tangent at a nonuniform knot has bounded ownership",
          "[lc-offsetcurve][worker-kernel][spline-variant][oracle][rational][tangent-root][knot]") {
    RS_SplineData data{3, false};
    data.type = RS_SplineData::SplineType::ClampedOpen;
    data.controlPoints = {{-2, 0}, {-1.6, .4}, {0, 1}, {1.6, .4}, {2, 0}};
    data.knotslist = {0, 0, 0, 0, .5, 1, 1, 1, 1};
    data.weights = {1, 5.0 / 6.0, 2.0 / 3.0, 5.0 / 6.0, 1};
    const RS_Spline source{nullptr, data};
    REQUIRE(source.validate());

    RS_SplineData bezierData{3, false};
    bezierData.type = RS_SplineData::SplineType::ClampedOpen;
    bezierData.controlPoints = {{-2, 0}, {-1, 1}, {1, 1}, {2, 0}};
    bezierData.knotslist = {0, 0, 0, 0, 1, 1, 1, 1};
    bezierData.weights = {1, 2.0 / 3.0, 2.0 / 3.0, 1};
    for (const double parameter : {.1, .3, .5, .7, .9}) {
        const auto inserted = evaluateSplineOracle(data, SplineOracle{parameter});
        const auto original = evaluateSplineOracle(bezierData, SplineOracle{parameter});
        CAPTURE(parameter,
                (inserted.first - original.first).convert_to<double>(),
                (inserted.second - original.second).convert_to<double>());
        CHECK(sqrt((inserted.first - original.first) * (inserted.first - original.first) +
                   (inserted.second - original.second) * (inserted.second - original.second)) <
              SplineOracle{"1e-12"});
    }

    const SplineOracle domainStart{0};
    const SplineOracle domainEnd{1};
    const SplineOracle root{"0.5"};
    const SplineOracle step{"1e-24"};
    const SplineOracle unitLeft = oracleOffsetFactor(
        data, root, step, domainStart, domainEnd, SplineOracle{1}, true);
    const SplineOracle unitRight = oracleOffsetFactor(
        data, root, step, domainStart, domainEnd, SplineOracle{1}, false);
    const SplineOracle curvatureLeft = 1 - unitLeft;
    const SplineOracle curvatureRight = 1 - unitRight;
    REQUIRE(abs(curvatureLeft - curvatureRight) < SplineOracle{"1e-8"});
    const double signedDistance =
        (2 / (curvatureLeft + curvatureRight)).convert_to<double>();
    const SplineOracle distance{signedDistance};
    const SplineOracle tangentLeft = oracleOffsetFactor(
        data, root, step, domainStart, domainEnd, distance, true);
    const SplineOracle tangentRight = oracleOffsetFactor(
        data, root, step, domainStart, domainEnd, distance, false);
    const SplineOracle leftNeighbor = oracleOffsetFactor(
        data, root - SplineOracle{"1e-7"}, step, domainStart, domainEnd, distance);
    const SplineOracle rightNeighbor = oracleOffsetFactor(
        data, root + SplineOracle{"1e-7"}, step, domainStart, domainEnd, distance);
    CAPTURE(signedDistance, tangentLeft, tangentRight, leftNeighbor, rightNeighbor);
    CHECK(abs(tangentLeft) < SplineOracle{"1e-8"});
    CHECK(abs(tangentRight) < SplineOracle{"1e-8"});
    CHECK((leftNeighbor < 0) == (rightNeighbor < 0));

    const LC_OffsetSourceDefinition definition{
        LC_OffsetSourceGeometry{data}, signedDistance, 1e-4, false};
    const auto options = LC_OffsetWorkerTolerance::directOptions(definition, source);
    REQUIRE(options);
    LC_CurveOffsetCuspIsolationDiagnostics diagnostics;
    const auto built = LC_CurveOffset::buildDirectBranches(
        source,
        LC_CurveOffset::makeSideRequest(signedDistance > 0
                                            ? LC_CurveOffsetSide::Left
                                            : LC_CurveOffsetSide::Right,
                                        std::abs(signedDistance)),
        *options, LC_CurveOffset::makeDirectSourceBudget(), &diagnostics);
    CAPTURE(built.status, diagnostics.failureStage, diagnostics.stallIntervals,
            diagnostics.candidateBoxes, diagnostics.failingRunExtent,
            options->tolerance.nodeMerge);
    REQUIRE(built.status == LC_CurveOffsetStatus::Ok);
    REQUIRE(diagnostics.complete);
    const auto owningStall = std::find_if(
        diagnostics.stallIntervals.begin(), diagnostics.stallIntervals.end(),
        [](const std::pair<double, double>& interval) {
            return interval.first <= .5 && .5 <= interval.second;
        });
    REQUIRE(owningStall != diagnostics.stallIntervals.end());
    CHECK(owningStall->first <= .5);
    CHECK(.5 <= owningStall->second);

    double stallMotion = 0.0;
    const auto checkKnotOwnedSpan = [&](const double lo, const double hi,
                                        const bool knotIsLeftEndpoint) {
        LC_CurveJetBounds stallBounds;
        REQUIRE(source.tryBoundJetForCusp(lo, hi, stallBounds, signedDistance));
        const LC_Interval stallFactor =
            stallBounds.offsetFactorNumeratorForMotionBound(signedDistance);
        REQUIRE(stallFactor.isValid());
        REQUIRE(stallBounds.speedSquared().lo() > 0.0);
        stallMotion +=
            std::max(std::abs(stallFactor.lo()), std::abs(stallFactor.hi())) /
            stallBounds.speedSquared().lo() * (hi - lo);
        for (const double parameter : {lo, hi}) {
            if (parameter == .5) {
                const auto value = oracleOffsetFactor(
                    data, SplineOracle{parameter}, step, domainStart, domainEnd,
                    distance, !knotIsLeftEndpoint);
                CHECK(stallFactor.contains(value.convert_to<double>()));
            } else {
                const auto value = oracleOffsetFactor(
                    data, SplineOracle{parameter}, step, domainStart, domainEnd,
                    distance);
                CHECK(stallFactor.contains(value.convert_to<double>()));
            }
        }
    };
    checkKnotOwnedSpan(owningStall->first, .5, false);
    checkKnotOwnedSpan(.5, owningStall->second, true);
    CAPTURE(stallMotion, options->tolerance.nodeMerge);
    CHECK(stallMotion <= options->tolerance.nodeMerge);

    const auto offsetPoint = oracleOffsetPoint(
        data, root, step, domainStart, domainEnd, distance);
    const RS_Vector expected{offsetPoint.first.convert_to<double>(),
                             offsetPoint.second.convert_to<double>()};
    double nearestEndpoint = std::numeric_limits<double>::infinity();
    for (const LC_OffsetBranch& branch : built.branches) {
        for (const LC_OffsetCubicPiece& piece : branch.cubicPieces) {
            nearestEndpoint = std::min(
                nearestEndpoint,
                std::min(piece.bezier.front().distanceTo(expected),
                         piece.bezier.back().distanceTo(expected)));
        }
    }
    CAPTURE(expected.x, expected.y, nearestEndpoint,
            options->tolerance.requestedGeometry);
    CHECK(nearestEndpoint <= options->tolerance.requestedGeometry);
}

TEST_CASE("nonuniform rational knot tangent bounds across binary scales",
          "[lc-offsetcurve][worker-kernel][spline-variant][oracle][rational][tangent-root][knot][scale]") {
    for (const double scale : {.5, 2.0}) {
        RS_SplineData data{3, false};
        data.type = RS_SplineData::SplineType::ClampedOpen;
        data.controlPoints = {{-2 * scale, 0}, {-1.6 * scale, .4 * scale},
                              {0, scale}, {1.6 * scale, .4 * scale},
                              {2 * scale, 0}};
        data.knotslist = {0, 0, 0, 0, .5, 1, 1, 1, 1};
        data.weights = {1, 5.0 / 6.0, 2.0 / 3.0, 5.0 / 6.0, 1};
        const RS_Spline source{nullptr, data};
        REQUIRE(source.validate());

        const SplineOracle domainStart{0};
        const SplineOracle domainEnd{1};
        const SplineOracle root{"0.5"};
        const SplineOracle step{"1e-24"};
        const auto unitLeft = oracleOffsetFactor(
            data, root, step, domainStart, domainEnd, SplineOracle{1}, true);
        const auto unitRight = oracleOffsetFactor(
            data, root, step, domainStart, domainEnd, SplineOracle{1}, false);
        const auto curvatureLeft = 1 - unitLeft;
        const auto curvatureRight = 1 - unitRight;
        REQUIRE(abs(curvatureLeft - curvatureRight) < SplineOracle{"1e-8"});
        const double signedDistance =
            (2 / (curvatureLeft + curvatureRight)).convert_to<double>();
        const SplineOracle distance{signedDistance};
        const auto tangentLeft = oracleOffsetFactor(
            data, root, step, domainStart, domainEnd, distance, true);
        const auto tangentRight = oracleOffsetFactor(
            data, root, step, domainStart, domainEnd, distance, false);
        const auto before = oracleOffsetFactor(
            data, root - SplineOracle{"1e-7"}, step, domainStart, domainEnd,
            distance);
        const auto after = oracleOffsetFactor(
            data, root + SplineOracle{"1e-7"}, step, domainStart, domainEnd,
            distance);
        CAPTURE(scale, signedDistance, tangentLeft, tangentRight, before, after);
        CHECK(abs(tangentLeft) < SplineOracle{"1e-8"});
        CHECK(abs(tangentRight) < SplineOracle{"1e-8"});
        CHECK((before < 0) == (after < 0));

        const LC_OffsetSourceDefinition definition{
            LC_OffsetSourceGeometry{data}, signedDistance, 1e-4 * scale, false};
        const auto options =
            LC_OffsetWorkerTolerance::directOptions(definition, source);
        REQUIRE(options);
        LC_CurveOffsetCuspIsolationDiagnostics diagnostics;
        const auto built = LC_CurveOffset::buildDirectBranches(
            source,
            LC_CurveOffset::makeSideRequest(LC_CurveOffsetSide::Right,
                                            std::abs(signedDistance)),
            *options, LC_CurveOffset::makeDirectSourceBudget(), &diagnostics);
        CAPTURE(built.status, diagnostics.failureStage,
                diagnostics.stallIntervals, diagnostics.failingRunExtent,
                options->tolerance.nodeMerge);
        REQUIRE(built.status == LC_CurveOffsetStatus::Ok);
        REQUIRE(diagnostics.complete);
        const auto stall = std::find_if(
            diagnostics.stallIntervals.begin(), diagnostics.stallIntervals.end(),
            [](const std::pair<double, double>& interval) {
                return interval.first <= .5 && .5 <= interval.second;
            });
        REQUIRE(stall != diagnostics.stallIntervals.end());

        double motionBound = 0.0;
        const auto checkKnotSide = [&](const double lo, const double hi,
                                       const bool knotAtLeft) {
            LC_CurveJetBounds bounds;
            REQUIRE(source.tryBoundJetForCusp(lo, hi, bounds, signedDistance));
            const LC_Interval factor =
                bounds.offsetFactorNumeratorForMotionBound(signedDistance);
            REQUIRE(factor.isValid());
            REQUIRE(bounds.speedSquared().lo() > 0.0);
            motionBound +=
                std::max(std::abs(factor.lo()), std::abs(factor.hi())) /
                bounds.speedSquared().lo() * (hi - lo);
            for (const double parameter : {lo, hi}) {
                const bool leftLimit = parameter == .5 && !knotAtLeft;
                const auto value = oracleOffsetFactor(
                    data, SplineOracle{parameter}, step, domainStart, domainEnd,
                    distance, leftLimit);
                CHECK(factor.contains(value.convert_to<double>()));
            }
        };
        checkKnotSide(stall->first, .5, false);
        checkKnotSide(.5, stall->second, true);
        CAPTURE(motionBound, options->tolerance.nodeMerge);
        CHECK(motionBound <= options->tolerance.nodeMerge);

        const auto nearestEndpoint = [&](const RS_Vector& point) {
            double nearest = std::numeric_limits<double>::infinity();
            for (const LC_OffsetBranch& branch : built.branches) {
                for (const LC_OffsetCubicPiece& piece : branch.cubicPieces) {
                    nearest = std::min(
                        nearest,
                        std::min(piece.bezier.front().distanceTo(point),
                                 piece.bezier.back().distanceTo(point)));
                }
            }
            return nearest;
        };
        for (const double parameter : {stall->first, .5, stall->second}) {
            const auto point = oracleOffsetPoint(
                data, SplineOracle{parameter}, step, domainStart, domainEnd,
                distance);
            const RS_Vector expected{point.first.convert_to<double>(),
                                     point.second.convert_to<double>()};
            CHECK(nearestEndpoint(expected) <=
                  options->tolerance.requestedGeometry);
        }
    }
}

TEST_CASE("double nonuniform rational knot tangent maps to a bounded stall",
          "[lc-offsetcurve][worker-kernel][spline-variant][oracle][rational][tangent-root][knot][multiplicity]") {
    RS_SplineData data{3, false};
    data.type = RS_SplineData::SplineType::ClampedOpen;
    data.controlPoints = {{-2, 0}, {-1.6, .4}, {-8.0 / 9.0, 2.0 / 3.0},
                          {8.0 / 9.0, 2.0 / 3.0}, {1.6, .4}, {2, 0}};
    data.knotslist = {0, 0, 0, 0, .5, .5, 1, 1, 1, 1};
    data.weights = {1, 5.0 / 6.0, .75, .75, 5.0 / 6.0, 1};
    REQUIRE(data.controlPoints.size() == 6);
    REQUIRE(std::count(data.knotslist.begin(), data.knotslist.end(), .5) == 2);
    const RS_Spline source{nullptr, data};
    REQUIRE(source.validate());

    RS_SplineData originalData{3, false};
    originalData.type = RS_SplineData::SplineType::ClampedOpen;
    originalData.controlPoints = {{-2, 0}, {-1.6, .4}, {0, 1}, {1.6, .4}, {2, 0}};
    originalData.knotslist = {0, 0, 0, 0, .5, 1, 1, 1, 1};
    originalData.weights = {1, 5.0 / 6.0, 2.0 / 3.0, 5.0 / 6.0, 1};
    for (const double parameter : {.1, .3, .5, .7, .9}) {
        const auto refinedPoint = evaluateSplineOracle(data, SplineOracle{parameter});
        const auto originalPoint = evaluateSplineOracle(originalData, SplineOracle{parameter});
        CHECK(sqrt((refinedPoint.first - originalPoint.first) *
                   (refinedPoint.first - originalPoint.first) +
                   (refinedPoint.second - originalPoint.second) *
                   (refinedPoint.second - originalPoint.second)) <
              SplineOracle{"1e-12"});
    }

    const SplineOracle domainStart{0};
    const SplineOracle domainEnd{1};
    const SplineOracle root{"0.5"};
    const SplineOracle step{"1e-24"};
    const auto unitLeft = oracleOffsetFactor(
        data, root, step, domainStart, domainEnd, SplineOracle{1}, true);
    const auto unitRight = oracleOffsetFactor(
        data, root, step, domainStart, domainEnd, SplineOracle{1}, false);
    const auto curvatureLeft = 1 - unitLeft;
    const auto curvatureRight = 1 - unitRight;
    REQUIRE(abs(curvatureLeft - curvatureRight) < SplineOracle{"1e-8"});
    const double signedDistance =
        (2 / (curvatureLeft + curvatureRight)).convert_to<double>();
    const SplineOracle distance{signedDistance};
    const auto tangentLeft = oracleOffsetFactor(
        data, root, step, domainStart, domainEnd, distance, true);
    const auto tangentRight = oracleOffsetFactor(
        data, root, step, domainStart, domainEnd, distance, false);
    const auto before = oracleOffsetFactor(
        data, root - SplineOracle{"1e-7"}, step, domainStart, domainEnd,
        distance);
    const auto after = oracleOffsetFactor(
        data, root + SplineOracle{"1e-7"}, step, domainStart, domainEnd,
        distance);
    CAPTURE(signedDistance, tangentLeft, tangentRight, before, after);
    CHECK(abs(tangentLeft) < SplineOracle{"1e-8"});
    CHECK(abs(tangentRight) < SplineOracle{"1e-8"});
    CHECK((before < 0) == (after < 0));

    const LC_OffsetSourceDefinition definition{
        LC_OffsetSourceGeometry{data}, signedDistance, 1e-4, false};
    const auto options = LC_OffsetWorkerTolerance::directOptions(definition, source);
    REQUIRE(options);
    LC_CurveOffsetCuspIsolationDiagnostics diagnostics;
    const auto built = LC_CurveOffset::buildDirectBranches(
        source, LC_CurveOffset::makeSideRequest(LC_CurveOffsetSide::Right,
                                                std::abs(signedDistance)),
        *options, LC_CurveOffset::makeDirectSourceBudget(), &diagnostics);
    CAPTURE(built.status, diagnostics.failureStage, diagnostics.stallIntervals,
            diagnostics.failingRunExtent, diagnostics.failingRunBoxes,
            diagnostics.failingRunSignChanges, diagnostics.failingRunObserved,
            diagnostics.failureSpan, diagnostics.failureT0, diagnostics.failureT1,
            options->tolerance.nodeMerge);
    REQUIRE(built.status == LC_CurveOffsetStatus::Ok);
    REQUIRE(diagnostics.complete);
    const auto stall = std::find_if(
        diagnostics.stallIntervals.begin(), diagnostics.stallIntervals.end(),
        [](const std::pair<double, double>& interval) {
            return interval.first <= .5 && .5 <= interval.second;
        });
    REQUIRE(stall != diagnostics.stallIntervals.end());

    double motionBound = 0.0;
    for (const auto [lo, hi, knotAtLeft] : {
             std::tuple<double, double, bool>{stall->first, .5, false},
             std::tuple<double, double, bool>{.5, stall->second, true}}) {
        LC_CurveJetBounds bounds;
        REQUIRE(source.tryBoundJetForCusp(lo, hi, bounds, signedDistance));
        const LC_Interval factor =
            bounds.offsetFactorNumeratorForMotionBound(signedDistance);
        REQUIRE(factor.isValid());
        REQUIRE(bounds.speedSquared().lo() > 0.0);
        motionBound += std::max(std::abs(factor.lo()), std::abs(factor.hi())) /
                       bounds.speedSquared().lo() * (hi - lo);
        for (const double parameter : {lo, hi}) {
            const bool leftLimit = parameter == .5 && !knotAtLeft;
            const auto value = oracleOffsetFactor(
                data, SplineOracle{parameter}, step, domainStart, domainEnd,
                distance, leftLimit);
            CHECK(factor.contains(value.convert_to<double>()));
        }
    }
    CAPTURE(motionBound, options->tolerance.nodeMerge);
    CHECK(motionBound <= options->tolerance.nodeMerge);

    const auto nearestEndpoint = [&](const RS_Vector& point) {
        double nearest = std::numeric_limits<double>::infinity();
        for (const LC_OffsetBranch& branch : built.branches) {
            for (const LC_OffsetCubicPiece& piece : branch.cubicPieces) {
                nearest = std::min(
                    nearest,
                    std::min(piece.bezier.front().distanceTo(point),
                             piece.bezier.back().distanceTo(point)));
            }
        }
        return nearest;
    };
    for (const double parameter : {stall->first, .5, stall->second}) {
        const auto point = oracleOffsetPoint(
            data, SplineOracle{parameter}, step, domainStart, domainEnd, distance);
        const RS_Vector expected{point.first.convert_to<double>(),
                                 point.second.convert_to<double>()};
        CHECK(nearestEndpoint(expected) <= options->tolerance.requestedGeometry);
    }
}

TEST_CASE("alternate rational double-knot tangent maps to a bounded stall",
          "[lc-offsetcurve][worker-kernel][spline-variant][oracle][rational][tangent-root][knot][multiplicity]") {
    RS_SplineData data{3, false};
    data.type = RS_SplineData::SplineType::ClampedOpen;
    data.controlPoints = {{-3, 0}, {-18.0 / 7.0, 6.0 / 7.0},
                          {-18.0 / 13.0, 18.0 / 13.0},
                          {18.0 / 13.0, 18.0 / 13.0},
                          {18.0 / 7.0, 6.0 / 7.0}, {3, 0}};
    data.knotslist = {0, 0, 0, 0, .5, .5, 1, 1, 1, 1};
    data.weights = {1, .875, .8125, .8125, .875, 1};
    const RS_Spline source{nullptr, data};
    REQUIRE(source.validate());

    RS_SplineData singleSpan{3, false};
    singleSpan.type = RS_SplineData::SplineType::ClampedOpen;
    singleSpan.controlPoints = {{-3, 0}, {-2, 2}, {2, 2}, {3, 0}};
    singleSpan.knotslist = {0, 0, 0, 0, 1, 1, 1, 1};
    singleSpan.weights = {1, .75, .75, 1};
    for (const double parameter : {.1, .3, .5, .7, .9}) {
        const auto refined = evaluateSplineOracle(data, SplineOracle{parameter});
        const auto original = evaluateSplineOracle(singleSpan, SplineOracle{parameter});
        CHECK(sqrt((refined.first - original.first) * (refined.first - original.first) +
                   (refined.second - original.second) * (refined.second - original.second)) <
              SplineOracle{"1e-12"});
    }

    const SplineOracle domainStart{0};
    const SplineOracle domainEnd{1};
    const SplineOracle knot{"0.5"};
    const SplineOracle step{"1e-24"};
    const auto unitLeft = oracleOffsetFactor(
        data, knot, step, domainStart, domainEnd, SplineOracle{1}, true);
    const auto unitRight = oracleOffsetFactor(
        data, knot, step, domainStart, domainEnd, SplineOracle{1}, false);
    const auto curvatureLeft = 1 - unitLeft;
    const auto curvatureRight = 1 - unitRight;
    REQUIRE(abs(curvatureLeft - curvatureRight) < SplineOracle{"1e-10"});
    const double signedDistance =
        (2 / (curvatureLeft + curvatureRight)).convert_to<double>();
    const SplineOracle distance{signedDistance};
    const auto tangentFactor = oracleOffsetFactor(
        data, knot, step, domainStart, domainEnd, distance);
    const auto leftFactor = oracleOffsetFactor(
        data, knot - SplineOracle{"1e-7"}, step, domainStart, domainEnd, distance);
    const auto rightFactor = oracleOffsetFactor(
        data, knot + SplineOracle{"1e-7"}, step, domainStart, domainEnd, distance);
    CAPTURE(signedDistance, tangentFactor, leftFactor, rightFactor);
    CHECK(abs(tangentFactor) < SplineOracle{"1e-8"});
    CHECK(leftFactor != 0);
    CHECK(rightFactor != 0);
    CHECK((leftFactor < 0) == (rightFactor < 0));

    const LC_OffsetSourceDefinition definition{
        LC_OffsetSourceGeometry{data}, signedDistance, 1e-4, false};
    const auto options = LC_OffsetWorkerTolerance::directOptions(definition, source);
    REQUIRE(options);
    LC_CurveOffsetCuspIsolationDiagnostics diagnostics;
    const auto built = LC_CurveOffset::buildDirectBranches(
        source,
        LC_CurveOffset::makeSideRequest(signedDistance > 0
                                            ? LC_CurveOffsetSide::Left
                                            : LC_CurveOffsetSide::Right,
                                        std::abs(signedDistance)),
        *options, LC_CurveOffset::makeDirectSourceBudget(), &diagnostics);
    CAPTURE(signedDistance, built.status, diagnostics.failureStage,
            diagnostics.stallIntervals, diagnostics.failingRunExtent,
            diagnostics.failingRunBoxes, options->tolerance.nodeMerge);
    REQUIRE(built.status == LC_CurveOffsetStatus::Ok);
    REQUIRE(diagnostics.complete);
    const auto stall = std::find_if(
        diagnostics.stallIntervals.begin(), diagnostics.stallIntervals.end(),
        [](const std::pair<double, double>& interval) {
            return interval.first <= .5 && .5 <= interval.second;
        });
    REQUIRE(stall != diagnostics.stallIntervals.end());

    double motionBound = 0.0;
    for (const auto [lo, hi, knotAtLeft] : {
             std::tuple<double, double, bool>{stall->first, .5, false},
             std::tuple<double, double, bool>{.5, stall->second, true}}) {
        LC_CurveJetBounds bounds;
        REQUIRE(source.tryBoundJetForCusp(lo, hi, bounds, signedDistance));
        const auto factor =
            bounds.offsetFactorNumeratorForMotionBound(signedDistance);
        REQUIRE(factor.isValid());
        REQUIRE(bounds.speedSquared().lo() > 0.0);
        motionBound += std::max(std::abs(factor.lo()), std::abs(factor.hi())) /
                       bounds.speedSquared().lo() * (hi - lo);
        for (const double parameter : {lo, hi}) {
            const bool leftLimit = parameter == .5 && !knotAtLeft;
            const auto oracle = oracleOffsetFactor(
                data, SplineOracle{parameter}, step, domainStart, domainEnd,
                distance, leftLimit);
            CHECK(factor.contains(oracle.convert_to<double>()));
        }
    }
    CHECK(motionBound <= options->tolerance.nodeMerge);

    for (const double parameter : {stall->first, .5, stall->second}) {
        const auto point = oracleOffsetPoint(
            data, SplineOracle{parameter}, step, domainStart, domainEnd, distance);
        const RS_Vector expected{point.first.convert_to<double>(),
                                 point.second.convert_to<double>()};
        double nearestEndpoint = std::numeric_limits<double>::infinity();
        for (const LC_OffsetBranch& branch : built.branches) {
            for (const LC_OffsetCubicPiece& piece : branch.cubicPieces) {
                nearestEndpoint = std::min(
                    nearestEndpoint,
                    std::min(piece.bezier.front().distanceTo(expected),
                             piece.bezier.back().distanceTo(expected)));
            }
        }
        CHECK(nearestEndpoint <= options->tolerance.requestedGeometry);
    }
}

TEST_CASE("unit-weight cubic tangent at a double knot maps as a bounded stall",
          "[lc-offsetcurve][worker-kernel][spline-variant][oracle][tangent-root][knot][multiplicity]") {
    RS_SplineData data{3, false};
    data.type = RS_SplineData::SplineType::ClampedOpen;
    data.controlPoints = {{0, 1.0 / 8.0}, {1.0 / 6.0, 1.0 / 24.0},
                          {1.0 / 3.0, 0}, {2.0 / 3.0, 0},
                          {5.0 / 6.0, 1.0 / 24.0}, {1, 1.0 / 8.0}};
    data.knotslist = {0, 0, 0, 0, .5, .5, 1, 1, 1, 1};
    data.weights = {1, 1, 1, 1, 1, 1};
    const RS_Spline source{nullptr, data};
    REQUIRE(source.validate());

    const SplineOracle domainStart{0};
    const SplineOracle domainEnd{1};
    const SplineOracle root{"0.5"};
    const SplineOracle step{"1e-24"};
    const SplineOracle distance{1};
    for (const double parameter : {.1, .3, .5, .7, .9}) {
        const auto point = evaluateSplineOracle(data, SplineOracle{parameter});
        CHECK(abs(point.first - SplineOracle{parameter}) < SplineOracle{"1e-15"});
        const SplineOracle y{parameter};
        CHECK(abs(point.second - (y - SplineOracle{"0.5"}) *
                                 (y - SplineOracle{"0.5"}) / 2) <
              SplineOracle{"1e-15"});
    }
    const auto tangentLeft = oracleOffsetFactor(
        data, root, step, domainStart, domainEnd, distance, true);
    const auto tangentRight = oracleOffsetFactor(
        data, root, step, domainStart, domainEnd, distance, false);
    const auto before = oracleOffsetFactor(
        data, root - SplineOracle{"1e-6"}, step, domainStart, domainEnd,
        distance);
    const auto after = oracleOffsetFactor(
        data, root + SplineOracle{"1e-6"}, step, domainStart, domainEnd,
        distance);
    CAPTURE(tangentLeft, tangentRight, before, after);
    CHECK(abs(tangentLeft) < SplineOracle{"1e-15"});
    CHECK(abs(tangentRight) < SplineOracle{"1e-15"});
    CHECK(before > 0);
    CHECK(after > 0);

    const LC_OffsetSourceDefinition definition{
        LC_OffsetSourceGeometry{data}, 1.0, 1e-4, false};
    const auto options = LC_OffsetWorkerTolerance::directOptions(definition, source);
    REQUIRE(options);
    LC_CurveOffsetCuspIsolationDiagnostics diagnostics;
    const auto built = LC_CurveOffset::buildDirectBranches(
        source,
        LC_CurveOffset::makeSideRequest(LC_CurveOffsetSide::Left, 1.0),
        *options, LC_CurveOffset::makeDirectSourceBudget(), &diagnostics);
    CAPTURE(built.status, diagnostics.failureStage, diagnostics.stallIntervals,
            diagnostics.failingRunExtent, options->tolerance.nodeMerge);
    REQUIRE(built.status == LC_CurveOffsetStatus::Ok);
    REQUIRE(diagnostics.complete);
    const auto stall = std::find_if(
        diagnostics.stallIntervals.begin(), diagnostics.stallIntervals.end(),
        [](const std::pair<double, double>& interval) {
            return interval.first <= .5 && .5 <= interval.second;
        });
    REQUIRE(stall != diagnostics.stallIntervals.end());

    double motionBound = 0.0;
    for (const auto [lo, hi, knotAtLeft] : {
             std::tuple<double, double, bool>{stall->first, .5, false},
             std::tuple<double, double, bool>{.5, stall->second, true}}) {
        LC_CurveJetBounds bounds;
        REQUIRE(source.tryBoundJetForCusp(lo, hi, bounds, 1.0));
        const LC_Interval factor = bounds.offsetFactorNumeratorForMotionBound(1.0);
        REQUIRE(factor.isValid());
        REQUIRE(bounds.speedSquared().lo() > 0.0);
        motionBound += std::max(std::abs(factor.lo()), std::abs(factor.hi())) /
                       bounds.speedSquared().lo() * (hi - lo);
        for (const double parameter : {lo, hi}) {
            const bool leftLimit = parameter == .5 && !knotAtLeft;
            const auto value = oracleOffsetFactor(
                data, SplineOracle{parameter}, step, domainStart, domainEnd,
                distance, leftLimit);
            CHECK(factor.contains(value.convert_to<double>()));
        }
    }
    CAPTURE(motionBound, options->tolerance.nodeMerge);
    CHECK(motionBound <= options->tolerance.nodeMerge);

    const auto nearestEndpoint = [&](const RS_Vector& point) {
        double nearest = std::numeric_limits<double>::infinity();
        for (const LC_OffsetBranch& branch : built.branches) {
            for (const LC_OffsetCubicPiece& piece : branch.cubicPieces) {
                nearest = std::min(
                    nearest,
                    std::min(piece.bezier.front().distanceTo(point),
                             piece.bezier.back().distanceTo(point)));
            }
        }
        return nearest;
    };
    for (const double parameter : {stall->first, .5, stall->second}) {
        const auto point = oracleOffsetPoint(
            data, SplineOracle{parameter}, step, domainStart, domainEnd, distance);
        const RS_Vector expected{point.first.convert_to<double>(),
                                 point.second.convert_to<double>()};
        CHECK(nearestEndpoint(expected) <= options->tolerance.requestedGeometry);
    }
}

TEST_CASE("nonuniform rational tangent root at a knot remains a bounded stall",
          "[lc-offsetcurve][worker-kernel][spline-variant][oracle][rational][tangent-root]") {
    RS_SplineData data{2, false};
    data.type = RS_SplineData::SplineType::ClampedOpen;
    data.controlPoints = {{-6, 6}, {-1.5, -3}, {1.5, -3}, {6, 6}};
    data.knotslist = {0, 0, 0, .5, 1, 1, 1};
    data.weights = {1, 2, 2, 1};
    const RS_Spline source{nullptr, data};
    REQUIRE(source.validate());

    const SplineOracle domainStart{0};
    const SplineOracle domainEnd{1};
    const SplineOracle knot{"0.5"};
    const SplineOracle step{"1e-24"};
    const SplineOracle unitLeft = oracleOffsetFactor(
        data, knot, step, domainStart, domainEnd, SplineOracle{1}, true);
    const SplineOracle unitRight = oracleOffsetFactor(
        data, knot, step, domainStart, domainEnd, SplineOracle{1}, false);
    const SplineOracle curvatureLeft = 1 - unitLeft;
    const SplineOracle curvatureRight = 1 - unitRight;
    REQUIRE(curvatureLeft > 0);
    REQUIRE(curvatureRight > 0);
    CHECK(abs(curvatureLeft - curvatureRight) < SplineOracle{"1e-15"});
    CHECK(abs(curvatureLeft - 1) < SplineOracle{"1e-15"});
    CHECK(abs(curvatureRight - 1) < SplineOracle{"1e-15"});
    const double signedDistance = 1.0;
    const SplineOracle distance{signedDistance};
    const SplineOracle tangentLeft = oracleOffsetFactor(
        data, knot, step, domainStart, domainEnd, distance, true);
    const SplineOracle tangentRight = oracleOffsetFactor(
        data, knot, step, domainStart, domainEnd, distance, false);
    const SplineOracle leftNeighbor = oracleOffsetFactor(
        data, knot - SplineOracle{"1e-8"}, step, domainStart, domainEnd, distance);
    const SplineOracle rightNeighbor = oracleOffsetFactor(
        data, knot + SplineOracle{"1e-8"}, step, domainStart, domainEnd, distance);
    CAPTURE(signedDistance, tangentLeft, tangentRight, leftNeighbor, rightNeighbor);
    CHECK(abs(tangentLeft) < SplineOracle{"1e-12"});
    CHECK(abs(tangentRight) < SplineOracle{"1e-12"});
    CHECK(leftNeighbor != 0);
    CHECK(rightNeighbor != 0);
    CHECK((leftNeighbor < 0) == (rightNeighbor < 0));

    const LC_OffsetSourceDefinition definition{
        LC_OffsetSourceGeometry{data}, signedDistance, 1e-4, false};
    const auto options = LC_OffsetWorkerTolerance::directOptions(definition, source);
    REQUIRE(options);
    LC_CurveOffsetCuspIsolationDiagnostics diagnostics;
    const auto built = LC_CurveOffset::buildDirectBranches(
        source,
        LC_CurveOffset::makeSideRequest(LC_CurveOffsetSide::Left, signedDistance),
        *options, LC_CurveOffset::makeDirectSourceBudget(), &diagnostics);
    CAPTURE(built.status, diagnostics.status, diagnostics.candidateParameters,
            diagnostics.candidateBoxes, diagnostics.stallIntervals,
            diagnostics.failureSpan, diagnostics.failureT0, diagnostics.failureT1,
            diagnostics.failureStage,
            diagnostics.failingRunBoxes, diagnostics.failingRunExtent,
            diagnostics.failingRunSignChanges, diagnostics.failingRunObserved);
    CHECK(built.status == LC_CurveOffsetStatus::Ok);
    CHECK(diagnostics.complete == (diagnostics.status == LC_CurveOffsetStatus::Ok));
    const bool tangentCandidate = std::any_of(
        diagnostics.candidateBoxes.begin(), diagnostics.candidateBoxes.end(),
        [](const LC_CurveOffsetCuspCandidateBox& box) {
            return std::abs(box.parameter - .5) < 1e-6;
        });
    const bool tangentStall = std::any_of(
        diagnostics.stallIntervals.begin(), diagnostics.stallIntervals.end(),
        [](const std::pair<double, double>& interval) {
            return interval.first <= .5 && interval.second >= .5;
        });
    CHECK((tangentCandidate || tangentStall));

    const auto oraclePoint = oracleOffsetPoint(
        data, knot, step, domainStart, domainEnd, distance, false);
    const RS_Vector expectedPoint{oraclePoint.first.convert_to<double>(),
                                  oraclePoint.second.convert_to<double>()};
    double nearestOutput = std::numeric_limits<double>::infinity();
    for (const LC_OffsetBranch& branch : built.branches) {
        for (const LC_OffsetCubicPiece& piece : branch.cubicPieces) {
            const auto& p = piece.bezier;
            for (int sample = 0; sample <= 100; ++sample) {
                const double t = sample / 100.0;
                const double u = 1.0 - t;
                const RS_Vector point = p[0] * (u * u * u) +
                                        p[1] * (3.0 * u * u * t) +
                                        p[2] * (3.0 * u * t * t) +
                                        p[3] * (t * t * t);
                nearestOutput = std::min(nearestOutput, point.distanceTo(expectedPoint));
            }
        }
    }
    CAPTURE(expectedPoint.x, expectedPoint.y, nearestOutput,
            options->tolerance.requestedGeometry);
    CHECK(nearestOutput <= options->tolerance.requestedGeometry);
}

TEST_CASE("ellipse, arc, and circle source caches validate across signed distances",
          "[lc-offsetcurve][worker-kernel][source-cache][conic][side]") {
    const double tau = 2.0 * M_PI;
    const std::array<LC_OffsetSourceGeometry, 8> sources{
        LC_OffsetSourceGeometry{RS_EllipseData{{0, 0}, {4, 0}, .6, 0, tau, false}},
        LC_OffsetSourceGeometry{RS_EllipseData{{3, -2}, {3, 0}, .7, .2, 2.1, false}},
        LC_OffsetSourceGeometry{RS_EllipseData{{-3, 1}, {2, 1}, .8, 1.8, .1, true}},
        LC_OffsetSourceGeometry{RS_EllipseData{{1, -2}, {2, 0}, 1.4, .3, 2.3, false}},
        LC_OffsetSourceGeometry{RS_EllipseData{{0, 0}, {2, 0}, .2, 0, tau, false}},
        LC_OffsetSourceGeometry{RS_EllipseData{{0, 0}, {2, 0}, .2, 0, 0, true}},
        LC_OffsetSourceGeometry{RS_ArcData{{-1, 2}, 2.5, .2, 2.4, true}},
        LC_OffsetSourceGeometry{RS_CircleData{{1, -1}, 2}}
    };
    std::size_t acceptedCaches = 0;
    std::size_t refusedCaches = 0;
    std::size_t expectedCases = 0;
    for (const auto& source : sources) {
        CAPTURE(static_cast<int>(source.kind()));
        REQUIRE(source.isWellFormed());
        const auto* sourceEllipse = std::get_if<RS_EllipseData>(&source.value);
        const bool highEccentricity = sourceEllipse && sourceEllipse->ratio == .2;
        const std::vector<double> distances = highEccentricity
            ? std::vector<double>{-.09, -.08, -.07, -.05, .05, .07, .08, .09}
            : std::vector<double>{-.4, -.1, .1, .4};
        expectedCases += distances.size();
        for (const double signedDistance : distances) {
            CAPTURE(signedDistance);
            const LC_OffsetSourceDefinition definition{source, signedDistance, 1e-4, false};
            const req::Request generationRequest{token,
                req::GenerateSource{definition, {}}};
            const auto generationReply = LC_OffsetWorkerKernel::run(generationRequest);
            const auto& generated = native(generationReply, 7);
            REQUIRE(generated.engineStatus == LC_CurveOffsetStatus::Ok);
            REQUIRE(generated.ordinaryStatus == result::OrdinaryStatus::Complete);
            REQUIRE_FALSE(generated.ordinary.empty());
            if (highEccentricity) {
                const bool inward = signedDistance *
                    (sourceEllipse->reversed ? -1.0 : 1.0) > 0.0;
                const double magnitude = std::abs(signedDistance);
                if (!inward || magnitude < .08)
                    REQUIRE(generated.ordinary.size() == 1);
                if (magnitude == .08)
                    CHECK(generated.ordinary.size() == 1);
                if (inward && magnitude == .09) CHECK(generated.ordinary.size() > 1);
            } else {
                REQUIRE(generated.ordinary.size() == 1);
            }

            RS_SplineData cache;
            if (const auto* spline = std::get_if<RS_SplineData>(&generated.ordinary.front())) {
                cache = *spline;
            } else {
                const auto* segment = std::get_if<req::Segment>(&generated.ordinary.front());
                REQUIRE(segment);
                cache.degree = 1;
                cache.controlPoints = {segment->start, segment->end};
                cache.knotslist = {0, 0, 1, 1};
                cache.weights = {1, 1};
            }

            const req::Request validationRequest{token,
                req::ValidateSourceCache{cache, definition, {}}};
            const auto validationReply = LC_OffsetWorkerKernel::run(validationRequest);
            const auto& validated = native(validationReply, 8);
            if (generated.ordinary.size() == 1) {
                CHECK(validated.refusal == LC_OffsetCurveRefusal::None);
                CHECK(validated.changed);
                ++acceptedCaches;
            } else {
                CHECK(validated.refusal == LC_OffsetCurveRefusal::NotCacheRepresentable);
                CHECK_FALSE(validated.changed);
                ++refusedCaches;
            }
            CHECK(validated.engineRuns == 1);
        }
    }
    CHECK(acceptedCaches + refusedCaches == expectedCases);
    CHECK(acceptedCaches >= 24);
    CHECK(refusedCaches > 0);
}

TEST_CASE("parabola source caches validate across orientation and curvature distances",
          "[lc-offsetcurve][worker-kernel][source-cache][parabola][side]") {
    const LC_ParabolaData bowl{{RS_Vector{0, 0}, RS_Vector{5, 10}, RS_Vector{10, 0}}};
    const LC_ParabolaData reversed{{RS_Vector{10, 0}, RS_Vector{5, 10}, RS_Vector{0, 0}}};
    const LC_ParabolaData rotated{{RS_Vector{3, 4}, RS_Vector{-7, 9}, RS_Vector{3, 14}}};
    const std::array<LC_OffsetSourceGeometry, 3> sources{
        LC_OffsetSourceGeometry{bowl},
        LC_OffsetSourceGeometry{reversed},
        LC_OffsetSourceGeometry{rotated}
    };
    std::size_t acceptedCaches = 0;
    std::size_t refusedCaches = 0;
    for (std::size_t sourceIndex = 0; sourceIndex < sources.size(); ++sourceIndex) {
        const auto& source = sources[sourceIndex];
        CAPTURE(sourceIndex);
        REQUIRE(source.kind() == LC_OffsetSourceGeometry::Kind::Parabola);
        REQUIRE(source.isWellFormed());
        for (const double signedDistance : {-.5, -.25, .25, .5, -2.0, 2.0,
                                            -2.5, 2.5, -3.0, 3.0}) {
            CAPTURE(signedDistance);
            const LC_OffsetSourceDefinition definition{source, signedDistance, 1e-4, false};
            const req::Request generationRequest{token,
                req::GenerateSource{definition, {}}};
            const auto generationReply = LC_OffsetWorkerKernel::run(generationRequest);
            const auto& generated = native(generationReply, 7);
            REQUIRE(generated.engineStatus == LC_CurveOffsetStatus::Ok);
            REQUIRE(generated.ordinaryStatus == result::OrdinaryStatus::Complete);
            REQUIRE_FALSE(generated.ordinary.empty());

            RS_SplineData cache;
            if (const auto* spline = std::get_if<RS_SplineData>(&generated.ordinary.front())) {
                cache = *spline;
            } else {
                const auto* segment = std::get_if<req::Segment>(&generated.ordinary.front());
                REQUIRE(segment);
                cache.degree = 1;
                cache.controlPoints = {segment->start, segment->end};
                cache.knotslist = {0, 0, 1, 1};
                cache.weights = {1, 1};
            }

            const req::Request validationRequest{token,
                req::ValidateSourceCache{cache, definition, {}}};
            const auto validationReply = LC_OffsetWorkerKernel::run(validationRequest);
            const auto& validated = native(validationReply, 8);
            if (generated.ordinary.size() == 1) {
                CHECK(validated.refusal == LC_OffsetCurveRefusal::None);
                CHECK(validated.changed);
                ++acceptedCaches;
            } else {
                CHECK(validated.refusal == LC_OffsetCurveRefusal::NotCacheRepresentable);
                CHECK_FALSE(validated.changed);
                ++refusedCaches;
            }
            CHECK(validated.engineRuns == 1);
        }
    }
    CHECK(acceptedCaches + refusedCaches == sources.size() * 10);
    CHECK(acceptedCaches > 0);
    CHECK(refusedCaches > 0);
}

TEST_CASE("parabola worker cache validation is stable across uniform scales",
          "[lc-offsetcurve][worker-kernel][source-cache][parabola][scale]") {
    std::size_t acceptedCaches = 0;
    std::size_t refusedCaches = 0;
    for (const double scale : {1e-8, 1e-3, 1e3, 1e12}) {
        CAPTURE(scale);
        const LC_ParabolaData parabola{{RS_Vector{0, 0}, RS_Vector{5 * scale, 10 * scale},
                                        RS_Vector{10 * scale, 0}}};
        const LC_OffsetSourceGeometry source{parabola};
        REQUIRE(source.isWellFormed());
        for (const double unitDistance : {-.5, .5, -2.0, 2.0, -2.5, 2.5, -3.0, 3.0}) {
            CAPTURE(unitDistance);
            const double signedDistance = unitDistance * scale;
            const LC_OffsetSourceDefinition definition{source, signedDistance, 1e-4, false};
            const req::Request generationRequest{token, req::GenerateSource{definition, {}}};
            const auto generationReply = LC_OffsetWorkerKernel::run(generationRequest);
            const auto& generated = native(generationReply, 7);
            REQUIRE(generated.engineStatus == LC_CurveOffsetStatus::Ok);
            REQUIRE(generated.ordinaryStatus == result::OrdinaryStatus::Complete);
            REQUIRE_FALSE(generated.ordinary.empty());

            auto sourceEntity = source.makeEntity();
            REQUIRE(sourceEntity);
            const auto effectiveOptions = LC_OffsetWorkerTolerance::directOptions(
                definition, *sourceEntity);
            REQUIRE(effectiveOptions);
            CHECK(effectiveOptions->tolerance.requestedGeometry < scale * 2e-3);

            RS_SplineData cache;
            if (const auto* spline = std::get_if<RS_SplineData>(&generated.ordinary.front())) {
                cache = *spline;
            } else {
                const auto* segment = std::get_if<req::Segment>(&generated.ordinary.front());
                REQUIRE(segment);
                cache.degree = 1;
                cache.controlPoints = {segment->start, segment->end};
                cache.knotslist = {0, 0, 1, 1};
                cache.weights = {1, 1};
            }

            const req::Request validationRequest{token,
                req::ValidateSourceCache{cache, definition, {}}};
            const auto validationReply = LC_OffsetWorkerKernel::run(validationRequest);
            const auto& validated = native(validationReply, 8);
            if (generated.ordinary.size() == 1) {
                CHECK(validated.refusal == LC_OffsetCurveRefusal::None);
                CHECK(validated.changed);
                ++acceptedCaches;
            } else {
                CHECK(validated.refusal == LC_OffsetCurveRefusal::NotCacheRepresentable);
                CHECK_FALSE(validated.changed);
                ++refusedCaches;
            }
            CHECK(validated.engineRuns == 1);
        }
    }
    CHECK(acceptedCaches + refusedCaches == 32);
    CHECK(acceptedCaches > 0);
    CHECK(refusedCaches > 0);
}

TEST_CASE("typed worker tolerance defaults relatively and scales with the source",
          "[lc-offsetcurve][parabola][scale][tolerance][worker-kernel]") {
    double previousNormalized = 0.0;
    for (const double scale : {1e-8, 1e-3, 1e3, 1e12}) {
        CAPTURE(scale);
        const LC_OffsetSourceGeometry source{LC_ParabolaData{{
            RS_Vector{0, 0}, RS_Vector{5 * scale, 10 * scale}, RS_Vector{10 * scale, 0}}}};
        REQUIRE(source.isWellFormed());
        auto sourceEntity = source.makeEntity();
        REQUIRE(sourceEntity);

        const LC_OffsetSourceDefinition defaultDefinition{source, 0.5 * scale, 0.0, false};
        const LC_OffsetSourceDefinition explicitDefinition{
            source, 0.5 * scale, LC_CurveOffset::relativeOffsetTolerance(*sourceEntity), false};
        const auto defaultOptions = LC_OffsetWorkerTolerance::directOptions(
            defaultDefinition, *sourceEntity);
        const auto explicitOptions = LC_OffsetWorkerTolerance::directOptions(
            explicitDefinition, *sourceEntity);
        REQUIRE(defaultOptions);
        REQUIRE(explicitOptions);
        CHECK(defaultOptions->tolerance.requestedGeometry ==
              explicitOptions->tolerance.requestedGeometry);
        const double normalizedTolerance = defaultOptions->tolerance.requestedGeometry / scale;
        if (previousNormalized > 0.0) {
            CHECK(std::abs(normalizedTolerance / previousNormalized - 1.0) < 1e-12);
        }
        previousNormalized = normalizedTolerance;

        const auto generated = LC_OffsetWorkerKernel::run({token,
            req::GenerateSource{defaultDefinition, {}}});
        CHECK(native(generated, 7).engineStatus == LC_CurveOffsetStatus::Ok);
    }
}

TEST_CASE("parabola worker reports the absolute lower distance bound explicitly",
          "[lc-offsetcurve][parabola][scale][refusal][worker-kernel]") {
    constexpr double scale = 1e-12;
    const LC_OffsetSourceGeometry source{LC_ParabolaData{{
        RS_Vector{0, 0}, RS_Vector{5 * scale, 10 * scale}, RS_Vector{10 * scale, 0}}}};
    REQUIRE(source.isWellFormed());
    const LC_OffsetSourceDefinition definition{source, 0.5 * scale, 1e-4, false};
    const auto reply = LC_OffsetWorkerKernel::run({token,
        req::GenerateSource{definition, {}}});
    const auto& generated = native(reply, 7);
    CHECK(generated.refusal == LC_OffsetCurveRefusal::TooSmall);
    CHECK(generated.engineRuns == 0);
    CHECK(generated.ordinaryStatus == result::OrdinaryStatus::None);
    CHECK(generated.ordinary.empty());
    CHECK_FALSE(generated.changed);
}

TEST_CASE("parabola worker cache validation preserves local detail at large coordinates",
          "[lc-offsetcurve][worker-kernel][source-cache][parabola][precision]") {
    const RS_Vector translation{1e9, -1e9};
    const LC_OffsetSourceGeometry centered{LC_ParabolaData{{
        RS_Vector{0, 0}, RS_Vector{5, 10}, RS_Vector{10, 0}}}};
    const LC_OffsetSourceGeometry translated{LC_ParabolaData{{
        RS_Vector{0, 0} + translation, RS_Vector{5, 10} + translation,
        RS_Vector{10, 0} + translation}}};
    REQUIRE(centered.isWellFormed());
    REQUIRE(translated.isWellFormed());

    for (const double signedDistance : {-.5, .5, -2.5, 2.5, -3.0, 3.0}) {
        CAPTURE(signedDistance);
        const LC_OffsetSourceDefinition centeredDefinition{
            centered, signedDistance, 1e-4, false};
        const LC_OffsetSourceDefinition translatedDefinition{
            translated, signedDistance, 1e-4, false};
        const auto centeredReply = LC_OffsetWorkerKernel::run({token,
            req::GenerateSource{centeredDefinition, {}}});
        const auto translatedReply = LC_OffsetWorkerKernel::run({token,
            req::GenerateSource{translatedDefinition, {}}});
        const auto& centeredOutput = native(centeredReply, 7);
        const auto& translatedOutput = native(translatedReply, 7);
        REQUIRE(centeredOutput.engineStatus == LC_CurveOffsetStatus::Ok);
        REQUIRE(translatedOutput.engineStatus == LC_CurveOffsetStatus::Ok);
        REQUIRE(centeredOutput.ordinaryStatus == result::OrdinaryStatus::Complete);
        REQUIRE(translatedOutput.ordinaryStatus == result::OrdinaryStatus::Complete);
        REQUIRE_FALSE(centeredOutput.ordinary.empty());
        REQUIRE_FALSE(translatedOutput.ordinary.empty());
        REQUIRE(centeredOutput.ordinary.size() == translatedOutput.ordinary.size());

        const auto firstCache = [](const result::Native& output) {
            if (const auto* spline = std::get_if<RS_SplineData>(&output.ordinary.front()))
                return *spline;
            const auto& segment = std::get<req::Segment>(output.ordinary.front());
            RS_SplineData cache{1, false};
            cache.controlPoints = {segment.start, segment.end};
            cache.knotslist = {0, 0, 1, 1};
            cache.weights = {1, 1};
            return cache;
        };
        const RS_SplineData centeredCache = firstCache(centeredOutput);
        const RS_SplineData translatedCache = firstCache(translatedOutput);
        const RS_Spline centeredSpline{nullptr, centeredCache};
        const RS_Spline translatedSpline{nullptr, translatedCache};
        double maximumLocalDeviation = 0.0;
        for (int sample = 0; sample <= 128; ++sample) {
            const double parameter = static_cast<double>(sample) / 128.0;
            const RS_Vector local = centeredSpline.getPointAt(parameter);
            const RS_Vector world = translatedSpline.getPointAt(parameter);
            REQUIRE(local.valid);
            REQUIRE(world.valid);
            maximumLocalDeviation = std::max(maximumLocalDeviation,
                std::hypot((world.x - translation.x) - local.x,
                           (world.y - translation.y) - local.y));
        }
        CHECK(maximumLocalDeviation < 1e-3);

        for (const auto& [definition, cache] : {
                 std::pair{centeredDefinition, centeredCache},
                 std::pair{translatedDefinition, translatedCache}}) {
            const auto validation = LC_OffsetWorkerKernel::run({token,
                req::ValidateSourceCache{cache, definition, {}}});
            const auto& validated = native(validation, 8);
            if (centeredOutput.ordinary.size() == 1) {
                CHECK(validated.refusal == LC_OffsetCurveRefusal::None);
                CHECK(validated.changed);
            } else {
                CHECK(validated.refusal == LC_OffsetCurveRefusal::NotCacheRepresentable);
                CHECK_FALSE(validated.changed);
            }
        }
    }
}

TEST_CASE("bounded hyperbola cache promotion preserves bounds and branch across offset sides",
          "[lc-offsetcurve][worker-kernel][source-cache][hyperbola]") {
    const std::array<LC_HyperbolaData, 8> variants{
        LC_HyperbolaData{{0, 0}, {3, 0}, .6, -1.2, 1.5, false},
        LC_HyperbolaData{{0, 0}, {3, 0}, .6, 1.5, -1.2, false},
        LC_HyperbolaData{{0, 0}, {3, 0}, .6, -.4, .4, true},
        LC_HyperbolaData{{0, 0}, {3, 0}, .6, -1.2, 1.5, true},
        LC_HyperbolaData{{0, 0}, {3, 0}, .6, .2, 1.2, false},
        LC_HyperbolaData{{0, 0}, {3, 0}, .6, -1.2, -.2, false},
        LC_HyperbolaData{{0, 0}, {3, 0}, .6, -3.0, 3.0, false},
        LC_HyperbolaData{{0, 0}, {3, 0}, .6, -3.0, 3.0, true}
    };
    std::size_t acceptedCaches = 0;
    std::size_t refusedCaches = 0;
    for (const auto& data : variants) {
        CAPTURE(data.angle1, data.angle2, data.reversed);
        for (const double signedDistance :
             {-.1, .1, -.3, .3, -.5, .5, -1.0, 1.0, -1.05, 1.05,
              -1.08, 1.08, -1.09, 1.09, -1.1, 1.1}) {
            CAPTURE(signedDistance);
            const LC_OffsetSourceDefinition definition{
                LC_OffsetSourceGeometry{data}, signedDistance, 1e-4, false};
            const req::Request generationRequest{token, req::GenerateSource{definition, {}}};
            const auto generationReply = LC_OffsetWorkerKernel::run(generationRequest);
            const auto& generated = native(generationReply, 7);
            REQUIRE(generated.engineStatus == LC_CurveOffsetStatus::Ok);
            REQUIRE(generated.ordinaryStatus == result::OrdinaryStatus::Complete);
            REQUIRE_FALSE(generated.ordinary.empty());
            REQUIRE(std::holds_alternative<RS_SplineData>(generated.ordinary.front()));
            const auto& cache = std::get<RS_SplineData>(generated.ordinary.front());
            const req::Request validationRequest{token,
                req::ValidateSourceCache{cache, definition, {}}};
            const auto validationReply = LC_OffsetWorkerKernel::run(validationRequest);
            const auto& validated = native(validationReply, 8);
            if (generated.ordinary.size() == 1) {
                CHECK(validated.refusal == LC_OffsetCurveRefusal::None);
                CHECK(validated.changed);
                ++acceptedCaches;
            } else {
                CHECK(validated.refusal == LC_OffsetCurveRefusal::NotCacheRepresentable);
                CHECK_FALSE(validated.changed);
                ++refusedCaches;
            }
            CHECK(validated.engineRuns == 1);
        }
    }
    CHECK(acceptedCaches + refusedCaches == variants.size() * 16);
    CHECK(refusedCaches > 0);
}

TEST_CASE("bounded hyperbola worker cache validation scales across curvature neighborhoods",
          "[lc-offsetcurve][worker-kernel][source-cache][hyperbola][scale]") {
    const std::array<LC_HyperbolaData, 8> variants{
        LC_HyperbolaData{{0, 0}, {3, 0}, .6, -1.2, 1.5, false}, LC_HyperbolaData{{0, 0}, {3, 0}, .6, 1.5, -1.2, false},
        LC_HyperbolaData{{0, 0}, {3, 0}, .6, -.4, .4, true},    LC_HyperbolaData{{0, 0}, {3, 0}, .6, -1.2, 1.5, true},
        LC_HyperbolaData{{0, 0}, {3, 0}, .6, .2, 1.2, false},   LC_HyperbolaData{{0, 0}, {3, 0}, .6, -1.2, -.2, false},
        LC_HyperbolaData{{0, 0}, {3, 0}, .6, -3.0, 3.0, false}, LC_HyperbolaData{{0, 0}, {3, 0}, .6, -3.0, 3.0, true}};
    constexpr std::array<double, 4> scales{1e-3, 1.0, 1e3, 1e12};
    constexpr std::array<double, 16> normalizedDistances{-.1,   .1,   -.3,   .3,   -.5,   .5,   -1.0, 1.0,
                                                         -1.05, 1.05, -1.08, 1.08, -1.09, 1.09, -1.1, 1.1};
    std::size_t acceptedCaches = 0;
    std::size_t refusedCaches = 0;
    std::array<std::array<double, normalizedDistances.size()>, variants.size()> unitScaleTolerances{};

    for (const double scale : scales) {
        CAPTURE(scale);
        for (std::size_t variantIndex = 0; variantIndex < variants.size(); ++variantIndex) {
            const auto& base = variants[variantIndex];
            const LC_HyperbolaData data{{base.center.x * scale, base.center.y * scale, base.center.z},
                                        {base.majorP.x * scale, base.majorP.y * scale, base.majorP.z},
                                        base.ratio,
                                        base.angle1,
                                        base.angle2,
                                        base.reversed};
            const LC_OffsetSourceGeometry source{data};
            REQUIRE(source.isWellFormed());
            auto sourceEntity = source.makeEntity();
            REQUIRE(sourceEntity);

            for (std::size_t distanceIndex = 0; distanceIndex < normalizedDistances.size(); ++distanceIndex) {
                const double normalizedDistance = normalizedDistances[distanceIndex];
                CAPTURE(data.angle1, data.angle2, data.reversed, normalizedDistance);
                const double signedDistance = normalizedDistance * scale;
                const LC_OffsetSourceDefinition definition{source, signedDistance, 1e-4, false};
                const auto options = LC_OffsetWorkerTolerance::directOptions(definition, *sourceEntity);
                REQUIRE(options);
                const double normalizedTolerance = options->tolerance.requestedGeometry / scale;
                if (scale == 1.0) {
                    unitScaleTolerances[variantIndex][distanceIndex] = normalizedTolerance;
                }
                else if (scale > 1.0) {
                    const double reference = unitScaleTolerances[variantIndex][distanceIndex];
                    REQUIRE(reference > 0.0);
                    CHECK(std::abs(normalizedTolerance / reference - 1.0) < 1e-10);
                }

                const auto generatedReply = LC_OffsetWorkerKernel::run({token, req::GenerateSource{definition, {}}});
                const auto& generated = native(generatedReply, 7);
                REQUIRE(generated.engineStatus == LC_CurveOffsetStatus::Ok);
                REQUIRE(generated.ordinaryStatus == result::OrdinaryStatus::Complete);
                REQUIRE_FALSE(generated.ordinary.empty());
                REQUIRE(std::holds_alternative<RS_SplineData>(generated.ordinary.front()));
                const auto& cache = std::get<RS_SplineData>(generated.ordinary.front());
                const auto cacheFingerprint = RS_Spline::geometryFingerprint(cache);

                const auto validationReply = LC_OffsetWorkerKernel::run({token, req::ValidateSourceCache{cache, definition, {}}});
                const auto& validated = native(validationReply, 8);
                if (generated.ordinary.size() == 1) {
                    CHECK(validated.refusal == LC_OffsetCurveRefusal::None);
                    CHECK(validated.changed);
                    ++acceptedCaches;
                }
                else {
                    CHECK(validated.refusal == LC_OffsetCurveRefusal::NotCacheRepresentable);
                    CHECK_FALSE(validated.changed);
                    ++refusedCaches;
                }
                CHECK(RS_Spline::geometryFingerprint(cache) == cacheFingerprint);
                CHECK(validated.engineRuns == 1);
            }
        }
    }
    CHECK(acceptedCaches + refusedCaches == scales.size() * variants.size() * normalizedDistances.size());
    CHECK(acceptedCaches > 0);
    CHECK(refusedCaches > 0);
}

TEST_CASE("bounded hyperbola cache validation preserves translated local detail",
          "[lc-offsetcurve][worker-kernel][source-cache][hyperbola][precision]") {
    const RS_Vector translation{1e9, -1e9};
    const std::array<LC_HyperbolaData, 8> variants{
        LC_HyperbolaData{{0, 0}, {3, 0}, .6, -1.2, 1.5, false}, LC_HyperbolaData{{0, 0}, {3, 0}, .6, 1.5, -1.2, false},
        LC_HyperbolaData{{0, 0}, {3, 0}, .6, -.4, .4, true},    LC_HyperbolaData{{0, 0}, {3, 0}, .6, -1.2, 1.5, true},
        LC_HyperbolaData{{0, 0}, {3, 0}, .6, .2, 1.2, false},   LC_HyperbolaData{{0, 0}, {3, 0}, .6, -1.2, -.2, false},
        LC_HyperbolaData{{0, 0}, {3, 0}, .6, -3.0, 3.0, false}, LC_HyperbolaData{{0, 0}, {3, 0}, .6, -3.0, 3.0, true}};
    const auto firstCache = [](const result::Native& output) {
        if (const auto* spline = std::get_if<RS_SplineData>(&output.ordinary.front()))
            return *spline;
        const auto& segment = std::get<req::Segment>(output.ordinary.front());
        RS_SplineData cache{1, false};
        cache.controlPoints = {segment.start, segment.end};
        cache.knotslist = {0, 0, 1, 1};
        cache.weights = {1, 1};
        return cache;
    };

    for (std::size_t variantIndex = 0; variantIndex < variants.size(); ++variantIndex) {
        const auto& base = variants[variantIndex];
        const LC_OffsetSourceGeometry centered{base};
        const LC_OffsetSourceGeometry translated{
            LC_HyperbolaData{{base.center.x + translation.x, base.center.y + translation.y, base.center.z},
                             base.majorP,
                             base.ratio,
                             base.angle1,
                             base.angle2,
                             base.reversed}};
        REQUIRE(centered.isWellFormed());
        REQUIRE(translated.isWellFormed());

        for (const double signedDistance : {-1.05, 1.05, -1.08, 1.08, -1.09, 1.09}) {
            CAPTURE(variantIndex, signedDistance);
            const LC_OffsetSourceDefinition centeredDefinition{centered, signedDistance, 1e-4, false};
            const LC_OffsetSourceDefinition translatedDefinition{translated, signedDistance, 1e-4, false};
            const auto centeredReply = LC_OffsetWorkerKernel::run({token, req::GenerateSource{centeredDefinition, {}}});
            const auto translatedReply = LC_OffsetWorkerKernel::run({token, req::GenerateSource{translatedDefinition, {}}});
            const auto& centeredOutput = native(centeredReply, 7);
            const auto& translatedOutput = native(translatedReply, 7);
            REQUIRE(centeredOutput.engineStatus == LC_CurveOffsetStatus::Ok);
            REQUIRE(translatedOutput.engineStatus == LC_CurveOffsetStatus::Ok);
            REQUIRE(centeredOutput.ordinaryStatus == result::OrdinaryStatus::Complete);
            REQUIRE(translatedOutput.ordinaryStatus == result::OrdinaryStatus::Complete);
            REQUIRE_FALSE(centeredOutput.ordinary.empty());
            REQUIRE_FALSE(translatedOutput.ordinary.empty());
            REQUIRE(centeredOutput.ordinary.size() == translatedOutput.ordinary.size());

            const RS_SplineData centeredCache = firstCache(centeredOutput);
            const RS_SplineData translatedCache = firstCache(translatedOutput);
            const RS_Spline centeredSpline{nullptr, centeredCache};
            const RS_Spline translatedSpline{nullptr, translatedCache};
            const auto centeredEntity = centered.makeEntity();
            const auto translatedEntity = translated.makeEntity();
            REQUIRE(centeredEntity);
            REQUIRE(translatedEntity);
            const auto centeredOptions = LC_OffsetWorkerTolerance::directOptions(centeredDefinition, *centeredEntity);
            const auto translatedOptions = LC_OffsetWorkerTolerance::directOptions(translatedDefinition, *translatedEntity);
            REQUIRE(centeredOptions);
            REQUIRE(translatedOptions);
            const double roundingAllowance =
                64.0 * std::numeric_limits<double>::epsilon() * std::max(std::abs(translation.x), std::abs(translation.y));
            const double deviationLimit =
                4.0 * std::max(centeredOptions->tolerance.requestedGeometry, translatedOptions->tolerance.requestedGeometry) +
                roundingAllowance;
            double maximumLocalDeviation = 0.0;
            for (int sample = 0; sample <= 128; ++sample) {
                const double parameter = static_cast<double>(sample) / 128.0;
                const RS_Vector local = centeredSpline.getPointAt(parameter);
                const RS_Vector world = translatedSpline.getPointAt(parameter);
                REQUIRE(local.valid);
                REQUIRE(world.valid);
                maximumLocalDeviation =
                    std::max(maximumLocalDeviation, std::hypot((world.x - translation.x) - local.x, (world.y - translation.y) - local.y));
            }
            CHECK(maximumLocalDeviation <= deviationLimit);

            for (const auto& [definition, cache, expectedOutput] : {std::tuple{centeredDefinition, centeredCache, &centeredOutput},
                                                                    std::tuple{translatedDefinition, translatedCache, &translatedOutput}}) {
                const auto validation = LC_OffsetWorkerKernel::run({token, req::ValidateSourceCache{cache, definition, {}}});
                const auto& validated = native(validation, 8);
                if (expectedOutput->ordinary.size() == 1) {
                    CHECK(validated.refusal == LC_OffsetCurveRefusal::None);
                    CHECK(validated.changed);
                }
                else {
                    CHECK(validated.refusal == LC_OffsetCurveRefusal::NotCacheRepresentable);
                    CHECK_FALSE(validated.changed);
                }
                CHECK(validated.engineRuns == 1);
            }
        }
    }
}

TEST_CASE("bounded hyperbola worker preserves detail across combined scale and translation",
          "[lc-offsetcurve][worker-kernel][source-cache][hyperbola][precision][combined]") {
    const std::array<LC_HyperbolaData, 8> variants{
        LC_HyperbolaData{{0, 0}, {3, 0}, .6, -1.2, 1.5, false}, LC_HyperbolaData{{0, 0}, {3, 0}, .6, 1.5, -1.2, false},
        LC_HyperbolaData{{0, 0}, {3, 0}, .6, -.4, .4, true},    LC_HyperbolaData{{0, 0}, {3, 0}, .6, -1.2, 1.5, true},
        LC_HyperbolaData{{0, 0}, {3, 0}, .6, .2, 1.2, false},   LC_HyperbolaData{{0, 0}, {3, 0}, .6, -1.2, -.2, false},
        LC_HyperbolaData{{0, 0}, {3, 0}, .6, -3.0, 3.0, false}, LC_HyperbolaData{{0, 0}, {3, 0}, .6, -3.0, 3.0, true}};
    constexpr std::array<double, 4> scales{1e-3, 1.0, 1e3, 1e12};
    constexpr std::array<double, 6> normalizedDistances{-1.05, 1.05, -1.08, 1.08, -1.09, 1.09};
    const auto firstCache = [](const result::Native& output) {
        if (const auto* spline = std::get_if<RS_SplineData>(&output.ordinary.front()))
            return *spline;
        const auto& segment = std::get<req::Segment>(output.ordinary.front());
        RS_SplineData cache{1, false};
        cache.controlPoints = {segment.start, segment.end};
        cache.knotslist = {0, 0, 1, 1};
        cache.weights = {1, 1};
        return cache;
    };

    std::size_t acceptedCaches = 0;
    std::size_t refusedCaches = 0;
    for (const double scale : scales) {
        CAPTURE(scale);
        const RS_Vector translation{1e9 * scale, -1e9 * scale};
        std::size_t acceptedAtScale = 0;
        std::size_t refusedAtScale = 0;
        for (std::size_t variantIndex = 0; variantIndex < variants.size(); ++variantIndex) {
            const auto& base = variants[variantIndex];
            const LC_HyperbolaData centeredData{{base.center.x * scale, base.center.y * scale, base.center.z},
                                                {base.majorP.x * scale, base.majorP.y * scale, base.majorP.z},
                                                base.ratio,
                                                base.angle1,
                                                base.angle2,
                                                base.reversed};
            const LC_HyperbolaData translatedData{
                {centeredData.center.x + translation.x, centeredData.center.y + translation.y, centeredData.center.z},
                centeredData.majorP,
                centeredData.ratio,
                centeredData.angle1,
                centeredData.angle2,
                centeredData.reversed};
            const LC_OffsetSourceGeometry centered{centeredData};
            const LC_OffsetSourceGeometry translated{translatedData};
            REQUIRE(centered.isWellFormed());
            REQUIRE(translated.isWellFormed());
            const auto centeredEntity = centered.makeEntity();
            const auto translatedEntity = translated.makeEntity();
            REQUIRE(centeredEntity);
            REQUIRE(translatedEntity);

            for (const double normalizedDistance : normalizedDistances) {
                CAPTURE(variantIndex, normalizedDistance);
                const double signedDistance = normalizedDistance * scale;
                const LC_OffsetSourceDefinition centeredDefinition{centered, signedDistance, 1e-4, false};
                const LC_OffsetSourceDefinition translatedDefinition{translated, signedDistance, 1e-4, false};
                const auto centeredOptions = LC_OffsetWorkerTolerance::directOptions(centeredDefinition, *centeredEntity);
                const auto translatedOptions = LC_OffsetWorkerTolerance::directOptions(translatedDefinition, *translatedEntity);
                REQUIRE(centeredOptions);
                REQUIRE(translatedOptions);
                const double toleranceRoundingAllowance = 64.0 * std::numeric_limits<double>::epsilon() *
                    std::max(std::abs(translation.x), std::abs(translation.y)) * centeredDefinition.relativeTolerance;
                CHECK(std::abs(centeredOptions->tolerance.requestedGeometry - translatedOptions->tolerance.requestedGeometry) <=
                      toleranceRoundingAllowance);

                const auto centeredReply = LC_OffsetWorkerKernel::run({token, req::GenerateSource{centeredDefinition, {}}});
                const auto translatedReply = LC_OffsetWorkerKernel::run({token, req::GenerateSource{translatedDefinition, {}}});
                const auto& centeredOutput = native(centeredReply, 7);
                const auto& translatedOutput = native(translatedReply, 7);
                REQUIRE(centeredOutput.engineStatus == LC_CurveOffsetStatus::Ok);
                REQUIRE(translatedOutput.engineStatus == LC_CurveOffsetStatus::Ok);
                REQUIRE(centeredOutput.ordinaryStatus == result::OrdinaryStatus::Complete);
                REQUIRE(translatedOutput.ordinaryStatus == result::OrdinaryStatus::Complete);
                REQUIRE_FALSE(centeredOutput.ordinary.empty());
                REQUIRE_FALSE(translatedOutput.ordinary.empty());
                REQUIRE(centeredOutput.ordinary.size() == translatedOutput.ordinary.size());

                const RS_SplineData centeredCache = firstCache(centeredOutput);
                const RS_SplineData translatedCache = firstCache(translatedOutput);
                const RS_Spline centeredSpline{nullptr, centeredCache};
                const RS_Spline translatedSpline{nullptr, translatedCache};
                const double roundingAllowance =
                    64.0 * std::numeric_limits<double>::epsilon() * std::max(std::abs(translation.x), std::abs(translation.y));
                const double deviationLimit =
                    4.0 * std::max(centeredOptions->tolerance.requestedGeometry, translatedOptions->tolerance.requestedGeometry) +
                    roundingAllowance;
                double maximumLocalDeviation = 0.0;
                for (int sample = 0; sample <= 128; ++sample) {
                    const double parameter = static_cast<double>(sample) / 128.0;
                    const RS_Vector local = centeredSpline.getPointAt(parameter);
                    const RS_Vector world = translatedSpline.getPointAt(parameter);
                    REQUIRE(local.valid);
                    REQUIRE(world.valid);
                    maximumLocalDeviation = std::max(maximumLocalDeviation,
                                                     std::hypot((world.x - translation.x) - local.x, (world.y - translation.y) - local.y));
                }
                CHECK(maximumLocalDeviation <= deviationLimit);

                for (const auto& [definition, cache, expectedOutput] :
                     {std::tuple{centeredDefinition, centeredCache, &centeredOutput},
                      std::tuple{translatedDefinition, translatedCache, &translatedOutput}}) {
                    const auto cacheFingerprint = RS_Spline::geometryFingerprint(cache);
                    const auto validation = LC_OffsetWorkerKernel::run({token, req::ValidateSourceCache{cache, definition, {}}});
                    const auto& validated = native(validation, 8);
                    if (expectedOutput->ordinary.size() == 1) {
                        CHECK(validated.refusal == LC_OffsetCurveRefusal::None);
                        CHECK(validated.changed);
                        ++acceptedAtScale;
                        ++acceptedCaches;
                    }
                    else {
                        CHECK(validated.refusal == LC_OffsetCurveRefusal::NotCacheRepresentable);
                        CHECK_FALSE(validated.changed);
                        CHECK(RS_Spline::geometryFingerprint(cache) == cacheFingerprint);
                        ++refusedAtScale;
                        ++refusedCaches;
                    }
                    CHECK(validated.engineRuns == 1);
                }
            }
        }
        CHECK(acceptedAtScale > 0);
        CHECK(refusedAtScale > 0);
    }
    CHECK(acceptedCaches + refusedCaches == scales.size() * variants.size() * normalizedDistances.size() * 2);
}

TEST_CASE("worker generates and caches high-ratio rational spline offsets when tangents stay regular",
          "[lc-offsetcurve][worker-kernel][source-cache][spline-variant][rational][conditioned]") {
    const std::array<std::vector<double>, 2> weightProfiles{{
        {1e-6, 3.162277660168379e-5, 1e-3, 0.03162277660168379, 1.0,
         31.622776601683793, 1e3, 31622.776601683792, 1e6},
        {1e6, 31622.776601683792, 1e3, 31.622776601683793, 1.0,
         0.03162277660168379, 1e-3, 3.162277660168379e-5, 1e-6}
    }};
    for (const auto& weights : weightProfiles) {
        RS_SplineData source{2, false};
        source.controlPoints = {{0, 0}, {10, 5}, {20, 12}, {30, 20},
                                {40, 28}, {50, 35}, {60, 40}, {70, 42},
                                {80, 40}};
        source.knotslist = {0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 7, 7};
        source.weights = weights;
        RS_Spline checkedSource{nullptr, source};
        REQUIRE(checkedSource.validate());
        for (const double signedDistance : {-.005, .005}) {
            CAPTURE(weights.front(), weights.back(), signedDistance);
            const LC_OffsetSourceDefinition definition{
                LC_OffsetSourceGeometry{source}, signedDistance, 1e-4, false};
            const req::Request generationRequest{token,
                req::GenerateSource{definition, {}}};
            const auto generationReply = LC_OffsetWorkerKernel::run(generationRequest);
            const auto& generated = native(generationReply, 7);
            CAPTURE(generated.engineStatus.has_value(),
                    generated.engineStatus
                        ? static_cast<int>(*generated.engineStatus)
                        : -1);
            REQUIRE(generated.engineStatus == LC_CurveOffsetStatus::Ok);
            REQUIRE(generated.ordinaryStatus == result::OrdinaryStatus::Complete);
            REQUIRE(generated.ordinary.size() == 1);
            REQUIRE(std::holds_alternative<RS_SplineData>(generated.ordinary.front()));
            const auto generationWire = result::encode(generationReply, generationRequest);
            REQUIRE(generationWire);
            CHECK(result::decode(*generationWire, generationRequest));

            const auto& cache = std::get<RS_SplineData>(generated.ordinary.front());
            const req::Request validationRequest{token,
                req::ValidateSourceCache{cache, definition, {}}};
            const auto validationReply = LC_OffsetWorkerKernel::run(validationRequest);
            const auto& validated = native(validationReply, 8);
            CHECK(validated.refusal == LC_OffsetCurveRefusal::None);
            CHECK(validated.changed);
            CHECK(validated.engineStatus == LC_CurveOffsetStatus::Ok);
            const auto validationWire = result::encode(validationReply, validationRequest);
            REQUIRE(validationWire);
            CHECK(result::decode(*validationWire, validationRequest));
        }
    }
}

TEST_CASE("bounded hyperbola worker classifies relative translation boundaries",
          "[lc-offsetcurve][worker-kernel][source-cache][hyperbola][precision][translation-boundary]") {
    const std::array<LC_HyperbolaData, 8> variants{
        LC_HyperbolaData{{0, 0}, {3, 0}, .6, -1.2, 1.5, false}, LC_HyperbolaData{{0, 0}, {3, 0}, .6, 1.5, -1.2, false},
        LC_HyperbolaData{{0, 0}, {3, 0}, .6, -.4, .4, true},    LC_HyperbolaData{{0, 0}, {3, 0}, .6, -1.2, 1.5, true},
        LC_HyperbolaData{{0, 0}, {3, 0}, .6, .2, 1.2, false},   LC_HyperbolaData{{0, 0}, {3, 0}, .6, -1.2, -.2, false},
        LC_HyperbolaData{{0, 0}, {3, 0}, .6, -3.0, 3.0, false}, LC_HyperbolaData{{0, 0}, {3, 0}, .6, -3.0, 3.0, true}};
    constexpr std::array<double, 3> scales{1e-3, 1.0, 1e12};
    constexpr std::array<double, 3> translationRatios{1e6, 1e9, 1e12};
    constexpr std::array<double, 6> normalizedDistances{-1.05, 1.05, -1.08, 1.08, -1.09, 1.09};
    const auto firstCache = [](const result::Native& output) {
        if (const auto* spline = std::get_if<RS_SplineData>(&output.ordinary.front()))
            return *spline;
        const auto& segment = std::get<req::Segment>(output.ordinary.front());
        RS_SplineData cache{1, false};
        cache.controlPoints = {segment.start, segment.end};
        cache.knotslist = {0, 0, 1, 1};
        cache.weights = {1, 1};
        return cache;
    };

    std::size_t completedTranslated = 0;
    std::size_t refusedTranslated = 0;
    for (const double scale : scales) {
        CAPTURE(scale);
        for (const double translationRatio : translationRatios) {
            CAPTURE(translationRatio);
            const RS_Vector translation{translationRatio * scale, -translationRatio * scale};
            for (std::size_t variantIndex = 0; variantIndex < variants.size(); ++variantIndex) {
                const auto& base = variants[variantIndex];
                const LC_HyperbolaData centeredData{{base.center.x * scale, base.center.y * scale, base.center.z},
                                                    {base.majorP.x * scale, base.majorP.y * scale, base.majorP.z},
                                                    base.ratio,
                                                    base.angle1,
                                                    base.angle2,
                                                    base.reversed};
                const LC_HyperbolaData translatedData{
                    {centeredData.center.x + translation.x, centeredData.center.y + translation.y, centeredData.center.z},
                    centeredData.majorP,
                    centeredData.ratio,
                    centeredData.angle1,
                    centeredData.angle2,
                    centeredData.reversed};
                const LC_OffsetSourceGeometry centered{centeredData};
                const LC_OffsetSourceGeometry translated{translatedData};
                REQUIRE(centered.isWellFormed());
                REQUIRE(translated.isWellFormed());
                for (const double normalizedDistance : normalizedDistances) {
                    CAPTURE(variantIndex, normalizedDistance);
                    const double signedDistance = normalizedDistance * scale;
                    const LC_OffsetSourceDefinition centeredDefinition{centered, signedDistance, 1e-4, false};
                    const LC_OffsetSourceDefinition translatedDefinition{translated, signedDistance, 1e-4, false};
                    const auto centeredReply = LC_OffsetWorkerKernel::run({token, req::GenerateSource{centeredDefinition, {}}});
                    const auto translatedReply = LC_OffsetWorkerKernel::run({token, req::GenerateSource{translatedDefinition, {}}});
                    const auto& centeredOutput = native(centeredReply, 7);
                    const auto& translatedOutput = native(translatedReply, 7);
                    REQUIRE(centeredOutput.engineStatus == LC_CurveOffsetStatus::Ok);
                    REQUIRE(centeredOutput.ordinaryStatus == result::OrdinaryStatus::Complete);
                    REQUIRE_FALSE(centeredOutput.ordinary.empty());

                    if (translatedOutput.engineStatus != LC_CurveOffsetStatus::Ok ||
                        translatedOutput.ordinaryStatus != result::OrdinaryStatus::Complete || translatedOutput.ordinary.empty()) {
                        CHECK(translatedOutput.ordinary.empty());
                        CHECK_FALSE(translatedOutput.changed);
                        ++refusedTranslated;
                        continue;
                    }

                    ++completedTranslated;
                    CHECK(centeredOutput.ordinary.size() == translatedOutput.ordinary.size());
                    const RS_SplineData centeredCache = firstCache(centeredOutput);
                    const RS_SplineData translatedCache = firstCache(translatedOutput);
                    const RS_Spline centeredSpline{nullptr, centeredCache};
                    const RS_Spline translatedSpline{nullptr, translatedCache};
                    const double roundingAllowance =
                        64.0 * std::numeric_limits<double>::epsilon() * std::max(std::abs(translation.x), std::abs(translation.y));
                    const auto centeredEntity = centered.makeEntity();
                    const auto translatedEntity = translated.makeEntity();
                    REQUIRE(centeredEntity);
                    REQUIRE(translatedEntity);
                    const LC_OffsetSourceDefinition centeredWithDistance{centered, signedDistance, 1e-4, false};
                    const LC_OffsetSourceDefinition translatedWithDistance{translated, signedDistance, 1e-4, false};
                    const auto centeredOptions = LC_OffsetWorkerTolerance::directOptions(centeredWithDistance, *centeredEntity);
                    const auto translatedOptions = LC_OffsetWorkerTolerance::directOptions(translatedWithDistance, *translatedEntity);
                    REQUIRE(centeredOptions);
                    REQUIRE(translatedOptions);
                    const double deviationLimit =
                        4.0 * std::max(centeredOptions->tolerance.requestedGeometry, translatedOptions->tolerance.requestedGeometry) +
                        roundingAllowance;
                    double maximumLocalDeviation = 0.0;
                    for (int sample = 0; sample <= 128; ++sample) {
                        const double parameter = static_cast<double>(sample) / 128.0;
                        const RS_Vector local = centeredSpline.getPointAt(parameter);
                        const RS_Vector world = translatedSpline.getPointAt(parameter);
                        REQUIRE(local.valid);
                        REQUIRE(world.valid);
                        maximumLocalDeviation = std::max(
                            maximumLocalDeviation, std::hypot((world.x - translation.x) - local.x, (world.y - translation.y) - local.y));
                    }
                    CHECK(maximumLocalDeviation <= deviationLimit);

                    for (const auto& [definition, cache, expectedOutput] :
                         {std::tuple{centeredDefinition, centeredCache, &centeredOutput},
                          std::tuple{translatedDefinition, translatedCache, &translatedOutput}}) {
                        const auto fingerprint = RS_Spline::geometryFingerprint(cache);
                        const auto validation = LC_OffsetWorkerKernel::run({token, req::ValidateSourceCache{cache, definition, {}}});
                        const auto& validated = native(validation, 8);
                        if (expectedOutput->ordinary.size() == 1) {
                            CHECK(validated.refusal == LC_OffsetCurveRefusal::None);
                            CHECK(validated.changed);
                        }
                        else {
                            CHECK(validated.refusal == LC_OffsetCurveRefusal::NotCacheRepresentable);
                            CHECK_FALSE(validated.changed);
                            CHECK(RS_Spline::geometryFingerprint(cache) == fingerprint);
                        }
                        CHECK(validated.engineRuns == 1);
                    }
                }
            }
        }
    }
    CHECK(completedTranslated > 0);
    CHECK(refusedTranslated > 0);
}

TEST_CASE("ellipse worker preserves detail across combined scale and translation",
          "[lc-offsetcurve][worker-kernel][source-cache][ellipse][precision][combined]") {
    constexpr double tau = 2.0 * M_PI;
    const std::array<RS_EllipseData, 25> variants{
        RS_EllipseData{{2, -3}, {4, 0}, .6, .2, 2.1, false},
        RS_EllipseData{{-1, 2}, {3, 1}, .8, 1.8, .1, true},
        RS_EllipseData{{3, 1}, {2, -1}, 1.4, .3, 2.3, false},
        RS_EllipseData{{0, 0}, {2, 0}, .2, 0, tau, false},
        RS_EllipseData{{0, 0}, {2, 0}, .2, 0, tau, true},
        RS_EllipseData{{0, 0}, {std::sqrt(3.0), 1}, .2, 0, tau, false},
        RS_EllipseData{{0, 0}, {std::sqrt(3.0), 1}, .2, 0, tau, true},
        RS_EllipseData{{0, 0}, {1, std::sqrt(3.0)}, .2, 0, tau, false},
        RS_EllipseData{{0, 0}, {1, std::sqrt(3.0)}, .2, 0, tau, true},
        RS_EllipseData{{0, 0}, {2, 0}, .2, .2, M_PI - .2, false},
        RS_EllipseData{{0, 0}, {2, 0}, .2, .2, M_PI - .2, true},
        RS_EllipseData{{0, 0}, {2, 0}, .2, .2, M_PI + .2, false},
        RS_EllipseData{{0, 0}, {2, 0}, .2, .2, M_PI + .2, true},
        RS_EllipseData{{0, 0}, {2, 0}, .2, tau - .2, .4, false},
        RS_EllipseData{{0, 0}, {2, 0}, .2, tau - .2, .4, true},
        RS_EllipseData{{0, 0}, {2, 0}, .2, tau - .2, M_PI + .2, false},
        RS_EllipseData{{0, 0}, {2, 0}, .2, tau - .2, M_PI + .2, true},
        RS_EllipseData{{0, 0}, {std::sqrt(3.0), 1}, .2, .2, M_PI - .2, false},
        RS_EllipseData{{0, 0}, {std::sqrt(3.0), 1}, .2, .2, M_PI - .2, true},
        RS_EllipseData{{0, 0}, {std::sqrt(3.0), 1}, .2, .2, M_PI + .2, false},
        RS_EllipseData{{0, 0}, {std::sqrt(3.0), 1}, .2, .2, M_PI + .2, true},
        RS_EllipseData{{0, 0}, {1, std::sqrt(3.0)}, .2, tau - .2, .4, false},
        RS_EllipseData{{0, 0}, {1, std::sqrt(3.0)}, .2, tau - .2, .4, true},
        RS_EllipseData{{0, 0}, {1, std::sqrt(3.0)}, .2, tau - .2, M_PI + .2, false},
        RS_EllipseData{{0, 0}, {1, std::sqrt(3.0)}, .2, tau - .2, M_PI + .2, true}};
    constexpr std::array<double, 3> scales{1e-3, 1.0, 1e12};
    constexpr std::array<double, 3> translationRatios{1e6, 1e9, 1e12};
    constexpr std::array<double, 4> ordinaryNormalizedDistances{-.4, -.1, .1, .4};
    constexpr std::array<double, 8> thinNormalizedDistances{-.09, -.08, -.07, -.05, .05, .07, .08, .09};
    constexpr std::array<double, 6> partialThinNormalizedDistances{-.09, -.08, -.07, .07, .08, .09};
    const auto asSpline = [](const req::Cutter& item) {
        if (const auto* spline = std::get_if<RS_SplineData>(&item)) return *spline;
        const auto& segment = std::get<req::Segment>(item);
        RS_SplineData cache{1, false};
        cache.controlPoints = {segment.start, segment.end};
        cache.knotslist = {0, 0, 1, 1};
        cache.weights = {1, 1};
        return cache;
    };

    std::size_t completedPairs = 0;
    std::size_t generationOrToleranceRefusals = 0;
    std::size_t acceptedCaches = 0;
    for (const double scale : scales) {
        CAPTURE(scale);
        for (const double translationRatio : translationRatios) {
          CAPTURE(translationRatio);
          const RS_Vector translation{translationRatio * scale, -translationRatio * scale};
          for (std::size_t variantIndex = 0; variantIndex < variants.size(); ++variantIndex) {
            CAPTURE(variantIndex);
            const auto& base = variants[variantIndex];
            const RS_EllipseData centeredData{{base.center.x * scale, base.center.y * scale, base.center.z},
                                              {base.majorP.x * scale, base.majorP.y * scale, base.majorP.z},
                                              base.ratio, base.angle1, base.angle2, base.reversed};
            const RS_EllipseData translatedData{{centeredData.center.x + translation.x,
                                                 centeredData.center.y + translation.y, centeredData.center.z},
                                                centeredData.majorP, centeredData.ratio, centeredData.angle1,
                                                centeredData.angle2, centeredData.reversed};
            const LC_OffsetSourceGeometry centered{centeredData};
            const LC_OffsetSourceGeometry translated{translatedData};
            REQUIRE(centered.isWellFormed());
            REQUIRE(translated.isWellFormed());
            const auto centeredEntity = centered.makeEntity();
            const auto translatedEntity = translated.makeEntity();
            REQUIRE(centeredEntity);
            REQUIRE(translatedEntity);
            const bool thin = base.ratio == .2;
            const bool thinFull = thin && base.angle1 == 0.0 &&
                (base.angle2 == tau || base.angle2 == 0.0);
            std::vector<double> distances;
            if (thinFull) {
                distances.assign(thinNormalizedDistances.begin(), thinNormalizedDistances.end());
            } else if (thin) {
                distances.assign(partialThinNormalizedDistances.begin(), partialThinNormalizedDistances.end());
            } else {
                distances.assign(ordinaryNormalizedDistances.begin(), ordinaryNormalizedDistances.end());
            }

            for (const double normalizedDistance : distances) {
                CAPTURE(normalizedDistance);
                const double signedDistance = normalizedDistance * scale;
                const LC_OffsetSourceDefinition centeredDefinition{centered, signedDistance, 1e-4, false};
                const LC_OffsetSourceDefinition translatedDefinition{translated, signedDistance, 1e-4, false};
                const auto centeredOptions = LC_OffsetWorkerTolerance::directOptions(centeredDefinition, *centeredEntity);
                const auto translatedOptions = LC_OffsetWorkerTolerance::directOptions(translatedDefinition, *translatedEntity);

                const auto centeredReply = LC_OffsetWorkerKernel::run(
                    {token, req::GenerateSource{centeredDefinition, {}}});
                const auto& centeredOutput = native(centeredReply, 7);
                const auto translatedReply = LC_OffsetWorkerKernel::run(
                    {token, req::GenerateSource{translatedDefinition, {}}});
                const auto& translatedOutput = native(translatedReply, 7);
                const auto generated = [](const result::Native& output) {
                    return output.engineStatus == LC_CurveOffsetStatus::Ok &&
                           output.ordinaryStatus == result::OrdinaryStatus::Complete &&
                           !output.ordinary.empty();
                };
                const bool centeredGenerated = generated(centeredOutput);
                const bool translatedGenerated = generated(translatedOutput);
                const auto validateOutput = [&](const LC_OffsetSourceDefinition& definition,
                                                const auto& options,
                                                const result::Native& output) {
                    if (!generated(output)) return;
                    const auto cache = asSpline(output.ordinary.front());
                    const auto fingerprint = RS_Spline::geometryFingerprint(cache);
                    const auto validation = LC_OffsetWorkerKernel::run(
                        {token, req::ValidateSourceCache{cache, definition, {}}});
                    const auto& validated = native(validation, 8);
                    if (output.ordinary.size() == 1) {
                        CHECK(validated.refusal == LC_OffsetCurveRefusal::None);
                        CHECK(validated.changed);
                        ++acceptedCaches;
                    } else {
                        CHECK(validated.refusal != LC_OffsetCurveRefusal::None);
                        CHECK_FALSE(validated.changed);
                        CHECK(RS_Spline::geometryFingerprint(cache) == fingerprint);
                    }
                    CHECK(validated.engineRuns == 1);
                };
                validateOutput(centeredDefinition, centeredOptions, centeredOutput);
                validateOutput(translatedDefinition, translatedOptions, translatedOutput);
                if (!centeredGenerated || !translatedGenerated) {
                    if (!centeredGenerated) {
                        CHECK(centeredOutput.ordinary.empty());
                        CHECK_FALSE(centeredOutput.changed);
                    }
                    if (!translatedGenerated) {
                        CHECK(translatedOutput.ordinary.empty());
                        CHECK_FALSE(translatedOutput.changed);
                    }
                    ++generationOrToleranceRefusals;
                    continue;
                }

                if (!centeredOptions ||
                    centeredOutput.ordinary.size() != translatedOutput.ordinary.size()) {
                    ++generationOrToleranceRefusals;
                    continue;
                }
                ++completedPairs;

                if (centeredOutput.ordinary.size() == translatedOutput.ordinary.size()) {
                    const double roundingAllowance = 64.0 * std::numeric_limits<double>::epsilon() *
                        std::max(std::abs(translation.x), std::abs(translation.y));
                    const double translatedTolerance = translatedOptions
                        ? translatedOptions->tolerance.requestedGeometry
                        : centeredOptions->tolerance.requestedGeometry;
                    const double deviationLimit = 4.0 * std::max(centeredOptions->tolerance.requestedGeometry,
                                                                  translatedTolerance) + roundingAllowance;
                    std::vector<RS_Spline> centeredSplines;
                    std::vector<RS_Spline> translatedSplines;
                    centeredSplines.reserve(centeredOutput.ordinary.size());
                    translatedSplines.reserve(translatedOutput.ordinary.size());
                    for (std::size_t branch = 0; branch < centeredOutput.ordinary.size(); ++branch) {
                        centeredSplines.emplace_back(nullptr, asSpline(centeredOutput.ordinary[branch]));
                        translatedSplines.emplace_back(nullptr, asSpline(translatedOutput.ordinary[branch]));
                    }
                    const auto deviation = [&](const RS_Spline& localSpline, const RS_Spline& worldSpline,
                                               const bool reverse) {
                        double maximum = 0.0;
                        for (int sample = 0; sample <= 128; ++sample) {
                            const double parameter = static_cast<double>(sample) / 128.0;
                            const RS_Vector local = localSpline.getPointAt(parameter);
                            const RS_Vector world = worldSpline.getPointAt(reverse ? 1.0 - parameter : parameter);
                            REQUIRE(local.valid);
                            REQUIRE(world.valid);
                            maximum = std::max(maximum, std::hypot((world.x - translation.x) - local.x,
                                                                   (world.y - translation.y) - local.y));
                        }
                        return maximum;
                    };
                    std::vector<bool> matched(translatedSplines.size(), false);
                    double maximumMatchedDeviation = 0.0;
                    for (const RS_Spline& localSpline : centeredSplines) {
                        double bestDeviation = std::numeric_limits<double>::infinity();
                        std::size_t bestIndex = translatedSplines.size();
                        for (std::size_t candidate = 0; candidate < translatedSplines.size(); ++candidate) {
                            if (matched[candidate]) continue;
                            const double forward = deviation(localSpline, translatedSplines[candidate], false);
                            const double reverse = deviation(localSpline, translatedSplines[candidate], true);
                            const double candidateDeviation = std::min(forward, reverse);
                            if (candidateDeviation < bestDeviation) {
                                bestDeviation = candidateDeviation;
                                bestIndex = candidate;
                            }
                        }
                        REQUIRE(bestIndex < translatedSplines.size());
                        matched[bestIndex] = true;
                        maximumMatchedDeviation = std::max(maximumMatchedDeviation, bestDeviation);
                    }
                    CHECK(maximumMatchedDeviation <= deviationLimit);
                }
            }
          }
        }
    }
    CHECK(completedPairs + generationOrToleranceRefusals == scales.size() *
          translationRatios.size() *
          (3 * ordinaryNormalizedDistances.size() + 6 * thinNormalizedDistances.size() +
           16 * partialThinNormalizedDistances.size()));
    CHECK(completedPairs > 0);
    CHECK(acceptedCaches > 0);
}

TEST_CASE("spline worker preserves cache detail across scale and translation",
          "[lc-offsetcurve][worker-kernel][source-cache][spline-variant][precision][combined]") {
    RS_SplineData rational{2, false};
    rational.controlPoints = {{0, 0}, {1, 2}, {2, -1}, {3, 2}, {4, 0}};
    rational.knotslist = {0, 0, 0, .3, .7, 1, 1, 1};
    rational.weights = {1, .6, 1.8, .8, 1};
    RS_SplineData cubic{3, false};
    cubic.controlPoints = {{0, 0}, {1, 2}, {2, 2}, {3, -1}, {4, -1}, {5, 1}};
    cubic.knotslist = {0, 0, 0, 0, .4, .4, 1, 1, 1, 1};
    cubic.weights = {1, .7, 1.4, .9, 1.6, 1};
    LC_SplinePointsData fitPoints{false, false};
    fitPoints.splinePoints = {{0, 0}, {1, 2}, {2, 2}, {3, -1}, {4, 0}};
    fitPoints.fitTolerance = 1e-6;
    const std::array<LC_OffsetSourceGeometry, 3> variants{
        LC_OffsetSourceGeometry{rational}, LC_OffsetSourceGeometry{cubic},
        LC_OffsetSourceGeometry{fitPoints}};
    constexpr std::array<double, 2> scales{1.0, 1e3};
    constexpr std::array<double, 2> translationRatios{1e6, 1e9};
    constexpr std::array<double, 4> normalizedDistances{-.4, -.1, .1, .4};
    const auto transformPoint = [](RS_Vector& point, const double scale,
                                   const RS_Vector& translation) {
        point.x = point.x * scale + translation.x;
        point.y = point.y * scale + translation.y;
        point.z *= scale;
    };
    const auto transform = [&](LC_OffsetSourceGeometry source, const double scale,
                               const RS_Vector& translation) {
        std::visit([&](auto& data) {
            using T = std::decay_t<decltype(data)>;
            if constexpr (std::is_same_v<T, RS_SplineData>) {
                for (auto& point : data.controlPoints) transformPoint(point, scale, translation);
                for (auto& point : data.fitPoints) transformPoint(point, scale, translation);
            } else if constexpr (std::is_same_v<T, LC_SplinePointsData>) {
                for (auto& point : data.controlPoints) transformPoint(point, scale, translation);
                for (auto& point : data.splinePoints) transformPoint(point, scale, translation);
            }
        }, source.value);
        return source;
    };
    const auto asSpline = [](const req::Cutter& item) {
        if (const auto* spline = std::get_if<RS_SplineData>(&item)) return *spline;
        const auto& segment = std::get<req::Segment>(item);
        RS_SplineData cache{1, false};
        cache.controlPoints = {segment.start, segment.end};
        cache.knotslist = {0, 0, 1, 1};
        cache.weights = {1, 1};
        return cache;
    };

    std::size_t completedPairs = 0;
    std::size_t symmetricRefusals = 0;
    std::size_t acceptedCaches = 0;
    for (std::size_t variantIndex = 0; variantIndex < variants.size(); ++variantIndex) {
        const auto& base = variants[variantIndex];
        CAPTURE(variantIndex);
        for (const double scale : scales) {
            CAPTURE(scale);
            for (const double translationRatio : translationRatios) {
                CAPTURE(translationRatio);
                const RS_Vector translation{translationRatio * scale,
                                            -translationRatio * scale};
                const auto centered = transform(base, scale, RS_Vector{0, 0});
                const auto translated = transform(base, scale, translation);
                REQUIRE(centered.isWellFormed());
                REQUIRE(translated.isWellFormed());
                CHECK(centered.kind() == base.kind());
                CHECK(translated.kind() == base.kind());
                if (const auto* source = std::get_if<RS_SplineData>(&base.value)) {
                    const auto* local = std::get_if<RS_SplineData>(&centered.value);
                    const auto* world = std::get_if<RS_SplineData>(&translated.value);
                    REQUIRE(local); REQUIRE(world);
                    CHECK(local->degree == source->degree);
                    CHECK(world->degree == source->degree);
                    CHECK(local->knotslist == source->knotslist);
                    CHECK(world->knotslist == source->knotslist);
                    CHECK(local->weights == source->weights);
                    CHECK(world->weights == source->weights);
                } else {
                    const auto& baseFit = std::get<LC_SplinePointsData>(base.value);
                    const auto& local = std::get<LC_SplinePointsData>(centered.value);
                    const auto& world = std::get<LC_SplinePointsData>(translated.value);
                    CHECK(local.useControlPoints == baseFit.useControlPoints);
                    CHECK(world.useControlPoints == baseFit.useControlPoints);
                    CHECK(local.fitTangentsConstrained == baseFit.fitTangentsConstrained);
                    CHECK(world.fitTangentsConstrained == baseFit.fitTangentsConstrained);
                    CHECK(local.fitTolerance == baseFit.fitTolerance);
                    CHECK(world.fitTolerance == baseFit.fitTolerance);
                }

                const auto centeredEntity = centered.makeEntity();
                const auto translatedEntity = translated.makeEntity();
                REQUIRE(centeredEntity); REQUIRE(translatedEntity);
                for (const double normalizedDistance : normalizedDistances) {
                    CAPTURE(normalizedDistance);
                    const double signedDistance = normalizedDistance * scale;
                    const LC_OffsetSourceDefinition centeredDefinition{
                        centered, signedDistance, 1e-4, false};
                    const LC_OffsetSourceDefinition translatedDefinition{
                        translated, signedDistance, 1e-4, false};
                    const auto centeredOptions = LC_OffsetWorkerTolerance::directOptions(
                        centeredDefinition, *centeredEntity);
                    const auto translatedOptions = LC_OffsetWorkerTolerance::directOptions(
                        translatedDefinition, *translatedEntity);
                    REQUIRE(centeredOptions); REQUIRE(translatedOptions);

                    const auto centeredReply = LC_OffsetWorkerKernel::run(
                        {token, req::GenerateSource{centeredDefinition, {}}});
                    const auto& centeredOutput = native(centeredReply, 7);
                    const auto translatedReply = LC_OffsetWorkerKernel::run(
                        {token, req::GenerateSource{translatedDefinition, {}}});
                    const auto& translatedOutput = native(translatedReply, 7);
                    CHECK(centeredOutput.engineStatus == translatedOutput.engineStatus);
                    CHECK(centeredOutput.ordinaryStatus == translatedOutput.ordinaryStatus);
                    CHECK(centeredOutput.ordinary.size() == translatedOutput.ordinary.size());

                    const auto generated = [](const result::Native& output) {
                        return output.engineStatus == LC_CurveOffsetStatus::Ok &&
                            output.ordinaryStatus == result::OrdinaryStatus::Complete &&
                            !output.ordinary.empty();
                    };
                    const bool centeredGenerated = generated(centeredOutput);
                    const bool translatedGenerated = generated(translatedOutput);
                    CHECK(centeredGenerated == translatedGenerated);
                    if (!centeredGenerated || !translatedGenerated) {
                        CHECK(centeredOutput.ordinary.empty());
                        CHECK(translatedOutput.ordinary.empty());
                        CHECK_FALSE(centeredOutput.changed);
                        CHECK_FALSE(translatedOutput.changed);
                        ++symmetricRefusals;
                        continue;
                    }
                    ++completedPairs;

                    std::vector<RS_Spline> centeredSplines;
                    std::vector<RS_Spline> translatedSplines;
                    for (const auto& item : centeredOutput.ordinary)
                        centeredSplines.emplace_back(nullptr, asSpline(item));
                    for (const auto& item : translatedOutput.ordinary)
                        translatedSplines.emplace_back(nullptr, asSpline(item));
                    std::vector<bool> matched(translatedSplines.size(), false);
                    const double roundingAllowance = 64.0 * std::numeric_limits<double>::epsilon() *
                        std::max(std::abs(translation.x), std::abs(translation.y));
                    const double deviationLimit = 4.0 * std::max(
                        centeredOptions->tolerance.requestedGeometry,
                        translatedOptions->tolerance.requestedGeometry) + roundingAllowance;
                    const auto deviation = [&](const RS_Spline& local, const RS_Spline& world,
                                               const bool reverse) {
                        double maximum = 0.0;
                        for (int sample = 0; sample <= 128; ++sample) {
                            const double parameter = static_cast<double>(sample) / 128.0;
                            const RS_Vector p = local.getPointAt(parameter);
                            const RS_Vector q = world.getPointAt(reverse ? 1.0 - parameter : parameter);
                            REQUIRE(p.valid); REQUIRE(q.valid);
                            maximum = std::max(maximum,
                                std::hypot((q.x - translation.x) - p.x,
                                           (q.y - translation.y) - p.y));
                        }
                        return maximum;
                    };
                    for (const auto& local : centeredSplines) {
                        double best = std::numeric_limits<double>::infinity();
                        std::size_t bestIndex = translatedSplines.size();
                        for (std::size_t candidate = 0; candidate < translatedSplines.size(); ++candidate) {
                            if (matched[candidate]) continue;
                            const double candidateDeviation = std::min(
                                deviation(local, translatedSplines[candidate], false),
                                deviation(local, translatedSplines[candidate], true));
                            if (candidateDeviation < best) {
                                best = candidateDeviation;
                                bestIndex = candidate;
                            }
                        }
                        REQUIRE(bestIndex < translatedSplines.size());
                        matched[bestIndex] = true;
                        CHECK(best <= deviationLimit);
                    }

                    for (const auto& item : centeredOutput.ordinary) {
                        auto cache = asSpline(item);
                        const auto fingerprint = RS_Spline::geometryFingerprint(cache);
                        const auto validation = LC_OffsetWorkerKernel::run(
                            {token, req::ValidateSourceCache{cache, centeredDefinition, {}}});
                        const auto& checked = native(validation, 8);
                        if (centeredOutput.ordinary.size() == 1) {
                            CHECK(checked.refusal == LC_OffsetCurveRefusal::None);
                            CHECK(checked.changed);
                            ++acceptedCaches;
                        } else {
                            CHECK(checked.refusal == LC_OffsetCurveRefusal::NotCacheRepresentable);
                            CHECK_FALSE(checked.changed);
                            CHECK(RS_Spline::geometryFingerprint(cache) == fingerprint);
                        }
                    }
                    for (const auto& item : translatedOutput.ordinary) {
                        auto cache = asSpline(item);
                        const auto fingerprint = RS_Spline::geometryFingerprint(cache);
                        const auto validation = LC_OffsetWorkerKernel::run(
                            {token, req::ValidateSourceCache{cache, translatedDefinition, {}}});
                        const auto& checked = native(validation, 8);
                        if (translatedOutput.ordinary.size() == 1) {
                            CHECK(checked.refusal == LC_OffsetCurveRefusal::None);
                            CHECK(checked.changed);
                            ++acceptedCaches;
                        } else {
                            CHECK(checked.refusal == LC_OffsetCurveRefusal::NotCacheRepresentable);
                            CHECK_FALSE(checked.changed);
                            CHECK(RS_Spline::geometryFingerprint(cache) == fingerprint);
                        }
                    }
                }
            }
        }
    }
    CHECK(completedPairs + symmetricRefusals == variants.size() * scales.size() *
          translationRatios.size() * normalizedDistances.size());
    CHECK(completedPairs > 0);
    CHECK(acceptedCaches > 0);
}

TEST_CASE("polyline cache promotion preserves mixed segment orientation and offset side",
          "[lc-offsetcurve][worker-kernel][source-cache][polyline]") {
    const std::array<LC_OffsetPolylineData, 2> variants{
        LC_OffsetPolylineData{{0, 0}, {2, 2}, false, {RS_ArcData{{1, 0}, 1, M_PI, 0, false}, RS_LineData{{2, 0}, {2, 2}}}},
        LC_OffsetPolylineData{{2, 2}, {0, 0}, false, {RS_LineData{{2, 2}, {2, 0}}, RS_ArcData{{1, 0}, 1, 0, M_PI, true}}}};
    for (const auto& data : variants) {
        CAPTURE(data.startpoint.x, data.startpoint.y, data.endpoint.x, data.endpoint.y);
        const LC_OffsetSourceGeometry source{data};
        REQUIRE(source.isWellFormed());
        for (const double signedDistance : {-.25, .25}) {
            CAPTURE(signedDistance);
            const LC_OffsetSourceDefinition definition{source, signedDistance, 1e-4, false};
            const req::Request generationRequest{token, req::GenerateSource{definition, {}}};
            const auto generationReply = LC_OffsetWorkerKernel::run(generationRequest);
            const auto& generated = native(generationReply, 7);
            REQUIRE(generated.engineStatus == LC_CurveOffsetStatus::Ok);
            REQUIRE(generated.ordinaryStatus == result::OrdinaryStatus::Complete);
            REQUIRE(generated.ordinary.size() == 1);
            REQUIRE(std::holds_alternative<RS_SplineData>(generated.ordinary.front()));
            const auto& cache = std::get<RS_SplineData>(generated.ordinary.front());
            const req::Request validationRequest{token, req::ValidateSourceCache{cache, definition, {}}};
            const auto validationReply = LC_OffsetWorkerKernel::run(validationRequest);
            const auto& validated = native(validationReply, 8);
            CHECK(validated.refusal == LC_OffsetCurveRefusal::None);
            CHECK(validated.changed);
        }
    }
}

TEST_CASE("open polyline worker generation and cache admission follow output topology",
          "[lc-offsetcurve][worker-kernel][source-cache][polyline][open][side]") {
    const double pi = std::acos(-1.0);
    const std::array<LC_OffsetPolylineData, 4> variants{
        LC_OffsetPolylineData{{0, 0}, {2, 2}, false,
            {RS_ArcData{{1, 0}, 1, pi, 0, false}, RS_LineData{{2, 0}, {2, 2}}}},
        LC_OffsetPolylineData{{2, 2}, {0, 0}, false,
            {RS_LineData{{2, 2}, {2, 0}}, RS_ArcData{{1, 0}, 1, 0, pi, true}}},
        LC_OffsetPolylineData{{0, 0}, {4, 2}, false,
            {RS_LineData{{0, 0}, {2, 0}}, RS_LineData{{2, 0}, {2, 2}},
             RS_LineData{{2, 2}, {4, 2}}}},
        LC_OffsetPolylineData{{1, 0}, {-1, 0}, false,
            {RS_ArcData{{0, 0}, 1, 0, pi / 2, false},
             RS_ArcData{{0, 0}, 1, pi / 2, pi, false}}}
    };

    for (std::size_t variant = 0; variant < variants.size(); ++variant) {
        const LC_OffsetSourceGeometry source{variants[variant]};
        CAPTURE(variant);
        REQUIRE(source.isWellFormed());
        for (const double signedDistance : {-.25, .25}) {
            CAPTURE(signedDistance);
            const LC_OffsetSourceDefinition definition{source, signedDistance, 1e-4, false};
            const req::Request generationRequest{token,
                req::GenerateSource{definition, {}}};
            const auto generationReply = LC_OffsetWorkerKernel::run(generationRequest);
            const auto& generated = native(generationReply, 7);
            REQUIRE(generated.engineStatus == LC_CurveOffsetStatus::Ok);
            REQUIRE(generated.ordinaryStatus == result::OrdinaryStatus::Complete);
            REQUIRE_FALSE(generated.ordinary.empty());

            const auto generatedWire = result::encode(generationReply, generationRequest);
            REQUIRE(generatedWire);
            const auto generatedRoundTrip = result::decode(*generatedWire, generationRequest);
            REQUIRE(generatedRoundTrip);

            RS_SplineData candidate;
            const bool singleSpline = generated.ordinary.size() == 1 &&
                std::holds_alternative<RS_SplineData>(generated.ordinary.front());
            if (singleSpline) {
                candidate = std::get<RS_SplineData>(generated.ordinary.front());
            } else if (const auto* segment =
                           std::get_if<req::Segment>(&generated.ordinary.front())) {
                candidate.degree = 1;
                candidate.controlPoints = {segment->start, segment->end};
                candidate.knotslist = {0, 0, 1, 1};
                candidate.weights = {1, 1};
            }

            const req::Request validationRequest{token,
                req::ValidateSourceCache{candidate, definition, {}}};
            const auto validationReply = LC_OffsetWorkerKernel::run(validationRequest);
            const auto& validated = native(validationReply, 8);
            CHECK(validated.engineRuns == 1);
            if (singleSpline && validated.refusal == LC_OffsetCurveRefusal::None) {
                CHECK(validated.changed);
            } else {
                CHECK(validated.refusal == LC_OffsetCurveRefusal::NotCacheRepresentable);
                CHECK_FALSE(validated.changed);
                CHECK_FALSE(validated.curve);
            }
            const auto validationWire = result::encode(validationReply, validationRequest);
            REQUIRE(validationWire);
            CHECK(result::decode(*validationWire, validationRequest));
        }
    }
}

TEST_CASE("closed polyline cache promotion or refusal covers line and arc chains on both sides",
          "[lc-offsetcurve][worker-kernel][source-cache][polyline]") {
    const double quarterTurn = M_PI / 2.0;
    RS_Polyline mixedBulgePolyline;
    mixedBulgePolyline.addVertex({0, 0}, 0.2);
    mixedBulgePolyline.addVertex({8, 0}, -0.2);
    mixedBulgePolyline.addVertex({8, 6});
    mixedBulgePolyline.addVertex({0, 6});
    mixedBulgePolyline.setClosed(true);
    mixedBulgePolyline.endPolyline();
    const auto mixedBulgeSource = LC_OffsetSourceGeometry::capture(mixedBulgePolyline);
    REQUIRE(mixedBulgeSource);
    REQUIRE(mixedBulgeSource->kind() == LC_OffsetSourceGeometry::Kind::Polyline);
    const auto& mixedBulgeData = std::get<LC_OffsetPolylineData>(mixedBulgeSource->value);
    REQUIRE(mixedBulgeData.closed);
    REQUIRE(mixedBulgeData.segments.size() == 4);
    const std::array<LC_OffsetPolylineData, 3> variants{
        LC_OffsetPolylineData{
            {0, 0},
            {0, 0},
            true,
            {RS_LineData{{0, 0}, {2, 0}}, RS_LineData{{2, 0}, {2, 2}}, RS_LineData{{2, 2}, {0, 2}}, RS_LineData{{0, 2}, {0, 0}}}},
        LC_OffsetPolylineData{{2, 0},
                              {2, 0},
                              true,
                              {RS_ArcData{{0, 0}, 2, 0, quarterTurn, false}, RS_ArcData{{0, 0}, 2, quarterTurn, M_PI, false},
                               RS_ArcData{{0, 0}, 2, M_PI, 3 * quarterTurn, false},
                               RS_ArcData{{0, 0}, 2, 3 * quarterTurn, 2 * M_PI, false}}},
        mixedBulgeData};
    for (const auto& data : variants) {
        const LC_OffsetSourceGeometry source{data};
        REQUIRE(source.isWellFormed());
        for (const double signedDistance : {-.25, .25}) {
            CAPTURE(signedDistance, data.segments.size());
            const LC_OffsetSourceDefinition definition{source, signedDistance, 1e-4, false};
            const req::Request generationRequest{token, req::GenerateSource{definition, {}}};
            const auto generationReply = LC_OffsetWorkerKernel::run(generationRequest);
            const auto& generated = native(generationReply, 7);
            REQUIRE(generated.engineStatus == LC_CurveOffsetStatus::Ok);
            REQUIRE(generated.ordinaryStatus == result::OrdinaryStatus::Complete);
            REQUIRE_FALSE(generated.ordinary.empty());
            RS_SplineData cache;
            if (const auto* spline = std::get_if<RS_SplineData>(&generated.ordinary.front())) {
                cache = *spline;
            } else {
                const auto& segment = std::get<req::Segment>(generated.ordinary.front());
                cache.degree = 1;
                cache.controlPoints = {segment.start, segment.end};
                cache.knotslist = {0, 0, 1, 1};
                cache.weights = {1, 1};
            }
            const req::Request validationRequest{token,
                req::ValidateSourceCache{cache, definition, {}}};
            const auto validationReply = LC_OffsetWorkerKernel::run(validationRequest);
            const auto& validated = native(validationReply, 8);
            if (generated.ordinary.size() == 1 &&
                std::holds_alternative<RS_SplineData>(generated.ordinary.front())) {
                CHECK(validated.refusal == LC_OffsetCurveRefusal::None);
                CHECK(validated.changed);
            } else {
                CHECK(validated.refusal == LC_OffsetCurveRefusal::NotCacheRepresentable);
                CHECK_FALSE(validated.changed);
          }
        }
    }
}

TEST_CASE("closed mixed-bulge source promotes only the cache-representable side",
          "[lc-offsetcurve][worker-kernel][source-cache][polyline][mixed-bulge]") {
    RS_Polyline polyline;
    polyline.addVertex({0, 0}, 0.2);
    polyline.addVertex({8, 0}, -0.2);
    polyline.addVertex({8, 6});
    polyline.addVertex({0, 6});
    polyline.setClosed(true);
    polyline.endPolyline();

    const auto source = LC_OffsetSourceGeometry::capture(polyline);
    REQUIRE(source);
    REQUIRE(source->kind() == LC_OffsetSourceGeometry::Kind::Polyline);
    REQUIRE(source->isWellFormed());
    for (const double signedDistance : {-.25, .25}) {
        CAPTURE(signedDistance);
        const LC_OffsetSourceDefinition definition{*source, signedDistance, 1e-4, false};
        const req::Request generationRequest{token, req::GenerateSource{definition, {}}};
        const auto generationReply = LC_OffsetWorkerKernel::run(generationRequest);
        const auto& generated = native(generationReply, 7);
        REQUIRE(generated.engineStatus == LC_CurveOffsetStatus::Ok);
        REQUIRE(generated.ordinaryStatus == result::OrdinaryStatus::Complete);
        RS_SplineData cache;
        if (signedDistance < 0.0) {
            REQUIRE(generated.ordinary.size() == 1);
            REQUIRE(std::holds_alternative<RS_SplineData>(generated.ordinary.front()));
            cache = std::get<RS_SplineData>(generated.ordinary.front());
        } else {
            REQUIRE(generated.ordinary.size() > 1);
            if (const auto* spline =
                    std::get_if<RS_SplineData>(&generated.ordinary.front())) {
                cache = *spline;
            } else {
                const auto& segment = std::get<req::Segment>(generated.ordinary.front());
                cache.degree = 1;
                cache.controlPoints = {segment.start, segment.end};
                cache.knotslist = {0, 0, 1, 1};
                cache.weights = {1, 1};
            }
        }
        const req::Request validationRequest{token,
            req::ValidateSourceCache{cache, definition, {}}};
        const auto validationReply = LC_OffsetWorkerKernel::run(validationRequest);
        const auto& validated = native(validationReply, 8);
        if (signedDistance < 0.0) {
            CHECK(validated.refusal == LC_OffsetCurveRefusal::None);
            CHECK(validated.changed);
            CHECK(validated.engineRuns == 1);
        } else {
            CHECK(validated.refusal == LC_OffsetCurveRefusal::NotCacheRepresentable);
            CHECK_FALSE(validated.changed);
            CHECK(validated.engineRuns == 1);
        }
    }
}

TEST_CASE("polyline source validation rejects degenerate and inconsistent chains",
          "[lc-offsetcurve][worker-kernel][source-cache][polyline]") {
    const std::array<LC_OffsetPolylineData, 4> invalidVariants{
        LC_OffsetPolylineData{{0, 0}, {1, 0}, false,
            {RS_LineData{{0, 0}, {0, 0}}, RS_LineData{{0, 0}, {1, 0}}}},
        LC_OffsetPolylineData{{0, 0}, {2, 0}, false,
            {RS_LineData{{0, 0}, {1, 0}}, RS_LineData{{1.01, 0}, {2, 0}}}},
        LC_OffsetPolylineData{{0, 0}, {2, 0}, false,
            {RS_LineData{{0, 0}, {1, 0}}, RS_LineData{{1, 0}, {2, 0}},
             RS_LineData{{2, 0}, {2, 0}}}},
        LC_OffsetPolylineData{{0, 0}, {1, 0}, true,
            {RS_LineData{{0, 0}, {1, 0}}}}
    };

    for (const auto& data : invalidVariants) {
        const LC_OffsetSourceGeometry source{data};
        CHECK_FALSE(source.isWellFormed());
        CHECK_FALSE(source.makeEntity());
        const LC_OffsetSourceDefinition definition{source, .25, 1e-4, false};
        CHECK_FALSE(definition.isWellFormed());
    }
}

TEST_CASE("long mixed polyline cache promotion preserves segment order across offset sides",
          "[lc-offsetcurve][worker-kernel][source-cache][polyline]") {
    const double pi = std::acos(-1.0);
    const LC_OffsetPolylineData data{{0, 0}, {-1, 1}, false, {
        RS_LineData{{0, 0}, {2, 0}},
        RS_ArcData{{2, 1}, 1, -pi / 2, 0, false},
        RS_LineData{{3, 1}, {3, 3}},
        RS_ArcData{{2, 3}, 1, 0, pi / 2, false},
        RS_LineData{{2, 4}, {0, 4}},
        RS_ArcData{{0, 3}, 1, pi / 2, pi, false},
        RS_LineData{{-1, 3}, {-1, 1}}
    }};
    const LC_OffsetSourceGeometry source{data};
    REQUIRE(source.isWellFormed());
    for (const double signedDistance : {-.5, -.1, .1, .5}) {
        CAPTURE(signedDistance);
        const LC_OffsetSourceDefinition definition{source, signedDistance, 1e-4, false};
        const req::Request generationRequest{token, req::GenerateSource{definition, {}}};
        const auto generationReply = LC_OffsetWorkerKernel::run(generationRequest);
        const auto& generated = native(generationReply, 7);
        REQUIRE(generated.engineStatus == LC_CurveOffsetStatus::Ok);
        REQUIRE(generated.ordinaryStatus == result::OrdinaryStatus::Complete);
        REQUIRE_FALSE(generated.ordinary.empty());

        if (generated.ordinary.size() == 1 &&
            std::holds_alternative<RS_SplineData>(generated.ordinary.front())) {
            const auto& cache = std::get<RS_SplineData>(generated.ordinary.front());
            const req::Request validationRequest{token,
                req::ValidateSourceCache{cache, definition, {}}};
            const auto validationReply = LC_OffsetWorkerKernel::run(validationRequest);
            const auto& validated = native(validationReply, 8);
            CHECK(validated.refusal == LC_OffsetCurveRefusal::None);
            CHECK(validated.changed);
        } else {
            RS_SplineData cache;
            const auto& segment = std::get<req::Segment>(generated.ordinary.front());
            cache.degree = 1;
            cache.controlPoints = {segment.start, segment.end};
            cache.knotslist = {0, 0, 1, 1};
            cache.weights = {1, 1};
            const req::Request validationRequest{token,
                req::ValidateSourceCache{cache, definition, {}}};
            const auto validationReply = LC_OffsetWorkerKernel::run(validationRequest);
            const auto& validated = native(validationReply, 8);
            CHECK(validated.refusal == LC_OffsetCurveRefusal::NotCacheRepresentable);
            CHECK_FALSE(validated.changed);
        }
    }
}

TEST_CASE("spline and fit-point caches promote on both signed offset sides",
          "[lc-offsetcurve][worker-kernel][source-cache][spline-variant]") {
    RS_SplineData rationalQuadratic{2, false};
    rationalQuadratic.controlPoints = {{0, 0}, {1, 2}, {3, 0}};
    rationalQuadratic.knotslist = {0, 0, 0, 1, 1, 1};
    rationalQuadratic.weights = {1, .7, 1};
    RS_SplineData cubic{3, false};
    cubic.controlPoints = {{0, 0}, {1, 2}, {2, 2}, {3, 0}};
    cubic.knotslist = {0, 0, 0, 0, 1, 1, 1, 1};
    LC_SplinePointsData fitPoints{false, false};
    fitPoints.splinePoints = {{0, 0}, {1, 2}, {2, 0}};
    const std::array<LC_OffsetSourceGeometry, 3> sources{
        LC_OffsetSourceGeometry{rationalQuadratic},
        LC_OffsetSourceGeometry{cubic},
        LC_OffsetSourceGeometry{fitPoints}
    };
    for (const auto& source : sources) {
        CAPTURE(static_cast<int>(source.kind()));
        for (const double signedDistance : {-.25, .25}) {
            CAPTURE(signedDistance);
            const LC_OffsetSourceDefinition definition{source, signedDistance, 1e-4, false};
            const req::Request generationRequest{token, req::GenerateSource{definition, {}}};
            const auto generationReply = LC_OffsetWorkerKernel::run(generationRequest);
            const auto& generated = native(generationReply, 7);
            REQUIRE(generated.engineStatus == LC_CurveOffsetStatus::Ok);
            REQUIRE(generated.ordinaryStatus == result::OrdinaryStatus::Complete);
            REQUIRE(generated.ordinary.size() == 1);
            REQUIRE(std::holds_alternative<RS_SplineData>(generated.ordinary.front()));
            const auto& cache = std::get<RS_SplineData>(generated.ordinary.front());
            const req::Request validationRequest{token,
                req::ValidateSourceCache{cache, definition, {}}};
            const auto validationReply = LC_OffsetWorkerKernel::run(validationRequest);
            const auto& validated = native(validationReply, 8);
            CHECK(validated.refusal == LC_OffsetCurveRefusal::None);
            CHECK(validated.changed);
        }
    }
}

TEST_CASE("nonuniform rational spline knot layouts preserve exact cache validation",
          "[lc-offsetcurve][worker-kernel][source-cache][spline-variant]") {
    RS_SplineData quadratic{2, false};
    quadratic.controlPoints = {{0, 0}, {1, 2}, {2, -1}, {3, 2}, {4, 0}};
    quadratic.knotslist = {0, 0, 0, .3, .7, 1, 1, 1};
    quadratic.weights = {1, .6, 1.8, .8, 1};
    RS_SplineData cubic{3, false};
    cubic.controlPoints = {{0, 0}, {1, 2}, {2, 2}, {3, -1}, {4, -1}, {5, 1}};
    cubic.knotslist = {0, 0, 0, 0, .4, .4, 1, 1, 1, 1};
    cubic.weights = {1, .7, 1.4, .9, 1.6, 1};
    const std::array<LC_OffsetSourceGeometry, 2> sources{
        LC_OffsetSourceGeometry{quadratic}, LC_OffsetSourceGeometry{cubic}};

    for (const auto& source : sources) {
        CAPTURE(static_cast<int>(source.kind()));
        REQUIRE(source.isWellFormed());
        for (const double signedDistance : {-.2, .2}) {
            CAPTURE(signedDistance);
            const LC_OffsetSourceDefinition definition{source, signedDistance, 1e-4, false};
            const req::Request generationRequest{token, req::GenerateSource{definition, {}}};
            const auto generationReply = LC_OffsetWorkerKernel::run(generationRequest);
            const auto& generated = native(generationReply, 7);
            REQUIRE(generated.engineStatus == LC_CurveOffsetStatus::Ok);
            REQUIRE(generated.ordinaryStatus == result::OrdinaryStatus::Complete);
            REQUIRE_FALSE(generated.ordinary.empty());

            if (generated.ordinary.size() == 1 &&
                std::holds_alternative<RS_SplineData>(generated.ordinary.front())) {
                const auto& cache = std::get<RS_SplineData>(generated.ordinary.front());
                const req::Request validationRequest{token,
                    req::ValidateSourceCache{cache, definition, {}}};
                const auto validationReply = LC_OffsetWorkerKernel::run(validationRequest);
                const auto& validated = native(validationReply, 8);
                CHECK(validated.refusal == LC_OffsetCurveRefusal::None);
                CHECK(validated.changed);
            } else {
                const RS_SplineData cache = arcCache();
                const req::Request validationRequest{token,
                    req::ValidateSourceCache{cache, definition, {}}};
                const auto validationReply = LC_OffsetWorkerKernel::run(validationRequest);
                const auto& validated = native(validationReply, 8);
                CHECK(validated.refusal == LC_OffsetCurveRefusal::NotCacheRepresentable);
                CHECK_FALSE(validated.changed);
            }
        }
    }
}

TEST_CASE("smooth internal-knot rational spline produces an exact-promotable cache",
          "[lc-offsetcurve][worker-kernel][source-cache][spline-variant]") {
    RS_SplineData rational{2, false};
    rational.controlPoints = {{0, 0}, {1, .25}, {3, .25}, {4, 0}};
    rational.knotslist = {0, 0, 0, .5, 1, 1, 1};
    rational.weights = {1, .9, 1.1, 1};
    const LC_OffsetSourceDefinition definition{
        LC_OffsetSourceGeometry{rational}, .05, 1e-4, false};
    REQUIRE(definition.isWellFormed());

    const req::Request generationRequest{token, req::GenerateSource{definition, {}}};
    const auto generationReply = LC_OffsetWorkerKernel::run(generationRequest);
    const auto& generated = native(generationReply, 7);
    REQUIRE(generated.engineStatus == LC_CurveOffsetStatus::Ok);
    REQUIRE(generated.ordinaryStatus == result::OrdinaryStatus::Complete);
    REQUIRE(generated.ordinary.size() == 1);
    REQUIRE(std::holds_alternative<RS_SplineData>(generated.ordinary.front()));

    const auto& cache = std::get<RS_SplineData>(generated.ordinary.front());
    const req::Request validationRequest{token,
        req::ValidateSourceCache{cache, definition, {}}};
    const auto validationReply = LC_OffsetWorkerKernel::run(validationRequest);
    const auto& validated = native(validationReply, 8);
    CHECK(validated.refusal == LC_OffsetCurveRefusal::None);
    CHECK(validated.changed);
}

TEST_CASE("offset child validates original raw cache once without generating or repairing it",
          "[lc-offsetcurve][worker-kernel][raw]") {
    req::ValidateCache op{arcCache(), definition(), .01, {}};
    const auto original = RS_Spline::geometryFingerprint(op.cache);
    const auto expected = geo::checkCache(op.cache, op.definition, op.worldTolerance);
    REQUIRE(expected);
    LC_OffsetWorkerKernel::Trace trace;
    const auto reply = LC_OffsetWorkerKernel::run({token, op}, &trace);
    const auto& output = native(reply, 1);
    REQUIRE(output.curve); CHECK(output.changed); CHECK(output.engineRuns == 0);
    CHECK_FALSE(output.engineStatus); CHECK_FALSE(trace.nativePrepareEntered);
    CHECK(trace.checkEntered); CHECK(trace.computationCompleted);
    work(output.checkWork, expected.work); work(trace.checkWork, expected.work);
    CHECK(RS_Spline::geometryFingerprint(output.curve->cache) == original);
    CHECK(RS_Spline::geometryFingerprint(op.cache) == original); certified(*output.curve);
    for (const int malformed : {0, 1, 2, 3}) {
        auto bad = op;
        if (malformed == 0) bad.cache.degree = 2;
        if (malformed == 1) bad.cache.weights[0] = 2;
        if (malformed == 2) bad.cache.fitPoints.push_back({0, 0});
        if (malformed == 3) bad.cache.knotslist[2] = -1;
        const auto raw = RS_Spline::geometryFingerprint(bad.cache);
        const auto rejected = LC_OffsetWorkerKernel::run({token, bad}, &trace);
        const auto& failed = native(rejected, 1);
        CHECK_FALSE(failed.curve); CHECK_FALSE(failed.changed); CHECK(failed.engineRuns == 0);
        CHECK(failed.refusal == LC_OffsetCurveRefusal::CheckFailed);
        REQUIRE(failed.checkReason); CHECK(*failed.checkReason == LC_OffsetCheckResult::Reason::Layout);
        CHECK(RS_Spline::geometryFingerprint(bad.cache) == raw);
    }
    op.limits.maxVertices = 0;
    const auto exhausted = LC_OffsetWorkerKernel::run({token, op});
    const auto& noBudget = native(exhausted, 1);
    CHECK_FALSE(noBudget.curve); work(noBudget.checkWork, {});
}

TEST_CASE("offset child CheckThrough checks raw membership without generation C or native adoption",
          "[lc-offsetcurve][worker-kernel][through]") {
    for (const std::size_t degree : {1u, 3u}) {
        req::CheckThrough op{lineCache(), definition(), .01, {-3, 0}, {}};
        if (degree == 3) {
            op.cache.degree = 3; op.cache.controlPoints = {{-3, 0}, {-1, 0}, {1, 0}, {3, 0}};
            op.cache.knotslist = {0, 0, 0, 0, 1, 1, 1, 1}; op.cache.weights = {1, 1, 1, 1};
        }
        const auto raw = req::Values::encodeCache(op.cache); REQUIRE(raw);
        geo::WorkBudget budget{op.limits};
        const auto layout = geo::inspectCacheLayout(op.cache, budget);
        REQUIRE(layout.status == geo::CacheLayoutStatus::Ok);
        auto expected = budget.consumed(); ++expected.vertices; ++expected.distancePairs;
        LC_OffsetWorkerKernel::Trace trace;
        const req::Request input{token, op};
        const auto reply = LC_OffsetWorkerKernel::run(input, &trace);
        const auto& output = native(reply, 4);
        REQUIRE(output); CHECK_FALSE(output.curve); CHECK_FALSE(output.changed);
        CHECK(output.engineRuns == 0); CHECK_FALSE(output.engineStatus); CHECK_FALSE(output.checkReason);
        CHECK_FALSE(trace.nativePrepareEntered); CHECK_FALSE(trace.checkEntered); CHECK_FALSE(trace.trimEntered);
        CHECK(trace.throughEntered); CHECK(trace.computationCompleted);
        work(output.checkWork, {}); work(trace.checkWork, {});
        work(output.throughWork, expected); work(trace.throughWork, expected);
        CHECK(output.outputUsage.cubicPieces == 0); CHECK(output.outputUsage.outputEntities == 0);
        CHECK(output.outputUsage.deepEntities == 0); CHECK(output.sampleUnits == 0);
        CHECK(req::Values::encodeCache(op.cache) == raw);
        const auto encoded = result::encode(reply, input); REQUIRE(encoded);
        const auto decoded = result::decode(*encoded, input); REQUIRE(decoded);
        work(native(*decoded, 4).throughWork, expected);
        for (const RS_Vector point : {RS_Vector{0, 0}, RS_Vector{50, 50}}) {
            op.point = point;
            const auto checked = LC_OffsetWorkerKernel::run({token, op}, &trace);
            const auto& membership = native(checked, 4);
            CHECK(membership.refusal == (point.x == 0 ? LC_OffsetCurveRefusal::None :
                LC_OffsetCurveRefusal::ThroughNotOnCache));
            CHECK_FALSE(membership.curve); CHECK_FALSE(membership.changed);
            CHECK(membership.throughWork.vertices > layout.work.vertices);
            CHECK(membership.throughWork.distancePairs > 0);
            work(trace.throughWork, membership.throughWork);
            work(membership.checkWork, {}); CHECK_FALSE(membership.checkReason);
            REQUIRE(result::encode(checked, {token, op}));
        }
    }
}

TEST_CASE("offset child CheckThrough retains bounded layout and membership refusal prefixes",
          "[lc-offsetcurve][worker-kernel][through][limits]") {
    req::CheckThrough op{lineCache(), definition(), .01, {-3, 0}, {}};
    LC_OffsetWorkerKernel::Trace trace;
    for (int malformed : {0, 1, 2, 3, 4}) {
        auto bad = op;
        if (malformed == 0) bad.cache.degree = 2;
        if (malformed == 1) bad.cache.weights[0] = 2;
        if (malformed == 2) bad.cache.fitPoints.push_back({0, 0});
        if (malformed == 3) bad.cache.knotslist[2] = -1;
        if (malformed == 4) bad.cache.savedOpenKnots.push_back(1);
        geo::WorkBudget budget{bad.limits};
        const auto layout = geo::inspectCacheLayout(bad.cache, budget);
        REQUIRE(layout.status != geo::CacheLayoutStatus::Ok);
        const req::Request input{token, bad};
        const auto reply = LC_OffsetWorkerKernel::run(input, &trace);
        const auto& failed = native(reply, 4);
        CHECK(failed.refusal == LC_OffsetCurveRefusal::ThroughNotOnCache);
        CHECK_FALSE(failed.curve); CHECK_FALSE(failed.changed); CHECK_FALSE(failed.checkReason);
        work(failed.checkWork, {}); work(failed.throughWork, budget.consumed());
        work(trace.throughWork, failed.throughWork); REQUIRE(result::encode(reply, input));
    }
    for (int limit = 0; limit < 6; ++limit) {
        CAPTURE(limit);
        auto limited = op; limited.point = {0, 0};
        if (limit == 0) limited.limits.maxCacheSpans = 0;
        if (limit == 1) limited.limits.maxVertices = 0;
        if (limit == 2) limited.limits.maxVertices = 2;
        if (limit == 3) limited.limits.maxDistancePairs = 0;
        if (limit == 4) limited.limits.maxDepth = 0;
        if (limit == 5) limited.limits.maxOrderingComparisons = 0;
        const req::Request input{token, limited};
        const auto reply = LC_OffsetWorkerKernel::run(input, &trace);
        const auto& output = native(reply, 4);
        if (limit < 5) CHECK(output.refusal == LC_OffsetCurveRefusal::ThroughNotOnCache);
        CHECK(output.throughWork.cacheSpans <= limited.limits.maxCacheSpans);
        CHECK(output.throughWork.vertices <= limited.limits.maxVertices);
        CHECK(output.throughWork.distancePairs <= limited.limits.maxDistancePairs);
        CHECK(output.throughWork.deepest <= limited.limits.maxDepth);
        CHECK(output.throughWork.orderingComparisons <= limited.limits.maxOrderingComparisons);
        if (limit == 2) work(output.throughWork, {1, 2, 0});
        // A zero pair allowance rejects the WorkBudget before layout entry.
        if (limit == 3) work(output.throughWork, {});
        work(output.checkWork, {}); CHECK_FALSE(output.checkReason);
        work(trace.throughWork, output.throughWork); REQUIRE(result::encode(reply, input));
    }
    for (int invalid = 0; invalid < 3; ++invalid) {
        auto bad = op;
        if (invalid == 0) bad.worldTolerance = 0;
        if (invalid == 1) bad.worldTolerance = -1;
        if (invalid == 2) bad.definition.basis.majorP = {0, 0};
        const req::Request input{token, bad};
        const auto reply = LC_OffsetWorkerKernel::run(input, &trace);
        const auto& output = native(reply, 4);
        CHECK(output.refusal == LC_OffsetCurveRefusal::InvalidInput);
        work(output.checkWork, {}); work(output.throughWork, {});
        CHECK_FALSE(trace.throughEntered); CHECK_FALSE(trace.checkEntered); CHECK_FALSE(trace.nativePrepareEntered);
        CHECK(trace.computationCompleted); REQUIRE(result::encode(reply, input));
    }
    op.point.x = std::numeric_limits<double>::quiet_NaN();
    const auto unadmitted = LC_OffsetWorkerKernel::run({token, op}, &trace);
    CHECK(unadmitted.failure == result::Failure::InternalFailure);
    CHECK_FALSE(trace.throughEntered); CHECK_FALSE(trace.computationCompleted);
}

TEST_CASE("offset child raw trim adapters preserve original polynomial and conic payloads",
          "[lc-offsetcurve][worker-kernel][trim]") {
    for (const bool reverse : {false, true}) {
        auto data = lineCache();
        data.controlPoints = {{-3, .5}, {3, .5}};
        if (reverse) std::reverse(data.controlPoints.begin(), data.controlPoints.end());
        const auto original = RS_Spline::geometryFingerprint(data);
        const req::Cutter circle = RS_CircleData{{0, 0}, 2};
        LC_OffsetWorkerKernel::Trace trace;
        const auto reply = LC_OffsetWorkerKernel::run({token, req::TrimCache{data, circle, {{0, .5}}, {}}}, &trace);
        const auto& output = trimmed(reply, 2);
        REQUIRE(output.refusal == trim::Refusal::None); REQUIRE(output.fragments.size() == 2);
        CHECK(output.rootCount == 2); CHECK(output.fragmentCount == 2);
        CHECK(trace.trimEntered); CHECK(trace.computationCompleted);
        CHECK_FALSE(trace.nativePrepareEntered); CHECK_FALSE(trace.checkEntered);
        CHECK(trace.trimWork.intersections.totalBoxPairs == output.work.intersections.totalBoxPairs);
        CHECK(RS_Spline::geometryFingerprint(data) == original);
        const auto paired = LC_OffsetWorkerKernel::run({token,
            req::TrimConic{data, circle, {{0, 2}}, trim::Pick{{0, .5}}, {}}});
        const auto& two = trimmed(paired, 3);
        REQUIRE(two.refusal == trim::Refusal::None);
        CHECK(two.fragments.size() == 2); CHECK_FALSE(two.conicFragments.empty());
        auto bad = data; bad.weights[0] = 2;
        const auto malformed = LC_OffsetWorkerKernel::run({token,
            req::TrimCache{data, bad, {{0, 0}}, {}}});
        const auto& refused = trimmed(malformed, 2);
        CHECK(refused.refusal == trim::Refusal::UnsupportedLayout); CHECK(refused.fragments.empty());
    }
    // An ordinary ellipse constructor rounds these angles to a whole ellipse.
    // The child adapter must leave them raw for strict trim admission.
    RS_EllipseData tiny{{0, 0}, {2, 0}, 1, 0, 1e-12, false};
    const auto tinyReply = LC_OffsetWorkerKernel::run({token,
        req::TrimCache{lineCache(), tiny, {{0, 0}}, {}}});
    const auto& tinyResult = trimmed(tinyReply, 2);
    CHECK(tinyResult.refusal != trim::Refusal::None); CHECK(tinyResult.fragments.empty());
    CHECK(tinyResult.rootCount < 2);
}

TEST_CASE("offset child rejects unadmitted values before entering computation",
          "[lc-offsetcurve][worker-kernel]") {
    auto op = req::Generate{definition(), {}, {}};
    op.limits.checkLimits.maxVertices = LC_OffsetCheckLimits{}.maxVertices + 1;
    LC_OffsetWorkerKernel::Trace trace;
    const auto reply = LC_OffsetWorkerKernel::run({token, op}, &trace);
    CHECK(reply.failure == result::Failure::InternalFailure);
    CHECK(std::holds_alternative<std::monostate>(reply.result));
    CHECK_FALSE(trace.computationCompleted); CHECK_FALSE(trace.nativePrepareEntered);
    CHECK_FALSE(trace.checkEntered); CHECK_FALSE(trace.trimEntered);
}

TEST_CASE("offset child serializes actual lowered-budget refusal prefixes without raising caps",
          "[lc-offsetcurve][worker-kernel][limits]") {
    for (unsigned limit = 0; limit < 8; ++limit) {
        CAPTURE(limit);
        auto def = definition();
        def.basis = {{0, 0}, {100, 0}, .5, 0, 3.14159265358979323846, false};
        req::Generate op{def, {}, {}};
        switch (limit) {
        case 0: op.limits.maxSamples = 1; break;
        case 1: op.limits.maxIntersectionPairs = 1; break;
        case 2: op.limits.outputBudget.maxCubicPieces = 1; break;
        case 3: op.limits.outputBudget.maxDeepEntities = 1; break;
        case 4: op.limits.checkLimits.maxDistancePairs = 1; break;
        case 5: op.limits.checkLimits.maxVertices = 1; break;
        case 6: op.limits.checkLimits.maxDepth = 1; break;
        case 7: op.limits.checkLimits.maxOrderingComparisons = 0; break;
        }
        const req::Request request{token, op};
        const auto reply = LC_OffsetWorkerKernel::run(request);
        const auto& output = native(reply, 0);
        CHECK_FALSE(output); CHECK_FALSE(output.curve); CHECK_FALSE(output.changed);
        const auto encoded = result::encode(reply, request);
        REQUIRE(encoded);
        const auto decoded = result::decode(*encoded, request);
        REQUIRE(decoded);
        const auto& retained = std::get<result::Native>(decoded->result);
        CHECK(retained.refusal == output.refusal); CHECK(retained.engineStatus == output.engineStatus);
        CHECK(retained.checkReason == output.checkReason); CHECK(retained.engineRuns == output.engineRuns);
        work(retained.checkWork, output.checkWork); work(retained.throughWork, output.throughWork);
        CHECK(retained.sampleUnits == output.sampleUnits);
        CHECK(retained.attemptedDeepCountComplete == output.attemptedDeepCountComplete);
        CHECK(retained.attemptedOutputUsage.deepEntities == output.attemptedOutputUsage.deepEntities);
    }
}


namespace {
req::Produce seriesInput(const RS_Entity& source, bool native = false) {
    req::Produce op;
    if (const auto* ellipse = dynamic_cast<const RS_Ellipse*>(&source)) op.source = ellipse->getData();
    else op.source = dynamic_cast<const RS_Spline&>(source).getData();
    if (source.getMin().valid) op.minimum = source.getMin();
    if (source.getMax().valid) op.maximum = source.getMax();
    op.context.topLevel = true; op.coordinate = {3, 2}; op.distance = .25;
    op.copies = 2; op.asOffsetCurve = native;
    return op;
}
void sameProducedWork(const result::Produced& value, const LC_OffsetSourceOutcome& expected) {
    const auto& s = value.source;
    CHECK(s.status == expected.status); CHECK(s.engineStatus == expected.engineStatus); CHECK(s.detail == expected.detail);
    CHECK(s.stoppedBy == expected.stoppedBy); CHECK(s.copiesMade == expected.copiesMade);
    CHECK(s.copiesRequested == expected.copiesRequested); CHECK(s.sourceRemoved == expected.sourceRemoved);
    CHECK(s.copiesKeptPlain == expected.copiesKeptPlain); CHECK(s.resolvedSide == expected.resolvedSide);
    CHECK(value.copyOutputCounts == expected.copyOutputCounts);
    CHECK(s.rebaseEngineRuns == expected.rebaseEngineRuns); CHECK(s.cacheEngineRuns == expected.cacheEngineRuns);
    CHECK(s.reusedCacheCopies == expected.reusedCacheCopies); CHECK(s.sampleUnits == expected.sampleUnits);
    CHECK(s.intersectionPairAllowance == expected.intersectionPairAllowance);
    CHECK(s.adoptionAttempts == expected.adoptionAttempts); work(s.adoptionCheckWork, expected.adoptionCheckWork);
    CHECK(s.usage.cubicPieces == expected.usage.cubicPieces); CHECK(s.usage.outputEntities == expected.usage.outputEntities);
    CHECK(s.usage.deepEntities == expected.usage.deepEntities);
    CHECK(s.attemptedOutputUsage.cubicPieces == expected.attemptedOutputUsage.cubicPieces);
    CHECK(s.attemptedOutputUsage.outputEntities == expected.attemptedOutputUsage.outputEntities);
    CHECK(s.attemptedOutputUsage.deepEntities == expected.attemptedOutputUsage.deepEntities);
    CHECK(s.attemptedDeepCountComplete == expected.attemptedDeepCountComplete);
    REQUIRE(value.keptPlain.size() == expected.keptPlain.size());
    for (std::size_t i = 0; i < value.keptPlain.size(); ++i) {
        CHECK(value.keptPlain[i].copy == expected.keptPlain[i].copy);
        CHECK(value.keptPlain[i].reason == expected.keptPlain[i].reason);
        CHECK(value.keptPlain[i].checkReason == expected.keptPlain[i].checkReason);
    }
    REQUIRE(value.rebases.size() == expected.rebases.size());
    for (std::size_t i = 0; i < value.rebases.size(); ++i) {
        const auto& a = value.rebases[i]; const auto& b = expected.rebases[i];
        CHECK(a.copy == b.copy); CHECK(a.proposalRefusal == b.proposalRefusal); CHECK(a.nativeRefusal == b.nativeRefusal);
        CHECK(a.engineStatus == b.engineStatus); CHECK(a.checkReason == b.checkReason); work(a.checkWork, b.checkWork);
        CHECK(a.engineRuns == b.engineRuns); CHECK(a.sampleUnits == b.sampleUnits);
        CHECK(a.intersectionPairAllowance == b.intersectionPairAllowance); CHECK(a.adopted == b.adopted);
    }
}
result::Produced seriesParity(RS_Entity& source, const req::Produce& op) {
    RS_OffsetData data; data.coord = op.coordinate; data.sideFallback = op.fallback.value_or(RS_Vector{false});
    data.distance = op.distance; data.number = op.copies; data.multipleCopies = true; data.keepOriginals = true;
    data.asOffsetCurve = op.asOffsetCurve;
    LC_OffsetSourceOutcome expected;
    std::vector<std::unique_ptr<RS_Entity>> roots;
    expected.status = RS_Modification::prepareOffsetSource(data, source, op.preview, op.limits,
        op.limits.maxDeepEntitiesPerRequest, op.context, roots, expected);
    const req::Request input{token, op}; const auto before = req::encode(input); REQUIRE(before);
    const auto reply = LC_OffsetWorkerKernel::run(input);
    REQUIRE(reply.failure == result::Failure::None); REQUIRE(std::holds_alternative<result::Produced>(reply.result));
    const auto& value = std::get<result::Produced>(reply.result);
    sameProducedWork(value, expected); CHECK(req::encode(input) == before);
    if (value.outputStatus == result::OrdinaryStatus::Complete) {
        REQUIRE(value.outputs.size() == roots.size());
        for (std::size_t i = 0; i < roots.size(); ++i) {
            const auto& output = value.outputs[i];
            if (const auto* curve = std::get_if<result::Curve>(&output)) {
                const auto* native = dynamic_cast<const LC_OffsetCurve*>(roots[i].get()); REQUIRE(native);
                CHECK(req::Values::encodeCache(curve->cache) == req::Values::encodeCache(native->getData()));
                CHECK(req::Values::encodeDefinition(curve->definition) == req::Values::encodeDefinition(native->definition()));
                CHECK(curve->worldTolerance == native->checkedWorldTolerance());
                CHECK(curve->closed == native->cacheFormsClosedChain());
                CHECK(curve->grips == native->cachedDefinitionGrips()); CHECK(curve->midpoint == native->getMiddlePoint());
            } else if (const auto* line = std::get_if<req::Segment>(&output)) {
                REQUIRE(dynamic_cast<const RS_Line*>(roots[i].get()));
                CHECK(line->start == roots[i]->getStartpoint()); CHECK(line->end == roots[i]->getEndpoint());
            } else {
                const auto* spline = dynamic_cast<const RS_Spline*>(roots[i].get()); REQUIRE(spline);
                CHECK_FALSE(dynamic_cast<const LC_OffsetCurve*>(roots[i].get()));
                CHECK(req::Values::encodeCache(std::get<RS_SplineData>(output)) == req::Values::encodeCache(spline->getData()));
            }
        }
    } else CHECK(value.outputs.empty());
    const auto encoded = result::encode(reply, input); REQUIRE(encoded);
    const auto decoded = result::decode(*encoded, input); REQUIRE(decoded);
    CHECK(result::encode(*decoded, input) == encoded);
    return value;
}
}

TEST_CASE("Produce kernel matches shared ellipse source series and records actual adoption C work",
          "[lc-offsetcurve][worker-kernel][produce]") {
    RS_Ellipse source{nullptr, RS_EllipseData{{0, 0}, {1, 0}, 1, 0, .1, false}};
    for (bool native : {false, true}) for (bool preview : {false, true}) {
        auto op = seriesInput(source, native); op.preview = preview;
        const auto value = seriesParity(source, op);
        REQUIRE(value.source.status == LC_OffsetSourceStatus::Succeeded);
        REQUIRE(value.outputStatus == result::OrdinaryStatus::Complete);
        CHECK(value.copyOutputCounts == std::vector<std::size_t>{1, 1});
        if (native && !preview) {
            CHECK(value.source.adoptionAttempts == 2);
            geo::CheckWork actual;
            for (const auto& output : value.outputs) {
                REQUIRE(std::holds_alternative<result::Curve>(output));
                const auto& c = std::get<result::Curve>(output);
                const auto checked = geo::checkCache(c.cache, c.definition, c.worldTolerance); REQUIRE(checked);
                actual.cacheSpans += checked.work.cacheSpans; actual.vertices += checked.work.vertices;
                actual.distancePairs += checked.work.distancePairs; actual.orderingComparisons += checked.work.orderingComparisons;
                actual.deepest = std::max(actual.deepest, checked.work.deepest);
            }
            work(value.source.adoptionCheckWork, actual); CHECK(actual.vertices > 0);
        } else {
            CHECK(value.source.adoptionAttempts == 0); work(value.source.adoptionCheckWork, {});
            for (const auto& output : value.outputs) CHECK_FALSE(std::holds_alternative<result::Curve>(output));
        }
    }
    auto op = seriesInput(source, true); op.context.topLevel = false;
    const auto plain = seriesParity(source, op);
    CHECK(plain.source.adoptionAttempts == 0); CHECK(plain.source.copiesKeptPlain == plain.source.copiesMade);
}

TEST_CASE("Produce resolves the inward pick for a very-large-origin thin ellipse cusp",
          "[lc-offsetcurve][worker-kernel][produce][ellipse][cusp]") {
    const double angle = 3.14159265358979323846 / 6.0;
    RS_Ellipse source{nullptr, RS_EllipseData{{1e7, -1e7}, {100.0 * std::cos(angle), 100.0 * std::sin(angle)},
                                               1e-3, 0.2, 0.0, true}};
    auto op = seriesInput(source);
    op.coordinate = source.getCenter();
    op.fallback = source.getCenter();
    op.distance = 1.1e-4;
    op.copies = 1;

    const auto defaultTolerance = LC_CurveOffset::createEntities(
        source, LC_CurveOffset::makeSideRequest(LC_CurveOffsetSide::Right, op.distance),
        LC_CurveOffset::makeOffsetOptions(source, op.distance), LC_CurveOffset::makeDirectSourceBudget());
    CHECK(defaultTolerance.status == LC_CurveOffsetStatus::Ok);
    CHECK(defaultTolerance.intersectionDiagnostics.stage ==
          LC_CurveOffsetGeometryResult::IntersectionDiagnostics::Stage::MaterializedOutput);
    const auto explicitTolerance = LC_CurveOffset::createEntities(
        source, LC_CurveOffset::makeSideRequest(LC_CurveOffsetSide::Right, op.distance),
        LC_CurveOffset::makeOffsetOptions(source, op.distance, 0.1 * op.distance),
        LC_CurveOffset::makeDirectSourceBudget());
    CHECK(explicitTolerance.status == LC_CurveOffsetStatus::Ok);
    CHECK(explicitTolerance.intersectionDiagnostics.status == LC_IntersectionStatus::Ok);

    const auto value = seriesParity(source, op);
    REQUIRE(value.source.resolvedSide);
    CHECK(*value.source.resolvedSide == LC_CurveOffsetSide::Right);
    CHECK(value.source.status == LC_OffsetSourceStatus::Succeeded);
    CHECK(value.source.engineStatus == LC_CurveOffsetStatus::Ok);
    CHECK(value.outputStatus == result::OrdinaryStatus::Complete);
    CHECK(value.copyOutputCounts == std::vector<std::size_t>{1});
    REQUIRE(value.outputs.size() == 1);
    CHECK(std::holds_alternative<RS_SplineData>(value.outputs.front()));
}

TEST_CASE("Produce kernel repeats actual stored caches using raw definition context without native input shells",
          "[lc-offsetcurve][worker-kernel][produce]") {
    RS_Ellipse original{nullptr, RS_EllipseData{{0, 0}, {1, 0}, 1, 0, .1, false}};
    auto made = LC_OffsetCurve::create(original, 1, false); REQUIRE(made);
    for (bool native : {false, true}) for (bool preview : {false, true}) {
        auto op = seriesInput(*made.entity, native); op.preview = preview;
        op.context.nativeSource = true; op.context.nativeDefinition = made.entity->definition();
        const auto value = seriesParity(*made.entity, op);
        REQUIRE(value.source.status == LC_OffsetSourceStatus::Succeeded);
        CHECK(value.source.adoptionAttempts == 0);
        if (!native || preview) {
            CHECK(value.rebases.empty()); CHECK(value.source.rebaseEngineRuns == 0);
            for (const auto& output : value.outputs) CHECK_FALSE(std::holds_alternative<result::Curve>(output));
        } else {
            REQUIRE(value.rebases.size() == 2);
            CHECK(value.source.rebaseEngineRuns == 2); CHECK(value.source.cacheEngineRuns == 0);
        }
    }
    auto op = seriesInput(*made.entity, true);
    op.context.nativeSource = true; op.context.nativeDefinition = made.entity->definition();
    op.coordinate = {0, 0}; op.distance = 1; op.copies = 1;
    const auto zero = seriesParity(*made.entity, op);
    REQUIRE(zero.rebases.size() == 1); CHECK(zero.rebases[0].proposalRefusal == LC_OffsetCurveAgain::Refusal::ZeroSum);
    CHECK(zero.source.rebaseEngineRuns == 0); CHECK(zero.source.cacheEngineRuns == 1);
    auto ineligible = seriesInput(*made.entity, true); ineligible.copies = 1;
    ineligible.context.nativeSource = true;
    const auto undefined = seriesParity(*made.entity, ineligible);
    REQUIRE(undefined.keptPlain.size() == 1);
    CHECK(undefined.keptPlain[0].reason == LC_OffsetPlainReason::UndefinedDefinition);
    ineligible.context.nativeDefinition = made.entity->definition(); ineligible.context.topLevel = false;
    const auto nested = seriesParity(*made.entity, ineligible);
    REQUIRE(nested.keptPlain.size() == 1); CHECK(nested.keptPlain[0].reason == LC_OffsetPlainReason::NotTopLevel);
    for (const auto& output : nested.outputs) CHECK_FALSE(std::holds_alternative<result::Curve>(output));
    op.coordinate = {3, 2}; op.distance = .25; op.limits.maxSamples = 1;
    const auto refused = seriesParity(*made.entity, op);
    CHECK(refused.source.status == LC_OffsetSourceStatus::LimitExceeded);
    CHECK(refused.copyOutputCounts.empty()); CHECK(refused.outputs.empty());
    CHECK(refused.outputStatus == result::OrdinaryStatus::None);
    CHECK(refused.source.rebaseEngineRuns == 1); CHECK(refused.source.sampleUnits > 1);
}

TEST_CASE("Produce kernel preserves real vanished prefixes and aggregate cap unsupported receipts",
          "[lc-offsetcurve][worker-kernel][produce]") {
    RS_Ellipse circle{nullptr, RS_EllipseData{{0, 0}, {5, 0}, 1}};
    auto op = seriesInput(circle); op.coordinate = {0, 0}; op.distance = 2; op.copies = 3;
    const auto prefix = seriesParity(circle, op);
    REQUIRE(prefix.source.status == LC_OffsetSourceStatus::Succeeded);
    CHECK(prefix.source.copiesMade == 2); CHECK(prefix.source.stoppedBy == LC_OffsetSourceStatus::Vanished);
    CHECK(prefix.copyOutputCounts == std::vector<std::size_t>{1, 1});
    op.distance = 1; op.copies = 1;
    const auto baseline = seriesParity(circle, op);
    REQUIRE(baseline.outputStatus == result::OrdinaryStatus::Complete);
    op.limits.perSource.maxCubicPieces = baseline.source.usage.cubicPieces;
    const auto seam = seriesParity(circle, op);
    REQUIRE(seam.source.status == LC_OffsetSourceStatus::Succeeded);
    CHECK(seam.source.usage.cubicPieces == baseline.source.usage.cubicPieces);
    CHECK(seam.outputStatus == result::OrdinaryStatus::Unsupported); CHECK(seam.outputs.empty());
    CHECK(seam.copyOutputCounts == baseline.copyOutputCounts);
}

TEST_CASE("Produce raw ellipse adapters preserve captured bounds and do not repair source angles",
          "[lc-offsetcurve][worker-kernel][produce][raw]") {
    class RawSource final : public RS_Ellipse {
    public:
        RawSource(const RS_EllipseData& d, const RS_Vector& lo, const RS_Vector& hi) {
            m_data = d; m_minV = lo; m_maxV = hi;
        }
    };
    RS_Ellipse normal{nullptr, RS_EllipseData{{0, 0}, {1, 0}, 1, 0, .1, false}};
    // Captured bounds are values, not a request to calculateBorders() and repair raw data.
    RawSource captured{normal.getData(), {-1e9, -1e9}, {1e9, 1e9}};
    auto op = seriesInput(captured); op.copies = 1; op.coordinate = {1.1, .05}; op.fallback = RS_Vector{0, 0};
    const auto fallback = seriesParity(captured, op);
    REQUIRE(fallback.source.resolvedSide);
    CHECK(*fallback.source.resolvedSide == LC_CurveOffsetSide::Left);
    op.distance = -.25; seriesParity(captured, op);
    auto tiny = normal.getData(); tiny.angle1 = 0; tiny.angle2 = 1e-12; tiny.isArc = true;
    RawSource unmodified{tiny, normal.getMin(), normal.getMax()};
    op = seriesInput(unmodified); op.copies = 1;
    seriesParity(unmodified, op); // Compare the raw source, not its constructor-normalized whole ellipse.
}

namespace {
result::Produced cachedSeries(const req::CachedProduce& op) {
    const req::Request input{token, op};
    const auto request = req::encode(input); REQUIRE(request);
    const auto reply = LC_OffsetWorkerKernel::run(input);
    INFO("cached Produce failure=" << int(reply.failure));
    REQUIRE(reply.failure == result::Failure::None);
    REQUIRE(std::holds_alternative<result::Produced>(reply.result));
    const auto wire = result::encode(reply, input); REQUIRE(wire);
    const auto decoded = result::decode(*wire, input); REQUIRE(decoded);
    CHECK(result::encode(*decoded, input) == wire);
    CHECK(req::encode(input) == request);
    return std::get<result::Produced>(decoded->result);
}
}

TEST_CASE("Cached Produce reuses per-copy geometry with zero producer effort and preserves side and eviction",
          "[lc-offsetcurve][worker-kernel][preview-portable]") {
    RS_Ellipse source{nullptr, RS_EllipseData{{0, 0}, {1, 0}, 1, 0, .1, false}};
    req::CachedProduce op{seriesInput(source), {}}; op.produce.preview = true;
    auto first = cachedSeries(op);
    REQUIRE(first.source.status == LC_OffsetSourceStatus::Succeeded);
    CHECK(first.source.cacheEngineRuns == 2); REQUIRE(first.previewCache);
    CHECK(first.previewCache->entries.size() == 2);
    op.cache = *first.previewCache;
    op.produce.limits.maxSamples = 1; op.produce.limits.maxIntersectionPairs = 1;
    const auto reused = cachedSeries(op);
    CHECK(reused.source.status == LC_OffsetSourceStatus::Succeeded);
    CHECK(reused.source.cacheEngineRuns == 0); CHECK(reused.source.sampleUnits == 0);
    CHECK(reused.source.intersectionPairAllowance == 0); CHECK(reused.source.reusedCacheCopies == 2);
    REQUIRE(reused.reusedCopies.size() == 2);
    CHECK(reused.reusedCopies[0].copy == 1); CHECK(reused.reusedCopies[1].copy == 2);
    CHECK(reused.previewCache->revision == op.cache.revision);
    op.produce.limits = {}; op.produce.coordinate = {0, 0};
    const auto opposite = cachedSeries(op);
    REQUIRE(opposite.source.resolvedSide); REQUIRE(first.source.resolvedSide);
    CHECK(opposite.source.resolvedSide != first.source.resolvedSide);
    CHECK(opposite.source.reusedCacheCopies == 0); CHECK(opposite.source.cacheEngineRuns == 2);
    op.produce.coordinate = {3, 2}; op.produce.distance = .5;
    op.cache.otherEntries = 254;
    const auto evicted = cachedSeries(op);
    CHECK(evicted.source.reusedCacheCopies == 1); CHECK(evicted.source.cacheEngineRuns == 1);
    REQUIRE(evicted.previewCache); CHECK(evicted.previewCache->cleared);
    CHECK(evicted.previewCache->otherEntries == 0); CHECK(evicted.previewCache->entries.size() == 1);
}

TEST_CASE("Cached Produce refusal budgets grow correctly and cached ellipse adoption still runs C",
          "[lc-offsetcurve][worker-kernel][preview-portable]") {
    RS_Ellipse source{nullptr, RS_EllipseData{{0, 0}, {1, 0}, 1, 0, .1, false}};
    req::CachedProduce op{seriesInput(source), {}}; op.produce.preview = true; op.produce.copies = 1;
    const auto initial = cachedSeries(op); REQUIRE(initial.previewCache);
    op.cache = *initial.previewCache;
    op.produce.preview = false; op.produce.asOffsetCurve = true;
    const auto native = cachedSeries(op);
    REQUIRE(native.source.status == LC_OffsetSourceStatus::Succeeded);
    CHECK(native.source.reusedCacheCopies == 1); CHECK(native.source.cacheEngineRuns == 0);
    CHECK(native.source.adoptionAttempts == 1); CHECK(native.source.adoptionCheckWork.vertices > 0);
    REQUIRE(native.outputs.size() == 1); REQUIRE(std::holds_alternative<result::Curve>(native.outputs[0]));
    auto& shape = op.cache.entries[0].geometry[0];
    if (auto* line = std::get_if<RS_LineData>(&shape)) line->endpoint.x += 100;
    else std::get<RS_SplineData>(shape).controlPoints[0].x += 100;
    const auto unchecked = cachedSeries(op);
    CHECK(unchecked.source.adoptionAttempts == 1);
    REQUIRE(unchecked.outputs.size() == 1); CHECK_FALSE(std::holds_alternative<result::Curve>(unchecked.outputs[0]));
    REQUIRE(unchecked.keptPlain.size() == 1);
    CHECK(unchecked.keptPlain[0].reason == LC_OffsetPlainReason::CheckFailed);
    auto& e = op.cache.entries[0]; e.geometry.clear(); e.usage = {};
    e.status = LC_CurveOffsetStatus::LimitExceeded; e.budget = {1, 1, 1};
    op.produce.asOffsetCurve = false; op.produce.preview = true; op.produce.limits.perSource = e.budget;
    const auto refused = cachedSeries(op);
    CHECK(refused.source.status == LC_OffsetSourceStatus::LimitExceeded);
    CHECK(refused.source.cacheEngineRuns == 0); CHECK(refused.source.reusedCacheCopies == 1);
    op.produce.limits = {};
    const auto grown = cachedSeries(op);
    CHECK(grown.source.status == LC_OffsetSourceStatus::Succeeded);
    CHECK(grown.source.cacheEngineRuns == 1); CHECK(grown.source.reusedCacheCopies == 0);
}

TEST_CASE("Cached Produce native zero-sum repeats use the distinct stored-cache fallback",
          "[lc-offsetcurve][worker-kernel][preview-portable]") {
    RS_Ellipse original{nullptr, RS_EllipseData{{0, 0}, {1, 0}, 1, 0, .1, false}};
    auto source = LC_OffsetCurve::create(original, 1, false); REQUIRE(source);
    auto input = seriesInput(*source.entity, true); input.context.nativeSource = true;
    input.context.nativeDefinition = source.entity->definition();
    input.coordinate = {0, 0}; input.distance = 1; input.copies = 1; input.preview = true;
    req::CachedProduce op{input, {}};
    const auto preview = cachedSeries(op); REQUIRE(preview.previewCache);
    op.cache = *preview.previewCache; op.produce.preview = false;
    const auto fallback = cachedSeries(op);
    REQUIRE(fallback.source.status == LC_OffsetSourceStatus::Succeeded);
    CHECK(fallback.source.reusedCacheCopies == 1); CHECK(fallback.source.cacheEngineRuns == 0);
    CHECK(fallback.source.rebaseEngineRuns == 0); CHECK(fallback.source.sampleUnits == 0);
    REQUIRE(fallback.rebases.size() == 1);
    CHECK(fallback.rebases[0].proposalRefusal == LC_OffsetCurveAgain::Refusal::ZeroSum);
    REQUIRE(fallback.keptPlain.size() == 1); CHECK(fallback.keptPlain[0].reason == LC_OffsetPlainReason::ZeroSum);
    for (const auto& output : fallback.outputs) CHECK_FALSE(std::holds_alternative<result::Curve>(output));
}

TEST_CASE("Cached Produce transports real cubic spline records without geometry repair",
          "[lc-offsetcurve][worker-kernel][preview-portable]") {
    RS_Ellipse source{nullptr, RS_EllipseData{{0, 0}, {3, 0}, .7, 0, 1.5, false}};
    req::CachedProduce op{seriesInput(source), {}}; op.produce.copies = 1; op.produce.preview = true;
    op.produce.coordinate = {8, 8};
    const auto first = cachedSeries(op); REQUIRE(first.source.status == LC_OffsetSourceStatus::Succeeded);
    REQUIRE(first.previewCache); REQUIRE(first.previewCache->entries.size() == 1);
    REQUIRE_FALSE(first.previewCache->entries[0].geometry.empty());
    bool spline = false;
    for (const auto& value : first.previewCache->entries[0].geometry) spline |= std::holds_alternative<RS_SplineData>(value);
    REQUIRE(spline);
    op.cache = *first.previewCache; op.produce.limits.maxSamples = 1;
    const auto reused = cachedSeries(op);
    CHECK(reused.source.status == LC_OffsetSourceStatus::Succeeded);
    CHECK(reused.source.cacheEngineRuns == 0); CHECK(reused.source.reusedCacheCopies == 1);
    REQUIRE(reused.outputs.size() == first.outputs.size());
    for (std::size_t i = 0; i < reused.outputs.size(); ++i) {
        const auto& a = std::get<RS_SplineData>(reused.outputs[i]);
        const auto& b = std::get<RS_SplineData>(first.outputs[i]);
        CHECK(req::Values::encodeCache(a) == req::Values::encodeCache(b));
    }
}

TEST_CASE("polyline worker generation and cache admission survive scale and translation",
          "[lc-offsetcurve][worker-kernel][source-cache][polyline][translation]") {
    const double pi = std::acos(-1.0);
    const std::array<LC_OffsetPolylineData, 4> variants{{
        {{0, 0}, {2, 2}, false,
         {RS_ArcData{{1, 0}, 1, pi, 0, false}, RS_LineData{{2, 0}, {2, 2}}}},
        {{2, 2}, {0, 0}, false,
         {RS_LineData{{2, 2}, {2, 0}}, RS_ArcData{{1, 0}, 1, 0, pi, true}}},
        {{0, 0}, {0, 0}, true,
         {RS_LineData{{0, 0}, {2, 0}}, RS_LineData{{2, 0}, {2, 2}},
          RS_LineData{{2, 2}, {0, 2}}, RS_LineData{{0, 2}, {0, 0}}}},
        {{2, 0}, {2, 0}, true,
         {RS_ArcData{{0, 0}, 2, 0, pi / 2, false},
          RS_ArcData{{0, 0}, 2, pi / 2, pi, false},
          RS_ArcData{{0, 0}, 2, pi, 3 * pi / 2, false},
          RS_ArcData{{0, 0}, 2, 3 * pi / 2, 2 * pi, false}}}}};
    constexpr std::array<double, 3> scales{1e-3, 1.0, 1e3};
    constexpr std::array<double, 2> translationRatios{1e4, 1e6};
    constexpr std::array<double, 4> normalizedDistances{-.25, -.1, .1, .25};
    const auto transform = [](LC_OffsetPolylineData data, const double scale,
                              const RS_Vector& translation) {
        const auto point = [&](RS_Vector& value) {
            value.x = value.x * scale + translation.x;
            value.y = value.y * scale + translation.y;
            value.z *= scale;
        };
        point(data.startpoint);
        point(data.endpoint);
        for (auto& segment : data.segments) {
            std::visit([&](auto& value) {
                using T = std::decay_t<decltype(value)>;
                if constexpr (std::is_same_v<T, RS_LineData>) {
                    point(value.startpoint);
                    point(value.endpoint);
                } else {
                    point(value.center);
                    value.radius *= scale;
                }
            }, segment);
        }
        return LC_OffsetSourceGeometry{std::move(data)};
    };
    const auto asSpline = [](const req::Cutter& item) {
        if (const auto* spline = std::get_if<RS_SplineData>(&item)) return *spline;
        const auto& segment = std::get<req::Segment>(item);
        RS_SplineData line{1, false};
        line.controlPoints = {segment.start, segment.end};
        line.knotslist = {0, 0, 1, 1};
        line.weights = {1, 1};
        return line;
    };
    const auto geometryError = [](const RS_Spline& centered, const RS_Spline& translated,
                                  const RS_Vector& translation) {
        double centeredFirst = 0.0, centeredLast = 0.0;
        double translatedFirst = 0.0, translatedLast = 0.0;
        REQUIRE(centered.getParameterDomain(centeredFirst, centeredLast));
        REQUIRE(translated.getParameterDomain(translatedFirst, translatedLast));
        double forward = 0.0;
        double reverse = 0.0;
        for (int i = 0; i <= 128; ++i) {
            const double fraction = static_cast<double>(i) / 128.0;
            const auto local = centered.getPointAt(centeredFirst + (centeredLast - centeredFirst) * fraction);
            const auto worldForward = translated.getPointAt(
                translatedFirst + (translatedLast - translatedFirst) * fraction);
            const auto worldReverse = translated.getPointAt(
                translatedLast - (translatedLast - translatedFirst) * fraction);
            REQUIRE(local.valid);
            REQUIRE(worldForward.valid);
            REQUIRE(worldReverse.valid);
            forward = std::max(forward, std::hypot(
                local.x - (worldForward.x - translation.x),
                local.y - (worldForward.y - translation.y)));
            reverse = std::max(reverse, std::hypot(
                local.x - (worldReverse.x - translation.x),
                local.y - (worldReverse.y - translation.y)));
        }
        return std::min(forward, reverse);
    };
    const auto ulp = [](const double value) {
        const double magnitude = std::abs(value);
        return std::nextafter(magnitude, std::numeric_limits<double>::infinity()) - magnitude;
    };

    std::size_t cells = 0;
    std::size_t generatedCells = 0;
    std::size_t generationRefusedCells = 0;
    std::size_t validatedBranches = 0;
    std::size_t expectedValidatedBranches = 0;
    std::size_t centeredAdmitted = 0;
    std::size_t centeredRefused = 0;
    std::size_t translatedAdmitted = 0;
    std::size_t translatedRefused = 0;
    for (std::size_t variant = 0; variant < variants.size(); ++variant) {
        for (const double scale : scales) {
            for (const double ratio : translationRatios) {
                const RS_Vector translation{ratio * scale, -ratio * scale};
                const auto centeredSource = transform(variants[variant], scale, {});
                const auto translatedSource = transform(variants[variant], scale, translation);
                REQUIRE(centeredSource.isWellFormed());
                REQUIRE(translatedSource.isWellFormed());
                for (const double normalizedDistance : normalizedDistances) {
                    ++cells;
                    CAPTURE(variant, scale, ratio, normalizedDistance);
                    const double signedDistance = normalizedDistance * scale;
                    const LC_OffsetSourceDefinition centeredDefinition{
                        centeredSource, signedDistance, 1e-4, false};
                    const LC_OffsetSourceDefinition translatedDefinition{
                        translatedSource, signedDistance, 1e-4, false};
                    const auto centeredReply = LC_OffsetWorkerKernel::run(
                        {token, req::GenerateSource{centeredDefinition, {}}});
                    const auto translatedReply = LC_OffsetWorkerKernel::run(
                        {token, req::GenerateSource{translatedDefinition, {}}});
                    const auto& centered = native(centeredReply, 7);
                    const auto& translated = native(translatedReply, 7);
                    const int centeredEngine = centered.engineStatus
                        ? static_cast<int>(*centered.engineStatus) : -1;
                    const int translatedEngine = translated.engineStatus
                        ? static_cast<int>(*translated.engineStatus) : -1;
                    CAPTURE(centeredEngine, translatedEngine,
                            static_cast<int>(centered.ordinaryStatus),
                            static_cast<int>(translated.ordinaryStatus),
                            static_cast<int>(centered.refusal),
                            static_cast<int>(translated.refusal));
                    CHECK(centered.engineStatus == translated.engineStatus);
                    CHECK(centered.ordinaryStatus == translated.ordinaryStatus);
                    CHECK(centered.ordinary.size() == translated.ordinary.size());
                    const auto generated = [](const result::Native& output) {
                        return output.engineStatus == LC_CurveOffsetStatus::Ok &&
                            output.ordinaryStatus == result::OrdinaryStatus::Complete &&
                            !output.ordinary.empty();
                    };
                    const bool centeredGenerated = generated(centered);
                    const bool translatedGenerated = generated(translated);
                    REQUIRE(centeredGenerated == translatedGenerated);
                    if (!centeredGenerated) {
                        CHECK(centered.ordinary.empty());
                        CHECK(translated.ordinary.empty());
                        CHECK_FALSE(centered.changed);
                        CHECK_FALSE(translated.changed);
                        ++generationRefusedCells;
                        continue;
                    }
                    ++generatedCells;
                    const auto centeredEntity = centeredSource.makeEntity();
                    const auto translatedEntity = translatedSource.makeEntity();
                    REQUIRE(centeredEntity);
                    REQUIRE(translatedEntity);
                    const auto centeredOptions = LC_OffsetWorkerTolerance::directOptions(
                        centeredDefinition, *centeredEntity);
                    const auto translatedOptions = LC_OffsetWorkerTolerance::directOptions(
                        translatedDefinition, *translatedEntity);
                    REQUIRE(centeredOptions);
                    REQUIRE(translatedOptions);
                    const double geometryLimit = 4.0 * std::max(
                        centeredOptions->tolerance.requestedGeometry,
                        translatedOptions->tolerance.requestedGeometry) +
                        64.0 * std::max(ulp(translation.x), ulp(translation.y));
                    std::vector<RS_Spline> centeredSplines;
                    std::vector<RS_Spline> translatedSplines;
                    for (const auto& item : centered.ordinary)
                        centeredSplines.emplace_back(nullptr, asSpline(item));
                    for (const auto& item : translated.ordinary)
                        translatedSplines.emplace_back(nullptr, asSpline(item));
                    std::vector<bool> matched(translatedSplines.size(), false);
                    for (const auto& local : centeredSplines) {
                        double best = std::numeric_limits<double>::infinity();
                        std::size_t bestIndex = translatedSplines.size();
                        for (std::size_t candidate = 0; candidate < translatedSplines.size(); ++candidate) {
                            if (matched[candidate]) continue;
                            const double error = geometryError(local, translatedSplines[candidate], translation);
                            if (error < best) {
                                best = error;
                                bestIndex = candidate;
                            }
                        }
                        REQUIRE(bestIndex < translatedSplines.size());
                        matched[bestIndex] = true;
                        CHECK(best <= geometryLimit);
                    }

                    const auto validate = [&](const result::Native& output,
                                              const LC_OffsetSourceDefinition& definition) {
                        expectedValidatedBranches += output.ordinary.size();
                        bool admitted = false;
                        for (const auto& item : output.ordinary) {
                            auto cache = asSpline(item);
                            const auto fingerprint = RS_Spline::geometryFingerprint(cache);
                            const auto reply = LC_OffsetWorkerKernel::run(
                                {token, req::ValidateSourceCache{cache, definition, {}}});
                            const auto& checked = native(reply, 8);
                            ++validatedBranches;
                            if (output.ordinary.size() == 1) {
                                CHECK(checked.refusal == LC_OffsetCurveRefusal::None);
                                CHECK(checked.changed);
                                admitted = checked.refusal == LC_OffsetCurveRefusal::None &&
                                    checked.changed;
                            } else {
                                CHECK(checked.refusal == LC_OffsetCurveRefusal::NotCacheRepresentable);
                                CHECK_FALSE(checked.changed);
                                CHECK(RS_Spline::geometryFingerprint(cache) == fingerprint);
                            }
                        }
                        return admitted;
                    };
                    if (validate(centered, centeredDefinition)) ++centeredAdmitted;
                    else ++centeredRefused;
                    if (validate(translated, translatedDefinition)) ++translatedAdmitted;
                    else ++translatedRefused;
                }
            }
        }
    }
    CHECK(cells == variants.size() * scales.size() * translationRatios.size() * normalizedDistances.size());
    CHECK(generatedCells + generationRefusedCells == cells);
    CHECK(centeredAdmitted + centeredRefused == generatedCells);
    CHECK(translatedAdmitted + translatedRefused == generatedCells);
    CHECK(validatedBranches == expectedValidatedBranches);
    CHECK(generatedCells > 0);
    CHECK(centeredAdmitted > 0);
    CHECK(translatedAdmitted > 0);
}
