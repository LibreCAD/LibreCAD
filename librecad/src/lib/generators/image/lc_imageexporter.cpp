/*
 * **************************************************************************
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
 * *********************************************************************
 */

#include "lc_imageexporter.h"

#include <QImageWriter>
#include <QSvgGenerator>

#include "lc_graphicviewport.h"
#include "lc_printviewportrenderer.h"
#include "rs_graphic.h"
#include "rs_painter.h"

LC_ImageExporter::LC_ImageExporter(QObject* parent)
    : QObject{parent} {
}

// fixme - sand - files - restore - so far this is just a direct port from application window. Refactor later, add more features!
bool LC_ImageExporter::exportToImage(RS_Graphic* graphic, const ExportOptions& options) {
    // fixme - sand - files = export improvement
    // improve:
    // 1) generic refactor
    // 2) support of ucs?
    // 3) export of the entire drawing/visible area?
    // 4) custom background color (i.e transparent)?
    // 5) only visible layers(?)

    bool exportToSvg = options.format.toLower() == "svg";
    if (exportToSvg) {
        return exportGraphicToSVG(graphic, options);
    }
    else {
        return exportGraphicToImage(graphic,options);
    }
}

bool LC_ImageExporter::exportGraphicToSVG(RS_Graphic* graphic, const ExportOptions& options) {
    QSize size = options.size;
    QSvgGenerator svgGenerator;
    prepareSVGGenerator(options, size, svgGenerator);
    QPaintDevice* buffer = &svgGenerator;
    renderGraphic(graphic, options, buffer);
    return true;
}

bool LC_ImageExporter::exportGraphicToImage(RS_Graphic* graphic, const ExportOptions& options) {
    QSize size = options.size;
    auto pixMap = QPixmap(size);
    QPaintDevice* buffer = &pixMap;
    renderGraphic(graphic, options, buffer);
    return savePixmapToImage(options, pixMap);
}

bool LC_ImageExporter::savePixmapToImage(const LC_ImageExporter::ExportOptions& options, const QPixmap &pixMap) {
    QImage img = pixMap.toImage();
    QImageWriter iio;
    iio.setFileName(options.fileName);
    iio.setFormat(options.format.toLatin1());

    bool ret = false;
    if (iio.write(img)) {
        ret = true;
    }
    else {
        //  QString error=iio.errorString();
        // fixme - sand - files - add error reporting
    }
    return ret;
}

void LC_ImageExporter::renderGraphic(RS_Graphic* graphic, const LC_ImageExporter::ExportOptions& options, QPaintDevice* buffer) {
    RS_Painter painter(buffer);
    preparePainter(options, painter);

    // fixme - sand - rework to more generic printing (add progress or confirmation ?)
    LC_GraphicViewport viewport;
    prepareViewport(graphic, options, viewport);

    LC_PrintViewportRenderer renderer = LC_PrintViewportRenderer(&viewport, &painter);
    renderer.setBackground(options.backgroundBlack ? Qt::black : Qt::white);
    renderer.loadSettings();
    renderer.render();
}

void LC_ImageExporter::preparePainter(const ExportOptions& options, RS_Painter& painter) {
    bool black = options.backgroundBlack;
    painter.setBackground(black ? Qt::black : Qt::white);
    if (options.blackAndWhite) {
        painter.setDrawingMode(black ? RS2::ModeWB : RS2::ModeBW);
    }
    painter.eraseRect(0, 0, options.size.width(), options.size.height());
}

void LC_ImageExporter::prepareSVGGenerator(const ExportOptions& options, const QSize size,
                                           QSvgGenerator& svgGenerator) {
    svgGenerator.setSize(size);
    svgGenerator.setViewBox(QRectF(QPointF(0, 0), size));
    svgGenerator.setFileName(options.fileName);
}

void LC_ImageExporter::prepareViewport(RS_Graphic* graphic, const ExportOptions& options, LC_GraphicViewport &viewport) {
    viewport.setSize(options.size.width(), options.size.height());
    auto borderWidth = options.borders.width();
    auto borderHeight = options.borders.height();
    viewport.setBorders(borderWidth, borderHeight, borderWidth, borderHeight);
    viewport.setContainer(graphic);
    viewport.zoomAuto(false);
    viewport.loadSettings();
}
