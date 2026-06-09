/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2026 LibreCAD.org
 * Copyright (C) 2026 sand1024
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 * ********************************************************************************
 */

#ifndef LC_ACTIONGROUPMANAGER_H
#define LC_ACTIONGROUPMANAGER_H

#include <QMap>
#include <QObject>

#include "rs.h"

class LC_ShortcutsManager;
class LC_ShortcutInfo;
class LC_ActionGroup;
class QAction;
class QC_ApplicationWindow;

class LC_ActionGroupManager : public QObject{
    Q_OBJECT
public:
    explicit LC_ActionGroupManager(QC_ApplicationWindow *parent);
    ~LC_ActionGroupManager() override;


    QList<LC_ActionGroup*> toolGroups() const;
    QMap<QString, LC_ActionGroup*> allGroups();
    QList<LC_ActionGroup *> allGroupsList();
    void sortGroupsByName(QList<LC_ActionGroup*>& list);
    void assignShortcutsToActions(const QMap<QString, QAction *> &map, const std::vector<LC_ShortcutInfo> &shortcutsList) const;
    int loadShortcuts(const QMap<QString, QAction *> &map);
    int loadShortcuts(const QString &fileName, QMap<QString, QKeySequence> *result) const;
    int saveShortcuts(QMap<QString, LC_ShortcutInfo *> shortcutsMap);
    int saveShortcuts(const QList<LC_ShortcutInfo *> &shortcutsList, const QString &fileName) const;
    QString getShortcutsMappingsFolder() const;
    QMap<QString, QAction *> &getActionsMap();
    QAction *getActionByName(const QString &name) const;
    bool hasActionGroup(const QString& categoryName) const;
    LC_ActionGroup* getActionGroup(const QString& groupName) const;
    bool isActionTypeSetsTheIcon(RS2::ActionType actionType);
    void completeInit();
    QAction* getActionByType(RS2::ActionType actionType) const;
    static void associateQActionWithActionType(QAction* action, RS2::ActionType actionType);
    void persist();
    LC_ActionGroup* getGroupByName(const QString &name) const;
    void addActionGroup(const QString &name, LC_ActionGroup *actionGroup, bool isToolsGroup);
public slots:
    void toggleExclusiveSnapMode(bool state); // fixme - sand - refactor later!!! Should be out of generic AGM?
    void toggleTools(bool state) const;
    void onOptionsChanged() const;
private:
    QList<LC_ActionGroup *> m_toolsGroups;
    QMap<QString, LC_ActionGroup*> m_actionGroups;
    QMap<QString, QAction*> m_actionsMap; // should be initialized by action factory by call of loadShortcuts()
    QMap<int, QAction*> m_actionsByTypes;
    std::unique_ptr<LC_ShortcutsManager> m_shortcutsManager;
    QList<bool> m_snapState;
};

#endif
