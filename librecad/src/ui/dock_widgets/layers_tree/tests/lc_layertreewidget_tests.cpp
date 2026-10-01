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

#include <catch2/catch_test_macros.hpp>

#include <QAbstractItemModel>
#include <QItemSelectionModel>
#include <QString>
#include <QTreeView>

#include "lc_actiontestsupport.h"
#include "lc_layertreewidget.h"
#include "rs_color.h"
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
