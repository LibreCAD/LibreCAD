/*
 * This file is part of LibreCAD, a 2D CAD program.
 * Copyright (C) 2026 LibreCAD.org
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "drw_base.h"
#include "lc_offsetcurve.h"
#include "rs_line.h"

namespace {
namespace geo = LC_EllipseOffsetGeometry;

std::uint64_t bits(const double value) {
    std::uint64_t result;
    static_assert(sizeof(result) == sizeof(value));
    std::memcpy(&result, &value, sizeof(result));
    return result;
}

bool identical(const RS_Vector& a, const RS_Vector& b) {
    return a.valid == b.valid && bits(a.x) == bits(b.x) && bits(a.y) == bits(b.y) && bits(a.z) == bits(b.z);
}

void checkPoints(const std::vector<RS_Vector>& actual, const std::vector<RS_Vector>& expected) {
    REQUIRE(actual.size() == expected.size());
    for (std::size_t i = 0; i < actual.size(); ++i) { CAPTURE(i); CHECK(identical(actual[i], expected[i])); }
}

void checkDefinition(const LC_EllipseOffsetDefinition& a, const LC_EllipseOffsetDefinition& b) {
    CHECK(identical(a.basis.center, b.basis.center)); CHECK(identical(a.basis.majorP, b.basis.majorP));
    CHECK(bits(a.basis.ratio) == bits(b.basis.ratio)); CHECK(bits(a.basis.angle1) == bits(b.basis.angle1));
    CHECK(bits(a.basis.angle2) == bits(b.basis.angle2)); CHECK(a.basis.reversed == b.basis.reversed);
    CHECK(a.basis.isArc == b.basis.isArc); CHECK(bits(a.basis.angleDegrees) == bits(b.basis.angleDegrees));
    CHECK(bits(a.basis.startAngleDegrees) == bits(b.basis.startAngleDegrees));
    CHECK(bits(a.basis.otherAngleDegrees) == bits(b.basis.otherAngleDegrees));
    CHECK(bits(a.basis.angularLength) == bits(b.basis.angularLength));
    CHECK(bits(a.distance) == bits(b.distance)); CHECK(bits(a.relTolerance) == bits(b.relTolerance));
    CHECK(a.towardCentre == b.towardCentre); CHECK(a.cacheReversed == b.cacheReversed);
}

void checkData(const RS_SplineData& a, const RS_SplineData& b) {
    CHECK(a.degree == b.degree); CHECK(a.type == b.type); CHECK(a.savedOpenType == b.savedOpenType);
    CHECK(a.m_closedFlag == b.m_closedFlag); CHECK(a.m_periodicFlag == b.m_periodicFlag);
    checkPoints(a.controlPoints, b.controlPoints); checkPoints(a.fitPoints, b.fitPoints);
    const auto doubles = [](const std::vector<double>& x, const std::vector<double>& y) {
        REQUIRE(x.size() == y.size());
        for (std::size_t i = 0; i < x.size(); ++i) CHECK(bits(x[i]) == bits(y[i]));
    };
    doubles(a.knotslist, b.knotslist); doubles(a.weights, b.weights); doubles(a.savedOpenKnots, b.savedOpenKnots);
}

void checkTail(const LC_OffsetCurve::OwnedTail& a, const LC_OffsetCurve::OwnedTail& b) {
    REQUIRE(a.size() == b.size());
    for (std::size_t i = 0; i < a.size(); ++i) {
        CHECK(a[i].code == b[i].code); REQUIRE(a[i].value.index() == b[i].value.index());
        switch (a[i].value.index()) {
        case 0: CHECK(std::get<QString>(a[i].value) == std::get<QString>(b[i].value)); break;
        case 1: CHECK(identical(std::get<RS_Vector>(a[i].value), std::get<RS_Vector>(b[i].value))); break;
        case 2: CHECK(bits(std::get<double>(a[i].value)) == bits(std::get<double>(b[i].value))); break;
        case 3: CHECK(std::get<std::int16_t>(a[i].value) == std::get<std::int16_t>(b[i].value)); break;
        case 4: CHECK(std::get<std::int32_t>(a[i].value) == std::get<std::int32_t>(b[i].value)); break;
        default: FAIL("unexpected owned extension type");
        }
    }
}

void decorate(LC_OffsetCurve& curve) {
    curve.setFlag(RS2::FlagSelected); curve.setFlag(RS2::FlagTransparent);
    curve.setPen(RS_Pen{RS_Color{23, 47, 71}, RS2::Width11, RS2::SolidLine});
    curve.setSourceHandle(701); curve.setXDictHandle(702); curve.setReactorHandles({703, 704});
    curve.setMaterialHandle(705); curve.setPlotStyleHandle(706); curve.setShadowMode(2);
    curve.setShadowHandle(707); curve.setVisualStyleHandles(708, 709, 710); curve.setOwnerLineage(711);
    curve.setUserDefVar("D-grips", "query must retain this value");
    curve.setDrwExtData({std::make_shared<DRW_Variant>(1001, std::string("D_GRIPS_TEST")),
        std::make_shared<DRW_Variant>(1000, std::string("foreign payload"))});
    REQUIRE(curve.setOwnedExtensions(0x30, {{1002, QStringLiteral("{")}, {1000, QStringLiteral("retained")},
        {1010, RS_Vector{3, 4}}, {1040, 0.125}, {1070, std::int16_t{3}},
        {1071, std::int32_t{765432}}, {1002, QStringLiteral("}")}}));
}

struct State {
    RS_SplineData data;
    LC_EllipseOffsetDefinition definition;
    std::vector<RS_Vector> grips;
    LC_OffsetCurve::OwnedTail tail;
    const RS_Vector* controls;
    const RS_Vector* gripStorage;
    const double* knots;
    const double* weights;
    std::uint64_t fingerprint;
    double tolerance;
    unsigned flags, ownedFlags;
    bool defined, consistent, whole, closed;
    RS_Vector middle, minimum, maximum;
    RS_EntityContainer* parent;
    RS_Layer* layer;
    RS_Pen pen;
    unsigned long long id, lineage, ownerLineage;
    std::array<quint32, 8> handles;
    int shadowMode;
    std::vector<quint32> reactors;
    QString userValue;
    std::vector<std::pair<int, std::string>> xdata;
    std::vector<const RS_Entity*> children;
    std::vector<RS_Vector> childEnds;

    explicit State(const LC_OffsetCurve& curve)
        : data(curve.getData()), definition(curve.definition()), grips(curve.cachedDefinitionGrips()),
          tail(curve.unknownOwnedTail()), controls(curve.getData().controlPoints.data()),
          gripStorage(curve.cachedDefinitionGrips().data()), knots(curve.getData().knotslist.data()),
          weights(curve.getData().weights.data()), fingerprint(RS_Spline::geometryFingerprint(data)),
          tolerance(curve.checkedWorldTolerance()), flags(curve.getFlags()), ownedFlags(curve.unknownOwnedFlags()),
          defined(curve.isDefined()), consistent(curve.isConsistent()), whole(curve.sourceIsWhole()),
          closed(curve.cacheFormsClosedChain()), middle(curve.getMiddlePoint()), minimum(curve.getMin()), maximum(curve.getMax()),
          parent(curve.getParent()), layer(curve.getLayer()), pen(curve.getPen(false)), id(curve.getId()),
          lineage(curve.lineage()), ownerLineage(curve.ownerLineage()), handles{curve.sourceHandle(), curve.xDictHandle(),
              curve.materialHandle(), curve.plotStyleHandle(), curve.shadowHandle(), curve.fullVisualStyleHandle(),
              curve.faceVisualStyleHandle(), curve.edgeVisualStyleHandle()}, shadowMode(curve.shadowMode()),
          reactors(curve.reactorHandles()), userValue(curve.getUserDefVar("D-grips")) {
        for (const auto& item : curve.getDrwExtData()) {
            REQUIRE(item); REQUIRE(item->type() == DRW_Variant::STRING);
            xdata.emplace_back(item->code(), *item->content.s);
        }
        for (const auto* child : curve) {
            REQUIRE(child); children.push_back(child);
            childEnds.push_back(child->getStartpoint()); childEnds.push_back(child->getEndpoint());
        }
    }

    void check(const LC_OffsetCurve& curve) const {
        checkData(curve.getData(), data); checkDefinition(curve.definition(), definition);
        checkPoints(curve.cachedDefinitionGrips(), grips); checkTail(curve.unknownOwnedTail(), tail);
        CHECK(curve.getData().controlPoints.data() == controls); CHECK(curve.cachedDefinitionGrips().data() == gripStorage);
        CHECK(curve.getData().knotslist.data() == knots); CHECK(curve.getData().weights.data() == weights);
        CHECK(RS_Spline::geometryFingerprint(curve.getData()) == fingerprint);
        CHECK(bits(curve.checkedWorldTolerance()) == bits(tolerance)); CHECK(curve.getFlags() == flags);
        CHECK(curve.unknownOwnedFlags() == ownedFlags); CHECK(curve.isDefined() == defined);
        CHECK(curve.isConsistent() == consistent); CHECK(curve.sourceIsWhole() == whole);
        CHECK(curve.cacheFormsClosedChain() == closed); CHECK(identical(curve.getMiddlePoint(), middle));
        CHECK(identical(curve.getMin(), minimum)); CHECK(identical(curve.getMax(), maximum));
        CHECK(curve.getParent() == parent); CHECK(curve.getLayer() == layer); CHECK(curve.getPen(false) == pen);
        CHECK(curve.getId() == id); CHECK(curve.lineage() == lineage); CHECK(curve.ownerLineage() == ownerLineage);
        CHECK(curve.sourceHandle() == handles[0]); CHECK(curve.xDictHandle() == handles[1]);
        CHECK(curve.materialHandle() == handles[2]); CHECK(curve.plotStyleHandle() == handles[3]);
        CHECK(curve.shadowHandle() == handles[4]); CHECK(curve.fullVisualStyleHandle() == handles[5]);
        CHECK(curve.faceVisualStyleHandle() == handles[6]); CHECK(curve.edgeVisualStyleHandle() == handles[7]);
        CHECK(curve.shadowMode() == shadowMode); CHECK(curve.reactorHandles() == reactors);
        CHECK(curve.getUserDefVar("D-grips") == userValue);
        REQUIRE(curve.getDrwExtData().size() == xdata.size());
        for (std::size_t i = 0; i < xdata.size(); ++i) {
            const auto& item = curve.getDrwExtData()[i]; REQUIRE(item); REQUIRE(item->type() == DRW_Variant::STRING);
            CHECK(item->code() == xdata[i].first); CHECK(*item->content.s == xdata[i].second);
        }
        REQUIRE(curve.count() == children.size());
        for (std::size_t i = 0; i < children.size(); ++i) {
            CHECK(curve.entityAt(static_cast<int>(i)) == children[i]); CHECK(children[i]->getParent() == &curve);
            CHECK(identical(children[i]->getStartpoint(), childEnds[2*i]));
            CHECK(identical(children[i]->getEndpoint(), childEnds[2*i + 1]));
        }
    }
};

std::unique_ptr<LC_OffsetCurve> ring(const bool sourceReversed = false, const bool cacheReversed = false) {
    RS_SplineData data{3, false};
    std::array<RS_Vector, 4> net{{{2, 0}, {2, 9.0/8}, {9.0/8, 2}, {0, 2}}};
    for (unsigned quadrant = 0; quadrant < 4; ++quadrant) {
        for (unsigned i = quadrant ? 1 : 0; i < 4; ++i) data.controlPoints.push_back(net[i]);
        for (auto& point : net) point = {-point.y, point.x};
    }
    for (unsigned i = 0; i <= 4; ++i)
        for (unsigned j = 0; j < (i == 0 || i == 4 ? 4u : 3u); ++j) data.knotslist.push_back(i);
    data.weights.assign(data.controlPoints.size(), 1); data.m_closedFlag = true;
    RS_Spline cache{nullptr, data};
    if (sourceReversed != cacheReversed) cache.revertDirection();
    LC_EllipseOffsetDefinition definition;
    definition.basis = {{0, 0}, {1, 0}, 1, 0, 0, sourceReversed}; definition.distance = 1;
    definition.relTolerance = 0.01; definition.cacheReversed = cacheReversed;
    LC_OffsetCheckResult checked;
    auto curve = LC_OffsetCurve::adopt(cache, definition, 0.02, &checked);
    CAPTURE(checked.reason); REQUIRE(checked); REQUIRE(curve); decorate(*curve); return curve;
}

std::unique_ptr<LC_OffsetCurve> nonQuarterSeamRing(const bool cacheReversed) {
    RS_SplineData data{3, false};
    std::array<RS_Vector, 4> net{{{2, 0}, {2, 9.0/8}, {9.0/8, 2}, {0, 2}}};
    for (unsigned quadrant = 0; quadrant < 4; ++quadrant) {
        for (unsigned i = quadrant ? 1 : 0; i < 4; ++i) data.controlPoints.push_back(net[i]);
        for (auto& point : net) point = {-point.y, point.x};
    }
    for (unsigned i = 0; i <= 4; ++i)
        for (unsigned j = 0; j < (i == 0 || i == 4 ? 4u : 3u); ++j) data.knotslist.push_back(i);
    data.weights.assign(data.controlPoints.size(), 1); data.m_closedFlag = true;
    RS_Spline cache{nullptr, data};
    if (cacheReversed) cache.revertDirection();
    // Keep the four-span cache unchanged. This fixed near-unit circle frame
    // makes its (2,0) seam non-quarter relative to the embedded source. Both
    // cache orders must earn a fresh public C certificate; no fixture search.
    LC_EllipseOffsetDefinition definition;
    definition.basis = {{0, 0}, {3.0/5, 4.0/5}, 1}; definition.distance = 1;
    definition.relTolerance = 0.01; definition.cacheReversed = cacheReversed;
    LC_OffsetCheckResult checked;
    auto curve = LC_OffsetCurve::adopt(cache, definition, 0.02, &checked);
    CAPTURE(cacheReversed, checked.reason, checked.work.vertices);
    REQUIRE(checked); REQUIRE(curve); decorate(*curve); return curve;
}

std::unique_ptr<LC_OffsetCurve> lineArc(const bool sourceReversed = false, const bool cacheReversed = false,
                                      const RS_Vector center = {0, 0}, const double end = 0.1, const double start = 0) {
    LC_EllipseOffsetDefinition definition;
    definition.basis = {center, {1, 0}, 1, sourceReversed ? end : start, sourceReversed ? start : end, sourceReversed};
    definition.distance = 0.25; definition.relTolerance = 0.01; definition.cacheReversed = cacheReversed;
    RS_Line cache{nullptr, center + RS_Vector{1.25*std::cos(start), 1.25*std::sin(start)},
        center + RS_Vector{1.25*std::cos(end), 1.25*std::sin(end)}};
    if (sourceReversed != cacheReversed) cache.revertDirection();
    LC_OffsetCheckResult checked;
    auto curve = LC_OffsetCurve::adopt(cache, definition, 0.01, &checked);
    CAPTURE(checked.reason, center.x, end); REQUIRE(checked); REQUIRE(curve); decorate(*curve); return curve;
}

std::unique_ptr<LC_OffsetCurve> produced(const RS_EllipseData& basis, const double distance, const bool inward,
                                       const bool cacheReversed = false) {
    const RS_Ellipse source{nullptr, basis};
    auto made = LC_OffsetCurve::create(source, distance, inward, {}, 1e-4, cacheReversed);
    const int engine = made.engineStatus ? static_cast<int>(*made.engineStatus) : -1;
    const int checked = made.checkReason ? static_cast<int>(*made.checkReason) : -1;
    CAPTURE(made.refusal, engine, checked); REQUIRE(made); REQUIRE(made.engineRuns == 1);
    REQUIRE(made.checkReason); REQUIRE(*made.checkReason == LC_OffsetCheckResult::Reason::None);
    decorate(*made.entity); return std::move(made.entity);
}

double distance(const RS_Vector& a, const RS_Vector& b) { return std::hypot(a.x - b.x, a.y - b.y); }

double nearestGripDistance(const LC_OffsetCurve& curve, const RS_Vector& point) {
    double result = std::numeric_limits<double>::infinity();
    for (const auto& grip : curve.cachedDefinitionGrips()) result = std::min(result, distance(grip, point));
    return result;
}

bool isGrip(const LC_OffsetCurve& curve, const RS_Vector& point) {
    const auto& grips = curve.cachedDefinitionGrips();
    return std::any_of(grips.begin(), grips.end(), [&](const auto& grip) { return identical(grip, point); });
}

void checkGripRefs(const LC_OffsetCurve& curve) {
    REQUIRE_FALSE(curve.cachedDefinitionGrips().empty());
    checkPoints(curve.getRefPoints().getVector(), curve.cachedDefinitionGrips());
    for (std::size_t i = 0; i < curve.cachedDefinitionGrips().size(); ++i) {
        const auto index = curve.definitionGripIndex(curve.cachedDefinitionGrips()[i]);
        REQUIRE(index); CHECK(*index == i);
    }
}

double ulp(const double value) {
    return std::max(std::nextafter(value, std::numeric_limits<double>::infinity()) - value,
        value - std::nextafter(value, -std::numeric_limits<double>::infinity()));
}
}

TEST_CASE("Offset grips follow the directed cache through all source and cache orientations",
          "[lc-offsetcurve][grips]") {
    for (const bool reversed : {false, true}) for (const bool cacheReversed : {false, true}) {
        CAPTURE(reversed, cacheReversed);
        auto curve = ring(reversed, cacheReversed); checkGripRefs(*curve);
        const auto& grips = curve->cachedDefinitionGrips(); REQUIRE(grips.size() == 4);
        for (const auto& point : std::array<RS_Vector, 4>{{{2, 0}, {0, 2}, {-2, 0}, {0, -2}}})
            CHECK(nearestGripDistance(*curve, point) <= curve->checkedWorldTolerance());
        for (std::size_t i = 0; i < grips.size(); ++i) {
            const auto& a = grips[i]; const auto& b = grips[(i + 1) % grips.size()];
            CHECK((a.x*b.y - a.y*b.x) * (reversed != cacheReversed ? -1 : 1) > 0);
        }
        const auto seam = curve->getData().controlPoints.front(); REQUIRE(seam.x == 2); REQUIRE(seam.y == 0);
        CHECK(nearestGripDistance(*curve, seam) == 0);
        double dist = -1; RS_Entity* entity = nullptr;
        const auto snapped = curve->getNearestEndpoint(seam, &entity, &dist);
        CHECK(isGrip(*curve, snapped)); CHECK(distance(snapped, seam) == 0); CHECK(dist == 0); CHECK(entity == curve.get());
    }
}

TEST_CASE("Whole inward ellipse grips retain analytic corners instead of removed normal tips",
          "[lc-offsetcurve][grips][lens]") {
    const double corner = std::sqrt((100.0*100 - 60.0*60)*(1 - 49.0*49/(60.0*60)));
    for (const bool cacheReversed : {false, true}) {
        auto curve = produced({{0, 0}, {100, 0}, 0.6}, 49, true, cacheReversed);
        REQUIRE(curve->sourceIsWhole()); REQUIRE(curve->cacheFormsClosedChain()); checkGripRefs(*curve);
        for (const auto& point : std::array<RS_Vector, 4>{{{corner, 0}, {-corner, 0}, {0, 11}, {0, -11}}}) {
            CAPTURE(point.x, point.y); CHECK(nearestGripDistance(*curve, point) <= curve->checkedWorldTolerance());
            double dist = -1; RS_Entity* entity = nullptr;
            const auto snapped = curve->getNearestEndpoint(point, &entity, &dist);
            CHECK(isGrip(*curve, snapped)); CHECK(entity == curve.get()); CHECK(dist <= curve->checkedWorldTolerance());
        }
        CHECK(nearestGripDistance(*curve, {51, 0}) > 1); CHECK(nearestGripDistance(*curve, {-51, 0}) > 1);
    }
}

TEST_CASE("Closed offset results from open elliptic arcs snap to visible grips",
          "[lc-offsetcurve][grips][lens]") {
    const double pi = std::acos(-1.0);
    for (const bool cacheReversed : {false, true}) {
        auto curve = produced({{0, 0}, {100, 0}, 0.5, pi/18, 35*pi/18, false}, 49.5, true, cacheReversed);
        REQUIRE_FALSE(curve->sourceIsWhole()); REQUIRE(curve->cacheFormsClosedChain()); checkGripRefs(*curve);
        const double corner = std::sqrt((100.0*100 - 50.0*50)*(1 - 49.5*49.5/(50.0*50)));
        for (const auto& point : std::array<RS_Vector, 4>{{{corner, 0}, {-corner, 0}, {0, 0.5}, {0, -0.5}}})
            CHECK(nearestGripDistance(*curve, point) <= curve->checkedWorldTolerance());
        const State before{*curve};
        for (const auto& query : std::array<RS_Vector, 4>{{{20, 0}, {-20, 0}, {0, 5}, {0, -5}}}) {
            double dist = -1; RS_Entity* entity = nullptr;
            const auto point = curve->getNearestEndpoint(query, &entity, &dist);
            CHECK(isGrip(*curve, point)); CHECK(entity == curve.get());
            CHECK(dist == Catch::Approx(nearestGripDistance(*curve, query)));
        }
        double dist = -1; CHECK_FALSE(curve->getNearestDist(0.1, {0, 0}, &dist).valid); CHECK(dist == RS_MAXDOUBLE);
        before.check(*curve);
    }
}

TEST_CASE("Whole offset endpoint snaps do not expose an arbitrary construction seam",
          "[lc-offsetcurve][grips][snaps]") {
    for (const bool cacheReversed : {false, true}) {
        auto curve = nonQuarterSeamRing(cacheReversed); checkGripRefs(*curve);
        CAPTURE(cacheReversed, curve->definition().basis.majorP.x, curve->definition().basis.majorP.y);
        const auto seam = curve->getData().controlPoints.front();
        REQUIRE(seam.x == 2); REQUIRE(seam.y == 0);
        REQUIRE(nearestGripDistance(*curve, seam) > curve->checkedWorldTolerance());
        double dist = -1; RS_Entity* entity = nullptr;
        const auto endpoint = curve->getNearestEndpoint(seam, &entity, &dist);
        CHECK(isGrip(*curve, endpoint)); CHECK(distance(endpoint, seam) > curve->checkedWorldTolerance()); CHECK(entity == curve.get());
        CHECK(dist == Catch::Approx(nearestGripDistance(*curve, seam)));
    }
}

TEST_CASE("Open offset endpoint snaps preserve exact cache endpoints in either order",
          "[lc-offsetcurve][grips][snaps]") {
    for (const bool reversed : {false, true}) for (const bool cacheReversed : {false, true}) {
        auto curve = lineArc(reversed, cacheReversed); checkGripRefs(*curve); const State before{*curve};
        const auto& data = curve->getData();
        for (const auto& endpoint : {data.controlPoints.front(), data.controlPoints.back()}) {
            double dist = -1; RS_Entity* entity = nullptr;
            CHECK(identical(curve->getNearestEndpoint(endpoint, &entity, &dist), endpoint));
            CHECK(dist == 0); CHECK(entity == curve.get());
        }
        const auto middle = (data.controlPoints.front() + data.controlPoints.back())*0.5;
        REQUIRE(curve->getMiddlePoint().valid); CHECK(distance(curve->getMiddlePoint(), middle) < 1e-10);
        double dist = -1;
        CHECK(identical(curve->getNearestMiddle(curve->getMiddlePoint(), &dist), curve->getMiddlePoint())); CHECK(dist == 0);
        auto plain = curve->toPlainSpline(); double plainDist = -1;
        const auto query = data.controlPoints.front();
        CHECK(distance(curve->getNearestMiddle(query, &dist, 3), plain->getNearestMiddle(query, &plainDist, 3)) < 1e-10);
        CHECK(dist == Catch::Approx(plainDist));
        CHECK(distance(curve->getNearestDist(0.02, query, &dist), plain->getNearestDist(0.02, query, &plainDist)) < 1e-10);
        CHECK(dist == Catch::Approx(plainDist));
        for (const double invalid : {-1.0, std::numeric_limits<double>::infinity(), std::numeric_limits<double>::quiet_NaN()}) {
            dist = -1; CHECK_FALSE(curve->getNearestDist(invalid, query, &dist).valid); CHECK(dist == RS_MAXDOUBLE);
        }
        before.check(*curve);
    }
}

TEST_CASE("Offset snap refusals initialize distance and entity outputs",
          "[lc-offsetcurve][grips][snaps]") {
    auto curve = ring(); curve->delFlag(RS2::FlagSelected); const State before{*curve};
    for (const auto& invalid : {RS_Vector{false}, RS_Vector{0, 0, 1},
         RS_Vector{std::numeric_limits<double>::infinity(), 0}, RS_Vector{std::numeric_limits<double>::quiet_NaN(), 0}}) {
        double dist = -1; RS_Entity* entity = curve.get();
        CHECK_FALSE(curve->getNearestEndpoint(invalid, &entity, &dist).valid); CHECK(dist == RS_MAXDOUBLE); CHECK(entity == nullptr);
        dist = -1; entity = curve.get();
        CHECK_FALSE(curve->getNearestCenter(invalid, &dist, &entity).valid); CHECK(dist == RS_MAXDOUBLE); CHECK(entity == nullptr);
        dist = -1; CHECK_FALSE(curve->getNearestRef(invalid, &dist).valid); CHECK(dist == RS_MAXDOUBLE);
        dist = -1; CHECK_FALSE(curve->getNearestSelectedRef(invalid, &dist).valid); CHECK(dist == RS_MAXDOUBLE);
        dist = -1; CHECK_FALSE(curve->getNearestMiddle(invalid, &dist).valid); CHECK(dist == RS_MAXDOUBLE);
        CHECK_FALSE(curve->definitionGripIndex(invalid));
    }
    double dist = -1; CHECK_FALSE(curve->getNearestSelectedRef({2, 0}, &dist).valid); CHECK(dist == RS_MAXDOUBLE);
    for (const int count : {-1, 0, 65}) {
        dist = -1; CHECK_FALSE(curve->getNearestMiddle({0, 0}, &dist, count).valid); CHECK(dist == RS_MAXDOUBLE);
    }
    dist = -1; CHECK_FALSE(curve->getNearestDist(0.1, {2, 0}, &dist).valid); CHECK(dist == RS_MAXDOUBLE);
    before.check(*curve);
}

TEST_CASE("Arcs without a quarter-axis vertex retain a usable cached midpoint grip",
          "[lc-offsetcurve][grips][fallback]") {
    for (const bool cacheReversed : {false, true}) {
        auto curve = lineArc(false, cacheReversed, {0, 0}, 0.4, 0.2); checkGripRefs(*curve);
        const auto& data = curve->getData();
        const auto middle = (data.controlPoints.front() + data.controlPoints.back())*0.5;
        REQUIRE(curve->getMiddlePoint().valid); CHECK(distance(curve->getMiddlePoint(), middle) < 1e-10);
        CHECK(nearestGripDistance(*curve, middle) <= curve->checkedWorldTolerance());
        const State before{*curve}; double dist = -1;
        CHECK(identical(curve->getNearestMiddle(middle, &dist), curve->getMiddlePoint()));
        CHECK(dist < 1e-10); before.check(*curve);
    }
}

TEST_CASE("Finite offset snaps beyond the refusal sentinel retain valid points and actual distances",
          "[lc-offsetcurve][grips][large-coordinates]") {
    auto curve = ring(); const double factor = std::ldexp(1.0, 34);
    curve->scale({0, 0}, {factor, factor});
    REQUIRE(curve->hasEditableDefinition()); REQUIRE(curve->sourceIsWhole()); REQUIRE(curve->cacheFormsClosedChain());
    REQUIRE(curve->definition().basis.majorP.x == factor); REQUIRE(curve->definition().distance == factor);
    decorate(*curve); const State before{*curve};
    const auto centerQuery = curve->cachedDefinitionGrips().front();
    const auto center = curve->definition().basis.center;
    REQUIRE(distance(centerQuery, center) > RS_MAXDOUBLE);
    const RS_Vector far{8*factor, 4*factor};
    const double nearest = nearestGripDistance(*curve, far); REQUIRE(nearest > RS_MAXDOUBLE); REQUIRE(std::isfinite(nearest));
    for (unsigned repeat = 0; repeat < 4; ++repeat) {
        double dist = -1; RS_Entity* entity = nullptr;
        const auto snappedCenter = curve->getNearestCenter(centerQuery, &dist, &entity);
        REQUIRE(snappedCenter.valid); CHECK(identical(snappedCenter, center)); CHECK(entity == curve.get());
        CHECK(dist > RS_MAXDOUBLE); CHECK(dist == Catch::Approx(distance(centerQuery, center)));
        before.check(*curve);
        dist = -1; const auto ref = curve->getNearestRef(far, &dist);
        REQUIRE(ref.valid); CHECK(isGrip(*curve, ref)); CHECK(dist == Catch::Approx(nearest)); CHECK(dist > RS_MAXDOUBLE);
        before.check(*curve);
        dist = -1; const auto selected = curve->getNearestSelectedRef(far, &dist);
        REQUIRE(selected.valid); CHECK(isGrip(*curve, selected)); CHECK(dist == Catch::Approx(nearest)); CHECK(dist > RS_MAXDOUBLE);
        before.check(*curve);
        dist = -1; entity = nullptr;
        const auto endpoint = curve->getNearestEndpoint(far, &entity, &dist);
        REQUIRE(endpoint.valid); CHECK(isGrip(*curve, endpoint)); CHECK(entity == curve.get());
        CHECK(dist == Catch::Approx(nearest)); CHECK(dist > RS_MAXDOUBLE); before.check(*curve);
        dist = -1; const auto middle = curve->getNearestMiddle(far, &dist);
        REQUIRE(middle.valid); CHECK(identical(middle, curve->getMiddlePoint()));
        CHECK(dist == Catch::Approx(distance(far, middle))); CHECK(dist > RS_MAXDOUBLE); before.check(*curve);
        entity = curve.get();
        CHECK_FALSE(curve->getNearestCenter(RS_Vector{false}, &dist, &entity).valid);
        CHECK(dist == RS_MAXDOUBLE); CHECK(entity == nullptr); before.check(*curve);
    }
}

TEST_CASE("Cached offset queries and editable moveRef retain complete live state",
          "[lc-offsetcurve][grips][readonly]") {
    for (const bool whole : {false, true}) {
        auto curve = whole ? ring() : lineArc(); const State before{*curve};
        const auto query = curve->cachedDefinitionGrips().front() + RS_Vector{0.01, 0.02};
        for (unsigned repeat = 0; repeat < 16; ++repeat) {
            checkGripRefs(*curve); double dist = -1; RS_Entity* entity = nullptr;
            const auto center = curve->getNearestCenter(query, &dist, &entity);
            CHECK(identical(center, curve->definition().basis.center)); CHECK(entity == curve.get());
            CHECK(dist == Catch::Approx(distance(query, center)));
            CHECK(isGrip(*curve, curve->getNearestRef(query, &dist)));
            CHECK(isGrip(*curve, curve->getNearestSelectedRef(query, &dist)));
            REQUIRE(curve->getMiddlePoint().valid);
            CHECK(identical(curve->getNearestMiddle(query, &dist), curve->getMiddlePoint()));
            CHECK(dist == Catch::Approx(distance(query, curve->getMiddlePoint())));
            curve->getNearestEndpoint(query, &entity, &dist); curve->getNearestDist(0.02, query, &dist);
            CHECK(curve->proposeThrough({1.5, 0}));
            // These are the cached inputs consumed by reference/paint paths;
            // state identity is checked, not a fabricated engine-call counter.
            std::vector<RS_Vector> display; curve->fillStrokePoints(32, display); CHECK_FALSE(display.empty());
            curve->moveRef(curve->cachedDefinitionGrips().front(), {0.25, -0.5});
            curve->moveRef(curve->getData().controlPoints.front(), {0.25, -0.5});
            before.check(*curve);
        }
    }
}

TEST_CASE("Undefined and inconsistent offset grips delegate to ordinary spline controls without restamping",
          "[lc-offsetcurve][grips][fallback]") {
    for (const bool undefined : {false, true}) {
        auto curve = lineArc();
        if (undefined) curve->scale({0, 0}, {2, 3});
        else curve->getData().controlPoints.front().x += 0.25;
        REQUIRE_FALSE(curve->hasEditableDefinition()); decorate(*curve);
        auto plain = curve->toPlainSpline(); const State before{*curve};
        checkPoints(curve->getRefPoints().getVector(), plain->getRefPoints().getVector());
        CHECK_FALSE(curve->definitionGripIndex(curve->getData().controlPoints.front()));
        const auto query = curve->getData().controlPoints.back(); double a = -1, b = -1;
        CHECK(identical(curve->getNearestRef(query, &a), plain->getNearestRef(query, &b))); CHECK(a == b);
        RS_Entity* entity = curve.get(); a = -1;
        CHECK_FALSE(curve->getNearestCenter(query, &a, &entity).valid); CHECK(a == RS_MAXDOUBLE); CHECK(entity == nullptr);
        before.check(*curve);
        const auto definition = curve->definition(); const auto tail = curve->unknownOwnedTail(); const auto flags = curve->unknownOwnedFlags();
        curve->moveRef(query, {0.0625, 0.125}); plain->moveRef(query, {0.0625, 0.125});
        checkData(curve->getData(), plain->getData()); REQUIRE_FALSE(curve->hasEditableDefinition());
        CHECK(RS_Spline::geometryFingerprint(curve->getData()) != before.fingerprint);
        checkDefinition(curve->definition(), definition); checkTail(curve->unknownOwnedTail(), tail); CHECK(curve->unknownOwnedFlags() == flags);
    }
}

TEST_CASE("Large-center grip matching uses explicit local ULP allowance not a world-scaled tolerance",
          "[lc-offsetcurve][grips][ulp]") {
    auto curve = lineArc(false, false, {1e12, 1e12}, 0.05); checkGripRefs(*curve); const State before{*curve};
    const auto& grips = curve->cachedDefinitionGrips();
    for (std::size_t i = 0; i < grips.size(); ++i) {
        auto query = grips[i]; query.x = std::nextafter(query.x, std::numeric_limits<double>::infinity());
        const auto index = curve->definitionGripIndex(query); REQUIRE(index); CHECK(*index == i);
        CHECK_FALSE(curve->definitionGripIndex(grips[i] + RS_Vector{0.01, 0}));
    }
    before.check(*curve);
}

TEST_CASE("Ambiguous large-center local grip matches refuse an index",
          "[lc-offsetcurve][grips][ulp]") {
    auto curve = lineArc(false, false, {1e12, 1e12}, 0.001); const State before{*curve};
    const auto& grips = curve->cachedDefinitionGrips(); REQUIRE(grips.size() >= 2);
    std::optional<RS_Vector> ambiguous;
    for (std::size_t i = 0; i < grips.size(); ++i) for (std::size_t j = i + 1; j < grips.size(); ++j) {
        const auto middle = grips[i]*0.5 + grips[j]*0.5;
        if (!identical(grips[i], grips[j]) &&
            std::abs(middle.x - grips[i].x) <= 4*ulp(middle.x) && std::abs(middle.x - grips[j].x) <= 4*ulp(middle.x) &&
            std::abs(middle.y - grips[i].y) <= 4*ulp(middle.y) && std::abs(middle.y - grips[j].y) <= 4*ulp(middle.y)) ambiguous = middle;
    }
    REQUIRE(ambiguous); CHECK_FALSE(curve->definitionGripIndex(*ambiguous)); before.check(*curve);
}

TEST_CASE("Native offset clones transport owned grips independently through exact transforms and reversal",
          "[lc-offsetcurve][grips][clone]") {
    auto original = ring(); const State before{*original};
    auto ownedClone = std::unique_ptr<RS_Entity>{original->clone()};
    auto* clone = dynamic_cast<LC_OffsetCurve*>(ownedClone.get());
    REQUIRE(clone); checkPoints(clone->cachedDefinitionGrips(), original->cachedDefinitionGrips());
    REQUIRE(clone->cachedDefinitionGrips().data() != original->cachedDefinitionGrips().data());
    REQUIRE(clone->count() == original->count());
    for (unsigned i = 0; i < clone->count(); ++i) {
        CHECK(clone->entityAt(static_cast<int>(i)) != original->entityAt(static_cast<int>(i)));
        CHECK(clone->entityAt(static_cast<int>(i))->getParent() == clone);
    }
    clone->move({4, 8}); REQUIRE(clone->hasEditableDefinition());
    REQUIRE(clone->cachedDefinitionGrips().size() == before.grips.size());
    for (std::size_t i = 0; i < before.grips.size(); ++i)
        CHECK(identical(clone->cachedDefinitionGrips()[i], before.grips[i] + RS_Vector{4, 8}));
    double dist = -1; RS_Entity* entity = nullptr;
    CHECK(identical(clone->getNearestCenter({4, 8}, &dist, &entity), RS_Vector{4, 8})); CHECK(dist == 0); CHECK(entity == clone);
    const auto grips = clone->cachedDefinitionGrips(); const bool reversed = clone->definition().cacheReversed;
    clone->revertDirection(); REQUIRE(clone->hasEditableDefinition()); CHECK(clone->definition().cacheReversed != reversed);
    auto reverseGrips = grips; std::reverse(reverseGrips.begin(), reverseGrips.end());
    checkPoints(clone->cachedDefinitionGrips(), reverseGrips); checkGripRefs(*clone); before.check(*original);
}

TEST_CASE("Meaningful offset edits regenerate visible grips and subsequent queries retain the new cache",
          "[lc-offsetcurve][grips][edit]") {
    for (const bool cacheReversed : {false, true}) {
        auto curve = ring(false, cacheReversed);
        const auto changed = curve->setDistance(0.5); REQUIRE(changed); CHECK(changed.changed); CHECK(changed.engineRuns == 1);
        REQUIRE(changed.checkReason); CHECK(*changed.checkReason == LC_OffsetCheckResult::Reason::None);
        CHECK(curve->unknownOwnedFlags() == 0); CHECK(curve->unknownOwnedTail().empty()); checkGripRefs(*curve);
        for (const auto& point : std::array<RS_Vector, 4>{{{1.5, 0}, {0, 1.5}, {-1.5, 0}, {0, -1.5}}})
            CHECK(nearestGripDistance(*curve, point) <= curve->checkedWorldTolerance());
        CHECK(nearestGripDistance(*curve, {2, 0}) > 0.25);
        decorate(*curve); const State before{*curve};
        for (unsigned i = 0; i < 8; ++i) { checkGripRefs(*curve); curve->getMiddlePoint(); before.check(*curve); }
    }
}

TEST_CASE("Arc basis edits recompute visible midpoint grips while no-op retains their storage",
          "[lc-offsetcurve][grips][edit]") {
    auto curve = lineArc(); auto basis = curve->definition().basis; basis.angle2 = 0.2;
    const auto changed = curve->setBasis(basis); REQUIRE(changed); CHECK(changed.changed); CHECK(changed.engineRuns == 1);
    REQUIRE(changed.checkReason); CHECK(*changed.checkReason == LC_OffsetCheckResult::Reason::None); checkGripRefs(*curve);
    CHECK(nearestGripDistance(*curve, {1.25*std::cos(0.1), 1.25*std::sin(0.1)}) <= curve->checkedWorldTolerance());
    decorate(*curve); const State before{*curve};
    const auto unchanged = curve->setBasis(curve->definition().basis);
    REQUIRE(unchanged); CHECK_FALSE(unchanged.changed); CHECK(unchanged.engineRuns == 0);
    CHECK_FALSE(unchanged.engineStatus); CHECK_FALSE(unchanged.checkReason); before.check(*curve);
}
