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

// "Save Block" writes the active block as a drawing of its own. The action
// builds a temporary RS_Graphic for that, which is deleted as soon as the file
// is written. It used to move the block's own entities into it, which
// re-parented them: once the temporary drawing was gone, every entity of the
// block still in the source drawing pointed at freed memory (isDeleted() and
// getGraphic() walk the parent chain). The temporary drawing now holds copies,
// so saving leaves the source block as it was.

#include <memory>

#include <catch2/catch_test_macros.hpp>

#include <QTemporaryDir>

#include "lc_actiontestsupport.h"
#include "lc_documentsstorage.h"
#include "rs_actionblockssave.h"
#include "rs_block.h"
#include "rs_blocklist.h"
#include "rs_circle.h"
#include "rs_fileio.h"
#include "rs_graphic.h"
#include "rs_insert.h"
#include "rs_layer.h"
#include "rs_layerlist.h"
#include "rs_line.h"

namespace {

// A drawing with the blocks OUTER (a line, an insert of MID and an insert of
// MID again), MID (a circle and an insert of INNER) and INNER (a line), and a
// top-level insert of OUTER.
struct Drawing {
    const bool m_qtReady{lc::test::application() != nullptr};
    RS_Graphic m_graphic;
    RS_Block* m_outer = nullptr;
    RS_Block* m_mid = nullptr;
    RS_Block* m_inner = nullptr;
    RS_Line* m_outerLine = nullptr;
    RS_Insert* m_outerInsert = nullptr;
    RS_Insert* m_midInsert = nullptr;

    Drawing() {
        m_graphic.initForNewDocument();

        m_inner = addBlock("INNER");
        m_inner->addEntity(new RS_Line(m_inner, RS_Vector(0.0, 0.0), RS_Vector(1.0, 1.0)));

        m_mid = addBlock("MID");
        m_mid->addEntity(new RS_Circle(m_mid, RS_CircleData(RS_Vector(0.0, 0.0), 2.0)));
        m_mid->addEntity(makeInsert(m_mid, "INNER"));

        m_outer = addBlock("OUTER");
        m_outerLine = new RS_Line(m_outer, RS_Vector(0.0, 0.0), RS_Vector(5.0, 0.0));
        m_outer->addEntity(m_outerLine);
        m_midInsert = makeInsert(m_outer, "MID");
        m_outer->addEntity(m_midInsert);
        m_outer->addEntity(makeInsert(m_outer, "MID"));

        m_outerInsert = makeInsert(&m_graphic, "OUTER");
        m_graphic.addEntity(m_outerInsert);
        m_graphic.updateInserts();
    }

    RS_Block* addBlock(const QString& name) {
        auto* block = new RS_Block(&m_graphic, RS_BlockData(name, RS_Vector(0.0, 0.0), false));
        m_graphic.addBlock(block);
        return block;
    }

    static RS_Insert* makeInsert(RS_EntityContainer* parent, const QString& blockName) {
        return new RS_Insert(parent, RS_InsertData(blockName, RS_Vector(0.0, 0.0), RS_Vector(1.0, 1.0), 0.0, 1, 1,
                                                   RS_Vector(0.0, 0.0)));
    }
};

unsigned countOfType(RS_EntityContainer& container, const RS2::EntityType type) {
    unsigned n = 0;
    for (const RS_Entity* e : container) {
        n += e != nullptr && !e->isDeleted() && e->rtti() == type ? 1 : 0;
    }
    return n;
}

} // namespace

TEST_CASE("Save Block leaves the source block's entities where they were", "[block][save][ownership]") {
    Drawing source;
    const unsigned outerCount = source.m_outer->count();
    REQUIRE(outerCount == 3);

    {
        const std::unique_ptr<RS_Graphic> temporary = RS_ActionBlocksSave::createGraphicForBlock(source.m_outer);
        REQUIRE(temporary != nullptr);
        CHECK(temporary->count() == outerCount);
        for (const RS_Entity* copy : *temporary) {
            CHECK(copy->getParent() == temporary.get());
        }
        // The temporary drawing holds copies, not the block's own entities.
        for (const RS_Entity* original : *source.m_outer) {
            CHECK(original->getParent() == source.m_outer);
            for (const RS_Entity* copy : *temporary) {
                CHECK(copy != original);
            }
        }
    }

    // The temporary drawing is gone. Everything below walks the parent chain,
    // which reached freed memory before.
    CHECK(source.m_outer->count() == outerCount);
    for (const RS_Entity* original : *source.m_outer) {
        CHECK(original->getParent() == source.m_outer);
        CHECK(original->getGraphic() == &source.m_graphic);
        CHECK_FALSE(original->isDeleted());
    }
    CHECK(source.m_midInsert->getBlockForInsert() == source.m_mid);
    CHECK(source.m_graphic.countBlocks() == 3);
    CHECK(source.m_graphic.findBlock("MID") == source.m_mid);
}

TEST_CASE("Save Block writes the block's entities and the blocks they insert", "[block][save]") {
    Drawing source;
    QTemporaryDir dir;
    REQUIRE(dir.isValid());
    const QString path = dir.filePath("outer.dxf");

    {
        const std::unique_ptr<RS_Graphic> temporary = RS_ActionBlocksSave::createGraphicForBlock(source.m_outer);
        temporary->setModified(true);
        LC_DocumentsStorage storage;
        REQUIRE(storage.saveBlockAs(temporary.get(), path));
    }

    RS_Graphic saved;
    saved.initForNewDocument();
    REQUIRE(RS_FileIO::instance()->fileImport(saved, path, RS2::FormatDXFRW));
    saved.updateInserts();

    // The block's own entities are the saved drawing's content.
    CHECK(countOfType(saved, RS2::EntityLine) == 1);
    CHECK(countOfType(saved, RS2::EntityInsert) == 2);

    // Every block the inserts reach is saved with it, and resolves.
    CHECK(saved.findBlock("MID") != nullptr);
    CHECK(saved.findBlock("INNER") != nullptr);
    CHECK(saved.findBlock("OUTER") == nullptr);
    for (RS_Entity* e : saved) {
        if (e->rtti() == RS2::EntityInsert) {
            const auto* insert = static_cast<RS_Insert*>(e);
            CHECK(insert->getBlockForInsert() == saved.findBlock("MID"));
        }
    }
    RS_Block* mid = saved.findBlock("MID");
    REQUIRE(mid != nullptr);
    CHECK(countOfType(*mid, RS2::EntityCircle) == 1);
    REQUIRE(countOfType(*mid, RS2::EntityInsert) == 1);
    for (RS_Entity* e : *mid) {
        if (e->rtti() == RS2::EntityInsert) {
            CHECK(static_cast<RS_Insert*>(e)->getBlockForInsert() == saved.findBlock("INNER"));
        }
    }
}

TEST_CASE("Save Block skips entities deleted in the block editor", "[block][save]") {
    Drawing source;
    // The block editor keeps a deleted entity in the block's list as undo history.
    source.m_outerLine->setFlag(RS2::FlagDeleted);

    const std::unique_ptr<RS_Graphic> temporary = RS_ActionBlocksSave::createGraphicForBlock(source.m_outer);
    CHECK(countOfType(*temporary, RS2::EntityLine) == 0);
    CHECK(countOfType(*temporary, RS2::EntityInsert) == 2);

    // The deleted entity is still the block's, untouched.
    CHECK(source.m_outerLine->getParent() == source.m_outer);
    CHECK(source.m_outerLine->getFlag(RS2::FlagDeleted));
}

TEST_CASE("Saving a block does not change the blocks the source drawing holds", "[block][save][ownership]") {
    auto source = std::make_unique<Drawing>();
    {
        const std::unique_ptr<RS_Graphic> temporary = RS_ActionBlocksSave::createGraphicForBlock(source->m_outer);
        // The temporary drawing only lists the blocks the source owns.
        CHECK_FALSE(temporary->getBlockList()->isOwner());
        CHECK(temporary->countBlocks() == 2);
    }
    CHECK(source->m_graphic.countBlocks() == 3);
    CHECK(source->m_mid->count() == 2);
    CHECK(source->m_inner->count() == 1);

    // The source drawing frees its blocks, with the save long done.
    source.reset();
}

// Blocks that insert each other cannot be expanded, but the block names still
// resolve, so saving must list each block once and finish.
TEST_CASE("Save Block finishes when blocks insert each other", "[block][save]") {
    Drawing source;
    RS_Block* ping = source.addBlock("PING");
    RS_Block* pong = source.addBlock("PONG");
    ping->addEntity(Drawing::makeInsert(ping, "PONG"));
    pong->addEntity(Drawing::makeInsert(pong, "PING"));

    const std::unique_ptr<RS_Graphic> temporary = RS_ActionBlocksSave::createGraphicForBlock(ping);
    CHECK(temporary->count() == 1);
    CHECK(temporary->countBlocks() == 2);
    CHECK(temporary->findBlock("PING") == ping);
    CHECK(temporary->findBlock("PONG") == pong);
}

// The copies used to keep naming the source drawing's layers, which the
// temporary drawing does not have: getLayer() resolves a layer of another
// drawing to none, so every entity was written on layer "0", and the file had
// no layer table of its own (the old fixme). Nested blocks were not copied at
// all, so their entities fared the same.
TEST_CASE("Save Block writes the layers the block's entities are on", "[block][save][layers]") {
    Drawing source;
    auto addLayer = [&](const QString& name, const RS_Color& color) {
        auto* layer = new RS_Layer(name);
        layer->setPen(RS_Pen(color, RS2::Width05, RS2::DashLine));
        source.m_graphic.addLayer(layer);
        return source.m_graphic.findLayer(name);
    };
    RS_Layer* walls = addLayer("WALLS", RS_Color(255, 0, 0));
    RS_Layer* doors = addLayer("DOORS", RS_Color(0, 0, 255));
    source.m_outerLine->setLayer(walls);
    for (RS_Entity* e : *source.m_mid) {
        if (e->rtti() == RS2::EntityCircle) {
            e->setLayer(doors);
        }
    }
    // A layer nothing in the saved block is on stays out of the file.
    addLayer("UNUSED", RS_Color(0, 255, 0));

    QTemporaryDir dir;
    REQUIRE(dir.isValid());
    const QString path = dir.filePath("outer.dxf");
    {
        const std::unique_ptr<RS_Graphic> temporary = RS_ActionBlocksSave::createGraphicForBlock(source.m_outer);
        temporary->setModified(true);
        LC_DocumentsStorage storage;
        REQUIRE(storage.saveBlockAs(temporary.get(), path));
    }

    RS_Graphic saved;
    saved.initForNewDocument();
    REQUIRE(RS_FileIO::instance()->fileImport(saved, path, RS2::FormatDXFRW));

    RS_Layer* savedWalls = saved.findLayer("WALLS");
    REQUIRE(savedWalls != nullptr);
    CHECK(savedWalls->getPen().getColor() == RS_Color(255, 0, 0));
    RS_Layer* savedDoors = saved.findLayer("DOORS");
    REQUIRE(savedDoors != nullptr);
    CHECK(savedDoors->getPen().getColor() == RS_Color(0, 0, 255));
    CHECK(saved.findLayer("UNUSED") == nullptr);

    unsigned onWalls = 0;
    for (RS_Entity* e : saved) {
        if (e->rtti() == RS2::EntityLine) {
            CHECK(e->getLayer() == savedWalls);
            ++onWalls;
        }
    }
    CHECK(onWalls == 1);

    RS_Block* mid = saved.findBlock("MID");
    REQUIRE(mid != nullptr);
    unsigned onDoors = 0;
    for (RS_Entity* e : *mid) {
        if (e->rtti() == RS2::EntityCircle) {
            CHECK(e->getLayer() == savedDoors);
            ++onDoors;
        }
    }
    CHECK(onDoors == 1);

    // The source drawing is untouched.
    CHECK(source.m_outerLine->getLayer() == walls);
    CHECK(source.m_graphic.findLayer("WALLS") == walls);
}
