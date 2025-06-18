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

#include "lc_filenameselectionservice.h"

#include <QFileDialog>
#include <QWidget>

#include "rs_settings.h"
#include "rs_system.h"

bool LC_FileNameSelectionService::doObtainFileName(QWidget* parent, QString& fileName,
                                                   bool forRead,
                                                   const QString& extensionStr,
                                                   const QString& defaultFileName,
                                                   const QString& captionForImport, const QString& captionForExport,
                                                   QString fileNameFilter) {
    LC_GROUP("Export");
    QString defDir = LC_GET_STR("ExportSettingsDir", RS_SYSTEM->getHomeDir());
    LC_GROUP_END();

    bool useQtFileDialog = LC_GET_ONE_BOOL("Defaults","UseQtFileOpenDialog");

    const QString defaultExtension{extensionStr};

    const auto defaultFilter = fileNameFilter.arg(defaultExtension);
    QFileDialog fileDlg(parent, forRead ? captionForImport  : captionForExport);
    fileDlg.setDefaultSuffix(defaultExtension);
    fileDlg.setNameFilter(defaultFilter);
    fileDlg.setFileMode(forRead ? QFileDialog::ExistingFile : QFileDialog::AnyFile);
    fileDlg.selectNameFilter(defaultFilter);
    fileDlg.setAcceptMode(forRead ? QFileDialog::AcceptOpen : QFileDialog::AcceptSave);
    fileDlg.setOption(QFileDialog::DontUseNativeDialog, useQtFileDialog);
    fileDlg.setDirectory(defDir);

    QString fileNameForSelection = defaultFileName;
    if (!defaultFileName.endsWith(extensionStr)) {
        int dotIndex = defaultFileName.indexOf('.');
        fileNameForSelection = defaultFileName.left(dotIndex);
        fileNameForSelection.append(".").append(extensionStr);

    }
    fileDlg.selectFile(fileNameForSelection);

    bool proceed = false;
    if (fileDlg.exec() == QDialog::Accepted) {
        QStringList files = fileDlg.selectedFiles();
        if (!files.isEmpty()) {
            fileName = files.front();
            QString extension = QString(".").append(extensionStr);
            if (!fileName.endsWith(extension)) {
                fileName = fileName +extension;
            }
            proceed = true;
        }
    }
    return proceed;
}
