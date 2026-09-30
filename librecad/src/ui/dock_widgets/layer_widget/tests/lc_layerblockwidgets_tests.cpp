/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2026 LibreCAD (librecad.org)
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

// Issue #2969: LibreCAD crashed when started with a file on the command line, in the layer
// list's table view asking a model for data. The dock widgets list the layers and blocks of
// the drawing of the active window; loading a file resets that drawing (initForNewDocument),
// which frees its layers and blocks, and a widget attached to it must not keep naming them.
//
// A layer added to a drawing that a QG_LayerWidget shows is activated through the application
// window singleton, which a test must not create (its constructor builds the whole main window
// and changes process-wide state). A layer that is frozen is not activated: FreezeAdded freezes
// each layer as it is added, and is listed before the widget so it hears of the layer first.

#include <catch2/catch_test_macros.hpp>

#include <memory>

#include <QAbstractItemModel>
#include <QCoreApplication>
#include <QScrollBar>
#include <QTableView>
#include <QTreeView>

#include "lc_actiongroupmanager.h"
#include "lc_actiontestsupport.h"
#include "lc_layertreewidget.h"
#include "lc_propertiesprovider_graphic_layer.h"
#include "lc_property_container.h"
#include "lc_propertysheetwidget.h"
#include "qg_blockwidget.h"
#include "qg_layerwidget.h"
#include "qg_pentoolbar.h"
#include "rs_block.h"
#include "rs_graphic.h"
#include "rs_layer.h"
#include "rs_layerlistlistener.h"
#include "rs_line.h"

namespace {

/// freezes every layer as it is added, so that no widget activates it (see the header comment)
class FreezeAdded final : public RS_LayerListListener {
public:
    void layerAdded(RS_Layer* layer) override {
        layer->freeze(true);
    }
};

/// reads every row the way a view does; under ASan this fails on a freed layer or block
int readAllRows(const QAbstractItemModel& model) {
    int read = 0;
    for (int row = 0; row < model.rowCount(QModelIndex{}); ++row) {
        for (int column = 0; column < model.columnCount(QModelIndex{}); ++column) {
            for (const int role : {Qt::DisplayRole, Qt::DecorationRole, Qt::BackgroundRole, Qt::FontRole}) {
                if (model.data(model.index(row, column, QModelIndex{}), role).isValid()) {
                    ++read;
                }
            }
        }
    }
    return read;
}

void addBlock(RS_Graphic& graphic, const QString& name) {
    auto* block = new RS_Block(&graphic, RS_BlockData(name, RS_Vector{0, 0}, false));
    block->addEntity(new RS_Line(block, RS_LineData(RS_Vector{0, 0}, RS_Vector{1, 0})));
    graphic.addBlock(block);
}

RS_Layer* frozenLayer(const QString& name) {
    auto* layer = new RS_Layer(name);
    layer->freeze(true);
    return layer;
}

/// the model of the one table (or tree) view of a dock widget
template <typename View>
QAbstractItemModel* modelOf(const QWidget& widget) {
    auto* view = widget.findChild<View*>();
    return view != nullptr ? view->model() : nullptr;
}

}

TEST_CASE("A layer widget lists no layer of a drawing that is reset under it", "[gui][layers][2969]") {
    (void)lc::test::application();
    LC_ActionGroupManager actions(nullptr);
    RS_Graphic graphic;
    graphic.initForNewDocument();
    graphic.addLayer(new RS_Layer("Walls"));
    graphic.addLayer(new RS_Layer("Doors"));

    lc::test::TestGraphicView view;
    view.setDocument(&graphic);
    FreezeAdded freezer;
    graphic.addLayerListListener(&freezer);
    QG_LayerWidget widget(&actions, nullptr, nullptr);
    widget.setGraphicView(&view);
    QAbstractItemModel* model = modelOf<QTableView>(widget);
    REQUIRE(model != nullptr);
    REQUIRE(model->rowCount(QModelIndex{}) == 3);
    REQUIRE(readAllRows(*model) > 0);

    // what loading a file does first
    graphic.clearLayers();
    // the layers are gone: so are the rows (checked before anything reads them)
    REQUIRE(model->rowCount(QModelIndex{}) == 0);
    CHECK(readAllRows(*model) == 0);

    graphic.addLayer(new RS_Layer("0"));
    graphic.addLayer(new RS_Layer("Loaded"));
    REQUIRE(model->rowCount(QModelIndex{}) == 2);
    CHECK(readAllRows(*model) > 0);
    graphic.removeLayerListListener(&freezer);
}

TEST_CASE("A layer widget survives the drawing being initialised again", "[gui][layers][2969]") {
    (void)lc::test::application();
    LC_ActionGroupManager actions(nullptr);
    RS_Graphic graphic;
    graphic.initForNewDocument();
    graphic.addLayer(new RS_Layer("Walls"));

    lc::test::TestGraphicView view;
    view.setDocument(&graphic);
    FreezeAdded freezer;
    graphic.addLayerListListener(&freezer);
    QG_LayerWidget widget(&actions, nullptr, nullptr);
    widget.setGraphicView(&view);
    QAbstractItemModel* model = modelOf<QTableView>(widget);
    REQUIRE(model != nullptr);

    // a window is created for the file (with its own new drawing), the docks attach to it, and
    // the file is loaded into it: LC_DocumentsStorage::loadGraphic() begins with this
    for (int i = 0; i < 3; ++i) {
        graphic.initForNewDocument();
        REQUIRE(model->rowCount(QModelIndex{}) == 1);
        CHECK(readAllRows(*model) > 0);
    }
    graphic.removeLayerListListener(&freezer);
}

TEST_CASE("A layer tree lists no layer of a drawing that is reset under it", "[gui][layers][2969]") {
    (void)lc::test::application();
    RS_Graphic graphic;
    graphic.initForNewDocument();
    graphic.addLayer(new RS_Layer("Walls"));
    graphic.addLayer(new RS_Layer("Doors"));

    lc::test::TestGraphicView view;
    view.setDocument(&graphic);
    LC_LayerTreeWidget widget(nullptr, nullptr);
    widget.setGraphicView(&view);
    QAbstractItemModel* model = modelOf<QTreeView>(widget);
    REQUIRE(model != nullptr);
    REQUIRE(model->rowCount(QModelIndex{}) == 3);

    graphic.clearLayers();
    REQUIRE(model->rowCount(QModelIndex{}) == 0);

    graphic.addLayer(new RS_Layer("0"));
    graphic.addLayer(new RS_Layer("Loaded"));
    REQUIRE(model->rowCount(QModelIndex{}) == 2);
    // every row can be read (under ASan a freed layer fails here) and names its layer
    QStringList names;
    for (int row = 0; row < 2; ++row) {
        for (int column = 0; column < model->columnCount(QModelIndex{}); ++column) {
            const QString text = model->data(model->index(row, column, QModelIndex{}), Qt::DisplayRole).toString();
            if (!text.isEmpty()) {
                names << text;
            }
        }
    }
    names.sort();
    CHECK(names == QStringList{QStringLiteral("0"), QStringLiteral("Loaded")});
}

TEST_CASE("A block widget lists no block of a drawing that is reset under it", "[gui][blocks][2969]") {
    (void)lc::test::application();
    LC_ActionGroupManager actions(nullptr);
    RS_Graphic graphic;
    graphic.initForNewDocument();
    addBlock(graphic, "Door");
    addBlock(graphic, "Window");

    lc::test::TestGraphicView view;
    view.setDocument(&graphic);
    QG_BlockWidget widget(&actions, nullptr, nullptr);
    widget.setGraphicView(&view);
    QAbstractItemModel* model = modelOf<QTableView>(widget);
    REQUIRE(model != nullptr);
    REQUIRE(model->rowCount(QModelIndex{}) == 2);
    REQUIRE(readAllRows(*model) > 0);

    graphic.clearBlocks();
    REQUIRE(model->rowCount(QModelIndex{}) == 0);
    CHECK(readAllRows(*model) == 0);

    addBlock(graphic, "Loaded");
    REQUIRE(model->rowCount(QModelIndex{}) == 1);
    CHECK(readAllRows(*model) > 0);
}

TEST_CASE("Shown widgets attached to a drawing survive it being initialised again", "[gui][layers][blocks][2969]") {
    (void)lc::test::application();
    LC_ActionGroupManager actions(nullptr);
    RS_Graphic graphic;
    graphic.initForNewDocument();
    graphic.addLayer(new RS_Layer("Walls"));
    addBlock(graphic, "Door");

    lc::test::TestGraphicView view;
    view.setDocument(&graphic);
    FreezeAdded freezer;
    graphic.addLayerListListener(&freezer);
    QG_LayerWidget layers(&actions, nullptr, nullptr);
    layers.setGraphicView(&view);
    QG_BlockWidget blocks(&actions, nullptr, nullptr);
    blocks.setGraphicView(&view);
    layers.show();
    blocks.show();
    // the rows are sized and painted before the reset, as they are in a window that is on screen
    QCoreApplication::processEvents();

    graphic.initForNewDocument();
    // and again while the file's blocks and layers are added
    QCoreApplication::processEvents();
    addBlock(graphic, "Loaded");
    graphic.addLayer(new RS_Layer("Loaded"));
    QCoreApplication::processEvents();

    QAbstractItemModel* layerModel = modelOf<QTableView>(layers);
    QAbstractItemModel* blockModel = modelOf<QTableView>(blocks);
    REQUIRE(layerModel != nullptr);
    REQUIRE(blockModel != nullptr);
    CHECK(layerModel->rowCount(QModelIndex{}) == 2);
    CHECK(blockModel->rowCount(QModelIndex{}) == 1);
    graphic.removeLayerListListener(&freezer);
}

TEST_CASE("A layer widget keeps its scroll position when it is updated", "[gui][layers][2969]") {
    (void)lc::test::application();
    LC_ActionGroupManager actions(nullptr);
    RS_Graphic graphic;
    graphic.initForNewDocument();
    for (int i = 0; i < 80; ++i) {
        graphic.addLayer(frozenLayer(QStringLiteral("Layer %1").arg(i, 3, 10, QLatin1Char('0'))));
    }

    lc::test::TestGraphicView view;
    view.setDocument(&graphic);
    QG_LayerWidget widget(&actions, nullptr, nullptr);
    widget.resize(220, 240);
    widget.setGraphicView(&view);
    widget.show();
    QCoreApplication::processEvents();

    auto* table = widget.findChild<QTableView*>();
    REQUIRE(table != nullptr);
    QScrollBar* bar = table->verticalScrollBar();
    REQUIRE(bar->maximum() > 0);
    const int scrolled = bar->maximum() / 2;
    bar->setValue(scrolled);
    REQUIRE(bar->value() == scrolled);

    widget.updateWidget();
    CHECK(bar->value() == scrolled);
}

TEST_CASE("The property sheet fills for a drawing with no active layer", "[gui][properties][2969]") {
    // between clearing a drawing's layers and adding the file's there is no active layer, and the
    // sheet is told the layers changed in between: it dereferenced the active layer
    (void)lc::test::application();
    LC_ActionGroupManager actions(nullptr);
    RS_Graphic graphic;
    graphic.initForNewDocument();
    lc::test::TestGraphicView view;
    view.setDocument(&graphic);
    LC_ActionContext context;
    context.setDocumentAndView(&graphic, &view);
    LC_PropertySheetWidget sheet(nullptr, &context, &actions);
    LC_PropertiesProviderGraphicLayer provider(&context, &sheet);

    graphic.clearLayers();
    REQUIRE(graphic.getActiveLayer() == nullptr);
    LC_PropertyContainer container;
    provider.fillDocumentProperties(&container, &graphic);
    // nothing to show about a layer that does not exist
    CHECK_FALSE(container.hasChildProperties());
}

// --- the drawing is destroyed while a dock is still attached to it (the destructors of the lists)

TEST_CASE("A layer widget outlives the drawing it shows", "[gui][layers][2969]") {
    (void)lc::test::application();
    LC_ActionGroupManager actions(nullptr);
    QG_LayerWidget widget(&actions, nullptr, nullptr); // first, so that it is destroyed after the drawings
    auto graphic = std::make_unique<RS_Graphic>();
    graphic->initForNewDocument();
    graphic->addLayer(frozenLayer(QStringLiteral("Walls")));
    graphic->addLayer(frozenLayer(QStringLiteral("Doors")));
    lc::test::TestGraphicView view;
    view.setDocument(graphic.get());
    widget.setGraphicView(&view);
    QAbstractItemModel* model = modelOf<QTableView>(widget);
    REQUIRE(model != nullptr);
    REQUIRE(model->rowCount(QModelIndex{}) == 3);

    // the drawing goes with the widget still attached to it (the view is not used again)
    graphic.reset();
    REQUIRE(model->rowCount(QModelIndex{}) == 0);
    CHECK(readAllRows(*model) == 0);
    // typing in the filter has no drawing to filter
    widget.slotUpdateLayerList();
    // and activating a layer has no drawing to activate it in (it would reach the freed one)
    RS_Layer stray(QStringLiteral("Stray"));
    widget.activateLayer(&stray);
    // detaching finds nothing of the destroyed list to unregister from
    widget.setGraphicView(nullptr);
    CHECK(model->rowCount(QModelIndex{}) == 0);

    // and the widget shows another drawing
    RS_Graphic second;
    second.initForNewDocument();
    second.addLayer(frozenLayer(QStringLiteral("Roof")));
    lc::test::TestGraphicView secondView;
    secondView.setDocument(&second);
    widget.setGraphicView(&secondView);
    REQUIRE(model->rowCount(QModelIndex{}) == 2);
    CHECK(readAllRows(*model) > 0);
    // (the second drawing is destroyed before the widget, still attached to it)
}

TEST_CASE("A layer tree outlives the drawing it shows", "[gui][layers][2969]") {
    (void)lc::test::application();
    LC_LayerTreeWidget widget(nullptr, nullptr);
    auto graphic = std::make_unique<RS_Graphic>();
    graphic->initForNewDocument();
    graphic->addLayer(new RS_Layer("Walls"));
    graphic->addLayer(new RS_Layer("Doors"));
    lc::test::TestGraphicView view;
    view.setDocument(graphic.get());
    widget.setGraphicView(&view);
    QAbstractItemModel* model = modelOf<QTreeView>(widget);
    REQUIRE(model != nullptr);
    REQUIRE(model->rowCount(QModelIndex{}) == 3);

    graphic.reset();
    REQUIRE(model->rowCount(QModelIndex{}) == 0);
    // the buttons that act on all layers have no drawing to act on (they read the freed one)
    widget.showAllLayers();
    widget.hideAllLayers();
    widget.setGraphicView(nullptr);
    CHECK(model->rowCount(QModelIndex{}) == 0);

    RS_Graphic second;
    second.initForNewDocument();
    second.addLayer(new RS_Layer("Roof"));
    lc::test::TestGraphicView secondView;
    secondView.setDocument(&second);
    widget.setGraphicView(&secondView);
    CHECK(model->rowCount(QModelIndex{}) == 2);
}

TEST_CASE("A block widget outlives the drawing it shows", "[gui][blocks][2969]") {
    (void)lc::test::application();
    LC_ActionGroupManager actions(nullptr);
    QG_BlockWidget widget(&actions, nullptr, nullptr);
    auto graphic = std::make_unique<RS_Graphic>();
    graphic->initForNewDocument();
    addBlock(*graphic, "Door");
    addBlock(*graphic, "Window");
    lc::test::TestGraphicView view;
    view.setDocument(graphic.get());
    widget.setGraphicView(&view);
    QAbstractItemModel* model = modelOf<QTableView>(widget);
    REQUIRE(model != nullptr);
    REQUIRE(model->rowCount(QModelIndex{}) == 2);
    REQUIRE(readAllRows(*model) > 0);

    graphic.reset();
    REQUIRE(model->rowCount(QModelIndex{}) == 0);
    CHECK(readAllRows(*model) == 0);
    CHECK(widget.getBlockList() == nullptr);
    // a stray block has no drawing to be activated in
    RS_Block stray(nullptr, RS_BlockData(QStringLiteral("Stray"), RS_Vector{0, 0}, false));
    widget.activateBlock(&stray);
    widget.setGraphicView(nullptr);
    CHECK(model->rowCount(QModelIndex{}) == 0);

    RS_Graphic second;
    second.initForNewDocument();
    addBlock(second, "Roof");
    lc::test::TestGraphicView secondView;
    secondView.setDocument(&second);
    widget.setGraphicView(&secondView);
    REQUIRE(model->rowCount(QModelIndex{}) == 1);
    CHECK(readAllRows(*model) > 0);
}

TEST_CASE("A pen tool bar outlives the drawing it follows", "[gui][layers][2969]") {
    (void)lc::test::application();
    QG_PenToolBar bar(QStringLiteral("Pen"));
    auto graphic = std::make_unique<RS_Graphic>();
    graphic->initForNewDocument();
    lc::test::TestGraphicView view;
    view.setDocument(graphic.get());
    bar.setGraphicView(&view);

    graphic.reset();
    // detaching finds nothing of the destroyed list to unregister from
    bar.setGraphicView(nullptr);

    RS_Graphic second;
    second.initForNewDocument();
    lc::test::TestGraphicView secondView;
    secondView.setDocument(&second);
    bar.setGraphicView(&secondView);
}

TEST_CASE("Widgets destroyed before the drawing they show leave nothing registered on it", "[gui][layers][blocks][2969]") {
    (void)lc::test::application();
    LC_ActionGroupManager actions(nullptr);
    RS_Graphic graphic;
    graphic.initForNewDocument();
    addBlock(graphic, "Door");
    lc::test::TestGraphicView view;
    view.setDocument(&graphic);
    const int layerListeners = graphic.getLayerList()->listenerCount();
    const int blockListeners = graphic.getBlockList()->listenerCount();
    {
        QG_LayerWidget layers(&actions, nullptr, nullptr);
        LC_LayerTreeWidget tree(nullptr, nullptr);
        QG_BlockWidget blocks(&actions, nullptr, nullptr);
        QG_PenToolBar pen(QStringLiteral("Pen"));
        layers.setGraphicView(&view);
        tree.setGraphicView(&view);
        blocks.setGraphicView(&view);
        pen.setGraphicView(&view);
        // the layer widget, the tree and the tool bar listen to the layers, the block widget to the blocks
        CHECK(graphic.getLayerList()->listenerCount() == layerListeners + 3);
        CHECK(graphic.getBlockList()->listenerCount() == blockListeners + 1);
    }
    // each unregistered when it was destroyed: the lists hold none of them
    CHECK(graphic.getLayerList()->listenerCount() == layerListeners);
    CHECK(graphic.getBlockList()->listenerCount() == blockListeners);
    // so clearing the lists, and destroying them at the end of the test, reach no destroyed widget
    // (the tool bar has nothing else to observe: under ASan it fails if one is still listed)
    graphic.clearLayers();
    graphic.clearBlocks();
    graphic.addLayer(new RS_Layer("0"));
    addBlock(graphic, "Window");
}
