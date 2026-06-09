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

#ifndef LC_MENUFACTORYBASE_H
#define LC_MENUFACTORYBASE_H

#include <QMenuBar>
#include <QObject>

#include "lc_appwindowaware.h"

class QDockWidget;
class LC_ActionGroupManager;
struct LC_MenusHolder;

class LC_MenuFactoryBase: public QObject, public LC_AppWindowAware {
    Q_OBJECT
public:
    LC_MenuFactoryBase(QC_ApplicationWindow* mainWin, LC_ActionGroupManager* actionGroupManager, LC_MenusHolder* menusHolder);
protected:
    QMenu* subMenuWithActions(QMenu *parent, const QString& title, const QString& name, const char *icon, const QList<QAction *> &actions) const;
    QMenu *subMenu(QMenu *parent, const QString& title, const QString& name, const char *icon, const std::vector<QString> &actionNames, bool supportTearOff = true) const;
    void addAction(QMenu *menu, const char *actionName) const;
    void addActions(QMenu *result, const std::vector<QString> &actionNames) const;

    QMenu* menu(const QString& title, const QString& name, QMenuBar* parent) const;
    QMenu *menu(const QString& title, const QString& name,  QMenuBar *parent, const std::vector<QString> &actionNames) const;
    QMenu *doCreateSubMenu(QMenu *parent, const QString& title, const QString& name, const char *icon, bool supportTearOff = true) const;

    void findViewAndUCSToggleActions(QList<QDockWidget*> dockWidgetsList, QAction*& namedViewsToggleViewAction,
                                     QAction*& ucsToggleViewAction) const;

    void createWorkspacesListSubMenu(QMenu* parentMenu);

    LC_ActionGroupManager* m_actionGroupManager = nullptr;
    bool m_allowTearOffMenus = true;
    LC_MenusHolder* m_menusHolder = nullptr;
};

#endif
