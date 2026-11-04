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
// and what is drawn on it is hidden with it. A drawing's saved current layer is restored as it was
// saved.

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
#include "rs_layerlistlistener.h"
#include "rs_line.h"

namespace {

/// the layers the list said were activated
class ActivationRecorder final : public RS_LayerListListener {
public:
    QList<RS_Layer*> m_activated;

    void layerActivated(RS_Layer* layer) override {
        m_activated << layer;
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
    SECTION("else, all being locked, the first shown one") {
        f.m_zero->lock(true);
        f.m_doors->lock(true);
        expected = f.m_construction;
    }

    f.m_graphic.removeLayer(f.m_walls);
    CHECK(f.m_graphic.getActiveLayer() == expected);
    // nothing was shown or unlocked for it
    CHECK(f.m_zero->isFrozen());
    CHECK(f.m_construction->isLocked());
    CHECK(f.m_doors->isFrozen());
}

TEST_CASE("A drawing opens with its saved current layer, hidden or locked as saved", "[layers][active][3229][dxf]") {
    const QTemporaryDir directory;
    REQUIRE(directory.isValid());
    const QString path = directory.filePath(QStringLiteral("current.dxf"));
    bool hidden = false;
    bool locked = false;
    SECTION("hidden") {
        hidden = true;
    }
    SECTION("locked") {
        locked = true;
    }
    SECTION("hidden and locked") {
        hidden = true;
        locked = true;
    }
    {
        Layers saved;
        saved.m_walls->freeze(hidden);
        saved.m_walls->lock(locked);
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
    CHECK(opened.getActiveLayer() == walls);
    CHECK(walls->isFrozen() == hidden);
    CHECK(walls->isLocked() == locked);
}
