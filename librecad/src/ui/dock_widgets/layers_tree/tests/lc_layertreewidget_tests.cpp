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

// Copying a layer in the layer tree copies its pen, linetype name included.
// "Hide all layers except current" keeps the current layer under a filter too.

#include <catch2/catch_test_macros.hpp>

#include <QAbstractItemModel>
#include <QCheckBox>
#include <QItemSelectionModel>
#include <QLineEdit>
#include <QString>
#include <QTreeView>

#include "lc_actiontestsupport.h"
#include "lc_layertreewidget.h"
#include "rs_color.h"
#include "rs_graphic.h"
#include "rs_layer.h"
#include "rs_layerlist.h"
#include "rs_pen.h"

namespace {

// Opens up the context menu's "copy layer" command.
class LayerTree final : public LC_LayerTreeWidget {
public:
    LayerTree() : LC_LayerTreeWidget(nullptr, nullptr) {}
    using LC_LayerTreeWidget::createLayerCopy;
};

} // namespace

TEST_CASE("Copying a layer keeps its linetype name", "[gui][layers][linetype]") {
    (void)lc::test::application();
    RS_Graphic graphic;
    graphic.initForNewDocument();
    RS_Pen pen(RS_Color(Qt::red), RS2::Width05, RS2::SolidLine);
    pen.setLineTypeName(QStringLiteral("VENDOR_TAB"));
    auto *source = new RS_Layer(QStringLiteral("NAMED"));
    source->setPen(pen);
    graphic.addLayer(source);

    lc::test::TestGraphicView view;
    view.setDocument(&graphic);
    LayerTree tree;
    tree.setGraphicView(&view);

    auto *treeView = tree.findChild<QTreeView *>();
    REQUIRE(treeView != nullptr);
    const QAbstractItemModel *model = treeView->model();
    const QModelIndexList hits = model->match(model->index(0, 0), Qt::UserRole, source->getName(), 1,
                                              Qt::MatchExactly | Qt::MatchRecursive);
    REQUIRE(hits.size() == 1);
    // As a click does: only the name cell of the row is selectable.
    treeView->selectionModel()->select(hits.first(),
                                       QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    REQUIRE(treeView->selectionModel()->selectedIndexes().size() == 1);
    tree.createLayerCopy();

    RS_Layer *copy = nullptr;
    for (RS_Layer *layer : *graphic.getLayerList()) {
        if (layer != source && layer->getName() != QStringLiteral("0")) {
            copy = layer;
        }
    }
    REQUIRE(copy != nullptr);
    CHECK(copy->getPen().getLineTypeId() == pen.getLineTypeId());
    CHECK(copy->getPen().getLineTypeName() == QStringLiteral("VENDOR_TAB"));
    CHECK(copy->getPen() == pen);
}

// In filter mode the tree holds the matching layers only. Hiding them all hid
// the current one as well, and the layer list then made a layer the filter
// had left visible the active one.
TEST_CASE("Hiding all layers of the layer tree keeps the current layer", "[gui][layers][active]") {
    (void)lc::test::application();
    RS_Graphic graphic;
    graphic.initForNewDocument();
    RS_Layer *zero = graphic.findLayer(QStringLiteral("0"));
    REQUIRE(zero != nullptr);
    auto *doors = new RS_Layer(QStringLiteral("Doors"));
    auto *wall1 = new RS_Layer(QStringLiteral("Wall1"));
    auto *wall2 = new RS_Layer(QStringLiteral("Wall2"));
    for (RS_Layer *layer : {doors, wall1, wall2}) {
        graphic.addLayer(layer);
    }

    graphic.activateLayer(wall1);
    REQUIRE(graphic.getActiveLayer() == wall1);

    lc::test::TestGraphicView view;
    view.setDocument(&graphic);
    LC_LayerTreeWidget tree(nullptr, nullptr);
    tree.setGraphicView(&view);

    auto *filter = tree.findChild<QLineEdit *>();
    auto *highlightMode = tree.findChild<QCheckBox *>();
    REQUIRE(filter != nullptr);
    REQUIRE(highlightMode != nullptr);
    REQUIRE(highlightMode->isChecked());

    SECTION("without a filter every other layer is hidden") {
        tree.hideAllLayers();
        CHECK(graphic.getActiveLayer() == wall1);
        CHECK_FALSE(wall1->isFrozen());
        CHECK(wall2->isFrozen());
        CHECK(doors->isFrozen());
        CHECK(zero->isFrozen());
    }

    SECTION("a highlighting filter hides the same layers") {
        filter->setText(QStringLiteral("Wall"));
        tree.hideAllLayers();
        CHECK(graphic.getActiveLayer() == wall1);
        CHECK_FALSE(wall1->isFrozen());
        CHECK(wall2->isFrozen());
        CHECK(doors->isFrozen());
        CHECK(zero->isFrozen());
    }

    SECTION("a filter that removes layers from the tree leaves those as they are") {
        highlightMode->click();
        REQUIRE_FALSE(highlightMode->isChecked());
        filter->setText(QStringLiteral("Wall"));
        tree.hideAllLayers();
        CHECK(graphic.getActiveLayer() == wall1);
        CHECK_FALSE(wall1->isFrozen());
        CHECK(wall2->isFrozen());
        CHECK_FALSE(doors->isFrozen());
        CHECK_FALSE(zero->isFrozen());
    }
}
