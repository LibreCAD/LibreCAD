/*
**********************************************************************************
**
** This file was created for the LibreCAD project (librecad.org), a 2D CAD program.
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
#include "actionlist.h"

#include "lc_actiongroup.h"

ActionList::ActionList(QWidget* parent)
    : QListWidget(parent)
{}

void ActionList::addActionItem(QAction *action) {
    auto item = new QListWidgetItem;
    item->setText(action->text().remove("&"));
    item->setIcon(action->icon());
    item->setWhatsThis(action->objectName());
    auto actionGroup = action->actionGroup();
    if (actionGroup != nullptr) {
        LC_ActionGroup* lc_actiongroup = dynamic_cast<LC_ActionGroup*>(actionGroup);
        if (lc_actiongroup != nullptr) {
            item->setData(Qt::UserRole, lc_actiongroup->getName());
        }
    }
    addItem(item);
}

void ActionList::fromActionList(const QList<QAction *> &a_list) {
    m_actionList = a_list;
    foreach(QAction *a, a_list) {
        if (a != nullptr) {
            addActionItem(a);
        }
    }
}

void ActionList::fromActionMap(QMap<QString, QAction *> &a_map) {
    foreach(QAction *a, a_map) {
        if (a != nullptr) {
            addActionItem(a);
        }
    }
}

void ActionList::activateAction(QListWidgetItem *item) {
    foreach(QAction *a, m_actionList) {
        if (a->text() == item->text())
            a->activate(QAction::Trigger);
    }
}
