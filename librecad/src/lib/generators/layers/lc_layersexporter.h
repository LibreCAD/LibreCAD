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
    bool m_createSeparateDocumentPerLayer{false};
    bool m_putEntitiesToOriginalLayer{true};
    bool m_exportUCSs{false};
    bool m_exportNamedViews{false};
    std::list<RS_Layer*> m_layers;
    QString m_sourceDrawingFileName;
};

struct LC_LayerExportData {
    QString m_name;
    RS_Graphic* m_graphic {nullptr};
};

class LC_LayersExporter {
public:
  LC_LayersExporter();
  bool exportLayers(LC_LayersExportOptions* options, RS_Graphic* originalGraphic, std::vector<LC_LayerExportData*> &exportResultList);
protected:
  bool exportLayersToSingleDocument(LC_LayersExportOptions* options, RS_Graphic* originalGraphic,
                                    std::vector<LC_LayerExportData*>& exportResultList);
  bool exportLayersToIndividualDocuments(LC_LayersExportOptions* options, RS_Graphic* rs_graphic,
                                         std::vector<LC_LayerExportData*>& list);
  void copyLayerAttributes(RS_Layer* zeroLayer, RS_Layer* originalLayer);
  void exportUCSList(LC_LayersExportOptions* options, RS_Graphic* originalGraphic, RS_Graphic* graphicToExport);
  void exportViewsList(LC_LayersExportOptions* options, RS_Graphic* originalGraphic, RS_Graphic* graphicToExport);
};

#endif // LC_LAYERSEXPORTER_H
