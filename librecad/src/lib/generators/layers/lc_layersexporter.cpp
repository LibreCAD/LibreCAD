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
#include "lc_layersexporter.h"

#include <unordered_set>

#include "rs_block.h"
#include "rs_graphic.h"
#include "rs_insert.h"
#include "rs_layer.h"

LC_LayersExporter::LC_LayersExporter() {
}

bool LC_LayersExporter::exportLayers(LC_LayersExportOptions* options, RS_Graphic* originalGraphic,
                                     std::vector<LC_LayerExportData*>& exportResultList) {
    if (options->m_createSeparateDocumentPerLayer) {
        return exportLayersToIndividualDocuments(options, originalGraphic, exportResultList);
    }
    else {
        return exportLayersToSingleDocument(options, originalGraphic, exportResultList);
    }
}

bool LC_LayersExporter::exportLayersToIndividualDocuments(LC_LayersExportOptions* options, RS_Graphic* originalGraphic, std::vector<LC_LayerExportData*>& exportResultList) {

    struct LayerExportInfo {
        RS_Layer* exportLayer {nullptr};
        RS_Graphic* exportGraphic {nullptr};
        std::unordered_set<RS_Block*> originalBlocks;
    };

    std::unordered_map<RS_Layer*, LayerExportInfo*> exportDataMap;

    auto originalGraphicView = originalGraphic->getGraphicView();

    for (auto originalLayer : options->m_layers) {
        auto* exportGraphics = new RS_Graphic();
        exportGraphics->newDoc();
        exportGraphics->setVariableDictObject(originalGraphic->getVariableDictObject());
        exportGraphics->setGraphicView(originalGraphicView);  // fixme - sand - dependency

        auto* exportInfo = new LayerExportInfo();

        exportInfo->exportGraphic = exportGraphics;
        auto zeroLayer = exportInfo->exportGraphic->findLayer("0");

        if (options->m_putEntitiesToOriginalLayer) {
            if (originalLayer->getName() == "0") {
                exportInfo->exportLayer = zeroLayer;
                copyLayerAttributes(zeroLayer, originalLayer);
            }
            else{
                auto exportLayer = originalLayer->clone();
                exportInfo->exportLayer = exportLayer;
                exportGraphics->addLayer(exportLayer);
            }
        }
        else {
            exportInfo->exportLayer = zeroLayer;
            copyLayerAttributes(zeroLayer, originalLayer);
        }

        exportDataMap.insert({originalLayer,exportInfo});
        exportUCSList(options, originalGraphic, exportGraphics);
        exportViewsList(options, originalGraphic, exportGraphics);

        auto* exportData = new LC_LayerExportData();
        exportData->m_graphic = exportGraphics;
        exportData->m_name = originalLayer->getName();
        exportResultList.push_back(exportData);
    }

    bool hasBlocks = false;
    for (RS_Entity* originalEntity : *originalGraphic) {
        if (originalEntity == nullptr || originalEntity->getLayer() == nullptr || originalEntity->isUndone()) {
            continue;
        }

        RS_Layer* originalLayer = originalEntity->getLayer();
        auto exportInfo = exportDataMap[originalLayer];
        if (exportInfo != nullptr) {
            RS_Layer* exportLayer = exportInfo->exportLayer;
            auto exportEntity = originalEntity->clone();
            auto exportGraphic = exportInfo->exportGraphic;
            exportEntity->reparent(exportGraphic);
            exportEntity->setLayer(exportLayer);
            exportGraphic->addEntity(exportEntity);

            if (originalEntity->rtti() == RS2::EntityInsert) {
                auto* originalInsert = dynamic_cast<RS_Insert*>(originalEntity);
                RS_Block* originalBlock = originalInsert->getBlockForInsert();
                exportInfo->originalBlocks.insert(originalBlock);
                hasBlocks = true;
            }
        }
    }

    if (hasBlocks) {
        for (auto pair: exportDataMap) {
            auto exportInfo = pair.second;
            if (!exportInfo->originalBlocks.empty()) {
                auto exportGraphic = exportInfo->exportGraphic;
                for (auto originalBlock : exportInfo->originalBlocks) {
                    auto* exportBlock = dynamic_cast<RS_Block*>(originalBlock->clone());
                    exportGraphic->addBlock(exportBlock, false);
                }
            }
        }
    }

    for (auto pair: exportDataMap) {
        auto exportInfo = pair.second;
        exportInfo->originalBlocks.clear();
        delete exportInfo;
    }

    exportDataMap.clear();
    return true;
}

void LC_LayersExporter::copyLayerAttributes(RS_Layer* zeroLayer, RS_Layer* originalLayer) {
    zeroLayer->setPen(originalLayer->getPen());
    zeroLayer->setConstruction(originalLayer->isConstruction());
    zeroLayer->setPrint(originalLayer->isPrint());
}

bool LC_LayersExporter::exportLayersToSingleDocument(LC_LayersExportOptions* options, RS_Graphic* originalGraphic,
                                                     std::vector<LC_LayerExportData*>& exportResultList) {
    std::unordered_map<RS_Layer*, RS_Layer*, std::hash<RS_Layer*>> layerMap;

    auto* exportGraphic = new RS_Graphic();
    exportGraphic->newDoc();
    exportGraphic->setVariableDictObject(originalGraphic->getVariableDictObject());
    auto zeroLayer = exportGraphic->findLayer("0"); // it's always present, created in newDoc

    for (auto originalLayer : options->m_layers) {
        if (originalLayer->getName() == "0") {
            layerMap.insert({originalLayer, zeroLayer});
            copyLayerAttributes(zeroLayer, originalLayer);
        }
        else {
            RS_Layer* exportLayer = originalLayer->clone();
            layerMap.insert({originalLayer, exportLayer});
            exportGraphic->addLayer(exportLayer);
        }
    }

    exportUCSList(options, originalGraphic, exportGraphic);
    exportViewsList(options, originalGraphic, exportGraphic);

    // fixme - sand - dependency
    exportGraphic->setGraphicView(originalGraphic->getGraphicView());

    std::unordered_set<RS_Block*> usedBlocksSet;

    for (RS_Entity* originalEntity : *originalGraphic) {
        if (originalEntity == nullptr || originalEntity->getLayer() == nullptr || originalEntity->isUndone()) {
            continue;
        }

        RS_Layer* originalLayer = originalEntity->getLayer();
        if (auto pair = layerMap.find(originalLayer); pair != layerMap.end()) {
            auto duplicateLayer = pair->second;
            RS_Entity* exportEntity = originalEntity->clone();
            exportEntity->reparent(exportGraphic);
            exportEntity->setLayer(duplicateLayer);
            exportGraphic->addEntity(exportEntity);

            if (originalEntity->rtti() == RS2::EntityInsert) {
                auto* originalInsert = dynamic_cast<RS_Insert*>(originalEntity);
                RS_Block* originalBlock = originalInsert->getBlockForInsert();
                usedBlocksSet.insert(originalBlock);
            }
        }
    }

    if (!usedBlocksSet.empty()) {
        for (auto originalBlock : usedBlocksSet) {
            auto* exportBlock = dynamic_cast<RS_Block*>(originalBlock->clone());
            exportGraphic->addBlock(exportBlock, false);
        }
    }

    usedBlocksSet.clear();

    auto* exportData = new LC_LayerExportData();
    exportData->m_graphic = exportGraphic;
    exportData->m_name = "";

    exportResultList.push_back(exportData);
    return true;
}

void LC_LayersExporter::exportUCSList(LC_LayersExportOptions* options, RS_Graphic* originalGraphic, RS_Graphic* graphicToExport) {
    if (options->m_exportUCSs) {
        const auto ucslist = originalGraphic->getUCSList();
        const int ucsCount = ucslist->count();
        for (int i = 0; i < ucsCount; i++) {
            const auto ucs = ucslist->at(i);
            const auto clone = ucs->clone();
            graphicToExport->addUCS(clone);
        }
    }
}

void LC_LayersExporter::exportViewsList(LC_LayersExportOptions* options, RS_Graphic* originalGraphic, RS_Graphic* graphicToExport) {
    if (options->m_exportNamedViews) {
        const auto viewsList = originalGraphic->getViewList();
        const int viewsCount = viewsList->count();
        for (int i =0; i < viewsCount; i++) {
            const auto view = viewsList->at(i);
            const auto viewClone = view->clone();
            graphicToExport->addNamedView(viewClone);
        }
    }
}
