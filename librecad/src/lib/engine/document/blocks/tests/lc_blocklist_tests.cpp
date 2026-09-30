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

#include <functional>
#include <initializer_list>
#include <memory>
#include <new>

#include <catch2/catch_test_macros.hpp>

#include "lc_actiontestsupport.h"
#include "lc_copyutils.h"
#include "rs_block.h"
#include "rs_blocklist.h"
#include "rs_blocklistlistener.h"
#include "rs_clipboard.h"
#include "rs_font.h"
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

// A line whose clones are counted too, so the copies the clipboard and a
// paste make can be accounted for.
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
};

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

namespace {

/// what a block list looks like when it tells its listeners that it was cleared
class BlockClearRecorder final : public RS_BlockListListener {
public:
    BlockClearRecorder(const RS_BlockList& list, const int& live) : m_list{list}, m_live{live} {}
    void blockListCleared() override {
        ++m_cleared;
        m_count = m_list.count();
        m_liveInCallback = m_live;
    }
    void blockListModified(const bool) override { ++m_modified; }
    const RS_BlockList& m_list;
    const int& m_live;
    int m_cleared = 0;
    int m_count = 99;
    int m_liveInCallback = -1;
    int m_modified = 0;
};

}

TEST_CASE("Clearing a block list tells the listeners once, with the list empty and the blocks alive", "[block][ownership][2969]") {
    int live = 0;
    RS_BlockList list(true);
    list.add(new CountedBlock(nullptr, "A", live));
    list.add(new CountedBlock(nullptr, "B", live));
    BlockClearRecorder recorder(list, live);
    list.addListener(&recorder);
    recorder.m_modified = 0;

    list.clear();
    list.removeListener(&recorder);

    CHECK(recorder.m_cleared == 1);
    CHECK(recorder.m_count == 0);
    // told before they are freed
    CHECK(recorder.m_liveInCallback == 2);
    CHECK(live == 0);
    // and a listener that only knows blockListModified() hears it too
    CHECK(recorder.m_modified >= 1);
}

TEST_CASE("Clearing a non-owning block list tells the listeners and frees nothing", "[block][ownership][2969]") {
    int live = 0;
    auto* a = new CountedBlock(nullptr, "A", live);
    auto* b = new CountedBlock(nullptr, "B", live);
    {
        RS_BlockList list(false);
        list.add(a);
        list.add(b);
        BlockClearRecorder recorder(list, live);
        list.addListener(&recorder);
        list.clear();
        list.removeListener(&recorder);
        CHECK(recorder.m_cleared == 1);
        CHECK(recorder.m_count == 0);
        CHECK(live == 2);
    }
    delete a;
    delete b;
    CHECK(live == 0);
}

namespace {

/// what a block list tells its listeners when it is destroyed
class BlockDestroyRecorder final : public RS_BlockListListener {
public:
    /// \p list is only read to check that it is empty: a listener must not read it (the list is in its destructor)
    explicit BlockDestroyRecorder(const int& live, const RS_BlockList* list = nullptr) : m_live{live}, m_list{list} {}
    void blockListDestroyed() override {
        ++m_destroyed;
        m_liveInCallback = m_live;
        if (m_list != nullptr) {
            m_count = m_list->count();
        }
    }
    void blockListModified(const bool) override { ++m_modified; }
    void blockListCleared() override { ++m_cleared; }
    const int& m_live;
    const RS_BlockList* m_list;
    int m_count = 99;
    int m_destroyed = 0;
    int m_liveInCallback = -1;
    int m_modified = 0;
    int m_cleared = 0;
};

/// unregisters itself from the list in the callback
class BlockSelfRemoving final : public RS_BlockListListener {
public:
    explicit BlockSelfRemoving(RS_BlockList& list) : m_list{list} {}
    void blockListDestroyed() override {
        ++m_destroyed;
        m_list.removeListener(this);
    }
    RS_BlockList& m_list;
    int m_destroyed = 0;
};

}

TEST_CASE("A block list tells its listeners once when it is destroyed, before it frees the blocks", "[block][ownership][2969]") {
    int live = 0;
    auto list = std::make_unique<RS_BlockList>(true);
    list->add(new CountedBlock(nullptr, "A", live));
    list->add(new CountedBlock(nullptr, "B", live));
    BlockDestroyRecorder recorder(live, list.get());
    list->addListener(&recorder);
    list.reset();

    CHECK(recorder.m_destroyed == 1);
    CHECK(recorder.m_count == 0); // the list is empty when it tells them
    CHECK(recorder.m_liveInCallback == 2); // told before they are freed
    CHECK(live == 0);
    CHECK(recorder.m_modified == 0);
    CHECK(recorder.m_cleared == 0);
}

TEST_CASE("A non-owning block list tells its listeners when it is destroyed and frees nothing", "[block][ownership][2969]") {
    int live = 0;
    auto* a = new CountedBlock(nullptr, "A", live);
    auto* b = new CountedBlock(nullptr, "B", live);
    auto list = std::make_unique<RS_BlockList>(false);
    list->add(a);
    list->add(b);
    BlockDestroyRecorder recorder(live, list.get());
    list->addListener(&recorder);
    list.reset();
    CHECK(recorder.m_destroyed == 1);
    CHECK(recorder.m_count == 0);
    CHECK(recorder.m_liveInCallback == 2); // nothing was freed, before or after
    CHECK(live == 2);
    delete a;
    delete b;
    CHECK(live == 0);
}

TEST_CASE("A block list is destroyed without listeners, and after they unregistered", "[block][ownership][2969]") {
    int live = 0;
    { RS_BlockList empty(true); }
    auto list = std::make_unique<RS_BlockList>(true);
    list->add(new CountedBlock(nullptr, "A", live));
    BlockDestroyRecorder recorder(live);
    list->addListener(&recorder);
    CHECK(list->listenerCount() == 1);
    list->addListener(&recorder); // listed once
    CHECK(list->listenerCount() == 1);
    list->removeListener(&recorder);
    CHECK(list->listenerCount() == 0);
    list.reset();
    CHECK(recorder.m_destroyed == 0);
    CHECK(live == 0);
}

TEST_CASE("A block listener that unregisters itself when its list is destroyed does not make the next one miss it", "[block][ownership][2969]") {
    int live = 0;
    // one that removes itself from the middle of the list: a loop over the list itself would then
    // skip the next listener and call the last one twice
    auto list = std::make_unique<RS_BlockList>(true);
    BlockDestroyRecorder before(live);
    BlockSelfRemoving removing(*list);
    BlockDestroyRecorder afterOne(live);
    BlockDestroyRecorder afterTwo(live);
    for (RS_BlockListListener* listener : std::initializer_list<RS_BlockListListener*>{&before, &removing, &afterOne, &afterTwo}) {
        list->addListener(listener);
    }
    list.reset();
    CHECK(before.m_destroyed == 1);
    CHECK(removing.m_destroyed == 1);
    CHECK(afterOne.m_destroyed == 1);
    CHECK(afterTwo.m_destroyed == 1);
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

TEST_CASE("A non-owning RS_BlockList leaves removed and rejected blocks to the caller", "[block][ownership]") {
    int live = 0;
    auto* a = new CountedBlock(nullptr, "A", live);
    auto* twin = new CountedBlock(nullptr, "A", live);
    {
        RS_BlockList list(false);
        REQUIRE(list.add(a));
        CHECK_FALSE(list.add(twin));
        list.remove(a);
        CHECK(live == 2);
        CHECK(list.count() == 0);
    }
    CHECK(live == 2);
    delete a;
    delete twin;
    CHECK(live == 0);
}

// The end-to-end form of the generation test above: an insert that cached a
// block of a list keeps working when a new list is built at the very address
// of the destroyed one, with as many additions, instead of handing back the
// block freed with the old list.
TEST_CASE("An insert does not reuse a block cached against a destroyed list", "[block][ownership]") {
    int live = 0;
    alignas(RS_BlockList) unsigned char storage[sizeof(RS_BlockList)];

    auto* first = new (storage) RS_BlockList(true);
    first->add(new CountedBlock(nullptr, "PART", live));
    RS_Insert insert(nullptr, RS_InsertData("PART", RS_Vector(0.0, 0.0), RS_Vector(1.0, 1.0), 0.0, 1, 1,
                                            RS_Vector(0.0, 0.0), first));
    REQUIRE(insert.getBlockForInsert() == first->find("PART"));
    first->~RS_BlockList();
    REQUIRE(live == 0);

    auto* second = new (storage) RS_BlockList(true);
    second->add(new CountedBlock(nullptr, "PART", live));
    CHECK(insert.getBlockForInsert() == second->find("PART"));
    second->~RS_BlockList();
    CHECK(live == 0);
}

TEST_CASE("A font frees its letters unless it does not own them", "[block][ownership]") {
    int live = 0;
    {
        RS_Font font("owning");
        font.getLetterList()->add(new CountedBlock(nullptr, "A", live));
        REQUIRE(live == 1);
    }
    CHECK(live == 0);

    auto* letter = new CountedBlock(nullptr, "A", live);
    {
        RS_Font font("borrowing", false);
        font.getLetterList()->add(letter);
    }
    CHECK(live == 1);
    delete letter;
    CHECK(live == 0);
}

// Copying to the clipboard clones the blocks an insert needs into the
// clipboard's drawing, and pasting clones them again into the destination.
// Each drawing frees only its own, in any order, and what was pasted keeps
// resolving its blocks in its own drawing.
TEST_CASE("A pasted insert keeps its blocks after the source and the clipboard are gone", "[block][ownership][copy]") {
    int live = 0;
    {
        auto source = std::make_unique<Drawing>();
        auto* block = new RS_Block(&source->m_graphic, RS_BlockData("PART", RS_Vector(0.0, 0.0), false));
        source->m_graphic.addBlock(block);
        block->addEntity(new CountedLine(block, RS_LineData(RS_Vector(0.0, 0.0), RS_Vector(1.0, 0.0)), live));
        RS_Insert* part = makeInsert(&source->m_graphic, "PART");
        source->m_graphic.addEntity(part);
        part->update();
        REQUIRE(part->count() == 1);

        QList<RS_Entity*> selection{part};
        source->m_graphic.select(selection, true);
        LC_CopyUtils::copy(RS_Vector(0.0, 0.0), selection, &source->m_graphic);
        source.reset();

        Drawing destination;
        destination.m_graphic.undoableModify(destination.m_view.getViewPort(), [&](LC_DocumentModificationBatch& ctx) {
            LC_CopyUtils::paste(LC_CopyUtils::RS_PasteData(RS_Vector(0.0, 100.0)), &destination.m_graphic, ctx);
            ctx.dontSetActiveLayerAndPen();
            return true;
        });
        RS_CLIPBOARD->clear();
        destination.m_graphic.updateInserts();

        RS_Block* ownPart = destination.m_graphic.findBlock("PART");
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
