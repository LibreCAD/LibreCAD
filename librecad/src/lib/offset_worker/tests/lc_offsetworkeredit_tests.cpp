/*
 * This file is part of LibreCAD, a 2D CAD program.
 * Copyright (C) 2026 LibreCAD.org
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "../lc_offsetworkeredit.h"
#include "drw_base.h"
#include "lc_offsetoutputbudget.h"
#include "lc_testsettingsisolation.h"
#include "rs_layer.h"
#include "rs_line.h"

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace {
using Edit = LC_OffsetWorkerEdit;
using Preparation = Edit::Preparation;
using Refusal = LC_OffsetCurveRefusal;
namespace geo = LC_EllipseOffsetGeometry;

static_assert(!std::is_default_constructible_v<Preparation>);
static_assert(!std::is_copy_constructible_v<Preparation>);
static_assert(!std::is_copy_assignable_v<Preparation>);
static_assert(std::is_nothrow_move_constructible_v<Preparation>);
static_assert(std::is_nothrow_move_assignable_v<Preparation>);
static_assert(!std::is_constructible_v<Preparation, LC_OffsetCurveEditResult>);
static_assert(!std::is_constructible_v<Preparation, LC_OffsetWorkerRequest::Generate>);
static_assert(std::is_same_v<decltype(std::declval<const Preparation&>().result()),
                             const LC_OffsetCurveEditResult&>);
static_assert(std::is_same_v<decltype(std::declval<const Preparation&>().generate()),
                             const std::optional<LC_OffsetWorkerRequest::Generate>&>);

void requireSettingsIsolation() {
    // The test provider owns the settings listener; no window or helper process is needed.
    REQUIRE(lc::test::SettingsIsolation::isActive());
    REQUIRE(QSettings::defaultFormat() == QSettings::IniFormat);
}

std::uint64_t bits(const double value) {
    std::uint64_t result;
    static_assert(sizeof(result) == sizeof(value));
    std::memcpy(&result, &value, sizeof(result));
    return result;
}

bool samePoint(const RS_Vector& actual, const RS_Vector& expected) {
    return bits(actual.x) == bits(expected.x) && bits(actual.y) == bits(expected.y) &&
        bits(actual.z) == bits(expected.z) && actual.valid == expected.valid;
}

void checkPoints(const std::vector<RS_Vector>& actual, const std::vector<RS_Vector>& expected) {
    REQUIRE(actual.size() == expected.size());
    for (std::size_t i = 0; i < actual.size(); ++i) {
        CAPTURE(i);
        CHECK(samePoint(actual[i], expected[i]));
    }
}

void checkNumbers(const std::vector<double>& actual, const std::vector<double>& expected) {
    REQUIRE(actual.size() == expected.size());
    for (std::size_t i = 0; i < actual.size(); ++i) {
        CAPTURE(i);
        CHECK(bits(actual[i]) == bits(expected[i]));
    }
}

void checkDefinition(const LC_EllipseOffsetDefinition& actual, const LC_EllipseOffsetDefinition& expected) {
    CHECK(samePoint(actual.basis.center, expected.basis.center));
    CHECK(samePoint(actual.basis.majorP, expected.basis.majorP));
    CHECK(bits(actual.basis.ratio) == bits(expected.basis.ratio));
    CHECK(bits(actual.basis.angle1) == bits(expected.basis.angle1));
    CHECK(bits(actual.basis.angle2) == bits(expected.basis.angle2));
    CHECK(actual.basis.reversed == expected.basis.reversed);
    CHECK(actual.basis.isArc == expected.basis.isArc);
    CHECK(bits(actual.basis.angleDegrees) == bits(expected.basis.angleDegrees));
    CHECK(bits(actual.basis.startAngleDegrees) == bits(expected.basis.startAngleDegrees));
    CHECK(bits(actual.basis.otherAngleDegrees) == bits(expected.basis.otherAngleDegrees));
    CHECK(bits(actual.basis.angularLength) == bits(expected.basis.angularLength));
    CHECK(bits(actual.distance) == bits(expected.distance));
    CHECK(bits(actual.relTolerance) == bits(expected.relTolerance));
    CHECK(actual.towardCentre == expected.towardCentre);
    CHECK(actual.cacheReversed == expected.cacheReversed);
}

void checkSpline(const RS_SplineData& actual, const RS_SplineData& expected) {
    checkPoints(actual.controlPoints, expected.controlPoints);
    checkPoints(actual.fitPoints, expected.fitPoints);
    checkNumbers(actual.knotslist, expected.knotslist);
    checkNumbers(actual.weights, expected.weights);
    checkNumbers(actual.savedOpenKnots, expected.savedOpenKnots);
    CHECK(actual.degree == expected.degree);
    CHECK(actual.type == expected.type);
    CHECK(actual.savedOpenType == expected.savedOpenType);
    CHECK(actual.m_closedFlag == expected.m_closedFlag);
    CHECK(actual.m_periodicFlag == expected.m_periodicFlag);
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
        default: FAIL("unexpected owned extension type");
        }
    }
}

void decorate(LC_OffsetCurve& curve) {
    curve.setVisible(false);
    curve.setFlag(RS2::FlagTransparent);
    curve.setFlag(RS2::FlagSelected);
    curve.setPen(RS_Pen{RS_Color{17, 31, 47}, RS2::Width11, RS2::DashLine});
    curve.setSourceHandle(101); curve.setXDictHandle(102); curve.setReactorHandles({103, 104});
    curve.setMaterialHandle(105); curve.setPlotStyleHandle(106);
    curve.setShadowMode(2); curve.setShadowHandle(107); curve.setVisualStyleHandles(108, 109, 110);
    curve.setOwnerLineage(111);
    curve.setUserDefVar("D-worker-edit", "unrelated user value");
    curve.setDrwExtData({std::make_shared<DRW_Variant>(1001, std::string("D_WORKER_EDIT")),
                         std::make_shared<DRW_Variant>(1000, std::string("unrelated application data"))});
    REQUIRE(curve.setOwnedExtensions(0x30, {{1002, QStringLiteral("{")},
        {1000, QStringLiteral("reserved")}, {1010, RS_Vector{3, 4}}, {1040, -0.0},
        {1070, std::int16_t{7}}, {1071, std::int32_t{123456}}, {1002, QStringLiteral("}")}}));
}

struct Attributes {
    RS_EntityContainer* parent;
    RS_Layer* layer;
    RS_Pen pen;
    unsigned flags;
    unsigned long long id, lineage, ownerLineage;
    quint32 sourceHandle, dictionary;
    std::vector<quint32> reactors;
    std::array<quint32, 6> passive;
    int shadowMode;
    QString userValue;
    std::vector<std::pair<int, std::string>> xdata;

    explicit Attributes(const RS_Entity& entity)
        : parent(entity.getParent()), layer(entity.getLayer(false)), pen(entity.getPen(false)),
          flags(entity.getFlags()), id(entity.getId()), lineage(entity.lineage()), ownerLineage(entity.ownerLineage()),
          sourceHandle(entity.sourceHandle()), dictionary(entity.xDictHandle()), reactors(entity.reactorHandles()),
          passive{entity.materialHandle(), entity.plotStyleHandle(), entity.shadowHandle(), entity.fullVisualStyleHandle(),
                  entity.faceVisualStyleHandle(), entity.edgeVisualStyleHandle()}, shadowMode(entity.shadowMode()),
          userValue(entity.getUserDefVar("D-worker-edit")) {
        for (const auto& item : entity.getDrwExtData()) {
            REQUIRE(item);
            REQUIRE(item->type() == DRW_Variant::STRING);
            xdata.emplace_back(item->code(), *item->content.s);
        }
    }

    void check(const RS_Entity& entity) const {
        CHECK(entity.getParent() == parent); CHECK(entity.getLayer(false) == layer);
        CHECK(entity.getPen(false) == pen); CHECK(entity.getFlags() == flags);
        CHECK(entity.getId() == id); CHECK(entity.lineage() == lineage); CHECK(entity.ownerLineage() == ownerLineage);
        CHECK(entity.sourceHandle() == sourceHandle); CHECK(entity.xDictHandle() == dictionary);
        CHECK(entity.reactorHandles() == reactors);
        CHECK(entity.materialHandle() == passive[0]); CHECK(entity.plotStyleHandle() == passive[1]);
        CHECK(entity.shadowHandle() == passive[2]); CHECK(entity.fullVisualStyleHandle() == passive[3]);
        CHECK(entity.faceVisualStyleHandle() == passive[4]); CHECK(entity.edgeVisualStyleHandle() == passive[5]);
        CHECK(entity.shadowMode() == shadowMode); CHECK(entity.getUserDefVar("D-worker-edit") == userValue);
        REQUIRE(entity.getDrwExtData().size() == xdata.size());
        for (std::size_t i = 0; i < xdata.size(); ++i) {
            const auto& item = entity.getDrwExtData()[i];
            REQUIRE(item); REQUIRE(item->type() == DRW_Variant::STRING);
            CHECK(item->code() == xdata[i].first); CHECK(*item->content.s == xdata[i].second);
        }
    }
};

struct Snapshot {
    RS_SplineData data;
    LC_EllipseOffsetDefinition definition;
    Attributes attributes;
    std::uint64_t fingerprint;
    const RS_Vector* controls;
    const RS_Vector* fitPoints;
    const double* knots;
    const double* weights;
    const double* savedKnots;
    std::vector<RS_Vector> grips;
    const RS_Vector* gripStorage;
    LC_OffsetCurve::OwnedTail tail;
    const LC_OffsetCurveExtension* tailStorage;
    unsigned ownedFlags;
    double tolerance;
    bool defined, consistent, editable, whole, closed;
    RS_Vector minimum, maximum, middle;
    struct Child {
        const RS_Entity* identity;
        Attributes attributes;
        RS_Vector start, end, minimum, maximum;
    };
    std::vector<Child> children;

    explicit Snapshot(const LC_OffsetCurve& curve)
        : data(curve.getData()), definition(curve.definition()), attributes(curve),
          fingerprint(RS_Spline::geometryFingerprint(curve.getData())),
          controls(curve.getData().controlPoints.data()), fitPoints(curve.getData().fitPoints.data()),
          knots(curve.getData().knotslist.data()), weights(curve.getData().weights.data()),
          savedKnots(curve.getData().savedOpenKnots.data()), grips(curve.cachedDefinitionGrips()),
          gripStorage(curve.cachedDefinitionGrips().data()), tail(curve.unknownOwnedTail()),
          tailStorage(curve.unknownOwnedTail().data()), ownedFlags(curve.unknownOwnedFlags()),
          tolerance(curve.checkedWorldTolerance()), defined(curve.isDefined()), consistent(curve.isConsistent()),
          editable(curve.hasEditableDefinition()), whole(curve.sourceIsWhole()), closed(curve.cacheFormsClosedChain()),
          minimum(curve.getMin()), maximum(curve.getMax()), middle(curve.getMiddlePoint()) {
        for (const RS_Entity* child : curve) {
            REQUIRE(child);
            children.push_back({child, Attributes{*child}, child->getStartpoint(), child->getEndpoint(),
                                child->getMin(), child->getMax()});
        }
    }

    void check(const LC_OffsetCurve& curve) const {
        attributes.check(curve);
        checkSpline(curve.getData(), data); checkDefinition(curve.definition(), definition);
        CHECK(RS_Spline::geometryFingerprint(curve.getData()) == fingerprint);
        CHECK(curve.getData().controlPoints.data() == controls); CHECK(curve.getData().fitPoints.data() == fitPoints);
        CHECK(curve.getData().knotslist.data() == knots); CHECK(curve.getData().weights.data() == weights);
        CHECK(curve.getData().savedOpenKnots.data() == savedKnots);
        checkPoints(curve.cachedDefinitionGrips(), grips); CHECK(curve.cachedDefinitionGrips().data() == gripStorage);
        checkTail(curve.unknownOwnedTail(), tail); CHECK(curve.unknownOwnedTail().data() == tailStorage);
        CHECK(curve.unknownOwnedFlags() == ownedFlags); CHECK(bits(curve.checkedWorldTolerance()) == bits(tolerance));
        CHECK(curve.isDefined() == defined); CHECK(curve.isConsistent() == consistent);
        CHECK(curve.hasEditableDefinition() == editable); CHECK(curve.sourceIsWhole() == whole);
        CHECK(curve.cacheFormsClosedChain() == closed);
        CHECK(samePoint(curve.getMin(), minimum)); CHECK(samePoint(curve.getMax(), maximum));
        CHECK(samePoint(curve.getMiddlePoint(), middle));
        REQUIRE(curve.count() == children.size());
        for (std::size_t i = 0; i < children.size(); ++i) {
            CAPTURE(i);
            const auto* child = curve.entityAt(static_cast<int>(i));
            REQUIRE(child); CHECK(child == children[i].identity);
            children[i].attributes.check(*child); CHECK(child->getParent() == &curve);
            CHECK(samePoint(child->getStartpoint(), children[i].start));
            CHECK(samePoint(child->getEndpoint(), children[i].end));
            CHECK(samePoint(child->getMin(), children[i].minimum)); CHECK(samePoint(child->getMax(), children[i].maximum));
        }
    }
};

// Keep the actual C-adopted arithmetic from lc_offsetcurve_edit_tests.cpp.
std::unique_ptr<LC_OffsetCurve> adoptedArc() {
    requireSettingsIsolation();
    LC_EllipseOffsetDefinition definition;
    definition.basis = {{0, 0}, {1, 0}, 1, 0, 0.1, false};
    definition.distance = 0.25; definition.relTolerance = 0.01;
    const double radius = 1 + definition.distance;
    const RS_Line cache{nullptr, {radius, 0},
        {radius * std::cos(definition.basis.angle2), radius * std::sin(definition.basis.angle2)}};
    LC_OffsetCheckResult checked;
    auto curve = LC_OffsetCurve::adopt(cache, definition, 0.01, &checked);
    CAPTURE(checked.reason, checked.span); REQUIRE(checked); REQUIRE(curve);
    REQUIRE(curve->hasEditableDefinition()); REQUIRE_FALSE(curve->sourceIsWhole());
    decorate(*curve);
    return curve;
}

std::unique_ptr<LC_OffsetCurve> adoptedRing() {
    requireSettingsIsolation();
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
    LC_OffsetCheckResult checked;
    auto curve = LC_OffsetCurve::adopt(cache, definition, 0.02, &checked);
    CAPTURE(checked.reason, checked.span); REQUIRE(checked); REQUIRE(curve);
    REQUIRE(curve->hasEditableDefinition()); REQUIRE(curve->sourceIsWhole());
    REQUIRE(curve->cacheFormsClosedChain());
    decorate(*curve);
    return curve;
}

void noWork(const geo::CheckWork& work) {
    CHECK(work.cacheSpans == 0); CHECK(work.vertices == 0); CHECK(work.distancePairs == 0);
    CHECK(work.deepest == 0); CHECK(work.orderingComparisons == 0);
}

void noStages(const LC_OffsetCurveEditResult& result) {
    CHECK(result.engineRuns == 0); CHECK_FALSE(result.engineStatus); CHECK_FALSE(result.checkReason);
    noWork(result.checkWork); noWork(result.throughWork);
}

void checkLimits(const LC_OffsetRunLimits& actual, const LC_OffsetRunLimits& expected) {
    CHECK(actual.outputBudget.maxCubicPieces == expected.outputBudget.maxCubicPieces);
    CHECK(actual.outputBudget.maxOutputEntities == expected.outputBudget.maxOutputEntities);
    CHECK(actual.outputBudget.maxDeepEntities == expected.outputBudget.maxDeepEntities);
    CHECK(actual.maxSamples == expected.maxSamples); CHECK(actual.maxIntersectionPairs == expected.maxIntersectionPairs);
    CHECK(actual.checkLimits.maxCacheSpans == expected.checkLimits.maxCacheSpans);
    CHECK(actual.checkLimits.maxVertices == expected.checkLimits.maxVertices);
    CHECK(actual.checkLimits.maxDistancePairs == expected.checkLimits.maxDistancePairs);
    CHECK(actual.checkLimits.maxDepth == expected.checkLimits.maxDepth);
    CHECK(actual.checkLimits.maxOrderingComparisons == expected.checkLimits.maxOrderingComparisons);
}

void checkGenerate(const Preparation& plan, const LC_EllipseOffsetDefinition& definition,
                   const LC_OffsetRunLimits& limits) {
    REQUIRE(plan.generate());
    CHECK_FALSE(plan.result()); CHECK_FALSE(plan.result().changed); noStages(plan.result());
    checkDefinition(plan.generate()->definition, definition); checkLimits(plan.generate()->limits, limits);
    CHECK_FALSE(plan.generate()->throughPoint);
}

void checkRefused(const LC_OffsetCurveEditResult& result, const Refusal refusal) {
    CHECK_FALSE(result); CHECK_FALSE(result.changed); CHECK(result.refusal == refusal); noStages(result);
}
}

TEST_CASE("Worker edit generation owns the exact proposal and requested limits without running stages",
          "[lc-offsetcurve][worker-edit][generate]") {
    for (unsigned proposal = 0; proposal < 3; ++proposal) {
        CAPTURE(proposal);
        auto source = adoptedArc(); const Snapshot before{*source};
        auto next = source->definition();
        if (proposal == 0) next.distance = 0.5;
        if (proposal == 1) next.basis.center = {4, 8}; // Rounded arc controls cannot transport the proof.
        if (proposal == 2) next.cacheReversed = !next.cacheReversed;
        LC_OffsetRunLimits limits;
        limits.outputBudget = {7, 3, 257}; limits.maxSamples = 31; limits.maxIntersectionPairs = 113;
        limits.checkLimits = {5, 127, 251, 7, 509};
        if (proposal == 2) { limits.maxSamples = 0; limits.maxIntersectionPairs = 0; }
        const auto requested = next; const auto requestedLimits = limits;
        auto plan = Edit::prepare(*source, next, limits);
        checkGenerate(plan, requested, requestedLimits); before.check(*source);
        next.distance = 4; next.basis.center = {32, 64}; limits = {};
        auto owned = std::move(plan);
        checkGenerate(owned, requested, requestedLimits); before.check(*source);
        const auto result = Edit::applyLocal(*source, std::move(owned));
        CHECK_FALSE(result); CHECK_FALSE(result.changed); noStages(result); before.check(*source);
    }
}

TEST_CASE("Worker edit no-op application retains complete source state and owned extensions",
          "[lc-offsetcurve][worker-edit][noop]") {
    for (const bool ring : {false, true}) {
        CAPTURE(ring);
        RS_EntityContainer owner; RS_Layer layer{QStringLiteral("worker no-op")};
        auto source = ring ? adoptedRing() : adoptedArc();
        source->setParent(&owner); source->setLayer(&layer);
        const Snapshot before{*source};
        LC_OffsetRunLimits limits;
        limits.outputBudget = {1, 1, 1}; limits.maxSamples = 1; limits.maxIntersectionPairs = 1;
        limits.checkLimits = {0, 0, 0, 0, 0};
        auto plan = Edit::prepare(*source, source->definition(), limits);
        REQUIRE(plan.result()); CHECK_FALSE(plan.result().changed); CHECK_FALSE(plan.generate());
        noStages(plan.result()); before.check(*source);
        const auto result = Edit::applyLocal(*source, std::move(plan));
        REQUIRE(result); CHECK(result.refusal == Refusal::None); CHECK_FALSE(result.changed);
        noStages(result); before.check(*source);
    }
}

TEST_CASE("Worker Through preparation preserves exact endpoint no-ops without geometry work",
          "[lc-offsetcurve][worker-edit][through]") {
    auto source = adoptedRing(); const Snapshot before{*source};
    LC_OffsetRunLimits limits; limits.checkLimits = {0, 0, 0, 0, 0};
    auto plan = Edit::prepareThrough(*source, source->getData().controlPoints.front(), limits);
    REQUIRE(plan.result()); CHECK_FALSE(plan.result().changed);
    CHECK_FALSE(plan.generate()); CHECK_FALSE(plan.membership()); noStages(plan.result());
    REQUIRE(Edit::applyLocal(*source, std::move(plan))); before.check(*source);
}

TEST_CASE("Worker Through preparation describes unchanged-cache membership without running C",
          "[lc-offsetcurve][worker-edit][through]") {
    auto source = adoptedRing(); const Snapshot before{*source};
    LC_OffsetRunLimits limits; limits.checkLimits = {7, 129, 257, 9, 513};
    const RS_Vector point{1.2, 1.6};
    auto plan = Edit::prepareThrough(*source, point, limits);
    REQUIRE(plan.membership()); CHECK_FALSE(plan.generate()); CHECK_FALSE(plan.result()); noStages(plan.result());
    const auto owned = *plan.membership();
    checkSpline(owned.cache, source->getData()); checkDefinition(owned.definition, source->definition());
    CHECK(samePoint(owned.point, point)); CHECK(owned.worldTolerance == source->checkedWorldTolerance());
    CHECK(owned.limits.maxDistancePairs == 257); CHECK(owned.limits.maxCacheSpans == 7);
    CHECK_FALSE(Edit::applyLocal(*source, std::move(plan))); before.check(*source);
}

TEST_CASE("Worker Through preparation owns the raw target of a real generation proposal",
          "[lc-offsetcurve][worker-edit][through]") {
    auto source = adoptedRing(); const Snapshot before{*source};
    LC_OffsetRunLimits limits; limits.maxSamples = 17;
    const RS_Vector point{0, 3};
    auto plan = Edit::prepareThrough(*source, point, limits);
    REQUIRE(plan.generate()); CHECK_FALSE(plan.membership()); CHECK_FALSE(plan.result()); noStages(plan.result());
    REQUIRE(plan.generate()->throughPoint); CHECK(samePoint(*plan.generate()->throughPoint, point));
    CHECK(plan.generate()->definition.distance == 2); CHECK_FALSE(plan.generate()->definition.towardCentre);
    checkLimits(plan.generate()->limits, limits);
    CHECK_FALSE(Edit::applyLocal(*source, std::move(plan))); before.check(*source);
}

TEST_CASE("Worker Through preparation refuses invalid points before submitting work",
          "[lc-offsetcurve][worker-edit][through]") {
    auto source = adoptedRing(); const Snapshot before{*source};
    for (const auto point : {RS_Vector{false}, RS_Vector{0, 2, 1},
                            RS_Vector{std::numeric_limits<double>::infinity(), 0}}) {
        auto plan = Edit::prepareThrough(*source, point, {});
        CHECK_FALSE(plan.generate()); CHECK_FALSE(plan.membership());
        checkRefused(plan.result(), Refusal::InvalidInput); before.check(*source);
    }
}

TEST_CASE("Worker Through centre proposal still delegates disappearance to the producer",
          "[lc-offsetcurve][worker-edit][through]") {
    auto source = adoptedRing(); const Snapshot before{*source};
    auto plan = Edit::prepareThrough(*source, {0, 0}, {});
    REQUIRE(plan.generate()); CHECK_FALSE(plan.membership()); CHECK_FALSE(plan.result()); noStages(plan.result());
    CHECK(plan.generate()->definition.distance == 1); CHECK(plan.generate()->definition.towardCentre);
    REQUIRE(plan.generate()->throughPoint); CHECK(samePoint(*plan.generate()->throughPoint, {0, 0}));
    CHECK_FALSE(Edit::applyLocal(*source, std::move(plan))); before.check(*source);
}

TEST_CASE("Worker edit exact translation stays private until one bounded local application",
          "[lc-offsetcurve][worker-edit][translation]") {
    RS_EntityContainer owner; RS_Layer layer{QStringLiteral("worker translation")};
    auto source = adoptedRing(); source->setParent(&owner); source->setLayer(&layer);
    const Snapshot before{*source};
    const auto layout = geo::cacheLayoutCountsFromControls(before.data.degree, before.data.controlPoints.size());
    REQUIRE(layout); REQUIRE(layout->spans == 4);
    const auto cost = measureOffsetOutput({source.get()}, makeDefaultOffsetSourceBudget().maxDeepEntities);
    REQUIRE(cost.status == LC_OffsetTreeStatus::Ok); REQUIRE(cost.deepEntities > 1);
    LC_OffsetRunLimits limits; limits.outputBudget = {layout->spans, 1, cost.deepEntities};
    limits.maxSamples = 1; limits.maxIntersectionPairs = 1; limits.checkLimits = {0, 0, 0, 0, 0};
    auto next = source->definition(); next.basis.center = {4, 8};
    {
        auto discarded = Edit::prepare(*source, next, limits);
        REQUIRE(discarded.result()); CHECK(discarded.result().changed); CHECK_FALSE(discarded.generate());
        noStages(discarded.result()); before.check(*source);
    }
    before.check(*source);
    auto plan = Edit::prepare(*source, next, limits);
    REQUIRE(plan.result()); CHECK(plan.result().changed); CHECK_FALSE(plan.generate());
    noStages(plan.result()); before.check(*source);
    auto owned = std::move(plan);
    before.check(*source);
    const auto result = Edit::applyLocal(*source, std::move(owned));
    REQUIRE(result); CHECK(result.changed); CHECK(result.refusal == Refusal::None); noStages(result);
    before.attributes.check(*source); checkDefinition(source->definition(), next);
    auto translated = before.data;
    for (auto& point : translated.controlPoints) point += RS_Vector{4, 8};
    for (auto& point : translated.fitPoints) point += RS_Vector{4, 8};
    checkSpline(source->getData(), translated);
    auto grips = before.grips;
    for (auto& point : grips) point += RS_Vector{4, 8};
    checkPoints(source->cachedDefinitionGrips(), grips);
    CHECK(bits(source->checkedWorldTolerance()) == bits(before.tolerance));
    REQUIRE(source->hasEditableDefinition()); CHECK(source->sourceIsWhole()); CHECK(source->cacheFormsClosedChain());
    CHECK(source->unknownOwnedFlags() == 0); CHECK(source->unknownOwnedTail().empty());
    CHECK(samePoint(source->getMin(), before.minimum + RS_Vector{4, 8}));
    CHECK(samePoint(source->getMax(), before.maximum + RS_Vector{4, 8}));
    const auto translatedCost = measureOffsetOutput({source.get()}, limits.outputBudget.maxDeepEntities);
    CHECK(translatedCost.status == LC_OffsetTreeStatus::Ok); CHECK(translatedCost.deepEntities == cost.deepEntities);
    for (const auto* child : *source) { REQUIRE(child); CHECK(child->getParent() == source.get()); }
    const Snapshot applied{*source};
    checkRefused(Edit::applyLocal(*source, std::move(owned)), Refusal::Inconsistent);
    applied.check(*source);
}

TEST_CASE("Worker edit local application preserves metadata changed after preparation",
          "[lc-offsetcurve][worker-edit][attributes]") {
    RS_EntityContainer owner; RS_Layer layer{QStringLiteral("late worker metadata")};
    auto source = adoptedRing(); auto next = source->definition(); next.basis.center = {4, 8};
    auto plan = Edit::prepare(*source, next, {});
    REQUIRE(plan.result()); REQUIRE(plan.result().changed); REQUIRE_FALSE(plan.generate());
    source->setParent(&owner); source->setLayer(&layer);
    source->setPen(RS_Pen{RS_Color{71, 53, 29}, RS2::Width07, RS2::DotLine});
    source->setUserDefVar("D-worker-edit", "late attribute value"); source->setSourceHandle(901);
    source->setReactorHandles({902, 903}); source->setOwnerLineage(904);
    const Attributes late{*source};
    const auto result = Edit::applyLocal(*source, std::move(plan));
    REQUIRE(result); CHECK(result.changed); noStages(result); late.check(*source);
    checkDefinition(source->definition(), next);
    CHECK(source->unknownOwnedFlags() == 0); CHECK(source->unknownOwnedTail().empty());
}

TEST_CASE("Worker edit moving a local preparation transfers its sole authority to report a change",
          "[lc-offsetcurve][worker-edit][translation]") {
    auto source = adoptedRing(); const Snapshot before{*source};
    auto next = source->definition(); next.basis.center = {4, 8};
    auto plan = Edit::prepare(*source, next, {});
    REQUIRE(plan.result()); REQUIRE(plan.result().changed); REQUIRE_FALSE(plan.generate());
    auto owned = std::move(plan);
    before.check(*source);
    const auto movedFrom = Edit::applyLocal(*source, std::move(plan));
    CHECK_FALSE(movedFrom); CHECK_FALSE(movedFrom.changed); noStages(movedFrom); before.check(*source);
    const auto applied = Edit::applyLocal(*source, std::move(owned));
    REQUIRE(applied); CHECK(applied.changed); noStages(applied);
    REQUIRE(source->hasEditableDefinition()); checkDefinition(source->definition(), next);
    before.attributes.check(*source);
    CHECK(source->unknownOwnedFlags() == 0); CHECK(source->unknownOwnedTail().empty());
}

TEST_CASE("Worker edit no-op and translated preparations cannot apply to another source",
          "[lc-offsetcurve][worker-edit][stale]") {
    for (const bool translate : {false, true}) {
        CAPTURE(translate);
        auto source = adoptedRing(); auto other = adoptedRing();
        REQUIRE(source->getId() != other->getId());
        const Snapshot original{*source}, unrelated{*other};
        auto next = source->definition(); if (translate) next.basis.center = {4, 8};
        auto plan = Edit::prepare(*source, next, {});
        REQUIRE(plan.result()); REQUIRE_FALSE(plan.generate());
        checkRefused(Edit::applyLocal(*other, std::move(plan)), Refusal::Inconsistent);
        original.check(*source); unrelated.check(*other);
    }
}

TEST_CASE("Worker edit source mutation revokes preparations even when the entity ID is unchanged",
          "[lc-offsetcurve][worker-edit][stale]") {
    for (const bool translate : {false, true}) for (unsigned mutation = 0; mutation < 3; ++mutation) {
        CAPTURE(translate, mutation);
        auto source = adoptedRing(); const auto id = source->getId();
        auto next = source->definition(); if (translate) next.basis.center = {4, 8};
        auto plan = Edit::prepare(*source, next, {});
        REQUIRE(plan.result()); REQUIRE_FALSE(plan.generate());
        if (mutation == 0) {
            source->move({8, 16}); REQUIRE(source->hasEditableDefinition());
            REQUIRE(samePoint(source->definition().basis.center, RS_Vector{8, 16}));
        }
        if (mutation == 1) { source->getData().controlPoints.front().x += 0.25; REQUIRE_FALSE(source->isConsistent()); }
        if (mutation == 2) { source->scale({0, 0}, {2, 3}); REQUIRE_FALSE(source->isDefined()); }
        REQUIRE(source->getId() == id);
        const Snapshot mutated{*source};
        checkRefused(Edit::applyLocal(*source, std::move(plan)), Refusal::Inconsistent);
        mutated.check(*source);
    }
}

TEST_CASE("Worker edit malformed and lowered output budgets cannot bypass generation with a cheap translation",
          "[lc-offsetcurve][worker-edit][limits]") {
    auto source = adoptedRing(); const Snapshot before{*source};
    const auto layout = geo::cacheLayoutCountsFromControls(before.data.degree, before.data.controlPoints.size());
    REQUIRE(layout); REQUIRE(layout->spans == 4);
    const auto cost = measureOffsetOutput({source.get()}, makeDefaultOffsetSourceBudget().maxDeepEntities);
    REQUIRE(cost.status == LC_OffsetTreeStatus::Ok); REQUIRE(cost.deepEntities > 1);
    for (unsigned budget = 0; budget < 6; ++budget) {
        CAPTURE(budget);
        LC_OffsetRunLimits limits;
        limits.outputBudget = {layout->spans, 1, cost.deepEntities};
        if (budget == 0) limits.outputBudget.maxCubicPieces = 0;
        if (budget == 1) limits.outputBudget.maxOutputEntities = 0;
        if (budget == 2) limits.outputBudget.maxDeepEntities = 0;
        if (budget == 3) limits.outputBudget.maxCubicPieces = layout->spans - 1;
        if (budget == 4) limits.outputBudget.maxDeepEntities = cost.deepEntities - 1;
        if (budget == 5) limits.outputBudget.maxDeepEntities = 1;
        limits.maxSamples = 1; limits.maxIntersectionPairs = 1; limits.checkLimits = {0, 0, 0, 0, 0};
        auto next = source->definition(); next.basis.center = {4, 8};
        auto plan = Edit::prepare(*source, next, limits);
        REQUIRE_FALSE(plan.result()); CHECK_FALSE(plan.result().changed); noStages(plan.result());
        if (plan.generate()) checkGenerate(plan, next, limits);
        before.check(*source);
        const auto result = Edit::applyLocal(*source, std::move(plan));
        CHECK_FALSE(result); CHECK_FALSE(result.changed); noStages(result); before.check(*source);
    }
}

TEST_CASE("Worker edit invalid definitions refuse without preparing a local edit or generation",
          "[lc-offsetcurve][worker-edit][refusal]") {
    auto source = adoptedArc(); const Snapshot before{*source};
    for (unsigned invalid = 0; invalid < 7; ++invalid) {
        CAPTURE(invalid);
        auto next = source->definition();
        if (invalid == 0) next.distance = 0;
        if (invalid == 1) next.distance = -1;
        if (invalid == 2) next.distance = std::numeric_limits<double>::quiet_NaN();
        if (invalid == 3) next.relTolerance = 0;
        if (invalid == 4) next.basis.center.z = 1;
        if (invalid == 5) next.basis.majorP = {0, 0};
        if (invalid == 6) next.basis.center.x = std::numeric_limits<double>::infinity();
        auto plan = Edit::prepare(*source, next, {});
        checkRefused(plan.result(), Refusal::InvalidInput); CHECK_FALSE(plan.generate()); before.check(*source);
        checkRefused(Edit::applyLocal(*source, std::move(plan)), Refusal::InvalidInput); before.check(*source);
    }
}

TEST_CASE("Worker edit undefined inconsistent and unsupported source caches refuse before any stages",
          "[lc-offsetcurve][worker-edit][refusal]") {
    for (unsigned state = 0; state < 3; ++state) {
        CAPTURE(state);
        auto source = adoptedRing();
        if (state == 0) { source->scale({0, 0}, {2, 3}); REQUIRE_FALSE(source->isDefined()); }
        if (state == 1) source->getData().controlPoints.front().x += 0.25;
        if (state == 2) source->getData().degree = 4;
        REQUIRE_FALSE(source->hasEditableDefinition());
        const Snapshot before{*source};
        for (const bool translate : {false, true}) {
            CAPTURE(translate);
            auto next = source->definition(); if (translate) next.basis.center = {4, 8};
            auto plan = Edit::prepare(*source, next, {});
            const auto refusal = state == 0 ? Refusal::Undefined : Refusal::Inconsistent;
            checkRefused(plan.result(), refusal); CHECK_FALSE(plan.generate()); before.check(*source);
            checkRefused(Edit::applyLocal(*source, std::move(plan)), refusal); before.check(*source);
        }
    }
}
