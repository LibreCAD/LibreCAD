/*
 * This file is part of LibreCAD, a 2D CAD program.
 * Copyright (C) 2026 LibreCAD.org
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <initializer_list>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "drw_base.h"
#include "lc_actiontestsupport.h"
#include "lc_offsetcurve.h"
#include "rs_layer.h"
#include "rs_line.h"
#include "rs_modification.h"

namespace geo = LC_EllipseOffsetGeometry;
namespace again = LC_OffsetCurveAgain;
namespace {
struct Pending {
    LC_DocumentModificationBatch ctx;
    ~Pending() { qDeleteAll(ctx.entitiesToAdd); }
};

struct Drawing {
    const bool qtReady{lc::test::application() != nullptr};
    RS_Graphic graphic;
    lc::test::TestGraphicView view;
    Drawing() {
        graphic.initForNewDocument(); graphic.onLoadingCompleted(); view.setDocument(&graphic);
    }
    bool modify(const RS_Document::FunUndoable &operation) {
        return graphic.undoableModify(view.getViewPort(), operation);
    }
    QList<RS_Entity *> live() const {
        QList<RS_Entity *> result;
        for (auto *entity : graphic) if (entity && !entity->isDeleted()) result.append(entity);
        return result;
    }
};

std::uint64_t bits(double value) {
    std::uint64_t result;
    static_assert(sizeof(result) == sizeof(value));
    std::memcpy(&result, &value, sizeof(result)); return result;
}
void samePoint(const RS_Vector &actual, const RS_Vector &expected) {
    CHECK(actual.valid == expected.valid);
    CHECK(bits(actual.x) == bits(expected.x)); CHECK(bits(actual.y) == bits(expected.y));
    CHECK(bits(actual.z) == bits(expected.z));
}
void samePoints(const std::vector<RS_Vector> &actual, const std::vector<RS_Vector> &expected) {
    REQUIRE(actual.size() == expected.size());
    for (std::size_t i = 0; i < actual.size(); ++i) { CAPTURE(i); samePoint(actual[i], expected[i]); }
}
void sameNumbers(const std::vector<double> &actual, const std::vector<double> &expected) {
    REQUIRE(actual.size() == expected.size());
    for (std::size_t i = 0; i < actual.size(); ++i) { CAPTURE(i); CHECK(bits(actual[i]) == bits(expected[i])); }
}
void sameSpline(const RS_SplineData &actual, const RS_SplineData &expected) {
    CHECK(actual.degree == expected.degree); CHECK(actual.type == expected.type);
    CHECK(actual.savedOpenType == expected.savedOpenType);
    CHECK(actual.m_closedFlag == expected.m_closedFlag); CHECK(actual.m_periodicFlag == expected.m_periodicFlag);
    samePoints(actual.controlPoints, expected.controlPoints); samePoints(actual.fitPoints, expected.fitPoints);
    sameNumbers(actual.knotslist, expected.knotslist); sameNumbers(actual.weights, expected.weights);
    sameNumbers(actual.savedOpenKnots, expected.savedOpenKnots);
}
void sameBasis(const RS_EllipseData &actual, const RS_EllipseData &expected) {
    samePoint(actual.center, expected.center); samePoint(actual.majorP, expected.majorP);
    CHECK(bits(actual.ratio) == bits(expected.ratio)); CHECK(bits(actual.angle1) == bits(expected.angle1));
    CHECK(bits(actual.angle2) == bits(expected.angle2)); CHECK(actual.reversed == expected.reversed);
}
void sameDefinition(const LC_EllipseOffsetDefinition &actual, const LC_EllipseOffsetDefinition &expected) {
    sameBasis(actual.basis, expected.basis);
    CHECK(bits(actual.distance) == bits(expected.distance)); CHECK(actual.towardCentre == expected.towardCentre);
    CHECK(actual.cacheReversed == expected.cacheReversed); CHECK(bits(actual.relTolerance) == bits(expected.relTolerance));
}

using Foreign = std::vector<std::pair<int, std::string>>;
Foreign foreign(const RS_Entity &entity) {
    Foreign result;
    for (const auto &item : entity.getDrwExtData()) {
        REQUIRE(item); REQUIRE(item->type() == DRW_Variant::STRING);
        result.emplace_back(item->code(), *item->content.s);
    }
    return result;
}
void decorate(RS_Entity &entity, RS_Layer *layer) {
    entity.setLayer(layer); entity.setPen(RS_Pen{RS_Color{29, 53, 97}, RS2::Width11, RS2::DashLine});
    entity.setSourceHandle(901); entity.setXDictHandle(902); entity.setReactorHandles({903, 904});
    entity.setOwnerLineage(905); entity.setUserDefVar("E-again", "foreign value");
    entity.setDrwExtData({std::make_shared<DRW_Variant>(1001, std::string("E_AGAIN_FOREIGN")),
        std::make_shared<DRW_Variant>(1000, std::string("preserve without nesting"))});
}
void valueAttributes(const RS_Entity &actual, const RS_Entity &source) {
    CHECK(actual.getParent() == source.getParent()); CHECK(actual.getLayer() == source.getLayer());
    CHECK(actual.getPen(false) == source.getPen(false)); CHECK(foreign(actual) == foreign(source));
    CHECK(actual.getUserDefVar("E-again") == source.getUserDefVar("E-again"));
    CHECK(actual.getId() != source.getId()); CHECK(actual.lineage() != source.lineage());
    CHECK(actual.sourceHandle() == 0); CHECK(actual.xDictHandle() == 0);
    CHECK(actual.reactorHandles().empty()); CHECK(actual.ownerLineage() == 0);
}

// Refusal/staging checks require stable storage; document regeneration compares current values.
struct SourceState {
    const LC_OffsetCurve &source;
    RS_SplineData data;
    LC_EllipseOffsetDefinition definition;
    RS_EntityContainer *parent;
    RS_Layer *layer;
    RS_Pen pen;
    unsigned flags, ownedFlags;
    unsigned long long id, lineage, owner;
    quint32 handle, dictionary;
    std::vector<quint32> reactors;
    Foreign xdata;
    QString userValue;
    LC_OffsetCurve::OwnedTail tail;
    std::vector<RS_Vector> grips;
    const RS_Vector *controlStorage, *gripStorage;
    std::vector<const RS_Entity *> children;
    std::vector<RS_Vector> childEnds;
    bool defined, editable;
    double tolerance;

    explicit SourceState(const LC_OffsetCurve &entity) : source(entity), data(entity.getData()),
        definition(entity.definition()), parent(entity.getParent()), layer(entity.getLayer()), pen(entity.getPen(false)),
        flags(entity.getFlags()), ownedFlags(entity.unknownOwnedFlags()), id(entity.getId()), lineage(entity.lineage()),
        owner(entity.ownerLineage()), handle(entity.sourceHandle()), dictionary(entity.xDictHandle()),
        reactors(entity.reactorHandles()), xdata(foreign(entity)), userValue(entity.getUserDefVar("E-again")),
        tail(entity.unknownOwnedTail()), grips(entity.cachedDefinitionGrips()),
        controlStorage(entity.getData().controlPoints.data()), gripStorage(entity.cachedDefinitionGrips().data()),
        defined(entity.isDefined()), editable(entity.hasEditableDefinition()), tolerance(entity.checkedWorldTolerance()) {
        for (const auto *child : entity) {
            REQUIRE(child); children.push_back(child);
            childEnds.push_back(child->getStartpoint()); childEnds.push_back(child->getEndpoint());
        }
    }
    void check(bool exactFlags = true, bool exactStorage = true) const {
        if (exactFlags) CHECK(source.getFlags() == flags);
        CHECK(source.getParent() == parent); CHECK(source.getLayer() == layer); CHECK(source.getPen(false) == pen);
        CHECK(source.getId() == id); CHECK(source.lineage() == lineage); CHECK(source.ownerLineage() == owner);
        CHECK(source.sourceHandle() == handle); CHECK(source.xDictHandle() == dictionary);
        CHECK(source.reactorHandles() == reactors); CHECK(foreign(source) == xdata);
        CHECK(source.getUserDefVar("E-again") == userValue);
        CHECK(source.isDefined() == defined); CHECK(source.hasEditableDefinition() == editable);
        CHECK(bits(source.checkedWorldTolerance()) == bits(tolerance));
        sameSpline(source.getData(), data); sameDefinition(source.definition(), definition);
        const auto &basis = source.definition().basis;
        CHECK(basis.isArc == definition.basis.isArc);
        CHECK(bits(basis.angleDegrees) == bits(definition.basis.angleDegrees));
        CHECK(bits(basis.startAngleDegrees) == bits(definition.basis.startAngleDegrees));
        CHECK(bits(basis.otherAngleDegrees) == bits(definition.basis.otherAngleDegrees));
        CHECK(bits(basis.angularLength) == bits(definition.basis.angularLength));
        if (exactStorage) CHECK(source.getData().controlPoints.data() == controlStorage);
        samePoints(source.cachedDefinitionGrips(), grips);
        if (exactStorage) CHECK(source.cachedDefinitionGrips().data() == gripStorage);
        CHECK(source.unknownOwnedFlags() == ownedFlags); REQUIRE(source.unknownOwnedTail().size() == tail.size());
        for (std::size_t i = 0; i < tail.size(); ++i) {
            CHECK(source.unknownOwnedTail()[i].code == tail[i].code);
            REQUIRE(source.unknownOwnedTail()[i].value.index() == tail[i].value.index());
            REQUIRE(tail[i].value.index() == 0);
            CHECK(std::get<QString>(source.unknownOwnedTail()[i].value) == std::get<QString>(tail[i].value));
        }
        REQUIRE(source.count() == children.size());
        for (std::size_t i = 0; i < children.size(); ++i) {
            const auto* actual = source.entityAt(static_cast<int>(i)); REQUIRE(actual);
            if (exactStorage) CHECK(actual == children[i]);
            CHECK(actual->getParent() == &source);
            samePoint(actual->getStartpoint(), childEnds[2*i]); samePoint(actual->getEndpoint(), childEnds[2*i+1]);
        }
    }
};

template<class T> int code(const std::optional<T> &value) { return value ? static_cast<int>(*value) : -1; }
std::string traces(const LC_OffsetSourceOutcome &result) {
    std::ostringstream out;
    out << "status=" << static_cast<int>(result.status) << " rebaseEngineRuns=" << result.rebaseEngineRuns
        << " cacheEngineRuns=" << result.cacheEngineRuns << " reusedCacheCopies=" << result.reusedCacheCopies
        << " sampleUnits=" << result.sampleUnits << " pairAllowance=" << result.intersectionPairAllowance
        << " attemptedOutputUsage=" << result.attemptedOutputUsage.cubicPieces << ','
        << result.attemptedOutputUsage.outputEntities << ',' << result.attemptedOutputUsage.deepEntities
        << " attemptedDeepCountComplete=" << result.attemptedDeepCountComplete;
    for (const auto &trace : result.rebases) {
        const auto &work = trace.checkWork;
        out << "\ncopy=" << trace.copy << " proposalRefusal=" << code(trace.proposalRefusal)
            << " nativeRefusal=" << code(trace.nativeRefusal) << " checkReason=" << code(trace.checkReason)
            << " engineStatus=" << code(trace.engineStatus) << " engineRuns=" << trace.engineRuns
            << " sampleUnits=" << trace.sampleUnits << " pairAllowance=" << trace.intersectionPairAllowance
            << " checkWork=" << work.cacheSpans << ',' << work.vertices << ',' << work.distancePairs << ','
            << work.orderingComparisons << ',' << work.deepest << " adopted=" << trace.adopted;
    }
    return out.str();
}
void noCheckWork(const geo::CheckWork &work) {
    CHECK(work.cacheSpans == 0); CHECK(work.vertices == 0); CHECK(work.distancePairs == 0);
    CHECK(work.orderingComparisons == 0); CHECK(work.deepest == 0);
}
void unattempted(const LC_OffsetRebaseOutcome &trace) {
    CHECK(trace.engineRuns == 0); CHECK(trace.sampleUnits == 0); CHECK(trace.intersectionPairAllowance == 0);
    CHECK_FALSE(trace.nativeRefusal); CHECK_FALSE(trace.engineStatus); CHECK_FALSE(trace.checkReason);
    CHECK_FALSE(trace.adopted); noCheckWork(trace.checkWork);
}
void adopted(const LC_OffsetRebaseOutcome &trace) {
    REQUIRE(trace.proposalRefusal); CHECK(*trace.proposalRefusal == again::Refusal::None);
    REQUIRE(trace.nativeRefusal); CHECK(*trace.nativeRefusal == LC_OffsetCurveRefusal::None);
    REQUIRE(trace.engineStatus); CHECK(*trace.engineStatus == LC_CurveOffsetStatus::Ok);
    REQUIRE(trace.checkReason); CHECK(*trace.checkReason == LC_OffsetCheckResult::Reason::None);
    CHECK(trace.engineRuns == 1); CHECK(trace.sampleUnits > 0); CHECK(trace.intersectionPairAllowance > 0);
    CHECK(trace.checkWork.cacheSpans > 0); CHECK(trace.checkWork.vertices > 0); CHECK(trace.adopted);
}
const LC_OffsetSourceOutcome &single(const LC_OffsetBatchOutcome &batch) {
    REQUIRE(batch.sources.size() == 1); return batch.sources.front();
}
LC_OffsetBatchOutcome commit(Drawing &drawing, LC_OffsetCurve *source, RS_OffsetData data) {
    data.keepOriginals = true;
    LC_OffsetBatchOutcome result;
    const bool changed = drawing.modify([&](LC_DocumentModificationBatch &ctx) {
        result = RS_Modification::offsetWithOutcome(data, {source}, false, {}, ctx);
        return result.anySourceSucceeded();
    });
    INFO(traces(single(result))); REQUIRE(changed);
    for (const auto *entity : single(result).createdEntities) {
        REQUIRE(entity); CHECK(drawing.graphic.findEntityIndex(entity) >= 0); CHECK_FALSE(entity->isDeleted());
    }
    return result;
}
void accounting(const LC_OffsetSourceOutcome &result) {
    std::size_t runs = 0, units = 0, pairs = 0;
    int previous = 0;
    for (const auto &trace : result.rebases) {
        CHECK(trace.copy > previous); CHECK(trace.copy <= result.copiesRequested); previous = trace.copy;
        CHECK(trace.engineRuns <= 1); runs += trace.engineRuns; units += trace.sampleUnits;
        pairs += trace.intersectionPairAllowance;
        if (trace.adopted) adopted(trace);
    }
    CHECK(result.rebases.size() <= static_cast<std::size_t>(result.copiesRequested));
    CHECK(result.rebaseEngineRuns == runs); CHECK(result.sampleUnits >= units);
    CHECK(result.intersectionPairAllowance >= pairs);
    CHECK(result.cacheEngineRuns + result.reusedCacheCopies <= static_cast<std::size_t>(result.copiesRequested));
    if (result.cacheEngineRuns == 0) { CHECK(result.sampleUnits == units); CHECK(result.intersectionPairAllowance == pairs); }
}

RS_EllipseData basis(double major, double ratio, bool reversed, bool placed, bool arc = false) {
    RS_EllipseData result;
    result.center = placed ? RS_Vector{1024, -512} : RS_Vector{0, 0};
    result.majorP = placed ? RS_Vector{.8 * major, .6 * major} : RS_Vector{major, 0};
    result.ratio = ratio; result.reversed = reversed;
    if (arc) { result.angle1 = reversed ? 2.4 : .7; result.angle2 = reversed ? .7 : 2.4; result.isArc = true; }
    return result;
}
LC_OffsetCurve *materialized(Drawing &drawing, const RS_EllipseData &data, double distance,
    bool inward, bool cacheReversed) {
    RS_Ellipse original{&drawing.graphic, data}; decorate(original, drawing.graphic.getActiveLayer());
    auto made = LC_OffsetCurve::create(original, distance, inward, {}, 1e-4, cacheReversed);
    INFO("fixture nativeRefusal=" << static_cast<int>(made.refusal) << " engineStatus=" << code(made.engineStatus)
        << " checkReason=" << code(made.checkReason) << " engineRuns=" << made.engineRuns
        << " sampleUnits=" << made.sampleUnits << " checkWork=" << made.checkWork.cacheSpans << ','
        << made.checkWork.vertices << ',' << made.checkWork.distancePairs << ',' << made.checkWork.orderingComparisons);
    REQUIRE(made); REQUIRE(made.entity); CHECK(made.engineRuns == 1);
    REQUIRE(made.engineStatus); CHECK(*made.engineStatus == LC_CurveOffsetStatus::Ok);
    REQUIRE(made.checkReason); CHECK(*made.checkReason == LC_OffsetCheckResult::Reason::None);
    CHECK(made.sampleUnits > 0); CHECK(made.checkWork.cacheSpans > 0); CHECK(made.checkWork.vertices > 0);
    auto *result = made.entity.release(); drawing.graphic.addEntity(result);
    REQUIRE(result->hasEditableDefinition()); REQUIRE(drawing.graphic.findEntityIndex(result) >= 0);
    decorate(*result, drawing.graphic.getActiveLayer());
    REQUIRE(result->setOwnedExtensions(0x30, {{1000, QStringLiteral("source owned tail")}}));
    return result;
}

// Independent analytic normal coordinates, used only as endpoint/side witnesses.
RS_Vector normalPoint(const RS_EllipseData &data, double angle, double signedOutward) {
    const double a = std::hypot(data.majorP.x, data.majorP.y), c = std::cos(angle), s = std::sin(angle);
    const RS_Vector u{data.majorP.x / a, data.majorP.y / a}, v{-u.y, u.x};
    const double length = std::hypot(c, s / data.ratio);
    const RS_Vector normal = u * (c / length) + v * (s / data.ratio / length);
    return data.center + data.majorP * c + v * (a * data.ratio * s) + normal * signedOutward;
}
RS_OffsetData request(const LC_OffsetCurve &source, double magnitude, bool inward, int copies = 1) {
    const auto &definition = source.definition();
    const double middle = source.sourceIsWhole() ? M_PI_2 : .5 * (definition.basis.angle1 + definition.basis.angle2);
    const double original = definition.towardCentre ? -definition.distance : definition.distance;
    RS_OffsetData data; data.coord = normalPoint(definition.basis, middle, original + (inward ? -1 : 1));
    data.distance = magnitude; data.asOffsetCurve = true; data.keepOriginals = false;
    data.useCurrentAttributes = false; data.useCurrentLayer = false;
    data.multipleCopies = copies > 1; data.number = copies;
    const auto resolved = LC_CurveOffset::resolveSide(source, data.coord, LC_CurveOffset::makeOffsetOptions(source, magnitude));
    REQUIRE(resolved.status == LC_CurveOffsetStatus::Ok);
    const bool clockwise = definition.basis.reversed != definition.cacheReversed;
    const auto inwardSide = clockwise ? LC_CurveOffsetSide::Right : LC_CurveOffsetSide::Left;
    CHECK(resolved.side == (inward ? inwardSide :
        inwardSide == LC_CurveOffsetSide::Left ? LC_CurveOffsetSide::Right : LC_CurveOffsetSide::Left));
    return data;
}
void nativeOutput(const RS_Entity &entity, const LC_OffsetCurve &source, double total, bool checkAgain = false) {
    const auto *native = dynamic_cast<const LC_OffsetCurve *>(&entity); REQUIRE(native);
    REQUIRE(native->hasEditableDefinition());
    auto expected = source.definition(); expected.distance = std::abs(total); expected.towardCentre = total < 0;
    sameDefinition(native->definition(), expected); valueAttributes(*native, source);
    // The original ELLIPSE, not the current spline or another definition, remains the basis.
    sameBasis(native->definition().basis, source.definition().basis);
    CHECK(native->unknownOwnedFlags() == 0); CHECK(native->unknownOwnedTail().empty());
    if (checkAgain) {
        const auto checked = geo::checkCache(native->getData(), native->definition(), native->checkedWorldTolerance());
        CAPTURE(checked.reason, checked.span); REQUIRE(checked);
        CHECK(checked.analysis->closed() == native->cacheFormsClosedChain());
    }
}
void plainOutput(const LC_OffsetSourceOutcome &result, int ordinal, LC_OffsetPlainReason reason) {
    const auto found = std::find_if(result.keptPlain.begin(), result.keptPlain.end(),
        [ordinal](const auto &entry) { return entry.copy == ordinal; });
    REQUIRE(found != result.keptPlain.end()); CHECK(found->reason == reason);
}
void noPublication(const LC_OffsetSourceOutcome &result, const Pending &pending) {
    CHECK(result.copiesMade == 0); CHECK(result.copiesKeptPlain == 0); CHECK(result.keptPlain.empty());
    CHECK(result.createdEntities.empty()); CHECK_FALSE(result.sourceRemoved);
    CHECK(pending.ctx.entitiesToAdd.empty()); CHECK(pending.ctx.entitiesToDelete.empty());
}
std::pair<std::size_t, std::size_t> effort(const LC_OffsetCurve &source, double magnitude,
    const LC_OffsetBatchLimits &limits = {}) {
    const auto options = LC_CurveOffset::makeOffsetOptions(source, magnitude);
    return {limits.maxSamples ? std::min(options.maxSamples, limits.maxSamples) : options.maxSamples,
        limits.maxIntersectionPairs ? std::min(options.maxIntersectionPairs, limits.maxIntersectionPairs) : options.maxIntersectionPairs};
}
}

TEST_CASE("E-again actual E10050 four-copy document profiles preserve original basis in every order",
          "[modification][offsetcurve][again][profiles]") {
    for (bool reversed : {false, true}) for (bool cacheReversed : {false, true}) for (bool placed : {false, true}) {
        CAPTURE(reversed, cacheReversed, placed);
        Drawing drawing; auto *source = materialized(drawing, basis(100, .5, reversed, placed), 10, false, cacheReversed);
        const SourceState before{*source};
        const auto batch = commit(drawing, source, request(*source, 5, true, 4));
        const auto &result = single(batch); INFO(traces(result)); accounting(result);
        REQUIRE(result.complete()); REQUIRE(result.createdEntities.size() == 4);
        CHECK_FALSE(result.sourceRemoved); CHECK(result.rebaseEngineRuns == 3); CHECK(result.cacheEngineRuns == 1);
        CHECK(result.reusedCacheCopies == 0); CHECK(result.copiesKeptPlain == 1); REQUIRE(result.rebases.size() == 4);
        nativeOutput(*result.createdEntities[0], *source, 5, !reversed && !cacheReversed && !placed);
        nativeOutput(*result.createdEntities[2], *source, -5, !reversed && !cacheReversed && !placed);
        nativeOutput(*result.createdEntities[3], *source, -10, !reversed && !cacheReversed && !placed);
        auto *zero = result.createdEntities[1]; REQUIRE(zero); CHECK(zero->rtti() != RS2::EntityOffsetCurve);
        valueAttributes(*zero, *source); plainOutput(result, 2, LC_OffsetPlainReason::ZeroSum);
        REQUIRE(result.rebases[1].proposalRefusal); CHECK(*result.rebases[1].proposalRefusal == again::Refusal::ZeroSum);
        unattempted(result.rebases[1]);
        const auto [samples, pairs] = effort(*source, 5);
        CHECK(result.sampleUnits <= 4 * (samples + 1));
        CHECK(result.intersectionPairAllowance == 3 * (pairs / 2) + pairs);
        CHECK(result.usage.outputEntities == 4); CHECK(result.attemptedOutputUsage.outputEntities == 4);
        CHECK(result.attemptedOutputUsage.cubicPieces == result.usage.cubicPieces);
        CHECK(result.attemptedDeepCountComplete);
        before.check(); CHECK(drawing.live().size() == 5); CHECK(drawing.live().contains(source));
    }
}

TEST_CASE("E-again actual placed E8x4 arcs keep natural endpoints for both basis and cache directions",
          "[modification][offsetcurve][again][arc]") {
    for (bool reversed : {false, true}) for (bool cacheReversed : {false, true}) for (bool placed : {false, true}) {
        CAPTURE(reversed, cacheReversed, placed);
        Drawing drawing; auto *source = materialized(drawing, basis(8, .5, reversed, placed, true), .5, false, cacheReversed);
        const SourceState before{*source};
        for (bool inward : {false, true}) {
            CAPTURE(inward);
            const double increment = inward ? .75 : .25, total = inward ? -.25 : .75;
            const auto batch = commit(drawing, source, request(*source, increment, inward));
            const auto &result = single(batch); INFO(traces(result)); accounting(result);
            REQUIRE(result.complete()); REQUIRE(result.createdEntities.size() == 1); REQUIRE(result.rebases.size() == 1);
            adopted(result.rebases.front()); CHECK(result.cacheEngineRuns == 0); CHECK(result.reusedCacheCopies == 0);
            CHECK(result.copiesKeptPlain == 0); nativeOutput(*result.createdEntities.front(), *source, total, true);
            const auto *native = dynamic_cast<const LC_OffsetCurve *>(result.createdEntities.front()); REQUIRE(native);
            const auto &original = source->definition().basis;
            const auto start = normalPoint(original, cacheReversed ? original.angle2 : original.angle1, total);
            const auto end = normalPoint(original, cacheReversed ? original.angle1 : original.angle2, total);
            CHECK(native->getStartpoint().distanceTo(start) <= 2 * native->checkedWorldTolerance());
            CHECK(native->getEndpoint().distanceTo(end) <= 2 * native->checkedWorldTolerance());
            CHECK_FALSE(native->sourceIsWhole()); CHECK_FALSE(native->cacheFormsClosedChain()); before.check();
        }
    }
}

TEST_CASE("E-again near-critical E10060 total inward 36.036 is an actual C-certified rebase",
          "[modification][offsetcurve][again][near-critical]") {
    for (bool cacheReversed : {false, true}) {
        CAPTURE(cacheReversed);
        Drawing drawing; auto *source = materialized(drawing, basis(100, .6, false, false), 10, false, cacheReversed);
        const SourceState before{*source};
        const auto batch = commit(drawing, source, request(*source, 46.036, true));
        const auto &result = single(batch); INFO(traces(result)); accounting(result);
        REQUIRE(result.complete()); REQUIRE(result.rebases.size() == 1); REQUIRE(result.createdEntities.size() == 1);
        adopted(result.rebases.front()); CHECK(result.rebaseEngineRuns == 1); CHECK(result.cacheEngineRuns == 0);
        CHECK(result.copiesKeptPlain == 0); CHECK(result.reusedCacheCopies == 0);
        nativeOutput(*result.createdEntities.front(), *source, -36.036, true);
        const auto *native = dynamic_cast<const LC_OffsetCurve *>(result.createdEntities.front()); REQUIRE(native);
        CHECK(native->cacheFormsClosedChain()); CHECK(native->definition().towardCentre);
        CHECK(result.attemptedOutputUsage.outputEntities == 1); CHECK(result.attemptedDeepCountComplete); before.check();
    }
}

TEST_CASE("E-again undefined actual caches fall back without a structural proposal or native engine run",
          "[modification][offsetcurve][again][undefined]") {
    Drawing drawing; auto *source = materialized(drawing, basis(8, .5, false, false), .5, false, true);
    auto data = request(*source, .25, false);
    source->scale({0, 0}, {2, 3}); REQUIRE_FALSE(source->isDefined()); REQUIRE_FALSE(source->hasEditableDefinition());
    const SourceState before{*source}; Pending pending;
    data.coord = source->getMax() + RS_Vector{100, 100};
    const auto batch = RS_Modification::offsetWithOutcome(data, {source}, false, {}, pending.ctx);
    const auto &result = single(batch); INFO(traces(result)); accounting(result);
    REQUIRE(result.complete()); REQUIRE(result.rebases.size() == 1); unattempted(result.rebases.front());
    CHECK_FALSE(result.rebases.front().proposalRefusal); CHECK(result.rebaseEngineRuns == 0);
    CHECK(result.cacheEngineRuns == 1); CHECK(result.reusedCacheCopies == 0);
    plainOutput(result, 1, LC_OffsetPlainReason::UndefinedDefinition);
    for (const auto *entity : result.createdEntities) {
        REQUIRE(entity); CHECK(entity->rtti() != RS2::EntityOffsetCurve); valueAttributes(*entity, *source);
    }
    before.check();
}

TEST_CASE("E-again a real trimmed inward whole profile refuses outward structural recovery and stays plain",
          "[modification][offsetcurve][again][trimmed]") {
    Drawing drawing; auto *source = materialized(drawing, basis(100, .5, false, false), 49, true, true);
    REQUIRE(source->cacheFormsClosedChain()); const SourceState before{*source}; Pending pending;
    const auto batch = RS_Modification::offsetWithOutcome(request(*source, 1, false), {source}, false, {}, pending.ctx);
    const auto &result = single(batch); INFO(traces(result)); accounting(result);
    REQUIRE(result.complete()); REQUIRE(result.rebases.size() == 1); const auto &trace = result.rebases.front();
    REQUIRE(trace.proposalRefusal); CHECK(*trace.proposalRefusal == again::Refusal::FirstNotProvedUntrimmed);
    unattempted(trace); CHECK(result.rebaseEngineRuns == 0); CHECK(result.cacheEngineRuns == 1);
    CHECK(result.reusedCacheCopies == 0); plainOutput(result, 1, LC_OffsetPlainReason::RebaseRefused);
    for (const auto *entity : result.createdEntities) {
        REQUIRE(entity); CHECK(entity->rtti() != RS2::EntityOffsetCurve); valueAttributes(*entity, *source);
    }
    before.check();
}

TEST_CASE("E-again four-copy replacement is one actual undo batch with either Keep Originals setting",
          "[modification][offsetcurve][again][undo]") {
    for (bool keep : {false, true}) {
        CAPTURE(keep);
        Drawing drawing; auto *source = materialized(drawing, basis(100, .5, true, true), 10, false, true);
        const SourceState before{*source};
        auto *sibling = new RS_Line(&drawing.graphic, RS_Vector{-200, -300}, RS_Vector{-190, -300});
        drawing.graphic.addEntity(sibling); decorate(*sibling, drawing.graphic.getActiveLayer());
        const auto siblingStart = sibling->getStartpoint(), siblingEnd = sibling->getEndpoint();
        const auto siblingId = sibling->getId(); const auto siblingForeign = foreign(*sibling);
        auto data = request(*source, 5, true, 4); data.keepOriginals = keep;
        LC_OffsetBatchOutcome batch; QList<RS_Entity *> additions, deletions;
        REQUIRE(drawing.modify([&](LC_DocumentModificationBatch &ctx) {
            batch = RS_Modification::offsetWithOutcome(data, {source}, false, {}, ctx);
            additions = ctx.entitiesToAdd; deletions = ctx.entitiesToDelete; return batch.anySourceSucceeded();
        }));
        const auto &result = single(batch); INFO(traces(result)); accounting(result); REQUIRE(result.complete());
        REQUIRE(additions.size() == 4); CHECK(result.sourceRemoved == !keep);
        CHECK(deletions == (keep ? QList<RS_Entity *>{} : QList<RS_Entity *>{source}));
        CHECK(source->isDeleted() == !keep); before.check(keep, false);
        REQUIRE(drawing.live().size() == (keep ? 6 : 5));
        for (auto *entity : additions) { REQUIRE(entity); CHECK_FALSE(entity->isDeleted()); valueAttributes(*entity, *source); }
        REQUIRE(drawing.graphic.undo()); CHECK_FALSE(source->isDeleted()); before.check(true, false);
        CHECK(drawing.live().size() == 2); CHECK(drawing.live().contains(source)); CHECK(drawing.live().contains(sibling));
        for (const auto *entity : additions) CHECK(entity->isDeleted());
        CHECK_FALSE(drawing.graphic.undo()); // No fixture insertion created another undo cycle.
        REQUIRE(drawing.graphic.redo()); CHECK(source->isDeleted() == !keep); before.check(keep, false);
        CHECK(drawing.live().size() == (keep ? 6 : 5));
        for (const auto *entity : additions) CHECK_FALSE(entity->isDeleted());
        CHECK_FALSE(drawing.graphic.redo());
        CHECK(sibling->getId() == siblingId); CHECK_FALSE(sibling->isDeleted()); CHECK(foreign(*sibling) == siblingForeign);
        samePoint(sibling->getStartpoint(), siblingStart); samePoint(sibling->getEndpoint(), siblingEnd);
    }
}

TEST_CASE("E-again preview stays plain and a zero-sum commit reuses actual preview geometry without engine work",
          "[modification][offsetcurve][again][preview]") {
    Drawing drawing; auto *source = materialized(drawing, basis(8, .5, true, true, true), .5, false, true);
    const SourceState before{*source};
    for (double magnitude : {.25, .5}) {
        CAPTURE(magnitude); Pending off, on, committed; LC_OffsetPreviewCache cache;
        auto data = request(*source, magnitude, true); data.asOffsetCurve = false;
        const auto a = RS_Modification::offsetWithOutcome(data, {source}, true, {}, off.ctx);
        data.asOffsetCurve = true;
        const auto b = RS_Modification::offsetWithOutcome(data, {source}, true, {}, on.ctx, &cache);
        const auto &plain = single(a), &preview = single(b); INFO(traces(preview));
        REQUIRE(plain.complete()); REQUIRE(preview.complete()); REQUIRE(plain.createdEntities.size() == preview.createdEntities.size());
        CHECK(preview.rebases.empty()); CHECK(preview.rebaseEngineRuns == 0); CHECK(preview.cacheEngineRuns == 1);
        CHECK(preview.reusedCacheCopies == 0); CHECK(preview.sampleUnits == plain.sampleUnits);
        CHECK(preview.intersectionPairAllowance == plain.intersectionPairAllowance);
        CHECK(preview.usage.cubicPieces == plain.usage.cubicPieces); CHECK(preview.usage.deepEntities == plain.usage.deepEntities);
        CHECK(off.ctx.entitiesToDelete.empty()); CHECK(on.ctx.entitiesToDelete.empty());
        for (int i = 0; i < preview.createdEntities.size(); ++i) {
            const auto *x = dynamic_cast<const RS_Spline *>(plain.createdEntities[i]);
            const auto *y = dynamic_cast<const RS_Spline *>(preview.createdEntities[i]); REQUIRE(x); REQUIRE(y);
            CHECK(x->rtti() == RS2::EntitySpline); CHECK(y->rtti() == RS2::EntitySpline); CHECK(x != y);
            sameSpline(x->getData(), y->getData());
        }
        if (magnitude == .5) {
            const auto batch = RS_Modification::offsetWithOutcome(data, {source}, false, {}, committed.ctx, &cache);
            const auto &result = single(batch); INFO(traces(result)); accounting(result);
            REQUIRE(result.complete()); REQUIRE(result.rebases.size() == 1); unattempted(result.rebases.front());
            REQUIRE(result.rebases.front().proposalRefusal); CHECK(*result.rebases.front().proposalRefusal == again::Refusal::ZeroSum);
            CHECK(result.rebaseEngineRuns == 0); CHECK(result.cacheEngineRuns == 0); CHECK(result.reusedCacheCopies == 1);
            CHECK(result.sampleUnits == 0); CHECK(result.intersectionPairAllowance == 0);
            CHECK(result.attemptedOutputUsage.cubicPieces == 0); CHECK(result.attemptedOutputUsage.outputEntities == 0);
            CHECK(result.attemptedOutputUsage.deepEntities == 0); CHECK(result.attemptedDeepCountComplete);
            plainOutput(result, 1, LC_OffsetPlainReason::ZeroSum);
            REQUIRE(result.createdEntities.size() == preview.createdEntities.size());
            for (int i = 0; i < result.createdEntities.size(); ++i) {
                auto *spline = dynamic_cast<RS_Spline *>(result.createdEntities[i]); REQUIRE(spline);
                CHECK(spline->rtti() == RS2::EntitySpline); CHECK(spline != preview.createdEntities[i]);
                sameSpline(spline->getData(), static_cast<RS_Spline *>(preview.createdEntities[i])->getData());
                valueAttributes(*spline, *source);
            }
        }
        before.check();
    }
}

TEST_CASE("E-again native TooSmall admission refuses before the engine and leaves one full cache fallback",
          "[modification][offsetcurve][again][fallback]") {
    Drawing drawing; auto *source = materialized(drawing, basis(100, .5, false, false), 10, false, false);
    const SourceState before{*source}; Pending pending;
    const auto batch = RS_Modification::offsetWithOutcome(request(*source, 9.999, true), {source}, false, {}, pending.ctx);
    const auto &result = single(batch); INFO(traces(result)); accounting(result);
    REQUIRE(result.complete()); REQUIRE(result.rebases.size() == 1); const auto &trace = result.rebases.front();
    REQUIRE(trace.proposalRefusal); CHECK(*trace.proposalRefusal == again::Refusal::None);
    REQUIRE(trace.nativeRefusal); CHECK(*trace.nativeRefusal == LC_OffsetCurveRefusal::TooSmall);
    CHECK(trace.engineRuns == 0); CHECK(trace.sampleUnits == 0); CHECK(trace.intersectionPairAllowance == 0);
    CHECK_FALSE(trace.engineStatus); CHECK_FALSE(trace.checkReason); CHECK_FALSE(trace.adopted); noCheckWork(trace.checkWork);
    CHECK(result.rebaseEngineRuns == 0); CHECK(result.cacheEngineRuns == 1); CHECK(result.reusedCacheCopies == 0);
    const auto [samples, pairs] = effort(*source, 9.999);
    CHECK(result.sampleUnits > 0); CHECK(result.sampleUnits <= samples + 1); CHECK(result.intersectionPairAllowance == pairs);
    plainOutput(result, 1, LC_OffsetPlainReason::RebaseRefused);
    for (const auto *entity : result.createdEntities) { CHECK(entity->rtti() != RS2::EntityOffsetCurve); valueAttributes(*entity, *source); }
    before.check();
}

TEST_CASE("E-again actual vanished rebase runs its producer once and permits at most one bounded cache fallback",
          "[modification][offsetcurve][again][fallback][vanished]") {
    Drawing drawing; auto *source = materialized(drawing, basis(5, 1, false, false), 1, false, false);
    const SourceState before{*source}; Pending pending;
    const auto batch = RS_Modification::offsetWithOutcome(request(*source, 7, true), {source}, false, {}, pending.ctx);
    const auto &result = single(batch); INFO(traces(result)); accounting(result);
    REQUIRE(result.rebases.size() == 1); const auto &trace = result.rebases.front();
    REQUIRE(trace.proposalRefusal); CHECK(*trace.proposalRefusal == again::Refusal::None);
    REQUIRE(trace.nativeRefusal); CHECK(*trace.nativeRefusal == LC_OffsetCurveRefusal::NothingLeft);
    REQUIRE(trace.engineStatus); CHECK(*trace.engineStatus == LC_CurveOffsetStatus::Ok);
    CHECK(trace.engineRuns == 1); CHECK_FALSE(trace.checkReason); CHECK_FALSE(trace.adopted); noCheckWork(trace.checkWork);
    CHECK(result.rebaseEngineRuns == 1); CHECK(result.reusedCacheCopies == 0);
    // This producer can demand no sample units when it rejects an empty circle.
    // Exhaustion, including a refused unit, may instead forbid the fallback.
    const auto [samples, pairs] = effort(*source, 7);
    CHECK(result.cacheEngineRuns <= 1);
    CHECK(result.cacheEngineRuns == (trace.sampleUnits < samples ? 1 : 0));
    CHECK(trace.intersectionPairAllowance == pairs / 2);
    CHECK(result.intersectionPairAllowance == pairs / 2 + (result.cacheEngineRuns ? pairs - pairs / 2 : 0));
    CHECK(result.sampleUnits <= samples + 1); noPublication(result, pending); before.check();
}

TEST_CASE("E-again low sample demand is shared and exhaustion cannot buy a fresh fallback allowance",
          "[modification][offsetcurve][again][limits]") {
    Drawing drawing; auto *source = materialized(drawing, basis(8, .5, false, false, true), .5, false, false);
    const SourceState before{*source}; LC_OffsetBatchLimits limits; limits.maxSamples = 1; Pending pending;
    const auto batch = RS_Modification::offsetWithOutcome(request(*source, .25, false), {source}, false, limits, pending.ctx);
    const auto &result = single(batch); INFO(traces(result)); accounting(result);
    CHECK(result.status == LC_OffsetSourceStatus::LimitExceeded); REQUIRE(result.rebases.size() == 1);
    const auto &trace = result.rebases.front(); REQUIRE(trace.nativeRefusal);
    CHECK(*trace.nativeRefusal == LC_OffsetCurveRefusal::Engine); REQUIRE(trace.engineStatus);
    CHECK(*trace.engineStatus == LC_CurveOffsetStatus::LimitExceeded); CHECK_FALSE(trace.checkReason); noCheckWork(trace.checkWork);
    CHECK(trace.engineRuns == 1); CHECK(trace.sampleUnits > limits.maxSamples);
    CHECK(trace.sampleUnits <= limits.maxSamples + 1); CHECK_FALSE(trace.adopted);
    CHECK(result.rebaseEngineRuns == 1); CHECK(result.cacheEngineRuns == 0); CHECK(result.reusedCacheCopies == 0);
    CHECK(result.sampleUnits == trace.sampleUnits); noPublication(result, pending); before.check();
}

TEST_CASE("E-again one pair of allowance bypasses rebase without invented component results",
          "[modification][offsetcurve][again][limits]") {
    Drawing drawing; auto *source = materialized(drawing, basis(8, .5, false, false, true), .5, false, false);
    const SourceState before{*source}; LC_OffsetBatchLimits limits; limits.maxIntersectionPairs = 1; Pending pending;
    const auto batch = RS_Modification::offsetWithOutcome(request(*source, .25, false), {source}, false, limits, pending.ctx);
    const auto &result = single(batch); INFO(traces(result)); accounting(result);
    CHECK(result.status == LC_OffsetSourceStatus::LimitExceeded); REQUIRE(result.rebases.size() == 1);
    REQUIRE(result.rebases.front().proposalRefusal); CHECK(*result.rebases.front().proposalRefusal == again::Refusal::None);
    unattempted(result.rebases.front()); CHECK(result.rebaseEngineRuns == 0); CHECK(result.cacheEngineRuns == 1);
    CHECK(result.reusedCacheCopies == 0); CHECK(result.intersectionPairAllowance == 1);
    noPublication(result, pending); before.check();
}

TEST_CASE("E-again output rollback retains attempted work but publishes no partial series",
          "[modification][offsetcurve][again][limits][transaction]") {
    Drawing drawing; auto *source = materialized(drawing, basis(8, .5, false, false, true), .5, false, false);
    const RS_Ellipse original{nullptr, source->definition().basis};
    const auto expectedFirst = LC_OffsetCurve::create(original, .75, false, {}, source->definition().relTolerance);
    REQUIRE(expectedFirst);
    const auto cost = measureOffsetOutput({expectedFirst.entity.get()}, makeDefaultOffsetSourceBudget().maxDeepEntities);
    REQUIRE(cost.status == LC_OffsetTreeStatus::Ok); REQUIRE(cost.deepEntities > 0);
    const SourceState before{*source}; LC_OffsetBatchLimits limits; limits.perSource.maxDeepEntities = cost.deepEntities; Pending pending;
    const auto batch = RS_Modification::offsetWithOutcome(request(*source, .25, false, 2), {source}, false, limits, pending.ctx);
    const auto &result = single(batch); INFO(traces(result)); accounting(result);
    CHECK(result.status == LC_OffsetSourceStatus::LimitExceeded); REQUIRE(result.rebases.size() == 1);
    adopted(result.rebases.front()); CHECK(result.rebaseEngineRuns == 1); CHECK(result.cacheEngineRuns == 0);
    CHECK(result.attemptedOutputUsage.outputEntities == 1); CHECK(result.attemptedOutputUsage.cubicPieces > 0);
    CHECK(result.attemptedOutputUsage.deepEntities > 0); CHECK(result.sampleUnits > 0);
    CHECK(result.attemptedDeepCountComplete);
    noPublication(result, pending); before.check(); CHECK(drawing.live() == QList<RS_Entity *>{source});
}

TEST_CASE("E-again failed deep-cap attempts preserve bounded observed leaves instead of full tree counts",
          "[modification][offsetcurve][again][limits][attempted-output]") {
    Drawing drawing; auto *source = materialized(drawing, basis(8, .5, false, false, true), .5, false, false);
    const SourceState before{*source}; LC_OffsetBatchLimits limits; limits.perSource.maxDeepEntities = 1; Pending pending;
    const auto batch = RS_Modification::offsetWithOutcome(request(*source, .25, false), {source}, false, limits, pending.ctx);
    const auto &result = single(batch); INFO(traces(result)); accounting(result);
    CHECK(result.status == LC_OffsetSourceStatus::LimitExceeded); REQUIRE(result.rebases.size() == 1);
    const auto &trace = result.rebases.front(); REQUIRE(trace.nativeRefusal); REQUIRE(trace.engineStatus);
    CHECK(*trace.nativeRefusal == LC_OffsetCurveRefusal::Engine);
    CHECK(*trace.engineStatus == LC_CurveOffsetStatus::LimitExceeded); CHECK_FALSE(trace.checkReason);
    CHECK(trace.engineRuns == 1); CHECK_FALSE(trace.adopted); noCheckWork(trace.checkWork);
    CHECK(result.rebaseEngineRuns == 1); CHECK(result.cacheEngineRuns <= 1); CHECK(result.reusedCacheCopies == 0);
    CHECK(result.attemptedOutputUsage.cubicPieces > 0);
    CHECK(result.attemptedOutputUsage.outputEntities >= 1);
    CHECK(result.attemptedOutputUsage.outputEntities <= 1 + result.cacheEngineRuns);
    CHECK(result.attemptedOutputUsage.deepEntities >= 1);
    CHECK(result.attemptedOutputUsage.deepEntities <= 1 + result.cacheEngineRuns);
    CHECK_FALSE(result.attemptedDeepCountComplete);
    const auto [samples, pairs] = effort(*source, .25, limits);
    CHECK(result.sampleUnits <= samples + 1);
    CHECK(result.intersectionPairAllowance == pairs / 2 + (result.cacheEngineRuns ? pairs - pairs / 2 : 0));
    noPublication(result, pending); before.check();
}

TEST_CASE("E-again an exhausted request output cap makes no proposal and runs no producer",
          "[modification][offsetcurve][again][limits]") {
    Drawing drawing; auto *source = materialized(drawing, basis(8, .5, false, false, true), .5, false, false);
    const SourceState before{*source}; LC_OffsetBatchLimits limits; limits.maxDeepEntitiesPerRequest = 0; Pending pending;
    const auto batch = RS_Modification::offsetWithOutcome(request(*source, .25, false), {source}, false, limits, pending.ctx);
    const auto &result = single(batch); INFO(traces(result)); accounting(result);
    CHECK(result.status == LC_OffsetSourceStatus::LimitExceeded); CHECK(result.rebases.empty());
    CHECK(result.rebaseEngineRuns == 0); CHECK(result.cacheEngineRuns == 0); CHECK(result.reusedCacheCopies == 0);
    CHECK(result.sampleUnits == 0); CHECK(result.intersectionPairAllowance == 0);
    CHECK(result.attemptedOutputUsage.cubicPieces == 0); CHECK(result.attemptedOutputUsage.outputEntities == 0);
    CHECK(result.attemptedOutputUsage.deepEntities == 0); CHECK(result.attemptedDeepCountComplete);
    noPublication(result, pending); before.check();
}
