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

#include "lc_actiongroupmanager.h"

#include "qc_applicationwindow.h"
#include "lc_actiongroup.h"
#include "lc_shortcuts_manager.h"


namespace Sorting
{
    bool byObjectName(const QActionGroup* left, const QActionGroup* right) {
        return left->objectName() < right->objectName();
    }
}

LC_ActionGroupManager::LC_ActionGroupManager(QC_ApplicationWindow* parent)
    : QObject(parent),
      m_shortcutsManager{std::make_unique<LC_ShortcutsManager>()} {
}


LC_ActionGroupManager::~LC_ActionGroupManager() = default;

void LC_ActionGroupManager::sortGroupsByName(QList<LC_ActionGroup *> &list) {
    std::sort(list.begin(), list.end(), Sorting::byObjectName);
}

QList<LC_ActionGroup *> LC_ActionGroupManager::toolGroups() const {
    /*QList<LC_ActionGroup *> ag_list;
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
    return ag_list;*/
    return m_toolsGroups;
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
    auto snap = getGroupByName("snap");
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

void LC_ActionGroupManager::onOptionsChanged() const {
    m_shortcutsManager->updateActionTooltips(m_actionsMap);
}

void LC_ActionGroupManager::assignShortcutsToActions(QMap<QString, QAction *> &map, std::vector<LC_ShortcutInfo> &shortcutsList) const {
    m_shortcutsManager->assignShortcutsToActions(map, shortcutsList);
}

int LC_ActionGroupManager::loadShortcuts([[maybe_unused]] const QMap<QString, QAction *> &map) {
    m_shortcutsManager->init();
    int loadResult = m_shortcutsManager->loadShortcuts(m_actionsMap);
    return loadResult;
}

int LC_ActionGroupManager::loadShortcuts(const QString &fileName, QMap<QString, QKeySequence> *result) const {
    int loadResult = m_shortcutsManager->loadShortcuts(fileName, result);
    return loadResult;
}

int LC_ActionGroupManager::saveShortcuts(const QList<LC_ShortcutInfo*> &shortcutsList, const QString &fileName) const {
    int saveResult = m_shortcutsManager->saveShortcuts(fileName, shortcutsList);
    return saveResult;
}

int LC_ActionGroupManager::saveShortcuts(QMap<QString, LC_ShortcutInfo *> shortcutsMap) {
    int saveResult = m_shortcutsManager->saveShortcuts(shortcutsMap, m_actionsMap);
    return saveResult;
}

const QString LC_ActionGroupManager::getShortcutsMappingsFolder() const {
    return m_shortcutsManager->getShortcutsMappingsFolder();
}

QMap<QString, QAction *> &LC_ActionGroupManager::getActionsMap() {
    return m_actionsMap;
}

QAction* LC_ActionGroupManager::getActionByName(const QString& name) const {
     return m_actionsMap.value(name, nullptr);
}

bool LC_ActionGroupManager::hasActionGroup(const QString& categoryName) const {
    return m_actionGroups.contains(categoryName);
}

LC_ActionGroup* LC_ActionGroupManager::getActionGroup(const QString& groupName) {
    return m_actionGroups.value(groupName, nullptr);
}


bool LC_ActionGroupManager::isActionTypeSetsTheIcon([[maybe_unused]]RS2::ActionType actionType){
    // return actionType != RS2::ActionSetRelativeZero;
    return true;
}

void LC_ActionGroupManager::associateQActionWithActionType(QAction *action, RS2::ActionType actionType){
    action->setProperty("RS2:actionType", actionType);
}

void LC_ActionGroupManager::persist() {
    // intentionally do nothing so far. This method will be expanded later, as if will be clear
    // whether it's necessary to save shortcuts on program's exit or not.
}

LC_ActionGroup* LC_ActionGroupManager::getGroupByName(const QString& name) const {
    if (m_actionGroups.contains(name)) {
        return m_actionGroups.value(name, nullptr);
    }
    return nullptr;
}

void LC_ActionGroupManager::addActionGroup(const QString& name, LC_ActionGroup* actionGroup, bool toolsGroup) {
    m_actionGroups.insert(name, actionGroup);
    if (toolsGroup) {
       m_toolsGroups << actionGroup;
    }
}

void LC_ActionGroupManager::completeInit(){
   for (const auto action: m_actionsMap) {
       if (action != nullptr) { // fixme - sand - check where from null may be inserted to action map
           auto property = action->property("RS2:actionType");
           if (property.isValid()) {
               auto actionType = property.value<RS2::ActionType>();
               if (isActionTypeSetsTheIcon(actionType)){
                   m_actionsByTypes.insert(actionType, action);
               }
           }
       }
   }
}

QAction* LC_ActionGroupManager::getActionByType(RS2::ActionType actionType){
    return m_actionsByTypes.value(actionType);
}
