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
#include <QPixmap>
#include <QSaveFile>
#include <QSvgGenerator>

#include "lc_graphicviewport.h"
#include "lc_printviewportrenderer.h"
#include "rs_graphic.h"
#include "rs_painter.h"

namespace {

// Some encoders ignore short writes or failed seeks. Cancel the transaction
// permanently, while retaining real file seeking for formats such as TIFF.
class WriteCheckingFile final : public QSaveFile {
public:
    using QSaveFile::QSaveFile;

    bool seek(const qint64 position) override {
        const bool result = QSaveFile::seek(position);
        if (!result)
            cancelWriting();
        return result;
    }

protected:
    qint64 writeData(const char* data, const qint64 size) override {
        const qint64 written = QSaveFile::writeData(data, size);
        if (written != size)
            cancelWriting();
        return written;
    }
};

} // namespace

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

    if (graphic == nullptr || options.size.isEmpty()) {
        return false;
    }

    WriteCheckingFile file{options.fileName};
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }

    const bool exportToSvg = options.format.toLower() == "svg";
    const bool written = exportToSvg
        ? exportGraphicToSVG(graphic, options, file)
        : exportGraphicToImage(graphic, options, file);
    // Before Qt 6.8.2, commit() ignores a failed flush (QTBUG-132332).
    return written && file.error() == QFileDevice::NoError && file.flush() && file.commit();
}

bool LC_ImageExporter::exportGraphicToSVG(RS_Graphic* graphic, const ExportOptions& options, QFileDevice& output) {
    QSvgGenerator svgGenerator;
    prepareSVGGenerator(options, svgGenerator, &output);
    return renderGraphic(graphic, options, &svgGenerator, true);
}

bool LC_ImageExporter::exportGraphicToImage(RS_Graphic* graphic, const ExportOptions& options, QFileDevice& output) {
    auto pixMap = QPixmap(options.size);
    return !pixMap.isNull() && renderGraphic(graphic, options, &pixMap, false)
        && savePixmapToImage(options, pixMap, output);
}

bool LC_ImageExporter::savePixmapToImage(const ExportOptions& options, const QPixmap &pixMap, QFileDevice& output) {
    QImageWriter iio{&output, options.format.toLatin1()};
    return iio.write(pixMap.toImage());
}

bool LC_ImageExporter::renderGraphic(RS_Graphic* graphic, const ExportOptions& options, QPaintDevice* buffer,
                                     const bool vector) {
    RS_Painter painter(buffer);
    if (!painter.isActive()) {
        return false;
    }
    if (vector) {
        // Vector detail must survive zooming, even when smaller than a pixel.
        painter.setMinCircleDrawingRadius(0.0);
        painter.setMinArcDrawingRadius(0.0);
        painter.setMinEllipseMajorRadius(0.0);
        painter.setMinEllipseMinorRadius(0.0);
        painter.setMinLineDrawingLen(0.0);
    }
    preparePainter(options, painter);

    // fixme - sand - rework to more generic printing (add progress or confirmation ?)
    LC_GraphicViewport viewport;
    prepareViewport(graphic, options, viewport);

    auto renderer = LC_PrintViewportRenderer(&viewport, &painter);
    renderer.loadSettings(); // sets white paper
    renderer.setBackground(options.backgroundBlack ? Qt::black : Qt::white);
    renderer.setDrawingMode(options.blackAndWhite
        ? (options.backgroundBlack ? RS2::ModeWB : RS2::ModeBW) : RS2::ModeAuto);
    renderer.render();

    // QSvgGenerator writes its document when painting ends, not at destruction.
    return painter.end();
}

void LC_ImageExporter::preparePainter(const ExportOptions& options, RS_Painter& painter) {
    const bool black = options.backgroundBlack;
    painter.setBackground(black ? Qt::black : Qt::white);
    painter.eraseRect(0, 0, options.size.width(), options.size.height());
}

void LC_ImageExporter::prepareSVGGenerator(const ExportOptions& options,
                                           QSvgGenerator& svgGenerator, QIODevice* output) {
    svgGenerator.setSize(options.size);
    svgGenerator.setViewBox(QRectF(QPointF(0, 0), options.size));
    svgGenerator.setOutputDevice(output);
}

void LC_ImageExporter::prepareViewport(RS_Graphic* graphic, const ExportOptions& options, LC_GraphicViewport &viewport) {
    viewport.setSize(options.size.width(), options.size.height());
    const auto borderWidth = options.borders.width();
    const auto borderHeight = options.borders.height();
    viewport.setBorders(borderWidth, borderHeight, borderWidth, borderHeight);
    viewport.setDocument(graphic);
    viewport.loadSettings();
    viewport.zoomAuto(false);
}
