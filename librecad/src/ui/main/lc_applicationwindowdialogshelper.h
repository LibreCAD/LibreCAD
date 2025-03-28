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

#ifndef LC_APPLICATIONWINDOWDIALOGSHELPER_H
#define LC_APPLICATIONWINDOWDIALOGSHELPER_H

#include <QObject>
class LC_ReleaseChecker;
class QC_ApplicationWindow;

class LC_ApplicationWindowDialogsHelper : public QObject{
    Q_OBJECT
public:
    LC_ApplicationWindowDialogsHelper(QC_ApplicationWindow *appWin);
    void showAboutWindow();
    void showNewVersionAvailableDialog(LC_ReleaseChecker* releaseChecker);
    void showLicenseWindow();
    void showDeviceOptions();
    bool showWidgetOptionsDialog();
    bool showGeneralOptionsDialog();
    int requestOptionsDrawingDialog(RS_Graphic& graphic, int tabIndex);
    int showCloseDialog(QC_MDIWindow *w, bool showSaveAll);
    QPair<QString, QString> showExportFileSelectionDialog(const QString& drawingFileName);
    QPair<QString, RS2::FormatType> requestDrawingFileName(RS2::FormatType type = RS2::FormatDXFRW);
signals:
private:
    QC_ApplicationWindow* m_appWindow;
};

#endif // LC_APPLICATIONWINDOWDIALOGSHELPER_H
