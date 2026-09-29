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

// An owning RS_BlockList frees its blocks (when destroyed, on clear() and
// remove(), and a block add() rejects); a non-owning one never does. Before,
// the destructor and clear() never freed anything, so every block of every
// drawing, with all its entities, leaked.
//
// The blocks are counted through a subclass, and the entities through one
// whose clone() is counted too, so that the copies made by the clipboard, and
// the drawings that hold them, can be accounted for.

#include <functional>
#include <memory>
#include <new>

#include <catch2/catch_test_macros.hpp>

#include "lc_actiontestsupport.h"
#include "lc_copyutils.h"
#include "rs_block.h"
#include "rs_blocklist.h"
#include "rs_clipboard.h"
#include "rs_font.h"
#include "rs_graphic.h"
#include "rs_insert.h"
#include "rs_line.h"

namespace {

class CountedBlock final : public RS_Block {
public:
    CountedBlock(const QString& name, int& liveCount)
        : RS_Block(nullptr, RS_BlockData(name, RS_Vector{0.0, 0.0}, false)), m_liveCount{liveCount} {
        ++m_liveCount;
    }
    ~CountedBlock() override { --m_liveCount; }

private:
    int& m_liveCount;
};

// A line whose clones are counted too.
class CountedLine final : public RS_Line {
public:
    CountedLine(RS_EntityContainer* parent, const RS_LineData& data, int& liveCount)
        : RS_Line(parent, data), m_liveCount{liveCount} {
        ++m_liveCount;
    }
    CountedLine(const CountedLine& other) : RS_Line(other), m_liveCount{other.m_liveCount} { ++m_liveCount; }
    ~CountedLine() override { --m_liveCount; }
    RS_Entity* clone() const override { return new CountedLine(*this); }

private:
    int& m_liveCount;
};

struct Drawing {
    // first, so the settings RS_Graphic reads are set up before it is built
    const bool m_qtReady{lc::test::application() != nullptr};
    RS_Graphic m_graphic;
    lc::test::TestGraphicView m_view;
    LC_ActionContext m_context;

    Drawing() {
        m_graphic.initForNewDocument();
        m_graphic.onLoadingCompleted();
        m_view.setDocument(&m_graphic);
        m_context.setDocumentAndView(&m_graphic, &m_view);
    }

    void addBlockOfLine(const QString& name, int& liveCount) {
        auto* block = new RS_Block(&m_graphic, RS_BlockData(name, RS_Vector{0.0, 0.0}, false));
        m_graphic.addBlock(block);
        block->addEntity(new CountedLine(block, RS_LineData(RS_Vector{0.0, 0.0}, RS_Vector{1.0, 0.0}), liveCount));
    }

    RS_Insert* addInsert(const QString& name) {
        auto* insert = new RS_Insert(&m_graphic, RS_InsertData(name, RS_Vector{5.0, 5.0}, RS_Vector{1.0, 1.0}, 0.0, 1, 1,
                                                               RS_Vector{0.0, 0.0}));
        m_graphic.addEntity(insert);
        insert->update();
        return insert;
    }

    void modify(const std::function<void(LC_DocumentModificationBatch&)>& operation) {
        m_graphic.undoableModify(m_view.getViewPort(), [&](LC_DocumentModificationBatch& ctx) {
            operation(ctx);
            return true;
        });
    }
};

} // namespace

TEST_CASE("An owning RS_BlockList frees its blocks when destroyed", "[blocks][ownership]") {
    int live = 0;
    {
        RS_BlockList list(true);
        REQUIRE(list.add(new CountedBlock(QStringLiteral("A"), live)));
        REQUIRE(list.add(new CountedBlock(QStringLiteral("B"), live)));
        REQUIRE(list.count() == 2);
        REQUIRE(live == 2);
    }
    CHECK(live == 0);
}

TEST_CASE("A non-owning RS_BlockList never frees a block", "[blocks][ownership]") {
    int live = 0;
    auto* a = new CountedBlock(QStringLiteral("A"), live);
    auto* twin = new CountedBlock(QStringLiteral("A"), live);
    {
        RS_BlockList list(false);
        REQUIRE(list.add(a));

        // a rejected duplicate stays with the caller
        CHECK_FALSE(list.add(twin));
        CHECK(live == 2);

        list.remove(a);
        CHECK(live == 2);
        CHECK(list.count() == 0);

        REQUIRE(list.add(a));
        list.clear();
        CHECK(live == 2);
        REQUIRE(list.add(a));
    }
    // destroyed with `a` in it
    CHECK(live == 2);
    delete a;
    delete twin;
    CHECK(live == 0);
}

TEST_CASE("RS_BlockList::clear frees the blocks and forgets the active one", "[blocks][ownership]") {
    int live = 0;
    RS_BlockList list(true);
    auto* a = new CountedBlock(QStringLiteral("A"), live);
    list.add(a);
    list.add(new CountedBlock(QStringLiteral("B"), live));
    list.activate(a);
    REQUIRE(list.getActive() == a);
    const std::size_t before = list.generation();

    list.clear();
    CHECK(live == 0);
    CHECK(list.count() == 0);
    CHECK(list.getActive() == nullptr);
    CHECK(list.find(QStringLiteral("A")) == nullptr);
    CHECK(list.generation() != before);

    // The list is usable again after clearing.
    CHECK(list.add(new CountedBlock(QStringLiteral("A"), live)));
    CHECK(live == 1);
}

TEST_CASE("RS_BlockList::remove frees the block it removes", "[blocks][ownership]") {
    int live = 0;
    RS_BlockList list(true);
    auto* a = new CountedBlock(QStringLiteral("A"), live);
    list.add(a);
    list.add(new CountedBlock(QStringLiteral("B"), live));
    list.activate(a);

    list.remove(a);
    CHECK(live == 1);
    CHECK(list.count() == 1);
    CHECK(list.getActive() == nullptr);
}

TEST_CASE("RS_BlockList::add frees a rejected duplicate but keeps a block added again", "[blocks][ownership]") {
    int live = 0;
    RS_BlockList list(true);
    auto* a = new CountedBlock(QStringLiteral("A"), live);
    REQUIRE(list.add(a));

    CHECK_FALSE(list.add(new CountedBlock(QStringLiteral("A"), live)));
    CHECK(live == 1);
    CHECK(list.find(QStringLiteral("A")) == a);

    // Adding the block that already holds the name must not free it.
    CHECK_FALSE(list.add(a));
    CHECK(live == 1);
    CHECK(list.count() == 1);
    CHECK(a->getName() == QStringLiteral("A"));
}

TEST_CASE("A generation is never shared between block lists", "[blocks][generation]") {
    RS_BlockList first(true);
    RS_BlockList second(true);
    CHECK(first.generation() != 0U);
    CHECK(first.generation() != second.generation());

    int live = 0;
    std::size_t last = first.generation();
    auto step = [&](const char* what) {
        INFO(what);
        CHECK(first.generation() > last);
        CHECK(first.generation() != second.generation());
        last = first.generation();
    };
    auto* a = new CountedBlock(QStringLiteral("A"), live);
    first.add(a);
    step("add");
    first.rename(a, QStringLiteral("B"));
    step("rename");
    first.remove(a);
    step("remove");
    first.clear();
    step("clear");
}

// An insert caches the block it found against (list, generation). A drawing
// created at the address of a destroyed one, with as many blocks added as the
// old one had, used to look exactly like the old list: the insert then handed
// out the block freed with it.
TEST_CASE("An insert does not reuse a block cached against a destroyed list", "[blocks][generation]") {
    int live = 0;
    alignas(RS_BlockList) unsigned char storage[sizeof(RS_BlockList)];

    auto* first = new (storage) RS_BlockList(true);
    first->add(new CountedBlock(QStringLiteral("PART"), live));
    const std::size_t firstGeneration = first->generation();

    RS_Insert insert(nullptr, RS_InsertData(QStringLiteral("PART"), RS_Vector{0.0, 0.0}, RS_Vector{1.0, 1.0}, 0.0, 1, 1,
                                            RS_Vector{0.0, 0.0}, first));
    REQUIRE(insert.getBlockForInsert() == first->find(QStringLiteral("PART")));
    first->~RS_BlockList();
    REQUIRE(live == 0);

    auto* second = new (storage) RS_BlockList(true);
    second->add(new CountedBlock(QStringLiteral("PART"), live));
    CHECK(second->generation() != firstGeneration);
    CHECK(insert.getBlockForInsert() == second->find(QStringLiteral("PART")));
    second->~RS_BlockList();
    CHECK(live == 0);
}

TEST_CASE("A drawing frees its blocks when re-initialised and destroyed", "[blocks][ownership]") {
    int live = 0;
    {
        Drawing drawing;
        drawing.m_graphic.addBlock(new CountedBlock(QStringLiteral("A"), live));
        drawing.m_graphic.addBlock(new CountedBlock(QStringLiteral("B"), live));
        REQUIRE(live == 2);

        // Loading a file starts with initForNewDocument().
        drawing.m_graphic.initForNewDocument();
        CHECK(live == 0);
        CHECK(drawing.m_graphic.countBlocks() == 0);

        drawing.m_graphic.addBlock(new CountedBlock(QStringLiteral("C"), live));
        REQUIRE(live == 1);
    }
    CHECK(live == 0);
}

TEST_CASE("A font frees its letters unless it does not own them", "[blocks][ownership]") {
    int live = 0;
    {
        RS_Font font(QStringLiteral("owning"));
        font.getLetterList()->add(new CountedBlock(QStringLiteral("A"), live));
        REQUIRE(live == 1);
    }
    CHECK(live == 0);

    auto* letter = new CountedBlock(QStringLiteral("A"), live);
    {
        RS_Font font(QStringLiteral("borrowing"), false);
        font.getLetterList()->add(letter);
    }
    CHECK(live == 1);
    delete letter;
    CHECK(live == 0);
}

// Copying to the clipboard clones the blocks the insert needs into the
// clipboard's own drawing, and pasting clones them again into the destination.
// Each drawing frees only its own, in any order, and what was pasted keeps
// resolving its blocks in its own drawing.
TEST_CASE("A pasted insert keeps its blocks after the source and the clipboard are gone", "[blocks][ownership][copy]") {
    int live = 0;
    {
        auto source = std::make_unique<Drawing>();
        source->addBlockOfLine(QStringLiteral("PART"), live);
        RS_Insert* part = source->addInsert(QStringLiteral("PART"));
        REQUIRE(part->count() == 1);

        QList<RS_Entity*> selection{part};
        source->m_graphic.select(selection, true);
        LC_CopyUtils::copy(RS_Vector{0.0, 0.0}, selection, &source->m_graphic);
        source.reset();

        Drawing destination;
        destination.modify([&](LC_DocumentModificationBatch& ctx) {
            LC_CopyUtils::paste(LC_CopyUtils::RS_PasteData(RS_Vector{0.0, 100.0}), &destination.m_graphic, ctx);
            ctx.dontSetActiveLayerAndPen();
        });
        RS_CLIPBOARD->clear();
        destination.m_graphic.updateInserts();

        RS_Block* ownPart = destination.m_graphic.findBlock(QStringLiteral("PART"));
        REQUIRE(ownPart != nullptr);
        int inserts = 0;
        for (RS_Entity* e : destination.m_graphic) {
            if (e != nullptr && !e->isDeleted() && e->rtti() == RS2::EntityInsert) {
                auto* insert = static_cast<RS_Insert*>(e);
                insert->update();
                CHECK(insert->getBlockForInsert() == ownPart);
                CHECK(insert->count() == 1);
                ++inserts;
            }
        }
        CHECK(inserts == 1);
    }
    RS_CLIPBOARD->clear();
    CHECK(live == 0);
}
