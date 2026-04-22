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

#ifndef LC_MENUFACTORYMAIN_H
#define LC_MENUFACTORYMAIN_H

#include "lc_menufactory_base.h"

class QC_MDIWindow;
class LC_ActionFactory;
struct LC_MenusHolder;

class LC_MenuFactoryMain: public LC_MenuFactoryBase {
    Q_OBJECT
public:
    explicit LC_MenuFactoryMain(QC_ApplicationWindow* mainWin,  LC_ActionGroupManager* actionGroupManager, LC_ActionFactory* actionFactory, LC_MenusHolder* menusHolder)
        : LC_MenuFactoryBase(mainWin, actionGroupManager, menusHolder), m_actionFactory{actionFactory} {
    }
    void recreateMainMenuIfNeeded(QMenuBar *menuBar);
    void createMainMenu(QMenuBar* menuBar);
    void onWorkspaceMenuAboutToShow(const QList<QC_MDIWindow *> &windowList);
    QMenu* createMainWindowPopupMenu() const;
    QMenu * getRecentFilesMenu() const;
    void recreateToolbarsMenu() const;
protected:
    LC_ActionFactory *m_actionFactory = nullptr;

    struct MenuOptions {
        bool expandToolsMenu = false;
        bool expandToolsTillEntity = false;

        bool isDifferent(const MenuOptions& other) const {
            return expandToolsMenu != other.expandToolsMenu || expandToolsTillEntity != other.expandToolsTillEntity;
        }

        void apply(const MenuOptions &other){
            expandToolsMenu = other.expandToolsMenu;
            expandToolsTillEntity = other.expandToolsTillEntity;
        }
    };

    MenuOptions m_menuOptions;

    void prepareWorkspaceMenuComponents() const;
    void createToolsMenu(QMenuBar *menuBar, QList<QMenu *> &topMenuMenus) const;
    void createHelpMenu(QMenuBar *menuBar, QList<QMenu *> &topMenuMenus);
    void createFileMenu(QMenuBar *menuBar, QList<QMenu *> &topMenuMenus) const;
    void createSettingsMenu(QMenuBar *menuBar, QList<QMenu *> &topMenuMenus) const;
    void createEditMenu(QMenuBar *menuBar, QList<QMenu *> &topMenuMenus) const;
    void createViewMenu(QMenuBar *menuBar, QList<QMenu *> &topMenuMenus) const;
    void createPluginsMenu(QMenuBar *menuBar, QList<QMenu *> &topMenuMenus) const;
    void createWorkspaceMenu(QMenuBar *menuBar, QList<QMenu *> &topMenuMenus);

    void doCreateMenus(QMenuBar *menuBar, bool firstCreation);
    QAction* urlActionTR(const QString& title, const char *url);
    void createToolsMenuCombined(QMenuBar *menuBar, QList<QMenu *> &topMenuMenus) const;
    void createToolsMenuExpanded(QMenuBar *menuBar, QList<QMenu *> &topMenuMenus) const;
};

#endif
