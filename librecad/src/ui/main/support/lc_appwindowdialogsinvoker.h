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

#ifndef LC_APPLICATIONWINDOWDIALOGSHELPER_H
#define LC_APPLICATIONWINDOWDIALOGSHELPER_H

#include <QObject>
#include <rs.h>

#include "lc_appwindowaware.h"

class LC_ReleaseChecker;
class QC_ApplicationWindow;
class RS_Graphic;
class QC_MDIWindow;

class LC_AppWindowDialogsInvoker : public QObject, public LC_AppWindowAware{
    Q_OBJECT
public:
    explicit LC_AppWindowDialogsInvoker(QC_ApplicationWindow *appWin);
    void showAboutWindow() const;
    void showNewVersionAvailableDialog(LC_ReleaseChecker* releaseChecker) const;
    void showLicenseWindow() const;
    void showDeviceOptions();
    bool showWidgetOptionsDialog() const;
    bool showGeneralOptionsDialog() const;
    int requestOptionsDrawingDialog(RS_Graphic& graphic, int tabIndex) const;
    int showCloseDialog(const QC_MDIWindow *w, bool showSaveAll) const;
    QPair<QString, QString> showExportFileSelectionDialog(const QString& drawingFileName) const;
    QPair<QString, RS2::FormatType> requestDrawingFileName(RS2::FormatType type = RS2::FormatDXFRW) const;
};

#endif // LC_APPLICATIONWINDOWDIALOGSHELPER_H
