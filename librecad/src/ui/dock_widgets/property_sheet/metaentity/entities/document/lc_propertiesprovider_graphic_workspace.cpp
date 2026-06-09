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

#include "lc_propertiesprovider_graphic_workspace.h"

#include "lc_propertyprovider_utils.h"
#include "qc_applicationwindow.h"
#include "rs_graphic.h"

const QString LC_PropertiesProviderGraphicWorkspace::SECTION_WORKSPACE = "_secWorkspace";

using namespace LC_PropertyProviderUtils;

void LC_PropertiesProviderGraphicWorkspace::fillDocumentProperties(LC_PropertyContainer* container) {
    if (m_widget->getOptions()->noSelectionWorkspace) {
        const auto cont = createSection(container, {SECTION_WORKSPACE, tr("Workspace"), tr("Generic UI settings")});
        createShowFullScreen(cont);
        createShowMainMenu(cont);
        createShowStatusBar(cont);
        createWorkspaceSelector(cont);
        if (isShowLinks()) {
            createWorkspaceCommands(cont);
        }
    }
}

void LC_PropertiesProviderGraphicWorkspace::createWorkspaceSelector(LC_PropertyContainer* container) const {
    int currentUCSIndex = -1;

    QList<QPair<int, QString>> workspacesList;
    QC_ApplicationWindow::getAppWindow()->fillWorkspacesList(workspacesList);

    if (!workspacesList.empty()) {
        const auto count = workspacesList.count();
        QVector<LC_EnumValueDescriptor> values;
        auto desc = LC_EnumValueDescriptor(-1, tr("<Select Workspace>"));
        values.append(desc);

        for (int i = 0; i < count; i++) {
            const auto [fst, snd] = workspacesList.at(i);
            desc = LC_EnumValueDescriptor(fst, snd);
            values.append(desc);
        }
        const auto enumDescriptor = new LC_EnumDescriptor("uiWorkspaceListEnum", values);

        auto funGet = [currentUCSIndex]([[maybe_unused]] const RS_Graphic* e)-> LC_PropertyEnumValueType {
            return currentUCSIndex;
        };

        auto funSet = [this](const LC_PropertyEnumValueType& v, [[maybe_unused]] const RS_Graphic* e) -> void {
            if (v > 0) {
                QC_ApplicationWindow::getAppWindow()->applyWorkspaceById(v);
            }
            m_widget->refill();
        };

        const LC_Property::Names names = {"viewSelection", tr("Workspace to use"), tr("Restores one of previously saved workspaces")};
        addDirectEnum<LC_PropertyEnumValueType, RS_Graphic>(container, names, enumDescriptor, funGet, funSet, nullptr, true);
    }
}

void LC_PropertiesProviderGraphicWorkspace::createUIActionProperty(LC_PropertyContainer* cont, const char* actionName, const LC_Property::Names& names) {
    auto action = QC_ApplicationWindow::getAppWindow()->getAction(actionName);
    auto funGet = [action]([[maybe_unused]] const RS_Graphic* e) -> bool {
        if (action != nullptr) {
            return action->isChecked();
        }
        return false;
    };
    auto funSet = [action]([[maybe_unused]] const bool& v, [[maybe_unused]] RS_Graphic* e) -> void {
        action->setChecked(v);
    };

    createDirectDelegatedBool<RS_Graphic>(cont, names, funGet, funSet, nullptr);
}

void LC_PropertiesProviderGraphicWorkspace::createShowFullScreen(LC_PropertyContainer* cont) {
    const LC_Property::Names names = {"uiFullScreen", tr("Fullscreen"), tr("Defines whether main window is shown in fullscreen window or not")};
    createUIActionProperty(cont, "Fullscreen", names);
}

void LC_PropertiesProviderGraphicWorkspace::createShowMainMenu(LC_PropertyContainer* cont) {
    const LC_Property::Names names = {"uiMainMenu", tr("Main menu"), tr("Defines whether main menu is shown or not")};
    createUIActionProperty(cont, "MainMenu", names);
}

void LC_PropertiesProviderGraphicWorkspace::createShowStatusBar(LC_PropertyContainer* cont) {
    const LC_Property::Names names = {"uiStatusBar", tr("Statusbar"), tr("Defines whether main menu is shown or not")};
    createUIActionProperty(cont, "ViewStatusBar", names);
}

void LC_PropertiesProviderGraphicWorkspace::createWorkspaceCommands(LC_PropertyContainer* cont) const {
    auto saveClickHandler = [this]([[maybe_unused]] RS_Graphic* g, [[maybe_unused]] const int linkIndex) {
        QC_ApplicationWindow::getAppWindow()->saveWorkspace(true);
        m_widget->refill();
    };

    createSingleEntityCommand<RS_Graphic>(cont, "workspaceSave", tr("Save workspace..."),
                                          tr("Save current workspace for later use"), "", "", nullptr, saveClickHandler,
                                          tr("Saving current workspace for later use"));
}
