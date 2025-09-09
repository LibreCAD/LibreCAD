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

#include <QObject>
#include <qdockwidget.h>

#include "lc_actioncontext.h"
#include "lc_actionfactory.h"
#include "lc_appwindowaware.h"
#include "rs_entity.h"

class QG_GraphicView;
class QC_MDIWindow;
class QMenuBar;
class QAction;
class QMenu;
class LC_ActionGroupManager;

class LC_MenuFactory: public QObject, public LC_AppWindowAware{
    Q_OBJECT
public:
    explicit LC_MenuFactory(QC_ApplicationWindow* main_win);
    void recreateMainMenuIfNeeded(QMenuBar *menuBar);
    void createMainMenu(QMenuBar* menu_bar);
    void onWorkspaceMenuAboutToShow(const QList<QC_MDIWindow *> &window_list);
    QMenu* createMainWindowPopupMenu() const;
    QMenu* createGraphicViewPopupMenu(QG_GraphicView* graphicView, RS_Entity* contextEntity,
                                      const RS_Vector& contextPosition, QStringList& actionNames, bool mayInvokeDefaultMenu);

    QMenu * getRecentFilesMenu() const{
        return m_menuRecentFiles;
    }
protected:
    LC_ActionGroupManager* m_actionGroupManager = nullptr;
    LC_ActionFactory *m_actionFactory = nullptr;

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
    void findViewAndUCSToggleActions(QList<QDockWidget*> dockwidgetsList, QAction*& namedViewsToggleViewAction,
                                     QAction*& ucsToggleViewAction);
    void doCreateMenus(QMenuBar *menu_bar, bool firstCreation);

    void addProxyActions(QMenu* menu, RS_Entity* entity, const RS_Vector& pos, LC_ActionContext* actionContext,
                         const std::vector<QString>& actionNames) const;
    void addActionProxy(QMenu* menu, QAction* srcAction, RS_Entity* entity, const RS_Vector& pos,
                           LC_ActionContext* actionContext) const;
    void addActionProxy(QMenu* menu, const QString& actionName, RS_Entity* entity, const RS_Vector& pos,
                        LC_ActionContext* actionContext) const;

    QAction* urlActionTR(const QString& title, const char *url);
    void addAction(QMenu *menu, const char *actionName) const;
    void addActions(QMenu *result, const std::vector<QString> &actionNames) const;
    QMenu* subMenuWithActions(QMenu *parent, const QString& title, const QString& name, const char *icon, const QList<QAction *> &actions) const;
    QMenu* menu(const QString& title, const QString& name, QMenuBar* parent) const;
    QMenu *menu(const QString& title, const QString& name,  QMenuBar *parent, const std::vector<QString> &actionNames) const;
    QMenu *doCreateSubMenu(QMenu *parent, const QString& title, const QString& name, const char *icon, bool supportTearOff = true) const;
    QMenu *subMenu(QMenu *parent, const QString& title, const QString& name, const char *icon, const std::vector<QString> &actionNames, bool supportTearOff = true) const;
    void createToolsMenuCombined(QMenuBar *menu_bar, QList<QMenu *> &topMenuMenus);
    void createToolsMenuExpanded(QMenuBar *menu_bar, QList<QMenu *> &topMenuMenus) const;
    void createGVMenuView(QMenu* ctxMenu);
    void createGVMenuFiles(QMenu* menu);
    void createGVMenuEntitySpecific(QMenu* contextMenu, QG_GraphicView* graphicView, RS_Entity* entity, const RS_Vector& pos);
    void createGVEditPropertiesAction(QMenu* menu, QG_GraphicView* graphicView, RS_Entity* entity);
    void createGVEditPropertiesAction(QMenu* menu, QG_GraphicView* graphicView, RS_Entity* entity, const QString &actionText);
    void createGVMenuModifyGeneral(QMenu* contextMenu, QG_GraphicView* graphicView, RS_Entity* entity, const RS_Vector& pos,
                                   LC_ActionContext* actionContext);
    void createGVMenuSelect(QMenu* ctxMenu, RS_Entity* contextEntity,const  RS_Vector &contextPosition,
                            LC_ActionContext* actionContext, int selectionCount);
    void createGVMenuRecent(QG_GraphicView* graphicView, QMenu* ctxMenu, LC_ActionContext* actionContext,
                            RS_Entity* contextEntity, const RS_Vector &contextPosition, bool hasEntity);

    void createGVMenuEdit(QMenu* ctxMenu, LC_ActionContext* actionContext,RS_Entity* contextEntity, const RS_Vector &contextPosition);
    void createGVMenuOptions(QMenu* ctxMenu);
    QMenu* addProxyActionsSubMenu(QMenu* menu, const QString &subMenuName, const char* subMenuIconName, RS_Entity* entity,
                                  const RS_Vector& pos, LC_ActionContext* actionContext,
                                  const std::vector<QString>& actionNames) const;
    QMenu* createGraphicViewDefaultPopupMenu(QG_GraphicView* graphicView,
                                             RS_Entity* contextEntity, const RS_Vector& contextPosition);
    QMenu* createGraphicViewCustomPopupMenu(QG_GraphicView* graphicView,
                                            RS_Entity* contextEntity, const RS_Vector& contextPosition,
                                            QStringList& actionNames);
};

#endif // LC_MENUFACTORY_H
