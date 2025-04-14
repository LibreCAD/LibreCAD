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
#ifndef LC_WORKSPACESHELPER_H
#define LC_WORKSPACESHELPER_H

#include<QObject>
#include "lc_appwindowaware.h"

class LC_WorkspacesManager;

class LC_WorkspacesInvoker: public QObject, public LC_AppWindowAware{
     Q_OBJECT
public:
    LC_WorkspacesInvoker(QC_ApplicationWindow* mainWin);
     ~LC_WorkspacesInvoker() override;
     void saveWorkspace(bool on);
     void fillWorkspacesList(QList<QPair<int, QString>>& list);
     void applyWorkspaceById(int id);
     void removeWorkspace(bool on);
     void restoreWorkspace(bool on);
     void init();
     void persist();
     bool hasWorkspaces();
 private:
    std::unique_ptr<LC_WorkspacesManager> m_workspacesManager;
};

#endif // LC_WORKSPACESHELPER_H
