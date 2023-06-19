/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2021 Melwyn Francis Carlo
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software
** Foundation and appearing in the file gpl-2.0.txt included in the
** packaging of this file.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
**
** This copyright notice MUST APPEAR in all copies of the script!
**
**********************************************************************/

#include <QDebug>
#include <QDir>
#include <QLayout>
#include <QCheckBox>
#include <QFileDialog>
#include <QMessageBox>

#include "rs.h"
#include "rs_debug.h"
#include "rs_settings.h"

#include "lc_filedialogservice.h"

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
const int numberOf_openFileModes = 0;

const QStringList filterSettingsPaths =
{
    /* List of open modes. */


    /* List of save modes. */
    "/SaveDrawingFilter",               /* SaveDrawing mode           */
    "/SaveDrawingFilter",               /* ExportLayersSelected mode  */
    "/SaveDrawingFilter"                /* ExportLayersVisible mode   */
};

const QStringList fileIOSettingsPaths =
{
    /* List of open modes. */


    /* List of save modes. */
    "/Save",                            /* SaveDrawing mode           */
    "/Save",                            /* ExportLayersSelected mode  */
    "/Save"                             /* ExportLayersVisible mode   */
};

const int defaultFilters_indices[] =
{
    /* List of open modes. */


    /* List of save modes. */
    0,                                  /* SaveDrawing mode           */
    0,                                  /* ExportLayersSelected mode  */
    0                                   /* ExportLayersVisible mode   */
};

const QStringList fileDialogTitles =
{
    /* List of open modes. */


    /* List of save modes. */
    "Save Drawing as",                  /* SaveDrawing mode           */
    "Export selected layer(s)",         /* ExportLayersSelected mode  */
    "Export visible layer(s)"           /* ExportLayersVisible mode   */
};

/* Constant variables initialization - END */
void updateFileExtension(LC_FileDialogService::FileDialogResult& result, const QString& selectedFilter)
{

    QString saveFileExtension = selectedFilter.mid (selectedFilter.lastIndexOf ('.'));
    saveFileExtension.chop(1);

    result.fileExtension = saveFileExtension;

    if (!result.fileName.endsWith(result.fileExtension, Qt::CaseInsensitive))
        result.filePath += result.fileExtension;
    if (result.fileName.endsWith(saveFileExtension))
        result.fileName.resize(result.fileName.size() - result.fileExtension.size());
}

RS2::FormatType getFormatType(const QString& formatString)
{
    int index = filtersStringList.indexOf(formatString);
    return (index >= 0 && index < filtersTypeList.count()) ? filtersTypeList.at(index) : RS2::FormatDXFRW;
}

std::pair<QString, QString> readDefaultDirFilter()
{
    auto groupGuard = RS_SETTINGS->beginGroupGuard("/Paths");
    QString defaultDir    = RS_SETTINGS->readEntry( "/Save",
                                                    QDir::toNativeSeparators (QDir::homePath()));

    QString defaultFilter = RS_SETTINGS->readEntry( "/SaveDrawingFilter",
                                                    filtersStringList.at(0));
    return {defaultDir, defaultFilter};
}
}

/*
    This service class centralizes the file I/O user interface.
*/


LC_FileDialogService::FileDialogResult LC_FileDialogService::getFileDetails (FileDialogMode const& fileDialogMode)
{
    RS_DEBUG->print("LC_FileDialogService::getFileName");


    /* File save mode. */

            /* Reading from path settings - START */
    std::pair<QString, QString> defaultDirFilter=readDefaultDirFilter();
    /* Reading from path settings - END */

    /* Setting up the QFileDialog widget - START */
    auto saveFileDialog = std::make_unique<QFileDialog>( nullptr,
                                                         fileDialogTitles.at (fileDialogMode),
                                                         defaultDirFilter.first,
                                                         filtersStringList.join(";;"));

    saveFileDialog->selectNameFilter (defaultDirFilter.second);

    /* File open mode. */
//    if ((int) fileDialogMode < (int) numberOf_openFileModes)
//    {
//        saveFileDialog->QFileDialog::setAcceptMode (QFileDialog::AcceptOpen);
//    }
//    /* File save mode. */
//    else
//    {
    saveFileDialog->setAcceptMode (QFileDialog::AcceptSave);
//    }

    saveFileDialog->setOption (QFileDialog::HideNameFilterDetails, false);
    /* Setting up the QFileDialog widget - END */



    /* Reading from default settings - START */
    {
        auto groupGuard = RS_SETTINGS->beginGroupGuard("/Defaults");
        bool useQtFileDialog = RS_SETTINGS->readNumEntry("/UseQtFileOpenDialog", 0) != 0;
        saveFileDialog->setOption (QFileDialog::DontUseNativeDialog, useQtFileDialog);
    }
    /* Reading from default settings - END */



    /* Styling the QFileDialog widget - START LC_FileDialogService*/
    std::unique_ptr<QCheckBox> checkBox_combinedSave;

    if ((fileDialogMode == ExportLayersSelected) || (fileDialogMode == ExportLayersVisible))
    {
        checkBox_combinedSave = std::make_unique<QCheckBox>(QObject::tr("Combine all layers"));

        saveFileDialog->layout()->addWidget(checkBox_combinedSave.get());
    }
    /* Styling the QFileDialog widget - END */


    FileDialogResult result{};
    while (true)
    {
        if (saveFileDialog->QFileDialog::exec() == QDialog::Accepted)
        {
            result.filePath = QDir::toNativeSeparators (QFileInfo (saveFileDialog->QFileDialog::selectedFiles().at(0)).absoluteFilePath());
            result.dirPath  = QFileInfo(result.filePath).absolutePath();
            result.fileName = QFileInfo(result.filePath).fileName();

            const QString selectedFilter = saveFileDialog->QFileDialog::selectedNameFilter();

            result.fileType = getFormatType(selectedFilter);

            // update file extension info
            updateFileExtension(result, selectedFilter);

            /* File save mode. */
            //#if !defined (Q_OS_WIN) && !defined(Q_OS_MACOS)
            /* Confirm if the user wants to overwrite an existing file. */
            if (QFileInfo(result.filePath).exists())
            {
                int replaceFileResponse = QMessageBox::warning(
                            nullptr,
                            fileDialogTitles.at (fileDialogMode),
                            QObject::tr(R"(File "%1" already exists. Do you want to replace it?)").arg(result.fileName),
                            QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel, QMessageBox::Cancel
                            );

                if (replaceFileResponse != QMessageBox::Yes)
                    continue;
            }
            //#endif

            if (checkBox_combinedSave != nullptr)
                result.checkState = checkBox_combinedSave->checkState();

            /* Writing to path settings - START */
            {
                auto groupGuard = RS_SETTINGS->beginGroupGuard("/Paths");
                RS_SETTINGS->writeEntry(fileIOSettingsPaths.at(fileDialogMode), result.dirPath);
                RS_SETTINGS->writeEntry(filterSettingsPaths.at(fileDialogMode), selectedFilter);
            }
            /* Writing to path settings - END */

            break;
        }
        else
        {
            result = {};
            break;
        }
    }

    RS_DEBUG->print("LC_FileDialogService::getFileName: OK");

    return result;
}

