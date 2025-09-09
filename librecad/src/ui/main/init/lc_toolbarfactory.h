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

class LC_ActionFactory;
class QC_ApplicationWindow;
class QToolBar;
class QToolButton;
class QMenu;
class QAction;
class LC_ActionGroupManager;

class LC_ToolbarFactory:public QObject, public LC_AppWindowAware {
    Q_OBJECT
public:
    explicit LC_ToolbarFactory(QC_ApplicationWindow *main_win);
    ~LC_ToolbarFactory() override = default;
    void initToolBars();
private:
    LC_ActionGroupManager *m_agm;
    LC_ActionFactory *m_actionFactory;
    bool m_showToolbarTooltips{true};
    void createStandardToolbars();
    void addInfoCursorOptionAction(QMenu *menu, const char *name, int tag) const;
    void initCADToolbars() const;
    void createCADToolbars() const;
    void createCustomToolbars();

    QToolBar *createPenToolbar(const QSizePolicy &tbPolicy) const;
    QToolBar *createSnapToolbar(const QSizePolicy &tbPolicy) const;
    QToolBar *createFileToolbar(const QSizePolicy &tbPolicy) const;
    QToolBar *createEditToolbar(const QSizePolicy &tbPolicy) const;
    QToolBar *createOrderToolbar(const QSizePolicy &tbPolicy) const;
    QToolBar *createViewToolbar(const QSizePolicy &tbPolicy) const;
    QToolBar *createDockAreasToolbar(const QSizePolicy &tbPolicy) const;
    QToolBar *createCreatorsToolbar(const QSizePolicy &tbPolicy) const;
    QToolBar *createPreferencesToolbar(const QSizePolicy &tbPolicy) const;
    QToolBar *createInfoCursorToolbar(const QSizePolicy &tbPolicy);
    QToolBar *createEntityLayersToolbar(const QSizePolicy &tbPolicy) const;

    QToolBar *createCategoriesToolbar();
    QToolBar *createNamedViewsToolbar(const QSizePolicy &toolBarPolicy) const;
    QToolBar *createUCSToolbar(const QSizePolicy &toolBarPolicy);
    QToolBar *createWorkspacesToolbar(const QSizePolicy &toolBarPolicy);
    QToolBar *createGenericToolbar(const QString &title, const QString &name, QSizePolicy toolBarPolicy, const std::vector<QString> &actionNames, int group) const;
    QToolBar *doCreateToolBar(const QString &title, const QString &name, const QSizePolicy &toolBarPolicy, int group) const;
    QToolBar *createCADToolbar(const QString &title, const QString &name, QSizePolicy toolBarPolicy, const QList<QAction *> &actions) const;
    QToolBar *genericToolbarWithActions(
        const QString &title, const QString &name, QSizePolicy toolBarPolicy, const QList<QAction *> &actions, int toolbarGroup) const;
    QToolButton *toolButton(QToolBar *toolbar, const QString &tooltip, const char *icon, const QList<QAction *> &actions);
    void addToTop(QToolBar *toolbar) const;
    void addToBottom(QToolBar *toolbar) const;
    void addToLeft(QToolBar *toolbar) const;
    void setToolbarTooltip(QToolBar* toolbar, const QString& text) const;
};
#endif // LC_TOOLBARFACTORY_H
