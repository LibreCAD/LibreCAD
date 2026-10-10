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

// RS_LayerList owns its layers: it frees them on clear() and destruction,
// as remove() and add() (for a rejected duplicate) always did. Before, every
// layer of every drawing leaked. RS_Layer has no virtual destructor, so these
// tests cannot count instances; run them under ASan or `leaks -atExit` to
// see the layers freed, and to catch anything still reaching a freed layer.
//
// Freeing the layers is only safe if nothing keeps a pointer to a layer of a
// drawing that is gone. The paths that move entities between drawings
// (clipboard copy and paste, library insert, a plugin's block from disk) are
// exercised here with the source drawing destroyed before its entities are
// used again.

#include <functional>
#include <initializer_list>
#include <memory>

#include <catch2/catch_test_macros.hpp>

#include <QTemporaryDir>

#include "doc_plugin_interface.h"
#include "lc_actiontestsupport.h"
#include "lc_copyutils.h"
#include "lc_documentinvariants.h"
#include "rs_block.h"
#include "rs_circle.h"
#include "rs_clipboard.h"
#include "rs_creation.h"
#include "rs_fileio.h"
#include "rs_graphic.h"
#include "rs_insert.h"
#include "rs_layer.h"
#include "rs_layerlist.h"
#include "rs_layerlistlistener.h"
#include "rs_line.h"
#include "rs_modification.h"

namespace {

struct Drawing {
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

    RS_Layer* addLayer(const QString& name) {
        auto* layer = new RS_Layer(name);
        layer->setPen(RS_Pen(RS_Color(255, 0, 0), RS2::Width05, RS2::DashLine));
        m_graphic.addLayer(layer);
        return m_graphic.findLayer(name);
    }

    void addBlock(const QString& name, const std::function<void(RS_Block&)>& fill) {
        auto* block = new RS_Block(&m_graphic, RS_BlockData(name, RS_Vector{0, 0}, false));
        m_graphic.addBlock(block);
        fill(*block);
    }

    RS_Insert* addInsert(const QString& name, RS_Layer* layer) {
        auto* insert = new RS_Insert(&m_graphic, RS_InsertData(name, RS_Vector{5, 5}, RS_Vector{1, 1}, 0, 1, 1,
                                                               RS_Vector{0, 0}));
        insert->setLayer(layer);
        m_graphic.addEntity(insert);
        insert->update();
        return insert;
    }

    // DOOR inserts KNOB; both draw on `layer`, as does the DOOR insert itself.
    RS_Insert* addNestedInsert(RS_Layer* layer) {
        addBlock("KNOB", [&](RS_Block& b) {
            auto* circle = new RS_Circle(&b, RS_CircleData(RS_Vector{0, 0}, 0.1));
            circle->setLayer(layer);
            b.addEntity(circle);
        });
        addBlock("DOOR", [&](RS_Block& b) {
            auto* line = new RS_Line(&b, RS_LineData(RS_Vector{0, 0}, RS_Vector{1, 0}));
            line->setLayer(layer);
            b.addEntity(line);
            b.addEntity(new RS_Insert(&b, RS_InsertData("KNOB", RS_Vector{1, 0}, RS_Vector{1, 1}, 0, 1, 1,
                                                        RS_Vector{0, 0})));
        });
        return addInsert("DOOR", layer);
    }

    void modify(const std::function<void(LC_DocumentModificationBatch&)>& operation) {
        m_graphic.undoableModify(m_view.getViewPort(), [&](LC_DocumentModificationBatch& ctx) {
            operation(ctx);
            return true;
        });
    }
};

// Every layer reachable from the drawing's entities, down to the expansion
// children of inserts, is one of its own. Resolving each entity's pen and
// visibility reads its layer, so ASan also catches a freed one here.
void checkOwnLayers(RS_Graphic& graphic) {
    CHECK(lc::test::documentProblems(graphic).isEmpty());
    const RS_LayerList* layers = graphic.getLayerList();
    std::function<void(const RS_EntityContainer&)> walk = [&](const RS_EntityContainer& container) {
        for (const RS_Entity* e : container) {
            if (e == nullptr) {
                continue;
            }
            // the entity's own pointer: getLayer(true) answers nullptr for a
            // layer that is not the drawing's, which hides it
            RS_Layer* own = e->getLayer(false);
            CHECK((own == nullptr || layers->contains(own)));
            const RS_Layer* layer = e->getLayer(true);
            if (layer != nullptr) {
                CHECK(layers->contains(const_cast<RS_Layer*>(layer)));
                CHECK_FALSE(layer->getName().isEmpty());
            }
            (void)e->getPen(true);
            (void)e->isVisible();
            if (e->isContainer()) {
                walk(*static_cast<const RS_EntityContainer*>(e));
            }
        }
    };
    walk(graphic);
    for (unsigned i = 0; i < graphic.countBlocks(); ++i) {
        walk(*graphic.blockAt(i));
    }
}

// Copies the entity to the clipboard and pastes it into the other drawing.
void copyAndPaste(Drawing& source, RS_Entity* entity, Drawing& destination) {
    QList<RS_Entity*> selection{entity};
    source.m_graphic.select(selection, true);
    LC_CopyUtils::copy(RS_Vector{0, 0}, selection, &source.m_graphic);
    destination.modify([&](LC_DocumentModificationBatch& ctx) {
        LC_CopyUtils::paste(LC_CopyUtils::RS_PasteData(RS_Vector{0, 100}), &destination.m_graphic, ctx);
        ctx.dontSetActiveLayerAndPen();
    });
}

} // namespace

TEST_CASE("RS_LayerList keeps a layer that is added to it again", "[layers][ownership]") {
    RS_LayerList list;
    auto* layer = new RS_Layer(QStringLiteral("WALLS"));
    list.add(layer);

    // add() merges a same-named layer into the listed one and frees it; the
    // listed layer itself must neither be merged into itself nor freed.
    list.add(layer);
    REQUIRE(list.count() == 1);
    CHECK(list.find(QStringLiteral("WALLS")) == layer);
    CHECK(layer->getName() == QStringLiteral("WALLS"));
    CHECK(list.getActive() == layer);

    // A distinct layer of the same name is merged and freed.
    auto* duplicate = new RS_Layer(QStringLiteral("WALLS"));
    duplicate->lock(true);
    list.add(duplicate);
    CHECK(list.count() == 1);
    CHECK(list.find(QStringLiteral("WALLS")) == layer);
    CHECK(layer->isLocked());
}

// RS_LayerList::clear() is reached through RS_Graphic::clearLayers() (and
// initForNewDocument(), below), as is activation.
TEST_CASE("Clearing a drawing's layers frees them and forgets the active one", "[layers][ownership]") {
    Drawing drawing;
    RS_Layer* walls = drawing.addLayer(QStringLiteral("WALLS"));
    drawing.addLayer(QStringLiteral("DOORS"));
    drawing.m_graphic.activateLayer(walls);
    const RS_LayerList* layers = drawing.m_graphic.getLayerList();
    REQUIRE(layers->count() == 3); // "0" + the two above
    REQUIRE(drawing.m_graphic.getActiveLayer() == walls);

    drawing.m_graphic.clearLayers();
    CHECK(layers->count() == 0);
    CHECK(drawing.m_graphic.getActiveLayer() == nullptr);
    CHECK_FALSE(layers->contains(walls));
    CHECK(drawing.m_graphic.findLayer(QStringLiteral("WALLS")) == nullptr);

    // The list is usable again after clearing.
    drawing.addLayer(QStringLiteral("0"));
    CHECK(layers->count() == 1);
}

namespace {

/// what a layer list looks like when it tells its listeners that it was cleared
class ClearRecorder final : public RS_LayerListListener {
public:
    ClearRecorder(RS_Graphic& graphic, const QList<RS_Layer*>& layers) : m_graphic{graphic}, m_layers{layers} {}
    void layerListCleared() override {
        ++m_cleared;
        m_count = m_graphic.countLayers();
        m_active = m_graphic.getActiveLayer();
        // the layers are about to be freed: they must still be readable (ASan fails if one was already)
        for (const RS_Layer* layer : std::as_const(m_layers)) {
            m_names << layer->getName();
        }
    }
    RS_Graphic& m_graphic;
    QList<RS_Layer*> m_layers;
    int m_cleared = 0;
    unsigned m_count = 99;
    const RS_Layer* m_active = nullptr;
    QStringList m_names;
};

/// a listener that only knows layerListModified()
class ModifiedCounter final : public RS_LayerListListener {
public:
    void layerListModified(const bool) override { ++m_modified; }
    int m_modified = 0;
};

}

TEST_CASE("Clearing a drawing's layers tells the listeners once, with the list empty and the layers alive", "[layers][ownership][2969]") {
    Drawing drawing;
    RS_Layer* walls = drawing.addLayer(QStringLiteral("WALLS"));
    drawing.addLayer(QStringLiteral("DOORS"));
    drawing.m_graphic.activateLayer(walls);
    QList<RS_Layer*> layers;
    for (unsigned i = 0; i < drawing.m_graphic.countLayers(); ++i) {
        layers << drawing.m_graphic.layerAt(i);
    }
    REQUIRE(layers.size() == 3);

    ClearRecorder recorder(drawing.m_graphic, layers);
    drawing.m_graphic.addLayerListListener(&recorder);
    drawing.m_graphic.clearLayers();
    drawing.m_graphic.removeLayerListListener(&recorder);

    CHECK(recorder.m_cleared == 1);
    CHECK(recorder.m_count == 0);
    CHECK(recorder.m_active == nullptr);
    recorder.m_names.sort();
    CHECK(recorder.m_names == QStringList{QStringLiteral("0"), QStringLiteral("DOORS"), QStringLiteral("WALLS")});
}

TEST_CASE("A layer listener that only knows layerListModified() hears a clear", "[layers][ownership][2969]") {
    Drawing drawing;
    ModifiedCounter counter;
    drawing.m_graphic.addLayerListListener(&counter);
    drawing.m_graphic.clearLayers();
    drawing.m_graphic.removeLayerListListener(&counter);
    CHECK(counter.m_modified == 1);
}

namespace {

/// what a layer list tells its listeners when it is destroyed
class DestroyRecorder final : public RS_LayerListListener {
public:
    /// \p list is only read to check that it is empty: a listener must not read it (the list is in its destructor)
    explicit DestroyRecorder(const QList<RS_Layer*>& layers, const RS_LayerList* list = nullptr) : m_layers{layers}, m_list{list} {}
    void layerListDestroyed() override {
        ++m_destroyed;
        if (m_list != nullptr) {
            m_count = m_list->count();
            m_active = m_list->getActive();
        }
        // the layers are freed after the callback: they must still be readable (ASan fails if not)
        for (const RS_Layer* layer : std::as_const(m_layers)) {
            m_names << layer->getName();
        }
    }
    void layerListModified(const bool) override { ++m_modified; }
    void layerListCleared() override { ++m_cleared; }
    QList<RS_Layer*> m_layers;
    const RS_LayerList* m_list;
    unsigned m_count = 99;
    const RS_Layer* m_active = nullptr;
    int m_destroyed = 0;
    int m_modified = 0;
    int m_cleared = 0;
    QStringList m_names;
};

/// unregisters itself from the list in the callback
class SelfRemoving final : public RS_LayerListListener {
public:
    explicit SelfRemoving(RS_LayerList& list) : m_list{list} {}
    void layerListDestroyed() override {
        ++m_destroyed;
        m_list.removeListener(this);
    }
    RS_LayerList& m_list;
    int m_destroyed = 0;
};

}

TEST_CASE("A layer list tells its listeners once when it is destroyed, before it frees the layers", "[layers][ownership][2969]") {
    auto list = std::make_unique<RS_LayerList>();
    list->add(new RS_Layer(QStringLiteral("WALLS")));
    list->add(new RS_Layer(QStringLiteral("DOORS")));
    QList<RS_Layer*> layers;
    for (unsigned i = 0; i < list->count(); ++i) {
        layers << list->at(i);
    }
    REQUIRE(layers.size() == 2);

    REQUIRE(list->getActive() != nullptr);
    DestroyRecorder recorder(layers, list.get());
    list->addListener(&recorder);
    list.reset();

    CHECK(recorder.m_destroyed == 1);
    // the list is empty when it tells them
    CHECK(recorder.m_count == 0);
    CHECK(recorder.m_active == nullptr);
    recorder.m_names.sort();
    CHECK(recorder.m_names == QStringList{QStringLiteral("DOORS"), QStringLiteral("WALLS")});
    // a listener that only knows layerListModified() hears nothing from a destruction
    CHECK(recorder.m_modified == 0);
    CHECK(recorder.m_cleared == 0);
}

TEST_CASE("A listener that unregisters itself when its list is destroyed does not make the next one miss it", "[layers][ownership][2969]") {
    // one that removes itself from the middle of the list: a loop over the list itself would then
    // skip the next listener and call the last one twice
    auto list = std::make_unique<RS_LayerList>();
    DestroyRecorder before({});
    SelfRemoving removing(*list);
    DestroyRecorder afterOne({});
    DestroyRecorder afterTwo({});
    for (RS_LayerListListener* listener : std::initializer_list<RS_LayerListListener*>{&before, &removing, &afterOne, &afterTwo}) {
        list->addListener(listener);
    }
    list.reset();
    CHECK(before.m_destroyed == 1);
    CHECK(removing.m_destroyed == 1);
    CHECK(afterOne.m_destroyed == 1);
    CHECK(afterTwo.m_destroyed == 1);
}

TEST_CASE("A layer list is destroyed without listeners, and after they unregistered", "[layers][ownership][2969]") {
    { RS_LayerList empty; }
    auto list = std::make_unique<RS_LayerList>();
    list->add(new RS_Layer(QStringLiteral("WALLS")));
    DestroyRecorder recorder({});
    list->addListener(&recorder);
    CHECK(list->listenerCount() == 1);
    list->addListener(&recorder); // listed once
    CHECK(list->listenerCount() == 1);
    list->removeListener(&recorder);
    CHECK(list->listenerCount() == 0);
    list.reset();
    CHECK(recorder.m_destroyed == 0);
}

TEST_CASE("A drawing frees its layers when re-initialised and destroyed", "[layers][ownership]") {
    Drawing drawing;
    drawing.addLayer(QStringLiteral("WALLS"));
    drawing.addNestedInsert(drawing.m_graphic.findLayer(QStringLiteral("WALLS")));
    REQUIRE(drawing.m_graphic.getLayerList()->count() == 2);

    // Loading a file into a drawing starts with initForNewDocument(), which
    // clears the entities, the layers and then the blocks.
    drawing.m_graphic.initForNewDocument();
    CHECK(drawing.m_graphic.getLayerList()->count() == 1);
    CHECK(drawing.m_graphic.findLayer(QStringLiteral("0")) != nullptr);
    CHECK(drawing.m_graphic.countBlocks() == 0);
}

// The clipboard copies the insert and the blocks it needs onto its own layers,
// and rebuilds the expansion children of the inserts from its own blocks (see
// LC_CopyUtils::doCopyBlock): as copied they name the source's layers, which
// are freed with the source drawing, before the paste.
TEST_CASE("An insert copied to the clipboard pastes after its drawing is closed", "[layers][ownership][copy]") {
    auto source = std::make_unique<Drawing>();
    RS_Layer* walls = source->addLayer(QStringLiteral("WALLS"));
    RS_Insert* door = source->addNestedInsert(walls);
    REQUIRE(door->count() == 2);

    QList<RS_Entity*> selection{door};
    source->m_graphic.select(selection, true);
    LC_CopyUtils::copy(RS_Vector{0, 0}, selection, &source->m_graphic);
    source.reset();
    checkOwnLayers(*RS_CLIPBOARD->getGraphic());

    Drawing destination;
    destination.modify([&](LC_DocumentModificationBatch& ctx) {
        LC_CopyUtils::paste(LC_CopyUtils::RS_PasteData(RS_Vector{0, 100}), &destination.m_graphic, ctx);
        ctx.dontSetActiveLayerAndPen();
    });
    destination.m_graphic.updateInserts();

    RS_Layer* ownWalls = destination.m_graphic.findLayer(QStringLiteral("WALLS"));
    REQUIRE(ownWalls != nullptr);
    CHECK(ownWalls->getPen().getColor() == RS_Color(255, 0, 0));
    checkOwnLayers(destination.m_graphic);

    // Pasting again after the clipboard's own layers were replaced.
    RS_CLIPBOARD->clear();
    checkOwnLayers(destination.m_graphic);
}

// In the next two tests the source stays open, so that its layers keep their
// addresses: a pointer to one of them then cannot pass for a layer that the
// destination allocated in its place, as it can above.

// A block is copied with the expansion children of the inserts in it, which
// name the layers of the drawing it comes from, and nothing rebuilt those.
TEST_CASE("An insert in a pasted block keeps no layer of the drawing it was copied from", "[layers][ownership][copy]") {
    Drawing source;
    RS_Insert* door = source.addNestedInsert(source.addLayer(QStringLiteral("WALLS")));
    Drawing destination;
    copyAndPaste(source, door, destination);
    checkOwnLayers(*RS_CLIPBOARD->getGraphic());
    checkOwnLayers(destination.m_graphic);

    // The knob in the pasted DOOR is on WALLS, as it was.
    const RS_Block* ownDoor = destination.m_graphic.findBlock(QStringLiteral("DOOR"));
    REQUIRE(ownDoor != nullptr);
    REQUIRE(ownDoor->count() == 2);
    const RS_Entity* knob = ownDoor->entityAt(1);
    REQUIRE(knob->rtti() == RS2::EntityInsert);
    REQUIRE(knob->count() == 1);
    CHECK(static_cast<const RS_Insert*>(knob)->entityAt(0)->getLayer(false)
          == destination.m_graphic.findLayer(QStringLiteral("WALLS")));
}

// RS_ActionBlocksCreate makes a block of a selection with
// RS_Creation::createBlock(), which gives it no parent: nothing then tells
// which drawing's layers its entities are on, so they cannot be put on the
// destination's layers of the same names. They must not keep the source's.
TEST_CASE("A pasted block that was made from a selection keeps no layer of the drawing it was copied from",
          "[layers][ownership][copy]") {
    Drawing source;
    RS_Layer* walls = source.addLayer(QStringLiteral("WALLS"));
    auto* line = new RS_Line(&source.m_graphic, RS_LineData(RS_Vector{0, 0}, RS_Vector{1, 0}));
    line->setLayer(walls);
    source.m_graphic.addEntity(line);
    const RS_BlockData made(QStringLiteral("MADE"), RS_Vector{0, 0}, false);
    source.m_graphic.addBlock(RS_Creation::createBlock(&made, RS_Vector{0, 0}, {line}));

    Drawing destination;
    copyAndPaste(source, source.addInsert(QStringLiteral("MADE"), walls), destination);
    checkOwnLayers(*RS_CLIPBOARD->getGraphic());
    checkOwnLayers(destination.m_graphic);
}

TEST_CASE("A library insert keeps nothing of the library drawing's layers", "[layers][ownership][copy]") {
    auto library = std::make_unique<Drawing>();
    RS_Layer* walls = library->addLayer(QStringLiteral("WALLS"));
    library->addNestedInsert(walls);

    Drawing destination;
    destination.modify([&](LC_DocumentModificationBatch& ctx) {
        RS_Modification::libraryInsert(LC_LibraryInsertData(RS_Vector{0, 0}, 1, 0, "PART", &library->m_graphic),
                                       &destination.m_graphic, ctx);
    });
    library.reset();
    destination.m_graphic.updateInserts();

    REQUIRE(destination.m_graphic.findLayer(QStringLiteral("WALLS")) != nullptr);
    checkOwnLayers(destination.m_graphic);
}

// Doc_plugin_interface::addBlockfromFromdisk() reads the file into a drawing of
// its own, which is gone when it returns.
TEST_CASE("A plugin's block from disk keeps nothing of the layers of the drawing its file was read into",
          "[layers][ownership][copy]") {
    const QTemporaryDir directory;
    const QString path = directory.filePath(QStringLiteral("part.dxf"));
    {
        Drawing part;
        part.addNestedInsert(part.addLayer(QStringLiteral("WALLS")));
        REQUIRE(RS_FileIO::instance()->fileExport(part.m_graphic, path, RS2::FormatDXFRW));
    }

    Drawing destination;
    Doc_plugin_interface plugin(&destination.m_context, nullptr);
    REQUIRE_FALSE(plugin.addBlockfromFromdisk(path).isEmpty());

    REQUIRE(destination.m_graphic.findLayer(QStringLiteral("WALLS")) != nullptr);
    checkOwnLayers(destination.m_graphic);
}

// "Hide all layers except current": the layer list's filter leaves the layers
// it does not show as they are, so the list used to find one of those still
// visible, make it the active layer and leave the current one hidden.
TEST_CASE("Hiding all layers never hides the current one", "[layers][active]") {
    (void)lc::test::application();
    RS_Graphic graphic;
    graphic.initForNewDocument();
    RS_Layer* zero = graphic.findLayer(QStringLiteral("0"));
    REQUIRE(zero != nullptr);
    auto* doors = new RS_Layer(QStringLiteral("Doors"));
    auto* wall1 = new RS_Layer(QStringLiteral("Wall1"));
    auto* wall2 = new RS_Layer(QStringLiteral("Wall2"));
    for (RS_Layer* layer : {doors, wall1, wall2}) {
        graphic.addLayer(layer);
    }
    graphic.activateLayer(wall1);
    REQUIRE(graphic.getActiveLayer() == wall1);

    SECTION("with the layer list showing every layer") {
        graphic.freezeAllLayers(true);
        CHECK(graphic.getActiveLayer() == wall1);
        CHECK_FALSE(wall1->isFrozen());
        CHECK(wall2->isFrozen());
        CHECK(doors->isFrozen());
        CHECK(zero->isFrozen());
    }

    SECTION("with the layer list filtered to the layers named Wall*") {
        zero->visibleInLayerList(false);
        doors->visibleInLayerList(false);
        graphic.freezeAllLayers(true);
        CHECK(graphic.getActiveLayer() == wall1);
        CHECK_FALSE(wall1->isFrozen());
        CHECK(wall2->isFrozen());
        // not listed, so not touched
        CHECK_FALSE(doors->isFrozen());
        CHECK_FALSE(zero->isFrozen());
    }

    SECTION("showing all layers again shows the listed ones only") {
        doors->visibleInLayerList(false);
        doors->freeze(true);
        graphic.freezeAllLayers(true);
        graphic.freezeAllLayers(false);
        for (const RS_Layer* layer : {zero, wall1, wall2}) {
            CHECK_FALSE(layer->isFrozen());
        }
        CHECK(doors->isFrozen());
        CHECK(graphic.getActiveLayer() == wall1);
    }
}
