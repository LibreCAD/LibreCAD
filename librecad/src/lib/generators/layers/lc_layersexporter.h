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
#ifndef LC_LAYERSEXPORTER_H
#define LC_LAYERSEXPORTER_H

#include <QString>
#include <list>

class RS_Layer;
class RS_Graphic;

struct LC_LayersExportOptions {
    bool createSeparateDocumentPerLayer{false};
    bool putEntitiesToOriginalLayer{true};
    bool exportUcSs{false};
    bool exportNamedViews{false};
    std::list<RS_Layer*> layers;
    QString sourceDrawingFileName;
};

struct LC_LayerExportData {
    QString name;
    RS_Graphic* graphic {nullptr};
};

class LC_LayersExporter {
public:
  LC_LayersExporter();
  void exportLayers(const LC_LayersExportOptions* options, RS_Graphic* originalGraphic, std::vector<LC_LayerExportData> &exportResultList);
protected:
  void exportLayersToSingleDocument(const LC_LayersExportOptions* options, RS_Graphic* originalGraphic,
                                    std::vector<LC_LayerExportData>& exportResultList);
  void exportLayersToIndividualDocuments(const LC_LayersExportOptions* options, RS_Graphic* originalGraphic,
                                         std::vector<LC_LayerExportData>& exportResultList);
  void copyLayerAttributes(RS_Layer* zeroLayer, const RS_Layer* originalLayer);
  void exportUCSList(const LC_LayersExportOptions* options, RS_Graphic* originalGraphic, RS_Graphic* graphicToExport);
  void exportViewsList(const LC_LayersExportOptions* options, RS_Graphic* originalGraphic, RS_Graphic* graphicToExport);
};

#endif
