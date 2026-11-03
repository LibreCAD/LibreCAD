/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2026 LibreCAD.org
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
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**
****************************************************************************/

// A spline and a leader rebuild the lines they are drawn with on every
// update(), whatever layer and pen are current then. The lines keep no layer
// of their own: they follow their owner's, and deleting the layer that was
// current leaves no pointer to it.

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include "lc_actiontestsupport.h"
#include "rs_block.h"
#include "rs_insert.h"
#include "rs_layer.h"
#include "rs_leader.h"
#include "rs_modification.h"
#include "rs_spline.h"

namespace {

const RS_Vector kPoints[] = {{0, 0}, {10, 20}, {20, -20}, {30, 0}};

// A drawing with the layers "own" and "other", "other" being current.
struct LayersFixture {
    const bool m_qtReady{lc::test::application() != nullptr};
    RS_Graphic m_graphic;
    lc::test::TestGraphicView m_view;
    RS_Layer* m_own{new RS_Layer("own")};
    RS_Layer* m_other{new RS_Layer("other")};

    LayersFixture() {
        m_graphic.initForNewDocument();
        m_view.setDocument(&m_graphic);
        m_own->setPen(RS_Pen(RS_Color(255, 0, 0), RS2::Width01, RS2::SolidLine));
        m_other->setPen(RS_Pen(RS_Color(0, 0, 255), RS2::Width05, RS2::DashLine));
        m_graphic.addLayer(m_own);
        m_graphic.addLayer(m_other);
        m_graphic.activateLayer(m_other);
    }

    // A spline or a leader on layer "own", with its lines built while "other" is current.
    RS_EntityContainer* draw(const bool leader, RS_EntityContainer* parent) const {
        RS_EntityContainer* drawn = nullptr;
        if (leader) {
            auto* entity = new RS_Leader(parent, RS_LeaderData(true, ""));
            for (const RS_Vector& point : kPoints) {
                entity->addVertex(point);
            }
            drawn = entity;
        }
        else {
            auto* entity = new RS_Spline(parent, RS_SplineData(3, false));
            for (const RS_Vector& point : kPoints) {
                entity->addControlPoint(point);
            }
            drawn = entity;
        }
        drawn->setLayer(m_own);
        drawn->update();
        parent->addEntity(drawn);
        return drawn;
    }

    RS_Block* addBlock() {
        auto* block = new RS_Block(&m_graphic, RS_BlockData("block", {0, 0}, false));
        REQUIRE(m_graphic.addBlock(block));
        return block;
    }

    // what an operation left in the drawing
    RS_EntityContainer* lastLive() {
        RS_EntityContainer* live = nullptr;
        for (RS_Entity* entity : m_graphic) {
            if (!entity->isDeleted()) {
                live = static_cast<RS_EntityContainer*>(entity);
            }
        }
        return live;
    }

    // keeping its layer, as a drag with the mouse does
    RS_EntityContainer* move(RS_EntityContainer* entity) {
        RS_MoveData data;
        data.offset = RS_Vector(5.0, 5.0);
        m_graphic.undoableModify(m_view.getViewPort(), [&](LC_DocumentModificationBatch& ctx) -> bool {
            RS_Modification::move(data, QList<RS_Entity*>{entity}, false, ctx);
            return true;
        });
        return lastLive();
    }

    void explode(RS_Entity* entity) {
        m_graphic.undoableModify(m_view.getViewPort(), [&](LC_DocumentModificationBatch& ctx) -> bool {
            ctx.setActiveLayerAndPen(false, false);
            return RS_Modification::explode(QList<RS_Entity*>{entity}, ctx);
        });
    }
};

} // namespace

TEST_CASE("The lines of a spline and of a leader follow its layer, not the current one", "[spline][leader][layer]") {
    const bool leader = GENERATE(false, true);
    CAPTURE(leader);
    LayersFixture f;
    RS_EntityContainer* moved = f.move(f.draw(leader, &f.m_graphic));
    REQUIRE(moved != nullptr);
    REQUIRE(moved->getLayer(false) == f.m_own);
    REQUIRE(moved->count() >= 3);

    for (const RS_Entity* part : *moved) {
        REQUIRE(part->getLayer(false) == nullptr);
        CHECK(part->getLayer(true) == f.m_own);
        CHECK(part->getPen(true) == moved->getPen(true));
    }

    // so the other layer neither hides them nor makes construction lines of them
    f.m_other->freeze(true);
    f.m_other->setConstruction(true);
    for (const RS_Entity* part : *moved) {
        CHECK(part->isVisible());
        CHECK_FALSE(part->isConstruction());
    }

    // and their own layer does: a spline's lines are its geometry, as a
    // polyline's segments are; the parts of a leader are not
    f.m_own->setConstruction(true);
    for (const RS_Entity* part : *moved) {
        CHECK(part->isConstruction() == !leader);
    }
    f.m_own->setConstruction(false);

    // Deleting the other layer leaves the entity, and no pointer to that layer.
    f.m_graphic.removeLayer(f.m_other);
    REQUIRE_FALSE(moved->isDeleted());
    for (const RS_Entity* part : *moved) {
        CHECK(part->isVisible());
        CHECK_FALSE(part->isConstruction());
        CHECK(part->isPrint());
    }
}

// The lines have no pen of their own to keep, so each takes the spline's.
TEST_CASE("Exploding a spline gives lines on its layer, with its pen", "[spline][layer]") {
    LayersFixture f;
    RS_EntityContainer* moved = f.move(f.draw(false, &f.m_graphic));
    REQUIRE(moved != nullptr);
    const RS_Pen pen{RS_Color(0, 255, 0), RS2::Width03, RS2::DotLine};
    moved->setPen(pen); // as Modify > Attributes does

    f.explode(moved);

    int lines = 0;
    for (const RS_Entity* entity : f.m_graphic) {
        if (entity->isDeleted()) {
            continue;
        }
        ++lines;
        CHECK(entity->rtti() == RS2::EntityLine);
        CHECK(entity->getLayer(false) == f.m_own);
        CHECK(entity->getPen(false) == pen);
    }
    CHECK(lines >= 3);
}

// A spline that a block reference hands to the drawing goes on following its
// layer: Modify > Attributes changes the spline and leaves its lines as they are.
TEST_CASE("Exploding a block reference leaves the lines of its spline without a layer or a pen", "[spline][layer][block]") {
    LayersFixture f;
    f.draw(false, f.addBlock());
    auto* insert = new RS_Insert(&f.m_graphic, RS_InsertData("block", {0, 0}, {1, 1}, 0.0, 1, 1, {0, 0}));
    f.m_graphic.addEntity(insert);
    insert->update();

    f.explode(insert);

    const RS_EntityContainer* spline = f.lastLive();
    REQUIRE(spline != nullptr);
    REQUIRE(spline->rtti() == RS2::EntitySpline);
    REQUIRE(spline->count() >= 3);
    for (const RS_Entity* part : *spline) {
        CHECK(part->getLayer(false) == nullptr);
        CHECK_FALSE(part->getPen(false).isValid());
    }
}

// The block editor shows the members of a block on their own, in the block's
// pen, which is two levels above the lines of a spline or a leader.
TEST_CASE("The lines of a ByBlock spline or leader in a block take the pen of the block", "[spline][leader][layer][block]") {
    const bool leader = GENERATE(false, true);
    CAPTURE(leader);
    LayersFixture f;
    RS_Block* block = f.addBlock();
    RS_EntityContainer* member = f.draw(leader, block);
    member->setPen(RS_Pen(RS_Color(RS2::FlagByBlock), RS2::WidthByBlock, RS2::LineByBlock));
    member->update();

    const RS_Pen blockPen = block->getPen(false);
    REQUIRE(blockPen != f.m_other->getPen());
    CHECK(member->getPen(true) == blockPen);
    REQUIRE(member->count() >= 3);
    for (const RS_Entity* part : *member) {
        CHECK(part->getPen(true) == blockPen);
    }
}
