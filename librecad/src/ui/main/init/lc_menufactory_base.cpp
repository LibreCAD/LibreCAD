/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2026 LibreCAD.org
 * Copyright (C) 2026 sand1024
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 * ********************************************************************************
 */

#include "lc_menufactory_base.h"

#include <QDockWidget>

#include "lc_actiongroupmanager.h"
#include "lc_menufactory.h"
#include "qc_applicationwindow.h"
#include "rs_settings.h"

void LC_MenuFactoryBase::findViewAndUCSToggleActions(QList<QDockWidget*> dockWidgetsList,
                                                 QAction*& namedViewsToggleViewAction, QAction*& ucsToggleViewAction) const {
    for (QDockWidget* dw : dockWidgetsList) {
        if (m_appWin->dockWidgetArea(dw) == Qt::RightDockWidgetArea) { // fixme - well, it seems one docking area is limiting...
            QAction* action = dw->toggleViewAction();
            m_menusHolder->m_menuDockWidgets->QWidget::addAction(action);
            QString objectName = dw->objectName();
            if (objectName == "view_dockwidget") {
                namedViewsToggleViewAction = action;
            }
            else if (objectName == "ucs_dockwidget") {
                ucsToggleViewAction = action;
            }
        }
    }
}

void LC_MenuFactoryBase::createWorkspacesListSubMenu(QMenu* parentMenu) {
    QList<QPair<int, QString>> workspacesList;
    m_appWin->fillWorkspacesList(workspacesList);
    if (!workspacesList.isEmpty()) {
        const auto wsIcon = QIcon(":/icons/workspace.lci");
        const auto wsMenu = new QMenu(tr("&Workspaces"), parentMenu);
        wsMenu->setTearOffEnabled(m_allowTearOffMenus);
        wsMenu->setIcon(wsIcon);
        for (const auto & [wId, name]: std::as_const(workspacesList)) {
            auto* a = wsMenu->addAction(wsIcon, name);
            const int workspaceId = wId;
            connect(a, &QAction::triggered, [workspaceId, this]  {
                m_appWin->applyWorkspaceById(workspaceId);
            });
            a->setEnabled(true);
            a->setCheckable(false);
            a->setVisible(true);
        }
        parentMenu->addMenu(wsMenu);
        parentMenu->QWidget::addAction(m_actionGroupManager->getActionByName("WorkspaceRemove"));
    }
}

QMenu* LC_MenuFactoryBase::subMenu(QMenu* parent, const QString& title, const QString& name, const char* icon,
                                   const std::vector<QString>& actionNames, const bool supportTearOff) const {
    QMenu* result = doCreateSubMenu(parent, title, name, icon, supportTearOff);
    addActions(result, actionNames);
    return result;
}

LC_MenuFactoryBase::LC_MenuFactoryBase(QC_ApplicationWindow* mainWin,  LC_ActionGroupManager* actionGroupManager, LC_MenusHolder* menusHolder):LC_AppWindowAware(mainWin),
     m_actionGroupManager{actionGroupManager}, m_menusHolder{menusHolder} {
    m_allowTearOffMenus = LC_GET_ONE_BOOL("Appearance", "AllowMenusTearOff", true);
}

QMenu* LC_MenuFactoryBase::subMenuWithActions(QMenu* parent, const QString& title, const QString& name, const char* icon,
                                              const QList<QAction*>& actions) const {
    QMenu* sub_menu = doCreateSubMenu(parent, title, name, icon);
    sub_menu->addActions(actions);
    return sub_menu;
}

QMenu* LC_MenuFactoryBase::doCreateSubMenu(QMenu* parent, const QString& title, const QString& name,
                                       const char* icon, const bool supportTearOff) const {
    const auto sub_menu = parent->addMenu(title);
    if (icon != nullptr) {
        sub_menu->setIcon(QIcon(icon));
    }
    if (supportTearOff) {
        sub_menu->setTearOffEnabled(m_allowTearOffMenus);
    }
    QString nameCleared(name);
    nameCleared.remove(' ');
    nameCleared.remove('&');
    const QString& objectName = nameCleared.toLower() + "_menu";
    sub_menu->setObjectName(objectName);
    return sub_menu;
}

QMenu* LC_MenuFactoryBase::menu(const QString& title, const QString& name, QMenuBar* parent) const {
    const auto result = new QMenu(title, parent);
    QString nameCleared(name);
    nameCleared.remove(' ');
    nameCleared.remove('&');
    result->setObjectName(nameCleared.toLower() + "_menu");
    result->setTearOffEnabled(m_allowTearOffMenus);
    return result;
}

QMenu* LC_MenuFactoryBase::menu(const QString& title, const QString& name, QMenuBar* parent,
                            const std::vector<QString>& actionNames) const {
    QMenu* result = menu(title, name, parent);
    addActions(result, actionNames);
    return result;
}

void LC_MenuFactoryBase::addAction(QMenu* menu, const char* actionName) const {
    QAction* action = m_actionGroupManager->getActionByName(actionName);
    if (action != nullptr) {
        menu->addAction(action);
    }
}

void LC_MenuFactoryBase::addActions(QMenu* result, const std::vector<QString>& actionNames) const {
    for (const QString& actionName : actionNames) {
        if (actionName.isEmpty()) {
            result->addSeparator();
        }
        else {
            QAction* action = m_actionGroupManager->getActionByName(actionName);
            if (action != nullptr) {
                result->addAction(action);
            }
        }
    }
}
