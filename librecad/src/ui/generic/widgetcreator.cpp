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

#include "widgetcreator.h"
#include "ui_widgetcreator.h"

#include <QAction>
#include <QActionGroup>
#include <QSettings>
#include <QLineEdit>
#include <QPushButton>

WidgetCreator::WidgetCreator(QWidget* parent,
                             QMap<QString, QAction*>& actions,
                             QMap<QString, QActionGroup*> action_groups,
                             bool assigner)
    : QFrame(parent)
    , ui(new Ui::WidgetCreator)
    , a_map(actions)
    , ag_map(action_groups)
{
    ui->setupUi(this);

    if (!assigner)
    {
        ui->assign_button->hide();
        ui->update_button->hide();
    }
    else
    {
        connect(ui->assign_button, SIGNAL(released()), this, SLOT(requestAssignment()));
        connect(ui->update_button, SIGNAL(released()), this, SLOT(requestUpdate()));
    }

    ui->categories_combobox->addItem(QObject::tr("All"));
    foreach (auto ag, ag_map)
    {
        ui->categories_combobox->addItem(ag->objectName());
    }
    ui->categories_combobox->setCurrentIndex(-1);
    ui->chosen_actions->setDragDropMode(QAbstractItemView::InternalMove);

    ui->offered_actions->setSortingEnabled(true);

    ui->offered_actions->fromActionMap(a_map);

    connect(ui->add_button, SIGNAL(released()), this, SLOT(addChosenAction()));
    connect(ui->remove_button, SIGNAL(released()), this, SLOT(removeChosenAction()));

    connect(ui->offered_actions, SIGNAL(itemDoubleClicked(QListWidgetItem*)),
            this, SLOT(addChosenAction(QListWidgetItem*)));
    connect(ui->chosen_actions, SIGNAL(itemDoubleClicked(QListWidgetItem*)),
            this, SLOT(removeChosenAction(QListWidgetItem*)));

    connect(ui->combo, SIGNAL(activated(QString)), this, SLOT(setLists(QString)));

    connect(ui->create_button, SIGNAL(released()), this, SLOT(createWidget()));
    connect(ui->destroy_button, SIGNAL(released()), this, SLOT(destroyWidget()));

    connect(ui->categories_combobox, SIGNAL(activated(QString)), this, SLOT(setCategory(QString)));
}

WidgetCreator::~WidgetCreator()
{
    delete ui;
}

void WidgetCreator::addChosenAction()
{
    QListWidgetItem* item = ui->offered_actions->currentItem();
    if (item)
    {
        ui->chosen_actions->addItem(item->clone());
        delete item;
    }
}

void WidgetCreator::addChosenAction(QListWidgetItem* item)
{
    ui->chosen_actions->addItem(item->clone());
    delete item;
}

void WidgetCreator::removeChosenAction()
{
    QListWidgetItem* item = ui->chosen_actions->currentItem();
    if (item)
    {
        ui->offered_actions->addItem(item->clone());
        delete item;
    }
}

void WidgetCreator::removeChosenAction(QListWidgetItem* item)
{
    ui->offered_actions->addItem(item->clone());
    delete item;
}

QStringList WidgetCreator::getChosenActions()
{
    QStringList s_list;

    for (int i = 0; i < ui->chosen_actions->count(); ++i)
    {
        s_list << ui->chosen_actions->item(i)->whatsThis();
    }
    return s_list;
}

QString WidgetCreator::getWidgetName()
{
    return ui->combo->lineEdit()->text();
}


void WidgetCreator::addCustomWidgets(const QString& group)
{
    w_group = group;

    QSettings settings;

    settings.beginGroup(group);
    foreach (auto key, settings.childKeys())
    {
        ui->combo->addItem(key);
    }
    settings.endGroup();

    ui->combo->lineEdit()->clear();
}

void WidgetCreator::setLists(QString key)
{
    w_key = key;
    QSettings settings;
    auto widget = QString("%1/%2").arg(w_group).arg(key);
    QStringList s_list = settings.value(widget).toStringList();

    if (s_list.isEmpty()) return;

    ui->chosen_actions->clear();
    ui->offered_actions->clear();

    ui->offered_actions->fromActionMap(a_map);
    int index = ui->categories_combobox->findText(QObject::tr("All"));
    ui->categories_combobox->setCurrentIndex(index);

    for(int i = 0; i < ui->offered_actions->count(); ++i)
    {
        auto item = ui->offered_actions->item(i);
        if (s_list.contains(item->whatsThis()))
            delete item;
    }

    foreach (auto str, s_list)
    {
        ui->chosen_actions->addActionItem(a_map[str]);
    }
}

void WidgetCreator::createWidget()
{
    w_key = ui->combo->lineEdit()->text();
    if(!w_key.isEmpty() && ui->combo->findText(w_key) == -1)
    {
        w_key.replace("/", "-");
        ui->combo->addItem(w_key);
    }

    QStringList a_list = getChosenActions();
    if (!a_list.isEmpty() && !w_key.isEmpty())
    {
        QSettings settings;
        auto widget = QString("%1/%2").arg(w_group).arg(w_key);
        settings.setValue(widget, a_list);
        emit widgetToCreate(w_key);
    }
}

void WidgetCreator::destroyWidget()
{
    auto key = ui->combo->lineEdit()->text();
    if (key.isEmpty()) return;

    int index = ui->combo->findText(key);
    if (index > -1)
    {
        ui->combo->removeItem(index);
        QSettings settings;
        settings.remove(QString("%1/%2").arg(w_group).arg(key));
        emit widgetToDestroy(key);
        ui->chosen_actions->clear();
        ui->combo->lineEdit()->clear();
    }
}

void WidgetCreator::setCategory(QString category)
{
    if (category == QObject::tr("All"))
    {
        ui->offered_actions->clear();
        ui->offered_actions->fromActionMap(a_map);
        return;
    }

    if (!ag_map.contains(category)) return;

    ui->offered_actions->clear();
    auto chosen_actions = getChosenActions();
    auto action_group = ag_map[category];
    foreach (auto action, action_group->actions())
    {
        if (!chosen_actions.contains(action->objectName()))
            ui->offered_actions->addActionItem(action);
    }
}

void WidgetCreator::addButton(QPushButton* button)
{
    ui->button_frame->layout()->addWidget(button);
}

void WidgetCreator::requestAssignment()
{
    auto widget_name = getWidgetName();
    if (hasBeenCreated(widget_name))
        emit widgetToAssign(widget_name);
}

void WidgetCreator::requestUpdate()
{
    auto widget_name = getWidgetName();
    if (hasBeenCreated(widget_name))
    {
        QSettings settings;
        auto widget = QString("%1/%2").arg(w_group).arg(w_key);
        QStringList a_list = getChosenActions();
        settings.setValue(widget, a_list);
        emit widgetToUpdate(widget_name);
    }
}

bool WidgetCreator::hasBeenCreated(const QString& widget_name)
{
    int index = ui->combo->findText(widget_name);
    return (index > -1) ? true : false;
}
