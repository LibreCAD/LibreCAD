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

#include <qtoolbar.h>

#include "lc_actioncontext.h"
#include "lc_menufactory_graphicview.h"
#include "lc_menufactory_main.h"

class QG_GraphicView;
class QC_MDIWindow;
class QMenuBar;
class QAction;
class QMenu;
class LC_ActionGroupManager;
class QC_ApplicationWindow;

struct LC_MenusHolder {
    QMenu* m_menuRecentFiles{nullptr};
    QMenu* m_menuDockAreas {nullptr};
    QMenu* m_menuToolBarAreas {nullptr};
    QMenu* m_menuDockWidgets {nullptr};
    QMenu* m_menuCADDockWidgets {nullptr};
    QMenu* m_menuToolbars {nullptr};
    QMenu* m_menuCADToolbars {nullptr};
    QMenu* m_menuFile {nullptr};
    QMenu* m_menuSettings {nullptr};
    QMenu* m_menuEdit {nullptr};
    QMenu* m_menuView {nullptr};
    QMenu* m_menuPlugins {nullptr};
    QMenu* m_menuWorkspace {nullptr};
    QMenu* m_menuHelp {nullptr};
    QMenu* m_menuToolsCombined {nullptr};
};

class LC_MenuFactory{
public:
    explicit LC_MenuFactory(QC_ApplicationWindow* mainWin);
    void recreateMainMenuIfNeeded(QMenuBar *menuBar) {m_menuFactoryMain.recreateMainMenuIfNeeded(menuBar);}
    void createMainMenu(QMenuBar* menuBar) {m_menuFactoryMain.createMainMenu(menuBar);}
    void onWorkspaceMenuAboutToShow(const QList<QC_MDIWindow *> &windowList) {m_menuFactoryMain.onWorkspaceMenuAboutToShow(windowList);}
    QMenu* createMainWindowPopupMenu() const {return m_menuFactoryMain.createMainWindowPopupMenu();}
    QMenu * getRecentFilesMenu() const{ return m_menuFactoryMain.getRecentFilesMenu();}
    QMenu* createGraphicViewPopupMenu(QG_GraphicView* graphicView, RS_Entity* contextEntity,
                                      const RS_Vector& contextPosition, QStringList& actionNames, bool mayInvokeDefaultMenu) {
        return m_menuFactoryGraphicView.createGraphicViewPopupMenu(graphicView, contextEntity, contextPosition, actionNames, mayInvokeDefaultMenu);
    }

    void recreateToolbarsMenu();

protected:
   LC_MenusHolder m_menusHolder;
   LC_MenuFactoryMain m_menuFactoryMain;
   LC_MenuFactoryGraphicView m_menuFactoryGraphicView;
};

#endif
