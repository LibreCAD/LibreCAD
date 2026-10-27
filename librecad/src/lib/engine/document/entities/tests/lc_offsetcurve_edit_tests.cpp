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
#include <string>
#include <utility>
#include <vector>

#include <boost/multiprecision/cpp_int.hpp>

#include "drw_base.h"
#include "lc_offsetcurve.h"
#include "lc_offsetoutputbudget.h"
#include "rs_layer.h"
#include "rs_line.h"

namespace geo = LC_EllipseOffsetGeometry;
namespace {
using Refusal = LC_OffsetCurveRefusal;
using CheckReason = LC_OffsetCheckResult::Reason;
using Exact = boost::multiprecision::cpp_rational;

std::uint64_t bits(const double value) {
    std::uint64_t result;
    static_assert(sizeof(result) == sizeof(value));
    std::memcpy(&result, &value, sizeof(result));
    return result;
}

bool samePoint(const RS_Vector& a, const RS_Vector& b) {
    return bits(a.x) == bits(b.x) && bits(a.y) == bits(b.y) && bits(a.z) == bits(b.z) && a.valid == b.valid;
}

void checkPoints(const std::vector<RS_Vector>& actual, const std::vector<RS_Vector>& expected) {
    REQUIRE(actual.size() == expected.size());
    for (std::size_t i = 0; i < actual.size(); ++i) {
        CAPTURE(i);
        CHECK(samePoint(actual[i], expected[i]));
    }
}

void checkBasis(const RS_EllipseData& actual, const RS_EllipseData& expected) {
    CHECK(samePoint(actual.center, expected.center));
    CHECK(samePoint(actual.majorP, expected.majorP));
    CHECK(bits(actual.ratio) == bits(expected.ratio));
    CHECK(bits(actual.angle1) == bits(expected.angle1));
    CHECK(bits(actual.angle2) == bits(expected.angle2));
    CHECK(actual.reversed == expected.reversed);
    CHECK(actual.isArc == expected.isArc);
    CHECK(bits(actual.angleDegrees) == bits(expected.angleDegrees));
    CHECK(bits(actual.startAngleDegrees) == bits(expected.startAngleDegrees));
    CHECK(bits(actual.otherAngleDegrees) == bits(expected.otherAngleDegrees));
    CHECK(bits(actual.angularLength) == bits(expected.angularLength));
}

void checkDefinition(const LC_EllipseOffsetDefinition& actual, const LC_EllipseOffsetDefinition& expected) {
    checkBasis(actual.basis, expected.basis);
    CHECK(bits(actual.distance) == bits(expected.distance));
    CHECK(bits(actual.relTolerance) == bits(expected.relTolerance));
    CHECK(actual.towardCentre == expected.towardCentre);
    CHECK(actual.cacheReversed == expected.cacheReversed);
}

void checkTail(const LC_OffsetCurve::OwnedTail& actual, const LC_OffsetCurve::OwnedTail& expected) {
    REQUIRE(actual.size() == expected.size());
    for (std::size_t i = 0; i < actual.size(); ++i) {
        CAPTURE(i);
        CHECK(actual[i].code == expected[i].code);
        REQUIRE(actual[i].value.index() == expected[i].value.index());
        switch (actual[i].value.index()) {
        case 0: CHECK(std::get<QString>(actual[i].value) == std::get<QString>(expected[i].value)); break;
        case 1: CHECK(samePoint(std::get<RS_Vector>(actual[i].value), std::get<RS_Vector>(expected[i].value))); break;
        case 2: CHECK(bits(std::get<double>(actual[i].value)) == bits(std::get<double>(expected[i].value))); break;
        case 3: CHECK(std::get<std::int16_t>(actual[i].value) == std::get<std::int16_t>(expected[i].value)); break;
        case 4: CHECK(std::get<std::int32_t>(actual[i].value) == std::get<std::int32_t>(expected[i].value)); break;
        default: FAIL("unexpected extension type");
        }
    }
}

void decorate(RS_Entity& entity) {
    entity.setVisible(false);
    entity.setFlag(RS2::FlagTransparent);
    entity.setPen(RS_Pen{RS_Color{17, 31, 47}, RS2::Width11, RS2::DashLine});
    entity.setSourceHandle(101); entity.setXDictHandle(102); entity.setReactorHandles({103, 104});
    entity.setMaterialHandle(105); entity.setPlotStyleHandle(106);
    entity.setShadowMode(2); entity.setShadowHandle(107); entity.setVisualStyleHandles(108, 109, 110);
    entity.setOwnerLineage(111);
    entity.setUserDefVar("D-edit", "unchanged user value");
    entity.setDrwExtData({std::make_shared<DRW_Variant>(1001, std::string("D_EDIT_TEST")),
                         std::make_shared<DRW_Variant>(1000, std::string("unrelated application data"))});
}

void decorate(LC_OffsetCurve& entity) {
    decorate(static_cast<RS_Entity&>(entity));
    entity.setFlag(RS2::FlagSelected);
    REQUIRE(entity.setOwnedExtensions(0x30, {{1002, QStringLiteral("{")}, {1000, QStringLiteral("reserved")},
        {1010, RS_Vector{3, 4}}, {1040, 0.125}, {1070, std::int16_t{7}},
        {1071, std::int32_t{123456}}, {1002, QStringLiteral("}")}}));
}

struct Attributes {
    RS_EntityContainer* parent;
    RS_Layer* layer;
    RS_Pen pen;
    unsigned flags;
    unsigned long long id;
    unsigned long long lineage, ownerLineage;
    quint32 sourceHandle, dictionary;
    std::vector<quint32> reactors;
    std::array<quint32, 6> passive;
    int shadowMode;
    QString userValue;
    std::vector<std::pair<int, std::string>> xdata;

    explicit Attributes(const RS_Entity& entity)
        : parent(entity.getParent()), layer(entity.getLayer()), pen(entity.getPen(false)), flags(entity.getFlags()),
          id(entity.getId()), lineage(entity.lineage()), ownerLineage(entity.ownerLineage()),
          sourceHandle(entity.sourceHandle()), dictionary(entity.xDictHandle()), reactors(entity.reactorHandles()),
          passive{entity.materialHandle(), entity.plotStyleHandle(), entity.shadowHandle(), entity.fullVisualStyleHandle(),
                  entity.faceVisualStyleHandle(), entity.edgeVisualStyleHandle()}, shadowMode(entity.shadowMode()),
          userValue(entity.getUserDefVar("D-edit")) {
        for (const auto& item : entity.getDrwExtData()) {
            REQUIRE(item != nullptr);
            REQUIRE(item->type() == DRW_Variant::STRING);
            xdata.emplace_back(item->code(), *item->content.s);
        }
    }

    void check(const RS_Entity& entity, const bool sameIdentity = true) const {
        CHECK(entity.getParent() == parent); CHECK(entity.getLayer() == layer);
        CHECK(entity.getPen(false) == pen); CHECK(entity.getFlags() == flags);
        CHECK(entity.materialHandle() == passive[0]); CHECK(entity.plotStyleHandle() == passive[1]);
        CHECK(entity.shadowHandle() == passive[2]); CHECK(entity.fullVisualStyleHandle() == passive[3]);
        CHECK(entity.faceVisualStyleHandle() == passive[4]); CHECK(entity.edgeVisualStyleHandle() == passive[5]);
        CHECK(entity.shadowMode() == shadowMode); CHECK(entity.getUserDefVar("D-edit") == userValue);
        REQUIRE(entity.getDrwExtData().size() == xdata.size());
        for (std::size_t i = 0; i < xdata.size(); ++i) {
            const auto& item = entity.getDrwExtData()[i];
            REQUIRE(item != nullptr); REQUIRE(item->type() == DRW_Variant::STRING);
            CHECK(item->code() == xdata[i].first); CHECK(*item->content.s == xdata[i].second);
        }
        if (sameIdentity) {
            CHECK(entity.getId() == id); CHECK(entity.lineage() == lineage); CHECK(entity.ownerLineage() == ownerLineage);
            CHECK(entity.sourceHandle() == sourceHandle); CHECK(entity.xDictHandle() == dictionary);
            CHECK(entity.reactorHandles() == reactors);
        }
    }
};

struct State {
    RS_SplineData data;
    LC_EllipseOffsetDefinition definition;
    Attributes attributes;
    std::uint64_t fingerprint;
    const RS_Vector* storage;
    std::vector<RS_Vector> grips;
    LC_OffsetCurve::OwnedTail tail;
    unsigned ownedFlags;
    double tolerance;
    bool defined, consistent, editable, whole, closed;
    RS_Vector minimum, maximum;
    std::vector<const RS_Entity*> children;

    explicit State(const LC_OffsetCurve& curve)
        : data(curve.getData()), definition(curve.definition()), attributes(curve),
          fingerprint(RS_Spline::geometryFingerprint(curve.getData())), storage(curve.getData().controlPoints.data()),
          grips(curve.cachedDefinitionGrips()), tail(curve.unknownOwnedTail()), ownedFlags(curve.unknownOwnedFlags()),
          tolerance(curve.checkedWorldTolerance()), defined(curve.isDefined()), consistent(curve.isConsistent()),
          editable(curve.hasEditableDefinition()), whole(curve.sourceIsWhole()), closed(curve.cacheFormsClosedChain()),
          minimum(curve.getMin()), maximum(curve.getMax()) {
        for (const RS_Entity* child : curve) children.push_back(child);
    }

    void check(const LC_OffsetCurve& curve) const {
        attributes.check(curve);
        CHECK(RS_Spline::geometryFingerprint(curve.getData()) == fingerprint);
        CHECK(curve.getData().controlPoints.data() == storage);
        checkPoints(curve.getData().controlPoints, data.controlPoints);
        CHECK(curve.getData().knotslist == data.knotslist); CHECK(curve.getData().weights == data.weights);
        checkDefinition(curve.definition(), definition);
        checkPoints(curve.cachedDefinitionGrips(), grips); checkTail(curve.unknownOwnedTail(), tail);
        CHECK(curve.unknownOwnedFlags() == ownedFlags); CHECK(bits(curve.checkedWorldTolerance()) == bits(tolerance));
        CHECK(curve.isDefined() == defined); CHECK(curve.isConsistent() == consistent);
        CHECK(curve.hasEditableDefinition() == editable); CHECK(curve.sourceIsWhole() == whole);
        CHECK(curve.cacheFormsClosedChain() == closed);
        CHECK(samePoint(curve.getMin(), minimum)); CHECK(samePoint(curve.getMax(), maximum));
        REQUIRE(curve.count() == children.size());
        for (std::size_t i = 0; i < children.size(); ++i) {
            CHECK(curve.entityAt(static_cast<int>(i)) == children[i]);
            REQUIRE(children[i] != nullptr); CHECK(children[i]->getParent() == &curve);
        }
    }
};

RS_EllipseData shortArc(const RS_Vector center = {0, 0}) {
    // Its chord is within the adoption tolerance, but the offset remains well
    // above the real trimmed producer's node-merge resolution at relTol=.01.
    return {center, {1, 0}, 1, 0, 0.1, false};
}

std::unique_ptr<LC_OffsetCurve> adoptedArc(const double distance = 0.25, const RS_Vector center = {0, 0},
                                         const double relativeTolerance = 0.01, const double endAngle = 0.1) {
    LC_EllipseOffsetDefinition definition;
    definition.basis = shortArc(center); definition.distance = distance; definition.relTolerance = relativeTolerance;
    definition.basis.angle2 = endAngle;
    const double radius = 1 + distance;
    const RS_Line cache{nullptr, center + RS_Vector{radius, 0},
        center + RS_Vector{radius * std::cos(definition.basis.angle2), radius * std::sin(definition.basis.angle2)}};
    LC_OffsetCheckResult check;
    auto curve = LC_OffsetCurve::adopt(cache, definition, 0.01, &check);
    CAPTURE(check.reason, check.span); REQUIRE(curve); REQUIRE(check);
    decorate(*curve);
    return curve;
}

std::unique_ptr<LC_OffsetCurve> adoptedThroughLine() {
    // The tangent segment stays within .002 of the circular offset. Its
    // dyadic midpoint is an exact interior witness for the unchanged definition.
    const double angle = std::atan2(0.0625, 2.0);
    LC_EllipseOffsetDefinition definition;
    definition.basis = {{0, 0}, {1, 0}, 1, -angle, angle, false};
    definition.distance = 1; definition.relTolerance = 0.01;
    const RS_Line cache{nullptr, {2, -0.0625}, {2, 0.0625}};
    auto curve = LC_OffsetCurve::adopt(cache, definition, 0.01);
    REQUIRE(curve); decorate(*curve); return curve;
}

std::unique_ptr<LC_OffsetCurve> adoptedRing() {
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
    LC_EllipseOffsetDefinition definition;
    definition.basis = {{0, 0}, {1, 0}, 1}; definition.distance = 1; definition.relTolerance = 0.01;
    auto curve = LC_OffsetCurve::adopt(cache, definition, 0.02);
    REQUIRE(curve); decorate(*curve); return curve;
}

void checkWork(const geo::CheckWork& actual, const geo::CheckWork& expected) {
    CHECK(actual.cacheSpans == expected.cacheSpans); CHECK(actual.vertices == expected.vertices);
    CHECK(actual.distancePairs == expected.distancePairs); CHECK(actual.deepest == expected.deepest);
    CHECK(actual.orderingComparisons == expected.orderingComparisons);
}

void combinedWorkFits(const LC_OffsetCurveEditResult& result, const LC_OffsetCheckLimits& requested = {}) {
    const LC_OffsetCheckLimits hard;
    const auto vertices = std::min(requested.maxVertices, hard.maxVertices);
    const auto pairs = std::min(requested.maxDistancePairs, hard.maxDistancePairs);
    const auto depth = std::min(requested.maxDepth, hard.maxDepth);
    CHECK(result.checkWork.cacheSpans <= std::min(requested.maxCacheSpans, hard.maxCacheSpans));
    CHECK(result.checkWork.orderingComparisons <= std::min(requested.maxOrderingComparisons, hard.maxOrderingComparisons));
    REQUIRE(result.checkWork.vertices <= vertices); REQUIRE(result.checkWork.distancePairs <= pairs);
    REQUIRE(result.checkWork.deepest <= depth);
    CHECK(result.throughWork.vertices <= vertices - result.checkWork.vertices);
    CHECK(result.throughWork.distancePairs <= pairs - result.checkWork.distancePairs);
    CHECK(result.throughWork.deepest <= depth);
    CHECK(result.throughWork.cacheSpans == 0); CHECK(result.throughWork.orderingComparisons == 0);
}

void notRun(const LC_OffsetCurveEditResult& result) {
    CHECK(result.engineRuns == 0); CHECK_FALSE(result.engineStatus); CHECK_FALSE(result.checkReason);
    CHECK(result.checkWork.cacheSpans == 0); CHECK(result.checkWork.vertices == 0);
    CHECK(result.checkWork.distancePairs == 0); CHECK(result.checkWork.orderingComparisons == 0);
    CHECK(result.checkWork.deepest == 0);
}

void certified(const LC_OffsetCurve& curve) {
    REQUIRE(curve.hasEditableDefinition());
    const auto checked = geo::checkCache(curve.getData(), curve.definition(), curve.checkedWorldTolerance());
    CAPTURE(checked.reason, checked.span); REQUIRE(checked);
    CHECK(checked.analysis->closed() == curve.cacheFormsClosedChain());
}

void regenerated(const LC_OffsetCurveEditResult& result, const LC_OffsetCurve& curve) {
    const int engineStatus = result.engineStatus ? static_cast<int>(*result.engineStatus) : -1;
    const int checkReason = result.checkReason ? static_cast<int>(*result.checkReason) : -1;
    CAPTURE(result.refusal, engineStatus, checkReason, curve.definition().basis.center.x,
        curve.definition().basis.center.y, curve.definition().basis.angle1, curve.definition().basis.angle2);
    REQUIRE(static_cast<bool>(result)); CHECK(result.changed); CHECK(result.engineRuns == 1);
    REQUIRE(result.engineStatus); CHECK(*result.engineStatus == LC_CurveOffsetStatus::Ok);
    REQUIRE(result.checkReason); CHECK(*result.checkReason == CheckReason::None);
    CHECK(result.checkWork.cacheSpans > 0); CHECK(result.checkWork.vertices > 0);
    CHECK(curve.unknownOwnedFlags() == 0); CHECK(curve.unknownOwnedTail().empty());
    certified(curve);
}

// A normalized Bezier endpoint is an exact point on the actual cache. This is
// an independent membership witness, not an unevaluated stroke chord or seed.
bool hasEndpointWitness(const LC_OffsetCurve& curve, const RS_Vector& point) {
    geo::WorkBudget budget;
    const auto layout = geo::inspectCacheLayout(curve.getData(), budget);
    REQUIRE(layout.status == geo::CacheLayoutStatus::Ok);
    const Exact limit = Exact(curve.checkedWorldTolerance()) * Exact(curve.checkedWorldTolerance());
    for (const auto& span : layout.spans) for (const auto index : {std::size_t{0}, span.degree}) {
        const auto& endpoint = span.controls[index];
        const Exact dx = Exact(endpoint.x) - Exact(point.x), dy = Exact(endpoint.y) - Exact(point.y);
        if (dx*dx + dy*dy <= limit) return true;
    }
    return false;
}

bool hasInteriorWitness(const LC_OffsetCurve& curve, const RS_Vector& point) {
    geo::WorkBudget budget;
    const auto layout = geo::inspectCacheLayout(curve.getData(), budget);
    REQUIRE(layout.status == geo::CacheLayoutStatus::Ok);
    const Exact limit = Exact(curve.checkedWorldTolerance()) * Exact(curve.checkedWorldTolerance());
    for (const auto& span : layout.spans) for (unsigned numerator = 1; numerator < 64; ++numerator) {
        const Exact t = Exact(numerator) / 64;
        std::array<Exact, 4> x, y;
        for (std::size_t i = 0; i <= span.degree; ++i) {
            x[i] = Exact(span.controls[i].x); y[i] = Exact(span.controls[i].y);
        }
        // De Casteljau over exact binary controls supplies an actual interior
        // curve point, independent of the production interval-box query.
        for (std::size_t level = span.degree; level > 0; --level)
            for (std::size_t i = 0; i < level; ++i) {
                x[i] = (1 - t)*x[i] + t*x[i + 1]; y[i] = (1 - t)*y[i] + t*y[i + 1];
            }
        const Exact dx = x[0] - Exact(point.x), dy = y[0] - Exact(point.y);
        if (dx*dx + dy*dy <= limit) return true;
    }
    return false;
}
}

TEST_CASE("Offset creation uses one real engine and C check for either directed cache orientation",
          "[lc-offsetcurve][edit][create]") {
    for (const bool reversed : {false, true}) for (const bool towardCentre : {false, true})
        for (const bool cacheReversed : {false, true}) {
            CAPTURE(reversed, towardCentre, cacheReversed);
            auto basis = shortArc();
            if (reversed) { std::swap(basis.angle1, basis.angle2); basis.reversed = true; }
            const RS_Ellipse source{nullptr, basis};
            auto made = LC_OffsetCurve::create(source, 0.25, towardCentre, {}, 0.01, cacheReversed);
            REQUIRE(made.entity); regenerated(made, *made.entity);
            CHECK(made.entity->definition().towardCentre == towardCentre);
            CHECK(made.entity->definition().cacheReversed == cacheReversed);
            CHECK(made.entity->definition().basis.reversed == reversed);
            CHECK_FALSE(made.entity->sourceIsWhole()); CHECK_FALSE(made.entity->cacheFormsClosedChain());
        }
}

TEST_CASE("Offset creation retains value attributes but establishes a fresh drawing identity",
          "[lc-offsetcurve][edit][create][attributes]") {
    RS_EntityContainer owner;
    RS_Layer layer{QStringLiteral("D-edit layer")};
    RS_Ellipse source{&owner, shortArc()};
    decorate(source); source.setLayer(&layer);
    const Attributes before{source}; const auto geometry = source.getData();
    auto made = LC_OffsetCurve::create(source, 0.25, false, {}, 0.01);
    REQUIRE(made.entity); regenerated(made, *made.entity);
    before.check(*made.entity, false); before.check(source); checkBasis(source.getData(), geometry);
    CHECK(made.entity->getId() != source.getId()); CHECK(made.entity->lineage() != source.lineage());
    CHECK(made.entity->lineage() != 0); CHECK(made.entity->ownerLineage() == 0);
    CHECK(made.entity->sourceHandle() == 0); CHECK(made.entity->xDictHandle() == 0);
    CHECK(made.entity->reactorHandles().empty());
}

TEST_CASE("Offset creation selects the actual ellipse default relative tolerance",
          "[lc-offsetcurve][edit][create]") {
    const RS_Ellipse source{nullptr, shortArc()};
    auto made = LC_OffsetCurve::create(source, 0.25, false);
    REQUIRE(made.entity); regenerated(made, *made.entity);
    CHECK(made.entity->definition().relTolerance == LC_CurveOffset::kEllipseRelativeOffsetTolerance);
}

TEST_CASE("Offset creation certifies whole circles ellipses and inward corner lenses",
          "[lc-offsetcurve][edit][create][lens]") {
    const double pi = std::acos(-1.0);
    struct Fixture { RS_EllipseData basis; double distance; bool inward; bool whole; };
    const std::array<Fixture, 5> fixtures{{
        {{{0, 0}, {5, 0}, 1}, 1, false, true},
        {{{0, 0}, {5, 0}, 1}, 1, true, true},
        {{{0, 0}, {100, 0}, 0.5}, 4, true, true},
        {{{0, 0}, {100, 0}, 0.5}, 49, true, true},
        {{{0, 0}, {100, 0}, 0.5, pi/18, 35*pi/18, false}, 49.5, true, false}
    }};
    for (const auto& fixture : fixtures) {
        CAPTURE(fixture.distance, fixture.inward, fixture.whole);
        const RS_Ellipse source{nullptr, fixture.basis};
        auto made = LC_OffsetCurve::create(source, fixture.distance, fixture.inward, {}, 1e-4);
        REQUIRE(made.entity); regenerated(made, *made.entity);
        CHECK(made.entity->sourceIsWhole() == fixture.whole); CHECK(made.entity->cacheFormsClosedChain());
    }
}

TEST_CASE("Offset invalid creation and too-small requests never run the engine",
          "[lc-offsetcurve][edit][refusal]") {
    const RS_Ellipse source{nullptr, shortArc()};
    for (const double distance : {-1.0, 0.0, std::numeric_limits<double>::infinity(), std::numeric_limits<double>::quiet_NaN()}) {
        auto made = LC_OffsetCurve::create(source, distance, false, {}, 0.01);
        CHECK_FALSE(made); CHECK_FALSE(made.entity); CHECK(made.refusal == Refusal::InvalidInput);
        CHECK_FALSE(made.changed); notRun(made);
    }
    for (const double tolerance : {-1.0, std::numeric_limits<double>::infinity(), std::numeric_limits<double>::quiet_NaN()}) {
        auto made = LC_OffsetCurve::create(source, 1, false, {}, tolerance);
        CHECK_FALSE(made); CHECK(made.refusal == Refusal::InvalidInput); notRun(made);
    }
    auto badBasis = shortArc(); badBasis.center.z = 1;
    const RS_Ellipse nonplanar{nullptr, badBasis};
    auto invalid = LC_OffsetCurve::create(nonplanar, 1, false, {}, 0.01);
    CHECK(invalid.refusal == Refusal::InvalidInput); CHECK_FALSE(invalid.entity); notRun(invalid);
    LC_OffsetRunLimits zeroBudget; zeroBudget.outputBudget.maxOutputEntities = 0;
    invalid = LC_OffsetCurve::create(source, 1, false, zeroBudget, 0.01);
    CHECK(invalid.refusal == Refusal::InvalidInput); notRun(invalid);
    auto tiny = LC_OffsetCurve::create(source, 1e-8, false, {}, 0.01);
    CHECK_FALSE(tiny); CHECK_FALSE(tiny.entity); CHECK(tiny.refusal == Refusal::TooSmall); notRun(tiny);
}

TEST_CASE("Offset engine zero and multiple outputs retain their real component statuses",
          "[lc-offsetcurve][edit][refusal]") {
    const RS_Ellipse circle{nullptr, RS_EllipseData{{0, 0}, {5, 0}, 1}};
    auto empty = LC_OffsetCurve::create(circle, 5, true, {}, 1e-4);
    CHECK_FALSE(empty); CHECK_FALSE(empty.entity); CHECK_FALSE(empty.changed);
    CHECK(empty.refusal == Refusal::NothingLeft); CHECK(empty.engineRuns == 1);
    REQUIRE(empty.engineStatus); CHECK(*empty.engineStatus == LC_CurveOffsetStatus::Ok); CHECK_FALSE(empty.checkReason);
    auto tinyBasis = shortArc(); tinyBasis.angle2 = 0.0001;
    const RS_Ellipse tinyArc{nullptr, tinyBasis};
    auto unresolved = LC_OffsetCurve::create(tinyArc, 0.25, false, {}, 0.01);
    CHECK_FALSE(unresolved); CHECK_FALSE(unresolved.entity); CHECK_FALSE(unresolved.changed);
    CHECK(unresolved.refusal == Refusal::NothingLeft); CHECK(unresolved.engineRuns == 1);
    REQUIRE(unresolved.engineStatus); CHECK(*unresolved.engineStatus == LC_CurveOffsetStatus::Ok);
    CHECK_FALSE(unresolved.checkReason);
    const double pi = std::acos(-1.0);
    const RS_Ellipse split{nullptr, RS_EllipseData{{0, 0}, {100, 0}, 0.05, pi/12, 7*pi/4, false}};
    auto several = LC_OffsetCurve::create(split, 4, true, {}, 1e-5);
    CHECK_FALSE(several); CHECK_FALSE(several.entity); CHECK_FALSE(several.changed);
    CHECK(several.refusal == Refusal::SeveralPieces); CHECK(several.engineRuns == 1);
    REQUIRE(several.engineStatus); CHECK(*several.engineStatus == LC_CurveOffsetStatus::Ok); CHECK_FALSE(several.checkReason);
}

TEST_CASE("Every exact offset edit no-op retains the complete live state and not-run statuses",
          "[lc-offsetcurve][edit][noop]") {
    for (unsigned operation = 0; operation < 5; ++operation) {
        CAPTURE(operation);
        auto curve = adoptedArc(); const State before{*curve};
        LC_OffsetRunLimits limits; limits.maxSamples = 1; limits.checkLimits.maxCacheSpans = 1;
        LC_OffsetCurveEditResult result;
        switch (operation) {
        case 0: result = curve->setDefinition(curve->definition(), limits); break;
        case 1: result = curve->setDistance(curve->definition().distance, limits); break;
        case 2: result = curve->setTowardCentre(curve->definition().towardCentre, limits); break;
        case 3: result = curve->setBasis(curve->definition().basis, limits); break;
        case 4: result = curve->setThrough({1.25, 0}, limits); break;
        }
        CHECK(result); CHECK(result.refusal == Refusal::None); CHECK_FALSE(result.changed); notRun(result); before.check(*curve);
    }
}

TEST_CASE("Adopted positive tolerances are not restricted by future file-schema limits",
          "[lc-offsetcurve][edit][noop]") {
    auto curve = adoptedArc(0.25, {0, 0}, 2.0); const State before{*curve};
    const auto result = curve->setDefinition(curve->definition());
    CHECK(result); CHECK_FALSE(result.changed); notRun(result); before.check(*curve);
}

TEST_CASE("All meaningful offset definition edits regenerate once preserve attributes and clear extensions",
          "[lc-offsetcurve][edit]") {
    for (unsigned operation = 0; operation < 6; ++operation) {
        CAPTURE(operation);
        auto curve = adoptedArc(); const Attributes attributes{*curve};
        auto definition = curve->definition();
        LC_OffsetCurveEditResult result;
        switch (operation) {
        case 0: result = curve->setDistance(0.5); break;
        case 1: result = curve->setTowardCentre(true); break;
        case 2: definition.basis.ratio = 0.8; result = curve->setBasis(definition.basis); break;
        case 3: definition.distance = 0.5; definition.basis.ratio = 0.75; result = curve->setDefinition(definition); break;
        case 4: definition.relTolerance = 0.005; result = curve->setDefinition(definition); break;
        case 5: definition.cacheReversed = true; result = curve->setDefinition(definition); break;
        }
        regenerated(result, *curve); attributes.check(*curve);
    }
}

TEST_CASE("Offset edits reject invalid and too-small definitions without disturbing any live state",
          "[lc-offsetcurve][edit][refusal]") {
    for (unsigned invalid = 0; invalid < 10; ++invalid) {
        CAPTURE(invalid);
        auto curve = adoptedArc(); const State before{*curve}; auto definition = curve->definition();
        if (invalid == 0) definition.distance = -1;
        if (invalid == 1) definition.distance = 0;
        if (invalid == 2) definition.distance = std::numeric_limits<double>::quiet_NaN();
        if (invalid == 3) definition.distance = std::numeric_limits<double>::infinity();
        if (invalid == 4) definition.relTolerance = 0;
        if (invalid == 5) definition.relTolerance = std::numeric_limits<double>::quiet_NaN();
        if (invalid == 6) definition.basis.center.z = 1;
        if (invalid == 7) definition.basis.majorP = {0, 0};
        if (invalid == 8) definition.basis.center.x = std::numeric_limits<double>::infinity();
        if (invalid == 9) definition.distance = 1e-8;
        const auto result = curve->setDefinition(definition);
        CHECK_FALSE(result); CHECK_FALSE(result.changed);
        CHECK(result.refusal == (invalid == 9 ? Refusal::TooSmall : Refusal::InvalidInput));
        notRun(result); before.check(*curve);
    }
}

TEST_CASE("Every offset setter refuses undefined or inconsistent live caches before running stages",
          "[lc-offsetcurve][edit][refusal]") {
    for (const bool undefined : {false, true}) for (unsigned operation = 0; operation < 5; ++operation) {
        CAPTURE(undefined, operation);
        auto curve = adoptedArc();
        if (undefined) { curve->scale({0, 0}, {2, 3}); REQUIRE_FALSE(curve->isDefined()); }
        else { curve->getData().controlPoints[0].x += 0.25; REQUIRE_FALSE(curve->isConsistent()); }
        const State before{*curve}; auto next = curve->definition(); next.distance = 0.5;
        LC_OffsetCurveEditResult result;
        switch (operation) {
        case 0: result = curve->setDefinition(next); break;
        case 1: result = curve->setDistance(0.5); break;
        case 2: result = curve->setTowardCentre(true); break;
        case 3: result = curve->setBasis(shortArc()); break;
        case 4: result = curve->setThrough({1.5, 0}); break;
        }
        CHECK_FALSE(result); CHECK_FALSE(result.changed);
        CHECK(result.refusal == (undefined ? Refusal::Undefined : Refusal::Inconsistent));
        notRun(result); CHECK_FALSE(curve->proposeThrough({1.5, 0})); before.check(*curve);
    }
}

TEST_CASE("Real offset regeneration failures are transactional across all produced-result stages",
          "[lc-offsetcurve][edit][refusal]") {
    const double pi = std::acos(-1.0);
    for (unsigned failure = 0; failure < 5; ++failure) {
        CAPTURE(failure);
        auto curve = adoptedArc(); const State before{*curve};
        auto next = curve->definition(); next.basis = {{0, 0}, {5, 0}, 1}; next.distance = 1; next.relTolerance = 1e-4;
        LC_OffsetRunLimits limits;
        if (failure == 0) { next.distance = 5; next.towardCentre = true; }
        if (failure == 1) { next.basis = {{0, 0}, {100, 0}, 0.05, pi/12, 7*pi/4, false}; next.distance = 4; next.towardCentre = true; next.relTolerance = 1e-5; }
        if (failure == 2) limits.maxSamples = 1;
        if (failure == 3) limits.outputBudget.maxDeepEntities = 1;
        if (failure == 4) limits.checkLimits.maxCacheSpans = 1;
        const auto result = curve->setDefinition(next, limits);
        CHECK_FALSE(result); CHECK_FALSE(result.changed); CHECK(result.engineRuns == 1); REQUIRE(result.engineStatus);
        if (failure == 0 || failure == 1) {
            CHECK(result.refusal == (failure == 0 ? Refusal::NothingLeft : Refusal::SeveralPieces));
            CHECK(*result.engineStatus == LC_CurveOffsetStatus::Ok); CHECK_FALSE(result.checkReason);
        } else if (failure == 2 || failure == 3) {
            CHECK(result.refusal == Refusal::Engine); CHECK(*result.engineStatus == LC_CurveOffsetStatus::LimitExceeded);
            CHECK_FALSE(result.checkReason);
        } else {
            CHECK(result.refusal == Refusal::CheckFailed); CHECK(*result.engineStatus == LC_CurveOffsetStatus::Ok);
            REQUIRE(result.checkReason); CHECK(*result.checkReason == CheckReason::LimitExceeded);
        }
        before.check(*curve);
    }
}

TEST_CASE("Raised optional engine and output limits only retain the actual default behavior",
          "[lc-offsetcurve][edit][limits]") {
    const RS_Ellipse source{nullptr, shortArc()};
    auto normal = LC_OffsetCurve::create(source, 0.25, false, {}, 0.01);
    REQUIRE(normal.entity);
    LC_OffsetRunLimits raised;
    raised.maxSamples = raised.maxIntersectionPairs = std::numeric_limits<std::size_t>::max();
    raised.outputBudget = {raised.maxSamples, raised.maxSamples, raised.maxSamples};
    auto result = LC_OffsetCurve::create(source, 0.25, false, raised, 0.01);
    REQUIRE(result.entity); regenerated(result, *result.entity);
    CHECK(RS_Spline::geometryFingerprint(result.entity->getData()) == RS_Spline::geometryFingerprint(normal.entity->getData()));
}

TEST_CASE("Exact dyadic offset basis translation transports the existing proof without running stages",
          "[lc-offsetcurve][edit][basis]") {
    auto curve = adoptedRing(); const Attributes attributes{*curve}; const auto old = curve->getData();
    const auto layout = geo::cacheLayoutCountsFromControls(old.degree, old.controlPoints.size());
    REQUIRE(layout); REQUIRE(layout->spans == 4);
    const auto cost = measureOffsetOutput({curve.get()}, makeDefaultOffsetSourceBudget().maxDeepEntities);
    REQUIRE(cost.status == LC_OffsetTreeStatus::Ok);
    LC_OffsetRunLimits limits; limits.outputBudget = {layout->spans, 1, cost.deepEntities};
    auto basis = curve->definition().basis; basis.center = {4, 8};
    const auto result = curve->setBasis(basis, limits);
    REQUIRE(result); CHECK(result.changed); notRun(result); attributes.check(*curve);
    CHECK(curve->unknownOwnedFlags() == 0); CHECK(curve->unknownOwnedTail().empty());
    REQUIRE(curve->getData().controlPoints.size() == old.controlPoints.size());
    for (std::size_t i = 0; i < old.controlPoints.size(); ++i)
        CHECK(samePoint(curve->getData().controlPoints[i], old.controlPoints[i] + RS_Vector{4, 8}));
    const auto translatedCost = measureOffsetOutput({curve.get()}, limits.outputBudget.maxDeepEntities);
    CHECK(translatedCost.status == LC_OffsetTreeStatus::Ok); CHECK(translatedCost.deepEntities == cost.deepEntities);
    certified(*curve);
}

TEST_CASE("Exact offset translations cannot bypass lowered output span or leaf budgets",
          "[lc-offsetcurve][edit][basis][limits]") {
    for (const bool leaves : {false, true}) {
        CAPTURE(leaves);
        auto curve = adoptedRing(); const State before{*curve};
        LC_OffsetRunLimits limits;
        if (leaves) limits.outputBudget.maxDeepEntities = 1;
        else limits.outputBudget.maxCubicPieces = 1;
        const auto unchanged = curve->setBasis(curve->definition().basis, limits);
        REQUIRE(unchanged); CHECK_FALSE(unchanged.changed); notRun(unchanged); before.check(*curve);
        auto basis = curve->definition().basis; basis.center = {4, 8};
        const auto refused = curve->setBasis(basis, limits);
        const int engineStatus = refused.engineStatus ? static_cast<int>(*refused.engineStatus) : -1;
        const int checkReason = refused.checkReason ? static_cast<int>(*refused.checkReason) : -1;
        CAPTURE(refused.refusal, engineStatus, checkReason);
        REQUIRE_FALSE(refused); CHECK_FALSE(refused.changed); CHECK(refused.engineRuns == 1);
        REQUIRE(refused.engineStatus);
        if (refused.refusal == Refusal::Engine) {
            CHECK(*refused.engineStatus == LC_CurveOffsetStatus::LimitExceeded); CHECK_FALSE(refused.checkReason);
            CHECK(refused.checkWork.cacheSpans == 0); CHECK(refused.checkWork.vertices == 0);
            CHECK(refused.checkWork.distancePairs == 0); CHECK(refused.checkWork.orderingComparisons == 0);
            CHECK(refused.checkWork.deepest == 0);
        } else {
            CHECK(refused.refusal == Refusal::NotCacheRepresentable);
            CHECK(*refused.engineStatus == LC_CurveOffsetStatus::Ok);
            REQUIRE(refused.checkReason); CHECK(*refused.checkReason == CheckReason::None);
            CHECK(refused.checkWork.cacheSpans > 0);
        }
        before.check(*curve);
    }
}

TEST_CASE("Rounded offset translations regenerate instead of transporting an unproved fingerprint",
          "[lc-offsetcurve][edit][basis]") {
    auto curve = adoptedArc(); const Attributes attributes{*curve};
    auto basis = curve->definition().basis; basis.center = {4, 8};
    const auto result = curve->setBasis(basis);
    regenerated(result, *curve); attributes.check(*curve);
    CHECK(samePoint(curve->definition().basis.center, basis.center));
}

TEST_CASE("Offset basis changes compare every directed semantic field before the rigid fast path",
          "[lc-offsetcurve][edit][basis]") {
    const double pi = std::acos(-1.0);
    for (unsigned change = 0; change < 6; ++change) {
        CAPTURE(change);
        auto curve = adoptedArc(); const Attributes attributes{*curve}; auto basis = curve->definition().basis;
        if (change == 0) basis.angle1 = 0.02;
        if (change == 1) basis.angle2 = 0.2;
        if (change == 2) basis.majorP.x = 2;
        if (change == 3) basis.ratio = 0.8;
        if (change == 4) { std::swap(basis.angle1, basis.angle2); basis.reversed = true; }
        if (change == 5) { basis.majorP = {-1, 0}; basis.angle1 += pi; basis.angle2 += pi; basis.center = {4, 8}; }
        REQUIRE(geo::normaliseBasis(basis));
        const auto result = curve->setBasis(basis);
        regenerated(result, *curve); attributes.check(*curve); checkBasis(curve->definition().basis, basis);
    }
}

TEST_CASE("Through proposals are analytic read-only queries and reject invalid or out-of-sweep feet",
          "[lc-offsetcurve][edit][through]") {
    auto curve = adoptedArc(); const State before{*curve};
    for (unsigned repeat = 0; repeat < 8; ++repeat) {
        const auto proposal = curve->proposeThrough({0.5, 0});
        REQUIRE(proposal); CHECK(proposal->distance == Catch::Approx(0.5));
        CHECK(proposal->towardCentre); CHECK(proposal->parameter == Catch::Approx(0).margin(1e-12));
    }
    for (const RS_Vector point : {RS_Vector{false}, RS_Vector{0, 2}, RS_Vector{1.5, 0, 1},
         RS_Vector{std::numeric_limits<double>::infinity(), 0}, RS_Vector{std::numeric_limits<double>::quiet_NaN(), 0}}) {
        CHECK_FALSE(curve->proposeThrough(point));
        const auto result = curve->setThrough(point);
        CHECK_FALSE(result); CHECK(result.refusal == Refusal::InvalidInput); CHECK_FALSE(result.changed); notRun(result);
        before.check(*curve);
    }
    before.check(*curve);
}

TEST_CASE("Through edits commit distance and side together with an actual cache endpoint witness",
          "[lc-offsetcurve][edit][through]") {
    auto curve = adoptedArc(); const Attributes attributes{*curve};
    const RS_Vector point{0.5, 0};
    const auto result = curve->setThrough(point);
    regenerated(result, *curve); attributes.check(*curve);
    CHECK(curve->definition().distance == Catch::Approx(0.5)); CHECK(curve->definition().towardCentre);
    CHECK(hasEndpointWitness(*curve, point));
    checkWork(result.throughWork, {0, 1, 1}); combinedWorkFits(result);
    const auto checked = geo::checkCache(curve->getData(), curve->definition(), curve->checkedWorldTolerance());
    REQUIRE(checked); checkWork(result.checkWork, checked.work);
    decorate(*curve); const State before{*curve};
    const auto unchanged = curve->setThrough(curve->getData().controlPoints.front());
    REQUIRE(unchanged); CHECK_FALSE(unchanged.changed); notRun(unchanged);
    checkWork(unchanged.throughWork, {}); before.check(*curve);
}

TEST_CASE("Through edits certify an interior cache point rather than requiring a cache endpoint",
          "[lc-offsetcurve][edit][through]") {
    const RS_Ellipse source{nullptr, RS_EllipseData{{0, 0}, {10, 0}, 1, 0, 1.2, false}};
    auto made = LC_OffsetCurve::create(source, 1, false, {}, 0.01);
    REQUIRE(made.entity); regenerated(made, *made.entity);
    decorate(*made.entity); const Attributes attributes{*made.entity};
    const RS_Vector point{12*std::cos(0.5), 12*std::sin(0.5)};
    const auto proposal = made.entity->proposeThrough(point);
    REQUIRE(proposal); CHECK(proposal->distance == Catch::Approx(2)); CHECK_FALSE(proposal->towardCentre);
    const auto result = made.entity->setThrough(point);
    regenerated(result, *made.entity); attributes.check(*made.entity);
    CHECK(made.entity->definition().distance == Catch::Approx(2));
    CHECK_FALSE(hasEndpointWitness(*made.entity, point)); CHECK(hasInteriorWitness(*made.entity, point));
    CHECK(result.throughWork.vertices > 0); CHECK(result.throughWork.distancePairs > 0);
    CHECK(result.throughWork.deepest > 0); combinedWorkFits(result);
    decorate(*made.entity); const State before{*made.entity};
    const auto unchanged = made.entity->setThrough(point);
    REQUIRE(unchanged); CHECK_FALSE(unchanged.changed); notRun(unchanged); before.check(*made.entity);
    CHECK(unchanged.throughWork.vertices > 0); CHECK(unchanged.throughWork.distancePairs > 0);
    CHECK(unchanged.throughWork.deepest > 0); combinedWorkFits(unchanged);
}

TEST_CASE("Through edits spend only the checker remaining vertex and pair allowances",
          "[lc-offsetcurve][edit][through][limits]") {
    auto control = adoptedArc(); auto next = control->definition(); next.distance = 0.5; next.towardCentre = true;
    const auto completed = control->setDefinition(next);
    regenerated(completed, *control);
    REQUIRE(completed.checkWork.vertices > 0); REQUIRE(completed.checkWork.distancePairs > 0);
    checkWork(completed.throughWork, {});
    for (const bool vertices : {false, true}) {
        CAPTURE(vertices);
        auto curve = adoptedArc(); const State before{*curve}; LC_OffsetRunLimits limits;
        if (vertices) limits.checkLimits.maxVertices = completed.checkWork.vertices;
        else limits.checkLimits.maxDistancePairs = completed.checkWork.distancePairs;
        const auto result = curve->setThrough({0.5, 0}, limits);
        CHECK_FALSE(result); CHECK_FALSE(result.changed); CHECK(result.refusal == Refusal::ThroughNotOnCache);
        CHECK(result.engineRuns == 1); REQUIRE(result.engineStatus); CHECK(*result.engineStatus == LC_CurveOffsetStatus::Ok);
        REQUIRE(result.checkReason); CHECK(*result.checkReason == CheckReason::None);
        CHECK(result.checkWork.vertices == completed.checkWork.vertices);
        CHECK(result.checkWork.distancePairs == completed.checkWork.distancePairs);
        checkWork(result.checkWork, completed.checkWork); checkWork(result.throughWork, {});
        combinedWorkFits(result, limits.checkLimits);
        before.check(*curve);
    }

    LC_OffsetRunLimits exact;
    exact.checkLimits.maxVertices = completed.checkWork.vertices + 1;
    exact.checkLimits.maxDistancePairs = completed.checkWork.distancePairs + 1;
    exact.checkLimits.maxDepth = std::max(1u, completed.checkWork.deepest);
    auto curve = adoptedArc();
    const auto result = curve->setThrough({0.5, 0}, exact);
    regenerated(result, *curve); checkWork(result.checkWork, completed.checkWork);
    checkWork(result.throughWork, {0, 1, 1}); combinedWorkFits(result, exact.checkLimits);
}

TEST_CASE("Through membership reports actual partial work without partial endpoint or child debits",
          "[lc-offsetcurve][edit][through][limits][accounting]") {
    auto curve = adoptedThroughLine(); const State before{*curve};
    const RS_Vector point{2, 0};
    const auto proposal = curve->proposeThrough(point);
    REQUIRE(proposal); REQUIRE(proposal->distance == curve->definition().distance);
    REQUIRE(proposal->towardCentre == curve->definition().towardCentre);
    REQUIRE_FALSE(hasEndpointWitness(*curve, point)); REQUIRE(hasInteriorWitness(*curve, point));
    for (unsigned resource = 0; resource < 7; ++resource) {
        CAPTURE(resource);
        LC_OffsetRunLimits limits;
        geo::CheckWork expected;
        switch (resource) {
        case 0: limits.checkLimits.maxVertices = 0; break;
        case 1: limits.checkLimits.maxDistancePairs = 0; break;
        case 2:
            limits.checkLimits.maxVertices = 1; expected = {0, 1, 1}; break;
        case 3:
            limits.checkLimits.maxDistancePairs = 1; expected = {0, 1, 1}; break;
        case 4:
            // Two endpoints and a root were entered, but two child slots do not fit.
            limits.checkLimits.maxVertices = 4; expected = {0, 3, 4}; break;
        case 5:
            // Root children are queued, but neither has an admitted distance pair.
            limits.checkLimits.maxDistancePairs = 4; expected = {0, 5, 4}; break;
        case 6:
            limits.checkLimits.maxDistancePairs = 5; expected = {0, 5, 5, 1}; break;
        }
        const auto refused = curve->setThrough(point, limits);
        CHECK_FALSE(refused); CHECK_FALSE(refused.changed); CHECK(refused.refusal == Refusal::ThroughNotOnCache);
        notRun(refused); checkWork(refused.throughWork, expected);
        combinedWorkFits(refused, limits.checkLimits); before.check(*curve);
    }

    LC_OffsetRunLimits shallow; shallow.checkLimits.maxDepth = 1;
    const auto refused = curve->setThrough(point, shallow);
    CHECK_FALSE(refused); CHECK_FALSE(refused.changed); CHECK(refused.refusal == Refusal::ThroughNotOnCache);
    notRun(refused); checkWork(refused.throughWork, {0, 5, 8, 1});
    combinedWorkFits(refused, shallow.checkLimits); before.check(*curve);

    const auto completed = curve->setThrough(point);
    REQUIRE(completed); CHECK_FALSE(completed.changed); notRun(completed);
    CHECK(completed.throughWork.vertices > 5); CHECK(completed.throughWork.distancePairs > 8);
    CHECK(completed.throughWork.deepest > 1); combinedWorkFits(completed); before.check(*curve);

    LC_OffsetRunLimits raised;
    raised.checkLimits = {std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max(),
        std::numeric_limits<std::size_t>::max(), std::numeric_limits<unsigned>::max(),
        std::numeric_limits<std::size_t>::max()};
    const auto clamped = curve->setThrough(point, raised);
    REQUIRE(clamped); CHECK_FALSE(clamped.changed); notRun(clamped);
    checkWork(clamped.throughWork, completed.throughWork); combinedWorkFits(clamped, raised.checkLimits);
    before.check(*curve);

    // A later short refusal must report its own prefix, not a prior successful traversal.
    const auto retained = completed.throughWork;
    const auto again = curve->setThrough(point, shallow);
    checkWork(again.throughWork, refused.throughWork); checkWork(completed.throughWork, retained);
    before.check(*curve);
}

TEST_CASE("Regenerated through checks share counts but not serial depth with C",
          "[lc-offsetcurve][edit][through][limits][accounting]") {
    const RS_Ellipse source{nullptr, RS_EllipseData{{0, 0}, {10, 0}, 1, 0, 1.2, false}};
    const RS_Vector point{12*std::cos(0.5), 12*std::sin(0.5)};
    auto control = LC_OffsetCurve::create(source, 1, false, {}, 0.01);
    REQUIRE(control.entity);
    const auto completed = control.entity->setThrough(point);
    regenerated(completed, *control.entity); REQUIRE(completed.throughWork.deepest > 0);
    const auto checked = geo::checkCache(control.entity->getData(), control.entity->definition(),
        control.entity->checkedWorldTolerance());
    REQUIRE(checked); checkWork(completed.checkWork, checked.work); combinedWorkFits(completed);

    LC_OffsetRunLimits exact;
    exact.checkLimits.maxVertices = completed.checkWork.vertices + completed.throughWork.vertices;
    exact.checkLimits.maxDistancePairs = completed.checkWork.distancePairs + completed.throughWork.distancePairs;
    // These are successive queries, not nested recursion. Both must fit the
    // same ceiling; adding/subtracting their independent depths is incorrect.
    exact.checkLimits.maxDepth = std::max(completed.checkWork.deepest, completed.throughWork.deepest);
    auto replay = LC_OffsetCurve::create(source, 1, false, {}, 0.01);
    REQUIRE(replay.entity);
    const auto repeated = replay.entity->setThrough(point, exact);
    regenerated(repeated, *replay.entity); checkWork(repeated.checkWork, completed.checkWork);
    checkWork(repeated.throughWork, completed.throughWork); combinedWorkFits(repeated, exact.checkLimits);

    REQUIRE(completed.checkWork.deepest > 0);
    REQUIRE(exact.checkLimits.maxDepth < completed.checkWork.deepest + completed.throughWork.deepest);
}

TEST_CASE("Through edits near a large world center use local geometry not a world-sized epsilon",
          "[lc-offsetcurve][edit][through]") {
    const RS_Vector center{1e12, 1e12};
    const auto produce = [](const RS_Ellipse& source) {
        const double tolerance = LC_CurveOffset::toleranceFor(source, 0.5, 0.1);
        const auto options = LC_CurveOffset::makeOffsetOptions(source, 0.5, tolerance);
        return LC_CurveOffset::createEntities(source,
            LC_CurveOffset::makeSideRequest(LC_CurveOffsetSide::Right, 0.5), options, makeDefaultOffsetSourceBudget());
    };
    // At this center the producer's tangent floor is 16*.01421/spanWidth.
    // The .05 span cannot certify unit speed; neither can the short tail of
    // a 1-radian arc after its mandatory pi/4 break. Both 1.2-radian spans can.
    const RS_Ellipse narrow{nullptr, RS_EllipseData{center, {1, 0}, 1, 0, 0.05, false}};
    auto unresolved = produce(narrow);
    CAPTURE(unresolved.status);
    REQUIRE(unresolved.status == LC_CurveOffsetStatus::UndefinedTangent); CHECK(unresolved.entities.empty());
    const RS_Ellipse source{nullptr, RS_EllipseData{center, {1, 0}, 1, 0, 1.2, false}};
    auto control = produce(source);
    CAPTURE(control.status); REQUIRE(control.status == LC_CurveOffsetStatus::Ok); REQUIRE(control.entities.size() == 1);
    auto made = LC_OffsetCurve::create(source, 0.25, false, {}, 0.1);
    REQUIRE(made.entity); regenerated(made, *made.entity);
    auto curve = std::move(made.entity); decorate(*curve); const State before{*curve};
    const RS_Vector outside = center + RS_Vector{0, 2};
    CHECK_FALSE(curve->proposeThrough(outside));
    const auto refused = curve->setThrough(outside);
    CHECK(refused.refusal == Refusal::InvalidInput); notRun(refused); before.check(*curve);
    const RS_Vector point = center + RS_Vector{1.5, 0};
    const Attributes attributes{*curve}; const auto result = curve->setThrough(point);
    regenerated(result, *curve); attributes.check(*curve);
    CHECK(curve->definition().distance == Catch::Approx(0.5)); CHECK_FALSE(curve->definition().towardCentre);
    CHECK(hasEndpointWitness(*curve, point));
}

TEST_CASE("A removed elliptic loop cannot supply its obsolete normal distance to a through edit",
          "[lc-offsetcurve][edit][through][lens]") {
    const RS_Ellipse source{nullptr, RS_EllipseData{{0, 0}, {100, 0}, 0.5}};
    auto made = LC_OffsetCurve::create(source, 49, true, {}, 1e-4);
    REQUIRE(made.entity); regenerated(made, *made.entity);
    const State before{*made.entity}; const RS_Vector point{51, 0};
    const auto proposal = made.entity->proposeThrough(point);
    REQUIRE(proposal); CHECK(proposal->towardCentre);
    CHECK(proposal->distance == Catch::Approx(std::sqrt(1633.0)).margin(1e-8));
    CHECK(proposal->distance < 49); before.check(*made.entity);
    const auto result = made.entity->setThrough(point);
    regenerated(result, *made.entity);
    CHECK(made.entity->definition().distance == Catch::Approx(proposal->distance));
    CHECK(hasEndpointWitness(*made.entity, point));
}
