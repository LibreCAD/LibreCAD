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

#ifndef LC_ACTIONFACTORY_H
#define LC_ACTIONFACTORY_H

#include "lc_actionfactorybase.h"
#include "lc_actiongroup.h"

class QActionGroup;
class QAction;
class LC_ActionGroupManager;
class QC_ApplicationWindow;
class QG_ActionHandler;

class LC_ActionFactory : public LC_ActionFactoryBase{
    Q_OBJECT
public:
    LC_ActionFactory(QC_ApplicationWindow* parent, QG_ActionHandler* a_handler);
    void initActions(LC_ActionGroupManager* agm, bool useTheme);

    QList<QAction*> file_actions;
    QList<QAction*> line_actions;
    QList<QAction*> point_actions;
    QList<QAction*> shape_actions;
    QList<QAction*> circle_actions;
    QList<QAction*> curve_actions;
    QList<QAction*> spline_actions;
    QList<QAction*> ellipse_actions;
    QList<QAction*> polyline_actions;
    QList<QAction*> select_actions;
    QList<QAction*> dimension_actions;
    QList<QAction*> other_drawing_actions;
    QList<QAction*> modify_actions;
    QList<QAction*> order_actions;
    QList<QAction*> info_actions;
    QList<QAction*> layer_actions;
    QList<QAction*> block_actions;
    QList<QAction*> pen_actions;
    QList<QAction*> entity_layer_actions;
private:
    void initActionGroupManager(LC_ActionGroupManager* agm);
    void createEntityLayerActions(QMap<QString, QAction*>& map, LC_ActionGroup* group);
    void fillActionContainer(LC_ActionGroupManager* agm, bool useTheme);
    void createDrawLineActions(QMap<QString, QAction *> &map, QActionGroup *group) const;
    void createDrawPointsActions(QMap<QString, QAction *> &map, QActionGroup *group) const;
    void createDrawShapeActions(QMap<QString, QAction *> &map, QActionGroup *group) const;
    void createInfoActions(QMap<QString, QAction *> &map, QActionGroup *group) const;
    void createViewActions(QMap<QString, QAction *> &map, QActionGroup *group);
    void createDrawCircleActions(QMap<QString, QAction *> &map, QActionGroup *group) const;
    void createSelectActions(QMap<QString, QAction *> &map, QActionGroup *group) const;
    void createDrawCurveActions(QMap<QString, QAction *> &map, QActionGroup *group) const;
    void createDrawSplineActions(QMap<QString, QAction *> &map, QActionGroup *group) const;
    void createDrawEllipseActions(QMap<QString, QAction *> &map, QActionGroup *group) const;
    void createDrawPolylineActions(QMap<QString, QAction *> &map, QActionGroup *group) const;
    void createDrawOtherActions(QMap<QString, QAction *> &map, QActionGroup *group) const;
    void createDrawDimensionsActions(QMap<QString, QAction *> &map, QActionGroup *group) const;
    void createModifyActions(QMap<QString, QAction *> &map, QActionGroup *pGroup) const;
    void createPenActions(QMap<QString, QAction *> &map, QActionGroup *group) const;
    void createFileActions(QMap<QString, QAction *> &map, QActionGroup *group);
    void createPenActionsUncheckable(QMap<QString, QAction *> &map, QActionGroup *group) const;
    void createOrderActionsUncheckable(QMap<QString, QAction *> &map, QActionGroup *group) const;
    void createLayerActionsUncheckable(QMap<QString, QAction *> &map, QActionGroup *group) const;
    void createBlockActionsUncheckable(QMap<QString, QAction *> &map, QActionGroup *group) const;
    void createOptionsActionsUncheckable(QMap<QString, QAction *> &map, QActionGroup *group);
    void createFileActionsUncheckable(QMap<QString, QAction *> &map, QActionGroup *group);
    void createWidgetActionsUncheckable(QMap<QString, QAction *> &map, QActionGroup *group);
    void createWidgetActions(QMap<QString, QAction *> &map, QActionGroup *group);
    void createViewActionsUncheckable(QMap<QString, QAction *> &map, QActionGroup *group);
    void createSelectActionsUncheckable(QMap<QString, QAction *> &map, QActionGroup *group) const;
    void createEditActionsUncheckable(QMap<QString, QAction *> &map, QActionGroup *group) const;
    void createDrawDimensionsUncheckable(QMap<QString, QAction*>& map, QActionGroup* group) const;
    void createInteractivePickActions(QMap<QString, QAction*>& map, QActionGroup* group) const;
    void createWorkspacesActionsUncheckable(QMap<QString, QAction *> &map, QActionGroup *group);
    void createNamedViewActionsUncheckable(QMap<QString, QAction *> &map, QActionGroup *group);

    void createEditActions(QMap<QString, QAction *> &map, QActionGroup *group) const;
    void createSnapActions(QMap<QString, QAction *> &map, QActionGroup *group) const;
    void createInfoCursorActions(QMap<QString, QAction *> &map, QActionGroup *group);
    void createSnapExtraActions(QMap<QString, QAction *> &map, QActionGroup *group) const;
    void setDefaultShortcuts(QMap<QString, QAction *> &map, LC_ActionGroupManager* agm);
    void setupCreatedActions(QMap<QString, QAction *> &map);
    void markNotEditableActionsShortcuts(const QMap<QString, QAction *> &map);
    void fillActionLists(QMap<QString, QAction *> &map);
    void prepareActionsToDisableInPrintPreview(QList<QAction *> &actionsList,QMap<QString, QAction *> &map);
    void createRestrictActions(QMap<QString, QAction *> &map, QActionGroup *group) const;
    void createRelZeroActions(QMap<QString, QAction *> &map, QActionGroup *group) const;
    void createUCSActions(QMap<QString, QAction *> &map, QActionGroup *group) const;

};

#endif
