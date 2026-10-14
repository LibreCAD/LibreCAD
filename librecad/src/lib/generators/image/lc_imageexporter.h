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

#include <QObject>
#include <QSize>
#include <QString>

class QFileDevice;
class QIODevice;
class QPaintDevice;
class QPixmap;
class QSvgGenerator;
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
        bool backgroundBlack = false;
        bool blackAndWhite = false;
    };
    explicit LC_ImageExporter(QObject* parent = nullptr);
    /**
     * Draws the graphic into options.fileName, as SVG or in a raster format
     * QImageWriter knows. The file is put in place only once the whole image is
     * written, so an export that fails leaves an existing file as it was.
     * @return true if the file was written; otherwise errorString() says why.
     */
    bool exportToImage(RS_Graphic* graphic, const ExportOptions& options);
    QString errorString() const {return m_errorString;}
protected:
    void prepareViewport(RS_Graphic* graphic, const ExportOptions& options, LC_GraphicViewport& viewport);
    void preparePainter(const ExportOptions& options, RS_Painter& painter);
    void prepareSVGGenerator(const ExportOptions& options, QSize size, QSvgGenerator& svgGenerator, QIODevice* output);
    bool exportGraphicToSVG(RS_Graphic* graphic, const ExportOptions& options, QFileDevice& output);
    bool savePixmapToImage(const ExportOptions& options, const QPixmap &pixMap, QFileDevice& output);
    bool exportGraphicToImage(RS_Graphic* graphic, const ExportOptions& options, QFileDevice& output);
    bool renderGraphic(RS_Graphic* graphic, const ExportOptions& options, QPaintDevice* buffer, bool vector);
private:
    QString m_errorString;
};
#endif
