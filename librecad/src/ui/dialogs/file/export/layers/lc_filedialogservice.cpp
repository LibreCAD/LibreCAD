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
 *
 */

#include "lc_filedialogservice.h"

#include <QApplication>
#include <QCheckBox>
#include <QDir>
#include <QFileDialog>
#include <QGridLayout>
#include <QMessageBox>

#include "lc_layerexportoptions.h"
#include "rs_debug.h"
#include "rs_settings.h"

class QCheckBox;

namespace {

/* Constant variables initialization - START */
    QStringList filtersStringList = {
        /* Drawing filters */
        "Drawing Exchange DXF 2007 (*.dxf)",
        "Drawing Exchange DXF 2004 (*.dxf)",
        "Drawing Exchange DXF 2000 (*.dxf)",
        "Drawing Exchange DXF R14 (*.dxf)",
        "Drawing Exchange DXF R12 (*.dxf)",

#ifdef DWGSUPPORT
        "DWG Drawing (*.dwg)",
#endif

        "LFF Font (*.lff)",
        "QCAD Font (*.cxf)"

#ifdef JWW_WRITE_SUPPORT
        , "JWW Drawing (*.jww)"
#endif
    };

    QList<RS2::FormatType> filtersTypeList =
    {
        /* Drawing filters */
        RS2::FormatDXFRW,
        RS2::FormatDXFRW2004,
        RS2::FormatDXFRW2000,
        RS2::FormatDXFRW14,
        RS2::FormatDXFRW12,

#ifdef DWGSUPPORT
        RS2::FormatDWG,
#endif

        RS2::FormatLFF,
        RS2::FormatCXF

#ifdef JWW_WRITE_SUPPORT
            , RS2::FormatJWW
#endif
    };

    /*
            Number of values pertaining to the open file dialog mode,
            as defined in one of the relevant lists below.
        */
    [[maybe_unused]] const int numberOf_openFileModes = 0;

    const QStringList filterSettingsPaths =
    {
        "/SaveDrawingFilter", /* SaveDrawing mode           */
        "/SaveDrawingFilter", /* ExportLayersSelected mode  */
        "/SaveDrawingFilter" /* ExportLayersVisible mode   */
    };

    const QStringList fileIOSettingsPaths =
    {
        "/Save", /* SaveDrawing mode           */
        "/Save", /* ExportLayersSelected mode  */
        "/Save" /* ExportLayersVisible mode   */
    };

    const int defaultFilters_indices[] =
    {
        /* List of save modes. */
        0, /* SaveDrawing mode           */
        0, /* ExportLayersSelected mode  */
        0 /* ExportLayersVisible mode   */
    };

    const QStringList fileDialogTitles =
    {
        /* List of save modes. */
        "Save Drawing as", /* SaveDrawing mode           */
        "Export selected layer(s)", /* ExportLayersSelected mode  */
        "Export visible layer(s)" /* ExportLayersVisible mode   */
    };


    void updateFileExtension(LC_FileDialogService::FileDialogResult& result, const QString& selectedFilter) {
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

    RS2::FormatType getFormatType(const QString& formatString){
        int index = filtersStringList.indexOf(formatString);
        return (index >= 0 && index < filtersTypeList.count()) ? filtersTypeList.at(index) : RS2::FormatDXFRW;
    }

    std::pair<QString, QString> readDefaultDirFilter() {
        LC_GROUP_GUARD("Paths");
        {
            QString defaultDir = LC_GET_STR("Save", QDir::toNativeSeparators(QDir::homePath()));
            QString defaultFilter = LC_GET_STR("SaveDrawingFilter", filtersStringList.at(0));
            return {defaultDir, defaultFilter};
        }
    }
}

/*
    This service class centralizes the file I/O user interface.
*/
// fixme - sand - decide what to do with this method, whether it's possible to have truly reusable generic file dialogs service?
LC_FileDialogService::FileDialogResult LC_FileDialogService::getFileDetails (FileDialogMode const& fileDialogMode){
    RS_DEBUG->print("LC_FileDialogService::getFileName");

    std::pair<QString, QString> defaultDirFilter = readDefaultDirFilter();

    auto saveFileDialog = std::make_unique<QFileDialog>( nullptr,
                                                         fileDialogTitles.at (fileDialogMode),
                                                         defaultDirFilter.first,
                                                         filtersStringList.join(";;"));

    saveFileDialog->selectNameFilter (defaultDirFilter.second);
    saveFileDialog->setAcceptMode (QFileDialog::AcceptSave);
    saveFileDialog->setOption (QFileDialog::HideNameFilterDetails, false);

    bool useQtFileDialog = LC_GET_ONE_BOOL("Defaults","UseQtFileOpenDialog");
    saveFileDialog->setOption (QFileDialog::DontUseNativeDialog, useQtFileDialog);

    // Styling the QFileDialog widget
    std::unique_ptr<QCheckBox> checkBox_combinedSave;

    if ((fileDialogMode == ExportLayersSelected) || (fileDialogMode == ExportLayersVisible))    {
       saveFileDialog->setOption (QFileDialog::DontUseNativeDialog, true);
       QLayout* layout = saveFileDialog->layout();
       if (layout != nullptr) {
           checkBox_combinedSave = std::make_unique<QCheckBox>(QObject::tr("Combine all layers"));
           // layout->addWidget(checkBox_combinedSave.get());

           auto* exportOptionsWidget = new LC_LayerExportOptionsWidget(saveFileDialog.get());
           auto* gridLayout = dynamic_cast<QGridLayout*>(layout);
           if (gridLayout != nullptr) {
               gridLayout->addWidget(exportOptionsWidget, 5, 0, 1, 3);
           }
       }
    }

    FileDialogResult result{};
    while (true) {
        if (saveFileDialog->QFileDialog::exec() == QDialog::Accepted) {
            result.filePath = QDir::toNativeSeparators(QFileInfo(saveFileDialog->QFileDialog::selectedFiles().at(0)).absoluteFilePath());
            result.dirPath = QFileInfo(result.filePath).absolutePath();
            result.fileName = QFileInfo(result.filePath).fileName();

            const QString selectedFilter = saveFileDialog->QFileDialog::selectedNameFilter();
            result.fileType = getFormatType(selectedFilter);

            // update file extension info
            updateFileExtension(result, selectedFilter);

            /* Confirm if the user wants to overwrite an existing file. */
            if (QFileInfo::exists(result.filePath)) {
                int replaceFileResponse = QMessageBox::warning(
                    QApplication::activeWindow(),
                    fileDialogTitles.at(fileDialogMode),
                    QObject::tr(R"(File "%1" already exists. Do you want to replace it?)").arg(result.fileName),
                    QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel, QMessageBox::Cancel
                );

                if (replaceFileResponse != QMessageBox::Yes)
                    continue;
            }

            if (checkBox_combinedSave != nullptr)
                result.checkState = checkBox_combinedSave->checkState();

            LC_GROUP_GUARD("Paths");
            {
                LC_SET(fileIOSettingsPaths.at(fileDialogMode), result.dirPath);
                LC_SET(filterSettingsPaths.at(fileDialogMode), selectedFilter);
            }

            break;
        } else {
            result = {};
            break;
        }
    }

    RS_DEBUG->print("LC_FileDialogService::getFileName: OK");
    return result;
}
