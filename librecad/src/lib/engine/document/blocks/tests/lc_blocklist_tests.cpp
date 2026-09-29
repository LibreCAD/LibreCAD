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

// An owning RS_BlockList (every drawing's) frees the blocks it holds when it
// is cleared or destroyed, as remove() and add() (for a rejected duplicate)
// always did. Before, every block of every drawing leaked. A non-owning list
// (RS_ActionBlocksSave's, a font imported as a drawing) frees nothing.

#include <memory>

#include <catch2/catch_test_macros.hpp>

#include "lc_actiontestsupport.h"
#include "rs_block.h"
#include "rs_blocklist.h"
#include "rs_graphic.h"
#include "rs_insert.h"
#include "rs_line.h"

namespace {

// Counts live instances so a test can tell whether a list freed a block.
class CountedBlock final : public RS_Block {
public:
    CountedBlock(RS_EntityContainer* parent, const QString& name, int& liveCount)
        : RS_Block(parent, RS_BlockData(name, RS_Vector(0.0, 0.0), false)), m_liveCount{liveCount} {
        ++m_liveCount;
    }
    ~CountedBlock() override { --m_liveCount; }

private:
    int& m_liveCount;
};

RS_Insert* makeInsert(RS_EntityContainer* parent, const QString& blockName) {
    return new RS_Insert(parent, RS_InsertData(blockName, RS_Vector(0.0, 0.0), RS_Vector(1.0, 1.0), 0.0, 1, 1,
                                               RS_Vector(0.0, 0.0)));
}

} // namespace

TEST_CASE("An owning RS_BlockList frees its blocks when destroyed", "[block][ownership]") {
    int live = 0;
    {
        RS_BlockList list(true);
        list.add(new CountedBlock(nullptr, "A", live));
        list.add(new CountedBlock(nullptr, "B", live));
        REQUIRE(list.count() == 2);
        REQUIRE(live == 2);
    }
    CHECK(live == 0);
}

TEST_CASE("A non-owning RS_BlockList frees nothing", "[block][ownership]") {
    int live = 0;
    auto* a = new CountedBlock(nullptr, "A", live);
    auto* b = new CountedBlock(nullptr, "B", live);
    {
        RS_BlockList list(false);
        list.add(a);
        list.add(b);
        list.clear();
        CHECK(live == 2);
        list.add(a);
        list.add(b);
    }
    CHECK(live == 2);
    delete a;
    delete b;
    CHECK(live == 0);
}

TEST_CASE("Clearing an owning RS_BlockList frees its blocks and forgets the active one", "[block][ownership]") {
    int live = 0;
    RS_BlockList list(true);
    auto* a = new CountedBlock(nullptr, "A", live);
    list.add(a);
    list.add(new CountedBlock(nullptr, "B", live));
    list.activate(a);
    REQUIRE(list.getActive() == a);

    list.clear();
    CHECK(live == 0);
    CHECK(list.count() == 0);
    CHECK(list.getActive() == nullptr);

    // The list is usable again after clearing.
    list.add(new CountedBlock(nullptr, "A", live));
    CHECK(list.count() == 1);
    CHECK(live == 1);
}

TEST_CASE("RS_BlockList::add frees a rejected duplicate but never a block it holds", "[block][ownership]") {
    int live = 0;
    {
        RS_BlockList list(true);
        auto* a = new CountedBlock(nullptr, "A", live);
        REQUIRE(list.add(a));

        // Adding the block the list already holds keeps it.
        CHECK_FALSE(list.add(a));
        CHECK(live == 1);
        CHECK(list.find("A") == a);

        // A distinct block of the same name is freed.
        CHECK_FALSE(list.add(new CountedBlock(nullptr, "A", live)));
        CHECK(live == 1);
        CHECK(list.count() == 1);

        // find() skips a block flagged deleted, so adding it again must not
        // list it twice (the destructor would free it twice).
        a->setFlag(RS2::FlagDeleted);
        CHECK_FALSE(list.add(a));
        CHECK(list.count() == 1);
    }
    CHECK(live == 0);
}

TEST_CASE("RS_BlockList::remove frees an owned block", "[block][ownership]") {
    int live = 0;
    RS_BlockList list(true);
    auto* a = new CountedBlock(nullptr, "A", live);
    list.add(a);
    list.add(new CountedBlock(nullptr, "B", live));
    list.activate(a);

    list.remove(a);
    CHECK(live == 1);
    CHECK(list.count() == 1);
    CHECK(list.getActive() == nullptr);
}

// Blocks hold inserts of other blocks and the drawing holds inserts of the
// blocks; none of them may reach a freed block while the drawing is torn down.
TEST_CASE("A drawing frees its blocks when re-initialised and destroyed", "[block][ownership]") {
    lc::test::application();
    int live = 0;
    {
        RS_Graphic graphic;
        graphic.initForNewDocument();
        auto addBlocks = [&] {
            auto* inner = new CountedBlock(&graphic, "INNER", live);
            inner->addEntity(new RS_Line(inner, RS_Vector(0.0, 0.0), RS_Vector(1.0, 1.0)));
            graphic.addBlock(inner);
            auto* outer = new CountedBlock(&graphic, "OUTER", live);
            outer->addEntity(makeInsert(outer, "INNER"));
            graphic.addBlock(outer);
            RS_Insert* insert = makeInsert(&graphic, "OUTER");
            graphic.addEntity(insert);
            insert->update();
            REQUIRE(insert->getBlockForInsert() == outer);
        };

        addBlocks();
        REQUIRE(live == 2);
        graphic.initForNewDocument();
        CHECK(live == 0);
        CHECK(graphic.countBlocks() == 0);

        addBlocks();
        REQUIRE(live == 2);
    }
    CHECK(live == 0);
}

// RS_Insert caches the block it found together with the list and the list's
// generation. A new list can reuse the address of a freed one, so its
// generation must never repeat one the freed list had, or the cache would
// hand back a freed block.
TEST_CASE("RS_BlockList generations are unique across lists", "[block][ownership]") {
    int live = 0;
    auto first = std::make_unique<RS_BlockList>(true);
    const std::size_t created = first->generation();
    first->add(new CountedBlock(nullptr, "A", live));
    const std::size_t afterAdd = first->generation();
    CHECK(afterAdd != created);
    first.reset();

    RS_BlockList second(true);
    CHECK(second.generation() != created);
    CHECK(second.generation() != afterAdd);
    second.add(new CountedBlock(nullptr, "A", live));
    CHECK(second.generation() != afterAdd);
}
