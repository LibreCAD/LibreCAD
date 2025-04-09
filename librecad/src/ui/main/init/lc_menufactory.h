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

#ifndef LC_MENUFACTORY_H
#define LC_MENUFACTORY_H

#include <QList>
#include <QObject>

#include "lc_appwindowaware.h"
#include "qc_applicationwindow.h"

class QMenuBar;
class QAction;
class QMenu;
class LC_ActionGroupManager;

class LC_MenuFactory: public QObject, public LC_AppWindowAware{
    Q_OBJECT
public:
    LC_MenuFactory(QC_ApplicationWindow* main_win,
                   LC_ActionGroupManager* agm);
    void recreateMainMenuIfNeeded(QMenuBar *menuBar);
    void createMainMenu(QMenuBar* menu_bar);
    void onWorkspaceMenuAboutToShow(const QList<QC_MDIWindow *> &window_list);
    QMenu* createMainWindowPopupMenu();
    QMenu * getRecentFilesMenu() const{
        return m_menuRecentFiles;
    }
protected:
    LC_ActionGroupManager* m_actionGroupManager = nullptr;

    struct MenuOptions {
        bool expandToolsMenu = false;
        bool expandToolsTillEntity = false;

        bool isDifferent(MenuOptions& other){
            return expandToolsMenu != other.expandToolsMenu || expandToolsTillEntity != other.expandToolsTillEntity;
        }

        void apply(MenuOptions &other){
            expandToolsMenu = other.expandToolsMenu;
            expandToolsTillEntity = other.expandToolsTillEntity;
        };
    };

    MenuOptions m_menuOptions;

    // --- Menus ---

    QMenu* m_menuFile {nullptr};
    QMenu* m_menuSettings {nullptr};
    QMenu* m_menuEdit {nullptr};
    QMenu* m_menuView {nullptr};
    QMenu* m_menuPlugins {nullptr};
    QMenu* m_menuWorkspace {nullptr};
    QMenu* m_menuHelp {nullptr};
    QMenu* m_menuToolsCombined {nullptr};

    QMenu* m_menuRecentFiles{nullptr};
    QMenu* m_menuDockareas {nullptr};
    QMenu* m_menuDockWidgets {nullptr};
    QMenu* m_menuCADDockWidgets {nullptr};
    QMenu* m_menuToolbars {nullptr};
    QMenu* m_menuCADToolbars {nullptr};

    bool m_allowTearOffMenus = true;

    void prepareWorkspaceMenuComponents();
    void createToolsMenu(QMenuBar *menu_bar, QList<QMenu *> &topMenuMenus);
    void createHelpMenu(QMenuBar *menu_bar, QList<QMenu *> &topMenuMenus);
    void createFileMenu(QMenuBar *menu_bar, QList<QMenu *> &topMenuMenus);
    void createSettingsMenu(QMenuBar *menu_bar, QList<QMenu *> &topMenuMenus);
    void createEditMenu(QMenuBar *menu_bar, QList<QMenu *> &topMenuMenus);
    void createViewMenu(QMenuBar *menu_bar, QList<QMenu *> &topMenuMenus);
    void createPluginsMenu(QMenuBar *menu_bar, QList<QMenu *> &topMenuMenus);
    void createWorkspaceMenu(QMenuBar *menu_bar, QList<QMenu *> &topMenuMenus);
    void doCreateMenus(QMenuBar *menu_bar, bool firstCreation);

    QAction* urlActionTR(const QString& title, const char *url);
    void addAction(QMenu *menu, const char *actionName);
    void addActions(QMenu *result, const std::vector<QString> &actionNames);
    QMenu* subMenuWithActions(QMenu *parent, const QString& title, const QString& name, const char *icon, const QList<QAction *> &actions);
    QMenu* menu(const QString& title, const QString& name, QMenuBar* parent);
    QMenu *menu(const QString& title, const QString& name,  QMenuBar *parent, const std::vector<QString> &actionNames);
    QMenu *doCreateSubMenu(QMenu *parent, const QString& title, const QString& name, const char *icon) const;
    QMenu *subMenu(QMenu *parent, const QString& title, const QString& name, const char *icon, const std::vector<QString> &actionNames);
    void createToolsMenuCombined(QMenuBar *menu_bar, QList<QMenu *> &topMenuMenus);
    void createToolsMenuExpanded(QMenuBar *menu_bar, QList<QMenu *> &topMenuMenus);
};

#endif // LC_MENUFACTORY_H
