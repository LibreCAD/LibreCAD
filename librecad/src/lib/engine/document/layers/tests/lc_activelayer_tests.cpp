/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2026 LibreCAD.org
** Copyright (C) 2026 Dongxu Li
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

// Issue #3229: a hidden layer can be the current layer. Since #2746 the layer list kept the
// current layer visible: hiding it made another layer current, and a hidden layer could not be
// made current, so what the user drew went to a layer they had not chosen (the issue: draw a
// parallel on a construction layer that is hidden so that its own entities are out of the way).
// Now hiding a layer never changes the current one, a hidden layer is made current as any other,
// and what is drawn on it is hidden with it. A locked layer is not made current, since what is
// drawn would change it: locking the current layer makes another layer current (with the command
// line saying so), or, when no other layer is unlocked, is not done; a drawing's saved current
// layer is restored as it was saved unless it is locked and another layer is not.

#include <catch2/catch_test_macros.hpp>

#include <QList>
#include <QStringList>
#include <QTemporaryDir>

#include "lc_action_draw_line_parallel.h"
#include "lc_actioncontext.h"
#include "lc_actionlayerscmd.h"
#include "lc_actiontestsupport.h"
#include "rs_actionlayerstoggleview.h"
#include "rs_dialogfactory.h"
#include "rs_dialogfactoryinterface.h"
#include "rs_fileio.h"
#include "rs_graphic.h"
#include "rs_layer.h"
#include "rs_layerlist.h"
#include "rs_layerlistlistener.h"
#include "rs_line.h"

namespace {

/// the layers the list said were activated
class ActivationRecorder final : public RS_LayerListListener {
public:
    QList<RS_Layer*> m_activated;
    QList<RS_Layer*> m_replaced;

    void layerActivated(RS_Layer* layer) override {
        m_activated << layer;
    }
    void activeLayerReplaced(RS_Layer* layer) override {
        m_replaced << layer;
    }
};

/// keeps the command line messages of RS_DIALOGFACTORY while it lives
class CommandMessages final : public RS_DialogFactoryInterface {
public:
    QStringList m_messages;

    CommandMessages() : m_previous{RS_DialogFactory::instance()->getFactoryObject()} {
        RS_DialogFactory::instance()->setFactoryObject(this);
    }
    ~CommandMessages() override {
        RS_DialogFactory::instance()->setFactoryObject(m_previous);
    }
    CommandMessages(const CommandMessages&) = delete;
    CommandMessages& operator=(const CommandMessages&) = delete;

    void commandMessage(const QString& message) override {
        m_messages << message;
    }

private:
    RS_DialogFactoryInterface* m_previous;
};

// "0", "Construction", "Doors" and "Walls", with "Walls" current
struct Layers {
    const bool m_qtReady{lc::test::application() != nullptr};
    RS_Graphic m_graphic;
    lc::test::TestGraphicView m_view;
    LC_ActionContext m_context;
    RS_Layer* m_zero = nullptr;
    RS_Layer* m_construction = new RS_Layer(QStringLiteral("Construction"));
    RS_Layer* m_doors = new RS_Layer(QStringLiteral("Doors"));
    RS_Layer* m_walls = new RS_Layer(QStringLiteral("Walls"));
    ActivationRecorder m_activations;

    Layers() {
        m_graphic.initForNewDocument();
        m_zero = m_graphic.findLayer(QStringLiteral("0"));
        for (RS_Layer* layer : {m_construction, m_doors, m_walls}) {
            m_graphic.addLayer(layer);
        }
        m_graphic.activateLayer(m_walls);
        m_view.setDocument(&m_graphic);
        m_context.setDocumentAndView(&m_graphic, &m_view);
        m_graphic.addLayerListListener(&m_activations);
    }

    void toggleView(RS_Layer* layer) {
        RS_ActionLayersToggleView action(&m_context, layer);
        action.trigger();
    }
};

class LayersCommand final : public LC_ActionLayersCmd {
public:
    using LC_ActionLayersCmd::LC_ActionLayersCmd;
    using LC_ActionLayersCmd::doProcessCommand;
};

/// the parallel tool, picking the entity at a point as a click there does
class ParallelTool final : public LC_ActionDrawLineParallel {
public:
    explicit ParallelTool(LC_ActionContext* context)
        : LC_ActionDrawLineParallel(context, RS2::ActionDrawLineParallel) {}

    void click(const RS_Vector& point) {
        // status 0: the tool waits for the entity to draw a parallel to
        const LC_MouseEvent event = lc::test::eventAt(point.x, point.y);
        onMouseMoveEvent(0, &event);
        onMouseLeftButtonRelease(0, &event);
    }
};

const QString lockedMessage = QStringLiteral("Layer \"Doors\" is locked: unlock it to make it the current layer.");

} // namespace

TEST_CASE("A hidden layer is made current and stays hidden", "[layers][active][3229]") {
    Layers f;
    f.m_construction->freeze(true);
    CommandMessages command;

    SECTION("by its pointer") {
        CHECK(f.m_graphic.activateLayer(f.m_construction, true));
    }
    SECTION("by its name") {
        CHECK(f.m_graphic.activateLayer(QStringLiteral("Construction"), true));
    }
    SECTION("as the user chooses it in the docks and the property sheet") {
        CHECK(f.m_graphic.activateChosenLayer(f.m_construction));
    }
    SECTION("with the cslayer command") {
        LayersCommand cslayer(&f.m_context, RS2::ActionLayersActivateCmd);
        cslayer.doProcessCommand(0, QStringLiteral("Construction"));
    }

    CHECK(f.m_graphic.getActiveLayer() == f.m_construction);
    CHECK(f.m_construction->isFrozen());
    CHECK(f.m_activations.m_activated == QList<RS_Layer*>{f.m_construction});
    CHECK(command.m_messages.join('\n').toStdString().empty());
}

TEST_CASE("Hiding the current layer keeps it current", "[layers][active][3229]") {
    Layers f;

    SECTION("with its eye icon") {
        f.toggleView(f.m_walls);
    }
    SECTION("with its eye icon, as the only layer that was visible") {
        for (RS_Layer* layer : {f.m_zero, f.m_construction, f.m_doors}) {
            layer->freeze(true);
        }
        f.toggleView(f.m_walls);
        // no layer was shown in its place
        for (const RS_Layer* layer : {f.m_zero, f.m_construction, f.m_doors}) {
            CHECK(layer->isFrozen());
        }
    }
    SECTION("with the layers selected in the Layer List") {
        f.m_walls->selectedInLayerList(true);
        f.m_doors->selectedInLayerList(true);
        f.toggleView(f.m_walls);
        CHECK(f.m_doors->isFrozen());
    }
    SECTION("as a group of the Layer Tree is hidden while another is shown") {
        f.m_zero->freeze(true);
        f.m_graphic.setFreezeLayers({f.m_zero}, {f.m_walls, f.m_doors});
        CHECK_FALSE(f.m_zero->isFrozen());
    }
    SECTION("by editing it") {
        RS_Layer edited = *f.m_walls;
        edited.freeze(true);
        f.m_graphic.editLayer(f.m_walls, edited);
    }

    CHECK(f.m_walls->isFrozen());
    CHECK(f.m_graphic.getActiveLayer() == f.m_walls);
    CHECK(f.m_activations.m_activated.isEmpty());
}

TEST_CASE("Hide all layers except current still leaves the current layer shown", "[layers][active][3229]") {
    Layers f;
    f.m_walls->freeze(true);
    f.m_graphic.freezeAllLayers(true);
    CHECK(f.m_graphic.getActiveLayer() == f.m_walls);
    // it does not show the current layer either
    CHECK(f.m_walls->isFrozen());
    f.m_graphic.freezeAllLayers(false);
    CHECK_FALSE(f.m_walls->isFrozen());
    CHECK(f.m_graphic.getActiveLayer() == f.m_walls);
}

TEST_CASE("A parallel drawn with a hidden current layer goes to that layer", "[layers][active][3229][parallel_action]") {
    // the steps of the issue: make a construction layer current, hide it, and draw a parallel to
    // a line of a visible layer
    Layers f;
    auto* wall = new RS_Line(&f.m_graphic, RS_Vector{0.0, 0.0}, RS_Vector{10.0, 0.0});
    wall->setLayer(f.m_walls);
    f.m_graphic.addEntity(wall);
    REQUIRE(f.m_graphic.activateChosenLayer(f.m_construction));
    f.toggleView(f.m_construction);
    REQUIRE(f.m_construction->isFrozen());
    const unsigned before = f.m_graphic.count();

    ParallelTool tool(&f.m_context);
    tool.click(RS_Vector{5.0, 0.25});

    REQUIRE(f.m_graphic.count() == before + 1);
    RS_Entity* parallel = f.m_graphic.last();
    REQUIRE(parallel != nullptr);
    REQUIRE(parallel != wall);
    CHECK(parallel->getLayer() == f.m_construction);
    CHECK_FALSE(parallel->isVisible());
    CHECK(f.m_graphic.getActiveLayer() == f.m_construction);
    CHECK(wall->getLayer() == f.m_walls);
    CHECK(wall->isVisible());
}

TEST_CASE("A locked layer is not made current", "[layers][active][3229]") {
    Layers f;
    f.m_graphic.toggleLayerLock(f.m_doors);
    REQUIRE(f.m_doors->isLocked());
    CommandMessages command;

    SECTION("by its pointer") {
        CHECK_FALSE(f.m_graphic.activateLayer(f.m_doors, true));
        CHECK(command.m_messages.isEmpty());
    }
    SECTION("by its name") {
        CHECK_FALSE(f.m_graphic.activateLayer(QStringLiteral("Doors"), true));
        CHECK(command.m_messages.isEmpty());
    }
    SECTION("as the user chooses it, who is told why") {
        CHECK_FALSE(f.m_graphic.activateChosenLayer(f.m_doors));
        CHECK(command.m_messages == QStringList{lockedMessage});
    }
    SECTION("with the cslayer command, which says why") {
        LayersCommand cslayer(&f.m_context, RS2::ActionLayersActivateCmd);
        cslayer.doProcessCommand(0, QStringLiteral("Doors"));
        CHECK(command.m_messages == QStringList{lockedMessage});
    }
    SECTION("while it is hidden too") {
        f.m_doors->freeze(true);
        CHECK_FALSE(f.m_graphic.activateChosenLayer(f.m_doors));
        CHECK(command.m_messages == QStringList{lockedMessage});
    }

    CHECK(f.m_graphic.getActiveLayer() == f.m_walls);
    CHECK(f.m_doors->isLocked());
    CHECK(f.m_activations.m_activated.isEmpty());

    // unlocked, it is made current
    f.m_graphic.toggleLayerLock(f.m_doors);
    CHECK(f.m_graphic.activateChosenLayer(f.m_doors));
    CHECK(f.m_graphic.getActiveLayer() == f.m_doors);
}

TEST_CASE("The cslayer command says when there is no such layer", "[layers][active][3229]") {
    Layers f;
    CommandMessages command;
    LayersCommand cslayer(&f.m_context, RS2::ActionLayersActivateCmd);
    cslayer.doProcessCommand(0, QStringLiteral("Missing"));
    CHECK(f.m_graphic.getActiveLayer() == f.m_walls);
    CHECK(command.m_messages == QStringList{QStringLiteral("There is no layer \"Missing\".")});
}

TEST_CASE("A layer that is not in the drawing is not made current", "[layers][active][3229]") {
    Layers f;
    RS_Layer foreign(QStringLiteral("Foreign"));
    CHECK_FALSE(f.m_graphic.activateLayer(&foreign, true));
    CHECK_FALSE(f.m_graphic.activateChosenLayer(&foreign));
    CHECK_FALSE(f.m_graphic.activateLayer(nullptr, true));
    CHECK(f.m_graphic.getActiveLayer() == f.m_walls);
    CHECK(f.m_activations.m_activated.isEmpty());
}

const QString replacedMessage = QStringLiteral(
    "Layer \"Walls\" is locked, so it cannot stay the current layer: \"%1\" is the current layer now.");
const QString keptMessage = QStringLiteral(
    "Layer \"Walls\" was not locked: it is the current layer, and no other layer is unlocked to take its place.");

TEST_CASE("Locking the current layer makes another layer current", "[layers][active][3229][lock]") {
    Layers f;
    CommandMessages command;
    RS_Layer* expected = f.m_zero;

    // in name order: "0", "Construction", "Doors", "Walls" (current)
    SECTION("with its lock icon: the first shown layer") {
        f.m_graphic.toggleLayerLock(f.m_walls);
    }
    SECTION("not a hidden layer, while a shown one is unlocked") {
        f.m_zero->freeze(true);
        f.m_graphic.toggleLayerLock(f.m_walls);
        expected = f.m_construction;
    }
    SECTION("a hidden layer, when every unlocked layer is hidden") {
        f.m_construction->freeze(true);
        f.m_doors->freeze(true);
        f.m_zero->lock(true);
        f.m_graphic.toggleLayerLock(f.m_walls);
        expected = f.m_construction;
        // nothing was shown for it
        CHECK(f.m_construction->isFrozen());
    }
    SECTION("as one of the layers selected in a dock") {
        f.m_graphic.toggleLockLayers({f.m_walls, f.m_zero});
        REQUIRE(f.m_zero->isLocked());
        expected = f.m_construction;
    }
    SECTION("as one of the layers a dock locks and unlocks") {
        f.m_graphic.setLockLayers({}, {f.m_zero, f.m_construction, f.m_walls});
        REQUIRE(f.m_zero->isLocked());
        REQUIRE(f.m_construction->isLocked());
        expected = f.m_doors;
    }
    SECTION("with Lock all layers, when a layer that the dock does not list is unlocked") {
        f.m_construction->visibleInLayerList(false);
        f.m_graphic.lockAllLayers(true);
        CHECK(f.m_zero->isLocked());
        CHECK(f.m_doors->isLocked());
        CHECK_FALSE(f.m_construction->isLocked());
        expected = f.m_construction;
    }

    CHECK(f.m_walls->isLocked());
    CHECK(f.m_graphic.getActiveLayer() == expected);
    CHECK_FALSE(expected->isLocked());
    CHECK(command.m_messages == QStringList{replacedMessage.arg(expected->getName())});
    // the listeners are told, but nothing selected is moved to the layer: that is what
    // activating a layer does when "Modify entities to the active layer" is on
    CHECK(f.m_activations.m_replaced == QList<RS_Layer*>{expected});
    CHECK(f.m_activations.m_activated.isEmpty());
}

TEST_CASE("The current layer is not locked when no other layer is unlocked", "[layers][active][3229][lock]") {
    CommandMessages command;

    SECTION("the only layer of a new drawing") {
        RS_Graphic graphic;
        graphic.initForNewDocument();
        RS_Layer* zero = graphic.findLayer(QStringLiteral("0"));
        REQUIRE(zero != nullptr);
        REQUIRE(graphic.getActiveLayer() == zero);
        graphic.toggleLayerLock(zero);
        CHECK_FALSE(zero->isLocked());
        CHECK(graphic.getActiveLayer() == zero);
        CHECK(command.m_messages == QStringList{QStringLiteral(
                  "Layer \"0\" was not locked: it is the current layer, and no other layer is unlocked to take its place.")});
    }

    SECTION("every other layer being locked") {
        Layers f;
        f.m_zero->lock(true);
        f.m_construction->lock(true);
        f.m_doors->lock(true);
        f.m_graphic.toggleLayerLock(f.m_walls);
        CHECK_FALSE(f.m_walls->isLocked());
        CHECK(f.m_graphic.getActiveLayer() == f.m_walls);
        CHECK(command.m_messages == QStringList{keptMessage});
        CHECK(f.m_activations.m_replaced.isEmpty());
    }

    SECTION("Lock all layers locks all the other layers") {
        Layers f;
        f.m_graphic.lockAllLayers(true);
        CHECK(f.m_zero->isLocked());
        CHECK(f.m_construction->isLocked());
        CHECK(f.m_doors->isLocked());
        CHECK_FALSE(f.m_walls->isLocked());
        CHECK(f.m_graphic.getActiveLayer() == f.m_walls);
        CHECK(command.m_messages == QStringList{keptMessage});
        // and unlocking them all still works
        f.m_graphic.lockAllLayers(false);
        CHECK_FALSE(f.m_zero->isLocked());
    }

    SECTION("locking all the layers with the Layer Tree's multiple lock") {
        Layers f;
        f.m_graphic.setLockLayers({}, {f.m_zero, f.m_construction, f.m_doors, f.m_walls});
        CHECK(f.m_zero->isLocked());
        CHECK_FALSE(f.m_walls->isLocked());
        CHECK(f.m_graphic.getActiveLayer() == f.m_walls);
        CHECK(command.m_messages == QStringList{keptMessage});
    }
}

TEST_CASE("Locking or unlocking other layers leaves the current layer alone", "[layers][active][3229][lock]") {
    Layers f;
    CommandMessages command;
    f.m_graphic.toggleLayerLock(f.m_doors);
    f.m_graphic.toggleLockLayers({f.m_zero, f.m_construction});
    f.m_graphic.toggleLayerLock(f.m_doors);
    f.m_graphic.setLockLayers({f.m_zero}, {f.m_doors});
    CHECK(f.m_graphic.getActiveLayer() == f.m_walls);
    CHECK_FALSE(f.m_walls->isLocked());
    CHECK(command.m_messages.isEmpty());
    CHECK(f.m_activations.m_replaced.isEmpty());
    CHECK(f.m_activations.m_activated.isEmpty());
}

TEST_CASE("Locking is offered exactly when it would lock a layer", "[layers][active][3229][lock][offer]") {
    // what the lock icons, the Locked box and Lock all ask before they offer to lock: the lock the
    // engine would refuse (the current layer, with no other layer unlocked) is not offered
    Layers f;
    RS_LayerList* list = f.m_graphic.getLayerList();
    REQUIRE(list != nullptr);
    CommandMessages command;

    SECTION("every layer can be locked while another layer is unlocked, the current layer too") {
        for (RS_Layer* layer : {f.m_zero, f.m_construction, f.m_doors, f.m_walls}) {
            CAPTURE(layer->getName());
            CHECK(list->canLock(layer));
            CHECK(list->canToggleLock(layer));
        }
        CHECK(list->canLockAll());
        CHECK(list->canToggleLockOfSelection());
        CHECK(list->canLockAny({f.m_walls}));
    }

    SECTION("the current layer is not offered when every other layer is locked") {
        for (RS_Layer* layer : {f.m_zero, f.m_construction, f.m_doors}) {
            layer->lock(true);
        }
        CHECK_FALSE(list->canLock(f.m_walls));
        CHECK_FALSE(list->canToggleLock(f.m_walls));
        CHECK_FALSE(list->canLockAny({f.m_walls}));
        CHECK_FALSE(list->canLockAny({f.m_walls, f.m_doors}));
        CHECK_FALSE(list->canLockAll());
        CHECK_FALSE(list->canToggleLockOfSelection());
        // a locked layer is not locked again, but it can be unlocked
        CHECK_FALSE(list->canLock(f.m_doors));
        CHECK(list->canToggleLock(f.m_doors));
        // and the engine does refuse it
        f.m_graphic.toggleLayerLock(f.m_walls);
        CHECK_FALSE(f.m_walls->isLocked());
        CHECK(command.m_messages == QStringList{keptMessage});
    }

    SECTION("it is offered again as soon as another layer is unlocked") {
        for (RS_Layer* layer : {f.m_zero, f.m_construction, f.m_doors}) {
            layer->lock(true);
        }
        REQUIRE_FALSE(list->canLock(f.m_walls));
        f.m_graphic.toggleLayerLock(f.m_zero);
        CHECK(list->canLock(f.m_walls));
        CHECK(list->canLockAll());
        CHECK(list->canToggleLockOfSelection());
    }

    SECTION("a hidden layer that is not locked can take the place of the current layer") {
        f.m_zero->lock(true);
        f.m_doors->lock(true);
        f.m_construction->freeze(true);
        CHECK(list->canLock(f.m_walls));
        f.m_graphic.toggleLayerLock(f.m_walls);
        CHECK(f.m_walls->isLocked());
        CHECK(f.m_graphic.getActiveLayer() == f.m_construction);
    }

    SECTION("a layer that is not the current one is offered whatever the others are") {
        f.m_construction->lock(true);
        f.m_doors->lock(true);
        REQUIRE_FALSE(f.m_zero->isLocked());
        CHECK(list->canLock(f.m_zero));
        CHECK(list->canLockAny({f.m_zero}));
        f.m_graphic.toggleLayerLock(f.m_zero);
        CHECK(f.m_zero->isLocked());
        CHECK(f.m_graphic.getActiveLayer() == f.m_walls);
    }

    SECTION("a drawing opened with every layer locked, the current layer too") {
        for (RS_Layer* layer : {f.m_zero, f.m_construction, f.m_doors, f.m_walls}) {
            layer->lock(true);
        }
        REQUIRE(f.m_graphic.getActiveLayer() == f.m_walls);
        // the current layer can be unlocked, and nothing else is offered to lock
        CHECK(list->canToggleLock(f.m_walls));
        CHECK_FALSE(list->canLock(f.m_walls));
        CHECK_FALSE(list->canLockAll());
        f.m_zero->lock(false);
        // a layer that is not the current one can be locked, as the engine does: the current layer is
        // locked already, so it is left as it is
        CHECK(list->canLock(f.m_zero));
        CHECK(list->canLockAll());
        f.m_graphic.toggleLayerLock(f.m_zero);
        CHECK(f.m_zero->isLocked());
        CHECK(command.m_messages.isEmpty());
    }

    SECTION("the only layer of a new drawing") {
        RS_Graphic graphic;
        graphic.initForNewDocument();
        RS_LayerList* only = graphic.getLayerList();
        RS_Layer* zero = graphic.findLayer(QStringLiteral("0"));
        REQUIRE(zero != nullptr);
        CHECK_FALSE(only->canLock(zero));
        CHECK_FALSE(only->canToggleLock(zero));
        CHECK_FALSE(only->canLockAll());
        CHECK_FALSE(only->canToggleLockOfSelection());
    }

    SECTION("a layer that is not in the list, and no layer") {
        RS_Layer stranger(QStringLiteral("Stranger"));
        CHECK_FALSE(list->canLock(&stranger));
        CHECK_FALSE(list->canToggleLock(&stranger));
        CHECK_FALSE(list->canLock(nullptr));
        CHECK_FALSE(list->canToggleLock(nullptr));
        CHECK_FALSE(list->canLockAny({}));
    }

    SECTION("the selected layers decide Toggle layer lock, else the current layer") {
        for (RS_Layer* layer : {f.m_zero, f.m_construction, f.m_doors}) {
            layer->lock(true);
        }
        // nothing selected: the current layer, which cannot be locked
        CHECK_FALSE(list->canToggleLockOfSelection());
        // the current layer alone selected
        f.m_walls->selectedInLayerList(true);
        CHECK_FALSE(list->canToggleLockOfSelection());
        // a locked layer selected with it: that one can be unlocked
        f.m_doors->selectedInLayerList(true);
        CHECK(list->canToggleLockOfSelection());
        // a selected layer the dock does not list, as a filter hides it, does not count
        f.m_doors->visibleInLayerList(false);
        CHECK_FALSE(list->canToggleLockOfSelection());
    }

    SECTION("Lock all is offered while it locks a layer") {
        f.m_zero->lock(true);
        f.m_construction->lock(true);
        f.m_doors->lock(true);
        REQUIRE_FALSE(list->canLockAll());
        f.m_doors->lock(false);
        CHECK(list->canLockAll());
        // the unlocked layer is one the dock does not list: Lock all locks the current layer, and
        // that layer takes its place
        f.m_doors->visibleInLayerList(false);
        CHECK(list->canLockAll());
        f.m_graphic.lockAllLayers(true);
        CHECK(f.m_walls->isLocked());
        CHECK(f.m_graphic.getActiveLayer() == f.m_doors);
        // nothing is left to lock: the current layer is the only one that is not
        CHECK_FALSE(list->canLockAll());
    }

    SECTION("the controls say why") {
        CHECK_FALSE(RS_LayerList::lockUnavailableText().isEmpty());
    }
}

TEST_CASE("A hidden current layer is locked like any other", "[layers][active][3229][lock]") {
    Layers f;
    CommandMessages command;
    f.m_walls->freeze(true);
    f.m_graphic.toggleLayerLock(f.m_walls);
    CHECK(f.m_walls->isLocked());
    CHECK(f.m_walls->isFrozen());
    CHECK(f.m_graphic.getActiveLayer() == f.m_zero);
    CHECK(command.m_messages == QStringList{replacedMessage.arg("0")});
}

TEST_CASE("Removing the current layer makes a layer current that can be, shown ones first", "[layers][active][3229]") {
    Layers f;
    // in name order: "0" hidden, "Construction" locked, "Doors" hidden, "Walls" current
    f.m_zero->freeze(true);
    f.m_construction->lock(true);
    f.m_doors->freeze(true);
    RS_Layer* expected = nullptr;

    SECTION("a shown layer that is not locked") {
        auto* shown = new RS_Layer(QStringLiteral("Roof"));
        f.m_graphic.addLayer(shown);
        expected = shown;
    }
    SECTION("else the first that is not locked, though hidden") {
        expected = f.m_zero;
    }

    REQUIRE(f.m_graphic.removeLayer(f.m_walls));
    CHECK(f.m_graphic.getActiveLayer() == expected);
    // nothing was shown or unlocked for it
    CHECK(f.m_zero->isFrozen());
    CHECK(f.m_construction->isLocked());
    CHECK(f.m_doors->isFrozen());
}

TEST_CASE("The current layer is not removed when no other layer is unlocked, nor is layer 0", "[layers][active][3229][remove]") {
    Layers f;
    CommandMessages command;
    const RS_LayerList* layers = f.m_graphic.getLayerList();
    const QString refused = QStringLiteral(
        "Layer \"Walls\" was not removed: it is the current layer, and no other layer is unlocked to take its place.");

    SECTION("layer 0 is never removed, and the command line says so") {
        CHECK_FALSE(layers->canRemove(f.m_zero));
        CHECK_FALSE(f.m_graphic.removeLayer(f.m_zero));
        CHECK(layers->contains(f.m_zero));
        CHECK(command.m_messages == QStringList{QStringLiteral("Layer \"0\" can never be removed.")});
    }
    SECTION("layer 0 is not removed though no other layer is current") {
        f.m_graphic.activateLayer(f.m_doors, true);
        CHECK_FALSE(f.m_graphic.removeLayer(f.m_zero));
        CHECK(layers->contains(f.m_zero));
    }
    SECTION("the current layer, every other layer being locked") {
        for (RS_Layer* layer : {f.m_zero, f.m_construction, f.m_doors}) {
            layer->lock(true);
        }
        CHECK_FALSE(layers->canRemove(f.m_walls));
        CHECK_FALSE(f.m_graphic.removeLayer(f.m_walls));
        CHECK(layers->contains(f.m_walls));
        CHECK(f.m_graphic.getActiveLayer() == f.m_walls);
        CHECK(command.m_messages == QStringList{refused});
        // a locked layer that is not current is removed, as it was before
        CHECK(layers->canRemove(f.m_doors));
        CHECK(f.m_graphic.removeLayer(f.m_doors));
        CHECK_FALSE(layers->contains(f.m_doors));
        CHECK(command.m_messages == QStringList{refused});
    }
    SECTION("the current layer, another being unlocked, hidden or not") {
        for (RS_Layer* layer : {f.m_zero, f.m_construction}) {
            layer->lock(true);
        }
        f.m_doors->freeze(true);
        CHECK(layers->canRemove(f.m_walls));
        CHECK(f.m_graphic.removeLayer(f.m_walls));
        CHECK(f.m_graphic.getActiveLayer() == f.m_doors);
        CHECK(command.m_messages.isEmpty());
    }
    SECTION("a locked layer is removed while another is unlocked") {
        f.m_doors->lock(true);
        CHECK(layers->canRemove(f.m_doors));
        CHECK(f.m_graphic.removeLayer(f.m_doors));
        CHECK_FALSE(layers->contains(f.m_doors));
        CHECK(command.m_messages.isEmpty());
    }
    SECTION("a drawing opened with every layer locked keeps its current layer") {
        for (RS_Layer* layer : {f.m_zero, f.m_construction, f.m_doors, f.m_walls}) {
            layer->lock(true);
        }
        CHECK_FALSE(layers->canRemove(f.m_walls));
        CHECK_FALSE(f.m_graphic.removeLayer(f.m_walls));
        CHECK(command.m_messages == QStringList{refused});
        CHECK(layers->canRemove(f.m_construction));
    }
    SECTION("nothing to remove") {
        RS_Layer stranger(QStringLiteral("Stranger"));
        CHECK_FALSE(layers->canRemove(nullptr));
        CHECK_FALSE(layers->canRemove(&stranger));
        CHECK_FALSE(f.m_graphic.removeLayer(nullptr));
        CHECK_FALSE(f.m_graphic.removeLayer(&stranger));
        CHECK(command.m_messages.isEmpty());
    }
}

TEST_CASE("A drawing opens with its saved current layer, unless it is locked and another is not", "[layers][active][3229][dxf]") {
    const QTemporaryDir directory;
    REQUIRE(directory.isValid());
    const QString path = directory.filePath(QStringLiteral("current.dxf"));
    bool hidden = false;
    bool locked = false;
    bool othersLocked = false;
    SECTION("hidden") {
        hidden = true;
    }
    SECTION("locked, another layer being unlocked") {
        locked = true;
    }
    SECTION("hidden and locked, another layer being unlocked") {
        hidden = true;
        locked = true;
    }
    SECTION("locked, every other layer being locked") {
        locked = true;
        othersLocked = true;
    }
    {
        Layers saved;
        saved.m_walls->freeze(hidden);
        saved.m_walls->lock(locked);
        if (othersLocked) {
            for (RS_Layer* layer : {saved.m_zero, saved.m_construction, saved.m_doors}) {
                layer->lock(true);
            }
        }
        REQUIRE(saved.m_graphic.getActiveLayer() == saved.m_walls);
        REQUIRE(RS_FileIO::instance()->fileExport(saved.m_graphic, path, RS2::FormatDXFRW));
    }

    const bool qtReady{lc::test::application() != nullptr};
    REQUIRE(qtReady);
    RS_Graphic opened;
    opened.initForNewDocument();
    // with a callback: without one, a failed import waits in a message box
    REQUIRE(RS_FileIO::instance()->fileImport(opened, path, RS2::FormatDXFRW,
                                              [](bool, const QString&) { return false; }));
    opened.onLoadingCompleted();
    RS_Layer* walls = opened.findLayer(QStringLiteral("Walls"));
    REQUIRE(walls != nullptr);
    CHECK(walls->isFrozen() == hidden);
    CHECK(walls->isLocked() == locked);
    if (locked && !othersLocked) {
        // AutoCAD lets a locked layer be current, and a file may say so: a locked layer is not current
        // here, so the layer that a removed current layer would give way to is
        CHECK(opened.getActiveLayer() == opened.findLayer(QStringLiteral("0")));
    }
    else {
        CHECK(opened.getActiveLayer() == walls);
    }
}

TEST_CASE("A saved locked current layer gives way to a layer that is not locked", "[layers][active][3229][lock]") {
    SECTION("restored from a drawing's header") {
        Layers f;
        f.m_walls->lock(true);
        f.m_graphic.restoreActiveLayer(f.m_walls);
        // "0" is the first layer that is shown and not locked
        CHECK(f.m_graphic.getActiveLayer() == f.m_zero);
        // the listeners are told of the layer that is current
        REQUIRE_FALSE(f.m_activations.m_activated.isEmpty());
        CHECK(f.m_activations.m_activated.constLast() == f.m_zero);
    }
    SECTION("restored, every other layer being locked") {
        Layers f;
        for (RS_Layer* layer : {f.m_zero, f.m_construction, f.m_doors, f.m_walls}) {
            layer->lock(true);
        }
        f.m_graphic.restoreActiveLayer(f.m_doors);
        CHECK(f.m_graphic.getActiveLayer() == f.m_doors);
    }
    SECTION("restored, hidden") {
        Layers f;
        f.m_doors->freeze(true);
        f.m_graphic.restoreActiveLayer(f.m_doors);
        CHECK(f.m_graphic.getActiveLayer() == f.m_doors);
    }
    SECTION("a drawing completed with a locked layer as the only current one it has had") {
        // loading adds the file's layers: the first one added is current, locked or not, until the
        // drawing says which is
        RS_Graphic graphic;
        graphic.clearLayers();
        auto* locked = new RS_Layer(QStringLiteral("Locked"));
        locked->lock(true);
        auto* free = new RS_Layer(QStringLiteral("Free"));
        graphic.addLayer(locked);
        graphic.addLayer(free);
        graphic.onLoadingCompleted();
        CHECK(graphic.getActiveLayer() == free);
    }
}
