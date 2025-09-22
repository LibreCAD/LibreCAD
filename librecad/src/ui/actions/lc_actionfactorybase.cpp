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
#include "lc_actionfactorybase.h"

#include "lc_actiongroup.h"
#include "lc_actiongroupmanager.h"
#include "lc_shortcutinfo.h"
#include "qc_applicationwindow.h"
#include "qg_actionhandler.h"

LC_ActionFactoryBase::LC_ActionFactoryBase(QC_ApplicationWindow* parent, QG_ActionHandler* a_handler):
    QObject(parent), LC_AppWindowAware(parent),m_actionHandler(a_handler){
}

QAction* LC_ActionFactoryBase::createAction_MW(const char* name, void (QC_ApplicationWindow::*slotPtr)(),
                                               void (QC_ApplicationWindow::*slotBoolPtr)(bool), const QString& text,
                                               const char* iconName, const char* themeIconName,
                                               QActionGroup* parent, QMap<QString, QAction*>& a_map,
                                               bool useToggled) const {
    QAction* action = justCreateAction(a_map, name, text, iconName, themeIconName, parent);
    if (slotPtr != nullptr) {
        if (useToggled) {
            connect(action, &QAction::toggled, m_appWin, slotPtr);
        } else {
            connect(action, &QAction::triggered, m_appWin, slotPtr);
        }
    }else if (slotBoolPtr != nullptr) {
            if (useToggled) {
                connect(action, &QAction::toggled, m_appWin, slotBoolPtr);
            } else {
                connect(action, &QAction::triggered, m_appWin, slotBoolPtr);
            }
    }
    return action;
}

QAction * LC_ActionFactoryBase::createAction_AH(const char* name, RS2::ActionType actionType, const QString &text,
                                            const char *iconName, const char *themeIconName,
                                            QActionGroup *parent,
                                            QMap<QString, QAction *> &a_map) const{
    QAction *action = justCreateAction(a_map, name, text, iconName, themeIconName, parent);
    // LC_ERR <<  " ** original action handler" << this->action_handler;
    // well, a bit crazy hacky code to let the lambda properly capture action handler... without local var, class member is not captured
    QG_ActionHandler* capturedHandler = m_actionHandler;
    connect(action, &QAction::triggered, capturedHandler, [ capturedHandler, actionType](bool){ // fixme - sand - simplify by using data() on QAction and sender()
        // LC_ERR << " ++ captured action handler "<<   capturedHandler;
        capturedHandler->setCurrentAction(actionType);
    });
    LC_ActionGroupManager::associateQActionWithActionType(action, actionType);
    return action;
}

QAction *LC_ActionFactoryBase::justCreateAction(QMap<QString, QAction *> &a_map, const char* name, const QString& text, const char *iconName, const char *themeIconName,
                                                QActionGroup *parent) const {
    auto* action = new QAction(text, parent);
    if (iconName != nullptr) {
        QIcon icon = QIcon(iconName);
        if (m_usingTheme && themeIconName != nullptr)
            action->setIcon(QIcon::fromTheme(themeIconName, icon));
        else
            action->setIcon(icon);
    }
    action->setObjectName(name);
    action->setIconVisibleInMenu(true);
    action->setActionGroup(parent);
    a_map.insert(name, action);
    return action;
}

void LC_ActionFactoryBase::createActions(QMap<QString, QAction *> &map, QActionGroup *group, const std::vector<ActionInfo> &actionList) const {
    for (const LC_ActionFactoryBase::ActionInfo &a: actionList){
        justCreateAction(map, a.key, a.text, a.iconName,a.themeIconName, group);
    }
}

void LC_ActionFactoryBase::createActionHandlerActions(QMap<QString, QAction *> &map, QActionGroup *group, const std::vector<ActionInfo> &actionList) const {
    for (const LC_ActionFactoryBase::ActionInfo &a: actionList){
        createAction_AH(a.key, a.actionType, a.text, a.iconName,a.themeIconName, group, map);
    }
}

void LC_ActionFactoryBase::createMainWindowActions(QMap<QString, QAction *> &map, QActionGroup *group, const std::vector<ActionInfo> &actionList, bool useToggled) const {
    for (const LC_ActionFactoryBase::ActionInfo &a: actionList){
        createAction_MW(a.key, a.slotPtr, a.slotPtrBool, a.text, a.iconName, a.themeIconName, group, map, useToggled);
    }
}

void LC_ActionFactoryBase::makeActionsShortcutsNonEditable(const QMap<QString, QAction *> &map,
                                                           const std::vector<const char *> &actionNames) {
    for (auto name: actionNames){
        if (map.contains(name)) {
            QAction* action = map[name];
            if (action != nullptr){
                action ->setProperty(LC_ShortcutInfo::PROPERTY_ACTION_SHORTCUT_CONFIGURABLE, false);
            }
        }
    }
}

void LC_ActionFactoryBase::addActionsToMainWindow(const QMap<QString, QAction *> &map) const{
   // add actions to the main window to ensure that shortcuts for them will be invoked - even if the action is not visible.
   // without this, pressing shortcut for the action that is not visible does not activate the action
   for (const auto &a: map) {
       m_appWin->addAction(a);
   }
}

void LC_ActionFactoryBase::createActionGroups(const std::vector<ActionGroupInfo>& actionGroups,LC_ActionGroupManager* actionGroupManager) const {
    for (const ActionGroupInfo& groupInfo : actionGroups) {
        auto group = new LC_ActionGroup(actionGroupManager, groupInfo.name, groupInfo.title, groupInfo.description, groupInfo.iconName);
        actionGroupManager->addActionGroup(groupInfo.name, group, groupInfo.isToolGroup);
    }
}

void  LC_ActionFactoryBase::fillActionsList(QList<QAction *> &list, const std::vector<const char *> &actionNames, const QMap<QString, QAction*> &map) const {
    for (const char* actionName: actionNames){
        if (map.contains(actionName)) {
            auto action = map.value(actionName);
            list << action;
        }
    }
}
