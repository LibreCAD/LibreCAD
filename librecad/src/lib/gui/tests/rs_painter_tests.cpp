/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2026 LibreCAD.org
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
**********************************************************************/

// Regression tests for RS_Painter::setPen(): which QPen it installs for a given
// RS_Pen, and when it may keep the pen it already holds instead.

#include <cmath>
#include <filesystem>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <QApplication>
#include <QBuffer>
#include <QImage>
#include <QPdfWriter>
#include <QPixmap>

#include "lc_graphicviewport.h"
#include "lc_imageexporter.h"
#include "lc_printviewportrenderer.h"
#include "rs_color.h"
#include "rs_graphic.h"
#include "rs_line.h"
#include "rs_linetypepattern.h"
#include "rs_painter.h"
#include "rs_pen.h"
#include "rs_settings.h"

namespace {

QApplication& application() {
    static int argc = 1;
    static char name[] = "librecad-tests";
    static char* argv[] = {name, nullptr};
    // Reuse the process-wide QApplication if another test file built it first,
    // and leak the pointer on the create path: only one QApplication may exist
    // at a time, and only one ~QApplication may run at exit. Test files are
    // linked into one executable, so this helper is per translation unit;
    // rs_graphicview_close_tests.cpp carries the same one.
    static QApplication* app = [] {
        auto* existing = qobject_cast<QApplication*>(QCoreApplication::instance());
        return existing != nullptr ? existing : new QApplication(argc, argv);
    }();
    static bool settingsReady = [] {
        QCoreApplication::setOrganizationName("LibreCAD");
        QCoreApplication::setApplicationName("LibreCAD-tests");
        RS_Settings::init("LibreCAD", "LibreCAD-tests");
        return true;
    }();
    (void)settingsReady;
    return *app;
}

/**
 * A painter on a plain image. RS_Painter::setPen() reads only the drawing mode,
 * the device resolution and the painter's current join/cap styles, so no
 * viewport and no renderer are needed; rs_hatch_tests.cpp builds its test
 * painter on a QImage the same way.
 */
struct PenTestPainter {
    QImage image{64, 64, QImage::Format_RGB32};
    RS_Painter painter{&image};
};

RS_Pen testPen(const RS2::LineType lineType, const double screenWidth = 1.0) {
    RS_Pen pen{RS_Color(Qt::black), RS2::Width00, lineType};
    pen.setScreenWidth(screenWidth);
    return pen;
}

} // namespace

TEST_CASE("RS_Painter paints solid for a line type with no pattern",
          "[gui][painter][linetype]") {
    (void)application();

    // RS2::LineTypeUnchanged is the one enumerator RS_LineTypePattern::getPattern()
    // has no table entry for; the out-of-range value stands for a stale settings
    // integer, which is how the grid, snap and overlay pens reach the painter.
    for (const RS2::LineType lineType : {RS2::LineTypeUnchanged, static_cast<RS2::LineType>(999)}) {
        PenTestPainter rig;
        rig.painter.setPen(testPen(lineType));

        CHECK(rig.painter.pen().style() == Qt::SolidLine);
        CHECK(rig.painter.pen().dashPattern().isEmpty());
    }
}

TEST_CASE("RS_Painter answers a repeated request with the same pen",
          "[gui][painter]") {
    (void)application();

    for (const RS2::LineType lineType : {RS2::SolidLine, RS2::DashLine}) {
        PenTestPainter rig;
        RS_Painter& painter = rig.painter;

        const RS_Pen requested = testPen(lineType, 2.0);
        painter.setPen(requested);
        const QPen first = painter.pen();
        painter.setPen(requested);
        CHECK(painter.pen() == first);

        // Join and cap come from the painter, and a change reaches QPainter on the
        // next request even when the RS_Pen is the same.
        painter.setPenCapStyle(Qt::FlatCap);
        painter.setPenJoinStyle(Qt::BevelJoin);
        painter.setPen(requested);
        CHECK(painter.pen().capStyle() == Qt::FlatCap);
        CHECK(painter.pen().joinStyle() == Qt::BevelJoin);
        CHECK(painter.pen().style() == first.style());
        CHECK(painter.pen().dashPattern() == first.dashPattern());
    }
}

TEST_CASE("RS_Painter installs a pen the painter no longer holds", "[gui][painter]") {
    (void)application();

    // A pen set on the QPainter base replaces the painter's pen without
    // RS_Painter knowing; the next identical request must still install.
    const QPen marker{QColor(Qt::green)};

    for (const RS2::LineType lineType : {RS2::SolidLine, RS2::DashLine}) {
        PenTestPainter rig;
        RS_Painter& painter = rig.painter;
        QPainter& asQPainter = rig.painter;

        const RS_Pen requested = testPen(lineType);
        painter.setPen(requested);
        const QPen installed = painter.pen();
        asQPainter.setPen(marker);
        painter.setPen(requested);
        CHECK(painter.pen() == installed);
    }
}

TEST_CASE("RS_Painter installs a pen that QPainter::restore() put back",
          "[gui][painter]") {
    (void)application();

    // RS_Painter overrides neither save() nor restore(); LC_OverlayInfoCursor
    // sets colour pens inside such a pair.
    PenTestPainter rig;
    RS_Painter& painter = rig.painter;

    painter.setPen(testPen(RS2::SolidLine, 2.0));
    painter.save();
    painter.setPen(RS_Color(Qt::red));
    painter.restore();
    REQUIRE(painter.pen().color() == QColor(Qt::black));
    REQUIRE(painter.pen().widthF() == 2.0);

    RS_Pen red = testPen(RS2::SolidLine);
    red.setColor(RS_Color(Qt::red));
    painter.setPen(red);
    CHECK(painter.pen().color() == QColor(Qt::red));
    CHECK(painter.pen().widthF() == 1.0);
    CHECK(painter.pen().style() == Qt::SolidLine);
}

TEST_CASE("RS_Painter builds dashed pens from line type, width and offset",
          "[gui][painter]") {
    (void)application();

    PenTestPainter rig;
    RS_Painter& painter = rig.painter;

    const RS_Pen solid = testPen(RS2::SolidLine);
    painter.setPen(solid);
    CHECK(painter.pen().style() == Qt::SolidLine);

    const RS_Pen dashed = testPen(RS2::DashLine);
    painter.setPen(dashed);
    const QPen blackDashed = painter.pen();
    CHECK(blackDashed.style() == Qt::CustomDashLine);
    CHECK(blackDashed.color() == QColor(Qt::black));
    CHECK(blackDashed.widthF() == 1.0);
    CHECK_FALSE(blackDashed.dashPattern().isEmpty());
    CHECK(blackDashed.dashOffset() == 0.0);
    CHECK(blackDashed.capStyle() == Qt::RoundCap);
    CHECK(blackDashed.joinStyle() == Qt::RoundJoin);

    RS_Pen red = dashed;
    red.setColor(RS_Color(Qt::red));
    painter.setPen(red);
    CHECK(painter.pen().color() == QColor(Qt::red));
    CHECK(painter.pen().dashPattern() == blackDashed.dashPattern());

    // The pattern is keyed on the line type, not on the expanded pattern.
    painter.setPen(testPen(RS2::DotLine));
    CHECK(painter.pen().style() == Qt::CustomDashLine);
    CHECK_FALSE(painter.pen().dashPattern().isEmpty());
    CHECK(painter.pen().dashPattern() != blackDashed.dashPattern());
    painter.setPen(dashed);
    CHECK(painter.pen().dashPattern() == blackDashed.dashPattern());

    // The offset is scaled to device units, so compare a ratio.
    RS_Pen phaseOne = dashed;
    phaseOne.setDashOffset(-1.0);
    painter.setPen(phaseOne);
    const double offsetOne = painter.pen().dashOffset();
    RS_Pen phaseThree = dashed;
    phaseThree.setDashOffset(-3.0);
    painter.setPen(phaseThree);
    const double offsetThree = painter.pen().dashOffset();
    CHECK(offsetOne != 0.0);
    CHECK(std::abs(offsetThree - 3.0 * offsetOne) <= 1e-9 * std::abs(offsetOne));
    CHECK(painter.pen().dashPattern() == blackDashed.dashPattern());

    painter.setPen(testPen(RS2::DashLine, 2.0));
    CHECK(painter.pen().widthF() == 2.0);
    CHECK(painter.pen().style() == Qt::CustomDashLine);
    CHECK_FALSE(painter.pen().dashPattern().isEmpty());

    painter.setPen(solid);
    CHECK(painter.pen().style() == Qt::SolidLine);
    CHECK(painter.pen().dashPattern().isEmpty());
    painter.setPen(dashed);
    CHECK(painter.pen().dashPattern() == blackDashed.dashPattern());
    CHECK(painter.pen().dashOffset() == 0.0);
}

TEST_CASE("dash lengths and phase retain drawing-unit scale across zoom and lineweights",
          "[gui][painter][linetype]") {
    (void)application();
    PenTestPainter rig;
    LC_GraphicViewport viewport;
    viewport.setSize(640, 480);
    for (const auto lineType : {RS2::DashLine, RS2::DotLine, RS2::DashDotLine}) {
        const auto& pattern = RS_LineTypePattern::getPattern(lineType)->pattern;
        for (const double zoom : {0.5, 4.0, 1.0, 4.0}) {
            viewport.zoomIn(zoom / viewport.getFactor().x);
            rig.painter.setViewPort(&viewport);
            for (const double width : {0.0, 0.5, 1.0, 4.0}) {
                RS_Pen pen = testPen(lineType, width);
                pen.setDashOffset(-3.0);
                rig.painter.setPen(pen);
                const QPen actual = rig.painter.pen();
                const double unit = (width > 0.0 ? width : 1.0) / zoom;
                REQUIRE(actual.dashPattern().size() == pattern.size());
                for (qsizetype i = 0; i < pattern.size(); ++i)
                    CHECK(actual.dashPattern()[i] * unit == Catch::Approx(std::abs(pattern[i])));
                CHECK(actual.dashOffset() * unit == Catch::Approx(-3.0));
            }
        }
    }
}

TEST_CASE("viewport dash scale is independent of image DPI", "[gui][painter][linetype]") {
    (void)application();
    LC_GraphicViewport viewport;
    viewport.setSize(640, 480);
    viewport.zoomIn(2.0);
    for (const int dpmm : {4, 24}) {
        QImage image(640, 480, QImage::Format_RGB32);
        image.setDotsPerMeterX(dpmm * 1000);
        image.setDotsPerMeterY(dpmm * 1000);
        RS_Painter painter(&image);
        painter.setViewPort(&viewport);
        painter.setPen(testPen(RS2::DashLine, 0.0));
        CHECK(painter.pen().dashPattern() == QVector<qreal>{24.0, 12.0});
    }
}

TEST_CASE("zooming preserves the rendered number of dashes", "[gui][painter][linetype]") {
    (void)application();
    const int dpr = GENERATE(1, 2);
    for (const double zoom : {0.25, 1.0, 2.0, 4.0}) {
        QImage image(640, 100, QImage::Format_RGB32);
        image.setDevicePixelRatio(dpr);
        image.fill(Qt::white);
        LC_GraphicViewport viewport;
        viewport.setSize(image.width() / dpr, image.height() / dpr);
        viewport.zoomIn(zoom);
        RS_Painter painter(&image);
        painter.setViewPort(&viewport);
        painter.setPenCapStyle(Qt::FlatCap);
        painter.setPen(testPen(RS2::DashLine, 0.0));
        painter.drawLine(QPointF(10, 20), QPointF(10 + 51 * zoom, 20));
        REQUIRE(painter.end());
        int dashes = 0;
        bool previous = false;
        for (int x = 0; x < image.width(); ++x) {
            const bool dark = qGray(image.pixel(x, 20 * dpr)) < 128;
            dashes += dark && !previous;
            previous = dark;
        }
        CHECK(dashes == 3);
    }
}

TEST_CASE("PDF dash lengths retain drawing-unit scale at different resolutions",
          "[gui][painter][print][linetype]") {
    (void)application();
    for (const int dpi : {72, 600}) {
        QBuffer output;
        REQUIRE(output.open(QIODevice::WriteOnly));
        QPdfWriter writer(&output);
        writer.setResolution(dpi);
        LC_GraphicViewport viewport;
        viewport.setSize(writer.width(), writer.height());
        const double scale = dpi / 72.0;
        viewport.zoomIn(scale);
        RS_Painter painter(&writer);
        REQUIRE(painter.isActive());
        painter.setViewPort(&viewport);
        painter.setPen(testPen(RS2::DashLine, 0.5));
        CHECK(painter.pen().dashPattern().front() * 0.5 / scale == Catch::Approx(12.0));
        painter.drawLine(QPointF(0, 10), QPointF(100 * scale, 10));
        REQUIRE(painter.end());
        CHECK(output.data().startsWith("%PDF-"));
    }
}

TEST_CASE("screen-space dash pens remain independent of viewport zoom", "[gui][painter][linetype]") {
    (void)application();
    PenTestPainter rig;
    LC_GraphicViewport viewport;
    viewport.setSize(640, 480);
    const RS_Pen pen = testPen(RS2::DashLineTiny, 0.0);
    QVector<qreal> screenPattern;
    for (const double zoom : {1.0, 200.0, 0.5}) {
        viewport.setOffsetAndFactor(0, 0, zoom);
        rig.painter.setViewPort(&viewport);
        rig.painter.setPen(pen, RS_Painter::DashSpace::Screen);
        if (screenPattern.isEmpty())
            screenPattern = rig.painter.pen().dashPattern();
        CHECK(rig.painter.pen().dashPattern() == screenPattern);
        rig.painter.setPen(pen);
        CHECK(rig.painter.pen().dashPattern().front() == Catch::Approx(2.0 * zoom));
        rig.painter.setPen(pen, RS_Painter::DashSpace::Screen);
        CHECK(rig.painter.pen().dashPattern() == screenPattern);
    }
}

TEST_CASE("large raster dashes retain length phase and lineweight", "[gui][painter][linetype]") {
    (void)application();
    const int dpr = GENERATE(1, 2);
    const bool antialias = GENERATE(false, true);
    const double zoom = GENERATE(100.0, 200.0);
    QImage image(8000, 80, QImage::Format_RGB32);
    image.setDevicePixelRatio(dpr);
    LC_GraphicViewport viewport;
    viewport.setSize(image.width() / dpr, image.height() / dpr);
    viewport.setOffsetAndFactor(10, viewport.getHeight() - 20, zoom);
    RS_Painter painter(&image);
    painter.setViewPort(&viewport);
    painter.setPenCapStyle(Qt::FlatCap);
    painter.setRenderHint(QPainter::Antialiasing, antialias);
    RS_Line line(RS_Vector(0.0, 0.0), RS_Vector(36.0, 0.0));
    // Reuse the painter across fallback, normal, and fallback-again pens.
    for (const double width : {0.0, 1.0, 2.0, 0.0}) {
        for (const double offset : {0.0, 3.0}) {
            image.fill(Qt::white);
            RS_Pen pen = testPen(RS2::DashLine, width);
            pen.setDashOffset(offset);
            painter.setPen(pen);
            const QPen installed = painter.pen();
            painter.setPen(pen);
            CHECK(painter.pen() == installed);
            line.draw(&painter);
            const int start = 10 * dpr;
            const int row = 20 * dpr;
            int end = start;
            while (end < image.width() && (qGray(image.pixel(end, row)) < 250
                    || qGray(image.pixel(end, row - 1)) < 250))
                ++end;
            CHECK(end - start == Catch::Approx((12.0 - offset) * zoom * dpr).margin(1.0));
            const int gap = start + static_cast<int>((15.0 - offset) * zoom * dpr);
            CHECK(qGray(image.pixel(gap, row)) == 255);
            double coverage = 0.0;
            for (int y = 0; y < image.height(); ++y)
                coverage += (255 - qGray(image.pixel(start + 2 * dpr, y))) / 255.0;
            CHECK(coverage == Catch::Approx(width == 0.0 ? 1.0 : width * dpr).margin(0.02));
        }
    }
}

TEST_CASE("RS_Painter takes a dashed pen on a device it could not begin on",
          "[gui][painter][linetype]") {
    (void)application();
    // A view squashed to no height gives its painter a pixmap without pixels.
    // The painter is not active then and has no paint engine to ask for its type.
    QPixmap empty;
    RS_Painter painter(&empty);
    REQUIRE_FALSE(painter.isActive());
    // Without a viewport first, then with the squashed view's.
    LC_GraphicViewport viewport;
    viewport.setSize(10, 0);
    for (const bool withViewport : {false, true}) {
        if (withViewport)
            painter.setViewPort(&viewport);
        for (const double width : {0.0, 1.0, 2.0}) {
            painter.setPen(testPen(RS2::DashLine, width));
            CHECK(painter.getPen().getLineType() == RS2::DashLine);
        }
    }
}

TEST_CASE("RS_Painter re-installs a pen that noCapStyle() replaced", "[gui][painter]") {
    (void)application();

    // LC_GridSystem sets a grid pen, then noCapStyle(). The next request, even for
    // the same pen, gets the painter's configured cap back.
    for (const RS2::LineType lineType : {RS2::SolidLine, RS2::DashLine}) {
        PenTestPainter rig;
        RS_Painter& painter = rig.painter;

        const RS_Pen requested = testPen(lineType);
        painter.setPen(requested);
        painter.noCapStyle();
        REQUIRE(painter.pen().capStyle() == Qt::FlatCap);

        painter.setPen(requested);
        CHECK(painter.pen().capStyle() == Qt::RoundCap);
        CHECK(painter.pen().style() == (lineType == RS2::SolidLine ? Qt::SolidLine : Qt::CustomDashLine));
    }
}

TEST_CASE("RS_Painter re-installs a pen that setPen(RS_Color) replaced", "[gui][painter]") {
    (void)application();

    // setPen(RS_Color) installs QPen(QColor): width 1 with Qt's SquareCap and
    // BevelJoin. A request for that colour at width 1 must still get the painter's
    // configured cap and join.
    PenTestPainter rig;
    RS_Painter& painter = rig.painter;

    painter.setPen(testPen(RS2::SolidLine));
    painter.setPen(RS_Color(Qt::red));
    REQUIRE(painter.pen().capStyle() == Qt::SquareCap);
    REQUIRE(painter.pen().joinStyle() == Qt::BevelJoin);

    RS_Pen red = testPen(RS2::SolidLine);
    red.setColor(RS_Color(Qt::red));
    painter.setPen(red);
    CHECK(painter.pen().color() == QColor(Qt::red));
    CHECK(painter.pen().widthF() == 1.0);
    CHECK(painter.pen().capStyle() == Qt::RoundCap);
    CHECK(painter.pen().joinStyle() == Qt::RoundJoin);
}

// Black and white entities print in contrast to the paper. A Qt::white passed as an
// RS_Color became flags on an invalid black colour, so dxf2png took its white paper
// for black and drew them white.
namespace {

// a black line and a white line
void addBlackAndWhiteLines(RS_Graphic& graphic) {
    for (const int y : {0, 100}) {
        auto* line = new RS_Line(&graphic, RS_Vector(0, y), RS_Vector(100, y));
        const int shade = y == 0 ? 0 : 255;
        line->setPen(RS_Pen(RS_Color(shade, shade, shade), RS2::Width00, RS2::SolidLine));
        graphic.addEntity(line);
    }
}

int countPixels(const QImage& image, const QRgb rgb) {
    int n = 0;
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            n += (image.pixel(x, y) & 0xffffff) == (rgb & 0xffffff) ? 1 : 0;
        }
    }
    return n;
}

} // namespace

TEST_CASE("RS_Color from a Qt::GlobalColor is that colour", "[gui][color]") {
    for (const Qt::GlobalColor qt : {Qt::black, Qt::white, Qt::red, Qt::gray}) {
        const RS_Color color = [](const RS_Color& c) { return c; }(qt);
        CHECK(color.isValid());
        CHECK(color.rgba() == QColor(qt).rgba());
        CHECK(color.getFlags() == 0);
    }
}

TEST_CASE("dxf2png draws black and white entities in black on white paper", "[gui][print]") {
    (void)application();
    RS_Graphic graphic;
    graphic.initForNewDocument();
    addBlackAndWhiteLines(graphic);

    // as console_dxf2png.cpp renders
    QPixmap picture(200, 200);
    RS_Painter painter(&picture);
    painter.setBackground(Qt::white);
    painter.eraseRect(0, 0, 200, 200);
    LC_GraphicViewport viewport;
    viewport.setSize(200, 200);
    viewport.setBorders(10, 10, 10, 10);
    viewport.setDocument(&graphic);
    viewport.loadSettings();
    viewport.zoomAuto(false);
    LC_PrintViewportRenderer renderer(&viewport, &painter);
    renderer.loadSettings();
    renderer.setBackground(Qt::white);
    renderer.render();
    painter.end();

    CHECK(countPixels(picture.toImage(), qRgb(0, 0, 0)) > 300);
}

TEST_CASE("image export draws black and white entities in white on a black background", "[gui][print]") {
    (void)application();
    RS_Graphic graphic;
    graphic.initForNewDocument();
    addBlackAndWhiteLines(graphic);

    const auto file = std::filesystem::temp_directory_path() / "lc_imageexport_black.png";
    LC_ImageExporter::ExportOptions options;
    options.fileName = QString::fromStdString(file.string());
    options.format = QStringLiteral("png");
    options.size = QSize(200, 200);
    options.borders = QSize(10, 10);
    options.backgroundBlack = true;
    options.blackAndWhite = false;
    REQUIRE(LC_ImageExporter{}.exportToImage(&graphic, options));
    const QImage image(options.fileName);
    std::filesystem::remove(file);

    REQUIRE_FALSE(image.isNull());
    CHECK((image.pixel(0, 0) & 0xffffff) == 0);
    CHECK(countPixels(image, qRgb(255, 255, 255)) > 300);
}

TEST_CASE("print renderer survives a painter that could not begin", "[gui][print]") {
    (void)application();
    RS_Graphic graphic;
    graphic.initForNewDocument();
    // Narrower than a pixel at this scale: the renderer asks the paint engine
    // whether it writes SVG, which keeps such a pen.
    auto* line = new RS_Line(&graphic, RS_Vector(0, 0), RS_Vector(1000, 0));
    line->setPen(RS_Pen(RS_Color(0, 0, 0), RS2::Width07, RS2::SolidLine));
    graphic.addEntity(line);

    // As on an SVG generator whose file could not be opened.
    QPixmap nothing;
    RS_Painter painter(&nothing);
    REQUIRE_FALSE(painter.isActive());
    REQUIRE(painter.paintEngine() == nullptr);
    LC_GraphicViewport viewport;
    viewport.setSize(200, 200);
    viewport.setBorders(10, 10, 10, 10);
    viewport.setDocument(&graphic);
    viewport.loadSettings();
    viewport.zoomAuto(false);
    LC_PrintViewportRenderer renderer(&viewport, &painter);
    renderer.loadSettings();
    renderer.setBackground(Qt::white);
    renderer.render();

    CHECK_FALSE(painter.isActive());
}
