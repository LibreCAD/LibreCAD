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
#ifndef LC_INFOCURSORSETTINGSMANAGER_H
#define LC_INFOCURSORSETTINGSMANAGER_H

#include <QObject>
#include "lc_appwindowaware.h"

class LC_InfoCursorSettingsManager: public QObject, public LC_AppWindowAware{
    Q_OBJECT
public:
    explicit LC_InfoCursorSettingsManager(QC_ApplicationWindow* mainWin)
        : LC_AppWindowAware{mainWin} {
    }
    void slotInfoCursorSetting(bool toggle);
    void loadFromSettings() const;

signals:
     void showInfoCursorSettingChanged(bool enabled);
};

#endif // LC_INFOCURSORSETTINGSMANAGER_H
