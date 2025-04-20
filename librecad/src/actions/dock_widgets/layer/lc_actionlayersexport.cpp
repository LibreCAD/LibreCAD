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


#include "lc_actionlayersexport.h"

#include <QStatusBar>

#include "lc_actioncontext.h"
#include "lc_exportlayersdialogservice.h"
#include "lc_layersexporter.h"
#include "qc_applicationwindow.h"
#include "rs_debug.h"
#include "rs_document.h"
#include "rs_layer.h"
#include "rs_layerlist.h"

namespace
{
    bool isSelected(RS_Layer* layer) {
        return layer != nullptr && layer->isSelectedInLayerList();
    }

    bool isNotFrozen(RS_Layer* layer) {
        return layer != nullptr && !layer->isFrozen();
    }

}
/*
    This action class exports the current selected layers as a drawing file, 
    either as individual files, or combined within a single file.
*/
LC_ActionLayersExport::LC_ActionLayersExport(LC_ActionContext *actionContext, Mode inputExportMode)
    : RS_ActionInterface("Export selected layer(s)",actionContext, inputExportMode == SelectedMode ? RS2::ActionLayersExportSelected:RS2::ActionLayersExportVisible),
    m_exportMode{inputExportMode}{
    auto container = actionContext->getEntityContainer() ->getDocument();
    m_layersList = container->getLayerList();
}

void LC_ActionLayersExport::init(int status){
    RS_DEBUG->print("LC_ActionLayersExport::init");
    RS_ActionInterface::init(status);
    trigger();
}

bool LC_ActionLayersExport::collectLayersToExport(LC_LayersExportOptions* exportOptions) {
    RS_LayerList *originalLayersList = m_document->getLayerList();
    // layers to use: by selected or not frozen
    std::copy_if(originalLayersList->begin(), originalLayersList->end(), std::back_inserter(exportOptions->m_layers),
                 (m_exportMode == SelectedMode) ? isSelected : isNotFrozen);

    if (exportOptions->m_layers.empty())    {
        /* No export layer found. */
        QString exportModeString = (m_exportMode == SelectedMode) ? tr("selected", "Layers to export"): tr("visible", "Layers to export");
        // fixme - sand - files - use more generic way for message notify!
        QC_ApplicationWindow::getAppWindow()->statusBar()->showMessage( QObject::tr("No %1 layers found").arg(exportModeString),
                                                                        QC_ApplicationWindow::DEFAULT_STATUS_BAR_MESSAGE_TIMEOUT);
        RS_DEBUG->print(RS_Debug::D_ERROR, "LC_ActionLayersExport::trigger: No %s layers found", exportModeString.toStdString().c_str());
        return false;
    }
    return true;
}

void LC_ActionLayersExport::performExport() {
    LC_LayersExportOptions exportOptions;
    if (collectLayersToExport(&exportOptions)) {
        auto sourceGraphic = m_document->getGraphic();
        LC_ExportLayersService::exportLayers(exportOptions, sourceGraphic);
    }
}

void LC_ActionLayersExport::trigger(){
    RS_DEBUG->print("LC_ActionLayersExport::trigger");
    performExport();
    finish();
    RS_DEBUG->print("LC_ActionLayersExport::trigger: OK");
}
