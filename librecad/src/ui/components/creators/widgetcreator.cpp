/*
**********************************************************************************
**
** This file was created for LibreCAD (https://github.com/LibreCAD/LibreCAD).
**
** Copyright (C) 2016 ravas (github.com/r-a-v-a-s)
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
** http://www.gnu.org/licenses/gpl-2.0.html
**
**********************************************************************************
*/


#include <QAction>
#include <QActionGroup>
#include <QSettings>
#include <QLineEdit>
#include <QPushButton>

#include "widgetcreator.h"
#include "ui_widgetcreator.h"

WidgetCreator::WidgetCreator(
    QWidget *parent,
    LC_ActionGroupManager* agm,
    bool assigner)
    :QFrame(parent), ui(new Ui::WidgetCreator), actionGroupManager(agm){

    ui->setupUi(this);

    if (!assigner) {
        ui->assign_button->hide();
        ui->update_button->hide();
    } else {
        connect(ui->assign_button, &QPushButton::released, this, &WidgetCreator::requestAssignment);
        connect(ui->update_button, &QPushButton::released, this, &WidgetCreator::requestUpdate);
    }

    ui->categories_combobox->addItem(QObject::tr("All"));
    foreach (auto ag, actionGroupManager->allGroupsList()) {
       ui->categories_combobox->addItem(ag->objectName());
    }
    ui->categories_combobox->setCurrentIndex(-1);
    ui->chosen_actions->setDragDropMode(QAbstractItemView::InternalMove);

    ui->offered_actions->setSortingEnabled(true);

    ui->offered_actions->fromActionMap(actionGroupManager->getActionsMap());

    connect(ui->add_button, &QPushButton::released, this, &WidgetCreator::addChosenAction);
    connect(ui->remove_button,  &QPushButton::released, this, &WidgetCreator::removeChosenAction);

    connect(ui->offered_actions, &ActionList::itemDoubleClicked,this, &WidgetCreator::addChosenActionForItem);
    connect(ui->chosen_actions, &ActionList::itemDoubleClicked,this, &WidgetCreator::removeChosenActionForItem);

    connect(ui->combo, &QComboBox::activated, this, &WidgetCreator::setLists);

    connect(ui->create_button, &QPushButton::released, this, &WidgetCreator::createWidget);
    connect(ui->destroy_button, &QPushButton::released, this, &WidgetCreator::destroyWidget);

    connect(ui->categories_combobox, &QComboBox::activated, this, &WidgetCreator::setCategory);
}

WidgetCreator::~WidgetCreator() {
    delete ui;
}

void WidgetCreator::addChosenAction() {
    QListWidgetItem *item = ui->offered_actions->currentItem();
    if (item) {
        ui->chosen_actions->addItem(item->clone());
        delete item;
    }
}

void WidgetCreator::addChosenActionForItem(QListWidgetItem *item) {
    ui->chosen_actions->addItem(item->clone());
    delete item;
}

void WidgetCreator::removeChosenAction() {
    QListWidgetItem *item = ui->chosen_actions->currentItem();
    if (item) {
        ui->offered_actions->addItem(item->clone());
        delete item;
    }
}

void WidgetCreator::removeChosenActionForItem(QListWidgetItem *item) {
    ui->offered_actions->addItem(item->clone());
    delete item;
}

QStringList WidgetCreator::getChosenActions() {
    QStringList s_list;

    for (int i = 0; i < ui->chosen_actions->count(); ++i) {
        s_list << ui->chosen_actions->item(i)->whatsThis();
    }
    return s_list;
}

QString WidgetCreator::getWidgetName() {
    return ui->combo->lineEdit()->text();
}

void WidgetCreator::addCustomWidgets(const QString &group) {
    w_group = group;

    QSettings settings;

    settings.beginGroup(group);
        foreach (auto key, settings.childKeys()) {
            ui->combo->addItem(key);
        }
    settings.endGroup();

    ui->combo->lineEdit()->clear();
}

void WidgetCreator::setLists(int index) {
    QString key = ui->combo->itemText(index);
    w_key = key;
    QSettings settings;
    auto widget = QString("%1/%2").arg(w_group).arg(key);
    QStringList s_list = settings.value(widget).toStringList();

    if (s_list.isEmpty()) return;

    ui->chosen_actions->clear();
    ui->offered_actions->clear();

    ui->offered_actions->fromActionMap(actionGroupManager->getActionsMap());
    int allIndex = ui->categories_combobox->findText(QObject::tr("All"));
    ui->categories_combobox->setCurrentIndex(allIndex);

    for (int i = 0; i < ui->offered_actions->count(); ++i) {
        auto item = ui->offered_actions->item(i);
        if (s_list.contains(item->whatsThis()))
            delete item;
    }

        foreach (auto str, s_list) {
            QAction *action = actionGroupManager->getActionByName(str);
            if (action != nullptr) {
                ui->chosen_actions->addActionItem(action);
            }
        }
}

void WidgetCreator::createWidget() {
    w_key = ui->combo->lineEdit()->text();
    if (!w_key.isEmpty() && ui->combo->findText(w_key) == -1) {
        w_key.replace("/", "-");
        ui->combo->addItem(w_key);
    }

    QStringList a_list = getChosenActions();
    if (!a_list.isEmpty() && !w_key.isEmpty()) {
        QSettings settings;
        auto widget = QString("%1/%2").arg(w_group).arg(w_key);
        settings.setValue(widget, a_list);
        emit widgetToCreate(w_key);
    }
}

void WidgetCreator::destroyWidget() {
    auto key = ui->combo->lineEdit()->text();
    if (key.isEmpty()) return;

    int index = ui->combo->findText(key);
    if (index > -1) {
        ui->combo->removeItem(index);
        QSettings settings;
        settings.remove(QString("%1/%2").arg(w_group).arg(key));
        emit widgetToDestroy(key);
        ui->chosen_actions->clear();
        ui->combo->lineEdit()->clear();
    }
}

void WidgetCreator::setCategory(int index) {
    QString category = ui->categories_combobox->itemText(index);
    if (category == QObject::tr("All")) {
        auto a_map = actionGroupManager->getActionsMap();
        ui->offered_actions->clear();
        ui->offered_actions->fromActionMap(a_map);
        return;
    }

    if (!actionGroupManager->hasActionGroup(category)) {
        return;
    }

    ui->offered_actions->clear();
    auto chosen_actions = getChosenActions();
    auto action_group = actionGroupManager->getActionGroup(category);
    if (action_group != nullptr)
        for (auto action: action_group->actions()) {
            if (!chosen_actions.contains(action->objectName()))
                ui->offered_actions->addActionItem(action);
        }
}

void WidgetCreator::addButton(QPushButton *button) {
    ui->button_frame->layout()->addWidget(button);
}

void WidgetCreator::requestAssignment() {
    auto widget_name = getWidgetName();
    if (hasBeenCreated(widget_name))
        emit widgetToAssign(widget_name);
}

void WidgetCreator::requestUpdate() {
    auto widget_name = getWidgetName();
    if (hasBeenCreated(widget_name)) {
        QSettings settings;
        auto widget = QString("%1/%2").arg(w_group).arg(w_key);
        QStringList a_list = getChosenActions();
        settings.setValue(widget, a_list);
        emit widgetToUpdate(widget_name);
    }
}

bool WidgetCreator::hasBeenCreated(const QString &widget_name) {
    int index = ui->combo->findText(widget_name);
    return (index > -1) ? true : false;
}
