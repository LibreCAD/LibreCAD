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

#include <algorithm>
#include <memory>

#include <QApplication>
#include <QDir>
#include <QFileDialog>
#include <QHash>
#include <QStatusBar>

#include "lc_filedialogservice.h"
#include "qc_applicationwindow.h"
#include "rs_debug.h"
#include "rs_dialogfactory.h"
#include "rs_graphic.h"
#include "rs_layer.h"
#include "rs_layerlist.h"

#include "lc_actionlayersexport.h"

namespace {

// Convert export mode (to export layers : Selected or Visible)
LC_FileDialogService::FileDialogMode convertDialogMode(LC_ActionLayersExport::Mode exportMode);

/**
 * @brief format all indices to strings of the same width with padding of '0'
 * for example, for total of 10 and up but less than 100 (exclusive), the formatted indices would be: "01", "02", "03", etc.
 * @param index - a index to format; the indices are 1 based
 * @param totalNumber - total number of indices
 * @return the formatted string
 */
QString paddedIndex(int index, int totalNumber);

// Whether the layer is selected
bool isSelected(RS_Layer* layer);

// Whether the layer is frozen(i.e invisible)
bool isNotFrozen(RS_Layer* layer);

// a RAII style layer list. All layers are owned by the list.
// The layers are automatically deleted at the end of the lifetime of the list
class ScopedLayerList;

/* definitions starts */
LC_FileDialogService::FileDialogMode convertDialogMode(LC_ActionLayersExport::Mode exportMode)
{

using namespace LC_FileDialogService;
return (exportMode == LC_ActionLayersExport::SelectedMode) ? ExportLayersSelected : ExportLayersVisible;
}
bool isSelected(RS_Layer* layer)
{
    return layer != nullptr && layer->isSelectedInLayerList();
}

bool isNotFrozen(RS_Layer* layer)
{
    return layer != nullptr && !layer->isFrozen();
}

class ScopedLayerList
{
public:
    ScopedLayerList(RS_LayerList* list) : m_list{list}{}
    void add(RS_Layer* layer)
    {
        if (layer == nullptr)
            return;

        m_layers.emplace_back(layer);
        m_lookUp[layer->getName()] = layer;
    }

    size_t count() const
    {
        return m_layers.size();
    }

    const std::unique_ptr<RS_Layer>& at(size_t i) const
    {
        return m_layers.at(i);
    }

    RS_Layer* find(QString name) const
    {
        return (m_lookUp.contains(name)) ? m_lookUp[name] : nullptr;
    }

private:
    RS_LayerList* m_list=nullptr;
    std::vector<std::unique_ptr<RS_Layer>> m_layers;
    QHash<QString, RS_Layer*> m_lookUp;

};
}

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

    RS_LayerList *originalLayersList = document->getLayerList();
    // layers to use: by selected or not frozen
    std::vector<RS_Layer*> layersToUse;
    std::copy_if(originalLayersList->begin(), originalLayersList->end(), std::back_inserter(layersToUse),
                 (exportMode == SelectedMode) ? isSelected : isNotFrozen);

    if (layersToUse.empty())
    {
    /* No export layer found. */
        QString exportModeString = (exportMode == SelectedMode) ? "selected" : "visible";
        QC_ApplicationWindow::getAppWindow()->statusBar()->showMessage( QObject::tr("No %1 layers found").arg(exportModeString),
                                                                        QC_ApplicationWindow::DEFAULT_STATUS_BAR_MESSAGE_TIMEOUT);
        RS_DEBUG->print(RS_Debug::D_ERROR, "LC_ActionLayersExport::trigger: No %s layers found", exportModeString.toStdString().c_str());
        finish();
        return;
    }

    // Find layer names
    QStringList layersToExport;
    std::transform(layersToUse.begin(), layersToUse.end(), std::back_inserter(layersToExport), [](RS_Layer* layer) {return layer->getName();});

    // Show file dialog for saving
    LC_FileDialogService::FileDialogResult result = LC_FileDialogService::getFileDetails (convertDialogMode(exportMode));
    if (result.filePath.isEmpty())
    {
        RS_DEBUG->print(RS_Debug::D_NOTICE, "LC_ActionLayersExport::trigger: User cancelled");
        finish();
        return;
    }

    QApplication::setOverrideCursor(Qt::WaitCursor);

    int currentExportLayerIndex = 0;

    while (currentExportLayerIndex < layersToExport.size())
    {
        RS_Graphic documentDeepCopy;

        documentDeepCopy.newDoc();

        ScopedLayerList duplicateLayersList = documentDeepCopy.getLayerList();

        documentDeepCopy.setVariableDictObject(document->getGraphic()->getVariableDictObject());

        QString modifiedFilePath;

        bool exportLayer0 = false;

        /* Combine all layers. */
        if (result.checkState == Qt::Checked)
        {
            /* This is just to break early out of the loop. */
            currentExportLayerIndex = layersToExport.size();

            modifiedFilePath = result.filePath;

            for (QString& layerName: layersToExport)
            {
                if (layerName.compare("0") == 0) exportLayer0 = true;

                /* It does a 'new' internally. */
                RS_Layer *duplicateLayer = originalLayersList->find (layerName)->clone();

                duplicateLayersList.add(duplicateLayer);
            }
            RS_DEBUG->print(QString("line: %1 ").arg(__LINE__)+" modifiedFilePath=" + modifiedFilePath);
        }
        /* Individualize all layers. */
        else
        {
            if (layersToExport.at (currentExportLayerIndex).compare("0") == 0) exportLayer0 = true;

            RS_Layer *duplicateLayer = originalLayersList->find (layersToExport.at (currentExportLayerIndex))->clone();

            duplicateLayersList.add(duplicateLayer);

            /* Note that the QString::append() function causes a bug; hence the '+' overload operator. */
            modifiedFilePath = QDir::toNativeSeparators (
                                    result.dirPath + "/" 
                                                   + result.fileName 
                                                   + paddedIndex(currentExportLayerIndex + 1, layersToExport.size()) 
                                                   + result.fileExtension 
                               );
        }

        const int totalNumberOfLayers = duplicateLayersList.count();

        int i = 0;

        while (1)
        {
            RS_Entity *entity = document->entityAt (i++);

            if (entity == nullptr) break;

            QString entityLayerName = entity->getLayer()->getName();

            for (int j = 0; j < totalNumberOfLayers; j++)
            {
                if (entityLayerName.compare(duplicateLayersList.at(j)->getName()) == 0)
                {
                    if ((duplicateLayersList.at(j)->getName().compare("0") == 0) && !exportLayer0) continue;

                    /* It does a 'new' internally. */
                    RS_Entity *duplicateEntity = entity->clone();

                    documentDeepCopy.addEntity(duplicateEntity);
                    duplicateEntity->reparent(&documentDeepCopy);

                    duplicateEntity->setLayer(duplicateLayersList.find (entityLayerName));
                }
            }
        }

        /* Saving. */
        documentDeepCopy.setGraphicView(graphicView);

        const bool saveWasSuccessful = documentDeepCopy.saveAs(modifiedFilePath, result.fileType, true);
        RS_DIALOGFACTORY->commandMessage(tr(R"(Saving layer "%1" as "%2" )")
                                         .arg(layersToExport.at(currentExportLayerIndex))
                                         .arg(modifiedFilePath));

        documentDeepCopy.setGraphicView(nullptr);
        documentDeepCopy.setParent(nullptr);
        documentDeepCopy.newDoc();

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

namespace {

QString paddedIndex(int index, int totalNumber)
{
    if (totalNumber <= 1 || totalNumber > 10) return "";
    // the maximum string size needed
    int fieldWidth=QString::number(totalNumber).size();
    auto str = QString("%1").arg(index, fieldWidth, 10, QChar{'0'});
    return str;
}
}
