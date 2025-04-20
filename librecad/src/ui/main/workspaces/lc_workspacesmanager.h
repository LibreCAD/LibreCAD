/*******************************************************************************
 *
 This file is part of the LibreCAD project, a 2D CAD program

 Copyright (C) 2025 LibreCAD.org
 Copyright (C) 2025 sand1024

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

#ifndef LC_WORKSPACESMANAGER_H
#define LC_WORKSPACESMANAGER_H

#include <QObject>
#include <QList>

class QC_ApplicationWindow;

class LC_WorkspacesManager : public QObject{
    Q_OBJECT
public:
    LC_WorkspacesManager();
    ~LC_WorkspacesManager() override;
    void getWorkspaces(QList<QPair<int, QString>> &workspacesList);
    void getWorkspaceNames(QStringList &workspacesList);
    void saveWorkspace(QString name, QWidget* parent = nullptr);
    void deleteWorkspace(int id);
    void activateWorkspace(int id);
    void init(QC_ApplicationWindow* win);
    void persist();
    bool isWorkspacesFileExists();
    bool hasWorkspaces();
protected:
    struct LC_Workspace {
        int id;
        QString name;
        QString geometry;
        int windowWidth;
        int windowHeight;
        int windowX;
        int windowY;
        QString widgetsState;

        bool dockAreaLeftActive = false;
        bool dockAreaRightActive = false;
        bool dockAreaToptActive = false;
        bool dockAreaBottomActive = false;
        bool docAreaFloatingActive = false;

        int iconsSizeToolbar = 24;
        int iconsSizeLeftDock = 24;
        int iconsSizeRightDoc = 16;
        int columnCountLeftDoc = 6;

        bool extendMenu = false;
        bool extendMenuTillEntities = false;

        bool showStatusBar = false;
    };

    int m_workspaceID = 0;
    int m_lastActivatedId = -1;

    QList<LC_Workspace*> m_workspacesList;
    QString getWorkspacesFileName();
    void createWorkspacesFileBackupCopy(const QString& workspacesFile);
    void restoreGeometryAndState(const LC_Workspace &workspace) const;
    void restore(const LC_Workspace& perspective);
    void applyToSettings(const LC_Workspace &workspace);
    void fillBySettings(LC_Workspace &workspace);
    void fillByState(LC_Workspace &workspace);
    void loadWorkspaces();
    void saveWorkspaces(QWidget* parent = nullptr);
    void restoreGeometryAndState(const LC_Workspace &workspace, QC_ApplicationWindow &appWin) const;
    void fillIconsAndMenuState(LC_WorkspacesManager::LC_Workspace &workspace);
};

#endif // LC_PERSPECTIVESMANAGER_H
