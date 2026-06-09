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

#include "lc_entity_type_propertiesprovider.h"
#include "rs_block.h"
#include "rs_graphic.h"
#include "rs_insert.h"
#include "rs_layer.h"

LC_LayersExporter::LC_LayersExporter() {
}

void LC_LayersExporter::exportLayers(const LC_LayersExportOptions* options, RS_Graphic* originalGraphic,
                                     std::vector<LC_LayerExportData>& exportResultList) {
    if (options->createSeparateDocumentPerLayer) {
        exportLayersToIndividualDocuments(options, originalGraphic, exportResultList);
    }
    else {
        exportLayersToSingleDocument(options, originalGraphic, exportResultList);
    }
}

void LC_LayersExporter::exportLayersToIndividualDocuments(const LC_LayersExportOptions* options, RS_Graphic* originalGraphic,
                                                          std::vector<LC_LayerExportData>& exportResultList) {

    struct LayerExportInfo {
        RS_Layer* exportLayer {nullptr};
        RS_Graphic* exportGraphic {nullptr};
        std::unordered_set<RS_Block*> originalBlocks;
    };

    std::unordered_map<RS_Layer*, LayerExportInfo*> exportDataMap;

    const auto originalGraphicView = originalGraphic->getGraphicView();

    for (auto originalLayer : options->layers) {
        auto* exportGraphics = new RS_Graphic();
        exportGraphics->initForNewDocument();
        exportGraphics->setVariableDictObject(originalGraphic->getVariableDictObject());
        exportGraphics->setGraphicView(originalGraphicView);  // fixme - sand - dependency

        auto* exportInfo = new LayerExportInfo();

        exportInfo->exportGraphic = exportGraphics;
        const auto zeroLayer = exportInfo->exportGraphic->findLayer("0");

        if (options->putEntitiesToOriginalLayer) {
            if (originalLayer->getName() == "0") {
                exportInfo->exportLayer = zeroLayer;
                copyLayerAttributes(zeroLayer, originalLayer);
            }
            else{
                const auto exportLayer = originalLayer->clone();
                exportInfo->exportLayer = exportLayer;
                exportGraphics->addLayer(exportLayer);
            }
        }
        else {
            exportInfo->exportLayer = zeroLayer;
            copyLayerAttributes(zeroLayer, originalLayer);
        }

        exportDataMap.emplace(originalLayer,exportInfo);
        exportUCSList(options, originalGraphic, exportGraphics);
        exportViewsList(options, originalGraphic, exportGraphics);

        auto exportData = LC_LayerExportData();
        exportData.graphic = exportGraphics;
        exportData.name = originalLayer->getName();
        exportResultList.push_back(exportData);
    }

    bool hasBlocks = false;
    for (RS_Entity* originalEntity : *originalGraphic) {
        if (originalEntity == nullptr || originalEntity->getLayer() == nullptr || originalEntity->isDeleted()) {
            continue;
        }

        RS_Layer* originalLayer = originalEntity->getLayer();
        const auto exportInfo = exportDataMap[originalLayer];
        if (exportInfo != nullptr) {
            RS_Layer* exportLayer = exportInfo->exportLayer;
            const auto exportEntity = originalEntity->clone();
            const auto exportGraphic = exportInfo->exportGraphic;
            exportEntity->reparent(exportGraphic);
            exportEntity->setLayer(exportLayer);
            exportGraphic->addEntity(exportEntity);

            if (originalEntity->rtti() == RS2::EntityInsert) {
                const auto* originalInsert = static_cast<RS_Insert*>(originalEntity);
                RS_Block* originalBlock = originalInsert->getBlockForInsert();
                exportInfo->originalBlocks.insert(originalBlock);
                hasBlocks = true;
            }
        }
    }

    if (hasBlocks) {
        for (const auto [fst, snd]: exportDataMap) {
            const auto exportInfo = snd;
            if (!exportInfo->originalBlocks.empty()) {
                const auto exportGraphic = exportInfo->exportGraphic;
                for (const auto originalBlock : exportInfo->originalBlocks) {
                    auto* exportBlock = static_cast<RS_Block*>(originalBlock->clone());
                    exportGraphic->addBlock(exportBlock, false);
                }
            }
        }
    }

    for (const auto [fst, snd]: exportDataMap) {
        const auto exportInfo = snd;
        exportInfo->originalBlocks.clear();
        delete exportInfo;
    }

    exportDataMap.clear();
}

void LC_LayersExporter::copyLayerAttributes(RS_Layer* zeroLayer, const RS_Layer* originalLayer) {
    zeroLayer->setPen(originalLayer->getPen());
    zeroLayer->setConstruction(originalLayer->isConstruction());
    zeroLayer->setPrint(originalLayer->isPrint());
}

void LC_LayersExporter::exportLayersToSingleDocument(const LC_LayersExportOptions* options, RS_Graphic* originalGraphic,
                                                     std::vector<LC_LayerExportData>& exportResultList) {
    std::unordered_map<RS_Layer*, RS_Layer*> layerMap;

    auto* exportGraphic = new RS_Graphic();
    exportGraphic->initForNewDocument();
    exportGraphic->setVariableDictObject(originalGraphic->getVariableDictObject());
    auto zeroLayer = exportGraphic->findLayer("0"); // it's always present, created in newDoc

    for (auto originalLayer : options->layers) {
        if (originalLayer->getName() == "0") {
            layerMap.emplace(originalLayer, zeroLayer);
            copyLayerAttributes(zeroLayer, originalLayer);
        }
        else {
            RS_Layer* exportLayer = originalLayer->clone();
            layerMap.emplace(originalLayer, exportLayer);
            exportGraphic->addLayer(exportLayer);
        }
    }

    exportUCSList(options, originalGraphic, exportGraphic);
    exportViewsList(options, originalGraphic, exportGraphic);

    // fixme - sand - dependency
    exportGraphic->setGraphicView(originalGraphic->getGraphicView());

    std::unordered_set<RS_Block*> usedBlocksSet;

    for (RS_Entity* originalEntity : *originalGraphic) {
        if (originalEntity == nullptr || originalEntity->getLayer() == nullptr || originalEntity->isDeleted()) {
            continue;
        }

        RS_Layer* originalLayer = originalEntity->getLayer();
        if (auto pair = layerMap.find(originalLayer); pair != layerMap.end()) {
            const auto duplicateLayer = pair->second;
            RS_Entity* exportEntity = originalEntity->clone();
            exportEntity->reparent(exportGraphic);
            exportEntity->setLayer(duplicateLayer);
            exportGraphic->addEntity(exportEntity);

            if (originalEntity->rtti() == RS2::EntityInsert) {
                const auto* originalInsert = static_cast<RS_Insert*>(originalEntity);
                RS_Block* originalBlock = originalInsert->getBlockForInsert();
                usedBlocksSet.insert(originalBlock);
            }
        }
    }

    if (!usedBlocksSet.empty()) {
        for (const auto originalBlock : usedBlocksSet) {
            auto* exportBlock = static_cast<RS_Block*>(originalBlock->clone());
            exportGraphic->addBlock(exportBlock, false);
        }
    }

    usedBlocksSet.clear();

    auto exportData = LC_LayerExportData();
    exportData.graphic = exportGraphic;
    exportData.name.clear();

    exportResultList.push_back(exportData);
}

void LC_LayersExporter::exportUCSList(const LC_LayersExportOptions* options, RS_Graphic* originalGraphic, RS_Graphic* graphicToExport) {
    if (options->exportUcSs) {
        const auto ucslist = originalGraphic->getUCSList();
        const int ucsCount = ucslist->count();
        for (int i = 0; i < ucsCount; i++) {
            const auto ucs = ucslist->at(i);
            const auto clone = ucs->clone();
            graphicToExport->addUCS(clone);
        }
    }
}

void LC_LayersExporter::exportViewsList(const LC_LayersExportOptions* options, RS_Graphic* originalGraphic, RS_Graphic* graphicToExport) {
    if (options->exportNamedViews) {
        const auto viewsList = originalGraphic->getViewList();
        const int viewsCount = viewsList->count();
        for (int i =0; i < viewsCount; i++) {
            const auto view = viewsList->at(i);
            const auto viewClone = view->clone();
            graphicToExport->addNamedView(viewClone);
        }
    }
}
