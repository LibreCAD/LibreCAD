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
#include "qg_actionhandler.h"
#include "lc_shortcutinfo.h"

LC_ActionFactoryBase::LC_ActionFactoryBase(QC_ApplicationWindow* parent, QG_ActionHandler* a_handler):
    QObject(parent), main_window(parent),action_handler(a_handler){
}

QAction *LC_ActionFactoryBase::createAction_MW(const char *name, const char *slot, const QString& text,
                                           const char *iconName, const char *themeIconName,
                                           QActionGroup *parent, QMap<QString, QAction *> &a_map, bool useToggled) const {
    QAction *action = justCreateAction(a_map, name, text, iconName, themeIconName, parent);
    if (slot != nullptr) {
        if (useToggled) {
            connect(action, SIGNAL(toggled(bool)), main_window, slot);
        } else {
            connect(action, SIGNAL(triggered(bool)), main_window, slot);
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
    QG_ActionHandler* capturedHandler = action_handler;
    connect(action, &QAction::triggered, capturedHandler, [ capturedHandler, actionType](bool){
        // LC_ERR << " ++ captured action handler "<<   capturedHandler;
        capturedHandler->setCurrentAction(actionType);
    });
    return action;
}

QAction *LC_ActionFactoryBase::justCreateAction(QMap<QString, QAction *> &a_map, const char* name, const QString& text, const char *iconName, const char *themeIconName,
                                                QActionGroup *parent) const {
    auto* action = new QAction(text, parent);
    if (iconName != nullptr) {
        QIcon icon = QIcon(iconName);
        if (using_theme && themeIconName != nullptr)
            action->setIcon(QIcon::fromTheme(themeIconName, icon));
        else
            action->setIcon(icon);
    }
    action->setObjectName(name);
    a_map[name] = action;
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
        createAction_MW(a.key, a.slot, a.text, a.iconName, a.themeIconName, group, map, useToggled);
    }
}

void LC_ActionFactoryBase::makeActionsShortcutsNonEditable(const QMap<QString, QAction *> &map,
                                                          const std::vector<const char *> &actionNames) {
    for (auto name: actionNames){
        QAction* action = map[name];
        if (action != nullptr){
          action ->setProperty(LC_ShortcutInfo::PROPERTY_ACTION_SHORTCUT_CONFIGURABLE, false);
        }
    }
}
