/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2026 LibreCAD.org
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

// LC_LayerTreeModel owns its options and its root item, with the tree of items
// below it. It freed a root only when setLayerList() replaced it, and never the
// options: every model that was destroyed left its last tree and its options
// behind, and so did every layer tree widget, which holds one.

#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <memory>
#include <optional>

#include <QAbstractItemModel>
#include <QString>
#include <QTreeView>

#include "lc_actiontestsupport.h"
#include "lc_heapprobe.h"
#include "lc_layertreemodel.h"
#include "lc_layertreemodel_options.h"
#include "lc_layertreewidget.h"
#include "rs_graphic.h"
#include "rs_layer.h"
#include "rs_layerlist.h"

namespace {

// With the default naming options these make a tree three levels deep: the
// groups "Walls" and "Walls-Outer", and secondary layers below "Doors" and
// "Walls-Inner".
void addTreeLayers(RS_LayerList& layers) {
    for (const char* name : {"Doors", "Doors+", "Walls-Inner", "Walls-Inner_meta", "Walls-Outer-North",
                             "Walls-Outer-South_pos"}) {
        layers.add(new RS_Layer(QString::fromLatin1(name)));
    }
}

// The rows of the model at every level.
int rowsBelow(const QAbstractItemModel& model, const QModelIndex& parent = {}) {
    const int rows = model.rowCount(parent);
    int total = rows;
    for (int row = 0; row < rows; ++row) {
        total += rowsBelow(model, model.index(row, 0, parent));
    }
    return total;
}

} // namespace

TEST_CASE("A layer tree model leaves nothing behind", "[gui][layers][model][leak]") {
    (void)lc::test::application();
    RS_LayerList layers;
    addTreeLayers(layers);

    // The tree is built twice: the first root is replaced, the second one is
    // still held when the model is destroyed.
    const auto buildAndDrop = [&layers] {
        LC_LayerTreeModel model(nullptr, std::make_unique<LC_LayerTreeModelOptions>());
        model.setLayerList(&layers);
        model.setLayerList(&layers);
        return rowsBelow(model);
    };
    REQUIRE(buildAndDrop() == 8); // six layers and the two groups; warms up the icons

    const std::optional<std::size_t> before = lc::test::heapBytesInUse();
    for (int i = 0; i < 100; ++i) {
        buildAndDrop();
    }
    const std::optional<std::size_t> after = lc::test::heapBytesInUse();

    if (before.has_value() && after.has_value()) {
        CHECK(*after <= *before);
    }
}

// The widget is where the application builds the model, and its options.
TEST_CASE("A layer tree widget leaves nothing of its model behind", "[gui][layers][model][leak]") {
    (void)lc::test::application();
    RS_Graphic graphic;
    graphic.initForNewDocument();
    addTreeLayers(*graphic.getLayerList());
    lc::test::TestGraphicView view;
    view.setDocument(&graphic);

    const auto buildAndDrop = [&view] {
        LC_LayerTreeWidget tree(nullptr, nullptr);
        tree.setGraphicView(&view);
        const auto* treeView = tree.findChild<QTreeView*>();
        return treeView != nullptr ? rowsBelow(*treeView->model()) : -1;
    };
    // A row for each layer, the drawing's own ones too, and for the two groups; warms up the style.
    REQUIRE(buildAndDrop() == static_cast<int>(graphic.getLayerList()->count()) + 2);

    const std::optional<std::size_t> before = lc::test::heapBytesInUse();
    for (int i = 0; i < 20; ++i) {
        buildAndDrop();
    }
    const std::optional<std::size_t> after = lc::test::heapBytesInUse();

    if (before.has_value() && after.has_value()) {
        CHECK(*after <= *before);
    }
}
