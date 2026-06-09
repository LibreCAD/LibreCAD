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

#include "lc_workspacesinvoker.h"

#include "lc_inputtextdialog.h"
#include "lc_workspacesmanager.h"
#include "qc_applicationwindow.h"

LC_WorkspacesInvoker::LC_WorkspacesInvoker(QC_ApplicationWindow* mainWin):
    LC_AppWindowAware{mainWin}, m_workspacesManager{std::make_unique<LC_WorkspacesManager>()} {
}

LC_WorkspacesInvoker::~LC_WorkspacesInvoker() = default;

void LC_WorkspacesInvoker::init() const {
    m_workspacesManager->init(m_appWin);
}

void LC_WorkspacesInvoker::persist() const {
    m_workspacesManager->persist();
}

bool LC_WorkspacesInvoker::hasWorkspaces() const {
    return m_workspacesManager->hasWorkspaces();
}

void LC_WorkspacesInvoker::saveWorkspace([[maybe_unused]]bool on) const {
    QStringList options;
    m_workspacesManager->getWorkspaceNames(options);
    bool ok;
    const auto name = LC_InputTextDialog::getText(m_appWin, tr("New Workspace"), tr("Name of workspace to save:"), options, true, "", &ok);
    if (ok) {
        m_workspacesManager->saveWorkspace(name, m_appWin);
        m_appWin->fireWorkspacesChanged();
    }
}

void  LC_WorkspacesInvoker::fillWorkspacesList(QList<QPair<int, QString>> &list) const {
    m_workspacesManager->getWorkspaces(list);
}

void LC_WorkspacesInvoker::applyWorkspaceById(const int id) const {
    m_workspacesManager->activateWorkspace(id);
}

void LC_WorkspacesInvoker::removeWorkspace([[maybe_unused]]bool on) const {
    QList<QPair<int, QString>> options;
    m_workspacesManager->getWorkspaces(options);
    bool ok = false;
    const int workspaceId = LC_InputTextDialog::selectId(m_appWin, tr("Remove Workspace"), tr("Select workspace to remove:"), options, &ok);
    if (ok) {
        m_workspacesManager->deleteWorkspace(workspaceId);
        m_appWin->fireWorkspacesChanged();
    }
}

void LC_WorkspacesInvoker::restoreWorkspace([[maybe_unused]]bool on) const {
    const auto invocationSender = sender();
    const auto *action = qobject_cast<QAction*>(invocationSender);
    if (action != nullptr) {
        const QVariant variant = action->property("_WSPS_IDX");
        if (variant.isValid()){
            const int id = variant.toInt();
            applyWorkspaceById(id);
        }
        else {
            m_workspacesManager->activateWorkspace(-1);
        }
    }
}
