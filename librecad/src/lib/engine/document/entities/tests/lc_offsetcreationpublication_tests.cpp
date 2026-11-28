/* Copyright (C) 2026 LibreCAD.org. SPDX-License-Identifier: GPL-2.0-or-later */
#include <catch2/catch_test_macros.hpp>

#include "lc_actiontestsupport.h"
#include "lc_graphicviewport.h"
#include "lc_offsetcurve.h"
#include "lc_offsetcurvepublication.h"
#include "lc_offsetcurveeditsnapshot.h"
#include "lc_offsetbatchpreparation.h"
#include "drw_base.h"
#include "rs_block.h"
#include "rs_ellipse.h"
#include "rs_graphic.h"
#include "rs_layer.h"
#include "rs_line.h"
#include "rs_polyline.h"

namespace {
class Graphic final : public RS_Graphic {
public:
    using RS_Graphic::countUndoCycles;
    using RS_Graphic::countRedoCycles;
    using RS_Graphic::select;
};
struct Fixture {
    const bool ready = lc::test::application() != nullptr;
    Graphic graphic;
    LC_GraphicViewport viewport;
    Fixture() { REQUIRE(ready); graphic.initForNewDocument(); }
    RS_Line* line(RS_Vector start = {0, 0}, RS_Vector end = {2, 0}) {
        auto* entity = new RS_Line(&graphic, start, end);
        entity->setLayer(graphic.getActiveLayer()); graphic.addEntity(entity); return entity;
    }
    std::unique_ptr<RS_Line> added(RS_Vector start = {20, 20}, RS_Vector end = {30, 20}) {
        auto entity = std::make_unique<RS_Line>(nullptr, start, end);
        entity->setLayer(graphic.getActiveLayer()); return entity;
    }
    RS_Polyline* polyline() {
        auto* entity = new RS_Polyline(&graphic);
        entity->addVertex({0, 0}); entity->addVertex({2, 0}); entity->addVertex({2, 2});
        entity->setLayer(graphic.getActiveLayer()); graphic.addEntity(entity); return entity;
    }
};
}

TEST_CASE("Offset creation publication adds unequal native and ordinary outputs with one undo cycle",
          "[lc-offsetcurve][creation-publication]") {
    Fixture f;
    auto* source = new RS_Ellipse(&f.graphic, RS_EllipseData{{0, 0}, {1, 0}, 1, 0, .1, false});
    source->setLayer(f.graphic.getActiveLayer()); f.graphic.addEntity(source);
    f.graphic.select(QList<RS_Entity*>{source}, true); f.graphic.setModified(false);
    auto line = f.added(); auto native = LC_OffsetCurve::create(*source, .25, false); REQUIRE(native);
    native.entity->setParent(nullptr);
    auto* lineAddress = line.get(); auto* curveAddress = native.entity.get();
    auto transaction = LC_OffsetCurvePublication::prepareCreation(f.graphic, f.viewport, {},
        {lineAddress, curveAddress}, {lineAddress, curveAddress}, {source}, {source});
    REQUIRE(transaction); CHECK(f.graphic.count() == 1); CHECK(f.graphic.countUndoCycles() == 0);
    CHECK(source->isSelected()); CHECK_FALSE(f.graphic.isModified());
    REQUIRE(transaction->commit()); line.release(); native.entity.release(); transaction->notify();
    CHECK(f.graphic.count() == 3); CHECK(f.graphic.countUndoCycles() == 1);
    CHECK_FALSE(source->isDeleted()); CHECK_FALSE(source->isSelected());
    CHECK(lineAddress->isSelected()); CHECK(curveAddress->isSelected());
    CHECK(curveAddress->hasEditableDefinition()); CHECK(f.graphic.getMax().x >= 30);
    REQUIRE(f.graphic.undo()); CHECK(lineAddress->isDeleted()); CHECK(curveAddress->isDeleted());
    CHECK_FALSE(source->isDeleted()); CHECK_FALSE(f.graphic.undo());
    REQUIRE(f.graphic.redo()); CHECK_FALSE(lineAddress->isDeleted()); CHECK_FALSE(curveAddress->isDeleted());
}

TEST_CASE("Offset creation publication removes only successful sources and retains failed selection",
          "[lc-offsetcurve][creation-publication]") {
    Fixture f;
    auto* source = f.line(); auto* failed = f.line({0, 4}, {2, 4});
    f.graphic.select({source, failed}, true);
    auto first = f.added(), second = f.added({20, 21}, {30, 21}), third = f.added({20, 22}, {30, 22});
    auto* a = first.get(); auto* b = second.get(); auto* c = third.get();
    auto transaction = LC_OffsetCurvePublication::prepareCreation(f.graphic, f.viewport, {source},
        {a, b, c}, {a, b, c}, {source}, {source});
    REQUIRE(transaction); REQUIRE(transaction->commit());
    first.release(); second.release(); third.release(); transaction->notify();
    CHECK(source->isDeleted()); CHECK_FALSE(failed->isDeleted()); CHECK(failed->isSelected());
    CHECK_FALSE(source->isSelected()); CHECK(a->isSelected()); CHECK(b->isSelected()); CHECK(c->isSelected());
    CHECK(f.graphic.countUndoCycles() == 1); REQUIRE(f.graphic.undo());
    CHECK_FALSE(source->isDeleted()); CHECK(a->isDeleted()); CHECK(b->isDeleted()); CHECK(c->isDeleted());
    REQUIRE(f.graphic.redo()); CHECK(source->isDeleted()); CHECK_FALSE(a->isDeleted());
}

TEST_CASE("Offset creation publication deselects a polyline child without touching its failed peer",
          "[lc-offsetcurve][creation-publication]") {
    Fixture f; auto* source = f.polyline();
    auto* first = source->entityAt(0); auto* second = source->entityAt(1);
    f.graphic.select({first, second}, true);
    REQUIRE(first->isSelected()); REQUIRE(second->isSelected());
    auto line = f.added(); auto* added = line.get();
    CHECK_FALSE(LC_OffsetCurvePublication::prepareCreation(f.graphic, f.viewport, {}, {added},
        {added}, {first}, {}));
    auto transaction = LC_OffsetCurvePublication::prepareCreation(f.graphic, f.viewport, {}, {added},
        {added}, {first}, {source});
    REQUIRE(transaction); REQUIRE(transaction->commit()); line.release(); transaction->notify();
    CHECK_FALSE(first->isSelected()); CHECK(second->isSelected()); CHECK_FALSE(source->isDeleted());
    CHECK(added->isSelected()); CHECK(source->count() == 2); CHECK(f.graphic.countUndoCycles() == 1);
}

TEST_CASE("Offset creation publication rejects changed child membership before reading retired addresses",
          "[lc-offsetcurve][creation-publication][stale][asan]") {
    Fixture f; auto* source = f.polyline(); f.graphic.select(QList<RS_Entity*>{source}, true);
    auto added = f.added();
    auto transaction = LC_OffsetCurvePublication::prepareCreation(f.graphic, f.viewport, {}, {added.get()},
        {added.get()}, {source}, {source});
    REQUIRE(transaction);
    REQUIRE(source->removeEntity(source->entityAt(1)));
    CHECK_FALSE(transaction->commit()); CHECK(added->getParent() == nullptr);
    CHECK(f.graphic.countUndoCycles() == 0); CHECK(f.graphic.count() == 1);
}

TEST_CASE("Offset creation publication stages relative zero and prunes only obsolete redo roots",
          "[lc-offsetcurve][creation-publication][redo]") {
    Fixture f; auto* source = f.line();
    const auto marked = f.viewport.getMarkedRelativeZero(); f.viewport.setRelativeZero({12, 34});
    auto first = f.added(); auto* old = first.get();
    auto initial = LC_OffsetCurvePublication::prepareCreation(f.graphic, f.viewport, {}, {old},
        {}, {}, {});
    REQUIRE(initial); CHECK(f.viewport.getMarkedRelativeZero() == marked);
    REQUIRE(initial->commit()); first.release(); initial->notify();
    CHECK(f.viewport.getMarkedRelativeZero() == RS_Vector(12, 34));
    REQUIRE(f.graphic.undo()); CHECK(old->isDeleted()); CHECK(f.viewport.getRelativeZero() == marked);
    REQUIRE(f.graphic.redo()); CHECK(f.viewport.getRelativeZero() == RS_Vector(12, 34));
    REQUIRE(f.graphic.undo()); CHECK(f.graphic.countRedoCycles() == 1);
    auto next = f.added({40, 20}, {50, 20}); auto* fresh = next.get();
    auto transaction = LC_OffsetCurvePublication::prepareCreation(f.graphic, f.viewport, {}, {fresh},
        {}, {}, {});
    REQUIRE(transaction); CHECK(f.graphic.count() == 2);
    REQUIRE(transaction->commit()); next.release(); transaction->notify();
    CHECK(f.graphic.count() == 2); CHECK(f.graphic.findEntityIndex(old) < 0);
    CHECK(f.graphic.findEntityIndex(source) >= 0); CHECK(f.graphic.findEntityIndex(fresh) >= 0);
    CHECK(f.graphic.countRedoCycles() == 0); CHECK(f.graphic.countUndoCycles() == 1);
    REQUIRE(f.graphic.undo()); CHECK(fresh->isDeleted()); CHECK_FALSE(source->isDeleted());
}

TEST_CASE("Offset creation publication refuses changed redo subtrees without touching freed children",
          "[lc-offsetcurve][creation-publication][redo][stale][asan]") {
    Fixture f; auto* source = f.line();
    auto polyline = std::make_unique<RS_Polyline>(nullptr);
    polyline->addVertex({0, 2}); polyline->addVertex({2, 2}); polyline->addVertex({2, 4});
    polyline->setLayer(f.graphic.getActiveLayer()); auto* old = polyline.get();
    auto initial = LC_OffsetCurvePublication::prepareCreation(f.graphic, f.viewport, {}, {old}, {}, {}, {});
    REQUIRE(initial); REQUIRE(initial->commit()); polyline.release(); initial->notify();
    REQUIRE(f.graphic.undo());
    auto added = f.added();
    auto transaction = LC_OffsetCurvePublication::prepareCreation(f.graphic, f.viewport, {}, {added.get()}, {}, {}, {});
    REQUIRE(transaction); REQUIRE(old->removeEntity(old->entityAt(1)));
    CHECK_FALSE(transaction->commit()); CHECK(added->getParent() == nullptr);
    CHECK(f.graphic.countRedoCycles() == 1); CHECK(f.graphic.countUndoCycles() == 0);
    CHECK_FALSE(source->isDeleted());
}

TEST_CASE("Offset creation publication tracks an enclosing graphic independently of a surviving block",
          "[lc-offsetcurve][creation-publication][lifetime][asan]") {
    REQUIRE(lc::test::application());
    auto graphic = std::make_unique<Graphic>(); graphic->initForNewDocument();
    RS_Block block{graphic.get(), RS_BlockData{"block", {0, 0}, false}};
    LC_GraphicViewport viewport;
    auto* source = new RS_Line(&block, RS_Vector{0, 0}, RS_Vector{2, 0});
    source->setLayer(graphic->getActiveLayer()); block.addEntity(source);
    auto added = std::make_unique<RS_Line>(nullptr, RS_Vector{0, 1}, RS_Vector{2, 1});
    added->setLayer(graphic->getActiveLayer());
    auto transaction = LC_OffsetCurvePublication::prepareCreation(block, viewport, {}, {added.get()}, {}, {}, {});
    REQUIRE(transaction); graphic.reset(); CHECK_FALSE(transaction->commit());
    source->setLayer(nullptr); added->setLayer(nullptr); block.setParent(nullptr);
}

TEST_CASE("Offset creation sources own ellipse and native inputs without retaining live layers",
          "[lc-offsetcurve][creation-sources][asan]") {
    REQUIRE(lc::test::application());
    auto graphic = std::make_unique<Graphic>(); graphic->initForNewDocument();
    auto* ellipse = new RS_Ellipse(graphic.get(), RS_EllipseData{{0, 0}, {1, 0}, 1, 0, .1, false});
    ellipse->setLayer(graphic->getActiveLayer()); graphic->addEntity(ellipse);
    auto native = LC_OffsetCurve::create(*ellipse, .25, false); REQUIRE(native);
    auto* curve = native.entity.get(); curve->setParent(graphic.get()); curve->setLayer(graphic->getActiveLayer());
    graphic->addEntity(curve); native.entity.release();
    auto inputs = LC_OffsetCreationSources::capture(*graphic, {ellipse, curve->entityAt(0), curve});
    REQUIRE(inputs); REQUIRE(inputs->needsWorker()); REQUIRE(inputs->sources().size() == 2);
    CHECK(inputs->originals() == QList<RS_Entity*>{ellipse, curve});
    CHECK(inputs->original(inputs->sources()[0]) == ellipse);
    CHECK(inputs->sources()[0]->getLayer(false) != ellipse->getLayer(false));
    CHECK(inputs->matches(*graphic));
    graphic.reset();
    CHECK(dynamic_cast<RS_Ellipse*>(inputs->sources()[0])->getData().ratio == 1);
    CHECK(dynamic_cast<LC_OffsetCurve*>(inputs->sources()[1])->hasEditableDefinition());
    CHECK_FALSE(inputs->sources()[0]->isLocked());
}

TEST_CASE("Offset creation source seals cover stored geometry metadata and layer changes",
          "[lc-offsetcurve][creation-sources]") {
    Fixture f;
    auto* ellipse = new RS_Ellipse(&f.graphic, RS_EllipseData{{0, 0}, {4, 0}, .5, 0, 0, false});
    ellipse->setLayer(f.graphic.getActiveLayer()); f.graphic.addEntity(ellipse);
    auto tag = std::make_shared<DRW_Variant>(1000, std::string("before")); ellipse->setDrwExtData({tag});
    auto inputs = LC_OffsetCreationSources::capture(f.graphic, {ellipse}); REQUIRE(inputs);
    SECTION("geometry") { ellipse->setRatio(.75); }
    SECTION("active metadata payload") { *tag->content.s = "after"; }
    SECTION("layer") { ellipse->getLayer(false)->lock(true); }
    SECTION("provenance") { ellipse->setSourceHandle(77); }
    CHECK_FALSE(inputs->matches(f.graphic));
}

TEST_CASE("Offset creation source ownership preserves polyline grouping and localizes only private outputs",
          "[lc-offsetcurve][creation-sources]") {
    Fixture f; auto* polyline = f.polyline();
    auto* first = polyline->entityAt(0); auto* second = polyline->entityAt(1);
    auto inputs = LC_OffsetCreationSources::capture(f.graphic, {first, second, polyline}); REQUIRE(inputs);
    REQUIRE(inputs->sources().size() == 3); CHECK_FALSE(inputs->needsWorker());
    CHECK(inputs->sources()[0]->getParent() == inputs->sources()[2]);
    auto clone = std::unique_ptr<RS_Entity>(inputs->sources()[0]->clone()); clone->setParent(nullptr);
    REQUIRE(inputs->localize({clone.get()}, f.graphic.getActiveLayer(), false));
    CHECK(clone->getLayer(false) == first->getLayer(false)); CHECK(inputs->matches(f.graphic));
    REQUIRE(polyline->removeEntity(second)); CHECK_FALSE(inputs->matches(f.graphic));
    CHECK(inputs->sources()[2]->isVisible()); // Its private children still exist.
}

TEST_CASE("Offset creation source capture rejects stale addresses and oversized dormant spline arrays",
          "[lc-offsetcurve][creation-sources][asan]") {
    Fixture f; auto* source = f.line();
    REQUIRE(f.graphic.removeEntity(source));
    CHECK_FALSE(LC_OffsetCreationSources::capture(f.graphic, {source}));
    auto* spline = new RS_Spline(&f.graphic, RS_SplineData{1, false}); f.graphic.addEntity(spline);
    spline->getData().savedOpenKnots.resize(4*LC_OffsetCreationSources::MaximumNodes + 1);
    CHECK_FALSE(LC_OffsetCreationSources::capture(f.graphic, {spline}));
}

TEST_CASE("Trim snapshots own detached limits and reject stale source values",
          "[lc-offsetcurve][creation-sources][trim-sources][asan]") {
    Fixture f; auto* target = f.line();
    auto limit = std::make_unique<RS_Line>(nullptr, RS_Vector{1, -1}, RS_Vector{1, 1});
    auto targetTag = std::make_shared<DRW_Variant>(1000, std::string("target"));
    auto limitTag = std::make_shared<DRW_Variant>(1000, std::string("limit"));
    target->setDrwExtData({targetTag}); limit->setDrwExtData({limitTag});
    auto inputs = LC_OffsetCreationSources::captureDetachedLimit(f.graphic, *target, *limit);
    REQUIRE(inputs); REQUIRE(inputs->sources().size() == 2);
    CHECK(inputs->roots() == QList<RS_Entity*>{target});
    CHECK_FALSE(inputs->matches(f.graphic));
    CHECK_FALSE(inputs->matchesDetached(f.graphic, nullptr));
    CHECK(inputs->matchesDetached(f.graphic, limit.get()));
    auto frozenTarget = inputs->cloneWorkerSource(0);
    auto frozenLimit = inputs->cloneWorkerSource(1);
    REQUIRE(frozenTarget); REQUIRE(frozenLimit);
    CHECK_FALSE(inputs->cloneWorkerSource(2));
    CHECK(frozenTarget->getParent() == nullptr); CHECK(frozenTarget->getLayer(false) == nullptr);
    CHECK(frozenLimit->getParent() == nullptr); CHECK(frozenLimit->getLayer(false) == nullptr);
    REQUIRE(frozenTarget->getDrwExtData().size() == 1); REQUIRE(frozenLimit->getDrwExtData().size() == 1);
    CHECK(frozenTarget->getDrwExtData()[0] != targetTag);
    CHECK(frozenLimit->getDrwExtData()[0] != limitTag);
    SECTION("detached geometry changes") { limit->move({3, 0}); }
    SECTION("document source changes") { target->move({3, 0}); }
    SECTION("detached metadata changes") { limit->setSourceHandle(99); }
    SECTION("shared extended-data value changes") { *targetTag->content.s = "changed"; }
    SECTION("owned limit replaced") {
        auto replacement = std::make_unique<RS_Line>(nullptr, RS_Vector{1, -1}, RS_Vector{1, 1});
        CHECK_FALSE(inputs->matchesDetached(f.graphic, replacement.get()));
        return;
    }
    CHECK_FALSE(inputs->matchesDetached(f.graphic, limit.get()));
    CHECK(dynamic_cast<RS_Line*>(frozenTarget.get())->getData().startpoint == RS_Vector(0, 0));
    CHECK(dynamic_cast<RS_Line*>(frozenLimit.get())->getData().startpoint == RS_Vector(1, -1));
    CHECK(*frozenTarget->getDrwExtData()[0]->content.s == "target");
}

TEST_CASE("Trim snapshots refuse non-detached limits and non-root targets",
          "[lc-offsetcurve][creation-sources][trim-sources]") {
    Fixture f; auto* target = f.line(); auto* other = f.line({1, -1}, {1, 1});
    CHECK_FALSE(LC_OffsetCreationSources::captureDetachedLimit(f.graphic, *target, *other));
    auto detached = std::make_unique<RS_Line>(nullptr, RS_Vector{1, -1}, RS_Vector{1, 1});
    detached->setLayer(f.graphic.getActiveLayer());
    CHECK_FALSE(LC_OffsetCreationSources::captureDetachedLimit(f.graphic, *target, *detached));
    detached->setLayer(nullptr);
    auto* polyline = f.polyline();
    CHECK_FALSE(LC_OffsetCreationSources::captureDetachedLimit(f.graphic, *polyline->entityAt(0), *detached));
    CHECK_FALSE(LC_OffsetCreationSources::captureDetachedLimit(f.graphic, *target, *target));
}

TEST_CASE("Trim worker clones outlive both source drawing and snapshot",
          "[lc-offsetcurve][creation-sources][trim-sources][lifetime][asan]") {
    REQUIRE(lc::test::application());
    std::unique_ptr<RS_Entity> workerTarget, workerLimit;
    {
        auto graphic = std::make_unique<Graphic>(); graphic->initForNewDocument();
        auto* ellipse = new RS_Ellipse(graphic.get(), RS_EllipseData{{0, 0}, {4, 0}, .5, 0, 0, false});
        ellipse->setLayer(graphic->getActiveLayer()); graphic->addEntity(ellipse);
        auto native = LC_OffsetCurve::create(*ellipse, .25, false); REQUIRE(native);
        auto* target = native.entity.get(); target->setParent(graphic.get());
        target->setLayer(graphic->getActiveLayer()); graphic->addEntity(target); native.entity.release();
        auto limit = std::make_unique<RS_Line>(nullptr, RS_Vector{1, -5}, RS_Vector{1, 5});
        auto inputs = LC_OffsetCreationSources::captureDetachedLimit(*graphic, *target, *limit); REQUIRE(inputs);
        workerTarget = inputs->cloneWorkerSource(0); workerLimit = inputs->cloneWorkerSource(1);
        REQUIRE(workerTarget); REQUIRE(workerLimit);
        inputs.reset(); limit.reset(); graphic.reset();
    }
    auto* native = dynamic_cast<LC_OffsetCurve*>(workerTarget.get()); REQUIRE(native);
    CHECK(native->hasEditableDefinition()); CHECK(native->getParent() == nullptr);
    CHECK(native->getLayer(false) == nullptr);
    for (auto* child : *native) CHECK(child->getLayer(false) == nullptr);
    auto* line = dynamic_cast<RS_Line*>(workerLimit.get()); REQUIRE(line);
    CHECK(line->getData().startpoint == RS_Vector(1, -5)); CHECK(line->getLayer(false) == nullptr);
}

TEST_CASE("Trim nested limit snapshots seal their live child before worker copying",
          "[lc-offsetcurve][creation-sources][trim-sources][nested]") {
    Fixture f; auto* target = f.line({0, -2}, {0, 2}); auto* polyline = f.polyline();
    polyline->setLayer(f.graphic.getActiveLayer());
    auto* limit = polyline->entityAt(0); REQUIRE(limit);
    auto inputs = LC_OffsetCreationSources::captureNestedLimit(f.graphic, *target, *limit); REQUIRE(inputs);
    CHECK_FALSE(inputs->matches(f.graphic)); CHECK_FALSE(inputs->matchesDetached(f.graphic, limit));
    CHECK(inputs->matchesNested(f.graphic, limit));
    auto copy = inputs->cloneWorkerSource(1); REQUIRE(copy);
    CHECK(copy->getParent() == nullptr); CHECK(copy->getLayer(false) == nullptr);
    SECTION("child geometry") { limit->move({0, 1}); }
    SECTION("owner visibility") { polyline->setVisible(false); }
    SECTION("inherited layer") { f.graphic.getActiveLayer()->freeze(true); }
    CHECK_FALSE(inputs->matchesNested(f.graphic, limit));
    CHECK(dynamic_cast<RS_Line*>(copy.get())->getData().startpoint == RS_Vector(0, 0));
    CHECK_FALSE(LC_OffsetCreationSources::captureNestedLimit(f.graphic, *target, *target));
}

TEST_CASE("Trim nested limit snapshot refuses overridden geometry types",
          "[lc-offsetcurve][creation-sources][trim-sources][nested]") {
    class CustomLine final : public RS_Line {
    public:
        explicit CustomLine(RS_EntityContainer* parent) : RS_Line(parent, {0, 0}, {2, 0}) {}
        RS_Vector getStartpoint() const override { return {5, 5}; }
    };
    Fixture f; auto* target = f.line({0, -2}, {0, 2});
    auto* owner = new RS_EntityContainer(&f.graphic, true); f.graphic.addEntity(owner);
    auto* nested = new CustomLine(owner); owner->addEntity(nested);
    CHECK_FALSE(LC_OffsetCreationSources::captureNestedLimit(f.graphic, *target, *nested));
}

TEST_CASE("Offset creation publication resolves a larger document inventory without changing unrelated members",
          "[lc-offsetcurve][creation-publication][inventory]") {
    Fixture f; QList<RS_Entity*> sources;
    for (int i = 0; i < 40; ++i) sources.append(f.line({double(i), 0}, {double(i), 2}));
    auto added = f.added(); auto* address = added.get();
    auto publication = LC_OffsetCurvePublication::prepareCreation(f.graphic, f.viewport,
        {sources[35]}, {address}, {}, {sources[35]}, {sources[35]});
    REQUIRE(publication); REQUIRE(publication->commit()); added.release(); publication->notify();
    CHECK(f.graphic.count() == 41); CHECK(sources[35]->isDeleted());
    for (int i = 0; i < sources.size(); ++i) if (i != 35) CHECK_FALSE(sources[i]->isDeleted());
    REQUIRE(f.graphic.undo()); CHECK_FALSE(sources[35]->isDeleted()); CHECK(address->isDeleted());
}

TEST_CASE("Offset creation source metadata shares a bounded clone allowance across sources",
          "[lc-offsetcurve][creation-sources][quota]") {
    Fixture f; QList<RS_Entity*> sources;
    for (int i = 0; i < 6; ++i) {
        auto* line = f.line({double(i), 0}, {double(i), 2});
        line->setDrwExtData({std::make_shared<DRW_Variant>(1000,
            std::string(LC_OffsetCurveEditSnapshot::MaximumSnapshotBytes, 'a'))});
        sources.append(line);
    }
    REQUIRE(LC_OffsetCreationSources::capture(f.graphic, {sources[0]}));
    CHECK_FALSE(LC_OffsetCreationSources::capture(f.graphic, sources));
    CHECK(f.graphic.count() == 6); CHECK(f.graphic.countUndoCycles() == 0);
}
