/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2026 LibreCAD.org
 * Copyright (C) 2026 Dongxu Li (github.com/dxli)
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
#include "rs_painter.h"

namespace {

class RecordingLine : public RS_Line {
public:
    using RS_Line::RS_Line;

    void draw(RS_Painter* painter) override {
        drawn = true;
        color = painter->pen().color();
        mode = painter->getDrawingMode();
        RS_Line::draw(painter);
    }

    bool drawn = false;
    QColor color;
    RS2::DrawingMode mode = RS2::ModeAuto;
};

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
            const auto name = reader.name();
            svgElements += name == QLatin1String("svg") ? 1 : 0;
            shapes += name == QLatin1String("polyline") || name == QLatin1String("circle")
                || name == QLatin1String("path") || name == QLatin1String("ellipse") ? 1 : 0;
        }
    }
    CHECK_FALSE(reader.hasError());
    CHECK(svgElements == 1);
    CHECK(shapes >= 7);
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

TEST_CASE("image export honors monochrome on either background", "[gui][export][color]") {
    REQUIRE(lc::test::application() != nullptr);
    const auto format = GENERATE("svg", "png");
    const bool black = GENERATE(false, true);
    const bool monochrome = GENERATE(false, true);
    CAPTURE(format, black, monochrome);
    const QTemporaryDir dir;
    REQUIRE(dir.isValid());
    RS_Graphic graphic;
    graphic.initForNewDocument();
    auto* line = new RecordingLine(&graphic, RS_Vector(0, 0), RS_Vector(100, 0));
    line->setPen(RS_Pen(RS_Color(Qt::red), RS2::Width00, RS2::SolidLine));
    graphic.addEntity(line);
    auto options = exportOptions(dir.filePath("colors"), format);
    options.backgroundBlack = black;
    options.blackAndWhite = monochrome;

    REQUIRE(LC_ImageExporter{}.exportToImage(&graphic, options));
    REQUIRE(line->drawn);
    CHECK(line->mode == (monochrome ? (black ? RS2::ModeWB : RS2::ModeBW) : RS2::ModeAuto));
    CHECK(line->color == QColor(monochrome ? (black ? Qt::white : Qt::black) : Qt::red));
}

TEST_CASE("image export rejects empty dimensions without replacing a file", "[gui][export][validation]") {
    REQUIRE(lc::test::application() != nullptr);
    const auto format = GENERATE("svg", "png");
    const auto size = GENERATE(QSize(), QSize(0, 800), QSize(1200, 0), QSize(-1, 800));
    CAPTURE(format, size.width(), size.height());
    const QTemporaryDir dir;
    REQUIRE(dir.isValid());
    RS_Graphic graphic;
    addDrawing(graphic);
    const QByteArray previous("previous export\n");
    const QString out = writeFile(dir, "existing", previous);
    auto options = exportOptions(out, format);
    options.size = size;
    LC_ImageExporter exporter;

    CHECK_FALSE(exporter.exportToImage(&graphic, options));
    CHECK_FALSE(exporter.errorString().isEmpty());
    CHECK(readFile(out) == previous);
    CHECK(QDir(dir.path()).entryList(QDir::Files | QDir::Hidden).size() == 1);
}

TEST_CASE("image export reports failures and clears its error on success", "[gui][export][validation]") {
    REQUIRE(lc::test::application() != nullptr);
    const QTemporaryDir dir;
    REQUIRE(dir.isValid());
    RS_Graphic graphic;
    addDrawing(graphic);
    const QByteArray previous("previous export\n");
    const QString out = writeFile(dir, "existing", previous);
    LC_ImageExporter exporter;
    auto options = exportOptions(out, "not-an-image-format");

    CHECK_FALSE(exporter.exportToImage(nullptr, options));
    CHECK_FALSE(exporter.errorString().isEmpty());
    CHECK_FALSE(exporter.exportToImage(&graphic, options));
    CHECK_FALSE(exporter.errorString().isEmpty());
    CHECK(readFile(out) == previous);
    CHECK(QDir(dir.path()).entryList(QDir::Files | QDir::Hidden).size() == 1);

    options.format = "PNG";
    REQUIRE(exporter.exportToImage(&graphic, options));
    CHECK(exporter.errorString().isEmpty());
    CHECK(QImage(out).size() == options.size);
}

#ifdef Q_OS_UNIX
TEST_CASE("image export rolls back a short write", "[gui][export]") {
    REQUIRE(lc::test::application() != nullptr);
    const bool existing = GENERATE(false, true);
    CAPTURE(existing);
    const QTemporaryDir dir;
    REQUIRE(dir.isValid());
    RS_Graphic graphic;
    addDrawing(graphic);
    const QByteArray previous("previous nonempty export\n");

    QList<QByteArray> formats = rasterFormats();
    formats.prepend("svg");
    for (const QByteArray& format : formats) {
        CAPTURE(format.toStdString());
        const QString out = dir.filePath("drawing." + QString::fromLatin1(format));
        if (existing) {
            writeFile(dir, QFileInfo(out).fileName(), previous);
        }
        LC_ImageExporter exporter;
        bool result;
        {
            lc::test::FileSizeLimit limit;
            result = exporter.exportToImage(&graphic, exportOptions(out, QString::fromLatin1(format)));
        }
        CHECK_FALSE(result);
        CHECK_FALSE(exporter.errorString().isEmpty());
        CHECK(QFileInfo::exists(out) == existing);
        if (existing) {
            CHECK(readFile(out) == previous);
        }
        CHECK(QFileInfo(out).dir().entryList(QDir::Files | QDir::Hidden).size() == (existing ? 1 : 0));
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
