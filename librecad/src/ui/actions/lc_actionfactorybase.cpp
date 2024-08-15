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

const char* LC_ActionFactoryBase::PROPERTY_SHORTCUT_BACKUP = "tooltip.original";

LC_ActionFactoryBase::LC_ActionFactoryBase(QC_ApplicationWindow* parent, QG_ActionHandler* a_handler):
    QObject(parent), main_window(parent),action_handler(a_handler){
}

QAction *LC_ActionFactoryBase::createAction_MW(const char *name, const char *slot, const char *text,
                                           const char *iconName, const char *themeIconName,
                                           QActionGroup *parent, QMap<QString, QAction *> &a_map, bool useToggled) const {
    QAction *action = doCreateActionTR(a_map, name, text, iconName, themeIconName, parent);
    if (slot != nullptr) {
        if (useToggled) {
            connect(action, SIGNAL(toggled(bool)), main_window, slot);
        } else {
            connect(action, SIGNAL(triggered(bool)), main_window, slot);
        }
    }
    return action;
}

QAction * LC_ActionFactoryBase::createAction_AH(const char* name, RS2::ActionType actionType, const char* text,
                                            const char *iconName, const char *themeIconName,
                                            QActionGroup *parent,
                                            QMap<QString, QAction *> &a_map) const{
    QAction *action = doCreateActionTR(a_map, name, text, iconName, themeIconName, parent);
    // LC_ERR <<  " ** original action handler" << this->action_handler;
    // well, a bit crazy hacky code to let the lambda properly capture action handler... without local var, class member is not captured
    QG_ActionHandler* capturedHandler = action_handler;
    QObject::connect(action, &QAction::triggered, capturedHandler, [ capturedHandler, actionType](bool){
        // LC_ERR << " ++ captured action handler "<<   capturedHandler;
        capturedHandler->setCurrentAction(actionType);
    });
    return action;
}

QAction *LC_ActionFactoryBase::doCreateActionTR(QMap<QString, QAction *> &a_map, const char* name,
                                                const char* text, const char *iconName, const char *themeIconName,
                                                QActionGroup *parent, const char* textDisambiguation) const {
    auto* action = new QAction( tr(text, textDisambiguation), parent);
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

void LC_ActionFactoryBase::createActionHandlerActions(QMap<QString, QAction *> &map, QActionGroup *group, const std::vector<ActionInfo> &actionList) const {
    for (const LC_ActionFactoryBase::ActionInfo a: actionList){
        createAction_AH(a.key, a.actionType, a.text, a.iconName,a.themeIconName, group, map);
    }
}

void LC_ActionFactoryBase::createMainWindowActions(QMap<QString, QAction *> &map, QActionGroup *group, const std::vector<ActionInfo> &actionList, bool useToggled) const {
    for (const LC_ActionFactoryBase::ActionInfo a: actionList){
        createAction_MW(a.key, a.slot, a.text, a.iconName, a.themeIconName, group, map, useToggled);
    }
}

void LC_ActionFactoryBase::assignShortcutsToActions(const QMap<QString, QAction *> &map,
                                                    std::vector<LC_ShortcutInfo> &shortcutsList) const {
    for (const LC_ShortcutInfo &a: shortcutsList){
        QAction* createdAction = map[a.getName()];
        if (createdAction != nullptr){
            const QList<QKeySequence> &list = a.getKeysList();
            if (list.isEmpty()) {
                const QKeySequence &sequence = a.getKey();
                if (sequence != QKeySequence::UnknownKey) {
                    createdAction->setShortcut(sequence);
                }
            } else { // todo - support for future
                createdAction->setShortcuts(list);
            }
        }
    }
}

void LC_ActionFactoryBase::updateActionShortcutTooltips(const QMap<QString, QAction *> &map, bool enable) {
    if (enable){
        for (QAction* action: map){
            if (!action->shortcut().isEmpty()) {
                QString tooltip = action->property(PROPERTY_SHORTCUT_BACKUP).toString();
                if (tooltip.isEmpty()) {
                    tooltip = action->toolTip();
                    if (tooltip != strippedActionText(action->text())) {
                        action->setProperty(PROPERTY_SHORTCUT_BACKUP, action->toolTip());  // action uses a custom tooltip. Backup so that we can restore it later.
                    }
                }
                QColor shortcutTextColor = QApplication::palette().color(QPalette::ToolTipText);
                QString shortCutTextColorName;
                if (shortcutTextColor.value() == 0) {
                    shortCutTextColorName = "gray";  // special handling for black because lighter() does not work there [QTBUG-9343].
                } else {
                    int factor = (shortcutTextColor.value() < 128) ? 150 : 50;
                    shortCutTextColorName = shortcutTextColor.lighter(factor).name();
                }
                action->setToolTip(QString("<p style='white-space:pre'>%1&nbsp;&nbsp;<code style='color:%2; font-size:small'>%3</code></p>")
                                       .arg(tooltip, shortCutTextColorName, action->shortcut().toString(QKeySequence::NativeText)));
            }
        }
    }
    else{
        for (QAction* action: map){
            action->setToolTip(action->property(PROPERTY_SHORTCUT_BACKUP).toString());
            action->setProperty(PROPERTY_SHORTCUT_BACKUP, QVariant()); // todo - is it really necessary to remove it?
        }
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

/* guesses a descriptive text from a text suited for a menu entry
   This is equivalent to QActions internal qt_strippedText()
*/
QString LC_ActionFactoryBase::strippedActionText(QString s) {
    s.remove( QString::fromLatin1("...") );
    for (int i = 0; i < s.size(); ++i) {
        if (s.at(i) == QLatin1Char('&'))
            s.remove(i, 1);
    }
    return s.trimmed();
}
