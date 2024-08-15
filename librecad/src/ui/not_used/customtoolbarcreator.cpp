/*
**********************************************************************************
**
** This file was created for LibreCAD (https://github.com/LibreCAD/LibreCAD).
**
** Copyright (C) 2016 ravas (https://github.com/r-a-v-a-s)
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

#include "customtoolbarcreator.h"
#include "ui_customtoolbarcreator.h"
#include "lc_actiongroupmanager.h"

#include <QSettings>
#include <QLineEdit>

CustomToolbarCreator::CustomToolbarCreator(QWidget* parent,
                                           QMap<QString, QAction*>& action_map,
                                           LC_ActionGroupManager* agm)
    : QFrame(parent)
    , ui(new Ui::CustomToolbarCreator)
    , a_map(action_map)
    , ag_manager(agm)
{
    ui->setupUi(this);

    foreach (auto ag, ag_manager->findChildren<QActionGroup*>())
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

    connect(ui->add_widget_button, SIGNAL(released()), this, SLOT(addWidget()));
    connect(ui->remove_widget_button, SIGNAL(released()), this, SLOT(removeWidget()));

    connect(ui->combo, SIGNAL(activated(QString)), this, SLOT(setLists(QString)));

    connect(ui->save_button, SIGNAL(released()), this, SLOT(create()));

    connect(ui->categories_combobox, SIGNAL(activated(QString)), this, SLOT(setCategory(QString)));
}

CustomToolbarCreator::~CustomToolbarCreator()
{
    delete ui;
}

void CustomToolbarCreator::addChosenAction()
{
    QListWidgetItem* item = ui->offered_actions->currentItem();
    if (item)
    {
        ui->chosen_actions->addItem(item->clone());
        delete item;
    }
}

void CustomToolbarCreator::addChosenAction(QListWidgetItem* item)
{
    ui->chosen_actions->addItem(item->clone());
    delete item;
}

void CustomToolbarCreator::removeChosenAction()
{
    QListWidgetItem* item = ui->chosen_actions->currentItem();
    if (item)
    {
        ui->offered_actions->addItem(item->clone());
        delete item;
    }
}

void CustomToolbarCreator::removeChosenAction(QListWidgetItem* item)
{
    ui->offered_actions->addItem(item->clone());
    delete item;
}

QStringList CustomToolbarCreator::getChosenActions()
{
    QStringList s_list;

    for (int i = 0; i < ui->chosen_actions->count(); ++i)
    {
        s_list << ui->chosen_actions->item(i)->whatsThis();
    }
    return s_list;
}


void CustomToolbarCreator::addCustomWidgets(const QString& group)
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

void CustomToolbarCreator::addWidget()
{
    w_key = ui->combo->lineEdit()->text();
    if(!w_key.isEmpty() && ui->combo->findText(w_key) == -1)
        ui->combo->addItem(w_key);
}

void CustomToolbarCreator::removeWidget()
{
    auto key = ui->combo->lineEdit()->text();
    if (key.isEmpty()) return;

    int index = ui->combo->findText(key);
    if (index > -1)
    {
        ui->combo->removeItem(index);
        ui->combo->lineEdit()->clear();
        QSettings settings;
        settings.remove(QString("%1/%2").arg(w_group).arg(key));
        ui->chosen_actions->clear();
        emit widgetToDestroy(key);
    }
}

void CustomToolbarCreator::setLists(QString key)
{
    w_key = key;
    QSettings settings;
    auto widget = QString("%1/%2").arg(w_group).arg(key);
    QStringList s_list = settings.value(widget).toStringList();

    if (s_list.isEmpty()) return;

    ui->chosen_actions->clear();
    ui->offered_actions->clear();

    ui->offered_actions->fromActionMap(a_map);

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

QString CustomToolbarCreator::getToolbarName()
{
    return ui->combo->lineEdit()->text();
}

void CustomToolbarCreator::create()
{
    QStringList a_list = getChosenActions();
    if (!a_list.isEmpty() && !w_key.isEmpty())
    {
        QSettings settings;
        auto widget = QString("%1/%2").arg(w_group).arg(w_key);
        settings.setValue(widget, a_list);
        emit widgetToCreate(w_key);
    }
}

void CustomToolbarCreator::setCategory(QString category)
{
    ui->offered_actions->clear();
    auto chosen_actions = getChosenActions();
    auto action_group = ag_manager->findChild<QActionGroup*>(category);
    foreach (auto action, action_group->actions())
    {
        if (!chosen_actions.contains(action->objectName()))
            ui->offered_actions->addActionItem(action);
    }
}
