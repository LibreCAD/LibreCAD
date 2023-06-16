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
    QStringList filters_stringList[] =
    {
        /* Drawing filters */
        {
            "Drawing Exchange DXF 2007 (*.dxf)",
            "Drawing Exchange DXF 2004 (*.dxf)",
            "Drawing Exchange DXF 2000 (*.dxf)",
            "Drawing Exchange DXF R14 (*.dxf)",
            "Drawing Exchange DXF R12 (*.dxf)",

            #ifdef DWGSUPPORT
                "DWG Drawing (*.dwg)",
            #endif

            "LFF Font (*.lff)",
            "QCAD Font (*.cxf)",
            "JWW Drawing (*.jww)"
        }
    };

    QList<RS2::FormatType> filters_typeList[] =
    {
        /* Drawing filters */
        {
            RS2::FormatDXFRW,
            RS2::FormatDXFRW2004,
            RS2::FormatDXFRW2000,
            RS2::FormatDXFRW14,
            RS2::FormatDXFRW12,

            #ifdef DWGSUPPORT
                RS2::FormatDWG,
            #endif

            RS2::FormatLFF,
            RS2::FormatCXF,
            RS2::FormatJWW
        }
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
}

/*
    This service class centralizes the file I/O user interface.
*/


LC_FileDialogService::FileDialogResult LC_FileDialogService::getFileDetails (FileDialogMode const& fileDialogMode)
{
    RS_DEBUG->print("LC_FileDialogService::getFileName");


    /* File save mode. */
    //if ((int) fileDialogMode >= (int) numberOf_openFileModes)
    {
#ifndef JWW_WRITE_SUPPORT
        filters_stringList[0].QList::removeLast();
        filters_typeList[0].QList::removeLast();
#endif
    }




    /* Reading from path settings - START */
        RS_SETTINGS->beginGroup("/Paths");
        QString defaultDir    = RS_SETTINGS->readEntry( fileIOSettingsPaths.at (fileDialogMode), 
                                                        QDir::toNativeSeparators (QDir::homePath()));

        QString defaultFilter = RS_SETTINGS->readEntry( filterSettingsPaths.at (fileDialogMode), 
                                                        filters_stringList [defaultFilters_indices [fileDialogMode]].at(0));
        RS_SETTINGS->endGroup();
    /* Reading from path settings - END */



    /* Setting up the QFileDialog widget - START */
        QFileDialog* saveFileDialog = new QFileDialog( nullptr, 
                                                       fileDialogTitles.at (fileDialogMode), 
                                                       defaultDir, 
                                                       filters_stringList [defaultFilters_indices [fileDialogMode]].join(";;"));

        saveFileDialog->QFileDialog::selectNameFilter (defaultFilter);

        /* File open mode. */
        if ((int) fileDialogMode < (int) numberOf_openFileModes)
        {
            saveFileDialog->QFileDialog::setAcceptMode (QFileDialog::AcceptOpen);
        }
        /* File save mode. */
        else
        {
            saveFileDialog->QFileDialog::setAcceptMode (QFileDialog::AcceptSave);
        }

        saveFileDialog->setOption (QFileDialog::HideNameFilterDetails, false);
    /* Setting up the QFileDialog widget - END */



    /* Reading from default settings - START */
        RS_SETTINGS->beginGroup("/Defaults");

        if (RS_SETTINGS->readEntry("/UseQtFileOpenDialog", 0) == 0)
        {
            saveFileDialog->setOption (QFileDialog::DontUseNativeDialog, false);
        }
        else
        {
            saveFileDialog->setOption (QFileDialog::DontUseNativeDialog, true);
        }

        RS_SETTINGS->endGroup();
    /* Reading from default settings - END */



    /* Styling the QFileDialog widget - START LC_FileDialogService*/
        QCheckBox *checkBox_combinedSave = nullptr;

        if ((fileDialogMode == ExportLayersSelected) || (fileDialogMode == ExportLayersVisible))
        {
            checkBox_combinedSave = new QCheckBox(QObject::tr("Combine all layers"));

            saveFileDialog->layout()->addWidget(checkBox_combinedSave);
        }
    /* Styling the QFileDialog widget - END */


    FileDialogResult result;
    while (true)
    {
        if (saveFileDialog->QFileDialog::exec() == QDialog::Accepted)
        {
            result.filePath = QDir::toNativeSeparators (QFileInfo (saveFileDialog->QFileDialog::selectedFiles().at(0)).absoluteFilePath());
            result.dirPath  = QFileInfo(result.filePath).absolutePath();
            result.fileName = QFileInfo(result.filePath).fileName();

            const QString selectedFilter = saveFileDialog->QFileDialog::selectedNameFilter();

            result.fileType = filters_typeList [defaultFilters_indices [fileDialogMode]].at (

                                    filters_stringList [defaultFilters_indices [fileDialogMode]].indexOf (selectedFilter)
                              );

            /* File save mode. */
            //if ((int) fileDialogMode >= (int) numberOf_openFileModes)
            //{
            QString saveFileExtension = selectedFilter.mid (selectedFilter.lastIndexOf ('.'));
            saveFileExtension.chop(1);

            result.fileExtension = saveFileExtension;
            RS_DEBUG->print("saveFileExtension=" + saveFileExtension);
            RS_DEBUG->print("result.fileName=" + result.fileName);
            RS_DEBUG->print("result.filePath=" + result.filePath);

            if (!result.fileName.endsWith(saveFileExtension, Qt::CaseInsensitive))
                result.filePath += saveFileExtension;
            if (result.fileName.endsWith(saveFileExtension))
                result.fileName.resize(result.fileName.size() - saveFileExtension.size());
            RS_DEBUG->print("result.filePath=" + result.filePath);

#if !defined (_WIN32) && !defined (__APPLE__)
            /* Confirm if the user wants to overwrite an existing file. */
            if (QFileInfo(result.filePath).exists())
            {
                int replaceFileResponse = QMessageBox::warning(

                            nullptr,
                            fileDialogTitles.at (fileDialogMode),
                            QObject::tr("File '%1' already exists. Do you want to replace it?").arg(result.fileName),
                            QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel, QMessageBox::Cancel
                            );

                if (replaceFileResponse != QMessageBox::Yes) continue;
            }
#endif
            //}

            if (checkBox_combinedSave != nullptr) result.checkState = checkBox_combinedSave->checkState();


            /* Writing to path settings - START */
                RS_SETTINGS->beginGroup("/Paths");
                RS_SETTINGS->writeEntry(fileIOSettingsPaths.at (fileDialogMode), result.dirPath);
                RS_SETTINGS->writeEntry(filterSettingsPaths.at (fileDialogMode), selectedFilter);
                RS_SETTINGS->endGroup();
            /* Writing to path settings - END */


            break;
        }
        else
        {
            result.checkState = -1;

            result.filePath      = "";
            result.dirPath       = "";
            result.fileName      = "";
            result.fileExtension = "";
            result.fileType = RS2::FormatUnknown;

            break;
        }
    }

    delete saveFileDialog;

    RS_DEBUG->print("LC_FileDialogService::getFileName: OK");

    return result;
}

