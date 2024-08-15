/*
**********************************************************************************
**
** This file was created for the LibreCAD project (librecad.org), a 2D CAD program.
**
** Copyright (C) 2016 ravas - https://github.com/r-a-v-a-s
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

#include "customwidgetcreator.h"
#include "ui_customwidgetcreator.h"

#include <QSettings>

CustomWidgetCreator::CustomWidgetCreator(QWidget* parent,
                                         QMap<QString, QAction*>& a_map)
    : QFrame(parent)
    , ui(new Ui::CustomWidgetCreator)
{
    ui->setupUi(this);

    QSettings settings;
    QStringList s_list = settings.value("CustomWidgets/DoubleClickMenu").toStringList();

    foreach (auto key, s_list)
    {
        ui->chosen_actions->addActionItem(a_map[key]);
    }
    ui->chosen_actions->setDragDropMode(QAbstractItemView::InternalMove);

    ui->offered_actions->setSortingEnabled(true);
    ui->offered_actions->fromActionMap(a_map);
    for(int i = 0; i < ui->offered_actions->count(); ++i)
    {
        auto item = ui->offered_actions->item(i);
        if (s_list.contains(item->whatsThis()))
            delete item;
    }

    connect(ui->add_button, SIGNAL(released()), this, SLOT(addChosenAction()));
    connect(ui->remove_button, SIGNAL(released()), this, SLOT(removeChosenAction()));

    connect(ui->offered_actions, SIGNAL(itemDoubleClicked(QListWidgetItem*)),
            this, SLOT(addChosenAction(QListWidgetItem*)));
    connect(ui->chosen_actions, SIGNAL(itemDoubleClicked(QListWidgetItem*)),
            this, SLOT(removeChosenAction(QListWidgetItem*)));

    connect(ui->save_button, SIGNAL(released()), parent, SLOT(accept()));
}

CustomWidgetCreator::~CustomWidgetCreator()
{
    delete ui;
}

void CustomWidgetCreator::addChosenAction()
{
    QListWidgetItem* item = ui->offered_actions->currentItem();
    if (item)
    {
        ui->chosen_actions->addItem(item->clone());
        delete item;
    }
}

void CustomWidgetCreator::addChosenAction(QListWidgetItem* item)
{
    ui->chosen_actions->addItem(item->clone());
    delete item;
}

void CustomWidgetCreator::removeChosenAction()
{
    QListWidgetItem* item = ui->chosen_actions->currentItem();
    if (item)
    {
        ui->offered_actions->addItem(item->clone());
        delete item;
    }
}

void CustomWidgetCreator::removeChosenAction(QListWidgetItem* item)
{
    ui->offered_actions->addItem(item->clone());
    delete item;
}

QStringList CustomWidgetCreator::getChosenActions()
{
    QStringList s_list;

    for (int i = 0; i < ui->chosen_actions->count(); ++i)
    {
        s_list << ui->chosen_actions->item(i)->whatsThis();
    }
    return s_list;
}
