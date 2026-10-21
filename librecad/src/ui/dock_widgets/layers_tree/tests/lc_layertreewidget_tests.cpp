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

// Copying a layer in the layer tree copies its pen, linetype name included.
// "Hide all layers except current" keeps the current layer under a filter too.

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <QAbstractButton>
#include <QAbstractItemModel>
#include <QCheckBox>
#include <QItemSelectionModel>
#include <QLineEdit>
#include <QMessageBox>
#include <QString>
#include <QTimer>
#include <QTreeView>

#include "lc_actiontestsupport.h"
#include "lc_layertreewidget.h"
#include "rs_color.h"
#include "rs_graphic.h"
#include "rs_layer.h"
#include "rs_layerlist.h"
#include "rs_line.h"
#include "rs_pen.h"

namespace {

// Expose the context-menu commands without changing the widget's API.
class LayerTree final : public LC_LayerTreeWidget {
public:
    LayerTree() : LC_LayerTreeWidget(nullptr, nullptr) {}
    using LC_LayerTreeWidget::createLayerCopy;
    using LC_LayerTreeWidget::createLayerDuplicate;
    using LC_LayerTreeWidget::duplicateSelectionToLayer;
    using LC_LayerTreeWidget::moveSelectionToLayer;
};

void selectLayer(LC_LayerTreeWidget& tree, const RS_Layer* layer) {
    auto* treeView = tree.findChild<QTreeView*>();
    REQUIRE(treeView != nullptr);
    const QAbstractItemModel* model = treeView->model();
    REQUIRE(model != nullptr);
    const QModelIndexList hits = model->match(model->index(0, 0), Qt::UserRole, layer->getName(), 1,
                                              Qt::MatchExactly | Qt::MatchRecursive);
    REQUIRE(hits.size() == 1);
    treeView->selectionModel()->select(hits.first(),
                                       QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    REQUIRE(treeView->selectionModel()->selectedIndexes().size() == 1);
}

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

TEST_CASE("Moving or duplicating a selection uses the chosen layer and pen policy", "[gui][layers][3100]") {
    (void)lc::test::application();
    const bool duplicate = GENERATE(false, true);
    const bool resolvePens = GENERATE(false, true);
    const bool targetIsActive = GENERATE(false, true);
    CAPTURE(duplicate, resolvePens, targetIsActive);

    RS_Graphic graphic;
    graphic.initForNewDocument();
    auto* source = new RS_Layer(QStringLiteral("SOURCE"));
    source->setPen(RS_Pen(RS_Color(Qt::red), RS2::Width05, RS2::DashLine));
    graphic.addLayer(source);
    auto* target = new RS_Layer(QStringLiteral("TARGET"));
    target->setPen(RS_Pen(RS_Color(Qt::blue), RS2::Width03, RS2::DotLine));
    graphic.addLayer(target);
    RS_Layer* active = targetIsActive ? target : graphic.findLayer(QStringLiteral("0"));
    graphic.activateLayer(active);
    graphic.setActivePen(RS_Pen(RS_Color(Qt::green), RS2::Width10, RS2::SolidLine));

    const RS_Pen pens[] = {
        RS_Pen(RS_Color(80, 90, 100), RS2::Width09, RS2::DashDotLine),
        RS_Pen(RS2::FlagByLayer, RS2::WidthByLayer, RS2::LineByLayer),
        RS_Pen(RS_Color(RS2::FlagByLayer), RS2::Width07, RS2::LineByLayer)
    };
    QList<RS_Entity*> originals;
    QList<RS_Pen> expected;
    for (int i = 0; i < 3; ++i) {
        auto* line = new RS_Line(&graphic, RS_LineData(RS_Vector{0., double(i)}, RS_Vector{10., double(i)}));
        line->setLayer(source);
        line->setPen(pens[i]);
        graphic.addEntity(line);
        originals.append(line);
        expected.append(resolvePens ? line->getPen(true) : pens[i]);
    }
    graphic.select(originals);

    lc::test::TestGraphicView view;
    view.setDocument(&graphic);
    LayerTree tree;
    tree.setGraphicView(&view);
    selectLayer(tree, target);

    bool answered = false;
    QTimer::singleShot(0, &tree, [&] {
        if (auto* question = tree.findChild<QMessageBox*>()) {
            answered = true;
            question->button(resolvePens ? QMessageBox::Yes : QMessageBox::No)->click();
        }
    });
    if (duplicate) {
        tree.duplicateSelectionToLayer();
    }
    else {
        tree.moveSelectionToLayer();
    }
    REQUIRE(answered);
    REQUIRE(graphic.count() == 6);
    CHECK(graphic.getActiveLayer() == active);

    QList<RS_Entity*> copies;
    for (int i = 0; i < originals.size(); ++i) {
        RS_Entity* copy = nullptr;
        for (RS_Entity* entity : graphic) {
            if (entity != originals[i] && entity->getStartpoint() == originals[i]->getStartpoint()) {
                copy = entity;
            }
        }
        REQUIRE(copy != nullptr);
        CHECK(copy->getLayer(false) == target);
        CHECK(copy->getPen(false) == expected[i]);
        CHECK(copy->isSelected());
        CHECK(originals[i]->getLayer(false) == source);
        CHECK(originals[i]->getPen(false) == pens[i]);
        CHECK(originals[i]->isDeleted() == !duplicate);
        copies.append(copy);
    }

    REQUIRE(graphic.undo());
    for (int i = 0; i < originals.size(); ++i) {
        CHECK_FALSE(originals[i]->isDeleted());
        CHECK(copies[i]->isDeleted());
    }
    REQUIRE(graphic.redo());
    for (int i = 0; i < copies.size(); ++i) {
        CHECK_FALSE(copies[i]->isDeleted());
        CHECK(copies[i]->getLayer(false) == target);
        CHECK(copies[i]->getPen(false) == expected[i]);
        CHECK(originals[i]->isDeleted() == !duplicate);
    }
}

TEST_CASE("Duplicating a whole layer preserves its entities' pens", "[gui][layers][3100]") {
    (void)lc::test::application();
    RS_Graphic graphic;
    graphic.initForNewDocument();
    auto* source = new RS_Layer(QStringLiteral("SOURCE"));
    source->setPen(RS_Pen(RS_Color(Qt::red), RS2::Width05, RS2::DashLine));
    graphic.addLayer(source);
    graphic.setActivePen(RS_Pen(RS_Color(Qt::green), RS2::Width10, RS2::SolidLine));
    const RS_Pen pen(RS_Color(Qt::blue), RS2::Width09, RS2::DotLine);
    auto* line = new RS_Line(&graphic, RS_LineData(RS_Vector{0., 0.}, RS_Vector{10., 0.}));
    line->setLayer(source);
    line->setPen(pen);
    graphic.addEntity(line);

    lc::test::TestGraphicView view;
    view.setDocument(&graphic);
    LayerTree tree;
    tree.setGraphicView(&view);
    selectLayer(tree, source);
    tree.createLayerDuplicate();

    RS_Layer* copyLayer = nullptr;
    for (RS_Layer* layer : *graphic.getLayerList()) {
        if (layer != source && layer->getName() != QStringLiteral("0")) {
            copyLayer = layer;
        }
    }
    REQUIRE(copyLayer != nullptr);
    REQUIRE(graphic.count() == 2);
    CHECK(graphic.entityAt(1)->getLayer(false) == copyLayer);
    CHECK(graphic.entityAt(1)->getPen(false) == pen);
    CHECK(line->getLayer(false) == source);
    CHECK(line->getPen(false) == pen);
}
