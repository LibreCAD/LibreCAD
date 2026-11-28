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
#include <functional>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "drw_base.h"
#include "lc_actiontestsupport.h"
#include "lc_action_edit_copy_cut.h"
#include "lc_copyutils.h"
#include "lc_offsetcurve.h"
#include "rs_block.h"
#include "rs_insert.h"
#include "rs_layer.h"
#include "rs_line.h"
#include "rs_modification.h"
#include "rs_selection.h"

namespace {
struct Pending {
    LC_DocumentModificationBatch ctx;
    ~Pending() { qDeleteAll(ctx.entitiesToAdd); }
};

struct Drawing {
    const bool qtReady{lc::test::application() != nullptr};
    RS_Graphic graphic;
    lc::test::TestGraphicView view;
    LC_ActionContext context;
    Drawing() {
        graphic.initForNewDocument(); graphic.onLoadingCompleted(); view.setDocument(&graphic);
        context.setDocumentAndView(&graphic, &view);
    }
    bool modify(const RS_Document::FunUndoable& operation) { return graphic.undoableModify(view.getViewPort(), operation); }
    QList<RS_Entity*> live() const {
        QList<RS_Entity*> result;
        for (auto* entity : graphic) if (entity && !entity->isDeleted()) result.append(entity);
        return result;
    }
};

class CutProbe : public RS_ActionEditCopyPaste {
public:
    explicit CutProbe(LC_ActionContext* context) : RS_ActionEditCopyPaste(context, RS2::ActionEditCut) {}
    bool run(const RS_Vector& ref, LC_DocumentModificationBatch& ctx) {
        *m_referencePoint = ref;
        return doTriggerModifications(ctx);
    }
};

std::uint64_t bits(const double value) {
    std::uint64_t result;
    static_assert(sizeof(result) == sizeof(value)); std::memcpy(&result, &value, sizeof(result)); return result;
}

bool identical(const RS_Vector& a, const RS_Vector& b) {
    return a.valid == b.valid && bits(a.x) == bits(b.x) && bits(a.y) == bits(b.y) && bits(a.z) == bits(b.z);
}

void checkPoints(const std::vector<RS_Vector>& a, const std::vector<RS_Vector>& b, const double tolerance = 0) {
    REQUIRE(a.size() == b.size());
    for (std::size_t i = 0; i < a.size(); ++i) {
        CAPTURE(i); CHECK(a[i].valid == b[i].valid);
        if (tolerance == 0) CHECK(identical(a[i], b[i]));
        else { CHECK(std::abs(a[i].x - b[i].x) <= tolerance); CHECK(std::abs(a[i].y - b[i].y) <= tolerance); }
    }
}

void checkSpline(const RS_Spline& a, const RS_Spline& b) {
    const auto& x = a.getData(); const auto& y = b.getData();
    CHECK(x.degree == y.degree); CHECK(x.type == y.type); CHECK(x.knotslist == y.knotslist);
    CHECK(x.weights == y.weights); CHECK(x.m_closedFlag == y.m_closedFlag); CHECK(x.m_periodicFlag == y.m_periodicFlag);
    checkPoints(x.controlPoints, y.controlPoints, 1e-10); checkPoints(x.fitPoints, y.fitPoints, 1e-10);
}

void decorate(RS_Entity& entity, RS_Layer* layer) {
    entity.setLayer(layer); entity.setPen(RS_Pen{RS_Color{19, 43, 67}, RS2::Width11, RS2::DashLine});
    entity.setSourceHandle(801); entity.setXDictHandle(802); entity.setReactorHandles({803, 804});
    entity.setOwnerLineage(805); entity.setUserDefVar("E-tools", "foreign value");
    entity.setDrwExtData({std::make_shared<DRW_Variant>(1001, std::string("E_TOOLS_TEST")),
        std::make_shared<DRW_Variant>(1000, std::string("preserve this payload"))});
}

void checkValueAttributes(const RS_Entity& actual, const RS_Entity& original) {
    CHECK(actual.getPen(false) == original.getPen(false)); CHECK(actual.getLayer() == original.getLayer());
    CHECK(actual.getUserDefVar("E-tools") == original.getUserDefVar("E-tools"));
    REQUIRE(actual.getDrwExtData().size() == original.getDrwExtData().size());
    for (std::size_t i = 0; i < original.getDrwExtData().size(); ++i) {
        const auto& a = actual.getDrwExtData()[i]; const auto& b = original.getDrwExtData()[i];
        REQUIRE(a); REQUIRE(b); REQUIRE(a->type() == DRW_Variant::STRING); REQUIRE(b->type() == DRW_Variant::STRING);
        CHECK(a->code() == b->code()); CHECK(*a->content.s == *b->content.s);
    }
}

struct SourceState {
    const RS_Entity& source;
    unsigned flags;
    RS_EntityContainer* parent;
    RS_Layer* layer;
    RS_Pen pen;
    unsigned long long id, lineage, owner;
    quint32 handle, dictionary;
    std::vector<quint32> reactors;
    std::vector<RS_Vector> geometry, grips;
    std::optional<std::uint64_t> fingerprint;
    const RS_Vector* controlStorage = nullptr;
    const RS_Vector* gripStorage = nullptr;
    std::optional<LC_EllipseOffsetDefinition> definition;
    std::optional<bool> ellipseReversed;
    std::optional<RS_InsertData> insertData;
    std::vector<const RS_Entity*> children;
    std::vector<RS_Vector> childEnds;
    QString userValue;
    std::vector<std::pair<int, std::string>> xdata;
    unsigned ownedFlags = 0;
    LC_OffsetCurve::OwnedTail tail;

    explicit SourceState(const RS_Entity& entity) : source(entity), flags(entity.getFlags()), parent(entity.getParent()),
        layer(entity.getLayer()), pen(entity.getPen(false)), id(entity.getId()), lineage(entity.lineage()),
        owner(entity.ownerLineage()), handle(entity.sourceHandle()), dictionary(entity.xDictHandle()), reactors(entity.reactorHandles()) {
        userValue = entity.getUserDefVar("E-tools");
        for (const auto& item : entity.getDrwExtData()) {
            REQUIRE(item); REQUIRE(item->type() == DRW_Variant::STRING); xdata.emplace_back(item->code(), *item->content.s);
        }
        if (const auto* insert = dynamic_cast<const RS_Insert*>(&entity)) insertData = insert->getData();
        if (const auto* ellipse = dynamic_cast<const RS_Ellipse*>(&entity)) {
            const auto& data = ellipse->getData(); geometry = {data.center, data.majorP, {data.ratio, data.angle1, data.angle2}};
            ellipseReversed = data.reversed;
        } else if (const auto* spline = dynamic_cast<const RS_Spline*>(&entity)) {
            geometry = spline->getData().controlPoints; fingerprint = RS_Spline::geometryFingerprint(spline->getData());
            controlStorage = spline->getData().controlPoints.data();
        } else geometry = {entity.getStartpoint(), entity.getEndpoint()};
        if (const auto* native = dynamic_cast<const LC_OffsetCurve*>(&entity)) {
            definition = native->definition(); grips = native->cachedDefinitionGrips(); gripStorage = native->cachedDefinitionGrips().data();
            ownedFlags = native->unknownOwnedFlags(); tail = native->unknownOwnedTail();
        }
        if (const auto* container = dynamic_cast<const RS_EntityContainer*>(&entity))
            for (const auto* child : *container) {
                REQUIRE(child); children.push_back(child); childEnds.push_back(child->getStartpoint()); childEnds.push_back(child->getEndpoint());
            }
    }
    void check() const {
        CHECK(source.getFlags() == flags); CHECK(source.getParent() == parent); CHECK(source.getLayer() == layer);
        CHECK(source.getPen(false) == pen); CHECK(source.getId() == id); CHECK(source.lineage() == lineage);
        CHECK(source.ownerLineage() == owner); CHECK(source.sourceHandle() == handle); CHECK(source.xDictHandle() == dictionary);
        CHECK(source.reactorHandles() == reactors);
        CHECK(source.getUserDefVar("E-tools") == userValue); REQUIRE(source.getDrwExtData().size() == xdata.size());
        for (std::size_t i = 0; i < xdata.size(); ++i) {
            const auto& item = source.getDrwExtData()[i]; REQUIRE(item); REQUIRE(item->type() == DRW_Variant::STRING);
            CHECK(item->code() == xdata[i].first); CHECK(*item->content.s == xdata[i].second);
        }
        if (const auto* insert = dynamic_cast<const RS_Insert*>(&source)) {
            REQUIRE(insertData); const auto actual = insert->getData(); CHECK(actual.name == insertData->name);
            CHECK(identical(actual.insertionPoint, insertData->insertionPoint)); CHECK(identical(actual.scaleFactor, insertData->scaleFactor));
            CHECK(identical(actual.extrusion, insertData->extrusion)); CHECK(bits(actual.angle) == bits(insertData->angle));
            CHECK(actual.cols == insertData->cols); CHECK(actual.rows == insertData->rows); CHECK(identical(actual.spacing, insertData->spacing));
            CHECK(actual.blockSource == insertData->blockSource); CHECK(actual.updateMode == insertData->updateMode);
        }
        if (const auto* ellipse = dynamic_cast<const RS_Ellipse*>(&source)) {
            const auto& data = ellipse->getData(); checkPoints({data.center, data.majorP, {data.ratio, data.angle1, data.angle2}}, geometry);
            REQUIRE(ellipseReversed); CHECK(data.reversed == *ellipseReversed);
        } else if (const auto* spline = dynamic_cast<const RS_Spline*>(&source)) {
            REQUIRE(fingerprint); CHECK(RS_Spline::geometryFingerprint(spline->getData()) == *fingerprint);
            checkPoints(spline->getData().controlPoints, geometry); CHECK(spline->getData().controlPoints.data() == controlStorage);
        } else checkPoints({source.getStartpoint(), source.getEndpoint()}, geometry);
        if (const auto* native = dynamic_cast<const LC_OffsetCurve*>(&source)) {
            REQUIRE(definition); CHECK(native->hasEditableDefinition());
            CHECK(identical(native->definition().basis.center, definition->basis.center));
            CHECK(identical(native->definition().basis.majorP, definition->basis.majorP));
            CHECK(bits(native->definition().basis.ratio) == bits(definition->basis.ratio));
            CHECK(bits(native->definition().basis.angle1) == bits(definition->basis.angle1));
            CHECK(bits(native->definition().basis.angle2) == bits(definition->basis.angle2));
            CHECK(native->definition().basis.reversed == definition->basis.reversed);
            CHECK(bits(native->definition().distance) == bits(definition->distance));
            CHECK(bits(native->definition().relTolerance) == bits(definition->relTolerance));
            CHECK(native->definition().towardCentre == definition->towardCentre);
            CHECK(native->definition().cacheReversed == definition->cacheReversed);
            checkPoints(native->cachedDefinitionGrips(), grips); CHECK(native->cachedDefinitionGrips().data() == gripStorage);
            CHECK(native->unknownOwnedFlags() == ownedFlags); REQUIRE(native->unknownOwnedTail().size() == tail.size());
            for (std::size_t i = 0; i < tail.size(); ++i) {
                CHECK(native->unknownOwnedTail()[i].code == tail[i].code);
                REQUIRE(native->unknownOwnedTail()[i].value.index() == tail[i].value.index());
                REQUIRE(tail[i].value.index() == 0);
                CHECK(std::get<QString>(native->unknownOwnedTail()[i].value) == std::get<QString>(tail[i].value));
            }
        }
        if (const auto* container = dynamic_cast<const RS_EntityContainer*>(&source)) {
            REQUIRE(container->count() == children.size());
            for (std::size_t i = 0; i < children.size(); ++i) {
                CHECK(container->entityAt(static_cast<int>(i)) == children[i]);
                CHECK(identical(children[i]->getStartpoint(), childEnds[2*i])); CHECK(identical(children[i]->getEndpoint(), childEnds[2*i + 1]));
            }
        }
    }
};

RS_Ellipse* ellipse(Drawing& drawing, const RS_EllipseData& basis = {{0, 0}, {5, 0}, 1}) {
    auto* result = new RS_Ellipse(&drawing.graphic, basis); drawing.graphic.addEntity(result);
    decorate(*result, drawing.graphic.getActiveLayer()); return result;
}

std::unique_ptr<LC_OffsetCurve> native(RS_EntityContainer* parent, const bool arc = false) {
    LC_EllipseOffsetDefinition definition;
    definition.basis = {{0, 0}, {1, 0}, 1, 0, arc ? 0.1 : 0, false};
    definition.distance = arc ? 0.25 : 1; definition.relTolerance = 0.01;
    std::unique_ptr<RS_Entity> cache;
    if (arc) cache = std::make_unique<RS_Line>(parent, RS_Vector{1.25, 0}, RS_Vector{1.25*std::cos(0.1), 1.25*std::sin(0.1)});
    else {
        RS_SplineData data{3, false}; std::array<RS_Vector, 4> net{{{2, 0}, {2, 9.0/8}, {9.0/8, 2}, {0, 2}}};
        for (unsigned quadrant = 0; quadrant < 4; ++quadrant) {
            for (unsigned i = quadrant ? 1 : 0; i < 4; ++i) data.controlPoints.push_back(net[i]);
            for (auto& point : net) point = {-point.y, point.x};
        }
        for (unsigned i = 0; i <= 4; ++i)
            for (unsigned j = 0; j < (i == 0 || i == 4 ? 4u : 3u); ++j) data.knotslist.push_back(i);
        data.weights.assign(data.controlPoints.size(), 1); data.m_closedFlag = true; cache = std::make_unique<RS_Spline>(parent, data);
    }
    LC_OffsetCheckResult checked;
    auto result = LC_OffsetCurve::adopt(*cache, definition, arc ? 0.01 : 0.02, &checked);
    CAPTURE(checked.reason); REQUIRE(checked); REQUIRE(result);
    decorate(*result, parent ? parent->getLayer() : nullptr);
    REQUIRE(result->setOwnedExtensions(0x10, {{1000, QStringLiteral("retain on refusal")}}));
    return result;
}

RS_OffsetData request(const bool enabled = true) {
    RS_OffsetData data; data.asOffsetCurve = enabled; data.coord = {20, 20}; data.distance = 0.5;
    data.keepOriginals = false; data.useCurrentAttributes = false; data.useCurrentLayer = false; return data;
}

const LC_OffsetSourceOutcome& single(const LC_OffsetBatchOutcome& result) {
    REQUIRE(result.sources.size() == 1); return result.sources.front();
}

void keptPlain(const LC_OffsetSourceOutcome& result, const LC_OffsetPlainReason reason) {
    REQUIRE(result.succeeded()); CHECK(result.copiesMade == 1); CHECK(result.copiesKeptPlain == 1);
    REQUIRE(result.keptPlain.size() == 1); CHECK(result.keptPlain.front().copy == 1);
    CHECK(result.keptPlain.front().reason == reason); CHECK_FALSE(result.keptPlain.front().checkReason);
    REQUIRE_FALSE(result.createdEntities.empty());
    REQUIRE(result.source);
    for (const auto* entity : result.createdEntities) {
        REQUIRE(entity); CHECK(entity->rtti() != RS2::EntityOffsetCurve); checkValueAttributes(*entity, *result.source);
        CHECK(entity->getId() != result.source->getId()); CHECK(entity->lineage() != result.source->lineage());
        CHECK(entity->sourceHandle() == 0); CHECK(entity->xDictHandle() == 0); CHECK(entity->reactorHandles().empty()); CHECK(entity->ownerLineage() == 0);
    }
}

enum class Operation { ExactMove, RoundedMove, Rotate, Mirror, Scale, Align, MoveRotate, Rotate2 };

bool modify(const Operation op, const QList<RS_Entity*>& entities, const bool preview, LC_DocumentModificationBatch& ctx) {
    switch (op) {
    case Operation::ExactMove: case Operation::RoundedMove: {
        RS_MoveData data; data.offset = op == Operation::ExactMove ? RS_Vector{4, 8} : RS_Vector{0.1, 0.2};
        RS_Modification::move(data, entities, preview, ctx); return ctx.success;
    }
    case Operation::Rotate: { RS_RotateData data; data.center = {0, 0}; data.angle = 0.3; return RS_Modification::rotate(data, entities, preview, ctx); }
    case Operation::Mirror: { RS_MirrorData data; data.axisPoint1 = {0, 0}; data.axisPoint2 = {1, 0}; return RS_Modification::mirror(data, entities, preview, ctx); }
    case Operation::Scale: { RS_ScaleData data; data.referencePoint = {0, 0}; data.factor = {2, 3}; data.isotropicScaling = false; return RS_Modification::scale(data, entities, preview, ctx); }
    case Operation::Align: { LC_AlignRefData data; data.rotationCenter = {0, 0}; data.rotationAngle = 0.3; data.scale = true; data.scaleFactor = 1.5; data.offset = {4, 8}; return RS_Modification::alignRef(data, entities, preview, ctx); }
    case Operation::MoveRotate: { RS_MoveRotateData data; data.referencePoint = {0, 0}; data.offset = {4, 8}; data.angle = 0.3; return RS_Modification::moveRotate(data, entities, preview, ctx); }
    case Operation::Rotate2: { RS_Rotate2Data data; data.center1 = {1, 0}; data.center2 = {0, 3}; data.angle1 = 0.3; data.angle2 = -0.1; return RS_Modification::rotate2(data, entities, preview, ctx); }
    }
    return false;
}

void ordinaryTransform(RS_Entity& entity, const Operation op) {
    switch (op) {
    case Operation::ExactMove: entity.move({4, 8}); break;
    case Operation::RoundedMove: entity.move({0.1, 0.2}); break;
    case Operation::Rotate: entity.rotate({0, 0}, 0.3); break;
    case Operation::Mirror: entity.mirror({0, 0}, {1, 0}); break;
    case Operation::Scale: entity.scale({0, 0}, {2, 3}); break;
    case Operation::Align: entity.rotate({0, 0}, 0.3); entity.scale({0, 0}, {1.5, 1.5}); entity.move({4, 8}); break;
    case Operation::MoveRotate: entity.move({4, 8}); entity.rotate({4, 8}, 0.3); break;
    case Operation::Rotate2: { entity.rotate({1, 0}, 0.3); RS_Vector center{0, 3}; center.rotate({1, 0}, 0.3); entity.rotate(center, -0.1); break; }
    }
}

const std::array<Operation, 8> operations{{Operation::ExactMove, Operation::RoundedMove, Operation::Rotate,
    Operation::Mirror, Operation::Scale, Operation::Align, Operation::MoveRotate, Operation::Rotate2}};
}

TEST_CASE("Native offset option defaults off and committed opt-in preserves attributes with a fresh identity",
          "[modification][offsetcurve][offset]") {
    Drawing drawing; auto* source = ellipse(drawing); const SourceState before{*source};
    CHECK_FALSE(RS_OffsetData{}.asOffsetCurve);
    for (const bool enabled : {false, true}) {
        Pending pending; const auto result = RS_Modification::offsetWithOutcome(request(enabled), {source}, false, {}, pending.ctx);
        const auto& outcome = single(result); REQUIRE(outcome.complete()); REQUIRE(pending.ctx.entitiesToAdd.size() == 1);
        REQUIRE(pending.ctx.entitiesToDelete == QList<RS_Entity*>{source});
        auto* created = pending.ctx.entitiesToAdd.front(); REQUIRE(created != source);
        CHECK(created->rtti() == (enabled ? RS2::EntityOffsetCurve : RS2::EntitySpline));
        CHECK(outcome.copiesKeptPlain == 0); CHECK(outcome.keptPlain.empty());
        CHECK(created->getPen(false) == source->getPen(false)); CHECK(created->getLayer() == source->getLayer());
        CHECK(created->getId() != source->getId());
        if (enabled) {
            const auto* nativeCurve = dynamic_cast<LC_OffsetCurve*>(created); REQUIRE(nativeCurve); CHECK(nativeCurve->hasEditableDefinition());
            checkValueAttributes(*created, *source); CHECK(created->getParent() == &drawing.graphic);
            CHECK(created->lineage() != source->lineage()); CHECK(created->sourceHandle() == 0);
            CHECK(created->xDictHandle() == 0); CHECK(created->reactorHandles().empty()); CHECK(created->ownerLineage() == 0);
        }
        before.check();
    }
}

TEST_CASE("Preview offset opt-in leaves ordinary geometry and usage identical to option off",
          "[modification][offsetcurve][preview]") {
    Drawing drawing; auto* source = ellipse(drawing); const SourceState before{*source}; Pending off, on;
    const auto a = RS_Modification::offsetWithOutcome(request(false), {source}, true, LC_OffsetBatchLimits::preview(), off.ctx);
    const auto b = RS_Modification::offsetWithOutcome(request(true), {source}, true, LC_OffsetBatchLimits::preview(), on.ctx);
    REQUIRE(single(a).complete()); REQUIRE(single(b).complete()); REQUIRE(off.ctx.entitiesToAdd.size() == 1); REQUIRE(on.ctx.entitiesToAdd.size() == 1);
    REQUIRE(off.ctx.entitiesToDelete.empty()); REQUIRE(on.ctx.entitiesToDelete.empty());
    auto* plainA = dynamic_cast<RS_Spline*>(off.ctx.entitiesToAdd.front()); auto* plainB = dynamic_cast<RS_Spline*>(on.ctx.entitiesToAdd.front());
    REQUIRE(plainA); REQUIRE(plainB); CHECK(plainA->rtti() == RS2::EntitySpline); CHECK(plainB->rtti() == RS2::EntitySpline); checkSpline(*plainA, *plainB);
    CHECK(single(a).usage.deepEntities == single(b).usage.deepEntities); CHECK(single(a).usage.cubicPieces == single(b).usage.cubicPieces);
    CHECK(single(b).copiesKeptPlain == 0); CHECK(single(b).keptPlain.empty()); before.check();
}

TEST_CASE("Native wrapping adopts a real cached producer result under an effort cap that forbids refitting",
          "[modification][offsetcurve][offset]") {
    Drawing drawing; auto* source = ellipse(drawing); auto data = request(); const auto budget = makeDefaultOffsetSourceBudget();
    const auto options = LC_CurveOffset::makeOffsetOptions(*source, data.distance);
    auto actual = LC_CurveOffset::createEntities(*source, LC_CurveOffset::makeSideRequest(LC_CurveOffsetSide::Right, data.distance), options, budget);
    REQUIRE(actual.status == LC_CurveOffsetStatus::Ok); REQUIRE(actual.entities.size() == 1);
    const auto* cached = dynamic_cast<const RS_Spline*>(actual.entities.front().get()); REQUIRE(cached);
    LC_OffsetPreviewCache cache; cache.keep(source, LC_CurveOffsetSide::Right, data.distance, budget, actual);
    LC_OffsetBatchLimits limits; limits.maxSamples = 1; Pending pending;
    const auto result = RS_Modification::offsetWithOutcome(data, {source}, false, limits, pending.ctx, &cache);
    REQUIRE(single(result).complete()); CHECK(single(result).copiesKeptPlain == 0);
    REQUIRE(pending.ctx.entitiesToAdd.size() == 1); const auto* adopted = dynamic_cast<LC_OffsetCurve*>(pending.ctx.entitiesToAdd.front());
    REQUIRE(adopted); CHECK(adopted->hasEditableDefinition()); checkSpline(*adopted, *cached);
}

TEST_CASE("Detached and non-document ellipse offsets remain visible plain geometry with a reason",
          "[modification][offsetcurve][fallback]") {
    Drawing drawing;
    for (const bool detached : {false, true}) {
        RS_EntityContainer owner{&drawing.graphic};
        RS_Ellipse source{detached ? nullptr : &owner, RS_EllipseData{{0, 0}, {5, 0}, 1}};
        decorate(source, drawing.graphic.getActiveLayer()); const SourceState before{source}; Pending pending;
        const auto result = RS_Modification::offsetWithOutcome(request(), {&source}, false, {}, pending.ctx);
        keptPlain(single(result), LC_OffsetPlainReason::NotTopLevel); CHECK(pending.ctx.entitiesToAdd.size() == 1);
        if (!detached) CHECK(pending.ctx.entitiesToDelete.empty());
        before.check();
    }
}

TEST_CASE("An uncommitted ellipse clone with a document parent never becomes a native document member",
          "[modification][offsetcurve][fallback]") {
    Drawing drawing; auto* committed = ellipse(drawing);
    auto staged = std::unique_ptr<RS_Ellipse>{static_cast<RS_Ellipse*>(committed->clone())};
    REQUIRE(staged->getParent() == &drawing.graphic); REQUIRE(drawing.graphic.findEntityIndex(staged.get()) < 0);
    const SourceState before{*staged}; Pending pending;
    const auto result = RS_Modification::offsetWithOutcome(request(), {staged.get()}, false, {}, pending.ctx);
    keptPlain(single(result), LC_OffsetPlainReason::NotTopLevel);
    CHECK(pending.ctx.entitiesToDelete.empty()); before.check();
}

TEST_CASE("A real multi-piece elliptic offset counts one kept-plain copy rather than discarding pieces",
          "[modification][offsetcurve][fallback]") {
    Drawing drawing; const double pi = std::acos(-1.0);
    auto* source = ellipse(drawing, {{0, 0}, {100, 0}, 0.05, pi/12, 7*pi/4, false}); const SourceState before{*source};
    auto data = request(); data.coord = {0, 0}; data.distance = 4; Pending pending;
    const auto result = RS_Modification::offsetWithOutcome(data, {source}, false, {}, pending.ctx);
    keptPlain(single(result), LC_OffsetPlainReason::SeveralPieces); REQUIRE(single(result).createdEntities.size() > 1);
    CHECK(pending.ctx.entitiesToAdd == single(result).createdEntities); CHECK(pending.ctx.entitiesToDelete == QList<RS_Entity*>{source}); before.check();
}

TEST_CASE("A real offset engine refusal keeps its source while an unrelated selected sibling succeeds",
          "[modification][offsetcurve][transaction]") {
    Drawing drawing; auto* source = ellipse(drawing); const SourceState before{*source};
    auto* line = new RS_Line(&drawing.graphic, RS_Vector{20, 0}, RS_Vector{30, 0}); drawing.graphic.addEntity(line);
    const SourceState lineBefore{*line}; LC_OffsetBatchLimits limits; limits.maxSamples = 1; Pending pending;
    const auto result = RS_Modification::offsetWithOutcome(request(), {source, line}, false, limits, pending.ctx);
    REQUIRE(result.sources.size() == 2); CHECK_FALSE(result.sources[0].succeeded()); CHECK(result.sources[0].copiesMade == 0);
    CHECK(result.sources[0].engineStatus == LC_CurveOffsetStatus::LimitExceeded); CHECK(result.sources[0].createdEntities.empty());
    CHECK(result.sources[0].copiesKeptPlain == 0); CHECK(result.sources[0].keptPlain.empty());
    CHECK(result.sources[1].complete()); REQUIRE(pending.ctx.entitiesToAdd.size() == 1);
    CHECK(pending.ctx.entitiesToAdd.front()->rtti() == RS2::EntityLine); CHECK(pending.ctx.entitiesToDelete == QList<RS_Entity*>{line});
    before.check(); lineBefore.check();
}

TEST_CASE("Native offset additions and replacement ownership participate in actual undo and redo",
          "[modification][offsetcurve][undo]") {
    Drawing drawing; auto* source = ellipse(drawing); const auto geometry = source->getData();
    auto* sibling = new RS_Line(&drawing.graphic, RS_Vector{20, 0}, RS_Vector{30, 0}); drawing.graphic.addEntity(sibling);
    const SourceState siblingBefore{*sibling}; QList<RS_Entity*> additions, deletions;
    REQUIRE(drawing.modify([&](LC_DocumentModificationBatch& ctx) {
        const auto result = RS_Modification::offsetWithOutcome(request(), {source}, false, {}, ctx);
        additions = ctx.entitiesToAdd; deletions = ctx.entitiesToDelete; return result.anySourceSucceeded();
    }));
    REQUIRE(additions.size() == 1); REQUIRE(deletions == QList<RS_Entity*>{source});
    auto* added = dynamic_cast<LC_OffsetCurve*>(additions.front()); REQUIRE(added); CHECK(added->getParent() == &drawing.graphic);
    CHECK(source->isDeleted()); CHECK_FALSE(added->isDeleted()); CHECK(drawing.live().contains(added)); siblingBefore.check();
    REQUIRE(drawing.graphic.undo()); CHECK_FALSE(source->isDeleted()); CHECK(added->isDeleted());
    CHECK(identical(source->getData().center, geometry.center)); CHECK(identical(source->getData().majorP, geometry.majorP)); siblingBefore.check();
    REQUIRE(drawing.graphic.redo()); CHECK(source->isDeleted()); CHECK_FALSE(added->isDeleted()); CHECK(added->hasEditableDefinition()); siblingBefore.check();
}

TEST_CASE("Native explode first produces one plain spline and only the next explode produces chords",
          "[modification][offsetcurve][explode]") {
    Drawing drawing; auto owned = native(&drawing.graphic); auto* source = owned.release(); drawing.graphic.addEntity(source);
    const SourceState before{*source}; Pending first;
    REQUIRE(RS_Modification::explode({source}, first.ctx)); REQUIRE(first.ctx.entitiesToAdd.size() == 1);
    CHECK(first.ctx.entitiesToDelete == QList<RS_Entity*>{source});
    auto* plain = dynamic_cast<RS_Spline*>(first.ctx.entitiesToAdd.front()); REQUIRE(plain); CHECK(plain->rtti() == RS2::EntitySpline);
    checkSpline(*plain, *source); checkValueAttributes(*plain, *source); REQUIRE(plain->getParent() == &drawing.graphic);
    Pending second; REQUIRE(RS_Modification::explode({plain}, second.ctx)); REQUIRE(second.ctx.entitiesToAdd.size() > 1);
    CHECK(second.ctx.entitiesToDelete == QList<RS_Entity*>{plain});
    for (const auto* child : second.ctx.entitiesToAdd) { REQUIRE(child); CHECK(child->rtti() == RS2::EntityLine); }
    before.check();
}

TEST_CASE("Native transformation APIs preserve exact translation and demote uncertifiable direct transforms",
          "[modification][offsetcurve][transform]") {
    for (const auto operation : operations) {
        CAPTURE(operation); Drawing drawing; auto owned = native(&drawing.graphic, operation == Operation::Mirror);
        auto* source = owned.release(); drawing.graphic.addEntity(source); const SourceState before{*source};
        auto expected = source->toPlainSpline(); ordinaryTransform(*expected, operation); Pending pending;
        REQUIRE(modify(operation, {source}, false, pending.ctx)); REQUIRE(pending.ctx.entitiesToAdd.size() == 1);
        CHECK(pending.ctx.entitiesToDelete == QList<RS_Entity*>{source}); auto* actual = dynamic_cast<RS_Spline*>(pending.ctx.entitiesToAdd.front()); REQUIRE(actual);
        CHECK(actual->rtti() == (operation == Operation::ExactMove ? RS2::EntityOffsetCurve : RS2::EntitySpline));
        checkSpline(*actual, *expected); checkValueAttributes(*actual, *source); before.check();
    }
}

TEST_CASE("Native transformation previews retain their originals and publish only fully transformed clones",
          "[modification][offsetcurve][preview]") {
    for (const auto operation : operations) {
        Drawing drawing; auto owned = native(&drawing.graphic, operation == Operation::Mirror); auto* source = owned.release(); drawing.graphic.addEntity(source);
        const SourceState before{*source}; auto expected = source->toPlainSpline(); ordinaryTransform(*expected, operation); Pending pending;
        auto ordinarySource = source->toPlainSpline(); Pending ordinary;
        REQUIRE(modify(operation, {ordinarySource.get()}, true, ordinary.ctx));
        REQUIRE(modify(operation, {source}, true, pending.ctx)); REQUIRE(pending.ctx.entitiesToAdd.size() == 1);
        CHECK(pending.ctx.entitiesToDelete.size() == ordinary.ctx.entitiesToDelete.size());
        CHECK(pending.ctx.entitiesToDelete.contains(source) == ordinary.ctx.entitiesToDelete.contains(ordinarySource.get()));
        const auto* actual = dynamic_cast<RS_Spline*>(pending.ctx.entitiesToAdd.front()); REQUIRE(actual);
        checkSpline(*actual, *expected); before.check();
    }
}

TEST_CASE("Compound native transforms reject an invalid later step without publishing a valid prefix",
          "[modification][offsetcurve][transaction]") {
    for (unsigned operation = 0; operation < 4; ++operation) {
        CAPTURE(operation); Drawing drawing; auto owned = native(&drawing.graphic); auto* source = owned.release(); drawing.graphic.addEntity(source);
        auto* sibling = new RS_Line(&drawing.graphic, RS_Vector{20, 0}, RS_Vector{30, 0}); drawing.graphic.addEntity(sibling);
        const SourceState before{*source}, siblingBefore{*sibling}; Pending pending; const double nan = std::numeric_limits<double>::quiet_NaN();
        bool accepted = true;
        if (operation == 0) { RS_MoveRotateData data; data.offset = {4, 8}; data.referencePoint = {0, 0}; data.angle = nan; accepted = RS_Modification::moveRotate(data, {source}, false, pending.ctx); }
        if (operation == 1) { RS_Rotate2Data data; data.center1 = {0, 0}; data.center2 = {3, 0}; data.angle1 = 0.3; data.angle2 = nan; accepted = RS_Modification::rotate2(data, {source}, false, pending.ctx); }
        if (operation == 2) { RS_RotateData data; data.center = {0, 0}; data.refPoint = {3, 0}; data.angle = 0.3; data.twoRotations = true; data.secondAngle = nan; accepted = RS_Modification::rotate(data, {source}, false, pending.ctx); }
        if (operation == 3) { LC_AlignRefData data; data.rotationCenter = {0, 0}; data.rotationAngle = 0.3; data.scale = true; data.scaleFactor = nan; data.offset = {4, 8}; accepted = RS_Modification::alignRef(data, {source}, false, pending.ctx); }
        CHECK_FALSE(accepted); CHECK(pending.ctx.entitiesToAdd.empty()); CHECK(pending.ctx.entitiesToDelete.empty()); before.check(); siblingBefore.check();
    }
}

TEST_CASE("Borrowed native containers refuse transforms without preventing an independent sibling edit",
          "[modification][offsetcurve][ownership]") {
    Drawing drawing; auto owned = native(&drawing.graphic); auto* source = owned.release(); drawing.graphic.addEntity(source);
    RS_EntityContainer borrowed{&drawing.graphic, false}; borrowed.addEntity(source);
    auto* sibling = new RS_Line(&drawing.graphic, RS_Vector{20, 0}, RS_Vector{30, 0}); drawing.graphic.addEntity(sibling);
    const SourceState before{*source}, borrowedBefore{borrowed}, siblingBefore{*sibling}; Pending pending;
    REQUIRE(modify(Operation::ExactMove, {&borrowed, sibling}, false, pending.ctx));
    REQUIRE(pending.ctx.entitiesToAdd.size() == 1); CHECK(pending.ctx.entitiesToAdd.front()->rtti() == RS2::EntityLine);
    CHECK(pending.ctx.entitiesToDelete == QList<RS_Entity*>{sibling});
    CHECK(identical(pending.ctx.entitiesToAdd.front()->getStartpoint(), RS_Vector{24, 8}));
    before.check(); borrowedBefore.check(); siblingBefore.check();
}

TEST_CASE("Owned nested native descendants follow every modification transform with their siblings",
          "[modification][offsetcurve][ownership]") {
    for (const auto operation : operations) {
        Drawing drawing; auto* group = new RS_EntityContainer(&drawing.graphic); drawing.graphic.addEntity(group);
        auto owned = native(group); auto* source = owned.release(); group->addEntity(source);
        auto* sibling = new RS_Line(group, RS_Vector{20, 0}, RS_Vector{30, 0}); group->addEntity(sibling);
        const SourceState before{*source}, siblingBefore{*sibling}, groupBefore{*group};
        auto expected = source->toPlainSpline(); ordinaryTransform(*expected, operation);
        auto expectedLine = std::unique_ptr<RS_Entity>{sibling->clone()}; ordinaryTransform(*expectedLine, operation); Pending pending;
        REQUIRE(modify(operation, {group}, false, pending.ctx)); REQUIRE(pending.ctx.entitiesToAdd.size() == 1);
        const auto* actual = dynamic_cast<RS_EntityContainer*>(pending.ctx.entitiesToAdd.front()); REQUIRE(actual); REQUIRE(actual->count() == 2);
        const auto* actualSpline = dynamic_cast<RS_Spline*>(actual->entityAt(0)); REQUIRE(actualSpline); checkSpline(*actualSpline, *expected);
        CHECK(actualSpline->getParent() == actual); CHECK(actual->entityAt(1)->getParent() == actual);
        CHECK(std::hypot(actual->entityAt(1)->getStartpoint().x - expectedLine->getStartpoint().x,
            actual->entityAt(1)->getStartpoint().y - expectedLine->getStartpoint().y) < 1e-10);
        CHECK(actual->entityAt(0) != source); CHECK(actual->entityAt(1) != sibling); before.check(); siblingBefore.check(); groupBefore.check();
    }
}

TEST_CASE("INSERTs containing native block geometry transform their actual expansion without editing the block",
          "[modification][offsetcurve][insert]") {
    for (const auto operation : operations) {
        Drawing drawing; auto* block = new RS_Block(&drawing.graphic, RS_BlockData("offset-native-block", {0, 0}, false)); drawing.graphic.addBlock(block);
        auto owned = native(block); auto* original = owned.release(); block->addEntity(original); const SourceState before{*original};
        auto* insert = new RS_Insert(&drawing.graphic, RS_InsertData("offset-native-block", {0, 0}, {1, 1}, 0, 1, 1, {0, 0})); drawing.graphic.addEntity(insert); insert->update();
        REQUIRE(insert->count() == 1); const auto* display = dynamic_cast<RS_Spline*>(insert->entityAt(0)); REQUIRE(display);
        auto expected = std::make_unique<RS_Spline>(*display); ordinaryTransform(*expected, operation); const SourceState insertBefore{*insert}; Pending pending;
        REQUIRE(modify(operation, {insert}, false, pending.ctx)); REQUIRE(pending.ctx.entitiesToAdd.size() == 1);
        const auto* actual = dynamic_cast<RS_Insert*>(pending.ctx.entitiesToAdd.front()); REQUIRE(actual); REQUIRE(actual->count() == 1);
        const auto* actualDisplay = dynamic_cast<RS_Spline*>(actual->entityAt(0)); REQUIRE(actualDisplay); checkSpline(*actualDisplay, *expected);
        CHECK(actualDisplay->getParent() == actual); before.check(); insertBefore.check();
    }
}

TEST_CASE("Clipboard native copies remain independent under the already-supported exact translation path",
          "[modification][offsetcurve][clipboard]") {
    Drawing source, destination; auto owned = native(&source.graphic); auto* original = owned.release(); source.graphic.addEntity(original);
    original->setSelectionFlag(true);
    const SourceState before{*original}; QList<RS_Entity*> selected{original}; LC_CopyUtils::copy({0, 0}, selected, &source.graphic);
    REQUIRE(destination.modify([&](LC_DocumentModificationBatch& ctx) {
        LC_CopyUtils::paste(LC_CopyUtils::RS_PasteData{{4, 8}}, &destination.graphic, ctx); ctx.dontSetActiveLayerAndPen(); return !ctx.entitiesToAdd.empty();
    }));
    REQUIRE(destination.live().size() == 1); const auto* pasted = dynamic_cast<LC_OffsetCurve*>(destination.live().front()); REQUIRE(pasted);
    REQUIRE(pasted->hasEditableDefinition()); CHECK(pasted->getParent() == &destination.graphic); CHECK(pasted->sourceHandle() == 0);
    auto expected = original->toPlainSpline(); expected->move({4, 8}); checkSpline(*pasted, *expected); before.check();
    REQUIRE(destination.graphic.undo()); CHECK(destination.live().empty()); REQUIRE(destination.graphic.redo()); CHECK(destination.live().size() == 1);
}

TEST_CASE("Native opt-in never hides a real source output-budget refusal or changes option-off geometry",
          "[modification][offsetcurve][limits]") {
    Drawing drawing; auto* source = ellipse(drawing); const SourceState before{*source}; LC_OffsetBatchLimits limits; limits.perSource.maxDeepEntities = 1;
    for (const bool enabled : {false, true}) {
        Pending pending; const auto result = RS_Modification::offsetWithOutcome(request(enabled), {source}, false, limits, pending.ctx);
        const auto& outcome = single(result); CHECK_FALSE(outcome.succeeded()); CHECK(outcome.copiesMade == 0);
        CHECK(outcome.engineStatus == LC_CurveOffsetStatus::LimitExceeded); CHECK(outcome.createdEntities.empty());
        CHECK(outcome.copiesKeptPlain == 0); CHECK(outcome.keptPlain.empty()); CHECK(pending.ctx.entitiesToAdd.empty()); CHECK(pending.ctx.entitiesToDelete.empty()); before.check();
    }
}

TEST_CASE("Clipboard native rounding and similarity fallback publish the fully transformed ordinary cache",
          "[modification][offsetcurve][clipboard]") {
    for (const bool roundedReference : {false, true}) {
        Drawing source, destination; auto owned = native(&source.graphic); auto* original = owned.release();
        source.graphic.addEntity(original); original->setSelectionFlag(true); const SourceState before{*original};
        const RS_Vector reference = roundedReference ? RS_Vector{0.1, 0.2} : RS_Vector{0, 0};
        const LC_CopyUtils::RS_PasteData data{{4, 8}, 1.5, 0.3};
        auto expected = original->toPlainSpline(); expected->move(-reference);
        expected->scale({0, 0}, {data.factor, data.factor}); expected->rotate({0, 0}, data.angle);
        expected->move(data.insertionPoint);
        QList<RS_Entity*> selected{original}; LC_CopyUtils::copy(reference, selected, &source.graphic);
        REQUIRE(destination.modify([&](LC_DocumentModificationBatch& ctx) {
            LC_CopyUtils::paste(data, &destination.graphic, ctx); ctx.dontSetActiveLayerAndPen(); return !ctx.entitiesToAdd.empty();
        }));
        REQUIRE(destination.live().size() == 1); const auto* pasted = dynamic_cast<RS_Spline*>(destination.live().front());
        REQUIRE(pasted); CHECK(pasted->rtti() == RS2::EntitySpline); checkSpline(*pasted, *expected);
        CHECK(pasted->getParent() == &destination.graphic); CHECK(pasted->sourceHandle() == 0); before.check();
        REQUIRE(destination.graphic.undo()); CHECK(destination.live().empty());
        REQUIRE(destination.graphic.redo()); CHECK(destination.live().size() == 1);
    }
}

TEST_CASE("Clipboard native compound refusal does not publish its valid transform prefix",
          "[modification][offsetcurve][clipboard]") {
    Drawing source, destination; auto owned = native(&source.graphic); auto* original = owned.release();
    source.graphic.addEntity(original); original->setSelectionFlag(true); const SourceState before{*original};
    QList<RS_Entity*> selected{original}; LC_CopyUtils::copy({0, 0}, selected, &source.graphic);
    LC_CopyUtils::RS_PasteData data{{4, 8}, 2, std::numeric_limits<double>::quiet_NaN()}; Pending pending;
    LC_CopyUtils::paste(data, &destination.graphic, pending.ctx);
    CHECK(pending.ctx.entitiesToAdd.empty()); CHECK(pending.ctx.entitiesToDelete.empty());
    CHECK(destination.live().empty()); before.check();
}

TEST_CASE("Cut only deletes sources whose staged clipboard copies were actually published",
          "[modification][offsetcurve][clipboard]") {
    Drawing drawing;
    auto* borrowed = new RS_EntityContainer(&drawing.graphic, false); drawing.graphic.addEntity(borrowed);
    auto owned = native(borrowed); borrowed->addEntity(owned.get());
    auto* sibling = new RS_Line(&drawing.graphic, RS_Vector{20, 0}, RS_Vector{30, 0});
    drawing.graphic.addEntity(sibling);
    RS_Selection::selectEntitiesList(&drawing.graphic, drawing.view.getViewPort(), {borrowed, sibling}, true);
    const SourceState before{*owned}, borrowedBefore{*borrowed};
    QList<RS_Entity*> selected{borrowed, sibling};
    CHECK(LC_CopyUtils::copy({0, 0}, selected, &drawing.graphic) == QList<RS_Entity*>{sibling});
    before.check(); borrowedBefore.check();
    CutProbe action{&drawing.context}; Pending pending; REQUIRE(action.run({0, 0}, pending.ctx));
    CHECK(pending.ctx.entitiesToDelete == QList<RS_Entity*>{sibling}); CHECK(pending.ctx.entitiesToAdd.empty());
    CHECK_FALSE(borrowed->isDeleted()); CHECK(borrowed->isSelected()); before.check(); borrowedBefore.check();
    borrowed->clear();
}
