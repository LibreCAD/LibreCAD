/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2026 LibreCAD.org
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
 * USA.
 * ********************************************************************************
 */

#include <cmath>

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <QByteArray>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QImageWriter>
#include <QTemporaryDir>
#include <QXmlStreamReader>

#include "lc_actiontestsupport.h"
#include "lc_imageexporter.h"
#include "lc_testfilesizelimit.h"
#include "rs_arc.h"
#include "rs_circle.h"
#include "rs_ellipse.h"
#include "rs_graphic.h"
#include "rs_line.h"

namespace {

// A few lines, a circle, an arc and an ellipse, as File > Export as image gets
// them from a new drawing. The drawing is large enough for the default pen of
// layer 0 to come out narrower than a pixel.
void addDrawing(RS_Graphic& graphic, const int extraLines = 0) {
    graphic.initForNewDocument();
    graphic.addEntity(new RS_Line(&graphic, {0, 0}, {1000, 0}));
    graphic.addEntity(new RS_Line(&graphic, {1000, 0}, {1000, 600}));
    graphic.addEntity(new RS_Line(&graphic, {1000, 600}, {0, 600}));
    graphic.addEntity(new RS_Line(&graphic, {0, 600}, {0, 0}));
    graphic.addEntity(new RS_Circle(&graphic, {{250, 300}, 100}));
    graphic.addEntity(new RS_Arc(&graphic, {{500, 300}, 100, 0.0, M_PI, false}));
    graphic.addEntity(new RS_Ellipse(&graphic, {{750, 300}, {150, 0}, 0.5, 0.0, 0.0, false}));
    for (int i = 0; i < extraLines; ++i) {
        graphic.addEntity(new RS_Line(&graphic, {double(i % 1000), 0}, {double((i * 7) % 1000), 600}));
    }
    graphic.calculateBorders();
}

LC_ImageExporter::ExportOptions exportOptions(const QString& fileName, const QString& format) {
    LC_ImageExporter::ExportOptions options;
    options.fileName = fileName;
    // Upper case, as the export dialog hands it on.
    options.format = format.toUpper();
    options.size = QSize(1200, 800);
    options.borders = QSize(5, 5);
    options.backgroundBlack = false;
    options.blackAndWhite = false;
    return options;
}

bool exportDrawing(RS_Graphic& graphic, const QString& fileName, const QString& format) {
    return LC_ImageExporter{}.exportToImage(&graphic, exportOptions(fileName, format));
}

QString writeFile(const QTemporaryDir& dir, const QString& name, const QByteArray& content) {
    const QString path = dir.filePath(name);
    QFile file{path};
    REQUIRE(file.open(QIODevice::WriteOnly));
    REQUIRE(file.write(content) == content.size());
    return path;
}

QByteArray readFile(const QString& path) {
    QFile file(path);
    REQUIRE(file.open(QIODevice::ReadOnly));
    return file.readAll();
}

// The raster formats of the export dialog that this Qt can write. BMP, PNG and
// PPM are built in; the others come from image format plugins. TIFF seeks back
// in the file it writes.
QList<QByteArray> rasterFormats() {
    QList<QByteArray> formats;
    const QList<QByteArray> supported = QImageWriter::supportedImageFormats();
    for (const QByteArray format : {"bmp", "jpg", "png", "ppm", "tif", "webp"}) {
        if (supported.contains(format)) {
            formats.append(format);
        }
    }
    return formats;
}

} // namespace

TEST_CASE("image export writes an SVG document", "[gui][export][svg]") {
    REQUIRE(lc::test::application() != nullptr);
    const QTemporaryDir dir;
    REQUIRE(dir.isValid());
    RS_Graphic graphic;
    addDrawing(graphic);
    const QString out = dir.filePath("drawing.svg");

    REQUIRE(exportDrawing(graphic, out, "svg"));

    QXmlStreamReader reader(readFile(out));
    int svgElements = 0;
    int shapes = 0;
    while (!reader.atEnd()) {
        reader.readNext();
        if (reader.isStartElement()) {
            svgElements += reader.name() == QLatin1String("svg") ? 1 : 0;
            shapes += reader.name() != QLatin1String("svg") && reader.name() != QLatin1String("g") ? 1 : 0;
        }
    }
    CHECK_FALSE(reader.hasError());
    CHECK(svgElements == 1);
    CHECK(shapes > 0);
}

TEST_CASE("image export writes raster images that read back", "[gui][export]") {
    REQUIRE(lc::test::application() != nullptr);
    const QTemporaryDir dir;
    REQUIRE(dir.isValid());
    RS_Graphic graphic;
    addDrawing(graphic);

    const QList<QByteArray> formats = rasterFormats();
    REQUIRE(formats.contains("png"));
    for (const QByteArray& format : formats) {
        CAPTURE(format.toStdString());
        const QString out = dir.filePath("drawing." + QString::fromLatin1(format));
        CHECK(exportDrawing(graphic, out, QString::fromLatin1(format)));
        const QImage image(out, format.constData());
        CHECK(image.size() == QSize(1200, 800));
    }
}

TEST_CASE("image export reports an output it cannot open", "[gui][export]") {
    REQUIRE(lc::test::application() != nullptr);
    const QTemporaryDir dir;
    REQUIRE(dir.isValid());
    RS_Graphic graphic;
    addDrawing(graphic);
    const auto format = GENERATE("svg", "png");
    CAPTURE(format);

    CHECK_FALSE(exportDrawing(graphic, dir.filePath("missing/drawing"), format));
    CHECK_FALSE(QFileInfo::exists(dir.filePath("missing")));
}

#ifdef Q_OS_UNIX
TEST_CASE("image export keeps the existing file after a short write", "[gui][export]") {
    REQUIRE(lc::test::application() != nullptr);
    const QTemporaryDir dir;
    REQUIRE(dir.isValid());
    RS_Graphic graphic;
    addDrawing(graphic);
    const QByteArray previous("previous nonempty export\n");

    QList<QByteArray> formats = rasterFormats();
    formats.prepend("svg");
    for (const QByteArray& format : formats) {
        CAPTURE(format.toStdString());
        const QString out = writeFile(dir, "existing." + QString::fromLatin1(format), previous);
        bool result;
        {
            lc::test::FileSizeLimit limit;
            result = exportDrawing(graphic, out, QString::fromLatin1(format));
        }
        CHECK_FALSE(result);
        CHECK(readFile(out) == previous);
        CHECK(QFileInfo(out).dir().entryList(QDir::Files).size() == 1);
        QFile::remove(out);
    }
}

TEST_CASE("image export keeps the existing file after a short write of a large SVG",
          "[gui][export][svg]") {
    REQUIRE(lc::test::application() != nullptr);
    const QTemporaryDir dir;
    REQUIRE(dir.isValid());
    // Enough lines for an SVG well beyond the 16 KiB a QFile buffers. The SVG
    // generator then hands whole blocks to the file; one that is cut short at
    // the limit leaves no error behind, and the generator writes no more.
    RS_Graphic graphic;
    addDrawing(graphic, 2000);
    const QString complete = dir.filePath("complete.svg");
    REQUIRE(exportDrawing(graphic, complete, "svg"));
    REQUIRE(QFileInfo(complete).size() > 64 * 1024);

    const QByteArray previous("previous nonempty export\n");
    const QString out = writeFile(dir, "existing.svg", previous);
    bool result;
    {
        lc::test::FileSizeLimit limit;
        result = exportDrawing(graphic, out, "svg");
    }
    CHECK_FALSE(result);
    CHECK(readFile(out) == previous);
}
#endif
