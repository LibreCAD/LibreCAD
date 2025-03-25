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
#include <qc_applicationwindow.h>
#include "lc_applicationwindowdialogshelper.h"
#include "comboboxoption.h"
#include "lc_dlgabout.h"
#include "lc_dlgnewversionavailable.h"
#include "lc_widgetoptionsdialog.h"
#include "qg_dlgoptionsgeneral.h"
#include "textfileviewer.h"

LC_ApplicationWindowDialogsHelper::LC_ApplicationWindowDialogsHelper(QC_ApplicationWindow *appWin):m_appWindow(appWin)
{}

void LC_ApplicationWindowDialogsHelper::showAboutWindow() {
    LC_DlgAbout dlg(m_appWindow);
    dlg.exec();
}

void LC_ApplicationWindowDialogsHelper::showNewVersionAvailableDialog( LC_ReleaseChecker* releaseChecker){
    LC_DlgNewVersionAvailable dlg(m_appWindow, releaseChecker);
    dlg.exec();
}

void LC_ApplicationWindowDialogsHelper::invokeLicenseWindow() {
    QDialog dlg(m_appWindow);
    dlg.setWindowTitle(QObject::tr("License"));
    auto viewer = new TextFileViewer(&dlg);
    auto layout = new QVBoxLayout;
    layout->addWidget(viewer);
    dlg.setLayout(layout);

    viewer->addFile("readme", ":/readme.md");
    viewer->addFile("GPLv2", ":/gpl-2.0.txt");
    viewer->setFile("readme");
    dlg.exec();
}

void LC_ApplicationWindowDialogsHelper::showDeviceOptions() {
    QSettings settings;
    QDialog dlg (m_appWindow);
    dlg.setWindowTitle(tr("Device Options"));
    auto layout = new QVBoxLayout;
    auto device_combo = new ComboBoxOption(&dlg);
    device_combo->setTitle(tr("Device"));
    device_combo->setOptionsList(QStringList({"Mouse", "Tablet", "Trackpad", "Touchscreen"}));
    device_combo->setCurrentOption(settings.value("Hardware/Device", "Mouse").toString());
    layout->addWidget(device_combo);
    dlg.setLayout(layout);
    connect(device_combo, &ComboBoxOption::optionToSave,m_appWindow, &QC_ApplicationWindow::updateDevice);
    dlg.exec();
}

bool LC_ApplicationWindowDialogsHelper::widgetOptionsDialog(){
    LC_WidgetOptionsDialog dlg(m_appWindow);
    return dlg.exec() == QDialog::Accepted;
}

bool LC_ApplicationWindowDialogsHelper::requestOptionsGeneralDialog() {
    QG_DlgOptionsGeneral dlg(m_appWindow);
    bool  result = dlg.exec() == QDialog::Accepted;
    // fixme - sand - files - restore
    // getSnapOptionsHolder(); // as side effect, should update location of snap options
    return result  ;
}
