/*
**********************************************************************************
**
** This file is part of the LibreCAD project (librecad.org), a 2D CAD program.
**
** Copyright (C) 2015 ravas (github.com/r-a-v-a-s)
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**
**********************************************************************************
*/

#ifndef LC_ACTIONFACTORY_H
#define LC_ACTIONFACTORY_H

#include <QObject>
#include <QMap>
#include "rs.h"
#include "lc_actionfactorybase.h"
#include "lc_shortcuts_manager.h"

class QActionGroup;
class QAction;
class LC_ActionGroupManager;
class QC_ApplicationWindow;
class QG_ActionHandler;

class LC_ActionFactory : public LC_ActionFactoryBase{
    Q_OBJECT
public:
    LC_ActionFactory(QC_ApplicationWindow* parent, QG_ActionHandler* a_handler);
    void fillActionContainer(QMap<QString, QAction*>& a_map, LC_ActionGroupManager* agm, bool useTheme);
private:

    void createDrawLineActions(QMap<QString, QAction *> &map, QActionGroup *group);
    void createInfoActions(QMap<QString, QAction *> &map, QActionGroup *group);
    void createViewActions(QMap<QString, QAction *> &map, QActionGroup *group);
    void createDrawCircleActions(QMap<QString, QAction *> &map, QActionGroup *group);
    void createSelectActions(QMap<QString, QAction *> &map, QActionGroup *group);
    void createDrawCurveActions(QMap<QString, QAction *> &map, QActionGroup *group);
    void createDrawEllipseActions(QMap<QString, QAction *> &map, QActionGroup *group);
    void createDrawPolylineActions(QMap<QString, QAction *> &map, QActionGroup *group);
    void createDrawOtherActions(QMap<QString, QAction *> &map, QActionGroup *group);
    void createDrawDimensionsActions(QMap<QString, QAction *> &map, QActionGroup *group);
    void createModifyActions(QMap<QString, QAction *> &map, QActionGroup *pGroup);
    void createPenActions(QMap<QString, QAction *> &map, QActionGroup *group);
    void createFileActions(QMap<QString, QAction *> &map, QActionGroup *group);
    void createPenActionsUncheckable(QMap<QString, QAction *> &map, QActionGroup *group);
    void createOrderActionsUncheckable(QMap<QString, QAction *> &map, QActionGroup *group);
    void createLayerActionsUncheckable(QMap<QString, QAction *> &map, QActionGroup *group);
    void createBlockActionsUncheckable(QMap<QString, QAction *> &map, QActionGroup *group);
    void createOptionsActionsUncheckable(QMap<QString, QAction *> &map, QActionGroup *group);
    void createFileActionsUncheckable(QMap<QString, QAction *> &map, QActionGroup *group);
    void createWidgetActionsUncheckable(QMap<QString, QAction *> &map, QActionGroup *group);
    void createWidgetActions(QMap<QString, QAction *> &map, QActionGroup *group);
    void createViewActionsUncheckable(QMap<QString, QAction *> &map, QActionGroup *group);
    void createSelectActionsUncheckable(QMap<QString, QAction *> &map, QActionGroup *group);
    void createEditActionsUncheckable(QMap<QString, QAction *> &map, QActionGroup *group);
    void createSnapActions(QMap<QString, QAction *> &map, QActionGroup *group);
    void createSnapExtraActions(QMap<QString, QAction *> &map, QActionGroup *group);
    void setDefaultShortcuts(QMap<QString, QAction *> &map, LC_ActionGroupManager* agm);
    void setupCreatedActions(QMap<QString, QAction *> &map);
    void markNotEditableActionsShortcuts(QMap<QString, QAction *> &map);

    void createRestrictActions(QMap<QString, QAction *> &map, QActionGroup *group);

    void createOtherActions(QMap<QString, QAction *> &map, QActionGroup *group);
};

#endif
