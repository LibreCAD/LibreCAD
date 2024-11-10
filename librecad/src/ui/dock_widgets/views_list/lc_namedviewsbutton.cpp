/*******************************************************************************
 *
 This file is part of the LibreCAD project, a 2D CAD program

 Copyright (C) 2024 LibreCAD.org
 Copyright (C) 2024 sand1024

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 ******************************************************************************/

#include <QMenu>
#include <QActionGroup>
#include "lc_namedviewsbutton.h"
#include "rs_debug.h"

LC_NamedViewsButton::LC_NamedViewsButton(LC_NamedViewsListWidget *w):QToolButton(nullptr), widget{w} {
    setPopupMode(QToolButton::MenuButtonPopup);
    menu = new QMenu();
    connect(menu, &QMenu::aboutToShow, this, &LC_NamedViewsButton::fillMenu);
    setMenu(menu);

}
// a bit weird logic, yet adding actions to menu and removing them on close does not work - menu is shown correctly, yet signal for trigger is not called.
// therefore, the list of actions is reused, and not-needed actions are simply invisible.
void LC_NamedViewsButton::fillMenu() {
    QList<LC_View*> views;
    widget->fillViewsList(views);

    int viewsCount = views.count();
    int actionsCount = createdActions.count();
    if (viewsCount <= actionsCount){
        int i;
        for (i = 0;  i < viewsCount; i++){
            QAction* a = createdActions.at(i);
            LC_View* v = views.at(i);
            QIcon typeIcon = widget->getViewTypeIcon(v);
            a->setText(v->getName());
            a->setIcon(typeIcon);
            a->setVisible(true);
        }
        for (;i < actionsCount; i++){
            QAction* a = createdActions.at(i);
            a->setVisible(false);
        }
    }
    else{
        int i;
        for (i = 0;  i < actionsCount; i++){
            QAction* a = createdActions.at(i);
            LC_View* v = views.at(i);
            QIcon typeIcon = widget->getViewTypeIcon(v);
            a->setText(v->getName());
            a->setIcon(typeIcon);
            a->setVisible(true);
        }
        for (; i < viewsCount; i++){
            LC_View* v = views.at(i);
            QString name = v->getName();
            QIcon typeIcon = widget->getViewTypeIcon(v);
            auto* action = menu->addAction(typeIcon, name);
            connect(action, &QAction::triggered, this, &LC_NamedViewsButton::menuTriggered);
            createdActions << action;
            action->setEnabled(true);
            action->setCheckable(false);
            action->setVisible(true);
        }
    }
}

void LC_NamedViewsButton::menuTriggered([[maybe_unused]]bool checked){
    auto *action = qobject_cast<QAction*>(sender());
    if (action != nullptr) {
        QString viewName = action->text();
        widget->restoreView(viewName);
    }
}
