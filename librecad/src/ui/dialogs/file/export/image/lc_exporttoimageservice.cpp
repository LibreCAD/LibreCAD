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

#include "lc_exporttoimageservice.h"

#include <QApplication>

#include "lc_appwindowdialogsinvoker.h"
#include "lc_imageexporter.h"
#include "qc_applicationwindow.h"
#include "qg_dlgimageoptions.h"
#include "rs_graphic.h"

bool LC_ExportToImageService::exportGraphicsToImage(RS_Graphic* graphic, const QString& documentFileName) {
    QG_ImageOptionsDialog imageOptionsDialog = new QG_ImageOptionsDialog(m_appWin);
    graphic->calculateBorders();
    imageOptionsDialog.setGraphicSize(graphic->getSize() * 2.);

    if (imageOptionsDialog.exec() == QDialog::Accepted) {
        LC_ImageExporter::ExportOptions options;
        options.size = imageOptionsDialog.getSize();
        options.borders = imageOptionsDialog.getBorders();
        options.blackAndWhite = imageOptionsDialog.isBlackWhite();
        options.backgroundBlack = imageOptionsDialog.isBackgroundBlack();

        QPair<QString, QString> fileFormat = m_dlgHelpr->showExportFileSelectionDialog(documentFileName);
        QString fileName = fileFormat.first;
        if (fileName.isEmpty()) {
            return true;
        }

        options.fileName = fileName;
        options.format = fileFormat.second;

        m_appWin->showStatusMessage(tr("Exporting drawing..."), 2000);
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

        LC_ImageExporter exporter;

        bool ret = exporter.exportToImage(graphic, options);
        if (ret) {
            QString message = tr("Exported: %1").arg(fileName);
            m_appWin->notificationMessage(message, 20000);
        } else {
            m_appWin->showStatusMessage(tr("Export failed!"), 2000);
        }
        QApplication::restoreOverrideCursor();
        return ret;
    }
    return true;
}
