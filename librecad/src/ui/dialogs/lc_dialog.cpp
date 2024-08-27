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

#include "lc_dialog.h"
#include "rs_settings.h"

LC_Dialog::LC_Dialog(QWidget* parent, const QString& dlgName)
    :QDialog(parent)
    ,dialogName(dlgName){
}

void LC_Dialog::showEvent(QShowEvent *event) {
    QDialog::showEvent(event);
    if (!positionLoaded) {
        loadDialogPosition();
        positionLoaded = true;
    }
}

QString LC_Dialog::getPositionSettingsGroupName() const{
    return "Dlg"+dialogName;
}

void LC_Dialog::loadDialogPosition() {
    LC_GROUP("Appearance");
    bool persistentDialogPositions = LC_GET_BOOL("PersistDialogPositions", false);
    bool restoreSizeOnly = LC_GET_BOOL("PersistDialogRestoreSizeOnly", false);
    if (persistentDialogPositions) {
        LC_GROUP_GUARD(getPositionSettingsGroupName());
        {
            bool hasSettings = LC_GET_BOOL("hasPosition");
            if (hasSettings) {
                int x = LC_GET_INT("X", 0);
                int y = LC_GET_INT("Y", 0);
                int h = LC_GET_INT("Height", 0);
                int w = LC_GET_INT("Width", 0);
                if (x > 0 && y > 0 && h > 0 && w > 0) {
                    if (!restoreSizeOnly) {
                        move(x, y);
                    }
                    resize(w, h);
                }
            }
        }
    }
}

void LC_Dialog::saveDialogPosition() const {
    bool persistentDialogPositions = LC_GET_ONE_BOOL("Appearance","PersistDialogPositions", false);
    if (persistentDialogPositions) {
        LC_GROUP_GUARD(getPositionSettingsGroupName());
        {
            LC_SET("hasPosition", true);

            const QPoint &point = pos();
            const QSize &size = QWidget::size();

            int x = point.x();
            int y = point.y();
            int h = size.height();
            int w = size.width();
            LC_SET("X", x);
            LC_SET("Y", y);
            LC_SET("Height", h);
            LC_SET("Width", w);
        }
    }
}

void LC_Dialog::accept() {
    saveDialogPosition();
    QDialog::accept();
}

void LC_Dialog::reject() {
    saveDialogPosition();
    QDialog::reject();
}
