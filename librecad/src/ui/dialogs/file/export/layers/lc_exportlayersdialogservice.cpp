/*
 * **************************************************************************
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
 * *********************************************************************
 */
#include "lc_exportlayersdialogservice.h"

#include <QApplication>
#include <QFileDialog>
#include <QGridLayout>
#include <QMessageBox>

#include "lc_documentsstorage.h"
#include "lc_layerexportoptions.h"
#include "lc_layersexporter.h"
#include "rs_graphic.h"
#include "rs_settings.h"

struct LC_FileDialogResult {
    QString dirPath;
    QString filePath;
    QString fileName;
    QString fileExtension;
    RS2::FormatType fileType = RS2::FormatUnknown;
};

void LC_ExportLayersService::exportLayers(LC_LayersExportOptions& exportOptions, RS_Graphic* sourceGraphic) {
    LC_ExportLayersService service;
    service.doExportLayers(exportOptions, sourceGraphic);
}

QString LC_ExportLayersService::createExportDocumentFileName(const bool separateFileForLayer, const LC_FileDialogResult& fileInfo, const size_t exportFilesCount, const size_t currentExportLayerIndex,
                                                             [[maybe_unused]]const QString& exportData) {
    QString actualFileName = QDir::toNativeSeparators(
        fileInfo.dirPath + "/"
        + fileInfo.fileName + (separateFileForLayer ? paddedIndex(currentExportLayerIndex + 1, exportFilesCount) : "")
        + fileInfo.fileExtension);
    return actualFileName;
}

void LC_ExportLayersService::doExportLayers(LC_LayersExportOptions& exportOptions, RS_Graphic* sourceGraphic) {
    LC_FileDialogResult fileInfo;
    exportOptions.sourceDrawingFileName = sourceGraphic->getFilename();
    if (selectLayersExportFile(exportOptions, fileInfo)) {
        QApplication::setOverrideCursor(Qt::WaitCursor);
        LC_LayersExporter layersExporter;
        std::vector<LC_LayerExportData> exportResultList;
        layersExporter.exportLayers(&exportOptions, sourceGraphic, exportResultList);
        LC_DocumentsStorage storage;

        const size_t exportFilesCount = exportResultList.size();

        for (size_t currentExportLayerIndex = 0; currentExportLayerIndex < exportFilesCount; currentExportLayerIndex++){
            const auto exportData = exportResultList[currentExportLayerIndex];
            const auto graphicToSave = exportData.graphic;
            QString actualFileName = createExportDocumentFileName(exportOptions.createSeparateDocumentPerLayer,
                                          fileInfo, exportFilesCount, currentExportLayerIndex, exportData.name);
            [[maybe_unused]]bool saveWasSuccessful = storage.exportGraphics(graphicToSave, actualFileName, fileInfo.fileType);

            graphicToSave->setGraphicView(nullptr);
            graphicToSave->setParent(nullptr);
            graphicToSave->initForNewDocument();

            /*                if (saveWasSuccessful){
                    RS_DIALOGFACTORY->commandMessage(
                        tr(R"(Saving layer "%1" as "%2" )").arg(copiedLayers, modifiedFilePath));
                } else {
                    RS_DEBUG->print(RS_Debug::D_ERROR, "LC_ActionLayersExport::trigger: Error encountered while exporting layers");
                    return;
                }
*/
        }
        QApplication::restoreOverrideCursor();
    }
    exportOptions.layers.clear();
}

QString LC_ExportLayersService::paddedIndex(const int index, const int totalNumber){
    // the maximum string size needed
    const int fieldWidth=QString::number(totalNumber).size();
    auto str = QString("%1").arg(index, fieldWidth, 10, QChar{'0'});
    return str;
}

bool LC_ExportLayersService::selectLayersExportFile(LC_LayersExportOptions& options, LC_FileDialogResult& fileInfo) {
    LC_ExportLayersService service;
    return service.selectExportFile(options, fileInfo);
}

LC_ExportLayersService::LC_ExportLayersService() {}

namespace
{
    const QStringList FILTERS_STRING_LIST = {
        /* Drawing filters */
        "Drawing Exchange DXF 2007 (*.dxf)",
        "Drawing Exchange DXF 2004 (*.dxf)",
        "Drawing Exchange DXF 2000 (*.dxf)",
        "Drawing Exchange DXF R14 (*.dxf)",
        "Drawing Exchange DXF R12 (*.dxf)",
    };

    const QList<RS2::FormatType> FILTERS_TYPE_LIST =
    {
        /* Drawing filters */
        RS2::FormatDXFRW,
        RS2::FormatDXFRW2004,
        RS2::FormatDXFRW2000,
        RS2::FormatDXFRW14,
        RS2::FormatDXFRW12,
    };

}

bool LC_ExportLayersService::selectExportFile(LC_LayersExportOptions& options, LC_FileDialogResult& fileInfo) {
    const std::pair<QString, QString> defaultDirFilter = readDefaultDirAndFilter();

    auto saveFileDialog = QFileDialog(nullptr, tr("Export Layers"), defaultDirFilter.first,
                                      FILTERS_STRING_LIST.join(";;"));
    saveFileDialog.selectNameFilter(defaultDirFilter.second);
    saveFileDialog.setAcceptMode (QFileDialog::AcceptSave);
    saveFileDialog.setOption (QFileDialog::HideNameFilterDetails, false);
    saveFileDialog.setOption (QFileDialog::DontUseNativeDialog, true);
    // We do our own overwrite confirmation below; suppress the dialog's built-in
    // one so the two do not stack into a double prompt.
    saveFileDialog.setOption (QFileDialog::DontConfirmOverwrite, true);

    const QString sourceFileName  = options.sourceDrawingFileName;
    QString preselectionFileName;
    if (!sourceFileName.isEmpty()) {
        const QFileInfo sourceFileInfo(sourceFileName);
        preselectionFileName = sourceFileInfo.baseName()  + " - Export "; // fixme - sand - use options?
    }
    else {
        preselectionFileName = tr("Exported Layers");
    }
    saveFileDialog.selectFile(preselectionFileName);

    QLayout* layout = saveFileDialog.layout();
    auto* exportOptionsWidget = new LC_LayerExportOptionsWidget(&saveFileDialog);
    auto* gridLayout = dynamic_cast<QGridLayout*>(layout);
    if (gridLayout != nullptr) {
        gridLayout->addWidget(exportOptionsWidget, 5, 0, 1, 3);
    }

    while (true) {
        if (saveFileDialog.exec() == QDialog::Accepted) {
            fileInfo.filePath = QDir::toNativeSeparators(QFileInfo(saveFileDialog.selectedFiles().at(0)).absoluteFilePath());
            fileInfo.dirPath = QFileInfo(fileInfo.filePath).absolutePath();
            fileInfo.fileName = QFileInfo(fileInfo.filePath).fileName();

            const QString selectedFilter = saveFileDialog.selectedNameFilter();
            fileInfo.fileType = getFormatType(selectedFilter);

            // update file extension info
            updateFileExtension(fileInfo, selectedFilter);

            /* Confirm if the user wants to overwrite an existing file. */
            if (QFileInfo::exists(fileInfo.filePath)) {
                const int replaceFileResponse = QMessageBox::warning(
                    QApplication::activeWindow(),
                    tr("Export Layers"),
                    QObject::tr(R"(File "%1" already exists. Do you want to replace it?)").arg(fileInfo.fileName),
                    QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel, QMessageBox::Cancel
                    );

                if (replaceFileResponse != QMessageBox::Yes) {
                    continue;
                }
            }

            exportOptionsWidget->fillLayerExportOptions(&options);

            saveDefaultDirAndFilter(fileInfo, selectedFilter);
            return true;
        }
        return false;
    }
}

std::pair<QString, QString> LC_ExportLayersService::readDefaultDirAndFilter() {
    LC_GROUP_GUARD("Export.Layers");
    {
        QString defaultDir = LC_GET_STR("DirPath", QDir::toNativeSeparators(QDir::homePath()));
        QString defaultFilter = LC_GET_STR("FileFilter", FILTERS_STRING_LIST.at(0));
        return {defaultDir, defaultFilter};
    }
}

void LC_ExportLayersService::saveDefaultDirAndFilter(const LC_FileDialogResult& fileInfo, const QString& selectedFilter) {
    LC_GROUP_GUARD("Export.Layers");
    {
        LC_SET("DirPath", fileInfo.dirPath);
        LC_SET("FileFilter", selectedFilter);
    }
}

RS2::FormatType LC_ExportLayersService::getFormatType(const QString& formatString){
    const int index = FILTERS_STRING_LIST.indexOf(formatString);
    return (index >= 0 && index < FILTERS_TYPE_LIST.count()) ? FILTERS_TYPE_LIST.at(index) : RS2::FormatDXFRW;
}

void LC_ExportLayersService::updateFileExtension(LC_FileDialogResult& result, const QString& selectedFilter) {
    QString saveFileExtension = selectedFilter.mid (selectedFilter.lastIndexOf ('.'));
    saveFileExtension.chop(1);
    result.fileExtension = saveFileExtension;
    if (!result.fileName.endsWith(result.fileExtension, Qt::CaseInsensitive)) {
        result.filePath += result.fileExtension;
    }
    if (result.fileName.endsWith(saveFileExtension)) {
        result.fileName.resize(result.fileName.size() - result.fileExtension.size());
    }
}
