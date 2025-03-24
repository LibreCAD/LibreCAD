/*******************************************************************************
 *
 This file is part of the LibreCAD project, a 2D CAD program

 Copyright (C) 2024 LibreCAD.org
 Copyright (C) 2024 sand1024

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

#include <QActionGroup>

#include "lc_actiongroupmanager.h"
#include "qc_applicationwindow.h"
#include "lc_actiongroup.h"
#include "lc_shortcuts_manager.h"


namespace Sorting
{
    bool byObjectName(QActionGroup* left, QActionGroup* right) {
        return left->objectName() < right->objectName();
    }
}

LC_ActionGroupManager::LC_ActionGroupManager(QC_ApplicationWindow *parent)
    :QObject(parent)
    , block(new LC_ActionGroup(this,tr("Block"),tr("Block related operations"), ":/icons/create_block.lci"))
    , circle(new LC_ActionGroup(this,tr("Circle"),tr("Circle drawing commands"),":/icons/circle.lci"))
    , curve(new LC_ActionGroup(this,tr("Arc"), tr("Arc drawing commands"), ":/icons/arc_center_point_angle.lci"))
    , spline(new LC_ActionGroup(this,tr("Spline"), tr("Spline drawing commands"), ":/icons/spline_points.lci"))
    , edit(new LC_ActionGroup(this,tr("Edit"), tr("Editing operations"), ":/icons/rename_active_block.lci"))
    , ellipse(new LC_ActionGroup(this,tr("Ellipse"),tr("Ellipse drawing commands") ,":/icons/ellipses.lci"))
    , file(new LC_ActionGroup(this,tr("File"),tr("File Operations"), ":/icons/save.lci"))
    , dimension(new LC_ActionGroup(this,tr("Dimension"),tr("Dimensions creation commands"), ":/icons/dim_horizontal.lci"))
    , info(new LC_ActionGroup(this,tr("Info"),tr("Informational commands"), ":/icons/measure.lci"))
    , layer(new LC_ActionGroup(this,tr("Layer"),tr("Layers operations"), ":/icons/deselect_layer.lci"))
    , line(new LC_ActionGroup(this,tr("Line"),tr("Line drawing commands"), ":/icons/line.lci"))
    , point(new LC_ActionGroup(this,tr("Point"),tr("Point drawing commands"), ":/icons/points.lci"))
    , shape(new LC_ActionGroup(this,tr("Polygon"),tr("Polygon drawing commands"), ":/icons/rectangle_2_points.lci"))
    , modify(new LC_ActionGroup(this,tr("Modify"), tr("Modification operations"), ":/icons/move_rotate.lci"))
    , options(new LC_ActionGroup(this,tr("Options"),tr("Options management"), ":/icons/settings.lci"))
    , other(new LC_ActionGroup(this,tr("Other"),tr("Other operations"), ":/icons/text.lci"))
    , relZero(new LC_ActionGroup(this,tr("Relative Zero"),tr("Relative Zero"), ":/icons/set_rel_zero.lci"))
    , polyline(new LC_ActionGroup(this,tr("Polyline"),tr("Polyline drawing commands"),":/icons/polylines_polyline.lci"))
    , restriction(new LC_ActionGroup(this,tr("Restriction"), tr("Snap restrictions"), ":/icons/restr_ortho.lci"))
    , select(new LC_ActionGroup(this,tr("Select"),tr("Entity selection operations"),":/icons/select.lci"))
    , snap(new LC_ActionGroup(this,tr("Snap"),tr("Snapping operations"), ":/icons/snap_intersection.lci"))
    , snap_extras(new LC_ActionGroup(this,tr("Snap Extras"), tr("Additional Snaps"), ":/icons/snap_free.lci"))
    , view(new LC_ActionGroup(this,tr("View"),tr("View related operations"), ":/icons/zoom_in.lci"))
    , namedViews(new LC_ActionGroup(this,tr("Named Views"),tr("Persistent Views operations"), ":/icons/visible.lci"))
    , workspaces(new LC_ActionGroup(this,tr("Workspaces"),tr("Workspaces operations"), ":/icons/workspace.lci"))
    , ucs(new LC_ActionGroup(this,tr("UCS"),tr("UCS operations"), ":/icons/set_ucs.lci"))
    , widgets(new LC_ActionGroup(this,tr("Widgets"), tr("Widgets management"),":/icons/dockwidgets_bottom.lci"))
    , pen(new LC_ActionGroup(this,tr("PenTB"),tr("Pen related operations"), ":/icons/pen_apply.lci"))
    , infoCursor(new LC_ActionGroup(this,tr("InfoCursor"),tr("Informational Cursor"), ":/icons/info_cursor_enable.lci")){

    for (auto const& ag : findChildren<QActionGroup*>()) {
        ag->setExclusive(false);
        if (QObject::tr("File") != ag->objectName()
                && QObject::tr("Options") != ag->objectName()) {
            connect( parent, &QC_ApplicationWindow::windowsChanged, ag, &QActionGroup::setEnabled);
        }
    }

    for (auto ag: toolGroups()) {
        connect( ag, &QActionGroup::triggered, parent, &QC_ApplicationWindow::relayAction);
    }

    m_shortcutsManager = LC_ShortcutsManager();
}

void LC_ActionGroupManager::sortGroupsByName(QList<LC_ActionGroup *> &list) {
    std::sort(list.begin(), list.end(), Sorting::byObjectName);
}

QList<LC_ActionGroup *> LC_ActionGroupManager::toolGroups() {
    QList<LC_ActionGroup *> ag_list;
    ag_list << block
            << circle
            << curve
            << spline
            << ellipse
            << dimension
            << info
            << line
            << point
            << shape
            << modify
            << other
            << polyline
            << select
            << pen
            << ucs;
    return ag_list;
}

QList<LC_ActionGroup *> LC_ActionGroupManager::allGroupsList() {
    QList<LC_ActionGroup *> ag_list = findChildren<LC_ActionGroup *>();
    sortGroupsByName(ag_list);
    return ag_list;
}

QMap<QString, LC_ActionGroup *> LC_ActionGroupManager::allGroups() {
    QList<LC_ActionGroup *> ag_list = findChildren<LC_ActionGroup *>();
    sortGroupsByName(ag_list);

    QMap<QString, LC_ActionGroup *> ag_map;

    for (auto ag: ag_list) {
        ag_map[ag->objectName()] = ag;
    }

    return ag_map;
}

void LC_ActionGroupManager::toggleExclusiveSnapMode(bool state) {
    auto snap_actions = snap->actions();

    QList<bool> temp_memory;

    for (auto action: snap_actions) {
        temp_memory << action->isChecked();
        if (action->isChecked()) {
            action->activate(QAction::Trigger);
            action->setChecked(false);
        }
    }

    snap->setExclusive(state);

    if (!snap_memory.isEmpty()) {
        for (int i = 0; i < snap_actions.size(); ++i) {
            if (snap_memory.at(i) == true)
                snap_actions.at(i)->activate(QAction::Trigger);
        }
    }
    snap_memory = temp_memory;
}

void LC_ActionGroupManager::toggleTools(bool state) {
    for (auto group: toolGroups()) {
        for (auto action: group->actions()) {
            action->setDisabled(state);
        }
    }
}

void LC_ActionGroupManager::onOptionsChanged() {
    m_shortcutsManager.updateActionTooltips(m_actionsMap);
}

void LC_ActionGroupManager::assignShortcutsToActions(QMap<QString, QAction *> &map, std::vector<LC_ShortcutInfo> &shortcutsList) {
    m_shortcutsManager.assignShortcutsToActions(map, shortcutsList);
}

int LC_ActionGroupManager::loadShortcuts([[maybe_unused]] const QMap<QString, QAction *> &map) {
//    a_map = map;
    int loadResult = m_shortcutsManager.loadShortcuts(m_actionsMap);
    return loadResult;
}

int LC_ActionGroupManager::loadShortcuts(const QString &fileName, QMap<QString, QKeySequence> *result) {
    int loadResult = m_shortcutsManager.loadShortcuts(fileName, result);
    return loadResult;
}

int LC_ActionGroupManager::saveShortcuts(const QList<LC_ShortcutInfo*> &shortcutsList, const QString &fileName) {
    int saveResult = m_shortcutsManager.saveShortcuts(fileName, shortcutsList);
    return saveResult;
}

int LC_ActionGroupManager::saveShortcuts(QMap<QString, LC_ShortcutInfo *> shortcutsMap) {
    int saveResult = m_shortcutsManager.saveShortcuts(shortcutsMap, m_actionsMap);
    return saveResult;
}

const QString LC_ActionGroupManager::getShortcutsMappingsFolder() {
    return m_shortcutsManager.getShortcutsMappingsFolder();
}

QMap<QString, QAction *> &LC_ActionGroupManager::getActionsMap() {
    return m_actionsMap;
}

QAction* LC_ActionGroupManager::getActionByName(const QString& name) {
     return m_actionsMap.value(name, nullptr);
}

bool LC_ActionGroupManager::hasActionGroup(QString categoryName) {
    QList<LC_ActionGroup *> ag_list = findChildren<LC_ActionGroup *>();
    for (auto ag: ag_list) {
        if (ag->objectName() == categoryName){
            return true;
        }
    }
    return false;
}

LC_ActionGroup* LC_ActionGroupManager::getActionGroup(QString groupName) {
    QList<LC_ActionGroup *> ag_list = findChildren<LC_ActionGroup *>();
    for (auto ag: ag_list) {
        if (ag->objectName() == groupName){
            return ag;
        }
    }
    return nullptr;
}

void  LC_ActionGroupManager::fillActionsList(QList<QAction *> &list, const std::vector<const char *> &actionNames){
    for (const char* actionName: actionNames){
        list << getActionByName(actionName);
    }
}

bool LC_ActionGroupManager::isActionTypeSetsTheIcon(RS2::ActionType actionType){
    return actionType != RS2::ActionSetRelativeZero;
}

void LC_ActionGroupManager::associateQActionWithActionType(QAction *action, RS2::ActionType actionType){
    action->setProperty("RS2:actionType", actionType);
}

void LC_ActionGroupManager::completeInit(){
   for (const auto action: m_actionsMap) {
       auto property = action->property("RS2:actionType");
       if (property.isValid()) {
           auto actionType = property.value<RS2::ActionType>();
           if (isActionTypeSetsTheIcon(actionType)){
               m_actionsByTypes.insert(actionType, action);
           }
       }
   }
}

QAction* LC_ActionGroupManager::getActionByType(RS2::ActionType actionType){
    return m_actionsByTypes.value(actionType);
}
