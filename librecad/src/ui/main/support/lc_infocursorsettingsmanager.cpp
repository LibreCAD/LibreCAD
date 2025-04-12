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

#include "lc_infocursorsettingsmanager.h"

#include <QAction>

#include "qc_applicationwindow.h"
#include "qc_mdiwindow.h"
#include "qg_graphicview.h"
#include "rs_settings.h"

void LC_InfoCursorSettingsManager::slotInfoCursorSetting(bool toggle) {
    auto* action = qobject_cast<QAction*>(sender());
    if (action != nullptr) {
        QVariant tag = action->property("InfoCursorActionTag");
        if (tag.isValid()) {
            bool ok;
            int tagValue = tag.toInt(&ok);
            if (ok) {
                bool doUpdate = true;
                switch (tagValue) {
                    case 0: {
                        LC_SET_ONE("InfoOverlayCursor", "Enabled", toggle);
                        emit showInfoCursorSettingChanged(toggle);
                        break;
                    }
                    case 1: {
                        LC_SET_ONE("InfoOverlayCursor", "ShowAbsolute", toggle);
                        break;
                    }
                    case 2: {
                        LC_SET_ONE("InfoOverlayCursor", "ShowSnapInfo", toggle);
                        break;
                    }
                    case 3: {
                        LC_SET_ONE("InfoOverlayCursor", "ShowRelativeDA", toggle);
                        break;
                    }
                    case 4: {
                        LC_SET_ONE("InfoOverlayCursor", "ShowPrompt", toggle);
                        break;
                    }
                    case 5: {
                        LC_SET_ONE("InfoOverlayCursor", "ShowPropertiesCatched", toggle);
                        break;
                    }
                    default:
                        doUpdate = false;
                        break;
                }

                if (doUpdate) {
                   m_appWin->doForEachWindowGraphicView([](QG_GraphicView* gv, [[maybe_unused]]QC_MDIWindow* w)
                    {
                        // fixme - sand - files - probably just rely on signal??
                        gv->loadSettings();
                        gv->redraw();
                    });
                }
            }
        }
    }
}

void LC_InfoCursorSettingsManager::loadFromSettings() const {
    LC_GROUP("InfoOverlayCursor");
    {
        bool infoCursorEnabled = LC_GET_BOOL("Enabled", true);
        QAction *action = m_appWin->getAction("EntityDescriptionInfo");
        if (action != nullptr) {
            action->setVisible(infoCursorEnabled);
        }

        action = m_appWin->getAction("InfoCursorEnable");
        if (action != nullptr) {
            action->setChecked(infoCursorEnabled);
        }
        m_appWin->checkAction("InfoCursorAbs", LC_GET_BOOL("ShowAbsolute", true));
        m_appWin->checkAction("InfoCursorSnap",LC_GET_BOOL("ShowSnapInfo", true));
        m_appWin->checkAction("InfoCursorRel",LC_GET_BOOL("ShowRelativeDA", true));
        m_appWin->checkAction("InfoCursorPrompt",LC_GET_BOOL("ShowPrompt", true));
        m_appWin->checkAction("InfoCursorCatchedEntity",LC_GET_BOOL("ShowPropertiesCatched", true));
    }
    LC_GROUP_END();
}
