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
#ifndef LC_TOOLBARFACTORY_H
#define LC_TOOLBARFACTORY_H

#include <QObject>
#include "lc_appwindowaware.h"

class QC_ApplicationWindow;
class QToolBar;
class QToolButton;
class QMenu;
class QAction;
class LC_ActionGroupManager;

class LC_ToolbarFactory:public QObject, public LC_AppWindowAware {
    Q_OBJECT
public:
    LC_ToolbarFactory(QC_ApplicationWindow *main_win);
    ~LC_ToolbarFactory() override = default;
    void initToolBars();
private:
    LC_ActionGroupManager *m_agm {nullptr};
    void createStandardToolbars();
    void addInfoCursorOptionAction(QMenu *menu, const char *name, int tag);
    void initCADToolbars();
    void createCADToolbars();
    void createCustomToolbars();

    QToolBar *createPenToolbar(QSizePolicy tbPolicy);
    QToolBar *createSnapToolbar(QSizePolicy tbPolicy);
    QToolBar *createFileToolbar(QSizePolicy &tbPolicy);
    QToolBar *createEditToolbar(QSizePolicy &tbPolicy);
    QToolBar *createOrderToolbar(QSizePolicy &tbPolicy);
    QToolBar *createViewToolbar(QSizePolicy &tbPolicy);
    QToolBar *createDockAreasToolbar(QSizePolicy &tbPolicy);
    QToolBar *createCreatorsToolbar(QSizePolicy &tbPolicy);
    QToolBar *createPreferencesToolbar(QSizePolicy &tbPolicy);
    QToolBar *createInfoCursorToolbar(QSizePolicy &tbPolicy);

    QToolBar *createCategoriesToolbar();
    QToolBar *createNamedViewsToolbar(QSizePolicy &toolBarPolicy);
    QToolBar *createUCSToolbar(QSizePolicy &toolBarPolicy);
    QToolBar *createWorkspacesToolbar(QSizePolicy &toolBarPolicy);
    QToolBar *createGenericToolbar(const QString &title, const QString &name, QSizePolicy toolBarPolicy, const std::vector<QString> &actionNames, int group);
    QToolBar *doCreateToolBar(const QString &title, const QString &name, const QSizePolicy &toolBarPolicy, int group) const;
    QToolBar *createCADToolbar(const QString &title, const QString &name, QSizePolicy toolBarPolicy, const QList<QAction *> &actions);
    QToolBar *genericToolbarWithActions(
        const QString &title, const QString &name, QSizePolicy toolBarPolicy, const QList<QAction *> &actions, int toolbarGroup);
    QToolButton *toolButton(QToolBar *toolbar, const QString &tooltip, const char *icon, const QList<QAction *> &actions);
    void addToTop(QToolBar *toolbar);
    void addToBottom(QToolBar *toolbar);
    void addToLeft(QToolBar *toolbar);
};
#endif // LC_TOOLBARFACTORY_H
