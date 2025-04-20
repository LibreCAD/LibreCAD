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
#ifndef LC_EXPORTLAYERSDIALOGSERVICE_H
#define LC_EXPORTLAYERSDIALOGSERVICE_H

#include <QObject>
#include "rs.h"

class RS_Graphic;
struct LC_LayersExportOptions;
struct LC_FileDialogResult;

class LC_ExportLayersService: public QObject{
    Q_OBJECT
public:
    static void exportLayers(LC_LayersExportOptions& exportOptions, RS_Graphic* sourceGraphic);

protected:
    LC_ExportLayersService();
    bool selectExportFile(LC_LayersExportOptions& options, LC_FileDialogResult& fileInfo);
    void doExportLayers(LC_LayersExportOptions& exportOptions, RS_Graphic* sourceGraphic);
    QString paddedIndex(int index, int totalNumber);
    bool selectLayersExportFile(LC_LayersExportOptions& options, LC_FileDialogResult& fileInfo);
    QString createExportDocumentFileName(bool separateFileForLayer, LC_FileDialogResult &fileInfo,
                                  size_t exportFilesCount, size_t currentExportLayerIndex,
                                  const QString &exportData);
private:
    std::pair<QString, QString> readDefaultDirAndFilter();
    void saveDefaultDirAndFilter(LC_FileDialogResult& fileInfo, QString selectedFilter);
    RS2::FormatType getFormatType(const QString& formatString);
    void updateFileExtension(LC_FileDialogResult& result, const QString& selectedFilter);
};

#endif // LC_EXPORTLAYERSDIALOGSERVICE_H
