/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2026 LibreCAD (librecad.org)
** Copyright (C) 2026 Dongxu Li (github.com/dxli)
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
**********************************************************************/

// RS_EntityContainer::updateDimensions() rebuilt a leader with RS_Leader::update() and then, a
// leader being one of the dimensional entities, cleared it again in RS_Dimension::updateDim(),
// which leaves the rebuilding to RS_Leader::doUpdateDim(), and that draws nothing. Every drawing
// that is opened goes through it (RS_Graphic::onLoadingCompleted()), so no leader was painted,
// not even one in a block reference, and a save dropped every leader outside the blocks: the
// writer takes a leader's vertices from its lines.

#include <catch2/catch_test_macros.hpp>

#include <vector>

#include "lc_actiontestsupport.h"
#include "lc_filtertestsupport.h"
#include "rs_block.h"
#include "rs_insert.h"
#include "rs_leader.h"

namespace {

const RS_Vector kVertices[] = {{0.0, 0.0}, {10.0, 10.0}, {20.0, 10.0}};

/// A leader with an arrow, built as the DXF and DWG import build one.
RS_Leader* addLeader(RS_EntityContainer& owner) {
    auto* leader = new RS_Leader(&owner, RS_LeaderData(true, QString()));
    for (const RS_Vector& v : kVertices) {
        leader->addVertex(v);
    }
    leader->update();
    owner.addEntity(leader);
    return leader;
}

/// The arrow, then a line along each of the two segments, \p offset away from kVertices.
void checkDrawn(const RS_Leader& leader, const RS_Vector& offset) {
    REQUIRE(leader.count() == 3);
    CHECK(leader.entityAt(0)->rtti() == RS2::EntitySolid);
    for (int segment = 1; segment <= 2; ++segment) {
        const RS_Entity* line = leader.entityAt(segment);
        REQUIRE(line->rtti() == RS2::EntityLine);
        CHECK(line->getStartpoint().distanceTo(kVertices[segment - 1] + offset) < 1e-9);
        CHECK(line->getEndpoint().distanceTo(kVertices[segment] + offset) < 1e-9);
    }
}

const RS_Leader& onlyLeader(const RS_Insert& insert) {
    REQUIRE(insert.count() == 1);
    REQUIRE(insert.entityAt(0)->rtti() == RS2::EntityDimLeader);
    return *static_cast<const RS_Leader*>(insert.entityAt(0));
}

} // namespace

TEST_CASE("Updating the dimensions keeps the leaders drawn, also in a block reference",
          "[dimension][leader]") {
    lc::test::application();
    RS_Graphic graphic;
    graphic.initForNewDocument();
    const RS_Leader* leader = addLeader(graphic);
    auto* block = new RS_Block(&graphic, RS_BlockData(QStringLiteral("CALLOUT"), RS_Vector(0.0, 0.0), false));
    addLeader(*block);
    graphic.addBlock(block);
    const RS_Vector insertionPoint{100.0, 0.0};
    auto* insert = new RS_Insert(&graphic, RS_InsertData(QStringLiteral("CALLOUT"), insertionPoint,
                                                         RS_Vector(1.0, 1.0), 0.0, 1, 1, RS_Vector(0.0, 0.0)));
    graphic.addEntity(insert);

    // as for every drawing that is opened
    graphic.onLoadingCompleted();
    checkDrawn(*leader, RS_Vector(0.0, 0.0));
    checkDrawn(onlyLeader(*insert), insertionPoint);

    // as Regenerate Dimensions does, which reports the count: each leader is updated once
    CHECK(graphic.updateDimensions(false) == 2);
    checkDrawn(*leader, RS_Vector(0.0, 0.0));
    checkDrawn(onlyLeader(*insert), insertionPoint);
}

TEST_CASE("A drawing that is opened and saved keeps its leader", "[dimension][leader][dxf]") {
    lc::test::application();
    const lc::test::QuietLog quietLog;
    const lc::test::OutputDirectory directory("lc_updatedimensions_tests");
    const auto drawn = directory.file("drawn.dxf");
    const auto saved = directory.file("saved.dxf");
    {
        RS_Graphic graphic;
        graphic.initForNewDocument();
        addLeader(graphic);
        REQUIRE(lc::test::exportDrawing(graphic, drawn, RS2::FormatDXFRW));
    }

    // what opening a file does (LC_DocumentsStorage::loadGraphic()), then a save
    RS_Graphic graphic;
    REQUIRE(lc::test::importDrawing(graphic, drawn, RS2::FormatDXFRW));
    graphic.onLoadingCompleted();
    REQUIRE(lc::test::exportDrawing(graphic, saved, RS2::FormatDXFRW));

    const std::vector<lc::test::DxfRecord> leaders = lc::test::dxfRecords(saved, "LEADER");
    REQUIRE(leaders.size() == 1);
    CHECK(lc::test::sameValues({"0", "10", "20"}, leaders.front().own(10)));
    CHECK(lc::test::sameValues({"0", "10", "10"}, leaders.front().own(20)));
}
