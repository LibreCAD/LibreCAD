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
#include <functional>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QHash>
#include <QPointF>
#include <QThread>
#include <QVariant>

#include "doc_plugin_interface.h"
#include "drw_base.h"
#include "lc_actiontestsupport.h"
#include "lc_offsetcurve.h"
#include "lc_offsetimportsession.h"
#include "rs_block.h"
#include "rs_insert.h"
#include "rs_layer.h"
#include "rs_line.h"

namespace {
struct Drawing {
    const bool qtReady{lc::test::application() != nullptr};
    RS_Graphic graphic;
    lc::test::TestGraphicView view;
    LC_ActionContext context;
    std::unique_ptr<Doc_plugin_interface> dpi;
    Drawing() {
        graphic.initForNewDocument(); graphic.onLoadingCompleted(); view.setDocument(&graphic);
        context.setDocumentAndView(&graphic, &view); dpi = std::make_unique<Doc_plugin_interface>(&context, nullptr);
    }
    QList<RS_Entity*> live() const {
        QList<RS_Entity*> result;
        for (auto* entity : graphic) if (entity && !entity->isDeleted()) result.append(entity);
        return result;
    }
    std::pair<bool, bool> undoState() const {
        bool undo = false, redo = false; graphic.collectUndoState(undo, redo); return {undo, redo};
    }
};

std::uint64_t bits(const double value) {
    std::uint64_t result; static_assert(sizeof(result) == sizeof(value)); std::memcpy(&result, &value, sizeof(result)); return result;
}

bool identical(const RS_Vector& a, const RS_Vector& b) {
    return a.valid == b.valid && bits(a.x) == bits(b.x) && bits(a.y) == bits(b.y) && bits(a.z) == bits(b.z);
}

void pointsEqual(const std::vector<RS_Vector>& a, const std::vector<RS_Vector>& b) {
    REQUIRE(a.size() == b.size());
    for (std::size_t i = 0; i < a.size(); ++i) { CAPTURE(i); CHECK(identical(a[i], b[i])); }
}

void checkSpline(const RS_Spline& actual, const RS_Spline& expected) {
    const auto& a = actual.getData(); const auto& b = expected.getData();
    CHECK(a.degree == b.degree); CHECK(a.type == b.type); CHECK(a.knotslist == b.knotslist); CHECK(a.weights == b.weights);
    CHECK(a.m_closedFlag == b.m_closedFlag); CHECK(a.m_periodicFlag == b.m_periodicFlag);
    REQUIRE(a.controlPoints.size() == b.controlPoints.size());
    for (std::size_t i = 0; i < a.controlPoints.size(); ++i) {
        CAPTURE(i); CHECK(a.controlPoints[i].valid == b.controlPoints[i].valid);
        CHECK(std::abs(a.controlPoints[i].x - b.controlPoints[i].x) < 1e-10);
        CHECK(std::abs(a.controlPoints[i].y - b.controlPoints[i].y) < 1e-10);
        CHECK(std::abs(a.controlPoints[i].z - b.controlPoints[i].z) < 1e-10);
    }
}

void decorate(RS_Entity& entity, RS_Layer* layer) {
    entity.setLayer(layer); entity.setFlag(RS2::FlagSelected); entity.setFlag(RS2::FlagTransparent);
    entity.setPen(RS_Pen{RS_Color{29, 53, 79}, RS2::Width11, RS2::DashLine});
    entity.setSourceHandle(901); entity.setXDictHandle(902); entity.setReactorHandles({903, 904});
    entity.setUserDefVar("plugin-native", "retained foreign value");
    entity.setDrwExtData({std::make_shared<DRW_Variant>(1001, std::string("PLUGIN_NATIVE_TEST")),
        std::make_shared<DRW_Variant>(1000, std::string("retained foreign payload"))});
}

void checkAttributes(const RS_Entity& a, const RS_Entity& b) {
    CHECK(a.getParent() == b.getParent()); CHECK(a.getLayer() == b.getLayer()); CHECK(a.getPen(false) == b.getPen(false));
    CHECK(a.getUserDefVar("plugin-native") == b.getUserDefVar("plugin-native"));
    REQUIRE(a.getDrwExtData().size() == b.getDrwExtData().size());
    for (std::size_t i = 0; i < b.getDrwExtData().size(); ++i) {
        const auto& x = a.getDrwExtData()[i]; const auto& y = b.getDrwExtData()[i];
        REQUIRE(x); REQUIRE(y); REQUIRE(x->type() == DRW_Variant::STRING); REQUIRE(y->type() == DRW_Variant::STRING);
        CHECK(x->code() == y->code()); CHECK(*x->content.s == *y->content.s);
    }
}

std::unique_ptr<LC_OffsetCurve> admitted(RS_EntityContainer* parent, RS_Layer* layer) {
    RS_SplineData data{3, false}; std::array<RS_Vector, 4> net{{{2, 0}, {2, 9.0/8}, {9.0/8, 2}, {0, 2}}};
    for (unsigned quadrant = 0; quadrant < 4; ++quadrant) {
        for (unsigned i = quadrant ? 1 : 0; i < 4; ++i) data.controlPoints.push_back(net[i]);
        for (auto& point : net) point = {-point.y, point.x};
    }
    for (unsigned i = 0; i <= 4; ++i)
        for (unsigned j = 0; j < (i == 0 || i == 4 ? 4u : 3u); ++j) data.knotslist.push_back(i);
    data.weights.assign(data.controlPoints.size(), 1); data.m_closedFlag = true;
    RS_Spline cache{parent, data}; LC_EllipseOffsetDefinition definition;
    definition.basis = {{0, 0}, {1, 0}, 1}; definition.distance = 1; definition.relTolerance = 0.01;
    LC_OffsetCheckResult checked;
    auto result = LC_OffsetCurve::adopt(cache, definition, 0.02, &checked);
    CAPTURE(checked.reason); REQUIRE(checked); REQUIRE(result); decorate(*result, layer);
    REQUIRE(result->setOwnedExtensions(0x10, {{1000, QStringLiteral("owned future data")}})); return result;
}

LC_OffsetCurve* addNative(Drawing& drawing) {
    auto owned = admitted(&drawing.graphic, drawing.graphic.getActiveLayer());
    auto* result = owned.release(); drawing.graphic.addEntity(result); return result;
}

RS_Line* addSibling(Drawing& drawing) {
    auto* result = new RS_Line(&drawing.graphic, RS_Vector{20, 0}, RS_Vector{30, 0}); drawing.graphic.addEntity(result); return result;
}

struct State {
    const RS_Entity& source;
    unsigned flags;
    RS_EntityContainer* parent;
    RS_Layer* layer;
    RS_Pen pen;
    unsigned long long id, lineage, owner;
    quint32 handle, dictionary;
    std::vector<quint32> reactors;
    QString variable;
    std::vector<std::pair<int, std::string>> xdata;
    std::vector<RS_Vector> geometry, grips, childEnds;
    const RS_Vector* controlStorage = nullptr;
    const RS_Vector* gripStorage = nullptr;
    std::optional<std::uint64_t> fingerprint;
    std::optional<LC_EllipseOffsetDefinition> definition;
    double tolerance = 0;
    unsigned ownedFlags = 0;
    LC_OffsetCurve::OwnedTail tail;
    std::vector<const RS_Entity*> children;

    explicit State(const RS_Entity& entity) : source(entity), flags(entity.getFlags()), parent(entity.getParent()), layer(entity.getLayer()),
        pen(entity.getPen(false)), id(entity.getId()), lineage(entity.lineage()), owner(entity.ownerLineage()),
        handle(entity.sourceHandle()), dictionary(entity.xDictHandle()), reactors(entity.reactorHandles()), variable(entity.getUserDefVar("plugin-native")) {
        for (const auto& item : entity.getDrwExtData()) {
            REQUIRE(item); REQUIRE(item->type() == DRW_Variant::STRING); xdata.emplace_back(item->code(), *item->content.s);
        }
        if (const auto* spline = dynamic_cast<const RS_Spline*>(&entity)) {
            fingerprint = RS_Spline::geometryFingerprint(spline->getData()); geometry = spline->getData().controlPoints;
            controlStorage = spline->getData().controlPoints.data();
        } else geometry = {entity.getStartpoint(), entity.getEndpoint()};
        if (const auto* native = dynamic_cast<const LC_OffsetCurve*>(&entity)) {
            definition = native->definition(); grips = native->cachedDefinitionGrips(); gripStorage = native->cachedDefinitionGrips().data();
            tolerance = native->checkedWorldTolerance(); ownedFlags = native->unknownOwnedFlags(); tail = native->unknownOwnedTail();
        }
        if (const auto* container = dynamic_cast<const RS_EntityContainer*>(&entity)) for (const auto* child : *container) {
            REQUIRE(child); children.push_back(child); childEnds.push_back(child->getStartpoint()); childEnds.push_back(child->getEndpoint());
        }
    }

    void check(const bool checkFlags = true) const {
        if (checkFlags) CHECK(source.getFlags() == flags);
        CHECK(source.getParent() == parent); CHECK(source.getLayer() == layer); CHECK(source.getPen(false) == pen);
        CHECK(source.getId() == id); CHECK(source.lineage() == lineage); CHECK(source.ownerLineage() == owner);
        CHECK(source.sourceHandle() == handle); CHECK(source.xDictHandle() == dictionary); CHECK(source.reactorHandles() == reactors);
        CHECK(source.getUserDefVar("plugin-native") == variable); REQUIRE(source.getDrwExtData().size() == xdata.size());
        for (std::size_t i = 0; i < xdata.size(); ++i) {
            const auto& item = source.getDrwExtData()[i]; REQUIRE(item); REQUIRE(item->type() == DRW_Variant::STRING);
            CHECK(item->code() == xdata[i].first); CHECK(*item->content.s == xdata[i].second);
        }
        if (const auto* spline = dynamic_cast<const RS_Spline*>(&source)) {
            REQUIRE(fingerprint); CHECK(RS_Spline::geometryFingerprint(spline->getData()) == *fingerprint);
            pointsEqual(spline->getData().controlPoints, geometry); CHECK(spline->getData().controlPoints.data() == controlStorage);
        } else pointsEqual({source.getStartpoint(), source.getEndpoint()}, geometry);
        if (const auto* native = dynamic_cast<const LC_OffsetCurve*>(&source)) {
            REQUIRE(definition); CHECK(native->hasEditableDefinition());
            const auto& actual = native->definition(); CHECK(identical(actual.basis.center, definition->basis.center));
            CHECK(identical(actual.basis.majorP, definition->basis.majorP)); CHECK(bits(actual.basis.ratio) == bits(definition->basis.ratio));
            CHECK(bits(actual.basis.angle1) == bits(definition->basis.angle1)); CHECK(bits(actual.basis.angle2) == bits(definition->basis.angle2));
            CHECK(actual.basis.reversed == definition->basis.reversed); CHECK(bits(actual.distance) == bits(definition->distance));
            CHECK(bits(actual.relTolerance) == bits(definition->relTolerance)); CHECK(actual.towardCentre == definition->towardCentre);
            CHECK(actual.cacheReversed == definition->cacheReversed); CHECK(bits(native->checkedWorldTolerance()) == bits(tolerance));
            pointsEqual(native->cachedDefinitionGrips(), grips); CHECK(native->cachedDefinitionGrips().data() == gripStorage);
            CHECK(native->unknownOwnedFlags() == ownedFlags); REQUIRE(native->unknownOwnedTail().size() == tail.size());
            for (std::size_t i = 0; i < tail.size(); ++i) {
                CHECK(native->unknownOwnedTail()[i].code == tail[i].code); REQUIRE(tail[i].value.index() == 0);
                REQUIRE(native->unknownOwnedTail()[i].value.index() == 0);
                CHECK(std::get<QString>(native->unknownOwnedTail()[i].value) == std::get<QString>(tail[i].value));
            }
        }
        if (const auto* container = dynamic_cast<const RS_EntityContainer*>(&source)) {
            REQUIRE(container->count() == children.size());
            for (std::size_t i = 0; i < children.size(); ++i) {
                CHECK(container->entityAt(static_cast<int>(i)) == children[i]); CHECK(children[i]->getParent() == &source);
                CHECK(identical(children[i]->getStartpoint(), childEnds[2*i])); CHECK(identical(children[i]->getEndpoint(), childEnds[2*i + 1]));
            }
        }
    }
};

enum class Operation { ExactMove, RoundedMove, Rotate, Scale, MoveRotate };
const std::array<Operation, 5> operations{{Operation::ExactMove, Operation::RoundedMove, Operation::Rotate, Operation::Scale, Operation::MoveRotate}};

void apply(Plugin_Entity& plugin, const Operation operation, const DPI::Disposition disposition) {
    switch (operation) {
    case Operation::ExactMove: plugin.move({4, 8}, disposition); break;
    case Operation::RoundedMove: plugin.move({0.1, 0.2}, disposition); break;
    case Operation::Rotate: plugin.rotate({1, 2}, 0.3, disposition); break;
    case Operation::Scale: plugin.scale({1, 2}, {2, 3}, disposition); break;
    case Operation::MoveRotate: plugin.moveRotate({4, 8}, {1, 2}, 0.3, disposition); break;
    }
}

void applyOrdinary(RS_Entity& entity, const Operation operation) {
    switch (operation) {
    case Operation::ExactMove: entity.move({4, 8}); break;
    case Operation::RoundedMove: entity.move({0.1, 0.2}); break;
    case Operation::Rotate: entity.rotate({1, 2}, 0.3); break;
    case Operation::Scale: entity.scale({1, 2}, {2, 3}); break;
    case Operation::MoveRotate: entity.move({4, 8}); entity.rotate({1, 2}, 0.3); break;
    }
}

void splineAlias(Plugin_Entity& plugin) {
    CHECK(plugin.getEntityType() == RS2::EntitySpline);
    QHash<int, QVariant> data; data.insert(DPI::ETYPE, -1); plugin.getData(&data);
    REQUIRE(data.contains(DPI::ETYPE)); CHECK(data.value(DPI::ETYPE).toInt() == DPI::SPLINE);
    CHECK(data.value(DPI::EID).toULongLong() == plugin.getEnt()->getId());
    CHECK(data.value(DPI::COLOR).toInt() == plugin.getEnt()->getPen(false).getColor().toIntColor());
}

QString nativeOffsetFixture() {
#ifdef LIBRECAD_TEST_DIR
    return QStringLiteral(LIBRECAD_TEST_DIR "/offsetcurve_independent/dxf-binary-R2018-native-degree1.dxf");
#else
    return QStringLiteral("librecad/src/lib/filters/tests/testdata/offsetcurve_independent/dxf-binary-R2018-native-degree1.dxf");
#endif
}

bool waitFor(const std::function<bool()>& ready) {
    QElapsedTimer timer;
    timer.start();
    while (!ready() && timer.elapsed() < 10000) {
        QCoreApplication::processEvents(QEventLoop::AllEvents);
        QThread::msleep(1);
    }
    return ready();
}
}

TEST_CASE("Plugin native type and data remain spline aliases without changing the source",
          "[plugins][offsetcurve][alias]") {
    Drawing drawing; auto* source = addNative(drawing); const State before{*source}; Plugin_Entity plugin{source, drawing.dpi.get()};
    const auto undo = drawing.undoState();
    for (unsigned repeat = 0; repeat < 8; ++repeat) { splineAlias(plugin); CHECK(plugin.getEnt() == source); before.check(); }
    CHECK(drawing.undoState() == undo); CHECK(drawing.live() == QList<RS_Entity*>{source});
}

TEST_CASE("Plugin exact native moves retain definitions and honor KEEP DELETE undo and redo",
          "[plugins][offsetcurve][undo]") {
    for (const auto disposition : {DPI::KEEP_ORIGINAL, DPI::DELETE_ORIGINAL}) {
        Drawing drawing; auto* source = addNative(drawing); auto* sibling = addSibling(drawing);
        const State before{*source}, siblingBefore{*sibling}; auto expected = source->toPlainSpline(); expected->move({4, 8});
        Plugin_Entity plugin{source, drawing.dpi.get()}; plugin.move({4, 8}, disposition);
        auto* moved = dynamic_cast<LC_OffsetCurve*>(plugin.getEnt()); REQUIRE(moved); REQUIRE(moved != source);
        CHECK(moved->hasEditableDefinition()); CHECK(identical(moved->definition().basis.center, RS_Vector{4, 8}));
        CHECK(moved->unknownOwnedFlags() == 0); CHECK(moved->unknownOwnedTail().empty());
        checkSpline(*moved, *expected); checkAttributes(*moved, *source); splineAlias(plugin);
        CHECK(source->isDeleted() == (disposition == DPI::DELETE_ORIGINAL)); CHECK_FALSE(moved->isDeleted());
        CHECK(drawing.live().size() == (disposition == DPI::KEEP_ORIGINAL ? 3 : 2)); before.check(false); siblingBefore.check();
        REQUIRE(drawing.graphic.undo()); CHECK_FALSE(source->isDeleted()); CHECK(moved->isDeleted()); siblingBefore.check();
        REQUIRE(drawing.graphic.redo()); CHECK(source->isDeleted() == (disposition == DPI::DELETE_ORIGINAL)); CHECK_FALSE(moved->isDeleted());
        CHECK(plugin.getEnt() == moved); CHECK(moved->getParent() == &drawing.graphic); siblingBefore.check();
    }
}

TEST_CASE("Plugin rounded moves rotations and nonuniform scales publish correctly transformed plain splines",
          "[plugins][offsetcurve][transform]") {
    for (const auto operation : {Operation::RoundedMove, Operation::Rotate, Operation::Scale})
        for (const auto disposition : {DPI::KEEP_ORIGINAL, DPI::DELETE_ORIGINAL}) {
            CAPTURE(operation, disposition); Drawing drawing; auto* source = addNative(drawing); const State before{*source};
            auto expected = source->toPlainSpline(); applyOrdinary(*expected, operation); Plugin_Entity plugin{source, drawing.dpi.get()};
            apply(plugin, operation, disposition); auto* changed = dynamic_cast<RS_Spline*>(plugin.getEnt()); REQUIRE(changed); REQUIRE(changed != source);
            CHECK(changed->rtti() == RS2::EntitySpline); checkSpline(*changed, *expected); checkAttributes(*changed, *source); splineAlias(plugin);
            CHECK(source->isDeleted() == (disposition == DPI::DELETE_ORIGINAL)); before.check(false);
            REQUIRE(drawing.graphic.undo()); CHECK_FALSE(source->isDeleted()); CHECK(changed->isDeleted());
            REQUIRE(drawing.graphic.redo()); CHECK_FALSE(changed->isDeleted()); checkSpline(*changed, *expected);
        }
}

TEST_CASE("Plugin moveRotate uses the supplied rotation center after moving the full cache",
          "[plugins][offsetcurve][transform]") {
    Drawing drawing; auto* source = addNative(drawing); const State before{*source}; auto expected = source->toPlainSpline();
    applyOrdinary(*expected, Operation::MoveRotate); Plugin_Entity plugin{source, drawing.dpi.get()};
    plugin.moveRotate({4, 8}, {1, 2}, 0.3, DPI::KEEP_ORIGINAL);
    auto* changed = dynamic_cast<RS_Spline*>(plugin.getEnt()); REQUIRE(changed); REQUIRE(changed != source);
    CHECK(changed->rtti() == RS2::EntitySpline); checkSpline(*changed, *expected); CHECK_FALSE(source->isDeleted()); before.check(false);
}

TEST_CASE("Plugin moveRotate refuses a nonfinite later rotation without publishing or applying its move prefix",
          "[plugins][offsetcurve][transaction]") {
    for (const auto disposition : {DPI::KEEP_ORIGINAL, DPI::DELETE_ORIGINAL}) {
        Drawing drawing; auto* source = addNative(drawing); auto* sibling = addSibling(drawing);
        const State before{*source}, siblingBefore{*sibling}; const auto undo = drawing.undoState(); const auto live = drawing.live();
        Plugin_Entity plugin{source, drawing.dpi.get()}; plugin.moveRotate({4, 8}, {1, 2}, std::numeric_limits<double>::quiet_NaN(), disposition);
        CHECK(plugin.getEnt() == source); CHECK(drawing.live() == live); CHECK(drawing.undoState() == undo); before.check(); siblingBefore.check();
    }
}

TEST_CASE("Plugin native transforms without a document interface refuse without an in-place fallback",
          "[plugins][offsetcurve][ownership]") {
    Drawing drawing; auto* source = addNative(drawing); const State before{*source}; const auto live = drawing.live(); const auto undo = drawing.undoState();
    Plugin_Entity plugin{source, static_cast<Doc_plugin_interface*>(nullptr)};
    for (const auto operation : operations) {
        apply(plugin, operation, DPI::DELETE_ORIGINAL); CHECK(plugin.getEnt() == source); before.check();
        CHECK(drawing.live() == live); CHECK(drawing.undoState() == undo);
    }
}

TEST_CASE("Plugin detached foreign-document and generated-child native owners refuse publication unchanged",
          "[plugins][offsetcurve][ownership]") {
    for (unsigned owner = 0; owner < 3; ++owner) {
        CAPTURE(owner); Drawing local, foreign;
        RS_EntityContainer* parent = nullptr;
        if (owner == 1) parent = &foreign.graphic;
        else if (owner == 2) {
            parent = new RS_EntityContainer(&local.graphic); local.graphic.addEntity(parent);
        }
        auto owned = admitted(parent, owner == 1 ? foreign.graphic.getActiveLayer() : local.graphic.getActiveLayer());
        auto* source = owned.get();
        if (parent) { parent->addEntity(owned.release()); REQUIRE(parent->count() == 1); }
        const State before{*source}; const auto localUndo = local.undoState(), foreignUndo = foreign.undoState();
        const auto localLive = local.live(), foreignLive = foreign.live(); Plugin_Entity plugin{source, local.dpi.get()};
        for (const auto operation : operations) {
            apply(plugin, operation, DPI::DELETE_ORIGINAL); CHECK(plugin.getEnt() == source); before.check();
            if (parent) { CHECK(parent->count() == 1); CHECK(parent->entityAt(0) == source); }
            CHECK(local.live() == localLive); CHECK(foreign.live() == foreignLive);
            CHECK(local.undoState() == localUndo); CHECK(foreign.undoState() == foreignUndo);
        }
    }
}

TEST_CASE("Plugin invalid native transform parameters never clear selection or publish",
          "[plugins][offsetcurve][transaction]") {
    Drawing drawing; auto* source = addNative(drawing); const State before{*source}; const auto undo = drawing.undoState();
    Plugin_Entity plugin{source, drawing.dpi.get()};
    plugin.move({std::numeric_limits<double>::quiet_NaN(), 0}); before.check(); CHECK(plugin.getEnt() == source);
    plugin.rotate({0, 0}, std::numeric_limits<double>::infinity()); before.check(); CHECK(plugin.getEnt() == source);
    plugin.scale({0, 0}, {2, std::numeric_limits<double>::quiet_NaN()}); before.check(); CHECK(plugin.getEnt() == source);
    CHECK(drawing.undoState() == undo); CHECK(drawing.live() == QList<RS_Entity*>{source});
}

TEST_CASE("Plugin INSERT transforms rebuild native block expansions without changing their source block",
          "[plugins][offsetcurve][insert]") {
    for (const auto operation : operations) {
        CAPTURE(operation); Drawing drawing;
        auto* block = new RS_Block(&drawing.graphic, RS_BlockData("plugin-native-block", {0, 0}, false)); drawing.graphic.addBlock(block);
        auto owned = admitted(block, drawing.graphic.getActiveLayer()); auto* original = owned.release(); block->addEntity(original); const State before{*original};
        auto* source = new RS_Insert(&drawing.graphic, RS_InsertData("plugin-native-block", {2, 3}, {1, 1}, 0, 1, 1, {0, 0})); drawing.graphic.addEntity(source); source->update();
        REQUIRE(source->count() == 1); const auto* display = dynamic_cast<RS_Spline*>(source->entityAt(0)); REQUIRE(display);
        auto expected = std::make_unique<RS_Spline>(*display); applyOrdinary(*expected, operation); Plugin_Entity plugin{source, drawing.dpi.get()};
        apply(plugin, operation, DPI::DELETE_ORIGINAL); const auto* changed = dynamic_cast<RS_Insert*>(plugin.getEnt()); REQUIRE(changed); REQUIRE(changed != source);
        REQUIRE(changed->count() == 1); const auto* actual = dynamic_cast<RS_Spline*>(changed->entityAt(0)); REQUIRE(actual);
        checkSpline(*actual, *expected); CHECK(actual->getParent() == changed); CHECK(changed->getParent() == &drawing.graphic); CHECK(source->isDeleted()); before.check();
        REQUIRE(drawing.graphic.undo()); CHECK_FALSE(source->isDeleted()); CHECK(changed->isDeleted()); before.check();
        REQUIRE(drawing.graphic.redo()); CHECK_FALSE(changed->isDeleted()); before.check();
    }
}

TEST_CASE("Plugin ordinary line transforms retain their existing KEEP DELETE geometry and undo behavior",
          "[plugins][offsetcurve][line-regression]") {
    for (const auto operation : operations) for (const auto disposition : {DPI::KEEP_ORIGINAL, DPI::DELETE_ORIGINAL}) {
        CAPTURE(operation, disposition); Drawing drawing;
        auto* source = new RS_Line(&drawing.graphic, RS_Vector{2, 3}, RS_Vector{7, 5}); drawing.graphic.addEntity(source);
        decorate(*source, drawing.graphic.getActiveLayer()); const State before{*source};
        auto expected = std::unique_ptr<RS_Entity>{source->clone()}; applyOrdinary(*expected, operation); Plugin_Entity plugin{source, drawing.dpi.get()};
        apply(plugin, operation, disposition); const auto* changed = dynamic_cast<RS_Line*>(plugin.getEnt()); REQUIRE(changed); REQUIRE(changed != source);
        CHECK(plugin.getEntityType() == RS2::EntityLine); checkAttributes(*changed, *source);
        CHECK(changed->getStartpoint().distanceTo(expected->getStartpoint()) < 1e-10);
        CHECK(changed->getEndpoint().distanceTo(expected->getEndpoint()) < 1e-10);
        CHECK(source->isDeleted() == (disposition == DPI::DELETE_ORIGINAL)); before.check(false);
        REQUIRE(drawing.graphic.undo()); CHECK_FALSE(source->isDeleted()); CHECK(changed->isDeleted());
        REQUIRE(drawing.graphic.redo()); CHECK_FALSE(changed->isDeleted());
    }
}

TEST_CASE("Plugin legacy synchronous block import retains its immediate result contract",
          "[plugins][offsetcurve][block-import][legacy]") {
    Drawing drawing;
    REQUIRE(drawing.qtReady);
    const QString name = drawing.dpi->addBlockfromFromdisk(nativeOffsetFixture());
    REQUIRE_FALSE(name.isEmpty());
    auto* block = drawing.graphic.findBlock(name);
    REQUIRE(block);
    bool foundNativeOffset = false;
    for (int i = 0; i < block->count(); ++i)
        foundNativeOffset |= dynamic_cast<LC_OffsetCurve*>(block->entityAt(i)) != nullptr;
    CHECK(foundNativeOffset);
}

TEST_CASE("Plugin asynchronous block import stages DXF before preserving its native offset curve",
          "[plugins][offsetcurve][block-import][async]") {
    Drawing drawing;
    REQUIRE(drawing.qtReady);
    const QString file = nativeOffsetFixture();
    const int blocksBefore = drawing.graphic.getBlockList()->count();
    bool completed = false;
    QString name, error;
    drawing.dpi->addBlockFromDiskAsync(file, [&](const QString& blockName, const QString& failure) {
        completed = true;
        name = blockName;
        error = failure;
    });

    CHECK_FALSE(completed);
    CHECK(drawing.graphic.getBlockList()->count() == blocksBefore);
    REQUIRE(waitFor([&] { return completed; }));
    CHECK(error.isEmpty());
    REQUIRE_FALSE(name.isEmpty());
    auto* block = drawing.graphic.findBlock(name);
    REQUIRE(block);
    CHECK(drawing.graphic.getBlockList()->count() == blocksBefore + 1);
    bool foundNativeOffset = false;
    for (int i = 0; i < block->count(); ++i) {
        const auto* native = dynamic_cast<LC_OffsetCurve*>(block->entityAt(i));
        if (native != nullptr) {
            foundNativeOffset = true;
            CHECK(native->getParent() == block);
            CHECK(native->hasEditableDefinition());
        }
    }
    CHECK(foundNativeOffset);
}

TEST_CASE("Plugin asynchronous block import refuses publication after the view changes documents",
          "[plugins][offsetcurve][block-import][async][stale]") {
    Drawing drawing;
    REQUIRE(drawing.qtReady);
    RS_Graphic replacement;
    replacement.initForNewDocument();
    replacement.onLoadingCompleted();
    const int originalBlocks = drawing.graphic.getBlockList()->count();
    const int replacementBlocks = replacement.getBlockList()->count();
    bool completed = false;
    QString name, error;
    drawing.dpi->addBlockFromDiskAsync(nativeOffsetFixture(), [&](const QString& blockName, const QString& failure) {
        completed = true;
        name = blockName;
        error = failure;
    });
    drawing.view.setDocument(&replacement);

    REQUIRE(waitFor([&] { return completed; }));
    CHECK(name.isEmpty());
    CHECK_FALSE(error.isEmpty());
    CHECK(drawing.graphic.getBlockList()->count() == originalBlocks);
    CHECK(replacement.getBlockList()->count() == replacementBlocks);
}

TEST_CASE("Plugin asynchronous block imports limit each view to one active worker",
          "[plugins][offsetcurve][block-import][async][queue]") {
    Drawing drawing;
    REQUIRE(drawing.qtReady);
    bool firstCompleted = false, secondCompleted = false;
    QString firstName, firstError, secondName, secondError;
    drawing.dpi->addBlockFromDiskAsync(nativeOffsetFixture(), [&](const QString& name, const QString& error) {
        firstCompleted = true;
        firstName = name;
        firstError = error;
    });
    drawing.dpi->addBlockFromDiskAsync(nativeOffsetFixture(), [&](const QString& name, const QString& error) {
        secondCompleted = true;
        secondName = name;
        secondError = error;
    });

    CHECK_FALSE(firstCompleted);
    CHECK(secondCompleted);
    CHECK(secondName.isEmpty());
    CHECK_FALSE(secondError.isEmpty());
    REQUIRE(waitFor([&] { return firstCompleted; }));
    CHECK(firstError.isEmpty());
    REQUIRE_FALSE(firstName.isEmpty());
    CHECK(drawing.graphic.findBlock(firstName) != nullptr);
}

TEST_CASE("Plugin asynchronous block import is cancelled with its owning view",
          "[plugins][offsetcurve][block-import][async][lifetime]") {
    int deliveries = 0;
    {
        auto drawing = std::make_unique<Drawing>();
        REQUIRE(drawing->qtReady);
        drawing->dpi->addBlockFromDiskAsync(nativeOffsetFixture(), [&](const QString&, const QString&) {
            ++deliveries;
        });
        bool workerBusy = false;
        QElapsedTimer timer;
        timer.start();
        while (!workerBusy && timer.elapsed() < 5000) {
            QCoreApplication::processEvents(QEventLoop::AllEvents);
            for (QObject* child : drawing->view.children()) {
                if (auto* session = dynamic_cast<LC_OffsetImportSession*>(child))
                    workerBusy |= session->busy();
            }
            if (!workerBusy) QThread::msleep(1);
        }
        REQUIRE(workerBusy);
        drawing.reset();
    }
    QCoreApplication::processEvents(QEventLoop::AllEvents);
    CHECK(deliveries == 0);
}
