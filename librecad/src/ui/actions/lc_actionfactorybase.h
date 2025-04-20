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

#ifndef LC_ACTIONFACTORYBASE_H
#define LC_ACTIONFACTORYBASE_H

#include <QObject>

#include "lc_appwindowaware.h"
#include "rs.h"

class LC_ActionGroupManager;
class QActionGroup;
class QAction;
class QG_ActionHandler;

class LC_ActionFactoryBase : public QObject, public LC_AppWindowAware{
    Q_OBJECT
public:
    LC_ActionFactoryBase(QC_ApplicationWindow* parent, QG_ActionHandler* a_handler);
protected:
    struct ActionInfo{
        const char* key;
        RS2::ActionType actionType;
        QString text;
        const char* iconName = nullptr;
        const char* themeIconName = nullptr;
        void (QC_ApplicationWindow::*slotPtr)() = nullptr;
        void (QC_ApplicationWindow::*slotPtrBool)(bool) = nullptr;

        ActionInfo(const char *key, const QString &text, const char *iconName):key(key), text(text), iconName(iconName) {}

        ActionInfo(const char *actionKey, RS2::ActionType actionType,const QString& text, const char *iconName = nullptr,
             const char *themeIcon = nullptr)
            :key(actionKey),actionType(actionType),text(text),iconName(iconName), themeIconName(themeIcon){}

        ActionInfo(const char *actionKey, void (QC_ApplicationWindow::*slotPtr)(), const QString &text,
            const char *iconName = nullptr, const char *themeIcon = nullptr)
            :key(actionKey),actionType(RS2::ActionNone),text(text),iconName(iconName), themeIconName(themeIcon), slotPtr(slotPtr){}

        ActionInfo(const char *actionKey, void (QC_ApplicationWindow::*slotPtrBool)(bool), const QString &text,
             const char *iconName = nullptr, const char *themeIcon = nullptr)
            :key(actionKey),actionType(RS2::ActionNone),text(text),iconName(iconName), themeIconName(themeIcon), slotPtrBool(slotPtrBool){}
    };

    struct ActionGroupInfo {
        ActionGroupInfo(const QString& name,  const QString& title, const QString& description,
            const char* iconName, bool toolGroup = true)
            : name{name},
              isToolGroup{toolGroup},
              title{title},
              description{description},
              iconName{iconName} {
        }

        QString name;
        bool isToolGroup{false};
        QString title;
        QString description;
        const char* iconName{""};
    };

    QG_ActionHandler* m_actionHandler = nullptr;
    bool m_usingTheme = false;

    QAction *createAction_AH(const char* name, RS2::ActionType actionType, const QString& text,
                             const char *iconName, const char *themeIconName,
                             QActionGroup *parent,
                             QMap<QString, QAction *> &a_map) const;

    void createActionHandlerActions(QMap<QString, QAction *> &map, QActionGroup *group, const std::vector<ActionInfo> &actionList) const;
    void createMainWindowActions( QMap<QString, QAction *> &map, QActionGroup *group, const std::vector<ActionInfo> &actionList, bool useToggled = false) const;

    QAction *createAction_MW(const char *name, void (QC_ApplicationWindow::*slotPtr)(), void (QC_ApplicationWindow::*slotBoolPtr)(bool),
        const QString& text, const char *iconName,  const char *themeIconName,  QActionGroup *parent,
        QMap<QString, QAction *> &a_map, bool toggled = false) const;

    void makeActionsShortcutsNonEditable(const QMap<QString, QAction *> &map, const std::vector<const char *> &actionNames);
    QAction *justCreateAction(QMap<QString, QAction *> &a_map, const char *name, const QString& text, const char *iconName,
                                 const char *themeIconName,QActionGroup *parent) const;
    void createActions(QMap<QString, QAction *> &map, QActionGroup *group, const std::vector<ActionInfo> &actionList) const;
    void addActionsToMainWindow(const QMap<QString, QAction *> &map) const;
    void createActionGroups(const std::vector<ActionGroupInfo>& actionGroups, LC_ActionGroupManager* actionGroupManager) const;
    void fillActionsList(QList<QAction*>& list, const std::vector<const char*>& actionNames, const QMap<QString, QAction*>& map) const;
};

#endif // LC_ACTIONFACTORYBASE_H
