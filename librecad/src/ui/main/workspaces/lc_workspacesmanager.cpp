/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2025 LibreCAD.org
 * Copyright (C) 2025 sand1024
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

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>

#include "lc_workspacesmanager.h"

#include <QFile>

#include "qc_applicationwindow.h"
#include "rs_debug.h"
#include "rs_settings.h"
#include "rs_system.h"
LC_WorkspacesManager::LC_WorkspacesManager() = default;

LC_WorkspacesManager::~LC_WorkspacesManager() {
    qDeleteAll(m_workspacesList);
}

void LC_WorkspacesManager::getWorkspaces(QList<QPair<int, QString>> &workspacesList){
    for(auto p: m_workspacesList){
        workspacesList << QPair<int, QString>(p->id, p->name);
    }
}

void LC_WorkspacesManager::getWorkspaceNames(QStringList &workspacesList){
    for(auto p: m_workspacesList){
        workspacesList << p->name;
    }
}

void LC_WorkspacesManager::saveWorkspace(QString name,QWidget*  parent){
    name = name.trimmed();
    QList<LC_Workspace *>::iterator it = m_workspacesList.begin();
    while (it != m_workspacesList.end()) {
        if ((*it)->name == name) {
            // just update existing perspective
            fillByState(**it);
            m_lastActivatedId = (*it)->id;
            saveWorkspaces(parent);
            return;
        }
        ++it;
    }
    // nothing found by name, create new one
    auto* workspace = new LC_Workspace;
    workspace->name = name;
    workspace->id = m_workspaceID++;
    m_lastActivatedId = workspace->id;
    fillByState(*workspace);
    m_workspacesList << workspace;
    saveWorkspaces(parent);
}

void LC_WorkspacesManager::deleteWorkspace(int id){
    LC_Workspace perspective;
    QList<LC_Workspace *>::iterator it = m_workspacesList.begin();
    while (it != m_workspacesList.end()) {
        if ((*it)->id == id) {
            LC_Workspace* w = *it;
            m_workspacesList.erase(it);
            delete w;
            saveWorkspaces();
            return;
        }
        else {
            ++it;
        }
    }
    saveWorkspaces();
}

void LC_WorkspacesManager::activateWorkspace(int id){
    if (id < 0) {
       id = m_lastActivatedId;
    }
    QList<LC_Workspace *>::iterator it = m_workspacesList.begin();
    while (it != m_workspacesList.end()) {
         if ((*it)->id == id) {
            restore(**it);
            if (m_lastActivatedId != id) {
                m_lastActivatedId = id;
                persist();
            }
            return;
         }
        ++it;
    }
}

void LC_WorkspacesManager::fillIconsAndMenuState(LC_WorkspacesManager::LC_Workspace &workspace){
    LC_GROUP("Widgets");
    {
        workspace.columnCountLeftDoc = LC_GET_INT("LeftToolbarColumnsCount", 5);
        workspace.iconsSizeLeftDock = LC_GET_INT("LeftToolbarIconSize", 24);
        workspace.iconsSizeRightDoc = LC_GET_INT("DockWidgetsIconSize", 16);
        workspace.iconsSizeToolbar = LC_GET_INT("ToolbarIconSize", 25);
    }
    LC_GROUP_END();
    LC_GROUP("Startup");
    {
        workspace.extendMenu = LC_GET_BOOL("ExpandedToolsMenu", false);
        workspace.extendMenuTillEntities = LC_GET_BOOL("ExpandedToolsMenuTillEntity", false);
    }
    LC_GROUP_END();

    workspace.showStatusBar = LC_GET_ONE_BOOL("Appearance", "StatusBarVisible", false);
}

void LC_WorkspacesManager::fillBySettings(LC_Workspace &workspace){
    LC_GROUP("Geometry");
    {
        auto geometryB64 = LC_GET_STR("WindowGeometry");

        workspace.geometry = geometryB64;
        workspace.windowWidth = LC_GET_INT("WindowWidth", 1024);
        workspace.windowHeight = LC_GET_INT("WindowHeight", 1024);
        workspace.windowX = LC_GET_INT("WindowX", 32);
        workspace.windowY = LC_GET_INT("WindowY", 32);

        workspace.widgetsState = LC_GET_STR("StateOfWidgets", "");

        workspace.dockAreaLeftActive = LC_GET_BOOL("LeftDockArea", false);
        workspace.dockAreaRightActive = LC_GET_BOOL("RightDockArea", true);
        workspace.dockAreaToptActive = LC_GET_BOOL("TopDockArea", false);
        workspace.dockAreaBottomActive = LC_GET_BOOL("BottomDockArea", false);
        workspace.docAreaFloatingActive = LC_GET_BOOL("FloatingDockwidgets", false);
    }
    LC_GROUP_END();

    workspace.showStatusBar = LC_GET_ONE_BOOL("Appearance", "StatusBarVisible", false);

    fillIconsAndMenuState(workspace);
}


void LC_WorkspacesManager::fillByState(LC_Workspace &workspace){
    QC_ApplicationWindow& appWin = *QC_ApplicationWindow::getAppWindow();
    QString geometryB64 = appWin.saveGeometry().toBase64(QByteArray::Base64Encoding);
    QString stateB64 = appWin.saveState().toBase64(QByteArray::Base64Encoding);
    workspace.geometry = geometryB64;
    workspace.widgetsState = stateB64;
    workspace.windowHeight = appWin.height();
    workspace.windowWidth = appWin.width();
    workspace.windowX = appWin.x();
    workspace.windowY = appWin.y();

    workspace.dockAreaLeftActive = appWin.getDockAreas().left->isChecked();
    workspace.dockAreaRightActive = appWin.getDockAreas().right->isChecked();
    workspace.dockAreaBottomActive = appWin.getDockAreas().bottom->isChecked();
    workspace.dockAreaToptActive = appWin.getDockAreas().top->isChecked();
    workspace.docAreaFloatingActive = appWin.getDockAreas().floating->isChecked();

    fillIconsAndMenuState(workspace);
}

void LC_WorkspacesManager::applyToSettings(const LC_Workspace &workspace){
    LC_GROUP("Geometry");
    {
        LC_SET("WindowGeometry",workspace.geometry);
        LC_SET("WindowWidth", workspace.windowWidth);
        LC_SET("WindowHeight", workspace.windowHeight);
        LC_SET("WindowY", workspace.windowY);
        LC_SET("WindowX", workspace.windowX);
        LC_SET("StateOfWidgets",workspace.widgetsState);
        LC_SET("LeftDockArea", workspace.dockAreaLeftActive);
        LC_SET("RightDockArea", workspace.dockAreaRightActive);
        LC_SET("TopDockArea", workspace.dockAreaToptActive);
        LC_SET("BottomDockArea", workspace.dockAreaBottomActive);
        LC_SET("FloatingDockwidgets", workspace.docAreaFloatingActive);
    }
    LC_GROUP_END();
    LC_GROUP("Widgets");
    {
        LC_SET("LeftToolbarColumnsCount", workspace.columnCountLeftDoc);
        LC_SET("LeftToolbarIconSize", workspace.iconsSizeLeftDock);
        LC_SET("DockWidgetsIconSize", workspace.iconsSizeRightDoc);
        LC_SET("ToolbarIconSize", workspace.iconsSizeToolbar);
    }
    LC_GROUP_END();
    LC_GROUP("Startup");
    {
        LC_SET("ExpandedToolsMenu", workspace.extendMenu);
        LC_SET("ExpandedToolsMenuTillEntity", workspace.extendMenuTillEntities);
    }
    LC_GROUP_END();

    LC_GROUP("Appearance");
    {
        LC_SET("StatusBarVisible",workspace.showStatusBar);
    }
}

void LC_WorkspacesManager::restoreGeometryAndState(const LC_Workspace &workspace) const {
    QC_ApplicationWindow& appWin = *QC_ApplicationWindow::getAppWindow();
    restoreGeometryAndState(workspace, appWin);
}

void LC_WorkspacesManager::restoreGeometryAndState(const LC_WorkspacesManager::LC_Workspace &workspace, QC_ApplicationWindow &appWin) const {
    appWin.setUpdatesEnabled(false);

    auto widgetsState = QByteArray::fromBase64(workspace.widgetsState.toUtf8(), QByteArray::Base64Encoding);
    appWin.restoreState(widgetsState);

    appWin.getDockAreas().left->setChecked(workspace.dockAreaLeftActive);
    appWin.getDockAreas().right->setChecked(workspace.dockAreaRightActive);
    appWin.getDockAreas().bottom->setChecked(workspace.dockAreaBottomActive);
    appWin.getDockAreas().top->setChecked(workspace.dockAreaToptActive);
    appWin.getDockAreas().floating->setChecked(workspace.docAreaFloatingActive);

    appWin.rebuildMenuIfNecessary();
    appWin.setIconSize(QSize(workspace.iconsSizeToolbar, workspace.iconsSizeToolbar));

    auto geometry = QByteArray::fromBase64(workspace.geometry.toUtf8(), QByteArray::Base64Encoding);
    if (!geometry.isEmpty()) {
        appWin.restoreGeometry(geometry);
    } else {
        // fallback
        int windowWidth = workspace.windowWidth;
        int windowHeight = workspace.windowHeight;
        int windowX = workspace.windowX;
        int windowY = workspace.windowY;
        appWin.resize(windowWidth, windowHeight);
        appWin.move(windowX, windowY);
    }

    appWin.slotViewStatusBar(workspace.showStatusBar);
    appWin.setUpdatesEnabled(true);
    appWin.fireWidgetSettingsChanged();
}

void LC_WorkspacesManager::restore(const LC_Workspace& perspective){
    applyToSettings(perspective);
    restoreGeometryAndState(perspective);
}

void LC_WorkspacesManager::init(QC_ApplicationWindow* win){
    LC_Workspace workspace;
    fillBySettings(workspace);
    restoreGeometryAndState(workspace, *win);
    loadWorkspaces();
}

void LC_WorkspacesManager::persist(){
    LC_Workspace workspace;
    fillByState(workspace);
    applyToSettings(workspace);
    saveWorkspaces();
}

QString LC_WorkspacesManager::getWorkspacesFileName(){
    QString settingsDir = LC_GET_ONE_STR("Paths","OtherSettingsDir", RS_System::instance()->getAppDataDir()).trimmed();
    QString workspacesFile = settingsDir + "/workspaces.lcws";
    return workspacesFile;
}

void LC_WorkspacesManager::createWorkspacesFileBackupCopy(const QString& workspacesFile) {
    // do backup of file on start
    QString backupFileName = workspacesFile + ".bak";
    QFile::copy(workspacesFile, backupFileName);
}

void LC_WorkspacesManager::loadWorkspaces(){
    QString workspacesFile = getWorkspacesFileName();
    if (!workspacesFile.isEmpty()) {
        QFile jsonFile(workspacesFile);
        if (jsonFile.exists()) {
            if (jsonFile.open(QFile::ReadOnly)) {
                createWorkspacesFileBackupCopy(workspacesFile);
                QJsonParseError parseError;
                auto doc = QJsonDocument::fromJson(jsonFile.readAll(), &parseError);
                if (parseError.error != QJsonParseError::NoError) {
                    LC_ERR << "Unexpected error during workspaces parsing. Message:" + parseError.errorString();
                }
                else {
                    bool canParse;
                    QJsonObject obj;
                    if (doc.isObject()) {
                        obj = doc.object();
                        auto type = obj.value("type").toString();
                        canParse = "LibreCAD Workspaces file" == type;
                    } else {
                        canParse = false;
                        LC_ERR << "Invalid content of workspaces file. File: " + workspacesFile;
                    }
                    if (canParse) {
                        m_workspaceID = obj.value("maxId").toInt(0);
                        m_lastActivatedId = obj.value("lastActivatedId").toInt(0);
                        QJsonArray jsonArray = obj.value("workspaces").toArray();
                        m_workspacesList.clear();
                            for(const auto& value: jsonArray) {
                                QJsonObject wsObj = value.toObject();

                                auto* p = new LC_Workspace;
                                p->name = wsObj["name"].toString();
                                p->id = wsObj["id"].toInt();

                                p->geometry = wsObj["geometry"].toString();
                                p->widgetsState = wsObj["widgetState"].toString();
                                p->windowX = wsObj["winX"].toInt();
                                p->windowY = wsObj["winY"].toInt();
                                p->windowWidth = wsObj["winHeight"].toInt();
                                p->windowHeight = wsObj["winWidth"].toInt();

                                p->dockAreaLeftActive = wsObj["dockLeft"].toBool();
                                p->dockAreaRightActive = wsObj["dockRight"].toBool();
                                p->dockAreaToptActive = wsObj["dockTop"].toBool();
                                p->dockAreaBottomActive = wsObj["dockBottom"].toBool();
                                p->docAreaFloatingActive = wsObj["dockFloat"].toBool();

                                p->columnCountLeftDoc = wsObj["columnCountLeftDock"].toInt(6);
                                p->iconsSizeToolbar = wsObj["iconSizeToolbar"].toInt(24);
                                p->iconsSizeLeftDock = wsObj["iconSizeLeftDock"].toInt(24);
                                p->iconsSizeRightDoc = wsObj["iconSizeRightDock"].toInt(16);

                                p->extendMenu  = wsObj["expandMenu"].toBool(false);
                                p->extendMenuTillEntities = wsObj["expandMenuTillEntity"].toBool(false);

                                p->showStatusBar  = wsObj["statusBarVisible"].toBool(false);

                                m_workspacesList << p;
                            }
                    }
                }
            }
            else {
                LC_ERR << "Can't read workspaces file. File: " + workspacesFile;
            }
        }
        else {
            LC_ERR << "Workspaces file does not exists. File: " + workspacesFile;
        }
    }
}

bool LC_WorkspacesManager::isWorkspacesFileExists(){
    QString workspacesFile = getWorkspacesFileName();
    if (!workspacesFile.isEmpty()) {
        QFile file(workspacesFile);
        return file.exists();
    }
    return false;
}

bool LC_WorkspacesManager::hasWorkspaces(){
    return !m_workspacesList.isEmpty();
}

void LC_WorkspacesManager::saveWorkspaces(QWidget* parent){
    QString workspacesFile = getWorkspacesFileName();
    if (!workspacesFile.isEmpty()) {
        QFile file(workspacesFile);
        if (file.open(QFile::WriteOnly)) {
            QJsonObject objSettings;
            objSettings.insert("type", QJsonValue::fromVariant("LibreCAD Workspaces file"));
            objSettings.insert("maxId", QJsonValue::fromVariant(m_workspaceID));
            objSettings.insert("lastActivatedId", QJsonValue::fromVariant(m_lastActivatedId));

            QJsonArray perspectivesArray;

            for (auto p: m_workspacesList) {
                QJsonObject wsObj;

                wsObj.insert("name", QJsonValue::fromVariant(p->name));
                wsObj.insert("id", QJsonValue::fromVariant(p->id));
                wsObj.insert("geometry", QJsonValue::fromVariant(p->geometry));
                wsObj.insert("widgetState", QJsonValue::fromVariant(p->widgetsState));
                wsObj.insert("winX", QJsonValue::fromVariant(p->windowX));
                wsObj.insert("winY", QJsonValue::fromVariant(p->windowY));
                wsObj.insert("winHeight", QJsonValue::fromVariant(p->windowHeight));
                wsObj.insert("winWidth", QJsonValue::fromVariant(p->windowWidth));

                wsObj.insert("dockLeft", QJsonValue::fromVariant(p->dockAreaLeftActive));
                wsObj.insert("dockRight", QJsonValue::fromVariant(p->dockAreaRightActive));
                wsObj.insert("dockTop", QJsonValue::fromVariant(p->dockAreaToptActive));
                wsObj.insert("dockBottom", QJsonValue::fromVariant(p->dockAreaBottomActive));
                wsObj.insert("dockFloat", QJsonValue::fromVariant(p->dockAreaBottomActive));

                wsObj.insert("columnCountLeftDock", QJsonValue::fromVariant(p->columnCountLeftDoc));
                wsObj.insert("iconSizeToolbar", QJsonValue::fromVariant(p->iconsSizeToolbar));
                wsObj.insert("iconSizeLeftDock", QJsonValue::fromVariant(p->iconsSizeLeftDock));
                wsObj.insert("iconSizeRightDock", QJsonValue::fromVariant(p->iconsSizeRightDoc));

                wsObj.insert("expandMenu", QJsonValue::fromVariant(p->extendMenu));
                wsObj.insert("expandMenuTillEntity", QJsonValue::fromVariant(p->extendMenuTillEntities));
                wsObj.insert("statusBarVisible", QJsonValue::fromVariant(p->showStatusBar));

                perspectivesArray.append(wsObj);
            }

            objSettings.insert("workspaces", perspectivesArray);

            QJsonDocument doc(objSettings);
            file.write(doc.toJson());
        } else {
            if (parent != nullptr) {
                QMessageBox::critical(parent, tr("Saving Workspaces"),
                                      tr("Can't open workspaces file for writing. Workspaces were not exported. File: ") + workspacesFile);
            } else {
                LC_ERR << "Can't open provided file for writing - check that provided location is writable. Workspaces were not saved. File: " + workspacesFile;
            }
        }
    } else {
        if (parent != nullptr) {
            QMessageBox::critical(parent, tr("Saving Workspaces"),
                                  tr("Workspaces file does not exists."));
        } else {
            LC_ERR << "Workspaces file does not exists. File: " + workspacesFile;
        }
    }
}
