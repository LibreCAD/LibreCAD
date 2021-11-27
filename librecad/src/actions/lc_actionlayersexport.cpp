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


#include <QDir>
#include <QStatusBar>
#include <QFileDialog>
#include <QApplication>

#include "rs_debug.h"
#include "rs_layer.h"
#include "rs_graphic.h"
#include "rs_layerlist.h"
#include "qc_applicationwindow.h"
#include "lc_filedialogservice.h"

#include "lc_actionlayersexport.h"


/*
    This action class exports the current selected layers as a drawing file, 
    either as individual files, or combined within a single file.
*/


LC_ActionLayersExport::LC_ActionLayersExport( RS_EntityContainer& document, 
                                              RS_GraphicView& graphicView, 
                                              RS_LayerList* inputLayersList, 
                                              Mode inputExportMode) 
                                              :
                                              RS_ActionInterface("Export selected layer(s)", document, graphicView), 
                                              layersList(inputLayersList), 
                                              exportMode(inputExportMode)
{
}


void LC_ActionLayersExport::init(int status)
{
    RS_DEBUG->print("LC_ActionLayersExport::init");

    RS_ActionInterface::init(status);

    trigger();
}


void LC_ActionLayersExport::trigger()
{
    RS_DEBUG->print("LC_ActionLayersExport::trigger");

    QString exportModeString = "visible";
    if (exportMode == LC_ActionLayersExport::SelectedMode) exportModeString = "selected";


    RS_LayerList *originalLayersList = document->getLayerList();

    const int totalNumberOfLayers = originalLayersList->count();

    QStringList layersToExport;

    for (int i = 0; i < totalNumberOfLayers; i++)
    {
        if ((   originalLayersList->at(i)->isSelectedInLayerList() && (exportMode == LC_ActionLayersExport::SelectedMode)) 
        ||  ( ! originalLayersList->at(i)->isFrozen()              && (exportMode == LC_ActionLayersExport::VisibleMode)))
        {
            layersToExport.append(originalLayersList->at(i)->getName());
        }
    }

    /* No export layer found. */
    if (layersToExport.isEmpty())
    {
        QC_ApplicationWindow::getAppWindow()->statusBar()->showMessage( QObject::tr("No %1 layers found").arg(exportModeString), 
                                                                        QC_ApplicationWindow::DEFAULT_STATUS_BAR_MESSAGE_TIMEOUT);
        RS_DEBUG->print(RS_Debug::D_NOTICE, "LC_ActionLayersExport::trigger: No %1 layers found", exportModeString);
        finish();
        return;
    }


    LC_FileDialogService::FileDialogResult result;

    if (exportMode == LC_ActionLayersExport::SelectedMode)
    {
        result = LC_FileDialogService::getFileDetails (LC_FileDialogService::ExportLayersSelected);
    }
    else
    {
        result = LC_FileDialogService::getFileDetails (LC_FileDialogService::ExportLayersVisible);
    }

    if (result.filePath.isEmpty())
    {
        RS_DEBUG->print(RS_Debug::D_NOTICE, "LC_ActionLayersExport::trigger: User cancelled");
        finish();
        return;
    }


    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));


    int currentExportLayerIndex = 0;

    while (currentExportLayerIndex < layersToExport.size())
    {
        RS_Graphic *documentDeepCopy = new RS_Graphic();

        documentDeepCopy->newDoc();

        RS_LayerList *duplicateLayersList = documentDeepCopy->getLayerList();

        documentDeepCopy->setVariableDictObject(document->getGraphic()->getVariableDictObject());

        QString modifiedFilePath;

        bool exportLayer0 = false;



        /* Combine all layers. */
        if (result.checkState == Qt::Checked)
        {
            /* This is just to break early out of the loop. */
            currentExportLayerIndex = layersToExport.size();

            modifiedFilePath = result.filePath;

            for (int i = 0; i < layersToExport.size(); i++)
            {
                if (layersToExport.at (i).compare("0") == 0) exportLayer0 = true;

                /* It does a 'new' internally. */
                RS_Layer *duplicateLayer = originalLayersList->find (layersToExport.at (i))->clone();

                duplicateLayersList->add(duplicateLayer);
            }
        }
        /* Individualize all layers. */
        else
        {
            if (layersToExport.at (currentExportLayerIndex).compare("0") == 0) exportLayer0 = true;

            RS_Layer *duplicateLayer = originalLayersList->find (layersToExport.at (currentExportLayerIndex))->clone();

            duplicateLayersList->add(duplicateLayer);

            /* Note that the QString::append() function causes a bug; hence the '+' overload operator. */
            modifiedFilePath = QDir::toNativeSeparators (

                                    result.dirPath + "/" 
                                                   + result.fileName 
                                                   + paddedIndex(currentExportLayerIndex + 1, layersToExport.size()) 
                                                   + result.fileExtension 
                               );
        }



        const int totalNumberOfLayers = duplicateLayersList->count();

        int i = 0;

        while (1)
        {
            RS_Entity *entity = document->entityAt (i++);

            if (entity == nullptr) break;

            QString entityLayerName = entity->getLayer()->getName();

            for (int j = 0; j < totalNumberOfLayers; j++)
            {
                if (entityLayerName.compare(duplicateLayersList->at(j)->getName()) == 0)
                {
                    if ((duplicateLayersList->at(j)->getName().compare("0") == 0) && !exportLayer0) continue;

                    /* It does a 'new' internally. */
                    RS_Entity *duplicateEntity = entity->clone();

                    documentDeepCopy->addEntity(duplicateEntity);

                    duplicateEntity->setLayer(duplicateLayersList->find (entityLayerName));
                }
            }
        }



        /* Saving. */
        documentDeepCopy->setGraphicView(graphicView);

        const bool saveWasSuccessful = documentDeepCopy->saveAs(modifiedFilePath, result.fileType, true);



        /* Cleaning up. */
        for (int j = 0; j < totalNumberOfLayers; j++)
        {
            delete duplicateLayersList->at(j);
        }

        documentDeepCopy->setGraphicView(nullptr);
        documentDeepCopy->setParent(nullptr);
        documentDeepCopy->newDoc();

        delete documentDeepCopy;



        currentExportLayerIndex++;



        if (!saveWasSuccessful)
        {
            RS_DEBUG->print(RS_Debug::D_ERROR, "LC_ActionLayersExport::trigger: Error encountered while exporting layers");

            QApplication::restoreOverrideCursor();

            finish();

            return;
        }
    }



    QApplication::restoreOverrideCursor();

    finish();

    RS_DEBUG->print("LC_ActionLayersExport::trigger: OK");
}


QString LC_ActionLayersExport::paddedIndex(int const& index, int const& totalNumber)
{
    if (totalNumber == 1) return "";

    const int minimumPaddingDigits  = 2;
    const int numberOfindexDigits   = QString::number(index).length();
          int numberOfPaddingDigits = QString::number(totalNumber).length();

    if (numberOfPaddingDigits < minimumPaddingDigits) numberOfPaddingDigits = minimumPaddingDigits;

    return QString("0").repeated(numberOfPaddingDigits - numberOfindexDigits).append(QString::number(index));
}

