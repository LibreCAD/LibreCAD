/***************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2025 LibreCAD.org
 * Copyright (C) 2025 sand1024
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 * **********************************************************************
 */

#ifndef LC_IMAGEEXPORTER_H
#define LC_IMAGEEXPORTER_H

#include <QSvgGenerator>

class RS_Painter;
class LC_GraphicViewport;
class RS_Graphic;

class LC_ImageExporter : public QObject {
    Q_OBJECT
public:
    struct ExportOptions {
        QString fileName;
        QString format;
        QSize size;
        QSize borders;
        bool backgroundBlack;
        bool blackAndWhite;
    };
    explicit LC_ImageExporter(QObject* parent = nullptr);
    bool exportToImage(RS_Graphic* graphic, const ExportOptions& options);
protected:
    void prepareViewport(RS_Graphic* graphic, const ExportOptions& options, LC_GraphicViewport& viewport);
    void preparePainter(const ExportOptions& options, RS_Painter& painter);
    void prepareSVGGenerator(const ExportOptions& options, QSize size, QSvgGenerator& svgGenerator);
    bool exportGraphicToSVG(RS_Graphic* graphic, const ExportOptions& options);
    bool savePixmapToImage(const ExportOptions& options, const QPixmap &pixMap);
    bool exportGraphicToImage(RS_Graphic* graphic, const ExportOptions& options);
    void renderGraphic(RS_Graphic* graphic, const ExportOptions& options, QPaintDevice* buffer);
};
#endif // LC_IMAGEEXPORTER_H
