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


#include <QFileDialog>
#include <QImageWriter>
#include <QMessageBox>

#include "comboboxoption.h"
#include "lc_appwindowdialogsinvoker.h"
#include "lc_dlgabout.h"
#include "lc_dlgnewversionavailable.h"
#include "lc_widgetoptionsdialog.h"
#include "qc_applicationwindow.h"
#include "qc_mdiwindow.h"
#include "qg_dialogfactory.h"
#include "qg_dlgoptionsdrawing.h"
#include "qg_dlgoptionsgeneral.h"
#include "qg_exitdialog.h"
#include "qg_filedialog.h"
#include "rs_settings.h"
#include "rs_system.h"
#include "textfileviewer.h"

LC_AppWindowDialogsInvoker::LC_AppWindowDialogsInvoker(QC_ApplicationWindow *appWin)
  :LC_AppWindowAware(appWin) {
}

void LC_AppWindowDialogsInvoker::showAboutWindow() const {
    LC_DlgAbout dlg(m_appWin);
    dlg.exec();
}

void LC_AppWindowDialogsInvoker::showNewVersionAvailableDialog( LC_ReleaseChecker* releaseChecker) const {
    LC_DlgNewVersionAvailable dlg(m_appWin, releaseChecker);
    dlg.exec();
}

void LC_AppWindowDialogsInvoker::showLicenseWindow() const {
    QDialog dlg(m_appWin); 
    dlg.setWindowTitle(tr("License"));  // Use non-static tr() since this class likely inherits QObject

    auto* viewer = new TextFileViewer(&dlg);  // Use raw pointer with parent for Qt ownership
    auto* layout = new QVBoxLayout;
    layout->addWidget(viewer);
    dlg.setLayout(layout);

    // Check and add files with error handling
    QStringList missingFiles;
    std::pair<QString, QString> files[] = {
	    {"readme.md", "readme"},
	    {"gpl-2.0.txt", "GPLv2"},
    };
    for(const auto& [fileName, title] : files) {
	const QString file = ":/" + fileName;
        if (QFile::exists(file)) {
            viewer->addFile(title, file);
        } else {
            missingFiles << file;
        }
    }

    // If any files are missing, show a warning
    if (missingFiles.isEmpty()) {
        // Attempt to set the default file if available
        viewer->setFile("readme");
        dlg.resize(800, 600);  // Set a reasonable default size for better UX; adjust as needed
        dlg.exec();  
    } else {
        QString errorMsg = tr("The following files could not be loaded:\n") + missingFiles.join("\n");
        QMessageBox::warning(&dlg, tr("Error"), errorMsg);
    }
}

void LC_AppWindowDialogsInvoker::showDeviceOptions() {
    QDialog dlg (m_appWin);
    dlg.setWindowTitle(tr("Device Options"));
    auto layout = new QVBoxLayout;
    auto device_combo = new ComboBoxOption(&dlg);
    device_combo->setTitle(tr("Device"));
    device_combo->setOptionsList(QStringList({"Mouse", "Tablet", "Trackpad", "Touchscreen"}));
    device_combo->setCurrentOption(LC_GET_ONE_STR("Hardware","Device", "Mouse"));
    layout->addWidget(device_combo);
    dlg.setLayout(layout);
    connect(device_combo, &ComboBoxOption::optionToSave,m_appWin, &QC_ApplicationWindow::updateDevice);
    dlg.exec();
}

bool LC_AppWindowDialogsInvoker::showWidgetOptionsDialog() const {
    LC_WidgetOptionsDialog dlg(m_appWin);
    return dlg.exec() == QDialog::Accepted;
}

bool LC_AppWindowDialogsInvoker::showGeneralOptionsDialog() const {
    QG_DlgOptionsGeneral dlg(m_appWin);
    bool  result = dlg.exec() == QDialog::Accepted;
    return result;
}

int LC_AppWindowDialogsInvoker::requestOptionsDrawingDialog(RS_Graphic &graphic, int tabIndex) const {
    QG_DlgOptionsDrawing dlg(m_appWin);
    dlg.setGraphic(&graphic);
    dlg.showInitialTab(tabIndex);
    int result = dlg.exec();
    return result;
}

/**
 * Show a Save/Close/Cancel(All) dialog for the content of this sub-window.
 * The window handle must not be null, and the document must actually have been modified.
 *
 * @param w
 * @param showSaveAll show a Save All button and rename Close -> Close All
 * @return QG_ExitDialog::ExitDialogResult the button that was pressed, or -1 if invoked in error
 * @see QG_ExitDialog
 */
int LC_AppWindowDialogsInvoker::showCloseDialog(const QC_MDIWindow *w, bool showSaveAll) const {
    QG_ExitDialog dlg(m_appWin);
    dlg.setShowOptionsForAll(showSaveAll);
    dlg.setTitle(tr("Closing Drawing"));
    if (w != nullptr && w->getDocument()->isModified()) {
        QString fileName = w->getFileName();
        if (fileName.isEmpty()) {
            fileName = w->windowTitle();
            fileName.remove("[*]");
            fileName.remove( " [" + tr("Draft Mode") + "]");
        }
        else if (fileName.length() > 50) {
            fileName = QString("%1...%2").arg(fileName.left(24)).arg(fileName.right(24));
        }

        dlg.setText(tr("Save changes to the following item?\n%1").arg(fileName));
        return dlg.exec();
    }
    return -1; // should never get here; please send only modified documents
}

QPair<QString, QString> LC_AppWindowDialogsInvoker::showExportFileSelectionDialog(const QString& drawingFileName) const {
    // read default settings:
    LC_GROUP("Export");
    QString defDir = LC_GET_STR("ExportImage", RS_SYSTEM->getHomeDir());
    QString defFilter = LC_GET_STR("ExportImageFilter",
                                   QString("%1 (%2)(*.%2)").arg(QG_DialogFactory::extToFormat("png")).arg("png"));
    LC_GROUP_END();

    QStringList filters;
    QList<QByteArray> supportedImageFormats = QImageWriter::supportedImageFormats();
    supportedImageFormats.push_back("svg"); // add svg

    for (QString format: supportedImageFormats) {
        format = format.toLower();
        QString st;
        if (format == "jpeg" || format == "tiff") {
            // Don't add the aliases
        } else {
            st = QString("%1 (%2)(*.%2)").arg(QG_DialogFactory::extToFormat(format), format);
        }
        if (st.length() > 0)
            filters.push_back(st);
    }
    // revise list of filters
    filters.removeDuplicates();
    filters.sort();

    // set dialog options: filters, mode, accept, directory, filename
    QFileDialog fileDlg(m_appWin, tr("Export as"));

    fileDlg.setNameFilters(filters);
    fileDlg.setFileMode(QFileDialog::AnyFile);
    fileDlg.selectNameFilter(defFilter);
    fileDlg.setAcceptMode(QFileDialog::AcceptSave);
    fileDlg.setDirectory(defDir);

    QString fileName = QFileInfo(drawingFileName).baseName();
    if (fileName == nullptr) {
        fileName = "unnamed";
    }
    fileDlg.selectFile(fileName);

    if (fileDlg.exec() == QDialog::Accepted) {
        QStringList files = fileDlg.selectedFiles();
        if (!files.isEmpty()) {
            fileName = files[0];

            // store new default settings:
            LC_GROUP_GUARD("Export");{
                LC_SET("ExportImage", QFileInfo(fileName).absolutePath());
                LC_SET("ExportImageFilter",
                       fileDlg.selectedNameFilter());
            }

            // find out extension:
            QString filter = fileDlg.selectedNameFilter();
            QString format = "";
            int i = filter.indexOf("(*.");
            if (i != -1) {
                int i2 = filter.indexOf(QRegularExpression("[) ]"), i);
                format = filter.mid(i + 3, i2 - (i + 3));
                format = format.toUpper();
            }

            // append extension to file:
            if (!QFileInfo(fileName).fileName().contains(".")) {
                fileName.push_back("." + format.toLower());
            }

            return {fileName, format};
        }
    }
    return {"", ""};
}

QPair<QString, RS2::FormatType> LC_AppWindowDialogsInvoker::requestDrawingFileName(RS2::FormatType format) const {
    QG_FileDialog dlg(m_appWin);
    RS2::FormatType type = format;
    QString dxfPath = dlg.getOpenFile(&type);
    return {dxfPath, type};
}
