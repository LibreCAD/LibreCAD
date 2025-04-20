// /****************************************************************************
//
// Utility base class for widgets that represents options for actions
//
// Copyright (C) 2025 LibreCAD.org
// Copyright (C) 2025 sand1024
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
// **********************************************************************
//
#ifndef LC_WIDGETFACTORY_H
#define LC_WIDGETFACTORY_H

#include <QObject>
#include <lc_appwindowaware.h>

#include "lc_actionfactory.h"

class QAction;
class QToolBar;
class QDockWidget;
class QG_ActionHandler;
class LC_CADDockWidget;
class QC_ApplicationWindow;
class LC_ActionGroupManager;
/**
 * creates the widgets and adds them to the main window;
 * it also "tags" a few widgets that the main window uses
 */
class LC_WidgetFactory:public QObject, public LC_AppWindowAware {
    Q_OBJECT
public:
    LC_WidgetFactory(QC_ApplicationWindow *main_win);
    ~LC_WidgetFactory() override = default;
    void initWidgets();
    static void updateDockWidgetsTitleBarType(const QC_ApplicationWindow* mainWin, bool verticalTitle);
    static void updateDockOptions(QC_ApplicationWindow* mainWin, bool allowDockNesting, bool verticalTabs);
private:
    LC_ActionGroupManager *m_agm {nullptr};
    LC_ActionFactory *m_actionFactory {nullptr};
    QDockWidget *createDockWidget(const QString &title, const char *name, const QString& verticalTitle = "") const;
    QDockWidget *createPenPalletteWidget();
    QDockWidget *createLayerWidget(QG_ActionHandler *action_handler);
    QDockWidget *createNamedViewsWidget();
    QDockWidget *createUCSListWidget();
    QDockWidget *createLayerTreeWidget(QG_ActionHandler *action_handler);
    QDockWidget *createEntityInfoWidget();
    QDockWidget *createBlockListWidget(QG_ActionHandler *actionHandler);
    QDockWidget *createLibraryWidget(QG_ActionHandler *action_handler);
    QDockWidget *createCmdWidget(QG_ActionHandler *action_handler);
    void modifyCommandTitleBar(Qt::DockWidgetArea area) const;
    QDockWidget* createPenWizardWidget();
    void initLeftCADSidebar();
    void createRightSidebar(QG_ActionHandler *action_handler);
    void initStatusBar();
    void createCADSidebar(int columns, int icon_size, bool flatButtons);
    LC_CADDockWidget *cadDockWidget(const QString &title, const char *name, const QList<QAction *> &actions, int columns, int iconSize, bool flatButtons);
    void addToBottom(QToolBar *toolbar) const;
    QToolBar *createStatusBarToolbar(QSizePolicy tbPolicy, QWidget *widget, const QString& title, const char *name) const;
    void addAction(QToolBar *toolbar, const char *actionName) const;
    void makeActionsInvisible(const std::vector<QString> &actionNames) const;
    static void setDockWidgetTitleType(QDockWidget *widget, bool verticalTitleBar);
};
#endif // LC_WIDGETFACTORY_H
