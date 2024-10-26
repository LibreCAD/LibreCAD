/*******************************************************************************
 *
 This file is part of the LibreCAD project, a 2D CAD program

 Copyright (C) 2024 LibreCAD.org
 Copyright (C) 2024 sand1024

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

#include "lc_qtstatusbarmanager.h"
#include "rs_settings.h"
#include "lc_shortcuts_manager.h"

LC_QTStatusbarManager::LC_QTStatusbarManager(QStatusBar *sBar):QObject(sBar) {
    statusBar = sBar;
}

void LC_QTStatusbarManager::setActionHelp(const QString &left, [[maybe_unused]]const QString &right, const LC_ModifiersInfo &modifiersInfo) const {
    if (actionPromptEnabled && statusBar->isVisible()){
        QString modifiersMsg = "";
        const QString &shiftMessage = modifiersInfo.getShiftMessage();
        if (!shiftMessage.isEmpty()){
            modifiersMsg = modifiersMsg + tr(" SHIFT Key: ") + shiftMessage;
        }
        const QString &ctrlMessage = modifiersInfo.getCtrlMessage();
        if (!ctrlMessage.isEmpty()){
            modifiersMsg = modifiersMsg + tr(" CTRL Key: ") + ctrlMessage;
        }

        QString infoMessage = left.trimmed();
        if (infoMessage.endsWith(":")){
            infoMessage.chop(1);
        }

        QString message;
        if (actionToolTip.isEmpty()){ // default action
            message = modifiersMsg;
        }
        else{
            message = actionToolTip + " | "  + infoMessage;
            if (!modifiersMsg.isEmpty()){
                message = message + " | "  + modifiersMsg;
            }
        }

        statusBar->showMessage(message);
    }
}

void LC_QTStatusbarManager::setCurrentQAction(QAction *a) {
    if (actionPromptEnabled && statusBar->isVisible()) {
        actionToolTip = LC_ShortcutsManager::getPlainActionToolTip(a);
    }
}

void LC_QTStatusbarManager::loadSettings() {
    LC_GROUP_GUARD("Startup");{
        bool useClassicalStatusBar = LC_GET_BOOL("UseClassicStatusBar", false);
        if (useClassicalStatusBar) {
            actionPromptEnabled = false;
        } else {
            actionPromptEnabled = LC_GET_BOOL("ShowCommandPromptInStatusBar", true);
        }
    }

    if (actionPromptEnabled){
        statusBar->showMessage("", 5); // just cleanup
    }
}

void LC_QTStatusbarManager::setup() {
    // todo - sand - for later use if more information will be shown in status bar as per #
//    QLabel* msgLabel = new QLabel(status_bar);
//    msgLabel->setText("Test");
//    status_bar->addPermanentWidget(msgLabel, 1);
}
