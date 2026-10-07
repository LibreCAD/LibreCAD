/****************************************************************************
** This file is part of the LibreCAD project, a 2D CAD program
** Copyright (C) 2026 librecad.org
** Copyright (C) 2026 Dongxu Li (github.com/dxli)
**
** This program is free software; you can redistribute it and/or modify it
** under the terms of the GNU General Public License as published by the
** Free Software Foundation; either version 2, or any later version.
****************************************************************************/

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <array>
#include <memory>
#include <utility>

#include "lc_actiontestsupport.h"
#include "lc_looputils.h"
#include "lc_settingguard.h"
#include "rs_block.h"
#include "rs_hatch.h"
#include "rs_insert.h"
#include "rs_layer.h"
#include "rs_line.h"
#include "rs_modification.h"
#include "rs_polyline.h"
#include "rs_preview.h"

namespace {

void rectangle(RS_EntityContainer& loop, const RS_Vector& min,
               const RS_Vector& max, const bool clockwise = false) {
    std::array<RS_Vector, 4> points{min, {max.x, min.y}, max, {min.x, max.y}};
    if (clockwise) {
        std::swap(points[1], points[3]);
    }
    for (size_t i = 0; i < points.size(); ++i) {
        loop.addEntity(new RS_Line(&loop, points[i], points[(i + 1) % points.size()]));
    }
}

void checkOwnedEdges(const RS_EntityContainer& loop) {
    REQUIRE(loop.count() == 4);
    for (const RS_Entity* edge : loop) {
        // Check ownership before following the parent in isDeleted().
        REQUIRE(edge->getParent() == &loop);
        CHECK(edge->getLayer(false) == nullptr);
        CHECK_FALSE(edge->isDeleted());
    }
}

void checkHierarchy(const LC_LoopUtils::LC_Loops& loops) {
    checkOwnedEdges(*loops.getOuterLoop());
    for (const auto& child : loops.getChildren()) {
        checkHierarchy(child);
    }
}

std::unique_ptr<RS_Hatch> makeHatch(RS_EntityContainer* parent, const bool solid) {
    auto hatch = std::make_unique<RS_Hatch>(parent, RS_HatchData(solid, 1.0, 0.0, solid ? "SOLID" : "ANSI31"));
    auto* outer = new RS_EntityContainer(hatch.get());
    hatch->addEntity(outer);
    rectangle(*outer, {0, 0}, {20, 10});
    auto* hole = new RS_EntityContainer(hatch.get());
    hatch->addEntity(hole);
    rectangle(*hole, {8, 4}, {12, 6}, true);
    hatch->update();
    return hatch;
}

// A hatch as Draw Hatch builds it: one loop of clones of the picked entities.
std::unique_ptr<RS_Hatch> hatchOver(RS_Graphic& graphic, const QList<RS_Entity*>& picked, const bool solid) {
    auto hatch = std::make_unique<RS_Hatch>(&graphic, RS_HatchData(solid, 1.0, 0.0, solid ? "SOLID" : "ANSI31"));
    auto* loop = new RS_EntityContainer(hatch.get());
    hatch->addEntity(loop);
    for (const RS_Entity* entity : picked) {
        RS_Entity* clone = entity->clone();
        clone->reparent(loop);
        loop->addEntity(clone);
    }
    hatch->update();
    return hatch;
}

void checkBounds(const RS_Entity& entity, const RS_Vector& offset) {
    CHECK_THAT(entity.getMin().x, Catch::Matchers::WithinAbs(offset.x, 1e-8));
    CHECK_THAT(entity.getMin().y, Catch::Matchers::WithinAbs(offset.y, 1e-8));
    CHECK_THAT(entity.getMax().x, Catch::Matchers::WithinAbs(offset.x + 20, 1e-8));
    CHECK_THAT(entity.getMax().y, Catch::Matchers::WithinAbs(offset.y + 10, 1e-8));
}

void checkHatch(RS_Hatch& hatch, const RS_Vector& offset) {
    REQUIRE(hatch.getUpdateError() == RS_Hatch::HATCH_OK);
    REQUIRE(hatch.countAllLoops() == 2);
    for (int i = 0; i < hatch.countAllLoops(); ++i) {
        checkOwnedEdges(*hatch.getBoundaryContainer(i));
    }
    CHECK_THAT(hatch.getTotalArea(), Catch::Matchers::WithinAbs(192.0, 1e-8));
    checkBounds(hatch, offset);
    hatch.activateContour(true);
    for (int i = 0; i < hatch.countAllLoops(); ++i) {
        for (const RS_Entity* edge : *hatch.getBoundaryContainer(i)) {
            CHECK(edge->isVisible());
        }
    }
    hatch.activateContour(false);
}

void checkCopies(const QList<RS_Entity*>& entities, const RS_MoveData& data, const bool withRectangle) {
    const int copies = data.obtainNumberOfCopies();
    REQUIRE(entities.size() == copies * (withRectangle ? 2 : 1));
    for (int i = 0; i < copies; ++i) {
        REQUIRE(entities[i]->rtti() == RS2::EntityHatch);
        checkHatch(*static_cast<RS_Hatch*>(entities[i]), data.offset * (i + 1));
        if (withRectangle) {
            REQUIRE(entities[copies + i]->rtti() == RS2::EntityPolyline);
            checkBounds(*entities[copies + i], data.offset * (i + 1));
        }
    }
}

} // namespace

TEST_CASE("Extracted loops own their edges after the source and its layer are destroyed", "[hatch-move][loop-ownership]") {
    const bool clockwise = GENERATE(false, true);
    auto layer = std::make_unique<RS_Layer>("contour");
    auto source = std::make_unique<RS_EntityContainer>();
    rectangle(*source, {0, 0}, {20, 10}, clockwise);
    for (RS_Entity* edge : *source) {
        edge->setLayer(layer.get());
    }
    LC_LoopUtils::LoopExtractor extractor{*source};
    const auto extracted = extractor.extract();
    REQUIRE(extracted.size() == 1);
    source.reset();
    layer.reset();
    checkOwnedEdges(*extracted.front());
    CHECK_THAT(extracted.front()->areaLineIntegral(), Catch::Matchers::WithinAbs(200.0, 1e-8));
}

TEST_CASE("Sorted loop hierarchies outlive their input containers", "[hatch-move][loop-ownership]") {
    std::shared_ptr<std::vector<LC_LoopUtils::LC_Loops>> results;
    {
        std::vector<std::unique_ptr<RS_EntityContainer>> inputs;
        for (const auto& corners : std::array<std::array<RS_Vector, 2>, 3>{
                 std::array<RS_Vector, 2>{RS_Vector{0, 0}, RS_Vector{20, 10}},
                 std::array<RS_Vector, 2>{RS_Vector{6, 3}, RS_Vector{14, 7}},
                 std::array<RS_Vector, 2>{RS_Vector{9, 4}, RS_Vector{11, 6}}}) {
            auto loop = std::make_unique<RS_EntityContainer>();
            rectangle(*loop, corners[0], corners[1]);
            inputs.push_back(std::move(loop));
        }
        LC_LoopUtils::LoopSorter sorter{std::move(inputs)};
        results = sorter.getResults();
    }
    REQUIRE(results->size() == 1);
    const auto& root = results->front();
    REQUIRE(root.getChildren().size() == 1);
    REQUIRE(root.getChildren().front().getChildren().size() == 1);
    checkHierarchy(root);
    CHECK_THAT(root.getTotalArea(), Catch::Matchers::WithinAbs(172.0, 1e-8));
}

TEST_CASE("Hatch clones retain independent boundaries across source deletion and edits", "[hatch-move]") {
    lc::test::application();
    lc::test::SettingGuard patterns{RS_SETTINGS, "Paths", "Patterns"};
    patterns.set(QStringLiteral(LIBRECAD_SOURCE_DIR "/librecad/support/patterns"));
    const bool solid = GENERATE(false, true);
    RS_Graphic graphic;
    graphic.initForNewDocument();
    auto source = makeHatch(&graphic, solid);
    checkHatch(*source, {0, 0});
    auto clone = std::unique_ptr<RS_Hatch>(static_cast<RS_Hatch*>(source->clone()));
    source->markDeleted();
    checkHatch(*clone, {0, 0});
    source.reset();
    clone->update();
    clone->move({7, -3});
    checkHatch(*clone, {7, -3});
    clone->rotate({17, 2}, M_PI);
    checkHatch(*clone, {7, -3});
    clone->scale({7, -3}, {2, 2});
    CHECK_THAT(clone->getTotalArea(), Catch::Matchers::WithinAbs(768.0, 1e-8));
    clone->scale({7, -3}, {0.5, 0.5});
    clone->mirror({17, -3}, {17, 7});
    checkHatch(*clone, {7, -3});
}

TEST_CASE("Hatch Move Copy supports repeated previews and undo redo", "[hatch-move]") {
    lc::test::application();
    lc::test::SettingGuard patterns{RS_SETTINGS, "Paths", "Patterns"};
    patterns.set(QStringLiteral(LIBRECAD_SOURCE_DIR "/librecad/support/patterns"));
    lc::test::SettingGuard previewLimit{RS_SETTINGS, "Appearance", "MaxPreview"};
    previewLimit.set(100);
    const bool solid = GENERATE(false, true);
    const bool withRectangle = GENERATE(false, true);
    const bool keepOriginals = GENERATE(false, true);
    RS_Graphic graphic;
    graphic.initForNewDocument();
    graphic.onLoadingCompleted();
    lc::test::TestGraphicView view;
    view.setDocument(&graphic);
    auto* hatch = makeHatch(&graphic, solid).release();
    graphic.addEntity(hatch);
    QList<RS_Entity*> selected{hatch};
    if (withRectangle) {
        auto* outline = new RS_Polyline(&graphic);
        outline->addVertex({0, 0});
        outline->addVertex({20, 0});
        outline->addVertex({20, 10});
        outline->addVertex({0, 10});
        outline->setClosed(true, 0.0);
        REQUIRE(outline->count() == 4);
        graphic.addEntity(outline);
        selected << outline;
    }
    graphic.select(selected, true);
    RS_MoveData data;
    data.keepOriginals = keepOriginals;
    data.multipleCopies = keepOriginals;
    data.number = 2;
    RS_Preview preview{&graphic, view.getViewPort()};
    for (const RS_Vector& offset : {RS_Vector{3, 2}, RS_Vector{-5, 1}, RS_Vector{7, -3}}) {
        data.offset = offset;
        LC_DocumentModificationBatch batch;
        RS_Modification::move(data, selected, true, batch);
        REQUIRE(batch.success);
        // Match the action's transfer of ownership even when an assertion fails.
        std::vector<std::unique_ptr<RS_Entity>> copies;
        for (RS_Entity* entity : batch.entitiesToAdd) {
            copies.emplace_back(entity);
        }
        checkCopies(batch.entitiesToAdd, data, withRectangle);
        for (auto& entity : copies) {
            preview.addEntity(entity.release());
        }
        REQUIRE(preview.count() > 0);
        preview.clear();
        checkHatch(*hatch, {0, 0});
    }
    QList<RS_Entity*> moved;
    REQUIRE(graphic.undoableModify(view.getViewPort(), [&](LC_DocumentModificationBatch& batch) {
        RS_Modification::move(data, selected, false, batch);
        moved = batch.entitiesToAdd;
        batch.dontSetActiveLayerAndPen();
        return batch.success;
    }));
    checkCopies(moved, data, withRectangle);
    for (RS_Entity* entity : moved) {
        CHECK_FALSE(entity->isDeleted());
    }
    for (RS_Entity* entity : selected) {
        CHECK(entity->isDeleted() == !keepOriginals);
    }
    REQUIRE(graphic.undo());
    for (RS_Entity* entity : moved) {
        CHECK(entity->isDeleted());
    }
    checkHatch(*hatch, {0, 0});
    REQUIRE(graphic.redo());
    checkCopies(moved, data, withRectangle);
}

// Issue #3008. Moving an insert rebuilds its children, so the polyline that the
// cached boundary edges were cloned from is gone before the hatch updates.
TEST_CASE("A hatch moves with a block insert in its contour", "[hatch-move][hatch-lifetime]") {
    lc::test::application();
    lc::test::SettingGuard patterns{RS_SETTINGS, "Paths", "Patterns"};
    patterns.set(QStringLiteral(LIBRECAD_SOURCE_DIR "/librecad/support/patterns"));
    const bool solid = GENERATE(false, true);
    RS_Graphic graphic;
    graphic.initForNewDocument();
    auto* block = new RS_Block(&graphic, RS_BlockData("frame", {0, 0}, false));
    auto* outline = new RS_Polyline(block);
    for (const RS_Vector& corner : {RS_Vector{0, 0}, RS_Vector{20, 0}, RS_Vector{20, 10}, RS_Vector{0, 10}}) {
        outline->addVertex(corner);
    }
    outline->setClosed(true, 0.0);
    block->addEntity(outline);
    REQUIRE(graphic.addBlock(block));
    RS_Insert insert{&graphic, RS_InsertData("frame", {0, 0}, {1, 1}, 0.0, 1, 1, {0, 0})};
    insert.update();
    const auto hatch = hatchOver(graphic, {&insert}, solid);
    REQUIRE(hatch->getUpdateError() == RS_Hatch::HATCH_OK);

    hatch->move({7, -3});

    REQUIRE(hatch->getUpdateError() == RS_Hatch::HATCH_OK);
    CHECK_THAT(hatch->getTotalArea(), Catch::Matchers::WithinAbs(200.0, 1e-8));
    checkBounds(*hatch, {7, -3});
}

// The clones in a contour keep the layer of the entities picked for it, and
// deleting a layer does not reach inside a hatch.
TEST_CASE("A hatch moves after the layer of its contour is deleted", "[hatch-move][hatch-lifetime]") {
    lc::test::application();
    lc::test::SettingGuard patterns{RS_SETTINGS, "Paths", "Patterns"};
    patterns.set(QStringLiteral(LIBRECAD_SOURCE_DIR "/librecad/support/patterns"));
    const bool solid = GENERATE(false, true);
    RS_Graphic graphic;
    graphic.initForNewDocument();
    auto* layer = new RS_Layer("contour");
    graphic.addLayer(layer);
    RS_EntityContainer picked;
    rectangle(picked, {0, 0}, {20, 10});
    QList<RS_Entity*> edges;
    for (RS_Entity* edge : picked) {
        edge->setLayer(layer);
        edges << edge;
    }
    const auto hatch = hatchOver(graphic, edges, solid);
    REQUIRE(hatch->getUpdateError() == RS_Hatch::HATCH_OK);
    graphic.removeLayer(layer);

    hatch->move({7, -3});

    REQUIRE(hatch->getUpdateError() == RS_Hatch::HATCH_OK);
    CHECK_THAT(hatch->getTotalArea(), Catch::Matchers::WithinAbs(200.0, 1e-8));
    checkBounds(*hatch, {7, -3});
}
