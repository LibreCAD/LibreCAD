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

#include <QBuffer>
#include <QImageWriter>
#include <QPixmap>
#include <QSaveFile>
#include <QSvgGenerator>

#include "lc_graphicviewport.h"
#include "lc_printviewportrenderer.h"
#include "rs_graphic.h"
#include "rs_painter.h"

namespace {

// Passes what is written on to another device and remembers a write that the
// device did not take whole. QSvgGenerator writes through a text stream that
// stops at the first short write and tells nobody.
class WriteCheckingDevice : public QIODevice {
public:
    explicit WriteCheckingDevice(QIODevice& target) : m_target(target) {
        open(QIODevice::WriteOnly);
    }
    bool failed() const { return m_failed; }

protected:
    qint64 readData(char*, qint64) override { return -1; }
    qint64 writeData(const char* data, const qint64 size) override {
        const qint64 written = m_target.write(data, size);
        if (written != size)
            m_failed = true;
        return written;
    }

private:
    QIODevice& m_target;
    bool m_failed = false;
};

// A write the file takes only in part may leave no error behind.
QString writeError(const QFileDevice& file) {
    return file.error() == QFileDevice::NoError
        ? LC_ImageExporter::tr("The file could not be written completely.")
        : file.errorString();
}

QString drawError(const QSize& size) {
    return LC_ImageExporter::tr("Could not draw an image of %1 x %2 pixels.").arg(size.width()).arg(size.height());
}

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

    m_errorString.clear();
    if (graphic == nullptr) {
        m_errorString = tr("There is no drawing to export.");
        return false;
    }

    // The image is written to a temporary file, which takes the place of the
    // named one only once all of it is written: a failed export leaves an
    // existing file as it was.
    QSaveFile file{options.fileName};
    if (!file.open(QIODevice::WriteOnly)) {
        m_errorString = file.errorString();
        return false;
    }

    const bool exportToSvg = options.format.toLower() == "svg";
    const bool written = exportToSvg
        ? exportGraphicToSVG(graphic, options, file)
        : exportGraphicToImage(graphic, options, file);
    if (!written) {
        return false;
    }

    // commit() flushes what is still buffered, and before Qt 6.8.2 it puts the
    // file in place even when that flush could not write everything
    // (QTBUG-132332). Flush here, where the result can be seen.
    if (file.error() != QFileDevice::NoError || !file.flush() || !file.commit()) {
        m_errorString = writeError(file);
        return false;
    }
    return true;
}

bool LC_ImageExporter::exportGraphicToSVG(RS_Graphic* graphic, const ExportOptions& options, QFileDevice& output) {
    WriteCheckingDevice checkedOutput{output};
    QSvgGenerator svgGenerator;
    prepareSVGGenerator(options, options.size, svgGenerator, &checkedOutput);
    const bool rendered = renderGraphic(graphic, options, &svgGenerator, true);
    if (checkedOutput.failed()) {
        m_errorString = writeError(output);
        return false;
    }
    return rendered;
}

bool LC_ImageExporter::exportGraphicToImage(RS_Graphic* graphic, const ExportOptions& options, QFileDevice& output) {
    auto pixMap = QPixmap(options.size);
    if (pixMap.isNull()) {
        m_errorString = drawError(options.size);
        return false;
    }
    return renderGraphic(graphic, options, &pixMap, false) && savePixmapToImage(options, pixMap, output);
}

bool LC_ImageExporter::savePixmapToImage(const ExportOptions& options, const QPixmap &pixMap, QFileDevice& output) {
    // Some writers seek back in what they write (TIFF does, and on a device
    // that cannot seek it writes a file nobody can read), and not all of them
    // check that each write is taken whole. Encode in memory, then write the
    // result in one piece that is checked.
    QBuffer encoded;
    encoded.open(QIODevice::WriteOnly);
    QImageWriter iio{&encoded, options.format.toLatin1()};
    if (!iio.write(pixMap.toImage())) {
        m_errorString = iio.errorString();
        return false;
    }
    if (output.write(encoded.data()) != encoded.size()) {
        m_errorString = writeError(output);
        return false;
    }
    return true;
}

bool LC_ImageExporter::renderGraphic(RS_Graphic* graphic, const ExportOptions& options, QPaintDevice* buffer,
                                     const bool vector) {
    RS_Painter painter(buffer);
    if (!painter.isActive()) {
        m_errorString = drawError(options.size);
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
    renderer.render();

    // QSvgGenerator writes its document when painting ends, not at destruction.
    if (!painter.end()) {
        m_errorString = drawError(options.size);
        return false;
    }
    return true;
}

void LC_ImageExporter::preparePainter(const ExportOptions& options, RS_Painter& painter) {
    const bool black = options.backgroundBlack;
    painter.setBackground(black ? Qt::black : Qt::white);
    if (options.blackAndWhite) {
        painter.setDrawingMode(black ? RS2::ModeWB : RS2::ModeBW);
    }
    painter.eraseRect(0, 0, options.size.width(), options.size.height());
}

void LC_ImageExporter::prepareSVGGenerator(const ExportOptions& options, const QSize size,
                                           QSvgGenerator& svgGenerator, QIODevice* output) {
    svgGenerator.setSize(size);
    svgGenerator.setViewBox(QRectF(QPointF(0, 0), size));
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
