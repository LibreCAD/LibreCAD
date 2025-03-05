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

#ifndef LC_WORKSPACELISTBUTTON_H
#define LC_WORKSPACELISTBUTTON_H

#include <QToolButton>
#include "qc_applicationwindow.h"

class LC_WorkspaceListButton:public QToolButton {
    Q_OBJECT
public:
    explicit LC_WorkspaceListButton(QC_ApplicationWindow *win);
    void enableSubActions(bool value);

protected:
    QMenu *menu;
    QList<QAction *> createdActions;
    QC_ApplicationWindow *mainWin{nullptr};
    QIcon wsIcon;
protected slots:
    void fillMenu();
    void menuTriggered(bool checked = false);
};
#endif // LC_WORKSPACELISTBUTTON_H
