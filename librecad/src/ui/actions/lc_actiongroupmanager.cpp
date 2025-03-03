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
    , block(new LC_ActionGroup(this,tr("Block"),tr("Block related operations"), nullptr))
    , circle(new LC_ActionGroup(this,tr("Circle"),tr("Circle drawing commands"),":/icons/circle.svg"))
    , curve(new LC_ActionGroup(this,tr("Curve"), tr("Curve drawing commands"), ":/icons/line_freehand.svg"))
    , edit(new LC_ActionGroup(this,tr("Edit"), tr("Editing operations"), nullptr))
    , ellipse(new LC_ActionGroup(this,tr("Ellipse"),tr("Ellipse drawing commands") ,":/icons/ellipses.svg"))
    , file(new LC_ActionGroup(this,tr("File"),tr("File Operations"), nullptr))
    , dimension(new LC_ActionGroup(this,tr("Dimension"),tr("Dimensions creation commands"), ":/icons/dim_horizontal.svg"))
    , info(new LC_ActionGroup(this,tr("Info"),tr("Informational commands"), ":/icons/measure.svg"))
    , layer(new LC_ActionGroup(this,tr("Layer"),tr("Layers operations"), nullptr))
    , line(new LC_ActionGroup(this,tr("Line"),tr("Line drawing commands"), ":/icons/line.svg"))
    , modify(new LC_ActionGroup(this,tr("Modify"), tr("Modification operations"), ":/icons/move_rotate.svg"))
    , options(new LC_ActionGroup(this,tr("Options"),tr("Options management"), nullptr))
    , other(new LC_ActionGroup(this,tr("Other"),tr("Other operations"), nullptr))
    , polyline(new LC_ActionGroup(this,tr("Polyline"),tr("Polyline drawing commands"),":/icons/polylines_polyline.svg"))
    , restriction(new LC_ActionGroup(this,tr("Restriction"), tr("Snap restrictions"), nullptr))
    , select(new LC_ActionGroup(this,tr("Select"),tr("Entity selection operations"),":/icons/select.svg"))
    , snap(new LC_ActionGroup(this,tr("Snap"),tr("Snapping operations"), nullptr))
    , snap_extras(new LC_ActionGroup(this,tr("Snap Extras"), tr("Additional Snaps"), nullptr))
    , view(new LC_ActionGroup(this,tr("View"),tr("View related operations"), nullptr))
    , widgets(new LC_ActionGroup(this,tr("Widgets"), tr("Widgets management"),nullptr))
    , pen(new LC_ActionGroup(this,tr("PenTB"),tr("Pen related operations"), nullptr)){

    for (auto const& ag : findChildren<QActionGroup*>()) {
        ag->setExclusive(false);
        if (QObject::tr("File") != ag->objectName()
                && QObject::tr("Options") != ag->objectName()) {
            connect( parent, &QC_ApplicationWindow::windowsChanged, ag, &QActionGroup::setEnabled);
        }
    }

    // fixme - review and probably remove relay (it seems that intention was to use it for #570, yet unsuccessfully
    for (auto ag: toolGroups()) {
        connect( ag, &QActionGroup::triggered, parent, &QC_ApplicationWindow::relayAction);
    }

    shortcutsManager = LC_ShortcutsManager();
}

void LC_ActionGroupManager::sortGroupsByName(QList<LC_ActionGroup *> &list) {
    std::sort(list.begin(), list.end(), Sorting::byObjectName);
}

QList<LC_ActionGroup *> LC_ActionGroupManager::toolGroups() {
    QList<LC_ActionGroup *> ag_list;
    ag_list << block
            << circle
            << curve
            << ellipse
            << dimension
            << info
            << line
            << modify
            << other
            << polyline
            << select
            << pen;

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
    shortcutsManager.updateActionTooltips(a_map);
}

void LC_ActionGroupManager::assignShortcutsToActions(QMap<QString, QAction *> &map, std::vector<LC_ShortcutInfo> &shortcutsList) {
    shortcutsManager.assignShortcutsToActions(map, shortcutsList);
}

int LC_ActionGroupManager::loadShortcuts(const QMap<QString, QAction *> &map) {
    a_map = map;
    int loadResult = shortcutsManager.loadShortcuts(a_map);
    return loadResult;
}

int LC_ActionGroupManager::loadShortcuts(const QString &fileName, QMap<QString, QKeySequence> *result) {
    int loadResult = shortcutsManager.loadShortcuts(fileName, result);
    return loadResult;
}

int LC_ActionGroupManager::saveShortcuts(const QList<LC_ShortcutInfo*> &shortcutsList, const QString &fileName) {
    int saveResult = shortcutsManager.saveShortcuts(fileName, shortcutsList);
    return saveResult;
}

int LC_ActionGroupManager::saveShortcuts(QMap<QString, LC_ShortcutInfo *> shortcutsMap) {
    int saveResult = shortcutsManager.saveShortcuts(shortcutsMap, a_map);
    return saveResult;
}

const QString LC_ActionGroupManager::getShortcutsMappingsFolder() {
    return shortcutsManager.getShortcutsMappingsFolder();
}
