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
// The status bar names the current layer with the Layer List's icon for hidden or locked when it is,
// whatever changed it; locking the current layer makes another layer current in the docks too.

#include <catch2/catch_test_macros.hpp>

#include <memory>
#include <utility>

#include <QAbstractItemModel>
#include <QApplication>
#include <QContextMenuEvent>
#include <QMenu>
#include <QMessageBox>
#include <QMouseEvent>
#include <QTimer>
#include <QAction>
#include <QFont>
#include <QItemSelectionModel>
#include <QLineEdit>
#include <QStringList>
#include <QTableView>
#include <QToolButton>
#include <QTreeView>

#include "lc_actiongroupmanager.h"
#include "lc_actiontestsupport.h"
#include "lc_layertreeitem.h"
#include "lc_layertreemodel.h"
#include "lc_layertreewidget.h"
#include "qg_actionhandler.h"
#include "qg_activelayername.h"
#include "qg_layerwidget.h"
#include "rs_dialogfactory.h"
#include "rs_dialogfactoryinterface.h"
#include "rs_graphic.h"
#include "rs_layer.h"
#include "rs_layerlist.h"

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

int treeColumn(const LC_LayerTreeModel& model, const int columnKind) {
    int column = -1;
    for (int c = 0; c < model.columnCount(QModelIndex{}); ++c) {
        if (model.translateColumn(c) == columnKind) {
            column = c;
        }
    }
    return column;
}

/// the index of the cell of that kind of the group (a virtual layer, as "Group" is for the layers
/// "Group-Walls" and "Group-Doors") in a Layer Tree model
QModelIndex treeGroupIndex(const LC_LayerTreeModel& model, const QString& group, const int columnKind) {
    const int column = treeColumn(model, columnKind);
    for (int row = 0; row < model.rowCount(QModelIndex{}); ++row) {
        const QModelIndex index = model.index(row, column, QModelIndex{});
        LC_LayerTreeItem* item = model.getItemForIndex(index);
        if (item != nullptr && item->isVirtual() && item->getName() == group) {
            return index;
        }
    }
    return {};
}

/// the index of the layer's cell of that kind in a Layer Tree model, which has no nesting for these names
QModelIndex treeIndex(const LC_LayerTreeModel& model, const RS_Layer* layer, const int columnKind) {
    const int column = treeColumn(model, columnKind);
    for (int row = 0; row < model.rowCount(QModelIndex{}); ++row) {
        const QModelIndex index = model.index(row, column, QModelIndex{});
        const LC_LayerTreeItem* item = model.getItemForIndex(index);
        if (item != nullptr && item->getLayer() == layer) {
            return index;
        }
    }
    return {};
}

QModelIndex treeNameIndex(const LC_LayerTreeModel& model, const RS_Layer* layer) {
    return treeIndex(model, layer, LC_LayerTreeModel::COLUMN_NAME);
}

const QString lockedMessage = QStringLiteral("Layer \"Doors\" is locked: unlock it to make it the current layer.");
/// what a click on the disabled lock icon of "Walls", the current layer, says
const QString unavailableMessage = QStringLiteral(
    "Layer \"Walls\" was not locked: it is the current layer, and no other layer is unlocked to take its place.");

/// the Layer Tree's button of that tool tip
QToolButton* treeButton(const LC_LayerTreeWidget& widget, const QString& toolTip) {
    for (QToolButton* button : widget.findChildren<QToolButton*>()) {
        if (button->toolTip() == toolTip) {
            return button;
        }
    }
    return nullptr;
}

/// locks every layer but "Walls", the current layer, which no lock can then take: "0", "Construction"
/// and "Doors"
void lockAllButWalls(Drawing& d) {
    RS_Layer* zero = d.m_graphic.findLayer(QStringLiteral("0"));
    d.m_graphic.setLockLayers({}, {zero, d.m_construction});
}

/// What a user does with the mouse on the cell of a view: press and release the right button on it
/// (which selects the row, as it does for the user), then the context menu it asks for. The menu is
/// found while it is open: its entries are noted, and the one named `choose` is triggered (none if it
/// is empty); a message box that the entry opens is answered with OK.
struct RightClick {
    QStringList m_entries;
    bool m_triggered{false};
};

RightClick rightClick(QAbstractItemView* view, const QModelIndex& index, const QString& choose) {
    // shared with the timer, which runs after the call when the menu has no entries and is not shown
    const auto result = std::make_shared<RightClick>();
    const QPoint pos = view->visualRect(index).center();
    QWidget* viewport = view->viewport();
    QMouseEvent press(QEvent::MouseButtonPress, pos, viewport->mapToGlobal(pos), Qt::RightButton, Qt::RightButton,
                      Qt::NoModifier);
    QApplication::sendEvent(viewport, &press);
    QMouseEvent release(QEvent::MouseButtonRelease, pos, viewport->mapToGlobal(pos), Qt::RightButton, Qt::NoButton,
                        Qt::NoModifier);
    QApplication::sendEvent(viewport, &release);
    QTimer::singleShot(0, [result, choose] {
        QMenu* menu = qobject_cast<QMenu*>(QApplication::activePopupWidget());
        if (menu == nullptr) {
            return; // a menu with no entries is not shown
        }
        for (QAction* action : menu->actions()) {
            if (!action->isSeparator()) {
                result->m_entries << action->text().remove('&');
            }
        }
        for (QAction* action : menu->actions()) {
            if (!choose.isEmpty() && action->text().remove('&') == choose) {
                QTimer::singleShot(0, [] {
                    auto* box = qobject_cast<QMessageBox*>(QApplication::activeModalWidget());
                    REQUIRE(box != nullptr);
                    box->button(QMessageBox::Ok)->click();
                });
                result->m_triggered = true;
                action->trigger();
            }
        }
        menu->close();
    });
    QContextMenuEvent menuEvent(QContextMenuEvent::Mouse, pos, viewport->mapToGlobal(pos));
    QApplication::sendEvent(viewport, &menuEvent);
    QCoreApplication::processEvents();
    return *result;
}

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

TEST_CASE("Right-clicking a layer of the Layer Tree offers Remove Layer, except for layer 0", "[gui][layers][remove][3229]") {
    Drawing d;
    QG_ActionHandler handler(nullptr);
    LC_LayerTreeWidget widget(&handler, nullptr);
    widget.setGraphicView(&d.m_view);
    widget.resize(500, 400);
    widget.show();
    auto* tree = widget.findChild<QTreeView*>();
    REQUIRE(tree != nullptr);
    auto* model = dynamic_cast<LC_LayerTreeModel*>(tree->model());
    REQUIRE(model != nullptr);
    CommandMessages command;
    const RS_LayerList* layers = d.m_graphic.getLayerList();
    RS_Layer* zero = d.m_graphic.findLayer(QStringLiteral("0"));
    const QString remove = QStringLiteral("Remove Layer");

    SECTION("layer 0 has no Remove entry") {
        const QModelIndex name = treeNameIndex(*model, zero);
        REQUIRE(name.isValid());
        const RightClick click = rightClick(tree, name, remove);
        CHECK_FALSE(click.m_entries.isEmpty());
        CHECK_FALSE(click.m_triggered);
        // "Find And Remove Empty Layers" is not about this layer: it is in every menu of the tree
        for (const QString& entry : click.m_entries) {
            CAPTURE(entry.toStdString());
            CHECK_FALSE(entry.startsWith(QStringLiteral("Remove")));
            CHECK_FALSE(entry.contains(QStringLiteral("Delete"), Qt::CaseInsensitive));
        }
        CHECK(layers->contains(zero));
    }
    SECTION("a layer that is not locked is removed, and the current layer stays") {
        const QModelIndex name = treeNameIndex(*model, d.m_construction);
        REQUIRE(name.isValid());
        const RightClick click = rightClick(tree, name, remove);
        CHECK(click.m_entries.contains(remove));
        CHECK(click.m_triggered);
        CHECK_FALSE(layers->contains(d.m_construction));
        CHECK(d.m_graphic.getActiveLayer() == d.m_walls);
        CHECK(command.m_messages.isEmpty());
    }
    SECTION("the current layer is removed too, another layer taking its place") {
        const QModelIndex name = treeNameIndex(*model, d.m_walls);
        REQUIRE(name.isValid());
        const RightClick click = rightClick(tree, name, remove);
        CHECK(click.m_triggered);
        CHECK_FALSE(layers->contains(d.m_walls));
        CHECK(d.m_graphic.getActiveLayer() == zero);
        CHECK(command.m_messages == QStringList{QStringLiteral("Layer \"Walls\" is being removed, so \"0\" is the current layer now.")});
    }
    SECTION("the current layer is kept when no other layer is unlocked") {
        lockAllButWalls(d);
        const QModelIndex name = treeNameIndex(*model, d.m_walls);
        REQUIRE(name.isValid());
        // the layer is skipped, with a message box of its own that is answered with OK
        const RightClick click = rightClick(tree, name, remove);
        CHECK(click.m_triggered);
        CHECK(layers->contains(d.m_walls));
        CHECK(d.m_graphic.getActiveLayer() == d.m_walls);
    }
}

TEST_CASE("Right-clicking a layer of the Layer List offers Remove Layer, except for layer 0", "[gui][layers][remove][3229]") {
    Drawing d;
    LC_ActionGroupManager actions(nullptr);
    QAction removeAction(QStringLiteral("&Remove Layer"));
    LC_ActionGroupManager::associateQActionWithActionType(&removeAction, RS2::ActionLayersRemove);
    actions.getActionsMap().insert(QStringLiteral("LayersRemove"), &removeAction);
    actions.completeInit();
    QG_LayerWidget widget(&actions, nullptr, nullptr);
    widget.setGraphicView(&d.m_view);
    widget.resize(500, 400);
    widget.show();
    auto* table = widget.findChild<QTableView*>();
    REQUIRE(table != nullptr);
    auto* model = dynamic_cast<QG_LayerModel*>(table->model());
    REQUIRE(model != nullptr);
    RS_Layer* zero = d.m_graphic.findLayer(QStringLiteral("0"));
    REQUIRE(zero != nullptr);
    const QString remove = QStringLiteral("Remove Layer");

    SECTION("layer 0 has no Remove entry") {
        const RightClick click = rightClick(table, model->getIndex(zero).siblingAtColumn(QG_LayerModel::COLUMN_NAME), QString());
        CHECK_FALSE(click.m_entries.contains(remove));
    }
    SECTION("another layer has one") {
        const RightClick click = rightClick(table, model->getIndex(d.m_construction).siblingAtColumn(QG_LayerModel::COLUMN_NAME), QString());
        CHECK(click.m_entries.contains(remove));
    }
}

TEST_CASE("Locking the current layer moves the current row of the Layer List", "[gui][layers][active][3229]") {
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
    RS_Layer* zero = d.m_graphic.findLayer(QStringLiteral("0"));
    REQUIRE(zero != nullptr);

    // what the lock icon's action does
    d.m_graphic.toggleLayerLock(d.m_walls);

    CHECK(d.m_walls->isLocked());
    CHECK(d.m_graphic.getActiveLayer() == zero);
    CHECK(model->getActiveLayer() == zero);
    CHECK(table->currentIndex().row() == model->getIndex(zero).row());
    CHECK(model->data(model->getIndex(zero), Qt::FontRole).value<QFont>().bold());
    CHECK_FALSE(model->data(model->getIndex(d.m_walls), Qt::FontRole).value<QFont>().bold());
    CHECK(command.m_messages.size() == 1);
}

TEST_CASE("Locking the current layer in the Layer Tree moves its current row", "[gui][layers][active][3229]") {
    Drawing d;
    LC_LayerTreeWidget widget(nullptr, nullptr);
    widget.setGraphicView(&d.m_view);
    auto* tree = widget.findChild<QTreeView*>();
    REQUIRE(tree != nullptr);
    auto* model = dynamic_cast<LC_LayerTreeModel*>(tree->model());
    REQUIRE(model != nullptr);
    CommandMessages command;
    RS_Layer* zero = d.m_graphic.findLayer(QStringLiteral("0"));
    REQUIRE(zero != nullptr);

    // what the lock icon's action does
    d.m_graphic.toggleLayerLock(d.m_walls);

    CHECK(d.m_walls->isLocked());
    CHECK(d.m_graphic.getActiveLayer() == zero);
    // the tree is rebuilt: look the rows up again
    const QModelIndex zeroName = treeNameIndex(*model, zero);
    const QModelIndex wallsName = treeNameIndex(*model, d.m_walls);
    REQUIRE(zeroName.isValid());
    REQUIRE(wallsName.isValid());
    CHECK(model->getItemForIndex(zeroName)->isActiveLayer());
    CHECK_FALSE(model->getItemForIndex(wallsName)->isActiveLayer());
    CHECK(command.m_messages.size() == 1);
}

TEST_CASE("The status bar names the current layer, with the dock icons when it is hidden or locked", "[gui][layers][active][3229]") {
    auto d = std::make_unique<Drawing>();
    QG_ActiveLayerName label;
    label.setGraphicView(&d->m_view);
    const auto text = [&label] { return label.lActiveLayerName->text().toStdString(); };
    // the icons that the Layer List and the Layer Tree show for a hidden and a locked layer
    using Icons = std::pair<bool, bool>;
    const auto icons = [&label] {
        return Icons{label.hiddenIcon()->isVisibleTo(&label), label.lockedIcon()->isVisibleTo(&label)};
    };
    CHECK(text() == "Walls");
    CHECK(icons() == Icons{false, false});

    d->m_graphic.toggleLayer(d->m_walls);
    CHECK(text() == "Walls");
    CHECK(icons() == Icons{true, false});
    CHECK(label.hiddenIcon()->toolTip().contains(QStringLiteral("hidden")));

    // a locked current layer: a drawing may be opened with every layer locked
    d->m_walls->lock(true);
    label.updateActiveLayer();
    CHECK(icons() == Icons{true, true});
    CHECK(label.lockedIcon()->toolTip().contains(QStringLiteral("locked")));
    d->m_graphic.toggleLayer(d->m_walls);
    CHECK(icons() == Icons{false, true});
    d->m_walls->lock(false);
    d->m_graphic.toggleLayerLock(d->m_doors);
    CHECK(icons() == Icons{false, false});

    d->m_graphic.activateChosenLayer(d->m_construction);
    CHECK(text() == "Construction");
    CHECK(icons() == Icons{true, false});
    // the current layer is not removed by removeLayer(), and the label stays
    CHECK_FALSE(d->m_graphic.removeLayer(d->m_construction));
    CHECK(text() == "Construction");
    CHECK(icons() == Icons{true, false});
    // a layer chosen in a dock is: the layer made current in place of the removed one
    CHECK(d->m_graphic.removeChosenLayer(d->m_construction));
    CHECK(text() == "0");
    CHECK(icons() == Icons{false, false});

    // the drawing goes with the label still attached to it
    d.reset();
    CHECK(text().empty());
    label.setGraphicView(nullptr);
    CHECK(text().empty());
}

TEST_CASE("The status bar follows the layer that locking the current layer made current", "[gui][layers][active][3229]") {
    Drawing d;
    QG_ActiveLayerName label;
    label.setGraphicView(&d.m_view);
    REQUIRE(label.lActiveLayerName->text().toStdString() == "Walls");
    d.m_graphic.toggleLayerLock(d.m_walls);
    CHECK(d.m_walls->isLocked());
    RS_Layer* zero = d.m_graphic.findLayer(QStringLiteral("0"));
    CHECK(d.m_graphic.getActiveLayer() == zero);
    CHECK(label.lActiveLayerName->text().toStdString() == "0");
    CHECK_FALSE(label.lockedIcon()->isVisibleTo(&label));
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
    d.m_graphic.addLayer(roof);

    CHECK(d.m_graphic.getActiveLayer() == roof);
    CHECK(label.lActiveLayerName->text().toStdString() == "Roof");
    CHECK_FALSE(label.hiddenIcon()->isVisibleTo(&label));

    // a layer added hidden, as a paste or a library insert brings one in, is not made current:
    // what is pasted goes to the current layer, and would not be seen
    auto* attic = new RS_Layer(QStringLiteral("Attic"));
    attic->freeze(true);
    d.m_graphic.addLayer(attic);

    CHECK(d.m_graphic.getActiveLayer() == roof);
    CHECK(label.lActiveLayerName->text().toStdString() == "Roof");
}

TEST_CASE("The Layer List disables the lock icon of the current layer when no other layer is unlocked", "[gui][layers][lock][3229][offer]") {
    Drawing d;
    LC_ActionGroupManager actions(nullptr);
    QG_LayerWidget widget(&actions, nullptr, nullptr);
    widget.setGraphicView(&d.m_view);
    auto* table = widget.findChild<QTableView*>();
    REQUIRE(table != nullptr);
    auto* model = dynamic_cast<QG_LayerModel*>(table->model());
    REQUIRE(model != nullptr);
    CommandMessages command;
    RS_Layer* zero = d.m_graphic.findLayer(QStringLiteral("0"));
    REQUIRE(zero != nullptr);
    const auto lockCell = [model](const RS_Layer* layer) {
        const QModelIndex name = model->getIndex(const_cast<RS_Layer*>(layer));
        return model->index(name.row(), QG_LayerModel::COLUMN_LOCKED, QModelIndex{});
    };
    const auto enabled = [model, &lockCell](const RS_Layer* layer) {
        return model->flags(lockCell(layer)).testFlag(Qt::ItemIsEnabled);
    };

    SECTION("while another layer is unlocked every lock icon is enabled") {
        for (const RS_Layer* layer : {zero, d.m_construction, d.m_doors, d.m_walls}) {
            CAPTURE(layer->getName());
            CHECK(enabled(layer));
            CHECK_FALSE(model->data(lockCell(layer), Qt::ToolTipRole).isValid());
        }
    }

    SECTION("with every other layer locked the current layer's is disabled, and says why") {
        lockAllButWalls(d);
        REQUIRE(d.m_doors->isLocked());
        REQUIRE_FALSE(d.m_walls->isLocked());
        CHECK_FALSE(enabled(d.m_walls));
        CHECK(model->data(lockCell(d.m_walls), Qt::ToolTipRole).toString() == RS_LayerList::lockUnavailableText());
        // the others are locked: their icons unlock them
        for (const RS_Layer* layer : {zero, d.m_construction, d.m_doors}) {
            CAPTURE(layer->getName());
            CHECK(enabled(layer));
        }
        // a click on the disabled icon locks nothing (there is no action handler to run it here), and
        // says why on the command line
        widget.slotActivated(lockCell(d.m_walls));
        CHECK_FALSE(d.m_walls->isLocked());
        CHECK(command.m_messages == QStringList{unavailableMessage});
    }

    SECTION("it is enabled again when another layer is unlocked") {
        lockAllButWalls(d);
        REQUIRE_FALSE(enabled(d.m_walls));
        d.m_graphic.toggleLayerLock(zero);
        CHECK_FALSE(zero->isLocked());
        CHECK(enabled(d.m_walls));
        CHECK_FALSE(model->data(lockCell(d.m_walls), Qt::ToolTipRole).isValid());
    }

    SECTION("a drawing of one layer") {
        RS_Graphic graphic;
        graphic.initForNewDocument();
        lc::test::TestGraphicView view;
        view.setDocument(&graphic);
        QG_LayerWidget single(&actions, nullptr, nullptr);
        single.setGraphicView(&view);
        auto* singleModel = dynamic_cast<QG_LayerModel*>(single.findChild<QTableView*>()->model());
        REQUIRE(singleModel != nullptr);
        RS_Layer* only = graphic.findLayer(QStringLiteral("0"));
        const QModelIndex name = singleModel->getIndex(only);
        const QModelIndex cell = singleModel->index(name.row(), QG_LayerModel::COLUMN_LOCKED, QModelIndex{});
        CHECK_FALSE(singleModel->flags(cell).testFlag(Qt::ItemIsEnabled));
        // a layer added is current: "0" is no longer the current layer, and can be locked
        graphic.addLayer(new RS_Layer(QStringLiteral("Added")));
        const QModelIndex cellAfter = singleModel->index(singleModel->getIndex(only).row(), QG_LayerModel::COLUMN_LOCKED, QModelIndex{});
        CHECK(singleModel->flags(cellAfter).testFlag(Qt::ItemIsEnabled));
    }
}

TEST_CASE("The Layer Tree disables the lock icon and Lock all when no other layer is unlocked", "[gui][layers][lock][3229][offer]") {
    Drawing d;
    LC_LayerTreeWidget widget(nullptr, nullptr);
    widget.setGraphicView(&d.m_view);
    auto* tree = widget.findChild<QTreeView*>();
    REQUIRE(tree != nullptr);
    auto* model = dynamic_cast<LC_LayerTreeModel*>(tree->model());
    REQUIRE(model != nullptr);
    CommandMessages command;
    RS_Layer* zero = d.m_graphic.findLayer(QStringLiteral("0"));
    REQUIRE(zero != nullptr);
    QToolButton* lockAll = treeButton(widget, QStringLiteral("Lock all layers"));
    REQUIRE(lockAll != nullptr);
    // the tree is rebuilt for each change: look the cell up again
    const auto lockCell = [&model](const RS_Layer* layer) { return treeIndex(*model, layer, LC_LayerTreeModel::COLUMN_LOCKED); };
    const auto enabled = [&](const RS_Layer* layer) {
        const QModelIndex cell = lockCell(layer);
        REQUIRE(cell.isValid());
        return model->flags(cell).testFlag(Qt::ItemIsEnabled);
    };

    SECTION("while another layer is unlocked the lock icons and Lock all are enabled") {
        for (const RS_Layer* layer : {zero, d.m_construction, d.m_doors, d.m_walls}) {
            CAPTURE(layer->getName());
            CHECK(enabled(layer));
        }
        CHECK(lockAll->isEnabled());
    }

    SECTION("with every other layer locked the current layer's icon and Lock all are disabled") {
        lockAllButWalls(d);
        REQUIRE(d.m_doors->isLocked());
        CHECK_FALSE(enabled(d.m_walls));
        CHECK(model->data(lockCell(d.m_walls), Qt::ToolTipRole).toString() == RS_LayerList::lockUnavailableText());
        for (const RS_Layer* layer : {zero, d.m_construction, d.m_doors}) {
            CAPTURE(layer->getName());
            CHECK(enabled(layer));
        }
        CHECK_FALSE(lockAll->isEnabled());
        // a click on the disabled icon locks nothing (there is no action handler to run it here), and
        // says why on the command line
        widget.slotTreeClicked(lockCell(d.m_walls));
        CHECK_FALSE(d.m_walls->isLocked());
        CHECK(command.m_messages == QStringList{unavailableMessage});
    }

    SECTION("they are enabled again when another layer is unlocked") {
        lockAllButWalls(d);
        REQUIRE_FALSE(enabled(d.m_walls));
        d.m_graphic.toggleLayerLock(zero);
        CHECK(enabled(d.m_walls));
        CHECK_FALSE(model->data(lockCell(d.m_walls), Qt::ToolTipRole).isValid());
        CHECK(lockAll->isEnabled());
    }
}

TEST_CASE("The toolbar and menu actions Lock all and Toggle layer lock are enabled when they would lock or unlock", "[gui][layers][lock][3229][offer]") {
    Drawing d;
    LC_ActionGroupManager actions(nullptr);
    QAction lockAllAction;
    QAction toggleLockAction;
    LC_ActionGroupManager::associateQActionWithActionType(&lockAllAction, RS2::ActionLayersLockAll);
    LC_ActionGroupManager::associateQActionWithActionType(&toggleLockAction, RS2::ActionLayersToggleLock);
    actions.getActionsMap().insert(QStringLiteral("LayersLockAll"), &lockAllAction);
    actions.getActionsMap().insert(QStringLiteral("LayersToggleLock"), &toggleLockAction);
    actions.completeInit();
    QG_LayerWidget widget(&actions, nullptr, nullptr);
    widget.setGraphicView(&d.m_view);
    auto* table = widget.findChild<QTableView*>();
    REQUIRE(table != nullptr);
    auto* model = dynamic_cast<QG_LayerModel*>(table->model());
    REQUIRE(model != nullptr);
    CommandMessages command;
    RS_Layer* zero = d.m_graphic.findLayer(QStringLiteral("0"));
    REQUIRE(zero != nullptr);

    CHECK(lockAllAction.isEnabled());
    CHECK(toggleLockAction.isEnabled());

    // nothing is left to lock but the current layer
    lockAllButWalls(d);
    CHECK_FALSE(lockAllAction.isEnabled());
    CHECK_FALSE(toggleLockAction.isEnabled());

    // selecting a locked layer gives Toggle layer lock something to unlock
    table->selectionModel()->select(model->getIndex(d.m_doors), QItemSelectionModel::Select);
    CHECK(d.m_doors->isSelectedInLayerList());
    CHECK(toggleLockAction.isEnabled());
    CHECK_FALSE(lockAllAction.isEnabled());
    table->selectionModel()->select(model->getIndex(d.m_doors), QItemSelectionModel::Deselect);
    CHECK_FALSE(d.m_doors->isSelectedInLayerList());
    CHECK_FALSE(toggleLockAction.isEnabled());

    // unlocking a layer gives both something to do again
    d.m_graphic.toggleLayerLock(zero);
    CHECK(lockAllAction.isEnabled());
    CHECK(toggleLockAction.isEnabled());

    // Lock all locks the layers the dock lists: with a filter that lists only a locked layer it
    // locks nothing, though "0" is not locked, and with the filter cleared it does again
    REQUIRE_FALSE(zero->isLocked());
    widget.getMatchLayerName()->setText(QStringLiteral("Doors"));
    CHECK_FALSE(lockAllAction.isEnabled());
    widget.getMatchLayerName()->clear();
    CHECK(lockAllAction.isEnabled());
}

TEST_CASE("The Layer Tree disables the lock icon of a group whose unlocked layer cannot be locked", "[gui][layers][lock][3229][offer]") {
    (void)lc::test::application();
    RS_Graphic graphic;
    graphic.initForNewDocument();
    auto* walls = new RS_Layer(QStringLiteral("Group-Walls"));
    auto* doors = new RS_Layer(QStringLiteral("Group-Doors"));
    graphic.addLayer(walls);
    graphic.addLayer(doors);
    graphic.activateLayer(walls);
    lc::test::TestGraphicView view;
    view.setDocument(&graphic);
    LC_LayerTreeWidget widget(nullptr, nullptr);
    widget.setGraphicView(&view);
    auto* tree = widget.findChild<QTreeView*>();
    REQUIRE(tree != nullptr);
    auto* model = dynamic_cast<LC_LayerTreeModel*>(tree->model());
    REQUIRE(model != nullptr);
    CommandMessages command;
    RS_Layer* zero = graphic.findLayer(QStringLiteral("0"));
    REQUIRE(zero != nullptr);
    const auto groupCell = [&model] { return treeGroupIndex(*model, QStringLiteral("Group"), LC_LayerTreeModel::COLUMN_LOCKED); };
    REQUIRE(groupCell().isValid());
    CHECK(model->flags(groupCell()).testFlag(Qt::ItemIsEnabled));

    // "Walls", the current layer, is the only layer left unlocked: the group has nothing to lock
    graphic.setLockLayers({}, {zero, doors});
    REQUIRE(groupCell().isValid());
    CHECK_FALSE(model->flags(groupCell()).testFlag(Qt::ItemIsEnabled));
    CHECK(model->data(groupCell(), Qt::ToolTipRole).toString() == RS_LayerList::lockUnavailableText());
    widget.slotTreeClicked(groupCell());
    CHECK_FALSE(walls->isLocked());
    CHECK(command.m_messages == QStringList{QStringLiteral(
              "Layer \"Group-Walls\" was not locked: it is the current layer, and no other layer is unlocked to take its place.")});

    // another layer unlocked: the group locks "Walls", and "0" takes its place
    graphic.toggleLayerLock(zero);
    REQUIRE(groupCell().isValid());
    CHECK(model->flags(groupCell()).testFlag(Qt::ItemIsEnabled));
}
