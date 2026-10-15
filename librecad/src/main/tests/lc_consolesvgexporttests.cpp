// File: lc_consolesvgexporttests.cpp

/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2026 raffa <raffa00025@gmail.com>
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

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <QByteArray>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QImageWriter>
#include <QMap>
#include <QPainter>
#include <QSize>
#include <QSvgRenderer>
#include <QTemporaryDir>
#include <QXmlStreamReader>

#ifdef Q_OS_UNIX
#include <csignal>
#include <sys/resource.h>
#endif

#include "console_command_utils.h"
#include "console_dxf2png.h"
#include "lc_actiontestsupport.h"
#include "lc_imageexporter.h"
#include "lc_plot_settings.h"
#include "rs_debug.h"
#include "rs_fontlist.h"
#include "rs_graphic.h"
#include "rs_line.h"
#include "rs_settings.h"
#include "rs_system.h"

namespace {

// Makes the font sources of the source tree available, the same way the
// text tests do: the export computes the text geometry from these fonts.
class SourceFonts {
public:
    SourceFonts() {
        RS_DEBUG->setLevel(RS_Debug::D_NOTHING);
        static bool systemReady = [] {
            RS_SYSTEM->init("LibreCAD", "tests", "librecad", "librecad_tests");
            return true;
        }();
        (void)systemReady;

        LC_GROUP_GUARD("Paths");
        m_previous = LC_GET_STR("Fonts", "");
        LC_SET("Fonts", QStringLiteral(LIBRECAD_SOURCE_DIR "/librecad/support/fonts"));
        RS_FONTLIST->clearFonts();
        RS_FONTLIST->init();
    }

    ~SourceFonts() {
        LC_GROUP_GUARD("Paths");
        LC_SET("Fonts", m_previous);
        RS_FONTLIST->clearFonts();
        RS_FONTLIST->init();
        RS_DEBUG->setLevel(m_previousDebugLevel);
    }

private:
    QString m_previous;
    RS_Debug::RS_DebugLevel m_previousDebugLevel = RS_DEBUG->getLevel();
};

// A common frame keeps the viewport scale consistent across fixtures.
constexpr const char* kFrameEntities =
    "0\nLINE\n8\n0\n10\n-10.0\n20\n-10.0\n30\n0.0\n11\n210.0\n21\n-10.0\n31\n0.0\n"
    "0\nLINE\n8\n0\n10\n210.0\n20\n-10.0\n30\n0.0\n11\n210.0\n21\n110.0\n31\n0.0\n"
    "0\nLINE\n8\n0\n10\n210.0\n20\n110.0\n30\n0.0\n11\n-10.0\n21\n110.0\n31\n0.0\n"
    "0\nLINE\n8\n0\n10\n-10.0\n20\n110.0\n30\n0.0\n11\n-10.0\n21\n-10.0\n31\n0.0\n";
constexpr const char* kTextEntity =
    "0\nTEXT\n8\n0\n10\n10.0\n20\n20.0\n30\n0.0\n40\n10.0\n1\nDRAWING NUMBER 4711\n";
constexpr const char* kMTextEntity =
    "0\nMTEXT\n8\n0\n10\n10.0\n20\n50.0\n30\n0.0\n40\n10.0\n1\nSHEET TITLE 12345\n";
constexpr const char* kSmallEntities =
    "0\nCIRCLE\n8\n0\n10\n50\n20\n50\n40\n0.05\n"
    "0\nARC\n8\n0\n10\n80\n20\n50\n40\n0.02\n50\n0\n51\n90\n"
    "0\nELLIPSE\n8\n0\n10\n100\n20\n50\n11\n1\n21\n0\n40\n0.01\n41\n0\n42\n6.283185307179586\n"
    "0\nLINE\n8\n0\n370\n5\n10\n120\n20\n50\n11\n120.05\n21\n50\n";

QString writeFile(const QTemporaryDir& dir, const QString& name, const QByteArray& content) {
    const QString path = dir.filePath(name);
    QFile file{path};
    REQUIRE(file.open(QIODevice::WriteOnly));
    REQUIRE(file.write(content) == content.size());
    return path;
}

// The same preparation the console converters apply before exporting.
void prepareForExport(RS_Graphic& graphic) {
    graphic.calculateBorders();
    LC_PlotSettings* ps = graphic.getPlotSettings();
    ps->setMarginsInMm(-1.0, -1.0, -1.0, -1.0);
    ps->setPagesNum(0, 0);
    graphic.fitToPage();
}

void importDrawing(const QTemporaryDir& dir, RS_Graphic& graphic, const QByteArray& entities) {
    const QByteArray dxf = QByteArray("0\nSECTION\n2\nENTITIES\n") + kFrameEntities + entities +
                           "0\nENDSEC\n0\nEOF\n";
    REQUIRE(LC_Console::importGraphic(graphic, writeFile(dir, "drawing.dxf", dxf)));
    prepareForExport(graphic);
}

QByteArray readFile(const QString& path) {
    QFile file(path);
    REQUIRE(file.open(QIODevice::ReadOnly));
    return file.readAll();
}

QMap<QString, int> svgElementCounts(const QByteArray& svg) {
    QXmlStreamReader reader(svg);
    QMap<QString, int> counts;
    while (!reader.atEnd()) {
        reader.readNext();
        if (reader.isStartElement())
            ++counts[reader.name().toString()];
    }
    REQUIRE_FALSE(reader.hasError());
    REQUIRE(counts.value("svg") == 1);
    return counts;
}

QList<QByteArray> rasterFormats() {
    QList<QByteArray> formats;
    const auto supported = QImageWriter::supportedImageFormats();
    for (const QByteArray format : {"bmp", "jpg", "png", "ppm", "tif", "webp"}) {
        if (supported.contains(format))
            formats.append(format);
    }
    return formats;
}

void addColoredLine(RS_Graphic& graphic) {
    graphic.initForNewDocument();
    auto* line = new RS_Line(&graphic, RS_Vector(0, 0), RS_Vector(100, 100));
    line->setPen(RS_Pen(RS_Color(Qt::red), RS2::Width07, RS2::SolidLine));
    graphic.addEntity(line);
}

#ifdef Q_OS_UNIX
// Restore process-wide state before Catch2 reports any failed assertions.
class FileSizeLimit {
public:
    FileSizeLimit() {
        REQUIRE(getrlimit(RLIMIT_FSIZE, &m_previous) == 0);
        if (m_previous.rlim_max < 512)
            SKIP("The process file-size limit is already below the test limit");
        m_previousSignal = std::signal(SIGXFSZ, SIG_IGN);
        REQUIRE(m_previousSignal != SIG_ERR);
        rlimit limited = m_previous;
        limited.rlim_cur = 512;
        if (setrlimit(RLIMIT_FSIZE, &limited) != 0) {
            std::signal(SIGXFSZ, m_previousSignal);
            FAIL("Could not set the test file-size limit");
        }
    }
    ~FileSizeLimit() {
        setrlimit(RLIMIT_FSIZE, &m_previous);
        std::signal(SIGXFSZ, m_previousSignal);
    }
    FileSizeLimit(const FileSizeLimit&) = delete;
    FileSizeLimit& operator=(const FileSizeLimit&) = delete;

private:
    rlimit m_previous{};
    void (*m_previousSignal)(int) = SIG_DFL;
};
#endif

} // namespace

TEST_CASE("console SVG export reports success and writes the file", "[console][svg]") {
    REQUIRE(lc::test::application() != nullptr);
    SourceFonts fonts;
    const QTemporaryDir dir;
    REQUIRE(dir.isValid());

    RS_Graphic graphic;
    importDrawing(dir, graphic, QByteArray(kTextEntity) + kMTextEntity);
    REQUIRE(graphic.count() == 6);
    const QString out = dir.filePath("text.svg");

    CHECK(slotFileExport(&graphic, out, "SVG", QSize(1200, 800), QSize(5, 5),
                          false, false));
    CHECK(QFileInfo(out).isFile());
    CHECK(QFileInfo(out).size() > 0);
}

TEST_CASE("console SVG export draws text", "[console][svg]") {
    REQUIRE(lc::test::application() != nullptr);
    SourceFonts fonts;
    const QTemporaryDir dir;
    REQUIRE(dir.isValid());

    RS_Graphic graphic;
    const auto entity = GENERATE(kTextEntity, kMTextEntity);
    importDrawing(dir, graphic, entity);
    REQUIRE(graphic.count() == 5);
    const QString out = dir.filePath("text.svg");

    CHECK(slotFileExport(&graphic, out, "SVG", QSize(1200, 800), QSize(5, 5),
                          false, false));

    // Neither the frame nor the background produces a path.
    CHECK(svgElementCounts(readFile(out)).value("path") > 0);
}

TEST_CASE("console SVG export retains subpixel geometry", "[console][svg]") {
    REQUIRE(lc::test::application() != nullptr);
    SourceFonts fonts;
    const QTemporaryDir dir;
    REQUIRE(dir.isValid());
    RS_Graphic graphic;
    importDrawing(dir, graphic, kSmallEntities);
    REQUIRE(graphic.count() == 8);
    const QString out = dir.filePath("small.svg");
    REQUIRE(slotFileExport(&graphic, out, "SVG", QSize(1200, 800), QSize(5, 5), false, false));
    const auto svg = readFile(out);
    const auto counts = svgElementCounts(svg);
    CHECK(counts.value("circle") == 1);
    // Qt may encode the ellipse as a path, depending on its arc parameters.
    CHECK(counts.value("ellipse") + counts.value("path") == 2);
    CHECK(counts.value("polyline") == 5);
    QXmlStreamReader reader(svg);
    bool fractionalStroke = false;
    while (!reader.atEnd()) {
        reader.readNext();
        const double width = reader.attributes().value("stroke-width").toDouble();
        fractionalStroke = fractionalStroke || (width > 0.0 && width < 1.0);
    }
    CHECK(fractionalStroke);
}

TEST_CASE("console image export rejects an invalid output path", "[console][svg]") {
    REQUIRE(lc::test::application() != nullptr);
    const QTemporaryDir dir;
    REQUIRE(dir.isValid());
    RS_Graphic graphic;
    const auto format = GENERATE("SVG", "PNG");
    CHECK_FALSE(slotFileExport(&graphic, dir.filePath("missing/output"), format,
                              QSize(1200, 800), QSize(5, 5), false, false));
    CHECK(QApplication::overrideCursor() == nullptr);
}

TEST_CASE("console raster exports read back at the requested size", "[console][png]") {
    REQUIRE(lc::test::application() != nullptr);
    SourceFonts fonts;
    const QTemporaryDir dir;
    REQUIRE(dir.isValid());
    RS_Graphic graphic;
    importDrawing(dir, graphic, kTextEntity);
    const auto formats = rasterFormats();
    REQUIRE(formats.contains("png"));
    for (const auto& format : formats) {
        CAPTURE(format.toStdString());
        const QString out = dir.filePath("drawing." + QString::fromLatin1(format));
        REQUIRE(slotFileExport(&graphic, out, QString::fromLatin1(format),
                               QSize(1200, 800), QSize(5, 5), false, false));
        CHECK(QImage(out).size() == QSize(1200, 800));
    }
    CHECK(readFile(dir.filePath("drawing.png")).startsWith(QByteArray::fromHex("89504e470d0a1a0a")));
}

TEST_CASE("image exporter rejects invalid requests without replacing output", "[console][export]") {
    REQUIRE(lc::test::application() != nullptr);
    const auto format = GENERATE("SVG", "PNG");
    CAPTURE(format);
    const QTemporaryDir dir;
    REQUIRE(dir.isValid());
    RS_Graphic graphic;
    addColoredLine(graphic);
    const QByteArray previous("previous export\n");
    const QString out = writeFile(dir, "existing", previous);
    LC_ImageExporter exporter;
    LC_ImageExporter::ExportOptions options{out, format, {200, 200}, {10, 10}, false, false};
    CHECK_FALSE(exporter.exportToImage(nullptr, options));
    for (const QSize size : {QSize(), QSize(0, 200), QSize(200, 0), QSize(-1, 200)}) {
        CAPTURE(size.width(), size.height());
        options.size = size;
        CHECK_FALSE(exporter.exportToImage(&graphic, options));
        CHECK(readFile(out) == previous);
    }
    options.size = {200, 200};
    options.format = "not-an-image-format";
    CHECK_FALSE(exporter.exportToImage(&graphic, options));
    CHECK(readFile(out) == previous);
    options.format = format;
    options.fileName = dir.filePath("missing/output");
    CHECK_FALSE(exporter.exportToImage(&graphic, options));
    CHECK(QDir(dir.path()).entryList(QDir::Files | QDir::Hidden) == QStringList{"existing"});
}

TEST_CASE("image exporter honors monochrome on either background", "[console][export]") {
    REQUIRE(lc::test::application() != nullptr);
    const auto format = GENERATE("SVG", "PNG");
    const bool black = GENERATE(false, true);
    const bool monochrome = GENERATE(false, true);
    CAPTURE(format, black, monochrome);
    const QTemporaryDir dir;
    REQUIRE(dir.isValid());
    RS_Graphic graphic;
    addColoredLine(graphic);
    LC_ImageExporter::ExportOptions options{dir.filePath("colors"), format, {200, 200}, {10, 10}, black, monochrome};
    REQUIRE(LC_ImageExporter{}.exportToImage(&graphic, options));
    QImage image(options.size, QImage::Format_RGB32);
    image.fill(Qt::transparent);
    if (QString::fromLatin1(format) == "SVG") {
        QSvgRenderer svg(options.fileName);
        REQUIRE(svg.isValid());
        QPainter painter(&image);
        svg.render(&painter);
    } else {
        REQUIRE(image.load(options.fileName));
    }
    REQUIRE(image.size() == options.size);
    const QRgb background = QColor(black ? Qt::black : Qt::white).rgb();
    CHECK(image.pixel(0, 0) == background);
    int ink = 0, colored = 0;
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            const QRgb pixel = image.pixel(x, y);
            ink += pixel != background;
            colored += qRed(pixel) != qGreen(pixel) || qGreen(pixel) != qBlue(pixel);
        }
    }
    CHECK(ink > 0);
    CHECK((colored > 0) == !monochrome);
}

#ifdef Q_OS_UNIX
TEST_CASE("console image export rolls back a short write", "[console][svg]") {
    REQUIRE(lc::test::application() != nullptr);
    SourceFonts fonts;
    const QTemporaryDir dir;
    REQUIRE(dir.isValid());
    RS_Graphic graphic;
    importDrawing(dir, graphic, kSmallEntities);
    const bool existing = GENERATE(false, true);
    CAPTURE(existing);
    const QByteArray previous("previous nonempty export\n");
    auto formats = rasterFormats();
    formats.prepend("svg");
    for (const auto& format : formats) {
        CAPTURE(format.toStdString());
        const QString out = dir.filePath("existing");
        if (existing)
            writeFile(dir, "existing", previous);
        const auto before = QDir(dir.path()).entryList(QDir::Files | QDir::Hidden);
        bool result;
        {
            FileSizeLimit limit;
            result = slotFileExport(&graphic, out, QString::fromLatin1(format), QSize(1200, 800), QSize(5, 5), false, false);
        }
        CHECK_FALSE(result);
        CHECK(QFileInfo::exists(out) == existing);
        if (existing)
            CHECK(readFile(out) == previous);
        CHECK(QDir(dir.path()).entryList(QDir::Files | QDir::Hidden) == before);
        QFile::remove(out);
    }
    CHECK(QApplication::overrideCursor() == nullptr);
}

TEST_CASE("console SVG export preserves existing output after a short write of a large file",
          "[console][svg]") {
    REQUIRE(lc::test::application() != nullptr);
    SourceFonts fonts;
    const QTemporaryDir dir;
    REQUIRE(dir.isValid());
    // Enough lines for an SVG well beyond the 16 KiB a QFile buffers. The SVG
    // generator then hands whole blocks to the file; one that is cut short at
    // the limit leaves no error behind, and the generator writes no more.
    QByteArray lines;
    for (int i = 0; i < 2000; ++i) {
        lines += "0\nLINE\n8\n0\n10\n" + QByteArray::number(i % 200) + "\n20\n0.0\n11\n" +
                 QByteArray::number((i * 7) % 200) + "\n21\n100.0\n";
    }
    RS_Graphic graphic;
    importDrawing(dir, graphic, lines);
    const QString complete = dir.filePath("complete.svg");
    REQUIRE(slotFileExport(&graphic, complete, "SVG", QSize(1200, 800), QSize(5, 5), false, false));
    REQUIRE(QFileInfo(complete).size() > 64 * 1024);

    const QByteArray previous("previous nonempty export\n");
    const QString out = writeFile(dir, "existing", previous);
    bool result;
    {
        FileSizeLimit limit;
        result = slotFileExport(&graphic, out, "SVG", QSize(1200, 800), QSize(5, 5), false, false);
    }
    CHECK_FALSE(result);
    CHECK(readFile(out) == previous);
    CHECK(QApplication::overrideCursor() == nullptr);
}
#endif
