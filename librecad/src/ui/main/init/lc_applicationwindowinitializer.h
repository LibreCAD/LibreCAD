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
#ifndef LC_APPLICATIONWINDOWINITIALIZER_H
#define LC_APPLICATIONWINDOWINITIALIZER_H

#include <QObject>

#include "lc_appwindowaware.h"
class QC_ApplicationWindow;

class LC_ApplicationWindowInitializer : public QObject, public LC_AppWindowAware{
    Q_OBJECT
public:
    explicit LC_ApplicationWindowInitializer(QC_ApplicationWindow* appWindow);
    ~LC_ApplicationWindowInitializer() override = default;
    void initApplication();
private:
    void initSnapManager();
    void initReleaseChecker();
    void initActionGroupManager();
    void initActionOptionsManager();
    void initActionFactory() const;
    void initDockCorners() const;
    void initCentralWidget();
    void initIconSize() const;
    void loadCmdWidgetVariablesFile() const;
    void initDockAreasActions() const;
    void initMainMenu() const;
    static void updateCommandsAlias();
    void initRecentFilesList() const;
    void initDialogFactory() const;
    void initWidgets() const;
    void initToolbars() const;
    void initPlugins();
    void initAutoSaveTimer() const;
    void initActionContext() const;
};

#endif // LC_APPLICATIONWINDOWINITIALIZER_H
