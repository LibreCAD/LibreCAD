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

// Issue #3229, in the docks: clicking the name of a hidden layer in the Layer List or the Layer
// Tree makes it the current layer, which the dock shows as current while the layer stays hidden;
// clicking the name of a locked layer leaves the current layer and says why on the command line.
// The status bar names the current layer with "hidden" or "locked" when it is, whatever changed it.

#include <catch2/catch_test_macros.hpp>

#include <memory>

#include <QAbstractItemModel>
#include <QFont>
#include <QStringList>
#include <QTableView>
#include <QTreeView>

#include "lc_actiongroupmanager.h"
#include "lc_actiontestsupport.h"
#include "lc_layertreeitem.h"
#include "lc_layertreemodel.h"
#include "lc_layertreewidget.h"
#include "qg_activelayername.h"
#include "qg_layerwidget.h"
#include "rs_dialogfactory.h"
#include "rs_dialogfactoryinterface.h"
#include "rs_graphic.h"
#include "rs_layer.h"

namespace {

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

// "0", "Construction" (hidden), "Doors" (locked) and "Walls", with "Walls" current
struct Drawing {
    const bool m_qtReady{lc::test::application() != nullptr};
    RS_Graphic m_graphic;
    lc::test::TestGraphicView m_view;
    RS_Layer* m_construction = new RS_Layer(QStringLiteral("Construction"));
    RS_Layer* m_doors = new RS_Layer(QStringLiteral("Doors"));
    RS_Layer* m_walls = new RS_Layer(QStringLiteral("Walls"));

    Drawing() {
        m_graphic.initForNewDocument();
        for (RS_Layer* layer : {m_construction, m_doors, m_walls}) {
            m_graphic.addLayer(layer);
        }
        m_construction->freeze(true);
        m_doors->lock(true);
        m_graphic.activateLayer(m_walls);
        m_view.setDocument(&m_graphic);
    }
};

/// the index of the name of the layer in a Layer Tree model, which has no nesting for these names
QModelIndex treeNameIndex(const LC_LayerTreeModel& model, const RS_Layer* layer) {
    int nameColumn = -1;
    for (int column = 0; column < model.columnCount(QModelIndex{}); ++column) {
        if (model.translateColumn(column) == LC_LayerTreeModel::COLUMN_NAME) {
            nameColumn = column;
        }
    }
    for (int row = 0; row < model.rowCount(QModelIndex{}); ++row) {
        const QModelIndex index = model.index(row, nameColumn, QModelIndex{});
        const LC_LayerTreeItem* item = model.getItemForIndex(index);
        if (item != nullptr && item->getLayer() == layer) {
            return index;
        }
    }
    return {};
}

const QString lockedMessage = QStringLiteral("Layer \"Doors\" is locked: unlock it to make it the current layer.");

} // namespace

TEST_CASE("Clicking a hidden layer in the Layer List makes it current, still hidden", "[gui][layers][active][3229]") {
    Drawing d;
    LC_ActionGroupManager actions(nullptr);
    QG_LayerWidget widget(&actions, nullptr, nullptr);
    widget.setGraphicView(&d.m_view);
    auto* table = widget.findChild<QTableView*>();
    REQUIRE(table != nullptr);
    auto* model = dynamic_cast<QG_LayerModel*>(table->model());
    REQUIRE(model != nullptr);
    REQUIRE(model->getActiveLayer() == d.m_walls);
    CommandMessages command;

    const QModelIndex name = model->getIndex(d.m_construction);
    REQUIRE(name.column() == QG_LayerModel::COLUMN_NAME);
    widget.slotActivated(name);

    CHECK(d.m_graphic.getActiveLayer() == d.m_construction);
    CHECK(d.m_construction->isFrozen());
    CHECK(model->getActiveLayer() == d.m_construction);
    CHECK(table->currentIndex().row() == name.row());
    CHECK(model->data(name, Qt::FontRole).value<QFont>().bold());
    CHECK(command.m_messages.isEmpty());
}

TEST_CASE("Clicking a locked layer in the Layer List keeps the current layer and says why", "[gui][layers][active][3229]") {
    Drawing d;
    LC_ActionGroupManager actions(nullptr);
    QG_LayerWidget widget(&actions, nullptr, nullptr);
    widget.setGraphicView(&d.m_view);
    auto* table = widget.findChild<QTableView*>();
    REQUIRE(table != nullptr);
    auto* model = dynamic_cast<QG_LayerModel*>(table->model());
    REQUIRE(model != nullptr);
    CommandMessages command;

    widget.slotActivated(model->getIndex(d.m_doors));

    CHECK(d.m_graphic.getActiveLayer() == d.m_walls);
    CHECK(model->getActiveLayer() == d.m_walls);
    CHECK(d.m_doors->isLocked());
    CHECK(command.m_messages == QStringList{lockedMessage});
}

TEST_CASE("Clicking a hidden or locked layer in the Layer Tree", "[gui][layers][active][3229]") {
    Drawing d;
    LC_LayerTreeWidget widget(nullptr, nullptr);
    widget.setGraphicView(&d.m_view);
    auto* tree = widget.findChild<QTreeView*>();
    REQUIRE(tree != nullptr);
    auto* model = dynamic_cast<LC_LayerTreeModel*>(tree->model());
    REQUIRE(model != nullptr);
    CommandMessages command;

    SECTION("makes a hidden one current, still hidden") {
        const QModelIndex name = treeNameIndex(*model, d.m_construction);
        REQUIRE(name.isValid());
        widget.slotTreeClicked(name);
        CHECK(d.m_graphic.getActiveLayer() == d.m_construction);
        CHECK(d.m_construction->isFrozen());
        // the tree is rebuilt for the activation: look the row up again
        const QModelIndex shown = treeNameIndex(*model, d.m_construction);
        REQUIRE(shown.isValid());
        CHECK(model->getItemForIndex(shown)->isActiveLayer());
        CHECK(model->data(shown, Qt::FontRole).value<QFont>().bold());
        CHECK(command.m_messages.isEmpty());
    }
    SECTION("keeps the current layer for a locked one, and says why") {
        const QModelIndex name = treeNameIndex(*model, d.m_doors);
        REQUIRE(name.isValid());
        widget.slotTreeClicked(name);
        CHECK(d.m_graphic.getActiveLayer() == d.m_walls);
        const QModelIndex walls = treeNameIndex(*model, d.m_walls);
        REQUIRE(walls.isValid());
        CHECK(model->getItemForIndex(walls)->isActiveLayer());
        CHECK(command.m_messages == QStringList{lockedMessage});
    }
}

TEST_CASE("The status bar names the current layer, and says when it is hidden or locked", "[gui][layers][active][3229]") {
    auto d = std::make_unique<Drawing>();
    QG_ActiveLayerName label;
    label.setGraphicView(&d->m_view);
    const auto text = [&label] { return label.lActiveLayerName->text().toStdString(); };
    CHECK(text() == "Walls");

    d->m_graphic.toggleLayer(d->m_walls);
    CHECK(text() == "Walls (hidden)");
    CHECK(label.lActiveLayerName->toolTip().contains(QStringLiteral("hidden")));
    d->m_graphic.toggleLayerLock(d->m_walls);
    CHECK(text() == "Walls (hidden, locked)");
    d->m_graphic.toggleLayer(d->m_walls);
    CHECK(text() == "Walls (locked)");
    d->m_graphic.toggleLayerLock(d->m_walls);
    CHECK(text() == "Walls");

    d->m_graphic.activateChosenLayer(d->m_construction);
    CHECK(text() == "Construction (hidden)");
    // the layer made current in place of a removed one
    d->m_graphic.removeLayer(d->m_construction);
    CHECK(text() == "0");

    // the drawing goes with the label still attached to it
    d.reset();
    CHECK(text().empty());
    label.setGraphicView(nullptr);
    CHECK(text().empty());
}

TEST_CASE("The status bar names a layer the Layer List makes current as it is added", "[gui][layers][active][3229]") {
    // the Layer List makes an added layer current without telling the listeners; attached before
    // the status bar, as the main window attaches them, it does so before the status bar hears of
    // the new layer
    Drawing d;
    LC_ActionGroupManager actions(nullptr);
    QG_LayerWidget widget(&actions, nullptr, nullptr);
    widget.setGraphicView(&d.m_view);
    QG_ActiveLayerName label;
    label.setGraphicView(&d.m_view);
    REQUIRE(label.lActiveLayerName->text().toStdString() == "Walls");

    auto* roof = new RS_Layer(QStringLiteral("Roof"));
    roof->freeze(true);
    d.m_graphic.addLayer(roof);

    CHECK(d.m_graphic.getActiveLayer() == roof);
    CHECK(label.lActiveLayerName->text().toStdString() == "Roof (hidden)");
}
