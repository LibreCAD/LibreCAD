/*
 * This file is part of LibreCAD, a 2D CAD program.
 * Copyright (C) 2026 LibreCAD.org
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include <catch2/catch_test_macros.hpp>

#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>
#include <memory>
#include <optional>
#include <vector>

#include "drw_entities.h"
#include "lc_offsetcurvecarrier.h"

namespace carrier = LC_OffsetCurveCarrier;
namespace geo = LC_EllipseOffsetGeometry;

namespace {
std::uint64_t bits(const double value) {
    std::uint64_t result;
    static_assert(sizeof(result) == sizeof(value));
    std::memcpy(&result, &value, sizeof(result));
    return result;
}

void samePoint(const DRW_Coord &actual, const DRW_Coord &expected) {
    CHECK(bits(actual.x) == bits(expected.x));
    CHECK(bits(actual.y) == bits(expected.y));
    CHECK(bits(actual.z) == bits(expected.z));
}

void sameDoubles(const std::vector<double> &actual, const std::vector<double> &expected) {
    REQUIRE(actual.size() == expected.size());
    for (std::size_t i = 0; i < expected.size(); ++i) {
        CAPTURE(i);
        CHECK(bits(actual[i]) == bits(expected[i]));
    }
}

void counts(DRW_Spline &raw) {
    raw.ncontrol = static_cast<std::int32_t>(raw.controllist.size());
    raw.nknots = static_cast<std::int32_t>(raw.knotslist.size());
    raw.nfit = static_cast<std::int32_t>(raw.fitlist.size());
}

// Hand-authored raw chains, independent of the filter, offset producer and writer.
DRW_Spline line(const bool closed = false) {
    DRW_Spline raw;
    raw.degree = 1;
    raw.flags = closed ? 9 : 8;
    if (closed) {
        for (const auto &point : std::array<DRW_Coord, 4>{{{0, 0}, {2, 0}, {1, 1}, {0, 0, -0.0}}})
            raw.controllist.push_back(std::make_shared<DRW_Coord>(point));
        raw.knotslist = {0, 0, 1, 2, 3, 3};
    } else {
        for (const auto &point : std::array<DRW_Coord, 3>{{{0, 0}, {2, 1}, {4, 0}}})
            raw.controllist.push_back(std::make_shared<DRW_Coord>(point));
        raw.knotslist = {-2, -2, 0.125, 3.5, 3.5};
    }
    counts(raw);
    return raw;
}

DRW_Spline cubic(const bool closed = false) {
    DRW_Spline raw;
    raw.degree = 3;
    raw.flags = closed ? 9 : 8;
    const std::array<DRW_Coord, 7> controls = closed ?
        std::array<DRW_Coord, 7>{{{0, 0}, {2, 0}, {2, 2}, {0, 2}, {-2, 2}, {-2, 0}, {0, 0}}} :
        std::array<DRW_Coord, 7>{{{0, 0}, {1, 0}, {2, 1}, {3, 1}, {4, 1}, {5, 0}, {6, 0}}};
    for (const auto &point : controls) raw.controllist.push_back(std::make_shared<DRW_Coord>(point));
    raw.knotslist = {-2, -2, -2, -2, 0.125, 0.125, 0.125, 3.5, 3.5, 3.5, 3.5};
    counts(raw);
    return raw;
}

struct RawState {
    DRW::ETYPE type;
    int degree, flags;
    std::int32_t scenario, method, parameter, controls, knots, fits;
    DRW_Coord normal, start, end;
    double knotTolerance, controlTolerance, fitTolerance;
    std::vector<double> knotValues, weights;
    const double *knotStorage, *weightStorage;
    std::vector<const DRW_Coord *> controlPointers, fitPointers;
    std::vector<std::optional<DRW_Coord>> controlValues, fitValues;

    explicit RawState(const DRW_Spline &raw) : type(raw.eType), degree(raw.degree), flags(raw.flags),
        scenario(raw.m_scenario), method(raw.m_splineFlags1), parameter(raw.m_knotParam),
        controls(raw.ncontrol), knots(raw.nknots), fits(raw.nfit), normal(raw.normalVec),
        start(raw.tgStart), end(raw.tgEnd), knotTolerance(raw.tolknot), controlTolerance(raw.tolcontrol),
        fitTolerance(raw.tolfit), knotValues(raw.knotslist), weights(raw.weightlist),
        knotStorage(raw.knotslist.data()), weightStorage(raw.weightlist.data()) {
        const auto capture = [](const auto &source, auto &pointers, auto &values) {
            for (const auto &point : source) {
                pointers.push_back(point.get());
                values.push_back(point ? std::optional<DRW_Coord>{*point} : std::nullopt);
            }
        };
        capture(raw.controllist, controlPointers, controlValues);
        capture(raw.fitlist, fitPointers, fitValues);
    }

    void check(const DRW_Spline &raw) const {
        CHECK(raw.eType == type); CHECK(raw.degree == degree); CHECK(raw.flags == flags);
        CHECK(raw.m_scenario == scenario); CHECK(raw.m_splineFlags1 == method); CHECK(raw.m_knotParam == parameter);
        CHECK(raw.ncontrol == controls); CHECK(raw.nknots == knots); CHECK(raw.nfit == fits);
        samePoint(raw.normalVec, normal); samePoint(raw.tgStart, start); samePoint(raw.tgEnd, end);
        CHECK(bits(raw.tolknot) == bits(knotTolerance)); CHECK(bits(raw.tolcontrol) == bits(controlTolerance));
        CHECK(bits(raw.tolfit) == bits(fitTolerance));
        CHECK(raw.knotslist.data() == knotStorage); CHECK(raw.weightlist.data() == weightStorage);
        sameDoubles(raw.knotslist, knotValues); sameDoubles(raw.weightlist, weights);
        const auto checkPoints = [](const auto &source, const auto &pointers, const auto &values) {
            REQUIRE(source.size() == pointers.size());
            for (std::size_t i = 0; i < source.size(); ++i) {
                CAPTURE(i);
                CHECK(source[i].get() == pointers[i]);
                if (values[i]) { REQUIRE(source[i]); samePoint(*source[i], *values[i]); }
                else CHECK_FALSE(source[i]);
            }
        };
        checkPoints(raw.controllist, controlPointers, controlValues);
        checkPoints(raw.fitlist, fitPointers, fitValues);
    }
};

void refusal(const DRW_Spline &raw, const carrier::Refusal expected,
             const std::optional<std::size_t> index = {}, const LC_OffsetCheckLimits &limits = {}) {
    const RawState before{raw};
    const auto result = carrier::fromRawSpline(raw, limits);
    CHECK_FALSE(result); CHECK_FALSE(result.data); CHECK(result.refusal == expected); CHECK(result.item == index);
    before.check(raw);
}

void expected(const carrier::Result &result, const std::size_t degree,
              const std::vector<RS_Vector> &controls, const std::vector<double> &knots,
              const bool closed = false, const bool periodic = false) {
    REQUIRE(result); CHECK(result.refusal == carrier::Refusal::None); CHECK_FALSE(result.item); REQUIRE(result.data);
    const auto &data = *result.data;
    CHECK(data.degree == degree); CHECK(data.type == RS_SplineData::SplineType::ClampedOpen);
    CHECK(data.savedOpenType == RS_SplineData::SplineType::ClampedOpen); CHECK(data.savedOpenKnots.empty()); CHECK(data.fitPoints.empty());
    CHECK(data.m_closedFlag == closed); CHECK(data.m_periodicFlag == periodic);
    REQUIRE(data.controlPoints.size() == controls.size());
    for (std::size_t i = 0; i < controls.size(); ++i) {
        CAPTURE(i); CHECK(data.controlPoints[i].valid);
        CHECK(bits(data.controlPoints[i].x) == bits(controls[i].x));
        CHECK(bits(data.controlPoints[i].y) == bits(controls[i].y));
        CHECK(bits(data.controlPoints[i].z) == bits(controls[i].z));
    }
    sameDoubles(data.knotslist, knots);
    CHECK(data.weights == std::vector<double>(controls.size(), 1.0));
    geo::WorkBudget budget;
    const auto layout = geo::inspectCacheLayout(data, budget);
    CHECK(layout.status == geo::CacheLayoutStatus::Ok);
    CHECK(layout.spanCount() == (controls.size() - 1) / degree);
}
} // namespace

TEST_CASE("raw offset carrier keeps the exact open degree-one chain with implied unit weights", "[offset-carrier]") {
    auto raw = line(); const RawState before{raw};
    expected(carrier::fromRawSpline(raw), 1, {{0, 0}, {2, 1}, {4, 0}}, {-2, -2, 0.125, 3.5, 3.5});
    before.check(raw);
}

TEST_CASE("raw offset carrier keeps the exact open cubic Bezier chain with explicit unit weights", "[offset-carrier]") {
    auto raw = cubic(); raw.weightlist.assign(7, 1); raw.flags |= 4 | 16;
    const RawState before{raw};
    expected(carrier::fromRawSpline(raw), 3, {{0, 0}, {1, 0}, {2, 1}, {3, 1}, {4, 1}, {5, 0}, {6, 0}},
        {-2, -2, -2, -2, 0.125, 0.125, 0.125, 3.5, 3.5, 3.5, 3.5});
    before.check(raw);
}

TEST_CASE("raw offset carrier retains closed and periodic bits without wrapping either degree", "[offset-carrier]") {
    for (const auto flags : {1, 3}) {
        auto linear = line(true); linear.flags = flags;
        expected(carrier::fromRawSpline(linear), 1, {{0, 0}, {2, 0}, {1, 1}, {0, 0, -0.0}},
            {0, 0, 1, 2, 3, 3}, true, flags == 3);
        auto bezier = cubic(true); bezier.flags = flags; bezier.m_scenario = 1; bezier.m_splineFlags1 = 4;
        expected(carrier::fromRawSpline(bezier), 3, {{0, 0}, {2, 0}, {2, 2}, {0, 2}, {-2, 2}, {-2, 0}, {0, 0}},
            {-2, -2, -2, -2, 0.125, 0.125, 0.125, 3.5, 3.5, 3.5, 3.5}, true, flags == 3);
    }
}

TEST_CASE("raw offset carrier accepts the zero normal sentinel and either compatible Z orientation", "[offset-carrier]") {
    for (const auto normalZ : {0.0, -0.0, 1.0, -1.0, 2.0, std::numeric_limits<double>::max(),
                              std::numeric_limits<double>::denorm_min()}) {
        auto raw = line(); raw.normalVec = {0, 0, normalZ};
        const auto result = carrier::fromRawSpline(raw); REQUIRE(result); CHECK(result.data->controlPoints.size() == 3);
    }
}

TEST_CASE("raw offset carrier rejects HELIX and every other entity discriminator", "[offset-carrier]") {
    DRW_Helix helix;
    refusal(helix, carrier::Refusal::NotSpline);
    for (const auto type : {DRW::HELIX, DRW::ARC, DRW::UNKNOWN}) {
        auto raw = cubic(); raw.eType = type; refusal(raw, carrier::Refusal::NotSpline);
    }
}

TEST_CASE("raw offset carrier rejects every unsupported degree before derived arithmetic", "[offset-carrier]") {
    for (const auto degree : {-1, 0, 2, 4, std::numeric_limits<int>::max()}) {
        auto raw = cubic(); raw.degree = degree; refusal(raw, carrier::Refusal::UnsupportedDegree);
    }
}

TEST_CASE("raw offset carrier checks original declared control counts", "[offset-carrier]") {
    for (const auto count : {-1, 0, 2, 4, std::numeric_limits<std::int32_t>::max()}) {
        auto raw = line(); raw.ncontrol = count; refusal(raw, carrier::Refusal::DeclaredCountMismatch);
    }
}

TEST_CASE("raw offset carrier checks original declared knot counts", "[offset-carrier]") {
    for (const auto count : {-1, 0, 4, 6, std::numeric_limits<std::int32_t>::max()}) {
        auto raw = line(); raw.nknots = count; refusal(raw, carrier::Refusal::DeclaredCountMismatch);
    }
}

TEST_CASE("raw offset carrier checks original declared fit counts even with control data", "[offset-carrier]") {
    for (const auto count : {-1, 1, std::numeric_limits<std::int32_t>::max()}) {
        auto raw = line(); raw.nfit = count; refusal(raw, carrier::Refusal::DeclaredCountMismatch);
    }
    auto raw = line(); raw.fitlist.push_back(std::make_shared<DRW_Coord>(1, 1));
    refusal(raw, carrier::Refusal::DeclaredCountMismatch);
}

TEST_CASE("raw offset carrier rejects null control entries without skipping or renumbering", "[offset-carrier]") {
    auto raw = line(); raw.controllist[1].reset(); refusal(raw, carrier::Refusal::NullControl, 1);
}

TEST_CASE("raw offset carrier rejects nonfinite original XY coordinates", "[offset-carrier]") {
    for (const auto value : {std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::infinity(),
                            -std::numeric_limits<double>::infinity()}) {
        auto x = line(); x.controllist[1]->x = value; refusal(x, carrier::Refusal::NonFiniteControl, 1);
        auto y = line(); y.controllist[1]->y = value; refusal(y, carrier::Refusal::NonFiniteControl, 1);
    }
}

TEST_CASE("raw offset carrier rejects nonfinite original Z before any projection", "[offset-carrier]") {
    for (const auto value : {std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::infinity(),
                            -std::numeric_limits<double>::infinity()}) {
        auto raw = cubic(); raw.controllist[2]->z = value; refusal(raw, carrier::Refusal::NonFiniteControl, 2);
    }
}

TEST_CASE("raw offset carrier requires exact Z zero even with a planar flag or unspecified normal", "[offset-carrier]") {
    for (const auto normal : {DRW_Coord{0, 0, 0}, DRW_Coord{0, 0, 1}})
        for (const auto z : {std::numeric_limits<double>::denorm_min(), -std::numeric_limits<double>::denorm_min(), 1e-20, 1.0}) {
            auto raw = line(); raw.normalVec = normal; raw.controllist[2]->z = z;
            refusal(raw, carrier::Refusal::Non2DControl, 2);
        }
}

TEST_CASE("raw offset carrier refuses incompatible and nonfinite explicit normals", "[offset-carrier]") {
    for (const auto normal : {DRW_Coord{1, 0, 0}, DRW_Coord{0, 1, 1},
        DRW_Coord{std::numeric_limits<double>::denorm_min(), 0, 1},
        DRW_Coord{0, 0, std::numeric_limits<double>::infinity()},
        DRW_Coord{0, std::numeric_limits<double>::quiet_NaN(), 0}}) {
        auto raw = line(); raw.normalVec = normal; refusal(raw, carrier::Refusal::IncompatibleNormal);
    }
}

TEST_CASE("raw offset carrier refuses missing short and surplus knots without synthesis", "[offset-carrier]") {
    auto missing = cubic(); missing.knotslist.clear(); counts(missing); refusal(missing, carrier::Refusal::InvalidKnots);
    auto shortKnots = cubic(); shortKnots.knotslist.pop_back(); counts(shortKnots); refusal(shortKnots, carrier::Refusal::InvalidKnots);
    auto surplus = cubic(); surplus.knotslist.push_back(3.5); counts(surplus); refusal(surplus, carrier::Refusal::InvalidKnots);
}

TEST_CASE("raw offset carrier requires a genuine complete line or cubic control layout", "[offset-carrier]") {
    for (const auto degree : {1, 3}) {
        DRW_Spline empty; empty.degree = degree; refusal(empty, carrier::Refusal::InvalidLayout);
    }
    auto tooFew = line(); tooFew.controllist.resize(1); tooFew.knotslist = {0, 0, 0}; counts(tooFew);
    refusal(tooFew, carrier::Refusal::InvalidLayout);
    auto incompleteCubic = cubic(); incompleteCubic.controllist.resize(5); incompleteCubic.knotslist.resize(9); counts(incompleteCubic);
    refusal(incompleteCubic, carrier::Refusal::InvalidLayout);
}

TEST_CASE("raw offset carrier rejects unclamped and incorrectly repeated knots rather than clamping", "[offset-carrier]") {
    auto standard = cubic();
    for (std::size_t i = 0; i < standard.knotslist.size(); ++i) standard.knotslist[i] = static_cast<double>(i);
    refusal(standard, carrier::Refusal::InvalidKnots, 1);
    auto wrongInterior = cubic(); wrongInterior.knotslist[5] = std::nextafter(0.125, 1.0);
    refusal(wrongInterior, carrier::Refusal::InvalidKnots, 5);
    auto wrongEnd = line(); wrongEnd.knotslist.back() = 4;
    refusal(wrongEnd, carrier::Refusal::InvalidKnots, 4);
}

TEST_CASE("raw offset carrier requires finite strictly ordered nonzero knot domains", "[offset-carrier]") {
    for (const auto value : {-3.0, -2.0, 4.0, std::numeric_limits<double>::quiet_NaN(),
                            std::numeric_limits<double>::infinity()}) {
        auto raw = line(); raw.knotslist[2] = value;
        const auto index = value == 4 ? 3u : 2u;
        refusal(raw, carrier::Refusal::InvalidKnots, index);
    }
    auto collapsed = line(); collapsed.knotslist.assign(5, 0); refusal(collapsed, carrier::Refusal::InvalidKnots, 2);
}

TEST_CASE("raw offset carrier never rounds valid raw controls or closely spaced knots", "[offset-carrier]") {
    auto raw = line(); raw.tolknot = 1e9; raw.tolcontrol = 1e9;
    raw.knotslist = {-0.0, -0.0, std::numeric_limits<double>::denorm_min(), 1, 1};
    raw.controllist[0]->x = std::nextafter(1.0, 2.0); raw.controllist[0]->y = -0.0; raw.controllist[0]->z = -0.0;
    const RawState before{raw};
    expected(carrier::fromRawSpline(raw), 1, {{std::nextafter(1.0, 2.0), -0.0, -0.0}, {2, 1}, {4, 0}},
        {-0.0, -0.0, std::numeric_limits<double>::denorm_min(), 1, 1});
    before.check(raw);
}

TEST_CASE("raw offset carrier rejects partial and surplus weights without filling a partial list", "[offset-carrier]") {
    for (const auto size : {1u, 2u, 4u}) {
        auto raw = line(); raw.weightlist.assign(size, 1.0); refusal(raw, carrier::Refusal::InvalidWeights);
    }
}

TEST_CASE("raw offset carrier accepts absent or complete unit weights even with rational markers", "[offset-carrier]") {
    for (const auto flags : {0, 4, 8, 16, 28}) for (const auto explicitWeights : {false, true}) {
        auto raw = line(); raw.flags = flags;
        if (explicitWeights) raw.weightlist.assign(3, 1.0);
        const auto result = carrier::fromRawSpline(raw); REQUIRE(result);
        CHECK(result.data->weights == std::vector<double>(3, 1.0));
    }
}

TEST_CASE("raw offset carrier rejects every nonunit and nonfinite weight exactly", "[offset-carrier]") {
    for (const auto weight : {0.0, -1.0, 0.5, std::nextafter(1.0, 0.0), std::nextafter(1.0, 2.0),
                             std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::infinity()}) {
        auto raw = cubic(); raw.weightlist.assign(7, 1); raw.weightlist[4] = weight;
        refusal(raw, carrier::Refusal::InvalidWeights, 4);
    }
}

TEST_CASE("raw offset carrier refuses fit records instead of silently dropping mixed data", "[offset-carrier]") {
    auto raw = cubic(); raw.fitlist.push_back(std::make_shared<DRW_Coord>(2, 1)); counts(raw);
    refusal(raw, carrier::Refusal::FitData);
    raw.fitlist[0].reset(); refusal(raw, carrier::Refusal::FitData);
}

TEST_CASE("raw offset carrier refuses fit scenarios flags and tangent constraints", "[offset-carrier]") {
    auto scenario = cubic(); scenario.m_scenario = 2; refusal(scenario, carrier::Refusal::FitData);
    auto method = cubic(); method.m_splineFlags1 = 1; refusal(method, carrier::Refusal::UnsupportedMode);
    for (const auto fitFlag : {32, 1024}) {
        auto raw = cubic(); raw.flags |= fitFlag; refusal(raw, carrier::Refusal::UnsupportedMode);
    }
    auto start = cubic(); start.tgStart = {1, 0, 0}; refusal(start, carrier::Refusal::FitData);
    auto end = cubic(); end.tgEnd = {0, std::numeric_limits<double>::denorm_min(), 0}; refusal(end, carrier::Refusal::FitData);
}

TEST_CASE("raw offset carrier refuses unknown modes and contradictory DWG closure state", "[offset-carrier]") {
    for (const auto scenario : {-1, 3, std::numeric_limits<std::int32_t>::max()}) {
        auto raw = cubic(); raw.m_scenario = scenario; refusal(raw, carrier::Refusal::UnsupportedMode);
    }
    for (const auto flags : {-1, 64, 0x10000}) {
        auto raw = cubic(); raw.flags = flags; refusal(raw, carrier::Refusal::UnsupportedMode);
    }
    for (const auto flags : {-1, 2, 16}) {
        auto raw = cubic(); raw.m_splineFlags1 = flags; refusal(raw, carrier::Refusal::UnsupportedMode);
    }
    auto contradictory = cubic(); contradictory.m_splineFlags1 = 4;
    refusal(contradictory, carrier::Refusal::UnsupportedMode);
    for (const auto parameter : {-1, 3, std::numeric_limits<std::int32_t>::max()}) {
        auto raw = cubic(); raw.m_knotParam = parameter; refusal(raw, carrier::Refusal::UnsupportedMode);
    }
    for (const auto parameter : {0, 1, 2}) {
        auto raw = cubic(); raw.m_knotParam = parameter; raw.m_splineFlags1 = 9;
        refusal(raw, carrier::Refusal::UnsupportedMode);
    }
}

TEST_CASE("raw offset carrier keeps explicit control knots regardless of known parameter hints", "[offset-carrier]") {
    for (const auto scenario : {0, 1}) for (const auto parameter : {0, 1, 2, 15}) for (const auto flags : {0, 8}) {
        auto raw = cubic(); raw.m_scenario = scenario; raw.m_knotParam = parameter; raw.m_splineFlags1 = flags;
        const auto result = carrier::fromRawSpline(raw); REQUIRE(result); sameDoubles(result.data->knotslist, raw.knotslist);
    }
    auto custom = cubic(); custom.m_scenario = 1; custom.m_splineFlags1 = 8;
    REQUIRE(carrier::fromRawSpline(custom));
}

TEST_CASE("raw offset carrier rejects nonfinite auxiliary data and invalid tolerances", "[offset-carrier]") {
    for (const auto value : {-1.0, std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::infinity()}) {
        auto knots = line(); knots.tolknot = value; refusal(knots, carrier::Refusal::InvalidAuxiliaryData);
        auto controls = line(); controls.tolcontrol = value; refusal(controls, carrier::Refusal::InvalidAuxiliaryData);
        auto fits = line(); fits.tolfit = value; refusal(fits, carrier::Refusal::InvalidAuxiliaryData);
    }
    auto start = line(); start.tgStart.z = std::numeric_limits<double>::quiet_NaN();
    refusal(start, carrier::Refusal::InvalidAuxiliaryData);
    auto end = line(); end.tgEnd.x = std::numeric_limits<double>::infinity();
    refusal(end, carrier::Refusal::InvalidAuxiliaryData);
}

TEST_CASE("raw offset carrier refuses closure flags that require saved-open or wrapped synthesis", "[offset-carrier]") {
    for (const auto flags : {1, 3}) {
        auto raw = line(); raw.flags = flags; refusal(raw, carrier::Refusal::RepairDependentClosure);
    }
    auto almost = cubic(true); almost.controllist.back()->y = std::numeric_limits<double>::denorm_min();
    refusal(almost, carrier::Refusal::RepairDependentClosure);
    auto periodicOnly = line(true); periodicOnly.flags = 2; refusal(periodicOnly, carrier::Refusal::RepairDependentClosure);
    auto unflagged = line(true); unflagged.flags = 0;
    const auto result = carrier::fromRawSpline(unflagged); REQUIRE(result);
    CHECK_FALSE(result.data->m_closedFlag); CHECK_FALSE(result.data->m_periodicFlag);
}

TEST_CASE("raw offset carrier obeys exact and one-less C storage caps without adaptive work", "[offset-carrier]") {
    auto raw = line(); LC_OffsetCheckLimits limits; limits.maxVertices = 3; limits.maxCacheSpans = 2;
    REQUIRE(carrier::fromRawSpline(raw, limits));
    limits.maxVertices = 2; refusal(raw, carrier::Refusal::ResourceLimit, {}, limits);
    limits.maxVertices = 3; limits.maxCacheSpans = 1; refusal(raw, carrier::Refusal::ResourceLimit, {}, limits);
    limits.maxCacheSpans = 0; refusal(raw, carrier::Refusal::ResourceLimit, {}, limits);
    limits.maxCacheSpans = 2; limits.maxVertices = 0; refusal(raw, carrier::Refusal::ResourceLimit, {}, limits);
    limits.maxVertices = 3; limits.maxDepth = 0; limits.maxDistancePairs = 0; limits.maxOrderingComparisons = 0;
    REQUIRE(carrier::fromRawSpline(raw, limits));
}

TEST_CASE("raw offset carrier preflights all storage before walking hostile arrays", "[offset-carrier]") {
    LC_OffsetCheckLimits limits; limits.maxVertices = 2;
    auto controls = line(); controls.controllist[0].reset(); refusal(controls, carrier::Refusal::ResourceLimit, {}, limits);
    auto knots = line(); knots.controllist.resize(2); knots.knotslist[0] = std::numeric_limits<double>::quiet_NaN(); counts(knots);
    refusal(knots, carrier::Refusal::ResourceLimit, {}, limits);
    auto weights = line(); weights.controllist.resize(2); weights.knotslist = {0, 0, 1, 1}; weights.weightlist = {1, 1, 1}; counts(weights);
    refusal(weights, carrier::Refusal::ResourceLimit, {}, limits);
    auto fits = line(); fits.controllist.resize(2); fits.knotslist = {0, 0, 1, 1}; fits.fitlist.resize(3); counts(fits);
    refusal(fits, carrier::Refusal::ResourceLimit, {}, limits);
}

TEST_CASE("raw offset carrier cannot raise C hard caps with SIZE_MAX caller limits", "[offset-carrier]") {
    constexpr LC_OffsetCheckLimits hard{};
    DRW_Spline raw; raw.degree = 1;
    raw.controllist.resize(hard.maxCacheSpans + 2); raw.knotslist.resize(raw.controllist.size() + 2); counts(raw);
    LC_OffsetCheckLimits raised; raised.maxVertices = std::numeric_limits<std::size_t>::max();
    raised.maxCacheSpans = std::numeric_limits<std::size_t>::max();
    refusal(raw, carrier::Refusal::ResourceLimit, {}, raised);
}

TEST_CASE("raw offset carrier also bounds original control and knot storage at C hard limits", "[offset-carrier]") {
    constexpr LC_OffsetCheckLimits hard{};
    LC_OffsetCheckLimits raised; raised.maxVertices = std::numeric_limits<std::size_t>::max();
    raised.maxCacheSpans = std::numeric_limits<std::size_t>::max();
    // Bounded ~1 MiB null-pointer input, not an INT_MAX allocation. If inspected
    // before the hard cap, the first element would be a different typed refusal.
    DRW_Spline controls; controls.degree = 1; controls.controllist.resize(hard.maxVertices + 1); counts(controls);
    const auto controlsResult = carrier::fromRawSpline(controls, raised);
    CHECK_FALSE(controlsResult); CHECK_FALSE(controlsResult.data); CHECK_FALSE(controlsResult.item);
    CHECK(controlsResult.refusal == carrier::Refusal::ResourceLimit); CHECK_FALSE(controls.controllist.front());
    auto knots = line(); knots.knotslist.resize(hard.maxVertices + 3); knots.knotslist[0] = std::numeric_limits<double>::quiet_NaN(); counts(knots);
    const auto knotsResult = carrier::fromRawSpline(knots, raised);
    CHECK_FALSE(knotsResult); CHECK_FALSE(knotsResult.data); CHECK_FALSE(knotsResult.item);
    CHECK(knotsResult.refusal == carrier::Refusal::ResourceLimit); CHECK(std::isnan(knots.knotslist[0]));
}

TEST_CASE("raw offset carrier values outlive the source and share no coordinate or knot storage", "[offset-carrier]") {
    auto raw = cubic(); const RawState before{raw}; auto result = carrier::fromRawSpline(raw); REQUIRE(result); before.check(raw);
    raw.controllist[0]->x = 123; raw.knotslist[0] = -7;
    CHECK(result.data->controlPoints[0].x == 0); CHECK(result.data->knotslist[0] == -2);
    result.data->controlPoints[0].y = 321; result.data->knotslist[1] = -9;
    CHECK(raw.controllist[0]->y == 0); CHECK(raw.knotslist[1] == -2);
    auto retained = [] { return carrier::fromRawSpline(line()); }();
    expected(retained, 1, {{0, 0}, {2, 1}, {4, 0}}, {-2, -2, 0.125, 3.5, 3.5});
}
