/*******************************************************************************
 *
 This file is part of the LibreCAD project, a 2D CAD program

 Copyright (C) 2025 LibreCAD.org
 Copyright (C) 2025 sand1024

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

#include "lc_workspacelistbutton.h"

#include <QMenu>

#include "qc_applicationwindow.h"

LC_WorkspaceListButton::LC_WorkspaceListButton(QC_ApplicationWindow *w):QToolButton(nullptr), m_appWin{w} {
    setPopupMode(QToolButton::MenuButtonPopup);
    m_menu = new QMenu();
    connect(m_menu, &QMenu::aboutToShow, this, &LC_WorkspaceListButton::fillMenu);
    setMenu(m_menu);
    m_wcsIcon = QIcon(":/icons/workspace.lci");
}

void LC_WorkspaceListButton::enableSubActions(bool value){
    if (value){
        setMenu(m_menu);
    }
    else{
        setMenu(nullptr);
    }
}

// a bit weird logic, yet adding actions to menu and removing them on close does not work - menu is shown correctly, yet signal for trigger is not called.
// therefore, the list of actions is reused, and not-needed actions are simply invisible.
void LC_WorkspaceListButton::fillMenu() {
    QList<QPair<int, QString>> workspacesList;
    m_appWin->fillWorkspacesList(workspacesList);

    int workspacesCount = workspacesList.count();
    int actionsCount = m_createdActions.count();
    if (workspacesCount <= actionsCount){
        int i;
        for (i = 0; i < workspacesCount; i++){
            auto a = m_createdActions.at(i);
            auto w = workspacesList.at(i);
            a->setText(w.second);
            a->setIcon(m_wcsIcon);
            a->setVisible(true);
            a->setProperty("_WSPS_IDX", QVariant(w.first));
        }
        for (;i < actionsCount; i++){
            QAction* a = m_createdActions.at(i);
            a->setVisible(false);
        }
    }
    else{
        int i;
        for (i = 0;  i < actionsCount; i++){
            auto a = m_createdActions.at(i);
            auto w = workspacesList.at(i);
            a->setText(w.second);
            a->setIcon(m_wcsIcon);
            a->setVisible(true);
            a->setProperty("_WSPS_IDX", QVariant(w.first));
        }
        for (; i < workspacesCount; i++){
            auto w = workspacesList.at(i);
            auto name = w.second;
            auto* a = m_menu->addAction(m_wcsIcon, name);
            connect(a, &QAction::triggered, this, &LC_WorkspaceListButton::menuTriggered);
            m_createdActions << a;
            a->setEnabled(true);
            a->setCheckable(false);
            a->setVisible(true);
            a->setProperty("_WSPS_IDX", QVariant(w.first));
        }
    }
}

void LC_WorkspaceListButton::menuTriggered([[maybe_unused]]bool checked) const {
    auto *action = qobject_cast<QAction*>(sender());
    if (action != nullptr) {
        QVariant variant = action->property("_WSPS_IDX");
        if (variant.isValid()){
            int id = variant.toInt();
            m_appWin->applyWorkspaceById(id);
        }
    }
}
